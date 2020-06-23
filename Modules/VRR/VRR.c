/*! @file
 *
 *  @brief Routines for regulating the Voltage Regulating Relay(VRR).
 *
 *  This contains the functions for operating the Voltage Regulating Relay(VRR).
 *
 *  @author Alexander Tran 12610655
 *  @date 2020-06-21
 */
/*!
**  @addtogroup VRR_module VRR module documentation
**  @{
*/
#include "types\types.h"
#include "VRR\VRR.h"
#include "FG.h"
#include "LEDs\LEDs.h"
#include <math.h>

const int16_t NB_OF_SAMPLES = 16;

int16_t VoltRMS(int16_t Sample[NB_OF_SAMPLES])
{
	float voltRMS;

	for (int i = 0; i < NB_OF_SAMPLES; i++)
	{
		voltRMS += pow(Sample[i], 2);
	}

	voltRMS = voltRMS/16;
	voltRMS = sqrt(voltRMS);
}

void Definte_Mode(void)
{
	uint64_t TimerPeriod = 5000000000;
	PIT_Set(5000000000, false);
}


void Alarm_Tap()
{
	LEDs_On(LED_RED);
}

void Raise_Tap()
{
	LEDs_On(LED_GREEN);
}

void Lower_Tap()
{
	LEDs_On(LED_BLUE);
}

void Idle_Signal()
{
	LEDs_Off(LED_GREEN | LED_BLUE | LED_RED);
}
