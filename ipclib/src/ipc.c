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
#include "mymalloc.h"

#include "objects.h"

#if defined(CPU1)
// Shared buffer mem zone declaration
#pragma DATA_SECTION(cl_r_w_data,".cpu1_cpu2_data");
#pragma DATA_SECTION(cl_r_w_addr,".base_cpu1_cpu2_addr");
#pragma DATA_SECTION(cr_r_addr,".base_cpu2_cpu1_addr");

#elif defined(CPU2)
// Shared buffer mem zone declaration
#pragma DATA_SECTION(cl_r_w_data,".cpu2_cpu1_data");
#pragma DATA_SECTION(cl_r_w_addr,"base_cpu2_cpu1_addr");
#pragma DATA_SECTION(cr_r_addr,"base_cpu1_cpu2_addr");

#endif
//
// Globals
//
struct ipcda_addr_obj cr_r_addr[IPCDA_RADDR_TOTAL];   // mapped to GS0 of shared RAM owned by CPU02
struct ipcda_addr_obj cl_r_w_addr[IPCDA_RADDR_TOTAL]; // mapped to GS1 of shared RAM owned by CPU01

uint16_t  cl_r_w_data[CL_R_W_DATA_LENGTH];

//
// IPC API Drivers handlers declaration.
// One handler should be declared for every interrupt
//
volatile tIpcController g_sIpcController1;
volatile tIpcController g_sIpcController2;

mymalloc_handler l_r_w_data_h;

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

static void IpcDa_GSxM_Master_Sel(void)
{
    /**
     * Each CPU owns different GSxM blocks of memory to send data & pointer address
     * to the remote core.
     * If a core master a block of GSxM RAM it can R/W/Fetc it.
     * If a core doesnt master a block o GSxM RAM it can only read the memory
     * block.
     */

#if defined(CPU1)
    // CPU01 master GSxM blocks R/W/F acces 
    GSxM_Acces(CPU01_TO_CPU02_GSxRAM, IPC_GSX_CPU1_MASTER);
    // CPU02 master GSxM blocks R/W/F acces 
    GSxM_Acces(CPU02_TO_CPU01_GSxRAM ,IPC_GSX_CPU2_MASTER);

    // Aknowledge Remote CPU that SGMEM was configured
    IPCLtoRFlagSet(IPCDA_FLAG_GSMEM_ACCES);
#elif defined(CPU2)
    //
    // Wait here until CPU01 has started
    //
    while(IPCRtoLFlagBusy(IPCDA_FLAG_GSMEM_ACCES) != 1)
    {
	// TODO: this wait should be timed
    }

    // Acknowledge CPU1 GSM mem acces
    IPCRtoLFlagAcknowledge (IPCDA_FLAG_GSMEM_ACCES);
#endif
}

static void IpcDa_CheckRemoteCPUInit(void){

#if defined(CPU1)
    //
    // Wait here until CPU02 has tarted
    //
    while(IPCRtoLFlagBusy(IPCDA_FLAG_R_CPU_INIT) != 1)
    {
	// TODO: this wait should be timed
    }
    // Acknowledge CPU1 Init
    IPCRtoLFlagAcknowledge (IPCDA_FLAG_R_CPU_INIT);
#elif defined (CPU2)
    // Acknowledge CPU2 Init
    IPCLtoRFlagSet(IPCDA_FLAG_R_CPU_INIT);
    //
    // Wait here until CPU01 has started
    //
    while(IPCRtoLFlagBusy(IPCDA_FLAG_R_CPU_INIT) != 0)
    {
	// TODO: this wait should be timed
    }
#endif

}

/**
 * GSxM_register_l_r_w_addr()
 */
//TODO: desarrollar una funcion que me permita registar la dirección de una
//variable en el lugar correspondiente segund estructura
void GSxM_register_l_r_w_addr(enum ipc_driver_app_addr_ind ind, uint32_t paddr, size_t size)
{
    cl_r_w_addr[ind].addr = paddr;
    cl_r_w_addr[ind].len = size;
}

