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
#include <sys/types.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <limits.h>
#include <ctype.h>
#include <fnmatch.h>

#include "sge_unistd.h"
#include "sge.h"
#include "sge_gdi.h"
#include "sge_options.h"
#include "sge_pe.h"
#include "sge_dstring.h"
#include "sge_string.h"
#include "sge_eventL.h"
#include "sge_idL.h"
#include "sge_answer.h"
#include "sge_cuser.h"
#include "parse.h"
#include "usage.h"
#include "commlib.h"
#include "config.h"
#include "sge_client_access.h"
#include "parse_qconf.h"
#include "sge_host.h"
#include "sge_sharetree.h"
#include "sge_userset.h"
#include "sge_feature.h"
#include "gdi_tsm.h"
#include "gdi_checkpermissions.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_str.h"
#include "scheduler.h"
#include "sge_support.h"
#include "sge_stdlib.h"
#include "sge_spool.h"
#include "sge_signal.h"
#include "sge_io.h"
#include "sge_schedd_conf.h"
#include "sge_userprj.h"
#include "sge_manop.h"
#include "sgeobj/sge_calendar.h"
#include "sge_hgroup.h"
#include "sge_conf.h"
#include "sge_ckpt.h"
#include "sge_hgroup_qconf.h"
#include "sge_cuser_qconf.h"
#include "sge_centry_qconf.h"
#include "sge_cqueue_qconf.h"
#include "sge_edit.h"
#include "sge_cqueue.h"
#include "sge_qinstance.h"
#include "sge_href.h"
#include "sge_qref.h"
#include "sge_centry.h"
#include "sge_attr.h"
#include "sge_qinstance_state.h"

#ifdef QCONF_FLATFILE
#include "spool/flatfile/sge_flatfile.h"
#include "spool/flatfile/sge_flatfile_obj.h"
#include "sgeobj/sge_usageL.h"
#include "sgeobj/sge_userprjL.h"
#include "sgeobj/sge_range.h"
#include "sgeobj/sge_cqueueL.h"
#include "sgeobj/sge_subordinateL.h"
#endif

#include "msg_common.h"
#include "msg_qconf.h"

static int sge_next_is_an_opt(char **ptr);
static int sge_error_and_exit(const char *ptr);

/* ------------------------------------------------------------- */
static int show_object_list(u_long32, lDescr *, int, char *);
static int show_processors(void);
static int show_eventclients(void);

static void show_gdi_request_answer(lList *alp);
static void show_gdi_request_answer_list(lList *alp);
/* ------------------------------------------------------------- */
static void parse_name_list_to_cull(char *name, lList **lpp, lDescr *dp, int nm, char *s);
static int add_host_of_type(lList *arglp, u_long32 target);
static int del_host_of_type(lList *arglp, u_long32 target);
static int print_acl(lList *arglp);
static int qconf_modify_attribute(lList **alpp, int from_file, char ***spp, lListElem **epp, int sub_command, struct object_info_entry *info_entry); 
static lListElem *edit_exechost(lListElem *ep);
static int edit_usersets(lList *arglp);

/************************************************************************/
static int print_config(const char *config_name);
static int delete_config(const char *config_name);
static int add_modify_config(const char *config_name, const char *filename, u_long32 flags);
static lList* edit_sched_conf(lList *confl);
static lListElem* edit_userprj(lListElem *ep, int user);
static lListElem *edit_sharetree(lListElem *ep);

static char **sge_parser_get_next(char **head);

/************************************************************************/
static int sge_gdi_is_manager(const char *user);
static int sge_gdi_is_adminhost(const char *host);
/************************************************************************/

#ifdef QCONF_FLATFILE
static const char *write_attr_tmp_file (const char *name, const char *value, 
                                        char delimiter);

static const spool_flatfile_instr qconf_sub_sfi = 
{
   NULL,
   false,
   false,
   false,
   false,
   false,
   '\0',
   '=',
   ' ',
   '\0',
   '\0',
   &qconf_sub_sfi,
   {NoName, NoName, NoName}
};

static const spool_flatfile_instr qconf_sfi = 
{
   NULL,
   true,
   false,
   false,
   true,
   false,
   '\0',
   '\n',
   '\0',
   '\0',
   '\0',
   &qconf_sub_sfi,
   {NoName, NoName, NoName}
};

static const spool_flatfile_instr qconf_sub_comma_list_sfi = 
{
   NULL,
   false,
   false,
   false,
   false,
   false,
   '\0',
   ',',
   '\0',
   '\0',
   '\0',
   NULL,
   {NoName, NoName, NoName}
};

static const spool_flatfile_instr qconf_name_value_list_sfi = 
{
   NULL,
   true,
   false,
   false,
   false,
   false,
   '=',
   '\n',
   ',',
   '\0',
   '\0',
   &qconf_sub_comma_list_sfi,
   {
      STN_children,
      STN_id,
      STN_version
   }
};

static const spool_flatfile_instr qconf_sub_name_value_comma_sfi = 
{
   NULL,
   false,
   false,
   false,
   false,
   false,
   '\0',
   '=',
   ',',
   '\0',
   '\0',
   NULL,
   { NoName, NoName, NoName }
};

static const spool_flatfile_instr qconf_sub_name_value_space_sfi = 
{
   NULL,
   false,
   false,
   false,
   false,
   false,
   '\0',
   '=',
   ' ',
   '\0',
   '\0',
   &qconf_sub_name_value_space_sfi,
   { NoName, NoName, NoName }
};

static const spool_flatfile_instr qconf_sub_comma_sfi = 
{
   NULL,
   false,
   false,
   false,
   false,
   false,
   '\0',
   '\0',
   ',',
   '\0',
   '\0',
   &qconf_sub_name_value_space_sfi,
   { NoName, NoName, NoName }
};

static const spool_flatfile_instr qconf_param_sfi = 
{
   NULL,
   true,
   false,
   false,
   false,
   false,
   '\0',
   '\n',
   '\0',
   '\0',
   '\0',
   &qconf_sub_comma_sfi,
   { NoName, NoName, NoName }
};

static const spool_flatfile_instr qconf_sub_param_sfi = 
{
   NULL,
   false,
   false,
   false,
   false,
   true,
   '\0',
   ' ',
   '\0',
   '\0',
   '\n',
   NULL,
   { NoName, NoName, NoName }
};

static const spool_flatfile_instr qconf_comma_sfi = 
{
   NULL,
   true,
   false,
   false,
   true,
   false,
   ' ',
   '\n',
   '\0',
   '\0',
   '\0',
   &qconf_sub_name_value_comma_sfi,
   { NoName, NoName, NoName }
};

static const spool_flatfile_instr qconf_sub_name_value_comma_braced_sfi = 
{
   NULL,
   false,
   false,
   false,
   false,
   false,
   '\0',
   '=',
   ',',
   '[',
   ']',
   &qconf_sub_name_value_comma_sfi,
   { NoName, NoName, NoName }
};

#endif
/***************************************************************************/
static char **sge_parser_get_next(char **arg) 
{
   DENTER(TOP_LAYER, "sge_parser_get_next");
   if (!*(arg+1)) {
      ERROR((SGE_EVENT, MSG_PARSE_NOOPTIONARGPROVIDEDTOX_S , *arg));
      sge_usage(stderr);
      SGE_EXIT(1);
   }

   DEXIT;
   return (++arg);
}

/*------------------------------------------------------------*/
int sge_parse_qconf(
char *argv[] 
) {
   int status;
   char *cp = NULL;
   char **spp = NULL;
   int opt;
   int sge_parse_return = 0;
   lEnumeration *what = NULL;
   lCondition *where = NULL;
   lList *lp=NULL, *arglp=NULL, *alp=NULL, *newlp=NULL;
   lListElem *hep=NULL, *ep=NULL, *argep=NULL, *aep=NULL, *newep = NULL;
   const char *host = NULL;
   char *filename;
#ifdef QCONF_FLATFILE
   int fields_out[MAX_NUM_FIELDS];
   int missing_field = NoName;
#endif

   DENTER(TOP_LAYER, "sge_parse_qconf");

   /* If no arguments were given, output the help message. */
   if (*argv == NULL) {
      sge_usage(stdout);
      SGE_EXIT(0);
   }
   
   spp = argv;

   while (*spp) {

/*----------------------------------------------------------------------------*/
      /* "-acal cal-name" */

      if ((strcmp("-acal", *spp) == 0) ||
          (strcmp("-Acal", *spp) == 0)) {
         if (!strcmp("-acal", *spp)) {
            sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
            sge_gdi_is_manager(uti_state_get_user_name());

            spp = sge_parser_get_next(spp);
           
            /* get a generic calendar */
            ep = sge_generic_cal(*spp); 
#ifdef QCONF_FLATFILE
            filename = (char *)spool_flatfile_write_object(&alp, ep, false,
                                                 CAL_fields, &qconf_sfi,
                                                 SP_DEST_TMP, SP_FORM_ASCII,
                                                 NULL, false);
            if (answer_list_output(&alp)) {
               sge_error_and_exit(NULL);
            }

#else
            filename = write_cal(0, 1, ep);
#endif
            ep = lFreeElem(ep);
            
            /* edit this file */
            status = sge_edit(filename);
            if (status < 0) {
               unlink(filename);
               if (sge_error_and_exit(MSG_PARSE_EDITFAILED))
                  continue;
            }

            if (status > 0) {
               unlink(filename);
               if (sge_error_and_exit(MSG_FILE_FILEUNCHANGED))
                  continue;
            }
         
            /* read it in again */
#ifdef QCONF_FLATFILE
            fields_out[0] = NoName;
            ep = spool_flatfile_read_object(&alp, CAL_Type, NULL,
                                            CAL_fields, fields_out, true, &qconf_sfi,
                                            SP_FORM_ASCII, NULL, filename);
            unlink(filename);
            
            if (answer_list_output (&alp)) {
               ep = lFreeElem (ep);
            }

            if (ep != NULL) {
               missing_field = spool_get_unprocessed_field (CAL_fields, fields_out, &alp);
            }

            if (missing_field != NoName) {
               ep = lFreeElem (ep);
               answer_list_output (&alp);
            }      
#else
            ep = cull_read_in_cal(NULL, filename, 0, 0, NULL, NULL); 
            unlink(filename);
#endif
            
            if (ep == NULL) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE)) {
                  continue;
               }
            }
         } else {
            spp = sge_parser_get_next(spp);
           
#ifdef QCONF_FLATFILE
            fields_out[0] = NoName;
            ep = spool_flatfile_read_object(&alp, CAL_Type, NULL,
                                            CAL_fields, fields_out, true, &qconf_sfi,
                                            SP_FORM_ASCII, NULL, *spp);
            if (answer_list_output(&alp)) {
               ep = lFreeElem (ep);
            }

            if (ep != NULL) {
               missing_field = spool_get_unprocessed_field (CAL_fields, fields_out, &alp);
            }

            if (missing_field != NoName) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }
#else
            ep = cull_read_in_cal(NULL, *spp, 0, 0, NULL, NULL); 
#endif
            
            if (ep == NULL) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE)) {
                  continue;
               }
            }
         }

         /* send it to qmaster */
         lp = lCreateList("cal to add", CAL_Type); 
         lAppendElem(lp, ep);
DPRINTF(("ep: %s %s\n",
   lGetString(ep, CAL_year_calendar)?lGetString(ep, CAL_year_calendar):MSG_SMALLNULL,
   lGetString(ep, CAL_week_calendar)?lGetString(ep, CAL_week_calendar):MSG_SMALLNULL));


         alp = sge_gdi(SGE_CALENDAR_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-ackpt ckpt_name" or "-Ackpt fname" */

      if ((strcmp("-ackpt", *spp) == 0) ||
          (strcmp("-Ackpt", *spp) == 0)) {

         if (!strcmp("-ackpt", *spp)) {
            sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
            sge_gdi_is_manager(uti_state_get_user_name());

            spp = sge_parser_get_next(spp);

            /* get a generic ckpt configuration */
            ep = sge_generic_ckpt(*spp); 
#ifdef QCONF_FLATFILE
            filename = (char *)spool_flatfile_write_object(&alp, ep, false,
                                                 CK_fields, &qconf_sfi,
                                                 SP_DEST_TMP, SP_FORM_ASCII,
                                                 NULL, false);
            if (answer_list_output(&alp)) {
               sge_error_and_exit(NULL);
            }
#else
            filename = write_ckpt(0, 1, ep);
#endif
            ep = lFreeElem(ep);
            
            /* edit this file */
            status = sge_edit(filename);
            if (status < 0) {
               unlink(filename);
               if (sge_error_and_exit(MSG_PARSE_EDITFAILED))
                  continue;
            }

            if (status > 0) {
               unlink(filename);
               if (sge_error_and_exit(MSG_FILE_FILEUNCHANGED))
                  continue;
            }
         
            /* read it in again */
#ifdef QCONF_FLATFILE
            fields_out[0] = NoName;
            ep = spool_flatfile_read_object(&alp, CK_Type, NULL,
                                            CK_fields, fields_out, true, &qconf_sfi,
                                            SP_FORM_ASCII, NULL, filename);
            unlink(filename);
            
            if (answer_list_output(&alp)) {
               ep = lFreeElem (ep);
            }

            if (ep != NULL) {
               missing_field = spool_get_unprocessed_field (CK_fields, fields_out, &alp);
            }

            if (missing_field != NoName) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }
            
            if ((ep != NULL) && (ckpt_validate(ep, &alp) != STATUS_OK)) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }
#else
            ep = cull_read_in_ckpt(NULL, filename, 0, 0, NULL, NULL);
            unlink(filename);
#endif
            
            if (ep == NULL) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE)) {
                  continue;
               }
            }
         } else {
            spp = sge_parser_get_next(spp);

#ifdef QCONF_FLATFILE
            fields_out[0] = NoName;
            ep = spool_flatfile_read_object(&alp, CK_Type, NULL,
                                            CK_fields, fields_out,  true, &qconf_sfi,
                                            SP_FORM_ASCII, NULL, *spp);
            
            if (answer_list_output(&alp)) {
               ep = lFreeElem(ep);
            }

            if (ep != NULL) {
               missing_field = spool_get_unprocessed_field (CK_fields, fields_out, &alp);
            }

            if (missing_field != NoName) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }

            if ((ep != NULL) && (ckpt_validate(ep, &alp) != STATUS_OK)) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }
#else
            ep = cull_read_in_ckpt(NULL, *spp, 0, 0, NULL, NULL);
#endif
               
            if (ep == NULL) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE)) {
                  continue;
               }
            }
         }

         /* send it to qmaster */
         lp = lCreateList("CKPT list to add", CK_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_CKPT_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-ae [server_name]" */
      if (strcmp("-ae", *spp) == 0) {
         char *host = NULL;
         lListElem *hep = NULL;

         cp = NULL;
         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());
         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            host = sge_strdup(host, *spp);

            /* try to resolve hostname */
            hep = lCreateElem(EH_Type);
            lSetHost(hep, EH_name, host);

            switch (sge_resolve_host(hep, EH_name)) {
               case CL_RETVAL_OK:
                  break;
               default:
                  fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetHost(hep, EH_name));
                  hep = lFreeElem(hep);
                  SGE_EXIT(1);
                  break;
            }
            
            host = sge_strdup(host, lGetHost(hep, EH_name));
         }
         else {
            /* no template name given - then use "template" as name */
            host = sge_strdup(host, SGE_TEMPLATE_NAME);
         }

         /* get a template host entry .. */
         where = lWhere("%T( %Ih=%s )", EH_Type, EH_name, host);
         what = lWhat("%T(ALL)", EH_Type);
         alp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_GET, &arglp, where, what);
         what = lFreeWhat(what);
         where = lFreeWhere(where);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         
         if ( !arglp ) {
            fprintf(stderr, MSG_EXEC_XISNOEXECHOST_S, host);   
            spp++;
            sge_parse_return = 1;
            continue;
         }

         argep = lFirst(arglp);
         
         /* edit the template */
         ep=edit_exechost(argep);
         if (!ep) {
            spp++;
            continue;
         }

         switch (sge_resolve_host(ep, EH_name)) {
         case CL_RETVAL_OK:
            break;
         default:
            fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetHost(ep, EH_name));
            ep = lFreeElem(ep);
            SGE_EXIT(1);
            break;
         }

         host = sge_strdup(host, lGetHost(ep, EH_name));
         arglp = lFreeList(arglp);


         lp = lCreateList("hosts to add", EH_Type);
         lAppendElem(lp, ep);
         alp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_ADD, &lp, NULL, NULL);
         lp = lFreeList(lp);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }

         ep = lFirst(alp);
         answer_exit_if_not_recoverable(ep);
         if (answer_get_status(ep) == STATUS_OK)
            fprintf(stderr, MSG_EXEC_ADDEDHOSTXTOEXECHOSTLIST_S, host);
         else
            fprintf(stderr, "%s", lGetString(ep, AN_text));
      
         alp = lFreeList(alp);
         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-Ae fname" */
      if (strcmp("-Ae", *spp) == 0) {
#ifdef QCONF_FLATFILE
      spooling_field *fields = sge_build_EH_field_list (false, false, false);
#endif
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp); 

         hep = lCreateElem(EH_Type);

         /* read file */
         lp = lCreateList("exechosts to add", EH_Type); 
#ifdef QCONF_FLATFILE
         fields_out[0] = NoName;
         ep = spool_flatfile_read_object(&alp, EH_Type, NULL,
                                         fields, fields_out, true, &qconf_sfi,
                                         SP_FORM_ASCII, NULL, *spp);
         if (answer_list_output(&alp)) {
            ep = lFreeElem(ep);
         }

         if (ep != NULL) {
            missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
         }

         FREE (fields);
            
         if (missing_field != NoName) {
            ep = lFreeElem(ep);
            answer_list_output(&alp);
         }
#else
         ep = cull_read_in_host(NULL, *spp, CULL_READ_MINIMUM, EH_name, NULL, NULL);
#endif
         if (!ep) {
            fprintf(stderr, MSG_ANSWER_INVALIDFORMAT); 
            SGE_EXIT(1);
         }
         lAppendElem(lp, ep);

         /* test host name */
         switch (sge_resolve_host(ep, EH_name)) {
         case CL_RETVAL_OK:
            break;
         default:
            fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetHost(ep, EH_name));
            ep = lFreeElem(ep);
            SGE_EXIT(1);
            break;
         }

         alp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
        
         answer_exit_if_not_recoverable(aep); 
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/

      /* "-ah server_name[,server_name,...]" */
      if (strcmp("-ah", *spp) == 0) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);
         parse_name_list_to_cull("host to add", &lp, AH_Type, AH_name, *spp);
         if ( add_host_of_type(lp, SGE_ADMINHOST_LIST) != 0) {
            sge_parse_return = 1;
         }
         
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-am user_list" */

      if (strcmp("-am", *spp) == 0) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);
         lString2List(*spp, &lp, MO_Type, MO_name, ", ");
         alp = sge_gdi(SGE_MANAGER_LIST, SGE_GDI_ADD, &lp, NULL, NULL);
         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-ao user_list" */

      if (strcmp("-ao", *spp) == 0) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);
         lString2List(*spp, &lp, MO_Type, MO_name, ", ");
         alp = sge_gdi(SGE_OPERATOR_LIST, SGE_GDI_ADD, &lp, NULL, NULL);
         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-ap pe_name" */

      if ((strcmp("-ap", *spp) == 0) ||
          (strcmp("-Ap", *spp) == 0)) {
#ifdef QCONF_FLATFILE
         spooling_field *fields = sge_build_PE_field_list (false, false);
#endif

         if (!strcmp("-ap", *spp)) {
            sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
            sge_gdi_is_manager(uti_state_get_user_name());
           
            spp = sge_parser_get_next(spp);

            /* get a generic parallel environment */
            ep = sge_generic_pe(*spp); 
#ifdef QCONF_FLATFILE
            filename = (char *)spool_flatfile_write_object(&alp, ep, false,
                                                 fields, &qconf_sfi,
                                                 SP_DEST_TMP, SP_FORM_ASCII,
                                                 NULL, false);
            if (answer_list_output(&alp)) {
               FREE (fields);
               sge_error_and_exit(NULL);
            }
#else
            filename = write_pe(0, 1, ep);
#endif
            ep = lFreeElem(ep);
            
            /* edit this file */
            status = sge_edit(filename);
            if (status < 0) {
               unlink(filename);
               if (sge_error_and_exit(MSG_PARSE_EDITFAILED)) {
#ifdef QCONF_FLATFILE
                  FREE (fields);
#endif
                  continue;
               }
            }

            if (status > 0) {
               unlink(filename);
               if (sge_error_and_exit(MSG_FILE_FILEUNCHANGED)) {
#ifdef QCONF_FLATFILE
                  FREE (fields);
#endif
                  continue;
               }
            }
         
            /* read it in again */
#ifdef QCONF_FLATFILE
            fields_out[0] = NoName;
            ep = spool_flatfile_read_object(&alp, PE_Type, NULL,
                                            fields, fields_out,  true, &qconf_sfi,
                                            SP_FORM_ASCII, NULL, filename);
            unlink(filename);
            
            if (answer_list_output(&alp)) {
               ep = lFreeElem(ep);
            }

            if (ep != NULL) {
               missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
            }

            FREE (fields);
            
            if (missing_field != NoName) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }

            if ((ep != NULL) && (pe_validate(ep, &alp, 0) != STATUS_OK)) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }
#else
            ep = cull_read_in_pe(NULL, filename, 0, 0, NULL, NULL); 
            unlink(filename);
