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
#include "ADC\ADC.h"
#include <math.h>

channelData_t Samples[1]; //1 phase sample implementation

const int16_t NB_OF_SAMPLES = 16;
const int16_t UPPER_RANGE = 9830.3;
const int16_t LOWER_RANGE = 6553.5;
const int16_t NOMINAL_VOLT = 8191.9;
const int16_t ANALOG_VALUE_PER_VOLT = 3276.7;
int16_t newVRMS;
float timeTaken;
float timeGlobal;
float percentageTaken;
float deviation;

void Alarm_Tap()
{
	LEDs_On(LED_RED); //Turn on Red LED
}

void Raise_Tap()
{
	LEDs_On(LED_GREEN); //Turn on Green LED
}

void Lower_Tap()
{
	LEDs_On(LED_BLUE); //Turn on Blue LED
}

void Idle_Signal()
{
	LEDs_Off(LED_GREEN | LED_BLUE | LED_RED); //Turn off all LEDs
}

int16_t Calc_RMS(int16_t Sample[])
{
	float voltRMS;

	for (int i = 0; i < NB_OF_SAMPLES; i++) //Loop through the 16 samples
	{
		voltRMS += pow(Sample[i], 2);
	}

	voltRMS = voltRMS/16;
	voltRMS = sqrt(voltRMS);
}

void Definte_Mode(void)
{
	uint32_t DefiniteTimer = 5000000000;
	PIT_Set(DefiniteTimer, false);
}

void Inverse_Mode(void)
{
	uint32_t InverseTimer = 10000000;
	PIT_Set3(InverseTimer, false);
}
void Voltage_Checker(int16_t voltRMS)
{

	//If RMS voltage is out of range trigger Alarm Signal
	if (voltRMS > UPPER_RANGE || voltRMS < LOWER_RANGE )
	{
		Alarm_Tap();
	}

	//If RMS voltage is within range, clear all time variables and set idle signal
	if (voltRMS < UPPER_RANGE || voltRMS > LOWER_RANGE)
	{
		deviation = 0;
		timeTaken = 0;
		timeGlobal = 0;
		percentageTaken = 0;
		Idle_Signal();
	}
}

/*! @brief Check the RMS Voltage from the sample array in definite timing mode
 *  @return void
 */
void Definite_Boundary(void)
{
	newVRMS = Samples[1].voltRMS;

	if (newVRMS > UPPER_RANGE)
	{
		Lower_Tap();
		Nb_Lowers++;
	}

	else if (newVRMS < LOWER_RANGE)
	{
		Raise_Tap();
		Nb_Raises++;
	}

	else
	{
		Idle_Signal();
	}
}

void Inverse_Boundary()
{
	if (Samples[1].voltRMS > UPPER_RANGE)
	{
		if (percentageTaken < 100)
		{
			deviation = (Samples[1].voltRMS - NOMINAL_VOLT) / ANALOG_VALUE_PER_VOLT;
			timeGlobal = (5 / (2 * deviation)) * 1000;
		}

		if (timeGlobal < 1000)
		{
			timeGlobal = 1000;
		}
			timeTaken = timeTaken + 10;
			percentageTaken = (100 * timeTaken / timeGlobal);
		}

	    else
	   {
			Lower_Tap();
			Nb_Lowers++;
			//start timing
			deviation = 0;
			timeTaken = 0;
			timeGlobal = 0;
			percentageTaken = 0;
	   }

	  if (Samples[1].voltRMS < LOWER_RANGE)
	  {
		if (percentageTaken < 100 && timeGlobal < 1000)
		{
			deviation = (NOMINAL_VOLT - Samples[1].voltRMS) / ANALOG_VALUE_PER_VOLT;
			timeGlobal = (5 / (2 * deviation)) * 1000;
		}

		if (timeGlobal < 1000)
		{
			timeGlobal = 1000;
		}
			timeTaken = timeTaken + 10;
			percentageTaken = (100 * timeTaken / timeGlobal);
		}

		else
		{
			Raise_Tap();
			Nb_Raises++;
			//start timing
			deviation = 0;
			timeTaken = 0;
			timeGlobal = 0;
			percentageTaken = 0;
		 }

	  if (Samples[1].voltRMS > LOWER_RANGE || Samples[1].voltRMS < UPPER_RANGE)
	  {
		  PIT_Enable(false); //Disable periodic interrupt timer
		  Idle_Signal();
		  //Clear and Reset time variables
		  deviation = 0;
		  timeTaken = 0;
		  timeGlobal = 0;
		  percentageTaken = 0;
	  }
}

void Frequency_Tracking(void)
{
	float period;
	float frequency;

	frequency = 1 / period; //Frequency calculation

	if (frequency > 5.25 || frequency < 4.75)
	{
		frequency = 5; //Set frequency to 5Hz
	}
}
