
#ifndef __IPC_DEFS_H__
#define __IPC_DEFS_H__

/*
 * Defines
 */
// gsxm blocks that master cpu1 reserverd for ipc driver app
#define APIPC_CPU01_TO_CPU02_GSxRAM GS3_ACCESS|GS4_ACCESS|GS5_ACCESS
// gsxm blocks that master cpu2 reserverd for ipc driver app
#define APIPC_CPU02_TO_CPU01_GSxRAM GS1_ACCESS|GS2_ACCESS|GS6_ACCESS

//GS3SARAM Start Address
#define CPU01_TO_CPU02_R_W_DATA_START (uint32_t)0x0000F000  

// CPU01 to CPU02 Local Addresses MSG RAM off sets
#define CPU01_TO_CPU02_R_W_ADDR (uint32_t)0x00011000  					              // for passing address
					
#define CPU02_TO_CPU01_R_W_DATA_START (uint32_t)0x0000D000 

// CPU02 to CPU01 Local Addresses MSG RAM offsets
#define CPU02_TO_CPU01_R_W_ADDR (uint32_t)0x00012000  

// Local R_W_DATA length space
#define CL_R_W_DATA_LENGTH 4096

enum apipc_sm
{
    APIPC_SM_UNKNOWN = 0,
    APIPC_SM_IDLE,
    APIPC_SM_READING_CONFIG,
    APIPC_SM_WRITING_CONFIG,
    APIPC_SM_WAITTING_REMOTE_RESPONSE,

};

enum apipc_addr_ind
{
    APIPC_RADDR_CONFIG = 0,
    APIPC_RADDR_VARS,
    APIPC_RADDR_VARS_RATED,
    APIPC_RADDR_TOTAL,
};

enum apipc_flags
{
    APIPC_FLAG_API_INITED = IPC_FLAG4, /* Local API implementation is inited */
    APIPC_FLAG_SRAM_ACCES = IPC_FLAG5, /* Local (CPU1) granted GSMEM acces to CPU2 */


    IPC_FLAG_L_R_ADDR = IPC_FLAG19,
    IPC_FLAG_BLOCK_RECEIVED = IPC_FLAG21,
};

#endif

struct apipc_addr_obj
{
    uint32_t addr;
    size_t len;
};

//
// End of file.
//