#endif
            
            if (ep == NULL) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE)) {
                  continue;
               }
            }
         } else {
            spp = sge_parser_get_next(spp);

#ifdef QCONF_FLATFILE
            fields_out[0] = NoName;
            ep = spool_flatfile_read_object(&alp, PE_Type, NULL,
                                            fields, fields_out, true, &qconf_sfi,
                                            SP_FORM_ASCII, NULL, *spp);
            
            if (answer_list_output(&alp)) {
               ep = lFreeElem(ep);
            }

            if (ep != NULL) {
               missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
            }

            FREE (fields);
            
            if (missing_field != NoName) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }

            if ((ep != NULL) && (pe_validate(ep, &alp, 0) != STATUS_OK)) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }
#else
            ep = cull_read_in_pe(NULL, *spp, 0, 0, NULL, NULL); 
#endif

            if (ep == NULL) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE)) {
                  continue;
               }
            }
         }

         /* send it to qmaster */
         lp = lCreateList("PE list to add", PE_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_PE_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-auser" */

      if (strcmp("-auser", *spp) == 0) {
         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());

         spp++;
        
         /* get a template for editing */
         ep = getUserPrjTemplate(); 
         
         ep = edit_userprj(ep, 1);

         /* send it to qmaster */
         lp = lCreateList("User list to add", UP_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_USER_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text)); 
            alp = lFreeList(alp);
            lp = lFreeList(lp);
            SGE_EXIT(1);
         } else {
            fprintf(stdout, "%s", lGetString(aep, AN_text));
         }
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-aprj" */

      if (strcmp("-aprj", *spp) == 0) {
         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());
        
         /* get a template for editing */
         ep = getUserPrjTemplate(); 
         
         ep = edit_userprj(ep, 0);

         /* send it to qmaster */
         lp = lCreateList("Project list to add", UP_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-Auser" */

      if (strcmp("-Auser", *spp) == 0) {
         char* file = NULL;
#ifdef QCONF_FLATFILE
         spooling_field *fields = NULL;
#endif
         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }        


         /* get project  */
         ep = NULL;
#ifdef QCONF_FLATFILE   
         fields_out[0] = NoName;
         fields = sge_build_UP_field_list (0, 1);
         ep = spool_flatfile_read_object(&alp, UP_Type, NULL, fields, fields_out,
                                          true, &qconf_sfi, SP_FORM_ASCII, NULL,
                                          file);
         
         if (answer_list_output(&alp)) {
            ep = lFreeElem (ep);
         }

         if (ep != NULL) {
            missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
         }

         FREE (fields);
         
         if (missing_field != NoName) {
            ep = lFreeElem(ep);
            answer_list_output(&alp);
         }
#else
         ep = cull_read_in_userprj(NULL, file, 0, 1, 0);
#endif

         if (ep == NULL) {
            sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE); 
         }
         
#ifdef QCONF_FLATFILE   
         NULL_OUT_NONE(ep, UP_default_project);
#endif
        
         /* send it to qmaster */
         lp = lCreateList("User to add", UP_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_USER_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text)); 
            alp = lFreeList(alp);
            lp = lFreeList(lp);
            SGE_EXIT(1);
         } else {
            fprintf(stdout, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }


/*----------------------------------------------------------------------------*/

      /* "-Aprj" */

      if (strcmp("-Aprj", *spp) == 0) {
         char* file = NULL;
#ifdef QCONF_FLATFILE
         spooling_field *fields = NULL;
#endif
         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }        


         /* get project  */
         ep = NULL;
#ifdef QCONF_FLATFILE   
         fields_out[0] = NoName;
         fields = sge_build_UP_field_list (0, 0);
         ep = spool_flatfile_read_object(&alp, UP_Type, NULL, fields, fields_out,
                                          true, &qconf_sfi, SP_FORM_ASCII, NULL,
                                          file);
         
         if (answer_list_output(&alp)) {
            ep = lFreeElem (ep);
         }

         if (ep != NULL) {
            missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
         }

         FREE (fields);
         
         if (missing_field != NoName) {
            ep = lFreeElem(ep);
            answer_list_output(&alp);
         }
#else
         ep = cull_read_in_userprj(NULL, file, 0, 0, 0); 
#endif
            
         if (ep == NULL) {
            sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE); 
         }
        
         /* send it to qmaster */
         lp = lCreateList("Project list to add", UP_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text)); 
            alp = lFreeList(alp);
            lp = lFreeList(lp);
            SGE_EXIT(1);
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/

      /* "-as server_name[,server_name,...]" */
      if (strcmp("-as", *spp) == 0) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);
         parse_name_list_to_cull("host to add", &lp, SH_Type, SH_name, *spp);
         add_host_of_type(lp, SGE_SUBMITHOST_LIST);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-astree", "-Astree file":  add sharetree */

      if ((strcmp("-astree", *spp) == 0) || (strcmp("-Astree", *spp) == 0)) {
         if(strcmp("-astree", *spp) == 0) { 
            sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
            sge_gdi_is_manager(uti_state_get_user_name());

            /* get the sharetree .. */
            what = lWhat("%T(ALL)", STN_Type);
            alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
            what = lFreeWhat(what);

            aep = lFirst(alp);
            answer_exit_if_not_recoverable(aep);
            if (answer_get_status(aep) != STATUS_OK) {
               fprintf(stderr, "%s", lGetString(aep, AN_text));
               spp++;
               continue;
            }
            alp = lFreeList(alp);
    
            ep = lFirst(lp);
            if (!(ep=edit_sharetree(ep)))
               continue;

            lp = lFreeList(lp);
         } else {
            char errstr[1024];
#ifdef QCONF_FLATFILE
            spooling_field *fields = sge_build_STN_field_list (0, 1);
#endif
            spp = sge_parser_get_next(spp);
           
#ifdef QCONF_FLATFILE
            fields_out[0] = NoName;
            ep = spool_flatfile_read_object(&alp, STN_Type, NULL,
                                            fields, fields_out, true,
                                            &qconf_name_value_list_sfi,
                                            SP_FORM_ASCII, NULL, *spp);
            
            if (answer_list_output(&alp)) {
               ep = lFreeElem (ep);
            }

            if (ep != NULL) {
               missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
            }

            FREE (fields);
            
            if (missing_field != NoName) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }
#else
            ep = read_sharetree(*spp, NULL, 0, errstr, 1, NULL);
#endif
               
            if (ep == NULL) {
               fprintf(stderr, errstr);
               
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE)) {
                  continue;
               }
            }      
         }
         
         newlp = lCreateList("sharetree add", STN_Type);
         lAppendElem(newlp, ep);

         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_ADD, &newlp, NULL, what);
         what = lFreeWhat(what);

         ep = lFirst(alp);
         answer_exit_if_not_recoverable(ep);
         if (answer_get_status(ep) == STATUS_OK)
            fprintf(stderr, MSG_TREE_CHANGEDSHARETREE);
         else
            fprintf(stderr, "%s", lGetString(ep, AN_text));
         newlp = lFreeList(newlp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-astnode node_path=shares[,...]"  create/modify sharetree node */
      /* "-mstnode node_path=shares[,...]"  modify sharetree node */

      if (((strcmp("-astnode", *spp) == 0) && ((opt=astnode_OPT) != 0)) ||
          ((strcmp("-mstnode", *spp) == 0) && ((opt=mstnode_OPT) != 0))) {
         int modified = 0;
         int print_usage = 0;

         /* no adminhost/manager check needed here */
         spp = sge_parser_get_next(spp);

         /* get the sharetree .. */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);
 
         ep = lFirst(lp);
         if (!ep && opt == astnode_OPT) {
            ep = lAddElemStr(&lp, STN_name, "Root", STN_Type);
            if (ep) lSetUlong(ep, STN_shares, 1);
         }
         if (!ep) {
            fprintf(stderr, MSG_TREE_NOSHARETREE);
            spp++;
            continue;
         }

         lString2List(*spp, &arglp, STN_Type, STN_name, ", ");

         for_each(argep, arglp) {
            lListElem *node = NULL;
            char *buf=NULL, *nodepath=NULL, *sharestr=NULL;
            int shares;
            ancestors_t ancestors;

            buf = sge_strdup(buf, lGetString(argep, STN_name));
            nodepath = sge_strtok(buf, "=");
            sharestr = sge_strtok(NULL, "");
            if (nodepath && sharestr && sscanf(sharestr, "%d", &shares) == 1) {
               memset(&ancestors, 0, sizeof(ancestors));
               node = search_named_node_path(ep, nodepath, &ancestors);
               if (!node && opt==astnode_OPT) {
                  char *c, *lc = NULL;
                  lListElem *pnode, *nnode;

                  /* scan for basename of nodepath */
                  for(c=nodepath; *c; c++)
                     if (*c == '/' || *c == '.')
                        lc = c;

                  /* search for parent node of new node */
                  if (lc && *nodepath && *(lc+1)) {
                     char savelc = *lc;
                     *lc = '\000';
                     if (lc == nodepath && savelc == '/') /* root? */
                        pnode = ep;
                     else
                        pnode = search_named_node_path(ep, nodepath,
                                                       &ancestors);
                     if (pnode) {
                        lList *children = lGetList(pnode, STN_children);
                        nnode = lAddElemStr(&children, STN_name, lc+1, STN_Type);
                        if (nnode && !lGetList(pnode, STN_children))
                           lSetList(pnode, STN_children, children);
                        free_ancestors(&ancestors);
                        memset(&ancestors, 0, sizeof(ancestors));
                        *lc = savelc;
                        node = search_named_node_path(ep, nodepath, &ancestors);
                        if (node != nnode) {
                           fprintf(stderr, MSG_TREE_CANTADDNODEXISNONUNIQUE_S, nodepath);
                           SGE_EXIT(1);
                        }
                     }
                  }
               }
               if (node) {
                  int i;
                  modified++;
                  lSetUlong(node, STN_shares, shares);
                  fprintf(stderr, MSG_TREE_SETTING);
                  for(i=0; i<ancestors.depth; i++)
                     fprintf(stderr, "/%s",
                             lGetString(ancestors.nodes[i], STN_name));
                  fprintf(stderr, "=%d\n", shares);
               } else 
                  fprintf(stderr, MSG_TREE_UNABLETOLACATEXINSHARETREE_S,
                          nodepath);
               free_ancestors(&ancestors);

            } else {
               fprintf(stderr, MSG_ANSWER_XISNOTVALIDSEENODESHARESLIST_S, *spp);
               print_usage = 1;
            }

            free(buf);
         }
         arglp = lFreeList(arglp);

         if (modified) {
            what = lWhat("%T(ALL)", STN_Type);
            alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_MOD, &lp, NULL, what);
            what = lFreeWhat(what);
            ep = lFirst(alp);
            answer_exit_if_not_recoverable(ep);
            if (answer_get_status(ep) == STATUS_OK)
               fprintf(stderr, MSG_TREE_MODIFIEDSHARETREE);
            else
               fprintf(stderr, "%s", lGetString(ep, AN_text));
            alp = lFreeList(alp);
         } else {
            fprintf(stderr, MSG_TREE_NOMIDIFIEDSHARETREE);
            if (print_usage)
               sge_usage(stderr);
            SGE_EXIT(1);
         }

         lp = lFreeList(lp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-au user_list list_name[,list_name,...]" */

      if (strcmp("-au", *spp) == 0) {

         /* no adminhost/manager check needed here */

         /* get user list */
         spp = sge_parser_get_next(spp);
         if (!*(spp+1)) {
            ERROR((SGE_EVENT, MSG_ANSWER_NOLISTNAMEPROVIDEDTOAUX_S, *spp));
            sge_usage(stderr);
            SGE_EXIT(1);
         }
         lString2List(*spp, &lp, UE_Type, UE_name, ",  ");
         
         /* get list_name list */
         spp = sge_parser_get_next(spp);
         lString2List(*spp, &arglp, US_Type, US_name, ", ");

         /* add all users/groups from lp to the acls in alp */
         sge_client_add_user(&alp, lp, arglp);

         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         arglp = lFreeList(arglp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-clearusage"  clear sharetree usage */

      if (strcmp("-clearusage", *spp) == 0) {
         lList *lp2=NULL;

         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());

         /* get user list */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_USER_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);
 
         /* get project list */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_GET, &lp2, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);
 
         /* clear user usage */
         for_each(ep, lp) {
            lSetList(ep, UP_usage, NULL);
            lSetList(ep, UP_project, NULL);
         }

         /* clear project usage */
         for_each(ep, lp2) {
            lSetList(ep, UP_usage, NULL);
            lSetList(ep, UP_project, NULL);
         }

         /* update user usage */
         if (lp) {
            alp = sge_gdi(SGE_USER_LIST, SGE_GDI_MOD, &lp, NULL, NULL);
            answer_list_on_error_print_or_exit(&alp, stderr);
         }

         /* update project usage */
         if (lp2) {
            alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_MOD, &lp2, NULL, NULL);
            answer_list_on_error_print_or_exit(&alp, stderr);
         }

         alp = lFreeList(alp);
         lp = lFreeList(lp);
         lp2 = lFreeList(lp2);
         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-dcal calendar_name" */

      if (strcmp("-dcal", *spp) == 0) {
         /* no adminhost/manager check needed here */
         spp = sge_parser_get_next(spp);
         ep = lCreateElem(CAL_Type);
         lSetString(ep, CAL_name, *spp);
         lp = lCreateList("cal's to del", CAL_Type);
         lAppendElem(lp, ep);
         alp = sge_gdi(SGE_CALENDAR_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-dckpt ckpt_name" */

      if (strcmp("-dckpt", *spp) == 0) {
         /* no adminhost/manager check needed here */
         spp = sge_parser_get_next(spp);

         ep = lCreateElem(CK_Type);
         lSetString(ep, CK_name, *spp);
         lp = lCreateList("ckpt interfaces to del", CK_Type);
         lAppendElem(lp, ep);
         alp = sge_gdi(SGE_CKPT_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/

      /* "-de server_name[,server_name,...]" */
      if (strcmp("-de", *spp) == 0) {
         /* no adminhost/manager check needed here */
         spp = sge_parser_get_next(spp);
         parse_name_list_to_cull("host to del", &lp, EH_Type, EH_name, *spp);
         del_host_of_type(lp, SGE_EXECHOST_LIST);
         lp = lFreeList(lp);
         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/

      /* "-dh server_name[,server_name,...]" */
      if (strcmp("-dh", *spp) == 0) {
         /* no adminhost/manager check needed here */
         spp = sge_parser_get_next(spp);
         parse_name_list_to_cull("host to del", &lp, AH_Type, AH_name, *spp);
         del_host_of_type(lp, SGE_ADMINHOST_LIST);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-dm user_list" */

      if (strcmp("-dm", *spp) == 0) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);

         lString2List(*spp, &lp, MO_Type, MO_name, ", ");
         alp = sge_gdi(SGE_MANAGER_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-do user_list" */

      if (strcmp("-do", *spp) == 0) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);

         lString2List(*spp, &lp, MO_Type, MO_name, ", ");
         alp = sge_gdi(SGE_OPERATOR_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-dp pe-name" */

      if (strcmp("-dp", *spp) == 0) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);

         ep = lCreateElem(PE_Type);
         lSetString(ep, PE_name, *spp);
         lp = lCreateList("pe's to del", PE_Type);
         lAppendElem(lp, ep);
         alp = sge_gdi(SGE_PE_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-ds server_name[,server_name,...]" */
      if (strcmp("-ds", *spp) == 0) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);
         parse_name_list_to_cull("host to del", &lp, SH_Type, SH_name, *spp);
         del_host_of_type(lp, SGE_SUBMITHOST_LIST);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-du user_list list_name[,list_name,...]" */

      if (strcmp("-du", *spp) == 0) {
         
         /* no adminhost/manager check needed here */

         /* get user list */
         spp = sge_parser_get_next(spp);
         if (!*(spp+1)) {
            ERROR((SGE_EVENT, MSG_ANSWER_NOLISTNAMEPROVIDEDTODUX_S, *spp));
            sge_usage(stderr);
            SGE_EXIT(1);
         }
         lString2List(*spp, &lp, UE_Type, UE_name, ",  ");
         
         /* get list_name list */
         spp = sge_parser_get_next(spp);
         lString2List(*spp, &arglp, US_Type, US_name, ", ");

         /* remove users/groups from lp from the acls in alp */
         sge_client_del_user(&alp, lp, arglp);
         answer_list_on_error_print_or_exit(&alp, stderr);
         lp = lFreeList(lp);
         alp = lFreeList(alp);
         arglp = lFreeList(arglp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-dul list_name_list" */

      if (strcmp("-dul", *spp) == 0) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);

         lString2List(*spp, &lp, US_Type, US_name, ", ");
         alp = sge_gdi(SGE_USERSET_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-duser user,..." */

      if (strcmp("-duser", *spp) == 0) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);

         lString2List(*spp, &lp, UP_Type, UP_name, ", ");
         alp = sge_gdi(SGE_USER_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-dprj project,..." */

      if (strcmp("-dprj", *spp) == 0) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);

         lString2List(*spp, &lp, UP_Type, UP_name, ", ");
         alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-dstnode node_path[,...]"  delete sharetree node(s) */

      if (strcmp("-dstnode", *spp) == 0) {
         int modified = 0;
         /* no adminhost/manager check needed here */
         spp = sge_parser_get_next(spp);

         /* get the sharetree .. */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);
 
         ep = lFirst(lp);
         if (!ep) {
            fprintf(stderr, MSG_TREE_NOSHARETREE);
            spp++;
            continue;
         }

         lString2List(*spp, &arglp, STN_Type, STN_name, ", ");

         for_each(argep, arglp) {

            lListElem *node = NULL;
            const char *nodepath = NULL;
            ancestors_t ancestors;

            nodepath = lGetString(argep, STN_name);
            if (nodepath) {
               memset(&ancestors, 0, sizeof (ancestors_t));
               node = search_named_node_path(ep, nodepath, &ancestors);
               if (node) {
                  if (lGetList(node, STN_children) == NULL) {
                     if (ancestors.depth > 0) {
                        int i;
                        lList *siblings = NULL;
                        lListElem *pnode = NULL;
                        modified++;
                        if (ancestors.depth == 1)
                           pnode = ep;
                        else
                           pnode = ancestors.nodes[ancestors.depth-2];
                        fprintf(stderr, MSG_TREE_REMOVING);
                        for(i=0; i<ancestors.depth; i++)
                           fprintf(stderr, "/%s",
                                   lGetString(ancestors.nodes[i], STN_name));
                        fprintf(stderr, "\n");
                        siblings = lGetList(pnode, STN_children);
                        lRemoveElem(siblings, node);
                        if (lGetNumberOfElem(siblings) == 0)
                           lSetList(pnode, STN_children, NULL);
                     } else {
                        fprintf(stderr, MSG_TREE_CANTDELROOTNODE);
                     }
                  } else {
                     fprintf(stderr, MSG_TREE_CANTDELNODESWITHCHILD);
                  }
               } else 
                  fprintf(stderr, MSG_TREE_UNABLETOLACATEXINSHARETREE_S,
                          nodepath);
               free_ancestors(&ancestors);

            }
         }
         arglp = lFreeList(arglp);

         if (modified) {
            what = lWhat("%T(ALL)", STN_Type);
            alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_MOD, &lp, NULL, what);
            what = lFreeWhat(what);
            ep = lFirst(alp);
            answer_exit_if_not_recoverable(ep);
            if (answer_get_status(ep) == STATUS_OK)
               fprintf(stderr, MSG_TREE_MODIFIEDSHARETREE);
            else
               fprintf(stderr, "%s", lGetString(ep, AN_text));
            alp = lFreeList(alp);
         } else {
            fprintf(stderr, MSG_TREE_NOMIDIFIEDSHARETREE);
            SGE_EXIT(1);
         }

         lp = lFreeList(lp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-dstree" */

      if (strcmp("-dstree", *spp) == 0) {
         /* no adminhost/manager check needed here */
         
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_DEL, NULL, NULL, NULL);
         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-help" */

      if (strcmp("-help", *spp) == 0) {
         sge_usage(stdout);
         SGE_EXIT(0);
      }

/*----------------------------------------------------------------------------*/
      /* "-ks" */

      if (strcmp("-ks", *spp) == 0) {
         /* no adminhost/manager check needed here */

         alp = gdi_kill(NULL, uti_state_get_default_cell(), 0, SCHEDD_KILL);
         for_each(aep, alp) {
            answer_exit_if_not_recoverable(aep);
            if (answer_get_status(aep) != STATUS_OK)
               sge_parse_return = 1;
            answer_print_text(aep, stderr, NULL, NULL);
         }

         alp = lFreeList(alp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-km" */

      if (strcmp("-km", *spp) == 0) {
         /* no adminhost/manager check needed here */
         alp = gdi_kill(NULL, uti_state_get_default_cell(), 0, MASTER_KILL);
         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* -kec <id> ... */
      /* <id> may be "all" */
      /* parse before -ke[j] */

      if (strncmp("-kec", *spp, 4) == 0) {
         int opt = EVENTCLIENT_KILL;
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);
         /* found namelist -> process */
         if(strcmp(*spp, "all") == 0) { /* kill all dynamic event clients (EV_ID_ANY) */
            alp = gdi_kill(NULL, uti_state_get_default_cell(), 0, opt);
         } else {
            lString2List(*spp, &lp, ID_Type, ID_str, ", ");
            alp = gdi_kill(lp, uti_state_get_default_cell(), 0, opt);
         }      
         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         lp = lFreeList(lp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* -ke[j] <host> ... */
      /* <host> may be "all" */
      if (strncmp("-k", *spp, 2) == 0) {
         int opt = EXECD_KILL;
         /* no adminhost/manager check needed here */

         cp = (*spp) + 2;
         switch (*cp++) {
            case 'e':
               break;
            default:
               ERROR((SGE_EVENT, MSG_ANSWER_XISNOTAVALIDOPTIONY_SU, *spp, u32c(uti_state_get_mewho())));
               sge_usage(stderr);
               SGE_EXIT(1);
         }

         if (*cp == 'j') {
            cp++;
            opt |= JOB_KILL;
         }

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
         } else {
            if (sge_error_and_exit(MSG_HOST_NEEDAHOSTNAMEORALL))
               continue;
         }

         if(strcmp(*spp, "all") == 0) { /* kill all dynamic event clients (EV_ID_ANY) */
            alp = gdi_kill(NULL, uti_state_get_default_cell(), 0, opt);
         } else {   
            /* found namelist -> process */
            lString2List(*spp, &lp, EH_Type, EH_name, ", ");
            alp = gdi_kill(lp, uti_state_get_default_cell(), 0, opt);
         }

         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         lp = lFreeList(lp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-mc" */

      if (strcmp("-mc", *spp) == 0) {
         lList *answer_list = NULL;

         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());

         centry_list_modify(&answer_list);
         show_gdi_request_answer_list(answer_list);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-mcal cal-name" */

      if ((strcmp("-mcal", *spp) == 0) || 
          (strcmp("-Mcal", *spp) == 0)) {
         if (!strcmp("-mcal", *spp)) {
            sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
            sge_gdi_is_manager(uti_state_get_user_name());

            spp = sge_parser_get_next(spp);
           
            where = lWhere("%T( %I==%s )", CAL_Type, CAL_name, *spp);
            what = lWhat("%T(ALL)", CAL_Type);
            alp = sge_gdi(SGE_CALENDAR_LIST, SGE_GDI_GET, &lp, where, what);
            where = lFreeWhere(where);
            what = lFreeWhat(what);

            aep = lFirst(alp);
            answer_exit_if_not_recoverable(aep);
            if (answer_get_status(aep) != STATUS_OK) {
              fprintf(stderr, "%s", lGetString(aep, AN_text));
               spp++;
               continue;
            }
            alp = lFreeList(alp);

            if (!lp) {
               fprintf(stderr, MSG_CALENDAR_XISNOTACALENDAR_S, *spp);
               SGE_EXIT(1);
            }

            ep = lFirst(lp);
#ifdef QCONF_FLATFILE
            filename = (char *)spool_flatfile_write_object(&alp, ep, false,
                                                 CAL_fields, &qconf_sfi,
                                                 SP_DEST_TMP, SP_FORM_ASCII,
                                                 NULL, false);
            
            if (answer_list_output(&alp)) {
               sge_error_and_exit(NULL);
            }

#else
            filename = write_cal(0, 1, ep);
#endif
            ep = lFreeElem(ep);
            
            /* edit this file */
            status = sge_edit(filename);
            if (status < 0) {
               unlink(filename);
               if (sge_error_and_exit(MSG_PARSE_EDITFAILED))
                  continue;
            }

            if (status > 0) {
               unlink(filename);
               if (sge_error_and_exit(MSG_FILE_FILEUNCHANGED))
                  continue;
            }

            /* read it in again */
#ifdef QCONF_FLATFILE
            fields_out[0] = NoName;
            ep = spool_flatfile_read_object(&alp, CAL_Type, NULL,
                                            CAL_fields, fields_out, true, &qconf_sfi,
                                            SP_FORM_ASCII, NULL, filename);
            unlink(filename);
            
            if (answer_list_output(&alp)) {
               ep = lFreeElem (ep);
            }

            if (ep != NULL) {
               missing_field = spool_get_unprocessed_field (CAL_fields, fields_out, &alp);
            }

            if (missing_field != NoName) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }
#else
            ep = cull_read_in_cal(NULL, filename, 0, 0, NULL, NULL); 
#endif
               
            if (ep == NULL) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE)) {
                  continue;
               }
            }
         } else {
            spp = sge_parser_get_next(spp);
           
#ifdef QCONF_FLATFILE
            fields_out[0] = NoName;
            ep = spool_flatfile_read_object(&alp, CAL_Type, NULL,
                                            CAL_fields, fields_out, true, &qconf_sfi,
                                            SP_FORM_ASCII, NULL, *spp);
            
            if (answer_list_output(&alp)) {
               ep = lFreeElem (ep);
            }

            if (ep != NULL) {
               missing_field = spool_get_unprocessed_field (CAL_fields, fields_out, &alp);
            }

            if (missing_field != NoName) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }
#else
            ep = cull_read_in_cal(NULL, *spp, 0, 0, NULL, NULL); 
#endif
               
            if (ep == NULL) 
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE)) {
                  continue;
               }
         }

         /* send it to qmaster */
         lp = lCreateList("calendar to add", CAL_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_CALENDAR_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-Mc complex file" */

      if (strcmp("-Mc", *spp) == 0) {
         lList *answer_list = NULL;
         char* file = NULL;

         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN);
         }
         centry_list_modify_from_file(&answer_list, file);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-mckpt ckpt_name" or "-Mckpt fname" */

      if ((strcmp("-mckpt", *spp) == 0) || 
          (strcmp("-Mckpt", *spp) == 0)) {
         if (strcmp("-mckpt", *spp) == 0) {
            sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
            sge_gdi_is_manager(uti_state_get_user_name());

            spp = sge_parser_get_next(spp);

            /* get last version of this pe from qmaster */
            where = lWhere("%T( %I==%s )", CK_Type, CK_name, *spp);
            what = lWhat("%T(ALL)", CK_Type);
            alp = sge_gdi(SGE_CKPT_LIST, SGE_GDI_GET, &lp, where, what);
            lFreeWhere(where);
            lFreeWhat(what);

            aep = lFirst(alp);
            answer_exit_if_not_recoverable(aep);
            if (answer_get_status(aep) != STATUS_OK) {
              fprintf(stderr, "%s", lGetString(aep, AN_text));
               spp++;
               continue;
            }
            alp = lFreeList(alp);

            if (!lp) {
               fprintf(stderr, MSG_CKPT_XISNOTCHKPINTERFACEDEF_S, *spp);
               SGE_EXIT(1);
            }

            ep = lFirst(lp);
#ifdef QCONF_FLATFILE
            filename = (char *)spool_flatfile_write_object(&alp, ep, false,
                                                 CK_fields, &qconf_sfi,
                                                 SP_DEST_TMP, SP_FORM_ASCII,
                                                 NULL, false);
            
            if (answer_list_output(&alp)) {
               sge_error_and_exit(NULL);
            }
#else
            filename = write_ckpt(0, 1, ep);
#endif
            ep = lFreeElem(ep);
            
            /* edit this file */
            status = sge_edit(filename);
            if (status < 0) {
               unlink(filename);
               if (sge_error_and_exit(MSG_PARSE_EDITFAILED))
                  continue;
            }

            if (status > 0) {
               unlink(filename);
               if (sge_error_and_exit(MSG_FILE_FILEUNCHANGED))
                  continue;
            }

            /* read it in again */
#ifdef QCONF_FLATFILE
            fields_out[0] = NoName;
            ep = spool_flatfile_read_object(&alp, CK_Type, NULL,
                                            CK_fields, fields_out, true, &qconf_sfi,
                                            SP_FORM_ASCII, NULL, filename);
            unlink(filename);
            
            if (answer_list_output(&alp)) {
               ep = lFreeElem(ep);
            }

            if (ep != NULL) {
               missing_field = spool_get_unprocessed_field (CK_fields, fields_out, &alp);
            }

            if (missing_field != NoName) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }

            if ((ep != NULL) && (ckpt_validate(ep, &alp) != STATUS_OK)) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }
#else
            ep = cull_read_in_ckpt(NULL, filename, 0, 0, NULL, NULL);
#endif
               
            if (ep == NULL) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE)) {
                  continue;
               }
            }
         } else {
            spp = sge_parser_get_next(spp);

#ifdef QCONF_FLATFILE
            fields_out[0] = NoName;
            ep = spool_flatfile_read_object(&alp, CK_Type, NULL,
                                            CK_fields, fields_out, true, &qconf_sfi,
                                            SP_FORM_ASCII, NULL, *spp);
            
            if (answer_list_output(&alp)) {
               ep = lFreeElem(ep);
            }

            if (ep != NULL) {
               missing_field = spool_get_unprocessed_field (CK_fields, fields_out, &alp);
            }

            if (missing_field != NoName) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }

            if ((ep != NULL) && (ckpt_validate(ep, &alp) != STATUS_OK)) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }
