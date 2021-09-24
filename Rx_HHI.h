#ifndef RX_HHI_H
#define RX_HHI_H

#include "HHI_commonUtil.h"

typedef enum {
	rxcrc_unknown = -1,
	rxcrc_ok = 0,
	rxcrc_nok = 1

} rxCrc_e;

//main commands
void HW_Rx( const HWStatus_t );
void HW_SetRxConfig( const HWStatus_t hwabs );

//reset commands
void HW_clearRxTimeoutIrq(void);
void HW_clearRxIrq(void);
void HW_clearRxPayloadCrc(void);

//status check commands
rxCrc_e HW_checkCRC(void);
int8_t HW_readSnr(void);
int16_t HW_readRssi(const HWStatus_t, int8_t);
uint16_t HW_getRxPayload(uint8_t* buf);

//option commands
#if (defined (BOARDVARIANT_SX1272) || defined (BOARDVARIANT_SX1276))
bool HW_IsChannelFree( uint32_t freq, int16_t rssiThresh );
#endif

#endif

