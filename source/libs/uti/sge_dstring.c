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

#include "sge.h"
#include "sgermon.h"
#include "sge_dstring.h"

#define REALLOC_CHUNK   1024
#define BUFFER_SIZE 20000

#define DSTRING_LAYER BASIS_LAYER

/* please add here a defined(<ARCH>) if vsnprintf() family is not supported */
#if 0
#define HAS_NO_VSNPRINTF 
#endif


/* JG: TODO: Introduction uti/dstring/--Dynamic_String is missing */

static void
sge_dstring_allocate(dstring *sb, size_t request)
{  
   /* always request multiples of REALLOC_CHUNK */
   size_t chunks = request / REALLOC_CHUNK + 1;
   request = chunks * REALLOC_CHUNK;

   /* set new size */
   sb->size += request;

   /* allocate memory */
   if (sb->s != NULL) {
      sb->s = realloc(sb->s, sb->size * sizeof(char));
   } else {
      sb->s = malloc(sb->size * sizeof(char));
      sb->s[0] = '\0';
   }
}

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
*  NOTES
*     MT-NOTE: sge_dstring_append() is MT safe
*
*  RESULT
*     const char* - result string
******************************************************************************/
const char* sge_dstring_append(dstring *sb, const char *a) 
{
   size_t len;  /* length of string a */

   DENTER(DSTRING_LAYER, "sge_dstring_append");

   if (sb == NULL || a == NULL) {
      DEXIT;
      return NULL;
   }

   len = strlen(a);
 
   if (sb->is_static) {
      if ((sb->length + len) > sb->size )
         len = sb->size - sb->length;

      strncat(sb->s + sb->length, a, len);
      sb->length += len;
   } else {
      size_t required;

      /* only allow to append a string with length 0
         for memory allocation */
      if (len == 0 && sb->s != NULL ) {
         DEXIT;
         return sb->s;
      }

      required = len + sb->length + 1;

      if (required > sb->size) {
         sge_dstring_allocate(sb, required - sb->size);
      }

      strcat(sb->s + sb->length, a);
      sb->length += len;
   }

   DEXIT;
   return sb->s;
}

