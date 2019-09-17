/*****************************************************************************
 *   adc.c:  ADC module file for NXP LPC17xx Family Microprocessors
 *
 *   Copyright(C) 2009, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2009.05.25  ver 1.00    Prelimnary version, first Release
 *
******************************************************************************/
#include "chip.h"
#include "board.h"
#include "timer.h"
#include "adc.h"
#include "io_ctrl.h"
#include "ProcessTask.h"
#include "trace.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#define ADC_OFFSET		0x10
#define ADC_INDEX		4

#define ADC_DONE		0x80000000
#define ADC_OVERRUN		0x40000000
#define ADC_ADINT		0x00010000

#define ADC_NUM			8		    /* for LPCxxxx */
#define ADC_CLK			1000000		/* set to 1Mhz */


uint16_t adc0AvgValue, adc1AvgValue;
uint16_t ldrAvgValue;
void adc_init_sampling_timer(uint32_t usecs);
uint16_t MovingAvgFilter(uint8_t adcChannel, uint16_t *adcBuffer);

uint16_t ADCValue[NUMBER_OF_ADC_CHANNELS][NUMBER_OF_SAMPLES_PER_SEC];


static const PINMUX_GRP_T T2_AdcPortMuxTable[] = {
	{0,  23,   IOCON_MODE_INACT | IOCON_FUNC1},	  /* P_OHM_1 */
	{0,  24,   IOCON_MODE_INACT | IOCON_FUNC1},	  /* P_OHM_2 */
	{0,  26,   IOCON_MODE_INACT | IOCON_FUNC1},	  /* LDR     */
};
/****************************************************************/
void adc_init()
{
	uint32_t adcClock;

	Chip_IOCON_SetPinMuxing(LPC_IOCON, T2_AdcPortMuxTable, sizeof(T2_AdcPortMuxTable) / sizeof(PINMUX_GRP_T));

	/* Enable CLOCK into ADC controller */
	LPC_SYSCTL->PCONP |= (1 << 12);

	adcClock = Chip_Clock_GetPeripheralClockRate(SYSCTL_PCLK_ADC);

	 LPC_ADC->CR = ( 0x01 << 0 ) |  /* SEL=1,select channel 0 on ADC0 */
	        ( ( adcClock  / 1000000 - 1 ) << 8 ) |  /* CLKDIV = Fpclk / ADC_Clk - 1 */
	        ( 0 << 16 ) |         /* BURST = 0, no BURST, software controlled */
	        ( 0 << 17 ) |         /* CLKS = 0, 11 clocks/10 bits */
	        ( 1 << 21 ) |         /* PDN = 1, normal operation */
	        ( 0 << 24 ) |         /* START = 0 A/D conversion stops */
	        ( 0 << 27 );          /* EDGE = 0 (CAP/MAT singal falling,trigger A/D conversion) */

	 adc0AvgValue = 0;
	 adc1AvgValue = 0;
	 ldrAvgValue = 0;

	 adc_init_sampling_timer(ADC_TIMER_PERIOD);
	 LPC_ADC->INTEN = 0xFF;      /* Enable all interrupts */
	 NVIC_EnableIRQ(ADC_IRQn);

	 PRINT_K("\nADC Initialization Done\n");
}
/*************************************************************
** Function name:       ADC_IRQHandler
**
** Descriptions:        ADC interrupt handler
**
** parameters:          None
** Returned value:      None
**
**************************************************************/
void ADC_IRQHandler(void)
{
  uint32_t regVal;
  char printBuf[128];
  static uint16_t ch0SampleCount = 0;
  static uint16_t ch1SampleCount = 0;
  static uint16_t ldrSampleCount = 0;
  uint16_t *ptr;

  StopADCConversion();

  regVal = LPC_ADC->STAT;      /* Read ADC will clear the interrupt */

  if ( regVal & ADC_ADINT )
  {
     switch ( regVal & 0xFF )    /* check DONE bit */
    {
       case 0x01:
       ADCValue[ADC_CH0][ch0SampleCount++] = ( LPC_ADC->DR[0] >> 4 ) & 0xFFF;
       if(ch0SampleCount == NUMBER_OF_SAMPLES_PER_SEC){
    	 //  PRINT_K("50 values sampled 1\n");
    	   ptr = &ADCValue[ADC_CH0][0];
    	   adc0AvgValue = MovingAvgFilter(ADC_CH0, ptr);
    	   ch0SampleCount = 0;
       }
       Chip_TIMER_Enable(ADC_SAMPLING_TIMER);
       break;

       case 0x02:
       ADCValue[ADC_CH1][ch1SampleCount++] = ( LPC_ADC->DR[1] >> 4 ) & 0xFFF;
       if(ch1SampleCount == NUMBER_OF_SAMPLES_PER_SEC){
    	//   PRINT_K("50 values sampled 2\n");
    	   ptr = &ADCValue[ADC_CH1][0];
    	   adc1AvgValue = MovingAvgFilter(ADC_CH1,ptr);
           ch1SampleCount = 0;
       }
       Chip_TIMER_Enable(ADC_SAMPLING_TIMER);
       break;

       case 0x04:
       ADCValue[ADC_CH2][ldrSampleCount++] = ( LPC_ADC->DR[2] >> 4 ) & 0xFFF;
    	if(ldrSampleCount == NUMBER_OF_SAMPLES_PER_SEC){
    		ldrSampleCount = 0;
    	}
       Chip_TIMER_Enable(ADC_SAMPLING_TIMER);
       break;

       case 0x08:
       ADCValue[ADC_CH3][ldrSampleCount++] = ( LPC_ADC->DR[3] >> 4 ) & 0xFFF;
       if(ldrSampleCount == NUMBER_OF_SAMPLES_PER_SEC){
         ptr = &ADCValue[ADC_CH3][0];
         ldrAvgValue = MovingAvgFilter(ADC_CH3,ptr);
         ldrSampleCount = 0;
     /*	 sprintf(printBuf, "\nLDR ADC Value: %d\n",ldrAvgValue);
         PRINT_K(printBuf);*/
       }
       Chip_TIMER_Enable(ADC_SAMPLING_TIMER);
       break;
    }
  }
  return;
}
/**************************************************************/
uint16_t Get_ADCValue(uint8_t channelNum)
{
	switch(channelNum)
	{
		case 0:
			return adc0AvgValue;
		case 1:
			return adc1AvgValue;
		case 3:
			return ldrAvgValue;
		default:
			return 0;
	}
}
/**************************************************************/
uint16_t MovingAvgFilter(uint8_t adcChannel, uint16_t *adcBuffer)
{
	uint32_t sum = 0;
	uint16_t i;

	for (i = 0; i< NUMBER_OF_SAMPLES_PER_SEC; i++)
		sum += adcBuffer[i];

	return (sum / NUMBER_OF_SAMPLES_PER_SEC);
}
/******************************************************************/
void StartADCConversion(uint8_t channelNum)
{
	LPC_ADC->CR |= (1 << 24) | (1 << channelNum);
}
/******************************************************************/
void  StopADCConversion()
{
	LPC_ADC->CR &= 0xF8FFFF00;
}
/****************************************************************/

