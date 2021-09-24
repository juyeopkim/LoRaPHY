#include "mbed.h"
#include "LoRa_RMAC_API.h"
#include "LoRa_PHY_API.h"
#define BUF_SIZE 32


Timeout tx;


uint8_t msg_pended = 0;

void handleMonitor(){
    indicateRMACMonitoring();
}

int main(void){
    debug("Start sending message\n");
    RMAC_FSM_start();
    tx.attach(&handleMonitor, 5);
    while(1){
        
            RMAC_FSM_run();
            PHY_FSM_run();
            if(getRMACState() == 0 && getRepeatFlag()==1)
            {
                tx.attach(&handleMonitor, 5);
            }
    }
}

