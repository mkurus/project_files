/*
 * dout_task.h
 *
 *  Created on: 28 Oca 2019
 *      Author: mkurus
 */

#ifndef DOUT_TASK_H_
#define DOUT_TASK_H_

#define DOUT_TASK_EVENT_QUE_SIZE       5

/* events processed by ignition task */
typedef enum DOUT_TASK_EVENT{
	SIGNAL_DOUT_ACTIVATE,
	SIGNAL_DOUT_DEACTIVATE,
}DOUT_TASK_SIGNAL_T;

typedef enum DOUT_TASK_STATE{
	DOUT_ACTIVATED_STATE,
	DOUT_DEACTIVATED_STATE
}DOUT_TASK_STATE_T;

/* event structure */
typedef struct{
	DOUT_TASK_SIGNAL_T  sig;
	uint8_t param1;                    /* channel number */
	uint32_t param2;                   /* pulse duration in msecs */
}DOUT_TASK_EVENT_T;

typedef struct {
	uint8_t        u8_wrtPtr;
	uint8_t        u8_readPtr;
	uint8_t        u8_count;
	DOUT_TASK_EVENT_T    tEventBuffer[DOUT_TASK_EVENT_QUE_SIZE];
}DoutTaskEventQueue;    // circular event queue


typedef struct {
	DOUT_TASK_STATE_T tTaskState;
	DoutTaskEventQueue tEventQueue;
}DoutTask;

typedef struct PULSE{
	uint32_t duration;        /* pulse duration in msecs */
	uint8_t channel;
}PULSE_T;

void PutEvent_DoutTask(DOUT_TASK_EVENT_T *tEventContainer);
void dout_task_init();
void dout_task();


#endif /* DOUT_TASK_H_ */
