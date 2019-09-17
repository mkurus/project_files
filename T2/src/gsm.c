#include "chip.h"
#include "board.h"
#include "ttypes.h"
#include "bsp.h"
#include "timer.h"
#include "event.h"
#include "iap_config.h"
#include "ProcessTask.h"
#include "ATCommands.h"
#include "QuectelM95.h"
#include "settings.h"
#include "messages.h"
#include "io_ctrl.h"
#include "offline.h"
#include "gsm.h"
#include "gsm_task.h"
#include "offline_task.h"
#include "scale_task.h"
#include "gps.h"
#include "rf_task.h"
#include "status.h"
#include "trace.h"
#include "utils.h"
#include "MMA8652.h"
#include "gSensor.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
/*********************************************************/
#define GSM_PORT                              LPC_UART1

#define MAX_RESPONSE_LENGTH                   32
#define MAX_COMMAND_LENGTH                    32
#define AT_RESPONSE_CHAR_TIMEOUT              10
#define GSM_RESET_RETRY_COUNT                 5
#define GSM_INIT_COMMAND_RETRY_COUNT          3
#define MAX_NUMBER_OF_FAILS_FOR_MCU_RESET     5
#define MAX_NUMBER_OF_FAIL_COUNT              5


static void Initialize_Timers();
uint16_t gsm_get_at_response(char *buffer, uint16_t length, uint16_t timeout, void (*callbackFunc)());
COMMAND_RESULT_T gsm_read_sms(char *buffer, COMMAND_RESPONSE_T *commandResponse, char *smsOrigAddrBuf);
int16_t Get_ValueByParamIndex(char *buffer, int param_no, AT_PARAM_TYPE param_type, char *out_buffer);

void Get_GsmInfo(GSM_INFO_T * gsm_info);
bool gsm_get_waiting_call_list(char *buffer, GSM_CALL_T *gsm_call);
bool InsertImeiToOfflineData(char *buffer);
bool gsm_parse_registration_status(char *buffer, uint8_t *gsmRegStatus);
bool gsm_close_socket(uint8_t retryCount);
bool gsm_select_audio_channel(GSM_AUDIO_CH channel, uint8_t retryCount);
bool gsm_set_mic_gain(GSM_AUDIO_CH channel, uint8_t retryCount, uint8_t gain);
bool gsm_softreset_module(uint8_t retryCount);
bool gsm_disable_echo(uint8_t retryCount);
bool gsm_get_imei(uint8_t retryCount);
bool gsm_get_imsi(uint8_t retryCount);
bool gsm_get_local_ip();
bool gsm_get_signal_power(char *buffer, uint8_t *gsmSigPower);
bool gsm_get_battery_voltage(char *buffer, uint16_t *batVoltage);
bool gsm_get_pin_status(uint8_t retryCount);
bool gsm_get_registration_status(TIMER_TICK_T regTimeout);
bool gsm_initialize_module();
bool gsm_attach_gprs();
bool gsm_set_ip_address_mode(bool b_alpha, uint8_t retryCount);
bool gsm_set_foreground_context(uint8_t context_num, uint8_t retryCount);
bool gsm_set_mux_mode(uint8_t mux_mode, uint8_t retryCount);
bool gsm_set_mode(uint8_t mux_mode, uint8_t retryCount);
bool gsm_set_recv_indicator_mode(uint8_t indi_mode, uint8_t retryCount);
bool gsm_activate_pdp_context(char *apn, char *username, char *password, TIMER_TICK_T attach_timeout);
bool gsm_start_sending_message(uint16_t i_msgLen, uint8_t retryCount);
void gsm_send_at_command(char *,char *);
bool Extract_SmsOrigAddr(char * buffer, char *smsOrigAddrBuf);
bool ConnectToServer(uint8_t retryCount);
bool gsm_answer_incoming_call(GSM_CALL_T *gsm_call, uint8_t retryCount);
bool ExtractIMEI(char *buffer, char *imei);
uint16_t Get_TcpDataSegment(char *buffer, uint16_t recvLength, void (*callback)());
void gsm_connect_to_server(char *p_serverIp, char *p_tcpPort);
void gsm_update_registration_status(uint8_t regStatus);
void Gsm_PowerOn();
int ConnectedToServer();
void RequestUpgrade( char* ip, char* port);
static bool gsm_parse_lbs_response(LBS_INFO_T *lbs_info_t, char *response);
static bool gsm_update_qloc_timeout();
extern bool requestConnectToUpdateServer;
TCP_CONNECTION_INFO_T tcpConnectionInfo_t;

/*****************************************************************
 Pin muxing configuration
 refer to section 8.5.5 Pin Function Select Register 4 of UM1060
 to select IOCON_FUNC mode
****************************************************************/
static const PINMUX_GRP_T T2_GsmPortMuxTable[] = {
	{2,  0,   IOCON_MODE_INACT | IOCON_FUNC2},	/* TXD1 */
	{2,  1,   IOCON_MODE_INACT | IOCON_FUNC2},	/* RXD1 */
};
/*****************************************************************/

#define MAX_AT_RESPONSE_LENGTH      256
#define GSM_BAUD_RATE   		    115200
#define GSM_UART_ISR_HANDLER 	    UART1_IRQHandler
#define GSM_IRQ_SELECTION 	        UART1_IRQn

#define AT_RESPONSE_TIMEOUT         250

typedef enum {
	MODULE_RESET_STATE,
	MODULE_INITIALIZING_STATE,
	MODULE_ATTACHING_TO_GPRS_STATE,
	MODULE_CONNECTING_TO_SERVER_STATE,
	MODULE_CONNECTED_TO_SERVER_STATE,
	MODULE_CONNECTED_TO_UPDATE_SERVER_STATE,
	MODULE_WAIT_IN_ROAMING_STATE,
	MODULE_AAA_STATE
}GSM_MODULE_STATE;

static GSM_MODULE_STATE t_gsmState;
static GSM_INFO_T gsm_info;
static RINGBUFF_T gsm_txring,gsm_rxring;          /* Transmit and receive ring buffers */

static TIMER_INFO_T STATUS_CHECK_TIMER;
static TIMER_INFO_T SEND_PERIODIC_DATA_TIMER;
static TIMER_INFO_T KEEP_ALIVE_TIMER;
static TIMER_INFO_T SEND_OFFLINE_DATA_TIMER;
static TIMER_INFO_T SEND_FAIL_TIMER;

static int32_t i_tempNonAckedBytes = 0;
static uint8_t gsm_rxbuff[GSM_UART_RRB_SIZE]  __attribute__ ((section (".big_buffers")));
static uint8_t gsm_txbuff[GSM_UART_SRB_SIZE]  __attribute__ ((section (".big_buffers")));     /* Transmit and receive buffers */
static uint8_t u8_gsmModemFailCount;
static uint8_t u8_counterMcuReset;

char messageBuffer[MAX_T_MESSAGE_SIZE] __attribute__ ((section (".big_buffers"))) ;


