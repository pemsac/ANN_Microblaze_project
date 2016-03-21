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

#include "portab.h"
#include "flash.h"
#include "errors.h"
#include "defs.h"
#include <stdio.h>

/*==============================================================================*/
/* Macros and defines                                                           */
/*==============================================================================*/
#define CFI_QRY_CMD_ADDR        0x55
#define CFI_QRY_CMD             0x98
#define CFI_QRY_STR_ADDR        0x10

/* Device memory access macros */
#define FLASHP_32(offset)       ((uint32_t*)(devinfo.base_addr + offset))
#define FLASHP_16(offset)       ((uint16_t*)(devinfo.base_addr + offset))
#define FLASHP_08(offset)       ((uint8_t* )(devinfo.base_addr + offset))

#define MASK8                   0x000000ff
#define MASK16                  0x0000ffff
#define MASK32                  0xffffffff

#define ALIGN16                 0xfffffffe
#define ALIGN32                 0xfffffffc

/*==============================================================================*/
/* Declarations                                                                 */
/*==============================================================================*/

/* Interface to CFI and flashwriter */
int8_t  tg_init_dev ();
void    tg_rst_dev ();
int8_t  tg_erase_dev ();
int8_t  tg_blk_erase_dev (uint32_t addr, uint32_t nbytes);
int8_t  tg_prog_dev (uint32_t dstoffset, uint8_t *srcaddr, uint32_t nbytes);
int8_t  fl_cfg_rw_mode ();
int8_t  fl_cfg_cmd_interleave (uint32_t layout);

uint32_t fl_read_08 (uint32_t addr);
uint32_t fl_read_16 (uint32_t addr);
uint32_t fl_read_32 (uint32_t addr);
void     fl_write_08 (uint32_t addr, uint32_t data);
void     fl_write_16 (uint32_t addr, uint32_t data);
void     fl_write_32 (uint32_t addr, uint32_t data);
uint32_t form_cmd_16_2 (uint8_t cmd);
uint32_t form_cmd_32_2 (uint8_t cmd);
uint32_t form_cmd_32_4 (uint8_t cmd);
uint32_t (*form_cmd)   (uint8_t);
uint32_t (*fl_read_p)  (uint32_t);
void (*fl_write_p) (uint32_t, uint32_t);

dev_info_t devinfo __attribute__ ((section (".data")));            
extern target_ops_t AMD_ops, Intel_ops;

typedef struct cmd_set_s {
    uint16_t            vendid;
    char                sponsor[40];
} cmd_set_t;

/*==============================================================================*/
/* Data structures                                                              */
/*==============================================================================*/
#ifdef FW_CFG_DEV_OPERATE
static cmd_set_t cmdsets[7] = { {0, "No vendor" },
                                {1, "Intel/Sharp Extended" }, 
                                {2, "AMD/Fujitsu Standard" }, 
                                {3, "Intel Standard" }, 
                                {4, "AMD/Fujitsu Extended" },
                                {256, "Mitsubishi Standard" },
                                {257, "Mitsubishi Extended" },
                              };
target_ops_t *the_target;
#endif

/*==============================================================================*/
/* Function definitions                                                         */
/*==============================================================================*/

/*==============================================================================*/

/* 
 *  fl_cfg_rw -- Based on total device bus width, configure read and write generics 
 */
int8_t fl_cfg_rw_mode ()
{                                                                       
    if (devinfo.bus_width == 8) {                                       
        fl_read_p  = fl_read_08;
        fl_write_p = fl_write_08 ;
        devinfo.addr_step = 1;
    } else if (devinfo.bus_width == 16) {
        fl_read_p  = fl_read_16;
        fl_write_p = fl_write_16 ;
        devinfo.addr_step = 2;
    } else if (devinfo.bus_width == 32) {
        fl_read_p  = fl_read_32;
        fl_write_p = fl_write_32;
        devinfo.addr_step = 4;
    }

    return 0;
}

/*
 *  fl_cfg_cmd_interleave -- Based on total device bus width, configure command interleave 
 */
int8_t fl_cfg_cmd_interleave (uint32_t layout)
{
    if (layout == PART_LAYOUT_UNKNOWN)
        return ERR_CFIQRY_LAYOUT;
    else if (layout == PART_LAYOUT_16_08_08_2) {
        form_cmd = form_cmd_16_2;
    }
    else if (layout == PART_LAYOUT_32_16_16_2) {
        form_cmd = form_cmd_32_2;
    }
    else if (layout == PART_LAYOUT_32_08_08_4) {
        form_cmd = form_cmd_32_4;
    }
    else 
        form_cmd = NULL;
    
    return 0;
}

