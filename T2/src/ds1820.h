/*
 * ds1820.h
 *
 *  Created on: 4 Mar 2016
 *      Author: admin
 */

#ifndef DS1820_H_
#define DS1820_H_

#define TEMP_RES              0x100 /* temperature resolution => 1/256°C = 0.0039°C */

/* -------------------------------------------------------------------------- */
/*                         DS1820 Timing Parameters                           */
/* -------------------------------------------------------------------------- */

#define DS1820_RST_PULSE                       480   /* master reset pulse time in [us] */
#define DS1820_MSTR_BITSTART                   3     /* delay time for bit start by master */
#define DS1820_DLY_BEFORE_PRESENCE_DETECT      100   /* delay before presence detect */
#define DS1820_DLY_AFTER_PRESENCE_DETECT       300   /* delay after presence detect */
#define DS1820_PRESENCE_WAIT                   400   /* delay after master reset pulse in [us] */
#define DS1820_PRESENCE_FIN                    480   /* delay after reading of presence pulse [us] */
#define DS1820_BITREAD_DLY                     8     /* bit read delay */
#define DS1820_BITWRITE_DLY                    60    /* bit write delay */
#define DS1820_DLY_AFTER_READ                  120   /* delay after read operation */


/* -------------------------------------------------------------------------- */
/*                            DS1820 Registers                                */
/* -------------------------------------------------------------------------- */

#define DS1820_REG_TEMPLSB    0
#define DS1820_REG_TEMPMSB    1
#define DS1820_REG_CNTREMAIN  6
#define DS1820_REG_CNTPERSEC  7
#define DS1820_SCRPADMEM_LEN  9     /* length of scratchpad memory */

#define DS1820_ADDR_LEN       8

/* -------------------------------------------------------------------------- */
/*                            DS1820 Commands                                 */
/* -------------------------------------------------------------------------- */

#define DS1820_CMD_SEARCHROM     0xF0
#define DS1820_CMD_READROM       0x33
#define DS1820_CMD_MATCHROM      0x55
#define DS1820_CMD_SKIPROM       0xCC
#define DS1820_CMD_ALARMSEARCH   0xEC
#define DS1820_CMD_CONVERTTEMP   0x44
#define DS1820_CMD_WRITESCRPAD   0x4E
#define DS1820_CMD_READSCRPAD    0xBE
#define DS1820_CMD_COPYSCRPAD    0x48
#define DS1820_CMD_RECALLEE      0xB8


#define DS1820_FAMILY_CODE_DS18B20      0x28
#define DS1820_FAMILY_CODE_DS18S20      0x10

#define DS1820_TEMP_CONVERSION_DELAY    150

#endif /* DS1820_H_ */
