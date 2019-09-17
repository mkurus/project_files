#include "board.h"
#include "chip.h"
#include <i2c_17xx_40xx.h>
#include <lpc_types.h>
#include "timer.h"
#include <stdbool.h>
#include <stdint.h>
#include "i2c.h"
#include "trace.h"
#include "MMA8652.h"

uint8_t rxBuff[4];
uint8_t txBuff[4] = {WHO_AM_I, 0x3A};
uint8_t mma865x_get_id()
{
	//I2C_XFER_T xfer;
	uint8_t id;
	i2c_read(ACCMETER_I2C_CH, MMA8652_I2C_ADDR, WHO_AM_I, 1, &id);
	return id;
/*	xfer.slaveAddr = MMA8652_I2C_ADDR;
	xfer.rxBuff = rxBuff;
	xfer.txBuff = txBuff;
	xfer.txSz = 3;
	xfer.rxSz = 2;*/
	//Chip_I2C_MasterCmdRead(ACCMETER_I2C_CH, MMA8652_I2C_ADDR, WHO_AM_I, rxBuff, 1);

/*	Chip_I2C_MasterTransfer(ACCMETER_I2C_CH, &xfer);
	PRINT_K("Reading MMA865222222222222\n");
	Delay(50);
	while(xfer.status != I2C_STATUS_DONE){
	}
	temp = Chip_I2C_MasterRead(ACCMETER_I2C_CH , MMA8652_I2C_ADDR, rxBuff, 1)	if( temp > 0){
		sprintf(printBuf, "Read data from slave 0x%02X: %X\n", MMA8652_I2C_ADDR, readVal);
		PRINT_K(printBuf);
	}*/

}
G_VALUES_T g_value_test_data[]={ {0.1111 , 0.2222, 0.3333},
								 {0.2222 , 0.3333, 0.4444},
								 {0.3333 , 0.1254, 0.2145},
								 {0.5678 , 0.2378, 0.4589},
								 {0.2908 , -1.2356, 0.5678},
								 {-0.1111 , -0.2222, -0.3333},
								 {0.1111 , 0.2222, 0.3333},
								 {0.2367 , 0.8907, 0.4567},
								 {0.3456 , 0.3678, 0.1367},
								 {0.1256 , 0.9087, 0.5643},
								 {0.7854 , 0.3478, 0.4456},
                                };
uint8_t i =0;
/*****************************************************************/
bool mma865x_read_accel_values2(G_VALUES_T *mma8652_accel_values)
{
	int8_t AccelData[6];
	uint8_t status;

	i2c_read(ACCMETER_I2C_CH, MMA8652_I2C_ADDR, F_STATUS, 1, &status);

	if(status | STATUS_ZYXDR_FLAG){
		i2c_read(ACCMETER_I2C_CH, MMA8652_I2C_ADDR, OUT_X_MSB, 6, (uint8_t *)AccelData);

		mma8652_accel_values->xAccel =  g_value_test_data[i].xAccel;
		mma8652_accel_values->yAccel =  g_value_test_data[i].yAccel;
		mma8652_accel_values->zAccel =  g_value_test_data[i].zAccel;
		i++;
		if( i == 11)
			i=0;
		return true;
	}
	else
		return false;
}
/*********************************************/
bool mma865x_read_accel_values(G_VALUES_T *mma8652_accel_values)
{
	int8_t AccelData[6];
//	char printBuf[64];
	uint8_t status;

	mma865x_read(F_STATUS, &status, 1);

	if(status & STATUS_ZYXDR_FLAG){
		i2c_read(ACCMETER_I2C_CH, MMA8652_I2C_ADDR, OUT_X_MSB, 6, (uint8_t *)AccelData);
	/*	sprintf(printBuf,"\nNew g data ready %.2X\n", status);
		PRINT_K(printBuf);*/

		mma8652_accel_values->xAccel =  (float)((AccelData[0] << 4) + (AccelData[1] >> 4)) / SENSITIVITY_8G;
		mma8652_accel_values->yAccel =  (float)((AccelData[2] << 4) + (AccelData[3] >> 4)) / SENSITIVITY_8G;
		mma8652_accel_values->zAccel =  (float)((AccelData[4] << 4) + (AccelData[5] >> 4)) / SENSITIVITY_8G;
		return true;
	}
	else
		return false;
}
/*****************************************************************/
uint8_t mma865x_read(uint8_t reg_address, uint8_t *out_buffer, uint8_t bytes_to_read)
{
	i2c_read(ACCMETER_I2C_CH, MMA8652_I2C_ADDR, reg_address, bytes_to_read, out_buffer);
	return bytes_to_read;
}
/*****************************************************************/
uint8_t mma865x_write(uint8_t reg_address, uint8_t *data, uint8_t bytes_to_write)
{
	i2c_write(ACCMETER_I2C_CH, MMA8652_I2C_ADDR, reg_address, data, bytes_to_write);
	return bytes_to_write;
}
/*****************************************************************/
void mma865x_active()
{
	uint8_t temp;

	mma865x_read(CTRL_REG1, &temp, 1);
	temp |= CTRL_REG1_ACTIVE;
	mma865x_write(CTRL_REG1, &temp, 1);
}
/*****************************************************************/
void mma865x_standby()
{
	uint8_t temp;

	mma865x_read(CTRL_REG1, &temp, 1);
	temp &= ~CTRL_REG1_ACTIVE;
	mma865x_write(CTRL_REG1, &temp, 1);
}
/*****************************************************************/
bool mma865x_set_dynamic_range(MMA865x_SCALE_T range)
{
	uint8_t temp;
	bool success;
	//char printBuf[64];

	mma865x_standby();

	mma865x_read(XYZ_DATA_CFG, &temp, 1);
	temp |= XYZ_DATA_CFG_FS(range);
	mma865x_write(XYZ_DATA_CFG, &temp, 1);
	Delay(10, NULL);
	mma865x_read(XYZ_DATA_CFG, &temp, 1);
	if((temp & XYZ_DATA_CFG_FS(range)) == XYZ_DATA_CFG_FS(range))
		success = TRUE;
	else
		success = FALSE;
	/*sprintf(printBuf,"XYZ_DATA_CFG :%X\n", temp);
	PRINT_K(printBuf);*/
	mma865x_active();
	return success;
}
/****************************************************************/
bool mma865x_set_ctrl_reg1(uint8_t value)
{
	uint8_t temp;
	bool success;
//	char printBuf[64];

	mma865x_standby();

	mma865x_write(CTRL_REG1, &value, 1);
	Delay(10, NULL);
	mma865x_read(CTRL_REG1, &temp, 1);

	if(temp  == value)
		success = TRUE;
	else
		success = FALSE;

/*	sprintf(printBuf,"CTRL_REG1 :%X\n", temp);
	PRINT_K(printBuf);*/

	mma865x_active();
	return success;
}
/*****************************************************************/
void mma865x_init()
{
	 mma865x_active();
	 if(mma865x_set_dynamic_range(FULL_SCALE_8G))
		 PRINT_K("\nMMA8652 scale set\n");
	 if(mma865x_set_ctrl_reg1(0x19))     /* Set ODR to 100 Hz */
		 PRINT_K("\nMMA8652 ODR set\n");
}
