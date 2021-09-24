#include "Common_API.h"
#include "Common_HHI.h"
#include "Tx_HHI.h"
#include "Rx_HHI.h"
#include "Rx_HAL.h"
#include "Tx_HAL.h"
#include "PHYHAL_types.h"
#include "HAL_FSM.h"

#define RX_BUFFER_SIZE                  256

// -- declarations
//1. ---------------------- Service Access Point related resources
static void (*txCnfFunc)(HALtype_txCmdErr) = NULL; 									//HAL_TX_CNF primitive
static void (*rxCnfFunc)(HALtype_rxCmdErr,uint8_t*,uint16_t,int16_t,int8_t) = NULL; //HAL_RX_CNF primitive




//2. ---------------------- inner resources and controller
static HWStatus_t HWStatus; 	//HW abstraction
static Timeout wdTimer;			//watchdog timer
static uint8_t *rxtxBuffer;		//TX / RX buffer
static HALtype_entity resOwner = HAL_ENT_NONE;	



//3. static functions ---------------------------
static int HAL_init_radio(void);
static void HAL_Sleep( void );
static void HAL_clearBuffer();
static void HAL_swTimeout( void );
static int8_t HAL_releaseResource(HALtype_entity entity);




// -- function definitions

/* 1. primitive functions (PHY-HAL)*/
int HAL_cmd_init(void (*HAL_tx_cnf)(HALtype_txCmdErr), void (*HAL_rx_cnf)(HALtype_rxCmdErr,uint8_t*,uint16_t,int16_t,int8_t))
{
	int radioInitRes;

	//initialization validity check
	if (HWStatus.settings.State != RF_NULL)
	{
		debug("[HAL] [ERROR] HAL initialization failed : common resource state (%i) is invalid\n", HWStatus.settings.State);
		return -1; 
	}

	//primitive handler function
	txCnfFunc = HAL_tx_cnf;
	rxCnfFunc = HAL_rx_cnf;

    //initialize lower layer
	radioInitRes = HAL_init_radio();
	
	return radioInitRes;
}

int HAL_cmd_Sleep()
{
	//HAL FSM operation & check
	if (HAL_FSM_do(HAL_EVENT_ABORT) == HAL_FSMRES_CANNOTHAPPEN)
	{
		debug("[HAL] [ERROR] FSM fail during sleeping\n");
		return -1;
	}

    HAL_Sleep();

	//HAL FSM operation
	HAL_FSM_do(HAL_EVENT_CFGEND);

	return 0;
}


/* 2. HAL API functions  (Common API, interface between RX_HAL or TX_HAL) */

/* ---------------- Common Resource Reservation ----------------*/
int8_t HAL_capi_reserveResource(HALtype_entity entity)
{
	int8_t res=-1;

	if (entity == HAL_ENT_NONE)
	{
		debug("[HAL] [WARNING] reserving common resource with entity NONE.\n");
		return -1;
	}
	if (resOwner != HAL_ENT_NONE)
	{
		debug("[HAL] [ERROR] Failed to reserve resource : already occupied by %i\n", resOwner);
		return -1;
	}

	if (HAL_FSM_do(HAL_EVENT_CFGREQ) == HAL_FSMRES_SUCCESS)
	{
		resOwner = entity;
		res = 0;
	}
	else
	{
		debug("[HAL] [ERROR] FSM fail during resource reservation\n");
	}
	
	return res;
}


/* ---------------- abstract configuration functions ----------------*/
int8_t HAL_capi_CfgLoRaLowDROpt(HALtype_entity entity, bool input)
{
	if (HAL_FSM_checkState() != HAL_FSMST_CFGING ||
		resOwner != entity)
	{
		debug("[HAL][ERROR] Failed to set parameter LowDROpt - state mismatch or res is not owned by %i (%i)\n", entity, resOwner);
		return -1;
	}

	debug_if(DBGMSG_HAL, "[HAL] config LoRa.LowDatarateOptimze : %i\n", input);
	HWStatus.settings.LoRa.LowDatarateOptimize = input;

	return 0;
}

