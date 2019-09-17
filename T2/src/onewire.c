#include "chip.h"
#include "board.h"
#include "bsp.h"
#include "timer.h"
#include "settings.h"
#include "adc.h"
#include "ProcessTask.h"
#include "trace.h"
#include "onewire.h"
#include "gsm.h"
#include "ds1820.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>

#define ONEWIRE1_GPIO_PORT_NUM   0
#define ONEWIRE1_GPIO_PIN_NUM    9

#define ONEWIRE2_GPIO_PORT_NUM   0
#define ONEWIRE2_GPIO_PIN_NUM    7

static const PINMUX_GRP_T T2_OneWirePinMuxTable[] = {
	{ONEWIRE1_GPIO_PORT_NUM,  ONEWIRE1_GPIO_PIN_NUM,  IOCON_MODE_PULLUP | IOCON_FUNC0},
	{ONEWIRE2_GPIO_PORT_NUM,  ONEWIRE2_GPIO_PIN_NUM,  IOCON_MODE_PULLUP | IOCON_FUNC0},
};

static ONEWIRE_PORT_T Port_oneWire[] ={
		{ONEWIRE_PORT_0, ONEWIRE1_GPIO_PORT_NUM, ONEWIRE1_GPIO_PIN_NUM, FALSE, 0, DS1820_START_CONVERSION_STATE, },
		{ONEWIRE_PORT_1, ONEWIRE2_GPIO_PORT_NUM, ONEWIRE2_GPIO_PIN_NUM, FALSE, 0, DS1820_START_CONVERSION_STATE, }
};


#define NUMBER_OF_ONEWIRE_PORTS  (sizeof(Port_oneWire) / sizeof(Port_oneWire[0]))

uint8_t onewire_read(ONEWIRE_PORT_T *oneWirePort);
bool Get_OneWireBit(ONEWIRE_PORT_T *oneWirePort);
void onewire_reset(ONEWIRE_PORT_T *oneWirePort);
void onewire_write(ONEWIRE_PORT_T *oneWirePort, uint8_t data);
void Set_OneWireOutputLow(ONEWIRE_PORT_T *oneWirePort);
void Set_OneWirePinInput(ONEWIRE_PORT_T *oneWirePort);
void Set_OneWireOutputHigh(ONEWIRE_PORT_T *oneWirePort);
float median_filter(float *buffer,  uint8_t size);
void ds1820b_shiftleft_samples(float *buffer, uint8_t size);
uint8_t calc_crc_1wire(uint8_t *data, uint16_t size);
/*********************************************************/
void onewire_init()
{
	int i = 0;
	Chip_IOCON_SetPinMuxing(LPC_IOCON, T2_OneWirePinMuxTable, sizeof(T2_OneWirePinMuxTable) / sizeof(PINMUX_GRP_T));
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, ONEWIRE1_GPIO_PORT_NUM,  ONEWIRE1_GPIO_PIN_NUM);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, ONEWIRE2_GPIO_PORT_NUM,  ONEWIRE2_GPIO_PIN_NUM);

	for(i = 0; i <  NUMBER_OF_ONEWIRE_PORTS; i++){
		Port_oneWire[i].sampleCount = 0;
	}


    PRINT_K("\n1-Wire Interface Initialized\n");

}



/*#define DQ_TRI          LPC_GPIO
#define DQ_PIN			6
#define DQ_PIN_WRITE(x) ((x) ? (DQ_TRI->SET |= (1<<DQ_PIN))  : (DQ_TRI->CLR = (1<<DQ_PIN)) );
#define DQ_PIN_READ 	(((DQ_TRI -> PIN)>>DQ_PIN)&0x01)*/


