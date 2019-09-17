
#include "chip.h"
#include "board.h"
#include "bsp.h"
#include "sst25.h"
#include "timer.h"
#include "event.h"
#include "messages.h"
#include "MMA8652.h"
#include "gps.h"
#include "gSensor.h"
#include "offline.h"
#include "settings.h"
#include "gsm.h"
#include "trace.h"
#include "status.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>



#define LOG_BLOCK_READ_TIMEOUT      5

typedef struct
{
	uint32_t u32_offLineDataWriteAddress;
	uint32_t u32_offLineDataReadAddress;
	uint32_t u32_offLineDataReadAddressPersisted;
	uint16_t bufferedBytes;
	char  buffer[OFFLINE_DATA_CACHE_SIZE];
}OFFLINE_DATA_T;



static OFFLINE_DATA_T offLineData  __attribute__ ((section (".big_buffers")));

static LOGTASK_STATE  offlineTaskState = OFFLINE_STATE;
TIMER_INFO_T OFFLINE_DATA_READ_ADDRESS_UPDATE_TIMER;
TIMER_INFO_T OFFLINE_DATA_RECORD_TIMER;
TIMER_INFO_T OFFLINE_DATA_FLUSH_TIMER;
TIMER_TICK_T offline_get_log_period();
void offline_append_data_to_cache(char *p_msgPtr, uint16_t i_msgLen);
void offline_update_log_read_address(uint32_t address);
void offline_update_log_write_address(uint32_t address);
static uint16_t offline_get_end_write_sector();
static uint16_t offline_get_end_write_sector();
static uint16_t offline_get_current_read_sector();
static uint16_t offline_get_current_write_address();
static void offline_flush_data();
static uint16_t offline_get_read_block_size();
static TIMER_TICK_T  offline_get_message_period();

