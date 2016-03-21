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
#ifndef FS_PORTAB_H
#define FS_PORTAB_H

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;

typedef char   int8_t;
typedef short  int16_t;
typedef int    int32_t;



/* An anonymous union allows the compiler to report typedef errors automatically */
/* Does not work with gcc. Might work only for g++ */

/* static union */
/* { */
/*     char int8_t_incorrect   [sizeof(  int8_t) == 1]; */
/*     char uint8_t_incorrect  [sizeof( uint8_t) == 1]; */
/*     char int16_t_incorrect  [sizeof( int16_t) == 2]; */
/*     char uint16_t_incorrect [sizeof(uint16_t) == 2]; */
/*     char int32_t_incorrect  [sizeof( int32_t) == 4]; */
/*     char uint32_t_incorrect [sizeof(uint32_t) == 4]; */
/* }; */



#endif /* FS_PORTTAB_H */
