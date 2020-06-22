/*
 * ADC.h
 *
 *  Created on: 19 Jun 2020
 *      Author: Alexander Tran
 */

#ifndef ADC_ADC_H_
#define ADC_ADC_H_

/*! @brief Sets up the ADC before first use.
 *
 *  @param moduleClock The module clock rate in Hz.
 *  @return bool - true if the UART was successfully initialized.
 */
bool ADC_Init(uint32_t moduleClock);

/*! @brief Takes a sample from ADC Channel 23.
 *
 *  @return bool - true if the channel was read successfully.
 */
int16_t ADC_Read();


#endif /* ADC_ADC_H_ */
