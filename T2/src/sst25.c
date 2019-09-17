#include "chip.h"
#include "board.h"
#include "timer.h"
#include "spi.h"
#include "sst25.h"
#include "ProcessTask.h"
#include "ATCommands.h"
#include "settings.h"
//#include "messages.h"
#include "gsm.h"
#include "status.h"
#include "trace.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>

static void sst25_dump_info(FLASH_INFO *flash_info);

static bool b_extFlashInitialized = FALSE;

void sst25_init()
{
	FLASH_INFO flash_info;

	Trio_Init_SPI();
	/*SST25_EnableWSR();
	SST25_WriteSR(0x00);
	SST25_WriteEnable();*/

	sst25_read_flashid(&flash_info);
	sst25_dump_info(&flash_info);

	b_extFlashInitialized = TRUE;
}
/***********************************************************/
bool sst25_is_initialized()
{
	return b_extFlashInitialized;
}
/***********************************************************/
 void sst25_dump_info(FLASH_INFO *flash_info)
{
	switch(flash_info->flashType){
		case S25FL208:
		PRINT_K("Spansion S25FL208\n");
		break;

		case SST25VF080B:
		PRINT_K("Microchip SST25VF080B\n");
		break;

		default:
		PRINT_K("Unknown Flash Type\n");
		break;
	}
}

 /**************************************************************/
bool sst25_is_empty()
{
	uint32_t buffer, i;
	for(i = 0; i < SST25_MAX_SECTOR_NUMBER; i++){
		sst25_read_array(SST25_SECTOR_SIZE * i, sizeof(uint32_t), (char *)&buffer);
		if(buffer != 0xFFFFFFFF){
			return FALSE;
		}
	}
	return TRUE;
}
/**************************************************************/
void sst25_erase_flash()
{
	TIMER_INFO_T spiTimer;

	uint8_t spiRxBuffer[4];
	uint8_t spiTxBuffer[4];


	sst25_disable_write_lock();
	SST25_WriteEnable();

	PRINT_K("\nErasing...");
	spiTxBuffer[0] = SST25_CE;                   /* Erase chip command (C7h) */
	xmitRecvSpiData(spiTxBuffer, spiRxBuffer, 1);
	Set_Timer(&spiTimer, FLASH_CHIP_ERASE_TIME);
	while(sst25_is_busy() && !mn_timer_expired(&spiTimer));

	sst25_enable_write_lock();
	PRINT_K("Done\n");
}
/**************************************************************/
bool sst25_write_array(char *array, uint16_t length, uint32_t u32_flashAddress)
{
	uint16_t i;

	for(i = 0; i< length; i++){
		if((u32_flashAddress & SST25_FLASH_SECTOR_MASK) == 0)
			sst25_erase_sector(u32_flashAddress/SST25_SECTOR_SIZE);

		sst25_write_byte(u32_flashAddress, array[i]);
		u32_flashAddress++;
	}
	return TRUE;
}
/***************************************************************/
void sst25_write_byte(uint32_t address, uint8_t data)
{
	TIMER_INFO_T spiTimer;
	uint8_t spiRxBuffer[5];
	uint8_t spiTxBuffer[5];

	SST25_WriteEnable();

	spiTxBuffer[0] = SST25_BP; /* Byte program command (0x02) */
	spiTxBuffer[1] = WORD32_BYTE2(address);
	spiTxBuffer[2] = WORD32_BYTE1(address);
	spiTxBuffer[3] = WORD32_BYTE0(address);
	spiTxBuffer[4] = data;

	xmitRecvSpiData(spiTxBuffer, spiRxBuffer, sizeof(spiTxBuffer));

	Set_Timer(&spiTimer, FLASH_WRITE_BYTE_TIMEOUT);
	while(sst25_is_busy() && !mn_timer_expired(&spiTimer));

}
/******************************************************************/
bool sst25_read_array(uint32_t address, uint16_t length, char *buffer)
{
	int i;
	uint8_t spiRxBuffer[SST25_SECTOR_SIZE];
	uint8_t spiTxBuffer[4];

	spiTxBuffer[0] = SST25_READ;       /* Read byte command (03h) */
	spiTxBuffer[1] = WORD32_BYTE2(address);
	spiTxBuffer[2] = WORD32_BYTE1(address);
	spiTxBuffer[3] = WORD32_BYTE0(address);

	xmitRecvSpiData(spiTxBuffer, spiRxBuffer, length+ 4);

	for(i= 4; i < length+ 4; i++)
		buffer[i - 4] = spiRxBuffer[i];
	return TRUE;
}
/**************************************************************/
void SST25_WriteEnable()
{
	TIMER_INFO_T spiTimer;
	uint8_t spiRxBuffer[4];
	uint8_t spiTxBuffer[4];

//	PRINT_K("SST25_Write_Enable...");
	spiTxBuffer[0] = SST25_WREN; /* send WREN command (0x06) */
	xmitRecvSpiData(spiTxBuffer, spiRxBuffer, 1);
//	PRINT_K("Done...\r\n");
	Set_Timer(&spiTimer, FLASH_WRITE_SR_TIME);

	while(sst25_is_busy() && !mn_timer_expired(&spiTimer));

}
/***************************************************************/
bool sst25_is_busy()
{
	if(sst25_read_sr() & FLASH_BUSY)
		 return TRUE;
	 else
		 return FALSE;
}
/***************************************************************/
uint8_t sst25_read_sr()
{
	uint8_t spiRxBuffer[4];
	uint8_t spiTxBuffer[4];

	//PRINT_K("Reading status register...\r\n");
	spiTxBuffer[0] = SST25_RDSR;

 	xmitRecvSpiData(spiTxBuffer, spiRxBuffer, 2);
 	return spiRxBuffer[1];

 }
