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
#include "sge_unistd.h"
#include "sge_dstring.h"
#include "sge_spool.h"
#include "sge_uidgid.h"
#include "setup_path.h"
#include "sge_prog.h"

#include "sge_answer.h"

#include "sge_all_listsL.h"
#include "sge_manop.h"

#include "sge_spooling.h"
#include "sge_spooling_loader.h"

#include "msg_utilbin.h"

static void usage(const char *argv0)
{
   fprintf(stderr, "%s\n %s command\n\n", MSG_UTILBIN_USAGE, argv0);
   fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_COMMANDINTRO1);
   fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_COMMANDINTRO2);
   fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_TEST);
   fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_MANAGERS);
/*    fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_CONFIG); */
   fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_OPERATORS);
   fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_MAKEPE);
}

static bool 
init_spooling_params(const char **shlib, const char **args)
{
   static dstring args_out = DSTRING_INIT;
   const char *name[1] = { "qmaster_spool_dir" };
   char value[1][1025];

   DENTER(TOP_LAYER, "init_spooling_params");

   if (sge_get_confval_array(path_state_get_conf_file(), 1, name, value)) {
      ERROR((SGE_EVENT, MSG_SPOOLDEFAULTS_CANNOTREADSPOOLPARAMS));
      DEXIT;
      return false;
   }

   sge_dstring_sprintf(&args_out, "%s/%s;%s", 
                       path_state_get_cell_root(), COMMON_DIR, value[0]);
   
   *shlib = "none";
   *args  = sge_dstring_get_string(&args_out);

   DEXIT;
   return true;
}

static int init_framework(void)
{
   int ret = EXIT_FAILURE;

   const char *shlib;
   const char *args;

   lList *answer_list = NULL;

   DENTER(TOP_LAYER, "init_framework");

   /* read arguments for spooling framework */
   if (init_spooling_params(&shlib, &args)) {
      /* create spooling context */
      lListElem *spooling_context = NULL;

      spooling_context = spool_create_dynamic_context(&answer_list, shlib, args);
      answer_list_output(&answer_list);
      if (spooling_context == NULL) {
         CRITICAL((SGE_EVENT, MSG_SPOOLDEFAULTS_CANNOTCREATECONTEXT));
      } else {
         spool_set_default_context(spooling_context);

         /* initialize spooling context */
         if (!spool_startup_context(&answer_list, spooling_context)) {
            CRITICAL((SGE_EVENT, MSG_SPOOLDEFAULTS_CANNOTSTARTUPCONTEXT));
         } else {
            ret = EXIT_SUCCESS;
         }
         answer_list_output(&answer_list);
      }
   }

   DEXIT;
   return ret;
}

static int spool_manop(const char *name, sge_object_type type)
{
   int ret = EXIT_SUCCESS;
   lList **lpp;
   lListElem *ep;
   lList *answer_list = NULL;

   DENTER(TOP_LAYER, "spool_manager");

   /* We have to store the objects in a master list, as classic spooling
    * writes one file with all managers / operators instead of spooling
    * individual objects.
    */
   lpp = object_type_get_master_list(type);

   if (*lpp == NULL) {
      *lpp = lCreateList("master list", object_type_get_descr(type));
   }

   ep = lCreateElem(MO_Type);
   lSetString(ep, MO_name, name);
   lAppendElem(*lpp, ep);

   if (!spool_write_object(&answer_list, spool_get_default_context(), ep, name, type)) {
      /* error output has been done in spooling function */
      ret = EXIT_FAILURE;
   }
   answer_list_output(&answer_list);

   DEXIT;
   return ret;
}

static int spool_manops(sge_object_type type)
{
   int ret = EXIT_SUCCESS;

   DENTER(TOP_LAYER, "spool_managers");

   if (type == SGE_TYPE_MANAGER) {
      /* add root as manager */
      ret = spool_manop("root", type);
   }

   if (ret == EXIT_SUCCESS) {
      /* add admin user as manager and operator */
      const char *name[1] = { "admin_user" };
      char value[1][1025];

      /* read admin username from config */
      if (sge_get_confval_array(path_state_get_conf_file(), 1, name, value)) {
         ERROR((SGE_EVENT, MSG_SPOOLDEFAULTS_CANNOTREADADMINUSER));
         ret = EXIT_FAILURE;
      } else {
         if (value[0] != NULL) {
            ret = spool_manop(value[0], type);
         }

      }
   }

   DEXIT;
   return ret;
}

static int spool_pe(const char *name)
{
   int ret = EXIT_SUCCESS;
   lListElem *ep;
   lList *answer_list = NULL;

   DENTER(TOP_LAYER, "spool_pe");

   ep = lCreateElem(PE_Type);
   lSetString(ep, PE_name, name);
   lAddSubStr(ep, QR_name, "all", PE_queue_list, QR_Type);
   lSetUlong(ep, PE_slots, 999);
   lSetString(ep, PE_allocation_rule, "$pe_slots");
   lSetBool(ep, PE_control_slaves, true);
   lSetBool(ep, PE_job_is_first_task, false);

   if (!spool_write_object(&answer_list, spool_get_default_context(), ep, name, SGE_TYPE_PE)) {
      /* error output has been done in spooling function */
      ret = EXIT_FAILURE;
   }
   answer_list_output(&answer_list);

   DEXIT;
   return ret;
}

static int spool_pes(void)
{
   int ret;

   DENTER(TOP_LAYER, "spool_pes");

   ret = spool_pe("make");

   DEXIT;
   return ret;
}

int main(int argc, char *argv[])
{
   int ret = EXIT_SUCCESS;

   DENTER_MAIN(TOP_LAYER, "test_sge_mirror");

   lInit(nmv);

   if (sge_setup_paths(sge_get_default_cell(), NULL)) {
      ret = EXIT_FAILURE;
   } else {
      /* parse commandline */
      if (argc < 2) {
         usage(argv[0]);
         ret = EXIT_FAILURE;
      } else {
         ret = init_framework();

         if (ret == EXIT_SUCCESS) {
            int i;

            for (i = 1; i < argc; i++) {
               if (strcmp(argv[i], "test") == 0) {
                  /* nothing to do - init_framework succeeded */
               } else if (strcmp(argv[i], "config") == 0) {
               } else if (strcmp(argv[i], "managers") == 0) {
                  ret = spool_manops(SGE_TYPE_MANAGER);
               } else if (strcmp(argv[i], "operators") == 0) {
                  ret = spool_manops(SGE_TYPE_OPERATOR);
               } else if (strcmp(argv[i], "pes") == 0) {
                  ret = spool_pes();
               } else {
                  usage(argv[0]);
                  ret = EXIT_FAILURE;
               }
            }
         }
      }
   }

   SGE_EXIT(ret);
   return ret;
}
