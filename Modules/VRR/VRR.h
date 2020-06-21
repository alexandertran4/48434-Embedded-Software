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

enum Timer_Mode
{
	DEFINITE,
	INVERSE
};

typedef struct AnalogThreadData
{
  OS_ECB* Semaphore;
  uint8_t channelNb;
  int16_t AnalogSamples[SAMPLES_PER_PERIOD]; // Channel samples
  uint16_t TRMS;  // Raw True RMS Calculation
  uint16_t setTRMSValue;  // TRMS value from set for inverse timing
  int16union_t VTRMS;    // Volt converted True RMS
  enum TIMER_MODE tmrMode;  // What timer mode
  bool inBounds;
  uint64_t timeSet;
  uint64_t newCountDown;
} AnalogThreadData_t;

#endif /* VRR_H_ */
