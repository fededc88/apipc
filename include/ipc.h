
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


void IpcDa_Init_r_config(void);


/**
 * @fn IpcDa_app
 *
 * @brief Initialize IPC Driver Application
 */
void IpcDa_app(void);

/**
 * @fn GSxM_register_l_r_w_addr()
 *
 * @brief 
 *
 * @param [in]
 * @param [in]
 */
void GSxM_register_l_r_w_addr(enum apipc_addr_ind ind, uint32_t paddr,
	                      size_t size);

//
// IPC interrupt Handlers Functions declarations
//
void RtoLIPC0IntHandler(void);
void RtoLIPC1IntHandler(void);

#endif

//
// End of file.
//
