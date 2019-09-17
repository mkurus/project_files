/*
 * buzzer_task.h
 *
 *  Created on: 25 Eki 2017
 *      Author: mkurus
 */

#ifndef BUZZER_TASK_H_
#define BUZZER_TASK_H_


#define BUZZER_TASK_EVENT_QUE_SIZE      3
#define RFID_READ_TIMEOUT              (3 *SECOND)

/* events processed by ignition task */
typedef enum BUZZER_TASK_EVENT{
	SIGNAL_BUZZER_ACTIVATE,
	SIGNAL_BUZZER_DEACTIVATE,
	SIGNAL_START_BUZZER_ALERT,
	SIGNAL_BUZZER_IGN_ON,
	SIGNAL_BUZZER_IGN_OFF,
	SIGNAL_BUZZER_RFID_READ_TIMEOUT,
	SIGNAL_BUZZER_ALERT_TIMEOUT,
	SIGNAL_BUZZER_RFID_READ,
	SIGNAL_BUZZER_OFF_TIMEOUT,
	SIGNAL_BUZZER_ON_TIMEOUT
}BUZZER_TASK_SIGNAL_T;

typedef enum BUZZER_TASK_STATE{
	BUZZER_OFF_STATE,
	BUZZER_ON_STATE
}BUZZER_TASK_STATE_T;

/* event structure */
typedef struct{
	BUZZER_TASK_SIGNAL_T  sig;
	uint8_t param;
}BUZZER_TASK_EVENT_T;

typedef struct {
	uint8_t        u8_wrtPtr;
	uint8_t        u8_readPtr;
	uint8_t        u8_count;
	BUZZER_TASK_EVENT_T    tEventBuffer[BUZZER_TASK_EVENT_QUE_SIZE];
}BuzzerTaskEventQueue;    // circular event queue


typedef struct {
	BUZZER_TASK_STATE_T tTaskState;
	BuzzerTaskEventQueue tEventQueue;
}BuzzerTask;

void PutEvent_BuzzerTask(BUZZER_TASK_EVENT_T *tEventContainer);
void buzzer_task_init();
void buzzer_task();

#endif /* BUZZER_TASK_H_ */
