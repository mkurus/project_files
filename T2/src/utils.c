#include "board.h"
#include "chip.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>

#define     POLY       0x1021

/*****************************************************************/
uint16_t crc16(char *p_data, uint16_t length)
{
	uint16_t u16_crc = 0;
	uint8_t i;
	do{
		i = 8;
		u16_crc = u16_crc ^ (((uint16_t)*p_data++) << 8);
		 do{
			 if (u16_crc & 0x8000)
		    	u16_crc = u16_crc << 1 ^ POLY;
			 else
		    	u16_crc = u16_crc << 1;
		  }
		   while(--i);

	}while(--length);
	 return u16_crc;
}
/*****************************************************************/
int search_chr(char *token, char s){
    if (!token || s=='\0')
        return 0;

    for (;*token; token++)
        if (*token == s)
            return 1;

    return 0;
}
/*****************************************************************/
char *remove_chr(char *str, char *charToRemove) {
    char *src = str , *dst = str;

    while(*src)
        if(search_chr(charToRemove, *src))
            *src++;
        else
            *dst++ = *src++;

    *dst='\0';
    return str;
}
/*****************************************************************/
void SetBit(uint32_t *x, uint32_t bitNum) {
    *x |= (1 << bitNum);
}
/*****************************************************************/
void ResetBit(uint32_t *x, uint32_t bitNum) {
    *x &= ~(1 << bitNum);
}
/******************************************************************/
static char bfr[20+1];

char* uint64ToDecimal(uint64_t v)
{
  bool first;
  char* p = bfr + sizeof(bfr);
  *(--p) = '\0';
  for (first = true; v || first; first = false) {
    const uint32_t digit = v % 10;
    const char c = '0' + digit;
    *(--p) = c;
    v = v / 10;
  }
  return p;
}
/*************************************************************************/
uint32_t calc_chksum(uint8_t *start_addr, uint16_t length)
{
	uint16_t  i;

	uint32_t chksum =0;

	for(i=0; i < length; i++)
		chksum += start_addr[i];

	return chksum;
}
/**
 * sort buffer in descending order
 * return: Median value of array
 */
float median_filter(float *buffer, uint8_t size)
{
	float temp;
	int i,k;


	for (i = 0 ; i < ( size - 1 ); i++){
	   for (k = 0; k < size - i - 1;  k++) {
	      if (buffer[k] < buffer[k + 1]) {
	    	  temp  = buffer[k];
	    	  buffer[k]  = buffer[k + 1];
	    	  buffer[k + 1] = temp;
	      }
	   }
	}
	return buffer[(size - 1) / 2];
}
