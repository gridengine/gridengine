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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "rmon.h"
#include "sge_log.h"
#include "sge.h"
#include "sge_arch.h"
#include "msg_utilib.h"
#include "sgermon.h"

char *sge_arch()
{

#if defined(AIX42)
#   define ARCHBIN "aix42"
#elif defined(AIX43)
#   define ARCHBIN "aix43"
#elif defined(ALPHA4)
#   define ARCHBIN "osf4"
#elif defined(ALPHA5)
#   define ARCHBIN "tru64"
#elif defined(IRIX6)
#   define ARCHBIN "irix6"
#elif defined(HP10)
#   define ARCHBIN "hp10"
#elif defined(HP11)
#   define ARCHBIN "hp11"
#elif defined(SOLARIS86)
#   define ARCHBIN "solaris86"
#elif defined(SOLARIS64)
#   define ARCHBIN "solaris64"
#elif defined(SOLARIS)
#   define ARCHBIN "solaris"
#elif defined(ALINUX)
#   define ARCHBIN "alinux"
#elif defined(LINUX5)
#   define ARCHBIN "linux"
#elif defined(LINUX6)
#   define ARCHBIN "glinux"
#elif defined(SLINUX)
#   define ARCHBIN "slinux"
#elif defined(CRAY)
# if defined(CRAYTSIEEE)
#   define ARCHBIN "craytsieee"
# elif defined(CRAYTS)
#   define ARCHBIN "crayts"
# else
#   define ARCHBIN "cray"
# endif
#elif defined(NECSX4)
#   define ARCHBIN "necsx4"
#elif defined(NECSX5)
#   define ARCHBIN "necsx5"   
#elif defined(WIN32)
#   define ARCHBIN "m$win"   
#else
#   pragma "Define an architecture for SGE"
#endif

   return ARCHBIN;
}

char *sge_sge_root(void)
{
   char *s;

   DENTER(TOP_LAYER, "sge_sge_root");
   s = getenv("SGE_ROOT");
   if (!s || strlen(s)==0) { 
      CRITICAL((SGE_EVENT, MSG_SGEROOTNOTSET));
      exit(1);                                             
   }
   if (s[strlen(s)-1] == '/')  /* get rid of trailing slash*/
      s[strlen(s)-1] = '\0';
   DEXIT;
   return s;
}

/* get cell name - remove trailing slash */
char *sge_default_cell(void)
{
   char *cp;

   cp = getenv("SGE_CELL");
   if (!cp || strlen(cp) == 0)
      cp = DEFAULT_CELL;
   if (cp[strlen(cp)-1] == '/')
      cp[strlen(cp)-1] = '\0';
   return cp;
}

