/*
 * offline.h
 *
 *  Created on: 25 Mar 2016
 *      Author: admin
 */

#ifndef OFFLINE_H_
#define OFFLINE_H_

#define MAX_LOG_ENTRY_SIZE                          MAX_T_MESSAGE_SIZE
#define OFFLINE_DATA_CACHE_SIZE           	        2048


#define OFFLINE_IGNITED_LOG_PERIOD                   (15 * SECOND)
#define OFFLINE_NOT_IGNITED_LOG_PERIOD               (15 * MINUTE)

typedef enum {
	OFFLINE_STATE,
	ONLINE_STATE
}LOGTASK_STATE;

typedef struct LOG_ENTRY_INFO{
	char logBuffer[MAX_LOG_ENTRY_SIZE];
	uint16_t u16_forward;
	bool b_validEntry;
}LOG_ENTRY_INFO_T;



#define  OFFLINE_DATA_READ_ADDRESS_UPDATE_TIMEOUT     (1  * HOUR)
#define  OFFLINE_DATA_FLUSH_TIMEOUT                   (30 * MINUTE)

#define  VALIDATE_OFFLINE_LOG_ADDRESS(address)        (address >= LOG_UPPER_BOUNDARY_ADDRESS ? FALSE : TRUE)
uint32_t GetNextFlashWriteAddressInPage(uint32_t pageStartAddress);
uint32_t get_last_valid_value_in_sector(uint32_t u32_sectorStartAddress);
uint32_t get_next_write_address_in_page(uint32_t pageStartAddress);
void task_offline();
void offline_refresh_message_timer();
void offline_data_init();
void offline_add_log_entry(char *p_msgPtr, uint32_t i_msgLen);
int get_offline_log_item(LOG_ENTRY_INFO_T *log_info);

#endif /* OFFLINE_H_ */
