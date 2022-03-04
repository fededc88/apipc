/**
 * 
 * \file ipc_defs.h
 *
 * \brief apipc definitions.
 *
 * \author Federico David Ceccarelli
 *
 * apipc is an ipc_driver library implementation for TMS320C28x Texas
 * Instruments cores. The api present a bunch of routines to simplify
 * comunication between cores.
 * 
 * The file includes all the library definitions. Some of them are only intended
 * to explicitly show the memory spaces used.
 *
 */

#ifndef __IPC_DEFS_H__
#define __IPC_DEFS_H__

/**
 * \brief ram_func attribute definition
 *
 * The ramfunc attribute specifies that a function will be placed in and
 * executed from RAM. The ramfunc attribute allows the compiler to optimize
 * functions for RAM execution.
 *
 * Use example:
 * \code{.c}
 *		__attribute__((ramfunc)) void f(void) {
 *		...
 *		}
 * \endcode
 *
 *	User should define rumfunc location on the corresponding .cmd command linker
 *	file according to project needs.
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

/**
 * \defgroup gsxm apipc GSxM memory block use description
 *
 * \brief apipc GSxM memory block use description
 * 
 * RAM blocks which are accessible from both the CPU and DMA are called global
 * shared RAMs (GSx RAMs).
 * Each shared RAM block can be owned by either CPU subsystem based on the
 * configuration of respective bits in the GSxMSEL register.
 *
 * Each cpu reserves three GSxM blocks to transfer data and allocate local
 * apipc objects.
 *
 * <b>CPU0n_TO_CPU0n_R_W_DATA</b> memory blocks are used to copy data when
 * variables are transfered as blocks. This memory space is dynamically
 * allocated with mymalloc library. \see mymalloc.
 *
 * <b>CPU0n_TO_CPU0n_R_W_ADDR</b> memory blocks are used to allocate apipc objs
 * to be accesible from both cores.
 *
 * --
 *
 * \subsection apipc GSxM memory space use description:
 *
 *           CPU1                 |                 CPU2
 *                             - - - - -
 *                               GSR0
 *                                ~
 *                             - - - - -
 *                               GSR2      
 *                                      CPU02_TO_CPU01_R_W_DATA
 *                               GSR3
 *                             - - - - -
 *                               GSR4
 *     CPU01_TO_CPU02_R_W_DATA
 *                               GSR5
 *
 *     CPU01_TO_CPU02_ADDR       GSR6
 *                            - - - - -
 *                               GSR7    CPU01_TO_CPU02_ADDR
 *                            - - - - -
 *                                ~
 *                               ...
 *                            - - - - -
 *
 * @{ */

/** gsxm blocks reserved for ipc driver app for cpu1 - cpu1 has W/R priviledges */
#define APIPC_CPU01_TO_CPU02_GSxRAM GS4_ACCESS|GS5_ACCESS|GS6_ACCESS
/** gsxm blocks reserved for ipc driver app for cpu2 - cpu2 has W/R priviledges */
#define APIPC_CPU02_TO_CPU01_GSxRAM GS2_ACCESS|GS3_ACCESS|GS7_ACCESS

/** GS4SARAM Start Address */
#define CPU01_TO_CPU02_R_W_DATA_START (uint32_t)0x00010000  

/** CPU01 to CPU02 Local Addresses MSG RAM off sets */
#define CPU01_TO_CPU02_R_W_ADDR (uint32_t)0x00012000    

/** GS2SARAM Start Address */
#define CPU02_TO_CPU01_R_W_DATA_START (uint32_t)0x0000E000 

/** CPU02 to CPU01 Local Addresses MSG RAM offsets */
#define CPU02_TO_CPU01_R_W_ADDR (uint32_t)0x00013000  

/**@}*/

/** CPU0n_TO_CPU0n_R_W_DATA space length*/
#define CL_R_W_DATA_LENGTH 4096

/** Maximum number of object apipc allocates and can handle */
#define APIPC_MAX_OBJ 10

/**
 * The following values extends the IPC driver command values passed between
 * processors in tIpcMessage.ulcommqnd register to determine what command is
 * requested to be processed by the sending core. 
 *
 * User should implement the corresponding function that serves the command on
 * the remote CPU side to interpret and use the message.
 */
#define APIPC_MESSAGE 0x0001000C 

/**
 * \brief apipc app state machine's states definition
 */
