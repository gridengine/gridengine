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
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

#include "sge_getpwnam.h"
#include "sge_max_nis_retries.h"

struct passwd *sge_getpwnam(
char *name 
) {
#ifndef WIN32 /* var not needed */
   int i = MAX_NIS_RETRIES;      
#endif
   struct passwd *pw;
   
   pw = NULL;

#ifndef WIN32 /* getpwnam not called */

   while (i-- && !pw)
      pw = getpwnam(name);

#else
   {
      char *pcHome;
      char *pcEnvHomeDrive;
      char *pcEnvHomePath;

      pcEnvHomeDrive = getenv("HOMEDRIVE");
      pcEnvHomePath  = getenv("HOMEPATH");

      if (!pcEnvHomeDrive || !pcEnvHomePath) {
         return pw;
      }
      pcHome = malloc(strlen(pcEnvHomeDrive) + strlen(pcEnvHomePath) + 1);
      if (!pcHome) {
         return NULL;
      }
      strcpy(pcHome, pcEnvHomeDrive);
      strcat(pcHome, pcEnvHomePath);
      
      pw = malloc(sizeof(struct passwd));
      if (!pw) {
         return NULL;
      }
      memset(pw, 0, sizeof(sizeof(struct passwd)));
      pw->pw_dir = pcHome;
      
   }
#endif

   /* sometime on failure struct is non NULL but name is empty */
   if (pw && !pw->pw_name)
      pw = NULL;
      
   return pw;
}