#else
            ep = cull_read_in_ckpt(NULL, *spp, 0, 0, NULL, NULL);
#endif

            if (ep == NULL) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE)) {
                  continue;
               }
            }
         }

         /* send it to qmaster */
         lp = lCreateList("CKPT list to add", CK_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_CKPT_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-Me fname" */
      if (strcmp("-Me", *spp) == 0) {
#ifdef QCONF_FLATFILE
         spooling_field *fields = sge_build_EH_field_list (false, false, false);
#endif
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp); 

         hep = lCreateElem(EH_Type);

         /* read file */
         lp = lCreateList("exechosts to add", EH_Type);
#ifdef QCONF_FLATFILE
         fields_out[0] = NoName;
         ep = spool_flatfile_read_object(&alp, EH_Type, NULL,
                                         fields, fields_out, true, &qconf_sfi,
                                         SP_FORM_ASCII, NULL, *spp);
         
         if (answer_list_output(&alp)) {
            ep = lFreeElem (ep);
         }

         if (ep != NULL) {
            missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
         }

         FREE (fields);
         
         if (missing_field != NoName) {
            ep = lFreeElem(ep);
            answer_list_output(&alp);
         }
#else
         ep = cull_read_in_host(NULL, *spp, CULL_READ_MINIMUM, EH_name, 
               NULL, NULL);
#endif

         if (ep == NULL) {
            fprintf(stderr, MSG_ANSWER_INVALIDFORMAT); 
            SGE_EXIT(1);
         }

         lAppendElem(lp, ep);

         /* test host name */
         switch (sge_resolve_host(ep, EH_name)) {
         case CL_RETVAL_OK:
            break;
         default:
            fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetHost(ep, EH_name));
            ep = lFreeElem(ep);
            SGE_EXIT(1);
            break;
         }

         alp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-me [server_name,...]" */

      if (strcmp("-me", *spp) == 0) {
         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());
         
         spp = sge_parser_get_next(spp);
         parse_name_list_to_cull("hosts to change", &arglp, EH_Type, EH_name, 
            *spp);

         for_each (argep, arglp) {
            /* resolve hostname */
            switch (sge_resolve_host(argep, EH_name)) {
            case CL_RETVAL_OK:
               break;
            default:
               fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetHost(argep, EH_name));
               argep = lFreeElem(argep);
               SGE_EXIT(1);
               break;
            }
            host = lGetHost(argep, EH_name);

            /* get the existing host entry .. */
            where = lWhere("%T( %Ih=%s )", EH_Type, EH_name, host);
            what = lWhat("%T(ALL)", EH_Type);
            alp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_GET, &lp, where, what);
            where = lFreeWhere(where);
            what = lFreeWhat(what);

            aep = lFirst(alp);
            answer_exit_if_not_recoverable(aep);
            if (answer_get_status(aep) != STATUS_OK) {
               fprintf(stderr, "%s", lGetString(aep, AN_text));
               spp++;
               continue;
            }

            if (!lp) {
               fprintf(stderr, MSG_EXEC_XISNOTANEXECUTIONHOST_S, host);
               spp++;
               continue;
            }
            alp = lFreeList(alp);

            ep=edit_exechost(lFirst(lp));
            if (!ep)
               continue;
            lp = lFreeList(lp);
            lp = lCreateList("host to mod", EH_Type);
            lAppendElem(lp, ep);
            
            alp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_MOD, &lp, NULL, NULL);
            ep = lFirst(alp);
            answer_exit_if_not_recoverable(ep);
            if (answer_get_status(ep) == STATUS_OK)
               fprintf(stderr, MSG_EXEC_HOSTENTRYOFXCHANGEDINEXECLIST_S,
                      host);
            else
               fprintf(stderr, "%s", lGetString(ep, AN_text));
            spp++;
         }
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-mp pe_name" */

      if ((strcmp("-mp", *spp) == 0) || 
          (strcmp("-Mp", *spp) == 0)) {
#ifdef QCONF_FLATFILE
         spooling_field *fields = sge_build_PE_field_list (false, false);
#endif
         if (!strcmp("-mp", *spp)) {
            sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
            sge_gdi_is_manager(uti_state_get_user_name());
         
            spp = sge_parser_get_next(spp);

            /* get last version of this pe from qmaster */
            where = lWhere("%T( %I==%s )", PE_Type, PE_name, *spp);
            what = lWhat("%T(ALL)", PE_Type);
            alp = sge_gdi(SGE_PE_LIST, SGE_GDI_GET, &lp, where, what);
            where = lFreeWhere(where);
            what = lFreeWhat(what);

            aep = lFirst(alp);
            answer_exit_if_not_recoverable(aep);
            if (answer_get_status(aep) != STATUS_OK) {
               fprintf(stderr, "%s", lGetString(aep, AN_text));
#ifdef QCONF_FLATFILE
               FREE (fields);
#endif
               spp++;
               continue;
            }
            alp = lFreeList(alp);

            if (!lp) {
               fprintf(stderr, MSG_PARALLEL_XNOTAPARALLELEVIRONMENT_S, *spp);
#ifdef QCONF_FLATFILE
               FREE (fields);
#endif
               SGE_EXIT(1);
            }

            ep = lFirst(lp);

            /* write pe to temp file */
#ifdef QCONF_FLATFILE
            filename = (char *)spool_flatfile_write_object(&alp, ep, false,
                                                 fields, &qconf_sfi,
                                                 SP_DEST_TMP, SP_FORM_ASCII,
                                                 NULL, false);
            
            if (answer_list_output(&alp)) {
               FREE (fields);
               sge_error_and_exit(NULL);
            }
#else
            filename = write_pe(0, 1, ep);
#endif
            ep = lFreeElem(ep);
            
            /* edit this file */
            status = sge_edit(filename);
            if (status < 0) {
               unlink(filename);
               if (sge_error_and_exit(MSG_PARSE_EDITFAILED)) {
#ifdef QCONF_FLATFILE
                  FREE (fields);
#endif
                  continue;
               }
            }

            if (status > 0) {
               unlink(filename);
               if (sge_error_and_exit(MSG_FILE_FILEUNCHANGED)) {
#ifdef QCONF_FLATFILE
                     FREE (fields);
#endif
                     continue;
               }
            }

#ifdef QCONF_FLATFILE
            fields_out[0] = NoName;
            ep = spool_flatfile_read_object(&alp, PE_Type, NULL,
                                            fields, fields_out, true, &qconf_sfi,
                                            SP_FORM_ASCII, NULL, filename);
            
            unlink(filename);
            
            if (answer_list_output(&alp)) {
               ep = lFreeElem(ep);
            }

            if (ep != NULL) {
               missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
            }

            FREE (fields);
            
            if (missing_field != NoName) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }

            if ((ep != NULL) && (pe_validate(ep, &alp, 0) != STATUS_OK)) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }
#else
            /* read it in again */
            ep = cull_read_in_pe(NULL, filename, 0, 0, NULL, NULL);
#endif
               
            if (ep == NULL) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE)) {
                  continue;
               }
            }

            FREE(filename);
         } else {
            spp = sge_parser_get_next(spp);

#ifdef QCONF_FLATFILE
            fields_out[0] = NoName;
            ep = spool_flatfile_read_object(&alp, PE_Type, NULL,
                                            fields, fields_out, true, &qconf_sfi,
                                            SP_FORM_ASCII, NULL, *spp);
            
            if (answer_list_output(&alp)) {
               ep = lFreeElem(ep);
            }

            if (ep != NULL) {
               missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
            }

            FREE (fields);
            
            if (missing_field != NoName) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }

            if ((ep != NULL) && (pe_validate(ep, &alp, 0) != STATUS_OK)) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }
#else
            ep = cull_read_in_pe(NULL, *spp, 0, 0, NULL, NULL); 
