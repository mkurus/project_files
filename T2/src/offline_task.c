/*
 * offline_task.c
 *
 *  Created on: 16 Oca 2018
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
#include "offline.h"
#include "offline_task.h"
#include "gsm_task.h"


bool GetEvent_OfflineTask(OFFLINE_TASK_EVENT_T *tEvent);
OfflineTask OfflineTask_t  __attribute__ ((section (".big_buffers")));

/**
 *
 */
bool GetEvent_OfflineTask(OFFLINE_TASK_EVENT_T *tEventContainer)
{
	if(OfflineTask_t.tEventQueue.u8_count){
		tEventContainer->sig = OfflineTask_t.tEventQueue.tEventBuffer[OfflineTask_t.tEventQueue.u8_readPtr].sig;
		memcpy(&(tEventContainer->param), &(OfflineTask_t.tEventQueue.tEventBuffer[OfflineTask_t.tEventQueue.u8_readPtr].param), sizeof(OFFLINE_PARAM_BUFFER_T));
		/* decrement event counter*/
		OfflineTask_t.tEventQueue.u8_count--;
		/* check if reached end of queue */
		if (OfflineTask_t.tEventQueue.u8_readPtr >= OFFLINE_TASK_EVENT_QUE_SIZE - 1)
			OfflineTask_t.tEventQueue.u8_readPtr = 0;
		else
			OfflineTask_t.tEventQueue.u8_readPtr++;

		return true;
	}
	else
		return false;
}
/**
 *
 */
void PutEvent_OfflineTask(OFFLINE_TASK_EVENT_T *tEventContainer)
{
	if(OfflineTask_t.tEventQueue.u8_count < OFFLINE_TASK_EVENT_QUE_SIZE){
		OfflineTask_t.tEventQueue.u8_count++;

		OfflineTask_t.tEventQueue.tEventBuffer[OfflineTask_t.tEventQueue.u8_wrtPtr].sig = tEventContainer->sig;
		memcpy(&(OfflineTask_t.tEventQueue.tEventBuffer[OfflineTask_t.tEventQueue.u8_readPtr].param), &(tEventContainer->param), sizeof(OFFLINE_PARAM_BUFFER_T));
		if(OfflineTask_t.tEventQueue.u8_wrtPtr >= OFFLINE_TASK_EVENT_QUE_SIZE - 1)
			OfflineTask_t.tEventQueue.u8_wrtPtr = 0;
		else
			OfflineTask_t.tEventQueue.u8_wrtPtr++;
	}
	else  /* critical error  */{
		PRINT_K("\nCannot write event to OfflineTask\n");
		return;
	}

}
/*******************************************/
void offline_task_init()
{
	OfflineTask_t.tTaskState = TASK_OFFLINE_STATE;
	OfflineTask_t.tEventQueue.u8_count = 0;
	OfflineTask_t.tEventQueue.u8_readPtr = 0;
	OfflineTask_t.tEventQueue.u8_wrtPtr = 0;
}
/*******************************************/
void task_offlineRev2()
{

	OFFLINE_TASK_EVENT_T offlineEvt;
	int16_t i_msgLen;
//	char tempBuf[64];
	char msg_buffer[MAX_LOG_ENTRY_SIZE];


		if(GetEvent_OfflineTask(&offlineEvt)){
		/*	sprintf(tempBuf,"\nSIGNAL number: %d\n", offlineEvt.sig);
			PRINT_K(tempBuf);*/
				switch(OfflineTask_t.tTaskState){

					case TASK_OFFLINE_STATE:
						switch(offlineEvt.sig){

							case SIGNAL_OFFLINE_TASK_ADD_LORA_MESSAGE:
								PRINT_K("SIGNAL: OFFLINE_TASK_ADD_LORA_MESSAGE\n");
								memset(msg_buffer, 0, MAX_LOG_ENTRY_SIZE);
								i_msgLen = prepare_t_message(msg_buffer, EXT_TYPE_LORA_MESSAGE, offlineEvt.param.loraMsgBuf, FALSE);
								PRINT_K(msg_buffer);
								offline_add_log_entry(msg_buffer, i_msgLen);
								break;

							case SIGNAL_OFFLINE_CONNECTED_SERVER:
								PRINT_K("SIGNAL: OFFLINE_CONNECTED_SERVER\n");
								OfflineTask_t.tTaskState = TASK_ONLINE_STATE;
								break;

							default:
								break;
						}
						break;

					case TASK_ONLINE_STATE:
						switch(offlineEvt.sig){

						case SIGNAL_OFFLINE_DISCONNECTED_SERVER:
							PRINT_K("SIGNAL: OFFLINE_DISCONNECTED_SERVER\n");
							OfflineTask_t.tTaskState = TASK_OFFLINE_STATE;
							break;

						default:
							break;
						}
						break;
				}
		}
}


