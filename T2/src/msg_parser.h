/*
 * tablet_msg.h
 *
 *  Created on: 6 Ara 2016
 *      Author: admin
 */

#ifndef MSG_PARSER_H_
#define MSG_PARSER_H_

#ifndef JSON_PARSER_H_
#include "json_parser.h"
#endif

#define JSON_TEXT_TYPE_CONTENT_SIZE           2
#define JSON_DRV_TYPE_CONTENT_SIZE            1
#define JFON_KEEPALIVE_TYPE_CONTENT_SIZE      1

/* callback function prototypes for different message types */
void rfid_msg_recv_callback(char *msg, COMMAND_RESPONSE_T *response, const UART_PORT_T *src_uart);
void set_msg_recv_callback(char *msg, COMMAND_RESPONSE_T *response,  const UART_PORT_T *src_uart);
void tablet_msg_recv_callback(char *msg, COMMAND_RESPONSE_T *response, const  UART_PORT_T *src_uart);

/*uint16_t json_type_object_callback(jsmntok_t *p_object, char *msg);
uint16_t json_content_object_callback(jsmntok_t *p_object, char *msg);*/
TABLET_APP_EVENT_T json_text_type_parser_callback(jsmntok_t *p_object, char *msg,int tagCount);
TABLET_APP_EVENT_T json_ack_type_parser_callback(jsmntok_t *p_object, char *msg,int tagCount);
TABLET_APP_EVENT_T json_drv_type_parser_callback(jsmntok_t *p_object, char *msg, int tagCount);
TABLET_APP_EVENT_T json_keep_alive_type_parser_callback(jsmntok_t *p_object, char *msg,int tagCount);




typedef struct JSON_OBJECT_TYPES{
	char json_obj_type[64];
	TABLET_APP_EVENT_T (*json_type_object_callback)(jsmntok_t *object, char * msg, int tagCount);
}JSON_OBJECT_TYPES_T;

typedef struct JSON_OBJECT_STR{
	char json_obj_name[64];
	uint16_t (*json_object_callback)(jsmntok_t *object, char *msg);
}JSON_OBJECT_STR_T;

static const JSON_OBJECT_TYPES_T json_obj_types_t[] =
{
		{"text",   json_text_type_parser_callback},
		{"ack",    json_ack_type_parser_callback},
		{"drv",    json_drv_type_parser_callback},
		{"k_a",    json_keep_alive_type_parser_callback}
	/*  {"canbus", json_canbus_type_parser_callback},
		{"sensor", json_sensor_type_parser_callback}*/
};

static const JSON_OBJECT_STR_T json_object_str_t[] =
{
	//	{"type",    json_type_object_callback},
	//	{"content", json_content_object_callback},
};
#endif /* MSG_PARSER_H_ */
