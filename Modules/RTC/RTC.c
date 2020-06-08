/*!
** @file RTC.c
** @brief Routines to implement packet encoding and decoding for the serial port.
**
**  This contains the functions for operating the real time clock (RTC).
**
** @author by Alexander Tran 12610655 and Hubert Chau 12568681
** @date 2020-06-03
*/
/*!
**  @addtogroup RTC_module RTC module documentation
**  @{
*/
// new types
#include "Types\types.h"
#include "RTC\RTC.h"
#include "Critical\critical.h"
#include "fsl_clock.h"
#include "MK64F12.h"
#include "core_cm4.h"
#include "OS.h"

/*! @brief Initializes the RTC before first use.
 *
 *  Sets up the control register for the RTC and locks it.
 *  Enables the RTC and sets an interrupt every second.
 *  @param userFunction is a pointer to a user callback function.
 *  @param userArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the RTC was successfully initialized.
 */
bool RTC_Init(void)
{
	RTC_Semaphore = OS_SemaphoreCreate(0);

	UserFunction = userFunction;//Globally accessible (userFunction)
	UserArguments = userArguments;//Globally accessible (userArguments)

	CLOCK_EnableClock(kCLOCK_Rtc0); //Enable Clock RTC Clock Gate

	//Enable 12pF load
	RTC0->CR |= RTC_CR_SC2P_MASK;
	RTC0->CR |= RTC_CR_SC8P_MASK;

	if (RTC0->SR & RTC_SR_TIF_MASK)
	{
		RTC_Set(0, 0, 0);
	}

	RTC0->IER |= RTC_IER_TSIE_MASK; //Enables seconds enable interrupt (on by default)
	RTC0->SR |= RTC_SR_TCE_MASK; // Initialise the timer
	RTC0->CR |= RTC_CR_OSCE_MASK; // Enables the 32.768kHz Oscillator

	NVIC_ClearPendingIRQ(RTC_Seconds_IRQn);
	NVIC_EnableIRQ(RTC_Seconds_IRQn);

	return true;
}

/*! @brief Sets the value of the real time clock.
 *
 *  @param hours The desired value of the real time clock hours (0-23).
 *  @param minutes The desired value of the real time clock minutes (0-59).
 *  @param seconds The desired value of the real time clock seconds (0-59).
 *  @note Assumes that the RTC module has been initialized and all input parameters are in range.
 */
void RTC_Set(const uint8_t hours, const uint8_t minutes, const uint8_t seconds)
{
	RTC0->SR &= ~RTC_SR_TCE_MASK; //disables the time counter
	RTC0->TSR = (uint32_t)seconds + (uint32_t)minutes*60 + (uint32_t)hours*3600; //calculates the value of the real time clock
	RTC0->SR |= RTC_SR_TCE_MASK; //enables the time counter
}

/*! @brief Gets the value of the real time clock.
 *
 *  @param hours The address of a variable to store the real time clock hours.
 *  @param minutes The address of a variable to store the real time clock minutes.
 *  @param seconds The address of a variable to store the real time clock seconds.
 *  @note Assumes that the RTC module has been initialized.
 */
void RTC_Get(uint8_t* const hours, uint8_t* const minutes, uint8_t* const seconds)
{
	uint32_t totalTimeSeconds = RTC0->TSR % 86400; //seconds in a day
	//Conversions to hours/minutes/seconds
	*hours = (totalTimeSeconds / 3600); //updates the current time value of hours
	*minutes = (totalTimeSeconds % 3600) / 60;//updates the current time value of minutes
	*seconds = totalTimeSeconds - (*hours)*3600 - (*minutes)*60;//updates the current time value of seconds
}

/*! @brief Interrupt service routine for the RTC.
 *
 *  The RTC has incremented one second.
 *  The user callback function will be called.
 *  @note Assumes the RTC has been initialized.
 */
void RTC_Seconds_IRQHandler(void)
{
	OS_ISREnter();

	OS_SempahoreSignal(RTC_Semaphore);

	OS_ISRExit();
}