/*********************************************************/
void onewire_reset2(ONEWIRE_PORT_ID portNum)
{
 /*  uint8_t i;

   char buffer[128];
	for (i= 0; i < NUMBER_OF_ONEWIRE_PORTS; i++){
    	if(Port_oneWire[i].logical_port_num  == portNum){
        	Set_OneWireOutputLow(portNum);
    		DelayUs(DS1820_RST_PULSE);
    		Set_OneWirePinInput(portNum);
    		DelayUs(DS1820_DLY_BEFORE_PRESENCE_DETECT);
    		if(Chip_GPIO_GetPinState(LPC_GPIO, Port_oneWire[i].physical_port_num, Port_oneWire[i].pin_num))
    			Port_oneWire[i].present = FALSE;
    		else
    			Port_oneWire[i].present = TRUE;
    		DelayUs(DS1820_DLY_AFTER_PRESENCE_DETECT);
    		Set_OneWireOutputHigh(portNum);
    	}
    }*/
}
/*********************************************************/
void onewire_reset(ONEWIRE_PORT_T *oneWirePort)
{

   	 Set_OneWireOutputLow(oneWirePort);
     DelayUs(DS1820_RST_PULSE);
     Set_OneWirePinInput(oneWirePort);
     DelayUs(DS1820_DLY_BEFORE_PRESENCE_DETECT);

     	 if(Chip_GPIO_GetPinState(LPC_GPIO, oneWirePort->physical_port_num, oneWirePort->pin_num))
     		 oneWirePort->present = FALSE;
    	 else
    		oneWirePort->present = TRUE;

     DelayUs(DS1820_DLY_AFTER_PRESENCE_DETECT);
     Set_OneWireOutputHigh(oneWirePort);
}
/*********************************************************/
void onewire_write2(ONEWIRE_PORT_ID portNum, uint8_t data)
{
  /*  uint8_t i, bitshifter;

    bitshifter = 1;

    for (i= 0; i< 8; i++){
        if (data & bitshifter){
        	Set_OneWireOutputLow(portNum);
            DelayUs(DS1820_MSTR_BITSTART);
            Set_OneWirePinInput(portNum);
            DelayUs(DS1820_BITWRITE_DLY);
        }
        else{
        	Set_OneWireOutputLow(portNum);
            DelayUs(DS1820_BITWRITE_DLY);
            Set_OneWirePinInput(portNum);
            DelayUs(DS1820_MSTR_BITSTART);
        }
	bitshifter = bitshifter<<1;
    }*/
}
/*********************************************************/
void onewire_write(ONEWIRE_PORT_T *oneWirePort, uint8_t data)
{
    uint8_t i, bitshifter;

    bitshifter = 1;

    for (i= 0; i< 8; i++){
        if (data & bitshifter){
        	Set_OneWireOutputLow(oneWirePort);
            DelayUs(DS1820_MSTR_BITSTART);
            Set_OneWirePinInput(oneWirePort);
            DelayUs(DS1820_BITWRITE_DLY);
        }
        else{
        	Set_OneWireOutputLow(oneWirePort);
            DelayUs(DS1820_BITWRITE_DLY);
            Set_OneWirePinInput(oneWirePort);
            DelayUs(DS1820_MSTR_BITSTART);
        }
	bitshifter = bitshifter<<1;
    }
}
/*********************************************************/
uint8_t onewire_read2(ONEWIRE_PORT_ID portNum)
{
 /*   uint8_t i;
    uint8_t data, bitshifter;
    data = 0; bitshifter = 1;

    for (i = 0; i < 8; i++){
    	Set_OneWireOutputLow(portNum);
        DelayUs(2 * DS1820_MSTR_BITSTART);
        Set_OneWirePinInput(portNum);
        DelayUs(DS1820_BITREAD_DLY);

        if(Get_OneWireBit(portNum))
            data |= bitshifter;
        DelayUs(DS1820_DLY_AFTER_READ);
        bitshifter = bitshifter<<1;
    }
    return data;*/
}
/*********************************************************/
uint8_t onewire_read(ONEWIRE_PORT_T *oneWirePort)
{
    uint8_t i;
    uint8_t data, bitshifter;
    data = 0; bitshifter = 1;

    for (i = 0; i < 8; i++){
    	Set_OneWireOutputLow(oneWirePort);
        DelayUs(2 * DS1820_MSTR_BITSTART);
        Set_OneWirePinInput(oneWirePort);
        DelayUs(DS1820_BITREAD_DLY);

        if(Get_OneWireBit(oneWirePort))
            data |= bitshifter;
        DelayUs(DS1820_DLY_AFTER_READ);
        bitshifter = bitshifter<<1;
    }
    return data;
}
/*********************************************************/
void Set_OneWireOutputLow2(ONEWIRE_PORT_ID portNum)
{
	uint8_t i;

	/*for (i= 0; i < NUMBER_OF_ONEWIRE_PORTS; i++){
	    	if(Port_oneWire[i].logical_port_num  == portNum){
	    		Chip_GPIO_SetPinDIROutput(LPC_GPIO, Port_oneWire[i].physical_port_num, Port_oneWire[i].pin_num);
	    		Chip_GPIO_SetPinState(LPC_GPIO, Port_oneWire[i].physical_port_num,  Port_oneWire[i].pin_num, FALSE);
	    	}
	 }*/
}
/*********************************************************/
void Set_OneWireOutputLow(ONEWIRE_PORT_T *oneWirePort)
{
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, oneWirePort->physical_port_num, oneWirePort->pin_num);
	Chip_GPIO_SetPinState(LPC_GPIO, oneWirePort->physical_port_num,  oneWirePort->pin_num, FALSE);
}
/*********************************************************/
void Set_OneWireOutputHigh2(ONEWIRE_PORT_ID portNum)
{
/*	uint8_t i;

	for (i= 0; i < NUMBER_OF_ONEWIRE_PORTS; i++){
		 if(Port_oneWire[i].logical_port_num  == portNum){
		   Chip_GPIO_SetPinDIROutput(LPC_GPIO, Port_oneWire[i].physical_port_num, Port_oneWire[i].pin_num);
		   Chip_GPIO_SetPinState(LPC_GPIO, Port_oneWire[i].physical_port_num,  Port_oneWire[i].pin_num, TRUE);
		 }
	}*/
}
/*********************************************************/
void Set_OneWireOutputHigh(ONEWIRE_PORT_T *oneWirePort)
{
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, oneWirePort->physical_port_num, oneWirePort->pin_num);
	Chip_GPIO_SetPinState(LPC_GPIO, oneWirePort->physical_port_num,  oneWirePort->pin_num, TRUE);
}
/*********************************************************/
void Set_OneWirePinInput2(ONEWIRE_PORT_ID portNum)
{
	uint8_t i;

	/*for (i= 0; i < NUMBER_OF_ONEWIRE_PORTS; i++){
		if(Port_oneWire[i].logical_port_num  == portNum)
			Chip_GPIO_SetPinDIRInput(LPC_GPIO, Port_oneWire[i].physical_port_num, Port_oneWire[i].pin_num);
	}*/
}
/*********************************************************/
void Set_OneWirePinInput(ONEWIRE_PORT_T *oneWirePort)
{
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, oneWirePort->physical_port_num, oneWirePort->pin_num);
}
/*********************************************************/
bool Get_OneWireBit2(ONEWIRE_PORT_ID portNum)
{
/*	uint8_t i;

	for (i= 0; i < NUMBER_OF_ONEWIRE_PORTS; i++){
			if(Port_oneWire[i].logical_port_num  == portNum)
				 return Chip_GPIO_GetPinState(LPC_GPIO, Port_oneWire[i].physical_port_num, Port_oneWire[i].pin_num);
	}
	return 0;*/
}
/*********************************************************/
bool Get_OneWireBit(ONEWIRE_PORT_T *oneWirePort)
{
	return Chip_GPIO_GetPinState(LPC_GPIO, oneWirePort->physical_port_num, oneWirePort->pin_num);
}
/*********************************************************/
float ds18b20_read2(ONEWIRE_PORT_ID portNum)
{
/*	uint16_t raw_total;
    uint8_t busy = 0;
    uint8_t num1,num2;

    onewire_reset(portNum);
    if(Port_oneWire[portNum].present){
    	onewire_write(portNum, DS1820_CMD_SKIPROM);
    	onewire_write(portNum, DS1820_CMD_CONVERTTEMP);
    	while(busy == 0)
    		busy = onewire_read(portNum);

    	onewire_reset(portNum);

    	onewire_write(portNum, DS1820_CMD_SKIPROM);
    	onewire_write(portNum, DS1820_CMD_READSCRPAD);

    	num1 = onewire_read(portNum);
    	num2 = onewire_read(portNum);

    	raw_total = num2*256 + num1;
    	raw_total = raw_total & 0xFFF;

    	if (raw_total < 2048)
    		return (float)raw_total / 16;
    	else
    		return ((float)(-((0xFFF - (raw_total + 1)) / 16)));
    }*/
}
/*********************************************************/
void TempSensor1Task()
{
/*	static DS1820_STATE_T ds1820_state = DS1820_START_CONVERSION_STATE;
	uint16_t raw_total;
	uint8_t num1,num2;
	char buffer[126];
	switch(ds1820_state)
	{
		case DS1820_START_CONVERSION_STATE:
		onewire_reset(ONEWIRE_PORT_0);
		onewire_write(ONEWIRE_PORT_0, DS1820_CMD_SKIPROM);
		onewire_write(ONEWIRE_PORT_0, DS1820_CMD_CONVERTTEMP);
		ds1820_state = DS1820_WAIT_CONVERSION_STATE;
		break;

		case DS1820_WAIT_CONVERSION_STATE:
		if(onewire_read(ONEWIRE_PORT_0) != 0){
			onewire_reset(ONEWIRE_PORT_0);

			onewire_write(ONEWIRE_PORT_0, DS1820_CMD_SKIPROM);
			onewire_write(ONEWIRE_PORT_0, DS1820_CMD_READSCRPAD);

			num1 = onewire_read(ONEWIRE_PORT_0);
			num2 = onewire_read(ONEWIRE_PORT_0);
		sprintf(buffer,"\nTemp Sensor num1 ,num2: %d,%d\n", num1,num2);
			PRINT_K(buffer);
			raw_total = num2*256 + num1;
			raw_total = raw_total & 0xFFF;
			if (raw_total < 2048)
				Port_oneWire[ONEWIRE_PORT_0].tempValue = (float)raw_total / 16;

			else
				Port_oneWire[ONEWIRE_PORT_0].tempValue =  ((float)(-((0xFFF - (raw_total + 1)) / 16)));

			ds1820_state= DS1820_START_CONVERSION_STATE;
		}
		break;
	}*/
}
/*********************************************************/
void TempSensor2Task()
{
	/*static DS1820_STATE_T ds1820_state = DS1820_START_CONVERSION_STATE;
	uint16_t raw_total;
	uint8_t num1,num2;
	char buffer[128];
	switch(ds1820_state)
	{
	case DS1820_START_CONVERSION_STATE:
		onewire_reset(ONEWIRE_PORT_1);
		onewire_write(ONEWIRE_PORT_1, DS1820_CMD_SKIPROM);
		onewire_write(ONEWIRE_PORT_1, DS1820_CMD_CONVERTTEMP);
		ds1820_state = DS1820_WAIT_CONVERSION_STATE;
		break;

		case DS1820_WAIT_CONVERSION_STATE:
		if(onewire_read(ONEWIRE_PORT_1) != 0){
			onewire_reset(ONEWIRE_PORT_1);

			onewire_write(ONEWIRE_PORT_1, DS1820_CMD_SKIPROM);
			onewire_write(ONEWIRE_PORT_1, DS1820_CMD_READSCRPAD);

			num1 = onewire_read(ONEWIRE_PORT_1);
			num2 = onewire_read(ONEWIRE_PORT_1);

			raw_total = num2*256 + num1;
			raw_total = raw_total & 0xFFF;

			if (raw_total < 2048)
				Port_oneWire[ONEWIRE_PORT_1].tempValue = (float)raw_total / 16;
			else
				Port_oneWire[ONEWIRE_PORT_1].tempValue = ((float)(-((0xFFF - (raw_total + 1)) / 16)));

			ds1820_state= DS1820_START_CONVERSION_STATE;
		}
		break;
	}*/
}
/**
 * +
 */
