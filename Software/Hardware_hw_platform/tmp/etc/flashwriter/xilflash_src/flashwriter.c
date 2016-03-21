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

/*  flashwriter.c
 *  - Top level interface for in-system flash programmer 
 *  - Receives commands from TCL driver executing on XMD, through mailboxes in memory
 *  - Breakpoint mechanism at 'xmd_callback' used to callback to XMD when operation is done.
 *  - Returns status through mailboxes in memory
 *  - Compiled in different profiles, based on required functionality
 *      - FW_MODE_CFIQRY, FW_MODE_ERASE, FW_MODE_PROG ...
 *  - Compiled with different algo support for _ERASE and _PROG modes
 *      - FW_SUPPORT_INTEL, FW_SUPPORT_AMD
 *
 *  - Flashwriter should occupy < 8K in all configurations. Current size info listed below.
 *
 *  ---------------------------------------------
 *  Size information with only -Os (15 JUN 2009)
 *  ---------------------------------------------
 *  Mode                MB Size             PPC Size
 *                 (accounting for Imms) 
 *                          
 *  FW_MODE_CFIQRY      1848                1380
 *
 *  FW_MODE_ERASE +     1310                1310
 *  FW_SUPPORT_AMD      
 *
 *  FW_MODE_ERASE +     1528                1460
 *  FW_SUPPORT_INTEL 
 *
 *  FW_MODE_PROG +      1060                10f0
 *  FW_SUPPORT_AMD      
 *
 *  FW_MODE_PROG +      1290                1250
 *  FW_SUPPORT_INTEL      
 *
 *
 *  Unsupported features:
 *      - Flash layouts - PART_LAYOUT_16_16_08_2, PART_LAYOUT_32_32_16_2, PART_LAYOUT_16_16_08_4, PART_LAYOUT_32_32_16_2, PART_LAYOUT_32_32_08_4 (refer flash.h)
 *      - Mitsubishi algorithms
 *      - Fast/buffered programming
 *      - Block unlocking/locking/protection for AMD algorithms
 */ 

#include "portab.h"
#include "errors.h"
#include "defs.h"
#include "flash.h"
#include "xilflash.h"
										
#define WRITER_MAX_PARAMS   4

/*==============================================================================*/
/* Macros and defines                                                           */
/*==============================================================================*/

/* Host commands */
#define CMD_NONE                '0'
#define CMD_CFI_QRY             'C'
#define CMD_INIT_DEV            'I'
#define CMD_ERASE_DEV           'E'
#define CMD_BLK_ERASE_DEV       'B'
#define CMD_RST_DEV             'R'
#define CMD_PROG_DEV            'P'
#define CMD_BATCH               'b'
#define CMD_EXIT                'X'
#define CMD_REPLY               'r'

/* Status indicators */
#define STATUS_IDLE             'I'
#define STATUS_BUSY             'B'
#define STATUS_ERR              'E'
#define STATUS_SUCCESS          'S'
#define STATUS_EXIT             'X'


/*============================================================================
 * Message formats:        (X = Don't care)
 * ----------------
 *
 * Host to Writer Messages:
 * ----------------------
 * 1. Null Command
 *    {CMD_NONE, X, --PAD--, (X...X)}
 * 
 * 2. CFI Query Command
 *   {CMD_CFI_QRY, X, --PAD--, (FLASH_BASE_ADDR, flash_buswidth...X)}
 *
 * 3. Initialize Device Command
 *   {CMD_INIT_DEV, X, --PAD--, (X...X)}
 *
 * 4. Reset Device Command
 *   {CMD_RST_DEV, X, --PAD--, (X...X)}
 *
 * 5. Erase Device Command
 *   {CMD_ERASE_DEV, X, --PAD--, (X...X)}
 *
 * 6. Block Erase Device Command
 *   {CMD_BLK_ERASE_DEV, X, --PAD--, (ADDR, NBYTES...X)}
 *
 * 7. Program Device Command
 *   {CMD_PROG_DEV, X, --PAD--, (PROG_OFFSET, DATA_BYTES_COUNT...X)}
 *
 * 8. Batch Mode Command
 *   {CMD_BATCH, X, --PAD--, (PROG_OFFSET, DATA_BYTES_COUNT, SRCADDR...X)}
 *
 * Writer to Host Messages:
 * ----------------------
 * 1. Null Command
 *    {CMD_NONE, X, --PAD--, (X...X)}
 *
 * 2. CFI Query Command Response
 *    {CMD_REPLY, STATUS, --PAD--, (DEV_ALGO, NUM_PARTS, PART_MODE, PART_SIZ)}
 *
 * 3. Other commands
 *    {X, STATUS,  --PAD--, (ERR_CODE...X)} 
 *============================================================================*/