void adc_init_sampling_timer(uint32_t usecs)
{
	uint32_t timerFreq;
	//char buffer[64];

	timerFreq = Chip_Clock_GetPeripheralClockRate(SYSCTL_PCLK_TIMER0)/ 1000000;
	/*sprintf(buffer, "\nTimer Peripheral Clock: %d\n",timerFreq);
	PRINT_K(buffer);*/

	Chip_TIMER_Init(ADC_SAMPLING_TIMER);

	/* Timer setup for match and interrupt at TICKRATE_HZ */
	Chip_TIMER_Reset(ADC_SAMPLING_TIMER);
	Chip_TIMER_MatchEnableInt(ADC_SAMPLING_TIMER, 1);
	Chip_TIMER_PrescaleSet(ADC_SAMPLING_TIMER, 0);
	Chip_TIMER_SetMatch(ADC_SAMPLING_TIMER, 1, usecs * timerFreq);

	Chip_TIMER_ResetOnMatchEnable(ADC_SAMPLING_TIMER, 1);
	Chip_TIMER_Enable(ADC_SAMPLING_TIMER);

	/* Enable timer interrupt */
	NVIC_ClearPendingIRQ(TIMER0_IRQn);
	NVIC_EnableIRQ(TIMER0_IRQn);
}

/****************************************************************/

void TIMER0_IRQHandler(void)
{
	static uint8_t adcChannelNumber = 0;

	if (Chip_TIMER_MatchPending(ADC_SAMPLING_TIMER, 1)) {

		Chip_TIMER_ClearMatch(ADC_SAMPLING_TIMER, 1);
		Chip_TIMER_Disable(ADC_SAMPLING_TIMER);
		StartADCConversion(adcChannelNumber);
		adcChannelNumber++;
	/*	if(adcChannelNumber == 2){
			adcChannelNumber++;   skip second ADC channel
			return;
		}*/
		if(adcChannelNumber == NUMBER_OF_ADC_CHANNELS){
			adcChannelNumber = 0;
			return;
		}
	}
}
/****************************************************************/



