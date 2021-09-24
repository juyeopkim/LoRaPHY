#if defined(BOARDVARIANT_SX1272)
#include "sx1272Regs-LoRa.h"
#include "sx1272Regs-FSK.h"
#elif defined(BOARDVARIANT_SX1276)
#include "sx1276Regs-LoRa.h"
#include "sx1276Regs-FSK.h"
#endif

#include "Common_HHI.h"
#include "HHI_commonUtil.h"

/*!
 * Sync word for Private LoRa networks
 */
#define LORA_MAC_PRIVATE_SYNCWORD                   0x12
/*!
 * Sync word for Public LoRa networks
 */
#define LORA_MAC_PUBLIC_SYNCWORD                    0x34

#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
#define MASK_REG_MODEMSTAT_SIGNAL_DETECTED                           0x01
#define MASK_REG_MODEMSTAT_SYNCHRONIZED                         0x02
#define MASK_REG_MODEMSTAT_RX_ONGOING                           0x04
#define MASK_REG_MODEMSTAT_HEADER_INFO_VALID                    0x08
#define MASK_REG_MODEMSTAT_MODEM_CLEAR                          0x10

#define FREQ_STEP                                   61.03515625
/*
SX1272/1276( events, D11, D12, D13, D10, A0, //spi
				D2, D3, D4, D5, D8, D9 ), //dio0~5
*/
InterruptIn dio0(D2);
InterruptIn dio1(D3);

SPI hw_spi(D11, D12, D13);
DigitalOut hw_nss(D10);
DigitalInOut hw_reset(A0);
DigitalInOut hw_AntSwitch(A4);
#endif

typedef struct
{
    ModemType   Modem;
    uint8_t     Addr;
    uint8_t     Value;
}RadioRegisters_t;

#if defined(BOARDVARIANT_SX1272) 

#define RADIO_INIT_REGISTERS_VALUE                \
{                                                 \
{ MODEM_FSK , REG_LNA                , 0x23 },\
{ MODEM_FSK , REG_RXCONFIG           , 0x1E },\
{ MODEM_FSK , REG_RSSICONFIG         , 0xD2 },\
{ MODEM_FSK , REG_AFCFEI             , 0x01 },\
{ MODEM_FSK , REG_PREAMBLEDETECT     , 0xAA },\
{ MODEM_FSK , REG_OSC                , 0x07 },\
{ MODEM_FSK , REG_SYNCCONFIG         , 0x12 },\
{ MODEM_FSK , REG_SYNCVALUE1         , 0xC1 },\
{ MODEM_FSK , REG_SYNCVALUE2         , 0x94 },\
{ MODEM_FSK , REG_SYNCVALUE3         , 0xC1 },\
{ MODEM_FSK , REG_PACKETCONFIG1      , 0xD8 },\
{ MODEM_FSK , REG_FIFOTHRESH         , 0x8F },\
{ MODEM_FSK , REG_IMAGECAL           , 0x02 },\
{ MODEM_FSK , REG_DIOMAPPING1        , 0x00 },\
{ MODEM_FSK , REG_DIOMAPPING2        , 0x30 },\
{ MODEM_LORA, REG_LR_DETECTOPTIMIZE  , 0x43 },\
{ MODEM_LORA, REG_LR_PAYLOADMAXLENGTH, 0x40 },\
}

#elif defined(BOARDVARIANT_SX1276)

#define RADIO_INIT_REGISTERS_VALUE                \
{                                                 \
    { MODEM_FSK , REG_LNA                , 0x23 },\
    { MODEM_FSK , REG_RXCONFIG           , 0x1E },\
    { MODEM_FSK , REG_RSSICONFIG         , 0xD2 },\
    { MODEM_FSK , REG_AFCFEI             , 0x01 },\
    { MODEM_FSK , REG_PREAMBLEDETECT     , 0xAA },\
    { MODEM_FSK , REG_OSC                , 0x07 },\
    { MODEM_FSK , REG_SYNCCONFIG         , 0x12 },\
    { MODEM_FSK , REG_SYNCVALUE1         , 0xC1 },\
    { MODEM_FSK , REG_SYNCVALUE2         , 0x94 },\
    { MODEM_FSK , REG_SYNCVALUE3         , 0xC1 },\
    { MODEM_FSK , REG_PACKETCONFIG1      , 0xD8 },\
    { MODEM_FSK , REG_FIFOTHRESH         , 0x8F },\
    { MODEM_FSK , REG_IMAGECAL           , 0x02 },\
    { MODEM_FSK , REG_DIOMAPPING1        , 0x00 },\
    { MODEM_FSK , REG_DIOMAPPING2        , 0x30 },\
    { MODEM_LORA, REG_LR_PAYLOADMAXLENGTH, 0x40 },\
}  
#endif