typedef struct msg_s {
    uint8_t     cmd;
    uint8_t     status;
    uint8_t     pad[2];
    uint32_t    param[WRITER_MAX_PARAMS];
} __attribute__ ((packed)) msg_t;


static char *errmap[] = {
    "",
    "Target does not support requested operation !",                            /* ERR_NOTSUPPORTED             */
    "Unable to query target part layout !",                                     /* ERR_CFIQRY_LAYOUT            */
    "Not a CFI compliant flash device - Did not scan a P-R-I !",                /* ERR_CFIQRY_PRI               */
    "CFI QRY of block info returned inconsistent results",                      /* ERR_CFIQRY_BLKINFO           */
    "CFI QRY ran out of space to accomodate region information",                /* ERR_CFIQRY_MAX_REGIONS       */
    "Flash timeout error !",                                                    /* ERR_FLASH_TIMEOUT            */
    "The flash sequence provided was incorrect !",                              /* ERR_FLASH_SEQ                */
    "The flash erase operation errored out !",                                  /* ERR_FLASH_ERASE              */
    "The flash block erase operation errored out !",                            /* ERR_FLASH_BLK_ERASE          */
    "The flash programming operation errored out !",                            /* ERR_FLASH_PROG               */
    "The flash operation ran into a lock error !"                               /* ERR_FLASH_LOCK               */
    "The flash part ran into a voltage error !"                                 /* ERR_FLASH_VOLTAGE            */
};

/* Mailbox access macros */
#define get_host_cmd()                  (rcvmbox.cmd)
#define get_host_status()               (rcvmbox.status)
#define get_writer_status()             (sndmbox.status)
#define get_host_param(n)               (rcvmbox.param[n])
#define get_writer_param(n)             (sndmbox.param[n])
#define put_writer_cmd(c)               (sndmbox.cmd = c)
#define put_writer_status(s)            (sndmbox.status = s)
#define put_writer_param(n, val)        (sndmbox.param[n] = val)
#define clr_host_cmd()                  (rcvmbox.cmd = CMD_NONE)
#define clr_writer_cmd()                (sndmbox.cmd = CMD_NONE)
#define clr_writer_status()             (sndmbox.status = STATUS_IDLE)

/* Message parameter numbers */
#define BASE_ADDR_PARAM                 0
#define BUS_WIDTH_PARAM                 1
#define ADDR_PARAM                      0
#define NBYTES_PARAM                    1
#define PROG_OFFSET_PARAM               0
#define DATA_BYTES_COUNT_PARAM          1
#define SRCADDR_PARAM                   2
#define DEV_ALGO_PARAM                  0
#define NUM_PARTS_PARAM                 1
#define PART_MODE_PARAM                 2
#define PART_SIZ_PARAM                  3
#define ERR_CODE_PARAM                  0

/*==============================================================================*/
/* Declarations                                                                 */
/*==============================================================================*/
void            xmd_callback () __attribute__ ((noinline));
int             flashwriter_sm ();
int8_t          check_error (); 
int             flashwriter_batch ();
extern int8_t   tg_init_dev ();
extern void     tg_rst_dev ();
extern int8_t   tg_erase_dev ();
extern int8_t   tg_blk_erase_dev (uint32_t addr, uint32_t nbytes);
extern int8_t   tg_prog_dev (uint32_t prog_addr, uint8_t *data_addr, uint32_t data_count);
extern int8_t   cfi_init (uint32_t ba, uint8_t bw);
extern uint32_t cfi_qry_part_layout ();
//extern dev_info_t devinfo;

XFlash devinfo __attribute__ ((section (".data"))); /* XFlash Instance. */

