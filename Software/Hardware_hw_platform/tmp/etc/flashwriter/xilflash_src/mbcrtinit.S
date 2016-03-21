###################################-*-asm*- 
# 
# Copyright (c) 2001 Xilinx, Inc.  All rights reserved. 
# 
# Xilinx, Inc. CONFIDENTIAL 
#
# XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A 
# COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
# ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR 
# STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
# IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE 
# FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.  
# XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO 
# THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO 
# ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE 
# FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY 
# AND FITNESS FOR A PARTICULAR PURPOSE.
# 
# mbcrtinit.S 
#
# Second stage of C run-time initialization
# Modified for flashwriter to optimize for size: 
#    Removed calls to language initialization routines    
# 
#######################################

	.globl _crtinit
	.align 2
	.ent _crtinit

_crtinit:
	addi	r1, r1, -20               	/* Save Link register	 */
	sw	r15, r1, r0

    	addi	r6, r0, __sbss_start          	/* clear SBSS */
	addi	r7, r0, __sbss_end	
	rsub	r18, r6, r7		
	blei	r18, .Lendsbss

.Lloopsbss:	
	sw	r0, r6, r0
	addi	r6, r6, 4
	rsub	r18, r6, r7
	bgti	r18, .Lloopsbss
.Lendsbss:

	addi	r6, r0, __bss_start             /* clear BSS */
	addi	r7, r0, __bss_end		
    	rsub	r18, r6, r7		
	blei	r18, .Lendbss
.Lloopbss:	
	sw	r0, r6, r0
	addi	r6, r6, 4
	rsub	r18, r6, r7
	bgti	r18, .Lloopbss
.Lendbss:

	addi	r6, r0, 0                       /* Initialize argc = 1 and argv = NULL and envp = NULL  */
	addi	r7, r0, 0			
      	brlid	r15, main                       /* Execute the program */
	addi	r5, r0, 0			

    	lw	r15, r1, r0                     /* Return back to CRT   */
	rtsd	r15, 8
    	addi	r1, r1, 20
	.end _crtinit

