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

channelData_t Samples[1];

const int16_t NB_OF_SAMPLES = 16;
const int16_t UPPER_RANGE = 9830.3;
const int16_t LOWER_RANGE = 6553.5;

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
	PIT_Set(TimerPeriod, false);
}

void Voltage_Checker(int16_t voltRMS)
{
	if((voltRMS > ))
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
