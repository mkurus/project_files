/*
 * timer_task.h
 *
 *  Created on: 25 Eki 2017
 *      Author: mkurus
 */

#ifndef TIMER_TASK_H_
#define TIMER_TASK_H_

#ifndef TIMER_H_
#include "timer.h"
#endif

#define MAX_NUMBER_OF_TIMERS                              15
#define TIMER_TASK_MAX_MSG_LENGTH                         10
#define TIMER_TASK_MIN_MSG_LENGTH                         1
#define TIMER_TASK_QUEUE_SIZE                             5




typedef enum TIMER_TASK_STATE{
	TIMER_TASK_STATE_IDLE,
}TIMER_TASK_STATE_T;

typedef enum {
	TIMER_TASK_TIMEOUT_SIG,
	TIMER_TASK_UPDATE_TIMERS_SIG
}TIMER_TASK_SIGNAL_T;

typedef struct{
	TIMER_TASK_SIGNAL_T  sig;
	void  (*callbackPtr)(uint32_t);
	uint32_t  param;
}TIMER_TASK_EVENT_T;

typedef struct {
	uint8_t        byInputPointer;
	uint8_t        byOutputPointer;
	uint8_t        byCount;
	TIMER_TASK_EVENT_T    tEventBuffer[TIMER_TASK_QUEUE_SIZE];
} TimerTaskEventQueue; // circular event queue

typedef struct {
	TIMER_TASK_STATE_T tTaskState;
	TimerTaskEventQueue tEventQueue;
}TimerTask;


void PutEvent_TimerTask(TIMER_TASK_EVENT_T *);
void Timer_Task(void);
void timer_task_init(void);
void Timer_Stop(TIMER_ASYNC *pTimer);
void Timer_Start( TIMER_ASYNC * timer, TimerType timer_type, uint32_t timer_set_value, void (*callback)(uint32_t), uint32_t);
bool Timer_Reschedule(int8_t timer_id);
void timer_task_update();


void task_timer();
void timer_task_init();

#endif /* TIMER_TASK_H_ */
