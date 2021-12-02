
#ifndef __IPC_DEFS_H__
#define __IPC_DEFS_H__

/*
 * Defines
 */
// gsxm blocks that master cpu1 reserverd for ipc driver app
#define CPU01_TO_CPU02_GSxRAM GS3_ACCESS|GS4_ACCESS|GS5_ACCESS
// gsxm blocks that master cpu2 reserverd for ipc driver app
#define CPU02_TO_CPU01_GSxRAM GS1_ACCESS|GS2_ACCESS|GS6_ACCESS

//GS3SARAM Start Address
#define CPU01_TO_CPU02_R_W_DATA_START (uint32_t)0x0000F000  

// CPU01 to CPU02 Local Addresses MSG RAM off sets
#define CPU01_TO_CPU02_R_W_ADDR (uint32_t)0x00011000  					              // for passing address
					
#define CPU02_TO_CPU01_R_W_DATA_START (uint32_t)0x0000D000 

// CPU02 to CPU01 Local Addresses MSG RAM offsets
#define CPU02_TO_CPU01_R_W_ADDR (uint32_t)0x00012000  

// Local R_W_DATA length space
#define CL_R_W_DATA_LENGTH 4096

enum ipcda_sm
{
    IPCDA_SM_UNKNOWN = 0,
    IPCDA_SM_IDLE,
    IPCDA_SM_READING_CONFIG,
    IPCDA_SM_WRITING_CONFIG,
    IPCDA_SM_WAITTING_REMOTE_RESPONSE,

};

enum ipc_driver_app_addr_ind
{
    IPCDA_RADDR_CONFIG = 0,
    IPCDA_RADDR_VARS,
    IPCDA_RADDR_VARS_RATED,
    IPCDA_RADDR_TOTAL,
    IPCDA_RADDR_CMD_ON,

};

enum ipc_driver_app_flags
{
    IPCDA_FLAG_R_CPU_INIT = IPC_FLAG17,
    IPCDA_FLAG_GSMEM_ACCES = IPC_FLAG18,
    IPCDA_FLAG_CPU1_L_R_ADDR = IPC_FLAG19,
    IPCDA_FLAG_CPU2_L_R_ADDR = IPC_FLAG20,
    IPCDA_FLAG_R_L_BLOCK_RECEIVED = IPC_FLAG21,
};

#endif

struct ipcda_addr_obj
{
    uint32_t addr;
    size_t len;
};

//
// End of file.
//
