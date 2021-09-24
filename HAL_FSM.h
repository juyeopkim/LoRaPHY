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

HALtype_fsmResult HAL_FSM_do(HALtype_fsmEvent event);
HALtype_fsmState HAL_FSM_checkState(void);
void HAL_FSM_INIT(void);