#ifdef FW_CFG_DEV_OPERATE
int8_t tg_init_dev ()
{
    int8_t  ret, i;
    char   *ptr;

#ifdef DEBUG
    /*
     *  Populate target operations structure, based on vendor ID in device info
     */
    for (i = 0; i < 7; i++) {
        if (cmdsets[i].vendid == devinfo.cfiqry.pri_vendor_id) {
            ptr = cmdsets[i].sponsor;
            break;
        }
    }

    if (i == 8)
        ptr = "UNKNOWN";

    DPRINTF ("CFI Info: \r\n");
    DPRINTF ("Device base addr: 0x%x\r\n", devinfo.base_addr);
    DPRINTF ("Device bus width: %d\r\n"  , devinfo.bus_width);
    DPRINTF ("Number of parts : %d\r\n"  , devinfo.num_parts);
    DPRINTF ("Part mode       : %d\r\n"  , devinfo.part_mode);
    DPRINTF ("Part mem size   : 0x%x\r\n", (1 << devinfo.cfiqry.partsiz));
    DPRINTF ("Command set     : %s\r\n"  , ptr);
#endif

#if defined (FW_SUPPORT_DETECT)
    if (devinfo.cfiqry.pri_vendor_id == 1 || devinfo.cfiqry.pri_vendor_id == 3) 
         the_target = &Intel_ops;
    else if (devinfo.cfiqry.pri_vendor_id == 2 || devinfo.cfiqry.pri_vendor_id == 4) 
        the_target = &AMD_ops;
#elif defined (FW_SUPPORT_INTEL)
    the_target = &Intel_ops;
#elif defined (FW_SUPPORT_AMD)
    the_target = &AMD_ops;
#endif

    if ((ret = fl_cfg_rw_mode ()))                                              /* Configure read and write generics based on bus width */
        return ret;

    if ((ret = fl_cfg_cmd_interleave (devinfo.layout)))                         /* Configure command interleave based on part layout */
        return ret;

    if (the_target && the_target->initdev) 
        (*the_target->initdev) (&devinfo);
    else 
        return ERR_NOTSUPPORTED; 

    return 0;
}

void tg_rst_dev ()
{
    if (the_target && the_target->rstdev)
        (*the_target->rstdev) ();
}

#ifdef FW_CFG_ERASE
int8_t tg_erase_dev ()
{
    if (the_target && the_target->erasedev)
        return (*the_target->erasedev) ();
    else
        return ERR_NOTSUPPORTED;
}

int8_t tg_blk_erase_dev (uint32_t addr, uint32_t nbytes)
{
    int32_t  i, j;
    int8_t   status, erasebegin;
    uint32_t nb, ba, off;
    uint32_t low, high, end;

    if (!the_target || !the_target->blkerasedev)
        return ERR_NOTSUPPORTED;

    DPRINTF ("BLKERASE: addr: 0x%x, devinfo.addr_shift: %d, byte_offset_factor: %d\r\n", 
              addr, devinfo.addr_shift, BYTE_OFFSET_FACTOR (devinfo.part_mode)); 

                                                                                /* Calculate low erase offset in terms of bytes within each part */
    nb   = nbytes / devinfo.num_parts;                                          /* Calculate num bytes that have to be erased within each part */
    low  = addr >> devinfo.addr_shift;                                          
    low  = low << BYTE_OFFSET_FACTOR (devinfo.part_mode);                       /* Convert raw address into bytes */

    high = low + nb;                                                            /* Calculate high erase offset in terms of bytes within each part */    
    off = 0;
    erasebegin = 0;

    DPRINTF ("BLKERASE: Start, size = (0x%x, 0x%x). Calculated low and high = (0x%x, 0x%x)\r\n", addr, nbytes, low, high); 
    for (i = 0; i < devinfo.flgeo.tot_regions; i++) {                           /* Locate and erase appropriate number of blocks within each part */
        for (j = 0; j < devinfo.flgeo.region[i].nblks; j++) {
            end = off + devinfo.flgeo.region[i].blksiz;

            if (!erasebegin)
                erasebegin = (low >= off && low < end);
            
            if (erasebegin) {
                ba = (off >> BYTE_OFFSET_FACTOR (devinfo.part_mode));           /* We calculate 'block' address here. */ 
                                                                                /* Compensate for having calculated offset as "bytes" earlier */

                DPRINTF ("BLKERASE: Erasing block: %d:%d, offset: 0x%x, ba: 0x%x.\r\n", i, j, off, ba); 
                if (status = (*the_target->blkerasedev) (ba))                   /* Ask the target to erase the specific block */
                    return status;
                if (high >= off && high <= end)                                 /* We reached the address where we need to stop erasing. So done. */
                    return 0;
                DPRINTF ("BLKERASE: Done.\r\n"); 
            }
            off = end;                                                          /* Advance current offset */
        }
    }
    return ERR_FLASH_BLK_ERASE;
}
#endif /* FW_CFG_ERASE */

