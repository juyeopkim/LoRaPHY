#ifndef TX_HHI_H
#define TX_HHI_H

#include "HHI_commonUtil.h"

//main commands
void HW_Tx( const HWStatus_t HWabs, uint32_t timeout );
void HW_SetRfTxPower( const HWStatus_t hwabs, int8_t power );
void HW_SetTxConfig( const HWStatus_t hwabs );
void HW_setLoRaIqInverted(bool IqInverted);
void HW_setTxPayload(uint8_t* buffer, uint8_t size);

//reset commands
void HW_clearTxIrq(void);


#endif
