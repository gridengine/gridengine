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
#include <stdlib.h>
#include <string.h>
#include "basis_types.h"
#include "sge_bitop.h"
#include "msg_utilib.h"

/****** uti/bitop/sge_area_cshift() *******************************************
*  NAME
*     sge_area_cshift() -- Shift area n bits left or right 
*
*  SYNOPSIS
*     int sge_area_cshift(char *area, int area_len, 
*                         int n, int direction) 
*
*  FUNCTION
*     Shift area n bits left or right 
*
*  INPUTS
*     char *area    - area 
*     int area_len  - length of area 
*     int n         - number of bits 
*     int direction - shift direction (SHIFT_RIGHT of SHIFT_LEFT)
*
*  NOTES
*     MT-NOTE: sge_area_cshift() is MT safe
*
*  RESULT
*     int - error state
*         0 - OK
*         1 - error (malloc failed) 
******************************************************************************/
int sge_area_cshift(char *area, int area_len, int n, int direction) 
{
   char lmask, rmask, *cp;
   int i, k, k_next, first_byte, shifts_in_byte, to_lower;
   unsigned short two_byte;

   /* compute number of bit shifts w/o roundtrips */
   n %= area_len*8;

   /* first byte for left shift; correction in case of right shift */
   first_byte = n / 8;
   if (direction == SHIFT_RIGHT)
      first_byte = abs(first_byte - area_len + 1);

   /* 
    * determine masks for 2 bytes in the source byte array to
    * correspond to 1 byte in the destination byte array.
    * also compute shifts necessary to move two byte mask to lower byte.
    */
   shifts_in_byte = n % 8;
   if (direction == SHIFT_RIGHT) {
      two_byte = 0x00ff << shifts_in_byte;
      to_lower = shifts_in_byte;
   } else {
      two_byte = 0xff00 >> shifts_in_byte;
      to_lower = 8 - shifts_in_byte;
   }
   lmask = (two_byte & 0xff00) >> 8;
   rmask = two_byte & 0x00ff;

   /* we ne a copy of source byte array to prevent overwritting */
   if (!(cp = (char *) malloc(area_len))) {
      fprintf(stderr, MSG_MEMORY_NOMEMORYFORBYTEARRAY_S , "cshift");
      return 1;
   }
   memcpy(cp, area, area_len);

   /* copy from source to dest according to shift requirements */
   k = first_byte;
   for(i=0; i<area_len; i++) {
      /* wrap around. */
      if ((k_next=k+1) >= area_len)
         k_next = abs(k - area_len + 1);
      two_byte = 0xff00 & ((cp[k] & lmask) << 8);
      two_byte |= 0x00ff & (cp[k_next] & rmask);
      *area++ = two_byte >> to_lower;
      k = k_next;
   }

   free(cp);
   return 0;
}

/****** uti/bitop/sge_area_print() ********************************************
*  NAME
*     sge_area_print() -- Print area into file 
*
*  SYNOPSIS
*     int sge_area_print(const char *fname, const char *label, 
*                        const char *area, int area_len) 
*
*  FUNCTION
*     Print 'area' with lenght 'area_len' into the file 'fname' after
*     the prefix 'label'. 
*
*  INPUTS
*     const char *fname - filename 
*     const char *label - prefix 
*     const char *area  - area 
*     int area_len      - length of area 
*
*  NOTES
*     MT-NOTE: sge_area_print() is MT safe
*
*  RESULT
*     int - error state
*         0 - OK
*         1 - Error
******************************************************************************/
int sge_area_print(const char *fname, const char *label, 
                   const char *area, int area_len) 
{
   int i;
   FILE *fp;

   if (!fname) {
      fp = stdout;
   } else {
      fp = fopen(fname,"w");
      if (!fp) {
         fprintf(stderr, MSG_FILE_NOOPENFORWRITEING_SS, "print_area", fname);
         return 1;
      }
   }
   fprintf(fp,"%s", label);
   for(i=0; i < area_len; i++) {
      fprintf(fp,"%2.2x", 0xff & area[i]);
   }
   fprintf(fp,"\n");
   if (fname) {
      fclose(fp);
   }

   return 0;
}

/****** uti/bitop/sge_area_xor() **********************************************
*  NAME
*     sge_area_xor() -- 
*
*  SYNOPSIS
*     void sge_area_xor(char *area, int area_len, 
*                       const char *key, int key_len) 
*
*  FUNCTION
*     ??? 
*
*  NOTES
*     MT-NOTE: sge_area_xor() is MT safe
*
*  INPUTS
*     char *area      - area 
*     int area_len    - length of area 
*     const char *key - 
*     int key_len     - 
******************************************************************************/
void sge_area_xor(char *area, int area_len, const char *key, int key_len) 
{
   int i, k, n, len, rest;

   if (area_len < key_len) {
      len = area_len;
      n = 1;
      rest = 0;
   } else {
      len = key_len;
      n = area_len / key_len;
      rest = area_len % key_len;
   }

   for(i = 0; i < n; i++) {
      for(k = 0; k < len; k++) {
         area[i*len+k] ^= key[k];
      }
   }
   for(k=0; k < rest; k++) {
      area[i*len+k] ^= key[k];
   }

   return;
}

/****** uti/bitop/sge_area_cmp() **********************************************
*  NAME
*     sge_area_cmp() -- Compare two byte areas 
*
*  SYNOPSIS
*     int sge_area_cmp(const char *a1, const char *a2, int size) 
*
*  FUNCTION
*     Compares two byte (char) areas and returns the number of bytes 
*     starting from the beginning, which both areas share.
*
*  INPUTS
*     const char *a1 - 1st byte area 
*     const char *a2 - 2nd byte area  
*     int size       - number of bytes to be compared 
*
*  NOTES
*     MT-NOTE: sge_area_cmp() is MT safe
*
*  RESULT
*     int - If the return value is equal to 'size', both areas are
*           identical in 'size' bytes.
******************************************************************************/
int sge_area_cmp(const char *a1, const char *a2, int size) 
{
   int i;

   for (i=0; i<size && a1[i]==a2[i]; i++) {
      ;
   }
   
   return (i);
}