/*==============================================================================*/
/* Data structures                                                              */
/*==============================================================================*/
/* Host to Writer and Writer to Host mailboxes */
volatile msg_t  rcvmbox __attribute__ ((section(".data"))), 
                sndmbox __attribute__ ((section(".data"))),
               *rcvptr = &rcvmbox,
               *sndptr = &sndmbox; 

/* Pointer to device info structure and its size */
//volatile dev_info_t *devinfop  = &devinfo;
volatile XFlash *devinfop  = &devinfo;
volatile int devinfosz = sizeof (devinfo);

/* Data Transfer Buffer */
#ifndef MEMBUF_SIZ
#define MEMBUF_SIZ                      4
#endif

volatile uint8_t membuf[MEMBUF_SIZ] __attribute__((aligned(4)));
volatile uint8_t readbuf[MEMBUF_SIZ] __attribute__((aligned(4)));
volatile int     membufsiz = MEMBUF_SIZ;
volatile uint8_t *membufptr = membuf;

/*==============================================================================*/
/* Function definitions                                                         */
/*==============================================================================*/

/* 
 *  xmd_callback -- XMD Callback routine.
 *
 *  - Dummy routine used by XMD to set a breakpoint at.                 
 *  - Serves as a callback for the writer.
 *
 */

void xmd_callback ()
{
    membufptr = membuf;         /* Dummy action to prevent function from being removed */
}

/*
 *  flashwriter_sm  -- Central flashwriter state-machine.
 *  
 *  - Waits for commands to be populated in mailboxes, by XMD.
 *  - Executes commands using CFI driver and flash device drivers.
 *  - Returns status, parameters back in mailboxes, to be read by XMD
 *  - Can be compiled in different modes, to scale down capabilities.
 */

#ifdef PLATFORM_FLASH
int IsPlatformFlash = 1;
#else
int IsPlatformFlash = 0;
#endif

