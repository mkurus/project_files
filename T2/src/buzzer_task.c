/*
 * buzzer_task.c
 *
 *  Created on: 25 Eki 2017
 *      Author: mkurus
 */


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
#include "buzzer_task.h"
#include "timer_task.h"
#include "rfid.h"


#define BUZZER_OFF_DURATION            (25)
#define BUZZER_ON_DURATION             (10)
#define BUZZER_ALLERT_DURATION         (10 * SECOND)

BuzzerTask BuzzerTask_t;
extern TIMER_ASYNC *buzzerOnOffTimer;
extern TIMER_ASYNC *buzzerActiveTimer;
/* function prototypes */
void task_buzzer();
void wrt_buzzer_task_event(uint32_t sig);
void set_buzzer_status(bool status);
bool GetEvent_BuzzerTask(BUZZER_TASK_EVENT_T *tEvent);

static void init_rfid_check_buffer();


bool rfidReadStatus[NUMBER_OF_RFID_CHANNELS];
/**
 *
 */
bool GetEvent_BuzzerTask(BUZZER_TASK_EVENT_T *tEventContainer)
{
	if(BuzzerTask_t.tEventQueue.u8_count){
		tEventContainer->sig = BuzzerTask_t.tEventQueue.tEventBuffer[BuzzerTask_t.tEventQueue.u8_readPtr].sig;
		tEventContainer->param = BuzzerTask_t.tEventQueue.tEventBuffer[BuzzerTask_t.tEventQueue.u8_readPtr].param;
		/* decrement event counter*/
		BuzzerTask_t.tEventQueue.u8_count--;
		/* check if reached end of queue */
		if (BuzzerTask_t.tEventQueue.u8_readPtr >= BUZZER_TASK_EVENT_QUE_SIZE - 1)
			BuzzerTask_t.tEventQueue.u8_readPtr = 0;
		else
			BuzzerTask_t.tEventQueue.u8_readPtr++;

		return true;
	}
	else
		return false;
}
/**
 *
 */
void PutEvent_BuzzerTask(BUZZER_TASK_EVENT_T *tEventContainer)
{
	if(BuzzerTask_t.tEventQueue.u8_count < BUZZER_TASK_EVENT_QUE_SIZE){
		BuzzerTask_t.tEventQueue.u8_count++;

		BuzzerTask_t.tEventQueue.tEventBuffer[BuzzerTask_t.tEventQueue.u8_wrtPtr].sig = tEventContainer->sig;
		BuzzerTask_t.tEventQueue.tEventBuffer[BuzzerTask_t.tEventQueue.u8_wrtPtr].param = tEventContainer->param;

		if(BuzzerTask_t.tEventQueue.u8_wrtPtr >= BUZZER_TASK_EVENT_QUE_SIZE - 1)
			BuzzerTask_t.tEventQueue.u8_wrtPtr = 0;
		else
			BuzzerTask_t.tEventQueue.u8_wrtPtr++;
	}
	else  /* critical error  */{
		PRINT_K("\nCannot write event to BuzzerTask\n");
		return;
	}

}
/**
 *
 */
void buzzer_task_init()
{
	BuzzerTask_t.tTaskState = BUZZER_OFF_STATE;
	BuzzerTask_t.tEventQueue.u8_count = 0;
	BuzzerTask_t.tEventQueue.u8_readPtr = 0;
	BuzzerTask_t.tEventQueue.u8_wrtPtr = 0;
}
/**
 *
 */
