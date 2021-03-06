/**
 *
 *  \file ipc.c
 *
 *  \author Federico D. Ceccarelli	 
 *
 *******************************************************************************
 *
 * \brief apipc implementation.
 *
 * apipc is an ipc_driver library implementation for TMS320C28x Texas
 * Instruments cores. The api present a bunch of routines to simplify data
 * tranfer between cores.
 *	
 *******************************************************************************
 * Version 0.1 	10/05/2021
 ******************************************************************************
 */

#include "F2837xD_device.h"        // F2837xD Headerfile Include File
#include "F2837xD_Examples.h"      // F2837xD Examples Include File

#include "ipc.h"
#include "../lib/mymalloc/mymalloc.h"
#include "../lib/circular_buffer/buffer.h"

/**
 * \defgroup apipc_gsram_alloc apipc symbols allocation
 *
 * apipc implements symbols that should be allocated and linked to specifics
 * memory spaces and directions. Those memory spaces corresponds to gsram and
 * are defined in the .cmd file included with the project.
 *
 * \note space is allocated depending on the .cmd file included in the project
 * @{*/
#pragma DATA_SECTION(cl_r_w_data,".cpul_cpur_data"); /**< cl_r_w_data is allocated to shared RAM .cpul_cpur_data space. */
#pragma DATA_SECTION(l_apipc_obj,".base_cpul_cpur_addr"); /**< l_apipc_obj mapped to shared RAM .base_cpul_cpur_addr space. */
#pragma DATA_SECTION(r_apipc_obj,".base_cpur_cpul_addr"); /**< r_apipc_obj mapped to shared RAM .base_cpur_cpul_addr space. */
/** @}*/

/** 
 * \defgrup apipc_data_declaration ipclib shared buffers space declarations
 * @{*/
uint16_t  cl_r_w_data[CL_R_W_DATA_LENGTH];   /**< Local to Remote data space */
struct apipc_obj l_apipc_obj[APIPC_MAX_OBJ]; /**< Local apipc objects buffer. */
struct apipc_obj r_apipc_obj[APIPC_MAX_OBJ]; /**< Remote apipc objects buffer. */
/** @}*/

/** 
 * \defgroup ipc_handlers IPC Drivers handlers declaration. 
 *
 * \note Should be declared one IPC handler for every interrupt
 * @{
 */
volatile tIpcController g_sIpcController1; /**< INT0 IPC Drivers handler. */
volatile tIpcController g_sIpcController2; /**< INT1 IPC Drivers handler. */
/** @}*/

/** mymalloc API handler declaration. */
mymalloc_handler l_r_w_data_h;

/** circular_buffer handler declaration. */
circular_buffer_handler message_cbh;
/** ipc mesasages array memory allocation */
tIpcMessage message_array[APIPC_MAX_OBJ];

/** statics functions prototipes declarations
* @{*/
static void apipc_sram_acces_config(void);
static void apipc_check_remote_cpu_init(void);
static void apipc_init_objs(void);
static void apipc_proc_obj(struct apipc_obj *plobj);
static void apipc_cmd_response (tIpcMessage *psMessage);
static void apipc_message_handler (tIpcMessage *psMessage);
static enum apipc_rc apipc_write(uint16_t obj_idx);
static enum apipc_rc apipc_process_messages(void);
/** @}*/

/* apipc_sram_acces_config: */
static void apipc_sram_acces_config(void)
{
    /*
     * Each CPU owns different GSxM blocks of memory to send data & pointer address
     * to the remote core.
     * If a core master a block of GSxM RAM it can R/W/Fetc it.
     * If a core doesnt master a block o GSxM RAM it can only read the memory
     * block.
     */

#if defined ( CPU1 )
    /* CPU01 master GSxM blocks R/W/F acces */
    GSxM_Acces(APIPC_CPU01_TO_CPU02_GSxRAM, IPC_GSX_CPU1_MASTER);
    /* CPU02 master GSxM blocks R/W/F acces */
    GSxM_Acces(APIPC_CPU02_TO_CPU01_GSxRAM ,IPC_GSX_CPU2_MASTER);

    /* Aknowledge Remote CPU that SGMEM was configured */
    IPCLtoRFlagSet(APIPC_FLAG_SRAM_ACCES);

#elif defined ( CPU2 )

    /*
     * CPU2 has nothing to do here besides wait
     */
    
    /* Wait here until CPU01 has started */
    while(IPCRtoLFlagBusy(APIPC_FLAG_SRAM_ACCES) != 1)
    {
	// TODO: this wait should be timed
    }

#endif
}

