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


/* flash.h
 * Requires "portab.h" to included prior to inclusion of this 
 */

#ifndef _FS_FLASH_H
#define _FS_FLASH_H

/* /\* Supported part layouts   *\/ */
/* #define PART_LAYOUT_UNKNOWN     0xfffff                 /\* Un-determinable layout                           *\/ */
/* #define PART_LAYOUT_08_08_1     0x08081                 /\* 8-bit data bus  - Single 8-bit part              *\/ */
/* #define PART_LAYOUT_08_16_1     0x08101                 /\* 8-bit data bus  - Single 16-bit part in x8 mode  *\/ */
/* #define PART_LAYOUT_08_32_1     0x08201                 /\* 8-bit data bus  - Single 32-bit part in x8 mode  *\/ */
/* #define PART_LAYOUT_16_16_1     0x10101                 /\* 16-bit data bus - Single 16-bit part             *\/ */
/* #define PART_LAYOUT_16_08_2     0x10082                 /\* 16-bit data bus - Two 8-bit parts                *\/ */
/* #define PART_LAYOUT_16_32_1     0x10201                 /\* 16-bit data bus - Single 32-bit part in x16 mode *\/ */
/* #define PART_LAYOUT_32_08_4     0x20084                 /\* 32-bit data bus - Four 8-bit parts               *\/ */
/* #define PART_LAYOUT_32_16_2     0x20102                 /\* 32-bit data bus - Two 16-bit parts               *\/ */
/* #define PART_LAYOUT_32_32_1     0x20201                 /\* 32-bit data bus - Single 32-bit part             *\/ */

/* /\* Unsupported/Untested part layouts *\/ */
/* #define PART_LAYOUT_16_16_2     0x10102                 /\* 16-bit data bus - Two 16-bit parts in x8 mode    *\/ */
/* #define PART_LAYOUT_16_32_2     0x10102                 /\* 16-bit data bus - Two 32-bit parts in x16 mode   *\/ */
/* #define PART_LAYOUT_32_16_4     0x20104                 /\* 32-bit data bus - Four 16-bit parts in x8 mode   *\/ */
/* #define PART_LAYOUT_32_32_2     0x20202                 /\* 32-bit data bus - Two 32-bit parts in x16 mode   *\/ */
/* #define PART_LAYOUT_32_32_4     0x20204                 /\* 32-bit data bus - Four 32-bit parts in x8 mode   *\/ */




/*==============================================================================*/
/* Supported part layouts                                                       */
/* PART_<BW>_<PBW>_<PM>_<NP>                                                    */
/* BW  - Total bus width                                                        */
/* PBW - Max device width of part                                               */
/* PM  - Mode in which part is working                                          */
/* NP  - Number of parts in parallel                                            */
/*==============================================================================*/
#define PART_LAYOUT_UNKNOWN     0xfffff                 /* Un-determinable layout         */
/* 8 bit device bus width */
#define PART_LAYOUT_08_08_08_1     0x0808081            /* Single 8-bit part              */
#define PART_LAYOUT_08_16_08_1     0x0810081            /* Single 16-bit part in x8 mode  */
#define PART_LAYOUT_08_32_08_1     0x0820081            /* Single 32-bit part in x8 mode  */
/* 16 bit device bus width */
#define PART_LAYOUT_16_16_16_1     0x1010101            /* Single 16-bit part             */
#define PART_LAYOUT_16_08_08_2     0x1008082            /* Two 8-bit parts                */
#define PART_LAYOUT_16_32_16_1     0x1020101            /* Single 32-bit part in x16 mode */
/* 32 bit device bus width */
#define PART_LAYOUT_32_08_08_4     0x2008084            /* Four 8-bit parts               */
#define PART_LAYOUT_32_16_16_2     0x2010102            /* Two 16-bit parts               */
#define PART_LAYOUT_32_32_32_1     0x2020201            /* Single 32-bit part             */