static void IpcDa_Load_Addr(void)
{
    extern Uint16 F_cmd_ON;

    GSxM_register_l_r_w_addr(IPCDA_RADDR_CONFIG, (uint32_t) &config, sizeof(config));
    GSxM_register_l_r_w_addr(IPCDA_RADDR_VARS, (uint32_t) &vars, sizeof(vars));
    GSxM_register_l_r_w_addr(IPCDA_RADDR_VARS_RATED, (uint32_t) &vars_rated, sizeof(vars_rated));
    GSxM_register_l_r_w_addr(IPCDA_RADDR_CMD_ON, (uint32_t) &F_cmd_ON, sizeof(F_cmd_ON)); // TODO: Eliminar cuando las solucione como implementar las banderas

}

/**
 * Initialize IPC Driver Application
 */
void IpcDa_Init(void){

    // Initializes System IPC driver controller
    IPCInitialize(&g_sIpcController1, IPC_INT0, IPC_INT0);
    IPCInitialize(&g_sIpcController2, IPC_INT1, IPC_INT1);

    // Set GSxM blocks property
    IpcDa_GSxM_Master_Sel();

    // Initialize pointers arrays
    IpcDa_Load_Addr();
    
    // Initialize mymalloc handler to allocate cl_r_w_data data dynamically
    l_r_w_data_h = mymalloc_init_array((void*)cl_r_w_data, (size_t)CL_R_W_DATA_LENGTH);

    // Set up IPC interrupts PIEIERx Registers
    // Set the apropropiate PIEIERx bit for IPC0
    PieCtrlRegs.PIEIER1.bit.INTx13 = 1;
    // Set the apropropiate PIEIERx bit for IPC1
    PieCtrlRegs.PIEIER1.bit.INTx14 = 1;

    //
    // Acknowledge Local CPU start & wait here until Remote CPU init
    //
    IpcDa_CheckRemoteCPUInit();
}

#if defined(CPU1)

void IpcDa_Init_r_config(void)
{
    static enum ipcda_sm IpcDa_SM = IPCDA_SM_UNKNOWN;
    uint16_t f_end_r_config = 0;
    uint16_t block_idx;

    while(!f_end_r_config)
    {
	switch(IpcDa_SM)
	{
	    case IPCDA_SM_UNKNOWN:
		block_idx = 0;
		f_end_r_config = 0;
		IpcDa_SM = IPCDA_SM_IDLE;
		break;

	    case IPCDA_SM_IDLE:
		if(block_idx < IPCDA_RADDR_TOTAL)
		    IpcDa_SM = IPCDA_SM_WRITING_CONFIG;
		else
		    f_end_r_config = 1;
		break;

	    case IPCDA_SM_WRITING_CONFIG:
		IpcDa_sendblock((void *)cl_r_w_addr[block_idx].addr, cl_r_w_addr[block_idx].len, (uint16_t *)0x0F000, (void *)cr_r_addr[block_idx].addr, IPCDA_FLAG_R_L_BLOCK_RECEIVED);
		IpcDa_SM = IPCDA_SM_WAITTING_REMOTE_RESPONSE;
		break;

	    case IPCDA_SM_WAITTING_REMOTE_RESPONSE:
		if(!IPCRtoLFlagBusy(IPCDA_FLAG_R_L_BLOCK_RECEIVED))
		{
		    block_idx++;
		    IpcDa_SM = IPCDA_SM_IDLE;
		}
		break;
	}
    }
}

void IpcDa_app(void)
{

    static enum ipcda_sm IpcDa_SM = IPCDA_SM_UNKNOWN;

    switch(IpcDa_SM)
    {
	case IPCDA_SM_UNKNOWN:
	    break;

	case IPCDA_SM_IDLE:

	case IPCDA_SM_WRITING_CONFIG:
	    break;

	case IPCDA_SM_READING_CONFIG:
	    break;

	case IPCDA_SM_WAITTING_REMOTE_RESPONSE:
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
