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

#include "sgermon.h"
#include "sge_log.h"

#include "sge_feature.h"

#include "sge_unistd.h"
#include "sge_dstring.h"
#include "sge_spool.h"
#include "sge_uidgid.h"
#include "setup_path.h"
#include "sge_prog.h"

#include "sge_answer.h"

#include "sge_all_listsL.h"
#include "sge_manop.h"

#include "spool/sge_spooling.h"
#include "spool/dynamic/sge_spooling_loader.h"

#include "msg_utilbin.h"

static void usage(const char *argv0)
{
   fprintf(stderr, "%s\n %s %s\n\n", MSG_UTILBIN_USAGE, argv0, 
                                     MSG_SPOOLINIT_COMMANDINTRO0);
   fprintf(stderr, "%s", MSG_SPOOLINIT_COMMANDINTRO1);
   fprintf(stderr, "%s", MSG_SPOOLINIT_COMMANDINTRO2);
   fprintf(stderr, "%s", MSG_SPOOLINIT_COMMANDINTRO3);
   fprintf(stderr, "%s", MSG_SPOOLINIT_COMMANDINTRO4);
   fprintf(stderr, "%s", MSG_SPOOLINIT_COMMANDINTRO5);
   fprintf(stderr, "%s", MSG_SPOOLINIT_COMMANDINTRO6);
   fprintf(stderr, "%s", MSG_SPOOLINIT_COMMANDINTRO7);
   fprintf(stderr, "%s", MSG_SPOOLINIT_COMMANDINTRO8);
   fprintf(stderr, "%s", MSG_SPOOLINIT_COMMANDINTRO9);
}

static int init_framework(const char *shlib, const char *libargs, 
                          bool check_context)
{
   int ret = EXIT_FAILURE;

   lListElem *spooling_context = NULL;
   lList *answer_list = NULL;

   DENTER(TOP_LAYER, "init_framework");

   /* create spooling context */
   spooling_context = spool_create_dynamic_context(&answer_list, shlib, 
                                                   libargs);
   answer_list_output(&answer_list);
   if (spooling_context == NULL) {
      CRITICAL((SGE_EVENT, MSG_SPOOLDEFAULTS_CANNOTCREATECONTEXT));
   } else {
      spool_set_default_context(spooling_context);

      /* initialize spooling context */
      if (!spool_startup_context(&answer_list, spooling_context, 
                                 check_context)) {
         CRITICAL((SGE_EVENT, MSG_SPOOLDEFAULTS_CANNOTSTARTUPCONTEXT));
      } else {
         ret = EXIT_SUCCESS;
      }
      answer_list_output(&answer_list);
   }

   DEXIT;
   return ret;
}

int main(int argc, char *argv[])
{
   int ret = EXIT_SUCCESS;
   lList *answer_list = NULL;

   DENTER_MAIN(TOP_LAYER, "test_sge_mirror");

   lInit(nmv);

   sge_getme(SPOOLDEFAULTS);

   if (argc == 2 && strcmp(argv[1], "method") == 0) {
      printf("%s\n", get_spooling_method());
   } else if (!sge_setup_paths(sge_get_default_cell(), NULL)) {
      /* will never be reached, as sge_setup_paths exits on failure */
      ret = EXIT_FAILURE;
   } else {
      spooling_maintenance_command cmd = SPM_info;
      feature_initialize_from_file(path_state_get_product_mode_file(), NULL);
      /* parse commandline */
     if (argc < 4) {
         usage(argv[0]);
         ret = EXIT_FAILURE;
      } else {
         bool check_framework = true;
         const char *shlib = argv[1];
         const char *libargs = argv[2];
         const char *command = argv[3];
         const char *args    = NULL;

         if (strcmp(command, "init") == 0) {
            cmd = SPM_init;
            /* check would fail, as database not yet exists */
            check_framework = false;
         }  else if (strcmp(command, "history") == 0) {
            cmd = SPM_history;
         }  else if (strcmp(command, "backup") == 0) {
            cmd = SPM_backup;
         }  else if (strcmp(command, "purge") == 0) {
            cmd = SPM_purge;
         }  else if (strcmp(command, "vacuum") == 0) {
            cmd = SPM_vacuum;
         }  else if (strcmp(command, "info") == 0) {
            cmd = SPM_info;
         } else {
            usage(argv[0]);
            ret = EXIT_FAILURE;
         }
       
         /* parse arguments to command */
         if (ret == EXIT_SUCCESS) {
            if (cmd == SPM_init) {
               if (argc == 5) {
                  args = argv[4];
               }
            } else if (cmd == SPM_history || cmd == SPM_backup || 
                       cmd == SPM_purge) {
               if (argc == 5) {
                  args = argv[4];
               } else {
                  usage(argv[0]);
                  ret = EXIT_FAILURE;
               }
            }
         }
       
         /* initialize spooling */
         if (ret == EXIT_SUCCESS) {
            ret = init_framework(shlib, libargs, check_framework);
         }

         /* call maintenance command */
         if (ret == EXIT_SUCCESS) {
            if (!spool_maintain_context(&answer_list, 
                                        spool_get_default_context(),
                                        cmd, args)) {
               answer_list_output(&answer_list);
               ret = EXIT_FAILURE;
            }
         }
      }
   }

   if (spool_get_default_context() != NULL) {
      if (!spool_shutdown_context(&answer_list, spool_get_default_context())) {
         ret = EXIT_FAILURE;
      }
   }

   answer_list_output(&answer_list);

   SGE_EXIT(ret);
   return ret;
}