/* apipc_check_remote_cpu_init: */
static void apipc_check_remote_cpu_init(void)
{

#if defined ( CPU1 )

    /* apipc cpu1 inited */
    IPCLtoRFlagSet(APIPC_FLAG_API_INITED);

#elif defined ( CPU2 )

    /* Wait here until CPU1 has started */
    while(IPCRtoLFlagBusy(APIPC_FLAG_API_INITED) != 1)
    {
	// TODO: this wait should be timed
    }

    /* apipc cpu2 inited */
    IPCLtoRFlagSet(APIPC_FLAG_API_INITED);
#endif

}

/* apipc_init_objs: */
static void apipc_init_objs(void)
{
    uint16_t obj_idx;
    struct apipc_obj *plobj;

    plobj = l_apipc_obj;

    /* objs are initialized making sure that paddr == NULL */
    for(obj_idx = 0; obj_idx < APIPC_MAX_OBJ; obj_idx++, plobj++)
        plobj->paddr = NULL;
}


 /* apipc_init: Initialize ipc API  */
void apipc_init(void)
{

    /* Initialize peripheral IPC device to a known state */
    InitIpc();

    /* Initializes System IPC driver controller */
    IPCInitialize(&g_sIpcController1, IPC_INT0, IPC_INT0);
    IPCInitialize(&g_sIpcController2, IPC_INT1, IPC_INT1);

    /* Set GSxM blocks property */
    apipc_sram_acces_config();

    /* Initialize mymalloc handler to allocate cl_r_w_data data dynamically */
    l_r_w_data_h = mymalloc_init_array((void*)cl_r_w_data, (size_t)CL_R_W_DATA_LENGTH);

    /* Initialize circular_buffer  handler to manage an array of tIpcMessage dynamically */
    message_cbh = circular_buffer_init((void *)&message_array, sizeof(tIpcMessage),
                                  (uint16_t)APIPC_MAX_OBJ);

    /* initialize the objs array to a known state */
    apipc_init_objs();

    /* Set up IPC interrupts PIEIERx Registers */
    PieCtrlRegs.PIEIER1.bit.INTx13 = 1; // Set the apropropiate PIEIERx bit for IPC0
    PieCtrlRegs.PIEIER1.bit.INTx14 = 1; // Set the apropropiate PIEIERx bit for IPC1

    /* Acknowledge Local CPU start & wait here until Remote CPU init */
    apipc_check_remote_cpu_init();
}

/* apipc_register_obj: register data as an apipc obj to be able to be tranfer
 * between cores */
enum apipc_rc apipc_register_obj(uint16_t obj_idx, enum apipc_obj_type obj_type,
                                 void *paddr, size_t size, uint16_t startup)
{
    enum apipc_rc rc;
    struct apipc_obj *plobj;

    rc = APIPC_RC_SUCCESS;
    plobj = &l_apipc_obj[obj_idx];

    if(plobj->paddr != NULL)
        return APIPC_RC_FAIL;

    plobj->idx = obj_idx;
    plobj->type = obj_type;
    plobj->obj_sm = APIPC_OBJ_SM_UNKNOWN;
    plobj->paddr = paddr;
    plobj->len = size;

    if(startup)
        plobj->flag.startup = 1;
    else
        plobj->flag.startup = 0;

    return rc;
}

/* apipc_obj_state: consult the actual state of the obj_idx object sm. */
enum apipc_obj_sm apipc_obj_state(uint16_t obj_idx)
{
    struct apipc_obj *plobj;

    plobj = &l_apipc_obj[obj_idx];
    return plobj->obj_sm;
}


/* apipc_send: request transfer an object on demand. */
ram_func enum apipc_rc apipc_send(uint16_t obj_idx)
{
    enum apipc_rc rc;
    struct apipc_obj *plobj;

    rc = APIPC_RC_SUCCESS;
    plobj = &l_apipc_obj[obj_idx];

    if(plobj->obj_sm == APIPC_OBJ_SM_IDLE)
        plobj->obj_sm = APIPC_OBJ_SM_INIT;
    else
        rc = APIPC_RC_FAIL;

    return rc;
}

