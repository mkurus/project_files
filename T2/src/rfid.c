/*
 * rfid.c
 *
 *  Created on: 2 Ara 2016
 *      Author: admin
 */

#include "chip.h"
#include "board.h"
#include "timer.h"
#include "iap_config.h"
#include "ProcessTask.h"
#include "settings.h"
#include "rfid.h"
#include "io_ctrl.h"
#include "status.h"
#include "trace.h"
#include "utils.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

//static const PINMUX_GRP_T T2_RFIDPortMuxTable[] = {
//	{RFID_TX_PORT_NUM,  RFID_TX_PIN_NUM,   IOCON_MODE_PULLUP | IOCON_FUNC3},	/* TXD3 */
//	{RFID_RX_PORT_NUM,  RFID_RX_PIN_NUM,   IOCON_MODE_PULLUP | IOCON_FUNC3} //* RXD3 */
//};