void task_temp_sensor()
{
	int i,bytes;
	uint16_t raw_total;
	uint8_t ds1820Data[DS1820_SCRPAD_SIZE];
	uint8_t crc_1wire;
	char printBuf[128];
	float tempBuf[TEMP_MEDIAN_BUFFER_SIZE];

	for (i = 0; i < NUMBER_OF_ONEWIRE_PORTS; i++){
		switch(Port_oneWire[i].state){

			case DS1820_START_CONVERSION_STATE:
			onewire_reset(&Port_oneWire[i]);
			onewire_write(&Port_oneWire[i], DS1820_CMD_SKIPROM);
			onewire_write(&Port_oneWire[i], DS1820_CMD_CONVERTTEMP);
			Port_oneWire[i].state = DS1820_WAIT_CONVERSION_STATE;
			Set_Timer(&Port_oneWire[i].cmd_wait_timer, DS1820_TEMP_CONVERSION_DELAY);
			break;

			case DS1820_WAIT_CONVERSION_STATE:
			if(mn_timer_expired(&Port_oneWire[i].cmd_wait_timer)){
			//	if(onewire_read(&Port_oneWire[i]) != 0){
					onewire_reset(&Port_oneWire[i]);

					onewire_write(&Port_oneWire[i], DS1820_CMD_SKIPROM);
					onewire_write(&Port_oneWire[i], DS1820_CMD_READSCRPAD);
					for(bytes =0; bytes < DS1820_SCRPAD_SIZE; bytes++)
						ds1820Data[bytes] = onewire_read(&Port_oneWire[i]);

					crc_1wire = calc_crc_1wire(ds1820Data, DS1820_SCRPAD_SIZE - 1);
					if( crc_1wire == ds1820Data[DS1820_SCRPAD_SIZE - 1 ]){
						sprintf(printBuf,"\nCorrect 1-Wire Data on Channel %d\n", i + 1);
						PRINT_K(printBuf);
						raw_total = ds1820Data[1] * 256 + ds1820Data[0];
						raw_total = raw_total & 0xFFF;


						if(Port_oneWire[i].sampleCount  < TEMP_MEDIAN_BUFFER_SIZE)
							Port_oneWire[i].sampleCount++;

						if(raw_total < 2048)
							Port_oneWire[i].sampleBuf[Port_oneWire[i].sampleCount - 1] = (float)raw_total / 16;
						else
							Port_oneWire[i].sampleBuf[Port_oneWire[i].sampleCount - 1] = ((float)(-((0xFFF - (raw_total + 1)) / 16)));


						memcpy(tempBuf, Port_oneWire[i].sampleBuf, TEMP_MEDIAN_BUFFER_SIZE * sizeof(float) );
						Port_oneWire[i].tempValue = median_filter(tempBuf, TEMP_MEDIAN_BUFFER_SIZE);

						if(Port_oneWire[i].sampleCount == TEMP_MEDIAN_BUFFER_SIZE)
							ds1820b_shiftleft_samples(Port_oneWire[i].sampleBuf, TEMP_MEDIAN_BUFFER_SIZE);
					}

				Port_oneWire[i].state= DS1820_START_CONVERSION_STATE;
			}
			break;
		}
	}
}
/**
 *
 */
