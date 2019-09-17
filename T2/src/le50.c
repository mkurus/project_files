
#include "bsp.h"
#include "board.h"
#include "timer.h"
#include "settings.h"
#include "gsm.h"
#include "gps.h"
#include "adc.h"
#include "can.h"
#include "rf_task.h"
#include "onewire.h"
#include "MMA8652.h"
#include "gSensor.h"
#include "utils.h"
#include "le50.h"
#include "io_ctrl.h"
#include "trace.h"
#include "status.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
bool le50_power_off_callback_bfr();
bool le50_power_on_callback_bfr();
bool activate_at_mode_callback_aft(char *);
bool firmware_version_callback_aft(char *);
bool set_operating_mode_callback_aft(char *);
bool set_network_opts_callback_aft(char *);
bool set_low_power_mode_callback_aft(char *);
bool set_aes_key_callback_aft(char *);
bool activate_online_mode_callback_aft(char *);

uint16_t le50_recv(char *buffer);

static const LE50_INIT_T le50_init_t[] = {
		{ "",                            le50_power_off_callback_bfr,  NULL,                                50 , 1},
		{ "",                            le50_power_on_callback_bfr,   NULL,                                50 , 1},
		{ "+++",                         NULL,                         activate_at_mode_callback_aft,       100, 10},
		{ "AT/V\r" ,                     NULL,                         firmware_version_callback_aft,       50,  1},
		{ "ATS220=9\r" ,                 NULL,                         set_operating_mode_callback_aft,     50,1},
		{ "ATS255=138\r" ,               NULL,                         set_network_opts_callback_aft,       50,1},
		{ "ATS240=2\r" ,                 NULL,                         set_low_power_mode_callback_aft,     50,1},
		{ "ATS280=1234567890123456\r" ,  NULL,                         set_aes_key_callback_aft,            50,1},
		{ "ATO\r" ,                      NULL,                         activate_online_mode_callback_aft,   50,1},
};

TIMER_INFO_T le50_detect_timer;
TIMER_INFO_T le50_cmd_timer;

bool le50_mount_status = FALSE;
uint8_t retry =0;
/*************************************/
bool le50_init()
{
	//LE50_EVENT_T event;
	//static int i =0;
	char buffer[128];
	char temp[128];
//	bool result;
	le50_power_off();
	le50_power_on();


	/*while(1){
		switch(le50_state_t){
			case LE50_SEND_COMMAND_STATE:
			if(le50_init_t[cmd_index].before_callback !=NULL)
				le50_init_t[cmd_index].before_callback();

			if(strlen(le50_init_t[cmd_index].command) > 0){
				rf_send(le50_init_t[cmd_index].command);
				memset(buffer, 0, sizeof(buffer));
			}
			Set_Timer(&le50_cmd_timer, le50_init_t[cmd_index].timeout);
			le50_state_t = LE50_WAIT_RESPONSE_STATE;
			break;

			case LE50_WAIT_RESPONSE_STATE:
			if(mn_timer_expired(&le50_cmd_timer)){
				rf_get_rx_buffer(buffer, sizeof(buffer), LE50_868_IDLE_TIMEOUT);
				if(le50_init_t[cmd_index].after_callback !=NULL){
					result = le50_init_t[cmd_index].after_callback(buffer);
					if(!result){
						retry++;
						if(retry == le50_init_t[cmd_index].retry){
								le50_state_t = LE50_IDLE;
								return false;
						}
						else
							le50_state_t = LE50_SEND_COMMAND_STATE;
					}
					else{
						cmd_index++;
						if(cmd_index == sizeof(le50_init_t)/sizeof(le50_init_t[0])){
							le50_mount_status = true;
							le50_state_t = LE50_IDLE;
							return true;
						}
						else
							le50_state_t = LE50_SEND_COMMAND_STATE;
					}
				}
				else{
					cmd_index++;
					if(cmd_index == sizeof(le50_init_t)/sizeof(le50_init_t[0])){
						le50_mount_status = true;
						le50_state_t = LE50_IDLE;
						return true;
					}
					else
						le50_state_t = LE50_SEND_COMMAND_STATE;
				}

			}
			break;

			case LE50_IDLE:
			break;
		}

	}
	return;
*/
	 Set_Timer(&le50_detect_timer, LE50_DETECT_TIMEOUT);
	 while(!mn_timer_expired(&le50_detect_timer)){
		 if(le50_activate_at_mode()){
				 PRINT_K("\nRF module detected\n");
				 le50_mount_status = TRUE;
				 break;
		 }
	}

	if(!le50_mount_status)
		return FALSE;

	if(le50_get_firmware_version(buffer)){
		sprintf(temp,"\nRF Module Firmware Version: %s\n", buffer);
		PRINT_K(temp);
		memset(buffer,0, sizeof(buffer));
	}
	else
		return false;

	if(le50_set_operating_mode(LE50_MODE_ADDRESSED_SECURE))
		PRINT_K("\nAddressed Secure Mode selected\n");
	else
		return false;

	 if(le50_set_network_options(LE50_OPTS_NO_HEADER |
			                    LE50_OPTS_AES |
								LE50_OPTS_CR |
								LE50_OPTS_RADIO_RSSI)){
		PRINT_K("\nNetwork options set\n");
	//	sprintf(buffer,"Network options %X",LE50_OPTS_NO_HEADER | LE50_OPTS_AES);
	}

	else
		return false;

	if(le50_set_low_power_mode(LE_OPTS_LP_SERIAL))   /* low power mode enabled by at command */
		PRINT_K("\nLow power mode selected\n");
	else
		return false;

	if(le50_set_aes_key(aes_key))
		PRINT_K("\nAES key set\n");
	else
		return false;

	if(le50_activate_online_mode()){
		PRINT_K("\nOnline mode activated\n");
		return true;
	}

	else{
		PRINT_K("\nCannot enter Online mode!!!\n");
    	return false;
	}

}
/**
 *
 */