const char* sge_dstring_append_char(dstring *sb, const char a)
{
   DENTER(DSTRING_LAYER, "sge_dstring_append_char");

   if (sb == NULL) {
      DEXIT;
      return NULL;
   }

   if (a == '\0') {
      DEXIT;
      return sb->s;
   }
  
   if (sb->is_static) {
      if (sb->length < sb->size ) {
         sb->s[sb->length++] = a;
         sb->s[sb->length] = '\0';
      }
   } else {
      size_t required = sb->length + 1 + 1;

      if (required > sb->size) {
         sge_dstring_allocate(sb, required - sb->size);
      }

      sb->s[sb->length++] = a;
      sb->s[sb->length] = '\0';
   }

   DEXIT;
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
*  NOTES
*     MT-NOTE: sge_dstring_append_dstring() is MT safe
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
*     MT-NOTE: sge_dstring_sprintf() is MT safe
*
*     JG: TODO (265): Do not use a fixed size buffer and vprintf!
*                     This undoes the benefits of a dynamic string
*                     implementation.
******************************************************************************/
const char* sge_dstring_sprintf(dstring *sb, const char *format, ...)
{
   char buf[BUFFER_SIZE];
   va_list ap;

   va_start(ap, format);

   if (sb == NULL) {
      return NULL;
   }

   if (format == NULL) {
      return sb != NULL ? sb->s : NULL;
   }

   if (sb->is_static) {
      /* add here a defined(<ARCH>) if snprintf is not supported */
#if defined(HAS_NO_VSNPRINTF)
      vsprintf(sb->s, format, ap);
#else
      vsnprintf(sb->s, sb->size, format, ap);
#endif
      sb->length = strlen(sb->s);
   } else {
#if defined(HAS_NO_VSNPRINTF)
      vsprintf(buf, format, ap);
#else
      vsnprintf(buf, sizeof(buf)-1, format, ap);
#endif
      sge_dstring_copy_string(sb, buf);
   }

   return sb->s;
}

/****** uti/dstring/sge_dstring_vsprintf() *************************************
*  NAME
*     sge_dstring_vsprintf() -- vsprintf() for dstring's 
*
*  SYNOPSIS
*     const char* sge_dstring_vsprintf(dstring *sb, const char *format,va_list ap)
*
*  FUNCTION
*     see vsprintf() 
*
*  INPUTS
*     dstring *sb        - dynamic string 
*     const char *format - format string 
*     va_list ap         - argument list
*
*  RESULT
*     const char* - result string 
*
*  NOTES
*     MT-NOTE: sge_dstring_vsprintf() is MT safe
*
*     JG: TODO (265): Do not use a fixed size buffer and vprintf!
*                     This undoes the benefits of a dynamic string
*                     implementation.
******************************************************************************/
const char* sge_dstring_vsprintf(dstring *sb, const char *format, va_list ap)
{
   char buf[BUFFER_SIZE];

   if (sb == NULL) {
      return NULL;
   }

   if (format == NULL) {
      return sb != NULL ? sb->s : NULL;
   }
   if (sb->is_static) {
#if defined(HAS_NO_VSNPRINTF)
      vsprintf(sb->s, format, ap);
#else
      vsnprintf(sb->s, sb->size, format, ap);
#endif
      sb->length = strlen(sb->s);
   } else {
#if defined(HAS_NO_VSNPRINTF)
      vsprintf(buf, format, ap);
#else
      vsnprintf(buf, sizeof(buf)-1, format, ap);
#endif
      sge_dstring_copy_string(sb, buf);
   }

   return sb->s;
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
*     MT-NOTE: sge_dstring_sprintf_append() is MT safe
*
*     JG: TODO (265): Do not use a fixed size buffer and vprintf!
*                     This undoes the benefits of a dynamic string
*                     implementation.
******************************************************************************/
const char* sge_dstring_sprintf_append(dstring *sb, const char *format, ...)
{
   char buf[BUFFER_SIZE];
   va_list ap;

   if (sb == NULL) {
      return NULL;
   }

   va_start(ap, format);
   if (format == NULL) {
      return sb != NULL ? sb->s : NULL;
   }

#if defined(HAS_NO_VSNPRINTF)
   vsprintf(buf, format, ap);
#else
   vsnprintf(buf, sizeof(buf)-1, format, ap);
#endif

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
*  NOTES
*     MT-NOTE: sge_dstring_copy_string() is MT safe
*
*  RESULT
*     const char* - result string 
*******************************************************************************/
const char *sge_dstring_copy_string(dstring *sb, const char *str) 
{
   const char *ret = NULL;

   DENTER(DSTRING_LAYER, "sge_dstring_copy_string");

   if (sb != NULL) {
      sge_dstring_clear(sb);
      ret = sge_dstring_append(sb, str);
   }

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
*  NOTES
*     MT-NOTE: sge_dstring_copy_dstring() is MT safe
*
*  RESULT
*     const char* - result string buffer 
*******************************************************************************/
const char *sge_dstring_copy_dstring(dstring *sb1, const dstring *sb2) 
{
   const char *ret = NULL;

   DENTER(DSTRING_LAYER, "sge_dstring_copy_dstring");

   if (sb1 != NULL) {
      sge_dstring_clear(sb1);
      ret = sge_dstring_append(sb1, sge_dstring_get_string(sb2));
   }

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
*  NOTES
*     MT-NOTE: sge_dstring_free() is MT safe
*
*  INPUTS
*     dstring *sb - dynamic string 
******************************************************************************/
void sge_dstring_free(dstring *sb) 
{
   if (sb != NULL && !sb->is_static && sb->s != NULL) {
      free(sb->s);
      sb->s = NULL;
      sb->size = 0;
      sb->length = 0;
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
*  NOTES
*     MT-NOTE: sge_dstring_clear() is MT safe
*
*  INPUTS
*     dstring *sb - dynamic string 
******************************************************************************/
void sge_dstring_clear(dstring *sb) 
{
   if (sb == NULL)
      return;

   if (sb->s != NULL) {
      sb->s[0] = '\0';
   }

   sb->length = 0;
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
*  NOTES
*     MT-NOTE: sge_dstring_get_string() is MT safe
*
*  RESULT
*     const char* - pointer to string buffer
*******************************************************************************/
const char *sge_dstring_get_string(const dstring *sb)
{
   return (sb != NULL) ? sb->s : NULL;
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
*  NOTES
*     MT-NOTE: sge_dstring_strlen() is MT safe
*
*  RESULT
*     size_t - string length
*******************************************************************************/
size_t sge_dstring_strlen(const dstring *sb)
{
   size_t ret = 0;

   if (sb != NULL) {
      ret = sb->length;
   }

   return ret;
}

/****** uti/dstring/sge_dstring_remaining() **************************************
*  NAME
*     sge_dstring_remaining() -- remaining chars in dstring
*
*  SYNOPSIS
*     size_t sge_dstring_remaining(const dstring *string) 
*
*  FUNCTION
*     Returns number of chars remaining in dstrings.
*
*  INPUTS
*     const dstring *string - pointer to dynamic string 
*
*  NOTES
*     MT-NOTE: sge_dstring_remaining() is MT safe
*
*  RESULT
*     size_t - remaining chars
*******************************************************************************/
size_t sge_dstring_remaining(const dstring *sb)
{
   size_t ret = 0;

   if (sb != NULL) {
      if (sb->is_static) {
         ret = sb->size - sb->length;
      } else {
         ret = MAX_ULONG32;
      }
   }

   return ret;
}

/****** uti/dstring/sge_dstring_init() **************************************
*  NAME
*     sge_dstring_init() -- init static dstrings
*
*  SYNOPSIS
*     size_t sge_dstring_init(dstring *string, char *s, size_t size) 
*
*  FUNCTION
*     Initialize dstring with a static buffer.
*
*  INPUTS
*     const dstring *string - pointer to dynamic string 
*
*  NOTES
*     MT-NOTE: sge_dstring_init() is MT safe
*
*  RESULT
*     size_t - remaining chars
*******************************************************************************/
void sge_dstring_init(dstring *sb, char *s, size_t size)
{
   if (sb != NULL && s != NULL) {
      sb->is_static = true;
      sb->length = 0;
      sb->size = size - 1;   /* leave space for trailing 0 */
      sb->s = s;
      sb->s[0] = '\0';
   }
}


#if 0 /* EB: DEBUG: */
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
