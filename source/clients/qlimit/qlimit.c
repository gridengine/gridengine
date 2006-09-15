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
#include <sys/stat.h>
#include <limits.h>
#include <math.h>
#include <float.h>
#include <pwd.h>

#include "basis_types.h"
#include "sge.h"

#include "sge_bootstrap.h"

#include "sge_gdi.h"
#include "sge_all_listsL.h"
#include "commlib.h"
#include "cull_xml.h"
#include "sig_handlers.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_feature.h"
#include "sge_unistd.h"
#include "sge_stdlib.h"
#include "cull_parse_util.h"
#include "parse.h"
#include "sge_host.h"
#include "sge_complex_schedd.h"
#include "sge_parse_num_par.h"
#include "sge_select_queue.h"
#include "qstat_printing.h"
#include "sge_range.h"
#include "load_correction.h"
#include "sge_conf.h"
#include "msg_common.h"
#include "msg_clients_common.h"
#include "msg_qlimit.h"
#include "sge_string.h"
#include "sge_hostname.h"
#include "sge_log.h"
#include "sge_answer.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_qinstance_type.h"
#include "sge_ulong.h"
#include "sge_centry.h"
#include "sgeobj/sge_schedd_conf.h"
#include "sge_mt_init.h"
#include "qlimit_output.h"
#include "sge_object.h"
#include "uti/sge_profiling.h"
#include "uti/sge_uidgid.h"
#include "uti/setup_path.h"
#include "read_defaults.h"
#include "uti/sge_io.h"

#ifdef TEST_GDI2
#include "sge_gdi_ctx.h"
#endif

static report_handler_t* create_xml_report_handler(lList **alpp);

static int xml_report_finished(report_handler_t* handler, lList **alpp);
static int xml_report_started(report_handler_t* handler, lList **alpp);

static int xml_report_limit_rule_begin(report_handler_t* handler, const char* host_name, lList **alpp);
static int xml_report_limit_rule_finished(report_handler_t* handler, const char *limit_name, lList **alpp);
static int xml_report_limit_string_value(report_handler_t* handler, const char* name, const char *value, bool exclude, lList **alpp);
static int xml_report_resource_value(report_handler_t* handler, const char* resource, const char* limit, const char*value, lList **alpp);
static int destroy_xml_report_handler(report_handler_t** handler, lList **alpp);

static int xml_report_started(report_handler_t* handler, lList **alpp) {
   printf("<?xml version='1.0'?>\n");
   printf("<qlimit_result xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n");
   return QLIMIT_SUCCESS;
}

static int xml_report_finished(report_handler_t* handler, lList **alpp) {
   printf("</qlimit_result>\n");
   return QLIMIT_SUCCESS;
}

