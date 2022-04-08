/**
 * 
 * \file ipc_utils.c
 *
 * \brief A bunch of util routines that helps on apipc implementattion.
 *
 * \author Federico David Ceccarelli
 *
 */

#include "F2837xD_device.h"
#include "F2837xD_Examples.h"

#include "ipc_utils.h"

/*
 * u16memcpy() - copies n uint16_t size memory blocks from s2 to s1  
 */
 void *u16memcpy(void *to, const void *from, size_t n)
{
    register uint16_t *rto = (uint16_t *) to;
    register uint16_t *rfrom = (uint16_t *) from;
    register unsigned int rn;
    register unsigned int nn = (unsigned int) n;

    /*
     *  Copy up to the first 64K. At the end compare the number of chars   moved with
     *  the size n. If they are equal return. Else continue to  copy the remaining
     *  chars.                                          
     */
    for (rn = 0; rn < nn; rn++) 
	*rto++ = *rfrom++;
    
    if (nn == n)
	return (to);

    /*
     *  Write the memcpy of size >64K using nested for loops to make use   
     *  of BANZ instrunction.                                              
     */
    {
	register unsigned int upper = (unsigned int) (n >> 16);
	register unsigned int tmp;
	for (tmp = 0; tmp < upper; tmp++)
	{
	    for (rn = 0; rn < 65535; rn++)
		*rto++ = *rfrom++;
	    *rto++ = *rfrom++;
	}
    }
    return (to);
}

/*
 * ipc_read_timer - Read the current IPC timer value. 
 */
uint64_t ipc_read_timer(void)
{
    uint32_t low, high;

    /*
     * The low register must be read first to latch a value in the high
     * register.
     */
    low = IpcRegs.IPCCOUNTERL;
    high = IpcRegs.IPCCOUNTERH;

    return ((uint64_t)high << 32) | (uint64_t)low;
}

/*
 * ipc_timer_expired - procces the current IPC timer. Return 1 if timer have
 *                     expired 
 */
uint16_t ipc_timer_expired(uint64_t start, uint64_t wait) 
{
    uint64_t actual;
    uint64_t delta;

    actual = ipc_read_timer();

    if( actual > start )
        delta = actual - start;
    else
        delta = actual + (~start);

    if( delta > wait ) 
        return 1;
    else 
        return 0;

}

#if defined(CPU1)

/*
 * GSxM_Acces() - Master CPU Configures master R/W/Exe Access to Shared SARAM
 */
void GSxM_Acces(uint32_t ulMask, uint16_t usMaster)
{

    if ( usMaster == IPC_GSX_CPU2_MASTER )
    {
	while ((MemCfgRegs.GSxMSEL.all & ulMask) != ulMask) 
	{
	    EALLOW;
	    MemCfgRegs.GSxMSEL.all |= ulMask;
	    EDIS;
	}
    }
    else if ( usMaster == IPC_GSX_CPU1_MASTER )
    {

	while ((MemCfgRegs.GSxMSEL.all & ulMask) != 0) 
	{
	    EALLOW;
	    MemCfgRegs.GSxMSEL.all &= ~(ulMask);
	    EDIS;
	}
    }
}

#endif
