#include "chip.h"
#include "board.h"
#include "timer.h"
#include "settings.h"
#include "MMA8652.h"
#include <string.h>
#include "trace.h"
#include "i2c.h"

#define I2C0_SDA_PORT_NUM            0
#define I2C0_SCL_PORT_NUM            0

#define I2C0_SDA_PIN_NUM             27
#define I2C0_SCL_PIN_NUM             28

typedef enum{
	I2C_IDLE,
	I2C_STARTED,
	I2C_RESTARTED,
	I2C_REPEATED_START,
	DATA_ACK,
	DATA_NACK
}I2C_STATE;

volatile I2C_STATE I2C0_MasterState = I2C_IDLE;
volatile uint32_t I2C0_Cmd;
volatile uint32_t I2C0_Mode;

volatile uint8_t  I2C0_MasterBuffer[I2C0_BUFSIZE];
volatile uint32_t I2C0_Count = 0;
volatile uint32_t I2C0_ReadLength;
volatile uint32_t I2C0_WriteLength;

volatile uint32_t RdIndex_0 = 0;
volatile uint32_t WrIndex_0 = 0;

typedef struct I2C_IFACE{
	LPC_I2C_T *ip;           /* base address of i2c interface  */
	I2C_ID_T iface_id;
	uint8_t *txBuff;
	uint8_t *rxBuff;
	uint16_t rxLength;
	uint16_t txLength;
	I2C_STATE state;
}I2C_IFACE_T;

I2C_IFACE_T i2c_iface0;
I2C_IFACE_T i2c_iface1;

static const PINMUX_GRP_T T2_i2cPortMuxTable[] = {
	{I2C0_SDA_PORT_NUM,  I2C0_SDA_PIN_NUM,   IOCON_MODE_INACT | IOCON_FUNC1},	/* I2C_SDA*/
	{I2C0_SCL_PORT_NUM,  I2C0_SCL_PIN_NUM,   IOCON_MODE_INACT | IOCON_FUNC1},	/* I2C_SCL */
};

void Start_I2C_Transfer();
void i2c_init()
{
	uint8_t id;
	char printBuf[64];
/*	LPC_SYSCTL->PCONP |= (1 << 7);
	LPC_IOCON->PINSEL[1] &= ~((0x03<<22)|(0x03<<24));
	LPC_IOCON->PINSEL[1]|= ((0x01<<22)|(0x01<<24));*/
/*	Chip_IOCON_SetPinMuxing(LPC_IOCON, T2_i2cPortMuxTable, sizeof(T2_i2cPortMuxTable) / sizeof(PINMUX_GRP_T));
	Chip_IOCON_SetI2CPad(LPC_IOCON, I2CPADCFG_STD_MODE);*/
	/*--- Clear flags ---*/
/*	LPC_I2C0->CONCLR = I2C_I2CONCLR_AAC | I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC| I2C_I2CONCLR_I2ENC;*/
	/*--- Reset registers ---*/
/*	LPC_IOCON->I2CPADCFG &= ~((0x1<<0)|(0x1<<2));
	LPC_I2C0->SCLL   = 0x00000080;
	LPC_I2C0->SCLH   = 0x00000080;*/
	/* Install interrupt handler */
/*	NVIC_EnableIRQ(I2C0_IRQn);
	LPC_I2C0->CONSET =  I2C_CON_I2EN;
	return 1;*/

	Chip_IOCON_SetPinMuxing(LPC_IOCON, T2_i2cPortMuxTable, sizeof(T2_i2cPortMuxTable) / sizeof(PINMUX_GRP_T));
	Chip_IOCON_SetI2CPad(LPC_IOCON, I2CPADCFG_STD_MODE);
   /* Initialize I2C */
	Chip_I2C_Init(I2C0);
	Chip_I2C_SetClockRate(I2C0, I2C_SPEED_400KHZ);
	NVIC_EnableIRQ(I2C0_IRQn);

	id = mma865x_get_id();
	sprintf(printBuf,"\nAccelorometer ID: %X", id);
	PRINT_K(printBuf);
	PRINT_K("\nI2C Interface Initialized\n");
	/* Set default mode to interrupt */
//	Chip_I2C_SetMasterEventHandler(I2C0, Chip_I2C_EventHandlerPolling);
//	if(Chip_I2C_SetMasterEventHandler(I2C0, Chip_I2C_EventHandler))
//	{
//		PRINT_K("Event handler assigned\n");


//	}
}

