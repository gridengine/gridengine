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
 *  License at http://www.gridengine.sunsource.net/license.html
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

int cshift(
char *area,
int area_len,
int n,
int direction 
) {
   char lmask, rmask, *cp;
   int i, k, k_next, first_byte, shifts_in_byte, to_lower;
   unsigned short two_byte;

   /* compute number of bit shifts w/o roundtrips */
   n %= area_len*8;

   /* first byte for left shift; correction in case of right shift */
   first_byte = n / 8;
   if (direction == SHIFT_RIGHT)
      first_byte = abs(first_byte - area_len + 1);

   /* determine masks for 2 bytes in the source byte array to
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


int print_area(
char *fname,
char *label,
char *area,
int area_len 
) {
   int i;
   FILE *fp;

   if (!fname)
      fp = stdout;
   else
      if (!(fp=fopen(fname,"w"))) {
         fprintf(stderr, MSG_FILE_NOOPENFORWRITEING_SS , "print_area" ,
                 fname);
         return 1;
      }

   fprintf(fp,"%s", label);
   for(i=0;i<area_len;i++) fprintf(fp,"%2.2x", 0xff & area[i]);
   fprintf(fp,"\n");

   if (fname) fclose(fp);

   return 0;
}


void xor_area(
char *area,
int area_len,
char *key,
int key_len 
) {
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

   for(i=0; i<n; i++)
      for(k=0; k<len; k++)
         area[i*len+k] ^= key[k];
   for(k=0; k<rest; k++)
      area[i*len+k] ^= key[k];

   return;
}


/*
 * Compares two byte (char) areas and returns the number of bytes starting
 * from the beginning, which both areas share.
 *
 * Parameters:
 *      a1   :    1st byte area
 *      a2   :    2nd byte area
 *      size :    number of bytes to be compared
 *
 * Return value: number of bytes which both areas share.
 *
 * If on exit the return value is equal to size, both areas are identical in
 * size bytes.
 */
int areacmp(
char *a1,
char *a2,
int size 
) {
   int i;

   for (i=0; i<size && a1[i]==a2[i]; i++);
   
   return (i);
}
