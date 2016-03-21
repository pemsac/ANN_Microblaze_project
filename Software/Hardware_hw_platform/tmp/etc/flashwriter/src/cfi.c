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

#ifdef FW_CFG_CFIQRY

/*==============================================================================*/
/* Macros and defines                                                           */
/*==============================================================================*/
#define CFI_QRY_CMD_ADDR        0x55
#define CFI_QRY_CMD             0x98
#define CFI_QRY_STR_ADDR_1      0x10
#define CFI_QRY_STR_ADDR_2      0x11
#define CFI_QRY_STR_ADDR_3      0x12
#define FRR_CMD                 0xFF

#define PLATFORM_FLASH_ASYNC_ADDR       0xBDDF
#define PLATFORM_FLASH_ASYNC_SETUP      0x0060
#define PLATFORM_FLASH_ASYNC_CONFIRM    0x0003

#ifdef __LITTLE_ENDIAN__ 
#define htonl(val)    (val)
#define htons(val)    (val)
#else
#define htonl(val)    ((((uint32_t)(val) & 0x000000FF) << 24) |    \
                       (((uint32_t)(val) & 0x0000FF00) <<  8) |    \
                       (((uint32_t)(val) & 0x00FF0000) >>  8) |    \
                       (((uint32_t)(val) & 0xFF000000) >> 24))

#define htons(val)    ((((uint16_t)(val) & 0x000000FF) <<  8) |    \
                       (((uint16_t)(val) & 0x0000FF00) >>  8))
#endif

/*==============================================================================*/
/* Declarations                                                                 */
/*==============================================================================*/

/* Interface to vendor specific files */
extern uint32_t fl_read  (uint32_t addr);
extern void     fl_write (uint32_t addr, uint32_t data);
extern void     fl_cmdwrite  (uint32_t cmdaddr, uint8_t cmddata);
extern uint32_t fl_form_cmd (uint8_t cmd);
extern uint32_t mem_read (uint8_t *addr);
extern uint32_t fl_read_08 (uint32_t addr);
extern uint32_t fl_read_16 (uint32_t addr);
extern uint32_t fl_read_32 (uint32_t addr);
extern void     fl_write_08 (uint32_t addr, uint32_t data);
extern void     fl_write_16 (uint32_t addr, uint32_t data);
extern void     fl_write_32 (uint32_t addr, uint32_t data);
extern uint32_t form_cmd_16_2 (uint8_t cmd);
extern uint32_t form_cmd_32_2 (uint8_t cmd);
extern uint32_t form_cmd_32_4 (uint8_t cmd);
extern uint32_t (*form_cmd)   (uint8_t);
extern uint32_t (*fl_read_p)  (uint32_t);
extern void (*fl_write_p) (uint32_t, uint32_t);
extern int8_t  fl_cfg_rw_mode ();
extern int8_t  fl_cfg_cmd_interleave (uint32_t layout);

/* CFI interface routines */
int8_t   cfi_init (uint32_t ba, uint8_t bw);
uint32_t cfi_qry_part_layout ();

/*==============================================================================*/
/* Data structures                                                              */
/*==============================================================================*/
extern dev_info_t devinfo;

/*==============================================================================*/
/* Function definitions                                                         */
/*==============================================================================*/


/*==============================================================================*/

/*
 *  cfi_init -- Initialize device info using CFI
 * 
 *  - Perform CFI queries and initialize flash device info.
 *  - Return zero on success.
 */