/*void I2C0_IsrHandler(void)
{
	PRINT_K("In Interrupt\n");
	Chip_I2C_MasterStateHandler(I2C0);

}*/
uint8_t i2c_read(I2C_ID_T i2c_id, uint8_t i2c_addr, uint8_t register_addr,uint8_t bytes_to_read, uint8_t *response_buffer)
{
		/* clear buffer */
	if (i2c_id == 0){
		memset(I2C0_MasterBuffer, 0, sizeof(I2C0_MasterBuffer));

		I2C0_WriteLength = 2;
		I2C0_ReadLength = bytes_to_read;
		I2C0_MasterBuffer[0] = 0x3A;
		I2C0_MasterBuffer[1] = register_addr;
		I2C0_MasterBuffer[2] = 0x3B;
		I2CEngine(i2c_id);
		I2CStop(i2c_id);
		memcpy(response_buffer, &I2C0_MasterBuffer[3], bytes_to_read);
		return bytes_to_read;
	}
}
/********************************************************************************************/
uint8_t i2c_write(I2C_ID_T i2c_id, uint8_t i2c_addr, uint8_t register_address, uint8_t *data, uint8_t bytes_to_write)
{
	/* clear buffer */
	if (i2c_id == 0){

		memset(I2C0_MasterBuffer, 0, sizeof(I2C0_MasterBuffer));

		I2C0_WriteLength = bytes_to_write + 2;
		I2C0_ReadLength = 0;
		I2C0_MasterBuffer[0] = 0x3A;
		I2C0_MasterBuffer[1] = register_address;
		memcpy(&I2C0_MasterBuffer[2], data, bytes_to_write);
	//	I2C0_MasterBuffer[2] = data;
		if (I2CEngine(i2c_id))
			return 1;
		else
			return 0;
	}
}
void I2C0_IRQHandler(void)
{
	uint8_t StatValue;
	/* this handler deals with master read and master write only */
	StatValue = LPC_I2C0->STAT;
	switch (StatValue) {
		case 0x08: /* A Start condition is issued. */
			LPC_I2C0->DAT = I2C0_MasterBuffer[WrIndex_0++];
			LPC_I2C0->CONCLR = (I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC);
			I2C0_MasterState = I2C_STARTED;
			break;
		case 0x10: /* A repeated started is issued */
			if (!I2C0_Cmd) {
				LPC_I2C0->DAT = I2C0_MasterBuffer[WrIndex_0++];
			}
			LPC_I2C0->CONCLR = (I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC);
			I2C0_MasterState = I2C_RESTARTED;
			break;
		case 0x18: /* Regardless, it's a ACK */
			if (I2C0_MasterState == I2C_STARTED) {
				LPC_I2C0->DAT = I2C0_MasterBuffer[WrIndex_0++];
				I2C0_MasterState = DATA_ACK;
			}
			LPC_I2C0->CONCLR = I2C_I2CONCLR_SIC;
			break;
		case 0x28: /* Data byte has been transmitted, regardless ACK or NACK */
			 if ( WrIndex_0 < I2C0_WriteLength )
				{
				  LPC_I2C0->DAT = I2C0_MasterBuffer[WrIndex_0++]; /* this should be the last one */
				}
				else {
				  if (I2C0_ReadLength != 0)
						LPC_I2C0->CONSET = I2C_I2CONSET_STA;   /* Set Repeated-start flag */
				  else{
						LPC_I2C0->CONSET = I2C_I2CONSET_STO;      /* Set Stop flag */
						I2C0_MasterState = I2C_IDLE;
				  }
				}
				LPC_I2C0->CONCLR = I2C_I2CONCLR_SIC;
				break;
		case 0x30:
			if (WrIndex_0 != I2C0_WriteLength) {
				LPC_I2C0->DAT = I2C0_MasterBuffer[1 + WrIndex_0]; /* this should be the last one */
				WrIndex_0++;
				if (WrIndex_0 != I2C0_WriteLength) {
					I2C0_MasterState = DATA_ACK;
				} else {
					I2C0_MasterState = DATA_NACK;
					if (I2C0_ReadLength != 0) {
						LPC_I2C0->CONSET = I2C_I2CONSET_STA; /* Set Repeated-start flag */
						I2C0_MasterState = I2C_REPEATED_START;
					}
				}
			} else {
				if (I2C0_ReadLength != 0) {
					LPC_I2C0->CONSET = I2C_I2CONSET_STA; /* Set Repeated-start flag */
					I2C0_MasterState = I2C_REPEATED_START;
				} else {
					I2C0_MasterState = DATA_NACK;
					LPC_I2C0->CONSET = I2C_I2CONSET_STO; /* Set Stop flag */
				}
			}
			LPC_I2C0->CONCLR = I2C_I2CONCLR_SIC;
			break;
		case 0x40: /* Master Receive, SLA_R has been sent */
			LPC_I2C0->CONSET = I2C_I2CONSET_AA; /* assert ACK after data is received */
			LPC_I2C0->CONCLR = I2C_I2CONCLR_SIC;
			break;
		case 0x50: /* Data byte has been received, regardless following ACK or NACK */
		case 0x58:
			I2C0_MasterBuffer[3 + RdIndex_0] = LPC_I2C0->DAT;
			RdIndex_0++;
			if (RdIndex_0 != I2C0_ReadLength) {
				I2C0_MasterState = DATA_ACK;
			} else {
				RdIndex_0 = 0;
				I2C0_MasterState = DATA_NACK;
			}
			LPC_I2C0->CONSET = I2C_I2CONSET_AA; /* assert ACK after data is received */
			LPC_I2C0->CONCLR = I2C_I2CONCLR_SIC;
			break;
		case 0x20: /* regardless, it's a NACK */
		case 0x48:
			LPC_I2C0->CONCLR = I2C_I2CONCLR_SIC;
			I2C0_MasterState = DATA_NACK;
			break;
		case 0x38: /* Arbitration lost, in this example, we don't
		 deal with multiple master situation */
		default:
			LPC_I2C0->CONCLR = I2C_I2CONCLR_SIC;
			break;
	}
}
uint32_t I2CEngine(I2C_ID_T i2c_id)
{
	int timeout = 0;

	if (i2c_id == I2C0){
			I2C0_MasterState = I2C_IDLE;
			RdIndex_0 = 0;
			WrIndex_0 = 0;
			if (I2CStart(i2c_id) != 1) {
				I2CStop(i2c_id);
				return (0);
			}

			while (1) {
				if (I2C0_MasterState == DATA_NACK) {
					I2CStop(i2c_id);
					break;
				}
				if (timeout >= 0xFFFF)
					break;
				timeout++;
			}
			return (1);
	}
	return 0;
}
/*****************************************************************************
 ** Function name:		I2CStart
 **
 ** Descriptions:		Create I2C start condition, a timeout
 **				value is set if the I2C never gets started,
 **				and timed out. It's a fatal error.
 **
 ** parameters:			None
 ** Returned value:		true or false, return false if timed out
 **
 *****************************************************************************/