static report_handler_t* create_xml_report_handler(lList **alpp) {
   
   report_handler_t* ret = (report_handler_t*)sge_malloc(sizeof(report_handler_t));
   if (ret == NULL ) {
      answer_list_add(alpp, "malloc of report_handler_t failed",
                            STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
      return NULL;
   }
   ret->ctx = sge_malloc(sizeof(dstring));
   if (ret->ctx == NULL ) {
      answer_list_add(alpp, "malloc of dstring buffer failed",
                            STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
      return NULL;
   }
   memset(ret->ctx, 0, sizeof(dstring));
   
   ret->report_started = xml_report_started;
   ret->report_finished = xml_report_finished;
   ret->report_limit_rule_begin = xml_report_limit_rule_begin;
   ret->report_limit_string_value = xml_report_limit_string_value;
   ret->report_limit_rule_finished = xml_report_limit_rule_finished;
   ret->report_resource_value = xml_report_resource_value;

   ret->destroy = destroy_xml_report_handler;
   return ret;
}

static int destroy_xml_report_handler(report_handler_t** handler, lList **alpp) {
   if (*handler != NULL ) {
      sge_dstring_free((dstring*)(*handler)->ctx);
      FREE((*handler)->ctx);
      FREE(*handler);
      *handler = NULL;
   }
   return QLIMIT_SUCCESS;
}

static int xml_report_limit_rule_begin(report_handler_t* handler, const char* limit_name, lList **alpp) {
   
  escape_string(limit_name, (dstring*)handler->ctx);
  printf(" <qlimit_rule name='%s'>\n", sge_dstring_get_string((dstring*)handler->ctx));
  sge_dstring_clear((dstring*)handler->ctx);
  return QLIMIT_SUCCESS;
}

static int xml_report_limit_string_value(report_handler_t* handler, const char *name, const char *value, bool exclude, lList **alpp) {
   escape_string(name, (dstring*)handler->ctx);
   if (exclude) {
      printf("   <x%s>", sge_dstring_get_string((dstring*)handler->ctx) );
   } else {
      printf("   <%s>", sge_dstring_get_string((dstring*)handler->ctx) );
   }

   sge_dstring_clear((dstring*)handler->ctx);
   escape_string(value, (dstring*)handler->ctx);
   printf("%s", sge_dstring_get_string((dstring*)handler->ctx));

   sge_dstring_clear((dstring*)handler->ctx);
   escape_string(name, (dstring*)handler->ctx);
   if (exclude) {
      printf("</x%s>\n", sge_dstring_get_string((dstring*)handler->ctx));
   } else {
      printf("</%s>\n", sge_dstring_get_string((dstring*)handler->ctx));
   }

   sge_dstring_clear((dstring*)handler->ctx);

   return QLIMIT_SUCCESS;
}

static int xml_report_limit_rule_finished(report_handler_t* handler, const char *limit_name, lList **alpp) {
  printf(" </qlimit_rule>\n");   
   return QLIMIT_SUCCESS;
}

static int xml_report_resource_value(report_handler_t* handler, const char* resource, const char* limit, const char *value, lList **alpp) {
   escape_string(resource, (dstring*)handler->ctx);
   printf("   <limit resource='%s' ", sge_dstring_get_string((dstring*)handler->ctx));
 
   sge_dstring_clear((dstring*)handler->ctx);
   escape_string(limit, (dstring*)handler->ctx);   
   printf("limit='%s'", sge_dstring_get_string((dstring*)handler->ctx));

   if (value != NULL) {
      sge_dstring_clear((dstring*)handler->ctx);
      escape_string(value, (dstring*)handler->ctx);   
      printf(" value='%s'", sge_dstring_get_string((dstring*)handler->ctx));
   }
   printf("/>\n");
   
   sge_dstring_clear((dstring*)handler->ctx);
   return QLIMIT_SUCCESS;
}

static bool sge_parse_from_file_qlimit(const char *file, lList **ppcmdline, lList **alpp);
static bool sge_parse_cmdline_qlimit(char **argv, lList **ppcmdline, lList **alpp);
static bool sge_parse_qlimit(lList **ppcmdline, lList **host_list, lList **resource_match_list,
                             lList **user_list, lList **pe_list, lList **project_list, lList **cqueue_list,
                             report_handler_t **report_handler, lList **alpp);
static bool qlimit_usage(FILE *fp);

extern char **environ;
                                      
/************************************************************************/
int main(int argc, char **argv)
{
   lList *pcmdline = NULL;
   lList *host_list = NULL;
   lList *resource_match_list = NULL;
   lList *user_list = NULL;
   lList *pe_list = NULL;
   lList *project_list = NULL;
   lList *cqueue_list = NULL;

   lList *alp = NULL;
   report_handler_t *report_handler = NULL;
   
   int qlimit_result = 0;

#ifdef TEST_GDI2   
   sge_gdi_ctx_class_t *ctx = NULL;
#endif

   DENTER_MAIN(TOP_LAYER, "qlimit");

   log_state_set_log_gui(true);

#ifdef TEST_GDI2
   if (sge_gdi2_setup(&ctx, QLIMIT, &alp) != AE_OK) {
      answer_list_output(&alp);
      sge_prof_cleanup();
      SGE_EXIT((void**)&ctx, 1);
   }
#else
   sge_mt_init();

   sge_gdi_param(SET_MEWHO, QLIMIT, NULL);
   if (sge_gdi_setup(prognames[QLIMIT], &alp) != AE_OK) {
      answer_list_output(&alp);
      sge_prof_cleanup();
      SGE_EXIT(NULL, 1);
   }
#endif

   sge_setup_sig_handlers(QLIMIT);
   
   /*
   ** stage 1 of commandline parsing
   */
   {
      dstring file = DSTRING_INIT;
#ifdef TEST_GDI2
      const char *user = ctx->get_username(ctx);
      const char *cell_root = ctx->get_cell_root(ctx);
#else
      const char *user = uti_state_get_user_name();
      const char *cell_root = path_state_get_cell_root();
#endif

      /* arguments from SGE_ROOT/common/sge_qlimit file */
      get_root_file_path(&file, cell_root, SGE_COMMON_DEF_QLIMIT_FILE);
      if (sge_parse_from_file_qlimit(sge_dstring_get_string(&file), &pcmdline, &alp) == true) {
         /* arguments from $HOME/.qlimit file */
         if (get_user_home_file_path(&file, SGE_HOME_DEF_QLIMIT_FILE, user, &alp)) {
            sge_parse_from_file_qlimit(sge_dstring_get_string(&file), &pcmdline, &alp);
         }
      }
      sge_dstring_free(&file); 

      if (alp) {
         answer_list_output(&alp);
         lFreeList(&pcmdline);
         sge_prof_cleanup();
         SGE_EXIT(NULL, 1);
      }
   }
   if (sge_parse_cmdline_qlimit(argv, &pcmdline, &alp) == false) {
      answer_list_output(&alp);
      lFreeList(&pcmdline);
      sge_prof_cleanup();
      SGE_EXIT(NULL, 1);
   }

   /*
   ** stage 2 of commandline parsing 
   */
   if (sge_parse_qlimit(&pcmdline, 
            &host_list,             /* -h host_list                  */
            &resource_match_list,   /* -l resource_request           */
            &user_list,             /* -u user_list                  */
            &pe_list,               /* -pe pe_list                   */
            &project_list,          /* -P project_list               */
            &cqueue_list,           /* -q wc_queue_list              */
            &report_handler,
            &alp) == false) {
      /*
      ** low level parsing error! show answer list
      */
      answer_list_output(&alp);
      lFreeList(&pcmdline);
      sge_prof_cleanup();
      SGE_EXIT(NULL, 1);
   }

#ifdef TEST_GDI2
   qlimit_result = qlimit_output(ctx, host_list, resource_match_list, user_list, pe_list, project_list, cqueue_list, &alp, report_handler);
#else
   qlimit_result = qlimit_output(NULL, host_list, resource_match_list, user_list, pe_list, project_list, cqueue_list, &alp, report_handler);
#endif
   
   if (report_handler != NULL ) {
      report_handler->destroy(&report_handler, &alp);
   }
   
   if ( qlimit_result != 0 ) {
      answer_list_output(&alp);
      sge_prof_cleanup();
      SGE_EXIT(NULL, 1);
   }
   sge_prof_cleanup();
   SGE_EXIT(NULL, 0); /* 0 means ok - others are errors */
   DEXIT;
   return 0;
}

/****** qlimit/qlimit_usage() **************************************************
*  NAME
*     qlimit_usage() -- displays qlimit help output
*
*  SYNOPSIS
*     static bool qlimit_usage(FILE *fp) 
*
*  FUNCTION
*     displays qlimit_usage for qlist client
*     note that the other clients use a common function
*     for this. output was adapted to a similar look.
*
*  INPUTS
*     FILE *fp - output file pointer
*
*  RESULT
*     static bool - true on success
*                   false on error
*
*  NOTES
*     MT-NOTE: qlimit_usage() is MT safe 
*
*******************************************************************************/
static bool 
qlimit_usage(FILE *fp)
{
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "qlimit_usage");

   if (fp == NULL) {
      DRETURN(false);
   }

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   fprintf(fp, "%s\n", feature_get_product_name(FS_SHORT_VERSION, &ds));
   fprintf(fp,"%s qlimit [options]\n", MSG_SRC_USAGE);
   fprintf(fp, "  [-help]                    %s\n", MSG_COMMON_help_OPT_USAGE);
   fprintf(fp, "  [-h host_list]             %s\n", MSG_QLIMIT_h_OPT_USAGE);
   fprintf(fp, "  [-l resource_attributes]   %s\n", MSG_QLIMIT_l_OPT_USAGE);
   fprintf(fp, "  [-u user_list]             %s\n", MSG_QLIMIT_u_OPT_USAGE);
   fprintf(fp, "  [-pe pe_list]              %s\n", MSG_QLIMIT_pe_OPT_USAGE);
   fprintf(fp, "  [-P project_list]          %s\n", MSG_QLIMIT_P_OPT_USAGE); 
   fprintf(fp, "  [-q wc_queue_list]         %s\n", MSG_QLIMIT_q_OPT_USAGE); 
   fprintf(fp, "  [-xml]                     %s\n", MSG_COMMON_xml_OPT_USAGE);

   DRETURN(true);
}

/****** qlimit/sge_parse_from_file_qlimit() ************************************
*  NAME
*     sge_parse_from_file_qlimit() -- parse qlimit command line options from
*                                     file
*
*  SYNOPSIS
*     static bool sge_parse_from_file_qlimit(const char *file, lList 
*     **ppcmdline, lList **alpp) 
*
*  FUNCTION
*     parses the qlimit command line options from file
*
*  INPUTS
*     const char *file  - file name
*     lList **ppcmdline - found command line options
*     lList **alpp      - answer list pointer
*
*  RESULT
*     static bool - true on success
*                   false on error
*
*  NOTES
*     MT-NOTE: sge_parse_from_file_qlimit() is MT safe 
*
*******************************************************************************/
static bool
sge_parse_from_file_qlimit(const char *file, lList **ppcmdline, lList **alpp)
{
   bool ret = true;

   DENTER(TOP_LAYER, "sge_parse_from_file_qlimit");
   if (ppcmdline == NULL) {
      ret = false;
   } else {
      if (!sge_is_file(file)) {
         /*
          * This is no error
          */
         DPRINTF(("file "SFQ" does not exist\n", file));
         ret = true;
      } else {
         char *file_as_string = NULL;
         int file_as_string_length;

         file_as_string = sge_file2string(file, &file_as_string_length);
         if (file_as_string == NULL) {
            answer_list_add_sprintf(alpp, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR,
                                    MSG_ANSWER_ERRORREADINGFROMFILEX_S, file);
            ret = false;
         } else {
            char **token = NULL;

            token = stra_from_str(file_as_string, " \n\t");
            ret = sge_parse_cmdline_qlimit(token, ppcmdline, alpp);
         }
      }
   }  
   DRETURN(ret); 
}

/****** qlimit/sge_parse_cmdline_qlimit() **************************************
*  NAME
*     sge_parse_cmdline_qlimit() -- ??? 
*
*  SYNOPSIS
*     static bool sge_parse_cmdline_qlimit(char **argv, lList **ppcmdline, 
*     lList **alpp) 
*
*  FUNCTION
*     'stage 1' parsing of qlimit-options. Parses options
*     with their arguments and stores them in ppcmdline.
*
*  INPUTS
*     char **argv       - argument list
*     lList **ppcmdline - found arguments
*     lList **alpp      - answer list pointer
*
*  RESULT
*     static bool - true on success
*                   false on error
*
*  NOTES
*     MT-NOTE: sge_parse_cmdline_qlimit() is MT safe 
*
*******************************************************************************/
static bool sge_parse_cmdline_qlimit(char **argv, lList **ppcmdline, lList **alpp)
{
   char **sp;
   char **rp;
   DENTER(TOP_LAYER, "sge_parse_cmdline_qlimit");

   if (argv == NULL) {
      answer_list_add_sprintf(alpp, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR, MSG_NULLPOINTER);
      DRETURN(false);
   }

   rp = ++argv;
   while(*(sp=rp)) {
      /* -help */
      if ((rp = parse_noopt(sp, "-help", NULL, ppcmdline, alpp)) != sp)
         continue;
 
      /* -h option */
      if ((rp = parse_until_next_opt2(sp, "-h", NULL, ppcmdline, alpp)) != sp)
         continue;

      /* -l option */
      if ((rp = parse_until_next_opt2(sp, "-l", NULL, ppcmdline, alpp)) != sp)
         continue;

      /* -u option */
      if ((rp = parse_until_next_opt2(sp, "-u", NULL, ppcmdline, alpp)) != sp)
         continue;

      /* -pe option */
      if ((rp = parse_until_next_opt2(sp, "-pe", NULL, ppcmdline, alpp)) != sp)
         continue;

      /* -P option */
      if ((rp = parse_until_next_opt2(sp, "-P", NULL, ppcmdline, alpp)) != sp)
         continue;

      /* -q */
      if ((rp = parse_until_next_opt2(sp, "-q", NULL, ppcmdline, alpp)) != sp)
         continue;

      /* -xml */
      if ((rp = parse_noopt(sp, "-xml", NULL, ppcmdline, alpp)) != sp)
         continue;
      
      /* oops */
      qlimit_usage(stderr);
      answer_list_add_sprintf(alpp, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR, MSG_PARSE_INVALIDOPTIONARGUMENTX_S, *sp);
      DRETURN(false);
   }
   DRETURN(true);
}

/****** qlimit/sge_parse_qlimit() **********************************************
*  NAME
*     sge_parse_qlimit() -- parse qlimit options
*
*  SYNOPSIS
*     static bool sge_parse_qlimit(lList **ppcmdline, lList **host_list, lList 
*     **resource_list, lList **user_list, lList **pe_list, lList 
*     **project_list, lList **cqueue_list, report_handler_t **report_handler, 
*     lList **alpp) 
*
*  FUNCTION
*     'stage 2' parsing of qlimit-options. Gets the options from pcmdline
*
*  INPUTS
*     lList **ppcmdline                 - found command line options (from stage 1)
*     lList **host_list                 - parsed host list (-h option)
*     lList **resource_list             - parsed resource list (-l option)
*     lList **user_list                 - parsed user list (-u option)
*     lList **pe_list                   - parsed pe list (-pe option)
*     lList **project_list              - parsed project list (-P option)
*     lList **cqueue_list               - parsed queue list (-q option)
*     report_handler_t **report_handler - report handler for xml output
*     lList **alpp                      - answer list
*
*  RESULT
*     static bool - true on success
*                   false on error
*
*  NOTES
*     MT-NOTE: sge_parse_qlimit() is MT safe 
*
*******************************************************************************/
static bool
sge_parse_qlimit(lList **ppcmdline, lList **host_list, lList **resource_list,
                 lList **user_list, lList **pe_list, lList **project_list,
                 lList **cqueue_list, report_handler_t **report_handler, lList **alpp)
{
   bool showedhelp = false;
   u_long32 helpflag = 0;
   char *argstr = NULL;
   bool ret = true;
 
   DENTER(TOP_LAYER, "sge_parse_qlimit");
 
   /* Loop over all options. Only valid options can be in the
      ppcmdline list. 
   */
   while (lGetNumberOfElem(*ppcmdline)) {
      if (parse_flag(ppcmdline, "-help",  alpp, &helpflag)) {
         showedhelp = true;
         qlimit_usage(stdout);
         ret = false;
         break;
      }

      if (parse_multi_stringlist(ppcmdline, "-h", alpp, host_list, ST_Type, ST_name)) {
         /* 
         ** resolve hostnames and replace them in list
         */
         lListElem *ep = NULL;
         for_each(ep, *host_list) {
            sge_resolve_host(ep, ST_name);
         }
         continue;
      }

      if (parse_string(ppcmdline, "-l", alpp, &argstr)) {
         *resource_list = centry_list_parse_from_string(*resource_list, argstr, false);
         FREE(argstr);
         continue;
      }
      if (parse_multi_stringlist(ppcmdline, "-u", alpp, user_list, ST_Type, ST_name)) {
         continue;
      }
      if (parse_multi_stringlist(ppcmdline, "-pe", alpp, pe_list, ST_Type, ST_name)) {
         continue;
      }
      if (parse_multi_stringlist(ppcmdline, "-P", alpp, project_list, ST_Type, ST_name)) {
         continue;
      }
      if (parse_multi_stringlist(ppcmdline, "-q", alpp, cqueue_list, ST_Type, ST_name)) {
         continue;
      }
      if (parse_flag(ppcmdline, "-xml", alpp, &helpflag)) {
         *report_handler = create_xml_report_handler(alpp);
         continue;
      }
   }

   if (lGetNumberOfElem(*ppcmdline)) {
     if (showedhelp == false) {
       qlimit_usage(stderr);
     }
     answer_list_add_sprintf(alpp, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR, MSG_PARSE_TOOMANYOPTIONS);
     ret = false;
   }

   DRETURN(ret);
}
