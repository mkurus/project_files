/*
 * ProcessTask.c
 *
 *  Created on: 29 Şub 2016
 *      Author: admin
 */
#include "chip.h"
#include "board.h"
#include "ttypes.h"
#include "bsp.h"
#include "can.h"
#include "timer.h"
#include "utils.h"
#include "settings.h"
#include "trace.h"
#include "timer.h"
#include "timer_task.h"
#include "ProcessTask.h"
#include "MMA8652.h"
#include "gsm.h"
#include "adc.h"

#define HEARTBEAT_INTERVAL         (1  * SECOND)

TIMER_INFO_T HEARTBEAT_TIMER1;

void Process_Task(void)
{

	while(1){

    	if(!gsm_scan())
    		break;
    	task_timer();
   /* 	task_buzzer();
    	task_lora();*/
    	task_ignition();
    	task_din();
    	task_gsm();
    	task_can();
     	task_gps();
    	task_status();
    /*	task_rf();
    	task_temp_sensor();
    	task_offline();*/
    	//task_offlineRev2();
   //     task_trace();
    //    task_dout();
        checkGuardPatern();
        Chip_WWDT_Feed(LPC_WWDT);
      //  task_tablet_app();
    //   task_scale();
	}
}
