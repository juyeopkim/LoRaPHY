#ifndef LoRa_PHY_API_H
#define LoRa_PHY_API_H
#include "mbed.h"
#include "PHYHAL_interface.h"


typedef enum{
    in_NULL = 0,
    TX,
    RX,
    FSM,
    CLEAR
}RMAC_PHY_indicator_e;

#if defined PHY_ONLY
typedef enum{
    PHY_IDLE=0,
    PHY_TX_RUN,
    PHY_RX_WAIT,
    PHY_RX_RUN
} PHYState;

void PHY_init(void (*MACRxDone)(uint8_t*, uint16_t));
void PHY_FSM_start(void (*MACRxDone)(uint8_t*, uint16_t));

uint8_t getRepeatFlag();
PHYState getPresentState();
PHYState FSM_receive_dl();
PHYState FSM_transmit_ul(uint8_t* buffer, uint8_t buffer_size);
PHYState FSM_transmit_done();

#elif defined RMAC_ON
void PHY_init(void (*MACRxDone)(uint8_t*, uint16_t),void (*MACRxFail)());
void PHY_FSM_start(void (*MACRxDone)(uint8_t*, uint16_t),void (*MACRxFail)());
#endif

#if defined ANALYSIS_ON
void clearVariables();
#endif


void PHY_FSM_run();
void PHY_configUlPayload(uint8_t *buffer, uint16_t size,HALenum_bw_e BW,HALenum_dr_e SF,uint32_t Freq);
void PHY_setRxSlot(HALenum_bw_e BW,HALenum_dr_e SF,uint32_t Freq);
void PHY_setRxDone(void (*functionPtr)(uint8_t*, uint16_t));
void PHY_setRxFail(void (*functionPtr)());
void PHY_indicateEvent(RMAC_PHY_indicator_e event);
uint8_t PHY_setDelayTime(uint32_t timeMilliSec);
uint8_t PHY_setWindowSize(uint8_t windowNum, uint32_t timeMilliSec);


#endif
