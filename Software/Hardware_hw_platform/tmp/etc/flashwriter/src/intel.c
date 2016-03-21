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

/*  intel.c -- Intel standard command set for programming flash parts
 */

#include "portab.h"
#include "errors.h"
#include "flash.h"
#include "defs.h"
#include <stdio.h>

#if defined (FW_CFG_DEV_OPERATE) && defined (FW_SUPPORT_INTEL)

/*==============================================================================*/
/* Macros and defines                                                           */
/*==============================================================================*/
#define INTEL_BLOCK_ERASE        0x20
#define INTEL_FULL_ERASE         0x30
#define INTEL_PROGRAM_WORD       0x40
#define INTEL_CLEAR_STATUS       0x50
#define INTEL_CHANGE_LOCK        0x60
#define INTEL_READ_STATUS        0x70
#define INTEL_READ_IDCODES       0x90
#define INTEL_READ_QUERY         0x98
#define INTEL_READ_ARRAY         0xff

#define INTEL_SET_LOCK_BIT       0x01
#define INTEL_CLEAR_LOCK_BITS    0xd0
#define INTEL_CONFIRM            0xd0
#define INTEL_RESUME             0xd0
#define INTEL_SUSPEND            0xb0

#define INTEL_READY              0x80
#define INTEL_ERASE_SUSPEND      0x40
#define INTEL_ERASE_ERRORS       0x20
#define INTEL_PROGRAM_ERRORS     0x10
#define INTEL_VOLTAGE_ERRORS     0x08
#define INTEL_PROGRAM_SUSPEND    0x04
#define INTEL_LOCKBIT_ERRORS     0x02

/* Various status conditions */
#define INTEL_STATUS_READY       0
#define INTEL_STATUS_CMDSEQ_ERR  ERR_FLASH_SEQ 
#define INTEL_STATUS_ERASE_ERR   ERR_FLASH_ERASE
#define INTEL_STATUS_PROG_ERR    ERR_FLASH_PROG 
#define INTEL_STATUS_VOLTAGE_ERR ERR_FLASH_VOLTAGE
#define INTEL_STATUS_LOCK_ERR    ERR_FLASH_LOCK 

#define INTEL_OPTIONAL_FEATURES_LEGACY_UNLOCK_MASK 0x8

#define BLKADDR_TO_ADDR(blkaddr)    (blkaddr << devinf->addr_shift)
#define ADDR_TO_BLKADDR(addr)       (addr >> devinf->addr_shift)

/*==============================================================================*/
/* Declarations                                                                 */
/*==============================================================================*/
static void     intel_init_dev_params (dev_info_t *devinfo);
static void     intel_rst_dev ();
static void     intel_rst_blk (uint32_t blkaddr);
static int8_t   intel_erase_dev ();
static int8_t   intel_blk_erase_dev (uint32_t blkaddr);
static int8_t   intel_prog_dev (uint32_t dstoffset, uint8_t *srcaddr, uint32_t nbytes);

static uint8_t  intel_lock_block (uint32_t blkaddr);
static uint8_t  intel_unlock_block (uint32_t blkaddr);
static uint8_t  intel_status_check (uint32_t blkaddr);
static uint32_t intel_read_status (uint32_t blkaddr);
static void     intel_clear_status ();
static void     intel_status_delay ();

extern void     fl_write (uint32_t addr, uint32_t data);
extern uint32_t fl_read  (uint32_t addr);
extern uint32_t fl_form_cmd (uint8_t);
extern void     fl_cmdwrite  (uint32_t cmdaddr, uint8_t cmddata);

/*==============================================================================*/
/* Data structures                                                              */
/*==============================================================================*/
target_ops_t Intel_ops = {                           
    intel_init_dev_params,
    intel_rst_dev,
    intel_rst_blk,
#ifdef FW_CFG_ERASE
    intel_erase_dev,
    intel_blk_erase_dev,
#else
    NULL,
    NULL,
#endif
#ifdef FW_CFG_PROG
    intel_prog_dev
#else
    NULL
#endif
};