int8_t HAL_capi_CfgLoRaDatarate(HALtype_entity entity, uint32_t input)
{
	if (HAL_FSM_checkState() != HAL_FSMST_CFGING ||
		resOwner != entity)
	{
		debug("[HAL][ERROR] Failed to set parameter DataRate - state mismatch or res is not owned by %i (%i)\n", entity, resOwner);
		return -1;
	}

	debug_if(DBGMSG_HAL, "[HAL] config LoRa.Datarate : %i\n", input);
	HWStatus.settings.LoRa.Datarate = input;

	return 0;
}


int8_t HAL_capi_CfgLoRaBandwidth(HALtype_entity entity, uint32_t input)
{
	if (HAL_FSM_checkState() != HAL_FSMST_CFGING ||
		resOwner != entity)
	{
		debug("[HAL][ERROR] Failed to set parameter BW - state mismatch or res is not owned by %i (%i)\n", entity, resOwner);
		return -1;
	}

	debug_if(DBGMSG_HAL, "[HAL] config LoRa.Bandwidth : %i\n", input);
	HWStatus.settings.LoRa.Bandwidth = input;

	return 0;
}

int8_t HAL_capi_CfgLoRaPower(HALtype_entity entity, uint32_t input)
{
	if (HAL_FSM_checkState() != HAL_FSMST_CFGING ||
		resOwner != entity)
	{
		debug("[HAL][ERROR] Failed to set parameter Power - state mismatch or res is not owned by %i (%i)\n", entity, resOwner);
		return -1;
	}

	debug_if(DBGMSG_HAL, "[HAL] config LoRa.Power : %i\n", input);
	HWStatus.settings.LoRa.Power = input;

	return 0;
}

int8_t HAL_capi_CfgLoRaCoderate(HALtype_entity entity, int8_t input)
{
	if (HAL_FSM_checkState() != HAL_FSMST_CFGING ||
		resOwner != entity)
	{
		debug("[HAL][ERROR] Failed to set parameter CodeRate - state mismatch or res is not owned by %i (%i)\n", entity, resOwner);
		return -1;
	}

	debug_if(DBGMSG_HAL, "[HAL] config LoRa.coderate : %i\n", input);
	HWStatus.settings.LoRa.Coderate = input;

	return 0;
}

int8_t HAL_capi_CfgLoRaPreambleLen(HALtype_entity entity, uint16_t input)
{
	if (HAL_FSM_checkState() != HAL_FSMST_CFGING ||
		resOwner != entity)
	{
		debug("[HAL][ERROR] Failed to set parameter PreambleLen - state mismatch or res is not owned by %i (%i)\n", entity, resOwner);
		return -1;
	}

	debug_if(DBGMSG_HAL, "[HAL] config LoRa.preambleLen : %i\n", input);
	HWStatus.settings.LoRa.PreambleLen = input;

	return 0;
}

int8_t HAL_capi_CfgLoRaFrequencyChannel(HALtype_entity entity, uint32_t frequency)
{
	if (HAL_FSM_checkState() != HAL_FSMST_CFGING ||
		resOwner != entity)
	{
		debug("[HAL][ERROR] Failed to set parameter Freq CH - state mismatch or res is not owned by %i (%i)\n", entity, resOwner);
		return -1;
	}

    debug_if(DBGMSG_HAL, "[HAL] config LoRa.Channel : %i\n", frequency);
    HWStatus.settings.Channel=frequency;

	return 0;
}

int8_t HAL_capi_CfgLoRaRxContinuous(HALtype_entity entity, bool option)
{
	if (HAL_FSM_checkState() != HAL_FSMST_CFGING ||
		resOwner != entity)
	{
		debug("[HAL][ERROR] Failed to set parameter RXCont. - state mismatch or res is not owned by %i (%i)\n", entity, resOwner);
		return -1;
	}

    debug_if(DBGMSG_HAL, "[HAL] config LoRa.RxContinuous : %i\n", option);
    HWStatus.settings.LoRa.RxContinuous=option;

	return 0;
}