/* apipc_flags_set_bits: Sets the designated bits at the remote CPU obj */
enum apipc_rc apipc_flags_set_bits(uint16_t obj_idx, uint32_t bmask)
{

    enum apipc_rc rc;
    struct apipc_obj *plobj;
    struct apipc_obj *probj;

    rc = APIPC_RC_SUCCESS;
    plobj = &l_apipc_obj[obj_idx];
    probj = &r_apipc_obj[obj_idx];

    if(STATUS_FAIL == IPCLtoRSetBits(&g_sIpcController2, (uint32_t)probj->paddr, bmask, (uint16_t)plobj->len,
                DISABLE_BLOCKING))
        rc = APIPC_RC_FAIL;

    return rc;
}

/* apipc_flags_set_bits: Clear the designated bits at the remote CPU obj */
enum apipc_rc apipc_flags_clear_bits(uint16_t obj_idx, uint32_t bmask)
{

    enum apipc_rc rc;
    struct apipc_obj *plobj;
    struct apipc_obj *probj;

    rc = APIPC_RC_SUCCESS;
    plobj = &l_apipc_obj[obj_idx];
    probj = &r_apipc_obj[obj_idx];

    if(STATUS_FAIL == IPCLtoRClearBits(&g_sIpcController2, (uint32_t)probj->paddr, bmask, (uint16_t)plobj->len,
                DISABLE_BLOCKING))
        rc = APIPC_RC_FAIL;

    return rc;
}

/* apipc_write: to transmit a value to the remote core apipc interacts with the
 * ipc driver according to the obj type */
static enum apipc_rc apipc_write(uint16_t obj_idx)
{
    enum apipc_rc rc;

    struct apipc_obj *plobj;
    struct apipc_obj *probj;

    uint32_t ulData;
    uint32_t ulMask;

    /* initialize local variables */
    rc = APIPC_RC_SUCCESS;
    plobj = &l_apipc_obj[obj_idx];
    probj = &r_apipc_obj[obj_idx];

    /* Check that l & r objects were initialized */
    if( (probj->paddr == NULL) || (plobj->paddr == NULL) )
        return APIPC_RC_FAIL;

        /* request ipc api write according to the obj type */
    switch(plobj->type)
    {
        case APIPC_OBJ_TYPE_BLOCK:

            /* Allocates spaces for a block on the statically reserved mem space */
            plobj->pGSxM = (uint16_t *) mymalloc(l_r_w_data_h, plobj->len);

            if(plobj->pGSxM == NULL)
            {
                rc = APIPC_RC_FAIL;
                break; 
            }

            /* Place data to be writen in shared memory */
            u16memcpy(plobj->pGSxM, plobj->paddr, plobj->len);

            /* request ipc driver write */
            if(STATUS_FAIL == IPCLtoRBlockWrite(&g_sIpcController2,
                                                (uint32_t)probj->paddr, 
                                                (uint32_t)plobj->pGSxM,
                                                (uint16_t)plobj->len,
                                                IPC_LENGTH_16_BITS, DISABLE_BLOCKING))
            {
                myfree(l_r_w_data_h, plobj->pGSxM);
                plobj->pGSxM = NULL;
                rc = APIPC_RC_FAIL;
            }
            break;

        case APIPC_OBJ_TYPE_DATA:

            /* retrieve obj data length */
            if(plobj->len == IPC_LENGTH_16_BITS)
                ulData = (uint32_t) *(uint16_t *)plobj->paddr;

            else if(plobj->len > IPC_LENGTH_32_BITS)
                ulData = (uint32_t) *(uint32_t *)plobj->paddr;

            else
            {
                rc = APIPC_RC_FAIL;
                break;
            }

            /* request ipc driver write */
            if(STATUS_FAIL == IPCLtoRDataWrite(&g_sIpcController2,
                                               (uint32_t)probj->paddr,
                                               ulData, (uint16_t)plobj->len,
                                               DISABLE_BLOCKING, NO_FLAG))
                rc = APIPC_RC_FAIL;
            break;

        case APIPC_OBJ_TYPE_FLAGS:

            /* retrieve obj data length */
            if(plobj->len == IPC_LENGTH_16_BITS)
                ulMask = (uint32_t) *(uint16_t *)plobj->paddr;

            else if(plobj->len > IPC_LENGTH_32_BITS)
                ulMask = (uint32_t) *(uint32_t *)plobj->paddr;

            /* request ipc driver write */
            if( APIPC_RC_FAIL == apipc_flags_set_bits(plobj->idx, ulMask))
                rc = APIPC_RC_FAIL;

            /* request ipc driver write */
            if( APIPC_RC_FAIL == apipc_flags_clear_bits(plobj->idx, ~ulMask))
                rc = APIPC_RC_FAIL;

            break;

        case APIPC_OBJ_TYPE_FUNC_CALL:

            /* retrieve function obj type argument */
            ulData = (uint32_t) plobj->payload;

            /* request ipc driver write */
            if(STATUS_FAIL == IPCLtoRFunctionCall(&g_sIpcController2,
                                               (uint32_t)probj->paddr, ulData,
                                               DISABLE_BLOCKING))
                rc = APIPC_RC_FAIL;
            break;

        default:
            break;
    }

