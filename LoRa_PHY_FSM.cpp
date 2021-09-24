/*Jean Park Aug.2019 LoRa PHY FSM code based on specifications*/

#define ANALYSIS_ON
#include "PHYHAL_interface.h"
#include "LoRa_PHY_FSM.h"
#include "LoRa_PHY_API.h"



Timer t1;
Timeout delay1Timer;
Timeout delay2Timer;
Timeout windowTimer;
//global variable
static PHYState presentState;
static PHYEvent triggeredEvent;
static MAC_PHY_params_e parameterSet;
static Message_t ul;
static Message_t dl;
static PHYstatus currentStatus;
static uint32_t eventFlag;
static double delayms;
static double window_1_ms,window_2_ms,window_ms;
#if defined ANALYSIS_ON
static float delayTime,time1,time2,avgTime,totalTime,totalRssi,avgRssi,bup,aup;
static uint32_t cnt,rx1cnt,rx2cnt;
#endif
uint8_t rxNum=0;
#if defined PHY_ONLY
uint8_t repeatFlag;
#endif
const uint8_t rxMax=2;
bool isMAC=true;
time_t seconds;
uint8_t msg[BUF_SIZE]="Hello World!\n";
bool rxErr=0;


//function ptr for mac interface
void (* indicateRxDone)(uint8_t *payload, uint16_t size);
void (* indicateRxFail)();
#if defined ANALYSIS_ON
static void calcData();
#endif

/*Acion*/
PHYState FSM_receive_dl(){
    uint8_t rxCfg;
    if(rxNum == 1 )
    {
        setDl(ul.BandWidth,ul.Datarate,ul.ChannelFreq);//dl freq:=ul freq, dl drate:=ul drate
        //debug("[PHY][ACTION] rx1: %s %s %d\n",dl.BandWidth,dl.Datarate,dl.ChannelFreq);
    }else if (rxNum==2)
    {
        // dl freq, drate modified through mac command
        //debug("[PHY][ACTION] rx2: %s %s %d\n",dl.BandWidth,dl.Datarate,dl.ChannelFreq);
    }else
    {
        if(currentStatus == PHYs_default)
        {
            debug("[PHY][ERROR] rx window can be opened only twice\n");
            return PHY_IDLE;
        }
    }
    
    debug_if(DEBUG_MSG,"[PHY][ACTION] rx: %d %d %d\n",dl.BandWidth,dl.Datarate,dl.ChannelFreq);
    rxCfg=HAL_cmd_SetRxConfig(
                              dl.BandWidth,
                              dl.Datarate,
                              PHY_DEFAULT_CODERATE,
                              PHY_DEFAULT_PREAMBLELEN,
                              dl.ChannelFreq,
                              0); //continuous mode : RX mode controlled by PHY
    
    if(HAL_isRfActive()==false){
        debug_if(DEBUG_MSG,"[PHY][ACTION][time:%f]Command receive\n",t1.read());
        HAL_cmd_receive();
    }
    
    return PHY_RX_RUN;
}

PHYState FSM_transmit_ul(uint8_t* buffer, uint16_t buffer_size){
    debug_if(0,"[PHY][ACTION]transmit up link msg %d %d %d %s\n",ul.BandWidth,ul.Datarate,ul.ChannelFreq,buffer);
    uint8_t size=0;
    HAL_cmd_SetTxConfig(PHY_DEFAULT_TXPOWER,ul.BandWidth,ul.Datarate,PHY_DEFAULT_CODERATE,PHY_DEFAULT_PREAMBLELEN,ul.ChannelFreq);
    size=(uint8_t)buffer_size;
    HAL_cmd_transmit(buffer,size);
    
    return PHY_TX_RUN;
}