void gsm_init()
{
	Chip_IOCON_SetPinMuxing(LPC_IOCON, T2_GsmPortMuxTable, sizeof(T2_GsmPortMuxTable) / sizeof(PINMUX_GRP_T));

	Chip_UART_Init(GSM_PORT);
	Chip_UART_SetBaud(GSM_PORT, GSM_BAUD_RATE);
	Chip_UART_ConfigData(GSM_PORT, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS);

	/* Enable UART Transmit */
	Chip_UART_TXEnable(GSM_PORT);

	RingBuffer_Init(&gsm_rxring, gsm_rxbuff, 1, GSM_UART_RRB_SIZE);
	RingBuffer_Init(&gsm_txring, gsm_txbuff, 1, GSM_UART_SRB_SIZE);

	/* Reset and enable FIFOs, FIFO trigger level 3 (14 chars) */
/*	Chip_UART_SetupFIFOS(GSM_PORT, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
						 UART_FCR_TX_RS | UART_FCR_TRG_LEV3));*/
	Init_GsmTask();

	/* set initial status for power-key and emergency-off control pins */

	Delay(50, NULL);
	gsm_power_off_module();
	Delay(QUECTEL_M66_POWER_ON_OFF_DELAY , NULL);
	gsm_power_on_module();

	gsm_hard_reset();
	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(GSM_PORT, (UART_IER_RBRINT | UART_IER_RLSINT));

	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(GSM_IRQ_SELECTION, 1);
	NVIC_EnableIRQ(GSM_IRQ_SELECTION);
	Set_Timer(&SEND_FAIL_TIMER, SEND_FAIL_TIMEOUT);
}
/*******************************************************************
* Interrupt handler for GSM UART interface                         *
********************************************************************/
void  UART1_IRQHandler(void)
{
	Chip_UART_IRQRBHandler(GSM_PORT, &gsm_rxring, &gsm_txring);
}
/*******************************************************************/
bool gsm_scan()
{
	int i_msgLen;
	int result = 0;

	if(mn_timer_expired(&SEND_FAIL_TIMER))
		return false;

	switch(t_gsmState)
	{
		case MODULE_RESET_STATE:
		if(gsm_softreset_module(GSM_RESET_RETRY_COUNT) == TRUE){
			t_gsmState = MODULE_INITIALIZING_STATE;
			u8_gsmModemFailCount = 0;
		}
		else{
			u8_counterMcuReset++;
			if(u8_counterMcuReset == MAX_NUMBER_OF_FAILS_FOR_MCU_RESET)
				NVIC_SystemReset();
		}
		break;

		/*****/

		case MODULE_INITIALIZING_STATE:
		if(gsm_initialize_module() == TRUE){
			u8_gsmModemFailCount = 0;
			gsm_check_modem_status (NUMBER_OF_STATUS_CHECK_RETRIES);
			t_gsmState = MODULE_ATTACHING_TO_GPRS_STATE;
		}
		else{
			if(u8_gsmModemFailCount++ == MAX_NUMBER_OF_FAIL_COUNT){
				u8_gsmModemFailCount = 0;
				t_gsmState = MODULE_RESET_STATE;
			}
		}
		break;

		/*****/

		case MODULE_ATTACHING_TO_GPRS_STATE:
		if(gsm_attach_gprs() == TRUE){
			u8_gsmModemFailCount = 0;
			FLASH_SETTINGS_T user_settings;
			settings_get(&user_settings);

			if(requestConnectToUpdateServer){
				requestConnectToUpdateServer = FALSE;
				if(ConnectToServer(NUMBER_OF_CONNECT_RETRY) == TRUE)
					t_gsmState = MODULE_CONNECTED_TO_UPDATE_SERVER_STATE;
				else{
					gsm_set_connection_parameters(user_settings.server_ip, user_settings.server_port);
					t_gsmState = MODULE_CONNECTING_TO_SERVER_STATE;
				}
			}
			else{
				gsm_set_connection_parameters(user_settings.server_ip, user_settings.server_port);
				t_gsmState = MODULE_CONNECTING_TO_SERVER_STATE;
			}
		}
		else{
			if(u8_gsmModemFailCount++ == MAX_NUMBER_OF_FAILS_FOR_MCU_RESET){
				u8_gsmModemFailCount = 0;
				t_gsmState = MODULE_RESET_STATE;
			}
		}
		break;
		/*****/
		case MODULE_CONNECTING_TO_SERVER_STATE:
		if(ConnectToServer(NUMBER_OF_CONNECT_RETRY) == TRUE){

			GSM_TASK_EVENT_T  gsm_event;
		//	OFFLINE_TASK_EVENT_T offlineEvt;
			gsm_event.sig = SIGNAL_GSM_CONNECTED_SERVER;
		//	offlineEvt.sig = SIGNAL_OFFLINE_CONNECTED_SERVER;
			PutEvent_GsmTask(&gsm_event);
		//	PutEvent_OfflineTask(&offlineEvt);

		//	gsm_get_local_ip();
			i_msgLen = Trio_PrepareSTMessage(messageBuffer, sizeof(messageBuffer));
			gsm_send_message(messageBuffer, i_msgLen, on_idle);
			i_msgLen = Trio_PrepareTMessage(messageBuffer, TRUE);
			if(gsm_send_message(messageBuffer, i_msgLen, on_idle)){
				gsm_info.b_socketActive = TRUE;
				u8_gsmModemFailCount = 0;
				Initialize_Timers();
				t_gsmState = MODULE_CONNECTED_TO_SERVER_STATE;
			}
			else if(u8_gsmModemFailCount++ == MAX_NUMBER_OF_FAILS_FOR_MCU_RESET){
					u8_gsmModemFailCount = 0;
					t_gsmState = MODULE_RESET_STATE;
			}
		}
		else if(u8_gsmModemFailCount++ == MAX_NUMBER_OF_FAILS_FOR_MCU_RESET){
				u8_gsmModemFailCount = 0;
				t_gsmState = MODULE_RESET_STATE;
		}
		break;
		/*****/
		case MODULE_CONNECTED_TO_SERVER_STATE:
		result = ConnectedToServer();
		if(result == -1){
			if(u8_gsmModemFailCount++ == MAX_NUMBER_OF_FAIL_COUNT){
			   u8_gsmModemFailCount = 0;
			   gsm_info.b_socketActive = FALSE;
			   t_gsmState = MODULE_RESET_STATE;
			}
			else{
				Initialize_Timers();
				PRINT_K("\nCONNECTED TO SERVER RETURNED ERROR\n");
				gsm_info.gsm_stats.u16_totalBytesNonAcked1 = gsm_info.gsm_stats.u16_totalBytesNonAcked2;
			    i_tempNonAckedBytes = 0;
			}
		}
		else if(result == 1)
				u8_gsmModemFailCount = 0;
		else if(result == 4){
			PRINT_K("\nFirmware Update requested\n" );
			gsm_close_socket(SOCKET_CLOSE_RETRIES);
			RequestUpgrade(tcpConnectionInfo_t.tcpServerInfo_t.server_ip,
						 tcpConnectionInfo_t.tcpServerInfo_t.server_port);
		}
		break;


		case MODULE_CONNECTED_TO_UPDATE_SERVER_STATE:
	//	XModem1K_Client(gsm_info.imei_no );
		NVIC_SystemReset();
		break;
	}
	return true;
}
/*******************************************************************/
void RequestUpgrade( char* ip, char* port)
{
	char buff[1024];
	int i;

	PRINT_K( "\nUpdating with parameters\n");
	PRINT_K( ip );
	PRINT_K( port );

     __disable_irq();
	Chip_IAP_PreSectorForReadWrite(UPGRADE_PARAMETERS_SEC, UPGRADE_PARAMETERS_SEC);
	Chip_IAP_EraseSector(UPGRADE_PARAMETERS_SEC, UPGRADE_PARAMETERS_SEC);
	 __enable_irq();

	for( i = 0; i < 1024; i++ )
		buff[i] = 0xFF;

	strcpy( buff, ip);

	char* pPort = strchr( buff, '\0' );

	pPort++;
	strcpy ( pPort, port );
	 __disable_irq();
	Chip_IAP_PreSectorForReadWrite(UPGRADE_PARAMETERS_SEC, UPGRADE_PARAMETERS_SEC );
	Chip_IAP_CopyRamToFlash(
			UPGRADE_PARAMETERS_ADDR,
			(uint32_t) buff,
			1024);
	__enable_irq();
	Delay(100, NULL);
	NVIC_SystemReset( );

}
/*******************************************************************/
static void Initialize_Timers()
{
	Set_Timer(&KEEP_ALIVE_TIMER, KEEP_ALIVE_TIMEOUT);
	Set_Timer(&SEND_OFFLINE_DATA_TIMER, OFFLINE_DATA_SEND_INTERVAL);
	Set_Timer(&SEND_PERIODIC_DATA_TIMER, gsm_get_data_send_period());
	Set_Timer(&STATUS_CHECK_TIMER, MODEM_STATUS_CHECK_PERIOD);
}
/*******************************************************************/
TIMER_TICK_T gsm_get_data_send_period()
{
	if(Get_IgnitionStatus() && (!gsm_info.b_roaming))
		return (settings_get_msg_period_ignited_not_roaming() *100);// - AT_RESPONSE_CHAR_TIMEOUT;

	else if(!Get_IgnitionStatus() && (!gsm_info.b_roaming)){
			if(gps_is_moving())
				return (settings_get_msg_period_ignited_not_roaming() *100);// - AT_RESPONSE_CHAR_TIMEOUT;
			else
				return (settings_get_msg_period_not_ignited_not_roaming() *100);// - AT_RESPONSE_CHAR_TIMEOUT;
	}

	else if(Get_IgnitionStatus() && (gsm_info.b_roaming))
		return (settings_get_msg_period_ignited_roaming() *100);// - AT_RESPONSE_CHAR_TIMEOUT;

	else if(!Get_IgnitionStatus() && (gsm_info.b_roaming)){
			if(gps_is_moving())
				return (settings_get_msg_period_ignited_roaming() *100);// - AT_RESPONSE_CHAR_TIMEOUT;
			else
				return (settings_get_msg_period_not_ignited_roaming() *100);// - AT_RESPONSE_CHAR_TIMEOUT;


	}

	return 100;
}
/*******************************************************************/
bool gsm_initialize_module()
{
	bool success = false;
	gsm_info.b_gsmHealtStatus = FALSE;
			/***/
			success = gsm_disable_echo(GSM_INIT_COMMAND_RETRY_COUNT);
			if(!success)
				return FALSE;

			success = gsm_get_imei(GSM_INIT_COMMAND_RETRY_COUNT);
			if(!success)
				return FALSE;

			success = gsm_get_pin_status(GSM_INIT_COMMAND_RETRY_COUNT);
			if(!success)
				return FALSE;

			success = gsm_get_imsi(GSM_INIT_COMMAND_RETRY_COUNT);
			if(!success)
				return FALSE;
			else
				gsm_info.b_gsmHealtStatus = TRUE;


		   success = gsm_select_audio_channel(AUDIO_CH1, DEFAULT_RETRIES);
		//	success = gsm_select_audio_channel(AUDIO_CH1, DEFAULT_RETRIES);
			if(!success)
				return FALSE;

			success = gsm_set_mic_gain(AUDIO_CH1, DEFAULT_RETRIES, 15);
			//success = gsm_set_mic_gain(AUDIO_CH1, DEFAULT_RETRIES, 15);
			if(!success)
				return FALSE;

			success = gsm_get_registration_status(GSM_REGISTRATION_TIMEOUT);
			if(!success)
				return FALSE;

			success = gsm_update_qloc_timeout();
			if(!success)
				return FALSE;
			/******/
	return success;
}
/*********************************************************************/
bool gsm_attach_gprs()
{
	FLASH_SETTINGS_T user_settings;
	TIMER_INFO_T gprs_attach_timer;

	settings_get(&user_settings);

	/* use DNS for remote server connection*/
	gsm_set_foreground_context(1, GSM_INIT_COMMAND_RETRY_COUNT);
	gsm_set_mux_mode(0, GSM_INIT_COMMAND_RETRY_COUNT);
	gsm_set_mode(0, GSM_INIT_COMMAND_RETRY_COUNT);
	gsm_set_ip_address_mode(isalpha(user_settings.server_ip[0]), GSM_INIT_COMMAND_RETRY_COUNT);

    gsm_set_recv_indicator_mode(1, GSM_INIT_COMMAND_RETRY_COUNT);

    Set_Timer(&gprs_attach_timer, GPRS_ATTACH_TIMEOUT);

    while(!mn_timer_expired(&gprs_attach_timer)){
    	PRINT_K("\nAttaching to GPRS...");
    	if(gsm_activate_pdp_context(user_settings.flash_apn ,
			           	   	   	   	user_settings.flash_apnusername,
									user_settings.flash_apnpassword,GPRS_ATTACH_TIMEOUT)){
    		PRINT_K("Done\n");
    		return TRUE;
    	}
    	gsm_check_modem_status(NUMBER_OF_STATUS_CHECK_RETRIES);
    	if(gsm_info.b_gsmRegistered == FALSE)
    	 return FALSE;
    }
	return FALSE;
}
/*********************************************************************/
bool ConnectToServer(uint8_t retryCount)
{
	uint8_t i =0;
	char respBuffer[64];

	while(i++ <= retryCount){
		gsm_connect_to_server(tcpConnectionInfo_t.tcpServerInfo_t.server_ip,
							   tcpConnectionInfo_t.tcpServerInfo_t.server_port);
		gsm_get_at_response(respBuffer, sizeof(respBuffer), SERVER_CONNECTION_TIMEOUT, NULL);
		if(strstr(respBuffer,RespConnectOK) != NULL || strstr(respBuffer,RespAlrdyConnect) != NULL )
			return TRUE;
		Delay(500, on_idle);
	 }
	 return FALSE;
}
/*********************************************************************/
int ConnectedToServer()
{
	BLOCKAGE_INFO_T blockage_info;
	COMMAND_RESULT_T result;
	COMMAND_RESPONSE_T commandResponse;
	int32_t i_msgLen = 0;
	int success = 0;


	Get_BlockageInfo(&blockage_info);
	if(blockage_info.blockageStatus== BLOCKAGE_ACTIVATED && blockage_info.cmdSource == COMMAND_SOURCE_TCP){
		i_msgLen = PrepareBlockageEchoPacket(messageBuffer);
		if(gsm_send_message(messageBuffer, i_msgLen, on_idle) == FALSE)
			return -1;
		Set_BlockageStatus(BLOCKAGE_NOT_EXIST);
	}
	if(get_t_msg_event_status() /*|| rf_check_node_alarm_exist()*/)
	{
		//PRINT_K("\n********************Event Detected*************************\n");
		i_msgLen = Trio_PrepareTMessage(messageBuffer, TRUE);
		if(gsm_send_message(messageBuffer, i_msgLen, on_idle) == FALSE)
				return -1;
		else{

			Set_Timer(&SEND_FAIL_TIMER, SEND_FAIL_TIMEOUT);
			Set_Timer(&KEEP_ALIVE_TIMER, KEEP_ALIVE_TIMEOUT);
			success = 1;
		}
	}
	/******/
	if(mn_timer_expired(&KEEP_ALIVE_TIMER)){
		PRINT_K("\nSend Keep-Alive\n");
		i_msgLen = Trio_PreparePingMessage(messageBuffer);
		if(gsm_send_message(messageBuffer, i_msgLen, on_idle) == FALSE)
			return -1;
		else{
			Set_Timer(&KEEP_ALIVE_TIMER, KEEP_ALIVE_TIMEOUT);
			Set_Timer(&SEND_FAIL_TIMER, SEND_FAIL_TIMEOUT);
			success = 1;
		}
	}
	/******/
	if(mn_timer_expired(&SEND_PERIODIC_DATA_TIMER)) {
	//	PRINT_K("\nSend periodic data\n");
		i_msgLen = Trio_PrepareTMessage(messageBuffer, TRUE);

		if(gsm_send_message(messageBuffer, i_msgLen, on_idle) == FALSE)
				return -1;
		else{
			Set_Timer(&SEND_PERIODIC_DATA_TIMER, gsm_get_data_send_period());
			Set_Timer(&KEEP_ALIVE_TIMER, KEEP_ALIVE_TIMEOUT);
			Set_Timer(&SEND_FAIL_TIMER, SEND_FAIL_TIMEOUT);
			success = 1;
		}
	}
	/*****/
	if(mn_timer_expired(&SEND_OFFLINE_DATA_TIMER)){
	//	 PRINT_K("\nChecking Offline data saved...\n");
		 LOG_ENTRY_INFO_T log_entry;
		 get_offline_log_item(&log_entry);
		 if(log_entry.b_validEntry){
			 if(InsertImeiToOfflineData(log_entry.logBuffer)){
				 if(gsm_send_message(log_entry.logBuffer, strlen(log_entry.logBuffer), on_idle) == FALSE)
						 return -1;
				 else{
				  Set_Timer(&KEEP_ALIVE_TIMER, KEEP_ALIVE_TIMEOUT);
				  Set_Timer(&SEND_FAIL_TIMER, SEND_FAIL_TIMEOUT);
				  success = 1;
			     }
		     }
			 else PRINT_K("\nCannot insert IMEI to Offline Data\n");
	     }
	  Set_Timer(&SEND_OFFLINE_DATA_TIMER, OFFLINE_DATA_SEND_INTERVAL);
	}

	/****/
	if(mn_timer_expired(&STATUS_CHECK_TIMER)){
	//	PRINT_K("\nPeriodic Check\n");
		if((gsm_check_modem_status(NUMBER_OF_STATUS_CHECK_RETRIES) == REPLY_CONNECT_TO_UPDATE_SERVER) || (requestConnectToUpdateServer == TRUE)){
			requestConnectToUpdateServer = FALSE;
			Delay(100, NULL);
			gsm_close_socket(SOCKET_CLOSE_RETRIES);
			PRINT_K("Returning 4\n");
		//	if(ConnectToServer(NUMBER_OF_CONNECT_RETRY) == TRUE)
				return 4;
		/*	else
				return -1;*/
		}
		if(Get_GsmTcpBuffer((uint8_t *)messageBuffer, MAX_T_MESSAGE_SIZE, on_idle) > 0){
			memset(&commandResponse, 0, sizeof(commandResponse));
			result = ProcessReceivedData(messageBuffer, &commandResponse, COMMAND_SOURCE_TCP);
			bool sendOk = TRUE;
			if(result == REPLY_ECHO_PACKET){
				PRINT_K(commandResponse.buffer);
				sendOk = gsm_send_message(commandResponse.buffer,
											   strlen(commandResponse.buffer),
												on_idle);
			}
			else if(result == REPLY_CONNECT_TO_UPDATE_SERVER){
				return 4;
			}
				/*Gsm_CloseSocket(SOCKET_CLOSE_RETRIES);
				if(ConnectToServer(NUMBER_OF_CONNECT_RETRY) == TRUE)
					return 4;*/

			if(sendOk == FALSE)
				return -1;
			else{
					Set_Timer(&SEND_FAIL_TIMER, SEND_FAIL_TIMEOUT);
					Set_Timer(&KEEP_ALIVE_TIMER, KEEP_ALIVE_TIMEOUT);
					if(commandResponse.b_needToReset){
					   PRINT_K("\nNeed To RESET\n");
					   NVIC_SystemReset();
					}
					success = 1;
				}
		}
		if((i_tempNonAckedBytes > MAX_NUMBER_OF_NONACKED_BYTES) ||
			(gsm_info.b_gsmRegistered == FALSE)                 ||
			(!gsm_info.b_socketActive)){
			PRINT_K("\nNON_ACK limit reached\n");
			return -1;
		}


			Set_Timer(&STATUS_CHECK_TIMER, MODEM_STATUS_CHECK_PERIOD);
		}
		return success;
}
/****************************************************************/
void Init_GsmTask()
{
	t_gsmState = MODULE_RESET_STATE;
	gsm_info.b_gsmRegistered = FALSE;
	gsm_info.b_socketActive = FALSE;
	gsm_info.b_gsmHealtStatus = FALSE;
	init_t_msg_event_que();
}
/****************************************************************/
bool gsm_softreset_module(uint8_t retryCount)
{
	uint8_t i = 0;
	char buffer[32];

    while(i++ <= retryCount){
    	gsm_send_at_command((char *)Command_RESET, "\nModule soft-reset\n");
    	gsm_get_at_response(buffer,sizeof(buffer), AT_RESPONSE_TIMEOUT*2, on_idle);
    	if(strstr(buffer, respOK) != NULL){
    		PRINT_K("Restarting...\n");
    		return TRUE;
    	}
    }
	return FALSE;
}
/****************************************************************/
bool gsm_disable_echo(uint8_t retryCount)
{
	uint8_t i = 0;
	char buffer[32];

	while(i++ <= retryCount){
		gsm_send_at_command((char *)Command_ATE0, "\nEcho disable\n");
    	gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
    	if(strstr(buffer, respOK) != NULL){
    		PRINT_K("Echo disabled\n");
    		return TRUE;
    	}
    }
    return FALSE;
}
/****************************************************************/
bool gsm_get_imei(uint8_t retryCount)
{
	uint8_t i = 0;
	char buffer[64];

	while(i++ <= retryCount){
		gsm_send_at_command((char *)Command_CGSN, "\nReading IMEI...\n");
		gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
    	if(strstr(buffer, respOK) != NULL){
    		memset(gsm_info.imei_no, 0, sizeof(gsm_info.imei_no));
    		if(ExtractIMEI(buffer,gsm_info.imei_no) == TRUE){
    			PRINT_K("\n\nIMEI: ");
    			PRINT_K(gsm_info.imei_no);
    			PRINT_K("\n\n");
    			return TRUE;
    		}

    	}
    }
    return FALSE;
}
/*************************************************************/
bool ExtractIMEI(char *buffer, char *imei)
{
	while(*buffer != '\0'){
		if(isdigit((int)*buffer)){
			memcpy(imei, buffer, IMEI_LENGTH);
			gsm_info.imei_no[IMEI_LENGTH] = '\0';
			return TRUE;
		}
		else
			buffer++;
	}
	return FALSE;
}
/****************************************************************/
bool gsm_get_imsi(uint8_t retryCount)
{
	uint8_t i = 0;
	char buffer[64];

	while(i++ <= retryCount){
		gsm_send_at_command((char *)Command_CIMI, "\nReading IMSI...");
		gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
    	if(strstr(buffer, respOK) != NULL){
    		PRINT_K("IMSI ready\n");
    		memset(gsm_info.imsi_no, 0, sizeof(gsm_info.imsi_no));
    		memcpy(gsm_info.imsi_no, &buffer[2] ,IMSI_LENGTH);
    		return TRUE;
    	}
    }
    return FALSE;
}
/****************************************************************/
bool gsm_get_pin_status(uint8_t retryCount)
{
	uint8_t i = 0;
	char buffer[64];

	while(i++ <= retryCount){
		gsm_send_at_command((char *)Command_CPIN, "\nReading PIN Status...");
		gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
		if(strstr(buffer, respReady) != NULL){
	    	PRINT_K("PIN READY\n");
	    	return TRUE;
	    }
	 }
	 PRINT_K("PIN ERROR\n");
	 return FALSE;
}
/*********************************************************************/
bool gsm_get_signal_power(char *buffer, uint8_t *gsmSigPower)
{
	char *p_temp;
    p_temp = strstr(buffer, RespCSQ);
	if(p_temp != NULL){
		*gsmSigPower =  (uint8_t)Get_ValueByParamIndex(p_temp, 1, AT_PARAM_INTEGER, NULL);
		return TRUE;
	}
	else
		return FALSE;
}
/*********************************************************************/
bool gsm_get_local_ip()
{
	uint16_t ipLen;
	char buffer[64];

	gsm_send_at_command((char *)Command_GetLocalIP, "");
	gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
	ipLen = strlen(buffer);
	remove_chr(buffer, "\r");
	remove_chr(buffer, "\n");
/*	for(i = 0; i< ipLen; i++){
		if(!isdigit((int)buffer[i]))

	}*/
	strcpy(gsm_info.local_ip, buffer);
	sprintf(buffer,"\nLocal IP Address: %s\n", gsm_info.local_ip);
	PRINT_K(buffer);
	return TRUE;

}
/*IP_ADDR_T ParseIPString(char *buffer, uint16_t bufLen)
{
	char temp[64];
	const char delim[] = ".";
	char *token;

	strcpy(temp, buffer, bufLen);

	token = strtok(temp, delim);
	while(*token != '\0')
		 token = strtok(NULL, s);
}*/
/****************************************************************/
bool gsm_select_audio_channel(GSM_AUDIO_CH channel, uint8_t retryCount)
{
	uint8_t i = 0;
    char buffer[64];
    char temp[16];

	while(i++ <= retryCount){
		memset(buffer, 0, sizeof(buffer));
		strcat(buffer, Command_QUADCH);
		sprintf(temp,"%c", channel);
		strcat(buffer, temp);
		strcat(buffer,"\r");
		gsm_send_at_command(buffer, "\nSelecting Audio Channel\n");
		gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
		if(strstr(buffer, respOK) != NULL){
		    sprintf(buffer, "\nAudio channel %c selected\n", channel + 1);
		    PRINT_K(buffer);
		    return TRUE;
		 }
	}
	return FALSE;
}
/******************************************************************/
bool gsm_set_mic_gain(GSM_AUDIO_CH channel, uint8_t retryCount, uint8_t gain)
{
	uint8_t i = 0;
    char buffer[64];
    char temp[16];

	while(i++ <= retryCount){
		memset(buffer, 0, sizeof(buffer));
		strcat(buffer, Command_QMIC);
		sprintf(temp,"%c,%d", channel,gain);
		strcat(buffer, temp);
		strcat(buffer,"\r");
		gsm_send_at_command(buffer, "\nSetting microphone gain\n");
		gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
		if(strstr(buffer, respOK) != NULL){
		    sprintf(buffer, "\nMicropone gain level set to %d\n", gain);
		    PRINT_K(buffer);
		    return TRUE;
		 }
	}
	return FALSE;
}
/****************************************************************/
bool gsm_get_registration_status(TIMER_TICK_T regTimeout)
{
	TIMER_INFO_T gsm_reg_timer;
	uint8_t gsmRegStatus;
	char buffer[64];

	Set_Timer(&gsm_reg_timer, regTimeout);

	while(!mn_timer_expired(&gsm_reg_timer)){
		gsm_send_at_command((char *)Command_CREG, "Waiting GSM registration...\n");
		gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
		gsm_parse_registration_status(buffer, &gsmRegStatus);
		gsm_update_registration_status(gsmRegStatus);
		if(gsm_info.b_gsmRegistered){
			PRINT_K("Registered\n");
			return TRUE;
		}
	}
	return FALSE;
}
/***************************************************************/
bool gsm_parse_registration_status(char *buffer, uint8_t *gsmRegStatus)
{
	char *p_temp;

	/* check registration status*/
	p_temp = strstr(buffer, cregResp);
	if(p_temp != NULL){
		*gsmRegStatus =  (uint8_t)Get_ValueByParamIndex(p_temp, 2, AT_PARAM_INTEGER, NULL);
		return TRUE;
	}
	else
		return FALSE;
}
/********************************************************************/
bool gsm_get_battery_voltage(char *buffer, uint16_t *batVoltage)
{
	char *p_temp;
    p_temp = strstr(buffer, RespCBC);
    if(p_temp != NULL){
    	*batVoltage =  Get_ValueByParamIndex(p_temp, 3, AT_PARAM_INTEGER, NULL);
    	return TRUE;
    }
    else
    	return FALSE;
}
/********************************************************************/
COMMAND_RESULT_T gsm_read_sms(char *buffer, COMMAND_RESPONSE_T *commandResponse, char *smsOrigAddrBuf)
{
	char *p_temp;
	char *p_data;

	//memset(&commandResponse, 0, sizeof(commandResponse));
	/* check SMS*/
	Extract_SmsOrigAddr(buffer, smsOrigAddrBuf);

	memset(commandResponse, 0, sizeof(COMMAND_RESPONSE_T));
	p_temp = strstr(buffer, RespCMGR);
	if(p_temp != NULL){
		PRINT_K("\nNew SMS received\n");
		p_data = strstr(p_temp, TRIO_CONFIG_WORD);
		PRINT_K(p_data);
		if(p_data != NULL){
			p_temp = strchr(p_data, '\r');
			if(p_temp != NULL){
				*p_temp = '\0';
				return ProcessReceivedData(p_data, commandResponse, COMMAND_SOURCE_SMS);
			}
			else return -1;   /* error parsing sms  */
		}
	}
	return 0;  /* no sms received */
}
/********************************************************************/
bool Extract_SmsOrigAddr(char *buffer, char *smsOrigAddrBuf)
{
	char *startPtr, *endPtr;
	char smsInfo[64];

	/* copy first 64 bytes of SMS header for processing */
	memcpy(smsInfo, buffer, sizeof(smsInfo));

	startPtr = strstr(smsInfo, "\"+") + 1;
	endPtr = strchr(startPtr,'"');

	if(startPtr !=  NULL && endPtr != NULL){
		*endPtr = '\0';
		strcpy(smsOrigAddrBuf, startPtr);
		return TRUE;
	}
	return FALSE;
}
/***************************************************************/
void gsm_update_registration_status(uint8_t i_regStatus)
{
	GSM_TASK_EVENT_T gsm_event;

	switch(i_regStatus){
		case 1:
			gsm_event.sig = SIGNAL_GSM_TASK_REGISTERED;
			strcpy(gsm_event.param.param1, "");
			strcpy(gsm_event.param.param2, "");
			PutEvent_GsmTask(&gsm_event);
			gsm_info.b_gsmRegistered = TRUE;
			break;

		case 5:
			gsm_event.sig = SIGNAL_GSM_TASK_REGISTERED;
			strcpy(gsm_event.param.param1, "");
			strcpy(gsm_event.param.param2, "");
			PutEvent_GsmTask(&gsm_event);
			gsm_info.b_gsmRegistered = TRUE;
			gsm_info.b_roaming = TRUE;
			break;

		default:
			gsm_event.sig = SIGNAL_GSM_TASK_DEREGISTERED;
			strcpy(gsm_event.param.param1, "");
			strcpy(gsm_event.param.param2, "");
			PutEvent_GsmTask(&gsm_event);
			gsm_info.b_gsmRegistered = FALSE;
			gsm_info.b_roaming = FALSE;
			break;
	}

}
/***************************************************************/
bool gsm_get_waiting_call_list(char *buffer, GSM_CALL_T *gsm_call)
{
	char *p_temp1, *p_temp2;
	char printBuf[512];
	/* get list of current calls*/
	p_temp1 = buffer;
	while(1){
		p_temp2 = strstr(p_temp1, respCLCC);
		if(p_temp2 != NULL){

			gsm_call->idx  = Get_ValueByParamIndex(p_temp2, 1, AT_PARAM_INTEGER, NULL);
			gsm_call->dir  = Get_ValueByParamIndex(p_temp2, 2, AT_PARAM_INTEGER, NULL);
			gsm_call->stat = Get_ValueByParamIndex(p_temp2, 3, AT_PARAM_INTEGER, NULL);
			gsm_call->mode = Get_ValueByParamIndex(p_temp2, 4, AT_PARAM_INTEGER, NULL);
			gsm_call->mpty = Get_ValueByParamIndex(p_temp2, 5, AT_PARAM_INTEGER, NULL);
			gsm_call->type = Get_ValueByParamIndex(p_temp2, 7, AT_PARAM_INTEGER, NULL);
			Get_ValueByParamIndex(p_temp2, 6, AT_PARAM_STRING, gsm_call->number);

			sprintf(printBuf,"ID: %d\nDirection: %d\nCall Status: %d\nMode: %d\nConference: %d\nType: %d\nNumber: %s\n",
					gsm_call->idx,
					gsm_call->dir,
					gsm_call->stat,
					gsm_call->mode,
					gsm_call->mpty,
					gsm_call->type,
					gsm_call->number);
			PRINT_K(printBuf);
			p_temp1 = strstr(p_temp2, "\n");
			return TRUE;
		}
		else
			return FALSE;

	/*	p_temp = strstr(buffer, ENTER);
		p_temp += sizeof(ENTER) - 1;
		numOfCalls++;*/
	}
}
/***************************************************************/
bool gsm_set_ip_address_mode(bool b_alpha, uint8_t retryCount)
{
	char cmdBuffer[32];
	char buffer[64];
	uint8_t i = 0;

	while(i++ <= retryCount){
		memset(cmdBuffer,0, sizeof(cmdBuffer));
		strcat(cmdBuffer,Command_SetIpAddrMode);
		if(b_alpha)
			strcat(cmdBuffer,"1\r");
		else
			strcat(cmdBuffer,"0\r");
		gsm_send_at_command(cmdBuffer,"");
		gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
		if(strstr(buffer, respOK) != NULL){
		//	PRINT_K("\nIP Address mode set\n");
			return TRUE;
		}
	}
	return FALSE;
}
/********************************************************************************/
bool gsm_set_foreground_context(uint8_t context_num, uint8_t retryCount)
{
	char cmdBuffer[32];
	char temp[4];
	char buffer[64];
	uint8_t i = 0;

	while(i++ <= retryCount){
		sprintf(temp,"%d", context_num);
		memset(cmdBuffer,0, sizeof(cmdBuffer));
		strcat(cmdBuffer,Command_SetFgndContext);
		strcat(cmdBuffer,temp);
		strcat(cmdBuffer, "\r");
		gsm_send_at_command(cmdBuffer,"");
		gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
		if(strstr(buffer, respOK) != NULL){
		//	PRINT_K("\nForeground Context Set\n");
			return TRUE;
		}
	}
	return FALSE;
}
/*******************************************************************************/
bool gsm_set_mux_mode(uint8_t mux_mode, uint8_t retryCount)
{
	char cmdBuffer[32];
//	char temp[4];
	char buffer[64];
	uint8_t i = 0;
	while(i++ <= retryCount){
			strcat(cmdBuffer,Command_SetMuxMode);
			gsm_send_at_command(cmdBuffer,"");
			gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
			if(strstr(buffer, respOK) != NULL){
				return TRUE;
			}
	}
	return FALSE;
}
/******************************************************************************/
bool gsm_set_mode(uint8_t mux_mode, uint8_t retryCount)
{
	char cmdBuffer[32];
	//char temp[4];
	char buffer[64];
	uint8_t i = 0;
	while(i++ <= retryCount){
		strcat(cmdBuffer,Command_SetMode);
		gsm_send_at_command(cmdBuffer,"");
		gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
		if(strstr(buffer, respOK) != NULL){
		  return TRUE;
		}
	}
	return FALSE;
}
/********************************************************************************/
bool gsm_set_recv_indicator_mode(uint8_t indi_mode, uint8_t retryCount)
{
	int i =0;
	char cmdBuffer[32];
	char buffer[32];
	char temp[4];

	while(i++ <= retryCount){
		sprintf(temp,"%d", indi_mode);
		memset(cmdBuffer,0, sizeof(cmdBuffer));
		strcat(cmdBuffer,Command_SetRecvIndiMode);
		strcat(cmdBuffer,temp);
		strcat(cmdBuffer, "\r");
		gsm_send_at_command(cmdBuffer,"");
		gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
		if(strstr(buffer, respOK) != NULL){
			return TRUE;
		}
	}
	return FALSE;
}
/*************************************************************************/
void gsm_connect_to_server(char *p_serverIp, char *p_tcpPort)
{
	char cmdBuffer[256];
	char temp[128];

	memset(cmdBuffer,0, sizeof(cmdBuffer));

	strcat(cmdBuffer,Command_TCPConnect);
	strcat(cmdBuffer, p_serverIp);
	strcat(cmdBuffer,"\",");
	strcat(cmdBuffer, p_tcpPort);
	strcat(cmdBuffer,"\r");
//	PRINT_K(cmdBuffer);
	sprintf(temp,"\nConnecting to %s : %s\n", p_serverIp, p_tcpPort);
	PRINT_K(temp);

	gsm_send_at_command(cmdBuffer, "");
}
/************************************************************************/
bool gsm_send_message(char *pdata, uint16_t i_msgLen, void (*onIdleCallback)())
{
	char responseBuf[128];
	bool success;

	if(i_msgLen > QUECTEL_MAX_TCP_SEND_SIZE)
		return FALSE;
	PRINT_K("\nSending Data...\n");
	success = gsm_start_sending_message(i_msgLen, DATA_SEND_RETRY_COUNT);
	if(success){
		gsm_send_at_command(pdata,"");
		PRINT_K(pdata);
		gsm_get_at_response(responseBuf, sizeof(responseBuf), 200, onIdleCallback);
		if(strstr(responseBuf, respSendOK) != NULL){
			PRINT_K("\nDone\n");
			return TRUE;
		}
		else{
			PRINT_K("\nFailed!!!\n");
			return FALSE;
		}
	}
	PRINT_K("\nFailed!!!\n");
	return FALSE;
}
/******************************************************************************/
bool gsm_start_sending_message(uint16_t i_msgLen, uint8_t retryCount)
{
	char temp[8];
	char responseBuf[128];
	char cmdBuffer[32];
	char esc[2];

	if(i_msgLen > QUECTEL_MAX_TCP_SEND_SIZE)
		return FALSE;

	memset(esc,0,sizeof(esc));
	esc[0] = CHAR_ESC;
	gsm_send_at_command(esc, "");              /* cancel pending data */

	memset(cmdBuffer,0, sizeof(cmdBuffer));
	sprintf(temp,"%d", i_msgLen);
	strcat(cmdBuffer,Command_SendData);
	strcat(cmdBuffer,temp);
	strcat(cmdBuffer, "\r");
	gsm_send_at_command(cmdBuffer,"");
	gsm_get_at_response(responseBuf, sizeof(responseBuf), 500, on_idle);
	if(strchr(responseBuf, DATA_SEND_PROMPT) != NULL)
		return TRUE;

	return FALSE;
}
/************************************************************/
bool gsm_close_socket(uint8_t retryCount)
{
	uint8_t i =0;
	char responseBuf[128];

	while(i++ < retryCount){
		gsm_send_at_command((char *)Command_CloseSocket, "\nClosing socket...");
		gsm_get_at_response(responseBuf, sizeof(responseBuf), 500, NULL);
		if(strstr(responseBuf, respOK) != NULL){
			PRINT_K("Done\n");
			return TRUE;
		}
	}
	return FALSE;
}
/*************************************************************************
 * Returns an AT response parameter specified by param_no                *
 *************************************************************************/
