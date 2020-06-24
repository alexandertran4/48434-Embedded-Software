/*!
** @file PIT.c
** @brief Routines for controlling Periodic Interrupt Timer (PIT).
**
** This contains the functions for operating the periodic interrupt timer (PIT).
**
** @author by Alexander Tran 12610655
** @date 2020-06-23
*/
/*!
**  @addtogroup PIT_module PIT module documentation
**  @{
*/

#include "Types\types.h"
#include "PIT\PIT.h"
#include "MK64F12.h"
#include "fsl_clock.h"
#include "core_cm4.h"
#include "OS.h"
#include "LEDs\LEDs.h"
#include "ADC\ADC.h"
#include "VRR\VRR.h"

channelData_t Sample[1];

static void (*Userfunction)(void*); /*!< User function to be called by the PIT ISR. */
static void* Userarguments; /*!< Arguments to pass to the PIT user function. */
OS_ECB *PIT2Semaphore;
OS_ECB *PIT3Semaphore;
int16_t ReadAnalog;
int position; //position of the array

/*! @brief Thread to control PIT channel 2
 *
 *  @param pData - Thread data pointer.
 */
void PIT2CallbackThread(void *pData)
{
	for (;;)
	{
		OS_SemaphoreWait(PIT2Semaphore, 0);

		if (Userfunction)
		{
			(*Userfunction)(Userarguments);
		}

		if (Timing_Mode == 1) //If Timing Mode is set to 1 definite mode is activated
		{
			Definite_Mode();
		}

		else if (Timing_Mode == 2)
		{
			Inverse_Mode();
		}
	}
}

/*! @brief Thread to control PIT channel 3
 *
 *  @param pData - Thread data pointer.
 */
void PIT3CallbackThread(void *pData)
{
	for (;;)
	{
		OS_SemaphoreWait(PIT3Semaphore, 0);

		static int16_t tempVar; //Temporary value to store variable

		tempVar = Calc_RMS(Sample[1].sampleArray); //Calculate RMS voltage from sample
		Sample[1].voltRMS = tempVar;

		Voltage_Checker(tempVar);

		if (Userfunction)
		{
			(*Userfunction)(Userarguments);
		}

	}
}
/*! @brief Sets up the PIT before first use.
 *
 *  Enables the PIT and freezes the timer when debugging.
 *  @param moduleClk The module clock rate in Hz.
 *  @param userFunction is a pointer to a user callback function.
 *  @param userArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the PIT was successfully initialized.
 *  @note Assumes that moduleClk has a period which can be expressed as an integral number of nanoseconds.
 */
bool PIT_Init(const uint32_t moduleClk, void (*userFunction)(void*), void* userArguments)
{
	PIT2Semaphore = OS_SemaphoreCreate(0);
	PIT3Semaphore = OS_SemaphoreCreate(0);

	Userfunction = userFunction;
	Userarguments = userArguments;

	CLOCK_EnableClock(kCLOCK_Pit0);

	PIT->MCR &= ~PIT_MCR_MDIS_MASK;//disable the module to allow any kind of setup to PIT
	PIT->MCR &= ~PIT_MCR_FRZ_MASK;

	PIT->CHANNEL[2].TCTRL |= PIT_TCTRL_TIE_MASK;//enables interrupts for PIT-channel 2
	PIT->CHANNEL[3].TCTRL |= PIT_TCTRL_TIE_MASK;//enables interrupts for PIT-channel 3

	NVIC_ClearPendingIRQ(PIT2_IRQn);
	NVIC_EnableIRQ(PIT2_IRQn);

	NVIC_ClearPendingIRQ(PIT3_IRQn);
	NVIC_EnableIRQ(PIT3_IRQn);

	return true;
}

/*! @brief Sets the value of the desired period of the PIT.
 *
 *  @param period The desired value of the timer period in nanoseconds.
 *  @param restart TRUE if the PIT is disabled, a new value set, and then enabled.
 *                 FALSE if the PIT will use the new value after a trigger event.
 *  @note The function will enable the timer and interrupts for the PIT.
 */