#endif
               
            if (ep == NULL) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE)) {
                  continue;
               }
            }
         }

         /* send it to qmaster */
         lp = lCreateList("PE list to add", PE_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_PE_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
   if (strcmp("-sobjl", *spp) == 0) {
      dstring object_name = DSTRING_INIT;
      dstring attribute_pattern = DSTRING_INIT;
      dstring value_pattern = DSTRING_INIT;
      bool handle_cqueue;
      bool handle_domain;
      bool handle_qinstance;
      bool handle_exechost;

      spp = sge_parser_get_next(spp);
      sge_dstring_sprintf(&object_name, "%s", *spp);
      spp = sge_parser_get_next(spp);
      sge_dstring_sprintf(&attribute_pattern, "%s", *spp);
      spp = sge_parser_get_next(spp);
      sge_dstring_sprintf(&value_pattern, "%s", *spp);

      handle_cqueue = !strcmp(sge_dstring_get_string(&object_name), "queue");
      handle_domain = !strcmp(sge_dstring_get_string(&object_name), "queue_domain");
      handle_qinstance = !strcmp(sge_dstring_get_string(&object_name), "queue_instance");
      handle_exechost = !strcmp(sge_dstring_get_string(&object_name), "exechost");

      if (handle_exechost) {
         lEnumeration *what = NULL;
         lList *list = NULL;
         lListElem *elem = NULL;
         lList *answer_list = NULL;

         what = lWhat("%T(ALL)", EH_Type);
         answer_list = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_GET, &list, NULL, what);
         what = lFreeWhat(what);

         for_each(elem, list) {
            const char *hostname = NULL;
            bool already_handled = false;

            /*
             * hostname
             */ 
            hostname = lGetHost(elem, EH_name);
            if (!fnmatch(sge_dstring_get_string(&attribute_pattern), SGE_ATTR_HOSTNAME, 0) &&
                !fnmatch(sge_dstring_get_string(&value_pattern), hostname, 0)) {
               printf("%s\n", hostname);
               already_handled = true;
            }

            /*
             * load scaling list
             */
            if (!already_handled && 
                !fnmatch(sge_dstring_get_string(&attribute_pattern), SGE_ATTR_LOAD_SCALING, 0)) {
               dstring value = DSTRING_INIT;
               lList *value_list = NULL;
               lListElem *value_elem = NULL;
               const char *value_string = NULL;

               value_list = lGetList(elem, EH_scaling_list);
               value_elem = lFirst(value_list);

               while (value_elem != NULL) {
                  sge_dstring_sprintf_append(&value, "%s=%.10g", lGetString(value_elem, HS_name), lGetDouble(value_elem, HS_value));
                  value_elem = lNext(value_elem);
                  if (value_elem != NULL) {
                     sge_dstring_sprintf_append(&value, ",");
                  }
               }
               value_string = sge_dstring_get_string(&value);
               if (value_string == NULL) {
                  sge_dstring_sprintf(&value, "NONE");
                  value_string = sge_dstring_get_string(&value);
               }
               if (!fnmatch(sge_dstring_get_string(&value_pattern), value_string, 0)) {
                  printf("%s\n", hostname);
                  already_handled = true;
               }
               sge_dstring_free(&value);
            }

            /*
             * complex_values list
             */
            if (!already_handled &&
                !fnmatch(sge_dstring_get_string(&attribute_pattern), SGE_ATTR_COMPLEX_VALUES, 0)) {
               dstring value = DSTRING_INIT;
               lList *value_list = NULL;
               lListElem *value_elem = NULL;
               const char *value_string = NULL;

               value_list = lGetList(elem, EH_consumable_config_list);
               value_elem = lFirst(value_list);

               while (value_elem != NULL) {
                  sge_dstring_sprintf_append(&value, "%s=", lGetString(value_elem, CE_name));
                  if (lGetString(value_elem, CE_stringval) != NULL) {
                     sge_dstring_sprintf_append(&value, "%s", lGetString(value_elem, CE_stringval));
                  } else {
                     sge_dstring_sprintf_append(&value, "%f", lGetString(value_elem, CE_doubleval));
                  }
                  value_elem = lNext(value_elem);
                  if (value_elem != NULL) {
                     sge_dstring_sprintf_append(&value, ",");
                  }
               }
               value_string = sge_dstring_get_string(&value);
               if (value_string == NULL) {
                  sge_dstring_sprintf(&value, "NONE");
                  value_string = sge_dstring_get_string(&value);
               }
               if (!fnmatch(sge_dstring_get_string(&value_pattern), value_string, 0)) {
                  printf("%s\n", hostname);
                  already_handled = true;
               }
               sge_dstring_free(&value);
            }

            /*
             * load_values list
             */
            if (!already_handled &&
                !fnmatch(sge_dstring_get_string(&attribute_pattern), SGE_ATTR_LOAD_VALUES, 0)) {
               dstring value = DSTRING_INIT;
               lList *value_list = NULL;
               lListElem *value_elem = NULL;
               const char *value_string = NULL;

               value_list = lGetList(elem, EH_load_list);
               value_elem = lFirst(value_list);

               while (value_elem != NULL) {
                  sge_dstring_sprintf_append(&value, "%s=", lGetString(value_elem, HL_name));
                  sge_dstring_sprintf_append(&value, "%s", lGetString(value_elem, HL_value));
                  value_elem = lNext(value_elem);
                  if (value_elem != NULL) {
                     sge_dstring_sprintf_append(&value, ",");
                  }
               }
               value_string = sge_dstring_get_string(&value);
               if (value_string == NULL) {
                  sge_dstring_sprintf(&value, "NONE");
                  value_string = sge_dstring_get_string(&value);
               }
               if (!fnmatch(sge_dstring_get_string(&value_pattern), value_string, 0)) {
                  printf("%s\n", hostname);
                  already_handled = true;
               }
               sge_dstring_free(&value);
            }

            /*
             * processors 
             */
            if (!already_handled &&
                !fnmatch(sge_dstring_get_string(&attribute_pattern), SGE_ATTR_PROCESSORS, 0)) {
               dstring value = DSTRING_INIT;
               const char *value_string = NULL;

               sge_dstring_sprintf(&value, "%d", (int) lGetUlong(elem, EH_processors));
               value_string = sge_dstring_get_string(&value);
               if (!fnmatch(sge_dstring_get_string(&value_pattern), value_string, 0)) {
                  printf("%s\n", hostname);
                  already_handled = true;
               }
               sge_dstring_free(&value);
            }


            /*
             * user_lists list
             */
            if (!already_handled &&
                !fnmatch(sge_dstring_get_string(&attribute_pattern), SGE_ATTR_USER_LISTS, 0)) {
               dstring value = DSTRING_INIT;
               lList *value_list = NULL;
               lListElem *value_elem = NULL;
               const char *value_string = NULL;

               value_list = lGetList(elem, EH_acl);
               value_elem = lFirst(value_list);

               while (value_elem != NULL) {
                  sge_dstring_sprintf_append(&value, "%s", lGetString(value_elem, US_name));
                  value_elem = lNext(value_elem);
                  if (value_elem != NULL) {
                     sge_dstring_sprintf_append(&value, " ");
                  }
               }
               value_string = sge_dstring_get_string(&value);
               if (value_string == NULL) {
                  sge_dstring_sprintf(&value, "NONE");
                  value_string = sge_dstring_get_string(&value);
               }
               if (!fnmatch(sge_dstring_get_string(&value_pattern), value_string, 0)) {
                  printf("%s\n", hostname);
                  already_handled = true;
               }
               sge_dstring_free(&value);
            }

            /*
             * user_lists list
             */
            if (!already_handled &&
                !fnmatch(sge_dstring_get_string(&attribute_pattern), SGE_ATTR_XUSER_LISTS, 0)) {
               dstring value = DSTRING_INIT;
               lList *value_list = NULL;
               lListElem *value_elem = NULL;
               const char *value_string = NULL;

               value_list = lGetList(elem, EH_xacl);
               value_elem = lFirst(value_list);

               while (value_elem != NULL) {
                  sge_dstring_sprintf_append(&value, "%s", lGetString(value_elem, US_name));
                  value_elem = lNext(value_elem);
                  if (value_elem != NULL) {
                     sge_dstring_sprintf_append(&value, " ");
                  }
               }
               value_string = sge_dstring_get_string(&value);
               if (value_string == NULL) {
                  sge_dstring_sprintf(&value, "NONE");
                  value_string = sge_dstring_get_string(&value);
               }
               if (!fnmatch(sge_dstring_get_string(&value_pattern), value_string, 0)) {
                  printf("%s\n", hostname);
                  already_handled = true;
               }
               sge_dstring_free(&value);
            }
         }
      } else if (handle_cqueue || handle_domain || handle_qinstance) {
         lEnumeration *what = NULL;
         lList *list = NULL;
         lListElem *elem = NULL;
         lList *answer_list = NULL;

         what = lWhat("%T(ALL)", CQ_Type);
         answer_list = sge_gdi(SGE_CQUEUE_LIST, SGE_GDI_GET, &list, NULL, what);
         what = lFreeWhat(what);

         for_each(elem, list) {
            int index = 0;
            bool already_handled = false;

            /*
             * Handle special case: qname
             */
            if (!fnmatch(sge_dstring_get_string(&attribute_pattern),
                         SGE_ATTR_QNAME, 0)) {
               if (handle_cqueue &&
                   !fnmatch(sge_dstring_get_string(&value_pattern), lGetString(elem, CQ_name), 0)) {
                  printf("%s\n", lGetString(elem, CQ_name));
                  already_handled = true;
               }
            }

            /*
             * Handle special case: hostlist
             */
            if (!already_handled &&
                !fnmatch(sge_dstring_get_string(&attribute_pattern),
                         SGE_ATTR_HOST_LIST, 0)) {
               dstring value = DSTRING_INIT;
               const lList *hostref_list = lGetList(elem, CQ_hostlist);

               if (hostref_list != NULL) {
                  href_list_append_to_dstring(hostref_list, &value);
               } else {
                  sge_dstring_sprintf(&value, "NONE");
               }
               if (handle_cqueue &&
                   !fnmatch(sge_dstring_get_string(&value_pattern), sge_dstring_get_string(&value), 0)) {
                  printf("%s\n", lGetString(elem, CQ_name));
                  already_handled = true;
               }
               sge_dstring_free(&value);
            }

            /*
             * Handle all other CQ attributes
             */
            while (!already_handled &&
                   cqueue_attribute_array[index].cqueue_attr != NoName) {
               if (!fnmatch(sge_dstring_get_string(&attribute_pattern),
                            cqueue_attribute_array[index].name, 0)) {
                  dstring value = DSTRING_INIT;
                  lList *attribute_list = lGetList(elem, cqueue_attribute_array[index].cqueue_attr);
                  lListElem *attribute;

                  already_handled = false;
                  for_each(attribute, attribute_list) {
                     const lDescr *descr = lGetListDescr(attribute_list);
                     lList *tmp_attribute_list = lCreateList("", descr);
                     lListElem *tmp_attribute = lCopyElem(attribute);
                     const char *host_hgroup = lGetHost(attribute, cqueue_attribute_array[index].href_attr);
                     bool is_cqueue;
                     bool is_domain;
                     bool is_qinstance;

                     is_cqueue = !strcmp(host_hgroup, HOSTREF_DEFAULT);
                     is_domain = false;
                     if (!is_cqueue) {
                        is_domain = is_hgroup_name(host_hgroup);
                     }
                     is_qinstance = (!is_domain && !is_cqueue);

                     lAppendElem(tmp_attribute_list, tmp_attribute);
                     lSetHost(tmp_attribute,
                              cqueue_attribute_array[index].href_attr,
                              HOSTREF_DEFAULT);

                     attr_list_append_to_dstring(tmp_attribute_list,
                                                 &value, descr, cqueue_attribute_array[index].href_attr,
                                                 cqueue_attribute_array[index].value_attr);

                     if (!fnmatch(sge_dstring_get_string(&value_pattern), sge_dstring_get_string(&value), 0)) {
                        if (handle_cqueue && is_cqueue) {
                           printf("%s\n", lGetString(elem, CQ_name));
                        } else if ((handle_domain && is_domain) || (handle_qinstance && is_qinstance)) {
                           printf("%s@%s\n", lGetString(elem, CQ_name), host_hgroup);
                        }
                        already_handled = true;
                     }
                     tmp_attribute_list = lFreeList(tmp_attribute_list);
                     if (already_handled) {
                        break;
                     }
                  }
                  sge_dstring_free(&value);
                  if (already_handled) {
                     break;
                  }
               }
               index++;
            }
         }
      }
      spp++;
      continue;
   }

/*---------------------------------------------------------------------------*/

   if ((strcmp("-mattr", *spp) == 0) || (strcmp("-Mattr", *spp) == 0) ||
       (strcmp("-aattr", *spp) == 0) || (strcmp("-Aattr", *spp) == 0) ||   
       (strcmp("-rattr", *spp) == 0) || (strcmp("-Rattr", *spp) == 0) ||   
       (strcmp("-dattr", *spp) == 0) || (strcmp("-Dattr", *spp) == 0)) {   
     
/* *INDENT-OFF* */ 
      static object_info_entry info_entry[] = {
#ifdef QCONF_FLATFILE
         {SGE_CQUEUE_LIST,     SGE_OBJ_CQUEUE,    CQ_Type,   SGE_ATTR_QNAME,     CQ_name,   NULL,        cqueue_xattr_pre_gdi},
         {SGE_EXECHOST_LIST,   SGE_OBJ_EXECHOST,  EH_Type,   SGE_ATTR_HOSTNAME,  EH_name,   NULL,        NULL},
         {SGE_PE_LIST,         SGE_OBJ_PE,        PE_Type,   SGE_ATTR_PE_NAME,   PE_name,   NULL,        NULL},
         {SGE_CKPT_LIST,       SGE_OBJ_CKPT,      CK_Type,   SGE_ATTR_CKPT_NAME, CK_name,   NULL,        NULL},
         {SGE_HGROUP_LIST,     SGE_OBJ_HGROUP,    HGRP_Type, SGE_ATTR_HGRP_NAME, HGRP_name, NULL,        NULL},
         {0,                   NULL,              0,         NULL,               0,         NULL,        NULL}
#else
         {SGE_CQUEUE_LIST,     SGE_OBJ_CQUEUE,    CQ_Type,   SGE_ATTR_QNAME,     CQ_name,   read_cqueue_work,     cull_read_in_cqueue,     cqueue_xattr_pre_gdi},
         {SGE_EXECHOST_LIST,   SGE_OBJ_EXECHOST,  EH_Type,   SGE_ATTR_HOSTNAME,  EH_name,   read_host_work,       cull_read_in_host,       NULL},
         {SGE_PE_LIST,         SGE_OBJ_PE,        PE_Type,   SGE_ATTR_PE_NAME,   PE_name,   read_pe_work,         cull_read_in_pe,         NULL},
         {SGE_CKPT_LIST,       SGE_OBJ_CKPT,      CK_Type,   SGE_ATTR_CKPT_NAME, CK_name,   read_ckpt_work,       cull_read_in_ckpt,       NULL},
         {SGE_HGROUP_LIST,     SGE_OBJ_HGROUP,    HGRP_Type, SGE_ATTR_HGRP_NAME, HGRP_name, read_host_group_work, cull_read_in_host_group, NULL},
         {0,                   NULL,              0,         NULL,               0,         NULL,                 NULL}
#endif
      }; 
/* *INDENT-ON* */
      
      int from_file;
      int index;
      int ret = 0;
      int sub_command = 0;
   
      /* This does not have to be freed later */
      info_entry[0].fields = CQ_fields;
      /* These have to be freed later */
      info_entry[1].fields = sge_build_EH_field_list (false, false, false);
      info_entry[2].fields = sge_build_PE_field_list (false, false);
      /* These do not */
      info_entry[3].fields = CK_fields;
      info_entry[4].fields = HGRP_fields;
      
      /* no adminhost/manager check needed here */
        
      /* Capital letter => we will read from file */
      if (isupper((*spp)[1])) {
         from_file = 1;
      } else {
         from_file = 0;
      }

      /* Set sub command for co_gdi call */
      if ((*spp)[1] == 'm' || (*spp)[1] == 'M') {
         sub_command = SGE_GDI_CHANGE;
      } else if ((*spp)[1] == 'a' || (*spp)[1] == 'A') {
         sub_command = SGE_GDI_APPEND;
      } else if ((*spp)[1] == 'd' || (*spp)[1] == 'D') {
         sub_command = SGE_GDI_REMOVE;
      } else if ((*spp)[1] == 'r' || (*spp)[1] == 'R') {
         sub_command = SGE_GDI_SET;
      }
      spp = sge_parser_get_next(spp);

      /* is the objectname given in commandline
         supported by this function */
      index = 0;
      while(info_entry[index].object_name) {
         if (!strcmp(info_entry[index].object_name, *spp)) {
            spp = sge_parser_get_next(spp);
            break; 
         }
         index++;
      }

      if (!info_entry[index].object_name) {
         fprintf(stderr, "Modification of object "SFQ" not supported\n", *spp);
         FREE (info_entry[1].fields);
         FREE (info_entry[2].fields);
         SGE_EXIT(1);
      } 

      /* */
      DTRACE;
      ret = qconf_modify_attribute(&alp, from_file, &spp, &ep, 
            sub_command, &(info_entry[index])); 

      /* Error handling */
      if (ret || lGetNumberOfElem(alp)) {
         int exit = 0;

         for_each(aep, alp) {
            FILE *std_x = NULL;
            
            if (lGetUlong(aep, AN_status) != STATUS_OK) {
               std_x = stderr;
               exit = 1;
            } else {
               std_x = stdout;
            } 
            fprintf(std_x, "%s", lGetString(aep, AN_text)); 
         }
         alp = lFreeList(alp);   
         if (exit) {
            FREE (info_entry[1].fields);
            FREE (info_entry[2].fields);
            SGE_EXIT(1);
         }
      }
      
      FREE (info_entry[1].fields);
      FREE (info_entry[2].fields);
      
      continue;
   }

/*----------------------------------------------------------------------------*/
      /* "-Msconf" */
      if (strcmp("-Msconf", *spp) == 0) {
         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());
         
         spp = sge_parser_get_next(spp);

#ifdef QCONF_FLATFILE
         fields_out[0] = NoName;
         ep = spool_flatfile_read_object(&alp, SC_Type, NULL,
                                         SC_fields, fields_out, true, &qconf_comma_sfi,
                                         SP_FORM_ASCII, NULL, *spp);

         if (answer_list_output(&alp)) {
            ep = lFreeElem (ep);
         }
         
         if (ep != NULL) {
            missing_field = spool_get_unprocessed_field (SC_fields, fields_out, &alp);
         }

         if (missing_field != NoName) {
            ep = lFreeElem(ep);
            answer_list_output(&alp);
         }
         
         if (ep != NULL) {
            lp = lCreateList("scheduler config", SC_Type);
            lAppendElem (lp, ep);
            
            if (!sconf_validate_config (&alp, lp)) {
               lp = lFreeList (lp);
               answer_list_output(&alp);
            }
         }
         
         /* else we let the check for lp != NULL catch the error below */
#else
         lp = read_sched_configuration(NULL, *spp, 0, &alp);
#endif

         if (!lp)
            if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
               continue;

         /* send it to qmaster */
/*         lp = lCreateList("sconf to modify", SC_Type);
         lAppendElem(lp, ep);*/

         alp = sge_gdi(SGE_SC_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) == STATUS_OK)
            fprintf(stderr, MSG_SCHEDD_CHANGEDSCHEDULERCONFIGURATION);
         else
            fprintf(stderr, "%s", lGetString(aep, AN_text));

         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
         
      }

/*----------------------------------------------------------------------------*/
      /* "-msconf"  modify scheduler configuration */

      if (strcmp("-msconf", *spp) == 0) {
         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());

         /* get the scheduler configuration .. */
         what = lWhat("%T(ALL)", SC_Type);
         alp = sge_gdi(SGE_SC_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);

         if (!(newlp=edit_sched_conf(lp)))
            continue;

         lp = lFreeList(lp);
         what = lWhat("%T(ALL)", SC_Type);
         alp = sge_gdi(SGE_SC_LIST, SGE_GDI_MOD, &newlp, NULL, what);
         what = lFreeWhat(what);
         ep = lFirst(alp);
         answer_exit_if_not_recoverable(ep);
         if (answer_get_status(ep) == STATUS_OK)
            fprintf(stderr, MSG_SCHEDD_CHANGEDSCHEDULERCONFIGURATION);
         else
            fprintf(stderr, "%s", lGetString(ep, AN_text));
         newlp = lFreeList(newlp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-mstree", "-Mstree file": modify sharetree */

      if ((strcmp("-mstree", *spp) == 0) || (strcmp("-Mstree", *spp) == 0)) {
         if(strcmp("-mstree", *spp) == 0) {
            sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
            sge_gdi_is_manager(uti_state_get_user_name());

            /* get the sharetree .. */
            what = lWhat("%T(ALL)", STN_Type);
            alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
            what = lFreeWhat(what);

            aep = lFirst(alp);
            answer_exit_if_not_recoverable(aep);
            if (answer_get_status(aep) != STATUS_OK) {
               fprintf(stderr, "%s", lGetString(aep, AN_text));
               spp++;
               continue;
            }
            alp = lFreeList(alp);
    
            ep = lFirst(lp);
            if (!(ep=edit_sharetree(ep)))
               continue;

            lp = lFreeList(lp);
         } else {
            char errstr[1024];
#ifdef QCONF_FLATFILE
            spooling_field *fields = sge_build_STN_field_list (0, 1);
#endif
            spp = sge_parser_get_next(spp);
           
#ifdef QCONF_FLATFILE
            fields_out[0] = NoName;
            ep = spool_flatfile_read_object(&alp, STN_Type, NULL,
                                            fields, fields_out, true,
                                            &qconf_name_value_list_sfi,
                                            SP_FORM_ASCII, NULL, *spp);
            
            if (answer_list_output(&alp)) {
               ep = lFreeElem (ep);
            }
         
            if (ep != NULL) {
               missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
            }

            FREE (fields);
            
            if (missing_field != NoName) {
               ep = lFreeElem(ep);
               answer_list_output(&alp);
            }
#else
            ep = read_sharetree(*spp, NULL, 0, errstr, 1, NULL);
#endif
               
            if (ep == NULL) {
               fprintf(stderr, errstr);
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
                  continue;
            }      
         }

         newlp = lCreateList("sharetree modify", STN_Type);
         lAppendElem(newlp, ep);

         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_MOD, &newlp, NULL, what);
         what = lFreeWhat(what);
         ep = lFirst(alp);
         answer_exit_if_not_recoverable(ep);
         if (answer_get_status(ep) == STATUS_OK)
            fprintf(stderr, MSG_TREE_CHANGEDSHARETREE);
         else
            fprintf(stderr, "%s", lGetString(ep, AN_text));
         newlp = lFreeList(newlp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-mu userset,..." */

      if (strcmp("-mu", *spp) == 0) {
         /* check for adminhost and manager privileges */
         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());

         spp = sge_parser_get_next(spp);

         /* get userset */
         parse_name_list_to_cull("usersets", &lp, US_Type, US_name, *spp);

         edit_usersets(lp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-Mu fname" */

      if (strcmp("-Mu", *spp) == 0) {
         char* file = NULL;
         const char* usersetname = NULL;
         lList *acl = NULL;

         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }     

          
         /* get userset from file */
         ep = NULL;
#ifdef QCONF_FLATFILE
         fields_out[0] = NoName;
         ep = spool_flatfile_read_object(&alp, US_Type, NULL,
                                         US_fields, fields_out, true, &qconf_param_sfi,
                                         SP_FORM_ASCII, NULL, file);
         
         if (answer_list_output(&alp)) {
            ep = lFreeElem(ep);
         }
         
         if (ep != NULL) {
            missing_field = spool_get_unprocessed_field (US_fields, fields_out, &alp);
         }

         if (missing_field != NoName) {
            ep = lFreeElem(ep);
            answer_list_output(&alp);
         }

         if ((ep != NULL) &&
             (userset_validate_entries(ep, &alp, 0) != STATUS_OK)) {
            ep = lFreeElem(ep);
            answer_list_output(&alp);
         }
#else
         ep = cull_read_in_userset(NULL, file ,0 ,0 ,0 );
#endif
            
         if (ep == NULL) {
            sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE); 
         }
         usersetname = lGetString(ep,US_name);
 
         /* get userset from qmaster */
         where = lWhere("%T( %I==%s )", US_Type, US_name, usersetname);
         what = lWhat("%T(ALL)", US_Type);
         alp = sge_gdi(SGE_USERSET_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            alp = lFreeList(alp);
            ep = lFreeElem(ep);
            lp = lFreeList(lp);
            SGE_EXIT(1); 
         }

         if (!lp) {
            fprintf(stderr, MSG_PROJECT_XISNOKNWOWNPROJECT_S, usersetname);
            fflush(stdout);
            fflush(stderr);
            alp = lFreeList(alp);
            ep = lFreeElem(ep);
            SGE_EXIT(1); 
         }
         alp = lFreeList(alp);

         acl = lCreateList("modified usersetlist", US_Type); 
         lAppendElem(acl,ep);

         alp = sge_gdi(SGE_USERSET_LIST, SGE_GDI_MOD, &acl, NULL, NULL);
         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            alp = lFreeList(alp);
            acl = lFreeList(acl);
            SGE_EXIT(1);
         } 
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         alp = lFreeList(alp);
         acl = lFreeList(acl);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-Au fname" */

      if (strcmp("-Au", *spp) == 0) {
         lList *acl = NULL;
         char* file = NULL;

         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }     

          
         /* get userset  */
         ep = NULL;
#ifdef QCONF_FLATFILE
         fields_out[0] = NoName;
         ep = spool_flatfile_read_object(&alp, US_Type, NULL,
                                         US_fields, fields_out,  true,
                                         &qconf_param_sfi,
                                         SP_FORM_ASCII, NULL, file);
         
         if (answer_list_output(&alp)) {
            ep = lFreeElem(ep);
         }

         if (ep != NULL) {
            missing_field = spool_get_unprocessed_field (US_fields, fields_out, &alp);
         }
         
         if (missing_field != NoName) {
            ep = lFreeElem(ep);
            answer_list_output(&alp);
         }

         if ((ep != NULL) && (userset_validate_entries(ep, &alp, 0) != STATUS_OK)) {
            ep = lFreeElem(ep);
            answer_list_output(&alp);
         }
#else
         ep = cull_read_in_userset(NULL, file ,0 ,0 ,0 ); 
#endif
            
         if (ep == NULL) {
            sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE); 
         }

         acl = lCreateList("usersetlist list to add", US_Type); 
         lAppendElem(acl,ep);

         alp = sge_gdi(SGE_USERSET_LIST, SGE_GDI_ADD, &acl, NULL, NULL);
         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            alp = lFreeList(alp);
            acl = lFreeList(acl);
            SGE_EXIT(1);
         } 
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         alp = lFreeList(alp);
         acl = lFreeList(acl);
         spp++;
         continue;
      }


