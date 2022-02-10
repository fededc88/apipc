
#ifndef __IPC_DEFS_H__
#define __IPC_DEFS_H__

/*
 * Defines
 */

/*
 * The ramfunc attribute specifies that a function will be placed in and
 * executed from RAM. The ramfunc // attribute allows the compiler to
 * optimize functions for RAM execution.
 * Use example:
 *		__attribute__((ramfunc))
 *		void f(void) {
 *		...
 *		}
 */
#if defined(_FLASH)
#ifndef ram_func
#if __TI_COMPILER_VERSION__ >= 16006000
#define ram_func __attribute__((ramfunc))
#else
#define ram_func 
#endif
#endif
#endif

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

// Maximum number of object apipc can handle
#define APIPC_MAX_OBJ 10

/*
 * The following values extends the IPC driver command values passed between
 * processors in tIpcMessage.ulcommqnd register to determine what command is
 * requested by the sending processor. 
 * User should implement the corresponding function that serves the command on
 * the remote CPU side to interpret and use the message.
 */
#define APIPC_MESSAGE 0x0001000C

enum apipc_sm
{
    APIPC_SM_UNKNOWN = 0,
    APIPC_SM_INIT,
    APIPC_SM_IDLE,
    APIPC_SM_STARTUP_REMOTE_CONFIG,
    APIPC_SM_READING,
    APIPC_SM_WRITING,
    APIPC_SM_WAITTING_RESPONSE,
    APIPC_SM_STARTED
};

enum apipc_startup_sm
{
    APIPC_SU_SM_UNKNOWN = 0,
    APIPC_SU_SM_INIT,
    APIPC_SU_SM_STARTING,
    APIPC_SU_SM_FINISHED,
};

enum apipc_obj_sm
{
    APIPC_OBJ_SM_UNKNOWN = 0,
    APIPC_OBJ_SM_INIT,
    APIPC_OBJ_SM_IDLE,
    APIPC_OBJ_SM_WRITING,
    APIPC_OBJ_SM_WAITTING_RESPONSE,
    APIPC_OBJ_SM_RETRY,
    APIPC_OBJ_SM_STARTED,
};

enum apipc_msg_cmd
{
    APIPC_MSG_CMD_FUNC_CALL_RSP             = 0x00000012,

    APIPC_MSG_CMD_SET_BITS_RSP              = 0x00010001,
    APIPC_MSG_CMD_CLEAR_BITS_RSP            = 0x00010002,
    APIPC_MSG_CMD_DATA_WRITE_RSP            = 0x00010003,
    APIPC_MSG_CMD_BLOCK_READ_RSP            = 0x00010004,
    APIPC_MSG_CMD_BLOCK_WRITE_RSP           = 0x00010005,
    APIPC_MSG_CMD_DATA_READ_PROTECTED_RSP   = 0x00010007,
    APIPC_MSG_CMD_SET_BITS_PROTECTED_RSP    = 0x00010008,
    APIPC_MSG_CMD_CLEAR_BITS_PROTECTED_RSP  = 0x00010009,
    APIPC_MSG_CMD_DATA_WRITE_PROTECTED_RSP  = 0x0001000A,
    APIPC_MSG_CMD_BLOCK_WRITE_PROTECTED_RSP = 0x0001000B,
};

enum apipc_rc
{
    APIPC_RC_FAIL = -1,
    APIPC_RC_SUCCESS = 0
};

enum apipc_flags
{
    APIPC_FLAG_API_INITED = IPC_FLAG4, /* Local API implementation is inited */
    APIPC_FLAG_SRAM_ACCES = IPC_FLAG5, /* Local (CPU1) granted GSMEM acces to CPU2 */


    IPC_FLAG_L_R_ADDR = IPC_FLAG19,
    IPC_FLAG_BLOCK_RECEIVED = IPC_FLAG21,
};

enum apipc_obj_type
{ 
    APIPC_OBJ_TYPE_ND      = 0,
    APIPC_OBJ_TYPE_BLOCK   = 1,
    APIPC_OBJ_TYPE_DATA    = 2,
    APIPC_OBJ_TYPE_FLAGS   = 3,
    APIPC_OBJ_TYPE_FUNC_CALL = 4,
};

struct apipc_obj_flag
{
    uint16_t startup:1;
    uint16_t error:1;
    uint16_t spare:14;
};

struct apipc_obj
{
    uint16_t idx;
    enum apipc_obj_type type;
    enum apipc_obj_sm obj_sm;
    void *paddr;
    uint32_t payload; // for func_call use to transfer de argument
    size_t len;
    uint16_t *pGSxM;
    uint64_t timer;
    uint16_t retry;
    struct apipc_obj_flag flag;
};

#endif
//
// End of file.
//
