/*
 * tablet_app.h
 *
 *  Created on: 8 Ara 2016
 *      Author: admin
 */

#ifndef TABLET_APP_H_
#define TABLET_APP_H_

#ifndef UTILS_H_
#include "utils.h"
#endif

#define TABLET_APP_EVENT_QUE_SIZE       12
#define TABLET_MSG_START_CHAR           0x04
#define TABLET_MSG_END_CHAR             0x05
#define TABLET_APP_WAIT_ACK_TIMEOUT     (3 * SECOND)
#define TABLET_APP_MSG_MAX_RESEND       5
#define TABLET_APP_MSG_SIZE             256

#define     NUMBER_OF_ON_BOARD_SENSORS           4  /* 2 temp sensors + 2 door sensors */
typedef enum MOBILE_EVENT_TYPE{
	TYPE_NULL,
	MOBILE_APP_EVENT_MSG_FROM_SERVER,
	TYPE_ACK_FROM_TABLET,
	MOBILE_APP_EVENT_MESSAGE_READ_ACK_FROM_SERVER,
	MOBILE_APP_EVENT_ACK_FROM_TRACKER,
	MOBILE_APP_EVENT_TEXT_MSG_FROM_USER,
	MOBILE_APP_EVENT_MSG_READ_ACK,
	MOBILE_APP_EVENT_KEEPALIVE_REQUEST,
	MOBILE_APP_EVENT_XMIT_CANBUS_MESSAGE,
	MOBILE_APP_EVENT_XMIT_SENSOR_MESSAGE,
	MOBILE_APP_EVENT_XMIT_FUEL_CAP_ALARM
}MOBILE_EVENT_TYPE_T;

typedef enum TABLET_APP_STATE{
	TABLET_APP_IDLE_STATE,
	TABLET_APP_SENDING_MESSAGE_STATE,
	MOBILE_APP_WAITING_ACK_FROM_TABLET_STATE
}TABLET_APP_STATE_T;

typedef struct JSON_TYPE_TEXT{
	char message[256];
	char msg_id[16];
}JSON_TYPE_TEXT_T;

typedef struct JSON_TYPE_CANBUS{
	char rpm[16];
	char speed[16];
	char acc_pedal_pos[16];
	char fuel_level[16];
	char total_fuel[16];
	char odometer[16];
	char amb_temp[8];
	char eng_temp[8];
	char trip_fuel[16];
}JSON_TYPE_CANBUS_T;

typedef struct SENSOR{
	uint32_t value;
	uint8_t index;
	char type[8];
	char unit[4];
}SENSOR_T;

typedef struct JSON_TYPE_SENSOR{
	char id[12];
	SENSOR_T sensor[NUMBER_OF_ON_BOARD_SENSORS];
}JSON_TYPE_SENSOR_T;

typedef union JSON_DATA{
	JSON_TYPE_TEXT_T   json_type_text;
	JSON_TYPE_CANBUS_T json_type_canbus;
	JSON_TYPE_SENSOR_T json_type_sensor;
}JSON_DATA_T;

typedef struct JSON_DATA_INFO{
	char json_type[16];
	JSON_DATA_T json_data;
}JSON_DATA_INFO_T;

typedef struct TABLET_APP_EVENT{
	UART_PORT_T *uart_ptr;
	MOBILE_EVENT_TYPE_T event_type;
	JSON_DATA_T json_data_t;
}TABLET_APP_EVENT_T;
void task_tablet_app();
bool get_tablet_app_event(TABLET_APP_EVENT_T *tablet_event);
bool put_tablet_app_event(TABLET_APP_EVENT_T *tablet_event);
#endif /* TABLET_APP_H_ */
