/*
 * rfid.h
 *
 *  Created on: 2 Ara 2016
 *      Author: admin
 */

#ifndef RFID_H_
#define RFID_H_

#define    RFID_TX_PORT_NUM                4
#define    RFID_RX_PORT_NUM                4
#define    RDID_TX_PIN_NUM                28
#define    RFID_RX_PIN_NUM                29

#define    NUMBER_OF_RFID_CHANNELS         2

typedef struct {
	uint8_t checkState;                                  /* check rfid on ignition status change, 0 = ignition -off, 1 = ignition on */
	bool check_ch[NUMBER_OF_RFID_CHANNELS];              /* enable/disable rfid read check on this channel*/
	uint16_t duration;
	bool padding[3];                                     /* padding bytes to align 4-byte boundary */
}RFID_CH_SETTING_T;



#endif /* RFID_H_ */
