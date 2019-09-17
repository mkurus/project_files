/*
 * QuectelM95.h
 *
 *  Created on: 29 Şub 2016
 *      Author: admin
 */

#ifndef QUECTELM95_H_
#define QUECTELM95_H_

#define QUECTEL_M95_POWER_ON_TIME       250      /* 2500 ms turn-on time */
#define QUECTEL_M95_POWER_OFF_TIME      80       /* 600 ms < turn-off time < 1000 ms */
#define QUECTEL_M95_EMERG_OFF_TIME      5        /* Emergency-off time should be > 20 ms*/
#define QUECTEL_M95_RESTART_DELAY       60       /* Restart delay time should be > 500ms */
#define QUECTEL_M66_POWER_ON_OFF_DELAY             200       /* Restart delay time should be > 500ms */
#define QUECTEL_M66_HARD_RESET_LOW_TIME            25
#define QUECTEL_M66_WAIT_TIME_AFTER_RESET          700

#define CTRL_Z              0x1A
#define QUECTEL_TCP_HDR     "TCP"


static const char ENTER[]              = "\r\n";
static const char Command_AT[]         = "AT\r";
static const char Command_RESET[]      = "AT+CFUN=1,1\r";                        /* soft reset */
static const char Command_ATE0[]  	   = "ATE0;+CMEE=1;+CREG=0;+CGREG=0;+CMGF=1\r";
static const char Command_CMER[] 	   = "AT+CMER=2,0,0,2,0\r";                  /* enable unsolicited message codes*/
static const char Command_CREG[]       = "AT+CREG?\r";
static const char Command_CMGS[]       = "AT+CMGS=\"";
static const char Command_CGSN[]       = "AT+CGSN\r";                          				        /* Read IMEI*/
static const char Command_CBC[]        = "AT#CBC\r";                            			        /* Read battery voltage*/
static const char Command_CIMI[] 	   = "AT+CIMI\r";                            		    	    /* Read IMSI*/
static const char Command_ATA[]        = "ATA\r";
static const char Command_QUADCH[]     = "AT+QAUDCH=";
static const char Command_QMIC[]       = "AT+QMIC=";
static const char Command_GetStatus[]  = "AT+CMGR=1;+CSQ;+CREG?;+CBC;+CMGD=1,4;+CLCC;+QISTAT;+QISACK\r"; /* query status*/
static const char Command_CMGL[] 	   = "AT+CMGL=4\r";                          /* Read SMS*/
static const char Command_CPIN[]       = "AT+CPIN?\r";                           /* get pin status*/
static const char Command_CGDCONT[]    = "AT+CGDCONT=1,\"IP\",\"";
static const char Command_SGACT[]      = "AT#SGACT=1,1,\"";
static const char Command_SCFG[]       = "AT#SCFG=1,1,300,480,100,0\r";
static const char Command_SCFGEXT[]    = "AT#SCFGEXT=1,0,0,0,0,0\r";         	 /* socket configuration */
static const char Command_SSENDEXT[]   = "AT#SSENDEXT=1,";                  	 /* send data */
static const char Command_SI[]    	   = "AT#SI=1\r";
static const char Command_ATSH[]       = "AT#SH=1\r";
static const char Command_CloseSocket[]     = "AT+QICLOSE\r";
static const char Command_ActivatePDP[]     = "AT+QICSGP=1,\"";
static const char Command_TCPConnect[]      = "AT+QIOPEN=\"TCP\",\"";
static const char Command_SendData[]        = "AT+QISEND=";
static const char Command_RecvData[]        = "AT+QIRD=1,1,0,";
static const char Command_LBS[]             = "AT+QCELLLOC=1\r";
static const char Command_Update_QLOC_Tout[] = "AT+QLOCCFG=\"timeout\", 10\r";
static const char DATA_SEND_PROMPT          ='>';
/* close socket connection*/
static const char Command_SetIpAddrMode[]   = "AT+QIDNSIP=";
static const char Command_SetFgndContext[]  = "AT+QIFGCNT=";
static const char Command_SetMuxMode[]      = "AT+QIMUX=0\r";
static const char Command_SetMode[]         = "AT+QIMODE=0\r";
static const char Command_SetRecvIndiMode[] = "AT+QINDI=";
static const char Command_Dial[]            = "ATD";
static const char RespQISACK[]              = "+QISACK:";
static const char RespLBS[]                 = "+QCELLLOC";
//static const char Command_SD[]         = "AT#SD=1,0,10510,\"modules.telit.com\",0,0,1";  /* connect to telit echo server*/
//static const char Command_SD[]         = "AT#SD=1,0,6081,\"178.63.30.81\",0,0,1\r";  /* connect to trio server */
//static const char Command_SD[]         = "AT#SD=1,0,1555,\"213.14.184.87\",0,0,1\r";  /* connect to my computer */
static const char Command_SD[]                ="AT#SD=1,0,";
static const char Command_SRECV[]             ="AT#SRECV=1,256\r";
static const char Command_GetLocalIP[] = "AT+QILOCIP\r";
static const char respOK[]             = "OK";
static const char respSendOK[]         = "SEND OK";
static const char respReady[]          = "READY";
static const char cregResp[]           = "+CREG: ";
static const char respCLCC[]           = "+CLCC:";
static const char RespCSQ[]            = "+CSQ: ";
static const char RespCBC[]            = "+CBC";
static const char RespCMGR[]           = "+CMGR:";

static const char RespConnectOK[]           = "CONNECT OK";
static const char RespAlrdyConnect[]        = "ALREADY CONNECT";

#endif /* QUECTELM95_H_ */
