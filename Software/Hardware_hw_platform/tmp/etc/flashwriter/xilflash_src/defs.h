////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2004 Xilinx, Inc.  All rights reserved. 
// 
// Xilinx, Inc. 
// XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A 
// COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS 
// ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR 
// STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION 
// IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE 
// FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION. 
// XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO 
// THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO 
// ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE 
// FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY 
// AND FITNESS FOR A PARTICULAR PURPOSE. 
////////////////////////////////////////////////////////////////////////////////////////////////////////

/* defs.h
 * Common defines for all of flashwriter files
 */

#ifndef _FS_DEFS_H
#define _FS_DEFS_H

#include "config.h"

/* Debug message macro */

/* #define DEBUG  */

#ifdef DEBUG
#define DPRINT(msg)             print (msg)
#define DPUTNUM(val)            putnum (val)
#define DPRINTF                 xil_printf
#else 
#define DPRINT(msg)                
#define DPUTNUM(val)
#define DPRINTF(...)
#endif


#if defined (FW_MODE_CFIQRY)

    #define FW_CFG_CFIQRY

#elif defined (FW_MODE_ERASE)

    #define FW_CFG_DEV_OPERATE
    #define FW_CFG_ERASE

    #if !defined (FW_SUPPORT_INTEL) && !defined (FW_SUPPORT_AMD)
        #define FW_SUPPORT_DETECT
        #define FW_SUPPORT_INTEL
        #define FW_SUPPORT_AMD
    #endif

#elif defined (FW_MODE_PROG)

    #define FW_CFG_DEV_OPERATE
    #define FW_CFG_PROG

    #if !defined (FW_SUPPORT_INTEL) && !defined (FW_SUPPORT_AMD)
        #define FW_SUPPORT_DETECT
        #define FW_SUPPORT_INTEL
        #define FW_SUPPORT_AMD
    #endif 

#else

    #define FW_SUPPORT_DETECT
    #define FW_SUPPORT_INTEL
    #define FW_SUPPORT_AMD

    #define FW_CFG_CFIQRY
    #define FW_CFG_DEV_OPERATE
    #define FW_CFG_ERASE
    #define FW_CFG_PROG

#endif  /* FW_MODE_CFIQRY */

#endif
