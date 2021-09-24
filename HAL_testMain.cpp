#include "mbed.h"
#include "PHYHAL_interface.h"

#define BUF_SIZE 32

#define DBGMSG_MAIN		0

Timeout tx;
uint8_t msg[BUF_SIZE] = "Hello world!";
uint8_t msg_pended = 0;

void handler_txTimeout(void)
{
	msg_pended = 1;
}

void TxCnfFunc(HALtype_txCmdErr res)
{
	debug_if(DBGMSG_MAIN, "MAIN : TX result : %i\n", res);
}

void RxCnfFunc(HALtype_rxCmdErr res, uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
	if (res == HAL_RX_CNF_DONE)
	{
		debug("MAIN : PDU comes :%s, (size:%i, rssi:%i, snr:%i)\n", payload, size, rssi, snr);
	}
	else if (res == HAL_RX_CNF_TIMEOUT)
	{
		debug_if(DBGMSG_MAIN, "MAIN : RX timeout\n");
	}
	else if (res == HAL_RX_CNF_CRCERROR)
	{
		debug_if(DBGMSG_MAIN, "MAIN : RX CRC ERROR!!\n");
	}
}

int main(void)
{
	debug("	   Starting simple TX/RX program...\n");
	debug( "	   Initializing LoRa H/W fixed \n" );
	HAL_cmd_init(&TxCnfFunc, &RxCnfFunc);
	  
	debug( "	   Starting TX/RX loop\n" );
	tx.attach(&handler_txTimeout, 3);
	HAL_cmd_receive();
	
	while(1)
	{
		if (msg_pended == 1)
		{
			debug( " -> sending a message (msg_pended)\n" );
			HAL_cmd_transmit(msg, BUF_SIZE);
			
			tx.attach(&handler_txTimeout, 3);
			msg_pended = 0;
		}
		else if(HAL_isRfActive()==false)
		{
			HAL_cmd_receive();
		}
	}
}

