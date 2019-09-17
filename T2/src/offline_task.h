/*
 * offline_task.h
 *
 *  Created on: 16 Oca 2018
 *      Author: mkurus
 */

#ifndef OFFLINE_TASK_H_
#define OFFLINE_TASK_H_

#ifndef LORA_TASK_H_
#include "lora_task.h"
#endif

#define OFFLINE_TASK_EVENT_QUE_SIZE      5

/* events processed by ignition task */
typedef enum OFFLINE_TASK_EVENT{
	SIGNAL_OFFLINE_TASK_ADD_LORA_MESSAGE,
	SIGNAL_OFFLINE_CONNECTED_SERVER,
	SIGNAL_OFFLINE_DISCONNECTED_SERVER
}OFFLINE_TASK_SIGNAL_T;

typedef enum OFFLINE_TASK_STATE{
	TASK_OFFLINE_STATE,
	TASK_ONLINE_STATE

}OFFLINE_TASK_STATE_T;

typedef union {
	char  loraMsgBuf[LORA_MAX_MSG_SIZE * LORA_MAX_MSG_PER_PACKET];   /* lora message buffer */
}OFFLINE_PARAM_BUFFER_T;

/* event structure */
typedef struct{
	OFFLINE_TASK_SIGNAL_T  sig;
	OFFLINE_PARAM_BUFFER_T param;
}OFFLINE_TASK_EVENT_T;

typedef struct {
	uint8_t        u8_wrtPtr;
	uint8_t        u8_readPtr;
	uint8_t        u8_count;
	OFFLINE_TASK_EVENT_T    tEventBuffer[OFFLINE_TASK_EVENT_QUE_SIZE];
}OfflineTaskEventQueue;    // circular event queue


typedef struct {
	OFFLINE_TASK_STATE_T tTaskState;
	OfflineTaskEventQueue tEventQueue;
}OfflineTask;

void PutEvent_OfflineTask(OFFLINE_TASK_EVENT_T *tEventContainer);
void offline_task_init();
void task_offlineRev2();
void offline_data_init();

#endif /* OFFLINE_TASK_H_ */