    return rc;
}

/* apipc_proc_obj - apipc obj state machine process */
static void apipc_proc_obj(struct apipc_obj *plobj)
{
    switch(plobj->obj_sm)
    {
        case APIPC_OBJ_SM_UNKNOWN:
            if(plobj->paddr == NULL)
            {
                plobj->obj_sm = APIPC_OBJ_SM_FREE;
                break;
            }
            if(!plobj->flag.startup)
            {
                plobj->obj_sm = APIPC_OBJ_SM_IDLE;
                break;
            }

        /* obj transmition process starst here */
        case APIPC_OBJ_SM_INIT:
        /* 
         * Every obj transmit process starts through this state.
         */
                plobj->retry = 3;
                plobj->obj_sm = APIPC_OBJ_SM_WRITING;

        case APIPC_OBJ_SM_WRITING:

            if(apipc_write(plobj->idx) == APIPC_RC_SUCCESS)
            {
                plobj->timer = ipc_read_timer();
                plobj->obj_sm = APIPC_OBJ_SM_WAITTING_RESPONSE;
            }
            else if (plobj->retry)
            {
                plobj->timer = ipc_read_timer();
                plobj->retry--;
                plobj->obj_sm = APIPC_OBJ_SM_RETRY;
            }
            else
            {
                if(plobj->pGSxM)
                {
                    myfree(l_r_w_data_h, plobj->pGSxM);
                    plobj->pGSxM = NULL;
                }

                plobj->obj_sm = APIPC_OBJ_SM_FAIL;
                plobj->flag.error = 1;
            }
            break;

        case APIPC_OBJ_SM_WAITTING_RESPONSE:

            if(ipc_timer_expired(plobj->timer, IPC_TIMER_WAIT_5mS))
            {
                if(plobj->pGSxM)
                {
                    myfree(l_r_w_data_h, plobj->pGSxM);
                    plobj->pGSxM = NULL;
                }

                if (plobj->retry)
                {
                    plobj->timer = ipc_read_timer();
                    plobj->retry--;
                    plobj->obj_sm = APIPC_OBJ_SM_RETRY;
                    break;
                }
                plobj->obj_sm = APIPC_OBJ_SM_FAIL;
                plobj->flag.error = 1;
            }
            break;

        case APIPC_OBJ_SM_RETRY:

            if(ipc_timer_expired(plobj->timer, IPC_TIMER_WAIT_5mS))
            {
                plobj->obj_sm = APIPC_OBJ_SM_WRITING;
            }
            break;

        case APIPC_OBJ_SM_FAIL:
            if(!plobj->flag.startup)
                plobj->obj_sm = APIPC_OBJ_SM_IDLE;
            break;

            /* obj is started and idle ready to transmit */
        case APIPC_OBJ_SM_IDLE:
            break;
            /* obj is free, do nothing */
        case APIPC_OBJ_SM_FREE:
            break;
    }
}

/* apipc_cmd_response - apipc response the received message over ipc to ack
 * reception */
