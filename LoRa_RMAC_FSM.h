#ifndef LoRa_RMAC_FSM_H
#define LoRa_RMAC_FSM_H

#define UPPER_SPREADING_FACTOR 12
#define LOWER_SPREADING_FACTOR 7

#include "PHYHAL_interface.h"
#include "mbed.h"
typedef enum{
    RMAC_IDLE=0,
    RMAC_MONITOR,
    RMAC_CLEAR,
    RMAC_RELAY
} RMACState;

typedef enum{
    RMACe_NULL=0,
    RMACe_PREPARE_MONITOR,
    RMACe_DELAY_END,
    RMACe_DETECT,
    RMACe_DETECT_FAIL,
    RMACe_CONVERT_REQ,
    RMACe_CLEARED,
    RMACe_RECEIVE,
    RMACe_RECEIVE_FAIL
} RMACEvent;

typedef  struct{
    uint8_t *Payload;
    uint16_t Size;
}RMACLoRaPacket_t;

typedef struct{
    HALenum_bw_e BandWidth;
    uint32_t ChannelFreq;
    HALenum_dr_e Datarate;
    RMACLoRaPacket_t RMACLoRaPacket;
}Message_t;

void setRandomParameter();
void setRxParameter();
void setTxParameterSet();
void PHYRxDoneInterrupt(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void PHYRxFailInterrupt();
void delayEnd();
void expireMonitor();

#endif
