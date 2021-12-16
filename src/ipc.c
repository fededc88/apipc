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

static void apipc_check_remote_cpu_init(void){

#if defined ( CPU1 )

    /* apipc cpu1 inited */
    IPCLtoRFlagSet(APIPC_FLAG_API_INITED);

#elif defined ( CPU2 )

    /* Wait here until CPU1 has started */
    while(IPCRtoLFlagBusy(APIPC_FLAG_API_INITED) != 0)
    {
	// TODO: this wait should be timed
    }

    /* apipc cpu2 inited */
    IPCLtoRFlagSet(APIPC_FLAG_API_INITED);
#endif

}

 /* apipc_init: Initialize ipc API  */
void apipc_init(void)
{
    /* Initializes System IPC driver controller */
    IPCInitialize(&g_sIpcController1, IPC_INT0, IPC_INT0);
    IPCInitialize(&g_sIpcController2, IPC_INT1, IPC_INT1);

    /* Set GSxM blocks property */
    apipc_sram_acces_config();

    /* Initialize mymalloc handler to allocate cl_r_w_data data dynamically */
    l_r_w_data_h = mymalloc_init_array((void*)cl_r_w_data, (size_t)CL_R_W_DATA_LENGTH);

    /* Set up IPC interrupts PIEIERx Registers */
    PieCtrlRegs.PIEIER1.bit.INTx13 = 1; // Set the apropropiate PIEIERx bit for IPC0
    PieCtrlRegs.PIEIER1.bit.INTx14 = 1; // Set the apropropiate PIEIERx bit for IPC1

    /* Acknowledge Local CPU start & wait here until Remote CPU init */
    apipc_check_remote_cpu_init();
}

/**
 * GSxM_register_l_r_w_addr()
 */
//TODO: desarrollar una funcion que me permita registar la dirección de una
//variable en el lugar correspondiente segund estructura
void apipc_new_obj(uint16_t obj_idx, enum apipc_obj_type, void *paddr, size_t size)
{
    
    if( l_apipc_obj[obj_idx].addr != NULL)
        return;

    l_apipc_obj[obj_idx].addr = paddr;
    l_apipc_obj[obj_idx].len = size;
}

static void IpcDa_Load_Addr(void)
{
    extern Uint16 F_cmd_ON;

    GSxM_register_l_r_w_addr(APIPC_RADDR_CONFIG, (uint32_t) &config, sizeof(config));
    GSxM_register_l_r_w_addr(APIPC_RADDR_VARS, (uint32_t) &vars, sizeof(vars));
    GSxM_register_l_r_w_addr(APIPC_RADDR_VARS_RATED, (uint32_t) &vars_rated, sizeof(vars_rated));

}

#if defined( CPU1 )

void IpcDa_Init_r_config(void)
{
    static enum apipc_sm IpcDa_SM = APIPC_SM_UNKNOWN;
    uint16_t f_end_r_config = 0;
    uint16_t block_idx;

    while(!f_end_r_config)
    {
	switch(IpcDa_SM)
	{
	    case APIPC_SM_UNKNOWN:
		block_idx = 0;
		f_end_r_config = 0;
		IpcDa_SM = APIPC_SM_IDLE;
		break;

	    case APIPC_SM_IDLE:
		if(block_idx < APIPC_RADDR_TOTAL)
		    IpcDa_SM = APIPC_SM_WRITING_CONFIG;
		else
		    f_end_r_config = 1;
		break;

	    case APIPC_SM_WRITING_CONFIG:
		IpcDa_sendblock((void *)l_apipc_obj[block_idx].addr, l_apipc_obj[block_idx].len, (uint16_t *)0x0F000, (void *)r_apipc_obj[block_idx].addr, IPC_FLAG_BLOCK_RECEIVED);
		IpcDa_SM = APIPC_SM_WAITTING_REMOTE_RESPONSE;
		break;

	    case APIPC_SM_WAITTING_REMOTE_RESPONSE:
		if(!IPCRtoLFlagBusy(IPC_FLAG_BLOCK_RECEIVED))
		{
		    block_idx++;
		    IpcDa_SM = APIPC_SM_IDLE;
		}
		break;
	}
    }
}

//
// Write Memory Block through GSRAM  
//
// NOTE: This is an experimental test
static void IpcDa_sendblock(void *pdata, size_t ndata, uint16_t *pGSxM, void *premdata, uint32_t ulResponseFlag)
{
    // Place the data to write in shared memory
    u16memcpy(pGSxM, pdata, ndata);
   
    IPCLtoRBlockWrite(&g_sIpcController2,(uint32_t)premdata, (uint32_t)pGSxM,
	                  (uint16_t)ndata, IPC_LENGTH_16_BITS, ENABLE_BLOCKING, ulResponseFlag);
}

void apipc_app(void)
{

    static enum apipc_sm IpcDa_SM = APIPC_SM_UNKNOWN;

    switch(IpcDa_SM)
    {
	case APIPC_SM_UNKNOWN:
	    break;

	case APIPC_SM_IDLE:

	case APIPC_SM_WRITING_CONFIG:
	    break;

	case APIPC_SM_READING_CONFIG:
	    break;

	case APIPC_SM_WAITTING_REMOTE_RESPONSE:
	    break;
    }

}
#elif defined(CPU2)
void IpcDa_Init_r_config(void)
{}
#endif


//
// RtoLIPC0IntHandler - Handler writes into CPU01 addesses as a result of
// read commands to CPU02.
void RtoLIPC0IntHandler(void)
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
    //
    // Acknowledge IC INT0 Flag 
    //
    IpcRegs.IPCACK.bit.IPC0 = 1;

}

//
// RtoLIPC1IntHandler - Handles Data Block Reads/Writes
// 
void RtoLIPC1IntHandler(void)
{
    tIpcMessage sMessage;

    //
    // Continue proccessing messages as long as GetBuffer2 is full
    //
    while(IpcGet(&g_sIpcController2, &sMessage, DISABLE_BLOCKING)!= STATUS_FAIL)
    {
	switch(sMessage.ulcommand) 
	{

	    case IPC_BLOCK_WRITE:
		IPCRtoLBlockWrite(&sMessage);
		break;
	    case IPC_BLOCK_READ:
		IPCRtoLBlockRead(&sMessage);
		break;
	    default:
		break;
	}
    }

    //
    // Acknowledge IC INT1 Flag 
    //
    IpcRegs.IPCACK.bit.IPC1 = 1;
}

//
// End of the file.
//
