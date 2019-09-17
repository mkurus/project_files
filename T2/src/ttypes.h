/*
 * ttypes.h
 *
 *  Created on: 6 Ara 2016
 *      Author: admin
 */

#ifndef TTYPES_H_
#define TTYPES_H_


/* T message types */


/**********************/
typedef enum{
	REPLY_DO_NOT_SEND,
	REPLY_SEND_LOCATION,
	REPLY_ECHO_PACKET,
	REPLY_CONNECT_TO_UPDATE_SERVER
}COMMAND_RESULT_T;

#endif /* TTYPES_H_ */
