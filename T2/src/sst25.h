
#ifndef SST25_H_
#define SST25_H_

/* FLASH ID and Manufacturer*****************************************************************/
#define  S25FL208                0x00144001      /* Spansion 8Mb SPI Flash */
#define  SST25VF080B             0x008E25BF      /* Microchip 8Mb SPI Flash */
#define  UNKNOWN_FLASH_TYPE      0xFFFFFFFF
typedef struct FLASH_INFO_T
{
	uint32_t flashType;
}FLASH_INFO;
/* SST25 Instructions ***************************************************************/
/*      Command                    Value      Description               Addr   Data */
/*                                                                         Dummy    */
#define SST25_READ                  0x03    /* Read data bytes           3   0  >=1 */
#define SST25_FAST_READ             0x0b    /* Higher speed read         3   1  >=1 */
#define SST25_SE                    0x20    /* 4Kb Sector erase          3   0   0  */
#define SST25_BE32                  0x52    /* 32Kbit block Erase        3   0   0  */
#define SST25_BE64                  0xd8    /* 64Kbit block Erase        3   0   0  */
#define SST25_CE                    0xc7    /* Chip erase                0   0   0  */
#define SST25_CE_ALT                0x60    /* Chip erase (alternate)    0   0   0  */
#define SST25_BP                    0x02    /* Byte program              3   0   1  */
#define SST25_AAI                   0xad    /* Auto address increment    3   0  >=2 */
#define SST25_RDSR                  0x05    /* Read status register      0   0  >=1 */
#define SST25_EWSR                  0x50    /* Write enable status       0   0   0  */
#define SST25_WRSR                  0x01    /* Write Status Register     0   0   1  */
#define SST25_WREN                  0x06    /* Write Enable              0   0   0  */
#define SST25_WRDI                  0x04    /* Write Disable             0   0   0  */
#define SST25_RDID                  0xab    /* Read Identification       0   0  >=1 */
#define SST25_RDID_ALT              0x90    /* Read Identification (alt) 0   0  >=1 */
#define SST25_JEDEC_ID              0x9f    /* JEDEC ID read             0   0  >=3 */
#define SST25_EBSY                  0x70    /* Enable SO RY/BY# status   0   0   0  */
#define SST25_DBSY                  0x80    /* Disable SO RY/BY# status  0   0   0  */

#define FLASH_BUSY                  0x01

#define SST25_SECTOR_SIZE           4096
#define SST25_FLASH_SIZE            (1024 * 1024)
#define SST25_MAX_SECTOR_NUMBER     (SST25_FLASH_SIZE / SST25_SECTOR_SIZE)
#define SST25_FLASH_SECTOR_MASK	    0x00000FFF


#define SST25_BP0                   4
#define SST25_BP1                   8
#define SST25_BP2                  16

#define  PROTECTED_BLOCK_START_ADDRESS          (225 * SST25_SECTOR_SIZE)     /* 0xE1000 */
#define  DIN_FUNCTION_TABLE                     (226 * SST25_SECTOR_SIZE)     /* 0xE2000 */
#define  ENGINE_HOUR_COUNT_ADDRESS              (227 * SST25_SECTOR_SIZE)     /* 0xE3000 */
#define  OFFLINE_DATA_WRITE_ADDRESS             (235 * SST25_SECTOR_SIZE)     /* 0xEB000 */
#define  OFFLINE_DATA_READ_ADDRESS              (236 * SST25_SECTOR_SIZE)     /* 0xEC000 */
#define  KM_COUNTER_FLASH_ADDRESS               (237 * SST25_SECTOR_SIZE)     /* 0xED000 */
#define  FIRST_FLASH_SETTINGS_ADDRESS           (238 * SST25_SECTOR_SIZE)
#define  SECOND_FLASH_SETTINGS_ADDRESS          (239 * SST25_SECTOR_SIZE)
#define  FIRMWARE_MAGICWORD_ADDRESS             (240 * SST25_SECTOR_SIZE)
#define  FIRMWARE_BACKUP_ADDRESS                (241 * SST25_SECTOR_SIZE)
#define  RF_MODULE_MOUNT_ADDRESS                (242 * SST25_SECTOR_SIZE)
#define  RF_NODES_ID_TABLE                      (248 * SST25_SECTOR_SIZE)

#define  LOG_UPPER_BOUNDARY_ADDRESS             (PROTECTED_BLOCK_START_ADDRESS)
#define  LOG_LOWER_BOUNDARY_ADDRESS             (0)

#define  LOG_UPPER_BOUNDARY_SECTOR              (LOG_UPPER_BOUNDARY_ADDRESS / SST25_SECTOR_SIZE)
#define  LOG_LOWER_BOUNDARY_SECTOR              (LOG_LOWER_BOUNDARY_ADDRESS / SST25_SECTOR_SIZE)
/**/

#define FLASH_CHIP_ERASE_TIME                   (15 * SECOND)
#define FLASH_SECTOR_ERASE_TIME                 30           /* 300 ms */
#define FLASH_WRITE_BYTE_TIMEOUT                2            /* 20 ms  */
#define FLASH_WRITE_SR_TIME                     2

#define SST25_DUMMY_BYTE                         0xFF

bool sst25_read_flashid(FLASH_INFO *flash_info);
uint8_t sst25_read_sr();
void sst25_init();
void sst25_erase_flash();
bool sst25_erase_sector(uint32_t sector_no);
void sst25_write_byte(uint32_t address, uint8_t data);
bool sst25_is_empty();
bool sst25_is_busy();
bool sst25_read_array(uint32_t address, uint16_t length, char *buffer);
bool sst25_write_array(char *array, uint16_t length, uint32_t u32_flashAddress);
bool sst25_is_initialized();
void sst25_disable_write_lock();
void sst25_enable_write_lock();


void SST25_EnableWSR();
void SST25_WriteEnable();
bool sst25_get_healt_status();
#endif /* SST25_H_ */