static uint32_t  sr7flag, sr6flag, sr5flag, sr4flag, sr3flag, sr2flag, sr1flag;
static dev_info_t *devinf;
static uint8_t addr_step;
static uint8_t addr_shift;


/*==============================================================================*/
/* Function definitions                                                         */
/*==============================================================================*/
static void intel_init_dev_params (dev_info_t *devinfo)
{
    addr_step = devinfo->addr_step;
    addr_shift = devinfo->addr_shift;
    sr7flag = fl_form_cmd (INTEL_READY);                                                /* Setup various status flags according to interleave */
    sr6flag = fl_form_cmd (INTEL_ERASE_SUSPEND);
    sr5flag = fl_form_cmd (INTEL_ERASE_ERRORS);
    sr4flag = fl_form_cmd (INTEL_PROGRAM_ERRORS);
    sr3flag = fl_form_cmd (INTEL_VOLTAGE_ERRORS);
    sr2flag = fl_form_cmd (INTEL_PROGRAM_SUSPEND);
    sr1flag = fl_form_cmd (INTEL_LOCKBIT_ERRORS);
    devinf = devinfo;
}

static void intel_rst_dev ()
{
    int i, j;
    uint32_t offset, blkaddr;
    
    for (i = 0; i < devinf->flgeo.tot_regions; i ++) {
        offset = devinf->flgeo.region[i].offset;
        for (j = 0; j < devinf->flgeo.region[i].nblks; j++) {
            blkaddr = offset >> (BYTE_OFFSET_FACTOR (devinf->part_mode));               /* We calculate 'block address here. */
            intel_rst_blk (blkaddr);
            offset += devinf->flgeo.region[i].blksiz;
        }
    }
}

static void intel_rst_blk (uint32_t blkaddr)
{
    intel_clear_status ();                                                              /* Clear status and do read reset */
    fl_cmdwrite (blkaddr, INTEL_READ_ARRAY);
    intel_status_delay ();
}

#ifdef FW_CFG_ERASE
/*
 *      Intel does not support full erase. Erase block by block.
 */
static int8_t intel_erase_dev ()
{
    int i, j;
    uint8_t status;
    uint32_t offset, blkaddr;

    for (i = 0; i < devinf->flgeo.tot_regions; i ++) {
        offset = devinf->flgeo.region[i].offset;
        for (j = 0; j < devinf->flgeo.region[i].nblks; j++) {
            blkaddr = offset >> (BYTE_OFFSET_FACTOR (devinf->part_mode));               /* We calculate 'block address here. */
            if ((status = intel_blk_erase_dev (blkaddr)))
                return status;
            offset += devinf->flgeo.region[i].blksiz;
        }
    }

    return 0;
}

/*
 *      Given an absolute block address, erase that block
 */
static int8_t intel_blk_erase_dev (uint32_t blkaddr)
{
    uint32_t data;
    uint8_t  status;

#ifndef NO_INTEL_UNLOCK_BLOCKS
    status = intel_unlock_block (blkaddr);
    if (status != INTEL_STATUS_READY)
        return status;
#endif

    fl_cmdwrite (blkaddr, INTEL_BLOCK_ERASE);
    fl_cmdwrite (blkaddr, INTEL_CONFIRM);
    intel_status_delay ();

    data = fl_read (BLKADDR_TO_ADDR (blkaddr));
    while ((data & sr7flag) != sr7flag) 
        data = fl_read (BLKADDR_TO_ADDR (blkaddr));
    
    status = 0;
    status = intel_status_check (blkaddr);
    intel_rst_blk (blkaddr);
    return status;
}
#endif  /* FW_CFG_ERASE */

