#include "chip.h"
#include "board.h"
#include "bsp.h"
#include "timer.h"
#include "utils.h"
#include "settings.h"
#include "io_ctrl.h"
#include "ProcessTask.h"
#include "tablet_app.h"
#include "msg_parser.h"

#include "trace.h"
#include "can.h"
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* Pin and port definitions for CAN1 port */
#define CAN_RD1_PORT_NUM      0
#define CAN_TD1_PORT_NUM      0
#define CAN_RD1_PIN_NUM       0
#define CAN_TD1_PIN_NUM       1

/* Pin and port definitions for CAN2 port */
#define CAN_RD2_PORT_NUM      2
#define CAN_TD2_PORT_NUM      2
#define CAN_RD2_PIN_NUM       7
#define CAN_TD2_PIN_NUM       8

/* PGN definitions:
 * 0xFEE9: Fuel consumption
 * 0xFEC1: High resolution vehicle distance
 * 0xFEF1: Cruise control/vehicle speed
 * 0xFEE5: Engine hours, Revolutions
 * 0xFEFC: Dash display
 * 0xF004: Engine speed #1
 * 0xF003: Accelerator Pedal Position #2
 * 0xFEEF: Engine Fluid Level/Pressure  #1
 * 0xFEF5: Ambient Conditions
 * 0xFE6C: Tachograph
 * 0xFEEE: Engine Temperature #1
 */

/* AXOR: 11-bit 500 kbit/s jumper and resistor mounted */
/* Standart CAN(CAN2.0b) 29-bit 250 kbit/s jumper and resistor not -mounted */
const uint32_t can1_mask_11bit[] = { 0x250, 0x251, 0x254, 0x22D, 0x361, 0x3B5, 0x3EC, 0x450, 0x65F, 0x6A0, 0x6A5, 0x6A6, 0x6B5 };
const uint32_t can2_mask_11bit[] = { 0x250, 0x251, 0x254, 0x22D, 0x361, 0x3B5, 0x3EC, 0x450, 0x65F, 0x6A0, 0x6A5, 0x6A6, 0x6B5 };

const uint32_t can1_mask_29bit[] = { 0xFEE9, 0xFEC1, 0xFEF1, 0xFEE5, 0xFEFC, 0xF004, 0xF003, 0xFEEF, 0xFEF5, 0xFE6C, 0xFEEE, 0xFD09 };
const uint32_t can2_mask_29bit[] = { 0xFEE9, 0xFEC1, 0xFEF1, 0xFEE5, 0xFEFC, 0xF004, 0xF003, 0xFEEF, 0xFEF5, 0xFE6C, 0xFEEE, 0xFD09 };

uint32_t CAN1ErrCount = 0;
uint32_t CAN2ErrCount = 0;

//volatile uint32_t CAN1_RxCount = 0;
volatile uint32_t CAN2_RxCount = 0;

volatile uint32_t b_can1RxDone = FALSE;
volatile uint32_t b_can2RxDone = FALSE;

static const J1939_SPN_INFO_T spn_defs_array[] ={

	{ .val = 100, .SpnId = SPN_RPM,           .Length = 2, .StartPos = 4,  .PgnId = PGN_ENGINE_SPEED,   .Offset = 0,    .Res = 0.125,    },
	{ .val = 200, .SpnId = SPN_ACC_PEDAL_POS, .Length = 1, .StartPos = 2,  .PgnId = PGN_ACC_PEDAL_POS,  .Offset = 0,    .Res = 0.4       },
	{ .val = 300, .SpnId = SPN_VEHICLE_SPEED, .Length = 2, .StartPos = 2,  .PgnId = PGN_VEHICLE_SPEED,  .Offset = 0,    .Res = 0.00390625},
	{ .val = 400, .SpnId = SPN_ENGINE_TEMP,   .Length = 1, .StartPos = 1,  .PgnId = PGN_ENGINE_TEMP,    .Offset = -40,  .Res = 1         },
	{ .val = 500, .SpnId = SPN_FUEL_LEVEL,    .Length = 1, .StartPos = 2,  .PgnId = PGN_FUEL_LEVEL,     .Offset = 0,    .Res= 0.4        },
	{ .val = 600, .SpnId = SPN_TOTAL_FUEL,    .Length = 4, .StartPos = 5,  .PgnId = PGN_TOTAL_FUEL,     .Offset = 0,    .Res = 0.5       },
	{ .val = 700, .SpnId = SPN_ODOMETER,      .Length = 4, .StartPos = 1,  .PgnId = PGN_ODOMETER,       .Offset = 0,    .Res = 0.005     },
	{ .val = 800, .SpnId = SPN_AMB_AIR_TEMP,  .Length = 2, .StartPos = 4,  .PgnId = PGN_AMB_CONDITIONS, .Offset = -273, .Res = 0.03125   },
	{ .val = 900, .SpnId = SPN_TRIP_FUEL   ,  .Length = 4, .StartPos = 1,  .PgnId = PGN_TOTAL_FUEL,     .Offset = 0,    .Res = 0.05      },
};

