
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
 * @fn ipc_init
 *
 * @brief Initialize IPC API
 */
void apipc_init(void);

/**
 * @fn apipc_register_obj
 *
 * @brief Register an IPC API object
 */
enum apipc_rc apipc_register_obj(uint16_t obj_idx, enum apipc_obj_type obj_type,
        void *paddr, size_t size);

/**
 * @fn apipc_send
 *
 * @brief Start a comunication over IPC API 
 */
enum apipc_rc apipc_send(uint16_t obj_idx);

/**
 * @fn apipc_init_config
 *
 * @brief Start a comunication over IPC API 
 */
void apipc_init_config(void);

/**
 * @fn apipc_app
 *
 * @brief apipc application
 */
void apipc_app(void);

//
// IPC interrupt Handlers Functions declarations
//
void apipc_ipc0_isr_handler(void);
void apipc_ipc1_isr_handler(void);

#endif

//
// End of file.
//
