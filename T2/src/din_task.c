/*
 * din_task.c
 *
 *  Created on: 27 Eki 2017
 *      Author: mkurus
 */


#include "chip.h"
#include "board.h"
#include "bsp.h"
#include "timer.h"
#include "settings.h"
#include "status.h"
#include "ProcessTask.h"
#include "trace.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "din_task.h"
#include "gsm_task.h"


bool GetEvent_DinTask(DIN_TASK_EVENT_T *tEvent);
DinTask DinTask_t;

/**
 *
 */
bool GetEvent_DinTask(DIN_TASK_EVENT_T *tEventContainer)
{
	if(DinTask_t.tEventQueue.u8_count){
		tEventContainer->sig = DinTask_t.tEventQueue.tEventBuffer[DinTask_t.tEventQueue.u8_readPtr].sig;
		tEventContainer->param1 = DinTask_t.tEventQueue.tEventBuffer[DinTask_t.tEventQueue.u8_readPtr].param1;
		/* decrement event counter*/
		DinTask_t.tEventQueue.u8_count--;
		/* check if reached end of queue */
		if (DinTask_t.tEventQueue.u8_readPtr >= DIN_TASK_EVENT_QUE_SIZE - 1)
			DinTask_t.tEventQueue.u8_readPtr = 0;
		else
			DinTask_t.tEventQueue.u8_readPtr++;

		return true;
	}
	else
		return false;
}
/**
 *
 */
void PutEvent_DinTask(DIN_TASK_EVENT_T *tEventContainer)
{
	if(DinTask_t.tEventQueue.u8_count < DIN_TASK_EVENT_QUE_SIZE){
		DinTask_t.tEventQueue.u8_count++;

		DinTask_t.tEventQueue.tEventBuffer[DinTask_t.tEventQueue.u8_wrtPtr].sig = tEventContainer->sig;
		DinTask_t.tEventQueue.tEventBuffer[DinTask_t.tEventQueue.u8_wrtPtr].param1= tEventContainer->param1;
		if(DinTask_t.tEventQueue.u8_wrtPtr >= DIN_TASK_EVENT_QUE_SIZE - 1)
			DinTask_t.tEventQueue.u8_wrtPtr = 0;
		else
			DinTask_t.tEventQueue.u8_wrtPtr++;
	}
	else  /* critical error  */{
		PRINT_K("\nCannot write event to DinTask\n");
		return;
	}

}
/*******************************************/
void din_task_init()
{
	DinTask_t.tEventQueue.u8_count = 0;
	DinTask_t.tEventQueue.u8_readPtr = 0;
	DinTask_t.tEventQueue.u8_wrtPtr = 0;
}
/*******************************************/
void task_din()
{
	DIN_TASK_EVENT_T eventContainer;
	GSM_TASK_EVENT_T gsm_event_t;
	DIN_PROPERTIES_T din_props;
//	DIG_INPUT_FUNC_T function;
	//char temp[64];
		if(GetEvent_DinTask(&eventContainer)){

				switch(eventContainer.sig){

					case SIGNAL_DIN_ACTIVATED:

						PRINT_K("\nSIGNAL: DIN_ACTIVATED \n");
						din_props = settings_get_din_properties(eventContainer.param1);

						switch(din_props.function){

							case DIN_FUNC_NOT_ASSIGNED:
								break;

							case DIN_FUNC_SEND_SMS:
								gsm_event_t.sig = SIGNAL_GSM_TASK_SEND_SMS;
								strcpy(gsm_event_t.param.param1, din_props.destNumber);
								strcpy(gsm_event_t.param.param2, din_props.sms);
								PutEvent_GsmTask(&gsm_event_t);
								break;

							case DIN_FUNC_CALL:
								gsm_event_t.sig = SIGNAL_GSM_TASK_DIAL;
								strcpy(gsm_event_t.param.param1, din_props.destNumber);
								PutEvent_GsmTask(&gsm_event_t);
								break;

							case DIN_FUNC_SCALE:
								break;

							default:
								break;
						}
						break;

					case SIGNAL_DIN_DEACTIVATED:
						PRINT_K("\nSIGNAL: DIN_DEACTIVATED\n");
						break;
				}
		}
}

