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