int16_t Get_ValueByParamIndex(char *buffer, int param_no, AT_PARAM_TYPE param_type, char *out_buffer)
{
	char *p_temp;
	char *p_start;
	int i_commaCount =0;
	char tempBuffer[512];

	memset(tempBuffer, 0, sizeof(tempBuffer));
	memcpy(tempBuffer, buffer, strlen(buffer));

	p_start = strchr(tempBuffer,' ');
	if(p_start == NULL)
		return 0;
	p_temp = strtok(p_start,"\r,");
	while (p_temp != NULL){
		if(++i_commaCount == param_no){
			if(param_type == AT_PARAM_INTEGER)
				return atoi(p_temp);
			else if(param_type == AT_PARAM_STRING){
				strcpy(out_buffer, remove_chr(p_temp, "\""));
				return  strlen(out_buffer);
			}
		}
		p_temp = strtok (NULL, "\r,");
	}
	return 0;
}
/***********************************************************************/
bool gsm_activate_pdp_context(char *apn, char *username, char *password, TIMER_TICK_T attach_timeout)
{
	char cmdBuffer[256];
	char buffer[32];
	memset(cmdBuffer,0, sizeof(cmdBuffer));
	strcat(cmdBuffer,Command_ActivatePDP);
	strcat(cmdBuffer, apn);
	strcat(cmdBuffer,"\",\"");
	strcat(cmdBuffer, username);
	strcat(cmdBuffer,"\",\"");
	strcat(cmdBuffer, password);
	strcat(cmdBuffer,"\"\r");
	gsm_send_at_command(cmdBuffer, "");
	gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
	if(strstr(buffer, respOK) != NULL)
		return TRUE;
	else
		return FALSE;
}
/***********************************************************************/
COMMAND_RESULT_T gsm_check_modem_status(uint8_t retryCount)
{
	COMMAND_RESULT_T result;
	COMMAND_RESPONSE_T commandResponse;
	GSM_CALL_T gsm_call;
	uint8_t gsmRegStatus;
	char smsOrigAddrBuf[32];
	char tempBuf[256];
	char buffer[512];

		memset(buffer, 0, sizeof(buffer));

		//PRINT_K("\n*********Periodic Check**********\n");
		gsm_send_at_command((char *)Command_GetStatus,"");
		gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);

		if(gsm_get_waiting_call_list(buffer, &gsm_call) == TRUE)
			gsm_answer_incoming_call(&gsm_call, NUMBER_OF_ANSWER_CALL_RETRIES);
		gsm_parse_registration_status(buffer, &gsmRegStatus);
		gsm_update_registration_status(gsmRegStatus);
		gsm_get_battery_voltage(buffer, &gsm_info.batteryLevel);
		gsm_get_signal_power(buffer, &gsm_info.csq);

		sprintf(tempBuf, "\nBattery Voltage: %d V\nSignal Power: %d\nGSM Registration: %d\n",
						   gsm_info.batteryLevel, gsm_info.csq, gsm_info.b_gsmRegistered);
		PRINT_K(tempBuf);

		result = gsm_read_sms(buffer, &commandResponse, smsOrigAddrBuf);

		if(result == REPLY_ECHO_PACKET){
			PRINT_K(commandResponse.buffer);
			gsm_send_sms(smsOrigAddrBuf, commandResponse.buffer, on_idle);
			if(commandResponse.b_needToReset)
				NVIC_SystemReset();
		}
		gsm_get_socket_stats(buffer);

		sprintf(tempBuf, "Bytes Sent: %d\nNon-acked bytes: %d\n",
				gsm_info.gsm_stats.u16_totalBytesSent, gsm_info.gsm_stats.u16_totalBytesNonAcked1);
		PRINT_K(tempBuf);

		gsm_get_socket_status(buffer);
	/*	sprintf(tempBuf, "\nSocket Status: %s\n", gsm_info.b_socketActive ? "CONNECTED" : "NOT-CONNECTED");
		PRINT_K(tempBuf);
		PRINT_K("\n*********************************\n");*/
		return result;
}
/*********************************************************************************/
bool gsm_send_sms(char *destAddr, char *smsText, void (*onIdleCallback)())
{
	TIMER_INFO_T sms_sent_timer;
	char cmdBuffer[200];
	const char ctrlStr[2] = {CTRL_Z, '\0'};

	memset(cmdBuffer,0, sizeof(cmdBuffer));
	strcat(cmdBuffer,Command_CMGS);
	strcat(cmdBuffer,destAddr);
/*
* Quectel module requires command in AT+CMGS="xxxxxxxxxx"\r format
* Telit module requires command   in AT+CMGS=xxxxxxxxxx\r   format
*/
	strcat(cmdBuffer,"\"\r");

	gsm_send_at_command(cmdBuffer,"\nSending SMS...");
	memset(cmdBuffer,0, sizeof(cmdBuffer));
	gsm_get_at_response(cmdBuffer, sizeof(cmdBuffer), 200, onIdleCallback);

	if(strchr(cmdBuffer, DATA_SEND_PROMPT) != NULL){
		memset(cmdBuffer,0, sizeof(cmdBuffer));
		strcat(cmdBuffer,smsText);
		strcat(cmdBuffer,ctrlStr);
		gsm_send_at_command(cmdBuffer,"");

		Set_Timer(&sms_sent_timer, SMS_SENT_RESPONSE_TIMEOUT);
		while(!mn_timer_expired(&sms_sent_timer)){
			memset(cmdBuffer,0, sizeof(cmdBuffer));
			gsm_get_at_response(cmdBuffer, sizeof(cmdBuffer), 200, onIdleCallback);
			if(strstr(cmdBuffer, respOK) != NULL){
				PRINT_K("Done\n");
				return TRUE;
			}
		}
		PRINT_K("Failed!!!\n");
	}
	return FALSE;
}
/********************************************************************/
bool gsm_get_socket_status(char *buffer)
{
	char *p_temp;

	p_temp = strstr(buffer, RespConnectOK);   /* get socket status*/
	if(p_temp != NULL){
		GSM_TASK_EVENT_T  gsm_event;
	//	OFFLINE_TASK_EVENT_T offlineEvt;
		gsm_event.sig = SIGNAL_GSM_CONNECTED_SERVER;
	//	offlineEvt.sig = SIGNAL_OFFLINE_CONNECTED_SERVER;
		strcpy(gsm_event.param.param1, "");
		strcpy(gsm_event.param.param2, "");
		PutEvent_GsmTask(&gsm_event);
	//	PutEvent_OfflineTask(&offlineEvt);
		gsm_info.b_socketActive = TRUE;
	}

	else{
		GSM_TASK_EVENT_T  gsm_event;
	//	OFFLINE_TASK_EVENT_T offlineEvt;
		gsm_event.sig = SIGNAL_GSM_DISCONNECTED_SERVER;
	//	offlineEvt.sig = SIGNAL_OFFLINE_DISCONNECTED_SERVER;
		strcpy(gsm_event.param.param1, "");
		strcpy(gsm_event.param.param2, "");
		PutEvent_GsmTask(&gsm_event);
	//	PutEvent_OfflineTask(&offlineEvt);
		gsm_info.b_socketActive = FALSE;
	}


	return TRUE;

}
/********************************************************************/
bool gsm_get_socket_stats(char *buffer)
{
	char *p_temp;
	//char printBuf[100];

	p_temp = strstr(buffer, RespQISACK);

	if(p_temp != NULL){
		gsm_info.gsm_stats.u16_totalBytesSent      = Get_ValueByParamIndex(p_temp, 1, AT_PARAM_INTEGER, NULL);
		gsm_info.gsm_stats.u16_totalBytesNonAcked1 = Get_ValueByParamIndex(p_temp, 3, AT_PARAM_INTEGER, NULL);
	}

	if(gsm_info.gsm_stats.u16_totalBytesNonAcked1 < gsm_info.gsm_stats.u16_totalBytesNonAcked2)
			i_tempNonAckedBytes = 0;
	else
			i_tempNonAckedBytes += gsm_info.gsm_stats.u16_totalBytesNonAcked1 - gsm_info.gsm_stats.u16_totalBytesNonAcked2;

	gsm_info.gsm_stats.u16_totalBytesNonAcked2 = gsm_info.gsm_stats.u16_totalBytesNonAcked1;
/*	sprintf(printBuf,"i_tempNonAckedBytes: %d", i_tempNonAckedBytes);
	PRINT_K(printBuf);*/
	return TRUE;

}
/***********************************************************************
 * Answers an incoming call                                            *
 **********************************************************************/
