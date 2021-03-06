/*
 * lora_task.h
 *
 *  Created on: 15 Oca 2018
 *      Author: mkurus
 */

#ifndef LORA_TASK_H_
#define LORA_TASK_H_


#define  LORA_TASK_EVENT_QUE_SIZE           5

#define  LORA_MSG_DELAY_TIMEOUT             100
#define  LORA_MAX_MSG_SIZE                  128
#define  LORA_MAX_MSG_PER_PACKET            5
/* events processed by lora task */
typedef enum {
	SIGNAL_LORA_TASK_MESSAGE_RECEIVED,
	SIGNAL_LORA_TASK_MESSAGE_DELAY_TIMEOUT
}LORA_TASK_SIGNAL_T;

typedef enum LORA_TASK_STATE{
	LORA_IDLE_STATE,
	//DIN_PASSIVE_STATE
}LORA_TASK_STATE_T;

/* event structure */
typedef struct{
	LORA_TASK_SIGNAL_T  sig;
	char message[LORA_MAX_MSG_SIZE];
}LORA_TASK_EVENT_T;

typedef struct {
	uint8_t        u8_wrtPtr;
	uint8_t        u8_readPtr;
	uint8_t        u8_count;
	LORA_TASK_EVENT_T    tEventBuffer[LORA_TASK_EVENT_QUE_SIZE];
}LoraTaskEventQueue;    // circular event queue


typedef struct {
	LORA_TASK_STATE_T tTaskState;
	LoraTaskEventQueue tEventQueue;
}LoraTask;

void PutEvent_LoraTask(LORA_TASK_EVENT_T *tEventContainer);
void lora_msg_recv_callback(char *buffer, COMMAND_RESPONSE_T *response, const UART_PORT_T *src_port);
void cbLoraMessageIdleTimeout(uint32_t param);
void lora_task_init();
void lora_task();


#endif /* LORA_TASK_H_ */