int8_t HAL_capi_CfgLoRaRxTime(HALtype_entity entity, uint16_t time)
{
	if (HAL_FSM_checkState() != HAL_FSMST_CFGING ||
		resOwner != entity)
	{
		debug("[HAL][ERROR] Failed to set parameter RxTime - state mismatch or res is not owned by %i (%i)\n", entity, resOwner);
		return -1;
	}

	debug_if(DBGMSG_HAL, "[HAL] config rxSymbTimeout : %i\n", time);
	HWStatus.settings.LoRa.rxSymbTimeout=time;

	return 0;
}

/* ---------------- H/W configuration functions ----------------*/
int8_t HAL_capi_SetTxConfig()
{
	if (HAL_FSM_checkState() != HAL_FSMST_CFGING ||
		resOwner != HAL_ENT_TX)
	{
		debug("[HAL][ERROR] Failed to start TX configuration : state mismatch or resource is not owned by TX (%i)\n", resOwner);
		return -1;
	}

	debug("[HAL][HW EVENT] HW config for TX\n");	
	HW_SetModem( MODEM_LORA );
	HW_SetTxConfig(HWStatus);

	HAL_releaseResource(HAL_ENT_TX);

	return 0;
}

int8_t HAL_capi_SetRxConfig()
{
	if (HAL_FSM_checkState() != HAL_FSMST_CFGING ||
		resOwner != HAL_ENT_RX)
	{
		debug("[HAL][ERROR] Failed to start RX configuration : state mismatch or resource is not owned by RX (%i)\n", resOwner);
		return -1;
	}

	debug("[HAL][HW EVENT] HW config for RX\n");
    HW_SetModem( MODEM_LORA);
	HW_SetRxConfig(HWStatus);

	HAL_releaseResource(HAL_ENT_RX);

	return 0;
}

int8_t HAL_capi_setRfTxPower(int8_t power)
{
	if (HAL_FSM_checkState() != HAL_FSMST_CFGING ||
		resOwner != HAL_ENT_TX)
	{
		debug("[HAL][ERROR] Failed to start RX configuration : state mismatch or resource is not owned by TX (%i)\n", resOwner);
		return -1;
	}

	HW_SetRfTxPower(HWStatus, power);

	HAL_releaseResource(HAL_ENT_TX);

	return 0;
}

int8_t HAL_capi_startRx(uint32_t timeout)
{
	if (HAL_FSM_do(HAL_EVENT_RUN) == HAL_FSMRES_CANNOTHAPPEN)
	{
		debug("[HAL] [ERROR] FSM fail during RXstart\n");
		return -1;
	}

	HAL_clearBuffer();

	HW_Rx( HWStatus ); // HHI code ->header not included
	HWStatus.settings.State = RF_RX_RUNNING;//HW_setState
	
	if( timeout != 0 )
	{
		wdTimer.attach_us( HAL_swTimeout , timeout * 1e3 ); //x
	}

	return 0;
}


