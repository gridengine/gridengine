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

#include "sge_stdlib.h"
#include "sgermon.h"
#include "rmon.h"
#include "sge_log.h"
#include "sge.h"
#include "msg_utilib.h"
#include "msg_common.h"
#include "sge_unistd.h"
#include "sge_arch.h"

/****** uti/prog/sge_get_arch() ************************************************
*  NAME
*     sge_get_arch() -- SGE/EE architecture string
*
*  SYNOPSIS
*     const char* sge_get_arch(void) 
*
*  FUNCTION
*     This function returns the SGE/EE architecture string of that
*     host where the application is running which called this 
*     functionon.
*
*  NOTES:
*     MT-NOTE: sge_get_arch() is MT safe
*
*  RESULT
*     const char* - architecture string
******************************************************************************/
const char *sge_get_arch(void)
{
#if defined(AIX43)
#   define ARCHBIN "aix43"
#elif defined(AIX51)
#   define ARCHBIN "aix51"
#elif defined(ALPHA5)
#   define ARCHBIN "tru64"
#elif defined(IRIX65)
#   define ARCHBIN "irix65"
#elif defined(HPUX)
#   if defined(HP10)
#      define ARCHBIN "hp10"
#   elif defined(HP11)  
#      define ARCHBIN "hp11"
#   elif defined(HP1164)
#      define ARCHBIN "hp11-64"
#   endif
#elif defined(SOLARIS86)
#   define ARCHBIN "sol-x86"
#elif defined(SOLARIS64)
#   define ARCHBIN "sol-sparc64"
#elif defined(SOLARIS)
#   define ARCHBIN "sol-sparc"
#elif defined(LINUX)
#   if defined(ALINUX_22)
#      define ARCHBIN "lx22-alpha"
#   elif defined(ALINUX_24)
#      define ARCHBIN "lx24-alpha"
#   elif defined(ALINUX_26)
#      define ARCHBIN "lx26-alpha"
#   elif defined(LINUXAMD64_24)
#      define ARCHBIN "lx24-amd64"
#   elif defined(LINUXAMD64_26)      
#      define ARCHBIN "lx26-amd64"
#   elif defined(LINUXIA64_24)   
#      define ARCHBIN "lx24-ia64" 
#   elif defined(LINUXIA64_26)   
#      define ARCHBIN "lx26-ia64"
#   elif defined(SLINUX_22)   
#      define ARCHBIN "lx22-sparc"
#   elif defined(SLINUX_24)   
#      define ARCHBIN "lx24-sparc"
#   elif defined(SLINUX_26)   
#      define ARCHBIN "lx26-sparc"
#   elif defined(LINUX86_22)
#      define ARCHBIN "lx22-x86"
#   elif defined(LINUX86_24)
#      define ARCHBIN "lx24-x86"
#   elif defined(LINUX86_26)
#      define ARCHBIN "lx26-x86"
#   endif
#elif defined(CRAY)
#   if defined(CRAYTSIEEE)
#      define ARCHBIN "craytsieee"
#   elif defined(CRAYTS)
#      define ARCHBIN "crayts"
#   else
#      define ARCHBIN "cray"
#   endif
#elif defined(NECSX4)
#   define ARCHBIN "necsx4"
#elif defined(NECSX5)
#   define ARCHBIN "sx"   
#elif defined(WIN32)
#   define ARCHBIN "m$win"   
#elif defined(FREEBSD)
#   if defined(FREEBSD_ALPHA)
#      define ARCHBIN "fbsd-alpha"
#   elif defined(FREEBSD_AMD64)
#      define ARCHBIN "fbsd-amd64"
#   elif defined(FREEBSD_I386)
#      define ARCHBIN "fbsd-i386"
#   elif defined(FREEBSD_IA64)
#      define ARCHBIN "fbsd-ia64"
#   elif defined(FREEBSD_PPC)
#      define ARCHBIN "fbsd-ppc"
#   elif defined(FREEBSD_SPARC64)
#      define ARCHBIN "fbsd-sparc64"
#   endif
#elif defined(DARWIN)
#   define ARCHBIN "darwin"   
#else
#   pragma "Define an architecture for SGE"
#endif

   return ARCHBIN;
}