bool le50_get_serial_number(char *serial_number)
{
	uint8_t i;
	char buffer[16];

	memset(buffer,0,sizeof(buffer));

	rf_send((char *)le50_cmd_get_serial_number);
	le50_get_response(buffer, sizeof(buffer), LE50_AT_COMMAND_RESP_TIMEOUT);
	for(i =0; i < LE50_SERIAL_NUMBER_SIZE; i++){
		if(!isalnum((int)buffer[i]))
			return false;
	}
	strcpy(serial_number,buffer);
	return TRUE;
}
/**
 *
 */
bool le50_set_low_power_mode(LE50_LOWPOWER_MODE_T mode)
{
	char buffer[16];
	char command[16];

	memset(command, 0, sizeof(command));
	sprintf(buffer,"%d", mode);
	strcat(command, le50_cmd_set_lowpower_mode);
	strcat(command, buffer);
	strcat(command,"\r");
	rf_send(command);
	PRINT_K(command);
	le50_get_response(buffer, sizeof(buffer), LE50_AT_COMMAND_RESP_TIMEOUT);
	if(strstr(buffer, str_le50_OK) != NULL)
		return TRUE;
	else
		return FALSE;
}
/**
 *
 */
bool le50_set_operating_mode(LE50_OPERATING_MODE_T mode)
{
	char buffer[16];
	char command[16];

	memset(command, 0, sizeof(command));

	sprintf(buffer,"%d", mode);
	strcat(command, le50_cmd_set_operating_mode);
	strcat(command, buffer);
	strcat(command,"\r");
	rf_send(command);
	PRINT_K(command);
	le50_get_response(buffer, sizeof(buffer), LE50_AT_COMMAND_RESP_TIMEOUT);
	if(strstr(buffer, str_le50_OK) != NULL)
		return TRUE;
	else
		return FALSE;
}
/**
 *
 */
bool le50_set_aes_key(const char *key)
{
	char buffer[16];
	char command[16];

	memset(command, 0, sizeof(command));
	strcat(command, le50_cmd_set_aes_key);
	strcat(command, key);
	strcat(command, "\r");
	rf_send(command);
	PRINT_K(command);
	le50_get_response(buffer, sizeof(buffer), LE50_AT_COMMAND_RESP_TIMEOUT);
	PRINT_K(buffer);
	if(strstr(buffer, str_le50_OK) != NULL)
		return TRUE;
	else
		return FALSE;
}
/**
 *
 */
bool le50_set_network_options(uint8_t options)
{
	char buffer[16];
	char command[16];

	memset(command, 0, sizeof(command));
	sprintf(buffer,"%d", options);
	strcat(command, le50_cmd_set_network_opt);
	strcat(command, buffer);
	strcat(command,"\r");
	rf_send(command);
	PRINT_K(command);
	le50_get_response(buffer, sizeof(buffer), LE50_AT_COMMAND_RESP_TIMEOUT);
	PRINT_K(buffer);
	if(strstr(buffer, str_le50_OK) != NULL)
		return TRUE;
	else
		return FALSE;
}
/**
 *
 */
bool le50_activate_at_mode()
{
	char buffer[16];
	PRINT_K(le50_cmd_set_at_mode);
	rf_send((char *)le50_cmd_set_at_mode);
	le50_get_response(buffer, sizeof(buffer), LE50_AT_COMMAND_RESP_TIMEOUT);
	PRINT_K(buffer);
	if(strstr(buffer, str_le50_OK) != NULL)
		return TRUE;
	else
		return FALSE;
}
/**
 *
 */
bool le50_activate_online_mode()
{
	char buffer[16];
	rf_send((char *)le50_cmd_set_online_mode);
	PRINT_K(le50_cmd_set_online_mode);
	le50_get_response(buffer, sizeof(buffer), LE50_AT_COMMAND_RESP_TIMEOUT);
	PRINT_K(buffer);
	if(strstr(buffer, str_le50_OK) != NULL)
		return TRUE;

	else
		return FALSE;
}


