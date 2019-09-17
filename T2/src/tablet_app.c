#include "chip.h"
#include "board.h"
#include "timer.h"
#include "utils.h"
#include "trace.h"
#include "tablet_app.h"
#include "rf_task.h"
#include "event.h"
#include "messages.h"
#include "json_parser.h"
#include "msg_parser.h"
#include <string.h>
#include <stdlib.h>

TABLET_APP_EVENT_T *get_free_tablet_event_item();

TABLET_APP_STATE_T tablet_app_state = TABLET_APP_IDLE_STATE;
TABLET_APP_EVENT_T tablet_app_event_que[TABLET_APP_EVENT_QUE_SIZE];
bool send_json_sensor_data_over_uart(JSON_TYPE_SENSOR_T *json_canbus, UART_PORT_T *uart_port);
bool send_json_canbus_over_uart(JSON_TYPE_CANBUS_T *json_canbus, UART_PORT_T *uart_port);
bool send_json_fuel_cap_alarm_over_uart(JSON_TYPE_SENSOR_T *json_sensor, UART_PORT_T *uart_port);
bool send_json_packet_over_uart(JSON_DATA_INFO_T *json_data, UART_PORT_T *uart_port);
bool send_json_keepalive_over_uart(UART_PORT_T *uart_port);

/* json sensor relevant functions */
void json_add_sensor_item( char *buffer, RF_NODE_T *node, const NODE_SENSOR_TYPE_DEFS_T *sensor_def);
bool json_add_node_item(char *buffer, JSON_TYPE_SENSOR_T *json_sensor);
void json_add_sensor_value(char *buffer, RF_NODE_T *node, const NODE_SENSOR_TYPE_DEFS_T *sensor_def);
void json_add_sensor_type(char *buffer, char *type);
void json_add_sensor_unit(char *buffer, char *unit);

void json_add_sensor_index(char *buffer, uint8_t index);
bool json_create_node_item(RF_NODE_T *node, char *buffer, uint8_t node_no);

uint16_t door_value_cb(RF_NODE_T *node, uint8_t sensor_index);
uint16_t temp_value_cb(RF_NODE_T *node, uint8_t sensor_index);

static const NODE_SENSOR_TYPE_DEFS_T sensor_type_def_t[] = {
		{1,    "door",    "",  door_value_cb},
		{2,    "door",    "",  door_value_cb},
		{1,    "temp",    "C", temp_value_cb},
		{2,    "temp",    "C", temp_value_cb},
};

typedef struct CANBUS_TAG_MAPPING{
	char *tagStr;
	char *buffer;
}CANBUS_TAG_MAPPING_T;

