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

	ADC0->CFG1 = 0; //Clear CFG1 Register
	ADC0->SC1[A] |= ADC_SC1_ADCH(31);
	//Set to 16-bit mode, Set to bus clock, Set clock divisor to bus clock /1
	ADC0->CFG1 |= (ADC_CFG1_MODE(3) | ADC_CFG1_ADICLK(0) | ADC_CFG1_ADIV(0));
	ADC0->CFG1 |= ADC_CFG1_ADLSMP_MASK; //Configure long sample time

	ADC0->SC2 &= ~ADC_SC2_ADACT_MASK;
	ADC0->SC2 &= ~ADC_SC2_ADTRG_MASK;
	ADC0->SC2 &= ~ADC_SC2_ACFE_MASK;
	ADC0->SC2 &= ~ADC_SC2_DMAEN_MASK;

	ADC0->SC1[A] |= ADC_SC1_AIEN_MASK;

	return true;
}

unsigned short ADC_Read(void)
{
	ADC0->SC1[A] &= ~ADC_SC1_DIFF_MASK; //Set Differential Mode to 0
	ADC0->SC1[A] |= ADC_SC1_ADCH(23); //Set Channel 23 as ADC Input
	while(ADC0->SC2 & ADC_SC2_ADACT_MASK);
	while(!(ADC0->SC1[A] & ADC_SC1_COCO_MASK));
	return ADC0->R[A];
}
