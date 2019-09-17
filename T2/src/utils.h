/*
 * utils.h
 *
 *  Created on: 2 Mar 2016
 *      Author: admin
 */

#ifndef UTILS_H_
#define UTILS_H_


/*********************/

typedef struct UART_PORT
{
	LPC_USART_T *usart;
	RINGBUFF_T *tx_ringbuf;
	RINGBUFF_T *rx_ringbuf;
}UART_PORT_T;
float median_filter(float *buffer,  uint8_t size);
char* uint64ToDecimal(uint64_t v);
uint16_t crc16(char *p_data, uint16_t length);
int  search_chr(char *token, char s);
char *remove_chr(char *str, char *charToRemove);
void SetBit(uint32_t *x, uint32_t bitNum);
void ResetBit(uint32_t *x, uint32_t bitNum);
uint32_t calc_chksum(uint8_t *start_addr, uint16_t length);
/* extract bytes out of a word32 */
#define WORD32_BYTE0(l)    ((uint8_t)(((uint32_t)(l)) & 0x000000FF))
#define WORD32_BYTE1(l)    ((uint8_t)(((uint32_t)(l) >>  8) & 0x000000FF))
#define WORD32_BYTE2(l)    ((uint8_t)(((uint32_t)(l) >> 16) & 0x000000FF))
#define WORD32_BYTE3(l)    ((uint8_t)(((uint32_t)(l) >> 24) & 0x000000FF))
/* convert 4 bytes to a word32 */
#define MK_WORD32(A,B,C,D)  ((uint32_t)( ((uint32_t)(A)<<24) | ((uint32_t)(B)<<16) | ((uint32_t)(C)<<8) | (D) ))
#define MK_WORD16(A,B)      ((uint16_t)( ((uint16_t)(A)<<8)  | (B) ))
#define MK_WORD64(A,B)      ((uint64_t)( ((uint64_t)(A)<<32) | (B) ))


#define CHAR_SOH                     0x01
#define CHAR_STX                     0x02
#define CHAR_ETX                     0x03
#define CHAR_CR                      0x0D
#define CHAR_LF                      0x0A
#define CHAR_ESC                     0x1B
#endif /* UTILS_H_ */
