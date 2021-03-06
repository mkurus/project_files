#include "chip.h"
#include "board.h"
#include "timer.h"
#include "bsp.h"
#include "utils.h"
#include "settings.h"
#include "gsm.h"
#include "gps.h"
#include "sst25.h"
#include "trace.h"
#include "messages.h"
#include "offline.h"
#include "sst25.h"
#include "spi.h"
#include "tablet_app.h"
#include "status.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "din_task.h"
#include "rfid.h"
#include "dout_task.h"
#include "gsm_task.h"

#define   SETTINGS_UPDATE_RETRY_COUNT           5

extern PIN_DOUT_MAP pioToDout[];

bool validate_password(char *pswd);
bool validate_node_id(char *node_id);
bool ExtractCommandPassword(char *p_pswdStart, char *pswdBuf);

void Get_ConfigParam(char * const in_buffer, char *out_buffer, uint16_t size, char *sep_char);

void BeginEchoPacket(char *buffer, COMMAND_TYPE_T command);
void CloseEchoPacket(char *buffer);
void AddStringToEchoPacket(char *buffer, char *strToAdd);

COMMAND_TYPE_T Get_ConfigCommand(char *buffer);
COMMAND_RESULT_T ParseConfigurationString(char *const buffer, COMMAND_RESPONSE_T *response, COMMAND_SOURCE_T cmdSource);
COMMAND_RESULT_T ParseGetCommand(char *const buffer, COMMAND_RESPONSE_T *response, COMMAND_SOURCE_T cmdSource);


