#include "mbed.h"
#include "HAL_FSM.h"
#include "Common_HAL.h"

#define MAX_STATE_NAME  100

/*
typedef enum {
    HAL_EVENT_CFGREQ,   //INIT, TXCFG, RXCFG, QUERY
    HAL_EVENT_RUN,      //TX, RX, SLEEP
    HAL_EVENT_CFGEND,   //TXDONE, TXTIMEOUT, RXERR, RXDONE, RXTIMEOUT
    HAL_EVENT_RUNEND,   //sending back the primitive
    HAL_EVENT_ABORT,
    HAL_EVENT_MAX
} HALtype_fsmEvent;


typedef enum {
	HAL_FSMST_IDLE, 			//nothing is done now
	HAL_FSMST_CFGING, 		    //try to do configuration
    HAL_FSMST_READY,		    //ready to run
	HAL_FSMST_RUN,				//running
    HAL_FSMST_MAX
} HALtype_fsmState;				//HAL layer status

typedef enum {
    HAL_FSMRES_SUCCESS,
    HAL_FSMRES_CANNOTHAPPEN,
    HAL_FSMRES_WARNING
} HALtype_fsmResult;
*/
static char fsmStateName[HAL_FSMST_MAX][MAX_STATE_NAME] = 
    {
        "IDLE",
        "CONFIGURING",
        "READY",
        "RUN"
    };
static char fsmEventName[HAL_EVENT_MAX][MAX_STATE_NAME] = 
    {
        "CONFIG REQ",
        "RUN REQ",
        "CONFIG CNF",
        "RUN END",
        "RX ABORT REQ"
    };

static HALtype_fsmState halFsmState = HAL_FSMST_IDLE;
static HALtype_fsmState FSMnextState[HAL_FSMST_MAX][HAL_EVENT_MAX] =
    {
        {HAL_FSMST_CFGING,  HAL_FSMST_RUN,      HAL_FSMST_IDLE,     HAL_FSMST_IDLE,     HAL_FSMST_IDLE},
        {HAL_FSMST_CFGING,  HAL_FSMST_CFGING,   HAL_FSMST_READY,    HAL_FSMST_CFGING,   HAL_FSMST_CFGING},
        {HAL_FSMST_CFGING,  HAL_FSMST_RUN,      HAL_FSMST_READY,    HAL_FSMST_READY,    HAL_FSMST_CFGING},
        {HAL_FSMST_RUN,     HAL_FSMST_RUN,      HAL_FSMST_RUN,      HAL_FSMST_READY,    HAL_FSMST_CFGING}
    };
static HALtype_fsmResult FSMtransition[HAL_FSMST_MAX][HAL_EVENT_MAX] =
    {
        {HAL_FSMRES_SUCCESS,        HAL_FSMRES_WARNING,         HAL_FSMRES_CANNOTHAPPEN, HAL_FSMRES_CANNOTHAPPEN,   HAL_FSMRES_SUCCESS},
        {HAL_FSMRES_CANNOTHAPPEN,   HAL_FSMRES_CANNOTHAPPEN,    HAL_FSMRES_SUCCESS,      HAL_FSMRES_CANNOTHAPPEN,   HAL_FSMRES_CANNOTHAPPEN},
        {HAL_FSMRES_SUCCESS,        HAL_FSMRES_SUCCESS,         HAL_FSMRES_CANNOTHAPPEN, HAL_FSMRES_CANNOTHAPPEN,   HAL_FSMRES_SUCCESS},
        {HAL_FSMRES_CANNOTHAPPEN,   HAL_FSMRES_CANNOTHAPPEN,    HAL_FSMRES_CANNOTHAPPEN, HAL_FSMRES_SUCCESS,        HAL_FSMRES_SUCCESS}
    };

HALtype_fsmResult HAL_FSM_do(HALtype_fsmEvent event)
{
    HALtype_fsmResult res = FSMtransition[halFsmState][event];

    if (res != HAL_FSMRES_CANNOTHAPPEN)
    {
        debug_if(DBGMSG_HAL, "[HAL] FSM transition from %i", halFsmState);
        halFsmState = FSMnextState[halFsmState][event];
        debug_if(DBGMSG_HAL, " to %i\n", halFsmState);
    }
    else
        debug("[HAL] FSM error! cannot happen event %i(%s) in state : %i(%s)\n", event, &fsmEventName[event][0], halFsmState, &fsmStateName[halFsmState][0]);

    return res;
}


HALtype_fsmState HAL_FSM_checkState(void)
{
    return halFsmState;
}

void HAL_FSM_INIT(void)
{
    halFsmState = HAL_FSMST_IDLE;
}