static const RadioRegisters_t RadioRegsInit[] = RADIO_INIT_REGISTERS_VALUE;
size_t sizeOfRadioRegsInit=sizeof( RadioRegsInit );

bool isRadioActive;


//inner function prototyping

static uint8_t HW_DetectBoardType( const HWStatus_t* hwabs );
static void HW_AntSwInit( void );
static void HW_AntSwDeInit( void );
static void HW_SetAntSw( uint8_t opMode );
static void HW_SetAntSwLowPower( bool status );

static void HW_SpiInit( void );
static void HW_SpiWriteAddrDataSize( uint8_t addr, uint8_t *buffer, uint8_t size );
static void HW_SpiReadAddrDataSize( uint8_t addr, uint8_t *buffer, uint8_t size );
void HW_SpiWriteFifo( uint8_t *buffer, uint8_t size );
void HW_SpiReadFifo( uint8_t *buffer, uint8_t size );

static void HW_reset(void);
static void HW_RadioRegistersInit( );

#if defined (BOARDVARIANT_SX1276)
static void HW_rxChainCalibration(void);
#endif






uint8_t HW_init_radio(const HWStatus_t* hwabs)
{
    uint8_t boardType = 0;
	
	HW_reset();
	boardType = HW_DetectBoardType(hwabs);

#if defined (BOARDVARIANT_SX1276)
    HW_rxChainCalibration();
#endif
	
	HW_AntSwInit( );
	HW_SpiInit();
	HW_SetOpMode( RF_OPMODE_SLEEP );

	//interrupt handler function
#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
	dio0.rise(&HAL_hwDone);
	dio1.rise(&HAL_hwTimeout);
#endif

	HW_RadioRegistersInit( );

	HW_SetModem( hwabs->settings.Modem );

#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
	HW_SetPublicNetwork( hwabs->settings.LoRa.PublicNetwork );
	// verify the connection with the board
	while( HW_SpiRead( REG_VERSION ) == 0x00  )
	{
		debug( "  - Radio could not be detected!\n");
		wait( 1 );
	}
#endif

	HW_SetChannel( hwabs->settings.Channel ); 

	return boardType;
}


void HW_Standby(void)
{
	HW_SetOpMode( RF_OPMODE_STANDBY );
}

void HW_Sleep(void)
{
    HW_SetOpMode( RF_OPMODE_SLEEP );
}


