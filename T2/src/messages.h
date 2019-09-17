/*
 * messages.h
 *
 *  Created on: 2 Mar 2016
 *      Author: admin
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_

#ifndef   GSM_H_
#include "gsm.h"
#endif

#ifndef TTYPES_H_
#include "ttypes.h"
#endif

#ifndef TABLET_APP_H
#include "tablet_app.h"
#endif

#ifndef RF_TASK_H_
#include "rf_task.h"
#endif

#ifndef EVENT_H_
#include "event.h"
#endif

#define  LORA_MSG_START_CHAR           0x06
#define  LORA_MSG_END_CHAR             0x07
#define  MAX_SPEED_IN_T_MSG    255
#define  MAX_COURSE_IN_T_MSG   360

#define  P_MSG_START_STR       "[P"
#define  T_MSG_START_STR       "[T"
#define  T_MSG_END_STR         "]"
#define  T_MSG_EXT_BEGIN_STR   "-"
#define  T_MSG_EXT_SEPERATOR   ";"
#define  T_MSG_PARAM_BEGIN_CHAR  ":"
#define  T_MSG_PARAM_SEPER_CHAR  ","
#define  MAX_T_MSG_FIELD_SIZE    256
#define  MAX_T_MESSAGE_SIZE        (QUECTEL_MAX_TCP_SEND_SIZE - sizeof(T_MSG_START_STR) - sizeof(T_MSG_END_STR))

/* 50 ye kadar digital girişler için reserve */
#define EVENT_DIGITAL_IN1_DEACTIVATED    "001"
#define EVENT_DIGITAL_IN1_ACTIVATED      "002"
#define EVENT_DIGITAL_IN2_DEACTIVATED    "003"
#define EVENT_DIGITAL_IN2_ACTIVATED      "004"
#define EVENT_TEXT_MESSAGE_TO_SERVER     "029"
#define EVENT_DIGITAL_IN3_DEACTIVATED    "031"
#define EVENT_DIGITAL_IN3_ACTIVATED      "032"
#define EVENT_DIGITAL_IN4_DEACTIVATED    "033"
#define EVENT_DIGITAL_IN4_ACTIVATED      "034"
#define EVENT_DIGITAL_IN5_DEACTIVATED    "035"
#define EVENT_DIGITAL_IN5_ACTIVATED      "036"
#define EVENT_DIGITAL_IN6_DEACTIVATED    "037"
#define EVENT_DIGITAL_IN6_ACTIVATED      "038"
#define EVENT_SPEED_LIMIT_VIOLATED       "005"
#define EVENT_RFID_CARD_READING          "007"
#define EVENT_STOP_TIME_ALARM            "00C"
#define EVENT_EXT_PWR_STATUS_CHANGED     "00D"
#define EVENT_IGN_STATUS_CHANGED         "024"
#define EVENT_SIM_REMOVED                "028"
#define EVENT_COVER_OPENED               "040"
#define EVENT_IDLE_ALARM                 "051"
#define EVENT_ACCELERATION               "052"
#define EVENT_DECELERATION               "053"
#define EVENT_RF_SENSOR                  "106"
#define EVENT_TRAILER_IDENTIFIED         "108"
#define EVENT_FUEL_CAP_REMOVED           "109"
#define EVENT_LORA_MESSAGE               "115"
#define EVENT_G_SENSOR                   "666"


#define EVENT_T_MSG_TRIGGER              "FFF"

typedef enum{
	CANBUS_DATA,
	TEMP_SENSOR_DATA,
	BATTERY_LEVEL,
	OFFLINE_DATA,
	ANALOG_INPUT_READING,
	GARMIN_NAVIGATION,
	CELL_ID,
	TRIP_INFO,
	GPS_INFO,
	ROAMING_DATA,
	INTER_MESSAGE_IDLE_TIME,
	DIGITAL_INPUT_STATUS = 0xA0,
	RF_NODE_DATA,
	TRAILER_IDENT,
	SCALE_INFO,
	LBS_INFO,
	ENGINE_HOUR_VALUE
}MESSAGE_TYPE_T;

/* T message extension types*/
typedef enum EXT_MESSAGE{
	EXT_TYPE_LORA_MESSAGE
}EXT_MESSAGE_T;

typedef struct COMMAND_RESPONSE{
	char buffer[MAX_T_MESSAGE_SIZE];
	bool b_needToReset;
} COMMAND_RESPONSE_T;

int32_t PrepareBlockageEchoPacket(char *buffer);
void Trio_PrepareTMessageExtension(char *u8_tMessage, MESSAGE_TYPE_T message_type);
void Trio_PrepareTMessageExtension2(char *buffer, T_MSG_EVENT_T *t_msg_event_t);
void Trio_PrepareRfMessageExtension(char * msgBuffer, RF_NODE_T *node, bool add_comma);
void Trio_AddTMessageExtensionSeperator(char *bufPtr);
void Trio_BeginTMessageExtension(char *bufPtr);
void Trio_BeginTMessage(char *msgBuffer, bool b_online);
void Trio_EndTMessage(char *pBuffer);
void Trio_StartTripInfoMessage(char *bufPtr);
//int32_t Trio_PreparePingMessage(char *pBuffer, const char *imei_no);
int32_t Trio_PreparePingMessage(char *pBuffer);
int32_t Trio_PrepareSTMessage(char *pBuffer, uint16_t bufSize);
int32_t Trio_PrepareTMessage(char *msgBuffer, bool b_online);
bool Trio_AddTMessageItem(char *dest_buffer, char * msg_item);
void update_rfid_msg_buffer(char *msg);
int16_t prepare_t_message(char *msgBuffer, EXT_MESSAGE_T extension_type, char *message , bool b_online);
void Trio_PrepareTMessageExtensionVer2(char *u8_tMessage, char *message, EXT_MESSAGE_T message_type);
#endif /* MESSAGES_H_ */
