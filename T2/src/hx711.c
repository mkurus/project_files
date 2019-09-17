/*
 * hx711.c

 *
 *  Created on: 24 Mar 2017
 *      Author: mkurus
 */

#include "chip.h"
#include "board.h"
#include "bsp.h"
#include "trace.h"
#include "hx711.h"
#include "timer.h"
#include "io_ctrl.h"
#include "settings.h"
#include "timer.h"
#include <string.h>
#include "scale_task.h"


HX711_CALIB_T hx711_calib_t;
void hx711_wait_dout_high();
void hx711_wait_dout_low();
/**
 *
 */
