#ifndef __SGE_BITFIELD_H
#define __SGE_BITFIELD_H
/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2001 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#include <stdio.h>

#include "basis_types.h"

/****** uti/bitfield/BIT_MANIPULATION_MAKROS() ********************************
*  NAME
*     ISSET(),VALID(),SETBIT(),CLEARBIT() - Bit manipulation makros 
*
*  SYNOPSIS
*     #define ISSET(a,b)      ((a&b)==b)
*     #define VALID(a,b)      ((a|b)==b)
*     #define SETBIT(a,b)     (b=(a)|b);
*     #define CLEARBIT(a,b)   (b &= (~(a)));
*
*  FUNCTION
*     Makros to get/set/clear bits in native variables. 
*
*  INPUTS
*     int,long,u_long32... a - Bitmask
*     int,long,u_long32... b - Variable 
*
*  RESULT
*     b will be modified
*
*  NOTE
*     These Makros can't be used in combination with the bitfield type.
*******************************************************************************/
#define ISSET(a,b)      ((a&b)==b)
#define VALID(a,b)      ((a|b)==b)
#define SETBIT(a,b)     (b=(a)|b);
#define CLEARBIT(a,b)   (b &= (~(a)));

typedef struct {
   int size;
   char *bf;
} _bitfield;

typedef _bitfield *bitfield;

bitfield sge_bitfield_new(int size);
bitfield sge_bitfield_free(bitfield bf);
bool sge_bitfield_copy(bitfield source, bitfield target);

int sge_bitfield_set(bitfield bf, int bit);
int sge_bitfield_get(bitfield bf, int bit);
int sge_bitfield_clear(bitfield bf, int bit);
bool sge_bitfield_reset(bitfield source);
bool sge_bitfield_changed(bitfield source);

void sge_bitfield_print(bitfield bf, FILE *fd); 

#endif /* __SGE_BITFIELD_H */