uint32_t I2CStart(I2C_ID_T i2c_id)
{
	TIMER_INFO_T i2c_timer;

	if (i2c_id == I2C0){
			/*--- Issue a start condition ---*/
		    Start_I2C_Transfer();
		    Set_Timer(&i2c_timer, I2C_BUS_TIMEOUT_VALUE);
			/*--- Wait until START transmitted ---*/
			while (!mn_timer_expired(&i2c_timer)) {
				if (I2C0_MasterState == I2C_STARTED) {
				/*	PRINT_K("Start condition OK\n");*/
					return 1;
				}
			}
			return 0;
	}
	return 0;
}
/*****************************************************************************/
void Start_I2C_Transfer()
{
	LPC_I2C0->CONSET =  I2C_CON_I2EN | I2C_I2CONSET_STA;
}
/*****************************************************************************
 ** Function name:		I2CStop
 **
 ** Descriptions:		Set the I2C stop condition, if the routine
 **				never exit, it's a fatal bus error.
 **
 ** parameters:			None
 ** Returned value:		true or never return
 **
 *****************************************************************************/
uint32_t I2CStop(I2C_ID_T i2c_id)
{
	TIMER_INFO_T i2c_timer;
	if (i2c_id == I2C0){
		LPC_I2C0->CONSET = I2C_I2CONSET_STO; /* Set Stop flag */
			LPC_I2C0->CONCLR = I2C_I2CONCLR_SIC; /* Clear SI flag */
			 Set_Timer(&i2c_timer, I2C_BUS_TIMEOUT_VALUE);
			/*--- Wait for STOP detected ---*/
			while (LPC_I2C0->CONSET & I2C_I2CONSET_STO){
				if(mn_timer_expired(&i2c_timer))
					return 0;
			}
			return 1;
		}
		return 0;
}
/*********************************************************************/
uint8_t Get_I2CState(I2C_ID_T iface_id)
{
	  if(iface_id ==  I2C0){
		  return (uint8_t)(LPC_I2C0->STAT);
	  }
	  else
		  return (uint8_t)(LPC_I2C1->STAT);
}