int flashwriter_sm ()
{
    uint8_t     cmd;
    uint8_t     ret;
    uint32_t    offset, srcaddr, nbytes;
    uint8_t     status;
    uint32_t    flash_baseaddr;
    uint8_t     flash_buswidth;

    while (1) {
        cmd = get_host_cmd ();
        switch (cmd) {
//#ifdef FW_CFG_CFIQRY
            case CMD_CFI_QRY:
                DPRINTF("Flashwriter: CMD_CFI_QRY...");
                put_writer_status (STATUS_BUSY);
                flash_baseaddr   = get_host_param (BASE_ADDR_PARAM);
                flash_buswidth  = get_host_param (BUS_WIDTH_PARAM);
				DPRINTF("flash_baseaddr: %x, flash_buswidth: %x\n", flash_baseaddr, flash_buswidth);
                //flash_baseaddr   = 0x48000000; //FLASH_BASEADDR;
                //flash_buswidth  = 16; //FLASH_BUSWIDTH/8;
                if ((ret = XFlash_Initialize(&devinfo, flash_baseaddr, flash_buswidth/8, IsPlatformFlash))) {
//                if ((ret = cfi_init (flash_baseaddr, flash_buswidth))) {
                    put_writer_status (STATUS_ERR);
                    put_writer_param (ERR_CODE_PARAM, ret);
                    DPRINTF("error code=0x%0x\n\r", ret);
                    break;
//                	return ret;
                }
                DPRINTF("cfi query succes. commandset=0x%x\n\r", devinfo.CommandSet);
                put_writer_cmd  (CMD_REPLY);
                put_writer_param (DEV_ALGO_PARAM, devinfo.CommandSet);
//                put_writer_param (DEV_ALGO_PARAM, devinfo.cfiqry.pri_vendor_id);
//                put_writer_param (NUM_PARTS_PARAM, devinfo.num_parts);
//                put_writer_param (PART_MODE_PARAM, devinfo.part_mode);
//                put_writer_param (PART_SIZ_PARAM, (1 << devinfo.cfiqry.partsiz));
                put_writer_status (STATUS_SUCCESS);
                break;
//#endif
//#ifdef FW_CFG_DEV_OPERATE
            case CMD_INIT_DEV:
                DPRINTF("Flashwriter: CMD_INIT_DEV...");
                put_writer_status (STATUS_BUSY);
//                if ((ret = tg_init_dev ())) {
//                    put_writer_status (STATUS_ERR);
//                    put_writer_param (ERR_CODE_PARAM, ret);
//                    break;
//                	return ret;
//                }

//                tg_rst_dev ();
#if 0
                if ((ret = XFlash_Reset(&devinfo))) {
                    put_writer_status (STATUS_ERR);
                    put_writer_param (ERR_CODE_PARAM, ret);
                    DPRINTF("error code=0x%0x\n\r", ret);
                    break;
//                	return ret;
                }
#endif
                put_writer_cmd  (CMD_REPLY);
                put_writer_status (STATUS_SUCCESS);
                break;
            case CMD_RST_DEV:
                DPRINTF("Flashwriter: CMD_RST_DEV...");
                put_writer_status (STATUS_BUSY);
//                tg_rst_dev ();
                if ((ret = XFlash_Reset(&devinfo))) {
                    put_writer_status (STATUS_ERR);
                    put_writer_param (ERR_CODE_PARAM, ret);
                    DPRINTF("error code=0x%0x\n\r", ret);
                    break;
//                	return ret;
                }
                put_writer_status (STATUS_SUCCESS);
                break;

//#ifdef FW_CFG_ERASE
            case CMD_ERASE_DEV:
                DPRINTF("Flashwriter: CMD_ERASE_DEV...");
                put_writer_status (STATUS_BUSY);
                if ((devinfo.CommandSet == XFL_CMDSET_INTEL_STANDARD) ||
                	(devinfo.CommandSet == XFL_CMDSET_INTEL_EXTENDED) ||
                	(devinfo.CommandSet == XFL_CMDSET_INTEL_G18)) {
                	if ((ret = XFlash_Unlock(&devinfo, get_host_param (ADDR_PARAM),
                                                 get_host_param (NBYTES_PARAM)))) {
                		put_writer_status (STATUS_ERR);
                		put_writer_param  (0, ret);
                		break;
                    DPRINTF("error code=0x%0x\n\r", ret);
//                		return ret;
                	}
                }
                if ((ret = XFlash_Erase(&devinfo, get_host_param (ADDR_PARAM),
                		                get_host_param (NBYTES_PARAM)))) {
                    put_writer_status (STATUS_ERR);
                    put_writer_param  (0, ret);
                    DPRINTF("error code=0x%0x\n\r", ret);
                    break;
//                	return ret;
                }
                put_writer_status (STATUS_SUCCESS);
                break;
            case CMD_BLK_ERASE_DEV:
                DPRINTF("Flashwriter: CMD_BLK_ERASE_DEV...");
                put_writer_status (STATUS_BUSY);
//                if ((ret = tg_blk_erase_dev (get_host_param (ADDR_PARAM),
//                                             get_host_param (NBYTES_PARAM))) == 0) {
//                    put_writer_status (STATUS_SUCCESS);
//                } else {
//                    put_writer_status (STATUS_ERR);
//                    put_writer_param  (0, ret);
//                }
                if ((devinfo.CommandSet == XFL_CMDSET_INTEL_STANDARD) ||
                	(devinfo.CommandSet == XFL_CMDSET_INTEL_EXTENDED) ||
                	(devinfo.CommandSet == XFL_CMDSET_INTEL_G18)) {
                	if ((ret = XFlash_Unlock(&devinfo, get_host_param (ADDR_PARAM),
                                                 get_host_param (NBYTES_PARAM)))) {
                		put_writer_status (STATUS_ERR);
                		put_writer_param  (0, ret);
                    DPRINTF("error code=0x%0x\n\r", ret);
                		break;
                	}
                }
                if ((ret = XFlash_Erase(&devinfo, get_host_param (ADDR_PARAM),
                		                get_host_param (NBYTES_PARAM)))) {
                    put_writer_status (STATUS_ERR);
                    put_writer_param  (0, ret);
                    DPRINTF("error code=0x%0x\n\r", ret);
                    break;
//                	return ret;
                }
                put_writer_status (STATUS_SUCCESS);
                break;
//#endif  /* FW_CFG_ERASE */

//#ifdef FW_CFG_PROG
            case CMD_PROG_DEV:
                put_writer_status (STATUS_BUSY);
                offset  = get_host_param (PROG_OFFSET_PARAM);
                nbytes  = get_host_param (DATA_BYTES_COUNT_PARAM);
//                offset  = FLASH_PROG_OFFSET;
//                nbytes  = 256;
                int i;
//                for (i = 0; i < nbytes; i++) {
//                	membuf[i] = i;
//                }
                DPRINTF ("Flashwriter: CMD_PROG_DEV - offset: 0x%x, size: %d ....", offset, nbytes);
#if 0
                if ((ret = tg_prog_dev (offset, (uint8_t*)membuf, nbytes)) == 0) {
//                    put_writer_status (STATUS_SUCCESS);
                } else {
//                    put_writer_status (STATUS_ERR);
//                    put_writer_param (ERR_CODE_PARAM, ret);
                	return ret;
                }
#endif
                if ((ret = XFlash_Write(&devinfo, offset, nbytes, (void *)membuf))) {
                    put_writer_status (STATUS_ERR);
                    put_writer_param (ERR_CODE_PARAM, ret);
                    DPRINTF("error code=0x%0x\n\r", ret);
                    break;
//                	return ret;
                }

                for (i = 0; i < nbytes; i++) {
                	readbuf[i] = 0;
                }
                if ((ret = XFlash_Read(&devinfo, offset, nbytes, (void *)readbuf))) {
                    put_writer_status (STATUS_ERR);
                    put_writer_param (ERR_CODE_PARAM, ret);
                    DPRINTF("read failed: error code=0x%0x\n\r", ret);
                    break;
//                	return ret;
                }

                /*
                 * Compare the data read against the data Written.
                 */
                for(i = 0; i < nbytes; i++) {
                    //DPRINTF("0x%02x-0x%02x  ", membuf[i], readbuf[i]);
                	if(membuf[i] != readbuf[i]) {
                		put_writer_status (STATUS_ERR);
                		put_writer_param (ERR_CODE_PARAM, ret);
                    DPRINTF("verify failed: error code=0x%0x\n\r", ret);
                		break;
                		//			return -1;
                	}
                }
                put_writer_status (STATUS_SUCCESS);
                break;
            case CMD_BATCH:
                put_writer_status (STATUS_BUSY);
                offset  = get_host_param (PROG_OFFSET_PARAM);
                nbytes  = get_host_param (DATA_BYTES_COUNT_PARAM);
                srcaddr = get_host_param (SRCADDR_PARAM);
                DPRINTF("Flashwriter: CMD_BATCH_DEV - offset: 0x%x, size: %d, src: 0x%x ....",
                         offset, nbytes, srcaddr);
//                if ((ret = tg_prog_dev (offset, (uint8_t*)srcaddr, nbytes)) == 0)
                if ((ret = XFlash_Write(&devinfo, offset, nbytes, (void *)srcaddr))) {
                    put_writer_status (STATUS_ERR);
                    put_writer_param (ERR_CODE_PARAM, ret);
                    DPRINTF("error code=0x%0x\n\r", ret);
                } else {
                    put_writer_status (STATUS_SUCCESS);
                }
                break;
//#endif  /* FW_CFG_PROG   */
//#endif  /* FW_CFG_DEV_OPERATE */

            case CMD_EXIT:
                DPRINTF("Flashwriter: CMD_EXIT...Done\r\n");
                put_writer_status (STATUS_EXIT);
            case CMD_NONE:
                put_writer_status (STATUS_IDLE);
                break;
        }
    
        clr_host_cmd ();
        status = get_writer_status ();
        if (status == STATUS_ERR) {
            DPRINTF("\r\nFlashwriter: Encountered error -- ");
            DPRINTF("%s\r\n", errmap [get_writer_param (0)]);
        }
        else
            //DPRINTF("Done !\r\n");

        xmd_callback ();
        clr_writer_cmd ();
        clr_writer_status ();
    }
}


/* 
 *  main -- Entry point
 */

int main ()
{
    DPRINTF("Flashwriter: Starting...");
    clr_writer_cmd ();
    clr_writer_status ();
    flashwriter_sm ();							/* Start writer state machine */
    DPRINTF("Flashwriter: Terminating !\r\n");
}


#ifdef __PPC__
#include <unistd.h>
/* Save some code and data space on PowerPC 
   by defining a minimal exit */
void exit (int ret)
{
    _exit (ret);
}
#endif