/*typedef struct J1939_JSON_MAPPING{

}J1939_JSON_MAPPING_T;*/
/* TX and RX Buffers for CAN message */
CAN_MSG MsgBuf_RX1[CAN_BUFFER_SIZE];
CAN_MSG MsgBuf_RX2[CAN_BUFFER_SIZE];

CAN_MSG can1_data[CAN_MASK_COUNT];                      /* Can data fields are 8 byte.*/
CAN_MSG can2_data[CAN_MASK_COUNT];                      /* Can data fields are 8 byte.*/

//TIMER_INFO_T canbus_over_mobile_app_timer;

bool isCanRead = FALSE;
const uint32_t *can_mask_ptr;
static const PINMUX_GRP_T T2_CanPortMuxTable[] = {

	{CAN_RD1_PORT_NUM,  CAN_RD1_PIN_NUM,  IOCON_MODE_PULLUP | IOCON_FUNC1},	/* configure CAN1 RD pin */
	{CAN_TD1_PORT_NUM,  CAN_TD1_PIN_NUM,  IOCON_MODE_PULLUP | IOCON_FUNC1},	/* configure CAN1 TD pin */
	{CAN_RD2_PORT_NUM,  CAN_RD2_PIN_NUM,  IOCON_MODE_PULLUP | IOCON_FUNC1},	/* configure CAN2 RD pin */
	{CAN_TD2_PORT_NUM,  CAN_TD2_PIN_NUM,  IOCON_MODE_PULLUP | IOCON_FUNC1},	/* configure CAN2 TD pin */
};
TIMER_INFO_T can_alive_timer;

bool get_can_msg_by_spn(const J1939_SPN_INFO_T *spn, CAN_MSG *can_msg_container);
float get_J1939_spn_value(const J1939_SPN_INFO_T *spn, CAN_MSG *can_msg_container);
char *map_to_json_buf(J1939_SPN_TYPE_T spnId, SPN_JSON_MAPPING_T *spn_json_mapping_t, uint16_t mapping_size);
/*bool search_spn(uint32_t msg_id, J1939_SPN_T *spn_t);*/

