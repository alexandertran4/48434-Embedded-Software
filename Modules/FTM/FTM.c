/*!
** @file FTM.c
** @brief Routines for setting up the FlexTimer module (FTM).
** @author by Alexander Tran 12610655 and Hubert Chau 12568681
** @date 2020-05-11
*/
/*!
**  @addtogroup FTM_Module FTM module documentation
**  @{
*/
// new types
#include "Types\types.h"
#include "FTM\FTM.h"
#include "MK64F12.h"
#include "core_cm4.h"
#include "fsl_clock.h"
#include "OS.h"
#include "LEDs\LEDs.h"
static void (*CallbackFunction)(void*);
static void *CallbackArguments;
#define THREAD_STACK_SIZE 100
static OS_ECB *FTMSemaphore;
OS_ERROR error;
OS_THREAD_STACK(FTMCallbackThreadStack, THREAD_STACK_SIZE);

/*! @brief Sets up the FTM before first use.
 *
 *  Enables the FTM as a free running 16-bit counter.
 *  @return bool - TRUE if the FTM was successfully initialized.
 */
bool FTM_Init()
{
	FTMSemaphore = OS_SemaphoreCreate(0); //Create FTM0 Semaphore

	static const uint32_t FIXED_FREQUENCY_CLOCK = 0b10;

	CLOCK_EnableClock(kCLOCK_Ftm0);

	FTM0->CNTIN = ~FTM_CNTIN_INIT_MASK;//Verify initial value of counter for space

	FTM0->MOD = FTM_MOD_MOD_MASK;//Initialise FTM counter by writing to CNT

	FTM0->CNT = ~FTM_CNT_COUNT_MASK; //Checks counter value

	FTM0->SC |= FTM_SC_CLKS(FIXED_FREQUENCY_CLOCK); //Enable FTM overflow interrupts, up counting mode

	NVIC_ClearPendingIRQ(FTM0_IRQn);
	NVIC_EnableIRQ(FTM0_IRQn);

	error = OS_ThreadCreate(FTMCallbackThread, NULL, &FTMCallbackThreadStack[THREAD_STACK_SIZE-1], 4);

	return true;
}

/*! @brief Sets up a timer channel.
 *
 *  @param aFTMChannel is a structure containing the parameters to be used in setting up the timer channel.
 *    channelNb is the channel number of the FTM to use.
 *    delayCount is the delay count (in module clock periods) for an output compare event.
 *    timerFunction is used to set the timer up as either an input capture or an output compare.
 *    ioType is a union that depends on the setting of the channel as input capture or output compare:
 *      outputAction is the action to take on a successful output compare.
 *      inputDetection is the type of input capture detection.
 *    callbackFunction is a pointer to a user callback function.
 *    callbackArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the timer was set up successfully.
 *  @note Assumes the FTM has been initialized.
 */
bool FTM_Set(const TFTMChannel* const aFTMChannel)
{
	CallbackFunction = aFTMChannel->callbackFunction;
	CallbackArguments = aFTMChannel->callbackArguments;

	switch (aFTMChannel->timerFunction)
	{
		case (TIMER_FUNCTION_INPUT_CAPTURE):
		return false;
		break;

		case (TIMER_FUNCTION_OUTPUT_COMPARE):
		//set register for output compare 01 for MSnB:MSnA registers
		FTM0->CONTROLS[aFTMChannel->channelNb].CnSC &= ~FTM_CnSC_MSB_MASK;
		FTM0->CONTROLS[aFTMChannel->channelNb].CnSC |= ~FTM_CnSC_MSA_MASK;

		switch (aFTMChannel->ioType.outputAction)
		{
			case (TIMER_OUTPUT_DISCONNECT)://TO set the ELSnB:ELSnA registers
					//00
			FTM0->CONTROLS[aFTMChannel->channelNb].CnSC &= ~FTM_CnSC_ELSB_MASK;
			FTM0->CONTROLS[aFTMChannel->channelNb].CnSC &= ~FTM_CnSC_ELSA_MASK;
			break;

			case (TIMER_OUTPUT_TOGGLE):
					//01
			FTM0->CONTROLS[aFTMChannel->channelNb].CnSC &= ~FTM_CnSC_ELSB_MASK;
			FTM0->CONTROLS[aFTMChannel->channelNb].CnSC |= FTM_CnSC_ELSA_MASK;
			break;

			case (TIMER_OUTPUT_LOW):
					//10
			FTM0->CONTROLS[aFTMChannel->channelNb].CnSC |= FTM_CnSC_ELSB_MASK;
			FTM0->CONTROLS[aFTMChannel->channelNb].CnSC &= ~FTM_CnSC_ELSA_MASK;
			break;

			case (TIMER_OUTPUT_HIGH):
					//11
			FTM0->CONTROLS[aFTMChannel->channelNb].CnSC |= FTM_CnSC_ELSB_MASK;
			FTM0->CONTROLS[aFTMChannel->channelNb].CnSC |= FTM_CnSC_ELSA_MASK;
			break;
		}
    break;
	}
}

void FTMCallbackThread(void *pData)
{
  for (;;)
  {
    //Wait on the FTM0 Semaphore
    OS_SemaphoreWait(FTM0Semaphore, 0);
    LEDs_Off(LED_BLUE);
  }
}

/*! @brief Starts a timer if set up for output compare.
 *
 *  @param aFTMChannel is a structure containing the parameters to be used in setting up the timer channel.
 *  @return bool - TRUE if the timer was started successfully.
 *  @note Assumes the FTM has been initialized.
 */
bool FTM_StartTimer(const TFTMChannel* const aFTMChannel)
{
	if(aFTMChannel->channelNb < 0 || aFTMChannel->channelNb > 8) //validate channel
	{
		return false;
	}

	if(aFTMChannel->timerFunction == TIMER_FUNCTION_OUTPUT_COMPARE)
	{
		 //CHIE - channel interrupt enable
		 FTM0->CONTROLS[aFTMChannel->channelNb].CnSC |= FTM_CnSC_CHIE_MASK; //enables channel interrupts
		 //Sets the channels initial counts
		 FTM0->CONTROLS[aFTMChannel->channelNb].CnV |= FTM0->CNT + aFTMChannel->delayNanoseconds;
		 return true;
	}
	else
	{
		return false;
	}
}

void FTM0_IRQHandler(void)
{
	OS_ISREnter();

	for(uint8_t channel = 0; channel < 8; channel++)
	{
		if (FTM0->CONTROLS[channel].CnSC & FTM_CnSC_CHF_MASK)
		{
			//Check if interrupt is enabled for channel and check if the flag is set for that channel
			if (!(FTM0->CONTROLS[channel].CnSC & FTM_CnSC_MSB_MASK) && (FTM0->CONTROLS[channel].CnSC & FTM_CnSC_MSA_MASK))
			{
				FTM0->CONTROLS[channel].CnSC &= ~FTM_CnSC_CHF_MASK;

				FTM0->CONTROLS[channel].CnSC &= ~FTM_CnSC_CHIE_MASK; //Disable Interrupt

				OS_SemaphoreSignal(FTMSemaphore);
			}
		}
	}

	OS_ISRExit();
}