bool gsm_answer_incoming_call(GSM_CALL_T *gsm_call, uint8_t retryCount)
{
	uint8_t i = 0;
	char buffer[64];

	if(gsm_call->stat == CALL_STATUS_INCOMING &&
		 gsm_call->dir == MT_CALL &&
		 gsm_call->mode == CALL_MODE_VOICE){
		PRINT_K("\nIncoming call...\n");
		while(i++ <= retryCount){
			gsm_send_at_command((char *)Command_ATA, "\nAnswering call\n");
			gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
			if(strstr(buffer, respOK) != NULL){
				PRINT_K("\n*****Call established*****\n");
				return TRUE;
			}
		}
	}
	return FALSE;
}

/****************************************************************/
void gsm_send_at_command(char *command, char *traceMessage)
{
	PRINT_K(traceMessage);
	//PRINT_K(command);
	Chip_UART_SendRB(GSM_PORT, &gsm_txring, command, strlen(command));
	RingBuffer_Init(&gsm_rxring, gsm_rxbuff, 1, GSM_UART_RRB_SIZE);
}
/****************************************************************/
/*         Returns null terminated AT command response          */
/****************************************************************/
uint16_t gsm_get_at_response(char *buffer, uint16_t length, uint16_t timeout, void (*callbackFunc)())
{
	TIMER_INFO_T at_response_timer;
	TIMER_INFO_T charTimer;
	int readBytes = 0;
	char data;

	memset(buffer,0,length);

    Set_Timer(&at_response_timer, timeout);

    while(!mn_timer_expired(&at_response_timer)){
    	if(Chip_UART_ReadRB(GSM_PORT, &gsm_rxring, &data, 1) > 0){
    		Set_Timer(&charTimer, AT_RESPONSE_CHAR_TIMEOUT);
    		buffer[readBytes++] = data;
    		if(readBytes == length)
    			return readBytes;
    	 }
    	else{
    		if((mn_timer_expired(&charTimer)) && ( readBytes > 0)){
    			PRINT_K(buffer);
    			return readBytes;
    		}
    	}
    	if(callbackFunc != NULL)
    		callbackFunc();
    }
    return 0;
}
/*****************************************************************************/
uint16_t Get_GsmTcpBuffer(uint8_t *buffer, uint16_t length, void (*callBackFunc)())
{
	uint16_t dataoffset = 0;
	uint16_t u16_segmentSize;
	char segmentBuffer[256];

	memset(buffer, 0, length);

	if(length < MAX_TCP_RECEIVE_SIZE){
		u16_segmentSize = Get_TcpDataSegment((char *)buffer, length, callBackFunc);
		return u16_segmentSize;
	}
	else{
		do{
			u16_segmentSize = Get_TcpDataSegment(segmentBuffer, MAX_TCP_RECEIVE_SIZE, callBackFunc);
			memcpy(&buffer[dataoffset], segmentBuffer, u16_segmentSize);
			dataoffset += u16_segmentSize;
		}while(u16_segmentSize > 0);

	return dataoffset;
	}
}
/*****************************************************************************/
#define MAX_TCP_READ_BLOCK_SIZE     256
#define MAX_SRECV_STRING_SIZE       44

