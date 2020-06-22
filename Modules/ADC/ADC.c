/*!
** @file ADC.c
** @brief Routines for setting up the Analog to Digital Converter module (ADC).
** @author by Alexander Tran 12610655
** @date 2020-06-20
*/
/*!
**  @addtogroup ADC_Module ADC module documentation
**  @{
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
bool ADC_Init(const uint32_t moduleClock)
{
	CLOCK_EnableClock(kCLOCK_Adc0);

	ADC0->CFG1 = 0; //Clear CFG1 Register

	//Set to 16-bit mode, Set to bus clock, Set clock divisor to bus clock /1
	ADC0->CFG1 |= (ADC_CFG1_MODE(3) | ADC_CFG1_ADICLK(0) | ADC_CFG1_ADIV(0));
	ADC0->CFG1 |= ADC_CFG1_ADLSMP_MASK; //Configure long sample time

	ADC0->SC2 &= ~ADC_SC2_ADACT_MASK; //Disable conversion
	ADC0->SC2 &= ~ADC_SC2_ADTRG_MASK;
	ADC0->SC2 &= ~ADC_SC2_ACFE_MASK;
	ADC0->SC2 &= ~ADC_SC2_DMAEN_MASK;

	ADC0->SC1[A] |= ADC_SC1_AIEN_MASK;

	return true;
}

bool ADC_Get(int16_t* const value)
{
	uint16_t valueData;
	ADC0->SC1[A] |= ADC_SC1_DIFF(0); //Set Differential Mode to 0 for Unipolar/Single ended conversion
	ADC0->SC1[A] |= ADC_SC1_ADCH(23); //Set Channel 23 as ADC Input
	while(ADC0->SC2 & ADC_SC2_ADACT_MASK); //Conversion taking place
	while(!(ADC0->SC1[A] & ADC_SC1_COCO_MASK)); //Wait until conversion is complete

	value[0] = ADC0->R[A]; //Dummy read

	for (valueData = 0; valueData < 16; valueData++) //Store up to 16 sample values in the ADC Register
	{
		value[valueData] = ADC0->R[A]; //Store values of the samples to ADC register
	}
}

bool ADC_Put(int16_t const value)
{

}
