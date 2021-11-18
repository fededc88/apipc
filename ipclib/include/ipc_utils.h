
#ifndef __IPC_UTILS_H__
#define __IPC_UTILS_H__

#include "F2837xD_Ipc_Drivers.h"

#include <stddef.h>
#include <stdint.h>

/**
 * @fn u16memcpy()
 *
 * @brief copies n uint16_t size memory blocks from s2 to s1  
 *
 * @param [in] s1 void pointer to a memory destination
 * @param [in] s2 void pointer fto a memory destination
 * @param [in] n number of uint16_t size elements to copy
 *
 * @return s1 value, NULL if fails
 *
 * Copies n uint16_t size elements from the object pointed to by s2 into the
 * object pointed to by s1. If copying takes place between objects that overlap,
 * the behavior is undefined. If pointed elemets are not multiple of uint16_t 
 * behavior is undefined. 
 * 
 */
 void *u16memcpy(void * __restrict s1, const void * __restrict s2, size_t n);

/**
 * @fn GSxM_acces()
 *
 * @brief Manage GSxM Ram memory access
 *
 * @param [in] ulMask specifies the 32-bit mask for the GSxMSEL RAM control
 *                  register to indicate which GSx SARAM blocks is requesting
 *                  master access to.
 * @param [in] usMaster specifies whether the CPU1 or CPU2 are given master 
 *                 access to the GSx blocks.
 *
 * This function will allow CPU1 configure master acces to R/W/Exe acces to
 * the GSx SARAM block specified by the ulMask parameter for the usMaster CPU
 * specified.
 * The usMaster parameter can be either: IPC_GSX_CPU2_MASTER or
 * IPC_GSX_CPU1_MASTER.
 * The ulMask parameter can be any of the options: S0_ACCESS - S7_ACCESS.
 */

void GSxM_Acces(uint32_t ulMask, uint16_t usMaster);
#endif

//
// End of file.
//