/* callback functions for flash write operations*/
static COMMAND_RESULT_T settings_update_server_and_port(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T update_message_period_setting(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T update_roaming_setting(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T update_sms_act_setting(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T update_speed_limit_setting(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T update_engine_blockage_setting(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T update_km_counter_setting(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T update_device_id(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T update_gps_baudrate_setting(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T reset_device(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T send_sms_command(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T request_device_status(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T uncontrolled_engine_blockage(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T location_request(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T update_apn_setting(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T update_password(char * buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T update_firmware(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T gps_cold_restart(char * buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T erase_external_flash(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T update_blockage_persistance(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T update_idle_alarm_setting(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T update_can_bitrate(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T update_can_frame_type(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T settings_add_rf_node(char * buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T settings_remove_rf_node(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T settings_list_rf_nodes(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T text_message_from_server(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T text_msg_read_ack_from_server(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T settings_update_g_sensor_threshold(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T settings_update_rfid_check(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T settings_update_din_func(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T settings_hx711_calibrate(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T settings_activate_lbs(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T settings_set_output_status(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T settings_activate_engine_hour_counter(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T settings_generate_output_pulse(char *buffer, COMMAND_RESPONSE_T *response);
/* get commands */
static COMMAND_RESULT_T get_healt_status(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T get_version_info(char *buffer, COMMAND_RESPONSE_T *response);
static COMMAND_RESULT_T get_config_info(char *buffer, COMMAND_RESPONSE_T *response);
static uint32_t u32_flashKmCounter = 0;

/**
 *  default user settings
 */
const FLASH_SETTINGS_T default_settings = {
		"internet",
		"vodafone",
		"vodafone",
		"bodrum.triomobil.com",	//"178.63.30.80",
		"6081",
		"1234567890ABCDE",
		"ATR33",
		TRUE,        /* keep blockage status in RAM */
		TRUE,        /* roaming */
		TRUE,        /* sms      */
		FALSE,       /* blocked?  */
		(1  * 60),
		(60 * 60),
		5,
		(60 * 60),
		120,       /* speed limit in km */
		10,        /* speed limit violation duration */
		(25),
		60,
		CAN_BUSSPEED_250K,   /* For Axor vehicles use 500K and 11- bit frame */
		CAN_EXT_FRAME,
		0xFFFFFFFF,        /* gps baud rate */
		4,
		{ 0 ,{1, 0}, 10 },
		{1.0 , 0},         // default calibration coefficents m = 1.0 c= 0
		1,                 // lbs
		0,                 // engine counter activation status
		0x00000000         // checksum
};
/*
 * default user setting for digital inputs
 * function    = No task assigned (0)
 * gsm number  = Empty ("")
 * sms         = Empty ("")
 *
 * */
const DIN_PROPERTIES_T  default_din_settings[] = {
		{ 0, "", ""}, { 0, "", ""}, { 0, "", ""},{ 0, "", ""},{ 0, "", ""},{ 0, "", ""}
};
/**
 *
 */
const SETTING_INFO_T settings_info[] =
{
	{ SERVER_AND_PORT_SETTING,       settings_update_server_and_port},
    { MESSAGE_PERIOD_SETTING,        update_message_period_setting },
/*	{ ROAMING_ACTIVATION,            update_roaming_setting },*/
/*	{ SMS_ACTIVATION,                update_sms_act_setting },*/
	{ IDLE_ALARM_SETTING,            update_idle_alarm_setting},
	{ SPEED_LIMIT_SETTING,           update_speed_limit_setting },
	{ ENGINE_BLOCKAGE_SETTING,       update_engine_blockage_setting},
	{ KM_COUNTER,                    update_km_counter_setting},
	{ DEVICE_RESET,                  reset_device },
	{ SEND_SMS,                      send_sms_command },
	{ DEVICE_ID_SETTING,             update_device_id },
	{ DEVICE_STATUS_REQUEST,         request_device_status },
	{ LOCATION_REQUEST,              location_request },
	{ APN_SETTING,                   update_apn_setting },
	{ RFID_CHECK_SETTING,            settings_update_rfid_check },
	{ CHANGE_PASSWORD,               update_password },
	{ FIRMWARE_UPDATE_REQUEST,       update_firmware},
	{ GPS_COLD_RESTART,              gps_cold_restart},
	{ ERASE_EXT_FLASH,               erase_external_flash},
	{ CAN_BUSSPEED_SETTING,          update_can_bitrate},
	{ CAN_FRAMETYPE_SETTING,         update_can_frame_type},
	{ DIN_SETTING,                   settings_update_din_func},
	{ GPS_BAUDRATE_SETTING,          update_gps_baudrate_setting},
	{ SET_BLOCKAGE_PERSISTANCE,      update_blockage_persistance},
	{ ADD_RF_NODE,                   settings_add_rf_node},
	{ REMOVE_RF_NODE,                settings_remove_rf_node},
	{ LIST_RF_NODES,                 settings_list_rf_nodes},
//	{ TEXT_MSG_FROM_SERVER,          text_message_from_server},
//	{ TEXT_MSG_READ_ACK_FROM_SERVER, text_msg_read_ack_from_server},
	{ UPDATE_G_THRESHOLD,            settings_update_g_sensor_threshold},
//	{ CALIBRATE_HX711,               settings_hx711_calibrate},
	{ ACTIVATE_LBS,                  settings_activate_lbs},
	{ SET_OUTPUT_STATUS,             settings_set_output_status},
	{ ACTIVATE_ENGINE_HOUR_COUNTER,  settings_activate_engine_hour_counter},
	{ SET_CMD_DOUT_PULSE,            settings_generate_output_pulse}

};

/**
 *
 */
const GET_CMD_INFO_T get_cmd[] =
{
	{ DEVICE_HEALT_STATUS,      get_healt_status},
	{ DEVICE_VERSION_INFO,      get_version_info},
	{ DEVICE_CONFIG_INFO,       get_config_info}
};

FLASH_SETTINGS_T flash_settings;
BLOCKAGE_INFO_T  blockage_info;
REG_NODE_INFO_T reg_node_table[MAX_NUMBER_OF_NODES];
char rfid_msg_buffer[64];
DIN_PROPERTIES_T din_func_table[NUM_DIGITAL_INPUTS];
bool requestConnectToUpdateServer = FALSE;
/*
 *
 */
COMMAND_RESULT_T ParseConfigurationString(char *buffer, COMMAND_RESPONSE_T *response, COMMAND_SOURCE_T cmdSource)
{
	COMMAND_TYPE_T command;
	COMMAND_RESULT_T result = REPLY_DO_NOT_SEND;
	uint32_t i;

	command = Get_ConfigCommand(buffer);
	for(i = 0; i < sizeof(settings_info) / sizeof(settings_info[0]); i++ ){
		if(command == settings_info[i].command){
			result = settings_info[i].update_callback(buffer, response);
			if(command == ENGINE_BLOCKAGE_SETTING)
				Set_BlockageSource(cmdSource);
			break;
		}
	}
	return result;
}
/**
 *
 */
COMMAND_RESULT_T ParseGetCommand(char *buffer, COMMAND_RESPONSE_T *response, COMMAND_SOURCE_T cmdSource)
{
	GET_COMMAND_TYPE command;
	COMMAND_RESULT_T result = REPLY_DO_NOT_SEND;
	uint32_t i;

	command = Get_ConfigCommand(buffer);
	for(i = 0; i < sizeof(get_cmd) / sizeof(get_cmd[0]); i++ ){
		if(command == get_cmd[i].command){
			result = get_cmd[i].get_callback(buffer, response);
			break;
		}
	}
	return result;
}
/**
 *
 */
COMMAND_TYPE_T Get_ConfigCommand(char *buffer)
{
	 char *pTemp;

	 pTemp = strtok (buffer, CONFIG_PARAM_SEPERATOR);
	 if(pTemp != NULL){
		 return (COMMAND_TYPE_T)atoi(buffer);
	 }
	 else
		 return (COMMAND_TYPE_T)0;
}
/**
 *
 */
void Get_ConfigParam(char * const in_buffer, char *out_buffer, uint16_t out_buffer_size, char *seperator)
{
	char *pTemp;
	pTemp = strtok(NULL, seperator);
	strncpy(out_buffer, pTemp, out_buffer_size);
}
/**
 *
 */
COMMAND_RESULT_T settings_update_server_and_port(char *buffer, COMMAND_RESPONSE_T *response)
{
	char transactionID[16];


	/* extract server ip address*/
	Get_ConfigParam(NULL, flash_settings.server_ip, sizeof(flash_settings.server_ip), CONFIG_PARAM_SEPERATOR);
	/* extract port number*/
	Get_ConfigParam(NULL, flash_settings.server_port, sizeof(flash_settings.server_port), CONFIG_PARAM_SEPERATOR);
	/* extract transaction id*/
	Get_ConfigParam(NULL, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);

	settings_update_all(&flash_settings);

	BeginEchoPacket(response->buffer, SERVER_AND_PORT_SETTING);
	AddStringToEchoPacket(response->buffer, flash_settings.server_ip);
	AddStringToEchoPacket(response->buffer, flash_settings.server_port);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);
	response->b_needToReset = TRUE;
	return REPLY_ECHO_PACKET;
}
/**
 * Sets message period for roaming/not-roaming states
 */
COMMAND_RESULT_T update_message_period_setting(char *buffer, COMMAND_RESPONSE_T *response)
{
	char temp[16];
	char command;
	uint32_t val1, val2;
	bool valid = FALSE;
	char transactionID[16];

//	PRINT_K("\nIn update_message_period_setting\n");
//	PRINT_K(buffer);

	Get_ConfigParam(NULL, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);
	command = temp[0];
	Get_ConfigParam(NULL, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);
	val1 = atoi(temp);
	Get_ConfigParam(NULL, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);
	val2 = atoi(temp);
	Get_ConfigParam(NULL, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);

	if(command == '0'){
		flash_settings.msg_period_not_roaming_ignited = val1;
		flash_settings.msg_period_not_roaming_not_ignited = val2;
		valid = TRUE;
	}
	else if(command == '1') {
		flash_settings.msg_period_roaming_ignited = val1;
		flash_settings.msg_period_roaming_not_ignited = val2;
		valid = TRUE;
	}
	if(valid){
		settings_update_all(&flash_settings);
		BeginEchoPacket(response->buffer, MESSAGE_PERIOD_SETTING);
		temp[0] = command;
		temp[1] = '\0';
		AddStringToEchoPacket(response->buffer, temp);
		if(command == '0'){
			sprintf(temp,"%u",(unsigned int)flash_settings.msg_period_not_roaming_ignited);
			AddStringToEchoPacket(response->buffer, temp);
			sprintf(temp,"%d",(int)flash_settings.msg_period_not_roaming_not_ignited);
			AddStringToEchoPacket(response->buffer, temp);
		}
		else{
			sprintf(temp,"%d",(int)flash_settings.msg_period_roaming_ignited);
			AddStringToEchoPacket(response->buffer, temp);
			sprintf(temp,"%d",(int)flash_settings.msg_period_roaming_not_ignited);
			AddStringToEchoPacket(response->buffer, temp);
		}
		AddStringToEchoPacket(response->buffer, transactionID);
		CloseEchoPacket(response->buffer);
		response->b_needToReset = FALSE;
		return REPLY_ECHO_PACKET;
	}
	else
		return REPLY_DO_NOT_SEND;
}
/**
 *
 */
COMMAND_RESULT_T update_roaming_setting(char *buffer, COMMAND_RESPONSE_T *response)
{
	/*char temp[2];
	char command;
	char transactionID[16];

	Get_ConfigParam(NULL, &command, sizeof(command), CONFIG_PARAM_SEPERATOR);

	if(command == '0' || command == '1'){
		Get_ConfigParam(NULL, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);
		flash_settings.roaming_activation = command - 0x30;
		settings_update_all(&flash_settings);
		BeginEchoPacket(response->buffer, ROAMING_ACTIVATION);
		temp[0] = flash_settings.roaming_activation + 0x30;
		temp[1] = '\0';
		AddStringToEchoPacket(response->buffer, temp);
		CloseEchoPacket(response->buffer);
		response->b_needToReset = FALSE;
		return REPLY_ECHO_PACKET;
	}
	else
		return REPLY_DO_NOT_SEND;*/
}
/****************************************************************/
COMMAND_RESULT_T update_sms_act_setting(char *buffer, COMMAND_RESPONSE_T *response)
{
	char temp[2];

	Get_ConfigParam(NULL, temp, sizeof(temp),CONFIG_PARAM_SEPERATOR);
	if(strcmp(temp, "0") == 0){
		flash_settings.sms_activation = FALSE;
		settings_update_all(&flash_settings);
	}
	else if(strcmp(temp, "1") == 0){
		flash_settings.sms_activation = TRUE;
		settings_update_all(&flash_settings);
	}

	return REPLY_DO_NOT_SEND;
}
/***************************************************************/
COMMAND_RESULT_T update_speed_limit_setting(char *buffer, COMMAND_RESPONSE_T *response)
{
	uint16_t u16_speedLimit;
	uint16_t u16_duration;
	char transactionID[16];
	char temp[8];

	/* extract speed limit*/
	Get_ConfigParam(buffer, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);
	u16_speedLimit = atoi(temp);
	flash_settings.u16_speedLimit = u16_speedLimit;

    /* extract speed limit violation duration */
	Get_ConfigParam(buffer, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);
	u16_duration = atoi(temp);
	flash_settings.u16_speedLimitViolationDuration = u16_duration;

	Get_ConfigParam(buffer, transactionID, sizeof(transactionID),TRANSACTION_ID_SEPERATOR);

	settings_update_all(&flash_settings);

	BeginEchoPacket(response->buffer, SPEED_LIMIT_SETTING);
	sprintf(temp,"%d",flash_settings.u16_speedLimit);
	AddStringToEchoPacket(response->buffer, temp);
	sprintf(temp,"%d",flash_settings.u16_speedLimitViolationDuration);
	AddStringToEchoPacket(response->buffer, temp);

	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);
	response->b_needToReset = FALSE;
	return REPLY_ECHO_PACKET;
}
/*******************************************************************/
COMMAND_RESULT_T update_idle_alarm_setting(char *buffer, COMMAND_RESPONSE_T *response)
{
	char transactionID[16];
	char temp[16];

	Get_ConfigParam(buffer, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);
	flash_settings.u32_maxIdleTime = atoi(temp);
	Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);

	settings_update_all(&flash_settings);

	BeginEchoPacket(response->buffer, IDLE_ALARM_SETTING);
	sprintf(temp, "%X",(int)flash_settings.u32_maxIdleTime);
	AddStringToEchoPacket(response->buffer, temp);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);
	response->b_needToReset = FALSE;
	return REPLY_ECHO_PACKET;
}
/*******************************************************************/
COMMAND_RESULT_T update_engine_blockage_setting(char *buffer, COMMAND_RESPONSE_T *response)
{
	char param[2];
	char transactionID[16];
	bool success = false;

	Get_ConfigParam(buffer, param, sizeof(param), CONFIG_PARAM_SEPERATOR);
	if(strcmp(param, "0") == 0){
		flash_settings.engine_blockage = FALSE;
		Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);
		PRINT_K("\nBLOCKAGE REMOVE REQUEST\n");

		if(!flash_settings.blockage_status_in_ram)
			settings_update_all(&flash_settings);

		BeginEchoPacket(response->buffer, ENGINE_BLOCKAGE_SETTING);
		AddStringToEchoPacket(response->buffer, "0");   /* blockage removed*/
		Set_BlockageStatus(BLOCKAGE_REMOVE_REQUESTED);
		success = true;
	}
	else if(strcmp(param, "1") == 0){
		PRINT_K("\nBLOCKAGE REQUEST\n");
		flash_settings.engine_blockage = TRUE;
		Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);

		if(!flash_settings.blockage_status_in_ram)
			settings_update_all(&flash_settings);

		BeginEchoPacket(response->buffer, ENGINE_BLOCKAGE_SETTING);
		AddStringToEchoPacket(response->buffer, "1");
		Set_BlockageStatus(BLOCKAGE_REQUESTED);
		success = true;
	}
	if(success){
	//	Set_BlockageTransID(transactionID);
		AddStringToEchoPacket(response->buffer, transactionID);
		CloseEchoPacket(response->buffer);
	    response->b_needToReset = FALSE;
	}
	return REPLY_ECHO_PACKET;
}
/*******************************************************************/
COMMAND_RESULT_T update_km_counter_setting(char *buffer, COMMAND_RESPONSE_T *response)
{
	char temp1[64];
	char transactionID[16];
	uint32_t u32_flashKmTempCounter;

	Get_ConfigParam(buffer, temp1, sizeof(temp1),CONFIG_PARAM_SEPERATOR);
	Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);

	u32_flashKmTempCounter = atof(temp1) /10000;

	if(settings_update_km_counter(u32_flashKmTempCounter))
		u32_flashKmCounter = u32_flashKmTempCounter;

	BeginEchoPacket(response->buffer, KM_COUNTER);
	sprintf(temp1, "%u",(unsigned int)u32_flashKmCounter);

	BeginEchoPacket(response->buffer, KM_COUNTER);
	sprintf(temp1,"%u",(unsigned int)u32_flashKmCounter);
	strcat(temp1,"0000");
	AddStringToEchoPacket(response->buffer, temp1);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);
	response->b_needToReset = FALSE;
	return REPLY_ECHO_PACKET;
}
/**
 *
 */
COMMAND_RESULT_T reset_device(char *buffer, COMMAND_RESPONSE_T *response)
{
	char temp[16];

	Get_ConfigParam(buffer, temp, sizeof(temp),CONFIG_PARAM_SEPERATOR);

	if(strcmp(temp, "1") == 0)
		NVIC_SystemReset();

	return REPLY_DO_NOT_SEND;
}
/**
 *
 */
COMMAND_RESULT_T update_device_id(char *buffer, COMMAND_RESPONSE_T *response)
{
	uint8_t i;
	char transactionID[16];

	Get_ConfigParam(buffer, flash_settings.device_id, sizeof(flash_settings.device_id), CONFIG_PARAM_SEPERATOR);
	for(i= 0; i< sizeof(flash_settings.device_id)- 1; i++){
		if(!isdigit((int)flash_settings.device_id[i])){
			return REPLY_DO_NOT_SEND;
		}
	}
	Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);
	settings_update_all(&flash_settings);

	BeginEchoPacket(response->buffer, DEVICE_ID_SETTING);
	AddStringToEchoPacket(response->buffer, flash_settings.device_id);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);
	response->b_needToReset = FALSE;
	return REPLY_ECHO_PACKET;

}
/*********************************************************************/
COMMAND_RESULT_T request_device_status(char *buffer, COMMAND_RESPONSE_T *response)
{
	char temp[2];

	GSM_INFO_T gsm_info;

	Get_GsmInfo(&gsm_info);

	Get_ConfigParam(buffer, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);
	if(strcmp(temp, "1") == 0){
	//	Trio_PreparePingMessage(response->buffer, gsm_info.imei_no);
		Trio_PreparePingMessage(response->buffer);
		response->b_needToReset = FALSE;
		return REPLY_ECHO_PACKET;
	}
	else
		return REPLY_DO_NOT_SEND;
}
/************************************************************/
/* not implemented n the current version                     /
*************************************************************/
COMMAND_RESULT_T uncontrolled_engine_blockage(char *buffer, COMMAND_RESPONSE_T *response)
{
	char temp[2];

	Get_ConfigParam(buffer, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);
	if(strcmp(temp, "0") == 0){
	//	flash_settings.uncontrolled_blockage = FALSE;
		settings_update_all(&flash_settings);
	}
	else if(strcmp(temp, "1") == 0){
		//flash_settings.uncontrolled_blockage = TRUE;
		settings_update_all(&flash_settings);
	}
	return REPLY_DO_NOT_SEND;
}
/********************************************************************/
/*  Send a T message in response to location request                 *
*********************************************************************/
COMMAND_RESULT_T location_request(char * buffer, COMMAND_RESPONSE_T *response)
{
	T_MSG_EVENT_T event;

	PRINT_K("\nLocation request\n");

	memset(&event, 0, sizeof(event));
	strcpy(event.msg_type, EVENT_T_MSG_TRIGGER);
	put_t_msg_event(&event);

//	Trio_PrepareTMessage(response->buffer, TRUE);
//	response->b_needToReset = FALSE;
//	return REPLY_DO_NOT_SEND;
}
/********************************************************************/
COMMAND_RESULT_T update_apn_setting(char *buffer, COMMAND_RESPONSE_T *response)
{
	char transactionID[16];
	//char *pBuf;

	Get_ConfigParam(buffer, flash_settings.flash_apn, sizeof(flash_settings.flash_apn), CONFIG_PARAM_SEPERATOR);
	Get_ConfigParam(buffer, flash_settings.flash_apnusername, sizeof(flash_settings.flash_apnusername), CONFIG_PARAM_SEPERATOR);
	Get_ConfigParam(buffer, flash_settings.flash_apnpassword,  sizeof(flash_settings.flash_apnpassword),CONFIG_PARAM_SEPERATOR);
	Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);
	settings_update_all(&flash_settings);

	BeginEchoPacket(response->buffer, APN_SETTING);
	AddStringToEchoPacket(response->buffer, flash_settings.flash_apn);
	AddStringToEchoPacket(response->buffer, flash_settings.flash_apnusername);
	AddStringToEchoPacket(response->buffer, flash_settings.flash_apnpassword);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);
	response->b_needToReset = TRUE;
	return REPLY_ECHO_PACKET;
}
/********************************************************************/
COMMAND_RESULT_T update_password(char *buffer, COMMAND_RESPONSE_T *response)
{
	char transactionID[16];
	char pswdBuf[MAX_SET_COMMAND_PSWD_LENGTH + 1];
//	char *pBuf;

	Get_ConfigParam(buffer, pswdBuf, sizeof(pswdBuf),CONFIG_PARAM_SEPERATOR);

	if(strlen(pswdBuf) > MAX_SET_COMMAND_PSWD_LENGTH)
		return 0;
	else{
		strcpy(flash_settings.password, pswdBuf);
		Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);
		settings_update_all(&flash_settings);

		BeginEchoPacket(response->buffer, CHANGE_PASSWORD);
		AddStringToEchoPacket(response->buffer, flash_settings.password);
		AddStringToEchoPacket(response->buffer, transactionID);
		CloseEchoPacket(response->buffer);
		response->b_needToReset = FALSE;
		return REPLY_ECHO_PACKET;
	}
}
/********************************************************************/
COMMAND_RESULT_T send_sms_command(char *buffer, COMMAND_RESPONSE_T *response)
{
	char transactionID[16];
	char smsBuf[MAX_SMS_LENGTH];
	char destAddr[DEST_ADDR_LENGTH];

	Get_ConfigParam(buffer, destAddr, sizeof(destAddr), CONFIG_PARAM_SEPERATOR);        /* extract destination address */
	Get_ConfigParam(buffer, smsBuf, sizeof(smsBuf), CONFIG_PARAM_SEPERATOR);          /* extract sms content         */
	Get_ConfigParam(buffer, transactionID, sizeof(transactionID),TRANSACTION_ID_SEPERATOR);   /* extract transaction id      */

	gsm_send_sms(destAddr, smsBuf, NULL);

	BeginEchoPacket(response->buffer, SEND_SMS);
	AddStringToEchoPacket(response->buffer, destAddr);
	AddStringToEchoPacket(response->buffer, smsBuf);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);

	response->b_needToReset = FALSE;
	return REPLY_ECHO_PACKET;
}
/********************************************************************/
COMMAND_RESULT_T update_firmware(char *buffer, COMMAND_RESPONSE_T *response)
{
	char server_ip[128];
	char server_port[6];
	PRINT_K("\nUpdate request\n");
	/* get firmware update server ip address*/
	Get_ConfigParam(buffer, server_ip , sizeof(server_ip), CONFIG_PARAM_SEPERATOR);
	/* get firmware update TCP port number*/
	Get_ConfigParam(buffer, server_port, sizeof(server_port), CONFIG_PARAM_SEPERATOR);

	gsm_set_connection_parameters(server_ip, server_port);
	requestConnectToUpdateServer = TRUE;
	response->b_needToReset = FALSE;
	return REPLY_CONNECT_TO_UPDATE_SERVER;
}
/********************************************************************/
COMMAND_RESULT_T  erase_external_flash(char *buffer, COMMAND_RESPONSE_T *response)
{
	char transactionID[16];

	sst25_erase_flash();
//	WriteInitialValuesToFlash();
/*	Init_SPI_Cache();*/
//	GetDefaultUserSettings(&default_settings);
	settings_update_all(&default_settings);
	settings_read_to_ram();
//	settings_print();
//	settings_load_km_counter();

	Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);

	BeginEchoPacket(response->buffer, ERASE_EXT_FLASH);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);

	response->b_needToReset = TRUE;
	return REPLY_ECHO_PACKET;
}
/********************************************************************/
COMMAND_RESULT_T update_blockage_persistance(char *buffer, COMMAND_RESPONSE_T *response)
{
	char temp[2];
	char command;
	char transactionID[16];

	Get_ConfigParam(NULL, &command, sizeof(command), CONFIG_PARAM_SEPERATOR);

	if(command == '0' || command == '1'){
		Get_ConfigParam(NULL, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);
		flash_settings.blockage_status_in_ram = command - 0x30;
		settings_update_all(&flash_settings);
		BeginEchoPacket(response->buffer, SET_BLOCKAGE_PERSISTANCE);
		temp[0] = flash_settings.blockage_status_in_ram + 0x30;
		temp[1] = '\0';
		AddStringToEchoPacket(response->buffer, temp);
		CloseEchoPacket(response->buffer);
		response->b_needToReset = FALSE;
		return REPLY_ECHO_PACKET;
	}
	else
		return REPLY_DO_NOT_SEND;
}
/***********************************************************************/
static COMMAND_RESULT_T get_healt_status(char *buffer, COMMAND_RESPONSE_T *response)
{
	memset(response->buffer, 0, MAX_T_MESSAGE_SIZE);
	BeginGetResponse(response->buffer, DEVICE_HEALT_STATUS);
	strcat(response->buffer, ";");
	strcat(response->buffer, SW_VERSION);
	if(gsm_get_healt_status())
		AddStringToEchoPacket(response->buffer, "GSM:1");
	else
		AddStringToEchoPacket(response->buffer, "GSM:0");
	if(gps_get_healt_status())
		AddStringToEchoPacket(response->buffer, "GPS:1");
	else
		AddStringToEchoPacket(response->buffer, "GPS:0");
	if(sst25_get_healt_status())
		AddStringToEchoPacket(response->buffer, "FLASH:1");
	else
		AddStringToEchoPacket(response->buffer, "FLASH:0");

	CloseEchoPacket(response->buffer);
	response->b_needToReset = FALSE;
	return REPLY_ECHO_PACKET;
}
/*************************************************************/
static COMMAND_RESULT_T get_version_info(char *buffer, COMMAND_RESPONSE_T *response)
{
	memset(response->buffer, 0, MAX_T_MESSAGE_SIZE);
	BeginGetResponse(response->buffer, DEVICE_VERSION_INFO);
	AddStringToEchoPacket(response->buffer, VERSION);
	CloseEchoPacket(response->buffer);
	response->b_needToReset = FALSE;
	return REPLY_ECHO_PACKET;
}
/***************************************************************/
static COMMAND_RESULT_T get_config_info(char *buffer, COMMAND_RESPONSE_T *response)
{
	int len =0;

	memset(response->buffer, 0, MAX_T_MESSAGE_SIZE);
	BeginGetResponse(response->buffer, DEVICE_CONFIG_INFO);
	strcat(response->buffer, ";");
	len = strlen(response->buffer);
	PRINT_RAW(response->buffer, len);
	//memcpy(&response->buffer[len], &flash_settings, sizeof(flash_settings));
	PRINT_RAW((char *)&flash_settings,sizeof(flash_settings));
	PRINT_RAW(";0!", 3);
	//CloseEchoPacket(response->buffer);
	response->b_needToReset = FALSE;
	return REPLY_DO_NOT_SEND;
}
/********************************************************************/
COMMAND_RESULT_T gps_cold_restart(char *const buffer, COMMAND_RESPONSE_T *response)
{
	PRINT_K("\nGPS COLD START...\n");
	gps_send_mtk_command("$PMTK104*37\r\n");
	return REPLY_DO_NOT_SEND;
}
/*********************************************************/
void BeginGetResponse(char *pBuf, COMMAND_TYPE_T command)
{
	GSM_INFO_T gsm_info;


	Get_GsmInfo(&gsm_info);
	strcpy(pBuf, "@GET;INFO;");
	strcat(pBuf,gsm_info.imei_no);
	strcat(pBuf,"70");
	strcat(pBuf,";");
	/*itoa(command, temp, 10);
	strcat(pBuf, temp);
	strcat(pBuf,";");*/
	strcat(pBuf, DEVICE_MODEL);
}
/*************************************************************/
void GetDefaultUserSettings(const FLASH_SETTINGS_T *settings)
{
	memcpy(settings, &default_settings, sizeof(FLASH_SETTINGS_T));
}
/*************************************************************/
bool settings_update_all2(FLASH_SETTINGS_T *new_settings)
{
	FLASH_SETTINGS_T settings;
	uint32_t chksum;

	PRINT_K("\nUpdating User Settings\n");

	memcpy(&settings, new_settings, sizeof(FLASH_SETTINGS_T));

	chksum = calc_chksum((uint8_t *)&settings, sizeof(FLASH_SETTINGS_T) - sizeof(settings.chksum));
	settings.chksum = chksum;

	sst25_disable_write_lock();
	sst25_write_array((char *)&settings, sizeof(FLASH_SETTINGS_T), FIRST_FLASH_SETTINGS_ADDRESS);
	sst25_enable_write_lock();

	PRINT_K("\nSettings Updated\n");

	return TRUE;
}
/***********************************************/
bool settings_update_all(FLASH_SETTINGS_T *new_settings)
{
	FLASH_SETTINGS_T settings;
	uint32_t chksum;

	PRINT_K("\nUpdating User Settings\n");

	memcpy(&settings, new_settings, sizeof(FLASH_SETTINGS_T));

	chksum = calc_chksum((uint8_t *)&settings, sizeof(FLASH_SETTINGS_T) - sizeof(settings.chksum));
	settings.chksum = chksum;

	sst25_disable_write_lock();
	sst25_write_array((char *)&settings, sizeof(FLASH_SETTINGS_T), FIRST_FLASH_SETTINGS_ADDRESS);
	sst25_enable_write_lock();

	PRINT_K("\nSettings Updated\n");

	return TRUE;
}
/**
 *
 */
uint32_t Get_FlashKmValue()
{
	return u32_flashKmCounter;
}
/**
 *
 */
void settings_get(FLASH_SETTINGS_T *user_settings)
{
	memcpy(user_settings, &flash_settings, sizeof(FLASH_SETTINGS_T));
}
/********************************************************/
void settings_load_km_counter()
{
	char printBuf[64];

	u32_flashKmCounter = get_last_valid_value_in_sector(KM_COUNTER_FLASH_ADDRESS);

	/*sst25_read_array(KM_COUNTER_FLASH_ADDRESS, sizeof(u32_flashKmCounter), (char *)&u32_flashKmCounter);
	if(u32_flashKmCounter == 0xFFFFFFFF)
		u32_flashKmCounter = 0;*/

	sprintf(printBuf, "\nLast Saved Km Value: %u\n",u32_flashKmCounter);
	PRINT_K(printBuf);
}
/**
 *
 */
uint32_t settings_load_engine_hour_counter()
{
	return get_last_valid_value_in_sector(ENGINE_HOUR_COUNT_ADDRESS);
}
/**
 *
 */
void settings_load_blockage_status()
{
	if(!flash_settings.blockage_status_in_ram){
		if(flash_settings.engine_blockage)
			Set_BlockageStatus(BLOCKAGE_REQUESTED);
		else
			Set_BlockageStatus(BLOCKAGE_NOT_EXIST);
	}
}
/**
 *
 */
COMMAND_RESULT_T ProcessReceivedData(char *dataBuffer, COMMAND_RESPONSE_T *response, COMMAND_SOURCE_T cmdSource)
{
	COMMAND_RESULT_T result = REPLY_DO_NOT_SEND;
	char  pswdBuf[MAX_SET_COMMAND_PSWD_LENGTH + 1];
	char printBuf[64];
	char *pPassword, *p_pswdStart;
	char *p_dataStart, *p_temp;

	if((p_dataStart = strstr(dataBuffer, TRIO_CONFIG_WORD)) != NULL){
		p_temp = strchr(p_dataStart, '\r');
		if(p_temp != NULL)
			*p_temp = '\0';
		p_pswdStart = strchr(p_dataStart ,':');
		if(p_pswdStart != NULL) {  /* password available */
			pPassword = strtok(p_pswdStart, CONFIG_PARAM_SEPERATOR);
			memcpy(pswdBuf, pPassword + 1, MAX_SET_COMMAND_PSWD_LENGTH);
			if(!validate_password(pswdBuf))
				return result;
			p_dataStart = strlen(pswdBuf) + pPassword + 2;  /* points to data*/
		}
		else if(strlen(flash_settings.password) > 0){
			return result;
		}
		else
		    p_dataStart += sizeof(TRIO_CONFIG_WORD);

		result = ParseConfigurationString(p_dataStart, response, cmdSource);
	}

	else if((p_dataStart = strstr(dataBuffer, TRIO_GET_COMMAND)) != NULL){
	//	PRINT_K(p_dataStart);
		if(p_dataStart != NULL){
			p_dataStart = &dataBuffer[sizeof(TRIO_GET_COMMAND)];
			result = ParseGetCommand(p_dataStart, response, cmdSource);
			//PRINT_K(p_dataStart);
		}
	}

	else if((p_dataStart = strchr(dataBuffer, CHAR_SOH)) != NULL)
	{
		sprintf(printBuf, "Message from RFID %s", p_dataStart);
		PRINT_K(p_dataStart);
		strcpy(rfid_msg_buffer, dataBuffer + 1);
	}

	return result;
}
/**************************************************************/
bool ExtractCommandPassword(char *p_pswdStart, char *pswdBuf)
{
	char tempBuf[200];
	char *pPassword;

	strcpy(tempBuf, p_pswdStart);
	pPassword = strtok(tempBuf, CONFIG_PARAM_SEPERATOR);
	memcpy(pswdBuf, pPassword + 1, MAX_SET_COMMAND_PSWD_LENGTH);
	return TRUE;
}
/**************************************************************/
bool validate_password(char *pswd)
{
	if(strcmp(pswd, flash_settings.password) == 0)
		return TRUE;
	else
		return FALSE;
}
/*********************************************************/
void BeginEchoPacket(char *pBuf, COMMAND_TYPE_T command)
{
	GSM_INFO_T gsm_info;
	char temp[4];

	Get_GsmInfo(&gsm_info);
	strcpy(pBuf, "@SET;");
	strcat(pBuf,gsm_info.imei_no);
	strcat(pBuf, CONFIG_PARAM_SEPERATOR);
	sprintf(temp,"%d", command);
	strcat(pBuf, temp);
}
/**********************************************************/
void CloseEchoPacket(char *pBuf)
{
	strcat(pBuf, "!");
}
/**********************************************************/
void AddStringToEchoPacket(char *pBuf, char *strToAdd)
{
	strcat(pBuf, CONFIG_PARAM_SEPERATOR);
	strcat(pBuf, strToAdd);
}
/*********************************************************/
bool settings_read_to_ram()
{
	if(sst25_is_empty()){
		PRINT_K("\nFlash Empty\n");
		settings_update_all(&default_settings);
		memcpy(din_func_table, default_din_settings, sizeof(din_func_table));
		settings_update_din_func_table();
		settings_read_user_settings(&flash_settings);

	}
	else{
		PRINT_K("\nFlash Not Empty\n");
		settings_read_user_settings(&flash_settings);
		settings_load_din_functions();
		if(*((uint32_t * )din_func_table) == 0xFFFFFFFF){
			memcpy(din_func_table, default_din_settings, sizeof(din_func_table));
			settings_update_din_func_table();
		}

	}
	settings_load_km_counter();
	set_initial_engine_hour_value();
	settings_load_engine_hour_counter();
	settings_load_registered_node_table();
	settings_load_din_functions();
	settings_print_din_func_table();
	return TRUE;
}
/*********************************************************/
bool settings_load_registered_node_table()
{
	sst25_read_array(RF_NODES_ID_TABLE, MAX_NUMBER_OF_NODES * sizeof(REG_NODE_INFO_T), (char *)reg_node_table);
	return TRUE;
}
/*********************************************************/
void settings_get_registered_node_table(REG_NODE_INFO_T *reg_node_list)
{
	memcpy(reg_node_list, reg_node_table, sizeof(reg_node_table));
}
/*********************************************************/
bool settings_load_din_functions()
{
	sst25_read_array(DIN_FUNCTION_TABLE, NUM_DIGITAL_INPUTS * sizeof(DIN_PROPERTIES_T), (char *)din_func_table);
	return TRUE;
}
/*********************************************************/
bool settings_read_user_settings(FLASH_SETTINGS_T *settings)
{
	uint32_t chksum;
	char temp[64];

	sst25_read_array(FIRST_FLASH_SETTINGS_ADDRESS, sizeof(FLASH_SETTINGS_T), (char *)settings);

	chksum = calc_chksum((uint8_t *)settings, sizeof(FLASH_SETTINGS_T) -sizeof(settings->chksum));
	sprintf(temp,"\nCalculated checksum %X, Table checksum: %X\n",chksum,settings->chksum );
	PRINT_K(temp);
	if(chksum != settings->chksum) {
		PRINT_K("\nInvalid Settings\n");
		settings_update_all(&default_settings);
	}
	else{
		if(!validate_connection_params(&flash_settings))
			memcpy(&flash_settings, &default_settings, sizeof(FLASH_SETTINGS_T));
	}
	return TRUE;
}
/*********************************************************/
void settings_print()
{
	char dumpBuf[256];
	int i;

	sprintf(dumpBuf,"\nAPN                        %s", flash_settings.flash_apn);
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nAPN Password               %s", flash_settings.flash_apnpassword);
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nAPN Username               %s", flash_settings.flash_apnusername);
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nServer                     %s : %s", flash_settings.server_ip, flash_settings.server_port);
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nDevice Id                  %s", flash_settings.device_id);
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nUser Password              %s", flash_settings.password);
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nRemove Blockage On Reset   %s", flash_settings.blockage_status_in_ram ? "Enabled" : "Disabled");
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nBlockage Status            %s", flash_settings.engine_blockage ? "Blockaged" : "Not-Blockaged");
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nSMS Setting                %s",flash_settings.sms_activation ? "Enabled" : "Disabled");
	PRINT_K(dumpBuf);
/*	sprintf(dumpBuf,"\nRoaming                    %s", flash_settings.roaming_activation ? "Enabled" : "Disabled");
	PRINT_K(dumpBuf);*/
	sprintf(dumpBuf,"\nSpeed Alarm Value          %d (Km)",  (int)flash_settings.u16_speedLimit);
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nSpeed Alarm  Duration      %d (sec)", (int)flash_settings.u16_speedLimitViolationDuration);
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nMax. Stop Time             %d (sec)", (int)flash_settings.u32_maxStopTime);
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nMessage Period(Not roaming-ignited)        %d (sec)", (int)flash_settings.msg_period_not_roaming_ignited);
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nMessage Period(Not roaming-Not ignited)    %d (sec)", (int)flash_settings.msg_period_not_roaming_not_ignited);
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nMessage Period(Roaming-Ignited)            %d (sec)", (int)flash_settings.msg_period_roaming_ignited);
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nMessage Period(Roaming-Not Ignited)        %d (sec)", (int)flash_settings.msg_period_roaming_not_ignited);
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nCAN Speed         %d (Kbit)", (int)flash_settings.canBusBitrate *1000);
	PRINT_K(dumpBuf);
	sprintf(dumpBuf,"\nCAN Frame Type    %d", flash_settings.canBusFrameType);
	PRINT_K(dumpBuf);
	/*sprintf(dumpBuf,"\nGPS baud rate     %d (bps)", (int)flash_settings.gps_baud_rate);
	PRINT_K(dumpBuf);*/
	sprintf(dumpBuf,"\nG sensor threshold  %d", flash_settings.g_threshold);
	PRINT_K(dumpBuf);

	for(i = 0; i < NUMBER_OF_RFID_CHANNELS; i++){
		if(flash_settings.rfid_ch_setting.check_ch[i]){
			sprintf(dumpBuf,"\nCheck RFID on UART channel %d  If Ignition %s", i + 1 , flash_settings.rfid_ch_setting.checkState ? "On" : "Off");
			PRINT_K(dumpBuf);
		}
	}
	sprintf(dumpBuf, "\nBuzzer alert duration : %d secs(s)",flash_settings.rfid_ch_setting.duration);
	PRINT_K(dumpBuf);
/*	sprintf(dumpBuf, "\nCalibration coefficents c = %d, m = %.4f",flash_settings.calib_coeffs.coeff_c, flash_settings.calib_coeffs.coeff_m );
	PRINT_K(dumpBuf);*/

	sprintf(dumpBuf, "\nLBS : %s", flash_settings.lbsActStatus ? "Enabled" : "Disabled");
	PRINT_K(dumpBuf);

	sprintf(dumpBuf, "\nEngine Hour Counter : %s", flash_settings.engineHrCntActStatus ? "Enabled" : "Disabled");
	PRINT_K(dumpBuf);

	sprintf(dumpBuf, "\nEngine Hour Counter Value : %d sec(s) (Hex: 0x%X)", settings_load_engine_hour_counter(),settings_load_engine_hour_counter());
	PRINT_K(dumpBuf);

	sprintf(dumpBuf,"\nChecksum          %.8X\n", (int)flash_settings.chksum);
	PRINT_K(dumpBuf);
}
/****************************************************************/
bool settings_update_km_counter(uint32_t newValue)
{
	//uint32_t u32_totalKmWritten;
	//char printBuf[64];
	uint32_t km_wrt_address;
	/*sprintf(printBuf,"\nUpdating Km counter to %u km...", (unsigned int)newValue);
	PRINT_OFFLINE(printBuf);*/

	km_wrt_address = get_next_write_address_in_page(KM_COUNTER_FLASH_ADDRESS);

	sst25_disable_write_lock();   /* disable flash protection */
	sst25_write_array((char *)&newValue, sizeof(newValue), km_wrt_address);
	sst25_enable_write_lock();   /* enable flash protection */
	return TRUE;
	/*sst25_read_array(km_wrt_address, sizeof(u32_totalKmWritten), (char *)&u32_totalKmWritten);

	if(u32_totalKmWritten == newValue){
		PRINT_K("Done\n");
		return TRUE;
	}
	else{
		PRINT_K("Failed\n");
		return FALSE;
	}*/
}
/*************************************************************************/
void WriteInitialValuesToFlash()
{
	uint32_t u32_initialValues = 0;

	sst25_disable_write_lock(); /* disable flash protection */

	sst25_write_array((char *)&u32_initialValues, 4, ENGINE_HOUR_COUNT_ADDRESS);
	sst25_write_array((char *)&u32_initialValues, 4, KM_COUNTER_FLASH_ADDRESS);      /* ED000 */
	sst25_write_array((char *)&u32_initialValues, 4, OFFLINE_DATA_WRITE_ADDRESS);    /* FC000 */
	sst25_write_array((char *)&u32_initialValues, 4, OFFLINE_DATA_READ_ADDRESS);     /* FD000 */

	sst25_enable_write_lock();  /* enable flash protection */
}
/***********************************************************/
void Set_BlockageStatus(BLOCKAGE_STATUS_T new_status)
{
	blockage_info.blockageStatus = new_status;
}
/************************************************************/
void Set_BlockageSource(COMMAND_SOURCE_T source)
{
	blockage_info.cmdSource = source;
}
/************************************************************/
uint16_t settings_get_max_speed_limit()
{
	return flash_settings.u16_speedLimit;
}
/************************************************************/
uint16_t Get_MaxSpeedViolationDuration()
{
	return flash_settings.u16_speedLimitViolationDuration;
}
/************************************************************************/
int32_t PrepareBlockageEchoPacket(char *buffer)
{
	BLOCKAGE_INFO_T blockage_info;

	Get_BlockageInfo(&blockage_info);

	BeginEchoPacket(buffer, ENGINE_BLOCKAGE_SETTING);
	AddStringToEchoPacket(buffer, "2");
	AddStringToEchoPacket(buffer, blockage_info.transactionID);
	CloseEchoPacket(buffer);
	return strlen(buffer);
}
/************************************************************************/
static COMMAND_RESULT_T update_can_bitrate(char *buffer, COMMAND_RESPONSE_T *response)
{
	CAN_BITRATE  bitRate;
	char transactionID[16];
	char temp[16];

	Get_ConfigParam(buffer, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);
	bitRate = (CAN_BITRATE)atoi(temp);

	if(bitRate == CAN_BUSSPEED_250K || bitRate == CAN_BUSSPEED_500K){
		flash_settings.canBusBitrate = bitRate;
		Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);
		settings_update_all(&flash_settings);
		settings_read_user_settings(&flash_settings);
		can_init();
		BeginEchoPacket(response->buffer, CAN_BUSSPEED_SETTING);
		sprintf(temp, "%d",(CAN_BITRATE)flash_settings.canBusBitrate);
		AddStringToEchoPacket(response->buffer, temp);
		AddStringToEchoPacket(response->buffer, transactionID);
		CloseEchoPacket(response->buffer);
		response->b_needToReset = FALSE;
		return REPLY_ECHO_PACKET;
	}
	else
		return REPLY_DO_NOT_SEND;
}
/************************************************************************/
static COMMAND_RESULT_T update_can_frame_type(char *buffer, COMMAND_RESPONSE_T *response)
{
	CAN_FRAME_TYPE  frameType;
	char transactionID[16];
	char temp[16];

		Get_ConfigParam(buffer, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);
		frameType = (CAN_FRAME_TYPE)atoi(temp);
		if(frameType == CAN_STD_FRAME || frameType == CAN_EXT_FRAME){
			flash_settings.canBusFrameType = frameType;
			Get_ConfigParam(buffer, transactionID, sizeof(transactionID),TRANSACTION_ID_SEPERATOR);
			settings_update_all(&flash_settings);
			BeginEchoPacket(response->buffer, CAN_FRAMETYPE_SETTING);
			sprintf(temp, "%d",(CAN_BITRATE)flash_settings.canBusFrameType);
			AddStringToEchoPacket(response->buffer, temp);
			AddStringToEchoPacket(response->buffer, transactionID);
			CloseEchoPacket(response->buffer);
			response->b_needToReset = FALSE;
			return REPLY_ECHO_PACKET;
		}
		else
			return REPLY_DO_NOT_SEND;
}
/****************************************************************/
COMMAND_RESULT_T update_gps_baudrate_setting(char *buffer, COMMAND_RESPONSE_T *response)
{
	char transactionID[16];
	char gps_baudrate[16];

	Get_ConfigParam(buffer, gps_baudrate, sizeof(gps_baudrate), CONFIG_PARAM_SEPERATOR);
	Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);
	flash_settings.gps_baud_rate = atoi(gps_baudrate);
	settings_update_all(&flash_settings);

	BeginEchoPacket(response->buffer, GPS_BAUDRATE_SETTING);
	sprintf(gps_baudrate,"%d",(int)flash_settings.gps_baud_rate);
	AddStringToEchoPacket(response->buffer, gps_baudrate);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);
	gps_init(flash_settings.gps_baud_rate);
	response->b_needToReset = FALSE;
	return REPLY_ECHO_PACKET;


}
/****************************************************************/
/* Packet format to register rf node to T2 master
 *
 * | Node ID
 * | 11-byte
 *
 *
 *  Bit 1: Trailer sensor
 *  Bit 2: Temp Sensor
 *
 */
COMMAND_RESULT_T settings_add_rf_node(char *buffer, COMMAND_RESPONSE_T *response)
{
	REG_NODE_INFO_T node;
	char printBuf[128];
	int i;
	char node_id[LE50_SERIAL_NUMBER_SIZE + 1];
	char transactionID[16];

	memset(node_id, 0 , sizeof(node_id));
	//memset(node_id, 0, sizeof(node_id));
	Get_ConfigParam(buffer, node_id, sizeof(node_id), CONFIG_PARAM_SEPERATOR);
	if(!validate_node_id(node_id)){
		PRINT_K("\nInvalid node Id\n");
		return 0;
	}
	/*Get_ConfigParam(buffer, app_id, sizeof(app_id),   CONFIG_PARAM_SEPERATOR);
	Get_ConfigParam(buffer, trailer_track, sizeof(trailer_track), CONFIG_PARAM_SEPERATOR);*/
	Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);

	/*node.app_id = atoi(app_id);
	node.tracking = atoi(trailer_track);*/
	strcpy(node.node_id, node_id);

	i = settings_search_in_registered_node_table(node_id);
	if( i < 0)
		settings_register_node(&node);
	else{
		sprintf(printBuf,"\nNode %s already exist at index: %d\n",node_id, i);
		PRINT_K(printBuf);
	}
}
/****************************************************************/
bool validate_node_id(char *node_id)
{
	int i;

	if(strlen(node_id) > LE50_SERIAL_NUMBER_SIZE)
		return  false;

	/* check node id contains a non-alphanumeric
	 * or lower case digit */
	for(i =0; i < LE50_SERIAL_NUMBER_SIZE; i++){
		if((!isalnum(node_id[i])) || (islower(node_id[i])))
				return false;
	}
	return true;
}
/****************************************************************/
COMMAND_RESULT_T settings_remove_rf_node(char *buffer, COMMAND_RESPONSE_T *response)
{
	int i;
	char node_id[LE50_SERIAL_NUMBER_SIZE + 1];
	char transactionID[16];

	Get_ConfigParam(buffer, node_id, sizeof(node_id), CONFIG_PARAM_SEPERATOR);
	Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);
	i = settings_search_in_registered_node_table(node_id);
	if( i < 0) {
		sprintf(buffer,"\nCannot find node %s\n", node_id);
		PRINT_K(buffer);
	}
	else{
		if(settings_remove_node_from_flash_table(i))
			PRINT_K("\nNode removed succesfully\n");
		else
			PRINT_K("\nError removing Node from FLASH!!!\n");
	}
}
/****************************************************************/
bool settings_update_node_table()
{
/*	REG_NODE_INFO_T temp_reg_nodes[MAX_NUMBER_OF_NODES];
    int i = 0;*/
    bool result = FALSE;

	PRINT_K("\nUpdating Node Table\n");

	sst25_disable_write_lock();  /* disable flash protection */

	/*while( i++ < SETTINGS_UPDATE_RETRY_COUNT){*/
	sst25_write_array((char *)reg_node_table, sizeof(reg_node_table), RF_NODES_ID_TABLE);
	/*	sst25_read_array(RF_NODES_ID_TABLE, sizeof(temp_reg_nodes) , (char *)temp_reg_nodes);
		if(memcmp(reg_node_table, temp_reg_nodes, sizeof(reg_node_table)) == 0){
			result = TRUE;
			break;
		}
	}*/
	sst25_enable_write_lock();   /* enable flash protection */

/*	if(!result)
		PRINT_K("\nError updating node table!\n");
	else{*/
		PRINT_K("\nNode table updated\n");
		settings_print_node_table();
	//}
	return result;
}
/*************************************************************/
bool settings_update_din_func_table()
{
	 bool result = FALSE;

	PRINT_K("\nUpdating DIN function table\n");

	sst25_disable_write_lock();  /* disable flash protection */
	sst25_write_array((char *)din_func_table, sizeof(din_func_table), DIN_FUNCTION_TABLE);
	sst25_enable_write_lock();   /* enable flash protection */
	PRINT_K("\nDIN function table updated\n");
    //settings_print_node_table();
	return result;
}
/****************************************************************/
void settings_print_node_table()
{
	int i;
	char buffer[256];

	for(i= 0; i< MAX_NUMBER_OF_NODES; i++){
		if(reg_node_table[i].node_number != 0xFFFF){
			sprintf(buffer,"\nNode ID: %s Node Number: %d\n",reg_node_table[i].node_id,reg_node_table[i].node_number);
			PRINT_K(buffer);
		}
	}
}
/****************************************************/
bool settings_register_node(REG_NODE_INFO_T *reg_node)
{
	int i;
	char buffer[64];

	for(i= 0; i< MAX_NUMBER_OF_NODES; i++){
		if(reg_node_table[i].node_number == 0xFFFF){    /* empty node index */
			sprintf(buffer,"\nFree node index at %d\n", i);
			PRINT_K(buffer);
			reg_node->node_number = i;
			reg_node_table[i] = *reg_node;
			settings_update_node_table();
			return true;
		}
	}
	return false;

}
/***************************************************/
bool settings_remove_node_from_flash_table(int node_index)
{
	char buffer[64];

	if(node_index >=0){
		reg_node_table[node_index].node_number = 0xFFFF;
		settings_update_node_table();
		sprintf(buffer,"\nNode removed at index: %d\n", node_index);
		PRINT_K(buffer);
		return TRUE;
	}
	else{
		sprintf(buffer,"\nInvalid node index: %d\n", node_index);
	    PRINT_K(buffer);
	    return FALSE;
	}
}
/*************************************************************
 Searchs a given node id in the registered node table and returns
 the index of item otherwise returns -1.
**************************************************************/
int settings_search_in_registered_node_table(char * node_id)
{
	int i;

	for(i= 0; i< MAX_NUMBER_OF_NODES; i++){
		if((strcmp(reg_node_table[i].node_id, node_id) == 0 && (reg_node_table[i].node_number < MAX_NUMBER_OF_NODES))){
		//	PRINT_K("\nNode Bulundu\n");
			return i;
		}

	 }
	return -1;
}
/*************************************************************
 Fills the buffer with the registered node ids and imei number.
 ************************************************************/
COMMAND_RESULT_T settings_list_rf_nodes(char *buffer, COMMAND_RESPONSE_T *response)
{
	GSM_INFO_T gsm_info;
	int i;

	Get_GsmInfo(&gsm_info);

	strcat(response->buffer, "[RF;");
	strcat(response->buffer, gsm_info.imei_no);
	for(i= 0; i< MAX_NUMBER_OF_NODES; i++){
		if(reg_node_table[i].node_number < MAX_NUMBER_OF_NODES){
			strcat(response->buffer, ":");
			strcat(response->buffer,reg_node_table[i].node_id);
		}
	}
	strcat(response->buffer, "]");
	response->b_needToReset = FALSE;
    return REPLY_ECHO_PACKET;
}
/*************************************************************/
bool validate_connection_params(FLASH_SETTINGS_T *settings)
{
	int i;
	bool found = FALSE;

	if(!isalnum((int)settings->flash_apn[0]))
		return FALSE;

	for(i = 0; i < MAX_SERVER_ADDR_LEN; i++){
		if(settings->server_ip[i] == '.'){
			found = TRUE;
			break;
		}
	}
	if(!found)
		return FALSE;

	if(atoi(settings->server_port) == 0)
		return FALSE;

	return TRUE;
}
/*************************************************************/
static COMMAND_RESULT_T text_message_from_server(char *buffer, COMMAND_RESPONSE_T *response)
{
	TABLET_APP_EVENT_T tablet_app_event;
	JSON_DATA_INFO_T json_info_t;
	char transactionID[16];
	char msg_id[64];
	char message[TABLET_APP_MSG_SIZE + 1];

	memset(message, 0, sizeof(message));
	memset(msg_id, 0, sizeof(msg_id));

	Get_ConfigParam(buffer, msg_id, sizeof(msg_id), CONFIG_PARAM_SEPERATOR);
	Get_ConfigParam(buffer, message, sizeof(message), CONFIG_PARAM_SEPERATOR);
	Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);

	BeginEchoPacket(response->buffer, TEXT_MSG_FROM_SERVER);
	AddStringToEchoPacket(response->buffer, msg_id);
	AddStringToEchoPacket(response->buffer, message);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);

	response->b_needToReset = FALSE;
	PRINT_K("\nNew text message from server\n");
	tablet_app_event.event_type = MOBILE_APP_EVENT_MSG_FROM_SERVER;
	strcpy(json_info_t.json_type,"text");
	strcpy(tablet_app_event.json_data_t.json_type_text.message, message);
	strcpy(tablet_app_event.json_data_t.json_type_text.msg_id, msg_id);
	put_tablet_app_event(&tablet_app_event);
	return REPLY_ECHO_PACKET;
}
/*************************************************************/
static COMMAND_RESULT_T text_msg_read_ack_from_server(char *buffer, COMMAND_RESPONSE_T *response)
{

	TABLET_APP_EVENT_T tablet_app_event;
	JSON_DATA_INFO_T json_info_t;
	char transactionID[16];
	char msg_id[64];
	char printBuf[128];

	memset(msg_id, 0, sizeof(msg_id));

	Get_ConfigParam(buffer, msg_id, sizeof(msg_id), CONFIG_PARAM_SEPERATOR);
	Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);

	BeginEchoPacket(response->buffer, TEXT_MSG_FROM_SERVER);
	AddStringToEchoPacket(response->buffer, msg_id);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);

	response->b_needToReset = FALSE;
	sprintf(printBuf,"\nMessage with ID %s read\n", msg_id);
	PRINT_K(printBuf);
	tablet_app_event.event_type = MOBILE_APP_EVENT_MESSAGE_READ_ACK_FROM_SERVER;
	strcpy(json_info_t.json_type, "drv");
	strcpy(tablet_app_event.json_data_t.json_type_text.msg_id, msg_id);
	put_tablet_app_event(&tablet_app_event);
	return REPLY_ECHO_PACKET;
}
/************************************************************/
static COMMAND_RESULT_T settings_update_g_sensor_threshold(char *buffer, COMMAND_RESPONSE_T *response)
{
	char transactionID[16];
	char g_threshold[8];

	Get_ConfigParam(buffer, g_threshold, sizeof(g_threshold), CONFIG_PARAM_SEPERATOR);
	Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);
	flash_settings.g_threshold = atoi(g_threshold);
	settings_update_all(&flash_settings);

	BeginEchoPacket(response->buffer, UPDATE_G_THRESHOLD);
	sprintf(g_threshold,"%d",flash_settings.g_threshold);
	AddStringToEchoPacket(response->buffer, g_threshold);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);
	response->b_needToReset = TRUE;
	return REPLY_ECHO_PACKET;
}
/************************************************************/
static COMMAND_RESULT_T settings_update_rfid_check(char *buffer, COMMAND_RESPONSE_T *response)
{
	char transactionID[16];
	char param1[2], param2[2], param3[2], param4[8];
	char temp[64];
	uint32_t tempInt;

	memset(param1, 0, sizeof(param1));
	memset(param2, 0, sizeof(param2));
	memset(param3, 0, sizeof(param3));
	memset(param4, 0, sizeof(param4));

	Get_ConfigParam(buffer, param1, sizeof(param1), CONFIG_PARAM_SEPERATOR);
	Get_ConfigParam(buffer, param2, sizeof(param2), CONFIG_PARAM_SEPERATOR);
	Get_ConfigParam(buffer, param3, sizeof(param3), CONFIG_PARAM_SEPERATOR);
	Get_ConfigParam(buffer, param4, sizeof(param4), CONFIG_PARAM_SEPERATOR);
	Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);

	/* validate parameters */


	if( !((strcmp(param1, "1") == 0) ||  (strcmp(param1, "0") == 0 )) ){
		PRINT_K("\nInvalid value for 1st parameter\n");
		return REPLY_DO_NOT_SEND;
	}

	if( !((strcmp(param2, "1") == 0) ||  (strcmp(param2, "0") == 0 )) ){
		PRINT_K("\nInvalid value for 2nd parameter\n");
		return REPLY_DO_NOT_SEND;
	}


	if( !((strcmp(param3, "1") == 0) ||  (strcmp(param3, "0") == 0 )) ){
		PRINT_K("\nInvalid value for 3rd parameter\n");
		return REPLY_DO_NOT_SEND;
	}
	tempInt = atoi(param4);
//	sprintf(printBu,"\nParam 4= %d\n", tempınt);
	if( (tempInt > ( 2 * HOUR) / (SYSTICK_PER_SEC) ) ){
			PRINT_K("\nInvalid value for 4th parameter\n");
			return REPLY_DO_NOT_SEND;
	}


	flash_settings.rfid_ch_setting.checkState    =  atoi(param1);
	flash_settings.rfid_ch_setting.check_ch[0]   =  atoi(param2);
	flash_settings.rfid_ch_setting.check_ch[1]   =  atoi(param3);
	flash_settings.rfid_ch_setting.duration      =  tempInt;

	settings_update_all(&flash_settings);

	BeginEchoPacket(response->buffer, RFID_CHECK_SETTING);
	sprintf(temp,"%s,%s,%s,%s", param1, param2, param3, param4);
	AddStringToEchoPacket(response->buffer, temp);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);
	response->b_needToReset = FALSE;
	return REPLY_ECHO_PACKET;
}
/**
 *
 */

static COMMAND_RESULT_T settings_update_din_func(char *buffer, COMMAND_RESPONSE_T *response)
{
	DIG_INPUT_FUNC_T function;
	char transactionID[16];
	char param1[2], param2[2], param3[16], param4[64];
	uint8_t chNumber;
	bool valid;
	char temp[128];

		memset(param1, 0, sizeof(param1));
		memset(param2, 0, sizeof(param2));
		memset(param3, 0, sizeof(param3));
		memset(param4, 0, sizeof(param4));

		Get_ConfigParam(buffer, param1, sizeof(param1), CONFIG_PARAM_SEPERATOR);               /* din channel number */
		Get_ConfigParam(buffer, param2, sizeof(param2), CONFIG_PARAM_SEPERATOR);               /* function */
		Get_ConfigParam(buffer, param3, sizeof(param3), CONFIG_PARAM_SEPERATOR);               /* gsm number */
		Get_ConfigParam(buffer, param4, sizeof(param4), CONFIG_PARAM_SEPERATOR);               /* sms text */
		Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);
		chNumber = atoi(param1);

		/* valid digital input channel number ? */
		if(chNumber > NUM_DIGITAL_INPUTS || chNumber == 0){
			PRINT_K("\nInvalid channel number(Should be between 1-6)\n");
			return REPLY_DO_NOT_SEND;
		}
		function = atoi(param2);

		/* valid function? */
		if(function > DIN_FUNC_COUNT -1)
			return REPLY_DO_NOT_SEND;

		if( function == DIN_FUNC_SCALE){
			if(chNumber != 1){
				PRINT_K("\nERROR! Scale data input pin can only be assigned to digital input 1\n");
				return REPLY_DO_NOT_SEND;
			}
			else
			goto update;
		}

		valid = validatePhoneNumber(param3, strlen(param3));
		if(valid){
update:			din_func_table[chNumber- 1].function = function;
				strcpy(din_func_table[chNumber- 1].destNumber, param3);
				strcpy(din_func_table[chNumber- 1].sms, param4);
				settings_update_din_func_table();
				settings_print_din_func_table();

				BeginEchoPacket(response->buffer, DIN_SETTING);
				sprintf(temp,"%d,%s,%s", din_func_table[chNumber- 1].function, din_func_table[chNumber- 1].destNumber, din_func_table[chNumber- 1].sms);
				AddStringToEchoPacket(response->buffer, temp);
				AddStringToEchoPacket(response->buffer, transactionID);
				CloseEchoPacket(response->buffer);
				response->b_needToReset = FALSE;
				return REPLY_ECHO_PACKET;
		}
		else
			return REPLY_DO_NOT_SEND;

}
/**************************************************************/
void settings_print_din_func_table()
{
	int i;
	char temp[128];
	PRINT_K("\nDigital input function assignments.(Function: 0= Not assigned, 1= Send SMS, 2= Dial)\n");
	for( i= 0; i< NUM_DIGITAL_INPUTS; i++){
		sprintf(temp, "\nChannel %d function %d, GSM No: \"%s\", SMS: \"%s\"", i + 1, din_func_table[i].function,
																					  din_func_table[i].destNumber,
																					  din_func_table[i].sms);
		PRINT_K(temp);
	}

}
/*************************************************************/
bool validatePhoneNumber(char *number, uint8_t length)
{
	int i;

	for( i = 0; i < length; i++){
		if(!isdigit(number[i])){
			if(i > 0)
				return false;
			else{
				if(number[i] != '+')
					return false;
			}
		}
	}
	return true;
}
/**
 *
 */
uint32_t settings_get_max_idle_time()
{
	return flash_settings.u32_maxIdleTime;
}
/**
 *
 */
uint32_t settings_get_max_stop_time()
{
	return flash_settings.u32_maxStopTime;
}
/**
 *
 */
uint16_t settings_get_speed_limit()
{
	return flash_settings.u16_speedLimit;
}
/**
 *
 */
uint16_t settings_get_speed_limit_violation_duration()
{
	return flash_settings.u16_speedLimitViolationDuration;
}
/**
 *
 */
uint32_t settings_get_msg_period_not_ignited_not_roaming()
{
	return flash_settings.msg_period_not_roaming_not_ignited;
}
/**
 *
 */
uint32_t settings_get_msg_period_not_ignited_roaming()
{
	return flash_settings.msg_period_roaming_not_ignited;
}
/**
 *
 */
uint32_t settings_get_msg_period_ignited_not_roaming()
{
	return flash_settings.msg_period_not_roaming_ignited;
}
/**
 *
 */
uint32_t settings_get_msg_period_ignited_roaming()
{
	return flash_settings.msg_period_roaming_ignited;
}
/**
 *
 */
CAN_BITRATE settings_get_can_bitrate()
{
	return flash_settings.canBusBitrate;
}
/**
 *
 */
CAN_FRAME_TYPE settings_get_can_frame_type()
{
	return flash_settings.canBusFrameType;
}
/**
 *
 */
void settings_check_persisted_blockage()
{
	if(!flash_settings.blockage_status_in_ram){
		if(flash_settings.engine_blockage)
			Set_BlockageStatus(BLOCKAGE_REQUESTED);
		else
			Set_BlockageStatus(BLOCKAGE_NOT_EXIST);
	}
}
/**
 *
 */
uint32_t get_g_threshold_setting()
{
	return flash_settings.g_threshold;
}
/**
 *
 */
bool settings_get_rfid_check(RFID_CH_SETTING_T *rfid_setting)
{
	memcpy(rfid_setting, &(flash_settings.rfid_ch_setting), sizeof(RFID_CH_SETTING_T) );
	return true;
}
/**
 * Returns the associated digital input pin function
 */
DIN_PROPERTIES_T settings_get_din_properties(DIG_INPUT_CH_NUM ch_number)
{
	return din_func_table[ch_number- 1];
}
/**
 * Returns the buzzer alert activation duration setting in secs.
 */
uint16_t settings_get_buzzer_alert_duration()
{
	return flash_settings.rfid_ch_setting.duration *  100 ;
}
/**
 * Calibrate the weight sensor using N samples and reference weigth
 */
/*************************************************************/
static COMMAND_RESULT_T settings_hx711_calibrate(char *buffer, COMMAND_RESPONSE_T *response)
{
	char temp[10];
	char transactionID[16];
	int ref_weigth;
	int calib = 0;
//	SCALE_TASK_EVENT_T scale_event;
	Get_ConfigParam(buffer, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);  /*  c = 0 , m =1  */
	calib = atoi(temp);
	Get_ConfigParam(buffer, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);  /* get reference weight */
	ref_weigth = atoi(temp);
	Get_ConfigParam(buffer, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);



/*	if( calib == 0){
		PRINT_K("\nCalibrating...Remove any weight on the scale\n");
		scale_event.sig = SIGNAL_SCALE_START_CALIBRATION;
		PutEvent_ScaleTask(&scale_event);
	}
	else if(calib == 1){
		PRINT_K("\nCalibrating...Put 1000 g weight on scale\n");
		scale_event.sig = SIGNAL_SCALE_CALIBRATE_M;
		PutEvent_ScaleTask(&scale_event);
	}*///

	//flash_settings.calib_coeffs = hx711_calibrate(avg_count, ref_weigth);
//	settings_update_all(&flash_settings);

	BeginEchoPacket(response->buffer, CALIBRATE_HX711);

	sprintf(temp,"%d;%d", calib, ref_weigth);
	AddStringToEchoPacket(response->buffer, temp);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);
	response->b_needToReset = FALSE;
	return REPLY_ECHO_PACKET;
}
/**
 *
 */
void settings_get_calibration_coefficents(HX711_CALIB_T * calib_coeffs)
{
	memcpy(calib_coeffs, &flash_settings.calib_coeffs, sizeof(HX711_CALIB_T));
}
/**
 *
 */
static COMMAND_RESULT_T settings_activate_lbs(char *buffer, COMMAND_RESPONSE_T *response)
{
	uint8_t param;
	char temp[10];

	Get_ConfigParam(buffer, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);
	param = atoi(temp);
	if(( param == 1) || (param == 0)){
		   GSM_TASK_EVENT_T event;

		   flash_settings.lbsActStatus= param;
		   settings_update_all(&flash_settings);
		   if( param == 1){
			   event.sig = SIGNAL_GSM_LBS_ACTIVATED;
			   PRINT_K("\nLBS Enabled\n");
		   }

		   else{
			   PRINT_K("\nLBS Disabled\n");
			   event.sig = SIGNAL_GSM_LBS_DEACTIVATED;
		   }

		   PutEvent_GsmTask(&event);
		   return REPLY_ECHO_PACKET;
	}
	else
			return REPLY_DO_NOT_SEND;
}
/**
 *
 */
bool settings_check_lbs_activation_status()
{
	if(flash_settings.lbsActStatus)
		return true;
	else
		return false;
}
/**
 *
 */
bool settings_check_engine_hour_activation_status()
{
	if(flash_settings.engineHrCntActStatus)
		return true;
	else
		return false;
}
/*
 * Updates calibration settings in Flash
 */
void update_calibration_settings(HX711_CALIB_T *calib_coeffs)
{
	memcpy(&flash_settings.calib_coeffs, calib_coeffs, sizeof(HX711_CALIB_T));
	settings_update_all(&flash_settings);
}
/**
 * Sets a digital output's status
 */
static COMMAND_RESULT_T settings_set_output_status(char *buffer, COMMAND_RESPONSE_T *response)
{
	char temp[16];
	int i;
	uint8_t ch, status;
	char transactionID[16];


	Get_ConfigParam(NULL, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);   /* get digital output channel number */
	ch = atoi(temp);
	if(ch > NUM_DIG_OUTPUTS)
		return REPLY_DO_NOT_SEND;

	Get_ConfigParam(NULL, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);   /* get requested set status */
	status = atoi(temp);

	for(i = 0; i< NUM_DIG_OUTPUTS; i++){
		if(ch == pioToDout[i].doutNum){
			Chip_GPIO_SetPinState(LPC_GPIO, pioToDout[i].port, pioToDout[i].pio, status);
		}
	}
	Get_ConfigParam(NULL, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);

	BeginEchoPacket(response->buffer, SET_OUTPUT_STATUS);
    sprintf(temp,"%d;%d", ch, status);
	AddStringToEchoPacket(response->buffer, temp);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);
	response->b_needToReset = FALSE;
	return REPLY_ECHO_PACKET;
}
/**
 *
 */
static COMMAND_RESULT_T settings_activate_engine_hour_counter(char *buffer, COMMAND_RESPONSE_T *response)
{
	char temp[8];
	char transactionID[16];

	Get_ConfigParam(NULL, temp, sizeof(temp), CONFIG_PARAM_SEPERATOR);   /* get requested status */
	Get_ConfigParam(NULL, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);

	flash_settings.engineHrCntActStatus = atoi(temp);

	settings_update_all(&flash_settings);

	BeginEchoPacket(response->buffer, ACTIVATE_ENGINE_HOUR_COUNTER);
	sprintf(temp,"%d", flash_settings.engineHrCntActStatus);
	AddStringToEchoPacket(response->buffer, temp);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);
	response->b_needToReset = FALSE;

	if(flash_settings.engineHrCntActStatus == 0)
		PRINT_K("\nEngine Hour Counter Disabled\n");
	else
		PRINT_K("\nEngine Hour Counter Enabled\n");

	return REPLY_ECHO_PACKET;
}
/**
 *
 */
bool settings_update_engine_hour_counter(uint32_t newValue)
{
	uint32_t address;

	address = get_next_write_address_in_page(ENGINE_HOUR_COUNT_ADDRESS);

	sst25_disable_write_lock();   /* disable flash protection */
	sst25_write_array((char *)&newValue, sizeof(newValue), address);
	sst25_enable_write_lock();   /* enable flash protection */
	return TRUE;
}
/**
 *
 */
static COMMAND_RESULT_T settings_generate_output_pulse(char *buffer, COMMAND_RESPONSE_T *response)
{
	DOUT_TASK_EVENT_T dout_event;

	char temp1[32];
	char temp2[8];
	char transactionID[16];

	Get_ConfigParam(NULL, temp1, sizeof(temp1), CONFIG_PARAM_SEPERATOR);                       /* digital output number */
	Get_ConfigParam(NULL, temp2, sizeof(temp2), CONFIG_PARAM_SEPERATOR);                       /* pulse duration */
	Get_ConfigParam(NULL, transactionID, sizeof(transactionID), TRANSACTION_ID_SEPERATOR);     /* transaction ID */

	dout_event.sig = SIGNAL_DOUT_ACTIVATE;
	dout_event.param1 = atoi(temp1);                /* pass dout. channel number as parameter */
	dout_event.param2 = atoi(temp2);                /* pass pulse duration as parameter */
	PutEvent_DoutTask(&dout_event);

	BeginEchoPacket(response->buffer, SET_CMD_DOUT_PULSE);
	sprintf(temp1,"%d;%d", dout_event.param1, dout_event.param2);
	AddStringToEchoPacket(response->buffer, temp1);
	AddStringToEchoPacket(response->buffer, transactionID);
	CloseEchoPacket(response->buffer);
	response->b_needToReset = FALSE;
	return REPLY_ECHO_PACKET;
}
