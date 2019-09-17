/*
 * can.h
 *
 *  Created on: 15 Mar 2016
 *      Author: admin
 */

#ifndef CAN_H_
#define CAN_H_

#define  NUMBER_OF_CAN_PORTS                      2         /* Number of CAN port on the chip */
#define  J1939_MOBILE_APP_UPDATE_INTERVAL        100
/* CAN test modülü tarafından gönderilen PGN'ler
 * 0xFEEE
 * 0xFEF2
 * 0xFEF1
 */
typedef enum J1939_PGN_TYPE_29BIT{
	PGN_ENGINE_SPEED   = 0xF004,    /* rpm*/
	PGN_VEHICLE_SPEED  = 0xFEF1,    /* speed*/
	PGN_ACC_PEDAL_POS =  0xF003,
	PGN_FUEL_LEVEL     = 0xFEFC,
	PGN_TOTAL_FUEL     = 0xFEE9,    /* bytes 1-4: trip fuel bytes 5-8: total fuel */
	PGN_ODOMETER       = 0xFEC1,    /* Total vehicle distance */
	PGN_AMB_CONDITIONS = 0xFEF5,
	PGN_ENGINE_TEMP     =0xFEEE
}J1939_PGN_TYPE_T_29BIT;

typedef enum J1939_SPN_TYPE{
	SPN_RPM = 190,
	SPN_ACC_PEDAL_POS  =  91,
	SPN_FUEL_LEVEL     =  96,
	SPN_VEHICLE_SPEED  =  84,
	SPN_ENGINE_TEMP    = 110,
	SPN_TOTAL_FUEL     = 250,
	SPN_ODOMETER       = 917,
	SPN_AMB_AIR_TEMP   = 171,
	SPN_TRIP_FUEL      = 182
}J1939_SPN_TYPE_T;

typedef struct J1939_SPN_INFO{
	uint16_t val;
	J1939_SPN_TYPE_T SpnId;
	J1939_PGN_TYPE_T_29BIT PgnId;
	uint32_t Length;
	uint32_t StartPos;
	float Offset;      // Converts to physical ID.
	float Res;         // Converts to physical ID.
} J1939_SPN_INFO_T;

typedef struct SPN_JSON_MAPPING{
	J1939_SPN_TYPE_T SpnId;
	char *destBuf;
}SPN_JSON_MAPPING_T;

//#define IS_J1708 1
#define IS_STD_CAN                  1
//#define IS_AXOR 1

#define CAN_WAKEUP					0
#define ACCEPTANCE_FILTER_ENABLED	1
#define ACCF_IDEN_NUM		    	4


#define CAN1_BIT_RATE               250000
#define CAN2_BIT_RATE               250000

#define CAN_MASK_COUNT              20
#define CAN_BUFFER_SIZE             100
#define CAN_ALIVE_TIMEOUT           (3* SECOND)

/* Acceptance filter mode in AFMR register */
typedef enum ACCF_MODE_T{
	ACCF_ON,
    ACCF_OFF,
	ACCF_BYPASS,
	ACCF_FULLCAN = 0x04
}ACCF_MODE;

typedef enum CAN_BITRATE_T {
	 CAN_BUSSPEED_250K = 250,
	 CAN_BUSSPEED_500K = 500
}CAN_BITRATE;

typedef enum CAN_FRAME_T{
	CAN_STD_FRAME = 1,                 /* 11-bit can frame */
	CAN_EXT_FRAME = 2                  /* 29-bit can frame */
}CAN_FRAME_TYPE;

/* Identifiers for FULLCAN, EXP STD, GRP STD, EXP EXT, GRP EXT */
#define FULLCAN_ID				0x100
#define EXP_STD_ID				0x100
#define GRP_STD_ID				0x200
#define EXP_EXT_ID				0x100000
#define GRP_EXT_ID				0x200000

/* Type definition to hold a CAN message */
typedef struct
{
	uint32_t Frame; // Bits 16..19: DLC - Data Length Counter
					// Bit 30: Set if this is a RTR message
					// Bit 31: Set if this is a 29-bit ID message
	uint32_t MsgID;	// CAN Message ID (11-bit or 29-bit)
	uint32_t DataA;	// CAN Message Data Bytes 0-3
	uint32_t DataB;	// CAN Message Data Bytes 4-7
	//bool empty;
} CAN_MSG;
void task_can();
void can_set_accf(ACCF_MODE ACCFMode);
bool can_init();
bool can_get_presence_status();
void CAN_ISR_Rx2(void);
void CAN_ISR_Rx1(void);
void CAN_SetACCF_Lookup(void);
CAN_MSG * can_get_free_msg_buffer();
bool ReadCanMsg(uint8_t canChNum);
bool ReadCanCh1Msg();
bool ReadCanCh2Msg();
void InsertCanMessage(CAN_MSG *);
void TraceCanDataOverUart();
uint16_t Get_CanMessageString(char * buffer,uint16_t length);
#endif /* CAN_H_ */
