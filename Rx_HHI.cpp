#include "Rx_HHI.h"
#include "Common_HHI.h"

#if defined(BOARDVARIANT_SX1272)
#include "sx1272Regs-LoRa.h"
#include "sx1272Regs-FSK.h"
#elif defined(BOARDVARIANT_SX1276)
#include "sx1276Regs-LoRa.h"
#include "sx1276Regs-FSK.h"
#endif


/*!
 * Constant values need to compute the RSSI value
 */
#if defined(BOARDVARIANT_SX1272)
#define RSSI_OFFSET                                 -139
#elif defined(BOARDVARIANT_SX1276)
#define RSSI_OFFSET_LF                              -164.0
#define RSSI_OFFSET_HF                              -157.0
#endif

void HW_Rx( const HWStatus_t hwabs )
{
    bool rxContinuous = false;

	switch(hwabs.settings.Modem)
	{
        default:
        case MODEM_FSK:
        {
            debug("[HAL] [ERROR] Failed to do RX : modem setting is invalid (%i)\n", hwabs.settings.Modem);
            return;
        }
        case MODEM_LORA:
        {
            //IQ invert process -------------------
            if( hwabs.settings.LoRa.IqInverted == true )
            {
#if (defined(BOARDVARIANT_SX1272) || defined(BOARDVARIANT_SX1276))
                HW_SpiWrite( REG_LR_INVERTIQ, ( ( HW_SpiRead( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_ON | RFLR_INVERTIQ_TX_OFF ) );
                HW_SpiWrite( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
#endif
            }
            else
            {
#if (defined(BOARDVARIANT_SX1272) || defined(BOARDVARIANT_SX1276))
                HW_SpiWrite( REG_LR_INVERTIQ, ( ( HW_SpiRead( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
                HW_SpiWrite( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
#endif
            }

#if defined(BOARDVARIANT_SX1276)
            // ERRATA 2.3 - Receiver Spurious Reception of a LoRa Signal
            if( hwabs.settings.LoRa.Bandwidth+7 < 9 )
            {
                HW_SpiWrite( REG_LR_DETECTOPTIMIZE, HW_SpiRead( REG_LR_DETECTOPTIMIZE ) & 0x7F );
                HW_SpiWrite( REG_LR_TEST30, 0x00 );
                switch( hwabs.settings.LoRa.Bandwidth+7)
                {
                case 0: // 7.8 kHz
                    HW_SpiWrite( REG_LR_TEST2F, 0x48 );
                    HW_SetChannel(hwabs.settings.Channel + 7.81e3 );
                    break;
                case 1: // 10.4 kHz
                    HW_SpiWrite( REG_LR_TEST2F, 0x44 );
                    HW_SetChannel(hwabs.settings.Channel + 10.42e3 );
                    break;
                case 2: // 15.6 kHz
                    HW_SpiWrite( REG_LR_TEST2F, 0x44 );
                    HW_SetChannel(hwabs.settings.Channel + 15.62e3 );
                    break;
                case 3: // 20.8 kHz
                    HW_SpiWrite( REG_LR_TEST2F, 0x44 );
                    HW_SetChannel(hwabs.settings.Channel + 20.83e3 );
                    break;
                case 4: // 31.2 kHz
                    HW_SpiWrite( REG_LR_TEST2F, 0x44 );
                    HW_SetChannel(hwabs.settings.Channel + 31.25e3 );
                    break;
                case 5: // 41.4 kHz
                    HW_SpiWrite( REG_LR_TEST2F, 0x44 );
                    HW_SetChannel(hwabs.settings.Channel + 41.67e3 );
                    break;
                case 6: // 62.5 kHz
                    HW_SpiWrite( REG_LR_TEST2F, 0x40 );
                    break;
                case 7: // 125 kHz
                    HW_SpiWrite( REG_LR_TEST2F, 0x40 );
                    break;
                case 8: // 250 kHz
                    HW_SpiWrite( REG_LR_TEST2F, 0x40 );
                    break;
                }
            }
            else
            {
                HW_SpiWrite( REG_LR_DETECTOPTIMIZE, HW_SpiRead( REG_LR_DETECTOPTIMIZE ) | 0x80 );
            }
#endif

            rxContinuous = hwabs.settings.LoRa.RxContinuous;

            //FREQ HOP process -------------------
            if( hwabs.settings.LoRa.FreqHopOn == true )
            {
#if (defined(BOARDVARIANT_SX1272) || defined(BOARDVARIANT_SX1276))
                HW_SpiWrite( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                                                  //RFLR_IRQFLAGS_RXDONE |
                                                  //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=RxDone, DIO2=FhssChangeChannel
                HW_SpiWrite( REG_DIOMAPPING1, ( HW_SpiRead( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK  ) | RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO2_00 );
#endif
            }
            else
            {
#if (defined(BOARDVARIANT_SX1272) || defined(BOARDVARIANT_SX1276))
                HW_SpiWrite( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                                                  //RFLR_IRQFLAGS_RXDONE |
                                                  //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=RxDone
                HW_SpiWrite( REG_DIOMAPPING1, ( HW_SpiRead( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_00 );
#endif
            }

#if (defined(BOARDVARIANT_SX1272) || defined(BOARDVARIANT_SX1276))
            HW_SpiWrite( REG_LR_FIFORXBASEADDR, 0 );
            HW_SpiWrite( REG_LR_FIFOADDRPTR, 0 );
#endif
        }

        if( rxContinuous == true )
        {
            HW_SetOpMode( RFLR_OPMODE_RECEIVER );
        }
        else
        {
            HW_SetOpMode( RFLR_OPMODE_RECEIVER_SINGLE );
        }
        break;
    }

}

void HW_SetRxConfig (const HWStatus_t hwabs)
{
    uint32_t bw_reg = hwabs.settings.LoRa.Bandwidth;
    HW_SetChannel(hwabs.settings.Channel);

//modem config
#if defined (BOARDVARIANT_SX1272)
    HW_SpiWrite( REG_LR_MODEMCONFIG1,
                ( HW_SpiRead( REG_LR_MODEMCONFIG1 ) &
                RFLR_MODEMCONFIG1_BW_MASK &
                RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK &
                RFLR_MODEMCONFIG1_RXPAYLOADCRC_MASK &
                RFLR_MODEMCONFIG1_LOWDATARATEOPTIMIZE_MASK ) |
                ( bw_reg << 6 ) | ( hwabs.settings.LoRa.Coderate << 3 ) |
                ( hwabs.settings.LoRa.FixLen << 2 ) | ( hwabs.settings.LoRa.CrcOn << 1 ) |
                hwabs.settings.LoRa.LowDatarateOptimize );

    HW_SpiWrite( REG_LR_MODEMCONFIG2,
                ( HW_SpiRead( REG_LR_MODEMCONFIG2 ) &
                RFLR_MODEMCONFIG2_SF_MASK &
                RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) |
                ( hwabs.settings.LoRa.Datarate << 4 ) |
                ( ( hwabs.settings.LoRa.rxSymbTimeout >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) );
#elif defined (BOARDVARIANT_SX1276)
    bw_reg += 7;

    HW_SpiWrite( REG_LR_MODEMCONFIG1,
                ( HW_SpiRead( REG_LR_MODEMCONFIG1 ) &
                RFLR_MODEMCONFIG1_BW_MASK &
                RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) |
                ( bw_reg << 4 ) | ( hwabs.settings.LoRa.Coderate << 1 ) |
                hwabs.settings.LoRa.FixLen );

    HW_SpiWrite( REG_LR_MODEMCONFIG2,
                ( HW_SpiRead( REG_LR_MODEMCONFIG2 ) &
                RFLR_MODEMCONFIG2_SF_MASK &
                RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK &
                RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) |
                ( hwabs.settings.LoRa.Datarate << 4 ) | ( hwabs.settings.LoRa.CrcOn << 2 ) |
                ( ( hwabs.settings.LoRa.rxSymbTimeout >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) );

    HW_SpiWrite( REG_LR_MODEMCONFIG3,
                ( HW_SpiRead( REG_LR_MODEMCONFIG3 ) &
                RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) |
                ( hwabs.settings.LoRa.LowDatarateOptimize << 3 ) );
#endif

#if (defined (BOARDVARIANT_SX1272) || defined (BOARDVARIANT_SX1276))
    HW_SpiWrite( REG_LR_SYMBTIMEOUTLSB, ( uint8_t )( hwabs.settings.LoRa.rxSymbTimeout & 0xFF ) );

    HW_SpiWrite( REG_LR_PREAMBLEMSB, ( uint8_t )( ( hwabs.settings.LoRa.PreambleLen >> 8 ) & 0xFF ) );
    HW_SpiWrite( REG_LR_PREAMBLELSB, ( uint8_t )( hwabs.settings.LoRa.PreambleLen & 0xFF ) );
#endif

    if( hwabs.settings.LoRa.FixLen == 1 )
    {
#if (defined (BOARDVARIANT_SX1272) || defined (BOARDVARIANT_SX1276))
        HW_SpiWrite( REG_LR_PAYLOADLENGTH, hwabs.settings.LoRa.PayloadLen );
#endif
    }


    if( hwabs.settings.LoRa.FreqHopOn == true )
    {
#if (defined (BOARDVARIANT_SX1272) || defined (BOARDVARIANT_SX1276))
        HW_SpiWrite( REG_LR_PLLHOP, ( HW_SpiRead( REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
        HW_SpiWrite( REG_LR_HOPPERIOD, hwabs.settings.LoRa.HopPeriod );
#endif
    }

#if defined (BOARDVARIANT_SX1276)
    if( ( bw_reg == 9 ) && ( hwabs.settings.Channel > RF_MID_BAND_THRESH ) )
    {
        // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
        HW_SpiWrite( REG_LR_TEST36, 0x02 );
        HW_SpiWrite( REG_LR_TEST3A, 0x64 );
    }
    else if( bw_reg == 9 )
    {
        // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
        HW_SpiWrite( REG_LR_TEST36, 0x02 );
        HW_SpiWrite( REG_LR_TEST3A, 0x7F );
    }
    else
    {
        // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
        HW_SpiWrite( REG_LR_TEST36, 0x03 );
    }
#endif

//detect optitmize option
#if (defined (BOARDVARIANT_SX1272) || defined (BOARDVARIANT_SX1276))
    if( hwabs.settings.LoRa.Datarate == 6 )
    {
        HW_SpiWrite( REG_LR_DETECTOPTIMIZE,
                        ( HW_SpiRead( REG_LR_DETECTOPTIMIZE ) &
                        RFLR_DETECTIONOPTIMIZE_MASK ) |
                        RFLR_DETECTIONOPTIMIZE_SF6 );
        HW_SpiWrite( REG_LR_DETECTIONTHRESHOLD,
                        RFLR_DETECTIONTHRESH_SF6 );
    }
    else
    {
        HW_SpiWrite( REG_LR_DETECTOPTIMIZE,
                        ( HW_SpiRead( REG_LR_DETECTOPTIMIZE ) &
                        RFLR_DETECTIONOPTIMIZE_MASK ) |
                        RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
        HW_SpiWrite( REG_LR_DETECTIONTHRESHOLD,
                        RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
    }
#endif
}


void HW_clearRxTimeoutIrq(void)
{
#if (defined (BOARDVARIANT_SX1272) || defined (BOARDVARIANT_SX1276)) 
	HW_SpiWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXTIMEOUT );
#endif
}



void HW_clearRxIrq(void)
{
#if (defined (BOARDVARIANT_SX1272) || defined (BOARDVARIANT_SX1276))   
	HW_SpiWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE );
#endif
}


rxCrc_e HW_checkCRC(void)
{
#if (defined (BOARDVARIANT_SX1272) || defined (BOARDVARIANT_SX1276))    
	volatile uint8_t irqFlags = HW_SpiRead( REG_LR_IRQFLAGS );
	if( ( irqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR_MASK ) == RFLR_IRQFLAGS_PAYLOADCRCERROR )
	{
		return rxcrc_nok;
	}
	else
#endif
	{
		return rxcrc_ok;
	}
}


void HW_clearRxPayloadCrc(void)
{
#if (defined (BOARDVARIANT_SX1272) || defined (BOARDVARIANT_SX1276)) 
	HW_SpiWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR );
#endif
}


int8_t HW_readSnr(void)
{
	int8_t snr=0;
#if (defined (BOARDVARIANT_SX1272) || defined (BOARDVARIANT_SX1276)) 
	uint8_t snrValue = HW_SpiRead( REG_LR_PKTSNRVALUE );
	if( snrValue & 0x80 ) // The SNR sign bit is 1
	{
		// Invert and divide by 4
		snr = ( ( ~snrValue + 1 ) & 0xFF ) >> 2;
		snr = -snr;
	}
	else
	{
		// Divide by 4
		snr = ( snrValue & 0xFF ) >> 2;
	}
#endif
	return snr;
}

int16_t HW_readRssi(const HWStatus_t hwabs, int8_t snr)
{
	int16_t rssi = 0;
#if (defined (BOARDVARIANT_SX1272) || defined (BOARDVARIANT_SX1276))
    int16_t rssi_offset;
#if defined (BOARDVARIANT_SX1272)
	uint8_t rssiValue = HW_SpiRead( REG_LR_PKTRSSIVALUE );
    rssi_offset = RSSI_OFFSET;
#elif defined (BOARDVARIANT_SX1276)
    int16_t rssiValue = HW_SpiRead( REG_LR_PKTRSSIVALUE );
    if( hwabs.settings.Channel > RF_MID_BAND_THRESH )
    {
        rssi_offset = RSSI_OFFSET_HF;
    }
    else
    {
        rssi_offset = RSSI_OFFSET_LF;
    }
#endif

    if( snr < 0 )
    {
        rssi = rssi_offset + rssiValue + ( rssiValue >> 4 ) + snr;
    }
    else
    {
        rssi = rssi_offset + rssiValue + ( rssiValue >> 4 );
    }
#endif
	return rssi;
}

uint16_t HW_getRxPayload(uint8_t* buf)
{
    uint16_t size = 0;
#if (defined (BOARDVARIANT_SX1272) || defined (BOARDVARIANT_SX1276))   
	size = HW_SpiRead( REG_LR_RXNBBYTES );
	HW_SpiReadFifo( buf, size );
#endif
	return size;
}


#if (defined (BOARDVARIANT_SX1272) || defined (BOARDVARIANT_SX1276))
static int16_t HW_getRssi( uint32_t freq )
{
    int16_t rssi = 0;
    int16_t rssi_offset;

#if defined (BOARDVARIANT_SX1276)
    if( freq > RF_MID_BAND_THRESH )
    {
        rssi_offset = RSSI_OFFSET_HF;
    }
    else
    {
        rssi_offset = RSSI_OFFSET_LF;
    }
#elif defined (BOARDVARIANT_SX1272)
    rssi_offset = RSSI_OFFSET;
#endif
    rssi = rssi_offset + HW_SpiRead( REG_LR_RSSIVALUE );
    
    return rssi;
}

bool HW_IsChannelFree( uint32_t freq, int16_t rssiThresh )
{

    int16_t rssi = 0;

    HW_SetModem( MODEM_LORA );
    HW_SetChannel( freq );
    HW_SetOpMode( RF_OPMODE_RECEIVER );

    wait_ms( 1 );
    rssi = HW_getRssi( MODEM_LORA );
    HW_Sleep( );

    if( rssi > rssiThresh )
    {
        return false;
    }
    return true;
}


#endif
