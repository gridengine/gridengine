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
#include "sgermon.h"
#include "sge_dstring.h"

#define REALLOC_CHUNK   1024
#define BUFFER_SIZE 20000

/* JG: TODO: Introduction uti/dstring/--Dynamic_String is missing */

/****** uti/dstring/sge_dstring_append() **************************************
*  NAME
*     sge_dstring_append() -- strcat() for dstring's 
*
*  SYNOPSIS
*     const char* sge_dstring_append(dstring *sb, const char *a) 
*
*  FUNCTION
*     Append 'a' after 'sb' 
*
*  INPUTS
*     dstring *sb   - dynamic string 
*     const char *a - string 
*
*  RESULT
*     const char* - result string
******************************************************************************/
const char* sge_dstring_append(dstring *sb, const char *a) 
{
   int n, m;

   if (!sb) {
      return NULL;
   }

   if (!a) {
      return NULL;
   }
   
   /* only allow to append a string with length 0
      for memory allocation */
   if (strlen(a) == 0 && sb->s != NULL ) {
      return sb->s;
   }

   n = strlen(a) + 1 ;
   if (sb->s == NULL) {
      m = 1;
   } else {
      m = strlen(sb->s) + 1;
   }  
   if ( (m + n - 1 ) > sb->size ) {
      if (n < REALLOC_CHUNK)
         n = REALLOC_CHUNK; 
      sb->size += n;
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

/****** uti/dstring/sge_dstring_append_dstring() ******************************
*  NAME
*     sge_dstring_append() -- strcat() for dstring's 
*
*  SYNOPSIS
*     const char* sge_dstring_append(dstring *sb, const dstring *a) 
*
*  FUNCTION
*     Append 'a' after 'sb' 
*
*  INPUTS
*     dstring *sb      - dynamic string 
*     const dstring *a - string 
*
*  RESULT
*     const char* - result string
******************************************************************************/
const char* sge_dstring_append_dstring(dstring *sb, const dstring *a) 
{
   return sge_dstring_append(sb, sge_dstring_get_string(a));
}

/****** uti/dstring/sge_dstring_sprintf() *************************************
*  NAME
*     sge_dstring_sprintf() -- sprintf() for dstring's 
*
*  SYNOPSIS
*     const char* sge_dstring_sprintf(dstring *sb, 
*                                     const char *format, ...) 
*
*  FUNCTION
*     see sprintf() 
*
*  INPUTS
*     dstring *sb        - dynamic string 
*     const char *format - format string 
*     ...                - additional parameters 
*
*  RESULT
*     const char* - result string 
*
*  NOTES
*     JG: TODO (265): Do not use a fixed size buffer and vprintf!
*                     This undoes the benefits of a dynamic string
*                     implementation.
*                     Either use a vsnprintf implementation (if 
*                     available for all platforms) or find other means 
*                     to prevent buffer overflows.
******************************************************************************/
const char* sge_dstring_sprintf(dstring *sb, const char *format, ...)
{
   char buf[BUFFER_SIZE];
   va_list ap;

   va_start(ap, format);
   if (!format) {
      return sb ? sb->s : NULL;
   }
   vsprintf(buf, format, ap);
   return sge_dstring_copy_string(sb, buf);
}

/****** uti/dstring/sge_dstring_sprintf_append() ******************************
*  NAME
*     sge_dstring_sprintf_append() -- sprintf() and append for dstring's 
*
*  SYNOPSIS
*     const char* sge_dstring_sprintf_append(dstring *sb, 
*                                            const char *format, ...) 
*
*  FUNCTION
*     See sprintf() 
*     The string created by sprintf is appended already existing 
*     contents of the dstring.
*
*  INPUTS
*     dstring *sb        - dynamic string 
*     const char *format - format string 
*     ...                - additional parameters 
*
*  RESULT
*     const char* - result string 
*
*  NOTES
*     JG: TODO (265): Do not use a fixed size buffer and vprintf!
*                     This undoes the benefits of a dynamic string
*                     implementation.
*                     Either use a vsnprintf implementation (if 
*                     available for all platforms) or find other 
*                     means to prevent buffer overflows.
******************************************************************************/
const char* sge_dstring_sprintf_append(dstring *sb, const char *format, ...)
{
   char buf[BUFFER_SIZE];
   va_list ap;

   va_start(ap, format);
   if (!format) {
      return sb ? sb->s : NULL;
   }
   vsprintf(buf, format, ap);
   return sge_dstring_append(sb, buf);
}

/****** uti/dstring/sge_dstring_copy_string() *********************************
*  NAME
*     sge_dstring_copy_string() -- copy string into dstring 
*
*  SYNOPSIS
*     const char* sge_dstring_copy_string(dstring *sb, char* str) 
*
*  FUNCTION
*     Copy string into dstring 
*
*  INPUTS
*     dstring *sb - destination dstring 
*     char* str   - source string 
*
*  RESULT
*     const char* - result string 
*******************************************************************************/
const char *sge_dstring_copy_string(dstring *sb, const char *str) 
{
   const char *ret = NULL;

   DENTER(TOP_LAYER, "sge_dstring_copy_string");
   if (sb != NULL && sb->s != NULL) {
      sb->s[0] = 0;
   }  
   if (sb != NULL && sb->s == NULL) {
      sge_dstring_append(sb, "");
   }

   ret = sge_dstring_append(sb, str);
   DEXIT;
   return ret;
}

/****** uti/dstring/sge_dstring_copy_dstring() ********************************
*  NAME
*     sge_dstring_copy_dstring() -- strcpy() for dstrings's 
*
*  SYNOPSIS
*     const char* sge_dstring_copy_dstring(dstring *sb1, 
*                                          const dstring *sb2) 
*
*  FUNCTION
*     strcpy() for dstrings's 
*
*  INPUTS
*     dstring *sb1 - destination dstring
*     const dstring *sb2 - source dstring 
*
*  RESULT
*     const char* - result string buffer 
*******************************************************************************/
const char *sge_dstring_copy_dstring(dstring *sb1, const dstring *sb2) 
{
   const char *ret = NULL;
   DENTER(TOP_LAYER, "sge_dstring_copy_dstring");

   if (sb1 != NULL && sb1->s != NULL) {
      sb1->s[0] = 0;
   }  
   if (sb1 != NULL && sb1->s == NULL) {
      sge_dstring_append(sb1, "");
   }

   ret = sge_dstring_append(sb1, sge_dstring_get_string(sb2));
   DEXIT;
   return ret;
}

/****** uti/dstring/sge_dstring_free() ****************************************
*  NAME
*     sge_dstring_free() -- free() for dstring's 
*
*  SYNOPSIS
*     void sge_dstring_free(dstring *sb) 
*
*  FUNCTION
*     Frees a dynamically allocated string 
*
*  INPUTS
*     dstring *sb - dynamic string 
******************************************************************************/
void sge_dstring_free(dstring *sb) 
{
   if (sb && sb->s) {
      free(sb->s);
      sb->s = NULL;
      sb->size = 0;
   }
}   

/****** uti/dstring/sge_dstring_clear() ****************************************
*  NAME
*     sge_dstring_clear() -- empty a dstring
*
*  SYNOPSIS
*     void sge_dstring_clear(dstring *sb) 
*
*  FUNCTION
*     Set a dstring to an empty string.
*
*  INPUTS
*     dstring *sb - dynamic string 
******************************************************************************/
void sge_dstring_clear(dstring *sb) 
{
   if (sb && sb->s) {
      sb->s[0] = 0;
   }
}   

/****** uti/dstring/sge_dstring_get_string() **********************************
*  NAME
*     sge_dstring_get_string() -- Returns string buffer 
*
*  SYNOPSIS
*     const char* sge_dstring_get_string(const dstring *string) 
*
*  FUNCTION
*     Returns a pointer to the buffer where the string is stored.
*     The pointer is not valid until doomsday. The next
*     sge_dstring_* call may make it invalid.
*
*  INPUTS
*     const dstring *string - pointer to dynamic string 
*
*  RESULT
*     const char* - pointer to string buffer
*******************************************************************************/
const char *sge_dstring_get_string(const dstring *string)
{
   return (string != NULL) ? string->s : NULL;
}


/****** uti/dstring/sge_dstring_strlen() **************************************
*  NAME
*     sge_dstring_strlen() -- strlen() for dstring's 
*
*  SYNOPSIS
*     size_t sge_dstring_strlen(const dstring *string) 
*
*  FUNCTION
*     strlen() for dstring's 
*
*  INPUTS
*     const dstring *string - pointer to dynamic string 
*
*  RESULT
*     size_t - string length
*******************************************************************************/
size_t sge_dstring_strlen(const dstring *string)
{
   size_t len = 0;

   DENTER(TOP_LAYER,"sge_dstring_strlen");
   if (string != NULL && string->s != NULL) {
      len = strlen(string->s);
   }
   DEXIT;
   return len;
}

#if 0 /* EB: debug */
int main(void)
{
   char *s;
   dstring sb = DSTRING_INIT;    /* initialize */

   /*
    * change content
    */
   s = sge_dstring_append(&sb, "Trala");
   s = sge_dstring_append(&sb, " trolo");
   s = sge_dstring_append(&sb, " troet");
   s = sge_dstring_sprintf(&sb, "%d, %s, %f\n", 5, "rabarber ", 5.6);

   /*
    * use string
    */
   printf("%s\n", s);
   printf("%s\n", sge_dstring_get_string(&sb));

   /*
    * free the string when no longer needed
    */
   sge_dstring_free(&sb);
   return 0;
}
#endif
