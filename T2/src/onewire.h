/*
 * onewire.h
 *
 *  Created on: 4 Mar 2016
 *      Author: admin
 */

#ifndef ONEWIRE_H_
#define ONEWIRE_H_

#define DS1820_SCRPAD_SIZE             9
#define TEMP_MEDIAN_BUFFER_SIZE        5

typedef enum DS1820_STATE{
	DS1820_START_CONVERSION_STATE,
	DS1820_WAIT_CONVERSION_STATE
}DS1820_STATE_T;

typedef enum {
	ONEWIRE_PORT_0,
	ONEWIRE_PORT_1
}ONEWIRE_PORT_ID;

typedef struct ONEWIRE_PORT{
	uint8_t logical_port_num;
	uint8_t physical_port_num;
	uint8_t pin_num;
	bool present;
	float tempValue;
	DS1820_STATE_T state;
	TIMER_INFO_T cmd_wait_timer;
	float sampleBuf[TEMP_MEDIAN_BUFFER_SIZE];
    uint8_t sampleCount;
}ONEWIRE_PORT_T;

void task_temp_sensor();
void onewire_init();
/*void onewire_reset(ONEWIRE_PORT_ID portNum);
void onewire_write(ONEWIRE_PORT_ID portNum, uint8_t data);
void Set_OneWireOutputLow(ONEWIRE_PORT_ID portNum);
void Set_OneWireOutputHigh(ONEWIRE_PORT_ID portNum);
void Set_OneWirePinInput(ONEWIRE_PORT_ID portNum);

bool Get_OneWireBit(ONEWIRE_PORT_ID portNum);*/

/*uint8_t onewire_read(ONEWIRE_PORT_ID portNum);*/
/*float ds18b20_read(ONEWIRE_PORT_ID portNum);*/
void task_temp_sensor();
bool  ds18b20_get_presence_status();
void Get_TempSensorValue(ONEWIRE_PORT_ID portNum, ONEWIRE_PORT_T *onewire_port);
/*void onewire_reset(void);
void onewire_write(char data);
unsigned char onewire_read( void );
float ds18b20_read(void);*/

#endif /* ONEWIRE_H_ */
