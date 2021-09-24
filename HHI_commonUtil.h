#ifndef HHI_COMMONUTIL_H
#define HHI_COMMONUTIL_H

#include "Common_HAL.h"

#if defined(BOARDVARIANT_SX1276)
#define RF_MID_BAND_THRESH                          525000000
#endif
#if (defined(BOARDVARIANT_SX1272) || defined(BOARDVARIANT_SX1276))
#define FREQ_STEP                                   61.03515625
#endif


typedef enum BoardType
{
#if defined(BOARDVARIANT_SX1272) 
    SX1272MB2XAS = 0,
    SX1272MB1DCS,
    NA_MOTE_72,
    MDOT_F411RE,
#elif defined(BOARDVARIANT_SX1276) 
    SX1276MB1MAS = 0,
    SX1276MB1LAS,
#endif
    UNKNOWN
}BoardType_t;

typedef enum ModemStatus
{
    SIGNAL_DETECTED = 1,
    SIGNAL_SYNCHRONIZED,
    RX_ONGOING,
    HEADER_INFO_VALID,
    MODEM_CLEAR
}ModemStatus_t;


void HW_SetOpMode( uint8_t opMode );
void HW_SetChannel( uint32_t freq );
void HW_Standby(void);

void HW_SpiWrite( uint8_t addr, uint8_t data );
void HW_SpiWriteFifo( uint8_t *buffer, uint8_t size );
uint8_t HW_SpiRead( uint8_t addr );
void HW_SpiReadFifo( uint8_t *buffer, uint8_t size );

#endif
