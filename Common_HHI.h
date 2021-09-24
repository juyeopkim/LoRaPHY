#ifndef COMMON_HHI_H
#define COMMON_HHI_H

#include "Common_HAL.h"

uint8_t HW_init_radio(const HWStatus_t* hwabs);
void HW_Sleep(void);
void HW_SetModem( RadioModems_t modem );
void HW_SetPublicNetwork( bool enable );
uint8_t HW_isSignalDetected();
uint8_t HW_isSignalSynchronized();
uint8_t HW_isRxOngoing();
uint8_t HW_isHeaderInfoValid();
uint8_t HW_isModemClear();

#endif