uint16_t Get_TcpDataSegment(char *buffer, uint16_t recvLength, void (*callback)())
{
	uint16_t length;
	char cmdBuffer[64];
	char tempBuffer[300];
	char temp[8];
	char *ptemp1;
	char *pnewline;

	memset(tempBuffer, 0, sizeof(tempBuffer));
	memset(cmdBuffer, 0, sizeof(cmdBuffer));
	memset(buffer, 0, recvLength);

	sprintf(temp,"%d",recvLength);
	strcat((char *)cmdBuffer, Command_RecvData);
	strcat((char *)cmdBuffer, temp);
	strcat((char *)cmdBuffer,"\r");

	gsm_send_at_command(cmdBuffer, "");
	length = gsm_get_at_response(tempBuffer, sizeof(tempBuffer), 200, callback);

	/* find the length of the message between  ",TCP," and "\r\n"  */
	ptemp1 = strstr(tempBuffer, QUECTEL_TCP_HDR);
	pnewline = strstr(ptemp1, ENTER);

	if((ptemp1 != NULL) && (pnewline != NULL)) {
		ptemp1 = ptemp1 + sizeof(QUECTEL_TCP_HDR);
		*pnewline = '\0';
		length = atoi(ptemp1);
		memcpy(buffer, (pnewline + 2), length);

		PRINT_K(buffer);
		return length;
	}
	else
		return (0);

}