JSON_DATA_INFO_T json_response_t;
static uint8_t msg_resend_count;
//static char tablet_msg_recv_buffer[TABLET_APP_MSG_SIZE +1];
TIMER_INFO_T keep_alive_timer;
TIMER_INFO_T msg_resend_timer;
/******************************************************/
void task_tablet_app()
{

	TABLET_APP_EVENT_T tablet_app_event;
	T_MSG_EVENT_T t_msg_event;
	static UART_PORT_T *tablet_app_port = NULL;

	memset(&t_msg_event, 0, sizeof(T_MSG_EVENT_T));

	switch(tablet_app_state){
		case TABLET_APP_IDLE_STATE:
			if(get_tablet_app_event(&tablet_app_event)){
				switch(tablet_app_event.event_type){

                    /* text message received from  server */
					case MOBILE_APP_EVENT_MSG_FROM_SERVER:
					strcpy(json_response_t.json_data.json_type_text.msg_id,
						   tablet_app_event.json_data_t.json_type_text.msg_id);
					strcpy(json_response_t.json_data.json_type_text.message,
							tablet_app_event.json_data_t.json_type_text.message);
					strcpy(json_response_t.json_type, "text");

					if(tablet_app_port != NULL)
						send_json_packet_over_uart(&json_response_t, tablet_app_port);
					tablet_app_state = MOBILE_APP_WAITING_ACK_FROM_TABLET_STATE;
					Set_Timer(&msg_resend_timer, TABLET_APP_WAIT_ACK_TIMEOUT);
					break;


					/* message read acknowledgement from server */
					case MOBILE_APP_EVENT_MESSAGE_READ_ACK_FROM_SERVER:
					PRINT_K("\nSENDING READ ACK FROM SERVER\n");
				//	tablet_app_port = tablet_app_event.uart_ptr;
					if(tablet_app_port != NULL){
						PRINT_K("\nSENDING \"drv\" to tablet\n");
						strcpy(json_response_t.json_data.json_type_text.message, "");
						strcpy(json_response_t.json_type, "drv");
						strcpy(json_response_t.json_data.json_type_text.msg_id,
						tablet_app_event.json_data_t.json_type_text.msg_id);
						send_json_packet_over_uart(&json_response_t, tablet_app_port);
					}
					break;


                    /* keep-alive request from tablet application */
					case MOBILE_APP_EVENT_KEEPALIVE_REQUEST:
					PRINT_K("\nKEEP_ALIVE received\n");
					tablet_app_port = tablet_app_event.uart_ptr;
					send_json_keepalive_over_uart(tablet_app_port);
					break;


					/* transmit canbus message timeout event */
					case MOBILE_APP_EVENT_XMIT_CANBUS_MESSAGE:
					if(tablet_app_port != NULL)
						send_json_canbus_over_uart(&tablet_app_event.json_data_t.json_type_canbus,tablet_app_port);
					break;


					/* transmit sensor data timeout event */
					case MOBILE_APP_EVENT_XMIT_SENSOR_MESSAGE:
					if(tablet_app_port != NULL)
						send_json_sensor_data_over_uart(&tablet_app_event.json_data_t.json_type_sensor, tablet_app_port);
					break;

					case MOBILE_APP_EVENT_XMIT_FUEL_CAP_ALARM:
					if(tablet_app_port != NULL)
						send_json_fuel_cap_alarm_over_uart(&tablet_app_event.json_data_t.json_type_sensor, tablet_app_port);
					break;


					/* message read by user confirmation */
					case MOBILE_APP_EVENT_MSG_READ_ACK:
					PRINT_K("\nReceived MOBILE_APP_EVENT_MSG_READ_ACK\n");
					memset(&t_msg_event, 0, sizeof(T_MSG_EVENT_T));
					strcpy(t_msg_event.msg_type, EVENT_TEXT_MESSAGE_TO_SERVER);
					strcat(t_msg_event.param1, "04,");
					strcat(t_msg_event.param1, tablet_app_event.json_data_t.json_type_text.msg_id);
					put_t_msg_event(&t_msg_event);
					break;

					/* text message from user */
					case MOBILE_APP_EVENT_TEXT_MSG_FROM_USER:
					tablet_app_port = tablet_app_event.uart_ptr;
					/* send ack. to tablet */
					if(tablet_app_port != NULL){
						PRINT_K("\nSENDING ACK to tablet\n");
						strcpy(json_response_t.json_data.json_type_text.message, "");
						strcpy(json_response_t.json_type, "ack");
						strcpy(json_response_t.json_data.json_type_text.msg_id,
								tablet_app_event.json_data_t.json_type_text.msg_id);
						send_json_packet_over_uart(&json_response_t, tablet_app_port);
					}

					PRINT_K("\nMOBILE_APP_EVENT_TEXT_MSG_FROM_USER event received\n");
					memset(&t_msg_event, 0, sizeof(T_MSG_EVENT_T));
					strcpy(t_msg_event.msg_type, EVENT_TEXT_MESSAGE_TO_SERVER);
					strcpy(t_msg_event.param1,  "01,");
					strcat(t_msg_event.param1,  json_response_t.json_data.json_type_text.msg_id);
					strcat(t_msg_event.param1,  ",");
					strcat(t_msg_event.param1,  tablet_app_event.json_data_t.json_type_text.message);
					put_t_msg_event(&t_msg_event);
					break;
				}
			}
		break;


		/*****************/
		case MOBILE_APP_WAITING_ACK_FROM_TABLET_STATE:
			if(get_tablet_app_event(&tablet_app_event)){
				switch(tablet_app_event.event_type){
					case TYPE_ACK_FROM_TABLET:
					PRINT_K("\nACK RECEIVED FROM TABLET\n");
					tablet_app_state = TABLET_APP_IDLE_STATE;
					break;

					case MOBILE_APP_EVENT_XMIT_CANBUS_MESSAGE:
					if(tablet_app_port != NULL)
						send_json_canbus_over_uart(&tablet_app_event.json_data_t.json_type_canbus,tablet_app_port);
					break;

					case MOBILE_APP_EVENT_XMIT_SENSOR_MESSAGE:
					if(tablet_app_port != NULL)
						send_json_sensor_data_over_uart(&tablet_app_event.json_data_t.json_type_sensor,tablet_app_port);
					break;

					case MOBILE_APP_EVENT_XMIT_FUEL_CAP_ALARM:
					if(tablet_app_port != NULL)
						send_json_fuel_cap_alarm_over_uart(&tablet_app_event.json_data_t.json_type_sensor, tablet_app_port);
					break;
				}
			}
			if(mn_timer_expired(&msg_resend_timer)){
				if(msg_resend_count < TABLET_APP_MSG_MAX_RESEND){
					msg_resend_count++;
					if(tablet_app_port != NULL)
						send_json_packet_over_uart(&json_response_t, tablet_app_port);
					Set_Timer(&msg_resend_timer, TABLET_APP_WAIT_ACK_TIMEOUT);
				}
				else{
					PRINT_K("\nMaximum Resend Count Reached.Discarding message\n");
					tablet_app_state = TABLET_APP_IDLE_STATE;
					msg_resend_count =0;
				}
			}
		break;
	}
}
/******************************************************/
bool send_json_canbus_over_uart(JSON_TYPE_CANBUS_T *json_canbus, UART_PORT_T *uart_port)
{
	uint16_t msg_len;
	char json_tx_buffer[512];
	int i;

	CANBUS_TAG_MAPPING_T canbus_tag_mapping_t[] ={
			{"\"rpm\":\""    			,json_canbus->rpm},
			{"\"speed\":\""  			,json_canbus->speed},
			{"\"acc_pedal_pos\":\""   	,json_canbus->acc_pedal_pos},
			{"\"fuel_level\":\""     	,json_canbus->fuel_level},
			{"\"total_fuel\":\""     	,json_canbus->total_fuel},
			{"\"odometer\":\""   	    ,json_canbus->odometer},
			{"\"amb_temp\":\""          ,json_canbus->amb_temp},
			{"\"eng_temp\":\""  	    ,json_canbus->eng_temp},
			{"\"trip_fuel\":\""  	    ,json_canbus->trip_fuel},
	};
	memset(json_tx_buffer, 0 ,sizeof(json_tx_buffer));

	json_tx_buffer[0] = TABLET_MSG_START_CHAR;
	strcat(json_tx_buffer, "{\"type\":\"canbus\",\"content\":{");
	Chip_UART_SendRB(uart_port->usart, uart_port->tx_ringbuf, json_tx_buffer, strlen(json_tx_buffer));

	for(i = 0; i < sizeof(canbus_tag_mapping_t) / sizeof(canbus_tag_mapping_t[0]); i++){
		memset(json_tx_buffer, 0, sizeof(json_tx_buffer));
		strcat(json_tx_buffer, canbus_tag_mapping_t[i].tagStr);
		strcat(json_tx_buffer, canbus_tag_mapping_t[i].buffer);
		strcat(json_tx_buffer, "\"");
		if(i < sizeof(canbus_tag_mapping_t) / sizeof(canbus_tag_mapping_t[0]) -1)
			strcat(json_tx_buffer, ",");
	    Chip_UART_SendRB(uart_port->usart, uart_port->tx_ringbuf, json_tx_buffer, strlen(json_tx_buffer));

	}
	strcpy(json_tx_buffer, "}}");
	msg_len = strlen(json_tx_buffer);
	json_tx_buffer[msg_len] = TABLET_MSG_END_CHAR;
	Chip_UART_SendRB(uart_port->usart, uart_port->tx_ringbuf, json_tx_buffer, msg_len + 1);

	return TRUE;
}
/******************************************************/
bool send_json_sensor_data_over_uart2(JSON_TYPE_SENSOR_T *json_sensor, UART_PORT_T *uart_port)
{
	char json_tx_buffer[512];
	char node_item_buf[300];
	RF_NODE_T node;
	uint16_t msg_len;
	uint8_t added_item = 0;
	int i;

	memset(json_tx_buffer, 0, sizeof(json_tx_buffer));
	json_tx_buffer[0] = TABLET_MSG_START_CHAR;

	strcat(json_tx_buffer, "{\"type\":\"sensor\",\"content\":[");
	Chip_UART_SendRB(uart_port->usart, uart_port->tx_ringbuf, json_tx_buffer, strlen(json_tx_buffer));
	memset(json_tx_buffer, 0, sizeof(json_tx_buffer));
	for (i= 0; i< MAX_NUMBER_OF_NODES; i++){
		if(rf_get_node_in_ram(&node, i)){
			memset(node_item_buf, 0, sizeof(node_item_buf));
			if(added_item > 0)
				strcat(json_tx_buffer, ",");
			strcat(json_tx_buffer, "{");
			json_add_node_item(node_item_buf, &node);
			strcat(json_tx_buffer, node_item_buf);
			strcat(json_tx_buffer, "}");
			Chip_UART_SendRB(uart_port->usart, uart_port->tx_ringbuf, json_tx_buffer, strlen(json_tx_buffer));
			memset(json_tx_buffer, 0, sizeof(json_tx_buffer));
			added_item++;
		}
	}
	strcpy(json_tx_buffer, "]}");
	msg_len = strlen(json_tx_buffer);
	json_tx_buffer[msg_len] = TABLET_MSG_END_CHAR;
	Chip_UART_SendRB(uart_port->usart, uart_port->tx_ringbuf, json_tx_buffer, msg_len + 1);
	return TRUE;
}
/***********************************************************************************************/
bool send_json_sensor_data_over_uart(JSON_TYPE_SENSOR_T *json_sensor, UART_PORT_T *uart_port)
{
	char json_tx_buffer[512];
	uint16_t msg_len;

	memset(json_tx_buffer, 0, sizeof(json_tx_buffer));

	json_tx_buffer[0] = TABLET_MSG_START_CHAR;

	strcat(json_tx_buffer, "{\"type\":\"sensor\",\"content\":{");
	json_add_node_item(json_tx_buffer, json_sensor);
	strcat(json_tx_buffer, "}}");
	msg_len = strlen(json_tx_buffer);

	json_tx_buffer[msg_len] = TABLET_MSG_END_CHAR;

	Chip_UART_SendRB(uart_port->usart, uart_port->tx_ringbuf, json_tx_buffer, msg_len + 1);

	return TRUE;
}
/***********************************************************************************************/
bool send_json_fuel_cap_alarm_over_uart(JSON_TYPE_SENSOR_T *json_sensor, UART_PORT_T *uart_port)
{
	char json_tx_buffer[256];
	uint16_t msg_len;

	memset(json_tx_buffer, 0, sizeof(json_tx_buffer));

	json_tx_buffer[0] = TABLET_MSG_START_CHAR;

	strcat(json_tx_buffer, "{\"type\":\"sensor\",\"content\":{\"Id\":\"");
	strcat(json_tx_buffer, json_sensor->id);
	strcat(json_tx_buffer, "\",\"sensors\":[{\"type\":\"fuel_cap\"}]}}");

	msg_len = strlen(json_tx_buffer);

	json_tx_buffer[msg_len] = TABLET_MSG_END_CHAR;

	Chip_UART_SendRB(uart_port->usart, uart_port->tx_ringbuf, json_tx_buffer, msg_len + 1);

	return TRUE;
}
/******************************************************/
bool json_add_node_item(char *buffer, JSON_TYPE_SENSOR_T *json_info)
{
	int i;
	char temp[16];

	strcat(buffer,"\"Id\":\"");
	strcat(buffer, json_info->id);
	strcat(buffer,"\"");

	strcat(buffer,",\"sensors\":[");
	uint8_t table_size = sizeof(sensor_type_def_t)/sizeof(sensor_type_def_t[0]);
	for(i =0; i < table_size; i++){

		strcat(buffer,"{\"type\":\"");
		strcat(buffer, json_info->sensor[i].type);
		strcat(buffer,"\",");

		strcat(buffer,"\"index\":\"");
		itoa(json_info->sensor[i].index, temp, 10);
		strcat(buffer, temp);
		strcat(buffer,"\",");

		strcat(buffer,"\"unit\":\"");
		strcat(buffer, json_info->sensor[i].unit);
		strcat(buffer,"\",");

		strcat(buffer,"\"value\":\"");
		itoa(json_info->sensor[i].value, temp, 10);
		strcat(buffer, temp);
		strcat(buffer,"\"}");
		if( i < table_size - 1)
			strcat(buffer,",");
	}
	strcat(buffer,"]");

	return TRUE;
}
/*********************************************************************/
void json_add_sensor_item(char *buffer, RF_NODE_T *node, const NODE_SENSOR_TYPE_DEFS_T *sensor_def)
{
/*	strcat(buffer,"{");
	json_add_sensor_type(buffer,  sensor_def->type);
	strcat(buffer,",");
	json_add_sensor_index(buffer, sensor_def->index);
	strcat(buffer,",");
	json_add_sensor_unit(buffer,  sensor_def->unit);
	strcat(buffer,",");
	json_add_sensor_value(buffer, node, sensor_def);
	strcat(buffer, "}");*/
}
/******************************************************/
void json_add_sensor_value(char *buffer, RF_NODE_T *node, const NODE_SENSOR_TYPE_DEFS_T *sensor_def)
{
/*	char temp[16];

	uint16_t value;
	strcat(buffer,"\"value\":\"");
	value = sensor_def->fill_value_cb(node, sensor_def->index);
	sprintf(temp,"%d",value);
	strcat(buffer,temp);

	strcat(buffer,"\"");*/
}
/******************************************************/
/*uint16_t temp_value_cb(RF_NODE_T *node, uint8_t sensor_index)
{
	return node->ds18b20_channel[sensor_index -1];
}*/
/******************************************************/
/*uint16_t door_value_cb(RF_NODE_T *node, uint8_t sensor_index)
{
	return node->door_status[sensor_index -1];
}*/
/******************************************************/
void json_add_sensor_type(char *buffer, char *type)
{
	strcat(buffer,"\"type\":\"");
	strcat(buffer, type);
	strcat(buffer,"\"");
}
/******************************************************/
void json_add_sensor_index(char *buffer, uint8_t index)
{
	char temp[16];

	strcat(buffer,"\"index\":\"");
	sprintf(temp,"%d",index);
	strcat(buffer,temp);
	strcat(buffer,"\"");
}
/******************************************************/
void json_add_sensor_unit(char *buffer, char *unit)
{
	strcat(buffer,"\"unit\":\"");
	strcat(buffer, unit);
	strcat(buffer,"\"");
}
/******************************************************/
bool send_json_keepalive_over_uart(UART_PORT_T *uart_port)
{
	char json_tx_buffer[128];
	uint16_t msg_len;

	memset(json_tx_buffer, 0 ,sizeof(json_tx_buffer));

	json_tx_buffer[0] = TABLET_MSG_START_CHAR;
	strcat(json_tx_buffer, "{\"type\":\"k_a\"}");
	msg_len = strlen(json_tx_buffer);
	json_tx_buffer[msg_len] = TABLET_MSG_END_CHAR;
	Chip_UART_SendRB(uart_port->usart, uart_port->tx_ringbuf, json_tx_buffer, strlen(json_tx_buffer));
	return TRUE;
}
/******************************************************/
/*bool send_json_ack_over_uart(JSON_DATA_INFO_T *json_data, UART_PORT_T *uart_port)
{
	uint16_t msg_len;
	char json_tx_buffer[128];

	json_tx_buffer[0] = TABLET_MSG_START_CHAR;

	strcat(json_tx_buffer, "{\"type\":\"");
	strcat(json_tx_buffer, json_data->json_type);
	strcat(json_tx_buffer,"\",\"content\":{\"id\":\"");
	strcat(json_tx_buffer,json_data->json_data.json_type_text.msg_id);
	strcat(json_tx_buffer,"\"}}");
	msg_len = strlen(json_tx_buffer);

	json_tx_buffer[msg_len] = TABLET_MSG_END_CHAR;

	Chip_UART_SendRB(uart_port->usart, uart_port->tx_ringbuf, json_tx_buffer, msg_len + 1);
	return TRUE;
}*/
/******************************************************/
bool send_json_packet_over_uart(JSON_DATA_INFO_T *json_data, UART_PORT_T *uart_port)
{
	char json_tx_buffer[256];
	uint16_t msg_len;

	memset(json_tx_buffer, 0 ,sizeof(json_tx_buffer));

	json_tx_buffer[0] = TABLET_MSG_START_CHAR;

	strcat(json_tx_buffer,  "{\"type\":\"");
	strcat(json_tx_buffer, json_data->json_type);
	strcat(json_tx_buffer,"\",\"content\":{");

	if(strlen(json_data->json_data.json_type_text.message) > 0){
		strcat(json_tx_buffer,"\"msg\":\"");
		strcat(json_tx_buffer,json_data->json_data.json_type_text.message);
		strcat(json_tx_buffer,"\",");
	}
	if(strlen(json_data->json_data.json_type_text.msg_id) > 0){
		strcat(json_tx_buffer,"\"id\":\"");
		strcat(json_tx_buffer,json_data->json_data.json_type_text.msg_id);
		strcat(json_tx_buffer,"\"}}");
	}

	msg_len = strlen(json_tx_buffer);
	json_tx_buffer[msg_len] = TABLET_MSG_END_CHAR;
//	PRINT_K(json_tx_buffer);
	Chip_UART_SendRB(uart_port->usart, uart_port->tx_ringbuf, json_tx_buffer, msg_len + 1);
	return TRUE;
}
/******************************************************/
bool put_tablet_app_event(TABLET_APP_EVENT_T *tablet_event)
{
	TABLET_APP_EVENT_T *temp_event;
	temp_event = get_free_tablet_event_item();
	if(temp_event != NULL){
		memcpy(temp_event, tablet_event, sizeof(TABLET_APP_EVENT_T));
		return TRUE;
	}
	return FALSE;
}
/******************************************************/
TABLET_APP_EVENT_T *get_free_tablet_event_item()
{
	int i;
	for(i=0; i<TABLET_APP_EVENT_QUE_SIZE; i++){
		if(tablet_app_event_que[i].event_type == TYPE_NULL){
			return &tablet_app_event_que[i];
		}
	}
	return NULL;
}
/******************************************************/
bool get_tablet_app_event(TABLET_APP_EVENT_T *tablet_event)
{
	int i;
	return FALSE;
	for(i =0 ; i < TABLET_APP_EVENT_QUE_SIZE; i++){
		if(tablet_app_event_que[i].event_type != TYPE_NULL){
			memcpy(tablet_event, &tablet_app_event_que[i], sizeof(TABLET_APP_EVENT_T));
			tablet_app_event_que[i].event_type = TYPE_NULL;
			return TRUE;
		}
	}
	return FALSE;
}
