#include "mbed.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
//#include "Common_API.h"
#include "LoRa_PHY_API.h"
#define BUF_SIZE 16


Timeout tx,u,v,w,y;
Timer t;
uint8_t msg1[BUF_SIZE]="  LoRaMSG";
uint8_t msg2[BUF_SIZE]={};
uint8_t *uplinkMsg;
uint8_t msg3[BUF_SIZE]="[ACK]:";
uint8_t msg_pended = 0;
uint16_t count=0;
uint16_t pktReceptionNum=0;
uint16_t relayPktNum=0;
uint16_t UL, DL=0;
float time1,time2,delayTime;
uint8_t caseScenario=1;
uint32_t delaySize=0,window1Size,window2Size;
uint8_t synchflag=0;
char buffer[10];
/*get packet status*/
uint8_t getPktStat(uint8_t *pkt, uint16_t pktSize){
    char statBuffer[2]={};
    uint8_t statNum=0;
    strncpy(statBuffer,(char *)pkt,2);
    statNum=atoi(statBuffer);
    return statNum;
}
/*set packet status*/
void setPktStat(uint8_t statNum){
    uint8_t *payloadContent;
    uint8_t *statNumStr;
    uint8_t bit1, bit2;
    bit1=statNum/10;
    bit2=statNum%10;
    payloadContent=(uint8_t *)malloc(sizeof(uint8_t)*BUF_SIZE);
    statNumStr=(uint8_t *)malloc(sizeof(uint8_t)*BUF_SIZE);
    memcpy(payloadContent,msg2,BUF_SIZE * sizeof(uint8_t));
    sprintf((char *)statNumStr,"%d%d",bit1,bit2);
    uplinkMsg=(uint8_t*)strcat((char *)statNumStr,(char *)payloadContent+2);
}
void handler_checkSynch(){

    if(HAL_isSignalSynchronized()==true)
        debug("[main][time:%f]synchronize status is 1\n",time1);
    else
        debug("[main]synchronize status is 0\n");
    synchflag=0;
}
void handler_txTimeout(void)
{
	PHY_indicateEvent(FSM);
    debug("\n count:%d\n",++count);
    if(count==151)
    {
        count=1;
        pktReceptionNum=0;
        relayPktNum=0;
        caseScenario+=2;
        #if defined ANALYSIS_ON
            clearVariables();
        #endif
    }
    memcpy(msg2,msg1,BUF_SIZE * sizeof(uint8_t));
    sprintf(buffer, "%d", count);
    strcat((char *)msg2,buffer);
    setPktStat(00);
    time1=t.read();
    debug("[time: %f] %s\n",time1,uplinkMsg);
#if 0
    if(getPktStat(uplinkMsg,BUF_SIZE)==0){
        printf("this is 0\n");
    }
    else{
        printf("this is not 0\n");
    }
#endif
    if(caseScenario%6==0)
    {
        PHY_configUlPayload(uplinkMsg,BUF_SIZE,HAL_LRBW_125,HAL_LRDatarate_SF7,922100000);
    }else if (caseScenario%6==1)
    {
        PHY_configUlPayload(uplinkMsg,BUF_SIZE,HAL_LRBW_125,HAL_LRDatarate_SF8,922100000);
    }else if (caseScenario%6==2)
    {
        PHY_configUlPayload(uplinkMsg,BUF_SIZE,HAL_LRBW_125,HAL_LRDatarate_SF9,922100000);
    }else if (caseScenario%6==3)
    {
        PHY_configUlPayload(uplinkMsg,BUF_SIZE,HAL_LRBW_125,HAL_LRDatarate_SF10,922100000);
    }else if (caseScenario%6==4)
    {
        PHY_configUlPayload(uplinkMsg,BUF_SIZE,HAL_LRBW_125,HAL_LRDatarate_SF11,922100000);
    }else
    {
        PHY_configUlPayload(uplinkMsg,BUF_SIZE,HAL_LRBW_125,HAL_LRDatarate_SF12,922100000);

    }
    
    msg_pended=1;
}
void ACKInterrupt(uint8_t *payload, uint16_t size){
    time2=t.read();
	debug("[time: %f]pkt received ! : %s\n",time2, payload);
    if(strstr((char *)payload,"LoRaMSG")!=NULL){
    if(getPktStat(payload,size)==10)
    {
        if(strstr((char *)payload,buffer)!=NULL)
        {
            ++pktReceptionNum;
            debug("\n\nreception number is : %d \nrelayed pkt reception number is : %d\n\n",pktReceptionNum,relayPktNum);
        }
    }else if (getPktStat(payload,size)==11)
    {
        if(strstr((char *)payload,buffer)!=NULL)
        {
            ++relayPktNum;
            debug("\n\nreception number is : %d \nrelayed pkt reception number is : %d\n\n",pktReceptionNum,relayPktNum);
        }
    }
    }
#if 0
    if(strstr((char *)payload,(char *)msg2)!=NULL){
        ++pktReceptionNum;
        
        if(strstr((char *)payload,"UL")!=NULL && strstr((char*)payload,"DL")!=NULL){
            ++relayPktNum;
        }
        debug("\n\nreception number is : %d \nrelayed pkt reception number is : %d\n\n",pktReceptionNum,relayPktNum);
    }
#endif
}
int main(void){
    uint16_t size;
    size=sizeof(msg1);
    t.start();
    debug("[time:%f]Start sending message\n",t.read());
    PHY_FSM_start(&ACKInterrupt);
    tx.attach(&handler_txTimeout, 5);
    synchflag=1;
    delaySize=1000;
    PHY_setDelayTime(delaySize);
    debug("delay size: %d\n",delaySize);
    window1Size=1000;
    window2Size=2000;
    PHY_setWindowSize(1, window1Size);
    PHY_setWindowSize(2, window2Size);
    debug("window size: %d %d\n",window1Size,window2Size);
    while(1){
       
            PHY_FSM_run();
            if(getPresentState()==PHY_IDLE && getRepeatFlag()==1 && msg_pended == 1){
                //debug("in to the if statement");
                msg_pended=0;
                u.attach(&handler_txTimeout, 7);
            }
    }
}