int8_t cfi_init (uint32_t ba, uint8_t bw)
{
    uint32_t        layout;
    int8_t          i, j, k, l;
    uint8_t        *ptr;
    uint32_t        siz, offset;
    uint32_t        nsect, sectsiz;
    region_info_t   tmp;
    uint8_t         cfi_shift;

    devinfo.base_addr = ba;
    devinfo.bus_width = bw;
    devinfo.addr_shift = bw >> 4;                                       /* 8: no shift, 16: shift = 1, 32: shift = 2 */ 

    if ((i = fl_cfg_rw_mode ()))                                        /* Configure read and write modes  */
        return i;

#ifdef PLATFORM_FLASH
    /* Have to place platform flash in async mode before doing any operations */
    fl_write (PLATFORM_FLASH_ASYNC_ADDR << devinfo.addr_shift, PLATFORM_FLASH_ASYNC_SETUP);
    fl_write (PLATFORM_FLASH_ASYNC_ADDR << devinfo.addr_shift, PLATFORM_FLASH_ASYNC_CONFIRM);
#endif

    /* Do CFI Query to figure out flash part arrangement */
    /* Configure interleave algorithm appropriately      */
    layout = cfi_qry_part_layout ();

    fl_cmdwrite (0, FRR_CMD);                                           /* Reset the flash for subsequent use */

    if ((i = fl_cfg_cmd_interleave (layout)))                           /* Configure flash command interleave based on part layout */
        return i; 

    devinfo.layout = layout;
    devinfo.part_mode = PART_MODE (layout);
    devinfo.num_parts  = PART_COUNT (layout);

    /* Adjust for device addressing, when parts are used in non-device bus width mode 
       CFI query table must be read in the correct byte address location.
       (Supported layouts only) */
    if (layout == PART_LAYOUT_08_16_08_1) 
        cfi_shift = 1;
    else if (layout == PART_LAYOUT_08_32_08_1)
        cfi_shift = 2;
    else if (layout == PART_LAYOUT_16_32_16_1)
        cfi_shift = 1;
    else
        cfi_shift = 0;
 
    fl_cmdwrite (CFI_QRY_CMD_ADDR << cfi_shift, CFI_QRY_CMD);           /* Set flash in CFI query mode  */
    cfi_shift += devinfo.addr_shift;                                    /* Add the CFI shift to the device address shift for subsequent reads below */

    ptr = (uint8_t*)&devinfo.cfiqry.qrystr[0];
    for (i = 0; i < 3; i++)
        *ptr++ = fl_read ((0x10 + i) << cfi_shift);

    ptr = (uint8_t*) &devinfo.cfiqry.pri_vendor_id;
    for (i = 0; i < 8; i++)
        *ptr++ = fl_read ((0x13 + i) << cfi_shift);

    devinfo.cfiqry.pri_vendor_id  = htons (devinfo.cfiqry.pri_vendor_id);
    devinfo.cfiqry.pri_addr       = htons (devinfo.cfiqry.pri_addr);
    devinfo.cfiqry.alt_vendor_id  = htons (devinfo.cfiqry.alt_vendor_id);
    devinfo.cfiqry.alt_addr       = htons (devinfo.cfiqry.alt_addr);

    ptr = (uint8_t*) &devinfo.cfiqry.vccmin;
    for (i = 0; i < 13; i++)
        *ptr++ = fl_read ((0x1b + i) << cfi_shift);

    ptr = (uint8_t*) &devinfo.cfiqry.interface_desc;
    for (i = 0; i < 4; i++)
        *ptr++ = fl_read ((0x28 + i) << cfi_shift);

    devinfo.cfiqry.interface_desc    = htons (devinfo.cfiqry.interface_desc);
    devinfo.cfiqry.max_write_buf_siz = htons (devinfo.cfiqry.max_write_buf_siz);

    ptr = (uint8_t*) &devinfo.cfiqry.num_erase_regions;
    *ptr++ = fl_read((0x2c) << cfi_shift);

    devinfo.flgeo.tot_regions = devinfo.cfiqry.num_erase_regions;
    if (devinfo.flgeo.tot_regions >= MAX_ERASE_REGIONS) 
        return ERR_CFIQRY_MAX_REGIONS;

    devinfo.flgeo.tot_blks = 0;
    siz = 0;
    offset = 0x2d;

    for (j = 0; j < devinfo.flgeo.tot_regions; j++) {
        for (i = 0; i < 4; i++)
            *ptr++ = fl_read (offset++ << cfi_shift);

        devinfo.cfiqry.erase_region_info[0] = htonl (devinfo.cfiqry.erase_region_info[0]);

        nsect = (devinfo.cfiqry.erase_region_info[0] + 1) & 0xffff;
        sectsiz = ((devinfo.cfiqry.erase_region_info[0] & 0xffff0000) >> 16);
        sectsiz = ((sectsiz == 0) ? 128 : (sectsiz * 256));

        devinfo.flgeo.region[j].offset = siz;
        devinfo.flgeo.region[j].nblks = nsect;
        devinfo.flgeo.region[j].blksiz = sectsiz;
        devinfo.flgeo.tot_blks += nsect;

        siz += (nsect * sectsiz);
        ptr = (uint8_t*)&devinfo.cfiqry.erase_region_info[0];
    }
    
/* Re-order sectors. If neither BOTTOM_BOOT_FLASH nor TOP_BOOT_FLASH are defined, the sector map is used as is */
#if defined (BOTTOM_BOOT_FLASH)
    /* Sort sectors to match a bottom-boot flash. Smaller sectors first, larger sectors last */
    for (k = 0; k <= devinfo.flgeo.tot_regions - 1; k++) {
        for (l = 0; l < (devinfo.flgeo.tot_regions - 1 - k); l++) {
            if (devinfo.flgeo.region[l].blksiz > devinfo.flgeo.region[l + 1].blksiz) {
                tmp = devinfo.flgeo.region[l];
                devinfo.flgeo.region[l] = devinfo.flgeo.region[l+1];
                devinfo.flgeo.region[l+1] = tmp;
            }
        }
    }
#elif defined (TOP_BOOT_FLASH)
    /* Sort sectors to match a top-boot flash. Larger sectors first, smaller sectors last */
    for (k = 0; k <= devinfo.flgeo.tot_regions - 1; k++) {
        for (l = 0; l < (devinfo.flgeo.tot_regions - 1 - k); l++) {
            if (devinfo.flgeo.region[l].blksiz < devinfo.flgeo.region[l + 1].blksiz) {
                tmp = devinfo.flgeo.region[l];
                devinfo.flgeo.region[l] = devinfo.flgeo.region[l+1];
                devinfo.flgeo.region[l+1] = tmp;
            }
        }
    }
#endif
    
    if (siz != (1 << devinfo.cfiqry.partsiz)) {
        /* DPRINTF ("DBUG: Part Size (=0x%x)!= Calculated device size (=0x%x)\r\n", (1 << devinfo.cfiqry.partsiz), siz); */
        return ERR_CFIQRY_BLKINFO;
    }

    /*
     * Make sure we really have a CFI device.
     */
    if (devinfo.cfiqry.pri_addr) {
        if ((fl_read ((devinfo.cfiqry.pri_addr + 0) << cfi_shift) != fl_form_cmd ('P')) ||
            (fl_read ((devinfo.cfiqry.pri_addr + 1) << cfi_shift) != fl_form_cmd ('R')) ||
            (fl_read ((devinfo.cfiqry.pri_addr + 2) << cfi_shift) != fl_form_cmd ('I')))
            return ERR_CFIQRY_PRI;
            ;
    }

#if 0
    /* 
     * For Intel, find out some more about block lock/unlock support
     */ 
    if (devinfo.cfiqry.pri_vendor_id == 1 || devinfo.cfiqry.pri_vendor_id == 3) {
        ptr = (uint8_t*)&devinfo.ext_qry_optional_features;
        *ptr++ = fl_read ((devinfo.cfiqry.pri_addr + 5) << cfi_shift);
        *ptr++ = fl_read ((devinfo.cfiqry.pri_addr + 6) << cfi_shift);
        *ptr++ = fl_read ((devinfo.cfiqry.pri_addr + 7) << cfi_shift);
        *ptr++ = fl_read ((devinfo.cfiqry.pri_addr + 8) << cfi_shift);        
        devinfo.ext_qry_optional_features = htonl (devinfo.ext_qry_optional_features);
    } else
        devinfo.ext_qry_optional_features = 0x0;
#endif
        
    DPRINTF ("CFI Info: \r\n");
    DPRINTF ("Device base addr: 0x%x\r\n", devinfo.base_addr);
    DPRINTF ("Device bus width: %d\r\n"  , devinfo.bus_width);
    DPRINTF ("Number of parts : %d\r\n"  , devinfo.num_parts);
    DPRINTF ("Part mode       : %d\r\n"  , devinfo.part_mode);
    DPRINTF ("Part mem size   : 0x%x\r\n", (1 << devinfo.cfiqry.partsiz));
    DPRINTF ("Command set id  : %d\r\n"  , devinfo.cfiqry.pri_vendor_id);

    return 0;
}