/*==============================================================================*/
/* Unsupported/Untested part layouts                                            */
/*==============================================================================*/
#define PART_LAYOUT_16_16_08_2     0x1010082                 /* Two 16-bit parts in x8 mode    */
#define PART_LAYOUT_32_32_16_2     0x2020102                 /* Two 32-bit parts in x16 mode   */
#define PART_LAYOUT_16_16_08_4     0x1010084                 /* Four 16-bit parts in x8 mode   */
#define PART_LAYOUT_32_32_16_2     0x2020102                 /* Two 32-bit parts in x16 mode   */
#define PART_LAYOUT_32_32_08_4     0x2020084                 /* Four 32-bit parts in x8 mode   */


#define PART_MODE_08               8
#define PART_MODE_16              16
#define PART_MODE_32              32

#define PART_MODE(layout)       ((layout & 0xff0) >> 4)        
#define PART_COUNT(layout)      ((layout & 0xf))        

#define BYTE_OFFSET_FACTOR(mode)    ((mode >> 4))               /* factor(8) = 0, factor(16) = 1, factor(32) = 2 */

#define MAX_ERASE_REGIONS           32

typedef struct cfi_qry_s  {
    uint8_t  qrystr[3]                  ;       
    uint16_t pri_vendor_id              ;
    uint16_t pri_addr                   ;
    uint16_t alt_vendor_id              ;
    uint16_t alt_addr                   ;
    uint8_t  vccmin                     ;       
    uint8_t  vccmax                     ;
    uint8_t  vppmin                     ;
    uint8_t  vppmax                     ;
    uint8_t  word_write_timeout         ;
    uint8_t  buffer_write_timeout       ;
    uint8_t  block_erase_timeout        ;
    uint8_t  full_erase_timeout         ;
    uint8_t  word_write_max_timeout     ;
    uint8_t  buffer_write_max_timeout   ;
    uint8_t  block_erase_max_timeout    ;
    uint8_t  full_erase_max_timeout     ;
    uint8_t  partsiz                    ;       
    uint16_t interface_desc             ;
    uint16_t max_write_buf_siz          ;
    uint8_t  num_erase_regions          ;
    uint32_t erase_region_info[1]       ;       
} __attribute__ ((packed)) cfi_qry_t;


struct dev_info_s;

typedef void    (*init_dev_params_t)(struct dev_info_s *);
typedef void    (*rst_dev_t      )(void);
typedef void    (*rst_blk_t      )(uint32_t blkaddr);
typedef int8_t  (*erase_dev_t    )(void);
typedef int8_t  (*blk_erase_dev_t)(uint32_t);
typedef int8_t  (*prog_dev_t     )(uint32_t, uint8_t*, uint32_t);

typedef struct target_ops_s {
    init_dev_params_t   initdev;                /* Pointers to interface routines       */
    rst_dev_t           rstdev;                 /*  .  */
    rst_blk_t           rstblk;                 /*  .  */    
    erase_dev_t         erasedev;               /*  .  */
    blk_erase_dev_t     blkerasedev;            /*  .  */
    prog_dev_t          progdev;                /*  .  */
} target_ops_t;

typedef struct region_info_s {
    uint32_t  offset;
    uint32_t  nblks;
    uint32_t  blksiz;
} region_info_t;

typedef struct flash_geometry_s {
    uint32_t      tot_regions;
    uint32_t      tot_blks;
    region_info_t region[MAX_ERASE_REGIONS];
} flash_geometry_t;


typedef struct dev_info_s {
    uint32_t            bus_width;
    uint32_t            base_addr;
    uint32_t            layout;
    uint8_t             part_mode;
    uint8_t             num_parts;
    uint8_t             addr_step;
    uint8_t             addr_shift;
    cfi_qry_t           cfiqry;
    flash_geometry_t    flgeo;
    target_ops_t       *devops;
#if 0
    uint32_t            ext_qry_optional_features;   /* Only for Intel */
#endif
} dev_info_t;

/* Interface to generic operations */
uint32_t fl_read  (uint32_t addr);
void     fl_write (uint32_t addr, uint32_t data);
void     fl_cmdwrite  (uint32_t cmdaddr, uint8_t cmddata);
uint32_t fl_form_cmd (uint8_t cmd);
uint32_t mem_read (uint8_t *addr);

#endif /* _FS_FLASH_H */