void PIT_Set(const uint64_t period, const bool restart)
{
	//K64 module refers to these variables
	static uint64_t PITmoduleClk;
	uint64_t freqHz = 1e9 / period;
	uint64_t cycleCount = PITmoduleClk / freqHz;
	uint64_t triggerVal = cycleCount - 1;

	//TSV - Timer Start Value.
	PIT->CHANNEL[0].LDVAL = PIT_LDVAL_TSV(triggerVal);

	if (restart)
	{
		PIT_Enable(false);
		PIT_Enable(true);
	}
}

/*! @brief Sets the value of the desired period of the PIT.
 *
 *  @param period The desired value of the timer period in nanoseconds.
 *  @param restart TRUE if the PIT is disabled, a new value set, and then enabled.
 *                 FALSE if the PIT will use the new value after a trigger event.
 *  @note The function will enable the timer and interrupts for the PIT.
 */
void PIT_Set3(const uint32_t period, const bool restart)
{
	//K64 module refers to these variables
	static uint32_t PITmoduleClk;
	uint32_t freqHz = 1e9 / period;
	uint32_t cycleCount = PITmoduleClk / freqHz;
	uint32_t triggerVal = cycleCount - 1;

	//TSV - Timer Start Value.
	PIT->CHANNEL[3].LDVAL = PIT_LDVAL_TSV(triggerVal);

	if (restart)
	{
		PIT_Enable3(false);
		PIT_Enable3(true);
	}
}
/*! @brief Enables or disables the PIT.
 *
 *  @param enable - TRUE if the PIT is to be enabled, FALSE if the PIT is to be disabled.
 */
void PIT_Enable(const bool enable)
{
	if (enable == true)
	{
		PIT->CHANNEL[2].TCTRL |= PIT_TCTRL_TEN_MASK; //enable timer 2
	}
	else
	{
		PIT->CHANNEL[2].TCTRL &= ~PIT_TCTRL_TEN_MASK; //disable timer 2

	}
}
/*! @brief Enables or disables the PIT.
 *
 *  @param enable - TRUE if the PIT is to be enabled, FALSE if the PIT is to be disabled.
 */
void PIT_Enable3(const bool enable)
{
	if (enable == true)
	{
		PIT->CHANNEL[3].TCTRL |= PIT_TCTRL_TEN_MASK; //enable timer 3
	}
	else
	{
		PIT->CHANNEL[3].TCTRL &= ~PIT_TCTRL_TEN_MASK; //disable timer 3
	}
}

/*! @brief Interrupt service routine for the PIT.
 *
 *  The periodic interrupt timer has timed out.
 *  The user callback function will be called.
 *  @note Assumes the PIT has been initialized.
 */

void PIT2_IRQHandler(void)
{
	OS_ISREnter();

	//clear flag
	PIT->CHANNEL[2].TFLG |= PIT_TFLG_TIF_MASK;

	OS_SemaphoreSignal(PIT2Semaphore);

	OS_ISRExit();
}

/*! @brief Interrupt service routine for the PIT.
 *
 *  The periodic interrupt timer has timed out.
 *  The user callback function will be called.
 *  @note Assumes the PIT has been initialized.
 */
void PIT3_IRQHandler(void)
{
	OS_ISREnter();

	//clear flag
	PIT->CHANNEL[3].TFLG |= PIT_TFLG_TIF_MASK;
	ReadAnalog = ADC_Read(); //Read ADC values from Channel 23
	Sample[1].sampleArray[position] = ReadAnalog; //Sliding window array of samples to store values
	position++;

	if (position == 16) //If array reaches 16 samples
	{
	    position = 0; //Set array back to 0
	    OS_SemaphoreSignal(PIT3Semaphore);
	}

	OS_ISRExit();
}
