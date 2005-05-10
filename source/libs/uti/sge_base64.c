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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sge_base64.h"


unsigned char *
buffer_encode_hex(unsigned char *input, size_t len, unsigned char **output)
{
   size_t s;

   s = len * 2 + 1;
   *output = malloc(s);
   memset(*output, 0, s);

   for (s = 0; s < len; s++) {
      char buffer[32] = "";
      int byte = input[s];

      sprintf(buffer, "%02x", byte);
      strcat((char*) *output, buffer);
   }
   return *output;
}

unsigned char *
buffer_decode_hex(unsigned char *input, size_t *len, unsigned char **output) 
{
   size_t s;

   s = *len / 2 + 1;
   *output = malloc(s);
   memset(*output, 0, s);

   for (s = 0; s < *len; s+=2) {
      char buffer[32] = "";
      int byte = 0;

      buffer[0] = input[s];
      buffer[1] = input[s+1];

      sscanf(buffer, "%02x", &byte);
      (*output)[s/2] = byte;
   }
   *len = *len / 2;
   return *output;
}