/* ---------------- H/W run (action) functions ----------------*/
int8_t HAL_capi_startTx( uint8_t *buffer, uint8_t size )
{
    uint32_t txTimeout = 0;

	//RX ABORT first
	if (HWStatus.settings.State == RF_TX_RUNNING ||
		(HAL_FSM_do(HAL_EVENT_ABORT) == HAL_FSMRES_CANNOTHAPPEN))
	{
		debug("[HAL] [ERROR] FSM fail rx abort during txStart\n");
		return -1;
	}

	HAL_Sleep();
	wait_ms( 10 );

	HAL_FSM_do(HAL_EVENT_CFGEND);
	if (HAL_FSM_do(HAL_EVENT_RUN) == HAL_FSMRES_CANNOTHAPPEN)
	{
		debug("[HAL] [ERROR] FSM fail during txStart\n");
		return -1;
	}

    switch(HWStatus.settings.Modem)
	{
		case MODEM_FSK:
			{
				debug("modem state is invalid(FSK, which is not supported now)\n");
				return -1;
			}

		case MODEM_LORA:
			{
				HW_setLoRaIqInverted(HWStatus.settings.LoRa.IqInverted);			
				HWStatus.settings.LoRaPacketHandler.Size = size;
				HW_setTxPayload(buffer, size);

				txTimeout = HWStatus.settings.LoRa.TxTimeout;
			}
			break;
    }

    HW_Tx( HWStatus, txTimeout );

	HWStatus.settings.State = RF_TX_RUNNING;
    wdTimer.attach_us(HAL_swTimeout, txTimeout*1e3);


	return 0;
}







/* 3. Interrupt handler functions (called by HHI modules as HW IRQ comes in) */
//HW timeout interrupt handler
void HAL_hwTimeout(void)
{
	RadioState state = HWStatus.settings.State;

	if (state == RF_RX_RUNNING)
	{
		// Sync time out
		HW_clearRxTimeoutIrq();
		HAL_Sleep();
		
		if (HAL_FSM_do(HAL_EVENT_RUNEND) != HAL_FSMRES_CANNOTHAPPEN)
		{
			rxCnfFunc(HAL_RX_CNF_TIMEOUT, NULL, 0, 0, 0);
		}
		else
		{
			debug("[HAL] [ERROR] FSM fail during HW time handler\n");
		}
		
	}
}

//HW done interrupt handler
void HAL_hwDone(void)
{
	RadioState state = HWStatus.settings.State;
	uint16_t size;
	int16_t rssi;
	int8_t snr;
	rxCrc_e crcres;

	if (state == RF_RX_RUNNING)
	{
        // Clear Irq
        HW_clearRxIrq();
		crcres = HW_checkCRC();
		
        if( crcres == rxcrc_nok )
        {
            // Clear Irq
            HW_clearRxPayloadCrc();
			
			if( HWStatus.settings.LoRa.RxContinuous == false )
			{
				HAL_Sleep();
				if (HAL_FSM_do(HAL_EVENT_RUNEND) != HAL_FSMRES_CANNOTHAPPEN)
				{
					rxCnfFunc(HAL_RX_CNF_CRCERROR, NULL, 0, 0, 0);
				}
				else
				{
					debug("[HAL] [ERROR] FSM fail during HW done handler (RX, CRCNOK)\n");
				}
			}
			else
			{
				if (HAL_FSM_checkState() == HAL_FSMST_RUN)
				{
					rxCnfFunc(HAL_RX_CNF_CRCERROR, NULL, 0, 0, 0);
				}
				else
				{
					debug("[HAL] [ERROR] FSM fail during HW done handler (RX, CRCNOK)\n");
				}
			}
        }
		else
		{
			snr = HW_readSnr();
            rssi = HW_readRssi(HWStatus, snr);
			size = HW_getRxPayload(rxtxBuffer);
	        

	        if( HWStatus.settings.LoRa.RxContinuous == false )
            {
                HAL_Sleep();
				if (HAL_FSM_do(HAL_EVENT_RUNEND) != HAL_FSMRES_CANNOTHAPPEN)
				{
					rxCnfFunc(HAL_RX_CNF_DONE, rxtxBuffer, size, rssi, snr);
				}
				else
				{
					debug("[HAL] [ERROR] FSM fail during HW done handler (RX, CRCOK)\n");
				}
            }
			else
			{
				if (HAL_FSM_checkState() == HAL_FSMST_RUN)
				{
					rxCnfFunc(HAL_RX_CNF_DONE, rxtxBuffer, size, rssi, snr);
				}
				else
				{
					debug("[HAL] [ERROR] FSM fail during HW done handler (RX, CRCOK)\n");
				}
			}
		}
    }
	else if (state == RF_TX_RUNNING)
	{
	    wdTimer.detach();
        // TxDone interrupt
        // Clear Irq
        HW_clearTxIrq();
		
		HAL_Sleep();

		if (HAL_FSM_do(HAL_EVENT_RUNEND) != HAL_FSMRES_CANNOTHAPPEN)
		{
			txCnfFunc(HAL_TX_CNF_DONE);
		}
		else
		{
			debug("[HAL] [ERROR] FSM fail during HW done handler (TX)\n");
		}
	}
	else
	{
		debug("[ERROR] state is weird :%i\n", state);
	}
}










