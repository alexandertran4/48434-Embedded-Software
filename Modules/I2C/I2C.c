/*! @file I2C.c
**
**  @brief I/O routines for the K64 I2C interface.
**
**  This contains the functions for operating the I2C (inter-integrated circuit) module.
** @author by Alexander Tran 12610655 and Hubert Chau 12568681
** @date 2020-05-22
*/
/*!
**  @addtogroup I2C_Module I2C module documentation
**  @{
*/
#include "I2C\I2C.h"
#include "MK64F12.h"
#include "LEDs\LEDs.h"
#include "types\types.h"
#include "fsl_clock.h"
#include "fsl_port.h"
#include "stdlib.h"
#include "core_cm4.h"
#include "OS.h"

//Constant variables
const uint8_t I2C_D_READ = 0x01;
const uint8_t I2C_D_WRITE = 0x00;

//Static global variables
static uint8_t SlaveAddress;
static uint8_t RegisterAddress;
static uint8_t NbBytes;
static uint8_t *ReceivedData;
static bool readReady = true;

//Enumerated structure for switch case in ISR
enum {
COMPLETE = 0,
LAST_BYTE = 1,
SECOND_LAST_BYTE = 2
};

/*!
 * @brief Wait for acknowledgment
 */
bool Wait_For_Ack(void)
{
	int timeOut = 100000;

	while ((I2C0->S & I2C_S_IICIF_MASK) == 0 || timeOut > 0)
	{
		timeOut--;
	}

	return true;
}
/*! @brief Sets up the I2C before first use.
 *
 *  @param moduleClk The module clock in Hz.
 *  @param aI2CModule is a structure containing the operating conditions for the module.
 *  @return BOOL - TRUE if the I2C module was successfully initialized.
 */
bool I2C_Init(const uint32_t moduleClk, const TI2CModule* const aI2CModule)
{
	I2C_ReadCompleteSemaphore = OS_SemaphoreCreate(0);

	//Arrays to store multiplier and scl divider values
	const static uint8_t mult[] = { 1, 2, 4 };
	const static uint16_t scl[] = { 20, 22, 24, 26, 28, 30, 34, 40, 28, 32, 36, 40, 44, 48,
				56, 68, 48, 56, 64, 72, 80, 88, 104, 128, 80, 96, 112, 128, 144, 160, 192,
				240, 160, 192, 224, 256, 288, 320, 384, 480, 320, 384, 448, 512, 576, 640,
				768, 960, 640, 768, 896, 1024, 1152, 1280, 1536, 1920, 1280, 1536, 1792,
				2048, 2304, 2560, 3072, 3840 };

	//enumerated variables
	enum {
	MULTI = sizeof(mult)/sizeof(uint8_t)-1,
	SCL_DIV = sizeof(scl)/sizeof(uint16_t)-1
	};

	//Variables for 'for loop' to loop over MULT and SCL arrays
	uint8_t i, j, multiplier, sclDivider;
	uint32_t baudRateError = 100000; //set baudrate as close to 100kbps
	uint32_t selectedBaudRate;

	CLOCK_EnableClock(kCLOCK_I2c0); //Enable clock gate I2C0 module
	CLOCK_EnableClock(kCLOCK_PortE); //Enable port E
	//Initialise PORT E to PIN 24 with I2C_SDA with OPEN DRAIN enabled
	PORTE->PCR[24] = PORT_PCR_MUX(0x5) | PORT_PCR_ODE_MASK;
	//Initialise PORT E to PIN 25 with I2C_SCL with OPEN DRAIN enabled
	PORTE->PCR[25] = PORT_PCR_MUX(0x5) | PORT_PCR_ODE_MASK;

	//loop to find baudrate pg 1539
	for (i = 0; i < SCL_DIV; i++)
	{
		for (j = 0; j < MULTI; j++)
		{
			if (abs(selectedBaudRate - aI2CModule->baudRate) < baudRateError) //check if the baudRate is closer to 100kbps
			{
				selectedBaudRate = moduleClk / mult[j] * scl[i]; //calculate baudRate
				baudRateError = abs(selectedBaudRate - aI2CModule->baudRate); //calculate difference between baudRates
				multiplier = j;
				sclDivider = i;
			}
		}
	}

	//set baudrate pg 1539 K64
	I2C0->F = I2C_F_ICR(0x23) | I2C_F_MULT(mult[0x00]);

	I2C0->FLT = I2C_FLT_FLT(0x00); //No glitch filtering used

	I2C0->C1 = 0;

	I2C0->S = I2C_S_IICIF_MASK; //Clear interrupts on Status register

	I2C_SelectSlaveDevice(aI2CModule->primarySlaveAddress);

	//Clears pending interrupts on I2C0 module
	NVIC_ClearPendingIRQ(I2C0_IRQn);
	//Enables interrupts on I2C0 module
	NVIC_EnableIRQ(I2C0_IRQn);

    I2C0->C1 |= I2C_C1_IICEN_MASK; //Enable I2C Register
	return true;
}