/*************************************/
uint16_t le50_get_response(char *buffer, uint16_t buffer_size, uint32_t timeout)
{
	TIMER_INFO_T at_response_timer;
	TIMER_INFO_T idle_timer;
	uint16_t bytes_read;
	uint16_t total_bytes = 0;
	char temp[16];

	memset(buffer,0,buffer_size);

	Set_Timer(&at_response_timer, timeout);

	while(!mn_timer_expired(&at_response_timer)){
		bytes_read = rf_get_rx_buffer(temp, sizeof(temp));
	    if(bytes_read > 0){
	    	Set_Timer(&idle_timer, LE50_868_IDLE_TIMEOUT);
	    	if((total_bytes + bytes_read) >= buffer_size){
	    		memcpy(&buffer[total_bytes], temp, (buffer_size - total_bytes));
	    		return buffer_size;
	    	}

	    	else
	    		memcpy(&buffer[total_bytes], temp, bytes_read);
	    	total_bytes += bytes_read;
	    }
	    else{
	        if((mn_timer_expired(&idle_timer)) && ( total_bytes > 0)){
	        	return total_bytes;
	        }
	     }
	}

	return total_bytes;
}

/*************************************/
bool le50_is_mounted()
{
	return le50_mount_status;
}
/*************************************/
bool le50_get_firmware_version(char * buffer)
{
	char temp[64];

	rf_send((char *)le50_cmd_get_firm_version);
	PRINT_K(le50_cmd_get_firm_version);
	le50_get_response(temp, sizeof(temp), LE50_AT_COMMAND_RESP_TIMEOUT);

//	PRINT_K(temp);
	memcpy(buffer, temp, sizeof(temp));
	return TRUE;

	/*else
		return FALSE;*/
}
/**
 *
 */
bool activate_online_mode_callback_aft(char *buffer)
{
	PRINT_K(buffer);
	if(strstr(buffer, str_le50_OK) != NULL)
		return TRUE;
	else
		return FALSE;
}
/**
 *
 */
bool set_aes_key_callback_aft(char *buffer)
{
	PRINT_K(buffer);
	if(strstr(buffer, str_le50_OK) != NULL)
		return TRUE;
	else
		return FALSE;
}
/**
 *
 */
bool set_low_power_mode_callback_aft(char *buffer)
{
	if(strstr(buffer, str_le50_OK) != NULL)
		return TRUE;
	else
		return FALSE;
}
/**
 *
 */
bool set_network_opts_callback_aft(char *buffer)
{
	PRINT_K(buffer);
	if(strstr(buffer, str_le50_OK) != NULL)
		return TRUE;
	else
		return FALSE;
}
/**
 *
 */
bool set_operating_mode_callback_aft(char *buffer)
{
	PRINT_K(buffer);
	if(strstr(buffer, str_le50_OK) != NULL)
		return TRUE;
	else
		return FALSE;
}
/**
 *
 */
bool firmware_version_callback_aft(char *buffer)
{
	PRINT_K(buffer);
	//memcpy(buffer, temp, sizeof(temp));

	return TRUE;
}
/**
 *
 */
bool activate_at_mode_callback_aft(char *buffer)
{
	PRINT_K(buffer);
	if(strstr(buffer, str_le50_OK) != NULL)
		return TRUE;
	else
		return FALSE;
}
/**
 *
 */
bool le50_power_off_callback_bfr(char *buffer)
{
	PRINT_K("\nPower-Off LE50\n");
	Chip_GPIO_SetPinState(LPC_GPIO, RF_POWER_CUTOFF_PORT_NUM, RF_POWER_CUTOFF_PIN_NUM, FALSE);
	return true;
}
/**
 *
 */
bool le50_power_on_callback_bfr(char *buffer)
{
	PRINT_K("\nPower-On LE50\n");
	Chip_GPIO_SetPinState(LPC_GPIO, RF_POWER_CUTOFF_PORT_NUM, RF_POWER_CUTOFF_PIN_NUM, TRUE);
	return true;
}
/**
 *
 */
/*************************************/
void le50_power_on()
{
	PRINT_K("\nPower-On LE50\n");
	Chip_GPIO_SetPinState(LPC_GPIO, RF_POWER_CUTOFF_PORT_NUM, RF_POWER_CUTOFF_PIN_NUM, TRUE);
	Delay(50, NULL);
}
/**
 *
 */
/*************************************/
void le50_power_off()
{
	PRINT_K("\nPower-Off LE50\n");
	Chip_GPIO_SetPinState(LPC_GPIO, RF_POWER_CUTOFF_PORT_NUM, RF_POWER_CUTOFF_PIN_NUM, FALSE);
	Delay(5, NULL);
}
/**
 *
 */
/*************************************/
void le50_toggle_power()
{
	PRINT_K("\nToggle LE50 power\n");
	le50_power_off();
	Chip_GPIO_SetPinState(LPC_GPIO, RF_POWER_CUTOFF_PORT_NUM, RF_POWER_CUTOFF_PIN_NUM, TRUE);
}
