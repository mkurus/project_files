/*
 * ign_task.h
 *
 *  Created on: 25 Eki 2017
 *      Author: mkurus
 */

#ifndef IGN_TASK_H_
#define IGN_TASK_H_

#define IGNITION_TASK_EVENT_QUE_SIZE      5


/* events processed by ignition task */
typedef enum IGNITION_TASK_EVENT{
	SIGNAL_IGNITION_ACTIVATED,
	SIGNAL_IGNITION_DEACTIVATED,
	SIGNAL_RFID_READ,
	SIGNAL_RFID_READ_TIMEOUT,
	NUM_IGNITION_SIGNALS,
}IGNITION_TASK_SIGNAL_T;

typedef enum IGNITION_TASK_STATE{
	IGN_ON_STATE,
	IGN_OFF_STATE,
}IGNITION_TASK_STATE_T;

/* event structure */
typedef struct{
	void  (*callbackPtr)(uint32_t);
	IGNITION_TASK_SIGNAL_T  sig;
}IGN_TASK_EVENT_T;

typedef struct {
	uint8_t        u8_wrtPtr;
	uint8_t        u8_readPtr;
	uint8_t        u8_count;
	IGN_TASK_EVENT_T    tEventBuffer[IGNITION_TASK_EVENT_QUE_SIZE];
}IgnitionTaskEventQueue;    // circular event queue


typedef struct {
	IGNITION_TASK_STATE_T tTaskState;
	IgnitionTaskEventQueue tEventQueue;
}IgnitionTask;

void task_ignition();
void PutEvent_IgnitionTask(IGN_TASK_EVENT_T *tEventContainer);
void ignition_task_init();
#endif /* IGN_TASK_H_ */
