/*!
** @file PIT.c
** @brief Routines for controlling Periodic Interrupt Timer (PIT).
**
** This contains the functions for operating the periodic interrupt timer (PIT).
**
** @author by Alexander Tran 12610655 and Hubert Chau 12568681
** @date 2020-05-11
*/
/*!
**  @addtogroup pit_module PIT module documentation
**  @{
*/

#include "Types\types.h"
#include "PIT\PIT.h"
#include "MK64F12.h"
#include "fsl_clock.h"
#include "core_cm4.h"
#include "OS.h"
#include "LEDs\LEDs.h"
#define THREAD_STACK_SIZE 100
OS_ECB *PITSemaphore;
OS_ERROR error;
OS_THREAD_STACK(PITCallbackThreadStack, THREAD_STACK_SIZE);

void PITCallbackThread(void *arg)
{
	for (;;)
	{
		OS_SemaphoreWait(PITSemaphore, 0);
		LEDs_Toggle(LED_GREEN);
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
	PITSemaphore = OS_SemaphoreCreate(0);

	CLOCK_EnableClock(kCLOCK_Pit0);

	PIT->MCR = ~PIT_MCR_MDIS_MASK;//disable the module to allow any kind of setup to PIT

	PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TIE_MASK;//enables interrupts for PIT-channel 0

	NVIC_ClearPendingIRQ(PIT0_IRQn);
	NVIC_EnableIRQ(PIT0_IRQn);

	error = OS_ThreadCreate(PITCallbackThread, NULL, &PITCallbackThreadStack[THREAD_STACK_SIZE-1], 3);

	return true;
}

/*! @brief Sets the value of the desired period of the PIT.
 *
 *  @param period The desired value of the timer period in nanoseconds.
 *  @param restart TRUE if the PIT is disabled, a new value set, and then enabled.
 *                 FALSE if the PIT will use the new value after a trigger event.
 *  @note The function will enable the timer and interrupts for the PIT.
 */
void PIT_Set(const uint32_t period, const bool restart)
{
	//K64 module refers to these variables
	static uint32_t PITmoduleClk;
	uint32_t freqHz = 1e9 / period;
	uint32_t cycleCount = PITmoduleClk / freqHz;
	uint32_t triggerVal = cycleCount - 1;

	//TSV - Timer Start Value.
	PIT->CHANNEL[0].LDVAL = PIT_LDVAL_TSV(triggerVal);

	if (restart)
	{
		PIT_Enable(false);
		PIT_Enable(true);
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
		PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TEN_MASK; //enable timer 0
	}
	else
	{
		PIT->CHANNEL[0].TCTRL &= ~PIT_TCTRL_TEN_MASK; //disable timer 0

	}
}

/*! @brief Interrupt service routine for the PIT.
 *
 *  The periodic interrupt timer has timed out.
 *  The user callback function will be called.
 *  @note Assumes the PIT has been initialized.
 */
void PIT0_IRQHandler(void)
{
	OS_ISREnter();

	//clear flag
	PIT->CHANNEL[0].TFLG |= PIT_TFLG_TIF_MASK;

	OS_SemaphoreSignal(PITSemaphore);

	OS_ISRExit();
}