/******************************************************************/
void Get_GsmInfo(GSM_INFO_T * gsm_info_buffer)
{
	memcpy(gsm_info_buffer, &gsm_info, sizeof(GSM_INFO_T));
}
/*******************************************************************/
int16_t GetBatteryVoltage()
{
	return gsm_info.batteryLevel;
}
/*******************************************************************/
bool gsm_get_server_connection_status()
{
	return gsm_info.b_socketActive;
}
/*******************************************************************/
void gsm_set_connection_parameters(char *server_ip, char *server_port)
{
	strcpy(tcpConnectionInfo_t.tcpServerInfo_t.server_ip, server_ip);
	strcpy(tcpConnectionInfo_t.tcpServerInfo_t.server_port, server_port);
}
/*********************************************************************/
void gsm_refresh_message_timer()
{
//	char printBuf[128];
	Set_Timer(&SEND_PERIODIC_DATA_TIMER, gsm_get_data_send_period());
	/*sprintf(printBuf,"\nSetting message periog to %d secs\n",gsm_get_data_send_period());
	PRINT_K(printBuf);*/
}
/*******************************************************************/
bool InsertImeiToOfflineData(char *buffer)
{
	char tempBuffer[MAX_T_MESSAGE_SIZE];
	char *p_msgHeader, *p_msgFooter;

	memset(tempBuffer, 0, MAX_T_MESSAGE_SIZE);

	/* check if we have a valid T message header */
	p_msgHeader = strstr(buffer, T_MSG_START_STR);
	if(p_msgHeader == NULL){
		PRINT_K(buffer);
		PRINT_K("\nIMEI ERROR 1\n");
		return FALSE;
	}


	/* check if we have a valid T message footer */
	p_msgFooter = strstr(buffer, T_MSG_END_STR);
	if(p_msgFooter == NULL){
		PRINT_K("\nIMEI ERROR 2\n");
		return FALSE;
	}


	/* check if we have a valid IMEI */
	if(strlen(gsm_info.imei_no) != IMEI_LENGTH){
		PRINT_K("\nIMEI ERROR 3\n");
		return FALSE;
	}


	/* start copying message to buffer*/
	strcat(tempBuffer, T_MSG_START_STR);
	*p_msgFooter ='\0';

	strcat(tempBuffer, gsm_info.imei_no);
	strcat(tempBuffer, p_msgHeader + sizeof(T_MSG_START_STR) - 1);

	Trio_AddTMessageExtensionSeperator(tempBuffer);
	Trio_PrepareTMessageExtension(tempBuffer, OFFLINE_DATA);
	Trio_EndTMessage(tempBuffer);

	strcpy(buffer, tempBuffer);

	return TRUE;
}
/**
 *
 */
