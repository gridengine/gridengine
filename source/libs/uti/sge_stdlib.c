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

#include "sge_stdlib.h"
#include "sge_dstring.h"
#include "sgermon.h"
#include "sge_log.h" 
#include "msg_utilib.h"

/****** uti/stdlib/sge_malloc() ***********************************************
*  NAME
*     sge_malloc() -- replacement for malloc() 
*
*  SYNOPSIS
*     char* sge_malloc(int size) 
*
*  FUNCTION
*     Allocates a memory block. Initilizes the block (0). Aborts in case
*     of error. 
*
*  INPUTS
*     int size - size in bytes 
*
*  RESULT
*     char* - pointer to memory block
******************************************************************************/
char *sge_malloc(int size) 
{
   char *cp = NULL;

   DENTER(BASIS_LAYER, "sge_malloc");

   if (!size) {
      DEXIT;
      return (NULL);
   }

   cp = (char *) malloc(size);
   if (!cp) {
      CRITICAL((SGE_EVENT, MSG_MEMORY_MALLOCFAILED));
      DEXIT;
      abort();
   }

   memset(cp, 0, size);

   DEXIT;
   return (cp);
}   

/****** uti/stdlib/sge_realloc() **********************************************
*  NAME
*     sge_realloc() -- replacement for realloc 
*
*  SYNOPSIS
*     char* sge_realloc(char *ptr, int size) 
*
*  FUNCTION
*     Reallocates a memory block. Aborts in case of an error. 
*
*  INPUTS
*     char *ptr - pointer to a memory block 
*     int size  - new size 
*
*  RESULT
*     char* - pointer to the (new) memory block
******************************************************************************/
char *sge_realloc(char *ptr, int size) 
{

   char *cp = NULL;

   DENTER(BASIS_LAYER, "sge_realloc");

   if (!size) {
      if (ptr) {
         FREE(ptr);
      }
      DEXIT;
      return (NULL);
   }

   cp = (char *) realloc(ptr, size);

   if (!cp) {
      CRITICAL((SGE_EVENT, MSG_MEMORY_REALLOCFAILED));
      DEXIT;
      abort();
   }

   DEXIT;
   return (cp);
}             

/****** uti/stdlib/sge_free() *************************************************
*  NAME
*     sge_free() -- replacement for free 
*
*  SYNOPSIS
*     char* sge_free(char *cp) 
*
*  FUNCTION
*     Replacement for free(). Accepts NULL pointers.
*
*  INPUTS
*     char *cp - pointer to a memory block 
*
*  RESULT
*     char* - NULL
******************************************************************************/
char *sge_free(char *cp) 
{
   free(cp);
   return NULL;
}  

/****** uti/stdlib/sge_getenv() ***********************************************
*  NAME
*     sge_getenv() -- get an environment variable 
*
*  SYNOPSIS
*     const char* sge_getenv(const char *env_str) 
*
*  FUNCTION
*     The function searches the environment list for a
*     string that matches the string pointed to by 'env_str'.
*     If the resultstring is longer than MAX_STRING_SIZE
*     the application will be terminated.
*
*  INPUTS
*     const char *env_str - name of env. varibale 
*
*  RESULT
*     const char* - value
*
*  SEE ALSO
*     uti/stdlib/sge_putenv()
*     uti/stdlib/sge_setenv() 
******************************************************************************/
const char *sge_getenv(const char *env_str) 
{
   const char *cp=NULL;
 
   DENTER(BASIS_LAYER, "sge_getenv");
 
   cp = (char *) getenv(env_str);
   if (!cp) {

      DEXIT;
      return (cp);
   }
 
   if (strlen(cp) >= MAX_STRING_SIZE) {
      CRITICAL((SGE_EVENT, MSG_GDI_STRING_LENGTHEXCEEDED_SI, env_str, 
                (int) MAX_STRING_SIZE));
      DCLOSE;
      abort();
   }
 
   DEXIT;
   return (cp);
}    

/****** uti/stdlib/sge_putenv() ***********************************************
*  NAME
*     sge_putenv() -- put an environment variable to environment
*
*  SYNOPSIS
*     static int sge_putenv(const char *var) 
*
*  FUNCTION
*     Duplicates the given environment variable and calls the system call
*     putenv.
*
*  INPUTS
*     const char *var - variable to put in the form <name>=<value>
*
*  RESULT
*     static int - 1 on success, else 0
*
*  SEE ALSO
*     uti/stdlib/sge_setenv() 
*     uti/stdlib/sge_getenv()
*******************************************************************************/
int sge_putenv(const char *var)
{
   char *duplicate;

   if(var == NULL) {
      return 0;
   }

   duplicate = strdup(var);

   if(duplicate == NULL) {
      return 0;
   }

   if(putenv(duplicate) != 0) {
      return 0;
   }

   return 1;
}

/****** uti/stdlib/sge_setenv() ***********************************************
*  NAME
*     sge_setenv() -- Change or add an environment variable 
*
*  SYNOPSIS
*     int sge_setenv(const char *name, const char *value) 
*
*  FUNCTION
*     Change or add an environment variable 
*
*  INPUTS
*     const char *name  - variable name 
*     const char *value - new value 
*
*  RESULT
*     int - error state
*         1 - success
*         0 - error 
*
*  SEE ALSO
*     uti/stdlib/sge_putenv() 
*     uti/stdlib/sge_getenv()
*******************************************************************************/
int sge_setenv(const char *name, const char *value)
{
   int ret = 0;

   if (name != NULL && value != NULL) {
      dstring variable = DSTRING_INIT;

      sge_dstring_sprintf(&variable, "%s=%s", name, value);
      ret = sge_putenv(sge_dstring_get_string(&variable));
      sge_dstring_free(&variable);
   }
   return ret;
}
