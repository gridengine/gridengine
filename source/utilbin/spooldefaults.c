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

#include "sge_bootstrap.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_unistd.h"
#include "sge_dstring.h"
#include "sge_spool.h"
#include "sge_uidgid.h"
#include "setup_path.h"
#include "sge_prog.h"
#include "sge_feature.h"
#include "sge_answer.h"
#include "sge_manop.h"
#include "sge_pe.h"
#include "sge_centry.h"
#include "sge_userset.h"
#include "sge_all_listsL.h"
#include "sge_conf.h"
#include "sge_mt_init.h"
#include "spool/sge_spooling.h"
#include "spool/dynamic/sge_spooling_loader.h"
#include "spool/classic/read_list.h"
#include "spool/classic/rw_configuration.h"
#include "msg_utilbin.h"


static void usage(const char *argv0)
{
   fprintf(stderr, "%s\n %s command\n\n", MSG_UTILBIN_USAGE, argv0);
   fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_COMMANDINTRO1);
   fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_COMMANDINTRO2);
   fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_TEST);
   fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_MANAGERS);
   fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_OPERATORS);
   fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_CONFIGURATION);
   fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_LOCAL_CONF);
   fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_PES);
   fprintf(stderr, "%s", MSG_SPOOLDEFAULTS_USERSETS);
}

static int init_framework(void)
{
   int ret = EXIT_FAILURE;

   lList *answer_list = NULL;
   lListElem *spooling_context = NULL;

   DENTER(TOP_LAYER, "init_framework");

   /* create spooling context */
   spooling_context = spool_create_dynamic_context(&answer_list, 
                              bootstrap_get_spooling_lib(), 
                              bootstrap_get_spooling_params());
   answer_list_output(&answer_list);
   if (spooling_context == NULL) {
      CRITICAL((SGE_EVENT, MSG_SPOOLDEFAULTS_CANNOTCREATECONTEXT));
   } else {
      spool_set_default_context(spooling_context);

      /* initialize spooling context */
      if (!spool_startup_context(&answer_list, spooling_context, true)) {
         CRITICAL((SGE_EVENT, MSG_SPOOLDEFAULTS_CANNOTSTARTUPCONTEXT));
      } else {
         ret = EXIT_SUCCESS;
      }
      answer_list_output(&answer_list);
   }

   DEXIT;
   return ret;
}

static int spool_manop(const char *name, sge_object_type type)
{
   /* nod master list */
   int ret = EXIT_SUCCESS;
   lList **lpp = NULL;
   lListElem *ep;
   lList *answer_list = NULL;

   DENTER(TOP_LAYER, "spool_manop");

   /* We have to store the objects in a master list, as classic spooling
    * writes one file with all managers / operators instead of spooling
    * individual objects.
    */
   lpp = object_type_get_master_list(type);
      
   if (*lpp == NULL) {
      *lpp = lCreateList("master list", object_type_get_descr(type));
   }

   /* only classic spooling */
   ep = lCreateElem(MO_Type);
   lSetString(ep, MO_name, name);
   lAppendElem(*lpp, ep);
   
   object_type_commit_master_list(type, &answer_list);
      
   if (!spool_write_object(&answer_list, spool_get_default_context(), ep, name, type)) {
      /* error output has been done in spooling function */
      ret = EXIT_FAILURE;
   }
   
   answer_list_output(&answer_list);

   DEXIT;
   return ret;
}

static int spool_manops(sge_object_type type, int argc, char *argv[])
{
   int ret = EXIT_SUCCESS;
   int i;

   DENTER(TOP_LAYER, "spool_managers");

   for (i = 2; i < argc; i++) {
      ret = spool_manop(argv[i], type);
      if (ret != EXIT_SUCCESS) {
         break;
      }
   }

   DEXIT;
   return ret;
}

static int spool_configuration(int argc, char *argv[])
{
   int ret = EXIT_SUCCESS;
   lListElem *conf;
   lList *answer_list = NULL;

   DENTER(TOP_LAYER, "spool_configuration");

   conf = read_configuration(argv[2], SGE_GLOBAL_NAME, FLG_CONF_SPOOL);
   if (conf == NULL) {
      ERROR((SGE_EVENT, "couldn't read local config file "SFN"\n", argv[2]));
      ret = EXIT_FAILURE;
   } else {
      if (!spool_write_object(&answer_list, spool_get_default_context(), conf, SGE_GLOBAL_NAME, SGE_TYPE_CONFIG)) {
         /* error output has been done in spooling function */
         ret = EXIT_FAILURE;
      }
      answer_list_output(&answer_list);
   }

   DEXIT;
   return ret;
}

static int spool_local_conf(int argc, char *argv[])
{
   int ret = EXIT_SUCCESS;
   lListElem *conf;
   lList *answer_list = NULL;

   DENTER(TOP_LAYER, "spool_configuration");

   /* we get an additional argument: the config name */
   if (argc < 4) {
      usage(argv[0]);
      ret = EXIT_FAILURE;
   } else {

      conf = read_configuration(argv[2], argv[3], FLG_CONF_SPOOL);

      if (conf == NULL) {
         ERROR((SGE_EVENT, "couldn't read local config file "SFN"\n", argv[2]));
         ret = EXIT_FAILURE;
      } else {
         if (!spool_write_object(&answer_list, spool_get_default_context(), 
                                 conf, argv[3], SGE_TYPE_CONFIG)) {
            /* error output has been done in spooling function */
            ret = EXIT_FAILURE;
         }
         answer_list_output(&answer_list);
      }
   }

   DEXIT;
   return ret;
}

