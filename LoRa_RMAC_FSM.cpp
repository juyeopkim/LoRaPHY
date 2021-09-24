/*Jean Park Oct.2019 LoRa Relay MAC FSM code based on specifications*/
/* random generate number num = (rand() % (upper – lower + 1)) + lower */
#include "LoRa_RMAC_API.h"
#include "LoRa_RMAC_FSM.h"
#include "LoRa_PHY_API.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
Timeout RMACdelayTimer;
Timeout RMACmonitorTimer;


static RMACState presentRState;
static RMACEvent triggeredREvent;
//static RMAC_PHY_indicator_e eventForPHY;
static HALenum_dr_e SF;
static HALenum_bw_e BW;
uint32_t Fq;
uint16_t msgSize;
uint8_t forward[32]="UL ";
//uint8_t relayingMSG[16]={};// size 명시 안해주면 mbed os error 발생 endnode에게 relay시 packet이 짤림
//uint8_t relayingMSG1[16]={};// size 명시 안해주면 mbed os error 발생
//uint8_t relayingMSG2[16]={};// size 명시 안해주면 mbed os error 발생
uint8_t relay[32]="DL ";
Message_t relay_msg;
uint8_t repeatFlag;
Timer t;


//PHY-RMAC interrupt
void PHYRxDoneInterrupt(uint8_t *payload, uint16_t size){
    if(presentRState == RMAC_RELAY)
    {
        relay_msg.RMACLoRaPacket.Payload = payload;
        relay_msg.RMACLoRaPacket.Size = size;
        triggeredREvent = RMACe_RECEIVE;
        debug("[RMAC][Trigger] Relay receive Done: %s \n",relay_msg.RMACLoRaPacket.Payload);
    }
    else if(presentRState == RMAC_MONITOR)
    {
        relay_msg.RMACLoRaPacket.Payload = payload;
        relay_msg.RMACLoRaPacket.Size = size;
        triggeredREvent = RMACe_DETECT;
        debug("[RMAC][Trigger] Monitor detect Done %s \n",relay_msg.RMACLoRaPacket.Payload);
    }
}
void PHYRxFailInterrupt(){
    if(presentRState == RMAC_RELAY)
    {
        triggeredREvent = RMACe_RECEIVE_FAIL;
        debug("[RMAC][Trigger] Relay receive Fail \n");
    }
    else if(presentRState == RMAC_MONITOR)
    {
        triggeredREvent = RMACe_DETECT_FAIL;
        debug("[RMAC][Trigger] Monitor detect Fail \n");
    }
}
//Timer interrupt
void delayEnd(){
    triggeredREvent = RMACe_DELAY_END;
    debug("[RMAC][Trigger] Delay End\n");
}
void expireMonitor(){
    triggeredREvent = RMACe_DETECT_FAIL;
    debug("[RMAC][Trigger] Detect fail\n");
}
//Parameter setting method function
void setRandomParameter(){
#if 0
    srand(time(NULL));
    uint8_t sf,bw;
    sf = ( rand() % ( UPPER_SPREADING_FACTOR - LOWER_SPREADING_FACTOR + 1 ) ) + LOWER_SPREADING_FACTOR;
    
    bw = ( rand() % 3 );
    SF= (HALenum_dr_e)sf;
    BW= (HALenum_bw_e)bw;
    Fq = ( rand() % 12 );
    Fq = 920900000 + (200000 * Fq);
#endif
    Fq=922100000;
    BW=HAL_LRBW_125;
    SF=HAL_LRDatarate_SF7;
    //debug("[RMAC][Action] BW:%d SF:%d Fq:%d\n",bw,sf,Fq);
}
//API
void indicateRMACMonitoring(){
    triggeredREvent=RMACe_PREPARE_MONITOR;
}

uint8_t getRMACState(){
    if(presentRState == RMAC_IDLE)
    {
        return 0;
    }
    else if (presentRState == RMAC_MONITOR)
    {
        return 1;
    }
    else if (presentRState == RMAC_CLEAR)
    {
        return 2;
    }
    else if (presentRState == RMAC_RELAY)
    {
        return 3;
    }
}

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
    payloadContent=(uint8_t *)malloc(sizeof(uint8_t)*relay_msg.RMACLoRaPacket.Size);
    statNumStr=(uint8_t *)malloc(sizeof(uint8_t)*relay_msg.RMACLoRaPacket.Size);
    memcpy(payloadContent,relay_msg.RMACLoRaPacket.Payload,relay_msg.RMACLoRaPacket.Size * sizeof(uint8_t));
    sprintf((char *)statNumStr,"%d%d",bit1,bit2);
    relay_msg.RMACLoRaPacket.Payload=(uint8_t*)strcat((char *)statNumStr,(char *)payloadContent+2);
}

