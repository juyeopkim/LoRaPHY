#include "mbed.h"
#include "PHYHAL_interface.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
//#include "LoRa_PHY_API.h"
#define BUF_SIZE 32

Timeout tx;
Timer t;
uint8_t msg[BUF_SIZE] = "[ACK]:";
uint8_t msg2[BUF_SIZE] = {};
uint8_t *downlinkMsg;
uint8_t flag=0;
uint8_t packetSize=0;

uint8_t msg_pended = 0;
uint16_t count=0;
uint16_t pktRecptionNum=0;
uint16_t relayNum=0;

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
    payloadContent=(uint8_t *)malloc(sizeof(uint8_t)*packetSize);
    statNumStr=(uint8_t *)malloc(sizeof(uint8_t)*packetSize);
    memcpy(payloadContent,msg,packetSize * sizeof(uint8_t));
    sprintf((char *)statNumStr,"%d%d",bit1,bit2);
    downlinkMsg=(uint8_t*)strcat((char *)statNumStr,(char *)payloadContent+2);
}

void handler_txTimeout(void)
{
    debug("[time:%f]transmitting %s\n",t.read(),downlinkMsg);
    if(packetSize!=0)
        HAL_cmd_transmit(downlinkMsg, packetSize);
    else
        HAL_cmd_transmit(downlinkMsg, BUF_SIZE);
    flag=0;
}
void HalTxCnfHandler(HALtype_txCmdErr res)
{
    switch (res)
    {
        case HAL_TX_CNF_DONE: //TxDoneInterrupt, tx done successfully
            debug("[PHY][Trigger][time:%f]TxDoneInterrupt\n",t.read());
            HAL_cmd_Sleep();
            break;

        case HAL_TX_CNF_TIMEOUT: //TxTimeoutInterrupt, timeout happened for TX
            debug("[PHY][Trigger][T:%d]TxTimeoutInterrupt\n");
            HAL_cmd_Sleep();
            break;

        default:
            break;
    }
}

void HalRxCnfHandler(HALtype_rxCmdErr res, uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
    switch (res)
    {
        case HAL_RX_CNF_DONE: //RxDoneInterrupt, RX PDU comes without error
            debug("[time:%f]pkt received ! : %s \nsize: %d\n", t.read(),payload,size);
            if(strstr((char *)payload,"LoRa")==NULL){
                debug("[Error] unintended packet\n");
                HAL_cmd_Sleep();
            }
            else
            {
                int localPktNum=0,localRelayNum=0;
                if(getPktStat(payload,size)==10)//direct DL from gateway
                {
                    debug("[Error] Aborted unintended packet\n");
                    HAL_cmd_Sleep();
                }
                else
                {
                    localPktNum=pktRecptionNum;
                    localRelayNum=relayNum;
                    debug("[corruption check][0] pktNum:%d\trelayNum:%d\n",pktRecptionNum,relayNum);
                    memcpy(msg,payload,BUF_SIZE * sizeof(uint8_t));
                    debug("[corruption check][1] pktNum:%d\trelayNum:%d\n",pktRecptionNum,relayNum);
                    if(getPktStat(payload,size)==00) //direct from
                    {
                        setPktStat(10);
                        if((pktRecptionNum-localPktNum) != 0 || (relayNum-localRelayNum) !=0){
                            debug("[Error] memory corruption\n");
                            pktRecptionNum=localPktNum;
                            relayNum=localRelayNum;
                            HAL_cmd_Sleep();
                            break;
                        }
                    }else if (getPktStat(payload,size)==01)
                    {
                        
                        setPktStat(11);
                        if((pktRecptionNum-localPktNum) != 0 || (relayNum-localRelayNum) !=0){
                            debug("[Error] memory corruption\n");
                            pktRecptionNum=localPktNum;
                            relayNum=localRelayNum;
                            HAL_cmd_Sleep();
                            break;
                        }
                        relayNum++;
                    }else if (getPktStat(payload,size)==11)
                    {
                        debug("[Error] Aborted unintended packet\n");
                        HAL_cmd_Sleep();
                        break;
                    }
                    debug("[corruption check][2] pktNum:%d\trelayNum:%d\n",pktRecptionNum,relayNum);
                    
                    debug("\n\n pkt reception number: %d\n\n from relay node: %d\n\n\n", ++pktRecptionNum,relayNum);
                    flag=1;//send tx
                    packetSize=size;
                }
                #if 0
                if(strstr((char *)payload, "[ACK]")!=NULL) // ACK included=intended for end node
                {
                    debug("[Error] Aborted unintended packet\n");
                    HAL_cmd_Sleep();

                }else
                {
                    localPktNum=pktRecptionNum; 
                    localRelayNum=relayNum;
                    debug("[corruption check][0] pktNum:%d\trelayNum:%d\n",pktRecptionNum,relayNum);
                    memcpy(msg2,msg,BUF_SIZE * sizeof(uint8_t));
                    debug("[corruption check][1] pktNum:%d\trelayNum:%d\n",pktRecptionNum,relayNum);
                    strcat((char *)msg2,(char *)payload);
                    debug("[corruption check][2] pktNum:%d\trelayNum:%d\n",pktRecptionNum,relayNum);
                    if((pktRecptionNum-localPktNum) != 0 || (relayNum-localRelayNum) !=0){
                        debug("[Error] memory corruption\n");
                        pktRecptionNum=localPktNum;
                        relayNum=localRelayNum;
                        HAL_cmd_Sleep();
                        break;
                    }
                    if(strstr((char *)payload,"DL")!=NULL){
                        debug("[Error] Aborted unintended packet\n");
                        HAL_cmd_Sleep();
                        break;
                    }
                    if(strstr((char *)payload, "UL")!=NULL){
                        relayNum++;
                    }
                    debug("\n\n pkt reception number: %d\n\n from relay node: %d\n\n\n", ++pktRecptionNum,relayNum);
                    flag=1;//send tx
                    packetSize=size;
                }
            #endif
            }
    
            break;
            
        case HAL_RX_CNF_CRCERROR: //RxErrorInterrupt, RX PDU with CRC error
            debug("[time:%f]RxErrorInterrupt\n",t.read());
            HAL_cmd_Sleep();
            break;
            
        case HAL_RX_CNF_TIMEOUT: //RxTimeoutInterrupt, timer expired without RX event
            debug("[time:%f]RxTimeoutInterrupt\n",t.read());
            HAL_cmd_Sleep();
            break;

        default:
            break;
    }
}
int main(void)
{
    debug("    Starting simple TX/RX program...\n");
    debug( "       Initializing LoRa H/W fixed \n" );
    HAL_cmd_init(&HalTxCnfHandler,&HalRxCnfHandler);
    t.start(); 
    debug( "[time:%f]Starting TX/RX loop \n",t.read());
    HAL_cmd_receive();
    
    while(1)
    {
        if(HAL_isRfActive()==false)
        {
            count=0;
            HAL_cmd_receive();
        }else if (flag == 1)
        {
            debug("[time: %f]\n",t.read());
            tx.attach(&handler_txTimeout, 1.1);
            sleep();
            
        }
    }
}

