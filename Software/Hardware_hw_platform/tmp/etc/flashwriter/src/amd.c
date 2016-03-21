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

/*  amd.c -- AMD/Fujitsu Standard command set for programming flash parts
 */

#include "portab.h"
#include "errors.h"
#include "flash.h"
#include "defs.h"
#include <stdio.h>

#if defined (FW_CFG_DEV_OPERATE) && defined (FW_SUPPORT_AMD)

/*==============================================================================*/
/* Macros and defines                                                           */
/*==============================================================================*/
#define ADDR_555                  0x555 
#define ADDR_2AA                  0x2aa  
#define ADDR_AAA                  0xaaa  
#define CMD_AA                    0xAA
#define CMD_A0                    0xA0
#define CMD_55                    0x55
#define CMD_READ_RESET            0xF0
#define CMD_ERASE                 0x80
#define CMD_AUTO_CHIP_ERASE       0x10
#define CMD_AUTO_BLOCK_ERASE      0x30
#define CMD_AUTO_PROGRAM          0xA0
#define CMD_FAST_PROGRAM_SET      0x20
#define CMD_FAST_PROGRAM_RESET    0x90
#define CMD_FAST_PROGRAM          0xA0

#define DQ7FLAG                   0x80
#define DQ5FLAG                   0x20

/* #define AMD_FAST_PROGRAM  */                 /* Disabled. Atmel has a different fast program sequence. Until then, we disable it to ensure correctness */


/*==============================================================================*/
/* Declarations                                                                 */
/*==============================================================================*/
static void     AMD_init_dev_params (dev_info_t *devinfo);
static void     AMD_rst_dev ();
static int8_t   AMD_erase_dev ();
static int8_t   AMD_blk_erase_dev (uint32_t blkaddr);
static int8_t   AMD_prog_dev (uint32_t dstoffset, uint8_t *srcaddr, uint32_t nbytes);
extern void     fl_write (uint32_t addr, uint32_t data);
extern uint32_t fl_read  (uint32_t addr);
extern uint32_t fl_form_cmd (uint8_t);
extern void     fl_cmdwrite  (uint32_t cmdaddr, uint8_t cmddata);

/*==============================================================================*/
/* Data structures                                                              */
/*==============================================================================*/
target_ops_t AMD_ops = {                           
    AMD_init_dev_params,
    AMD_rst_dev,
    NULL,
#ifdef FW_CFG_ERASE
    AMD_erase_dev,
    AMD_blk_erase_dev,
#else
    NULL,
    NULL,
#endif
#ifdef FW_CFG_PROG
    AMD_prog_dev
#else
    NULL
#endif
};

uint32_t dq7flag, dq5flag;
uint32_t unlockaddr_1, unlockaddr_2;
static uint8_t  addr_step;
static uint8_t  addr_shift;

/*==============================================================================*/
/* Function definitions                                                         */
/*==============================================================================*/
static void AMD_init_dev_params (dev_info_t *devinfo)
{
    dq7flag = fl_form_cmd (DQ7FLAG);
    dq5flag = fl_form_cmd (DQ5FLAG);
    addr_step = devinfo->addr_step;
    addr_shift = devinfo->addr_shift;
    if ((devinfo->part_mode) == PART_MODE_08) {
        unlockaddr_1 = ADDR_AAA;
        unlockaddr_2 = ADDR_555;
    } else {
        unlockaddr_1 = ADDR_555;
        unlockaddr_2 = ADDR_2AA;
    }
        
}


static void AMD_rst_dev ()
{
    fl_cmdwrite (unlockaddr_1, CMD_AA);
    fl_cmdwrite (unlockaddr_2, CMD_55);
    fl_cmdwrite (unlockaddr_1, CMD_READ_RESET);
}

#ifdef FW_CFG_ERASE
static int8_t AMD_erase_dev ()
{
    uint32_t data;

    fl_cmdwrite (unlockaddr_1, CMD_AA);    
    fl_cmdwrite (unlockaddr_2, CMD_55);    
    fl_cmdwrite (unlockaddr_1, CMD_ERASE); 
    fl_cmdwrite (unlockaddr_1, CMD_AA);    
    fl_cmdwrite (unlockaddr_2, CMD_55);    
    fl_cmdwrite (unlockaddr_1, CMD_AUTO_CHIP_ERASE);  

    data = fl_read (0);                                                 /* Check for errors and timeouts */
    while (((data & dq7flag) != dq7flag) && 
           ((data & dq5flag) != dq5flag)) 
        data = fl_read (0);

    data = fl_read (0);
    if ((data & dq7flag) == dq7flag)
        return 0;

    if ((data & dq5flag) == dq5flag) {
        data = fl_read (0);
        if ((data & dq7flag) != dq7flag)
            return ERR_FLASH_TIMEOUT;
    }
    
    AMD_rst_dev ();                                                     /* Reset device */
    return 0;
}

