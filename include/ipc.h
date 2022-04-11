/**
 * 
 * \file ipc.h
 *
 * \brief apipc declarations.
 *
 * \author Federico David Ceccarelli
 *
 * apipc is an ipc_driver library implementation for TMS320C28x Texas
 * Instruments cores. The api present a bunch of routines to simplify
 * data tranfer between cores.
 */

#ifndef __IPC_H__
#define __IPC_H__

#include "F2837xD_Ipc_drivers.h"

#include "ipc_defs.h"
#include "ipc_utils.h"

#include <stddef.h>
#include <stdint.h>


//
// IPC API Drivers handlers extern declaration.
// One handler should be declared for every interrupt
//
extern volatile tIpcController g_sIpcController1;
extern volatile tIpcController g_sIpcController2;

/**
 * \brief Initialize apipc IPC API
 *
 * apipc_init should be invoked on both cores during start up.
 *
 * To the correct functioning of the api, apipc_init sets GSxM memory blocks
 * privileges and configure dependencies initializing mymalloc handler to
 * allocate cl_r_w_data data dynamically and circular_buffer handler to manage
 * tIpcMessage array dynamically. Also initialize objs array to a known state. 
 *
 * Funtion take care not only to initialize apipc but also ipc driver by
 * calling InitIpc() and initializing ipc driver controllers on IPC_INT0 &
 * IPC_INT1 respectively.
 *
 * \note apipc_init will acknowledge the api local start and will wait until it
 * were also initiated on the Remote CPU blocking the process meanwhile. Fuction
 * is blocking. 
 */
void apipc_init(void);

/**
 * @brief Register an apipc IPC API object
 *
 * \param[in] obj_idx object index number 
 * \param[in] obj_type apipc obj type
 * \param[in] paddr pointer to data 
 * \param[in] size data size in bytes
 * \param[in] startup start up flag, set as 1 to transmit obj on apipc app start
 * up.
 *
 * \return apipc_rc APIPC_RC_SUCCESS if registration process success and
 * APIPC_RC_FAIL if object couldn be registered.
 *
 * Register data as an apipc obj to be able to be tranfer between cores. Every
 * piece of data that user would like to transmit between cores must be
 * pre-registered as an object to indicate apipc how manage it.
 *
 * Registration will fail if paddr == NULL. Multiple objects registration over
 * the same obj_idx will cause overwriting. 
 *
 */
enum apipc_rc apipc_register_obj(uint16_t obj_idx, enum apipc_obj_type obj_type,
                                 void *paddr, size_t size, uint16_t startup);

/**
 * @brief peep actual obj_sm state of obj_idx object 
 *
 * \param[in] obj_idx object index number 
 *
 * \return apipc_obj_sm object state machine actual state
 *
 * Every object implements and process its own state machine. Each object
 * process is independent respect to the others. Knowing the actual obj_sm state
 * could be usefull to wait the remote reception of the data or to detect
 * faulty transmition by pooling. Those funtions depends on the application and
 * arent implemented on apipc. 
 *
 * \note See apipc_obj_sm for states definitios. 
 */
enum apipc_obj_sm apipc_obj_state(uint16_t obj_idx);

/**
 * @brief start object transmition on demand.
 *
 * \param[in] obj_idx object index number 
 *
 * Function works evolving the obj state machine to the init transmition state
 * only if the object is ready to be transmited between cores. 
 *
 * \return apipc_rc APIPC_RC_SUCCESS if obj transmition process could be
 * successfully united. APIPC_RC_FAIL if object send process couldn be started.
 *
 * \note Object should have already been inited to APIPC_OBJ_SM_IDLE 

 */
enum apipc_rc apipc_send(uint16_t obj_idx);

/**
 * @brief Sets the designated bits at the remote obj.
 *
 * \param[in] obj_idx object index number 
 * \param[in] bmask especifies bits to be set.
 *
 * \return apipc_rc 
 *
 * \note function bypass apipc normal functioning and obj sm interacting
 * directly with ipc diver. Use is not recomended!
 *
 * \note hint! could be used to announce a flagiged event immediately .
 */
enum apipc_rc apipc_flags_set_bits(uint16_t obj_idx, uint32_t bmask);

/**
 * @brief Clear the designated bits at the remote obj.
 *
 * \param[in] obj_idx object index number 
 * \param[in] bmask especifies bits to be clear.
 *
 * \return apipc_rc 
 *
 * \note function bypass apipc normal functioning and obj sm interacting
 * directly with ipc diver. Use is not recomended!
 *
 * \note hint! could be used to announce a flagiged event immediately .
 */
enum apipc_rc apipc_flags_clear_bits(uint16_t obj_idx, uint32_t bmask);


/**
 * @brief Initialize local object data on the remote core 
 *
 * To be transmited, variables and blocks not only need to be registered as
 * objects but also their SM needs to be initialized. If startup flag is marked,
 * local core will transmit his value to remote core over obj sm startup. Thats
 * the apipc_startup_remote functionality.
 * Obj initialization is one of the task apipc_app will process when function
 * start to be recursively called. But may be you want to transmit obj initial
 * values over core initialization before main process and apipc_app starts
 * being called. 
 *
 * \note remote core should be able to process apipc messages.
 * 
 */
enum apipc_rc apipc_startup_remote(void);

/**
 * @brief apipc application
 *
 * apipc application should be recursively called in the main while to transmit
 * and receive previously declared objects.
 */
void apipc_app(void);

/* IPC interrupt Handlers Functions declarations */
interrupt void apipc_ipc0_isr_handler(void); /**< IPC0 interrupt Handler */
interrupt void apipc_ipc1_isr_handler(void); /**< IPC1 interrupt Handler */

#endif

//
// End of file.
//
