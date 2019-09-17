/*
 * din_task.h
 *
 *  Created on: 27 Eki 2017
 *      Author: mkurus
 */

#ifndef DIN_TASK_H_
#define DIN_TASK_H_

#define    DIN_TASK_EVENT_QUE_SIZE           5





/* events processed by ignition task */
typedef enum {
	SIGNAL_DIN_ACTIVATED,
	SIGNAL_DIN_DEACTIVATED
}DIN_TASK_SIGNAL_T;

typedef enum DIN_TASK_STATE{
	DIN_ACTIVE_STATE,
	DIN_PASSIVE_STATE
}DIN_TASK_STATE_T;

/* event structure */
typedef struct{
	DIN_TASK_SIGNAL_T  sig;
	DIG_INPUT_CH_NUM   param1;                  /* digital input channel number */
	uint8_t param2;
}DIN_TASK_EVENT_T;

typedef struct {
	uint8_t        u8_wrtPtr;
	uint8_t        u8_readPtr;
	uint8_t        u8_count;
	DIN_TASK_EVENT_T    tEventBuffer[DIN_TASK_EVENT_QUE_SIZE];
}DinTaskEventQueue;    // circular event queue


typedef struct {
	DIN_TASK_STATE_T tTaskState;
	DinTaskEventQueue tEventQueue;
}DinTask;

void PutEvent_DinTask(DIN_TASK_EVENT_T *tEventContainer);
void din_task_init();
void task_din();



#endif /* DIN_TASK_H_ */
