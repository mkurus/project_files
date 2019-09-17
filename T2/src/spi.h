/*
 * spi.h
 *
 *  Created on: 1 Mar 2016
 *      Author: admin
 */

#ifndef SPI_H_
#define SPI_H_


void Trio_Init_SPI();
void xmitRecvSpiData(uint8_t *tx_buffer, uint8_t *rx_buffer, uint16_t length);
bool SST25_IsEmpty();
void SST25_WriteSR(uint8_t value);
void SST25_ChipErase();
void SPI_IsrHandler(void);
#endif /* SPI_H_ */
