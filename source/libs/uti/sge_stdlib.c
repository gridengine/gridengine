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
#include "def.h"  
#include "sgermon.h"
#include "sge_log.h" 
#include "msg_utilib.h"

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

char *sge_free(char *cp) 
{
   FREE(cp);
   return NULL;
}   
