
#ifndef TIMER_H_
#define TIMER_H_

typedef uint32_t TIMER_TICK_T;

typedef struct timer_info_s {
   TIMER_TICK_T timer_start;
   TIMER_TICK_T timer_end;
   uint8_t timer_wrap;
} TIMER_INFO_T;
/**
 *
 */
typedef enum{
  TIMER_TYPE_ONE_SHOT,
  TIMER_TYPE_CONTINUOUS
}TimerType;
/**
 *
 */
typedef struct
{
	TimerType timer_type;                /* one-shot or continuous*/
	bool enabled;                    /* timer active?  */
	int8_t timer_id;
	uint32_t timer_set_value;        /* set value for timer */
	uint32_t tick_count;
	void (*p_callback)(uint32_t);    /* callback function */
	uint32_t param;                  /* parameter to callback function */
} TIMER_ASYNC;
typedef TIMER_INFO_T * PTIMER_INFO;

void wdt_init(uint16_t toutInSecs);
void mn_wait_ticks(TIMER_TICK_T num_ticks);
void Set_Timer(PTIMER_INFO, TIMER_TICK_T);
uint8_t mn_timer_expired(PTIMER_INFO);
TIMER_TICK_T mn_get_timer_tick(void);
void Delay(TIMER_TICK_T num_ticks, void (*callbackFunc)());
void Start_usTimer(uint32_t usecs);
void DelayUs(uint32_t u);
void SysTick_IsrHandler(void);
#define MN_TICK_UPDATE              ++timer_tick
#define MN_GET_TICK                 (mn_get_timer_tick())

#define SYSTICK_PER_SEC                (100)
#define WATCHDOG_TIMEOUT_IN_SECS       (150)


//#define MILISEC                     1000
#define SECOND                      (SYSTICK_PER_SEC)
#define MINUTE                      (60 * SECOND)
#define HOUR                        (60 * MINUTE)


#endif /* TIMER_H_ */
