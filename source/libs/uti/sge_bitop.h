#ifndef __SGE_BITOP_H
#define __SGE_BITOP_H
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

int cshift(char *area, int area_len, int n, int direction);

int print_area(const char *fname, const char *label, const char *area, 
               int area_len);

void xor_area(char *area, int area_len, const char *key, int key_len);

int areacmp(const char *a1, const char *a2, int size);

#define SHIFT_RIGHT -1
#define SHIFT_LEFT   1

/* read 'bits' least significant bits from byte 'i' of u_long32 'u' */
/* (((u) & ((u_long32) (~((unsigned char) 0xff<<((bits))))<<(8*(i))))>>(8*(i)))
*/
#define READBYTE(u,i,bits) \
   (((u) >> (8*(i))) & ((u_long32) (~((unsigned char) 0xff<<(bits)))))

/* set 'bits' least significant bits of byte 'i' of an u_long32 with 'c' */
#define SETBYTE(c,i,bits) \
 (((u_long32) ((unsigned char) (c)) << (8*(i))) >> (8-(bits)))

/* extract upper/lower byte from 2-byte word */
#define HIBYTE(a)  ((unsigned char) (((a) >> 8) & 0xff))
#define LOBYTE(a)  ((unsigned char)  ((a)       & 0xff))

/* set upper/lower byte of 2-byte word with byte 'a' */
#define SETHI(a)   (0xff00 & ((unsigned short) (a) << 8))
#define SETLO(a)   (0x00ff & ((unsigned short) (a)))

#endif /* __SGE_BITOP_H */