/*FEE900000000377B0500
FEC1708EA6065FC50000
FEF143802CC400FFE03F
FEE532D70200D6830A00
FEFCFFD6FFFFFFFFFFFF
F004F1A7A76024FFF0FF
F003F07F3FFFFFFFA5FF
FEEF47FFFF56FFFFFFFF
FEF5CAFFFFB723FFFFFF
FE6C4BFFFFC04416662E
FEEE79FF0D2EFFFFFFFF*/
/*******************************************************************/
bool can_init()
{
	char printBuf[128];
	Chip_IOCON_SetPinMuxing(LPC_IOCON, T2_CanPortMuxTable, sizeof(T2_CanPortMuxTable) / sizeof(PINMUX_GRP_T));

    Chip_CAN_DeInit(LPC_CAN1);
	Chip_CAN_DeInit(LPC_CAN2);

	/*enable peripheral clocks*/
	Chip_Clock_EnablePeriphClock(SYSCTL_PCLK_CAN1);
	Chip_Clock_EnablePeriphClock(SYSCTL_PCLK_CAN2);
	Chip_Clock_EnablePeriphClock(SYSCTL_PCLK_ACF);

	LPC_CAN1->MOD = LPC_CAN2->MOD = 1;    /* Reset CAN */
	LPC_CAN1->IER = LPC_CAN2->IER = 0;    /* Disable Receive Interrupt */
	LPC_CAN1->GSR = LPC_CAN2->GSR = 0;    /* Reset error counter when CANxMOD is in reset	*/

	strcpy(printBuf,"\nInitializing CAN Interface to ");

	if(settings_get_can_bitrate() == CAN_BUSSPEED_250K){
		strcat(printBuf,"250K bps ");
		Chip_CAN_SetBitRate(LPC_CAN1, 250000);
		Chip_CAN_SetBitRate(LPC_CAN2, 250000);
	}
	else {
		strcat(printBuf,"500K bps ");
		Chip_CAN_SetBitRate(LPC_CAN1, 500000);
		Chip_CAN_SetBitRate(LPC_CAN2, 500000);
	}
	if(settings_get_can_frame_type() == CAN_EXT_FRAME){
		strcat(printBuf,"-extended frame type\n");
		can_mask_ptr = can1_mask_29bit;
	}
	else{
		strcat(printBuf,"-standart frame type\n");
		can_mask_ptr = can1_mask_11bit;
	}


	LPC_CAN1->MOD = LPC_CAN2->MOD = 0x0;  /* CAN in normal operation mode */
	LPC_CAN1->IER = LPC_CAN2->IER = 0x01; /* Enable receive interrupts */

	b_can1RxDone = b_can2RxDone = FALSE;
	can_set_accf(ACCF_ON);
	NVIC_EnableIRQ(CAN_IRQn);
	Set_Timer(&can_alive_timer, CAN_ALIVE_TIMEOUT);
//	Set_Timer(&canbus_over_mobile_app_timer, J1939_MOBILE_APP_UPDATE_INTERVAL);
	PRINT_K(printBuf);
	return  TRUE;
}
/******************************************************************************
** Function name:		CAN_SetACCF
**
** Descriptions:		Set acceptance filter and SRAM associated with
**
** parameters:			ACMF mode
** Returned value:		None
**
**
******************************************************************************/
void can_set_accf(ACCF_MODE ACCFMode)
{
  switch (ACCFMode)
  {
	case ACCF_OFF:
	  LPC_CANAF->AFMR = ACCFMode;
	  LPC_CAN1->MOD = LPC_CAN2->MOD = 1;	// Reset CAN
	  LPC_CAN1->IER = LPC_CAN2->IER = 0;	// Disable Receive Interrupt
	  LPC_CAN1->GSR = LPC_CAN2->GSR = 0;	// Reset error counter when CANxMOD is in reset
	break;

	case ACCF_BYPASS:
	  LPC_CANAF->AFMR = ACCFMode;
	break;

	case ACCF_ON:
	case ACCF_FULLCAN:
	  LPC_CANAF->AFMR = ACCF_OFF;
	  CAN_SetACCF_Lookup();
	  LPC_CANAF->AFMR = ACCFMode;
	break;

	default:
	break;
  }
  return;
}
/*******************************************************/
void task_can()
{
/*	JSON_DATA_INFO_T json_info_t;
	SPN_JSON_MAPPING_T spn_json_mapping_t[] = {
			{ SPN_RPM,             json_info_t.json_data.json_type_canbus.rpm },
			{ SPN_ACC_PEDAL_POS,   json_info_t.json_data.json_type_canbus.acc_pedal_pos },
			{ SPN_VEHICLE_SPEED ,  json_info_t.json_data.json_type_canbus.speed },
			{ SPN_ENGINE_TEMP,     json_info_t.json_data.json_type_canbus.eng_temp },
			{ SPN_FUEL_LEVEL,      json_info_t.json_data.json_type_canbus.fuel_level},
			{ SPN_TOTAL_FUEL,      json_info_t.json_data.json_type_canbus.total_fuel},
			{ SPN_ODOMETER,        json_info_t.json_data.json_type_canbus.odometer },
			{ SPN_AMB_AIR_TEMP,    json_info_t.json_data.json_type_canbus.amb_temp },
			{ SPN_TRIP_FUEL,       json_info_t.json_data.json_type_canbus.trip_fuel}
	};

	TABLET_APP_EVENT_T tablet_event;*/
/*	CAN_MSG can_msg;
	char *json_buf;
	float value;
	char buffer[128];
	int i;


	if(mn_timer_expired(&canbus_over_mobile_app_timer)){
		Set_Timer(&canbus_over_mobile_app_timer, J1939_MOBILE_APP_UPDATE_INTERVAL);
		if(can_get_presence_status()){
			memset(&json_info_t, 0, sizeof(json_info_t));
			for(i =0; i < sizeof(spn_defs_array) / sizeof(spn_defs_array[0]); i++){
				if(get_can_msg_by_spn(&spn_defs_array[i], &can_msg)){
					value = get_J1939_spn_value(&spn_defs_array[i], &can_msg);
					json_buf = map_to_json_buf(spn_defs_array[i].SpnId,
						                   	   spn_json_mapping_t,
											   sizeof(spn_json_mapping_t)/sizeof(spn_json_mapping_t[0]));
					if(json_buf != NULL)
						sprintf(json_buf,"%.2f",value);
				}
			}
			tablet_event.event_type =MOBILE_APP_EVENT_XMIT_CANBUS_MESSAGE;
			strcpy(json_info_t.json_type,"canbus");
			tablet_event.json_data_t.json_type_canbus = json_info_t.json_data.json_type_canbus;
			put_tablet_app_event(&tablet_event);
		}
	}*/
}
/*******************************************************/
char *map_to_json_buf(J1939_SPN_TYPE_T spnId, SPN_JSON_MAPPING_T *spn_json_mapping_t, uint16_t mapping_size)
{
	int i;

	for(i =0; i <mapping_size; i++){
		if(spn_json_mapping_t[i].SpnId == spnId){
			return spn_json_mapping_t[i].destBuf;
		}
	}
	return NULL;
}
/*******************************************************/
float get_J1939_spn_value(const J1939_SPN_INFO_T *spn, CAN_MSG *can_msg_container)
{



	/*708EA6065FC50000*/
//	uint64_t DataB = 0x0000c55f; //can_msg_container->DataB;
//	uint64_t DataA = 0x06a68e70; //can_msg_container->DataA;


	uint64_t DataB = can_msg_container->DataB;
	uint64_t DataA = can_msg_container->DataA;


	uint64_t arranged_val = (((DataB << 0x20)) | DataA);
	int i = 0;
	uint64_t value = 0;
	uint64_t allBitsOne = 0;
	for (i = 0; i < spn->Length; i++) {
		value = value | (((arranged_val >> (8 * (spn->StartPos - 1 + i))) & 0xFF) << (8 * i));
		allBitsOne = allBitsOne | (0xFF << (8*i));
	}
	if (value == allBitsOne)
		return 0;
	return (value * spn->Res + spn->Offset);
}
/*******************************************************/
/*bool get_can_msg_by_spn_id(J1939_SPN_TYPE_T spn_id, CAN_MSG *can_msg)
{
	int i;
*/
/*	if (msg_id > 0x7FF)
		msg_id = ((msg_id >> 8) & 0xFFFF);*/ /*  MsgID >> 8 & 0xFFFF*/