static void apipc_cmd_response (tIpcMessage *psMessage)
{
    enum apipc_msg_cmd cmd_response;
     uint16_t *urAddess = NULL;
     uint32_t ulDataW1 = 0;
     uint32_t ulDataW2 = 0;

    cmd_response = (enum apipc_msg_cmd) psMessage->ulcommand;

    /* build response according to received ipc command */
    switch(cmd_response)
    {
        case APIPC_MSG_CMD_FUNC_CALL_RSP:
        case APIPC_MSG_CMD_SET_BITS_RSP:
        case APIPC_MSG_CMD_CLEAR_BITS_RSP:
        case APIPC_MSG_CMD_DATA_WRITE_RSP:
            urAddess = (uint16_t *) psMessage->uladdress;
            ulDataW1 = (uint32_t) cmd_response;
            break;

        case APIPC_MSG_CMD_BLOCK_READ_RSP:
            return;

        case APIPC_MSG_CMD_BLOCK_WRITE_RSP:
            urAddess = (uint16_t *) psMessage->uladdress;
            ulDataW1 = (uint32_t) cmd_response;
            break;

        case APIPC_MSG_CMD_DATA_READ_PROTECTED_RSP:
        case APIPC_MSG_CMD_SET_BITS_PROTECTED_RSP:
        case APIPC_MSG_CMD_CLEAR_BITS_PROTECTED_RSP:
        case APIPC_MSG_CMD_DATA_WRITE_PROTECTED_RSP:
        case APIPC_MSG_CMD_BLOCK_WRITE_PROTECTED_RSP:
            return;
    }

    /* request ipc driver write */
    IPCLtoRSendMessage(&g_sIpcController2,(uint32_t) APIPC_MESSAGE,
            (uint32_t) urAddess, ulDataW1, ulDataW2, DISABLE_BLOCKING);
}

/* apipc_message_handler - handle the received messege */
static void apipc_message_handler (tIpcMessage *psMessage)
{
    enum apipc_msg_cmd ulCommand;

    uint16_t obj_idx = 0;
    uint16_t *pusRAddress;

    struct apipc_obj *plobj;
    struct apipc_obj *probj;

    /* initialize local variables */
    plobj = &l_apipc_obj[obj_idx];
    probj = &r_apipc_obj[obj_idx];

    pusRAddress = (uint16_t *) psMessage->uladdress;
    ulCommand = (enum apipc_msg_cmd) psMessage->uldataw1;

    /* search the obj_idx corresponding to the response */
    for(obj_idx = 0; obj_idx < APIPC_MAX_OBJ; plobj++, probj++, obj_idx++)
        if(pusRAddress == probj->paddr)
            break;

    /* take actions according to the received ipc command */
    switch(ulCommand)
    {
        case APIPC_MSG_CMD_FUNC_CALL_RSP:
        	pusRAddress = probj->paddr;
            break;

        case APIPC_MSG_CMD_SET_BITS_RSP:
        case APIPC_MSG_CMD_CLEAR_BITS_RSP:
        case APIPC_MSG_CMD_DATA_WRITE_RSP:
        case APIPC_MSG_CMD_BLOCK_READ_RSP:
            break;

        case APIPC_MSG_CMD_BLOCK_WRITE_RSP:
            if(plobj->pGSxM)
            {
                myfree(l_r_w_data_h, plobj->pGSxM);
                plobj->pGSxM = NULL;
            }
            break;

        case APIPC_MSG_CMD_DATA_READ_PROTECTED_RSP:
        case APIPC_MSG_CMD_SET_BITS_PROTECTED_RSP:
        case APIPC_MSG_CMD_CLEAR_BITS_PROTECTED_RSP:
        case APIPC_MSG_CMD_DATA_WRITE_PROTECTED_RSP:
        case APIPC_MSG_CMD_BLOCK_WRITE_PROTECTED_RSP:
            break;
    }
    
    /* evolve obj sm */
    switch (plobj->obj_sm)
    {
        case APIPC_OBJ_SM_IDLE:
            break;

        case APIPC_OBJ_SM_WAITTING_RESPONSE:
            plobj->obj_sm = APIPC_OBJ_SM_IDLE;
            break;

        default:
            plobj->obj_sm = APIPC_OBJ_SM_UNKNOWN;
            break;
    }
}

/* apipc_app - apipc application */
void apipc_app(void)
{
    static enum apipc_sm apipc_app_sm = APIPC_SM_UNKNOWN;

    static struct apipc_obj *plobj;
    uint16_t obj_idx;

    plobj = l_apipc_obj;

    apipc_process_messages();

    switch(apipc_app_sm)
    {
        case APIPC_SM_UNKNOWN:
            if(IPCRtoLFlagBusy(APIPC_FLAG_API_INITED) && IPCLtoRFlagBusy(APIPC_FLAG_API_INITED))
#if defined( CPU2 )
                if(IPCRtoLFlagBusy(APIPC_FLAG_APP_START))
#endif
                    apipc_app_sm = APIPC_SM_STARTUP_REMOTE;
            break;

        case APIPC_SM_STARTUP_REMOTE:
            if(!apipc_startup_remote())
            {
                apipc_app_sm = APIPC_SM_STARTED;
                IPCLtoRFlagSet(APIPC_FLAG_APP_START);
            }
            break;

        case APIPC_SM_STARTED:

            for(obj_idx = 0; obj_idx < APIPC_MAX_OBJ; obj_idx++, plobj++)
                apipc_proc_obj(plobj);

            break;

        case APIPC_SM_IDLE:
            break;
    }

}