/*----------------------------------------------------------------------------*/

      /* "-muser username" */

      if (strcmp("-muser", *spp) == 0) {
         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());

         spp = sge_parser_get_next(spp);
        
         /* get user */
         where = lWhere("%T( %I==%s )", UP_Type, UP_name, *spp);
         what = lWhat("%T(ALL)", UP_Type);
         alp = sge_gdi(SGE_USER_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }

         if (!lp) {
            fprintf(stderr, MSG_USER_XISNOKNOWNUSER_S, *spp);
            spp++;
            continue;
         }
         alp = lFreeList(alp);
         ep = lFirst(lp);
         
         /* edit user */
         newep = edit_userprj(ep, 1);

         /* if the user name has changed, we need to print an error message */   
         if (strcmp(lGetString(ep, UP_name), lGetString(newep, UP_name))) {
            fprintf(stderr, MSG_QCONF_CANTCHANGEOBJECTNAME_SS, lGetString(ep, UP_name), lGetString(newep, UP_name));
            SGE_EXIT(1);
         }
         else {
            lp = lFreeList(lp);
            /* send it to qmaster */
            lp = lCreateList("User list to modify", UP_Type); 
            lAppendElem(lp, newep);

            alp = sge_gdi(SGE_USER_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

            aep = lFirst(alp);
            answer_exit_if_not_recoverable(aep);
            if (answer_get_status(aep) != STATUS_OK) {
               fprintf(stderr, "%s", lGetString(aep, AN_text));
               alp = lFreeList(alp);
               lp = lFreeList(lp);
               SGE_EXIT(1);
            } else {
               fprintf(stdout, "%s", lGetString(aep, AN_text));
            }
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-mprj projectname" */

      if (strcmp("-mprj", *spp) == 0) {
         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());

         spp = sge_parser_get_next(spp);
        
         /* get project */
         where = lWhere("%T( %I==%s )", UP_Type, UP_name, *spp);
         what = lWhat("%T(ALL)", UP_Type);
         alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);
                  
         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }

         if (!lp) {
            fprintf(stderr, MSG_PROJECT_XISNOKNWOWNPROJECT_S, *spp);
            continue;
         }
         alp = lFreeList(alp);
         ep = lFirst(lp);
         
         /* edit project */
         newep = edit_userprj(ep, 0);

         /* look whether name has changed. If so we have to delete the
            project with the old name */
         /*if (strcmp(lGetString(ep, UP_name), lGetString(newep, UP_name))) {
            alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
            aep = lFirst(alp);
            answer_exit_if_not_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            alp = lFreeList(alp);
         }*/

         /* send it to qmaster */
         lp = lCreateList("Project list to modify", UP_Type); 
         lAppendElem(lp, newep);

         alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }




/*----------------------------------------------------------------------------*/

      /* "-Muser file" */

      if (strcmp("-Muser", *spp) == 0) {
         char* file = NULL;
         const char* username = NULL;
#ifdef QCONF_FLATFILE   
         spooling_field *fields = NULL;
#endif
         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }

         /* get user from file */
         newep = NULL;
#ifdef QCONF_FLATFILE   
         fields_out[0] = NoName;
         fields = sge_build_UP_field_list (0, 1);
         newep = spool_flatfile_read_object(&alp, UP_Type, NULL,
                                         fields, fields_out, true, &qconf_sfi,
                                         SP_FORM_ASCII, NULL, file);
         
         if (answer_list_output(&alp)) {
            newep = lFreeElem (newep);
         }
         
         if (newep != NULL) {
            missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
         }

         FREE (fields);
         
         if (missing_field != NoName) {
            newep = lFreeElem (newep);
            answer_list_output(&alp);
         }
#else
         newep = cull_read_in_userprj(NULL, file, 0, 1, 0); 
#endif

         if (newep == NULL) {
            sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE); 
         } 
         
#ifdef QCONF_FLATFILE   
         NULL_OUT_NONE(newep, UP_default_project);
#endif
         
         username = lGetString(newep, UP_name); 
                 
         /* get user */
         where = lWhere("%T( %I==%s )", UP_Type, UP_name, username);
         what = lWhat("%T(ALL)", UP_Type);
         alp = sge_gdi(SGE_USER_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);
                  
         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            alp = lFreeList(alp);
            newep = lFreeElem(newep);
            lp = lFreeList(lp);
            SGE_EXIT(1); 
         }

         if (!lp) {
            fprintf(stderr, MSG_USER_XISNOKNOWNUSER_S, username);
            fflush(stdout);
            fflush(stderr);
            alp = lFreeList(alp);
            newep = lFreeElem(newep);
            SGE_EXIT(1); 
         }
         alp = lFreeList(alp);
         ep = lFirst(lp);
         
         /* edit user */
         /* newep = edit_userprj(ep, 1); */

         /* send it to qmaster */
         lp = lCreateList("User list to modify", UP_Type); 
         lAppendElem(lp, newep);

         alp = sge_gdi(SGE_USER_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
           fprintf(stderr, "%s", lGetString(aep, AN_text));
           alp = lFreeList(alp);
           lp = lFreeList(lp);
           SGE_EXIT(1);
         } else {
           fprintf(stdout, "%s", lGetString(aep, AN_text));
         }
          
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-Mprj file" */

      if (strcmp("-Mprj", *spp) == 0) {
         char* file = NULL;
         const char* projectname = NULL;
#ifdef QCONF_FLATFILE   
         spooling_field *fields = NULL;
#endif
   
         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }

         /* get project from file */
         newep = NULL;
#ifdef QCONF_FLATFILE   
         fields_out[0] = NoName;
         fields = sge_build_UP_field_list (0, 0);
         newep = spool_flatfile_read_object(&alp, UP_Type, NULL,
                                         fields, fields_out, true, &qconf_sfi,
                                         SP_FORM_ASCII, NULL, file);
         
         if (answer_list_output(&alp)) {
            newep = lFreeElem (newep);
         }
         
         if (newep != NULL) {
            missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
         }

         FREE (fields);
         
         if (missing_field != NoName) {
            newep = lFreeElem (newep);
            answer_list_output (&alp);
         }
#else
         newep = cull_read_in_userprj(NULL, file, 0, 0, 0); 
