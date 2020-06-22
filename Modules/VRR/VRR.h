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

const int16_t SAMPLES_PER_PERIOD = 16;

void Alarm_Tap();
void Raise_Tap();
void Lower_Tap();
void Idle_Signal();

#endif /* VRR_H_ */
