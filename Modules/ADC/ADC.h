/*
 * ADC.h
 *
 *  Created on: 19 Jun 2020
 *      Author: User
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
bool ADC_Get(const uint8_t channelNb);


#endif /* ADC_ADC_H_ */