/* ---------------- 4. inner static functions ----------------*/
static int HAL_init_radio()
{
	debug( "\n	   Initializing Radio Interrupt handlers \n" );

	rxtxBuffer = (uint8_t*)malloc(RX_BUFFER_SIZE*sizeof(uint8_t));
	if (rxtxBuffer == NULL)
	{
		debug("[ERROR] failed in initiating radio : memory alloc failed\n");
		return -2;
	}

	//initialize the HW abstraction by the initial values in API header
	HWStatus.settings.Channel = HAL_INIT_RF_FREQUENCY;
	HWStatus.settings.Modem = HAL_INIT_MODEM;


	if (HWStatus.settings.Modem == MODEM_LORA)
	{
	    HWStatus.settings.LoRa.FixLen = HAL_INIT_LORA_FIXED_LENGTH;
	    HWStatus.settings.LoRa.FreqHopOn = HAL_INIT_LORA_FREQHOP;
	    HWStatus.settings.LoRa.HopPeriod = HAL_INIT_LORA_HOPPERIOD;
	    HWStatus.settings.LoRa.CrcOn = HAL_INIT_LORA_CRCON;
	    HWStatus.settings.LoRa.IqInverted = HAL_INIT_LORA_IQINV;
	    HWStatus.settings.LoRa.TxTimeout = HAL_INIT_LORA_TXTIMEOUT;
		HWStatus.settings.LoRa.Power = HAL_INIT_LORA_TXPOWER;
		HWStatus.settings.LoRa.Bandwidth = HAL_INIT_LORA_BANDWIDTH;
		HWStatus.settings.LoRa.Datarate = HAL_INIT_LORA_DATARATE;
		HWStatus.settings.LoRa.Coderate = HAL_INIT_LORA_CODERATE;
		HWStatus.settings.LoRa.PreambleLen = HAL_INIT_LORA_PREAMBLELEN;
		HWStatus.settings.LoRa.rxSymbTimeout = HAL_INIT_LORA_RXSYMBTIMEOUT;
		HWStatus.settings.LoRa.PayloadLen = HAL_INIT_LORA_PAYLOADLEN;
		HWStatus.settings.LoRa.RxContinuous = HAL_INIT_LORA_RXCONTINUOUS;
		HWStatus.settings.LoRa.PublicNetwork = HAL_INIT_LORA_PUBLIC_NETWORK;
	}
	else
	{
		debug("[HAL][ERROR] failed to initialize : currently %i modem is disabled!\n", HAL_INIT_MODEM);
		return -1;
	}

	HWStatus.boardConnected = UNKNOWN;
	HWStatus.boardConnected = HW_init_radio(&HWStatus);

	debug("Board connected : %i\n", HWStatus.boardConnected);
	 
	//TX initial config
	HAL_tapi_TxInitConfig (HWStatus);
	
	//RX initial config
	HAL_rapi_RxInitConfig (HWStatus);
		
    HWStatus.settings.State = RF_IDLE;
	
	
    return 0;
}

static void HAL_Sleep( void )
{
    wdTimer.detach( );

	HW_Sleep();
	HWStatus.settings.State = RF_IDLE;
}

