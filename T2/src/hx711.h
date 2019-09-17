/*
 * hx711.h
 *
 *  Created on: 24 Mar 2017
 *      Author: mkurus
 */

#ifndef HX711_H_
#define HX711_H_

#ifndef IO_CTRL_H_
#include "io_ctrl.h"
#endif

#define    HX711_DAT                     DIN1_PIN_NUM
#define    HX711_CLK	                 DOUT1_GPIO_PIN_NUM
#define    CLK_PULSE_COUNT               24

#define    HX711_DATA_LINE_TIMEOUT       25
#define    HX711_DELAY_AFTER_RESET       10
#define    HX711_CLK_PLS_LENGTH          5

typedef struct HX711_CONFIG{
	uint8_t u8_dataPort;
	uint8_t u8_clkPort;
	uint8_t u8_dataPin;
	uint8_t u8_clkPin;
}HX711_CONFIG_T;

/* calibration coefficents according to y= mx+ c formula */
typedef struct HX711_CALIB{
	float coeff_m;
	int32_t coeff_c;
}HX711_CALIB_T;

HX711_CALIB_T hx711_calibrate(int avg_count, int ref_weigth);
bool hx711_read_adc(int32_t *result);
bool hx711_calc_load(int32_t *result);
bool hx711_get_data();
void hx711_init();
void hx711_set_clk(bool status);


#endif /* HX711_H_ */
