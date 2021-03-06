/*
 * trace.h
 *
 *  Created on: 25 Şub 2016
 *      Author: admin
 */

#ifndef TRACE_H_
#define TRACE_H_

#define TRACE_PORT       LPC_UART0



#define   SERIAL_DATA_IDLE_TIMEOUT      2




void task_trace();
void TraceCommUART_IsrHandler(void);
void debug_init();
void PRINT_K(char *msg);
void PRINT_G(char *msg);
void PRINT_GPS(char *gps_msg);
void PRINT_OFFLINE(char *msg);
void PRINT_CAN(char *msg);
void PRINT_RAW(char *msg, int length);
char trace_get_char();
#endif /* TRACE_H_ */
