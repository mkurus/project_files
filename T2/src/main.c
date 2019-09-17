/*
===============================================================================
 Name        : T2.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#include "iap_config.h"
#include "timer.h"
#include "le50.h"
#include "bsp.h"
#include "MMA8652.h"
#include "sst25.h"
#include "gps.h"
#include "gSensor.h"
#include "settings.h"
#include "gsm.h"
#include "i2c.h"
#include "io_ctrl.h"
#include "spi.h"
#include "status.h"
#include "onewire.h"
#include "can.h"
#include "adc.h"
//#include "messages.h"
//#include "offline.h"
#include "trace.h"
#include "ProcessTask.h"
#include "rf_task.h"
#include "string.h"
#include "timer_task.h"
#include "buzzer_task.h"
#include "ign_task.h"
#include "gsm_task.h"
#include "offline_task.h"
#include "scale_task.h"
#include "rf_task.h"
#include "hx711.h"
#endif
#endif

#include <cr_section_macros.h>
#define UPGRADE_PARAMETERS_SEC		15
void bod_init();
void print_device_info();
TIMER_INFO_T timer1;
extern TIMER_INFO_T logTimer;
int main(void)
{

	SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / SYSTICK_PER_SEC);
    bod_init();
    debug_init();
    UpgradeValidityCheck();
    print_device_info();

    gpio_init();

    DisableGpsPower();

   /// offline_task_init();
    status_task_init();
    timer_task_init();
    buzzer_task_init();
    ignition_task_init();
    lora_task_init();
    gsm_task_init();
    dout_task_init();


    sst25_init();

    settings_read_to_ram();

    settings_check_persisted_blockage();
    /* display user settings */
    settings_print();



    i2c_init();
    mma865x_init();
    can_init();
    offline_data_init();
    gsm_init();

   /* le50_uart_init(LE50_BAUD_RATE);
    if(le50_init()){
    	PRINT_K("\nLE50 detected on accessories port.Reserving for LE50 RF module.....\n");
    	rf_init();
    }
    else{
    	le50_uart_init(RFID_BAUD_RATE);
    	PRINT_K("\nCannot find LE50 on accessories port. Reserving for RFID module.....\n");
    }*/

    EnableGpsPower();

    gps_init(0);   /* 0 for auto-baud */

    gps_set_nmea_msg_freq(100);

 /*   onewire_init();
    adc_init();*/

  /*  while(1)
    {
    	Chip_GPIO_SetPinState(LPC_GPIO, DOUT1_GPIO_PORT_NUM, DOUT1_GPIO_PIN_NUM, FALSE);
    	Chip_GPIO_SetPinState(LPC_GPIO, DOUT2_GPIO_PORT_NUM, DOUT2_GPIO_PIN_NUM, FALSE);
    	Delay(10,NULL);
    	Chip_GPIO_SetPinState(LPC_GPIO, DOUT1_GPIO_PORT_NUM, DOUT1_GPIO_PIN_NUM, TRUE);
    	Chip_GPIO_SetPinState(LPC_GPIO, DOUT2_GPIO_PORT_NUM, DOUT2_GPIO_PIN_NUM, TRUE);
    	Delay(10,NULL);

    }*/
  //  test_function_to_register_new_node();
    Set_Timer(&logTimer, 1);
    Process_Task();
    PRINT_K("\nProcessTask Returned. Resetting....\n");
    Delay(SECOND, NULL);
    NVIC_SystemReset();
  /*  while(1){
    if(mn_timer_expired(&HEARTBEAT_TIMER1)){
    			MMA865x_Read_Accel_Values(&mma8652_accel_values);
    			gInputBuffer[gSampleCount] = mma8652_accel_values;

    			if(gSampleCount == MOVING_AVG_WINDOW_SIZE - 1){
    				forwardAccAnalyzeBuffer[i] = MovingAverage(gInputBuffer);
    				sprintf(printBuf, "%f,%f,%f,%f,%f,%f\n",gInputBuffer[gSampleCount].xAccel,
    														gInputBuffer[gSampleCount].yAccel,
															gInputBuffer[gSampleCount].zAccel,
															forwardAccAnalyzeBuffer[i].xAccel,
															forwardAccAnalyzeBuffer[i].yAccel,
															forwardAccAnalyzeBuffer[i].zAccel);
    				PRINT_K(printBuf);


    				ShiftBuffer(gInputBuffer, MOVING_AVG_WINDOW_SIZE);
    				 if(i == (FORWARD_ACC_ANALYZE_BUFFER_SIZE - 1)){
    					 if(CheckSuddenAccelAlarm(forwardAccAnalyzeBuffer, FORWARD_ACC_ANALYZE_BUFFER_SIZE, SUDDEN_ACCL_THREHOLD_VALUE))
    						 PRINT_K("\nAni Hizlanma Alarmi\n");
    					 ShiftBuffer(forwardAccAnalyzeBuffer, FORWARD_ACC_ANALYZE_BUFFER_SIZE);
    				 }


    				 else
    					i++;
    			}
    			else
    			    gSampleCount++;
    			Set_Timer(&HEARTBEAT_TIMER1, 10);
    	}
    }*/


    return 0;
}
/*******************************************************************/
void bod_init()
{
	Chip_SYSCTL_EnableBODReset();
}
/*******************************************************************/
void UpgradeValidityCheck() {

	char buffer[1024];
	uint16_t validity_check = (*(uint16_t *)(UPGRADE_PARAMETERS_ADDR + 1022));
	sprintf(buffer, "\nValidity Check: 0x%X ", validity_check);
	PRINT_K( buffer );
	validity_check = (*(uint16_t *)(UPGRADE_PARAMETERS_ADDR + 1022));
	sprintf(buffer, "\nValidity Check: 0x%X ", validity_check);
	PRINT_K( buffer );
	if( validity_check == 0x9999 ){

			PRINT_K("\nFirst start of upgrade image");
		//	__disable_irq();
		    Chip_IAP_PreSectorForReadWrite(UPGRADE_PARAMETERS_SEC, UPGRADE_PARAMETERS_SEC);
			Chip_IAP_EraseSector(UPGRADE_PARAMETERS_SEC, UPGRADE_PARAMETERS_SEC );
		//	__enable_irq();
			int i = 0;
			for (i=0; i<1022; i++){
				buffer[i] = 0xFF;
			}
			buffer[1022] = 0xAA;
			buffer[1023] = 0xAA;
		//	__disable_irq();
			Chip_IAP_PreSectorForReadWrite( UPGRADE_PARAMETERS_SEC, UPGRADE_PARAMETERS_SEC );
			Chip_IAP_CopyRamToFlash(UPGRADE_PARAMETERS_ADDR,(uint32_t *) buffer,1024);
			//__enable_irq();
			Delay(100, NULL);
	}
}
/*********************************************************/
void  print_device_info()
{
	char printBuf[128];
	extern uint32_t _pvHeapStart;
	extern void _vStackTop(void);
	PRINT_K("\n******************Starting-up T2*****************\n");
	PRINT_K(VERSION);
	PRINT_K("\n*************************************************\n");

    sprintf(printBuf, "\nSystem Clock Freq:%d\n",(int)Chip_Clock_GetSystemClockRate());
	PRINT_K(printBuf);

/*	sprintf(printBuf, "\nHeap Start Address: %.8X\n",&_pvHeapStart);
	PRINT_K(printBuf);*/

	/*sprintf(printBuf, "\nStack Top Address: %.8X\n", &_vStackTop);
	PRINT_K(printBuf);*/

//*	extern  unsigned int _ebss;
/*	unsigned int *SectionTableAddr;

    SectionTableAddr = &_ebss;
    SectionTableAddr = 0XaaBBccdd;
	 sprintf(printBuf, "\nbss_section_table_end = %.8X, %.8X\n", SectionTableAddr, *SectionTableAddr);
	 PRINT_K(printBuf);*/


}
/*void Init_Debug()
{

	Chip_UART_Init(LPC_UART0);
	Chip_UART_SetBaud(LPC_UART0, 115200 );
	Chip_UART_ConfigData(LPC_UART0, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS);


	Chip_UART_TXEnable(LPC_UART0);
}

*/



