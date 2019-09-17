/*
 * timer_task.c
 *
 *  Created on: 25 Eki 2017
 *      Author: mkurus
 */

#include <string.h>
#include <stdlib.h>
#include "bsp.h"
#include "board.h"
#include "timer.h"
#include "event.h"
#include "settings.h"
#include "trace.h"
#include "status.h"
#include "utils.h"
#include <string.h>
#include "ProcessTask.h"
#include "timer_task.h"


TIMER_ASYNC timer_info[MAX_NUMBER_OF_TIMERS];

TimerTask tTimerTask;
TIMER_ASYNC *buzzerOnOffTimer  = &timer_info[0];
TIMER_ASYNC *buzzerActiveTimer = &timer_info[1];
TIMER_ASYNC *lbsQueryTimerId   = &timer_info[2];
TIMER_ASYNC *loraMsgIdleTimer  = &timer_info[3];
TIMER_ASYNC *scaleDetectionTimerId = &timer_info[4];
TIMER_ASYNC *wifiResponseTimer    = &timer_info[5];
TIMER_ASYNC *wifiTcpConnTimer     = &timer_info[6];
TIMER_ASYNC *wifiDataXmitTimer      = &timer_info[7];
TIMER_ASYNC *wifiStatusCheckTimer   =  &timer_info[8];
TIMER_ASYNC *wifiKeepAliveTimer     =  &timer_info[9];
TIMER_ASYNC *wifiRecvBufCheckTimer  =  &timer_info[10];
TIMER_ASYNC *wifiInitTimer          =  &timer_info[11];
TIMER_ASYNC *wifiTxTaskCmdTimer     =  &timer_info[12];
TIMER_ASYNC *doutPulseTimer         =  &timer_info[13];

bool GetEvent_TimerTask(TIMER_TASK_EVENT_T *tEvent);
void disable_systick_int();
void enable_systick_int();

void task_timer()
{
	TIMER_TASK_EVENT_T tEvent;

 //   void (*func_ptr)(void *);

    if(GetEvent_TimerTask(&tEvent))
	{
		switch(tEvent.sig)
		{
			case TIMER_TASK_TIMEOUT_SIG:
				tEvent.callbackPtr(tEvent.param);
				break;

		/*	case TIMER_TASK_UPDATE_TIMERS_SIG:
				timer_task_update();
				break;
*/
			default:
				break;
		}
	}
}
/*****************************************************************************/
void PutEvent_TimerTask(TIMER_TASK_EVENT_T *tEventContainer)
{
	if(tTimerTask.tEventQueue.byCount < TIMER_TASK_QUEUE_SIZE){
		tTimerTask.tEventQueue.byCount++;

		tTimerTask.tEventQueue.tEventBuffer[tTimerTask.tEventQueue.byInputPointer].sig = tEventContainer->sig;
		tTimerTask.tEventQueue.tEventBuffer[tTimerTask.tEventQueue.byInputPointer].callbackPtr = tEventContainer->callbackPtr;

		tTimerTask.tEventQueue.tEventBuffer[tTimerTask.tEventQueue.byInputPointer].param = tEventContainer->param;

		if(tTimerTask.tEventQueue.byInputPointer >= TIMER_TASK_QUEUE_SIZE - 1)
			tTimerTask.tEventQueue.byInputPointer = 0;
		else
			tTimerTask.tEventQueue.byInputPointer++;
	}
	else  /* critical error  */
	return;
}
/****************************************************************************/
bool GetEvent_TimerTask(TIMER_TASK_EVENT_T *tEvent)
{
	if(tTimerTask.tEventQueue.byCount){
		tEvent->sig =tTimerTask.tEventQueue.tEventBuffer[tTimerTask.tEventQueue.byOutputPointer].sig;
		tEvent->callbackPtr = tTimerTask.tEventQueue.tEventBuffer[tTimerTask.tEventQueue.byOutputPointer].callbackPtr;

		tEvent->param =  tTimerTask.tEventQueue.tEventBuffer[tTimerTask.tEventQueue.byOutputPointer].param;

		/* decrement event counter*/
		tTimerTask.tEventQueue.byCount--;
		/* check if reached end of queue */
		if (tTimerTask.tEventQueue.byOutputPointer >= TIMER_TASK_QUEUE_SIZE - 1)
			tTimerTask.tEventQueue.byOutputPointer = 0;
		else
			tTimerTask.tEventQueue.byOutputPointer++;

		return true;
	}
	else
		return false;
}
/****************************************************************************/
void timer_task_init(void)
{
	for(uint8_t i = 0; i < MAX_NUMBER_OF_TIMERS; i++)
	{
		timer_info[i].p_callback = NULL;
		timer_info[i].tick_count = 0;
		timer_info[i].timer_set_value = 0;  /* used only for continuous timer*/
		timer_info[i].enabled = false;
		timer_info[i].timer_id = -1;
	}
}
/*****************************************************************************/
//bool Set_Timer(uint8_t timer_id, uint8_t timer_type,uint16_t timeout_value, void  *func_ptr_on_timeout(), void * param)
void Timer_Start(TIMER_ASYNC * timer, TimerType timer_type, uint32_t timeout_value,  void (*timeout_callback)(uint32_t),
		         uint32_t iParam)
{
	disable_systick_int();

	timer->timer_type      = timer_type;
	timer->p_callback      = timeout_callback;
	timer->tick_count      = timeout_value;
	timer->timer_set_value = timeout_value;  /* used only for continuous timer*/
	timer->enabled         = true;
	timer->param           = iParam;

    enable_systick_int();

}
/**
 * Unschedules an active timer
 */
void Timer_Stop(TIMER_ASYNC *pTimer)
{
	disable_systick_int();

	pTimer->enabled = false;

	enable_systick_int();

}

/**
 * Reschedules an already running timer to the previous set value.
 * If timer already expired returns false without activating timer.
 */
bool Timer_Reschedule(int8_t timer_id)
{
	bool result = true;

	if( (timer_id >= MAX_NUMBER_OF_TIMERS)  || (timer_id < 0))
		return false;

	disable_systick_int();

	if(timer_info[timer_id].enabled == false){
		result = false;
	}
	else
		timer_info[timer_id].tick_count = timer_info[timer_id].timer_set_value;

	enable_systick_int();
	return result;
}
/********************************************************************/
/* tick timer interrupt service routine */
/********************************************************************/
void timer_task_update()
{
	TIMER_TASK_EVENT_T tEventContainer;


	for(uint8_t i = 0; i < MAX_NUMBER_OF_TIMERS; i++)
	{
		if(timer_info[i]. enabled == true)
		{
			if(--timer_info[i].tick_count == 0)
			{
				switch(timer_info[i].timer_type)
				{
					case TIMER_TYPE_ONE_SHOT:
					timer_info[i].enabled = false;
					timer_info[i].timer_id = -1;
					break;

					case TIMER_TYPE_CONTINUOUS:
					timer_info[i].enabled = true;
					timer_info[i].tick_count = timer_info[i].timer_set_value;
					break;
				}
				tEventContainer.callbackPtr = timer_info[i].p_callback;
				tEventContainer.sig = TIMER_TASK_TIMEOUT_SIG;
				tEventContainer.param= timer_info[i].param;

				PutEvent_TimerTask(&tEventContainer);
			}
		}
	}
}
void disable_systick_int()
{
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}
/**
 *
 */
void enable_systick_int()
{
	/* Enable SysTick IRQ and SysTick Timer */
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk   |  SysTick_CTRL_ENABLE_Msk;
}