uint8_t calc_crc_1wire(uint8_t *data, uint16_t size)
{
	uint8_t crc = 0;
    for (int i = 0; i < size; ++i ){
    	uint8_t inbyte = data[i];
	        for ( uint8_t j = 0; j < 8; ++j ){
	        	uint8_t mix = (crc ^ inbyte) & 0x01;
	            crc >>= 1;
	            if ( mix ) crc ^= 0x8C;
	            inbyte >>= 1;
	        }
	    }
	    return crc;
}

/**
 *
 */
void ds1820b_shiftleft_samples(float *buffer, uint8_t size)
{
	int i;
	for(i = 0; i < size - 1; i++){
		buffer[i] = buffer[i + 1];
	}
}
/**
 *
 */
void Get_TempSensorValue(ONEWIRE_PORT_ID portNum, ONEWIRE_PORT_T *onewire_port)
{
	memcpy(onewire_port, &Port_oneWire[portNum], sizeof(ONEWIRE_PORT_T));
}
/**
 *
 */
bool ds18b20_get_presence_status()
{
	uint8_t i =0;
	for (i= 0; i < NUMBER_OF_ONEWIRE_PORTS; i++){
	    if(Port_oneWire[i].present)
	    	return TRUE;
	}
	return FALSE;
}
/*********************************************************/
/*float Get_TempSensor2Value()
{
	return tempSensor2Value;
}*/
/*********************************************************/
/*void onewire_reset(void)
{
    DQ_TRI ->DIR |= (1<<DQ_PIN);
    DQ_PIN_WRITE(0);
    DelayUs(480);
    DQ_TRI -> DIR &=~(1<<DQ_PIN);
    DelayUs(400);
    DQ_TRI -> DIR |= (1<<DQ_PIN);
}

void onewire_write(char data)
{
    unsigned char i, bitshifter;
    bitshifter = 1;
    for (i=0; i<8; i++)
    {
        if (data & bitshifter)
        {
            DQ_TRI -> DIR |= (1<<DQ_PIN);
            DQ_PIN_WRITE(0);
            DelayUs(3);
            DQ_TRI -> DIR &=~(1<<DQ_PIN);
            DelayUs(60);
        }
        else
        {
            DQ_TRI -> DIR |= (1<<DQ_PIN);
            DQ_PIN_WRITE(0);
            DelayUs(60);
            DQ_TRI -> DIR &=~(1<<DQ_PIN);
            DelayUs(3);
        }
	bitshifter = bitshifter<<1;
    }
}

unsigned char onewire_read( void )
{
    unsigned char i;
    unsigned char data, bitshifter;
    data = 0; bitshifter = 1;
    for (i=0; i<8; i++)
    {
        DQ_TRI -> DIR |= (1<<DQ_PIN);
        DQ_PIN_WRITE(0);
        DelayUs(6);
        DQ_TRI -> DIR &=~(1<<DQ_PIN);
        //DelayUs(4);
        DelayUs(8);
        if (DQ_PIN_READ)
            data |= bitshifter;
        //DelayUs(50);
        DelayUs(120);
        bitshifter = bitshifter<<1;
    }
    return data;
}

float ds18b20_read(void)
{
    int busy,sayi1,sayi2, raw_total;
    onewire_reset();
    onewire_write(0xCC);
    onewire_write(0x44);
    while(busy==0)
        busy=onewire_read();
    onewire_reset();
    onewire_write(0xCC);
    onewire_write(0xBE);
    sayi1 = onewire_read();
    sayi2 = onewire_read();
    raw_total=sayi2*256+sayi1;
    raw_total = raw_total & 0xFFF;
    Delay(25,NULL);
    if (raw_total < 2048) {
        return (float)raw_total / 16;
    }
    else
    {
    	//return raw_total;
    	//raw_total = raw_total & 0x7FF;
        return ((float)(-((0xFFF - (raw_total + 1)) / 16)));
    }
}*/