void task_buzzer()
{
	BUZZER_TASK_EVENT_T eventContainer;
	static uint32_t temp1, temp2;
	uint8_t i;

		if(GetEvent_BuzzerTask(&eventContainer)){
			switch(BuzzerTask_t.tTaskState){

			case BUZZER_ON_STATE:
				switch(eventContainer.sig){

				case SIGNAL_BUZZER_ON_TIMEOUT:
					set_buzzer_status(false);
					Timer_Start(buzzerOnOffTimer, TIMER_TYPE_ONE_SHOT,  BUZZER_OFF_DURATION, wrt_buzzer_task_event, SIGNAL_BUZZER_OFF_TIMEOUT);
					break;

				case SIGNAL_BUZZER_OFF_TIMEOUT:
					set_buzzer_status(true);
				    Timer_Start(buzzerOnOffTimer, TIMER_TYPE_ONE_SHOT,  BUZZER_ON_DURATION, wrt_buzzer_task_event, SIGNAL_BUZZER_ON_TIMEOUT);
					break;

				case SIGNAL_BUZZER_ALERT_TIMEOUT:
					PRINT_K("\nSIGNAL_BUZZER_ALERT_TIMEOUT in BUZZER_ON_STATE\n");
					set_buzzer_status(false);
					Timer_Stop(buzzerOnOffTimer);
					Timer_Stop(buzzerActiveTimer);
					BuzzerTask_t.tTaskState = BUZZER_OFF_STATE;
					break;

				case SIGNAL_BUZZER_RFID_READ:
					PRINT_K("\nSIGNAL_BUZZER_RFID_READ in BUZZER_ON_STATE\n");
					rfidReadStatus[eventContainer.param - 1] = true;
					for(i= 0; i< NUMBER_OF_RFID_CHANNELS; i++){
						if(rfidReadStatus[i] == false){
							Timer_Stop(buzzerActiveTimer);
							Timer_Start(buzzerActiveTimer, TIMER_TYPE_ONE_SHOT, settings_get_buzzer_alert_duration(), wrt_buzzer_task_event, SIGNAL_BUZZER_ALERT_TIMEOUT);
							return;
						}
					}

					set_buzzer_status(false);
					Timer_Stop(buzzerOnOffTimer);
					Timer_Stop(buzzerActiveTimer);
					BuzzerTask_t.tTaskState = BUZZER_OFF_STATE;
					break;

				default:
					break;
				}
				break;
				/**
				 *
				 */
			case BUZZER_OFF_STATE:
				switch(eventContainer.sig){

				case SIGNAL_BUZZER_IGN_ON:
				case SIGNAL_BUZZER_IGN_OFF:
					init_rfid_check_buffer();
					set_buzzer_status(false);
					Timer_Stop(buzzerOnOffTimer);
					Timer_Stop(buzzerActiveTimer);
					Timer_Start(buzzerOnOffTimer, TIMER_TYPE_ONE_SHOT, RFID_READ_TIMEOUT, wrt_buzzer_task_event, SIGNAL_BUZZER_RFID_READ_TIMEOUT);
					break;

				case SIGNAL_BUZZER_RFID_READ:
					PRINT_K("\nSIGNAL_BUZZER_RFID_READ in BUZZER_OFF_STATE\n");
					rfidReadStatus[eventContainer.param - 1] = true;
					for(i= 0; i< NUMBER_OF_RFID_CHANNELS; i++){
						if(rfidReadStatus[i] == false){
							Timer_Stop(buzzerActiveTimer);
							Timer_Start(buzzerActiveTimer, TIMER_TYPE_ONE_SHOT, settings_get_buzzer_alert_duration(), wrt_buzzer_task_event, SIGNAL_BUZZER_ALERT_TIMEOUT);
							return;
						}
					}
					Timer_Stop(buzzerOnOffTimer);
					Timer_Stop(buzzerActiveTimer);
					break;

				case SIGNAL_BUZZER_RFID_READ_TIMEOUT:
					PRINT_K("\nSIGNAL_BUZZER_RFID_READ_TIMEOUT in BUZZER_OFF_STATE\n");
					set_buzzer_status(true);
					Timer_Start( buzzerOnOffTimer, TIMER_TYPE_ONE_SHOT,  BUZZER_ON_DURATION, wrt_buzzer_task_event, SIGNAL_BUZZER_ON_TIMEOUT );
					if(settings_get_buzzer_alert_duration() > 0)
						Timer_Start(buzzerActiveTimer, TIMER_TYPE_ONE_SHOT, settings_get_buzzer_alert_duration(), wrt_buzzer_task_event, SIGNAL_BUZZER_ALERT_TIMEOUT);
					BuzzerTask_t.tTaskState = BUZZER_ON_STATE;
					break;

				default:
					break;
				}
				break;
			}
		}
}
/**
 *
 */
void set_buzzer_status(bool status)
{
	Chip_GPIO_SetPinState(LPC_GPIO, DOUT1_GPIO_PORT_NUM, DOUT1_GPIO_PIN_NUM, status);
}
/**
 *
 */
void wrt_buzzer_task_event(uint32_t sig)
{
	BUZZER_TASK_EVENT_T buzzer_event_t;
	memset(&buzzer_event_t, 0, sizeof(BUZZER_TASK_EVENT_T));
	buzzer_event_t.sig = ((BUZZER_TASK_SIGNAL_T)sig);
	PutEvent_BuzzerTask(&buzzer_event_t);
}
/**
 *
 */
static void init_rfid_check_buffer()
{
	RFID_CH_SETTING_T rfid_ch_setting;
	int i;
	settings_get_rfid_check(&rfid_ch_setting);

	for(i= 0; i < NUMBER_OF_RFID_CHANNELS; i++){
		if(!rfid_ch_setting.check_ch[i])
			rfidReadStatus[i] = true;
		else
			rfidReadStatus[i] = false;
	}

}
