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

#include "sgermon.h"
#include "rmon.h"
#include "sge_log.h"
#include "sge.h"
#include "sge_arch.h"
#include "msg_utilib.h"
#include "sge_stat.h"
#include "sgermon.h"
#include "msg_common.h"
#include "msg_commd.h"
#include "sge_exit.h"

/****** libs/uti/sge_get_arch() ************************************************
*  NAME
*     sge_get_arch() -- Return the architecture of the appl. using this func.
*
*  SYNOPSIS
*     const char* sge_get_arch() 
*
*  FUNCTION
*     This function returns the architecture of the application which called 
*     this function.
*
*  RESULT
*     const char* - architecture string
*******************************************************************************/
const char *sge_get_arch()
{

#if defined(AIX42)
#   define ARCHBIN "aix42"
#elif defined(AIX43)
#   define ARCHBIN "aix43"
#elif defined(AIX51)
#   define ARCHBIN "aix51"
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
#elif defined(HP1164)
#   define ARCHBIN "hp11-64"
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
#   define ARCHBIN "sx"   
#elif defined(WIN32)
#   define ARCHBIN "m$win"   
#else
#   pragma "Define an architecture for SGE"
#endif

   return ARCHBIN;
}

/****** lib/uti/sge_get_root_dir() *********************************************
*  NAME
*     sge_get_root_dir() -- Returns the installation directory of SGE/SGEEE 
*
*  SYNOPSIS
*     const char* sge_get_root_dir(int do_exit) 
*
*  FUNCTION
*     This function returns the installation directory of SGE/SGEEE.
*     This directory is defined by the SGE_ROOT environment variable 
*     of the calling process. 
*     If the environment variable does not exist or is not set then
*     this function will handle this as error and return NULL (do_exit = 0).
*     If do_exit is 1 an an error occures, the function will log
*     an appropriate message and terminat the calling application.
*
*  INPUTS
*     int do_exit - Terminate the application in case of an error
*     char *buffer - buffer to be used for error message
*     size_t size - size of buffer
*
*  RESULT
*     const char* - Root directory of the SGE/SGEEE installation
*
*  NOTES
*     For compatibility reason this function accepts following
*     env variables:
*
*        SGE_ROOT
*        CODINE_ROOT
*        GRD_ROOT 
*
*     Multiple environment variables will only be accepted when they are
*     identical. Other cases will be handled as error.
*******************************************************************************/
const char *sge_get_root_dir(int do_exit, char *buffer, size_t size)
{
   char *sge_root, *codine_root, *grd_root;
   char *s;
   int error_number = 0;

   DENTER(TOP_LAYER, "sge_get_root_dir");

   /*
    * Read some env variables
    */
   codine_root = getenv("CODINE_ROOT");
   grd_root = getenv("GRD_ROOT");
   sge_root = getenv("SGE_ROOT");

   /*
    * Check the env variables
    */
   if (sge_root && grd_root && codine_root) {
      if (strcmp(sge_root, grd_root)) {
         error_number = 1;
         goto error;
      } else if (strcmp(sge_root, codine_root)) {
         error_number = 2;
         goto error;
      }
      s = sge_root;
   } else if (sge_root && grd_root) {
      if (strcmp(sge_root, grd_root)) {
         error_number = 1;
         goto error;
      }
      s = sge_root;
   } else if (sge_root && codine_root) {
      if (strcmp(sge_root, codine_root)) {
         error_number = 2;
         goto error;
      }
      s = sge_root;
   } else if (grd_root && codine_root) {
      if (strcmp(grd_root, codine_root)) {
         error_number = 3;
         goto error;
      }
      s = grd_root;
   } else if (sge_root) {
      s = sge_root;
   } else if (grd_root) {
      s = grd_root;
   } else if (codine_root) {
      s = codine_root;
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
   switch(error_number) {
      case 1:
         CRITICAL((SGE_EVENT, MSG_SGEGRDROOTNOTEQUIV));
         break;
      case 2:
         CRITICAL((SGE_EVENT, MSG_SGECODINEROOTNOTEQUIV));
         break;
      case 3:
         CRITICAL((SGE_EVENT, MSG_GRDCODINEROOTNOTEQUIV));
         break;
      case 4:
         CRITICAL((SGE_EVENT, MSG_SGEROOTNOTSET));
         break;
      default:
         CRITICAL((SGE_EVENT, MSG_UNKNOWNERRORINSGEROOT));
         break;
   }
   if (buffer)
      strncpy(buffer, SGE_EVENT, size);

   DEXIT;
   if (do_exit) {
      SGE_EXIT(1);   
   }
   return NULL;
}

/****** lib/uti/sge_get_default_cell() *****************************************
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
*     For compatibility reason this function accepts following
*     env variables:
*
*        SGE_CELL
*        COD_CELL
*        GRD_CELL
*
*     Multiple environment variables will only be accepted when they are
*     identical. Other cases will be handled as error. In case of an error
*     the 'DEFAULT_CELL' will be returned.
******************************************************************************/
const char *sge_get_default_cell(void)
{
   char *cod_cell, *grd_cell, *sge_cell;
   char *s;

   DENTER(TOP_LAYER, "sge_get_default_cell");
   /*
    * Read some env variables
    */
   cod_cell = getenv("COD_CELL");
   grd_cell = getenv("GRD_CELL");
   sge_cell = getenv("SGE_CELL");

   /*
    * Check the env variables
    */
   if (sge_cell && grd_cell && cod_cell) {
      if (strcmp(sge_cell, grd_cell)) {
         s = NULL;
      } else if (strcmp(sge_cell, cod_cell)) {
         s = NULL;
      }
      s = sge_cell;
   } else if (sge_cell && grd_cell) {
      if (strcmp(sge_cell, grd_cell)) {
         s = NULL;
      }
      s = sge_cell;
   } else if (sge_cell && cod_cell) {
      if (strcmp(sge_cell, cod_cell)) {
         s = NULL;
      }
      s = sge_cell;
   } else if (grd_cell && cod_cell) {
      if (strcmp(grd_cell, cod_cell)) {
         s = NULL;
      }
      s = grd_cell;
   } else if (sge_cell) {
      s = sge_cell;
   } else if (grd_cell) {
      s = grd_cell;
   } else if (cod_cell) {
      s = cod_cell;
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

/*-----------------------------------------------------------------------
 * get_alias_path
 *-----------------------------------------------------------------------*/
char *get_alias_path(void) {
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

   DENTER(TOP_LAYER, "get_alias_path");

   sge_root = sge_get_root_dir(1, NULL, 0);
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
