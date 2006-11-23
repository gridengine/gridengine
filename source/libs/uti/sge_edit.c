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
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "sge_prog.h"
#include "sge.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_signal.h"
#include "msg_common.h"
#include "sge_edit.h"
#include "sge_unistd.h"

int sge_edit(const char *fname, uid_t myuid, gid_t mygid)
{
   SGE_STRUCT_STAT before, after;
   pid_t pid;
   int status;
   int ws = 0;

   DENTER(TOP_LAYER, "sge_edit");;

   if (fname == NULL) {
      ERROR((SGE_EVENT, MSG_NULLPOINTER));
      return -1;
   }

   if (SGE_STAT(fname, &before)) {
      ERROR((SGE_EVENT, MSG_FILE_EDITFILEXDOESNOTEXIST_S, fname));
      DEXIT;
      return -1;
   }

   chown(fname, myuid, mygid);

   pid = fork();
   if (pid) {
      while (ws != pid) {
         ws = waitpid(pid, &status, 0);
         if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != 0) {
               ERROR((SGE_EVENT, MSG_QCONF_EDITOREXITEDWITHERROR_I,
                      (int) WEXITSTATUS(status)));
               DEXIT;
               return -1;
            }
            else {
               if (SGE_STAT(fname, &after)) {
                  ERROR((SGE_EVENT, MSG_QCONF_EDITFILEXNOLONGEREXISTS_S, fname));
                  DEXIT;
                  return -1;
               }
               if ((before.st_mtime != after.st_mtime) || 
                    (before.st_size != after.st_size)) { 
                  DEXIT;
                  return 0;
               }
               else {
                  /* file is unchanged; inform caller */
                  DEXIT;
                  return 1;
               }
            }
         }
#ifndef WIN32  /* signals b18 */
         if (WIFSIGNALED(status)) {
            ERROR((SGE_EVENT, MSG_QCONF_EDITORWASTERMINATEDBYSIGX_I,
                   (int) WTERMSIG(status)));
            DEXIT;
            return -1;
         }
#endif
      }
   } else {
      const char *cp = NULL;

      sge_set_def_sig_mask(NULL, NULL);
      sge_unblock_all_signals();
      setuid(getuid());
      setgid(getgid());

      cp = sge_getenv("EDITOR");
      if (!cp || strlen(cp) == 0)
         cp = DEFAULT_EDITOR;
         
      execlp(cp, cp, fname, (char *) 0);
      ERROR((SGE_EVENT, MSG_QCONF_CANTSTARTEDITORX_S, cp));
      SGE_EXIT(NULL, 1);
   }

   DEXIT;
   return (-1);
}