void on_idle()
{

	task_timer();
/*	task_buzzer();
	task_lora();*/
	task_ignition();
	task_din();
	task_can();
	task_gps();
	task_status();
//	task_rf();
//	task_temp_sensor();
//	task_offline();
	//task_offlineRev2();
    task_trace();
    task_dout();
//    task_tablet_app();
  //  task_scale();
	Chip_WWDT_Feed(LPC_WWDT);
}
/**
 *
 */
bool gsm_get_healt_status()
{
	return gsm_info.b_gsmHealtStatus;
}
/**
 *
 */
bool gsm_dial(char *gsm_no,  void (*callback)())
{
	TIMER_INFO_T dial_timer;
	char temp[64];



	sprintf(temp, "\nDialing %s...", gsm_no);
	PRINT_K(temp);

	memset(temp, 0, sizeof(temp));

	strcat(temp, Command_Dial);
	strcat(temp, gsm_no);
	strcat(temp, ";");
	strcat(temp,"\r");
	PRINT_K(temp);

	gsm_send_at_command(temp,"");

	Set_Timer(&dial_timer, DIAL_TIMEOUT);
	while(!mn_timer_expired(&dial_timer)){
		memset(temp,0, sizeof(temp));
		gsm_get_at_response(temp, sizeof(temp), DIAL_TIMEOUT, callback);
		if(strstr(temp, respOK) != NULL){
			PRINT_K("Done\n");
			return TRUE;
		}
	}

	PRINT_K("\nDialing failed!!!\n");
	return FALSE;
}
/**
 * Request LBS information from GSM modem
 */