enum apipc_sm
{
    APIPC_SM_UNKNOWN = 0, /**< initial state. apipc app sm state is unknown */
    APIPC_SM_STARTUP_REMOTE, /**< apipc app is transmiting startup flaged
                               objects to remote */
    APIPC_SM_IDLE, /**< apipc app is idle, doing nothing */
    APIPC_SM_STARTED /**< apipc app is ready to process objects to transmit */
};

/**
 * \brief apipc obj state machine's states definition
 *
 * For every obj, declared or not, his own sm will be processed 
 */
enum apipc_obj_sm
{
    APIPC_OBJ_SM_UNKNOWN = 0, /**< initial state. obj sm state is unknown */
    APIPC_OBJ_SM_FREE, /**< obj is free. Wasn't registered. Do nothing */
    APIPC_OBJ_SM_INIT, /**< obj sm prepares to transmit */
    APIPC_OBJ_SM_WRITING, /**< obj sm is filling memory and writing ipc driver */
    APIPC_OBJ_SM_WAITTING_RESPONSE, /**< obj is waiting remote response */
    APIPC_OBJ_SM_RETRY, /**< obj transmition failed, retry */
    APIPC_OBJ_SM_IDLE, /**< obj is started and idle, ready to transmit */
    APIPC_OBJ_SM_FAIL, /**< obj is in fail state, catastrofic fail happened */
};

/**
 * \brief apipc messages commands definitions
 *
 *  The following are values that are used by apipc and matchs commands values
 *  that are passed between processors in tIpcMessage.ulmessage or in the
 *  xTOyIPCCOM register and that determine what command is requested by the
 *  sending processor. 
 *
 *  apipc uses them to identify and proccess responces from remote core
 */
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

/**
 * \brief apipc funtions return codes definition
 */
enum apipc_rc
{
    APIPC_RC_FAIL = -1, /**<  FAIL! */
    APIPC_RC_SUCCESS = 0 /**< SUCCESS! */
};

/**
 * \brief apipc private ipc flags definition
 *
 * There are 32 IPC event signals in each direction between the CPU pairs. apipc
 * reserves a few of those ipc flags for his private use.
 *
 * \note User should prevent occupying or using ipc flags here defined. In case
 * this turns inevitable apipc flags could be re-defined and re asigned with the
 * precaussion of the case.
 * 
 */
enum apipc_flags
{
    APIPC_FLAG_IRQ_IPC0 = IPC_FLAG0, /**< g_sIpcController1 interrupt flag */
    APIPC_FLAG_IRQ_IPC1 = IPC_FLAG1, /**< g_sIpcController2 interrupt flag */

    APIPC_FLAG_API_INITED = IPC_FLAG4, /**< Local apipc implementation inited */
    APIPC_FLAG_SRAM_ACCES = IPC_FLAG5, /**< Local (CPU1) granted GSMEM acces to
                                         remote core (CPU2) */
    APIPC_FLAG_APP_START = IPC_FLAG6,  /**< apipc_app has started! */
};

/**
 * \brief apipc obj type definition
 */
enum apipc_obj_type
{ 
    APIPC_OBJ_TYPE_ND      = 0, /**< obj type is undefined */
    APIPC_OBJ_TYPE_BLOCK   = 1, /**< obj will be treated as a block */
    APIPC_OBJ_TYPE_DATA    = 2, /**< obj will be treated as an unique value */ 
    APIPC_OBJ_TYPE_FLAGS   = 3, /**< obj will be treated as flags */
    APIPC_OBJ_TYPE_FUNC_CALL = 4, /** obj will be treated as a funcion */
};

/**
 * \brief apipc obj flags definition
 */
struct apipc_obj_flag
{
    uint16_t startup:1; /**< transmit obj on apipc app start up */
    uint16_t error:1; /**< obj transmition failed retry times */
    uint16_t spare:14; /** not defined - available */
};

/**
 * \brief apipc object obj definition
 */
struct apipc_obj
{
    uint16_t idx; /**< obj index position on l_apipc_obj array */
    enum apipc_obj_type type; /** obj type */
    enum apipc_obj_sm obj_sm; /** actual obj sm state */
    void *paddr; /**< pointer to the obj's local address */
    uint32_t payload; /**< spare data. TODO: implement */
    size_t len; /**< obj length in bytes */
    uint16_t *pGSxM; /**< pointer to the dynamycally allocated memory space on
                       cl_r_w_data */
    uint64_t timer; /**< start timer value */
    uint16_t retry; /**< retrys counts */
    struct apipc_obj_flag flag; /**< obj flags */
};

#endif
//
// End of file.
//
