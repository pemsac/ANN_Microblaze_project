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

/* errors.h
 * Error codes in flashwriter
 */

#ifndef FS_ERRORS_H
#define FS_ERRORS_H

#define ERR_NOTSUPPORTED        1               /* An unsupported operation was requested                       */
#define ERR_CFIQRY_LAYOUT       2               /* Could not identify layout of flash parts correctly           */
#define ERR_CFIQRY_PRI          3               /* Did not scan 'P' 'R' 'I' during CFI query                    */
#define ERR_CFIQRY_BLKINFO      4               /* CFI QRY of block info returned inconsistent results          */
#define ERR_CFIQRY_MAX_REGIONS  5               /* CFI QRY ran out of space to accomodate region information    */
#define ERR_FLASH_TIMEOUT       6               /* The flash device timed out during an operation               */
#define ERR_FLASH_SEQ           7               /* The flash sequence provided was incorrect                    */
#define ERR_FLASH_ERASE         8               /* The flash erase operation errored out                        */
#define ERR_FLASH_BLK_ERASE     9               /* The flash block erase operation errored out                  */
#define ERR_FLASH_PROG          10              /* The flash programming operation errored out                  */
#define ERR_FLASH_LOCK          11              /* The flash operation ran into a lock error                    */
#define ERR_FLASH_VOLTAGE       12              /* The flash part ran into a voltage error                      */


#endif /* FS_ERRORS_H */
