#ifndef __SGE_STDLIB_H
#define __SGE_STDLIB_H
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

/****** uti/stdlib/FREE() *****************************************************
*  NAME
*     FREE() -- replacement for sge_free()
*
*  SYNOPSIS
*     #define FREE(x)
*     void FREE(char *cp) 
*
*  FUNCTION
*     Replacement for sge_free(). Accepts NULL pointers.
*     After a call of this macro "cp" will be NULL.
*
*  INPUTS
*     char *cp - pointer to a memory block 
*
*  RESULT
*     char* - NULL
*
*  SEE ALSO
*     uti/stdlib/sge_free()
******************************************************************************/
#define FREE(x) \
   if (x != NULL) { \
      free((char *)x); \
      x = NULL; \
   }

char *sge_malloc(int size);

void *sge_realloc(void *ptr, int size, int do_abort);

char *sge_free(char *cp);        

const char *sge_getenv(const char *env_str); 
int sge_putenv(const char *var);
int sge_setenv(const char *name, const char *value);
void sge_unsetenv(const char* name);

#endif /* __SGE_STDLIB_H */
