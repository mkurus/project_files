/*
 * dout_task.c
 *
 *  Created on: 28 Oca 2019
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
#include "timer_task.h"
#include "dout_task.h"


DoutTask DoutTask_t;
extern TIMER_ASYNC *doutPulseTimer;

/* Digital output to PIO number mapping table*/

const PIN_DOUT_MAP pioToDout[] ={
	                       {1, DOUT1_GPIO_PORT_NUM, DOUT1_GPIO_PIN_NUM},
	                       {2, DOUT2_GPIO_PORT_NUM, DOUT2_GPIO_PIN_NUM}
};

/* function prototypes */
void task_dout();
void wrt_dout_task_event(uint32_t sig);
void activate_dout(uint32_t doutChNum);
void deactivate_dout(uint32_t doutChNum);
bool GetEvent_DoutTask(DOUT_TASK_EVENT_T *tEvent);

/**
 *
 */
bool GetEvent_DoutTask(DOUT_TASK_EVENT_T *tEventContainer)
{
	if(DoutTask_t.tEventQueue.u8_count){
		tEventContainer->sig = DoutTask_t.tEventQueue.tEventBuffer[DoutTask_t.tEventQueue.u8_readPtr].sig;
		tEventContainer->param1 = DoutTask_t.tEventQueue.tEventBuffer[DoutTask_t.tEventQueue.u8_readPtr].param1;
		tEventContainer->param2 = DoutTask_t.tEventQueue.tEventBuffer[DoutTask_t.tEventQueue.u8_readPtr].param2;

		/* decrement event counter*/
		DoutTask_t.tEventQueue.u8_count--;
		/* check if reached end of queue */
		if (DoutTask_t.tEventQueue.u8_readPtr >= DOUT_TASK_EVENT_QUE_SIZE - 1)
			DoutTask_t.tEventQueue.u8_readPtr = 0;
		else
			DoutTask_t.tEventQueue.u8_readPtr++;

		return true;
	}
	else
		return false;
}
/**
 *
 */
void PutEvent_DoutTask(DOUT_TASK_EVENT_T *tEventContainer)
{
	if(DoutTask_t.tEventQueue.u8_count < DOUT_TASK_EVENT_QUE_SIZE){
		DoutTask_t.tEventQueue.u8_count++;

		DoutTask_t.tEventQueue.tEventBuffer[DoutTask_t.tEventQueue.u8_wrtPtr].sig = tEventContainer->sig;
		DoutTask_t.tEventQueue.tEventBuffer[DoutTask_t.tEventQueue.u8_wrtPtr].param1 = tEventContainer->param1;
		DoutTask_t.tEventQueue.tEventBuffer[DoutTask_t.tEventQueue.u8_wrtPtr].param2 = tEventContainer->param2;

		if(DoutTask_t.tEventQueue.u8_wrtPtr >= DOUT_TASK_EVENT_QUE_SIZE - 1)
			DoutTask_t.tEventQueue.u8_wrtPtr = 0;
		else
			DoutTask_t.tEventQueue.u8_wrtPtr++;
	}
	else  /* critical error  */{
		PRINT_K("\nCannot write event to Digital Output Task\n");
		return;
	}

}
/**
 *
 */
void dout_task_init()
{
	DoutTask_t.tTaskState = DOUT_DEACTIVATED_STATE;
	DoutTask_t.tEventQueue.u8_count = 0;
	DoutTask_t.tEventQueue.u8_readPtr = 0;
	DoutTask_t.tEventQueue.u8_wrtPtr = 0;
}
/*
 *
 */
void task_dout()
{
	DOUT_TASK_EVENT_T evt;
	char printBuf[64];
	uint8_t i;

		if(GetEvent_DoutTask(&evt)){
			switch(DoutTask_t.tTaskState){

			case DOUT_DEACTIVATED_STATE:
				switch(evt.sig){

				case SIGNAL_DOUT_ACTIVATE:
					activate_dout(evt.param1);
					Timer_Start(doutPulseTimer, TIMER_TYPE_ONE_SHOT, evt.param2/SYSTICK_MS, deactivate_dout, evt.param1);
					DoutTask_t.tTaskState = DOUT_ACTIVATED_STATE;
					break;

				default:
					break;

				}
				break;

			case DOUT_ACTIVATED_STATE:
				switch(evt.sig){

				case SIGNAL_DOUT_DEACTIVATE:
					DoutTask_t.tTaskState = DOUT_DEACTIVATED_STATE;
					Chip_GPIO_SetPinState(LPC_GPIO, pioToDout[evt.param1 - 1].port, pioToDout[evt.param1 - 1].pio, false);
					break;


				default:
					break;

				}
			}
		}
}
/**
 *
 */
void activate_dout(uint32_t doutChNum)
{
	Chip_GPIO_SetPinState(LPC_GPIO, pioToDout[doutChNum - 1].port, pioToDout[doutChNum - 1].pio, true);
}
/**
 *
 */
void deactivate_dout(uint32_t doutChNum)
{
	DOUT_TASK_EVENT_T dout_event;

	dout_event.sig = SIGNAL_DOUT_DEACTIVATE;
	dout_event.param1 = doutChNum;                /* pass dout. channel number as parameter */
	PutEvent_DoutTask(&dout_event);

}