static int spool_complexes(int argc, char *argv[])
{
   int ret = EXIT_SUCCESS;
   lList **centry_list;
   lListElem *centry;
   lList *answer_list = NULL;

   DENTER(TOP_LAYER, "spool_complexes");

   read_all_centries(argv[2]);

   centry_list = centry_list_get_master_list();

   for_each(centry, *centry_list) {
      if (!spool_write_object(&answer_list, spool_get_default_context(), centry,
                              lGetString(centry, CE_name), SGE_TYPE_CENTRY)) {
         /* error output has been done in spooling function */
         ret = EXIT_FAILURE;
         answer_list_output(&answer_list);
         break;
      }
   }

   DEXIT;
   return ret;
}

static int spool_pes(int argc, char *argv[])
{
   int ret = EXIT_SUCCESS;
   lList *answer_list = NULL;
   lList **pe_list;
   lListElem *pe;

   DENTER(TOP_LAYER, "spool_pes");

   sge_read_pe_list_from_disk(argv[2]);

   pe_list = pe_list_get_master_list();
   for_each(pe, *pe_list) {
      if (!spool_write_object(&answer_list, spool_get_default_context(), pe, lGetString(pe, PE_name), SGE_TYPE_PE)) {
         /* error output has been done in spooling function */
         ret = EXIT_FAILURE;
         answer_list_output(&answer_list);
         break;
      }
   }

   DEXIT;
   return ret;
}

static int spool_usersets(int argc, char *argv[])
{
   int ret = EXIT_SUCCESS;
   lList *answer_list = NULL;
   lList **userset_list;
   lListElem *userset;

   DENTER(TOP_LAYER, "spool_usersets");

   sge_read_userset_list_from_disk(argv[2]);

   userset_list = userset_list_get_master_list();

   for_each(userset, *userset_list) {
      if (!spool_write_object(&answer_list, spool_get_default_context(), 
                              userset, lGetString(userset, US_name), 
                              SGE_TYPE_USERSET)) {
         /* error output has been done in spooling function */
         ret = EXIT_FAILURE;
         answer_list_output(&answer_list);
         break;
      }
   }

   DEXIT;
   return ret;
}

int main(int argc, char *argv[])
{
   int ret = EXIT_SUCCESS;
   lList *answer_list = NULL;

   DENTER_MAIN(TOP_LAYER, "test_sge_mirror");

   sge_mt_init();

   lInit(nmv);

   sge_getme(SPOOLDEFAULTS);

   if (!sge_setup_paths(sge_get_default_cell(), NULL)) {
      /* will never be reached, as sge_setup_paths exits on failure */
      ret = EXIT_FAILURE;
   } else if (!sge_bootstrap(NULL)) {
      ret = EXIT_FAILURE;
   } else if (feature_initialize_from_string(bootstrap_get_product_mode())) {
      ret = EXIT_FAILURE;
   } else {
      /* parse commandline */
      if (argc < 2) {
         usage(argv[0]);
         ret = EXIT_FAILURE;
      } else {
         ret = init_framework();

         if (ret == EXIT_SUCCESS) {
            if (strcmp(argv[1], "test") == 0) {
               /* nothing to do - init_framework succeeded */
            } else {
               /* all other commands have at least one parameter */
               if (argc < 3) {
                  usage(argv[0]);
                  ret = EXIT_FAILURE;
               } else if (strcmp(argv[1], "managers") == 0) {
                  ret = spool_manops(SGE_TYPE_MANAGER, argc, argv);
               } else if (strcmp(argv[1], "operators") == 0) {
                  ret = spool_manops(SGE_TYPE_OPERATOR, argc, argv);
               } else if (strcmp(argv[1], "pes") == 0) {
                  ret = spool_pes(argc, argv);
               } else if (strcmp(argv[1], "complexes") == 0) {
                  ret = spool_complexes(argc, argv);
               } else if (strcmp(argv[1], "configuration") == 0) {
                  ret = spool_configuration(argc, argv);
               } else if (strcmp(argv[1], "local_conf") == 0) {
                  ret = spool_local_conf(argc, argv);
               } else if (strcmp(argv[1], "usersets") == 0) {
                  ret = spool_usersets(argc, argv);
               } else {
                  usage(argv[0]);
                  ret = EXIT_FAILURE;
               }
            }
         }
      }
   }

   if (spool_get_default_context() != NULL) {
      time_t next_trigger = 0;

      if (!spool_trigger_context(&answer_list, spool_get_default_context(), 
                                 0, &next_trigger)) {
         ret = EXIT_FAILURE;
      }
      if (!spool_shutdown_context(&answer_list, spool_get_default_context())) {
         ret = EXIT_FAILURE;
      }
   }

   answer_list_output(&answer_list);

   SGE_EXIT(ret);
   DEXIT;
   return ret;
}