/*! @brief Selects the current slave device
 *
 * @param slaveAddress The slave device address.
 */
void I2C_SelectSlaveDevice(const uint8_t slaveAddress)
{
	SlaveAddress = slaveAddress;
}

/*! @brief Write a byte of data to a specified register
 *
 * @param registerAddress The register address.
 * @param data The 8-bit data to write.
 */
void I2C_Write(const uint8_t registerAddress, const uint8_t data)
{
	//Wait till I2C Bus is idle
	while((I2C0->S & I2C_S_BUSY_MASK) == I2C_S_BUSY_MASK)
	{

	}

	OS_SemaphoreWait(I2C_ReadCompleteSemaphore, 0);

	I2C0->C1 |= I2C_C1_MST_MASK; //Master mode selected, start signal
	I2C0->C1 |= I2C_C1_TX_MASK; //Transmit mode selected

	I2C0->D = (SlaveAddress << 1); //Send slave address with write bit

	Wait_For_Ack();

	I2C0->D = registerAddress; //Send Slave register address

	Wait_For_Ack();

	I2C0->D = data; //Send data to I2C0 data register

	Wait_For_Ack();

	I2C0->C1 &= ~(I2C_C1_MST_MASK | I2C_C1_TX_MASK); //disable TX to enable receive mode

	OS_SemaphoreSignal(I2C_ReadCompleteSemaphore);
}

/*! @brief Reads data of a specified length starting from a specified register
 *
 * Uses polling as the method of data reception.
 * @param registerAddress The register address.
 * @param data A pointer to store the bytes that are read.
 * @param nbBytes The number of bytes to read.
 */
void I2C_PollRead(const uint8_t registerAddress, uint8_t* const data, const uint8_t nbBytes)
{
	uint8_t countData;

	//Wait till I2C Bus is idle
	while((I2C0->S & I2C_S_BUSY_MASK) == I2C_S_BUSY_MASK)
	{

	}

	I2C0->C1 |= I2C_C1_MST_MASK; //Master mode selected, start signal
	I2C0->C1 |= I2C_C1_TX_MASK; //Transmit mode selected

	I2C0->D = (SlaveAddress << 1); //Send slave address with write bit

	Wait_For_Ack();

	I2C0->D = registerAddress; //Send slave register address

	Wait_For_Ack();

	I2C0->C1 |= I2C_C1_RSTA_MASK; //Repeat start
	I2C0->D = (SlaveAddress << 1) | I2C_D_READ; //Send slave address with read bit

	Wait_For_Ack();

	I2C0->C1 &= ~(I2C_C1_TX_MASK); //Selected receive mode
	I2C0->C1 &= ~(I2C_C1_TXAK_MASK); //Send ack after each message

	data[0] = I2C0->D; //Dummy read

	Wait_For_Ack();

	//For loop to add bytes to I2C0 data register
	for (countData = 0; countData < nbBytes - 1; countData++)
	{
		data[countData] = I2C0->D;
		Wait_For_Ack();
	}

	I2C0->C1 |= I2C_C1_TXAK_MASK; //Nack from mater

	data[countData++] = I2C0->D; //Read 2nd last byte

	Wait_For_Ack();

	I2C0->C1 &= ~(I2C_C1_MST_MASK | I2C_C1_TX_MASK); //disable transmit to set to receive mode

	data[countData++] = I2C0->D; //Read last byte
}