/*
 *      Given an absolute block address, erase that block
 */
static int8_t AMD_blk_erase_dev (uint32_t blkaddr)
{
    uint32_t data;

    fl_cmdwrite (unlockaddr_1, CMD_AA);
    fl_cmdwrite (unlockaddr_2, CMD_55);
    fl_cmdwrite (unlockaddr_1, CMD_ERASE);
    fl_cmdwrite (unlockaddr_1, CMD_AA);
    fl_cmdwrite (unlockaddr_2, CMD_55);
    fl_cmdwrite (blkaddr, CMD_AUTO_BLOCK_ERASE);

    blkaddr = (blkaddr << addr_shift);                                  /* Convert absolute block address, into logical address that can be fed to EMC */

    data = fl_read (blkaddr);                                           /* Check for errors and timeouts */
    while (((data & dq7flag) != dq7flag) && 
           ((data & dq5flag) != dq5flag)) 
        data = fl_read (blkaddr);

    data = fl_read (blkaddr);
    if ((data & dq7flag) == dq7flag)
        return 0;

    if ((data & dq5flag) == dq5flag) {
        data = fl_read (blkaddr);
        if ((data & dq7flag) != dq7flag)
            return ERR_FLASH_TIMEOUT;
    }

    AMD_rst_dev ();                                                     /* Reset device */
    return 0;
}
#endif  /* FW_CFG_ERASE */

#ifdef FW_CFG_PROG
static int8_t AMD_prog_dev (uint32_t offset, uint8_t *srcaddr, uint32_t nbytes)
{
    uint32_t data, fldata;
    uint32_t *srcp32, *srcp16, *srcp8;
    int8_t err = 0;

#ifdef AMD_FAST_PROGRAM                                                 /* Fast program setup */
    fl_cmdwrite (unlockaddr_1, CMD_AA);
    fl_cmdwrite (unlockaddr_2, CMD_55);
    fl_cmdwrite (unlockaddr_1, CMD_FAST_PROGRAM_SET);
#endif

    if ((nbytes % addr_step))
        nbytes += (addr_step - (nbytes % addr_step));                   /* Align to address boundary */

    while (nbytes) {
        data = mem_read (srcaddr);                                      /* Read FLASH_BUS_WIDTH amount of data from memory and store in data */
        
#ifdef AMD_FAST_PROGRAM
	fl_cmdwrite (0, CMD_FAST_PROGRAM);                              /* Fast program sequence - shorter */
	fl_write (offset, data);
#else 
        fl_cmdwrite (unlockaddr_1, CMD_AA);                             /* Normal programming sequence */
        fl_cmdwrite (unlockaddr_2, CMD_55);
        fl_cmdwrite (unlockaddr_1, CMD_A0);
        fl_write (offset, data);
#endif
        fldata = fl_read (offset);                                      /* Check for errors and timeouts */
        while (((fldata & dq7flag) != (data & dq7flag)) && 
               ((fldata & dq5flag) != dq5flag))
            fldata = fl_read (offset);
        
        fldata = fl_read (offset);
        if ((fldata & dq7flag) != (data &dq7flag)) {
            if ((fldata & dq5flag) == dq5flag) {
                fldata = fl_read (offset);
                if ((fldata & dq7flag) != (data & dq7flag))
                    return ERR_FLASH_TIMEOUT;
            }
        }

        offset += addr_step;                                            /* Update pointers */
        nbytes -= addr_step;
        srcaddr = (uint8_t*)(srcaddr + addr_step);
    }

#ifdef AMD_FAST_PROGRAM
    fl_cmdwrite (0, CMD_FAST_PROGRAM_RESET);                            /* Reset Fast-Program Mode */
    fl_cmdwrite (0, CMD_READ_RESET);            
#endif
    return err;
}
#endif  /* FW_CFG_PROG */
#endif  /* (FW_CFG_DEV_OPERATE) && (FW_SUPPORT_AMD) */
