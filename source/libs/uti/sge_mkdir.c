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
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "sgermon.h"
#include "sge_me.h"
#include "sge_mkdir.h"
#include "sge_log.h"
#include "sge_exit.h"
#include "msg_utilib.h"
#include "sge_stat.h" 

static int sge_domkdir(const char *, int, int);

int sge_mkdir(
const char *path,
int fmode,
int exit_on_error 
) {
   int i = 0, res=0;
   stringT path_;

   DENTER(TOP_LAYER, "sge_mkdir");
   if (!path) {
      if (exit_on_error) {
         CRITICAL((SGE_EVENT,MSG_VAR_PATHISNULLINSGEMKDIR ));
         DCLOSE;
         SGE_EXIT(1);
      } 
      else {
         ERROR((SGE_EVENT, MSG_VAR_PATHISNULLINSGEMKDIR ));
         DEXIT;
         return -1;
      }
   }   

   memset(path_, 0, sizeof(path_));
   while ((unsigned char) path[i]) {
      path_[i] = path[i];
      if ((path[i] == '/') && (i != 0)) {
         path_[i] = (unsigned char) 0;
         res = sge_domkdir(path_, fmode, exit_on_error);
         if (res) {
            DEXIT;
            return res;
         }
      }
      path_[i] = path[i];
      i++;
   }

   i = sge_domkdir(path_, fmode, exit_on_error);

   DEXIT;
   return i;
}

/*********************************************************/
static int sge_domkdir(
const char *path_,
int fmode,
int exit_on_error 
) {
   SGE_STRUCT_STAT statbuf;

   DENTER(BASIS_LAYER, "sge_domkdir");

   if (mkdir(path_, (mode_t) fmode)) {
      if (errno == EEXIST) {
         DPRINTF(("directory \"%s\" already exists\n", path_));
         DEXIT;
         return 0;
      }

      if (!SGE_STAT(path_, &statbuf) && S_ISDIR(statbuf.st_mode)) {
         DEXIT; /* may be we do not have permission, but directory already exists */
         return 0;
      }

      if (exit_on_error) {
         CRITICAL((SGE_EVENT, MSG_FILE_CREATEDIRFAILED_SS, path_, strerror(errno)));
         SGE_EXIT(1);
      }
      else {
         ERROR((SGE_EVENT, MSG_FILE_CREATEDIRFAILED_SS, path_, strerror(errno)));
         DEXIT;
         return -1;
      }
   }

   DEXIT;
   return 0;
}
