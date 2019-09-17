#include "board.h"
#include "timer.h"
#include "trace.h"
#include "io_ctrl.h"
#include "adc.h"
#include "timer_task.h"
#define   LOWRES_TIMER_TICK  (11)	/* 10 ticks per second */

volatile TIMER_TICK_T timer_tick = 0;
/***************************************************************/
void  SysTick_Handler(void)  {
	MN_TICK_UPDATE;
	timer_task_update();
/*	TIMER_TASK_EVENT_T timer_event_t;

	timer_event_t.sig = TIMER_TASK_UPDATE_TIMERS_SIG;
	PutEvent_TimerTask(&timer_event_t);*/
}
/***************************************************************/
/* resets  timer                                               */
/***************************************************************/
void Set_Timer(PTIMER_INFO timer_ptr, TIMER_TICK_T num_ticks) {
	timer_ptr->timer_start = MN_GET_TICK;
	timer_ptr->timer_end = timer_ptr->timer_start + num_ticks;

	/* check for wrap */
	if (timer_ptr->timer_start <= timer_ptr->timer_end)
		timer_ptr->timer_wrap = FALSE;
	else {
		timer_ptr->timer_end = num_ticks
				- (0xffffffff - timer_ptr->timer_start + 1);
		timer_ptr->timer_wrap = TRUE;
	}
}
/********************************************************************/
/* returns 1 if a timer has expired, otherwise returns 0 */
/********************************************************************/
uint8_t mn_timer_expired(PTIMER_INFO timer_ptr) {
	TIMER_TICK_T curr_tick;

	curr_tick = MN_GET_TICK;

	if (timer_ptr->timer_wrap) {
		if ((curr_tick < timer_ptr->timer_start)
				&& (curr_tick > timer_ptr->timer_end))
			return TRUE;
	} else {
		if ((curr_tick > timer_ptr->timer_end)
				|| (curr_tick < timer_ptr->timer_start))
			return TRUE;
	}

	return FALSE;
}
/*********************************************************************/
TIMER_TICK_T mn_get_timer_tick(void) {
	volatile TIMER_TICK_T curr_tick;

	curr_tick = timer_tick;

	return (curr_tick);
}
/**********************************************************************/
/* waits for a given number of ticks. */
void Delay(TIMER_TICK_T num_ticks, void (*callbackFunc)())
{
	TIMER_INFO_T wait_timer;

	Set_Timer(&wait_timer, num_ticks);
	while (!mn_timer_expired(&wait_timer)){
		Chip_WWDT_Feed(LPC_WWDT);
		if(callbackFunc != NULL)
			callbackFunc();
	}
}
/************************************************************************/
void wdt_init(uint16_t toutInSecs)
{

}
/************************************************************************/

void DelayUs(uint32_t u)             // 4095us bekleme en fazla
{
	u = (u * 5)/2;
	LPC_TIMER1 -> IR  = 1;		// Interrupt flagi temizleniyor
	LPC_TIMER1 -> CCR = 0;		// Timer modu
	LPC_TIMER1 -> PR  = 0;   		// Prescaler oranı 0
	LPC_TIMER1 -> MR[0] = 10*u;		// 1us için 12-1=11 yükleniyor
	LPC_TIMER1 -> MCR	= 3;		// MR0 TC ile esitlendiginde kesme olusacak ve TC degeri resetlenecek

	LPC_TIMER1 -> TC  = 0;		// Timer calistiriliyor
	LPC_TIMER1 -> TCR = 1;		// Timer calistiriliyor

	while(!(LPC_TIMER1 -> IR==1));
	LPC_TIMER1 -> TCR = 0;		// Timer durduruluyor
}




