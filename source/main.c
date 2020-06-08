/*!
** @file
** @version 5.0
** @brief  Main module.
**
**   This file contains the high-level code for the project.
**   It initialises appropriate hardware subsystems,
**   creates application threads, and then starts the OS.
**
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */

// Provided drivers
#include "clock_config.h"
#include "pin_mux.h"
#include "core_cm4.h"
#include "MK64F12.h"
#include "fsl_common.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "system_MK64F12.h"
// New types
#include "Types\types.h"
// FIFO Buffer
#include "FIFO\FIFO.h"
// Serial Communication Interface
#include "UART\UART.h"
// Flash Memory
#include "Flash\Flash.h"
// LEDs
#include "LEDs\LEDs.h"
//FTM
#include "FTM\FTM.h"
//PIT
#include "PIT\PIT.h"
//RTC
#include "RTC\RTC.h"
//Critical Sections
#include "Critical\critical.h"
//I2C
#include "I2C\I2C.h"
//Median
#include "Median\median.h"
//Accelerometer
#include "Accel\accel.h"
#include "Accel\accel_FXO.h"
#include "Packet\packet.h"

// Simple OS
#include "OS.h"

#define DEBUG_HALT __asm( "BKPT 255")

// Arbitrary thread stack size - big enough for stacking of interrupts and OS use.
#define THREAD_STACK_SIZE 100
#define NB_LEDS 3
static TFTMChannel UART_TIMER;
static AccelSetup_t ACCEL_MODULE_SETUP;
static UARTSetup_t UART_SETUP;
OS_ECB *UARTTimerSemaphore[8];