/***************************************************************/
bool sst25_erase_sector(uint32_t sector_no)
{
	 TIMER_INFO_T spiTimer;

	 uint8_t spiRxBuffer[4];
	 uint8_t spiTxBuffer[4];
	 char temp[64];

	 if(sector_no > SST25_MAX_SECTOR_NUMBER)
		 return FALSE;

	 uint32_t address = sector_no * SST25_SECTOR_SIZE;

	 SST25_WriteEnable();

	 sprintf(temp,"\nErasing sector %d...", sector_no);
	 PRINT_K(temp);

	 spiTxBuffer[0] = SST25_SE;		/* erase 4K sector command	*/
	 spiTxBuffer[1] = WORD32_BYTE2(address);
	 spiTxBuffer[2] = WORD32_BYTE1(address);
	 spiTxBuffer[3] = WORD32_BYTE0(address);

	 xmitRecvSpiData(spiTxBuffer, spiRxBuffer, 4);
	 Set_Timer(&spiTimer, FLASH_SECTOR_ERASE_TIME);
	 while(sst25_is_busy() && !mn_timer_expired(&spiTimer));

	 PRINT_K("Done\n");
	 return TRUE;
}
/**************************************************************/
bool sst25_read_flashid(FLASH_INFO *flash_info)
{
	uint8_t spiRxBuffer[4];
	uint8_t spiTxBuffer[4];

//	PRINT_K("\nReading Flash Type\n");
	spiTxBuffer[0] = SST25_JEDEC_ID; /* send JEDEC ID command (9Fh) */

	xmitRecvSpiData(spiTxBuffer, spiRxBuffer, sizeof(spiTxBuffer));
	if((spiRxBuffer[1] == 0x01) && (spiRxBuffer[2] == 0x40) && (spiRxBuffer[3] == 0x14)){
	//	PRINT_K("Spansion S25FL208\n");
		return TRUE;
	}
	else{
	//		PRINT_K("ERROR\n");
			return FALSE;
	}
	/*flash_info->flashType =  MK_WORD32(0, spiRxBuffer[3], spiRxBuffer[2], spiRxBuffer[1]);
	return sizeof(spiRxBuffer);*/
}
/***************************************************************/
void sst25_disable_write_lock()
{
	SST25_WriteSR(0);
}
/***************************************************************/
void sst25_enable_write_lock()
{
	SST25_WriteSR(SST25_BP1);
}
/****************************************************************/
bool sst25_get_healt_status()
{
	FLASH_INFO flash_info;
	return sst25_read_flashid(&flash_info);
}
