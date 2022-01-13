/******************************************************************************
 *                                                                             *
 *    file     : Ipc_Driver_app.c
 *    project  : Dual Core Inverter KMT.pjt
 *    authors  : Federico D. Ceccarelli	 
 *                                                                             *
 *******************************************************************************
 *    Description:
 *		  This is an implementation of the IPC driver for the KMT
 *		  dual core project purposes.	
 *	
 *******************************************************************************
 * Version 1.0 	10/05/2021
 *******************************************************************************/

#include "F2837xD_device.h"        // F2837xD Headerfile Include File
#include "F2837xD_Examples.h"      // F2837xD Examples Include File

#include "ipc.h"
#include "../lib/mymalloc/mymalloc.h"

/* ipclib shared buffers space allocation */
// NOTE: space is allocated depending on the .cmd file included in the project
#pragma DATA_SECTION(cl_r_w_data,".cpul_cpur_data");
#pragma DATA_SECTION(l_apipc_obj,".base_cpul_cpur_addr");
#pragma DATA_SECTION(r_apipc_obj,".base_cpur_cpul_addr");

/* ipclib shared buffers space declaration */
uint16_t  cl_r_w_data[CL_R_W_DATA_LENGTH];            // mapped to .cpul_cpur_data of shared RAM owned by local cpu.
struct apipc_obj l_apipc_obj[APIPC_MAX_OBJ]; // mapped to .base_cpul_cpur_addr of shared RAM owned by local cpu.
struct apipc_obj r_apipc_obj[APIPC_MAX_OBJ];   // mapped to .base_cpur_cpul_addr of shared RAM owned by remote cpu.

/* IPC API Drivers handlers declaration. */
// NOTE: Should be declared one handler for every interrupt
volatile tIpcController g_sIpcController1;
volatile tIpcController g_sIpcController2;

/* mymalloc API handler declaration. */
mymalloc_handler l_r_w_data_h;

/* statics functions prototipes declarations */
static void apipc_sram_acces_config(void);
static void apipc_check_remote_cpu_init(void);
static void apipc_init_objs(void);
static void apipc_proc_obj(struct apipc_obj *plobj);

/* apipc_sram_acces_config: */
static void apipc_sram_acces_config(void)
{
    /**
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

    for(obj_idx = 0; obj_idx < APIPC_MAX_OBJ; obj_idx++)
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

    /* initialize the objs array to a known state */
    apipc_init_objs();

    /* Set up IPC interrupts PIEIERx Registers */
    PieCtrlRegs.PIEIER1.bit.INTx13 = 1; // Set the apropropiate PIEIERx bit for IPC0
    PieCtrlRegs.PIEIER1.bit.INTx14 = 1; // Set the apropropiate PIEIERx bit for IPC1

    /* Acknowledge Local CPU start & wait here until Remote CPU init */
    apipc_check_remote_cpu_init();
}

/* apipc_register_obj: register an obj parameters to be able to tranfer */
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

/* apipc_send: */
enum apipc_rc apipc_send(uint16_t obj_idx)
{
    enum apipc_rc rc;

    struct apipc_obj *plobj;
    struct apipc_obj *probj;

    uint32_t ulData;
    uint32_t ulMask;

    rc = APIPC_RC_SUCCESS;
    plobj = &l_apipc_obj[obj_idx];
    probj = &r_apipc_obj[obj_idx];

    /* Check that l & r objects are initialized */
    if( (probj->paddr == NULL) || (plobj->paddr == NULL) )
        return APIPC_RC_FAIL;

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

            // Place the data to write in shared memory
            u16memcpy(plobj->pGSxM, plobj->paddr, plobj->len);

            if(STATUS_FAIL == IPCLtoRBlockWrite(&g_sIpcController2,
                                                (uint32_t)probj->paddr, 
                                                (uint32_t)plobj->pGSxM,
                                                (uint16_t)plobj->len,
                                                IPC_LENGTH_16_BITS, DISABLE_BLOCKING))
            {
                myfree(l_r_w_data_h, plobj->pGSxM);
                plobj->pGSxM = NULL;
                rc = APIPC_RC_FAIL;
                break;
            }
            break;

        case APIPC_OBJ_TYPE_DATA:

            if(plobj->len == IPC_LENGTH_16_BITS)
                ulData = (uint32_t) *(uint16_t *)plobj->paddr;

            else if(plobj->len > IPC_LENGTH_32_BITS)
                ulData = (uint32_t) *(uint32_t *)plobj->paddr;

            else
            {
                rc = APIPC_RC_FAIL;
                break;
            }

            if(STATUS_FAIL == IPCLtoRDataWrite(&g_sIpcController2,
                                               (uint32_t)probj->paddr,
                                               ulData, (uint16_t)plobj->len,
                                               DISABLE_BLOCKING, NO_FLAG))
                rc = APIPC_RC_FAIL;
            break;

        case APIPC_OBJ_TYPE_FLAGS:

            if(plobj->len == IPC_LENGTH_16_BITS)
                ulMask = (uint32_t) *(uint16_t *)plobj->paddr;

            else if(plobj->len > IPC_LENGTH_32_BITS)
                ulMask = (uint32_t) *(uint32_t *)plobj->paddr;

