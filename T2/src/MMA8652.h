/*
 * MMA8652.h
 *
 *  Created on: 16 Mar 2016
 *      Author: admin
 */

#ifndef MMA8652_H_
#define MMA8652_H_

#define ACCMETER_I2C_CH        I2C0

#define MMA8652_I2C_ADDR       0x1D

#define STATUS           0x00
#define F_STATUS         0x00
#define OUT_X_MSB        0x01
#define OUT_X_LSB        0x02
#define OUT_Y_MSB        0x03
#define OUT_Y_LSB        0x04
#define OUT_Z_MSB        0x05
#define OUT_Z_LSB        0x06
#define F_SETUP          0x09
#define TRIG_CFG         0x0A
#define SYSMOD           0x0B
#define INT_SOURCE       0x0C
#define WHO_AM_I         0x0D
#define XYZ_DATA_CFG     0x0E
#define HP_FILTER_CUTOFF 0x0F
#define PL_STATUS        0x10
#define PL_CFG           0x11
#define PL_COUNT         0x12
#define PL_BF_ZCOMP      0x13
#define P_L_THS_REG      0x14
#define FF_MT_CFG        0x15
#define FF_MT_SRC        0x16
#define FF_MT_THS        0x17
#define FF_MT_COUNT      0x18
#define TRANSIENT_CFG    0x1D
#define TRANSIENT_SRC    0x1E
#define TRANSIENT_THS    0x1F
#define TRANSIENT_COUNT  0x20
#define PULSE_CFG        0x21
#define PULSE_SRC        0x22
#define PULSE_THSX       0x23
#define PULSE_THSY       0x24
#define PULSE_THSZ       0x25
#define PULSE_TMLT       0x26
#define PULSE_LTCY       0x27
#define PULSE_WIND       0x28
#define ASLP_COUNT       0x29
#define CTRL_REG1        0x2A
#define CTRL_REG2        0x2B
#define CTRL_REG3        0x2C
#define CTRL_REG4        0x2D
#define CTRL_REG5        0x2E
#define OFF_X            0x2F
#define OFF_Y            0x30
#define OFF_Z            0x31

/*----------------------------------*/
#define   STATUS_XDR_FLAG         (1 << 0)
#define   STATUS_YDR_FLAG         (1 << 1)
#define   STATUS_ZDR_FLAG         (1 << 2)
#define   STATUS_ZYXDR_FLAG       (1 << 3)
#define   STATUS_XOW_FLAG         (1 << 4)
#define   STATUS_YOW_FLAG         (1 << 5)
#define   STATUS_ZOW_FLAG         (1 << 6)
#define   STATUS_ZYXOW_FLAG       (1 << 7)
/*----------------------------------*/
#define   F_SETUP_F_MODE(n)       (((n & 0x03) << 6))
#define   F_SETUP_F_WMRK(n)       (((n & 0x3F) << 0))
/*----------------------------------*/
/* CTRL_REG1 bit definitions */
#define   CTRL_REG1_ACTIVE        (1 << 0)
#define   CTRL_REG1_FREAD         (1 << 1)
#define   CTRL_REG1_LNOISE        (1 << 2)
#define   CTRL_REG1_DATA_RATE(n)  (((n & 0x7) << 3))
#define   CTRL_REG1_ASLP_RATE(n)  (((n & 0x2) << 2))
/*----------------------------------*/
typedef struct G_VALUE_SAMPLES{
	float xAccel;
	float yAccel;
	float zAccel;
}G_VALUES_T;

typedef enum{
	FULL_SCALE_2G,
	FULL_SCALE_4G,
	FULL_SCALE_8G
}MMA865x_SCALE_T;

#define XYZ_DATA_CFG_FS(n)      (((n & 0x3) << 0))

#define SENSITIVITY_2G		    1024
#define SENSITIVITY_4G		    512
#define SENSITIVITY_8G		    256


/*----------------------------------*/
/*typedef struct MMA8652_STATUS{
	uint8_t xdr:1;
	uint8_t ydr:1;
	uint8_t zdr:1;
	uint8_t zyxdr:1;
	uint8_t xow:1;
	uint8_t yow:1;
	uint8_t zow:1;
	uint8_t zyxow:1;
}MMA8652_STATUS_T;*/

/*typedef struct MMA8652_FIFO_SETUP{
	uint8_t f_mode:2;
	uint8_t f_wmrk:6;
}MMA8652_FIFO_SETUP_T;*/

/*typedef struct MMA865x_CTRL_REG1_SETUP{
	uint8_t active:1;
	uint8_t fread:1;
	uint8_t lnoise:1;
	uint8_t dr:3;
	uint8_t aslp_rate:2;
}MMA865x_CTRL_REG1_SETUP_T;*/

typedef struct MMA8652_ACCEL_VALUES{
	float xAccel;
	float yAccel;
	float zAccel;
}MMA8652_ACCEL_VALUES_T;


/* function prototypes */
void    mma865x_init();
uint8_t mma865x_get_id();
bool mma865x_read_accel_values(G_VALUES_T *mma8652_accel_values);
uint8_t mma865x_write(uint8_t reg_address, uint8_t *data, uint8_t bytes_to_write);
uint8_t mma865x_read(uint8_t reg_address,  uint8_t *out_buffer, uint8_t bytes_to_read);
bool mma865x_set_ctrl_reg1(uint8_t value);
bool mma865x_set_dynamic_range(MMA865x_SCALE_T range);
void mma865x_active();
void mma865x_standby();
#endif /* MMA8652_H_ */
