/*
 * rf_task.h
 *
 *  Created on: 24 Eki 2016
 *      Author: admin
 */

#ifndef RF_TASK_H_
#define RF_TASK_H_

#define HEADER_MODULE_ID        "1="
#define HEADER_MESSAGE_TYPE     "2="
#define HEADER_DOOR_STATUS      "3="
#define HEADER_TEMPERATURE      "4="
#define HEADER_BATT_VOLTAGE     "5="
#define HEADER_ALARM_STATUS     "6="
#define HEADER_RSSI             "/-"

#define     NUMBER_OF_TRAILER_ID_MESSAGES       5

#define     LE50_UART_RRB_SIZE                  512
#define     LE50_UART_SRB_SIZE                  128
#define		COM3_RECV_BUFFER_SIZE               256
#define     RF_MAX_MSG_FIELD_SIZE               16
#define     RF_PACKET_TIMEOUT                   100
#define     RF_NODE_ALIVE_TIMEOUT               (2* MINUTE)
#define     RF_NODE_MSG_FIELD_SEPERATOR         ';'
#define     NUMBER_OF_RF_NODE_DOOR_CHANNELS      2



#define     TRILER_DETECTION_DELAY_TIME         (1 * MINUTE)
#define     RF_DATA_IDLE_TIMEOUT                2

#define     INVALID_NODE_ID                       "00000000000"
#define     MAX_NUMBER_OF_NODES                  32
#define     LE50_SERIAL_NUMBER_SIZE              11
#define     RF_NODE_TEMP_CHANNELS                2
#define     RF_NODE_DOOR_CHANNELS                2

#define     CAP_SENSOR_ALARM_ID                  2

typedef enum APP_ID{
	APP_ID_DOOR_SENSOR = 1,
	APP_ID_TEMP_SENSOR,
	APP_ID_TRAILER_IDENT
}APP_ID_T;

typedef enum ALARM{
	RF_NODE_NO_ALARM,
	RF_NODE_DOOR1_OPENED,
	RF_NODE_DOOR1_CLOSED,
	RF_NODE_DOOR2_OPENED,
	RF_NODE_DOOR2_CLOSED
}ALARM_T;

typedef enum RF_NODE_TYPE{
	NODE_TYPE_TRAILER_TRACKER = 0,
	NODE_TYPE_SENSOR,
	NODE_TYPE_CAP_SENSOR
}RF_NODE_TYPE_T;

typedef struct RF_NODE{
	//APP_ID_T app_id;
	TIMER_INFO_T heartbeat_timer;
	ALARM_T alarm_status;
	uint16_t bat_voltage;
	uint16_t node_number;
	int16_t rssi;
	int ds18b20_channel[RF_NODE_TEMP_CHANNELS];
	char node_id[LE50_SERIAL_NUMBER_SIZE + 1];
	bool door_status[RF_NODE_DOOR_CHANNELS];
	bool isAlive;
	RF_NODE_TYPE_T node_type;
}RF_NODE_T;
typedef struct DS18B20_CH{
	bool present;
	int tempValue;
}DS18B20_CH_T;


typedef struct NODE_SENSOR_TYPE_DEFS{
	 uint8_t index;
	 char *type;
	 char *unit;
	 uint16_t (*fill_value_cb)(RF_NODE_T *node, uint8_t index);
}NODE_SENSOR_TYPE_DEFS_T;

typedef struct REG_NODE_INFO{
	uint16_t node_number;
	char node_id[LE50_SERIAL_NUMBER_SIZE + 1];
}REG_NODE_INFO_T;

typedef struct RF_MSG_PARSER_INFO
{
	char str_app_id[4];
	bool (*field_parser_callback)(RF_NODE_T *node, char * pos, char *buffer);
}RF_MSG_PARSER_INFO_T;

void le50_uart_init(uint32_t baud_rate);
void rf_send(char *command);
void read_registered_node_table();
bool rf_get_node_in_ram(RF_NODE_T *node, uint16_t index);
int16_t rf_search_node_in_array(RF_NODE_T *node);
int16_t rf_find_free_node_array_index();
int16_t rf_find_highest_trailer_id_rssi_node_index();
bool rf_add_node_to_array(RF_NODE_T *node);
bool rf_get_node_presence_status();
bool rf_check_node_alarm_exist();
int rf_is_registered_node(RF_NODE_T *node);
uint8_t rf_get_alive_nodes(RF_NODE_T *node);
void rf_get_registered_node_list(REG_NODE_INFO_T *reg_node_list);
void rf_update_node_table(RF_NODE_T *node);
bool rf_get_node_alarm(RF_NODE_T *node);
bool rf_get_node_alarm(RF_NODE_T *node);
bool rf_identify_trailer(RF_NODE_T *node);
bool rf_is_trailer_identified();
void rf_update_alive_status();
void rf_process_trailer_tracking_message();
bool rf_get_trailer_node_id(char *node_id);
bool rf_parse_packet(char *recv_buffer, RF_NODE_T *node);
uint16_t rf_get_rx_buffer(char *buffer,  uint16_t buffer_size);
void rf_clear_node_alarm(uint16_t node_index);
bool json_populate_sensor_info(RF_NODE_T *node, JSON_TYPE_SENSOR_T *json_sensor);
void json_fill_sensor_items(JSON_TYPE_SENSOR_T *json_sensor, RF_NODE_T *node, uint16_t indx);
uint16_t temp_value_cb(RF_NODE_T *node, uint8_t indx);
uint16_t door_value_cb(RF_NODE_T *node, uint8_t indx);
bool task_rf();
void rf_init();
#endif /* RF_TASK_H_ */