/*! @brief Reads data of a specified length starting from a specified register
 *
 * Uses interrupts as the method of data reception.
 * @param registerAddress The register address.
 * @param data A pointer to store the bytes that are read.
 * @param nbBytes The number of bytes to read.
 */
void I2C_IntRead(const uint8_t registerAddress, uint8_t* const data, const uint8_t nbBytes)
{
	uint8_t temp; //temporary variable

	I2C0->C1 |= I2C_C1_IICIE_MASK; //Enable interrupts

	if (readReady)
	{
		NbBytes = nbBytes;  //parse into global variable
		ReceivedData = data; //parse into global variable
		readReady = false;
	}

	//Wait till I2C Bus is idle
	while((I2C0->S & I2C_S_BUSY_MASK) == I2C_S_BUSY_MASK)
	{

	}

	I2C0->C1 |= I2C_C1_MST_MASK; //Master mode selected, start signal
	I2C0->C1 |= I2C_C1_TX_MASK; //Transmit mode selected

	I2C0->D = (SlaveAddress << 1); //Send slave address with write bit

	Wait_For_Ack();

	I2C0->D = registerAddress; //Send Slave register address

	Wait_For_Ack();

	I2C0->D = (SlaveAddress << 1) | I2C_D_READ;  //Send slave address with read bit

	I2C0->C1 &= ~(I2C_C1_TX_MASK); //Selected receive mode
	I2C0->C1 &= ~(I2C_C1_TXAK_MASK);  //Send ack after each message

	Wait_For_Ack();

	temp = I2C0->D; //In master transmit mode, data is written to register and transfer is initiated

	I2C0->C1 |= (I2C_C1_TX_MASK); //Set back to transmit mode

	I2C0->C1 &= ~(I2C_C1_MST_MASK | I2C_C1_TX_MASK); //disable transmit to set to receive mode
}

/*! @brief Interrupt service routine for the I2C.
 *
 *  Only used for reading data.
 *  At the end of reception, the user callback function will be called.
 *  @note Assumes the I2C module has been initialised.
 */
void I2C0_IRQHandler(void)
{
	OS_ISREnter();
	uint8_t statusRegister = I2C0->S;
	uint8_t counter = 0; //count number of interrupts that have occurred, indicates bytes read

	//Acknowledge interrupt
	I2C0->S |= I2C_S_IICIF_MASK;

	//Check transmission complete flag
	if (I2C0->S & I2C_S_IICIF_MASK)
	{	//Check if transmit type is set to received
		if(~(I2C0->C1 & I2C_C1_TX_MASK))
		{
		  switch (NbBytes)
		  {
		  	  case SECOND_LAST_BYTE:
		  	  //Send ack after each message
		  	  I2C0->C1 |= I2C_C1_TXAK_MASK;
		  	  //Decrement number of bytes to be read
		  	  NbBytes--;
		  	  //Read data and store in received data location
		  	  ReceivedData[counter] = I2C0->D;
		  	  break;

		  	  case LAST_BYTE:
		      //disable transmit to set to receive mode
		  	  I2C0->C1 &= ~(I2C_C1_MST_MASK | I2C_C1_TX_MASK);
		  	  //Decrement number of bytes to be read
		  	  NbBytes--;
		  	  //Read data and store in received data location
		  	  ReceivedData[counter] = I2C0->D;
		  	  //Increment Interrupt counter
		  	  counter++;
		  	  break;

		  	  case COMPLETE:
		  	  //Reset interrupt counter
		  	  counter = 0;
		  	  //Int_read function ready to be called again
		  	  readReady = true;
		  	  //Initiate callback function
		  	  OS_SemaphoreSignal(I2C_ReadCompleteSemaphore);
		  	  break;

		  	  default:
		  	  //Decrement number of bytes to be read
		  	  NbBytes--;
		  	  //Read data and store in received data location
		  	  ReceivedData[counter] = I2C0->D;
		  	  //Increment interrupt counter
		  	  counter++;
			  break;
		  }
		}
	}
    OS_ISRExit();
}

