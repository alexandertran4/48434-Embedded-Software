/*
 * ADC.c
 *
 *  Created on: 19 Jun 2020
 *      Author: User
 */
/*! @brief Sets up the ADC before first use.
 *
 *  @param moduleClock The module clock rate in Hz.
 *  @return bool - true if the UART was successfully initialized.
 */
#include "MK64F12.h"
#include "system_MK64F12.h"
#include "core_cm4.h"
#include "FIFO\FIFO.h"
// Serial Communication Interface
#include "UART\UART.h"
#include "Critical\critical.h"
//K64 module registers
#include "fsl_common.h"
//Port definitions
#include "fsl_port.h"
#include "fsl_clock.h"
#include "OS.h"
bool ADC_Init()
{
	CLOCK_EnableClock(kCLOCK_Adc0);

	ADC0->SC1 &= ~ADC_SC1_DIFF_MASK; //Set Differential Mode to 0
	ADC0->SC1[A] |= ADC_SC1_ADCH(23); //Set Channel 23 as ADC Input

	ADC0->CFG1 = 0;
	ADC0->CFG1 |= (ADC_CFG1_MODE(3) | ADC_CFG1_ADICLK(0));

	NVIC_ClearPendingIRQ(ADC0_IRQn);
	NVIC_EnableIRQ(ADC0_IRQn);

	return true;
}

/*! @brief Takes a sample from an analog input channel.
 *
 *  @param channelNb is the number of the analog input channel to sample.
 *  @return bool - true if the channel was read successfully.
 */
bool ADC_Get(const uint8_t channelNb)
{

}

void ADC_IRQHandler(void)
{

}
