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

void Alarm_Tap();
void Raise_Tap();
void Lower_Tap();
void Idle_Signal();

#endif /* VRR_H_ */