#ifdef FW_CFG_PROG
static int8_t intel_prog_dev (uint32_t offset, uint8_t *srcaddr, uint32_t nbytes)
{
    uint32_t data, fldata;
    uint32_t *srcp32, *srcp16, *srcp8;
    int8_t err = 0;
    uint8_t status;

    intel_rst_blk (ADDR_TO_BLKADDR (offset));

    if ((nbytes % addr_step))
        nbytes += (addr_step - (nbytes % addr_step));                   /* Align to address boundary */

    while (nbytes) {
        data = mem_read (srcaddr);                                      /* Read FLASH_BUS_WIDTH amount of data from memory and store in data */
        fl_cmdwrite (offset >> addr_shift, INTEL_PROGRAM_WORD);         /* Adjust offset, since we are using cmdwrite which expects command addresses */
        fl_write (offset, data);
        intel_status_delay ();

        data = fl_read (offset);
        while ((data & sr7flag) != sr7flag) 
            data = fl_read (offset);

        if ((status = intel_status_check (ADDR_TO_BLKADDR (offset)))) {
            intel_rst_blk (ADDR_TO_BLKADDR (offset));
            return status;
        }

        offset += addr_step;                                            /* Update pointers */
        nbytes -= addr_step;
        srcaddr = (uint8_t*)(srcaddr + addr_step);     
    }

    return 0;
}
#endif  /* FW_CFG_PROG */

static uint32_t intel_read_status (uint32_t blkaddr)
{
    fl_cmdwrite (blkaddr, INTEL_READ_STATUS);
    return fl_read (BLKADDR_TO_ADDR (blkaddr));
}

static uint8_t intel_status_check (uint32_t blkaddr)
{
    uint32_t status;

    status = intel_read_status (blkaddr);
    if ((status & sr5flag) == sr5flag) {
        if ((status & sr4flag) == sr4flag) 
            return INTEL_STATUS_CMDSEQ_ERR;
        return INTEL_STATUS_ERASE_ERR;
    } else if ((status & sr4flag) == sr4flag) 
        return INTEL_STATUS_PROG_ERR;
    else if ((status & sr3flag) == sr3flag) 
        return INTEL_STATUS_PROG_ERR;
    else if ((status & sr2flag) == sr2flag) 
        return INTEL_STATUS_VOLTAGE_ERR;
    else if ((status & sr1flag) == sr1flag) 
        return INTEL_STATUS_LOCK_ERR;
    
    return INTEL_STATUS_READY;
}

static uint8_t intel_lock_block (uint32_t blkaddr)
{
    uint32_t status;

    fl_cmdwrite (blkaddr, INTEL_CHANGE_LOCK);
    fl_cmdwrite (blkaddr, INTEL_SET_LOCK_BIT);    

    intel_status_delay ();

    do {
        status = fl_read (BLKADDR_TO_ADDR (blkaddr));
    } while ((status & sr7flag) != sr7flag);

    intel_status_delay ();
    status = intel_status_check (blkaddr); 
    intel_rst_blk (blkaddr);

    return (uint8_t) status; 
}

static uint8_t intel_unlock_block (uint32_t blkaddr)
{
    uint32_t status;

    fl_cmdwrite (blkaddr, INTEL_CHANGE_LOCK);
    fl_cmdwrite (blkaddr, INTEL_CLEAR_LOCK_BITS);

    intel_status_delay ();

    do {
        status = fl_read (BLKADDR_TO_ADDR (blkaddr));
    } while ((status & sr7flag) != sr7flag);

    intel_status_delay ();
    status = intel_status_check (blkaddr); 
    intel_rst_blk (blkaddr);

    return (uint8_t) status; 
}

static void intel_clear_status ()
{
    fl_cmdwrite (0, INTEL_CLEAR_STATUS);
}

/* CR 233698

    Intel parts have a tWB time delay that must be waited for after a command write,
    before the status is valid. We don't have the luxury of a timer on board - we use
    this crude delay scheme.
 */
#ifdef __MICROBLAZE__
#define INTEL_STATUS_DELAY_LOOPS    5
#else
#define INTEL_STATUS_DELAY_LOOPS    15
#endif

static void intel_status_delay ()
{
    volatile int i;

    for (i = 0; i < INTEL_STATUS_DELAY_LOOPS; i++)
        ;
}

#endif  /* !(FW_CFG_DEV_OPERATE) && (FW_SUPPORT_INTEL) */