/*
 *  cfi_qry_part_layout -- Figure out flash arrangement
 *
 *  - Send QRY command and figure out flash arrangement, based on the pattern of response.
 *  - Return part layout.
 */
uint32_t cfi_qry_part_layout ()
{
    uint8_t  reply8;
    uint16_t reply16;
    uint32_t reply32;
    uint32_t cfiqrystr_addr_1 = (CFI_QRY_STR_ADDR_1) << devinfo.addr_shift;

    if (devinfo.bus_width == 8) {
        fl_cmdwrite (0, FRR_CMD);
        fl_cmdwrite (CFI_QRY_CMD_ADDR, CFI_QRY_CMD);
        reply8 = fl_read (cfiqrystr_addr_1);
        if (reply8 == 0x51) 
            return PART_LAYOUT_08_08_08_1;

        /* Check for one 16 bit flash in x8 mode */
        fl_cmdwrite (0, FRR_CMD);                                       /* Set flash in read reset      */
        fl_cmdwrite (CFI_QRY_CMD_ADDR << 1, CFI_QRY_CMD);               /* Set flash in CFI query mode  */
        reply8 = fl_read (cfiqrystr_addr_1 << 1);                       /* Read query string            */
        if (reply8 == 0x51)                                             /* |Q| is the reply             */
            return PART_LAYOUT_08_16_08_1;

        /* Check for one 32 bit flash in x8 mode */
        fl_cmdwrite (0, FRR_CMD);                                       /* Set flash in read reset      */
        fl_cmdwrite (CFI_QRY_CMD_ADDR << 2, CFI_QRY_CMD);               /* Set flash in CFI query mode  */
        reply8 = fl_read (cfiqrystr_addr_1 << 2);                       /* Read query string            */
        if (reply8 == 0x51)                                             /* |Q| is the reply             */
            return PART_LAYOUT_08_32_08_1;
        
        return PART_LAYOUT_UNKNOWN;

    } else if (devinfo.bus_width == 16) {
        form_cmd = form_cmd_16_2;                                       /* Set 16_2 interleave mode     */
        fl_cmdwrite (0, FRR_CMD);                                       /* Set flash in read reset      */
        fl_cmdwrite (CFI_QRY_CMD_ADDR, CFI_QRY_CMD);                    /* Set flash in CFI query mode  */
        reply16 = fl_read (cfiqrystr_addr_1);                           /* Read query string            */
        if (reply16 == 0x5151)                                          /* |QQ| is the reply            */
            return PART_LAYOUT_16_08_08_2;

        form_cmd = NULL;                                                /* Set interleave mode - None   */
        fl_cmdwrite (0, FRR_CMD);                                       /* Set flash in read reset      */
        fl_cmdwrite (CFI_QRY_CMD_ADDR, CFI_QRY_CMD);                    /* Set flash in CFI query mode  */
        reply16 = fl_read (cfiqrystr_addr_1);                           /* Read query string            */        
        if (reply16 == 0x51)                                            /* |-Q| is the reply            */
            return PART_LAYOUT_16_16_16_1;
        
        /* Check for 32 bit flash in x16 mode */
        fl_cmdwrite (0, FRR_CMD);                                       /* Set flash in read reset      */
        fl_cmdwrite (CFI_QRY_CMD_ADDR << 1, CFI_QRY_CMD);               /* Set flash in CFI query mode  */
        reply16 = fl_read (cfiqrystr_addr_1 << 1);                      /* Read query string            */        
        if (reply16 == 0x51)                                            /* |-Q| is the reply            */
            return PART_LAYOUT_16_32_16_1;

        return PART_LAYOUT_UNKNOWN;

    } else if (devinfo.bus_width == 32) {
        form_cmd = form_cmd_32_4;                                       /* Set 32_4 interleave mode     */
        fl_cmdwrite (0, FRR_CMD);                                       /* Set flash in read reset      */
        fl_cmdwrite (CFI_QRY_CMD_ADDR, CFI_QRY_CMD);                    /* Set flash in CFI query mode  */
        reply32 = fl_read (cfiqrystr_addr_1);                           /* Read query string            */
        if (reply32 == 0x51515151)                                      /* |QQQQ| is the reply          */
            return PART_LAYOUT_32_08_08_4;

        form_cmd = form_cmd_32_2;                                       /* Set 32_2 interleave mode     */
        fl_cmdwrite (0, FRR_CMD);                                       /* Set flash in read reset      */
        fl_cmdwrite (CFI_QRY_CMD_ADDR, CFI_QRY_CMD);                    /* Set flash in CFI query mode  */
        reply32 = fl_read (cfiqrystr_addr_1);                           /* Read query string            */
        if (reply32 == 0x00510051)                                      /* |-Q-Q| is the reply          */
            return PART_LAYOUT_32_16_16_2;

        form_cmd = NULL;                                                /* Set interleave mode - None   */
        fl_cmdwrite (0, FRR_CMD);                                       /* Set flash in read reset      */
        fl_cmdwrite (CFI_QRY_CMD_ADDR, CFI_QRY_CMD);                    /* Set flash in CFI query mode  */
        reply32 = fl_read (cfiqrystr_addr_1);                           /* Read query string            */
        if (reply32 == 0x51)                                            /* |---Q| is the reply          */
            return PART_LAYOUT_32_32_32_1;

        return PART_LAYOUT_UNKNOWN;
    }
}

/*==============================================================================*/
#endif  /* FW_CFG_CFIQRY */
