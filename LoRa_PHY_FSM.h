#ifndef LoRa_PHY_FSM_H

#include "PHYHAL_interface.h"

#define LoRa_PHY_FSM_H

#define BUF_SIZE 32
#define TX_REQUEST 0x01
#define RX_DELAY1 1
#define RX_DELAY2 2

#define PHY_DEFAULT_TXPOWER         14
#define PHY_DEFAULT_BANDWIDTH         HAL_LRBW_125
#define PHY_DEFAULT_DATARATE         HAL_LRDatarate_SF7
#define PHY_DEFAULT_CODERATE         HAL_LRCoderate_4_5
#define PHY_DEFAULT_PREAMBLELEN     8
#define PHY_DEFAULT_FREQUENCY         922100000
#define DEBUG_MSG 0


typedef  struct{
    uint8_t *Payload;
    uint16_t Size;
    int16_t Rssi;
    int8_t Snr;
}PHYLoRaPacket_t;


typedef struct{
    HALenum_bw_e BandWidth;
    uint32_t ChannelFreq;
    HALenum_dr_e Datarate;
    PHYLoRaPacket_t PHYLoRaPacket;
}Message_t;

typedef struct{
    HALenum_bw_e BandWidth;
    HALenum_dr_e SpreadingFactor;
    uint32_t Frequency;
}MAC_PHY_params_e;

typedef enum{
    PHYe_NULL=0,
    PHYe_TX_Req,
    PHYe_TX_Only_Req,
    PHYe_TX_Fail,
    PHYe_TX_Done,
    PHYe_RX_Only_Req,
    PHYe_RX_Delay_End,
    PHYe_RX_Detect,
    PHYe_RX_Fail
} PHYEvent;


typedef enum{
    PHYs_default=0, //class A
    PHYs_Tx,
    PHYs_Rx
}PHYstatus;

#if defined RMAC_ON
typedef enum{
    PHY_IDLE=0,
    PHY_TX_RUN,
    PHY_RX_WAIT,
    PHY_RX_RUN
} PHYState;

PHYState FSM_receive_dl();
PHYState FSM_transmit_ul(uint8_t* buffer, uint8_t buffer_size);
PHYState FSM_transmit_done();
PHYState getPresentState();
#endif

void RxDelay1End();
void RxDelay2End();
void expireWindow();
#if 0
void TxDoneInterrupt();
void TxTimeoutInterrupt();
void RxDoneInterrupt(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void RxErrorInterrupt();
void RxTimeoutInterrupt();
#endif
void setDl(HALenum_bw_e bandwidth,HALenum_dr_e datarate,uint32_t freq);
void setUl(HALenum_bw_e bandwidth,HALenum_dr_e datarate,uint32_t freq);
void resetRxErr();
void setRxErr();
bool isRxError();

//event flag setting
void PHYEvent_setEventFlag(PHYEvent event);
void PHYEvent_clearEventFlag(PHYEvent event);
void PHYEvent_clearAllEventFlag(void);
int PHYEvent_checkEventFlag(PHYEvent event);
#endif
