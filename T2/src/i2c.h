/*
 * i2c.h
 *
 *  Created on: 16 Mar 2016
 *      Author: admin
 */

#ifndef I2C_H_
#define I2C_H_

#define  I2C_SPEED_400KHZ         400000

#define  I2C0_BUFSIZE             128

#define  I2C_BUS_TIMEOUT_VALUE    10

#define  I2C_MAX_TIMEOUT          0x00FFFFFF
void i2c_init();
void I2C0_IsrHandler(void);
uint8_t Get_I2CState(I2C_ID_T iface_id);
uint32_t I2CEngine(I2C_ID_T i2c_id);
uint32_t I2CStop(I2C_ID_T i2c_id);
uint32_t I2CStart(I2C_ID_T i2c_id);
uint8_t i2c_read(I2C_ID_T i2c_id, uint8_t i2c_addr, uint8_t register_addr, uint8_t bytes_to_read, uint8_t *response_buffer);
uint8_t i2c_write(I2C_ID_T i2c_id, uint8_t i2c_addr, uint8_t register_address, uint8_t *data, uint8_t bytes_to_write);

#endif /* I2C_H_ */