void HW_SetModem( RadioModems_t modem )
{
	RadioModems_t current_modem;

#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
	if( ( HW_SpiRead( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_ON ) != 0 ) 
    {
        current_modem = MODEM_LORA;
    }
    else
    {
        current_modem = MODEM_FSK;
    }

    if( current_modem != modem )
    {
	    //HWStatus.settings.Modem = modem;
	    switch( modem )
	    {
		    default:
		    case MODEM_FSK:
                HW_Sleep();
		        HW_SpiWrite( REG_OPMODE, ( HW_SpiRead( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_OFF );

		        HW_SpiWrite( REG_DIOMAPPING1, 0x00 );
		        HW_SpiWrite( REG_DIOMAPPING2, 0x30 );
		        break;
		    case MODEM_LORA:
                HW_Sleep();
		        HW_SpiWrite( REG_OPMODE, ( HW_SpiRead( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON );

		        HW_SpiWrite( REG_DIOMAPPING1, 0x00 );
		        HW_SpiWrite( REG_DIOMAPPING2, 0x00 );
		        break;
	    }
    }
#endif
}


void HW_SetPublicNetwork( bool enable )
{
    HW_SetModem( MODEM_LORA );

    if( enable == true )
    {
#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
        // Change LoRa modem SyncWord
        HW_SpiWrite( REG_LR_SYNCWORD, LORA_MAC_PUBLIC_SYNCWORD );
#endif
    }
    else
    {
#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
        // Change LoRa modem SyncWord
        HW_SpiWrite( REG_LR_SYNCWORD, LORA_MAC_PRIVATE_SYNCWORD );
#endif
    }
}

#if 1
uint8_t HW_isSignalDetected()
{
    uint8_t stat=0;
#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
    stat=HW_SpiRead(REG_LR_MODEMSTAT);
    if((stat&MASK_REG_MODEMSTAT_SIGNAL_DETECTED) == 0x01)
        stat=SIGNAL_DETECTED;
#endif

    return stat;
}
uint8_t HW_isSignalSynchronized()
{
    uint8_t stat=0;
#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
    stat=HW_SpiRead(REG_LR_MODEMSTAT);
    if ((stat&MASK_REG_MODEMSTAT_SYNCHRONIZED) == 0x02)
        stat=SIGNAL_SYNCHRONIZED;
#endif
    
    return stat;
}
uint8_t HW_isRxOngoing()
{
    uint8_t stat=0;
#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
    stat=HW_SpiRead(REG_LR_MODEMSTAT);
    if ((stat&MASK_REG_MODEMSTAT_RX_ONGOING) == 0x04)
        stat=RX_ONGOING;
#endif
    
    return stat;
}
uint8_t isHeaderInfoValid()
{
    uint8_t stat=0;
#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
    stat=HW_SpiRead(REG_LR_MODEMSTAT);
    if ((stat&MASK_REG_MODEMSTAT_HEADER_INFO_VALID) == 0x08)
        stat=HEADER_INFO_VALID;
#endif
    
    return stat;
}
uint8_t isModemClear()
{
    uint8_t stat=0;
#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
    stat=HW_SpiRead(REG_LR_MODEMSTAT);
    if ((~stat&MASK_REG_MODEMSTAT_MODEM_CLEAR) == 0x10)
        stat=MODEM_CLEAR;
#endif
    
    return stat;
}
#endif





//SPI functions
#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
static void HW_SpiInit( void )
{
    hw_nss = 1;
    hw_spi.format( 8,0 );   
    //uint32_t frequencyToSet = 8000000;
	
    wait(0.1); 
}

static void HW_SpiWriteAddrDataSize( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    hw_nss = 0;
    hw_spi.write( addr | 0x80 );
    for( i = 0; i < size; i++ )
    {
        hw_spi.write( buffer[i] );
    }
    hw_nss = 1;
}

static void HW_SpiReadAddrDataSize( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    hw_nss = 0;
    hw_spi.write( addr & 0x7F );
    for( i = 0; i < size; i++ )
    {
        buffer[i] = hw_spi.write( 0 );
    }
    hw_nss = 1;
}

void HW_SpiWrite( uint8_t addr, uint8_t data )
{
    HW_SpiWriteAddrDataSize( addr, &data, 1 );
}

uint8_t HW_SpiRead( uint8_t addr )
{
    uint8_t data;
	
    HW_SpiReadAddrDataSize( addr, &data, 1 );
	
    return data;
}

void HW_SpiWriteFifo( uint8_t *buffer, uint8_t size )
{
    HW_SpiWriteAddrDataSize( 0, buffer, size );
}

void HW_SpiReadFifo( uint8_t *buffer, uint8_t size )
{
    HW_SpiReadAddrDataSize( 0, buffer, size );
}
#endif

//register map initialization
static void HW_RadioRegistersInit( ) 
{
    uint8_t i = 0;
    for( i = 0; i < sizeOfRadioRegsInit / sizeof( RadioRegisters_t ); i++ )
    {
        HW_SetModem( RadioRegsInit[i].Modem );
        HW_SpiWrite( RadioRegsInit[i].Addr, RadioRegsInit[i].Value );
    }
}

//RF HW control functions
void HW_SetChannel( uint32_t freq )
{
#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
    freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );
    HW_SpiWrite( REG_FRFMSB, ( uint8_t )( ( freq >> 16 ) & 0xFF ) );
    HW_SpiWrite( REG_FRFMID, ( uint8_t )( ( freq >> 8 ) & 0xFF ) );
    HW_SpiWrite( REG_FRFLSB, ( uint8_t )( freq & 0xFF ) );
#endif
}

void HW_SetOpMode( uint8_t opMode )
{
#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
    if( opMode == RF_OPMODE_SLEEP )
    {
        HW_SetAntSwLowPower( true );
    }
    else
    {
        HW_SetAntSwLowPower( false );
        HW_SetAntSw( opMode );
    }
#endif

#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
    HW_SpiWrite( REG_OPMODE, ( HW_SpiRead( REG_OPMODE ) & RF_OPMODE_MASK ) | opMode );
#endif
}

//HW reset
static void HW_reset(void)
{
#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
	hw_reset.output( );
	hw_reset = 0;
	wait_ms( 1 );
	hw_reset.input( );
	wait_ms( 6 );
#endif
}


static uint8_t HW_DetectBoardType( const HWStatus_t* hwabs )
{
	uint8_t res = hwabs->boardConnected;
#if( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )	
    if( hwabs->boardConnected == UNKNOWN )
    {
        hw_AntSwitch.input( );
        wait_ms( 1 );
        if( hw_AntSwitch == 1 )
        {
#if defined(BOARDVARIANT_SX1272)
            res = SX1272MB1DCS;
#elif defined(BOARDVARIANT_SX1276)
            res = SX1276MB1LAS;
#endif
        }
        else
        {
#if defined(BOARDVARIANT_SX1272)
            res = SX1272MB2XAS;
#elif defined(BOARDVARIANT_SX1276)
            res = SX1276MB1MAS;
#endif
        }
        hw_AntSwitch.output( );
        wait_ms( 1 );
    }
#endif
    return ( res );
}


//Antenna-related functions
static void HW_AntSwInit( void )
{
#if ( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
    hw_AntSwitch = 0;
#endif
}

static void HW_AntSwDeInit( void )
{
#if ( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
    hw_AntSwitch = 0;
#endif
}

static void HW_SetAntSw( uint8_t opMode )
{
#if ( defined (BOARDVARIANT_SX1276) || defined(BOARDVARIANT_SX1272) )
    switch( opMode )
    {
    case RFLR_OPMODE_TRANSMITTER:
        hw_AntSwitch = 1;
        break;
    case RFLR_OPMODE_RECEIVER:
    case RFLR_OPMODE_RECEIVER_SINGLE:
    case RFLR_OPMODE_CAD:
        hw_AntSwitch = 0;
        break;
    default:
        hw_AntSwitch = 0;
        break;
    }
#endif
}


static void HW_SetAntSwLowPower( bool status )
{
    if( isRadioActive != status )
    {
        isRadioActive = status;
    
        if( status == false )
        {
            HW_AntSwInit( );
        }
        else
        {
            HW_AntSwDeInit( );
        }
    }
}



#if defined (BOARDVARIANT_SX1276)
static void HW_rxChainCalibration(void)
{
    uint8_t regPaConfigInitVal;
    uint32_t initialFreq;

    // Save context
    regPaConfigInitVal = HW_SpiRead( REG_PACONFIG );
    initialFreq = ( double )( ( ( uint32_t )HW_SpiRead( REG_FRFMSB ) << 16 ) |
                              ( ( uint32_t )HW_SpiRead( REG_FRFMID ) << 8 ) |
                              ( ( uint32_t )HW_SpiRead( REG_FRFLSB ) ) ) * ( double )FREQ_STEP;

    // Cut the PA just in case, RFO output, power = -1 dBm
    HW_SpiWrite( REG_PACONFIG, 0x00 );

    // Launch Rx chain calibration for LF band
    HW_SpiWrite ( REG_IMAGECAL, ( HW_SpiRead( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( HW_SpiRead( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Sets a Frequency in HF band
    HW_SetChannel( 868000000 );

    // Launch Rx chain calibration for HF band
    HW_SpiWrite ( REG_IMAGECAL, ( HW_SpiRead( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( HW_SpiRead( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Restore context
    HW_SpiWrite( REG_PACONFIG, regPaConfigInitVal );
    HW_SetChannel( initialFreq );
}
#endif
