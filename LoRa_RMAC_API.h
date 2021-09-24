#ifndef LoRa_RMAC_API_H
#define LoRa_RMAC_API_H
#include "mbed.h"
void RMAC_init();
void RMAC_FSM_start();
void RMAC_FSM_run();
void indicateRMACMonitoring();
uint8_t getRMACState();
uint8_t getRepeatFlag();
#endif
