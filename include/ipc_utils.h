
#ifndef __IPC_UTILS_H__
#define __IPC_UTILS_H__

#include "F2837xD_Ipc_Drivers.h"

#include <stddef.h>
#include <stdint.h>

#if CPU_FRQ_200MHZ
#define IPC_TIMER_WAIT_1mS 200000ll
#define IPC_TIMER_WAIT_2mS 400000ll
#define IPC_TIMER_WAIT_5mS 1000000ll
#define IPC_TIMER_WAIT_10mS 2000000ll
#define IPC_TIMER_WAIT_20mS 4000000ll
#define IPC_TIMER_WAIT_50mS 50000000ll
#define IPC_TIMER_WAIT_100mS 20000000ll
#define IPC_TIMER_WAIT_200mS 40000000ll
#define IPC_TIMER_WAIT_500mS 100000000ll
#define IPC_TIMER_WAIT_1S 200000000ll
#define IPC_TIMER_WAIT_2S 400000000ll
#define IPC_TIMER_WAIT_5S 1000000000ll
#define IPC_TIMER_WAIT_10S 2000000000ll
#endif

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

/**
 * @fn ipc_read_timer
 *
 * @brief Read the current IPC timer value.
 *
 * @return IPCCOUNTER H/L value
 *
 * A 64-bit free-running counter is present in the device and can be used to
 * timestamp IPC events between processors or timerize process.
 * This function will read the current ipc IPCCOUNTERH/L register and return his
 * value. 
 */
uint64_t ipc_read_timer(void);

/**
 * @fn ipc_timer_expired
 *
 * @brief Use ipc IPCCOUNTER register to function as timer for ipc applications.
 *
 * @param [in] start IPCCOUNTERH/L value at the time the timer is started. Value
 *                   should be preserved until timer expires.
 * @param [in] wait time timer will wait until expires in counts.
 *
 * @return 1 if timer have expired, 0 if not. 
 *
 * A 64-bit free-running counter is present in the device and can be used to
 * timestamp IPC events between processors or timerize process.
 * This function will read the current ipc IPCCOUNTERH/L register value and
 * compares it to start time to check if wait time have been completed.
 */
uint16_t ipc_timer_expired(uint64_t start, uint64_t wait);

#endif

//
// End of file.
//
