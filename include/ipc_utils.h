/**
 * 
 * \file ipc_utils.h
 *
 * \brief A bunch of util routines that helps on apipc implementattion.
 *
 * \author Federico David Ceccarelli
 *
 */

#ifndef __IPC_UTILS_H__
#define __IPC_UTILS_H__

#include "F2837xD_Ipc_Drivers.h"

#include <stddef.h>
#include <stdint.h>

/** 
 * \brief ipc's free-running counter wait ticks definitions.
 *
 * A 64-bit free-running counter is present in the device and it could be used
 * to measure time between actions.
 * The IPC_TIMER_WAIT_xxxmS defines interval times in PLLSYSCLK clock ticks. 
 */
#if CPU_FRQ_200MHZ
#define IPC_TIMER_WAIT_1mS   200000ll     /**< 200000 PLLSYSCLK ticks == 1mS */
#define IPC_TIMER_WAIT_2mS   400000ll     /**< 400000 PLLSYSCLK ticks == 2mS */
#define IPC_TIMER_WAIT_5mS   1000000ll    /**< 1000000 PLLSYSCLK ticks == 5mS */
#define IPC_TIMER_WAIT_10mS  2000000ll    /**< 2000000 PLLSYSCLK ticks == 10mS */
#define IPC_TIMER_WAIT_20mS  4000000ll    /**< 4000000 PLLSYSCLK ticks == 20mS */
#define IPC_TIMER_WAIT_50mS  50000000ll   /**< 50000000 PLLSYSCLK ticks == 50mS */
#define IPC_TIMER_WAIT_100mS 20000000ll   /**< 20000000 PLLSYSCLK ticks == 100mS */
#define IPC_TIMER_WAIT_200mS 40000000ll   /**< 40000000 PLLSYSCLK ticks == 200mS */
#define IPC_TIMER_WAIT_500mS 100000000ll  /**< 100000000 PLLSYSCLK ticks == 500mS */
#define IPC_TIMER_WAIT_1S    200000000ll  /**< 200000000 PLLSYSCLK ticks == 1S */
#define IPC_TIMER_WAIT_2S    400000000ll  /**< 400000000 PLLSYSCLK ticks == 2mS */
#define IPC_TIMER_WAIT_5S    1000000000ll /**< 1000000000 PLLSYSCLK ticks == 5S */
#define IPC_TIMER_WAIT_10S   2000000000ll /**< 2000000000 PLLSYSCLK ticks == 10S */
#endif

/**
 * \brief Copies n uint16_t size memory blocks from s2 to s1  
 *
 * \param [in] s1 void pointer to a memory destination
 * \param [in] s2 void pointer to a memory destination
 * \param [in] n number of uint16_t size elements to copy
 *
 * \return s1 value, NULL if fails
 *
 * Copies n uint16_t size elements from the object pointed to by s2 into the
 * object pointed to by s1.
 *
 * \note If copying takes place between objects that overlaps, the behavior is
 * undefined.
 * \note If pointed elemets are not multiple of uint16_t behavior is undefined. 
 */
 void *u16memcpy(void * __restrict s1, const void * __restrict s2, size_t n);

/**
 * \brief Reads the current ipc free-running counter register value.
 *
 * \return actual IPCCOUNTER H/L value
 *
 * A 64-bit free-running counter is present in the device and can be used to
 * timestamp IPC events between processors or timerize process.
 * This function will read the current ipc IPCCOUNTERH/L register and return his
 * value. 
 */
uint64_t ipc_read_timer(void);

/**
 * \brief Use ipc free-running counter IPCCOUNTER register to function as a
 *        timer.
 *
 * \param [in] start IPCCOUNTERH/L value at the time the timer was started. Value
 *                   should be preserved until timer expires.
 * \param [in] wait time timer will wait until expires in counts.
 *
 * \return 1 if timer have expired, 0 if not. 
 *
 * This function will read the current ipc free-running counter IPCCOUNTERH/L
 * register value and compares it to the provided start value to check if wait
 * time have been completed.
 * If the difference between actual IPCCOUNTERH/L value and the start value is
 * greater than wait value funtion will consider that time has expired and
 * return 1. If not will return 0 and it will have to be called again to check
 * if timer expired.
 * IPCCOUNTERH/L overflow and turn around have been taken in acount.
 * 
 */
uint16_t ipc_timer_expired(uint64_t start, uint64_t wait);

#if defined(CPU1)
/**
 * \brief Manage GSxM Ram memory access
 *
 * \param [in] ulMask specifies the 32-bit mask for the GSxMSEL RAM control
 *                    register to indicate which GSx SARAM blocks is requesting
 *                    master access to.
 * \param [in] usMaster specifies whether the CPU1 or CPU2 are given master 
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

#endif

//
// End of file.
//