/***************************************************************/
void offline_data_init()
{
	uint32_t u32_initialValues = 0;
	char printBuf[128];
	/*int i;
	uint32_t value;*/
	if(!sst25_is_initialized()) {
		PRINT_OFFLINE("\nError: External Flash Not Initialized!!!\n");
		while(1);
	}
	memset(&offLineData, 0, sizeof(offLineData));

	offLineData.u32_offLineDataWriteAddress = get_last_valid_value_in_sector(OFFLINE_DATA_WRITE_ADDRESS);

//	sst25_read_array(OFFLINE_DATA_WRITE_ADDRESS,sizeof(offLineData.u32_offLineDataWriteAddress),(char *)&offLineData.u32_offLineDataWriteAddress);

	if(!VALIDATE_OFFLINE_LOG_ADDRESS(offLineData.u32_offLineDataWriteAddress)){
		PRINT_OFFLINE("\nInvalid offline data write Address. Writing initial Value 0x00000000\n");
		sst25_disable_write_lock();
		sst25_write_array((char *)&u32_initialValues, sizeof(u32_initialValues), OFFLINE_DATA_WRITE_ADDRESS);
		sst25_enable_write_lock();
		sst25_read_array(OFFLINE_DATA_WRITE_ADDRESS, sizeof(offLineData.u32_offLineDataWriteAddress),(char *)&offLineData.u32_offLineDataWriteAddress);
	}
	sprintf(printBuf, "\nOff-line Data Write Address 0x%.8X\n",(unsigned int)offLineData.u32_offLineDataWriteAddress);
	PRINT_OFFLINE(printBuf);

	offLineData.u32_offLineDataReadAddress = get_last_valid_value_in_sector(OFFLINE_DATA_READ_ADDRESS);
//	sst25_read_array(OFFLINE_DATA_READ_ADDRESS,sizeof(offLineData.u32_offLineDataReadAddress),(char *)&(offLineData.u32_offLineDataReadAddress));

	if(!VALIDATE_OFFLINE_LOG_ADDRESS(offLineData.u32_offLineDataReadAddress)){
		PRINT_OFFLINE("\nInvalid offline data read Address. Writing initial Value 0x00000000\n");
		sst25_disable_write_lock();
		sst25_write_array((uint8_t *)&u32_initialValues, sizeof(u32_initialValues), OFFLINE_DATA_READ_ADDRESS);
		sst25_enable_write_lock();
		sst25_read_array(OFFLINE_DATA_READ_ADDRESS,sizeof(offLineData.u32_offLineDataReadAddress),(char *)&(offLineData.u32_offLineDataReadAddress));
	}
	offLineData.u32_offLineDataReadAddressPersisted = offLineData.u32_offLineDataReadAddress;

	sprintf(printBuf, "\nOff-line Data Read Address 0x%.8X\n",(unsigned int)offLineData.u32_offLineDataReadAddress);
	PRINT_OFFLINE(printBuf);

	Set_Timer(&OFFLINE_DATA_RECORD_TIMER, offline_get_message_period());

	PRINT_OFFLINE("\nOff-line Data Initialized\n");
}
/***************************************************************/
void task_offline()
{
	uint32_t i_msgLen;
	bool b_serverConnection;
	char c_TMessageBuffer[MAX_LOG_ENTRY_SIZE];

	b_serverConnection = gsm_get_server_connection_status();

	switch(offlineTaskState){
		case ONLINE_STATE:
		if(!b_serverConnection){
			Set_Timer(&OFFLINE_DATA_RECORD_TIMER,  offline_get_message_period());
			PRINT_K("\nSwitching to offline state\n");
			offlineTaskState = OFFLINE_STATE;
		}
		break;

		case OFFLINE_STATE:
		if(b_serverConnection){
			if(offLineData.bufferedBytes > 0)
				offline_flush_data();
		  PRINT_K("\nSwitching to online state\n");
		  offlineTaskState = ONLINE_STATE;
		}
		else{
			if(mn_timer_expired(&OFFLINE_DATA_RECORD_TIMER)){
				 PRINT_OFFLINE("\nLogging Data\n");
				if((gps_get_mode2() == NMEA_GSA_MODE2_3D) && (gps_get_position_fix())){
					memset(c_TMessageBuffer, 0, MAX_LOG_ENTRY_SIZE);
					i_msgLen = Trio_PrepareTMessage(c_TMessageBuffer, FALSE);
					PRINT_K(c_TMessageBuffer);
					offline_add_log_entry(c_TMessageBuffer, i_msgLen);
				}
				Set_Timer(&OFFLINE_DATA_RECORD_TIMER, offline_get_message_period());
			}

			if(get_t_msg_event_status()){
				 PRINT_OFFLINE("\nLogging Event\n");
				memset(c_TMessageBuffer, 0, MAX_LOG_ENTRY_SIZE);
				i_msgLen = Trio_PrepareTMessage(c_TMessageBuffer, FALSE);
				PRINT_K(c_TMessageBuffer);
				offline_add_log_entry(c_TMessageBuffer, i_msgLen);
			}
		}
		break;
	}
}
/*****************************************************************/
void offline_refresh_message_timer()
{
	Set_Timer(&OFFLINE_DATA_RECORD_TIMER, offline_get_message_period());
}
/******************************************************************/
TIMER_TICK_T  offline_get_log_period()
{
	if(Get_IgnitionStatus())
		return OFFLINE_IGNITED_LOG_PERIOD;
	else
		return OFFLINE_NOT_IGNITED_LOG_PERIOD;
}
/*****************************************************************/
void offline_add_log_entry(char *p_msgPtr, uint32_t i_msgLen)
{
	/* When caching off-line data, 6 extra bytes (including '[' and ']' chars)
	 * added to T message. So make sure that we have enough space in cache to store
	 * newly arrived data. Otherwise first flush cache buffer,
	                [STX][T Message][CRC 2 bytes][ETX]
	*/
//	char printBuf[128];
	if(offLineData.bufferedBytes + i_msgLen + 6 > OFFLINE_DATA_CACHE_SIZE){
	/*	sprintf(printBuf, "\nWriting %d bytes to FLASH.....",offLineData.bufferedBytes);
		PRINT_OFFLINE(printBuf);*/
		offline_flush_data();
	//	PRINT_OFFLINE("Done\n");

	}
	offline_append_data_to_cache(p_msgPtr, i_msgLen);
}
/*****************************************************************/
int get_offline_log_item(LOG_ENTRY_INFO_T *log_info)
{
	uint16_t i_blockSize;
	uint16_t u16_crcCalc;
	char *p_msgStart, *p_msgEnd, *p_nextStx;
	char strCRCRead[5];
	char strCRCCalc[9];
	char buffer[MAX_LOG_ENTRY_SIZE];
//	char printfBuf[128];

	log_info->b_validEntry = FALSE;

	if(offLineData.u32_offLineDataWriteAddress == offLineData.u32_offLineDataReadAddress)
		return -1;

	memset(log_info->logBuffer, 0, MAX_LOG_ENTRY_SIZE);
	memset(strCRCCalc, 0, sizeof(strCRCCalc));

	i_blockSize  = offline_get_read_block_size();

	if(i_blockSize >= MAX_LOG_ENTRY_SIZE)
		i_blockSize = MAX_LOG_ENTRY_SIZE;

	/*sprintf(printfBuf, "\nBlock size %u\n", (unsigned int)i_blockSize);
		 PRINT_OFFLINE(printfBuf);*/

/*	sprintf(printfBuf, "\nReading Off-line data from %.8X\n", (unsigned int)offLineData.u32_offLineDataReadAddress);
	 PRINT_OFFLINE(printfBuf);*/

	sst25_read_array(offLineData.u32_offLineDataReadAddress, i_blockSize, buffer);

	p_msgStart = strchr(buffer, CHAR_STX);    /* pointer to message start address*/
	p_msgEnd = strchr(buffer, CHAR_ETX);      /* pointer to message end address */

/*	sprintf(printfBuf, "\n%X,%X, %d bytes read from FLASH\n",p_msgEnd ,p_msgStart, (int)(p_msgEnd - p_msgStart));
	 PRINT_OFFLINE(printfBuf);*/
	/* both [ETX] and [STX] found but in reverse order */
	if(p_msgStart != NULL && p_msgEnd != NULL){
		if(p_msgStart > p_msgEnd){
			offLineData.u32_offLineDataReadAddress += (p_msgStart - buffer);
		//	 PRINT_OFFLINE("\nERROR!.Frame start stop characters in reverse order\n");
		}
		else{ /* both [ETX] and [STX] found in correct order */
			*p_msgEnd = '\0';
			strcpy(strCRCRead, p_msgEnd - 4);    /* extract CRC bytes in current record */
			*(p_msgEnd - 4) = '\0';

			u16_crcCalc = crc16(p_msgStart + 1, strlen(p_msgStart + 1));
			sprintf(strCRCCalc,"%X", u16_crcCalc);

			if(strcmp(strCRCRead, strCRCCalc) == 0){   /* compare CRC strings*/
				strcpy(log_info->logBuffer, p_msgStart + 1);
				log_info->b_validEntry = TRUE;
				PRINT_OFFLINE("\nCorrect Log Entry\n");
			}
			else{
				 PRINT_OFFLINE("\nCRC Error\n");
				log_info->b_validEntry = FALSE;
			}
			offLineData.u32_offLineDataReadAddress += (p_msgEnd - p_msgStart + 1);
		}
	}
	/* both [ETX] and [STX] not found */
	else if(p_msgStart == NULL && p_msgEnd == NULL){
		PRINT_OFFLINE("\nERROR!.[ETX] and [STX] not found\n");
		buffer[MAX_LOG_ENTRY_SIZE - 1] = '\0';
		offLineData.u32_offLineDataReadAddress +=  i_blockSize;
	}
	/* [ETX] found  no [STX] found */
	else if(p_msgStart == NULL && p_msgEnd != NULL){
		*p_msgEnd = '\0';
		offLineData.u32_offLineDataReadAddress +=  (p_msgEnd - buffer + 1);
		 PRINT_OFFLINE("\nERROR!.[ETX] found no [STX] found\n");
	}
	/* [STX] found but no [ETX] found */
	else if(p_msgStart != NULL && p_msgEnd == NULL){
		p_nextStx = strchr(p_msgStart + 1, CHAR_STX);
		if(p_nextStx != NULL){
			*(p_nextStx - 1) ='\0';
			offLineData.u32_offLineDataReadAddress += (p_nextStx - buffer);
			 PRINT_OFFLINE("\nERROR!.[STX] found no [ETX] found\n");
		}
		else
			offLineData.u32_offLineDataReadAddress += i_blockSize;
	}

	if(offLineData.u32_offLineDataReadAddress >= LOG_UPPER_BOUNDARY_ADDRESS)
		offLineData.u32_offLineDataReadAddress = 0;

	if(offLineData.u32_offLineDataWriteAddress == offLineData.u32_offLineDataReadAddress)
		offline_update_log_read_address(offLineData.u32_offLineDataReadAddress);

	return 1;
}
/*********************************************************************/
static uint16_t offline_get_read_block_size()
{
	if(offLineData.u32_offLineDataWriteAddress > offLineData.u32_offLineDataReadAddress){
		if((offLineData.u32_offLineDataReadAddress + MAX_LOG_ENTRY_SIZE) > offLineData.u32_offLineDataWriteAddress)
			return offLineData.u32_offLineDataWriteAddress - offLineData.u32_offLineDataReadAddress;
		else
			return MAX_LOG_ENTRY_SIZE;
	}
	else{
		if((offLineData.u32_offLineDataReadAddress + MAX_LOG_ENTRY_SIZE) >= LOG_UPPER_BOUNDARY_ADDRESS)
			return LOG_UPPER_BOUNDARY_SECTOR- offLineData.u32_offLineDataReadAddress;
		else
			return MAX_LOG_ENTRY_SIZE;
	}
}
/*****************************************************************/
void offline_append_data_to_cache(char *p_msgPtr, uint16_t i_msgLen)
{
	uint16_t u16_crc;
	char *p_bufPtr;
	char  strCRC[4];
	char *p_msgStart;
//	char printfBuf[128];

    memset(strCRC, 0, sizeof(strCRC));

    /* get o pointer to the beginning of next empty cache location */
    p_bufPtr = &offLineData.buffer[offLineData.bufferedBytes];
    p_msgStart = p_bufPtr;

    *(p_bufPtr++) = CHAR_STX;      /* start log message */

    u16_crc = crc16(p_msgPtr, i_msgLen);
    memcpy(p_bufPtr, p_msgPtr, i_msgLen);

    p_bufPtr+= i_msgLen;
    sprintf(strCRC,"%.4X",u16_crc);
    memcpy(p_bufPtr, strCRC, sizeof(strCRC));
    p_bufPtr+= sizeof(strCRC);

    *(p_bufPtr++) = CHAR_ETX;     /* end log message */
    *p_bufPtr = '\0';             /* stringize message*/

    offLineData.bufferedBytes+= (p_bufPtr- p_msgStart);
  /*  PRINT_OFFLINE(p_msgPtr);
    sprintf(printfBuf, "\n%d bytes buffered\n", p_bufPtr- p_msgStart);
    PRINT_OFFLINE(printfBuf);*/

}
/**********************************************************************/
static void offline_flush_data()
{
//	char printBuf[128];
	uint32_t u32_nextWriteAddress = offLineData.u32_offLineDataWriteAddress;

	if((offline_get_end_write_sector() == LOG_LOWER_BOUNDARY_SECTOR) &&
		   (offline_get_current_write_address() == (LOG_UPPER_BOUNDARY_SECTOR - 1))){
		//	PRINT_K("CASE 1\n");
			offLineData.u32_offLineDataWriteAddress = LOG_LOWER_BOUNDARY_ADDRESS;
			u32_nextWriteAddress = offLineData.bufferedBytes;
			 if(offline_get_current_read_sector() == LOG_LOWER_BOUNDARY_SECTOR){
				 offLineData.u32_offLineDataReadAddress = SST25_SECTOR_SIZE;
				 offline_update_log_read_address(offLineData.u32_offLineDataReadAddress);
			 }
	}
	else if((offline_get_end_write_sector() == LOG_UPPER_BOUNDARY_SECTOR - 1) &&
			(offline_get_current_read_sector() == LOG_UPPER_BOUNDARY_SECTOR - 1)){
		 //	PRINT_K("CASE 2\n");
			offLineData.u32_offLineDataReadAddress = LOG_LOWER_BOUNDARY_ADDRESS;
			u32_nextWriteAddress += offLineData.bufferedBytes;
	}
	else{
		if((offline_get_end_write_sector() > offline_get_current_write_address()) &&
				offline_get_end_write_sector() == offline_get_current_read_sector()){
				//	PRINT_K("CASE 3\n");
				offLineData.u32_offLineDataReadAddress = (offline_get_current_read_sector() + 1) * SST25_SECTOR_SIZE;
				offline_update_log_read_address(offLineData.u32_offLineDataReadAddress);
					//PRINT_K("\r\nFLUSH 3\r\n");
			}
		u32_nextWriteAddress += offLineData.bufferedBytes;
	}

	sst25_write_array(offLineData.buffer, offLineData.bufferedBytes, offLineData.u32_offLineDataWriteAddress);
	offline_update_log_write_address(u32_nextWriteAddress);
	offLineData.u32_offLineDataWriteAddress = u32_nextWriteAddress;

/*	sprintf(printBuf,"\nLog Read Address 0x%.8X Log Write Address 0x%.8X\n",
			                      (unsigned int)offLineData.u32_offLineDataReadAddress,
								  (unsigned int)offLineData.u32_offLineDataWriteAddress);
	PRINT_OFFLINE(printBuf);*/

	offLineData.bufferedBytes = 0;
	memset(offLineData.buffer, 0, OFFLINE_DATA_CACHE_SIZE);
}
/****************************************************************/
uint16_t offline_get_end_write_sector()
{
	if(offLineData.u32_offLineDataWriteAddress + offLineData.bufferedBytes >= LOG_UPPER_BOUNDARY_ADDRESS)
	  return 0;
	else
	  return (offLineData.u32_offLineDataWriteAddress + offLineData.bufferedBytes) / SST25_SECTOR_SIZE;
}
/****************************************************************/
uint16_t offline_get_current_read_sector()
{
	 return (uint16_t)(offLineData.u32_offLineDataReadAddress / SST25_SECTOR_SIZE);
}
/******************************************************************/
uint16_t offline_get_current_write_address()
{
	 return (uint16_t)(offLineData.u32_offLineDataWriteAddress / SST25_SECTOR_SIZE);
}
/****************************************************************/
void offline_update_log_read_address(uint32_t address)
{
	//char printBuf[128];
	uint32_t read_address;

	sst25_disable_write_lock();

	read_address = get_next_write_address_in_page(OFFLINE_DATA_READ_ADDRESS);

/*	sprintf(printBuf,"\nWriting 0x%.8X to 0x%.8X\n", (unsigned int)address, (unsigned int)read_address);
	PRINT_OFFLINE(printBuf);*/

	sst25_write_array((char *)&address, sizeof(address), read_address);
	sst25_enable_write_lock();
	offLineData.u32_offLineDataReadAddressPersisted = offLineData.u32_offLineDataReadAddress;
}
/*******************************************************************/
void offline_update_log_write_address(uint32_t address)
{
//	char printBuf[128];
	uint32_t wrt_address;

	sst25_disable_write_lock();

	wrt_address = get_next_write_address_in_page(OFFLINE_DATA_WRITE_ADDRESS);

/*	sprintf(printBuf,"\nWriting 0x%.8X to 0x%.8X...", (unsigned int)address, (unsigned int)wrt_address);
	PRINT_OFFLINE(printBuf);*/

	sst25_write_array((char *)&address, sizeof(address), wrt_address);
	sst25_enable_write_lock();

	PRINT_K("Done\n");
}
/********************************************************************/
TIMER_TICK_T  offline_get_message_period()
{
	TIMER_TICK_T period;
	//char printBuf[128];


	if(Get_IgnitionStatus() || gps_is_moving())
		period =  MAX(gsm_get_data_send_period(), OFFLINE_IGNITED_LOG_PERIOD);
	else
		period =  MAX(gsm_get_data_send_period(), OFFLINE_NOT_IGNITED_LOG_PERIOD);


/*	sprintf(printBuf, "\nSetting log period to %d msecs\n", (int)period);
	PRINT_OFFLINE(printBuf);*/
	return period;
}
/********************************************************************/
uint32_t get_next_write_address_in_page(uint32_t pageStartAddress)
{
	uint32_t buffer, i;

	for(i = 0; i < SST25_SECTOR_SIZE; i = i+ 4){
		sst25_read_array(pageStartAddress + i, sizeof(uint32_t), (char *)&buffer);
		if(buffer == 0xFFFFFFFF)
			return pageStartAddress + i;
	}
	return pageStartAddress;
}
/********************************************************************/
uint32_t get_last_valid_value_in_sector(uint32_t u32_sectorStartAddress)
{
	uint32_t buffer, i;
	uint32_t temp = 0, val = 0;
	bool found  = FALSE;

	for(i = 0; i < SST25_SECTOR_SIZE; i = i+ 4){
		sst25_read_array(u32_sectorStartAddress + i, sizeof(uint32_t), (char *)&buffer);
		if(buffer == 0xFFFFFFFF){
			val = temp;
			found = TRUE;
			break;
		}
		else
			temp = buffer;
	}
	if(found)
		return val;
	else
		return buffer;
	/*	if(u32_flashKmCounter == 0xFFFFFFFF)
			u32_flashKmCounter = 0;*/
	/*PRINT_K("Km: ");
	PRINT_INT(u32_flashKmCounter);
	PRINT_K(CHAR_ENTER);*/
}