#ifdef FW_CFG_PROG
int8_t tg_prog_dev (uint32_t dstoffset, uint8_t *srcaddr, uint32_t nbytes)
{
     if (the_target && the_target->progdev)
        return (*the_target->progdev) (dstoffset, srcaddr, nbytes);
    else
        return ERR_NOTSUPPORTED;
}
#endif  /* FW_CFG_PROG */
#endif  /* FW_CFG_DEV_OPERATE */
/*==============================================================================*/

/*==============================================================================*/

/*
 * Flash interface generics follow                                              
 */
void fl_cmdwrite  (uint32_t cmdaddr, uint8_t cmddata)
{
    uint32_t cmd;

    cmdaddr = cmdaddr << devinfo.addr_shift;
    if (form_cmd)
        cmd = (*form_cmd)(cmddata);                                     /* Use pre-configure interleave algorithm to interleave command */
    else
        cmd = cmddata;

    (*fl_write_p) (cmdaddr, cmd);
}

uint32_t fl_read (uint32_t addr)
{
    return (*fl_read_p) (addr);
}

void fl_write (uint32_t addr, uint32_t data)
{
    (*fl_write_p) (addr, data);
}

uint32_t fl_form_cmd (uint8_t cmd)
{
    if (form_cmd)
        return form_cmd (cmd);
    
    return (uint32_t)cmd;
}

/*      mem_read
 *      Read memory in chunks in units that can be programmed onto flash
 *      i.e for 32-bit device widths, read a full word
 *          for 16-bit device widths, read a half-word
 *          for  8-bit device widths, read a byte
 */
uint32_t mem_read (uint8_t *srcaddr)
{
    uint32_t *srcp32;
    uint16_t *srcp16;
    uint8_t  *srcp08;
    uint32_t data;

    if (devinfo.bus_width == 32) {
        srcp32 = (uint32_t*)srcaddr;
        data = *srcp32;
    }
    else if (devinfo.bus_width == 16) {
        srcp16 = (uint16_t*)srcaddr;
        data = *srcp16 & MASK16;
    } 
    else {
        srcp08 = (uint8_t*)srcaddr;
        data = *srcp08 & MASK8;
    } 

    return data;
}

/*==============================================================================*/

/*
 * Flash low-level interface specifics follow                                   
 */

uint32_t fl_read_08 (uint32_t addr)
{
    uint32_t val;
    val = *FLASHP_08 (addr) & MASK8;
    return val;
}

uint32_t fl_read_16 (uint32_t addr)
{
    uint32_t val;
    val = (*FLASHP_16 (addr & ALIGN16)) & MASK16;
    return val;
}

uint32_t fl_read_32 (uint32_t addr)
{
    uint32_t val;
    val = *FLASHP_32 (addr & ALIGN32);
    return val;
}

void fl_write_08 (uint32_t addr, uint32_t data)
{
    *FLASHP_08 (addr) = (data & MASK8);
}

void fl_write_16 (uint32_t addr, uint32_t data)
{
    *FLASHP_16 (addr & ALIGN16) = (data & MASK16);
}

void fl_write_32 (uint32_t addr, uint32_t data)
{
    *FLASHP_32 (addr & ALIGN32) = data;
}

/*      form_cmd_16_2 
 *      Form a command word for part configuration PART_LAYOUT_16_X_X_2
 *      E.G: form_cmd_16_2 (0x98) returns "0x00009898"
 */
uint32_t form_cmd_16_2 (uint8_t cmd)
{
    uint32_t ret;
    ret = (uint32_t) ((cmd << 8)  |
                      (cmd));
    return ret;
}

/*      form_cmd_32_2  
 *      Form a command word for part configuration PART_LAYOUT_32_X_X_2 
 *      E.G: form_cmd_32_2 (0x98) returns "0x00980098"
 */
uint32_t form_cmd_32_2 (uint8_t cmd)
{
    uint32_t ret;
    ret = (uint32_t) ((cmd << 16) | 
                      (cmd));
    return ret;
}

/*      form_cmd_32_4  
 *      Form a command word for part configuration PART_LAYOUT_32_X_X_4 
 *      E.G: form_cmd_32_4 (0x98) returns "0x98989898"
 */
uint32_t form_cmd_32_4 (uint8_t cmd)
{
    uint32_t ret;
    ret = (uint32_t) ((cmd << 24) | 
                      (cmd << 16) | 
                      (cmd << 8)  |
                      (cmd));
    return ret;
}
/*==============================================================================*/



