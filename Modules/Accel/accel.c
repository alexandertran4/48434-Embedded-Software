/*! @file accel.c
**
**  @brief HAL for the accelerometer.
**
**  This contains the functions for interfacing to the FXOS8700CQ accelerometer.
** @author by Alexander Tran 12610655 and Hubert Chau 12568681
** @date 2020-05-22
*/
/*!
**  @addtogroup accel_Module Accel module documentation
**  @{
*/

//Accelerometer Library
#include "Accel\accel.h"
//Accelerometer registers
#include "Accel\accel_FXO.h"
//I2C
#include "I2C\I2C.h"
//Median
#include "Median\median.h"
#include "MK64F12.h"
#include "Types\types.h"
#include "fsl_gpio.h"
#include "fsl_clock.h"
#include "fsl_port.h"
#include "Critical\critical.h"
#include "core_cm4.h"
#include "OS.h"

static AccelSetup_t AccelModuleSetup;
static AccelMode_t CurrentAccelMode;
static TI2CModule I2C;

static void (*DataReadyCallbackFunction)(void *);

/*!
 * @brief Argument to pass to data callback.
 */
static void *DataReadyCallbackArguments;
/*!

/*! @brief Initializes the accelerometer by calling the initialization routines of the supporting software modules.
 *
 *  @param moduleClk is the module clock rate in Hz.
 *  @param accelSetup is a pointer to an accelerometer setup structure.
 *  @return bool - true if the accelerometer module was successfully initialized.
 */
bool Accel_Init(const uint32_t moduleClk, const AccelSetup_t* const accelSetup)
{
	Accel_ReadCompleteSemaphore = OS_SemaphoreCreate(0);

	AccelModuleSetup = *accelSetup;

	//I2C Initialization for accelerometer
	I2C.primarySlaveAddress = ACCEL_ADDRESS;
	I2C.baudRate = ACCEL_BAUD_RATE;

	DataReadyCallbackFunction = accelSetup->dataReadyCallbackFunction;
	DataReadyCallbackArguments = accelSetup->dataReadyCallbackArguments;

	I2C_Init(moduleClk, &I2C); //Initialise I2C bus

	CTRL_REG1_ACTIVE = 0; //Enable Standby mode
	I2C_Write(ADDRESS_CTRL_REG1, CTRL_REG1); //Write to register

	CTRL_REG1_DR = DATE_RATE_6_25_HZ; //Set frequency to 6.25HZ
	CTRL_REG1_F_READ = 1; //8 bit precision
	CTRL_REG1_LNOISE = 0; //Set to full dynamic range mode
	CTRL_REG1_ASLP_RATE = SLEEP_MODE_RATE_6_25_HZ; //SLEEP_MODE_RATE_6_25_HZ;
	I2C_Write(ADDRESS_CTRL_REG1, CTRL_REG1); //Write to the register

	CTRL_REG3_PP_OD = 0; //Set as push-pull configuration
	CTRL_REG3_IPOL = 1; //Set as active High Logic Polarity
	I2C_Write(ADDRESS_CTRL_REG3, CTRL_REG3); //Write to the register

	CTRL_REG4_INT_EN_DRDY = 0; //Data ready interrupt disabled
	CTRL_REG4_INT_EN_LNDPRT	= 1;//Enable Orientation Interrupt
	CTRL_REG4_INT_EN_PULSE = 1; //Enable Pulse interrupt
	CTRL_REG4_INT_EN_FF_MT = 1; //Freefall motion interrupt enabled
	I2C_Write(ADDRESS_CTRL_REG4, CTRL_REG4); //Write to the register

	CTRL_REG5_INT_CFG_DRDY = 0; //Interrupt routed to INT2
	CTRL_REG5_INT_CFG_LNDPRT = 0;
	CTRL_REG5_INT_CFG_PULSE = 0;
	CTRL_REG5_INT_CFG_FF_MT	= 0; //Freefall/Motion detection routed to Pin 2
	I2C_Write(ADDRESS_CTRL_REG5, CTRL_REG5); //Write to the register

	CTRL_REG1_ACTIVE = 1; //Activate active (high) mode for accelerometer activation
	I2C_Write(ADDRESS_CTRL_REG1, CTRL_REG1); //Write to the register

	//Enable Port C
	CLOCK_EnableClock(kCLOCK_PortC);
	//Clear any bits set in Port C Mux
	PORTC->PCR[13] &= ~PORT_PCR_MUX_MASK;
	//Mux Port C as GPIO
	PORTC->PCR[13] |= PORT_PCR_MUX(1);

    // Clears pending interrupts on PORT C
	NVIC_ClearPendingIRQ(PORTC_IRQn);
	// Enables interrupts on PORT C
	NVIC_EnableIRQ(PORTC_IRQn);

	return true;
}

AccelMode_t Get_AccelMode()
{
	return CurrentAccelMode;
}

/*! @brief Reads X, Y and Z accelerations.
 *  @param data is an array of 3 bytes where the X, Y and Z data are stored.
 */
void Accel_ReadXYZ(uint8_t data[3])
{
	if (CurrentAccelMode == ACCEL_MODE_POLL)
	{
		I2C_PollRead(ADDRESS_OUT_X_MSB, data, 3);
	}

	else if (CurrentAccelMode == ACCEL_MODE_INT)
	{
		I2C_IntRead(ADDRESS_OUT_X_MSB, data, 3);
	}
}

/*! @brief Set the mode of the accelerometer.
 *  @param mode specifies either polled or interrupt driven operation.
 */
void Accel_SetMode(const AccelMode_t mode)
{
	CurrentAccelMode = mode;

	if (mode == ACCEL_MODE_POLL)
	{
		PORTC->PCR[13] &= ~PORT_PCR_IRQC_MASK; //No interrupt flag set

		CTRL_REG4_INT_EN_DRDY = 0; //Data ready interrupt disabled
		I2C_Write(ADDRESS_CTRL_REG4, CTRL_REG4); //Write to register
	}
	else if (mode == ACCEL_MODE_INT)
	{
		//Set interrupt flag as falling edge
		PORTC->PCR[13] |= PORT_PCR_IRQC(0b1010);

		CTRL_REG4_INT_EN_DRDY = 1; //Data ready interrupt enabled
		I2C_Write(ADDRESS_CTRL_REG4, CTRL_REG4); //Write to register
	}

}

void PORTC_IRQHandler(void)
{
	OS_ISREnter();

	//Check for pending interrupts on PORT C Pin 13
	if (PORTC->PCR[13] & PORT_PCR_ISF_MASK)
	{
		//Clear interrupt flags on Port C Pin 13
		PORTC->PCR[13] |= PORT_PCR_ISF_MASK;
		OS_SemaphoreSignal(Accel_ReadCompleteSemaphore);
	}

	OS_ISRExit();
}