/*	else
		msg_id = msg_id & 0x7FF;*/

/*	for(i =0; i < sizeof(spn_defs_array) / sizeof(spn_defs_array[0]); i++){
		if(msg_id == spn_defs_array[i].PgnId){
			memcpy(spn_t, &spn_defs_array[i], sizeof(J1939_SPN_T));
			return TRUE;
		}
	}
	return FALSE;
}*/
/*******************************************************/
bool get_can_msg_by_spn(const J1939_SPN_INFO_T *spn, CAN_MSG *can_msg_container)
{
	CAN_MSG can_msg;
	uint32_t msg_id;
	char printBuf[56];
	int i;

	 for (i = 0; i < CAN_MASK_COUNT; i++) {
		LPC_CAN1->IER = 0;
		can_msg = can1_data[i];
		LPC_CAN1->IER = 1;

		if (can_msg.MsgID > 0x7FF)
			msg_id = ((can_msg.MsgID >> 8) & 0xFFFF); /*  MsgID >> 8 & 0xFFFF*/
		else
			msg_id = can_msg.MsgID & 0x7FF;

		if (msg_id== spn->PgnId){
			memcpy(can_msg_container, &can_msg, sizeof(CAN_MSG));
			return TRUE;
		}
	}
	 return FALSE;
}
/*******************************************************/
bool can_get_presence_status()
{
	if(mn_timer_expired(&can_alive_timer))
		return FALSE;

	else
		return  TRUE;
	/*int i;
	for (i = 0; i < CAN_BUFFER_SIZE; i++) {
		if(MsgBuf_RX1[i].empty == FALSE)
			return TRUE;
	}
		return FALSE;*/
}
/*******************************************************/
bool ReadCanMsg(uint8_t canChNum)
{
	bool result;
/*	PRINT_K("Entered ReadCanMsg");*/
	switch(canChNum){
		case 0:
		result = ReadCanCh1Msg();
		break;

		case 1:
		result = ReadCanCh2Msg();
		break;
	}
	return result;
}
/*******************************************************/
void InsertCanMessage(CAN_MSG  *can_msg)
{
	uint32_t msg_id;
	int32_t msg_count;
	int32_t i, j;
	char buffer[80];

	for (j = 0; j < CAN_MASK_COUNT; j++) {
		if (can_mask_ptr[j] > 0) {
			if (can_msg->MsgID > 0x7FF)
					msg_id = ((can_msg->MsgID >> 8) & 0xFFFF); /*  MsgID >> 8 & 0xFFFF*/
				else
					msg_id =   can_msg->MsgID & 0x7FF;

			        if (can_mask_ptr[j] == msg_id){
			        	can1_data[j] = *can_msg;
			       /* 	uint32_t dataA1 = ((can_msg->DataA >> 0x18) & 0xFF);
			        	uint32_t dataA2 = ((can_msg->DataA >> 0x10) & 0xFF);
			        	uint32_t dataA3 = ((can_msg->DataA >> 0x8) & 0xFF);
			        	uint32_t dataA4 = ((can_msg->DataA & 0xFF));
			        	uint32_t dataB1 = ((can_msg->DataB >> 0x18) & 0xFF);
			        	uint32_t dataB2 = ((can_msg->DataB >> 0x10) & 0xFF);
			        	uint32_t dataB3 = ((can_msg->DataB >> 0x8) & 0xFF);
			        	uint32_t dataB4 = (can_msg->DataB & 0xFF);*/

				/*	sprintf(buffer,
							"\n%02x%02x%02x%02x%02x%02x%02x%02x\n", dataA4,
							dataA3, dataA2, dataA1, dataB4, dataB3, dataB2,dataB1);*/

				//	PRINT_K(buffer);

					/*	can1_data[j].DataA = can_msg->DataA;
						can1_data[j].DataB = can_msg->DataB;
						//can1_data[j].MsgID = MsgBuf_RX1[i].MsgID;
						can1_data[j].Frame = can_msg->Frame;*/
						break;
					}
				}
			}
}
/**************************************************************/
bool ReadCanCh1Msg()
{
	uint32_t msg_id;
/*	int32_t msg_count*/
	int32_t i, j;
	bool result = FALSE;
//	char buffer[80];

     for (i = 0; i < CAN_BUFFER_SIZE; i++) {

    	 if(1){

    		  if (MsgBuf_RX1[i].MsgID > 0x7FF)
					msg_id = ((MsgBuf_RX1[i].MsgID >> 8) & 0xFFFF); /*  MsgID >> 8 & 0xFFFF*/
			   else
					msg_id = MsgBuf_RX1[i].MsgID & 0x7FF;


					for (j = 0; j < CAN_MASK_COUNT; j++){
						if (can_mask_ptr[j] == msg_id) {
							can1_data[j].DataA = MsgBuf_RX1[i].DataA;
							can1_data[j].DataB = MsgBuf_RX1[i].DataB;
							can1_data[j].MsgID = MsgBuf_RX1[i].MsgID;
							can1_data[j].Frame = MsgBuf_RX1[i].Frame;
							result = TRUE;
						}
					}
				//	MsgBuf_RX1[i].empty = TRUE;
    	 }
     }
     return result;
}
bool ReadCanCh2Msg()
{
	uint32_t msg_id;
	int32_t msg_count;
	int32_t i, j;
//	char buffer[80];
	bool isRead = FALSE;

	if (b_can2RxDone) {
		b_can2RxDone = FALSE;
		isRead = TRUE;

	/*	sprintf(buffer, "\n%d Can Messages Received on Bus 2\n", CAN2_RxCount);
		PRINT_K(buffer);*/

		msg_count = CAN2_RxCount;

		if (msg_count > CAN_BUFFER_SIZE)
			msg_count = CAN_BUFFER_SIZE;

		for (j = 0; j < CAN_MASK_COUNT; j++) {
				if (can_mask_ptr[j] > 0) {
					for (i = msg_count - 1; i >= 0; i--) {

						if (MsgBuf_RX2[i].MsgID > 0x7FF)
							msg_id = ((MsgBuf_RX2[i].MsgID >> 8) & 0xFFFF); /*  MsgID >> 8 & 0xFFFF*/
						else
							msg_id = MsgBuf_RX2[i].MsgID & 0x7FF;
						/*	sprintf(buffer,"\nREAD CAN MSG:::::Msg ID : %x\n", (msg_id));
							PRINT_K(buffer);*/

						uint32_t dataA1 = ((MsgBuf_RX2[i].DataA >> 0x18) & 0xFF);
						uint32_t dataA2 = (((MsgBuf_RX2[i].DataA >> 0x10) & 0xFF));
						uint32_t dataA3 = (((MsgBuf_RX2[i].DataA >> 0x8) & 0xFF));
						uint32_t dataA4 = ((MsgBuf_RX2[i].DataA & 0xFF));
						uint32_t dataB1 = ((MsgBuf_RX2[i].DataB >> 0x18) & 0xFF);
						uint32_t dataB2 = (((MsgBuf_RX2[i].DataB >> 0x10) & 0xFF));
						uint32_t dataB3 = (((MsgBuf_RX2[i].DataB >> 0x8) & 0xFF));
						uint32_t dataB4 = (MsgBuf_RX2[i].DataB & 0xFF);

					/*	sprintf(buffer,
								"%02x%02x%02x%02x%02x%02x%02x%02x\n", dataA4,
								dataA3, dataA2, dataA1, dataB4, dataB3, dataB2,
								dataB1);

				    	PRINT_K(buffer);*/

						//TraceCanMsg(MsgBuf_RX1[i]);
						if (can_mask_ptr[j] == msg_id) {
							can2_data[j].DataA = MsgBuf_RX2[i].DataA;
							can2_data[j].DataB = MsgBuf_RX2[i].DataB;
							//can1_data[j].MsgID = MsgBuf_RX1[i].MsgID;
							can2_data[j].Frame = MsgBuf_RX2[i].Frame;
							break;
						}
					}
				}
			}

		/*	last_check_time = STT_Value / 100;*/

			//UARTSend(PortNum, buffer, count);
		/*	sprintf(buffer, "Time : %d\r\n", STT_Value / 1000);
			PRINT_K(buffer);*/

			/* Everything is correct, reset buffer */
			memset(MsgBuf_RX2, 0, CAN_BUFFER_SIZE);
			CAN2_RxCount = 0;
	}
	return isRead;

}
/*****************************************************************************/
void  CAN_IRQHandler(void)
{
	uint32_t CANRxStatus;

	CANRxStatus = LPC_CANCR->RXSR;
//	PRINT_K("Interrupt from CAN1");
	if (CANRxStatus & (1 << 8)){
	//	PRINT_K("Interrupt from CAN1");

		CAN_ISR_Rx1();
	}
	if (CANRxStatus & (1 << 9)){
	//	PRINT_K("Interrupt from CAN2");
		CAN2_RxCount++;
		CAN_ISR_Rx2();
	}
	if (LPC_CAN1->GSR & (1 << 6)){
	/* The error count includes both TX and RX */
		CAN1ErrCount = LPC_CAN1->GSR >> 16;
	}
	if (LPC_CAN2->GSR & (1 << 6 )){
	/* The error count includes both TX and RX */
		CAN2ErrCount = LPC_CAN2->GSR >> 16;
	}
	return;
}
/******************************************************************************
** Function name:		CAN_ISR_Rx1
**
** Descriptions:		CAN Rx1 interrupt handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void CAN_ISR_Rx1(void)
{
	//char temp[256];
	static volatile uint32_t CAN1_RxCount = 0;
	CAN_MSG can_msg;

/*	can_msg_buffer_ptr = can_get_free_msg_buffer();*/

