#include "chip.h"
#include "board.h"
#include "event.h"
#include "trace.h"
#include <string.h>

T_MSG_EVENT_T t_msg_event_que[T_MSG_EVENT_QUE_SIZE];

static T_MSG_EVENT_T *get_free_event_item();

/***********************************************************/
void init_t_msg_event_que()
{
	memset(t_msg_event_que, 0, sizeof(t_msg_event_que));
}
/***********************************************************/
bool put_t_msg_event(T_MSG_EVENT_T *eventContainer)
{
	T_MSG_EVENT_T *event_t;

	event_t = get_free_event_item();

	if(event_t != NULL){
		memcpy(event_t, eventContainer, sizeof(T_MSG_EVENT_T));
		return TRUE;
	}
	return FALSE;
}
/***********************************************************/
static T_MSG_EVENT_T *get_free_event_item()
{
	int i;
	for(i=0; i<T_MSG_EVENT_QUE_SIZE; i++){
		if(strlen(t_msg_event_que[i].msg_type) == 0)
			return &t_msg_event_que[i];
	}
	return NULL;
}
/***********************************************************/
bool get_t_msg_event(T_MSG_EVENT_T *event)
{
	int i;

	for(i =0 ; i < T_MSG_EVENT_QUE_SIZE; i++){
		if(strlen(t_msg_event_que[i].msg_type) > 0){
			memcpy(event, &t_msg_event_que[i], sizeof(T_MSG_EVENT_T));
			memset(&t_msg_event_que[i], 0, sizeof(T_MSG_EVENT_T));
			return TRUE;
		}
	}
	return FALSE;
}
/***********************************************************/
bool get_t_msg_event_status()
{
	int i;
	for(i =0 ; i < T_MSG_EVENT_QUE_SIZE; i++){
		if(strlen(t_msg_event_que[i].msg_type) > 0){
			return TRUE;
		}
	}
	return FALSE;
}