bool gsm_query_lbs_info(LBS_INFO_T *lbs_info)
{
	char buffer[64];

	gsm_send_at_command((char *)Command_LBS, "");
	gsm_get_at_response(buffer, sizeof(buffer), LBS_SERVER_RESPONSE_TIMEOUT + 200, on_idle);

	if(strstr(buffer, RespLBS) != NULL){
		PRINT_K("\nLBS Response OK");
		if(gsm_parse_lbs_response(lbs_info, buffer))
			return TRUE;
		else
			return FALSE;
	}

	return FALSE;
}
/**
 *
 */
static bool gsm_parse_lbs_response(LBS_INFO_T *lbs_info_t, char *response)
{
	char temp[128];
	char *token;

	memcpy(temp, response, sizeof(temp));

	token = strchr(temp, ' ');
	if(token == NULL)
		return false;
	token++;
	token = strtok(token, ",");    /* lat*/
	if(token == NULL)
		return false;
	strcpy(lbs_info_t->lat, token);

	token = strtok(NULL, "\r");     /* lon*/
	if(token == NULL)
		return false;
	strcpy(lbs_info_t->lon, token);
	return true;
}
/**
 * Updates QuecLocater HTTP response timeout
 */
static bool gsm_update_qloc_timeout()
{
	TIMER_INFO_T at_response_timer;
	char buffer[64];

	Set_Timer(&at_response_timer, SECOND);
	gsm_send_at_command((char *)Command_Update_QLOC_Tout, "");
	gsm_get_at_response(buffer, sizeof(buffer), AT_RESPONSE_TIMEOUT, on_idle);
	if(strstr(buffer, "OK") != NULL)
		return TRUE;
	else
		return FALSE;
}
