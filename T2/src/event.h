/*
 * event.h
 *
 *  Created on: 9 Ara 2016
 *      Author: admin
 */

#ifndef EVENT_H_
#define EVENT_H_

#define    T_MSG_EVENT_QUE_SIZE        6



typedef struct T_MSG_EVENT{
	char msg_type[4];
	char param1[384];
	char param2[2];
}T_MSG_EVENT_T;





void init_t_msg_event_que();
bool get_t_msg_event_status();
bool get_t_msg_event(T_MSG_EVENT_T *event);
bool put_t_msg_event(T_MSG_EVENT_T *event);

#endif /* EVENT_H_ */
