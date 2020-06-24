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
#include "PIT\PIT.h"
#include <math.h>

channelData_t Samples[1]; //1 phase sample implementation

const int16_t NB_OF_SAMPLES = 16;
const int16_t UPPER_RANGE = 9830.3;
const int16_t LOWER_RANGE = 6553.5;
const int16_t NOMINAL_VOLT = 8191.9;
const int16_t ANALOG_VALUE_PER_VOLT = 3276.7; //See assessment sheet for clarification of defined values
int16_t newVRMS;
float timeTaken; //time taken to change voltage
float timeGlobal;
float percentageTaken;
float deviation; //Voltage deviation

/*! @brief Alarm Signal, turn on Red LED
 *
 *
 */
void Alarm_Tap()
{
	LEDs_On(LED_RED); //Turn on Red LED
}

/*! @brief Raise Signal, turn off all LED's
 *
 *
 */
void Raise_Tap()
{
	LEDs_On(LED_GREEN); //Turn on Green LED
}

/*! @brief Lower Signal, turn on blue LED
 *
 *
 */
void Lower_Tap()
{
	LEDs_On(LED_BLUE); //Turn on Blue LED
}

/*! @brief Idle Signal, turn off all LED's
 *
 *
 */
void Idle_Signal()
{
	LEDs_Off(LED_GREEN | LED_BLUE | LED_RED); //Turn off all LEDs
}
/*! @brief Calculates the RMS voltage value
 *
 *  @param Sample array of 16 samples
 */
int16_t Calc_RMS(int16_t Sample[NB_OF_SAMPLES])
{
	float voltRMS;

	for (int i = 0; i < NB_OF_SAMPLES; i++) //Loop through the 16 samples
	{
		voltRMS += Sample[i] * Sample[i];
	}

	voltRMS = voltRMS/16;
	voltRMS = sqrt(voltRMS);
}

/*! @brief Sets the time period for definite mode
 *
 *  @param void
 */
void Definite_Mode(void)
{
	uint64_t DefiniteTimer = 5000000000; //Set to 5 secs as per specification
	PIT_Set(DefiniteTimer, false);
}

/*! @brief Sets the time period for inverse mode
 *
 *  @param void
 */
void Inverse_Mode(void)
{
	uint32_t InverseTimer = 10000000;
	PIT_Set3(InverseTimer, false);
}
/*! @brief Check if the value of RMS voltage is in or out of bounds
 *
 *  @param voltRMS RMS Voltage value being processed
 */
void Voltage_Checker(int16_t voltRMS)
{
	//If RMS voltage is out of range trigger Alarm Signal
	if (voltRMS > UPPER_RANGE || voltRMS < LOWER_RANGE )
	{
		Alarm_Tap();
		PIT_Enable3(true); //Enable PIT Channel 3 Timer

		if (Timing_Mode == 1) //Set to definite timing mode
		{
			Definite_Mode();
		}

		else if (Timing_Mode == 2) //Set to inverse timing mode
		{
			Inverse_Mode();
		}
	}

	//If RMS voltage is within range, clear all time variables and set idle signal
	if (voltRMS < UPPER_RANGE || voltRMS > LOWER_RANGE)
	{
		PIT_Enable3(false); //Disable PIT Channel 3 Timer
		Idle_Signal();
		//Set all variables to 0
	    deviation = 0;
	    timeTaken = 0;
	    timeGlobal = 0;
	    percentageTaken = 0;
	}
}

/*! @brief Check of boundaries in definite timing mode
 *  @return void
 */
void Definite_Boundary(void)
{
	newVRMS = Samples[1].voltRMS;

	if (newVRMS > UPPER_RANGE)
	{
		Lower_Tap(); //Trigger Lower_Tap alarm
		Nb_Lowers++;
	}

	else if (newVRMS < LOWER_RANGE)
	{
		Raise_Tap(); //Trigger Raise Alarm
		Nb_Raises++; //Increment Number of times RMS voltage has been raised
	}

	else
	{
		Idle_Signal(); //Once in range, trigger idle signal
	}
}

/*! @brief Check of boundaries in inverse timing mode
 *  @return void
 */
void Inverse_Boundary()
{
	newVRMS = Samples[1].voltRMS;

	if (newVRMS > UPPER_RANGE)
	{
		if (percentageTaken < 100)
		{
			deviation = (newVRMS - NOMINAL_VOLT) / ANALOG_VALUE_PER_VOLT; //Deviation from nominal voltage
			timeGlobal = (5 / (2 * deviation)) * 1000; //Time converted to milliseconds
		}

		if (timeGlobal < 1000)
		{
			timeGlobal = 1000; //Delay from formula is less than 1 second, set it to 1 as per specification constraint
		}
			timeTaken = timeTaken + 10;
			percentageTaken = (100 * timeTaken / timeGlobal);
		}

	    else
	   {
			Lower_Tap();
			Nb_Lowers++;
			//start timing
			PIT_Enable3(true);
			deviation = 0;
			timeTaken = 0;
			timeGlobal = 0;
			percentageTaken = 0;
	   }

	  if (newVRMS < LOWER_RANGE)
	  {
		if (percentageTaken < 100)
		{
			deviation = (NOMINAL_VOLT - newVRMS) / ANALOG_VALUE_PER_VOLT;
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
			PIT_Enable3(true);
			//start timing
			deviation = 0;
			timeTaken = 0;
			timeGlobal = 0;
			percentageTaken = 0;
		 }

	  if (newVRMS > LOWER_RANGE || newVRMS < UPPER_RANGE)
	  {
		  PIT_Enable3(false); //Disable periodic interrupt timer
		  Idle_Signal();
		  //Clear and Reset time variables
		  deviation = 0;
		  timeTaken = 0;
		  timeGlobal = 0;
		  percentageTaken = 0;
	  }
}
/*! @brief Frequency calculations
 *  @return void
 */
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