PHYState FSM_transmit_done(){
    debug_if(DEBUG_MSG,"[PHY][ACTION]rx delay 1 and 2 open\n");
    delay1Timer.attach(&RxDelay1End,delayms);
    delay2Timer.attach(&RxDelay2End,delayms+1);
    
    return PHY_RX_WAIT;
}
//trigger
void RxDelay1End(){
    debug_if(DEBUG_MSG,"[PHY][Trigger][time:%f]RxDelay1End\n",t1.read());
    PHYEvent_setEventFlag(PHYe_RX_Delay_End);
}
void RxDelay2End(){
    debug_if(DEBUG_MSG,"[PHY][Trigger][time:%f]RxDelay2End\n",t1.read());
    PHYEvent_setEventFlag(PHYe_RX_Delay_End);
}
void expireWindow(){
    debug("expire window\n");
    if(HAL_isSignalSynchronized()==true)
    {
        debug_if(DEBUG_MSG,"[PHY][Trigger][time:%f] Preamble detected - Rx Window expanded\n",t1.read());
    }
    else if(rxErr==0){
        debug_if(DEBUG_MSG,"[PHY][Trigger][time:%f]Rx Window expired\n",t1.read());
        PHYEvent_setEventFlag(PHYe_RX_Fail);
    }
}



void HalTxCnfHandler(HALtype_txCmdErr res)
{
    switch (res)
    {
        case HAL_TX_CNF_DONE: //TxDoneInterrupt, tx done successfully
            #if defined ANALYSIS_ON
                aup=t1.read();
                debug_if(DEBUG_MSG,"[PHY][Trigger][time:%f]TxDoneInterrupt\n",aup);
            #else
                debug_if(DEBUG_MSG,"[PHY][Trigger]TxDoneInterrupt\n");
            #endif
            PHYEvent_setEventFlag(PHYe_TX_Done);
            break;
            
        case HAL_TX_CNF_TIMEOUT: //TxTimeoutInterrupt, timeout happened for TX
            debug_if(DEBUG_MSG,"[PHY][Trigger][time:%f]TxTimeoutInterrupt\n",t1.read());
            PHYEvent_setEventFlag(PHYe_TX_Fail);
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
            debug("[PHY][Trigger][time:%f]RxDoneInterrupt: %s rssi: %d snr: %d\n",t1.read(),payload,rssi,snr);
            #if defined ANALYSIS_ON
                totalRssi+=rssi;
            #endif
            PHYEvent_setEventFlag(PHYe_RX_Detect);
            if(isRxError()==false){
                dl.PHYLoRaPacket.Payload=payload;
                dl.PHYLoRaPacket.Size=size;
                dl.PHYLoRaPacket.Rssi=rssi;
                dl.PHYLoRaPacket.Snr=snr;
            }
            break;
            
        case HAL_RX_CNF_CRCERROR: //RxErrorInterrupt, RX PDU with CRC error
            setRxErr();
            debug("[PHY][Trigger][time:%f]RxErrorInterrupt\n",t1.read());
            PHYEvent_setEventFlag(PHYe_RX_Fail);
            break;
            
        case HAL_RX_CNF_TIMEOUT: //RxTimeoutInterrupt, timer expired without RX event : hwtimeout only on rx single and sw timeout only when specified window time through configuration
            setRxErr();
            debug("[PHY][Trigger][time:%f]RxTimeoutInterrupt\n",t1.read());
            PHYEvent_setEventFlag(PHYe_RX_Fail);
            break;
            
        default:
            break;
    }
}

//configuration functions
void setDl(HALenum_bw_e bandwidth,HALenum_dr_e datarate,uint32_t freq){
    dl.BandWidth=bandwidth;
    dl.ChannelFreq=freq;
    dl.Datarate=datarate;
}
void setUl(HALenum_bw_e bandwidth,HALenum_dr_e datarate,uint32_t freq){
    ul.BandWidth=bandwidth;
    ul.ChannelFreq=freq;
    ul.Datarate=datarate;
}

uint8_t PHY_setDelayTime(uint32_t timeMilliSec){
    if(getPresentState()==PHY_IDLE)
    {
        delayms=timeMilliSec/1000;
        return 1;
    }
    debug("[Error][PHY] present state is not IDLE\n");
    return 0;
}

uint8_t PHY_setWindowSize(uint8_t windowNum, uint32_t timeMilliSec){
    if(getPresentState()==PHY_IDLE)
    {
        if(windowNum==1)
        {
            if(!PHYEvent_checkEventFlag(PHYe_RX_Only_Req))
            {
                window_1_ms=timeMilliSec/1000;
                return 1;
            }
        }else if (windowNum==2)
        {
            if(!PHYEvent_checkEventFlag(PHYe_RX_Only_Req))
            {
                window_2_ms=timeMilliSec/1000;
                return 1;
            }
        }
        else if (windowNum==0 && PHYEvent_checkEventFlag(PHYe_RX_Only_Req))
        {
            window_ms=timeMilliSec/1000;
            return 1;
        }
        else
        {
            debug("[Error][PHY] maximum number of winodws are two\n");
            return 0;
        }
    }
    debug("[Error][PHY] present state is not IDLE\n");
    return 0;
}

