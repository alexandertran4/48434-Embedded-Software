/*!
** @file
** @version 6.0
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
//Critical Sections
#include "Critical\critical.h"
//I2C
//Median
#include "Median\median.h"
//Accelerometer
#include "Packet\packet.h"
#include "FG.h"
#include "VRR\VRR.h"
#include "ADC\ADC.h"
#include "Library\FG.h"
// Simple OS
#include "OS.h"

#define DEBUG_HALT __asm( "BKPT 255")

// Arbitrary thread stack size - big enough for stacking of interrupts and OS use.
#define THREAD_STACK_SIZE 100
#define NB_LEDS 3
static TFTMChannel UART_TIMER;
const uint32_t BAUD_RATE = 115200;
// Thread stacks
OS_THREAD_STACK(InitModulesThreadStack, THREAD_STACK_SIZE); /*!< The stack for the LED Init thread. */
OS_THREAD_STACK(PacketGetModulesThreadStack, THREAD_STACK_SIZE); /*!< The stack for the PACKET GET thread. */
/*!< The stack for the FTM thread. */
/*!< The stack for the PIT thread. */
OS_THREAD_STACK(ReceiveThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(TransmitThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(FTMCallbackThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(PIT0CallbackThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(PIT2CallbackThreadStack, THREAD_STACK_SIZE);
// Commands
// TODO: Define the commands using enum.
enum Command {
	CMD_STARTUP = 0x04,
	CMD_MCU_SPECIAL = 0x09,
	CMD_MCU_NUMBER = 0x0B,
	CMD_MCU_MODE = 0x0D,
	CMD_MCU_READ = 0x08,
	CMD_MCU_FLASH = 0x07,
	CMD_TIMING_MODE = 0x10,
	CMD_NUMBER_OF_RAISES = 0x11,
	CMD_NUMBER_OF_LOWERS = 0x12,
	CMD_FREQUENCY = 0x17,
	CMD_VOLTAGE_RMS = 0x18,
	CMD_ANALOG = 0x50
};

enum Priority {
	FG_OUT_PRIORITY = 5,
	FG_PACKET_PRIORITY = 6,
	RX_PRIORITY = 1,
	TX_PRIORITY = 2
};

const FGSetup_t FG_SETUP =
{
	/*!< A pointer to the global Packet structure. */
	.pPacket = &Packet,
	/*!< The priority of the FG output value preparation thread.*/
	.fgOutPriority = FG_OUT_PRIORITY,
	/*!< The priority of the FG packet handling thread. */
	.fgPacketPriority = FG_PACKET_PRIORITY
};

const UARTSetup_t UART_SETUP =
{
	/*!< A pointer to the global Packet structure. */
	.baudRate = BAUD_RATE,
	.rxPriority = RX_PRIORITY,
	.txPriority = TX_PRIORITY
};

// Private global variables
// TODO: Define the MCU number.
const uint8_t MCU_MODE = 0x01;
const uint8_t PACKET_ACK_MASK = 0x80;
const uint16_t STUDENT_NUMBER = 0x655;
// Version number
// TODO: Define the major version number and minor version number using const.
//Last 4 digits of student number converted to binary form
static uint8_t MCU_NUM_L = 0b10001111;
static uint8_t MCU_NUM_H = 0b00000010;
//Latest bytes of accelerometer data, X, Y and Z

/*!
 * @brief Runs Packet Get thread
 */
void PacketGetThread(void *pData)
{
	for (;;)
	{
		Packet_Get();
		HandlePackets();
	}
}

void FTMCallbackThread(void *pData)
{
  for (;;)
  {
    //Wait on the FTM0 Semaphore
    OS_SemaphoreWait(FTMSemaphore, 0);
    LEDs_Off(LED_BLUE);
  }
}

void PITCallback(void *arg)
{
	LEDs_Toggle(LED_GREEN);
}
/*!
 * @brief Runs FTM thread
 */
/*! @brief Respond to a Startup packet sent from the PC.
 *
 *  @return bool - TRUE if the packet was handled successfully.
 *  @note Assumes that FreedomInit has been called successfully.
 */
bool HandlePacketVersion()
{
	Packet_Put(CMD_MCU_SPECIAL, 'v', 0x06, 0);
}

/*! @brief Sets or reads the MCU number.
 *			MCU number will be obtained from the flash memory.
 *  @return bool - TRUE if successfully sent by function FALSE if packet_put FAILS
 */
bool HandlePacketMCUNumber()
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
		Packet_Put(CMD_MCU_NUMBER, _FB(FLASH_DATA_START), _FB(0x00080001LU), 0);
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
bool HandlePacketMCUMode()
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
		Packet_Put(CMD_MCU_MODE, 1, _FB(0x00080002LU), _FB(0x00080003LU));
	}
	else
	{
		return false;
	}
}

/*! @brief  Retrieves flash memory store and sends data to the pc

 *  @return bool - TRUE if successfully sent by function FALSE if packet_put FAILS
 */
bool HandlePacketFlash()
{
	if (Packet_Parameter1 >= 0 && Packet_Parameter1 <= 7 && Packet_Parameter2 == 0) // Parameter 1 has to be between 0 to 7, 0x08 is used for erase (FLASH)
	{
		Packet_Put(CMD_MCU_READ, Packet_Parameter1, 0, _FB(FLASH_DATA_START + Packet_Parameter1));
	}
	else
	{
		return false;
	}
}

/*! @brief  Sends the Version command to the pc

 *  @return bool - TRUE if successfully written by Flash_Write8
 */
bool HandlePacketFlashProgram()
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
bool HandlePacketSpecial()
{
	if (Packet_Parameter1 == 'v' && Packet_Parameter2 == 'x' && Packet_Parameter3 == '\r')
	{
		Packet_Put(CMD_MCU_SPECIAL, 'v', 0x01, 0);
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
bool SendStartupPackets()
{
	Packet_Parameter1 = 1;
	// TODO: Send startup packets to the PC.
	Packet_Put(CMD_STARTUP, 0, 0, 0);
	HandlePacketVersion();
	HandlePacketMCUNumber();
	HandlePacketMCUMode();
}

/*! @brief Calls SendStartupPacket()
 *
 *  @return bool - TRUE if successfully send, FALSE if packet_put fails
 */
bool HandleStartupPacket(void)
{
  // TODO: Respond to a startup packet sent from the PC
	SendStartupPackets();
}

/*bool HandleTimingMode(void)
{
	if (Packet_Parameter1 == 0)
	{
		Packet_Put(CMD_TIMING_MODE, Timing_Mode, 0, 0);
	}
	else if (Packet_Parameter1 == 1)
	{
		Timing_Mode = Packet_Parameter1;
	}
}

bool HandleRaiseMode(void)
{
	if (Packet_Parameter1 == 0)
	{
		Packet_Put(CMD_NUMBER_OF_RAISES, Nb_Raise, 0, 0);
	}
	else if (Packet_Parameter1 == 1)
	{
		Nb_Raise = 0;
	}
}

bool HandleLowerMode(void)
{
  if (Packet_Parameter1 == 0) // Get number of raises
  {
     Packet_Put(CMD_NUMBER_OF_LOWERS, Nb_Lowers, 0, 0);
  }
  else if (Packet_Parameter1 == 1)
  {
     Nb_Lowers = 0;
  }
}

bool HandleRMSValues(void)
{
    Packet_Parameter1 = 1;

	uint16_t tempVRMS = (uint16_t) ((samples[0].vrmsValue / 3276.7) * 100);
	uint16_t lowVRMS = tempVRMS%256;
	uint16_t highVRMS = tempVRMS/256;
	uint8_t highVRMS8 = (uint8_t) highVRMS;
	uint8_t lowVRMS8 = (uint8_t) lowVRMS;
	Packet_Put(CMD_VOLTAGE_RMS, Packet_Parameter1, lowVRMS8, highVRMS8);
}*/

/*! @brief Sends Protocol Packet from PC TO MCU and sets accelerometer mode
*
*  @return bool - TRUE if sending the protocol packets was successful.
*/
/*! @brief Handle Packet Sending from PC TO MCU
*
*  @return bool - TRUE if sending the startup packets was successful.
*/
void HandlePackets()
{
	uint8_t success = 0;

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

/*		case CMD_TIMING_MODE:
		success = HandleTimingMode();
		break;

		case CMD_NUMBER_OF_RAISES:
		success = HandleRaiseMode();
		break;

		case CMD_NUMBER_OF_LOWERS:
		success = HandleLowerMode();
		break;*/

		default:
		(void)OS_SemaphoreSignal(FG_HandlePacketSemaphore);
		(void)OS_SemaphoreWait(FG_PacketProcessed, 0);
		success = FG_PacketHandledSuccessfully;
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

/*! @brief Initializes the MCU by initializing all variables and then sending startup packets to the PC.
*
*  @return bool - TRUE if sending the startup packets was successful.
*/
static bool MCUInit(void)
{
  OS_DisableInterrupts();

  BOARD_InitPins();
  BOARD_InitBootClocks();

  static const uint32_t TimerPeriod = 500000000;
  bool success;

  if (LEDs_Init() && Packet_Init(SystemCoreClock, &UART_SETUP) && Flash_Init() && FTM_Init() && PIT_Init(CLOCK_GetFreq(kCLOCK_BusClk), &PITCallback, NULL)) //Initialisation of packet
  {
	 LEDs_On(LED_GREEN); //Call LED function to turn on Green LED
	 PIT_Set(TimerPeriod, true); //Set PIT Timer period*/
  }

  success = FG_Init(SystemCoreClock, &FG_SETUP);
  if (!success)
  return false;

  OS_EnableInterrupts();

  //Set FTM Timer Properties
  UART_TIMER.channelNb = 0;
  UART_TIMER.delayNanoseconds = 24414;
  UART_TIMER.timerFunction = TIMER_FUNCTION_OUTPUT_COMPARE;
  UART_TIMER.ioType.outputAction = TIMER_OUTPUT_LOW;

  FTM_Set(&UART_TIMER);

  Packet_Parameter1 = 2;
  SendStartupPackets();

  // TODO: Initialize any modules that need to be initialized.
}

static void InitModulesThread(void *pData)
{
	 MCUInit();

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

  // Initialize the RTOS
  OS_Init(SystemCoreClock);

  // Create module initialisation thread

  error = OS_ThreadCreate(InitModulesThread, NULL, &InitModulesThreadStack[THREAD_STACK_SIZE-1], 0);
  error = OS_ThreadCreate(ReceiveThread, NULL, &ReceiveThreadStack[THREAD_STACK_SIZE-1], 1);
  error = OS_ThreadCreate(TransmitThread, NULL, &TransmitThreadStack[THREAD_STACK_SIZE-1], 2);
  /*error = OS_ThreadCreate(FTMCallbackThread, NULL, &FTMCallbackThreadStack[THREAD_STACK_SIZE-1], 5);*/
  error = OS_ThreadCreate(PacketGetThread, NULL, &PacketGetModulesThreadStack[THREAD_STACK_SIZE-1], 4);
  error = OS_ThreadCreate(PIT0CallbackThread, NULL, &PIT0CallbackThreadStack[THREAD_STACK_SIZE-1], 3);
 /*error = OS_ThreadCreate(PIT2CallbackThread, NULL, &PIT2CallbackThreadStack[THREAD_STACK_SIZE-1], 4);*/
  // Start multithreading - never returns!
  OS_Start();
  // If the program returns from OS_Start, we have a major problem!
  DEBUG_HALT;
  return 1;
}


/*!
** @}
*/