static void HAL_clearBuffer()
{
	memset( rxtxBuffer, 0, ( size_t )RX_BUFFER_SIZE );
}

static int8_t HAL_releaseResource(HALtype_entity entity)
{
	int8_t res=-1;

	if (entity == HAL_ENT_NONE)
	{
		debug("[HAL] [WARNING] reserving common resource with entity NONE.\n");
		return -1;
	}

	if (resOwner != entity)
	{
		debug("[HAL] [ERROR] Failed to release resource : occupied by %i, while %i tries to release\n", resOwner, entity);
		return -1;
	}

	if (HAL_FSM_do(HAL_EVENT_CFGEND) == HAL_FSMRES_SUCCESS)
	{
		resOwner = HAL_ENT_NONE;
		res = 0;
	}
	else
	{
		debug("[HAL] [ERROR] FSM fail during resource release\n");
	}
	
	return res;
}

static void HAL_swTimeout( void )
{
    switch(HWStatus.settings.State)
	{
    case RF_RX_RUNNING:
		HAL_rapi_onRxTimeoutIrq();
		HAL_Sleep();
        
		if (HAL_FSM_do(HAL_EVENT_RUNEND) != HAL_FSMRES_CANNOTHAPPEN)
		{
			rxCnfFunc(HAL_RX_CNF_TIMEOUT, NULL, 0, 0, 0);
		}
		else
		{
			debug("[HAL] [ERROR] FSM fail during SW timeout\n");
		}
     
        break;
    case RF_TX_RUNNING:
        // Tx timeout shouldn't happen.
        // But it has been observed that when it happens it is a result of a corrupted SPI transfer
        // it depends on the platform design.
        // 
        // The workaround is to put the radio in a known state. Thus, we re-initialize it.

        // BEGIN WORKAROUND

		#if 0
        // Reset the radio
        HW_reset( );
        // Initialize radio default values
		HW_Sleep();
        HW_RadioRegistersInit( );
        HW_SetModem( MODEM_FSK );
		#endif

		HW_init_radio(&HWStatus);
        // Restore previous network type setting.
        HW_SetPublicNetwork( HWStatus.settings.LoRa.PublicNetwork );
        // END WORKAROUND

		//HW_Sleep();
        HWStatus.settings.State = RF_IDLE;
		HAL_FSM_INIT();
		
		break;
    default:
        break;
    }
}




/* ---------------- 5. utility functions (no interaction/SPI read/write with HW) ----------------*/
bool HAL_isRfActive(void)
{
    if (HWStatus.settings.State == RF_IDLE)
    {
        return false;
    }
    
    return true;
}

uint8_t HAL_getRxStatus()
{
    uint8_t rxStat = 0;
	
	if (HWStatus.settings.State == RF_RX_RUNNING)
		rxStat = 1;
    	
	return rxStat;
}

bool HAL_isSignalDetected()
{
    uint8_t status=0;
    status=HW_isSignalDetected();
    if(status == SIGNAL_DETECTED){
        return true;
    }
    else{
        return false;
    }
}
bool HAL_isSignalSynchronized()
{
    uint8_t status=0;
    status=HW_isSignalSynchronized();
    if (status==SIGNAL_SYNCHRONIZED){
        return true;
    }
    else{
        return false;
    }
}
bool HAL_isRxOngoing()
{
    uint8_t status=0;
    status=HW_isRxOngoing();
    if (status==RX_ONGOING){
        return true;
    }
    else{
        return false;
    }
}
bool HAL_isHeaderInfoValid()
{
    uint8_t status=0;
    status=HW_isHeaderInfoValid();
    if (status==HEADER_INFO_VALID){
        return true;
    }
    else{
        return false;
    }
}
bool HAL_isModemClear()
{
    uint8_t status=0;
    status=HW_isModemClear();
    if (status==MODEM_CLEAR){
        return true;
    }
    else{
        return false;
    }
}