/*	if (can_msg_buffer_ptr != NULL) {*/
	/*	can_msg_buffer_ptr->Frame = LPC_CAN1->RX.RFS;*/  // Frame
	/*	can_msg_buffer_ptr->MsgID = LPC_CAN1->RX.RID; */  // ID
	/*	can_msg_buffer_ptr->DataA = LPC_CAN1->RX.RD[0]; */  // Data A
	/*	can_msg_buffer_ptr->DataB = LPC_CAN1->RX.RD[1];  */ // Data B
	/*	can_msg_buffer_ptr->empty = FALSE;

	}*/
	MsgBuf_RX1[CAN1_RxCount].Frame = LPC_CAN1->RX.RFS;  // Frame
	MsgBuf_RX1[CAN1_RxCount].MsgID = LPC_CAN1->RX.RID;   // ID
	MsgBuf_RX1[CAN1_RxCount].DataA = LPC_CAN1->RX.RD[0];   // Data A
	MsgBuf_RX1[CAN1_RxCount].DataB = LPC_CAN1->RX.RD[1];   // Data B
    can_msg = MsgBuf_RX1[CAN1_RxCount];
    InsertCanMessage(&can_msg);
/*	sprintf(temp,"Frame:%08X\nMessage ID:%08X\nDataA: %08X\nDataB: %08X\n",
			MsgBuf_RX1[CAN1_RxCount].Frame,
			MsgBuf_RX1[CAN1_RxCount].MsgID = LPC_CAN1->RX.RID,
			MsgBuf_RX1[CAN1_RxCount].DataA = LPC_CAN1->RX.RD[0],
			MsgBuf_RX1[CAN1_RxCount].DataB = LPC_CAN1->RX.RD[1]);
	PRINT_K(temp);
	PRINT_CAN("\n\n");*/
	CAN1_RxCount++;
	if(CAN1_RxCount == CAN_BUFFER_SIZE)
		CAN1_RxCount = 0;
	Set_Timer(&can_alive_timer, CAN_ALIVE_TIMEOUT);

	LPC_CAN1->CMR = 0x01 << 2;
	return;
}
/******************************************************************************
** Function name:		CAN_ISR_Rx2
**
** Descriptions:		CAN Rx2 interrupt handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void CAN_ISR_Rx2(void)
{
	if (CAN2_RxCount < CAN_BUFFER_SIZE) {
		MsgBuf_RX2[CAN2_RxCount].Frame = LPC_CAN2->RX.RFS;    // Frame
		MsgBuf_RX2[CAN2_RxCount].MsgID = LPC_CAN2->RX.RID;  // ID
		MsgBuf_RX2[CAN2_RxCount].DataA = LPC_CAN2->RX.RD[0];   // Data A
		MsgBuf_RX2[CAN2_RxCount].DataB = LPC_CAN2->RX.RD[1];   // Data B
	}
	b_can2RxDone = TRUE;
	LPC_CAN2->CMR = 0x01 << 2;
	return;
}
/******************************************************************************
** Function name:		CAN_SetACCF_Lookup
******************************************************************************/
void CAN_SetACCF_Lookup(void)
{
  uint32_t address = 0;
  uint32_t i;
  uint32_t ID_high, ID_low;
  //char printBuf[256];

	LPC_CANAF->ENDADDR[CANAF_RAM_SFF_SEC] = address;
  	/*  LPC_CANAF->SFF_sa = address;*/
  	  for (i = 0; i < ACCF_IDEN_NUM; i += 2 ){
  		  ID_low = (i << 29) | (EXP_STD_ID << 16);
  		  ID_high = ((i+1) << 13) | (EXP_STD_ID << 0);
  		  *((volatile uint32_t *)(LPC_CANAF_RAM_BASE + address)) = ID_low | ID_high;
  		/*  sprintf(printBuf,"\nCANAF_RAM_SFF_SEC : %X, %X\n", LPC_CANAF_RAM_BASE + address, ID_low | ID_high);
  		  PRINT_K(printBuf);*/
  		  address += 4;
  	  }

  	  /* Set group standard Frame */
  	LPC_CANAF->ENDADDR[CANAF_RAM_SFF_GRP_SEC] = address;
  	/*  LPC_CANAF->SFF_GRP_sa = address;*/
  	  for ( i = 0; i < ACCF_IDEN_NUM; i += 2 ){
  		  ID_low = (i << 29) | (GRP_STD_ID << 16);
  		  ID_high = ((i+1) << 13) | (GRP_STD_ID << 0);
  		  *((volatile uint32_t *)(LPC_CANAF_RAM_BASE + address)) = ID_low | ID_high;
  		  /* sprintf(printBuf,"\nCANAF_RAM_SFF_GRP_SEC: %X, %X\n", LPC_CANAF_RAM_BASE + address, ID_low | ID_high);
  		   PRINT_K(printBuf);*/
  		  address += 4;
  	  }

  	  /* Set explicit extended Frame */
  	LPC_CANAF->ENDADDR[CANAF_RAM_EFF_SEC] = address;
  /*	  LPC_CANAF->EFF_sa = address;*/
  	  for (i = 0; i < ACCF_IDEN_NUM; i++){
  		  ID_low = (i << 29) | (EXP_EXT_ID << 0);
  		  *((volatile uint32_t *)(LPC_CANAF_RAM_BASE + address)) = ID_low;
  		/* sprintf(printBuf,"\nCANAF_RAM_EFF_SEC: %X, %X\n", LPC_CANAF_RAM_BASE + address, ID_low | ID_high);
  		 PRINT_K(printBuf);*/
  		  address += 4;
  	  }

  	  /* Set group extended Frame */
      LPC_CANAF->ENDADDR[CANAF_RAM_EFF_GRP_SEC] = address;
  	 /* LPC_CANAF->EFF_GRP_sa = address;*/
  	  for (i = 0; i < ACCF_IDEN_NUM; i++){
  		  ID_low = (i << 29) | (GRP_EXT_ID << 0);
  		  *((volatile uint32_t *)(LPC_CANAF_RAM_BASE + address)) = ID_low;
  		/* sprintf(printBuf,"\nCANAF_RAM_EFF_GRP_SEC: %X, %X\n", LPC_CANAF_RAM_BASE + address, ID_low | ID_high);
  		 	 PRINT_K(printBuf);*/
  		  address += 4;
  	  }

  	  /* Set End of Table */
     LPC_CANAF->ENDADDR[CANAF_RAM_SECTION_NUM] = address;
  	 /* LPC_CANAF->ENDofTable = address;*/

  return;
}

