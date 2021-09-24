#ifndef PHYHAL_INTERFACE_H
#define PHYHAL_INTERFACE_H

#include "mbed.h"
#include "PHYHAL_types.h"

#define RX_MAX_RXSYMBOLTIME			1024


/* HAL - PHY PRIMITIVES */


//1. ------------- initialization primitive ------------- 
int HAL_cmd_init(void (*HAL_tx_cnf)(HALtype_txCmdErr), void (*HAL_rx_cnf)(HALtype_rxCmdErr, uint8_t*,uint16_t,int16_t,int8_t));


//2. ------------- Common functionality primitives ------------
int HAL_cmd_Sleep();           //force to initialize the H/W


/* ------------- TX commands and configuration primitives ------------ */
HALtype_txCfgErr HAL_cmd_SetRfTxPower( int8_t power );

int8_t HAL_cmd_transmit(uint8_t* buffer, uint8_t buffer_size);
HALtype_txCfgErr HAL_cmd_SetTxConfig( int8_t power, HALenum_bw_e bandwidth, HALenum_dr_e datarate, HALenum_cr_e coderate, uint16_t preambleLen,uint32_t frequency );


/* ------------- RX commands and configuration primitives ------------ */
//rxTime : 0 - RX CONTINUOUS mode, 1 ~ 1023 - RX SINGLE mode (value is the number of symbols. actual time will be the number of symbol multiplied by (2^SF / BW).
HALtype_rxCfgErr HAL_cmd_SetRxConfig( HALenum_bw_e bandwidth, HALenum_dr_e datarate, HALenum_cr_e coderate, uint16_t preambleLen,uint32_t frequency, uint16_t rxTime);
int8_t HAL_cmd_receive(void);


/* HAL utility functions */
bool HAL_isRfActive(void); //check RF status : true if the RF is in idle
uint8_t HAL_getRxStatus(); //Query the H/W status
bool HAL_isSignalDetected(); //check H/W modem status : true if signal detected
bool HAL_isSignalSynchronized();//check H/W modem status : true if preamble detected
bool HAL_isRxOngoing();//check H/W modem status : true if rx is on going
bool HAL_isHeaderInfoValid();//check H/W modem status : true if header is valid
bool HAL_isModemClear();//check H/W modem status : true if modem is clear
#endif