uint8_t getRepeatFlag(){
    return repeatFlag;
}
 void RMAC_FSM_start(){
    debug("starting LoRa Relay Mac Layer state machine \n");
    PHY_FSM_start(&PHYRxDoneInterrupt,&PHYRxFailInterrupt);
    debug("[RMAC][Init] RMAC initialize \n");
    RMAC_init();
}
void RMAC_init(){
    presentRState= RMAC_IDLE;
    triggeredREvent = RMACe_NULL;
    repeatFlag=0;
}

void RMAC_FSM_run(){
    RMACState nextRState;
    
    switch(presentRState)
    {
        default:
        case RMAC_IDLE:
            if(repeatFlag==1)
            {
                repeatFlag=0;
            }
            if(triggeredREvent == RMACe_DELAY_END)
            {
                debug("[RMAC][Event] Delay End\n");
                PHY_indicateEvent(RX);
                PHY_setWindowSize(0,10000);
                //convey parameters to PHY
                //RMACmonitorTimer.attach_us(&expireMonitor,3000000);
                triggeredREvent = RMACe_NULL;
                nextRState = RMAC_MONITOR;
            }
            else if(triggeredREvent == RMACe_PREPARE_MONITOR) //set delay1
            {
                debug("[RMAC][Event] Prepare Monitor\n");
                RMACdelayTimer.attach_us(&delayEnd,1000);
                setRandomParameter();
                //PHY_setRxSlot(HAL_LRBW_250,HAL_LRDatarate_SF12,922300000);
                PHY_setRxSlot(BW,SF,Fq);
                triggeredREvent = RMACe_NULL;
            }
            else
            {
                nextRState = RMAC_IDLE;
            }
            break;
        case RMAC_MONITOR:
            if(triggeredREvent == RMACe_DETECT)
            {
                debug("[RMAC][Event] Detect Msg\n");
                //relay sig add
                #if 0
                memcpy(RMACmsg,forward,size * sizeof(uint8_t));
                strcat((char *)RMACmsg,(char *)relay_msg.RMACLoRaPacket.Payload);
                #endif
                #if 1
                if(getPktStat(relay_msg.RMACLoRaPacket.Payload,relay_msg.RMACLoRaPacket.Size)==00)
                {
                    setPktStat(01);
                    PHY_configUlPayload(relay_msg.RMACLoRaPacket.Payload,
                                        relay_msg.RMACLoRaPacket.Size,
                                        BW,
                                        SF,
                                        922300000);
                    PHY_indicateEvent(FSM);
                    nextRState = RMAC_RELAY;
                    triggeredREvent = RMACe_NULL;
                }
                #endif
                
                t.start();
            }
            else if(triggeredREvent == RMACe_CONVERT_REQ)
            {
                debug("[RMAC][Event] Convert Req\n");
                //RMACmonitorTimer.detach();
                PHY_indicateEvent(CLEAR);
                nextRState = RMAC_CLEAR;
            }
            else if(triggeredREvent == RMACe_DETECT_FAIL)
            {
                debug("[RMAC][Event] Detect Fail\n");
                nextRState = RMAC_IDLE;
                triggeredREvent = RMACe_NULL;
                repeatFlag=1;
            }else
            {
                nextRState = RMAC_MONITOR;
            }
            break;
        case RMAC_CLEAR:
            if(triggeredREvent == RMACe_CLEARED)
            {
                debug("[RMAC][Event] Clearing done\n");
                nextRState = RMAC_IDLE;
                repeatFlag=1;
            }else
            {
                nextRState = RMAC_CLEAR;
            }
            break;
        case RMAC_RELAY:
            if(triggeredREvent == RMACe_RECEIVE)
            {
                debug("[RMAC][Event] Receive Msg and forward msg : %s\n",relay_msg.RMACLoRaPacket.Payload);
                # if 0
                //relay message to end node
                memcpy(RMACmsg2,relay,size * sizeof(uint8_t));
                strcat((char *)RMACmsg2,(char *)relay_msg.RMACLoRaPacket.Payload);
                #endif
                #if 1
                if(getPktStat(relay_msg.RMACLoRaPacket.Payload,relay_msg.RMACLoRaPacket.Size)==11){
                    setPktStat(11);
                   
                    PHY_configUlPayload(relay_msg.RMACLoRaPacket.Payload,
                                        relay_msg.RMACLoRaPacket.Size,
                                        BW,
                                        SF,
                                        Fq);
                    PHY_indicateEvent(TX);
                    repeatFlag=1;
                    t.stop();
                    printf("The timer taken was %f\n",t.read());
                }
                #endif
               /*PHY_configUlPayload(RMACmsg2,
                                    relay_msg.RMACLoRaPacket.Size,
                                   HAL_LRBW_250,
                                   HAL_LRDatarate_SF12,
                                   922300000);*/
                
            }
            else if (triggeredREvent == RMACe_RECEIVE_FAIL)
            {
                debug("[RMAC][Event] RMAC receive fail \n");
                triggeredREvent = RMACe_NULL;
                repeatFlag=1;
            }
            else
            {
                nextRState = RMAC_RELAY;
                break;
            }
            nextRState = RMAC_IDLE;
            break;
    }
    presentRState=nextRState;
}
