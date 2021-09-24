#include "PHYHAL_interface.h"
#include "Common_HAL.h"

int8_t HAL_cmd_transmit(uint8_t* buffer, uint8_t buffer_size)
{
    if(buffer == NULL)
    {
        debug("Failed to accept HAL cmd: packet is empty!\n");
        return -1;
    }
    if(buffer_size > 64)
    {
        debug("Failed to accept HAL cmd: packet size is too large! (%i)\n", buffer_size);
        return -2;
    }
	
	return HAL_capi_startTx( buffer, buffer_size );
}

HALtype_txCfgErr HAL_cmd_SetTxConfig( int8_t power, HALenum_bw_e bandwidth, HALenum_dr_e datarate, HALenum_cr_e coderate, uint16_t preambleLen,uint32_t frequency )
{
    debug_if(DBGMSG_HAL, "[HALTX] reserving resource for txConfig\n");
    if (HAL_capi_reserveResource(HAL_ENT_TX) != 0)
    {
        return HAL_TX_CRBUSY_CFG_ERR;
    }

	HAL_capi_CfgLoRaPower(HAL_ENT_TX, power);
	HAL_capi_CfgLoRaBandwidth(HAL_ENT_TX, (uint32_t)bandwidth);
	HAL_capi_CfgLoRaCoderate(HAL_ENT_TX, (uint8_t)coderate);
	HAL_capi_CfgLoRaPreambleLen(HAL_ENT_TX, preambleLen);
    HAL_capi_CfgLoRaFrequencyChannel(HAL_ENT_TX, frequency);

	HAL_capi_CfgLoRaDatarate(HAL_ENT_TX, datarate);

    if( ( ( bandwidth == HAL_LRBW_125) && ( ( datarate == HAL_LRDatarate_SF11 ) || ( datarate == HAL_LRDatarate_SF12 ) ) ) ||
        ( ( bandwidth == HAL_LRBW_250 ) && ( datarate == HAL_LRDatarate_SF12 ) ) )
    {
        HAL_capi_CfgLoRaLowDROpt(HAL_ENT_TX, 0x01);
    }
    else
    {
        HAL_capi_CfgLoRaLowDROpt(HAL_ENT_TX, 0x00);
    }


    if (HAL_capi_SetTxConfig() == -1)
    {
        return HAL_TX_CRBUSY_CFG_ERR;
    }

    return HAL_TX_NO_ERR;
}


HALtype_txCfgErr HAL_cmd_SetRfTxPower( int8_t power )
{
    //validity check code (TBD)
    debug_if(DBGMSG_HAL, "[HALTX] reserving resource for rfTxConfig\n");
    if (HAL_capi_reserveResource(HAL_ENT_TX) != 0)
    {
        return HAL_TX_CRBUSY_CFG_ERR;
    }
    
	HAL_capi_CfgLoRaPower(HAL_ENT_TX, power);
    if (HAL_capi_setRfTxPower(power) != 0)
    {
        return HAL_TX_POWER_CFG_ERR;
    }

    return HAL_TX_NO_ERR;
}



HALtype_txCfgErr HAL_tapi_TxInitConfig (const HWStatus_t hwabs)
{
    
    debug_if(DBGMSG_HAL, "[HALTX] reserving resource for init txConfig\n");
    if (HAL_capi_reserveResource(HAL_ENT_TX) != 0)
    {
        return HAL_TX_CRBUSY_CFG_ERR;
    }
	if (HAL_capi_setRfTxPower(hwabs.settings.LoRa.Power) != 0)
    {
        return HAL_TX_POWER_CFG_ERR;
    }

    debug_if(DBGMSG_HAL, "[HALTX] reserving resource for init txConfig 2 \n");
    if (HAL_capi_reserveResource(HAL_ENT_TX) != 0)
    {
        return HAL_TX_CRBUSY_CFG_ERR;
    }

    if( hwabs.settings.LoRa.Datarate > 12 )
    {
    	debug("[HAL][WARNING] data rate %i is truncated", hwabs.settings.LoRa.Datarate);
        HAL_capi_CfgLoRaDatarate(HAL_ENT_TX, 12);
    }
    else if( hwabs.settings.LoRa.Datarate < 6 )
    {
    	debug("[HAL][WARNING] data rate %i is truncated", hwabs.settings.LoRa.Datarate);
        HAL_capi_CfgLoRaDatarate(HAL_ENT_TX, 6);
    }
    if( ( ( hwabs.settings.LoRa.Bandwidth == 0 ) && ( ( hwabs.settings.LoRa.Datarate == 11 ) || ( hwabs.settings.LoRa.Datarate == 12) ) ) ||
        ( ( hwabs.settings.LoRa.Bandwidth == 1 ) && ( hwabs.settings.LoRa.Datarate == 12 ) ) )
    {
        HAL_capi_CfgLoRaLowDROpt(HAL_ENT_TX, 0x01);
    }
    else
    {
        HAL_capi_CfgLoRaLowDROpt(HAL_ENT_TX, 0x00);
    }


    if (HAL_capi_SetTxConfig() == -1)
    {
        return HAL_TX_CRBUSY_CFG_ERR;
    }

    return HAL_TX_NO_ERR;
}