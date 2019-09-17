/*
 * ign_task.c
 *
 *  Created on: 25 Eki 2017
 *      Author: mkurus
 */
#include "chip.h"
#include "board.h"
#include "bsp.h"
#include "timer.h"
#include "io_ctrl.h"
#include "status.h"
#include "ProcessTask.h"
#include "trace.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "ign_task.h"
#include "buzzer_task.h"
#include "timer_task.h"


extern TIMER_ASYNC *wifiKeepAliveTimer;
/* function prototypes */
void rfid_timeout_callback(void *param);
bool GetEvent_ProcessTask(IGN_TASK_EVENT_T *tEvent);
IgnitionTask IgnitionTask_t;

/************************************************************/
bool GetEvent_IgnitionTask(IGN_TASK_EVENT_T *tEventContainer)
{
	if(IgnitionTask_t.tEventQueue.u8_count){
		tEventContainer->sig = IgnitionTask_t.tEventQueue.tEventBuffer[IgnitionTask_t.tEventQueue.u8_readPtr].sig;
		tEventContainer->callbackPtr = IgnitionTask_t.tEventQueue.tEventBuffer[IgnitionTask_t.tEventQueue.u8_readPtr].callbackPtr;
		/* decrement event counter*/
		IgnitionTask_t.tEventQueue.u8_count--;
		/* check if reached end of queue */
		if (IgnitionTask_t.tEventQueue.u8_readPtr >= IGNITION_TASK_EVENT_QUE_SIZE - 1)
			IgnitionTask_t.tEventQueue.u8_readPtr = 0;
		else
			IgnitionTask_t.tEventQueue.u8_readPtr++;

		return true;
	}
	else
		return false;
}
/*****************************************************************************/
void PutEvent_IgnitionTask(IGN_TASK_EVENT_T *tEventContainer)
{
	if(IgnitionTask_t.tEventQueue.u8_count < IGNITION_TASK_EVENT_QUE_SIZE){
		IgnitionTask_t.tEventQueue.u8_count++;

		IgnitionTask_t.tEventQueue.tEventBuffer[IgnitionTask_t.tEventQueue.u8_wrtPtr].sig = tEventContainer->sig;
		IgnitionTask_t.tEventQueue.tEventBuffer[IgnitionTask_t.tEventQueue.u8_wrtPtr].callbackPtr = tEventContainer->callbackPtr;

		if(IgnitionTask_t.tEventQueue.u8_wrtPtr >= IGNITION_TASK_EVENT_QUE_SIZE - 1)
			IgnitionTask_t.tEventQueue.u8_wrtPtr = 0;
		else
			IgnitionTask_t.tEventQueue.u8_wrtPtr++;
	}
	else  /* critical error  */{
		PRINT_K("\nCannot write event to IgnitionTask\n");
		return;
	}

}
/*******************************************/
void ignition_task_init()
{
	if(Get_IgnitionStatus())
		IgnitionTask_t.tTaskState = IGN_ON_STATE;

	else
		IgnitionTask_t.tTaskState = IGN_OFF_STATE;


	IgnitionTask_t.tEventQueue.u8_count = 0;
	IgnitionTask_t.tEventQueue.u8_readPtr = 0;
	IgnitionTask_t.tEventQueue.u8_wrtPtr = 0;
}
/*******************************************/
void task_ignition()
{
	IGN_TASK_EVENT_T ignEvt;
	BUZZER_TASK_EVENT_T buzzEvt;
	RFID_CH_SETTING_T rfid_setting;

//	static uint32_t temp;
//	int16_t timer_id;

		if(GetEvent_IgnitionTask(&ignEvt)){
			switch(IgnitionTask_t.tTaskState){

			case IGN_ON_STATE:
				switch(ignEvt.sig){
				case SIGNAL_IGNITION_DEACTIVATED:
					PRINT_K("\nSIGNAL: IGNITION_DEACTIVATED\n");
					settings_get_rfid_check(&rfid_setting);
					if(rfid_setting.checkState == 0){
						PRINT_K("\nPosted BUZZER_IGN_OFF signal to Buzzer Task\n");
						buzzEvt.sig = SIGNAL_BUZZER_IGN_OFF;
						PutEvent_BuzzerTask(&buzzEvt);

					}
					IgnitionTask_t.tTaskState = IGN_OFF_STATE;
					break;

				default:
					break;

				}
				break;

			case IGN_OFF_STATE:
				switch(ignEvt.sig){
				case SIGNAL_IGNITION_ACTIVATED:
					PRINT_K("\nSIGNAL: IGNITION_ACTIVATED\n");
					settings_get_rfid_check(&rfid_setting);
					if(rfid_setting.checkState == 1){
						PRINT_K("\nPosted BUZZER_IGN_ON signal to Buzzer Task\n");
						buzzEvt.sig = SIGNAL_BUZZER_IGN_ON;
						PutEvent_BuzzerTask(&buzzEvt);
					}
					IgnitionTask_t.tTaskState = IGN_ON_STATE;
					break;

				default:
					break;

				}
				break;
			}
		}
}

