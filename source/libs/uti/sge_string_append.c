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
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "sge_string_append.h"

#define REALLOC_CHUNK   1024

char* sge_string_append(StringBufferT *sb, const char *a) {
   int n, m;

   if (!sb)
      return NULL;
   
   if (!a || *a == '\0')
      return sb->s;

   n = strlen(a);
   m = sb->s ? strlen(sb->s) : 0;
   if (m + n > sb->size) {
      if (n < REALLOC_CHUNK)
         n = REALLOC_CHUNK; 
      sb->size += (n+1);
      if (sb->s)
         sb->s = realloc(sb->s, sb->size * sizeof(char)); 
      else {
         sb->s = malloc(sb->size * sizeof(char));
         sb->s[0] = '\0';
      }
   }   
   
   strcat(sb->s, a);
   return sb->s;
}

char* sge_string_printf(StringBufferT *sb, const char *format,...)
{
   char buf[BUFSIZ];
   va_list ap;

   va_start(ap, format);
   if (!format) {
      return sb ? sb->s : NULL;
   }
   vsprintf(buf, format, ap);
   return sge_string_append(sb, buf);
}

void sge_string_free(
StringBufferT *sb 
) {
   if (sb && sb->s) {
      free(sb->s);
      sb->s = NULL;
      sb->size = 0;
   }
}   

#if 0
int main(void)
{
   char *s;
   StringBufferT sb = {NULL, 0};

   s = sge_string_append(&sb, "Trala");
   s = sge_string_append(&sb, " trolo");
   s = sge_string_append(&sb, " troet");
   s = sge_string_printf(&sb, "%d, %s, %f\n", 5, "rabarber ", 5.6);
   printf("%s\n", s);
   /*
   ** free the string when no longer needed
   */
   sge_string_free(&sb);
   return 0;
}
#endif