/* apipc_startup_remote - Initialize local object data on the remote core */
enum apipc_rc apipc_startup_remote(void)
{
    enum apipc_rc rc;
    struct apipc_obj *plobj;
    uint16_t obj_idx;

    plobj = l_apipc_obj;
    rc = APIPC_RC_SUCCESS;

    for(obj_idx = 0; obj_idx < APIPC_MAX_OBJ; obj_idx++, plobj++)
    {
        apipc_proc_obj(plobj);

        if(plobj->obj_sm != APIPC_OBJ_SM_FREE && plobj->obj_sm != APIPC_OBJ_SM_IDLE)
        	rc = APIPC_RC_FAIL;
    }

    return rc;
}

/* apipc_process_messages - apipc interacs here with ipc driver on received
 * messages and take action according to the command */
static enum apipc_rc apipc_process_messages(void)
{
    enum apipc_rc rc;
    tIpcMessage sMessage;

    rc = APIPC_RC_SUCCESS;

    if(!circular_buffer_pop(message_cbh, (void *)&sMessage))
    {
        switch(sMessage.ulcommand) 
        {
            case IPC_FUNC_CALL:
                IPCRtoLFunctionCall(&sMessage);
                apipc_cmd_response(&sMessage);
                break;

            case IPC_DATA_WRITE:
                IPCRtoLDataWrite(&sMessage);
                apipc_cmd_response (&sMessage);
                break;

            case IPC_BLOCK_READ:
                IPCRtoLBlockRead(&sMessage);
                apipc_cmd_response (&sMessage);
                break;

            case IPC_BLOCK_WRITE:
                IPCRtoLBlockWrite(&sMessage);
                apipc_cmd_response (&sMessage);
                break;

            case IPC_SET_BITS:
                IPCRtoLSetBits(&sMessage);
                apipc_cmd_response (&sMessage);
                break;

            case IPC_CLEAR_BITS:
                IPCRtoLClearBits(&sMessage);
                apipc_cmd_response (&sMessage);
                break;

            case APIPC_MESSAGE:
                apipc_message_handler(&sMessage);
                break;

            default:
                rc = APIPC_RC_FAIL;
                break;
        }
    }
    return rc;
}

#if defined( CPU1 )

#elif defined(CPU2)

#endif

//
// RtoLIPC0IntHandler - Handler writes into CPU01 addesses as a result of
// read commands to CPU02.
interrupt void apipc_ipc0_isr_handler(void)
{
    tIpcMessage sMessage;

    //
    // Continue processing messages as long as CPU01 to CPUE02
    // GetBuffer1 if full
    //
    while(IpcGet(&g_sIpcController1, &sMessage,
		DISABLE_BLOCKING) != STATUS_FAIL)
    {
	switch(sMessage.ulcommand)
	{
	    case IPC_DATA_WRITE:
		IPCRtoLDataWrite(&sMessage);
		break;
	    default:
		break;
	}	

    }
    /* Acknowledge IC INT0 Flag */
    IpcRegs.IPCACK.bit.IPC0 = 1;

    /* acknowledge the PIE group interrupt. */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

//
// RtoLIPC1IntHandler - Handles Data Block Reads/Writes
// 
interrupt void apipc_ipc1_isr_handler(void)
{
    tIpcMessage sMessage;
    //
    // Get messages from driver as long as GetBuffer2 is full and store on the
    // apipc circullar buffer to be processed
    //
    while(IpcGet(&g_sIpcController2, &sMessage, DISABLE_BLOCKING)!= STATUS_FAIL)
        circular_buffer_put(message_cbh, (void *)&sMessage);

    /* Acknowledge IC INT1 Flag */
    IpcRegs.IPCACK.bit.IPC1 = 1;
    
    /* acknowledge the PIE group interrupt. */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

//
// End of the file.
//
