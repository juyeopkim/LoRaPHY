#include "PHYHAL_interface.h"
#include "Common_HAL.h"



int8_t HAL_cmd_receive(void)
{
	uint32_t timeout = 0;
    
    debug_if(DBGMSG_HAL, "[HAL] RX start -----\n");

	return HAL_capi_startRx(timeout);
}

HALtype_rxCfgErr HAL_cmd_SetRxConfig( HALenum_bw_e bandwidth, HALenum_dr_e datarate, HALenum_cr_e coderate, uint16_t preambleLen, uint32_t frequency, uint16_t rxTime)
{
	if (rxTime >= RX_MAX_RXSYMBOLTIME)
	{
		debug("[HAL] error on config parameter : time is too long! (%i, must be less than 1024. if you want longer one, then do continous mode (rxTime = 0) )\n", rxTime);
		return HAL_RX_RXTIME_CFG_ERR;
	}
    debug_if(DBGMSG_HAL, "[HALRX] reserving resource for rxConfig1\n");
    if (HAL_capi_reserveResource(HAL_ENT_RX) != 0)
    {
        return HAL_RX_CRBUSY_CFG_ERR;
    }

	HAL_capi_CfgLoRaBandwidth(HAL_ENT_RX, (uint32_t)bandwidth);	
	HAL_capi_CfgLoRaCoderate(HAL_ENT_RX, (uint8_t)coderate);
	HAL_capi_CfgLoRaPreambleLen(HAL_ENT_RX, preambleLen);
    HAL_capi_CfgLoRaFrequencyChannel(HAL_ENT_RX, frequency);
	HAL_capi_CfgLoRaDatarate(HAL_ENT_RX, datarate);

    if( ( ( bandwidth == HAL_LRBW_125 ) && ( ( datarate == HAL_LRDatarate_SF11 ) || ( datarate == HAL_LRDatarate_SF12 ) ) ) ||
        ( ( bandwidth == HAL_LRBW_250 ) && ( datarate == HAL_LRDatarate_SF12) ) )
    {
        HAL_capi_CfgLoRaLowDROpt(HAL_ENT_RX, 0x01);
    }
    else
    {
        HAL_capi_CfgLoRaLowDROpt(HAL_ENT_RX, 0x00);
    }

	if (rxTime == 0) //set as continuous
	{
		HAL_capi_CfgLoRaRxContinuous(HAL_ENT_RX, true);
	}
	else
	{
		HAL_capi_CfgLoRaRxContinuous(HAL_ENT_RX, false);
		HAL_capi_CfgLoRaRxTime(HAL_ENT_RX, rxTime);
	}

	if (HAL_capi_SetRxConfig() == -1)
    {
        return HAL_RX_CRBUSY_CFG_ERR;
    }
	return HAL_RX_NO_ERR;
}




void HAL_rapi_onRxTimeoutIrq(void)
{
	
}

HALtype_rxCfgErr HAL_rapi_RxInitConfig (const HWStatus_t hwabs)
{
    
    debug_if(DBGMSG_HAL, "[HALRX] reserving resource for init rxConfig\n");
    if (HAL_capi_reserveResource(HAL_ENT_RX) != 0)
    {
        return HAL_RX_CRBUSY_CFG_ERR;
    }
    if( hwabs.settings.LoRa.Datarate > 12 )
    {
        debug("[HAL][WARNING] data rate %i is truncated", hwabs.settings.LoRa.Datarate);
        HAL_capi_CfgLoRaDatarate(HAL_ENT_RX, 12);
    }
    else if( hwabs.settings.LoRa.Datarate < 6 )
    {
        debug("[HAL][WARNING] data rate %i is truncated", hwabs.settings.LoRa.Datarate);
        HAL_capi_CfgLoRaDatarate(HAL_ENT_RX, 6);
    }

    if( ( ( hwabs.settings.LoRa.Bandwidth == 0 ) && ( ( hwabs.settings.LoRa.Datarate == 11 ) || ( hwabs.settings.LoRa.Datarate == 12) ) ) ||
        ( ( hwabs.settings.LoRa.Bandwidth == 1 ) && ( hwabs.settings.LoRa.Datarate == 12 ) ) )
    {
        HAL_capi_CfgLoRaLowDROpt(HAL_ENT_RX, 0x01);
    }
    else
    {
        HAL_capi_CfgLoRaLowDROpt(HAL_ENT_RX, 0x00);
    }


    if (HAL_capi_SetRxConfig() == -1)
    {
        return HAL_RX_CRBUSY_CFG_ERR;
    }

    return HAL_RX_NO_ERR;
}