#include "mbed.h"
#include "Common_API.h"
//#include "LoRa_PHY_API.h"
#define BUF_SIZE 32

Timeout tx;
uint8_t msg[BUF_SIZE] = "Hello world!";


uint8_t msg_pended = 0;


void handler_txTimeout(void)
{
	msg_pended = 1;
}
void TxDoneInterrupt(){
    
}
void TxTimeoutInterrupt(){
    
}
void RxDoneInterrupt(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr){

}
void RxErrorInterrupt(){
    
}
void RxTimeoutInterrupt(){
    
}
int main(void)
{
	debug("	   Starting simple TX/RX program...\n");
	debug( "	   Initializing LoRa H/W fixed \n" );
	HAL_cfg_init(&TxDoneInterrupt,&TxTimeoutInterrupt,&RxDoneInterrupt,&RxTimeoutInterrupt,&RxErrorInterrupt);
	  
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
		else if(HAL_cmd_isActive()==false)
		{
			HAL_cmd_receive();
		}
	}
}