// Thread stacks
OS_THREAD_STACK(InitModulesThreadStack, THREAD_STACK_SIZE); /*!< The stack for the LED Init thread. */
OS_THREAD_STACK(ReceiveThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(TransmitThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(PacketGetModulesThreadStack, THREAD_STACK_SIZE); /*!< The stack for the PACKET GET thread. */
OS_THREAD_STACK(UARTTimerThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(PITCallbackThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(RTCUpdateThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(I2CThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(AccelThreadStack, THREAD_STACK_SIZE);

// Commands
// TODO: Define the commands using enum.
enum {
	CMD_STARTUP = 0x04,
	CMD_MCU_SPECIAL = 0x09,
	CMD_MCU_NUMBER = 0x0B,
	CMD_MCU_MODE = 0x0D,
	CMD_MCU_READ = 0x08,
	CMD_MCU_FLASH = 0x07,
	CMD_MCU_TIME = 0x0C,
	CMD_MCU_PROTOCOL = 0x0A
};

// Private global variables
// TODO: Define the MCU number.
const uint8_t MCU_MODE = 0x01;
const uint8_t PACKET_ACK_MASK = 0x80;
const uint16_t STUDENT_NUMBER = 0x8681;
// Version number
// TODO: Define the major version number and minor version number using const.
//Last 4 digits of student number converted to binary form
static uint8_t MCU_NUM_L = 0b11101001;
static uint8_t MCU_NUM_H = 0b00100001;
//Latest bytes of accelerometer data, X, Y and Z
static uint8_t AccLatestByte[3] = {0}, AccXLatest[3] = {0}, AccYLatest[3] = {0}, AccZLatest[3] = {0}, AccLatestData[3] = {0};

static void ReceiveThread(void *pData)
{
  for(;;)
  {
    // Wait for the receive UART semaphore to start receive data
    OS_SemaphoreWait(RxSemaphore, 0);

    FIFO_Put(&RxFIFO, UART0->D); // Retrieve the data from UART0->D and stores it to RxFIFO

    UART0->C2 |= UART_C2_RIE_MASK; // Enable the RIE (receive interrupt)
  }
}

void TransmitThread(void *pData)
{
  uint8_t txData;

  for(;;)
  {
    // Wait for the transmit UART semaphore to do the transmission
    OS_SemaphoreWait(TxSemaphore, 0);

    FIFO_Get(&TxFIFO, (uint8_t *) &txData); // Transmit the data
    UART0->D = txData;

    // Enable the UART interrupt if the FIFO buffer is not empty, disable is done before every transmit in UART2_ISR
    UART0->C2 |= UART_C2_TIE_MASK;
  }
}

static void PacketGetThread(void *pData)
{
	for (;;)
	{
		if (Packet_Get())
		{
			HandlePackets();
		}
	}
}

void UARTTimerCallbackThread(void *arg)
{
	for (;;)
	{
		OS_SemaphoreWait(UARTTimerSemaphore[0], 0);
		LEDs_Off(LED_BLUE);
	}
}

void PITCallbackThread(void *arg)
{
     for (;;)
     {
    	 OS_SemaphoreWait(PITSemaphore, 0);
    	 if (Get_AccelMode() == ACCEL_MODE_POLL)
    	 {
    		 LEDs_Toggle(LED_GREEN);
    		 Accel_ReadXYZ(AccLatestData);
    		 Packet_Put(0x10, AccLatestData[0], AccLatestData[1], AccLatestData[2]);
    	 }
     }
}

void I2CThread(void *pData)
{
	 for (;;)
	 {
		 //Wait on the I2C Semaphore
		 OS_SemaphoreWait(I2C_ReadCompleteSemaphore, 0);
		 MedianData();
     }
}

void AccelThread(void *pData)
{
  for (;;)
  {
    //Wait on Accel Semaphore
    OS_SemaphoreWait(Accel_DataReadySemaphore, 0);
    Accel_ReadXYZ(AccLatestData);
    LEDs_Toggle(LED_GREEN);
    Packet_Put(0x10, AccLatestData[0], AccLatestData[1], AccLatestData[2]);
    OS_SemaphoreSignal(Accel_DataReadySemaphore);
  }
}

/*!
 * @brief Shifting window shifts the elements of an array one to the left.
 * @param arr Array to shifting window.
 * @param arrLen Length of the array.
 * @param value New value to insert at index 0.
 */
void ShiftingWindow(uint8_t* const arr, const size_t arrLen, const uint8_t value)
{
	for (size_t i = (arrLen - 1); i > 0; i --)
	{
		arr[i] = arr[i-1];
	}

	arr[0] = value;
}

/*!
 * @brief Handle new accelerometer data, running on main thread
 */
void MedianData()
{
	if (Get_AccelMode() == ACCEL_MODE_INT)
	{
		Accel_ReadXYZ(AccLatestData);
		Packet_Put(0x10, AccLatestData[0], AccLatestData[1],AccLatestData[2]);
	}

	//Keeping track of accelerometer movements on X, Y and Z axis
	ShiftingWindow(AccXLatest, 3, AccLatestData[0]);
	ShiftingWindow(AccYLatest, 3, AccLatestData[1]);
	ShiftingWindow(AccZLatest, 3, AccLatestData[2]);

	uint8_t medianX = Median_Filter3(AccXLatest[0], AccXLatest[1], AccXLatest[2]); //X Axis after passing through median filter
	uint8_t medianY = Median_Filter3(AccYLatest[0], AccYLatest[1], AccYLatest[2]); //Y Axis after passing through median filter
	uint8_t medianZ = Median_Filter3(AccZLatest[0], AccZLatest[1], AccZLatest[2]); //Z Axis after passing through median filter

	if ((medianX != AccLatestByte[0]) | (medianY != AccLatestByte[1]) | (medianZ != AccLatestByte[2]))
	{
		AccLatestByte[0] = medianX;
		AccLatestByte[1] = medianY;
		AccLatestByte[2] = medianZ;

		Packet_Put(0x10, medianX, medianY, medianZ);
	}
}

/*! @brief Respond to a Startup packet sent from the PC.
 *
 *  @return bool - TRUE if the packet was handled successfully.
 *  @note Assumes that FreedomInit has been called successfully.
 */
static void HandlePacketVersion()
{
	return Packet_Put(CMD_MCU_SPECIAL, 'v', 0x04, 0);
}

/*! @brief Sets or reads the MCU number.
 *			MCU number will be obtained from the flash memory.
 *  @return bool - TRUE if successfully sent by function FALSE if packet_put FAILS
 */
static void HandlePacketMCUNumber()
{
	Packet_Put(CMD_MCU_NUMBER, 0x01, MCU_NUM_L, MCU_NUM_H);

	if (Packet_Parameter1 == 2) // To allocate the MCU mode
	{
		uint16union_t MCUNumberToWrite;
		MCUNumberToWrite.s.Lo = Packet_Parameter2;
		MCUNumberToWrite.s.Hi = Packet_Parameter3;
		return Flash_Write16((uint16_t*)(FLASH_DATA_START + 2), MCUNumberToWrite.l);
	}
	else if (Packet_Parameter1 == 1) // To retrieve the MCU mode
	{
		return Packet_Put(CMD_MCU_NUMBER, _FB(FLASH_DATA_START), _FB(0x00080001LU), 0);
	}
	else
	{
		return false;
	}
}

/*! @brief Sets or reads the MCU mode
 *			MCU mode will be obtained from the flash memory.
 *  @return bool - TRUE if successfully if sent by function FALSE if packet_put FAILS
 */
static void HandlePacketMCUMode()
{
	if (Packet_Parameter1 == 2) // To allocate the MCU mode
	{
		uint16union_t MCUModeToWrite;
		MCUModeToWrite.s.Lo = Packet_Parameter2;
		MCUModeToWrite.s.Hi = Packet_Parameter3;
		return Flash_Write16((uint16_t*)(FLASH_DATA_START + 2), MCUModeToWrite.l);
	}
	else if (Packet_Parameter1 == 1) // To retrieve the MCU mode
	{
		return Packet_Put(CMD_MCU_MODE, 1, _FB(0x00080002LU), _FB(0x00080003LU));
	}
	else
	{
		return false;
	}
}

/*! @brief  Retrieves flash memory store and sends data to the pc

 *  @return bool - TRUE if successfully sent by function FALSE if packet_put FAILS
 */
static void HandlePacketFlash()
{
	if (Packet_Parameter1 >= 0 && Packet_Parameter1 <= 7 && Packet_Parameter2 == 0) // Parameter 1 has to be between 0 to 7, 0x08 is used for erase (FLASH)
	{
		return Packet_Put(CMD_MCU_READ, Packet_Parameter1, 0, _FB(FLASH_DATA_START + Packet_Parameter1));
	}
	else
	{
		return false;
	}
}

/*! @brief  Sends the Version command to the pc

 *  @return bool - TRUE if successfully written by Flash_Write8
 */
static bool HandlePacketFlashProgram()
{
	if (Packet_Parameter1 >= 0 && Packet_Parameter1 <= 7 && Packet_Parameter2 == 0) // Parameter 1 has to be between 0 to 7, 0x08 is used for erase (FLASH)
	{
		return Flash_Write8((uint8_t*)(FLASH_DATA_START + Packet_Parameter1), Packet_Parameter3);
	}
	else
	{
		return false;
	}
}

/*! @brief  To send packets to retrieve the packet version

 *  @return bool - TRUE if successfully sent by function FALSE if packet_put FAILS
 */
static void HandlePacketSpecial()
{
	if (Packet_Parameter1 == 'v' && Packet_Parameter2 == 'x' && Packet_Parameter3 == '\r')
		{
			return Packet_Put(CMD_MCU_SPECIAL, 'v', 0x01, 0);
		}
		else
		{
			return false;
		}
}


/*! @brief Sends the Initial 4 commands to the PC upon startup on the MCU, or upon the calling of that command
 *
 *  @return bool - TRUE if successfully send, FALSE if packet_put fails
 */
static void SendStartupPackets(void)
{
	Packet_Parameter1 = 1;
	// TODO: Send startup packets to the PC.
	return Packet_Put(CMD_STARTUP, 0, 0, 0) && HandlePacketVersion() && HandlePacketMCUNumber() && HandlePacketMCUMode();
}

/*! @brief Calls SendStartupPacket()
 *
 *  @return bool - TRUE if successfully send, FALSE if packet_put fails
 */
static void HandleStartupPacket(void)
{
  // TODO: Respond to a startup packet sent from the PC
	SendStartupPackets();
}

/*! @brief Verifies Packet Parameters meet time requiremets for hours, minutes and seconds
 *  If it meets requirement set RTC and send Packet
 *
 *  @return bool - TRUE if successfully send, FALSE if packet_put fails
 */
static void HandlePacketMCUTime(void)
{
	if (Packet_Parameter1 >= 0 && Packet_Parameter1 <= 23
		&& Packet_Parameter2 >= 0 && Packet_Parameter2 <= 59
		&& Packet_Parameter3 >= 0 && Packet_Parameter3 <= 59)
		{
			RTC_Set(Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
			return Packet_Put(CMD_MCU_TIME, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
		}
	else
	{
		return false;
	}
}

/*! @brief Sends Protocol Packet from PC TO MCU and sets accelerometer mode
*
*  @return bool - TRUE if sending the protocol packets was successful.
*/
static void HandleProtocolMCUMode()
{
	if (Packet_Parameter1 == 2)
	{
		if (Packet_Parameter2 == 0)
		{
			Accel_SetMode(ACCEL_MODE_POLL); //Set accelerometer to polling mode
		}
		else if (Packet_Parameter2 == 1)
		{
			Accel_SetMode(ACCEL_MODE_INT); //Set accelerometer to interrupt mode
		}
	}

	else if (Packet_Parameter1 == 1)
	{
		if (Get_AccelMode() == ACCEL_MODE_INT)
		{
			return Packet_Put(CMD_MCU_PROTOCOL, 0x0, 1, 0x0);
		}
		else if (Get_AccelMode() == ACCEL_MODE_POLL)
		{
			return Packet_Put(CMD_MCU_PROTOCOL, 0x0, 0, 0x0);
		}
	}
}

/*! @brief Handle Packet Sending from PC TO MCU
*
*  @return bool - TRUE if sending the startup packets was successful.
*/
void HandlePackets(void)
{
	bool success = false;

	Packet_Get();
	FTM_StartTimer(&UART_TIMER);
	LEDs_On(LED_BLUE);

	switch (Packet_Command & ~PACKET_ACK_MASK) 	//check command and send correct packet
	{
	    case CMD_STARTUP:						//send packet 0x04
		success = HandleStartupPacket();
		break;

		case CMD_MCU_SPECIAL:					//send packet 0x09
		success = HandlePacketSpecial();
		break;

		case CMD_MCU_NUMBER:					//send packet 0x0B
		success = HandlePacketMCUNumber();
		break;

		case CMD_MCU_MODE:						//send packet 0x0D
		success = HandlePacketMCUMode();
		break;

		case CMD_MCU_READ:						//send packet 0x08
		success = HandlePacketFlash();
		break;

		case CMD_MCU_FLASH: 					//send packet 0x07
		success = HandlePacketFlashProgram();
		break;

		case CMD_MCU_TIME:						//send packet 0x0C
		success = HandlePacketMCUTime();
		break;

		case CMD_MCU_PROTOCOL:					//send packet 0x0A
		success = HandleProtocolMCUMode();
		break;
    }

    if (Packet_Command & PACKET_ACK_MASK) //Acknowledgement ensures correct packet is sent and received
    {
    	if (success)
		  Packet_Put(Packet_Command, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
    	else
    	  Packet_Put(Packet_Command & ~PACKET_ACK_MASK, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
    }
}

static void RTCUpdateThread(void *arg)
{
	for (;;)
	{
		OS_SemaphoreWait(RTC_Semaphore, 0);

		//local time variables
		uint8_t hours, minutes, seconds;

		//Get the times
		RTC_Get(&hours, &minutes, &seconds);

		//send to PC
		Packet_Put(CMD_MCU_TIME, hours, minutes, seconds);

		//toggle red led
		LEDs_Toggle(LED_RED);
	}
}

/*! @brief Initializes the MCU by initializing all variables and then sending startup packets to the PC.
*
*  @return bool - TRUE if sending the startup packets was successful.
*/
static bool FreedomInit(void)
{
  OS_DisableInterrupts();

  BOARD_InitPins();
  BOARD_InitBootClocks();

  static const uint32_t TimerPeriod = 500000000;
  UART_SETUP.baudRate = 115200;

  if (LEDs_Init() && Packet_Init(SystemCoreClock, &UART_SETUP) && Flash_Init() && RTC_Init() && PIT_Init(CLOCK_GetFreq(kCLOCK_BusClk), PITCallbackThread, NULL) && FTM_Init() && Accel_Init(CLOCK_GetFreq(kCLOCK_BusClk), &ACCEL_MODULE_SETUP)) //Initialisation of packet
  {
	 LEDs_On(LED_GREEN); //Call LED function to turn on Green LED
	 PIT_Set(TimerPeriod, true); //Set PIT Timer period
  }

  OS_EnableInterrupts();

  //Set Accelerometer Properties
  ACCEL_MODULE_SETUP.dataReadyCallbackFunction = &MedianData;
  ACCEL_MODULE_SETUP.dataReadyCallbackArguments = 0;
 /* ACCEL_MODULE_SETUP.readCompleteCallbackFunction = &MedianData;
  ACCEL_MODULE_SETUP.readCompleteCallbackArguments = 0;*/

  //Set FTM Timer Properties
  UART_TIMER.channelNb = 0;
  UART_TIMER.delayNanoseconds = 24414;
  UART_TIMER.timerFunction = TIMER_FUNCTION_OUTPUT_COMPARE;
  UART_TIMER.ioType.outputAction = TIMER_OUTPUT_LOW;
  /*UART_TIMER.callbackFunction = &UARTTimerCallback;*/
  UART_TIMER.callbackArguments = NULL;

  FTM_Set(&UART_TIMER);

  Packet_Parameter1 = 2;
  return SendStartupPackets();

  // TODO: Initialize any modules that need to be initialized.
}

static void InitModulesThread(void *pData)
{
	 FreedomInit();

	 volatile uint16union_t *NvMCUNb; //Allocate space in flash for MCU number
	 volatile uint16union_t *NvModeNb; //Allocate space for flash in MCU Mode
	 static bool success;

	 success = Flash_AllocateVar((volatile void **)&NvMCUNb, sizeof(*NvMCUNb));

	 if (success && (NvMCUNb->l == 0xffff))
	 {
	    Flash_Write16((volatile uint16_t*)NvMCUNb, STUDENT_NUMBER);
	 }

	 success = Flash_AllocateVar((volatile void **)&NvModeNb, sizeof(*NvModeNb));
	 if (success && (NvModeNb->l == 0xffff))
	 {
	     Flash_Write16((volatile uint16_t*)NvModeNb, MCU_MODE);
	 }

	 OS_ThreadDelete(OS_PRIORITY_SELF);
}
/*! @brief Initialises the hardware, sets up threads, and starts the OS.
 *
 */
int main(void)
{
  OS_ERROR error;
  UART_SETUP.rxPriority = 1;
  UART_SETUP.txPriority = 2;

  // Initialize the RTOS
  OS_Init(SystemCoreClock);

  // Create module initialisation thread

  error = OS_ThreadCreate(InitModulesThread, NULL, &InitModulesThreadStack[THREAD_STACK_SIZE-1], 0);
  error = OS_ThreadCreate(ReceiveThread, NULL, &ReceiveThreadStack[THREAD_STACK_SIZE-1], UART_SETUP.rxPriority);
  error = OS_ThreadCreate(TransmitThread, NULL, &TransmitThreadStack[THREAD_STACK_SIZE-1], UART_SETUP.txPriority);
  error = OS_ThreadCreate(PITCallbackThread, NULL, &PITCallbackThreadStack[THREAD_STACK_SIZE-1], 3);
  error = OS_ThreadCreate(RTCUpdateThread, NULL, &RTCUpdateThreadStack[THREAD_STACK_SIZE-1], 4);
  error = OS_ThreadCreate(UARTTimerCallbackThread, NULL, &UARTTimerThreadStack[THREAD_STACK_SIZE-1], 5);
  error = OS_ThreadCreate(PacketGetThread, NULL, &PacketGetModulesThreadStack[THREAD_STACK_SIZE-1], 6);
  error = OS_ThreadCreate(AccelThread, NULL, &AccelThreadStack[THREAD_STACK_SIZE-1], 7);
  error = OS_ThreadCreate(I2CThread, NULL, &I2CThreadStack[THREAD_STACK_SIZE-1], 8);

  // Start multithreading - never returns!
  OS_Start();
  // If the program returns from OS_Start, we have a major problem!
  DEBUG_HALT;
  return 1;
}


/*!
** @}
*/