/******************************************************************************/
uint16_t Get_CanMessageString(char *buffer, uint16_t length)
{
	CAN_MSG can_msg;
//	uint32_t msg_id;
	int i,j =0;
	char temp[256];
  //  bool found;
    uint8_t table_size;

    memset(buffer,0, length);


    if(can_mask_ptr == can1_mask_29bit)
    	table_size = sizeof(can1_mask_29bit) / sizeof(can1_mask_29bit[0]);
    else if(can_mask_ptr == can1_mask_11bit)
    	table_size = sizeof(can1_mask_11bit) / sizeof(can1_mask_11bit[0]);
    else
    	NVIC_SystemReset();   /* fatal error */
    //	table_size =0;

	for (j = 0; j < table_size; j++){
		LPC_CAN1->IER = 0;
		can_msg = can1_data[j];
		LPC_CAN1->IER = 1;
		uint32_t dataA1 = ((can_msg.DataA >> 0x18) & 0xFF);
	    uint32_t dataA2 = (((can_msg.DataA >> 0x10) & 0xFF));
		uint32_t dataA3 = (((can_msg.DataA >> 0x8) & 0xFF));
		uint32_t dataA4 = ((can_msg.DataA & 0xFF));
		uint32_t dataB1 = ((can_msg.DataB >> 0x18) & 0xFF);
		uint32_t dataB2 = (((can_msg.DataB >> 0x10) & 0xFF));
		uint32_t dataB3 = (((can_msg.DataB >> 0x8) & 0xFF));
		uint32_t dataB4 = (can_msg.DataB & 0xFF);
		sprintf(temp,"%04X%02X%02X%02X%02X%02X%02X%02X%02X",can_mask_ptr[j], dataA4,
												dataA3, dataA2, dataA1, dataB4, dataB3, dataB2,dataB1);

	    strcat(buffer, temp);

	//sprintf(temp,"%04X%08X%08X", can_mask_ptr[j], can_msg.DataA, can_msg.DataB );

	}
	return strlen(buffer);
}
/*CAN_MSG *can_get_free_msg_buffer()
{
	int i;

	for(i =0; i < CAN_BUFFER_SIZE; i++){
		if(MsgBuf_RX1[i].empty)
			return &MsgBuf_RX1[i];
	}
	return NULL;

}*/
