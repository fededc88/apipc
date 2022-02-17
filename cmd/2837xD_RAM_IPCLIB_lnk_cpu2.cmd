/* Linker map for ipclib Shared Memory.
 * 
 * Developed by Federico D. Ceccarelli (fededc88@gmail.com). Any kind of
 * submissions are welcome.
*/


MEMORY
{
PAGE 0 : /* Program memory. This is a legacy description since the C28 has a unified memory model. */


PAGE 1 : /* Data memory. This is a legacy description since the C28 has a unified memory model. */

/*
   RAMGS1           : origin = 0x00D000, length = 0x001000
   RAMGS2           : origin = 0x00E000, length = 0x001000 
   RAMGS3           : origin = 0x00F000, length = 0x001000
   RAMGS4           : origin = 0x010000, length = 0x001000
   RAMGS5           : origin = 0x011000, length = 0x001000
   RAMGS6           : origin = 0x012000, length = 0x001000
   RAMGS7           : origin = 0x013000, length = 0x001000
*/

/*
   CPU2TOCPU1RAM    : origin = 0x03F800, length = 0x000400
   CPU1TOCPU2RAM    : origin = 0x03FC00, length = 0x000400
*/
}

SECTIONS
{

   .cpul_cpur_data            : > RAMGS2,       PAGE = 1

   GROUP               : > RAMGS6,       PAGE = 1
   {
         .base_cpur_cpul_addr            /* allocate base_ramgs6 to RAMSG6_BASE specific address */
         .cpur_cpul_addr               
   }
   GROUP               : > RAMGS7,       PAGE = 1
   {
         .base_cpul_cpur_addr            /* allocate base_ramgs7 to RAMSG7_BASE specific address */
         .cpul_cpur_addr              
   }

    /*
       The following section definitions are required when using the IPC API Drivers
       Take care to check that groups aren't already defined at the provided
       .cmd file. Uncomment if they arent.
     */
/*
    GROUP : > CPU1TOCPU2RAM, PAGE = 1
    {
        PUTBUFFER
        PUTWRITEIDX
        GETREADIDX
    }

    GROUP : > CPU2TOCPU1RAM, PAGE = 1
    {
        GETBUFFER :    TYPE = DSECT
        GETWRITEIDX :  TYPE = DSECT
        PUTREADIDX :   TYPE = DSECT
    }
*/
}

/*
 * End of file.
 */