#endif

         if (newep == NULL) {
            sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE); 
         } 
         
         projectname = lGetString(newep, UP_name); 
                 
         /* get project */
         where = lWhere("%T( %I==%s )", UP_Type, UP_name, projectname);
         what = lWhat("%T(ALL)", UP_Type);
         alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);
                  
         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            alp = lFreeList(alp);
            newep = lFreeElem(newep);
            lp = lFreeList(lp);
            SGE_EXIT(1); 
         }

         if (!lp) {
            fprintf(stderr, MSG_PROJECT_XISNOKNWOWNPROJECT_S, projectname);
            fflush(stdout);
            fflush(stderr);
            alp = lFreeList(alp);
            newep = lFreeElem(newep);
            SGE_EXIT(1); 
         }
         alp = lFreeList(alp);
         ep = lFirst(lp);
         
         /* edit project */
         /* newep = edit_userprj(ep, 0); */

         /* send it to qmaster */
         lp = lCreateList("Project list to modify", UP_Type); 
         lAppendElem(lp, newep);

         alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-sc complex_name_list" */

      if (strcmp("-sc", *spp) == 0) {
         lList *answer_list = NULL;

         centry_list_show(&answer_list);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-scal calendar_name" */
      if (strcmp("-scal", *spp) == 0) {
         spp = sge_parser_get_next(spp);

         /* get the existing pe entry .. */
         where = lWhere("%T( %I==%s )", CAL_Type, CAL_name, *spp);
         what = lWhat("%T(ALL)", CAL_Type);
         alp = sge_gdi(SGE_CALENDAR_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
           fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);

         if (!lp) {
            fprintf(stderr, MSG_CALENDAR_XISNOTACALENDAR_S, *spp);
            SGE_EXIT(1);
         }

         ep = lFirst(lp);
#ifdef QCONF_FLATFILE
         filename = (char *)spool_flatfile_write_object(&alp, ep, false,
                                              CAL_fields, &qconf_sfi,
                                              SP_DEST_STDOUT, SP_FORM_ASCII,
                                              NULL, false);
         if (answer_list_output(&alp)) {
            sge_error_and_exit(NULL);
         }
#else
         write_cal(0, 0, ep);
#endif

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-scall" */

      if (strcmp("-scall", *spp) == 0) {
         show_object_list(SGE_CALENDAR_LIST, CAL_Type, CAL_name, "calendar"); 
         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-sconf host_list" || "-mconf host_list" || "-aconf host_list" || "-Aconf file_list" */
      /* file list is also host list */
      if ((strcmp("-sconf", *spp) == 0) || 
          (strcmp("-aconf", *spp) == 0) || 
          (strcmp("-mconf", *spp) == 0) ||
          (strcmp("-Aconf", *spp) == 0)) {
         int action = 0;
         char *host_list = NULL;
         int ret, first = 1;
         lListElem *hep;
         const char *host;

         if (!strcmp("-aconf", *spp)) {
            sge_gdi_is_manager(uti_state_get_user_name());
            sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
            action = 1;
         }
         else if (!strcmp("-mconf", *spp)) {
            sge_gdi_is_manager(uti_state_get_user_name());
            if (sge_gdi_is_adminhost(uti_state_get_qualified_hostname()) != 0) {
               SGE_EXIT(1);
            }
            action = 2;
         }
         else if (!strcmp("-Aconf", *spp)) {
            action = 3;
         }
         if (!sge_next_is_an_opt(spp))  {
            spp = sge_parser_get_next(spp);
            host_list = sge_strdup(NULL, *spp);
         }
         else
            host_list = sge_strdup(NULL, SGE_GLOBAL_NAME);
            
         /* host_list might look like host1,host2,... */
         hep = lCreateElem(EH_Type);

         for ((cp = sge_strtok(host_list, ",")); cp && *cp;
              (cp = sge_strtok(NULL, ","))) {

            if (!first) {
               fprintf(stdout, "\n");
            }

            /*
            ** it would be uncomfortable if you could only give files in .
            */
            if ((action == 3) && cp && strrchr(cp, '/')) {
               lSetHost(hep, EH_name, strrchr(cp, '/') + 1);
            }
            else {
               lSetHost(hep, EH_name, cp);
            }
            
            switch ((ret=sge_resolve_host(hep, EH_name))) {
            case CL_RETVAL_OK:
               break;
            default:
               fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_SS, lGetHost(hep, EH_name), cl_get_error_text(ret));
               FREE(host_list);
               hep = lFreeElem(hep);
               SGE_EXIT(1);
               break;
            }
            host = lGetHost(hep, EH_name);

            if (action == 0)
               print_config(host);
            else if (action == 1)
               add_modify_config(host, NULL, 1);
            else if (action == 2)
               add_modify_config(host, NULL, 0);
            else if (action == 3)
               add_modify_config(host, cp, 0);
            first = 0;
         } /* end for */
         
         FREE(host_list);
         hep = lFreeElem(hep);

         spp++;
         continue;
      }
      
/*-----------------------------------------------------------------------------*/
      /* "-sckpt ckpt_name" */
      if (strcmp("-sckpt", *spp) == 0) {
         spp = sge_parser_get_next(spp);

         /* get the existing ckpt entry .. */
         where = lWhere("%T( %I==%s )", CK_Type, CK_name, *spp);
         what = lWhat("%T(ALL)", CK_Type);
         alp = sge_gdi(SGE_CKPT_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
           fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);

         if (!lp) {
            fprintf(stderr, MSG_CKPT_XISNOTCHKPINTERFACEDEF_S, *spp);
            SGE_EXIT(1);
         }

         ep = lFirst(lp);
#ifdef QCONF_FLATFILE
         (char *)spool_flatfile_write_object(&alp, ep, false,
                                             CK_fields, &qconf_sfi,
                                             SP_DEST_STDOUT, SP_FORM_ASCII,
                                             NULL, false);
         if (answer_list_output(&alp)) {
            sge_error_and_exit (NULL);
         }
#else
         write_ckpt(0, 0, ep);
#endif

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-sckptl" */
      if (strcmp("-sckptl", *spp) == 0) {
         show_object_list(SGE_CKPT_LIST, CK_Type, CK_name,
               "ckpt interface definition");
         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-sconfl" */

      if (strcmp("-sconfl", *spp) == 0) {
         show_object_list(SGE_CONFIG_LIST, CONF_Type, CONF_hname, "config"); 
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-dconf config_list" */
      if (strcmp("-dconf", *spp) == 0) {
         char *host_list = NULL;
         lListElem *hep = NULL;
         const char *host = NULL;
         int ret;

         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp))  {
            spp = sge_parser_get_next(spp);

            host_list = sge_strdup(NULL, *spp);
            hep = lCreateElem(EH_Type);

            for ((cp = sge_strtok(host_list, ",")); cp && *cp;
                 (cp = sge_strtok(NULL, ","))) {
               
               lSetHost(hep, EH_name, cp);
               
               switch (sge_resolve_host(hep, EH_name)) {
               case CL_RETVAL_OK:
                  break;
               default:
                  fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, cp);
                  FREE(host_list);
                  hep = lFreeElem(hep);
                  SGE_EXIT(1);
                  break;
               }
               host = lGetHost(hep, EH_name);
               ret = delete_config(host);
               /*
               ** try the unresolved name if this was different
               */
               if (ret && strcmp(cp, host)) {
                  delete_config(cp);
               }
            } /* end for */

            FREE(host_list);
            hep = lFreeElem(hep);
         }
         else
            fprintf(stderr, MSG_ANSWER_NEEDHOSTNAMETODELLOCALCONFIG);

         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-se exec_server" */
      if (strcmp("-se", *spp) == 0) {
         spp = sge_parser_get_next(spp);

         /* resolve host */
         hep = lCreateElem(EH_Type);
         lSetHost(hep, EH_name, *spp);
         
         switch (sge_resolve_host(hep, EH_name)) {
         case CL_RETVAL_OK:
            break;
         default:
            fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetHost(hep, EH_name));
            hep = lFreeElem(hep);
            SGE_EXIT(1);
            break;
         }

         host = lGetHost(hep, EH_name);
        
         /* get the existing host entry .. */
         where = lWhere("%T( %Ih=%s )", EH_Type, EH_name, host);
         what = lWhat("%T(ALL)", EH_Type);
         alp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
           fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);

         if (!lp) {
            fprintf(stderr, MSG_EXEC_XISNOTANEXECUTIONHOST_S, host);
            SGE_EXIT(1);
         }

         ep = lFirst(lp);
#ifdef QCONF_FLATFILE
         {
            spooling_field *fields = sge_build_EH_field_list (false, true, false);
            spool_flatfile_write_object(&alp, ep, false, fields, &qconf_sfi,
                                        SP_DEST_STDOUT, SP_FORM_ASCII, NULL,
                                        false);
            FREE (fields);
            
            if (answer_list_output(&alp)) {
               sge_error_and_exit (NULL);
            }
         }
#else
         write_host(0, 0, ep, EH_name, NULL);
#endif

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-secl" */
      if (strcmp("-secl", *spp) == 0) {
         show_eventclients();
         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-sel" */
      if (strcmp("-sel", *spp) == 0) {
         show_object_list(SGE_EXECHOST_LIST, EH_Type, EH_name, 
               "execution host"); 
         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-sep" */
      if (strcmp("-sep", *spp) == 0) {
         show_processors();
         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-sh" */
      if (strcmp("-sh", *spp) == 0) {
         show_object_list(SGE_ADMINHOST_LIST, AH_Type, AH_name, 
               "administrative host"); 
         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-sm" */

      if (strcmp("-sm", *spp) == 0) {
         show_object_list(SGE_MANAGER_LIST, MO_Type, MO_name, "manager"); 
         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-sp pe_name" */
      if (strcmp("-sp", *spp) == 0) {
         spp = sge_parser_get_next(spp);

         /* get the existing pe entry .. */
         where = lWhere("%T( %I==%s )", PE_Type, PE_name, *spp);
         what = lWhat("%T(ALL)", PE_Type);
         alp = sge_gdi(SGE_PE_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
           fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);

         if (!lp) {
            fprintf(stderr,  MSG_PARALLEL_XNOTAPARALLELEVIRONMENT_S , *spp);
            SGE_EXIT(1);
         }

         ep = lFirst(lp);
#ifdef QCONF_FLATFILE
         {
            spooling_field *fields = sge_build_PE_field_list (false, true);
            (char *)spool_flatfile_write_object(&alp, ep, false,
                                                 fields, &qconf_sfi,
                                                 SP_DEST_STDOUT, SP_FORM_ASCII,
                                                 NULL, false);
            FREE (fields);
            
            if (answer_list_output(&alp)) {
               sge_error_and_exit(NULL);
            }
         }
#else
         write_pe(0, 0, ep);
#endif

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-spl" */
      if (strcmp("-spl", *spp) == 0) {
         show_object_list(SGE_PE_LIST, PE_Type, PE_name,
               "parallel environment");
         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-so" */

      if (strcmp("-so", *spp) == 0) {
         show_object_list(SGE_OPERATOR_LIST, MO_Type, MO_name, "operator"); 
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-ssconf" */

      if (strcmp("-ssconf", *spp) == 0) {
         /* get the scheduler configuration .. */
         what = lWhat("%T(ALL)", SC_Type);
         alp = sge_gdi(SGE_SC_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);
 
#ifdef QCONF_FLATFILE
         spool_flatfile_write_object(&alp, lFirst(lp), false, SC_fields,
                                     &qconf_comma_sfi, SP_DEST_STDOUT,
                                     SP_FORM_ASCII, NULL, false);
         
         if (answer_list_output(&alp)) {
#else
         if (write_sched_configuration(0, 0, NULL, lFirst(lp)) == NULL) {
#endif
            fprintf(stderr, MSG_SCHEDCONF_CANTCREATESCHEDULERCONFIGURATION);
            spp++;
            continue;
         }

         lp = lFreeList(lp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-sstnode node_path[,...]"  show sharetree node */

      if (strcmp("-sstnode", *spp) == 0) {
         int found = 0;

         spp = sge_parser_get_next(spp);

         /* get the sharetree .. */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);
 
         ep = lFirst(lp);
         if (!ep) {
            fprintf(stderr, MSG_TREE_NOSHARETREE);
            spp++;
            continue;
         }

         lString2List(*spp, &arglp, STN_Type, STN_name, ", ");

         for_each(argep, arglp) {

            lListElem *node = NULL;
            const char *nodepath = NULL;
            ancestors_t ancestors;

            nodepath = lGetString(argep, STN_name);
            if (nodepath) {

               memset(&ancestors, 0, sizeof(ancestors));
               node = search_named_node_path(ep, nodepath, &ancestors);
               if (node) {
                  int i, shares;
                  found++;
                  for(i=0; i<ancestors.depth; i++)
                     printf("/%s", lGetString(ancestors.nodes[i], STN_name));
                  shares = (int)lGetUlong(node, STN_shares);
                  printf("=%d\n", shares);
               } else {
                  fprintf(stderr, MSG_TREE_UNABLETOLACATEXINSHARETREE_S,
                          nodepath);
               }
               free_ancestors(&ancestors);
            }
         }

         if (!found && *(spp+1) == NULL) {
            SGE_EXIT(1);
         }

         arglp = lFreeList(arglp);
         lp = lFreeList(lp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-rsstnode node_path[,...]"  show sharetree node */

      if (strcmp("-rsstnode", *spp) == 0) {
         int found = 0;

         spp = sge_parser_get_next(spp);

         /* get the sharetree .. */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);
 
         ep = lFirst(lp);
         if (!ep) {
            fprintf(stderr, MSG_TREE_NOSHARETREE);
            spp++;
            continue;
         }

         lString2List(*spp, &arglp, STN_Type, STN_name, ", ");

         for_each(argep, arglp) {
            const char *nodepath = NULL;
            nodepath = lGetString(argep, STN_name);
            if (nodepath) {
               show_sharetree_path(ep, nodepath);
            }
         }                                                                      

         if (!found && *(spp+1) == NULL) {
            SGE_EXIT(1);
         }

         arglp = lFreeList(arglp);
         lp = lFreeList(lp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-sstree" */

      if (strcmp("-sstree", *spp) == 0) {
#ifdef QCONF_FLATFILE
         spooling_field *fields = NULL;
#endif
         /* get the sharetree .. */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);
 
         ep = lFirst(lp);

#ifdef QCONF_FLATFILE
         fields = sge_build_STN_field_list (false, true);
         id_sharetree (ep, 0);
         spool_flatfile_write_object(&alp, ep, true, fields,
                                     &qconf_name_value_list_sfi,
                                     SP_DEST_STDOUT, SP_FORM_ASCII, 
                                     NULL, false);
         FREE (fields);
         
         if (answer_list_output(&alp)) {
            sge_error_and_exit(NULL);
         }
#else
         write_sharetree(NULL, ep, NULL, stdout, 0, 1, 1);
#endif

         lp = lFreeList(lp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-bonsai" */

      if (strcmp("-bonsai", *spp) == 0) {
         /* get the sharetree .. */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);
 
         ep = lFirst(lp);

         show_sharetree(ep, NULL);

         lp = lFreeList(lp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-ss" */
      if (strcmp("-ss", *spp) == 0) {

         show_object_list(SGE_SUBMITHOST_LIST, SH_Type, SH_name, 
               "submit host"); 
         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-sss" - show scheduler state */

      if (strcmp("-sss", *spp) == 0) {
         /* ... */
         show_object_list(SGE_EVENT_LIST, EV_Type, EV_host, "scheduling host"); 

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-su [list_name[,list_name,...]]" */

      if (strcmp("-su", *spp) == 0) {
         spp = sge_parser_get_next(spp);
         parse_name_list_to_cull("acl`s to show", &lp, 
               US_Type, US_name, *spp);
         print_acl(lp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }




/*----------------------------------------------------------------------------*/

#ifndef __SGE_NO_USERMAPPING__
      /* "-sumapl" */
      if (strcmp("-sumapl", *spp) == 0) {
         show_object_list(SGE_USER_MAPPING_LIST, CU_Type, CU_name, "user mapping entries");  
         spp++;
         continue;
      }
#endif


/*----------------------------------------------------------------------------*/

#ifndef __SGE_NO_USERMAPPING__
      /* "-Mumap user filename" */
      if (strcmp("-Mumap", *spp) == 0) {
         lList *answer_list = NULL;
         char* file = NULL;
        
         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }
         cuser_modify_from_file(&answer_list, file);
         show_gdi_request_answer(answer_list);
         
         spp++;
         continue;
      }
#endif

/*----------------------------------------------------------------------------*/

#ifndef __SGE_NO_USERMAPPING__
      /* "-sumap user"  */
      if (strcmp("-sumap", *spp) == 0) {
         lList *answer_list = NULL;

         spp = sge_parser_get_next(spp);
         cuser_show(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }
#endif

#ifdef __SGE_CENTRY_DEBUG__
      /* "-sce attribute"  */
      if (strcmp("-sce", *spp) == 0) {
         lList *answer_list = NULL;

         spp = sge_parser_get_next(spp);
         centry_show(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }
#endif

/*----------------------------------------------------------------------------*/

#ifndef __SGE_NO_USERMAPPING__
      /* "-mumap user"  */
      if (strcmp("-mumap", *spp) == 0) {
         lList *answer_list = NULL;

         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());

         spp = sge_parser_get_next(spp);
         sge_gdi_is_manager(uti_state_get_user_name());
         cuser_modify(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }
#endif

#ifdef __SGE_CENTRY_DEBUG__

/*----------------------------------------------------------------------------*/

      /* "-mce centry"  */
      if (strcmp("-mce", *spp) == 0) {
         lList *answer_list = NULL;

         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());

         spp = sge_parser_get_next(spp);
         sge_gdi_is_manager(uti_state_get_user_name());
         centry_modify(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

#endif

         
/*----------------------------------------------------------------------------*/

#ifndef __SGE_NO_USERMAPPING__
      /* "-dumap user "  */
      if (strcmp("-dumap", *spp) == 0) {
         lList *answer_list = NULL;

         spp = sge_parser_get_next(spp);
         sge_gdi_is_manager(uti_state_get_user_name());
         cuser_delete(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }
#endif

/*----------------------------------------------------------------------------*/

#ifdef __SGE_CENTRY_DEBUG__
      /* "-dce attribute "  */
      if (strcmp("-dce", *spp) == 0) {
         lList *answer_list = NULL;
   
         spp = sge_parser_get_next(spp);
         sge_gdi_is_manager(uti_state_get_user_name());
         centry_delete(&answer_list, *spp);
         show_gdi_request_answer(answer_list); 
         spp++;
         continue;
      }

#endif

/*----------------------------------------------------------------------------*/

#ifndef __SGE_NO_USERMAPPING__
      /* "-Aumap user mapfile"  */
      if (strcmp("-Aumap", *spp) == 0) {
         lList *answer_list = NULL;
         char* file = NULL;

         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }
   
         cuser_add_from_file(&answer_list, file);
         show_gdi_request_answer(answer_list); 
         spp++;
         continue;
      }
#endif

/*----------------------------------------------------------------------------*/

#ifndef __SGE_NO_USERMAPPING__
      /* "-aumap user"  */
      if (strcmp("-aumap", *spp) == 0) {
         lList *answer_list = NULL;

         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());

         spp = sge_parser_get_next(spp);
         sge_gdi_is_manager(uti_state_get_user_name());
         cuser_add(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }
#endif

/*----------------------------------------------------------------------------*/

#ifdef __SGE_CENTRY_DEBUG__

/*----------------------------------------------------------------------------*/

      /* "-ace attribute"  */
      if (strcmp("-ace", *spp) == 0) {
         lList *answer_list = NULL;

         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());

         spp = sge_parser_get_next(spp);
         sge_gdi_is_manager(uti_state_get_user_name());
         centry_add(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

#endif

/*----------------------------------------------------------------------------*/

#ifndef __SGE_NO_USERMAPPING__
      /* "-Mumap user filename" */
      if (strcmp("-Mumap", *spp) == 0) {
         lList *answer_list = NULL;
         char* file = NULL;
        
         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }
         cuser_modify_from_file(&answer_list, file);
         show_gdi_request_answer(answer_list);
         
         spp++;
         continue;
      }
#endif

/*----------------------------------------------------------------------------*/

#ifndef __SGE_NO_USERMAPPING__
      /* "-sumap user"  */
      if (strcmp("-sumap", *spp) == 0) {
         lList *answer_list = NULL;

         spp = sge_parser_get_next(spp);
         cuser_show(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }
#endif

/*----------------------------------------------------------------------------*/

#ifdef __SGE_CENTRY_DEBUG__
      /* "-sce attribute"  */
      if (strcmp("-sce", *spp) == 0) {
         lList *answer_list = NULL;

         spp = sge_parser_get_next(spp);
         centry_show(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }
#endif

/*----------------------------------------------------------------------------*/

#ifndef __SGE_NO_USERMAPPING__
      /* "-mumap user"  */
      if (strcmp("-mumap", *spp) == 0) {
         lList *answer_list = NULL;

         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());

         spp = sge_parser_get_next(spp);
         sge_gdi_is_manager(uti_state_get_user_name());
         cuser_modify(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }
#endif

#ifdef __SGE_CENTRY_DEBUG__

/*----------------------------------------------------------------------------*/

      /* "-mce centry"  */
      if (strcmp("-mce", *spp) == 0) {
         lList *answer_list = NULL;

         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());

         spp = sge_parser_get_next(spp);
         sge_gdi_is_manager(uti_state_get_user_name());
         centry_modify(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

#endif

         
/*----------------------------------------------------------------------------*/

#ifndef __SGE_NO_USERMAPPING__
      /* "-dumap user "  */
      if (strcmp("-dumap", *spp) == 0) {
         lList *answer_list = NULL;

         spp = sge_parser_get_next(spp);
         sge_gdi_is_manager(uti_state_get_user_name());
         cuser_delete(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }
#endif
      /*
       * Hostgroup parameters
       */

      /* "-shgrpl" */
      if (strcmp("-shgrpl", *spp) == 0) {
         show_object_list(SGE_HGROUP_LIST, HGRP_Type, HGRP_name, 
                          "host group list");  
         spp++;
         continue;
      }

      /* "-mhgrp user"  */
      if (strcmp("-mhgrp", *spp) == 0) {
         lList *answer_list = NULL;

         spp = sge_parser_get_next(spp);
         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());
         hgroup_modify(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

      /* "-Mhgrp user filename" */
      if (strcmp("-Mhgrp", *spp) == 0) {
         lList *answer_list = NULL;
         char* file = NULL;

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }
         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());
         hgroup_modify_from_file(&answer_list, file);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

      /* "-ahgrp group"  */
      if (strcmp("-ahgrp", *spp) == 0) {
         lList *answer_list = NULL;
         char* group = "@template";

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            group = *spp;
         }
         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());
         hgroup_add(&answer_list, group);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

      /* "-Ahgrp file"  */
      if (strcmp("-Ahgrp", *spp) == 0) {
         lList *answer_list = NULL;
         char* file = NULL;

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }
         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());
         hgroup_add_from_file(&answer_list, file);
         show_gdi_request_answer(answer_list); 
         spp++;
         continue;
      }

      /* "-dhgrp user "  */
      if (strcmp("-dhgrp", *spp) == 0) {
         lList *answer_list = NULL;
   
         spp = sge_parser_get_next(spp);
         sge_gdi_is_manager(uti_state_get_user_name());
         hgroup_delete(&answer_list, *spp);
         show_gdi_request_answer(answer_list); 
         spp++;
         continue;
      }

      /* "-shgrp group"  */
      if (strcmp("-shgrp", *spp) == 0) {
         lList *answer_list = NULL;

         spp = sge_parser_get_next(spp);
         hgroup_show(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

      /* "-shgrp_tree" */
      if (strcmp("-shgrp_tree", *spp) == 0) {
         lList *answer_list = NULL;

         spp = sge_parser_get_next(spp);
         hgroup_show_structure(&answer_list, *spp, true);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

      /* "-shgrp_resolved" */
      if (strcmp("-shgrp_resolved", *spp) == 0) {
         lList *answer_list = NULL;

         spp = sge_parser_get_next(spp);
         hgroup_show_structure(&answer_list, *spp, false);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

      /*
       * Cluster Queue parameter
       */

      /* "-sql" */
      if (strcmp("-sql", *spp) == 0) {
         show_object_list(SGE_CQUEUE_LIST, CQ_Type, CQ_name, "cqueue list");
         spp++;
         continue;
      }

      /* "-mq cqueue"  */
      if (strcmp("-mq", *spp) == 0) {
         lList *answer_list = NULL;

         spp = sge_parser_get_next(spp);

         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());
         cqueue_modify(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }
  
      /* "-Mq filename"  */ 
      if (strcmp("-Mq", *spp) == 0) {
         lList *answer_list = NULL;
         char* file = NULL;

         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());
         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN);
         }
         cqueue_modify_from_file(&answer_list, file);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

      /* "-aq cqueue"  */
      if (strcmp("-aq", *spp) == 0) {
         lList *answer_list = NULL;
         const char *name = "template";

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            name = *spp;
         }
         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());
         cqueue_add(&answer_list, name);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

      /* "-Aq filename"  */ 
      if (strcmp("-Aq", *spp) == 0) {
         lList *answer_list = NULL;
         char* file = NULL;

         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());
         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN);
         }
         cqueue_add_from_file(&answer_list, file);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

      /* "-dq cqueue"  */
      if (strcmp("-dq", *spp) == 0) {
         lList *answer_list = NULL;

         spp = sge_parser_get_next(spp);

         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());
         cqueue_delete(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

      /* "-sq [pattern[,pattern,...]]" */
      if (strcmp("-sq", *spp) == 0) {
         lList *answer_list = NULL;

         while (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            lString2List(*spp, &arglp, QR_Type, QR_name, ", ");
         }
         cqueue_show(&answer_list, arglp);
         arglp = lFreeList(arglp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

      /* "-cq destin_id[,destin_id,...]" */
      if (strcmp("-cq", *spp) == 0) {
         spp = sge_parser_get_next(spp);
         lString2List(*spp, &lp, ID_Type, ID_str, ", ");
         for_each(ep, lp) {
            lSetUlong(ep, ID_action, QI_DO_CLEAN);
         }
         alp = sge_gdi(SGE_CQUEUE_LIST, SGE_GDI_TRIGGER, &lp, NULL, NULL);
         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         lp = lFreeList(lp);
         spp++;
         continue;
      }

      if (strcmp("-sds", *spp) == 0) {
         lList *answer_list = NULL;

         cqueue_list_sick(&answer_list);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }


/*----------------------------------------------------------------------------*/

#ifdef __SGE_CENTRY_DEBUG__
      /* "-dce attribute "  */
      if (strcmp("-dce", *spp) == 0) {
         lList *answer_list = NULL;
   
         spp = sge_parser_get_next(spp);
         sge_gdi_is_manager(uti_state_get_user_name());
         centry_delete(&answer_list, *spp);
         show_gdi_request_answer(answer_list); 
         spp++;
         continue;
      }

#endif

/*----------------------------------------------------------------------------*/

#ifndef __SGE_NO_USERMAPPING__
      /* "-Aumap user mapfile"  */
      if (strcmp("-Aumap", *spp) == 0) {
         lList *answer_list = NULL;
         char* file = NULL;

         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }
   
         cuser_add_from_file(&answer_list, file);
         show_gdi_request_answer(answer_list); 
         spp++;
         continue;
      }
#endif

/*----------------------------------------------------------------------------*/

#ifndef __SGE_NO_USERMAPPING__
      /* "-aumap user"  */
      if (strcmp("-aumap", *spp) == 0) {
         lList *answer_list = NULL;

         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());

         spp = sge_parser_get_next(spp);
         sge_gdi_is_manager(uti_state_get_user_name());
         cuser_add(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }
#endif

#ifdef __SGE_CENTRY_DEBUG__

/*----------------------------------------------------------------------------*/

      /* "-ace attribute"  */
      if (strcmp("-ace", *spp) == 0) {
         lList *answer_list = NULL;

         sge_gdi_is_adminhost(uti_state_get_qualified_hostname());
         sge_gdi_is_manager(uti_state_get_user_name());

         spp = sge_parser_get_next(spp);
         sge_gdi_is_manager(uti_state_get_user_name());
         centry_add(&answer_list, *spp);
         show_gdi_request_answer(answer_list);
         spp++;
         continue;
      }

#endif

/*----------------------------------------------------------------------------*/

      /* "-suser username" */

      if (strcmp("-suser", *spp) == 0) {
         const char*  user = NULL;
         lList* uList = NULL;
         lListElem* uep = NULL;
         bool first = true;
#ifdef QCONF_FLATFILE         
         spooling_field *fields = NULL;
#endif

         spp = sge_parser_get_next(spp);

         lString2List(*spp, &uList, ST_Type, ST_name , ", ");
         for_each(uep,uList)
         {
            user = lGetString(uep,ST_name);
            /* get user */
            where = lWhere("%T( %I==%s )", UP_Type, UP_name, user);
            what = lWhat("%T(ALL)", UP_Type);
            alp = sge_gdi(SGE_USER_LIST, SGE_GDI_GET, &lp, where, what);
            where = lFreeWhere(where);
            what = lFreeWhat(what);
                        
            if (first) {
               first = false;
            }
            else {
               printf("\n");
            }
            
            aep = lFirst(alp);
            answer_exit_if_not_recoverable(aep);
            if (answer_get_status(aep) != STATUS_OK) {
               fprintf(stderr, "%s", lGetString(aep, AN_text));
               continue;
            }

            if (!lp) {
               fprintf(stderr, MSG_USER_XISNOKNOWNUSER_S, user);
               continue;
            }
            alp = lFreeList(alp);
            ep = lFirst(lp);
            
            /* print to stdout */
#ifdef QCONF_FLATFILE
            fields = sge_build_UP_field_list (0, 1);
            spool_flatfile_write_object(&alp, ep, false, fields, &qconf_param_sfi,
                                                 SP_DEST_STDOUT, SP_FORM_ASCII, 
                                                 NULL, false);
#else
            write_userprj(&alp, ep, NULL, stdout, 0, 1);
#endif
            alp = lFreeList(alp);
         }
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-sprj projectname" */

      if (strcmp("-sprj", *spp) == 0) {
#ifdef QCONF_FLATFILE         
         spooling_field *fields = NULL;
#endif
         spp = sge_parser_get_next(spp);

         /* get project */
         where = lWhere("%T( %I==%s )", UP_Type, UP_name, *spp);
         what = lWhat("%T(ALL)", UP_Type);
         alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);
                  
         aep = lFirst(alp);
         answer_exit_if_not_recoverable(aep);
         if (answer_get_status(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }

         if (!lp) {
            fprintf(stderr, MSG_PROJECT_XISNOKNWOWNPROJECT_S, *spp);
            continue;
         }
         alp = lFreeList(alp);
         ep = lFirst(lp);
         
         /* print to stdout */
#if QCONF_FLATFILE
         fields = sge_build_UP_field_list (0, 0);
         spool_flatfile_write_object(&alp, ep, false, fields, &qconf_param_sfi,
                                              SP_DEST_STDOUT, SP_FORM_ASCII, 
                                              NULL, false);
#else
         write_userprj(&alp, ep, NULL, stdout, 0, 0);
#endif
         alp = lFreeList(alp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-ss" */
      if (strcmp("-ss", *spp) == 0) {
         show_object_list(SGE_SUBMITHOST_LIST, SH_Type, SH_name, 
               "submit"); 
         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-sul" */

      if (strcmp("-sul", *spp) == 0) {
         show_object_list(SGE_USERSET_LIST, US_Type, US_name, 
               "userset list"); 
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

       /* "-suserl" */

      if (strcmp("-suserl", *spp) == 0) {
         show_object_list(SGE_USER_LIST, UP_Type, UP_name, 
               "user list"); 
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

       /* "-sprjl" */

      if (strcmp("-sprjl", *spp) == 0) {
         show_object_list(SGE_PROJECT_LIST, UP_Type, UP_name, 
               "project list"); 
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-tsm" trigger scheduler monitoring */

      if (strcmp("-tsm", *spp) == 0) {
         /* no adminhost/manager check needed here */

         alp = gdi_tsm(NULL, NULL);
         answer_list_on_error_print_or_exit(&alp, stderr);
         alp = lFreeList(alp);
         
         spp++;
         continue;
      } 

/*----------------------------------------------------------------------------*/
      /* "-huh?" */

      ERROR((SGE_EVENT, MSG_ANSWER_INVALIDOPTIONARGX_S, *spp));
      fprintf(stderr, MSG_SRC_X_HELP_USAGE_S , "qconf");
      SGE_EXIT(1);
   }

   DEXIT;
   return sge_parse_return;
}

/***********************************************************************/

static void parse_name_list_to_cull(
char *name,
lList **lpp,
lDescr *dp,
int nm,
char *s 
) {
   char *cp2 = NULL;
   lListElem *ep = NULL;
   int pos;
   int dataType;

   DENTER(TOP_LAYER, "parse_name_list_to_cull");

   
   *lpp = lCreateList(name, dp);
   cp2 = sge_strtok(s, ",");
   ep = lCreateElem(dp);
 
   pos = lGetPosInDescr(dp, nm);
   dataType = lGetPosType(dp,pos);
   switch (dataType) {
      case lStringT:
         DPRINTF(("parse_name_list_to_cull: Adding lStringT type element\n"));
         lSetString(ep, nm, cp2);
         break;
      case lHostT:
         DPRINTF(("parse_name_list_to_cull: Adding lHostT type element\n"));
         lSetHost(ep, nm, cp2);
         break;
      default:
         DPRINTF(("parse_name_list_to_cull: unexpected data type\n"));
         break;
   }
   lAppendElem(*lpp, ep);

   while ((cp2 = sge_strtok(0, ",")) != NULL) {
      ep = lCreateElem(dp);
      switch (dataType) {
         case lStringT:
            DPRINTF(("parse_name_list_to_cull: Adding lStringT type element\n"));
            lSetString(ep, nm, cp2);
            break;
         case lHostT:
            DPRINTF(("parse_name_list_to_cull: Adding lHostT type element\n"));
            lSetHost(ep, nm, cp2);
            break;
         default:
            DPRINTF(("parse_name_list_to_cull: unexpected data type\n"));
            break;
      }
      lAppendElem(*lpp, ep);
   }

   DEXIT;
   return;
}

/****************************************************************************/
static int sge_next_is_an_opt(char **pptr) 
{
   DENTER(TOP_LAYER, "sge_next_is_an_opt");

   if (!*(pptr+1)) {
      DEXIT;
      return 1;
   }

   if (**(pptr+1) == '-') {
      DEXIT;
      return 1;
   }

   DEXIT;
   return 0;
}

/****************************************************************************/
static int sge_error_and_exit(const char *ptr) {
   DENTER(TOP_LAYER, "sge_error_and_exit");

   fflush(stderr);
   fflush(stdout);

   if (ptr) {
      fprintf(stderr, "%s\n", ptr);
      fflush(stderr);
   }

   fflush(stderr);
   SGE_EXIT(1);
   DEXIT;
   return 1; /* to prevent warning */
}

static int add_host_of_type(
lList *arglp,
u_long32 target 
) {
   lListElem *argep=NULL, *ep=NULL;
   lList *lp=NULL, *alp=NULL;
   const char *host = NULL;
   int nm = NoName;
   lDescr *type = NULL;
   char *name = NULL;
   int ret = 0;

   DENTER(TOP_LAYER, "add_host_of_type");

   switch (target) {
      case SGE_SUBMITHOST_LIST:
         nm = SH_name;
         type = SH_Type;
         name = "submit host";
         break;
      case SGE_ADMINHOST_LIST:
         nm = AH_name;
         type = AH_Type;
         name = "administrative host";
         break;
      default:
         DPRINTF(("add_host_of_type: unexpected type\n"));
         ret |= 1;
         DEXIT;
         return ret;
   }
   
   for_each (argep, arglp) {
      /* resolve hostname */
      if (sge_resolve_host(argep, nm) != CL_RETVAL_OK) {
         ret |= 1;
         fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetHost(argep, nm));
         continue;
      }
      host = lGetHost(argep, nm);

      /* make a new host element */
      lp = lCreateList("host to add", type);
      ep = lCopyElem(argep);
      lAppendElem(lp, ep);


      /* add the new host to the host list */
      alp = sge_gdi(target, SGE_GDI_ADD, &lp, NULL, NULL);

      /* report results */
      ep = lFirst(alp);
      answer_exit_if_not_recoverable(ep);
      if (answer_get_status(ep) == STATUS_OK)
         fprintf(stderr, MSG_QCONF_XADDEDTOYLIST_SS, host, name);
      else 
         fprintf(stderr, "%s", lGetString(ep, AN_text));

      lp = lFreeList(lp);
      alp = lFreeList(alp);
   }

   DEXIT;
   return ret;
}

/* ------------------------------------------------------------ */

static int del_host_of_type(
lList *arglp,
u_long32 target 
) {
   lListElem *argep=NULL, *ep=NULL;
   lList *lp=NULL, *alp=NULL;
   int nm = NoName;
   lDescr *type = NULL;

   DENTER(TOP_LAYER, "del_host_of_type");

   switch (target) {
   case SGE_SUBMITHOST_LIST:
      nm = SH_name;
      type = SH_Type;
      break;
   case SGE_ADMINHOST_LIST:
      nm = AH_name;
      type = AH_Type;
      break;
   case SGE_EXECHOST_LIST:
      nm = EH_name;
      type = EH_Type;
      break;
   }

   for_each (argep, arglp) {

      /* make a new host element */
      lp = lCreateList("host to add", type);
      ep = lCopyElem(argep);
      lAppendElem(lp, ep);

      /* delete element */
      alp = sge_gdi(target, SGE_GDI_DEL, &lp, NULL, NULL);

      /* print results */
      ep = lFirst(alp);
      answer_exit_if_not_recoverable(ep);
		fprintf(stderr, "%s\n", lGetString(ep, AN_text));

      alp = lFreeList(alp);
      lp = lFreeList(lp);
   }

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ */

static lListElem* edit_exechost(
lListElem *ep 
) {
   int status;
   lListElem *hep = NULL;
#ifdef QCONF_FLATFILE
   spooling_field *fields = sge_build_EH_field_list (false, false, false);
   lList *alp = NULL;
   int fields_out[MAX_NUM_FIELDS];
   int missing_field = NoName;
#endif

   /* used for generating filenames */
   char *filename = NULL;

   DENTER(TOP_LAYER, "edit_exechost");

#ifdef QCONF_FLATFILE
   filename = (char *)spool_flatfile_write_object(&alp, ep, false, fields,
                                                  &qconf_sfi, SP_DEST_TMP,
                                                  SP_FORM_ASCII, filename,
                                                  false);
   alp = lFreeList (alp);
#else
   filename = write_host(0, 1, ep, EH_name, NULL);
#endif
   status = sge_edit(filename);

   if (status < 0) {
      unlink(filename);
      free(filename);
#ifdef QCONF_FLATFILE
      FREE (fields);
#endif
      if (sge_error_and_exit(MSG_PARSE_EDITFAILED))
         return NULL;
   }

   if (status > 0) {
      unlink(filename);
      free(filename);
#ifdef QCONF_FLATFILE
      FREE (fields);
#endif
      if (sge_error_and_exit(MSG_FILE_FILEUNCHANGED))
         return NULL;
   }
   
#ifdef QCONF_FLATFILE
   fields_out[0] = NoName;
   hep = spool_flatfile_read_object(&alp, EH_Type, NULL,
                                   fields, fields_out, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, filename);
            
   if (answer_list_output (&alp)) {
      hep = lFreeElem (hep);
   }

   if (hep != NULL) {
      missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
   }

   if (missing_field != NoName) {
      hep = lFreeElem (hep);
      answer_list_output (&alp);
   }
#else
   hep = cull_read_in_host(NULL, filename, CULL_READ_MINIMUM, EH_name, 
      NULL, NULL);
#endif
   unlink(filename);
   free(filename);

#ifdef QCONF_FLATFILE
   FREE (fields);
#endif
   DEXIT;
   return hep;
}

/* ------------------------------------------------------------ */

static lList* edit_sched_conf(
lList *confl 
) {
   int status;
   char *fname = NULL;
   lList *alp=NULL, *newconfl=NULL;
#ifdef QCONF_FLATFILE
   lListElem *ep = NULL;
   int fields_out[MAX_NUM_FIELDS];
   int missing_field = NoName;
#endif

   DENTER(TOP_LAYER, "edit_sched_conf");

#ifdef QCONF_FLATFILE   
   fname = (char *)spool_flatfile_write_object(&alp, lFirst(confl), false,
                                       SC_fields, &qconf_comma_sfi,
                                       SP_DEST_TMP, SP_FORM_ASCII, 
                                       fname, false);
   if (answer_list_output(&alp)) {
#else
   if ((fname = write_sched_configuration(0, 1, NULL, lFirst(confl))) == NULL) {
#endif
      fprintf(stderr, MSG_SCHEDCONF_CANTCREATESCHEDULERCONFIGURATION);
      SGE_EXIT(1);
   }

   status = sge_edit(fname);

   if (status < 0) {
      unlink(fname);
      
      if (sge_error_and_exit(MSG_PARSE_EDITFAILED))
         return NULL;
   }

   if (status > 0) {
      unlink(fname);
      
      if (sge_error_and_exit(MSG_FILE_FILEUNCHANGED))
         return NULL;
   }
   
#ifdef QCONF_FLATFILE   
   fields_out[0] = NoName;
   ep = spool_flatfile_read_object(&alp, SC_Type, NULL,
                                   SC_fields, fields_out, true, &qconf_comma_sfi,
                                   SP_FORM_ASCII, NULL, fname);
            
   if (answer_list_output (&alp)) {
      ep = lFreeElem (ep);
   }

   if (ep != NULL) {
      missing_field = spool_get_unprocessed_field (SC_fields, fields_out, &alp);
   }

   if (missing_field != NoName) {
      ep = lFreeElem (ep);
      answer_list_output (&alp);
   }

   if (ep != NULL) {
      newconfl = lCreateList ("scheduler config", SC_Type);
      lAppendElem (newconfl, ep);
   }
   
   if ((newconfl != NULL) && !sconf_validate_config (&alp, newconfl)) {
      newconfl = lFreeList (newconfl);
      answer_list_output(&alp);
   }
   
   if (newconfl == NULL) {
      fprintf(stderr, MSG_QCONF_CANTREADCONFIG_S, "can't parse config");
#else
   if (!(newconfl = read_sched_configuration(NULL, fname, 0, &alp))) {
      aep = lFirst(alp);
      fprintf(stderr, MSG_QCONF_CANTREADCONFIG_S, lGetString(aep, AN_text));
#endif
      unlink(fname);
      SGE_EXIT(1);
   }
   alp = lFreeList(alp);
   
   unlink(fname);

   DEXIT;
   return newconfl;
}

/* ------------------------------------------------------------ */

static lListElem *edit_userprj(
lListElem *ep,
int user        /* =1 user, =0 project */
) {
   int status;
   lListElem *newep = NULL;
   lList *alp = NULL;
   char *filename = NULL;
#ifndef QCONF_FLATFILE
   lListElem *aep = NULL;
#else
   spooling_field *fields = sge_build_UP_field_list (0, user);
   int fields_out[MAX_NUM_FIELDS];
   int missing_field = NoName;
#endif

   DENTER(TOP_LAYER, "edit_userprj");

#ifdef QCONF_FLATFILE
   
   filename = (char *)spool_flatfile_write_object(&alp, ep, false, fields,
                                                  &qconf_sfi, SP_DEST_TMP,
                                                  SP_FORM_ASCII, NULL, false);
   if (answer_list_output(&alp)) {
      FREE (fields);
      sge_error_and_exit(NULL);
   }
#else
   filename = (char *)malloc (sizeof (char) * SGE_PATH_MAX);

   if (!sge_tmpnam(filename)) {
      fprintf(stderr, MSG_FILE_CANTCREATETEMPFILE);
      SGE_EXIT(1);
   }
  
   if (write_userprj(&alp, ep, filename, NULL, 0, user)) {
      aep = lFirst(alp);
      fprintf(stderr, "%s\n", lGetString(aep, AN_text));
      SGE_EXIT(1);
   }

#endif

   alp = lFreeList(alp);

   status = sge_edit(filename);

   if (status < 0) {
#ifdef QCONF_FLATFILE
      FREE (fields);
#endif
      unlink(filename);
      if (sge_error_and_exit(MSG_PARSE_EDITFAILED))
         return NULL;
   }

   if (status > 0) {
#ifdef QCONF_FLATFILE
      FREE (fields);
#endif
      unlink(filename);
      if (sge_error_and_exit(MSG_FILE_FILEUNCHANGED))
         return NULL;
   }

#ifdef QCONF_FLATFILE   
   fields_out[0] = NoName;
   newep = spool_flatfile_read_object(&alp, UP_Type, NULL, fields, fields_out,
                                    true, &qconf_sfi, SP_FORM_ASCII, NULL,
                                    filename);
   
   if (answer_list_output(&alp)) {
      newep = lFreeElem (newep);
   }

   if (newep != NULL) {
      missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
   }

   FREE (fields);
   
   if (missing_field != NoName) {
      newep = lFreeElem (newep);
      answer_list_output (&alp);
   }
#else
   newep = cull_read_in_userprj(NULL, filename, 0, user, NULL);
#endif
   
   unlink(filename);
   FREE (filename);
   
   if (!newep) {
      fprintf(stderr, MSG_QCONF_CANTREADX_S, user?MSG_OBJ_USER:MSG_JOB_PROJECT);
      SGE_EXIT(1);
   }
   
#ifdef QCONF_FLATFILE   
   if (user) {
      NULL_OUT_NONE(newep, UP_default_project);
   }
#endif

   DEXIT;
   return newep;
}

/****************************************************************/
static lListElem *edit_sharetree(
lListElem *ep 
) {
   int status;
   lListElem *newep = NULL;
   char *filename = NULL;
   char errstr[1024];
   lList *alp = NULL;
#ifndef QCONF_FLATFILE
   lListElem *aep = NULL;
#else
   spooling_field *fields = NULL;
   int fields_out[MAX_NUM_FIELDS];
   int missing_field = NoName;
#endif


   DENTER(TOP_LAYER, "edit_sharetree");

   if (!ep) {
      ep = getSNTemplate();
   }

#ifdef QCONF_FLATFILE
   fields = sge_build_STN_field_list (0, 1);
   
   id_sharetree (ep, 0);
   
   filename = (char *)spool_flatfile_write_object(&alp, ep, false,
                                        fields, &qconf_name_value_list_sfi,
                                        SP_DEST_TMP, SP_FORM_ASCII,
                                        NULL, false);
   if (answer_list_output(&alp)) {
      FREE (fields);
      sge_error_and_exit(NULL);
   }
#else
   filename = (char *)malloc (sizeof (char) * SGE_PATH_MAX);

   if (!sge_tmpnam(filename)) {
      fprintf(stderr, MSG_FILE_CANTCREATETEMPFILE);
      SGE_EXIT(1);
   }

   if (write_sharetree(&alp, ep, filename, NULL, 0, 1, 1)) {
      aep = lFirst(alp);
      fprintf(stderr, "%s\n", lGetString(aep, AN_text));
      SGE_EXIT(1);
   }
#endif
   alp = lFreeList(alp);

   status = sge_edit(filename);

   if (status < 0) {
#ifdef QCONF_FLATFILE
      FREE (fields);
#endif
      unlink(filename);
      if (sge_error_and_exit(MSG_PARSE_EDITFAILED))
         return NULL;
   }

   if (status > 0) {
#ifdef QCONF_FLATFILE
      FREE (fields);
#endif
      unlink(filename);
      if (sge_error_and_exit(MSG_FILE_FILEUNCHANGED))
         return NULL;
   }
   
#ifdef QCONF_FLATFILE
   fields_out[0] = NoName;
   newep = spool_flatfile_read_object(&alp, STN_Type, NULL,
                                   fields, fields_out,  true,
                                   &qconf_name_value_list_sfi,
                                   SP_FORM_ASCII, NULL, filename);
   
   if (answer_list_output(&alp)) {
      newep = lFreeElem (newep);
   }

   if (newep != NULL) {
      missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
   }

   FREE (fields);
   
   if (missing_field != NoName) {
      newep = lFreeElem (newep);
      answer_list_output (&alp);
   }
#else
   newep = read_sharetree(fname, NULL, 0, errstr, 1, NULL);
#endif

   unlink(filename);
   FREE (filename);

   if (newep == NULL) {
      fprintf(stderr, MSG_QCONF_CANTREADSHARETREEX_S, errstr);
      SGE_EXIT(1);
   }
   
   DEXIT;
   return newep;
}

/* ------------------------------------------------------------ */

static int show_object_list(
u_long32 target,
lDescr *type,
int keynm,
char *name 
) {
   lEnumeration *what = NULL;
   lCondition *where = NULL;
   lList *alp = NULL, *lp = NULL;
   lListElem *ep = NULL;
   int pos;
   int dataType;
   

   DENTER(TOP_LAYER, "show_object_list");

   what = lWhat("%T(%I)", type, keynm);

   switch (keynm) {
   case EH_name:
      where = lWhere("%T(!(%Ic=%s || %Ic=%s))",
         type, keynm, SGE_TEMPLATE_NAME, 
               keynm, SGE_GLOBAL_NAME );
      break;
   case CONF_hname:
      where = lWhere("%T(!(%I c= %s))",
         type, keynm, SGE_GLOBAL_NAME );
      break;   
   case EV_host:
      where = lWhere("%T(%I==%u))", 
         type, EV_id, EV_ID_SCHEDD);
      break;
   default:
      where = NULL; /* all elements */
      break;
   } 

   alp = sge_gdi(target, SGE_GDI_GET, &lp, where, what);
   what = lFreeWhat(what);
   where = lFreeWhere(where);

   lPSortList(lp, "%I+", keynm);

   ep = lFirst(alp);
   answer_exit_if_not_recoverable(ep);
   if (answer_get_status(ep) != STATUS_OK) {
      fprintf(stderr, "%s\n", lGetString(ep, AN_text));
      return 0;
   }

   if (lp) {
      for_each (ep, lp) {
         const char *line = NULL;
         pos = lGetPosInDescr(type, keynm);
         dataType = lGetPosType(type , pos);
         switch(dataType) {
            case lStringT: 
               line = lGetString(ep, keynm);
               if (line && line[0] != COMMENT_CHAR) { 
                  printf("%s\n", lGetString(ep, keynm));
               }
               break;
            case lHostT:
                line = lGetHost(ep, keynm);
               if (line && line[0] != COMMENT_CHAR) { 
                  printf("%s\n", lGetHost(ep, keynm));
               }
               break;
            default:
               DPRINTF(("show_object_list: unexpected data type\n")); 
         }
      }
   } else {
      fprintf(stderr, MSG_QCONF_NOXDEFINED_S, name);
   }
   
   alp = lFreeList(alp);
   lp = lFreeList(lp);
   
   DEXIT;
   return 0;
}

static int show_eventclients()
{
   lEnumeration *what = NULL;
   lList *alp = NULL, *lp = NULL;
   lListElem *ep = NULL;

   DENTER(TOP_LAYER, "show_eventclients");

   what = lWhat("%T(%I %I %I)", EV_Type, EV_id, EV_name, EV_host);

   alp = sge_gdi(SGE_EVENT_LIST, SGE_GDI_GET, &lp, NULL, what);
   what = lFreeWhat(what);

   ep = lFirst(alp);
   answer_exit_if_not_recoverable(ep);
   if (answer_get_status(ep) != STATUS_OK) {
      fprintf(stderr, "%s\n", lGetString(ep, AN_text));
      DEXIT;
      return -1;
   }

   if (lp) {
      lPSortList(lp, "%I+", EV_id);
   
      printf("%8s %-15s %-25s\n",MSG_TABLE_EV_ID, MSG_TABLE_EV_NAME, MSG_TABLE_HOST);
      printf("--------------------------------------------------\n");
      for_each (ep, lp) {
         printf("%8d ", (int)lGetUlong(ep, EV_id));
         printf("%-15s ", lGetString(ep, EV_name));
         printf("%-25s\n", lGetHost(ep, EV_host));
      }
   }
   else
      fprintf(stderr,  MSG_QCONF_NOEVENTCLIENTSREGISTERED);
   
   alp = lFreeList(alp);
   lp = lFreeList(lp);
   
   DEXIT;
   return 0;
}



static int show_processors()
{
   lEnumeration *what = NULL;
   lCondition *where = NULL;
   lList *alp = NULL, *lp = NULL;
   lListElem *ep = NULL;
   const char *cp = NULL;
   u_long32 sum = 0;

   DENTER(TOP_LAYER, "show_processors");

   what = lWhat("%T(%I %I %I)", EH_Type, EH_name, EH_processors, EH_load_list);
   where = lWhere("%T(!(%Ic=%s || %Ic=%s))", EH_Type, EH_name, 
                  SGE_TEMPLATE_NAME, EH_name, SGE_GLOBAL_NAME);

   alp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_GET, &lp, where, what);
   what = lFreeWhat(what);
   where = lFreeWhere(where);

   ep = lFirst(alp);
   answer_exit_if_not_recoverable(ep);
   if (answer_get_status(ep) != STATUS_OK) {
      fprintf(stderr, "%s\n", lGetString(ep, AN_text));
      DEXIT;
      return -1;
   }

   if (lp) {
      lPSortList(lp,"%I+", EH_name);

      printf("%-25.24s%10.9s%12.11s\n",MSG_TABLE_HOST,MSG_TABLE_PROCESSORS,
            MSG_TABLE_ARCH);
      printf("===============================================\n");
      for_each (ep, lp) {
         lListElem *arch_elem = NULL;

         arch_elem = lGetSubStr(ep, HL_name, "arch", EH_load_list);

         printf("%-25.24s", ((cp = lGetHost(ep, EH_name)) ? cp : ""));
         printf("%10"fu32, lGetUlong(ep, EH_processors));
         if (arch_elem) {
            printf("%12.11s", lGetString(arch_elem, HL_value));
         }
         printf("\n");
         sum += lGetUlong(ep, EH_processors);
      }
      printf("===============================================\n");
/*      printf("%35"fu32"\r%-25.24s\n",sum, MSG_TABLE_SUM_F); */
      printf("%-25.24s%10"fu32"\n",MSG_TABLE_SUM_F,sum);
   }
   else
      fprintf(stderr,  MSG_QCONF_NOEXECUTIONHOSTSDEFINED );
   
   alp = lFreeList(alp);
   lp = lFreeList(lp);
   
   DEXIT;
   return 0;
}

static void show_gdi_request_answer(lList *alp) 
{
   lListElem *aep = NULL;
   DENTER(TOP_LAYER, "show_gdi_request_answer");
   if (alp != NULL) {
    
      for_each(aep,alp) {
         answer_exit_if_not_recoverable(aep);
      }
      aep = lLast(alp);
      fprintf(stderr, "%s", lGetString(aep, AN_text));
   }
   DEXIT;
}

static void show_gdi_request_answer_list(lList *alp) 
{
   lListElem *aep = NULL;
   DENTER(TOP_LAYER, "show_gdi_request_answer");
   if (alp != NULL) {
    
      for_each(aep,alp) {
         answer_exit_if_not_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
      }
   }
   DEXIT;
}

/* - -- -- -- -- -- -- -- -- -- -- -- -- -- -- -

   get all acls listed in acl_arg 
   from qmaster and print them

   returns
      0 on success
      1 if arglp contains acl names not present at qmaster
      -1 failed to get any acl
*/
static int print_acl(
lList *arglp 
) {
   lList *acls = NULL;
   lListElem *argep = NULL, *ep = NULL;
   int fail = 0;
   const char *acl_name = NULL;
#ifndef QCONF_FLATFILE
   lList *alpp = NULL;
#endif
   int first_time = 1;

   DENTER(TOP_LAYER, "print_acl");

   /* get all acls named in arglp, put them into acls */
   if (sge_client_get_acls(NULL, arglp, &acls)) {
      DEXIT;
      return -1;
   }

   for_each (argep, arglp) {
      acl_name = lGetString(argep, US_name);

      ep=lGetElemStr(acls, US_name, acl_name);
      if (ep == NULL) {
         fprintf(stderr, MSG_SGETEXT_DOESNOTEXIST_SS, "access list", acl_name);
         fail = 1;
      }
      else {
#ifdef QCONF_FLATFILE
         lList *alp = NULL;
         
#endif
         if (first_time)
            first_time = 0;
         else {
            printf("\n");
         }
         
#ifdef QCONF_FLATFILE
         spool_flatfile_write_object(&alp, ep, false, US_fields, &qconf_param_sfi,
                                     SP_DEST_STDOUT, SP_FORM_ASCII, NULL,
                                     false);
         alp = lFreeList (alp);
#else
         write_userset(&alpp, ep, NULL, stdout, 0);
#endif
      }
   }

   DEXIT;
   return fail;
}

/**************************************************************
   get all usersets listed in arglp 
   from qmaster and edit them

   This is pure client code, so output to stderr/out is allowed

   returns
      0 on success
      1 if arglp contains userset names not present at qmaster
      -1 failed to get any userset
      -2 if failed due to disk error
 **************************************************************/
static int edit_usersets(
lList *arglp 
) {
   lList *usersets = NULL;
   lListElem *argep=NULL, *ep=NULL, *aep=NULL, *changed_ep=NULL;
   int status;
   const char *userset_name = NULL;
   lList *alp = NULL, *lp = NULL;
   char *fname = NULL;
   int cmd;
#ifdef QCONF_FLATFILE
   int fields_out[MAX_NUM_FIELDS];
   int missing_field = NoName;
#endif

   DENTER(TOP_LAYER, "edit_usersets");

   /* get all usersets named in arglp, put them into usersets */
   if (sge_client_get_acls(NULL, arglp, &usersets)) {
      DEXIT;
      return -1;
   }

   for_each (argep, arglp) {
      alp = NULL;
      userset_name = lGetString(argep, US_name);

      ep=lGetElemStr(usersets, US_name, userset_name);
      if (!ep) {
         ep = lAddElemStr(&usersets, US_name, userset_name, US_Type);
         /* initialize type field in case of sge */
         lSetUlong(ep, US_type, US_ACL|US_DEPT);
         cmd = SGE_GDI_ADD;
      } else {
         cmd = SGE_GDI_MOD;
      }

#ifdef QCONF_FLATFILE
      fname = (char *)spool_flatfile_write_object(&alp, ep, false, US_fields,
                                           &qconf_param_sfi, SP_DEST_TMP,
                                           SP_FORM_ASCII, fname, false);
      if (answer_list_output(&alp)) {
#else
      fname = (char *)malloc (sizeof (char) * SGE_PATH_MAX);
            
      if (sge_tmpnam(fname) == NULL) {
         DEXIT;
         return -2;
      }

      if (write_userset(&alp, ep, fname, NULL, 0)) {
#endif
         fprintf(stderr, MSG_FILE_ERRORWRITINGUSERSETTOFILE);
         DEXIT;
         return -2;
      }
      status = sge_edit(fname);
      if (status < 0) {
         unlink(fname);
         fprintf(stderr, MSG_PARSE_EDITFAILED);
         DEXIT;
         return -2;  /* why should the next edit have more luck */
      }

      if (status > 0) {
         unlink(fname);
         fprintf(stdout, MSG_FILE_FILEUNCHANGED);
         continue;
      }

#ifdef QCONF_FLATFILE
      fields_out[0] = NoName;
      changed_ep = spool_flatfile_read_object(&alp, US_Type, NULL,
                                      US_fields, fields_out,  true, &qconf_param_sfi,
                                      SP_FORM_ASCII, NULL, fname);
      
      if (answer_list_output(&alp)) {
         changed_ep = lFreeElem (changed_ep);
      }

      if (changed_ep != NULL) {
         missing_field = spool_get_unprocessed_field (US_fields, fields_out, &alp);
      }

      if (missing_field != NoName) {
         changed_ep = lFreeElem (changed_ep);
         answer_list_output (&alp);
      }
#else
      changed_ep = cull_read_in_userset(NULL, fname, 0, 0, NULL);
#endif

      if (changed_ep == NULL) {
         fprintf(stderr, MSG_FILE_ERRORREADINGUSERSETFROMFILE_S, fname);
         continue;   /* May be the user made a mistake. Just proceed with 
                        the next */
      }

      /* Create List; append Element; and do a modification gdi call */
      lp = lCreateList("userset list", US_Type);
      lAppendElem(lp, changed_ep);
      alp = sge_gdi(SGE_USERSET_LIST, cmd, &lp, NULL, NULL);
      lp = lFreeList(lp);

      for_each(aep, alp) 
         fprintf(stderr, "%s", lGetString(aep, AN_text));
   }

   FREE (fname);
   DEXIT;
   return 0;
}

/***************************************************************************
  -sconf option 
 ***************************************************************************/
static int print_config(
const char *config_name 
) {
   lCondition *where = NULL;
   lEnumeration *what = NULL;
   lList *alp = NULL, *lp = NULL;
   lListElem *ep = NULL;
   int fail=0;
   const char *cfn = NULL;
#ifdef QCONF_FLATFILE
   spooling_field *fields = NULL;
#endif
   
   DENTER(TOP_LAYER, "print_config");

   /* get config */
   if (!strcasecmp(config_name, "global"))
      cfn = SGE_GLOBAL_NAME;
   else
      cfn = config_name;   
   
   where = lWhere("%T(%I == %s)", CONF_Type, CONF_hname, cfn);
   what = lWhat("%T(ALL)", CONF_Type);
   alp = sge_gdi(SGE_CONFIG_LIST, SGE_GDI_GET, &lp, where, what);
   what = lFreeWhat(what);
   where = lFreeWhere(where);

   ep = lFirst(alp);
   answer_exit_if_not_recoverable(ep);
   if (answer_get_status(ep) != STATUS_OK) {
      fprintf(stderr, "%s\n", lGetString(ep, AN_text));
      fail = 1;
   }
   else {
      if (!(ep = lFirst(lp))) {
         fprintf(stderr, MSG_ANSWER_CONFIGXNOTDEFINED_S, cfn);
         DEXIT;
         return 1;
      }
      printf("%s:\n", cfn);
      
#ifdef QCONF_FLATFILE
      fields = sge_build_CONF_field_list (false);
      spool_flatfile_write_object(&alp, ep, false, fields, &qconf_sfi,
                                  SP_DEST_STDOUT, SP_FORM_ASCII, NULL, false);
      FREE (fields);
      
      if (answer_list_output(&alp)) {
         sge_error_and_exit(NULL);
      }
#else
      write_configuration(0, NULL, NULL, ep, stdout, 0L);
#endif
   }

   alp = lFreeList(alp);
   lp = lFreeList(lp);

   DEXIT;
   return fail;
}

/*------------------------------------------------------------------------*
 * delete_config
 *------------------------------------------------------------------------*/
static int delete_config(
const char *config_name 
) {
   lList *alp = NULL, *lp = NULL;
   lListElem *ep = NULL;
   int fail = 0;
   
   DENTER(TOP_LAYER, "delete_config");

   lAddElemHost(&lp, CONF_hname, config_name, CONF_Type);
   alp = sge_gdi(SGE_CONFIG_LIST, SGE_GDI_DEL, &lp, NULL, NULL);

   ep = lFirst(alp);
   fprintf(stderr, "%s\n", lGetString(ep, AN_text));

   answer_exit_if_not_recoverable(ep);
   fail = !(answer_get_status(ep) == STATUS_OK);

   alp = lFreeList(alp);
   lp = lFreeList(lp);

   DEXIT;
   return fail;
}

/*------------------------------------------------------------------------*
 * add_modify_config
 ** flags = 1 = add, 2 = modify, 3 = modify if exists, add if not
 *------------------------------------------------------------------------*/
static int add_modify_config(
const char *cfn,
const char *filename,
u_long32 flags 
) {
   lCondition *where = NULL;
   lEnumeration *what = NULL;
   lList *alp = NULL, *lp = NULL;
   lListElem *ep = NULL;
   int failed=0;
   char *tmpname = NULL;
   int status;
#ifndef QCONF_FLATFILE
   stringT str;
#else
   spooling_field *fields = NULL;
   int fields_out[MAX_NUM_FIELDS];
   int missing_field = NoName;
#endif
   
   DENTER(TOP_LAYER, "add_modify_config");

   where = lWhere("%T(%I == %s)", CONF_Type, CONF_hname, cfn);
   what = lWhat("%T(ALL)", CONF_Type);
   alp = sge_gdi(SGE_CONFIG_LIST, SGE_GDI_GET, &lp, where, what);
   what = lFreeWhat(what);
   where = lFreeWhere(where);

   failed = false;
   ep = lFirst(alp);
   answer_exit_if_not_recoverable(ep);
   if (answer_get_status(ep) != STATUS_OK) {
      fprintf(stderr, "%s\n", lGetString(ep, AN_text));
      alp = lFreeList(alp);
      lp = lFreeList(lp);
      DEXIT;
      return 1;
   }

   alp = lFreeList(alp);

   ep = lCopyElem(lFirst(lp));
   lp = lFreeList(lp);

   if (ep && (flags == 1)) {
      fprintf(stderr, MSG_ANSWER_CONFIGXALREADYEXISTS_S, cfn);
      ep = lFreeElem(ep);
      DEXIT;
      return 2;
   }
   if (!ep && (flags == 2)) {
      fprintf(stderr, MSG_ANSWER_CONFIGXDOESNOTEXIST_S, cfn);
      ep = lFreeElem(ep);
      DEXIT;
      return 3;
   }
   

   if (filename == NULL) {
      /* get config or make an empty config entry if none exists */
      if (ep == NULL) {
         ep = lCreateElem(CONF_Type);
         lSetHost(ep, CONF_hname, cfn);
      }   

#ifdef QCONF_FLATFILE
      fields = sge_build_CONF_field_list (false);
      tmpname = (char *)spool_flatfile_write_object(&alp, ep, false, fields,
                                            &qconf_sfi, SP_DEST_TMP, SP_FORM_ASCII, 
                                            tmpname, false);
#else
      /* make temp file */
      if (!(tmpname = sge_tmpnam(str))) {
         sge_error_and_exit( MSG_FILE_CANTCREATETEMPFILE);
      }
      close(creat(tmpname, 0755));
      DPRINTF(("tmpname is: %s\n", tmpname));
   
      write_configuration(0, NULL, tmpname, ep, NULL, 0L);
#endif
   
      ep = lFreeElem(ep);
      status = sge_edit(tmpname);
      
      if (status != 0) {
         unlink(tmpname);
         failed = true;
#ifdef QCONF_FLATFILE
         FREE (fields);
#endif
      }
      if (status < 0) {
         fprintf(stderr, MSG_PARSE_EDITFAILED);
#ifdef QCONF_FLATFILE
         FREE (fields);
#endif
         DEXIT;
         return failed;
      }
      else if (status > 0) {
         fprintf(stderr,MSG_ANSWER_CONFIGUNCHANGED  );
#ifdef QCONF_FLATFILE
         FREE (fields);
#endif
         DEXIT;
         return failed;
      }
#ifdef QCONF_FLATFILE
      fields_out[0] = NoName;
      ep = spool_flatfile_read_object(&alp, CONF_Type, NULL,
                                      fields, fields_out, false, &qconf_sfi,
                                      SP_FORM_ASCII, NULL, tmpname);
      
      if (answer_list_output(&alp)) {
         ep = lFreeElem (ep);
      }

      if (ep != NULL) {
         missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
      }
      
      FREE (fields);

      if (missing_field != NoName) {
         ep = lFreeElem (ep);
         answer_list_output (&alp);
      }
            
      if (ep != NULL) {
         lSetHost(ep, CONF_hname, cfn);
      }
      else {
#else
      if (!(ep = read_configuration(tmpname, cfn, 0L))) {
#endif
         fprintf(stderr, MSG_ANSWER_ERRORREADINGTEMPFILE);
         unlink(tmpname);
         failed = true;
         DEXIT;
         return failed;
      }
      unlink(tmpname);
   }
   else {
#ifdef QCONF_FLATFILE
      fields_out[0] = NoName;
      fields = sge_build_CONF_field_list (false);
      ep = spool_flatfile_read_object(&alp, CONF_Type, NULL,
                                      fields, fields_out, false, &qconf_sfi,
                                      SP_FORM_ASCII, NULL, filename);
      
      if (answer_list_output(&alp)) {
         ep = lFreeElem (ep);
      }

      if (ep != NULL) {
         missing_field = spool_get_unprocessed_field (fields, fields_out, &alp);
      }

      FREE (fields);
      
      if (missing_field != NoName) {
         ep = lFreeElem (ep);
         answer_list_output (&alp);
      }
            
      if (ep != NULL) {
         lSetHost(ep, CONF_hname, cfn);
      }
#else
      ep = read_configuration(filename, cfn, 0L);
#endif

      if (!ep) {
         fprintf(stderr, MSG_ANSWER_ERRORREADINGCONFIGFROMFILEX_S, filename);
         failed = true;
         DEXIT;
         return failed;
      }      
   }

   lp = lCreateList("modified configuration", CONF_Type); 
   lAppendElem(lp, ep);
     
   alp = sge_gdi(SGE_CONFIG_LIST, SGE_GDI_MOD, &lp, NULL, NULL);
   lp = lFreeList(lp);
            
   /* report results */
   ep = lFirst(alp);                   
        
   answer_exit_if_not_recoverable(ep); 
   failed = !(answer_get_status(ep) == STATUS_OK);
         
   fprintf(stderr, "%s\n", lGetString(ep, AN_text));
     
   alp = lFreeList(alp);
   
   DEXIT;
   return failed;
}




/*
** NAME
**   sge_gdi_is_manager
** PARAMETER
**   user     - user name to check
** RETURN
**   
** EXTERNAL
**
** DESCRIPTION
**   retrieves manager list from qmaster and checks if the
**   given user is manager
*/
static int sge_gdi_is_manager(
const char *user 
) {
   int perm_return;
   lList *alp = NULL; 
   DENTER(TOP_LAYER, "sge_gdi_is_manager");

   
   if (!user || !*user) {
      /* no input name */
      DEXIT;
      return -1;
   }
   perm_return = sge_gdi_check_permission(&alp, MANAGER_CHECK);
   if (perm_return == true) {
     /* user is manager */
     if (alp != NULL) {
        alp = lFreeList(alp);
        alp = NULL;
     }
     DEXIT;
     return 1;
   }

   /*
   ** user is no manager
   */
   if (perm_return == -10 ) {
      /* fills SGE_EVENT with diagnosis information */
      if (alp != NULL) {
         lListElem *aep;
         if (lGetUlong(aep = lFirst(alp), AN_status) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
      }
   } else {
      fprintf(stderr, MSG_SGETEXT_MUSTBEMANAGER_S , user);
   }

   if (alp != NULL) {
      alp = lFreeList(alp);
      alp = NULL;
   }

   SGE_EXIT(1);
   return 0;
}



/*
** NAME
**   sge_gdi_is_adminhost
** PARAMETER
**   host     -  resolved hostname to check
** RETURN
**   
** EXTERNAL
**
** DESCRIPTION
**   retrieves adminhost list from qmaster and checks if the
**   given host is contained 
*/
static int sge_gdi_is_adminhost(
const char *host 
) {
   lCondition *where = NULL;
   lEnumeration *what = NULL;
   lList *alp = NULL;
   lListElem *aep = NULL;
   lList *lp = NULL;
   lListElem  *ep = NULL;

   DENTER(TOP_LAYER, "sge_gdi_is_adminhost");

   if (!host || !*host) {
      DEXIT;
      return -1;
   }

   DPRINTF(("host: '%s'\n", host));

   /*
   ** GET SGE_ADMINHOST_LIST 
   */
   where = lWhere("%T(%Ih=%s)", AH_Type, AH_name, host);
   what = lWhat("%T(ALL)", AH_Type);
   alp = sge_gdi(SGE_ADMINHOST_LIST, SGE_GDI_GET, &lp, where, what);
   what = lFreeWhat(what);
   where = lFreeWhere(where);

   if (!alp) {
      SGE_EXIT(1);
   }
   if (lGetUlong(aep = lFirst(alp), AN_status) != STATUS_OK) {
      fprintf(stderr, "%s", lGetString(aep, AN_text));
      alp = lFreeList(alp);
      SGE_EXIT(1);
   }
   alp = lFreeList(alp);

   ep = host_list_locate(lp, host);

   if (!ep) {
      /*
      ** host is no adminhost
      */
      lp = lFreeList(lp);
      fprintf(stderr, MSG_ANSWER_DENIEDHOSTXISNOADMINHOST_S, host);
      SGE_EXIT(1);
   }

   lp = lFreeList(lp);
   DEXIT;
   return 0;
}

/****** src/qconf_modify_attribute() ******************************************
*  NAME
*     qconf_modify_attribute() -- sends a modify request to the master 
*
*  SYNOPSIS
*
*     static int qconf_modify_attribute (lList **alpp, int from_file,
*                                        char ***spp, int sub_command,
*                                        struct object_info_entry *info_entry); 
*
*
*  FUNCTION
*     The function performs a SGE_GDI_MOD request to the qmaster.
*     It will get all necessary infomation from commandline or
*     file. 
*     Depending on the parameters specified, the function will
*     modify only parts of an object. It is possible to address
*     only parts of a sublist of an object.
*     
*
*  INPUTS
*     alpp        - reference to an answer list where the master will
*                 store necessary messages for the user 
*
*     from_file   - if set to 1 then the next commandline parameter 
*                   (stored in spp) will contain a filename. 
*                   This file contains the 
*                   attributes which should be modified 
*
*     spp         - pending list of commandline parameter
*
*     epp         - this reference will contain the reduced
*                   element which will be parsed from commandline or file
*                   after this function was called
*
*     sub_command - bitmask which will be added to the "command"
*                   parameter of the sge_gdi-request: 
*        SGE_GDI_CHANGE - modify sublist elements
*        SGE_GDI_APPEND - add elements to a sublist
*        SGE_GDI_REMOVE - remove sublist elements
*        SGE_GDI_SET - replace the complete sublist
*
*     info_entry -  pointer to a structure with function 
*                   pointers, string pointers, and CULL names
*
*  RESULT
*     [alpp] Masters answer for the gdi request
*     1 for error
*     0 for success
******************************************************************************/
static int qconf_modify_attribute(lList **alpp, int from_file, char ***spp,
                                  lListElem **epp, int sub_command, 
                                  struct object_info_entry *info_entry) 
{
   int fields[150];
   lListElem *add_qp = NULL;
   lList *qlp = NULL;
   lEnumeration *what = NULL;
   
   DENTER(TOP_LAYER, "qconf_modify_attribute");    

   DTRACE;

   fields[0] = NoName;

   if (from_file) {
      if (sge_next_is_an_opt(*spp))  {
         SGE_ADD_MSG_ID( sprintf(SGE_EVENT, 
            MSG_ANSWER_MISSINGFILENAMEASOPTIONARG_S, "qconf"));
         answer_list_add(alpp, SGE_EVENT, 
                         STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         return 1;
      }                
      DTRACE;
#ifdef QCONF_FLATFILE
      *epp = spool_flatfile_read_object(alpp, info_entry->cull_descriptor,
                                        NULL, info_entry->fields, fields,
                                        true, &qconf_sfi, SP_FORM_ASCII,
                                        NULL, **spp);
            
      if (answer_list_output(alpp)) {
         return 1;
      }
#else
      *epp = info_entry->cull_read_in_object(alpp, **spp, 0, 
         info_entry->nm_name, NULL, fields);
#endif
      DTRACE;
   } else {
#ifdef QCONF_FLATFILE
      const char *name = NULL;
      const char *value = NULL;
      const char *filename = NULL;

      name = (const char *)strdup (**spp);
      *spp = sge_parser_get_next (*spp);
      value = (const char *)strdup (**spp);

      filename = write_attr_tmp_file (name, value,
                                      qconf_sfi.name_value_delimiter);

      *epp = spool_flatfile_read_object(alpp, info_entry->cull_descriptor, NULL,
                                info_entry->fields, fields, true, &qconf_sfi,
                                SP_FORM_ASCII, NULL, filename);
      
      unlink (filename);
      FREE (filename);
      FREE (name);
      FREE (value);
      
      if (answer_list_output(alpp)) {
         return 1;
      }
#else
      lListElem *cfep = NULL;
      lList *cflp = NULL;
      int tag;

      /* build up a config name/value list 
         as it is used by read_queue_work() */
      cfep = lAddElemStr(&cflp, CF_name, **spp, CF_Type);
      *spp = sge_parser_get_next(*spp);
      lSetString(cfep, CF_value, **spp);

      DTRACE;

      /* transform config list into a queue element */
      *epp = lCreateElem(info_entry->cull_descriptor);

      if (info_entry->read_objectname_work(alpp, &cflp, fields, *epp, 0, 
            info_entry->nm_name, &tag, sub_command==SGE_GDI_REMOVE?1:0)) {
         return 1;
      }                           

      DTRACE;

      if (lGetNumberOfElem(cflp) > 0) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, 
                        MSG_QCONF_XISNOTAOBJECTATTRIB_SSS, "qconf", 
                        lGetString(lFirst(cflp), CF_name), 
                        info_entry->object_name));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, 
                         ANSWER_QUALITY_ERROR);
         return 1;
      }
#endif
   }
   /* add object name to int vector and transform
      it into an lEnumeration */

   DTRACE;

   if (add_nm_to_set(fields, info_entry->nm_name) < 0) {
      SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_QCONF_CANTCHANGEOBJECTNAME_SS, "qconf", 
         info_entry->attribute_name));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      return 1;
   }

   DTRACE;

   if (!(what = lIntVector2What(info_entry->cull_descriptor, fields))) {
      SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_QCONF_INTERNALFAILURE_S, "qconf"));
      return 1;
   }     

   DTRACE;

   while (!sge_next_is_an_opt(*spp)) { 
      *spp = sge_parser_get_next(*spp);
      if (!qlp)
         qlp = lCreateList("list", info_entry->cull_descriptor);
      add_qp = lCopyElem(*epp);
      switch(lGetType(add_qp->descr, info_entry->nm_name)) {
         case lUlongT:
            lSetUlong(add_qp, info_entry->nm_name, atol(**spp));
            break;
         case lLongT:
            lSetLong(add_qp, info_entry->nm_name, atol(**spp));
            break;
         case lIntT:
            lSetInt(add_qp, info_entry->nm_name, atoi(**spp));
            break;
         case lFloatT:
            lSetDouble(add_qp, info_entry->nm_name, atof(**spp));
            break;
         case lDoubleT:
            lSetDouble(add_qp, info_entry->nm_name, atof(**spp));
            break;
         case lCharT:
            lSetChar(add_qp, info_entry->nm_name, **spp[0]);
            break;
         case lStringT:
            lSetString(add_qp, info_entry->nm_name, **spp);
            break;
         case lHostT:   
            lSetHost(add_qp, info_entry->nm_name, **spp);
            break;
         default:
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_QCONF_INTERNALFAILURE_S, "qconf"));
            answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            return 1;
      }
      lAppendElem(qlp, add_qp);
   }

   DTRACE;

   if (!qlp) {
      SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_QCONF_MQATTR_MISSINGOBJECTLIST_S, 
         "qconf"));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      return 1;
   }

   if (info_entry->pre_gdi_function == NULL || 
      info_entry->pre_gdi_function(qlp, alpp)) {
      *alpp = sge_gdi(info_entry->target, SGE_GDI_MOD | sub_command, &qlp, 
                      NULL, what);
   }
   what = lFreeWhat(what);
   qlp = lFreeList(qlp);
   *epp = lFreeElem(*epp);
   (*spp)++;

   DEXIT;
   return 0;
}

static const char *write_attr_tmp_file (const char *name, const char *value, 
                                        char delimiter)
{
   char *filename = (char *)malloc (sizeof (char) * SGE_PATH_MAX);
   FILE *fp = NULL;

   if (sge_tmpnam(filename) == NULL) {
      return NULL;
   }

   fp = fopen(filename, "w");
   
   if (fp == NULL) {
      return NULL;
   }
   
   fprintf (fp, "%s", name);
   fprintf (fp, "%c", delimiter);
   fprintf (fp, "%s\n", value);
   
   fclose (fp);
   
   return (const char *)filename;
}