            if( APIPC_RC_FAIL == apipc_flags_set_bits(plobj->idx, ulMask))
                rc = APIPC_RC_FAIL;

            if( APIPC_RC_FAIL == apipc_flags_clear_bits(plobj->idx, ~ulMask))
                rc = APIPC_RC_FAIL;

            break;

        default:
            break;
    }

    return rc;
}

static void apipc_proc_obj(struct apipc_obj *plobj)
{
    switch(plobj->obj_sm)
    {
        case APIPC_OBJ_SM_UNKNOWN:
            if(plobj->paddr == NULL)
                break;

        case APIPC_OBJ_SM_INIT:
            if(plobj->flag.startup)
            {
                plobj->retry = 3;
                plobj->obj_sm = APIPC_OBJ_SM_WRITING;
            }
            else
                plobj->obj_sm = APIPC_OBJ_SM_STARTED;
            
            break;

        case APIPC_OBJ_SM_WRITING:

            if(apipc_send(plobj->idx) == APIPC_RC_SUCCESS)
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
                plobj->obj_sm = APIPC_OBJ_SM_IDLE;

            break;

        case APIPC_OBJ_SM_WAITTING_RESPONSE:

            if(ipc_timer_expired(plobj->timer, IPC_TIMER_WAIT_5mS))
            {
                myfree(l_r_w_data_h, plobj->pGSxM);
                plobj->pGSxM = NULL;
                plobj->obj_sm = APIPC_OBJ_SM_IDLE;
            }
            break;

        case APIPC_OBJ_SM_RETRY:

            if(ipc_timer_expired(plobj->timer, IPC_TIMER_WAIT_5mS))
            {
                plobj->obj_sm = APIPC_OBJ_SM_WRITING;
            }
            break;

        case APIPC_OBJ_SM_STARTED:
            break;

        case APIPC_OBJ_SM_IDLE:
            break;
    }
}

#if defined( CPU1 )

enum apipc_rc apipc_startup_config(void)
{
    static enum apipc_rc rc;
    static enum apipc_startup_sm apipc_startup_sm = APIPC_SU_SM_UNKNOWN;
    static struct apipc_obj *plobj;

    static uint16_t obj_idx;
    static uint16_t startup_finished = 0;


    switch(apipc_startup_sm)
    {
        case APIPC_SU_SM_UNKNOWN:
            apipc_startup_sm = APIPC_SU_SM_INIT;

        case APIPC_SU_SM_INIT:

            rc = APIPC_RC_FAIL;
            startup_finished = 1;
            plobj = l_apipc_obj;
            obj_idx = 0;
            apipc_startup_sm = APIPC_SU_SM_STARTING;
            break;

        case APIPC_SU_SM_STARTING:

            apipc_proc_obj(plobj);

            if(plobj->obj_sm != APIPC_OBJ_SM_UNKNOWN && plobj->obj_sm != APIPC_OBJ_SM_STARTED)
                startup_finished = 0;

            plobj++;
            obj_idx++;

            if(obj_idx >= APIPC_MAX_OBJ)
            {
                if(startup_finished)
                    apipc_startup_sm = APIPC_SU_SM_FINISHED;
                else
                    apipc_startup_sm = APIPC_SU_SM_INIT;
            }
            break;

        case APIPC_SU_SM_FINISHED:
            rc = APIPC_RC_SUCCESS;
            break;

    }

    return rc;
}


void apipc_app(void)
{
    static enum apipc_sm apipc_app_sm = APIPC_SM_UNKNOWN;

    switch(apipc_app_sm)
    {
        case APIPC_SM_UNKNOWN:
            if(IPCRtoLFlagBusy(APIPC_FLAG_API_INITED) && IPCLtoRFlagBusy(APIPC_FLAG_API_INITED))
                apipc_app_sm = APIPC_SM_STARTUP_REMOTE_CONFIG;
            break;

        case APIPC_SM_IDLE:
            break;

        case APIPC_SM_STARTUP_REMOTE_CONFIG:
            if(!apipc_startup_config())
                apipc_app_sm = APIPC_SM_STARTED;
            break;

        case APIPC_SM_STARTED:
            break;
    }

}

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
    // Continue proccessing messages as long as GetBuffer2 is full
    //
    while(IpcGet(&g_sIpcController2, &sMessage, DISABLE_BLOCKING)!= STATUS_FAIL)
    {
        switch(sMessage.ulcommand) 
        {
            case IPC_DATA_WRITE:
                IPCRtoLDataWrite(&sMessage);
                break;

            case IPC_BLOCK_READ:
                IPCRtoLBlockRead(&sMessage);
                break;

            case IPC_BLOCK_WRITE:
                IPCRtoLBlockWrite(&sMessage);
                break;

            case IPC_SET_BITS:
                IPCRtoLSetBits(&sMessage);
                break;

            case IPC_CLEAR_BITS:
                IPCRtoLClearBits(&sMessage);
                break;

            default:
                break;
        }
    }

    /* Acknowledge IC INT1 Flag */
    IpcRegs.IPCACK.bit.IPC1 = 1;
    
    /* acknowledge the PIE group interrupt. */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

//
// End of the file.
//
