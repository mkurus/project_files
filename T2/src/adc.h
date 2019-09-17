/*
 * adc.h
 *
 *  Created on: 3 Mar 2016
 *      Author: admin
 */

#ifndef ADC_H_
#define ADC_H_


#define ADC_SAMPLING_TIMER                 LPC_TIMER0

#define   NUMBER_OF_ADC_CHANNELS          4
#define   NUMBER_OF_SAMPLES_PER_SEC       50
#define   MICROSEC                        1000000
#define   ADC_TIMER_PERIOD                MICROSEC / NUMBER_OF_SAMPLES_PER_SEC

void adc_init();
uint16_t Get_ADCValue(uint8_t channelNum);
uint16_t Get_ADC_Channel_Value(uint8_t adc_channel);
void StartADCConversion(uint8_t channelNum);
void StopADCConversion();
void ADC_IsrHandler();
#endif /* ADC_H_ */
