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
bool ADC_Init();

/*! @brief Takes a sample from an analog input channel.
 *
 *  @param channelNb is the number of the analog input channel to sample.
 *  @return bool - true if the channel was read successfully.
 */
unsigned short ADC_Read(void);


#endif /* ADC_ADC_H_ */
