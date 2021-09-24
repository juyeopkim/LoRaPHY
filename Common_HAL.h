#ifndef COMMON_HAL_H
#define COMMON_HAL_H

#include "mbed.h"

#define DBGMSG_HAL      0

//HW abstraction -------------------------------------

/*!
 *    Type of the modem. [LORA / FSK]
 */
typedef enum RadioState
{
	RF_NULL = 0,
    RF_IDLE,
    RF_RX_RUNNING,
    RF_TX_RUNNING,
    RF_CAD,
}RadioState_t;

typedef enum ModemType
{
    MODEM_FSK = 0,
    MODEM_LORA
}RadioModems_t;

/*!
 * Radio LoRa modem parameters
 */
typedef struct
{
    int8_t   Power;
    uint32_t Bandwidth;
    uint32_t Datarate;
    bool     LowDatarateOptimize;
    uint8_t  Coderate;
    uint16_t PreambleLen;
    bool     FixLen;
    uint8_t  PayloadLen;
    bool     CrcOn;
    bool     FreqHopOn;
    uint8_t  HopPeriod;
    bool     IqInverted;
    bool     RxContinuous;
    uint32_t TxTimeout;
    bool     PublicNetwork;
	uint16_t rxSymbTimeout;
}RadioLoRaSettings_t;


typedef struct
{
    int8_t SnrValue;
    int8_t RssiValue;
    uint8_t Size;
}RadioLoRaPacketHandler_t;



typedef struct
{
    RadioState               State;
    ModemType                Modem;
    uint32_t                 Channel;
    RadioLoRaSettings_t      LoRa;
    RadioLoRaPacketHandler_t LoRaPacketHandler;
} RadioSettings_t;


typedef struct {
    uint8_t boardConnected;
    RadioSettings_t settings;
} HWStatus_t;


//Common resource
typedef enum {
    HAL_ENT_NONE,
    HAL_ENT_TX,
    HAL_ENT_RX
} HALtype_entity;

//Common API (CAPI) functions -------------------------------------

//Common resource reservation (by TX or RX HAL)
int8_t HAL_capi_reserveResource(HALtype_entity);

//abstract configuration functions
int8_t HAL_capi_CfgLoRaLowDROpt(HALtype_entity, bool input);
int8_t HAL_capi_CfgLoRaDatarate(HALtype_entity, uint32_t input);
int8_t HAL_capi_CfgLoRaPower(HALtype_entity, uint32_t input);
int8_t HAL_capi_CfgLoRaBandwidth(HALtype_entity, uint32_t input);
int8_t HAL_capi_CfgLoRaCoderate(HALtype_entity, int8_t input);
int8_t HAL_capi_CfgLoRaPreambleLen(HALtype_entity, uint16_t input);
int8_t HAL_capi_CfgLoRaFrequencyChannel(HALtype_entity, uint32_t frequency);
int8_t HAL_capi_CfgLoRaRxContinuous(HALtype_entity, bool option);
int8_t HAL_capi_CfgLoRaRxTime(HALtype_entity, uint16_t time);
//HW config command from HAL to HAL
int8_t HAL_capi_SetTxConfig(void);
int8_t HAL_capi_SetRxConfig(void);
int8_t HAL_capi_setRfTxPower(int8_t power);

int8_t HAL_capi_startRx(uint32_t timeout);
int8_t HAL_capi_startTx( uint8_t *buffer, uint8_t size );


// HW interrupt handlers
void HAL_hwTimeout(void);
void HAL_hwDone(void);

#endif

