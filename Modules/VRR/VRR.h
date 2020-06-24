/*! @file
 *
 *  @brief Routines for regulating the Voltage Regulating Relay(VRR).
 *
 *  This contains the functions for operating the Voltage Regulating Relay(VRR).
 *
 *  @author Alexander Tran 12610655
 *  @date 2020-06-21
 */

#ifndef VRR_H_
#define VRR_H_

#include "types\types.h"
extern int8_t Nb_Raises; //Number of raises VRR
extern int8_t Nb_Lowers; //Number of lower voltage VRR
extern int8_t Timing_Mode; //Timing Mode, Inverse or Definite

typedef struct
{
	int16_t sampleArray[16]; //Array to store samples
	int16_t voltRMS; //Voltage RMS values
}
channelData_t;

/*! @brief Alarm Signal, turn on Red LED
 *
 *
 */
void Alarm_Tap();
/*! @brief Raise Signal, turn off all LED's
 *
 *
 */
void Raise_Tap();
/*! @brief Lower Signal, turn on blue LED
 *
 *
 */
void Lower_Tap();
/*! @brief Idle Signal, turn off all LED's
 *
 *
 */
void Idle_Signal();
/*! @brief Calculates the RMS voltage value
 *
 *  @param Sample array of samples
 */
int16_t Calc_RMS(int16_t Sample[]);
/*! @brief Sets the time period for definite mode
 *
 *  @param void
 */
void Definite_Mode(void);
/*! @brief Sets the time period for inverse mode
 *
 *  @param void
 */
void Inverse_Mode(void);
/*! @brief Check if the value of RMS voltage is in or out of bounds
 *
 *  @param voltRMS RMS Voltage value being processed
 */
void Voltage_Checker(int16_t voltRMS);
/*! @brief Check of boundaries in definite timing mode
 *  @return void
 */
void Definite_Boundary(void);
/*! @brief Check of boundaries in inverse timing mode
 *  @return void
 */
void Inverse_Boundary();
void Frequency_Tracking(void);
#endif /* VRR_H_ */