/****** uti/prog/sge_get_root_dir() *******************************************
*  NAME
*     sge_get_root_dir() -- SGE/SGEEE installation directory 
*
*  SYNOPSIS
*     const char* sge_get_root_dir(int do_exit, 
*                                  char *buffer, 
*                                  size_t size,
*                                  int do_error_log ) 
*
*  FUNCTION
*     This function returns the installation directory of SGE/SGEEE.
*     This directory is defined by the SGE_ROOT environment variable 
*     of the calling process. 
*     If the environment variable does not exist or is not set then
*     this function will handle this as error and return NULL 
*     (do_exit = 0). If 'do_exit' is 1 and an error occures, the 
*     function will terminate the  calling application.
*
*  INPUTS
*     int do_exit - Terminate the application in case of an error
*     char *buffer - buffer to be used for error message
*     size_t size - size of buffer
*     int do_error_log - enable/disable error logging
*
*  RESULT
*     const char* - Root directory of the SGE/SGEEE installation
*
*  NOTES
*     MT-NOTE: sge_get_arch() is MT safe
*******************************************************************************/
const char *sge_get_root_dir(int do_exit, char *buffer, size_t size, int do_error_log)
{
   char *sge_root; 
   char *s;
   int error_number = 0;

   DENTER(TOP_LAYER, "sge_get_root_dir");

   /*
    * Read some env variables
    */
   sge_root = getenv("SGE_ROOT");

   /*
    * Check the env variables
    */
   if (sge_root) {
      s = sge_root;
   } else {
      error_number = 4;
      goto error;
   } 
   if (!s || strlen(s)==0) { 
      error_number = 4;
      goto error;
   } else {
      /*
       * Get rid of trailing slash
       */ 
      if (s[strlen(s)-1] == '/') { 
         s[strlen(s)-1] = '\0';
      }
   }
   DEXIT;
   return s;

error:
   if (do_error_log) {
      switch(error_number) {
         case 4:
            if (buffer != NULL) {
               strncpy(buffer, MSG_SGEROOTNOTSET, size);
            }
            else {
               CRITICAL((SGE_EVENT, MSG_SGEROOTNOTSET));
            }
            
            break;
         default:
            if (buffer != NULL) {
               strncpy(buffer, MSG_UNKNOWNERRORINSGEROOT, size);
            }
            else {
               CRITICAL((SGE_EVENT, MSG_UNKNOWNERRORINSGEROOT));
            }
            
            break;
      }
   }

   DEXIT;
   if (do_exit) {
      SGE_EXIT(1);   
   }
   return NULL;
}

/****** uti/prog/sge_get_default_cell() ***************************************
*  NAME
*     sge_get_default_cell() -- get cell name and remove trailing slash 
*
*  SYNOPSIS
*     const char* sge_get_default_cell(void) 
*
*  FUNCTION
*     This function returns the defined cell name of SGE/SGEEE.
*     This directory is defined by the SGE_CELL environment variable
*     of the calling process.
*     If the environment variable does not exist or is not set then
*     this function will return the 'DEFAULT_CELL'.
*
*  RESULT
*     const char* - Cell name of this SGE/SGEEE installation
*
*  NOTES
*     MT-NOTE: sge_get_default_cell() is MT safe
******************************************************************************/
const char *sge_get_default_cell(void)
{
   char *sge_cell;
   char *s;

   DENTER(TOP_LAYER, "sge_get_default_cell");
   /*
    * Read some env variables
    */
   sge_cell = getenv("SGE_CELL");

   /*
    * Check the env variables
    */
   if (sge_cell) {
      s = sge_cell;
   } else {
      s = NULL;
   } 

   /*
    * Use default? 
    */     
   if (!s || strlen(s) == 0) {
      s = DEFAULT_CELL;
   } else {
      /*
       * Get rid of trailing slash
       */    
      if (s[strlen(s)-1] == '/') {
         s[strlen(s)-1] = '\0';
      }
   }
   DEXIT;
   return s;
}

/****** uti/prog/sge_get_alias_path() *****************************************
*  NAME
*     sge_get_alias_path() -- Return the path of the 'alias_file' 
*
*  SYNOPSIS
*     char* sge_get_alias_path(void) 
*
*  FUNCTION
*     Return the path of the 'alias_file' 
*
*  NOTES
*     MT-NOTE: sge_get_alias_path() is MT safe
*
******************************************************************************/
char *sge_get_alias_path(void) 
{
/* JG: suppress READ_DANGLING. sge_root comes from a getenv() call.
 *     this should be handled properly in underlying function, e.g. by
 *     strdupping the value returned by getenv().
 */
#ifdef __INSIGHT__
_Insight_set_option("suppress", "READ_DANGLING");
#endif
   const char *sge_root, *sge_cell;
   char *cp;
   int len;
   SGE_STRUCT_STAT sbuf;

   DENTER(TOP_LAYER, "sge_get_alias_path");

   sge_root = sge_get_root_dir(1, NULL, 0, 1);
   sge_cell = sge_get_default_cell();

   if (SGE_STAT(sge_root, &sbuf)) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_SGEROOTNOTFOUND_S , sge_root));
      SGE_EXIT(1);
   }

   len = strlen(sge_root) + strlen(sge_cell) + strlen(COMMON_DIR) + strlen(ALIAS_FILE) + 5;
   if (!(cp = malloc(len))) {
      CRITICAL((SGE_EVENT, MSG_MEMORY_MALLOCFAILEDFORPATHTOHOSTALIASFILE ));
      SGE_EXIT(1);
   }

   sprintf(cp, "%s/%s/%s/%s", sge_root, sge_cell, COMMON_DIR, ALIAS_FILE);
   DEXIT;
   return cp;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "READ_DANGLING");
#endif
}