void setRxErr(){
    rxErr=1;
}
void resetRxErr(){
    rxErr=0;
}
bool isRxError(){
    if(rxErr==1){
        return true;
    }else{
        return false;
    }
}
#if 0
bool checkRxStatus(){
    if(HAL_getRxStatus()==1)
        return false;
    
    return true;
}
#endif
// event flag function
void PHYEvent_setEventFlag(PHYEvent event){
    eventFlag |= (0x01 << event);
}
void PHYEvent_clearEventFlag(PHYEvent event){
    eventFlag &= ~(0x01 << event);
}
void PHYEvent_clearAllEventFlag(void){
    eventFlag = 0;
}
int PHYEvent_checkEventFlag(PHYEvent event){
    return (eventFlag & (0x01 << event));
}

/*API*/
#if defined PHY_ONLY
void PHY_init(void (*MACRxDone)(uint8_t*, uint16_t)){
#elif defined RMAC_ON
void PHY_init(void (*MACRxDone)(uint8_t*, uint16_t),void (*MACRxFail)()){
#endif
    presentState = PHY_IDLE;
    PHYEvent_clearAllEventFlag();
    setUl(PHY_DEFAULT_BANDWIDTH,PHY_DEFAULT_DATARATE,PHY_DEFAULT_FREQUENCY);
    setDl(PHY_DEFAULT_BANDWIDTH,PHY_DEFAULT_DATARATE,PHY_DEFAULT_FREQUENCY);
    rxNum = 0;
    resetRxErr();
    seconds=time(NULL);
    PHY_setRxDone(MACRxDone);
    #if defined RMAC_ON
        PHY_setRxFail(MACRxFail);
    #endif
    currentStatus = PHYs_default;
    window_1_ms=1;
    window_2_ms=2;
    delayms=RX_DELAY1;
    
    #if defined PHY_ONLY
        repeatFlag=1;
    #endif
    #if defined ANALYSIS_ON
        delayTime=0;
        avgTime=0;
        cnt=0;
        totalTime=0;
        totalRssi=0;
        avgRssi=0;
        rx1cnt=0;
        rx2cnt=0;
    #endif
}
#if defined PHY_ONLY
void PHY_FSM_start(void (*MACRxDone)(uint8_t*, uint16_t)){
#elif defined RMAC_ON
void PHY_FSM_start(void (*MACRxDone)(uint8_t*, uint16_t),void (*MACRxFail)()){
#endif

    debug("starting LoRa physical Layer state machine\n");
    
    t1.start();
    HAL_cmd_init(&HalTxCnfHandler, &HalRxCnfHandler);
    debug("[time:%f]initialize PHY\n",t1.read());
    #if defined PHY_ONLY
        PHY_init(MACRxDone);
    #elif defined RMAC_ON
        PHY_init(MACRxDone,MACRxFail);
    #endif
    
    
}

void PHY_configUlPayload(uint8_t *buffer, uint16_t size,HALenum_bw_e BW,HALenum_dr_e SF,uint32_t Freq){
    debug_if(DEBUG_MSG,"[PHY] Uplink message configuration \n");
    ul.PHYLoRaPacket.Payload=buffer;
    ul.PHYLoRaPacket.Size=size;
    setUl(BW,SF,Freq);
}

void PHY_setRxSlot(HALenum_bw_e BW,HALenum_dr_e SF,uint32_t Freq){
    parameterSet.BandWidth=BW;
    parameterSet.SpreadingFactor=SF;
    parameterSet.Frequency=Freq;
}

void PHY_setRxDone(void (*functionPtr)(uint8_t*, uint16_t)){
    indicateRxDone=functionPtr;
}

void PHY_setRxFail(void (*functionPtr)()){
    indicateRxFail=functionPtr;
}

void PHY_indicateEvent(RMAC_PHY_indicator_e event){
    if(event == TX)
    {
        if(getPresentState() == PHY_IDLE){
            debug_if(DEBUG_MSG,"[PHY][cmd] indicate Event Tx \n");
            PHYEvent_setEventFlag(PHYe_TX_Only_Req);
            currentStatus = PHYs_Tx;
        }
    }else if(event == RX)
    {
        if(getPresentState() == PHY_IDLE){
            debug_if(DEBUG_MSG,"[PHY][cmd] indicate Event Rx \n");
            PHYEvent_setEventFlag(PHYe_RX_Only_Req);
            currentStatus = PHYs_Rx;
        }
    }else if(event == FSM)
    {
        if(getPresentState() == PHY_IDLE){
            debug_if(DEBUG_MSG,"[PHY][cmd] indicate Event FSM \n");
            PHYEvent_setEventFlag(PHYe_TX_Req);
            currentStatus = PHYs_default;
        }
    }else if (event == CLEAR)
    {
        if(getPresentState() == PHY_RX_WAIT || getPresentState() == PHY_RX_RUN){
            debug_if(DEBUG_MSG,"[PHY][cmd] indicate Event CLEAR \n");
            PHYEvent_clearAllEventFlag();
        }
    }
}

PHYState getPresentState(){
    return presentState;
}
#if defined PHY_ONLY
uint8_t getRepeatFlag(){
    return repeatFlag;
}
#endif
#if defined ANALYSIS_ON
static void calcData(){
    if(time1*time2<0)
    {
        if (time1>0)
        {
            delayTime=(time2-time1+4294)/2;
        }
    }
    else{
        if(time1>time2)
        {
            delayTime=(time1-time2)/2;
        }else
        {
            delayTime=(time2-time1)/2;
        }
    }
    if(rxNum==1) //counting packet reception of each rx slot
    {
        rx1cnt++;
    }else
    {
        rx2cnt++;
    }
    ++cnt;
    totalTime=totalTime+delayTime;
    avgTime=totalTime/cnt;
    avgRssi=totalRssi/cnt;
}
#endif
    
void PHY_FSM_run(){
    PHYState nextState;
    
    switch(presentState)
    {
        default:
        case PHY_IDLE:
            if(PHYEvent_checkEventFlag(PHYe_TX_Req) || PHYEvent_checkEventFlag(PHYe_TX_Only_Req))
            {
                
                #if defined ANALYSIS_ON
                    time1=t1.read();
                    bup=time1;
                    debug("[PHY][Event][time:%f]Tx Request \n",time1);
                #endif
                nextState = FSM_transmit_ul(
                                            ul.PHYLoRaPacket.Payload,
                                            ul.PHYLoRaPacket.Size);
                #if defined PHY_ONLY
                    repeatFlag=0;
                #endif
                
            }else if(PHYEvent_checkEventFlag(PHYe_RX_Only_Req)) // RMAC monitoring condition
            {
                debug("[PHY][Event][time:%f] Monitoring for RMAC\n",t1.read());
                setDl(parameterSet.BandWidth,parameterSet.SpreadingFactor,parameterSet.Frequency);
                nextState = PHY_RX_WAIT;
                PHYEvent_setEventFlag(PHYe_RX_Delay_End);
                #if defined PHY_ONLY
                    repeatFlag=0;
                #endif
            }else
            {
                nextState=PHY_IDLE;
            }
            break;
        case PHY_TX_RUN:
            if(PHYEvent_checkEventFlag(PHYe_TX_Fail))
            {
                debug("[PHY][Event][time:%f]Tx Fail\n",t1.read());
                PHYEvent_clearAllEventFlag();
                nextState = PHY_IDLE;
            }
            else if(PHYEvent_checkEventFlag(PHYe_TX_Done))
            {
                #if defined ANALYSIS_ON
                    debug("[PHY][Event][time:%f]Tx Done (%f)\n",t1.read(),aup-bup);
                #endif
                if(PHYEvent_checkEventFlag(PHYe_TX_Only_Req))
                {
                    nextState = PHY_IDLE;
                    PHYEvent_clearAllEventFlag();
                    #if defined PHY_ONLY
                        repeatFlag=1;
                    #endif
                }else
                {
                    nextState = FSM_transmit_done();
                }
                
            }else
            {
                nextState = PHY_TX_RUN;
            }
            break;
        case PHY_RX_WAIT:
            if(PHYEvent_checkEventFlag(PHYe_RX_Delay_End))
            {
                if(PHYEvent_checkEventFlag(PHYe_TX_Req))
                {
                    rxNum++;
                    resetRxErr();
                    debug("[PHY][Event][time:%f]Rx window%d opens\n",t1.read(),rxNum);
                    if(rxNum==1)
                    {
                        windowTimer.attach(&expireWindow,window_1_ms);
                    }else
                    {
                        windowTimer.attach(&expireWindow,window_2_ms);
                    }
               
                    nextState = FSM_receive_dl();
           
                    
                }else if(PHYEvent_checkEventFlag(PHYe_RX_Only_Req))
                {
                    debug("[PHY][Event][time:%f]Rx window for monitoring opens\n",t1.read(),rxNum);
                    resetRxErr();
                    windowTimer.attach(&expireWindow,window_ms);
                    nextState = FSM_receive_dl();//RMAC receive msg
                }
                PHYEvent_clearEventFlag(PHYe_RX_Delay_End);
            }
            else
            {
                nextState = PHY_RX_WAIT;
            }
            break;
        case PHY_RX_RUN:
            if(PHYEvent_checkEventFlag(PHYe_RX_Detect))
            {
                #if defined ANALYSIS_ON
                    time2=t1.read();
                    calcData();
                    debug("[PHY][Event][time:%f]DL detected :%s delayTime : %f\n\n",time2,dl.PHYLoRaPacket.Payload,delayTime);
                    debug("[PHY][analysis] avgDelayTime: %f avgR: %f Rx1: %d Rx2: %d\n\n",avgTime,avgRssi,rx1cnt,rx2cnt);
                #endif
                if(PHYEvent_checkEventFlag(PHYe_TX_Req)) // original
                {
                    rxNum=0;
                    delay1Timer.detach();
                    delay2Timer.detach();
                }
                resetRxErr();
                windowTimer.detach();
                nextState=PHY_IDLE;
                HAL_cmd_Sleep();
                indicateRxDone(dl.PHYLoRaPacket.Payload,
                               dl.PHYLoRaPacket.Size);
                PHYEvent_clearAllEventFlag();
                #if defined PHY_ONLY
                    repeatFlag=1;
                #endif
                
            }else if(PHYEvent_checkEventFlag(PHYe_RX_Fail))
            {
                debug("[PHY][Event][time:%f]Receive window%d fail\n\n",t1.read(),rxNum);
                HAL_cmd_Sleep();
                windowTimer.detach();
                if(PHYEvent_checkEventFlag(PHYe_TX_Req))
                {
        
                    if (rxNum < rxMax) // first receive window fail
                    {
                        nextState = PHY_RX_WAIT;
                        PHYEvent_clearEventFlag(PHYe_RX_Fail);
                        debug("[time:%f]first window failure\n",t1.read());
                        break;
                    }else{
                        rxNum=0;
                        nextState=PHY_IDLE;
                        delay1Timer.detach();
                        delay2Timer.detach();
                        PHYEvent_clearAllEventFlag();
                        #if defined ANALYSIS_ON
                            time1=0;
                            time2=0;
                            delayTime=0;
                        #endif
                        debug("[time:%f]second window failure\n",t1.read());
                        #if defined PHY_ONLY
                        repeatFlag=1;
                        #endif
                    }
                }else
                {
                    debug("[PHY][Event][time:%f]going to PHY_IDLE\n",t1.read());
                    nextState=PHY_IDLE;
                    PHYEvent_clearAllEventFlag();
                    #if defined PHY_ONLY
                        repeatFlag=1;
                    #endif
                    
                }
                #if defined RMAC_ON
                    indicateRxFail();
                #endif
            }else
            {
                nextState = PHY_RX_RUN;
            }
            break;
            
    }
    presentState=nextState;
}
#if defined ANALYSIS_ON
void clearVariables(){
    totalRssi=0;
    totalTime=0;
    cnt=0;
    avgRssi=0;
    avgTime=0;
    rx1cnt=0;
    rx2cnt=0;
}
#endif
