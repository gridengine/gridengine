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
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <limits.h>
#include <ctype.h>

#include "sge.h"
#include "def.h"
#include "sge_options.h"
#include "sge_gdi_intern.h"
#include "sge_peL.h"
#include "sge_jobL.h"
#include "sge_hostL.h"
#include "sge_ckptL.h"
#include "sge_confL.h"
#include "sge_exit.h"
#include "sge_string.h"
#include "sge_eventL.h"
#include "sge_queueL.h"
#include "sge_identL.h"
#include "sge_manopL.h"
#include "sge_answerL.h"
#include "sge_usersetL.h"
#include "sge_userprjL.h"
#include "sge_complexL.h"
#include "sge_calendarL.h"
#include "sge_schedconfL.h"
#include "sge_usermapL.h"
#include "sge_groupL.h"
#include "sge_share_tree_nodeL.h"
#include "parse.h"
#include "usage.h"
#include "commlib.h"
#include "config.h"
#include "rw_configuration.h"
#include "sge_me.h"
#include "sge_client_access.h"
#include "parse_qconf.h"
#include "sge_host.h"
#include "read_write_host.h"
#include "read_write_pe.h"
#include "read_write_cal.h"
#include "read_write_queue.h"
#include "read_write_ume.h"
#include "read_write_host_group.h"
#include "read_write_host.h"
#include "sge_complex.h"
#include "sched_conf.h"
#include "read_write_userprj.h"
#include "sge_sharetree.h"
#include "sge_userset.h"
#include "sge_feature.h"
#include "read_write_ckpt.h"
#include "read_write_userset.h"
#include "sge_tmpnam.h"
#include "gdi_tsm.h"
#include "gdi_checkpermissions.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_stringL.h"
#include "resolve_host.h"
#include "utility.h"
#include "sge_set_def_sig_mask.h"
#include "scheduler.h"
#include "sge_support.h"
#include "msg_gdilib.h"
#include "sge_stat.h" 
#include "msg_common.h"
#include "sge_spoolmsg.h"
#include "msg_qconf.h"

static int sge_edit(char *fname);
static int sge_next_is_an_opt(char **ptr);
static int sge_error_and_exit(const char *ptr);

/* ------------------------------------------------------------- */
static int show_object_list(u_long32, lDescr *, int, char *);
static int show_processors(void);
static int show_eventclients(void);
#ifndef __SGE_NO_USERMAPPING__
static int show_user_map_entry(char *user);
static void show_gdi_request_answer(lList *alp);

static int show_host_group_entry(char *group);
#endif
/* ------------------------------------------------------------- */
static void parse_name_list_to_cull(char *name, lList **lpp, lDescr *dp, int nm, char *s);
static int add_host_of_type(lList *arglp, u_long32 target);
static int del_host_of_type(lList *arglp, u_long32 target);
static int print_acl(lList *arglp);
static int qconf_modify_attribute(lList **alpp, int from_file, char ***spp, lListElem **epp, int sub_command, struct object_info_entry *info_entry); 
static lListElem *edit_exechost(lListElem *ep);
static int edit_usersets(lList *arglp);

/************************************************************************/
static int add_chg_cmplx(char *cmplx_name, int add, char *fname);

#ifndef __SGE_NO_USERMAPPING__
static lList*     get_user_mapping_list_from_master(const char *user);
static lList*     get_host_group_list_from_master(const char *group);

static int add_user_map_entry(char *user, char *mapname, char *hostname);
static int mod_user_map_entry(char *user);
static int del_user_map_entry(char *user);
static int add_user_map_entry_from_file(char *filename);
static int mod_user_map_entry_from_file(char *filename);

static int add_host_group_entry(char *group);
static int mod_host_group_entry(char *group);
static int del_host_group_entry(char *group);
static int add_host_group_entry_from_file(char *filename);
static int mod_host_group_entry_from_file(char *filename);
#endif
static int print_cmplx(const char *cmplx_name);
static int print_config(const char *config_name);
static int delete_config(const char *config_name);
static int add_modify_config(const char *config_name, const char *filename, u_long32 flags);
static lList* edit_sched_conf(lList *confl);
static lListElem* edit_userprj(lListElem *ep, int user);
static lListElem *edit_sharetree(lListElem *ep);

static char **sge_parser_get_next(char **head);

/************************************************************************/
static int sge_gdi_is_manager(char *user);
static int sge_gdi_is_adminhost(char *host);
/************************************************************************/

/***************************************************************************/
static char **sge_parser_get_next(
char **arg 
) {

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
   u_long32 old_version;
   char *cp = NULL;
   char **spp = NULL;
   int opt;
   int sge_parse_return = 0;

   lEnumeration *what;
   lCondition *where;
   lList *lp=NULL, *arglp=NULL, *alp=NULL, *newlp=NULL;
   lListElem *hep, *ep, *argep, *aep, *newep;

   const char *host = NULL;
   char fname[SGE_PATH_MAX];
   char *filename;
   char *templatename;

   DENTER(TOP_LAYER, "sge_parse_qconf");

   spp = argv;

   while (*spp) {

/*----------------------------------------------------------------------------*/
      /* "-ac complex" */

      if (!strcmp("-ac", *spp)) {
         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

         spp = sge_parser_get_next(spp);
         add_chg_cmplx(*spp, 1, NULL);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-acal cal-name" */

      if (!strcmp("-acal", *spp) ||
          !strcmp("-Acal", *spp)) {
         if (!strcmp("-acal", *spp)) {
            sge_gdi_is_adminhost(me.qualified_hostname);
            sge_gdi_is_manager(me.user_name);

            spp = sge_parser_get_next(spp);
           
            /* get a generic calendar */
            ep = sge_generic_cal(*spp); 
            filename = write_cal(0, 1, ep);
            lFreeElem(ep);
            
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
            ep = cull_read_in_cal(NULL, filename, 0, 0, NULL, NULL); 
            unlink(filename);
            if (!ep) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
                  continue;
            }
         } else {
            spp = sge_parser_get_next(spp);
           
            ep = cull_read_in_cal(NULL, *spp, 0, 0, NULL, NULL); 
            if (!ep) 
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
                  continue;
         }

         /* send it to qmaster */
         lp = lCreateList("cal to add", CAL_Type); 
         lAppendElem(lp, ep);
DPRINTF(("ep: %s %s\n",
   lGetString(ep, CAL_year_calendar)?lGetString(ep, CAL_year_calendar):MSG_SMALLNULL,
   lGetString(ep, CAL_week_calendar)?lGetString(ep, CAL_week_calendar):MSG_SMALLNULL));


         alp = sge_gdi(SGE_CALENDAR_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-Ac complex file" */

      if (!strcmp("-Ac", *spp)) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);
         cp = *spp;
         spp = sge_parser_get_next(spp);
         add_chg_cmplx(cp, 1, *spp);
         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-ackpt ckpt_name" or "-Ackpt fname" */

      if (!strcmp("-ackpt", *spp) ||
          !strcmp("-Ackpt", *spp)) {

         if (!strcmp("-ackpt", *spp)) {
            sge_gdi_is_adminhost(me.qualified_hostname);
            sge_gdi_is_manager(me.user_name);

            spp = sge_parser_get_next(spp);

            /* get a generic ckpt configuration */
            ep = sge_generic_ckpt(*spp); 
            filename = write_ckpt(0, 1, ep);
            lFreeElem(ep);
            
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
            ep = cull_read_in_ckpt(NULL, filename, 0, 0, NULL, NULL);
            unlink(filename);
            if (!ep) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
                  continue;
            }
         } else {
            spp = sge_parser_get_next(spp);

            ep = cull_read_in_ckpt(NULL, *spp, 0, 0, NULL, NULL);
            if (!ep) 
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
                  continue;
         }

         /* send it to qmaster */
         lp = lCreateList("CKPT list to add", CK_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_CKPT_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         lFreeList(alp);
         lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-ae [server_name]" */
      if (!strcmp("-ae", *spp)) {
         char *host = NULL;
         lListElem *hep;

         cp = NULL;
         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);
         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            host = sge_strdup(host, *spp);

            /* try to resolve hostname */
            hep = lCreateElem(EH_Type);
            lSetHost(hep, EH_name, host);

            switch (sge_resolve_host(hep, EH_name)) {
            case 0:
               break;
            case -1:
               fprintf(stderr, MSG_ANSWER_GETUNIQUEHNFAILEDRESX_S,
                  cl_errstr(-1));
               lFreeElem(hep);
               SGE_EXIT(1);
               break;
            default:
               fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetHost(hep, EH_name));
               lFreeElem(hep);
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
         if (sge_get_recoverable(aep) != STATUS_OK) {
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
         case 0:
            break;
         case -1:
            fprintf(stderr, MSG_ANSWER_GETUNIQUEHNFAILEDRESX_S,
               cl_errstr(-1));
            lFreeElem(ep);
            SGE_EXIT(1);
            break;
         default:
            fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetHost(ep, EH_name));
            lFreeElem(ep);
            SGE_EXIT(1);
            break;
         }

         host = sge_strdup(host, lGetHost(ep, EH_name));
         lFreeList(arglp);


         lp = lCreateList("hosts to add", EH_Type);
         lAppendElem(lp, ep);
         alp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_ADD, &lp, NULL, NULL);
         lFreeList(lp);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }

         ep = lFirst(alp);
         if (sge_get_recoverable(ep) == STATUS_OK)
            fprintf(stderr, MSG_EXEC_ADDEDHOSTXTOEXECHOSTLIST_S, host);
         else
            fprintf(stderr, "%s", lGetString(ep, AN_text));
      
         lFreeList(alp);
         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-Ae fname" */
      if (!strcmp("-Ae", *spp)) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp); 

         hep = lCreateElem(EH_Type);

         /* read file */
         lp = lCreateList("exechosts to add", EH_Type); 
         ep = cull_read_in_host(NULL, *spp, CULL_READ_MINIMUM, EH_name, NULL, NULL);
         if (!ep) {
            fprintf(stderr, MSG_ANSWER_INVALIDFORMAT); 
            SGE_EXIT(1);
         }
         lAppendElem(lp, ep);

         /* test host name */
         switch (sge_resolve_host(ep, EH_name)) {
         case 0:
            break;
         case -1:
            fprintf(stderr, MSG_ANSWER_GETUNIQUEHNFAILEDRESX_S,
               cl_errstr(-1));
            lFreeElem(ep);
            SGE_EXIT(1);
            break;
         default:
            fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetHost(ep, EH_name));
            lFreeElem(ep);
            SGE_EXIT(1);
            break;
         }

         alp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         lFreeList(alp);
         lFreeList(lp);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/

      /* "-ah server_name[,server_name,...]" */
      if (!strcmp("-ah", *spp)) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);
         parse_name_list_to_cull("host to add", &lp, AH_Type, AH_name, *spp);
         if ( add_host_of_type(lp, SGE_ADMINHOST_LIST) != 0)
            sge_parse_return = 1; 
         lFreeList(lp);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-am user_list" */

      if (!strcmp("-am", *spp)) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);
         lString2List(*spp, &lp, MO_Type, MO_name, ", ");
         alp = sge_gdi(SGE_MANAGER_LIST, SGE_GDI_ADD, &lp, NULL, NULL);
         for_each(aep,alp) {
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-ao user_list" */

      if (!strcmp("-ao", *spp)) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);
         lString2List(*spp, &lp, MO_Type, MO_name, ", ");
         alp = sge_gdi(SGE_OPERATOR_LIST, SGE_GDI_ADD, &lp, NULL, NULL);
         for_each(aep,alp) {
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-ap pe_name" */

      if (!strcmp("-ap", *spp) ||
          !strcmp("-Ap", *spp)) {

         if (!strcmp("-ap", *spp)) {
            sge_gdi_is_adminhost(me.qualified_hostname);
            sge_gdi_is_manager(me.user_name);
           
            spp = sge_parser_get_next(spp);

            /* get a generic parallel environment */
            ep = sge_generic_pe(*spp); 
            filename = write_pe(0, 1, ep);
            lFreeElem(ep);
            
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
            ep = cull_read_in_pe(NULL, filename, 0, 0, NULL, NULL); 
            unlink(filename);
            if (!ep) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
                  continue;
            }
         } else {
            spp = sge_parser_get_next(spp);

            ep = cull_read_in_pe(NULL, *spp, 0, 0, NULL, NULL); 
            if (!ep) 
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
                  continue;
         }

         /* send it to qmaster */
         lp = lCreateList("PE list to add", PE_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_PE_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-auser" */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-auser", *spp)) {
         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

         spp++;
        
         /* get a template for editing */
         ep = getUserPrjTemplate(); 
         
         ep = edit_userprj(ep, 1);

         /* send it to qmaster */
         lp = lCreateList("User list to add", UP_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_USER_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-aprj" */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-aprj", *spp)) {
         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);
        
         /* get a template for editing */
         ep = getUserPrjTemplate(); 
         
         ep = edit_userprj(ep, 0);

         /* send it to qmaster */
         lp = lCreateList("Project list to add", UP_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-Auser" */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-Auser", *spp)) {
         char* file = NULL;
         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }        


         /* get project  */
         ep = NULL;
         ep = cull_read_in_userprj(NULL, file ,0 ,1 ,0 ); 
         if (ep == NULL) {
            sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE); 
         }
        
         /* send it to qmaster */
         lp = lCreateList("User to add", UP_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_USER_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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


/*----------------------------------------------------------------------------*/

      /* "-Aprj" */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-Aprj", *spp)) {
         char* file = NULL;
         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }        


         /* get project  */
         ep = NULL;
         ep = cull_read_in_userprj(NULL, file ,0 ,0 ,0 ); 
         if (ep == NULL) {
            sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE); 
         }
        
         /* send it to qmaster */
         lp = lCreateList("Project list to add", UP_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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


/*----------------------------------------------------------------------------*/
      /*  "-aq [q_template]" */

      if (!strcmp("-aq", *spp)) {
         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

         /* get template queue from qmaster */
         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            templatename = *spp;
         } else {
            templatename = "template";
         }
         where = lWhere("%T( %I==%s )", QU_Type, QU_qname, templatename);
         what = lWhat("%T(ALL)", QU_Type);
         alp = sge_gdi(SGE_QUEUE_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);

         aep = lFirst(alp);

         /*  we don't need the argument (template queue name) anymore. move on to next option
             so we don't have to do it before every 'continue' */
         spp++;
         if (!lp) {
            fprintf(stderr, MSG_QUEUE_XISNOTAQUEUENAME_S, templatename);
            continue;
         }

         ep = lFirst(lp);

         /* write queue to temp file */
         if (cull_write_qconf(0, FALSE, NULL, NULL, fname, ep)) {
            if (sge_error_and_exit(MSG_QUEUE_UNABLETOWRITETEMPLATEQUEUE))
               continue;
         }

         /* edit template file */
         status = sge_edit(fname);
         if (status < 0) {
            unlink(fname);
            if (sge_error_and_exit(MSG_PARSE_EDITFAILED))
               continue;

         }

         if (status > 0) {
            unlink(fname);
            printf(MSG_FILE_FILEUNCHANGED);
            continue;
         }

         /* read it in again */
         ep = cull_read_in_qconf(NULL, fname, 0, 0, NULL, NULL);
         unlink(fname);
         if (!ep) {
            if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE)) {
               continue;
            }
         }

         /* send it to qmaster */
         lp = lCreateList("Queue to add", QU_Type);
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_QUEUE_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));

         alp = lFreeList(alp);
         lp = lFreeList(lp);

         continue;
      }

/*-----------------------------------------------------------------------------*/

      /* "-as server_name[,server_name,...]" */
      if (!strcmp("-as", *spp)) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);
         parse_name_list_to_cull("host to add", &lp, SH_Type, SH_name, *spp);
         add_host_of_type(lp, SGE_SUBMITHOST_LIST);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-astree" add sharetree */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-astree", *spp)) {
         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

         /* get the sharetree .. */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);
 
         ep = lFirst(lp);
         if (!(ep=edit_sharetree(ep)))
            continue;

         lp = lFreeList(lp);

         newlp = lCreateList("sharetree add", STN_Type);
         lAppendElem(newlp, ep);

         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_ADD, &newlp, NULL, what);
         what = lFreeWhat(what);

         ep = lFirst(alp);
         if (sge_get_recoverable(ep) == STATUS_OK)
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

      if (feature_is_enabled(FEATURE_SGEEE) &&
          ((!strcmp("-astnode", *spp) && (opt=astnode_OPT)) ||
           (!strcmp("-mstnode", *spp) && (opt=mstnode_OPT)))) {
         int modified = 0;
         int print_usage = 0;

         /* no adminhost/manager check needed here */
         spp = sge_parser_get_next(spp);

         /* get the sharetree .. */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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

            lListElem *node;
            char *buf=NULL, *nodepath, *sharestr;
            int shares;
            ancestors_t ancestors;

            buf = sge_strdup(buf, lGetString(argep, STN_name));
            nodepath = sge_strtok(buf, "=");
            sharestr = sge_strtok(NULL, "");
            if (nodepath && sharestr &&
                sscanf(sharestr, "%d", &shares) == 1) {

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
            if (sge_get_recoverable(ep) == STATUS_OK)
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

      if (!strcmp("-au", *spp)) {

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

         for_each(aep,alp) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         arglp = lFreeList(arglp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-Aq fname" */

      if (!strcmp("-Aq", *spp)) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);

         ep = cull_read_in_qconf(NULL, *spp, 0, 0, NULL, NULL);
         if (!ep)
            if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
               continue;

         /* send it to qmaster */
         lp = lCreateList("Queue to add", QU_Type);
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_QUEUE_LIST, SGE_GDI_ADD, &lp, NULL, NULL);

         aep = lFirst(alp);
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));

         alp = lFreeList(alp);
         lp = lFreeList(lp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-clearusage"  clear sharetree usage */

      if (feature_is_enabled(FEATURE_SGEEE) &&
          (!strcmp("-clearusage", *spp))) {
         lList *lp2=NULL;

         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

         /* get user list */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_USER_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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
         if (sge_get_recoverable(aep) != STATUS_OK) {
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
            for_each(aep, alp) {
               sge_get_recoverable(aep);
               fprintf(stderr, "%s", lGetString(aep, AN_text));
            }
         }

         /* update project usage */
         if (lp2) {
            alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_MOD, &lp2, NULL, NULL);
            for_each(aep, alp) {
               sge_get_recoverable(aep);
               fprintf(stderr, "%s", lGetString(aep, AN_text));
            }
         }

         alp = lFreeList(alp);
         lp = lFreeList(lp);
         lp2 = lFreeList(lp2);
         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-cq destin_id[,destin_id,...]" */

      if (!strcmp("-cq", *spp)) {
         /* no adminhost/manager check needed here */
         spp = sge_parser_get_next(spp);
         lString2List(*spp, &lp, ID_Type, ID_str, ", ");
         for_each(ep, lp) {
            lSetUlong(ep, ID_action, QCLEAN);
         }
         alp = sge_gdi(SGE_QUEUE_LIST, SGE_GDI_TRIGGER, &lp, NULL, NULL);
         for_each(aep, alp) {
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);
         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-dc complex_list" */

      if (!strcmp("-dc", *spp)) {
         /* no adminhost/manager check needed here */
         spp = sge_parser_get_next(spp);

         cp = sge_strtok(*spp, ",");
         add_chg_cmplx(cp, 2, NULL);
         while ((cp=sge_strtok(NULL, ","))) {
            add_chg_cmplx(cp, 2, NULL);
         }

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-dcal calendar_name" */

      if (!strcmp("-dcal", *spp)) {
         /* no adminhost/manager check needed here */
         spp = sge_parser_get_next(spp);
         ep = lCreateElem(CAL_Type);
         lSetString(ep, CAL_name, *spp);
         lp = lCreateList("cal's to del", CAL_Type);
         lAppendElem(lp, ep);
         alp = sge_gdi(SGE_CALENDAR_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         for_each(aep,alp) {
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-dckpt ckpt_name" */

      if (!strcmp("-dckpt", *spp)) {
         /* no adminhost/manager check needed here */
         spp = sge_parser_get_next(spp);

         ep = lCreateElem(CK_Type);
         lSetString(ep, CK_name, *spp);
         lp = lCreateList("ckpt interfaces to del", CK_Type);
         lAppendElem(lp, ep);
         alp = sge_gdi(SGE_CKPT_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         for_each(aep,alp) {
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/

      /* "-de server_name[,server_name,...]" */
      if (!strcmp("-de", *spp)) {
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
      if (!strcmp("-dh", *spp)) {
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

      if (!strcmp("-dm", *spp)) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);

         lString2List(*spp, &lp, MO_Type, MO_name, ", ");
         alp = sge_gdi(SGE_MANAGER_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         for_each(aep,alp) {
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-do user_list" */

      if (!strcmp("-do", *spp)) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);

         lString2List(*spp, &lp, MO_Type, MO_name, ", ");
         alp = sge_gdi(SGE_OPERATOR_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         for_each(aep,alp) {
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-dp pe-name" */

      if (!strcmp("-dp", *spp)) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);

         ep = lCreateElem(PE_Type);
         lSetString(ep, PE_name, *spp);
         lp = lCreateList("pe's to del", PE_Type);
         lAppendElem(lp, ep);
         alp = sge_gdi(SGE_PE_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         for_each(aep,alp) {
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-dq destin_id[,destin_id,...]" */

      if (!strcmp("-dq", *spp)) {
         /* no adminhost/manager check needed here */

         if(!sge_next_is_an_opt(spp)) {
            /* queues specified */
            while(!sge_next_is_an_opt(spp)) {
               spp = sge_parser_get_next(spp);
               lString2List(*spp, &lp, QU_Type, QU_qname, ", ");

               alp = sge_gdi(SGE_QUEUE_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
               for_each(aep, alp) {
                  sge_get_recoverable(aep);
                  fprintf(stderr, "%s", lGetString(aep, AN_text));
               }
               alp = lFreeList(alp);
               lp = lFreeList(lp);
            }
         }

         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-ds server_name[,server_name,...]" */
      if (!strcmp("-ds", *spp)) {
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

      if (!strcmp("-du", *spp)) {
         
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
         for_each(aep,alp) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         lp = lFreeList(lp);
         alp = lFreeList(alp);
         arglp = lFreeList(arglp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-dul list_name_list" */

      if (!strcmp("-dul", *spp)) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);

         lString2List(*spp, &lp, US_Type, US_name, ", ");
         alp = sge_gdi(SGE_USERSET_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         for_each(aep,alp) {
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-duser user,..." */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-duser", *spp)) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);

         lString2List(*spp, &lp, UP_Type, UP_name, ", ");
         alp = sge_gdi(SGE_USER_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         for_each(aep,alp) {
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-dprj project,..." */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-dprj", *spp)) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);

         lString2List(*spp, &lp, UP_Type, UP_name, ", ");
         alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
         for_each(aep,alp) {
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-dstnode node_path[,...]"  delete sharetree node(s) */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-dstnode", *spp)) {
         int modified = 0;
         /* no adminhost/manager check needed here */
         spp = sge_parser_get_next(spp);

         /* get the sharetree .. */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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

            lListElem *node;
            const char *nodepath;
            ancestors_t ancestors;

            nodepath = lGetString(argep, STN_name);
            if (nodepath) {

               memset(&ancestors, 0, sizeof(ancestors));
               node = search_named_node_path(ep, nodepath, &ancestors);
               if (node) {
                  if (lGetList(node, STN_children) == NULL) {
                     if (ancestors.depth > 0) {
                        int i;
                        lList *siblings;
                        lListElem *pnode;
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
            if (sge_get_recoverable(ep) == STATUS_OK)
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

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-dstree", *spp)) {
         /* no adminhost/manager check needed here */
         
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_DEL, NULL, NULL, NULL);
         for_each(aep,alp) {
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-help" */

      if (!strcmp("-help", *spp)) {
         sge_usage(stdout);
         SGE_EXIT(0);
      }

/*----------------------------------------------------------------------------*/
      /* "-ks" */

      if (!strcmp("-ks", *spp)) {
         /* no adminhost/manager check needed here */

         alp = gdi_kill(NULL, me.default_cell, 0, SCHEDD_KILL);
         for_each(aep, alp) {
            if (sge_get_recoverable(aep) != STATUS_OK)
               sge_parse_return = 1;
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }

         alp = lFreeList(alp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-km" */

      if (!strcmp("-km", *spp)) {
         /* no adminhost/manager check needed here */
         alp = gdi_kill(NULL, me.default_cell, 0, MASTER_KILL);
         for_each(aep, alp) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* -kec <id> ... */
      /* <id> may be "all" */
      /* parse before -ke[j] */

      if (!strncmp("-kec", *spp, 4)) {
         int opt = EVENTCLIENT_KILL;
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);
         /* found namelist -> process */
         if(strcmp(*spp, "all") == 0) { /* kill all dynamic event clients (EV_ID_ANY) */
            alp = gdi_kill(NULL, me.default_cell, 0, opt);
         } else {
            lString2List(*spp, &lp, ID_Type, ID_str, ", ");
            alp = gdi_kill(lp, me.default_cell, 0, opt);
         }      

         for_each(aep, alp) {
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* -ke[j] <host> ... */
      /* <host> may be "all" */
      if (!strncmp("-k", *spp, 2)) {
         int opt = EXECD_KILL;
         /* no adminhost/manager check needed here */

         cp = (*spp) + 2;
         switch (*cp++) {
            case 'e':
               break;
            default:
               ERROR((SGE_EVENT,MSG_ANSWER_XISNOTAVALIDOPTIONY_SU, *spp, u32c(me.who)));
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
            alp = gdi_kill(NULL, me.default_cell, 0, opt);
         } else {   
            /* found namelist -> process */
            lString2List(*spp, &lp, EH_Type, EH_name, ", ");
            alp = gdi_kill(lp, me.default_cell, 0, opt);
         }

         for_each(aep, alp) {
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         lp = lFreeList(lp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-mc complex_name" */

      if (!strcmp("-mc", *spp)) {
         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

         cp = NULL;

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
         }
         else {
            if (sge_error_and_exit(MSG_COMPLEX_NEEDACOMPLEXNAME))
               continue;
         }

         add_chg_cmplx(*spp, 0, NULL);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-mcal cal-name" */

      if (!strcmp("-mcal", *spp) || 
          !strcmp("-Mcal", *spp)) {
         if (!strcmp("-mcal", *spp)) {
            sge_gdi_is_adminhost(me.qualified_hostname);
            sge_gdi_is_manager(me.user_name);

            spp = sge_parser_get_next(spp);
           
            where = lWhere("%T( %I==%s )", CAL_Type, CAL_name, *spp);
            what = lWhat("%T(ALL)", CAL_Type);
            alp = sge_gdi(SGE_CALENDAR_LIST, SGE_GDI_GET, &lp, where, what);
            where = lFreeWhere(where);
            what = lFreeWhat(what);

            aep = lFirst(alp);
            if (sge_get_recoverable(aep) != STATUS_OK) {
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
            filename = write_cal(0, 1, ep);
            lFreeElem(ep);
            
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
            ep = cull_read_in_cal(NULL, filename, 0, 0, NULL, NULL); 
            unlink(filename);
            if (!ep) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
                  continue;
            }
         } else {
            spp = sge_parser_get_next(spp);
           
            ep = cull_read_in_cal(NULL, *spp, 0, 0, NULL, NULL); 
            if (!ep) 
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
                  continue;
         }

         /* send it to qmaster */
         lp = lCreateList("calendar to add", CAL_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_CALENDAR_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-Mc complex file" */

      if (!strcmp("-Mc", *spp)) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);
         cp = sge_strdup(NULL, *spp);
         spp = sge_parser_get_next(spp);
         add_chg_cmplx(cp, 0, *spp);
         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-mckpt ckpt_name" or "-Mckpt fname" */

      if (!strcmp("-mckpt", *spp) || 
          !strcmp("-Mckpt", *spp)) {
         if (!strcmp("-mckpt", *spp)) {
            sge_gdi_is_adminhost(me.qualified_hostname);
            sge_gdi_is_manager(me.user_name);

            spp = sge_parser_get_next(spp);

            /* get last version of this pe from qmaster */
            where = lWhere("%T( %I==%s )", CK_Type, CK_name, *spp);
            what = lWhat("%T(ALL)", CK_Type);
            alp = sge_gdi(SGE_CKPT_LIST, SGE_GDI_GET, &lp, where, what);
            lFreeWhere(where);
            lFreeWhat(what);

            aep = lFirst(alp);
            if (sge_get_recoverable(aep) != STATUS_OK) {
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
            filename = write_ckpt(0, 1, ep);
            lFreeElem(ep);
            
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
            ep = cull_read_in_ckpt(NULL, filename, 0, 0, NULL, NULL);
            unlink(filename);
            if (!ep) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
                  continue;
            }
         } else {
            spp = sge_parser_get_next(spp);

            ep = cull_read_in_ckpt(NULL, *spp, 0, 0, NULL, NULL);
            if (!ep) 
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
                  continue;
         }

         /* send it to qmaster */
         lp = lCreateList("CKPT list to add", CK_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_CKPT_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-Me fname" */
      if (!strcmp("-Me", *spp)) {
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp); 

         hep = lCreateElem(EH_Type);

         /* read file */
         lp = lCreateList("exechosts to add", EH_Type); 
         ep = cull_read_in_host(NULL, *spp, CULL_READ_MINIMUM, EH_name, 
               NULL, NULL);
         if (!ep) {
            fprintf(stderr, MSG_ANSWER_INVALIDFORMAT); 
            SGE_EXIT(1);
         }

         lAppendElem(lp, ep);

         /* test host name */
         switch (sge_resolve_host(ep, EH_name)) {
         case 0:
            break;
         case -1:
            fprintf(stderr, MSG_ANSWER_GETUNIQUEHNFAILEDRESX_S,
               cl_errstr(-1));
            lFreeElem(ep);
            SGE_EXIT(1);
            break;
         default:
            fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetHost(ep, EH_name));
            lFreeElem(ep);
            SGE_EXIT(1);
            break;
         }

         alp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-me [server_name,...]" */

      if (!strcmp("-me", *spp)) {
         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);
         
         spp = sge_parser_get_next(spp);
         parse_name_list_to_cull("hosts to change", &arglp, EH_Type, EH_name, 
            *spp);

         for_each (argep, arglp) {
            /* resolve hostname */
            switch (sge_resolve_host(argep, EH_name)) {
            case 0:
               break;
            case -1:
               fprintf(stderr, MSG_ANSWER_GETUNIQUEHNFAILEDRESX_S,
                  cl_errstr(-1));
               lFreeElem(argep);
               SGE_EXIT(1);
               break;
            default:
               fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, 
                  lGetHost(argep, EH_name));
               lFreeElem(argep);
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
            if (sge_get_recoverable(aep) != STATUS_OK) {
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
            if (sge_get_recoverable(ep) == STATUS_OK)
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

      if (!strcmp("-mp", *spp) || 
          !strcmp("-Mp", *spp)) {
         if (!strcmp("-mp", *spp)) {
            sge_gdi_is_adminhost(me.qualified_hostname);
            sge_gdi_is_manager(me.user_name);
         
            spp = sge_parser_get_next(spp);

            /* get last version of this pe from qmaster */
            where = lWhere("%T( %I==%s )", PE_Type, PE_name, *spp);
            what = lWhat("%T(ALL)", PE_Type);
            alp = sge_gdi(SGE_PE_LIST, SGE_GDI_GET, &lp, where, what);
            where = lFreeWhere(where);
            what = lFreeWhat(what);

            aep = lFirst(alp);
            if (sge_get_recoverable(aep) != STATUS_OK) {
              fprintf(stderr, "%s", lGetString(aep, AN_text));
               spp++;
               continue;
            }
            alp = lFreeList(alp);

            if (!lp) {
               fprintf(stderr, MSG_PARALLEL_XNOTAPARALLELEVIRONMENT_S, *spp);
               SGE_EXIT(1);
            }

            ep = lFirst(lp);
            filename = write_pe(0, 1, ep);
            lFreeElem(ep);
            
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
            ep = cull_read_in_pe(NULL, filename, 0, 0, NULL, NULL); 
            unlink(filename);
            if (!ep) {
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
                  continue;
            }
         } else {
            spp = sge_parser_get_next(spp);

            ep = cull_read_in_pe(NULL, *spp, 0, 0, NULL, NULL); 
            if (!ep) 
               if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
                  continue;
         }

         /* send it to qmaster */
         lp = lCreateList("PE list to add", PE_Type); 
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_PE_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /*  "-mq queue" */
      if (!strcmp("-mq", *spp)) {
         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

         spp = sge_parser_get_next(spp);

         /* get last version of this queue from qmaster */
         where = lWhere("%T( %I==%s )", QU_Type, QU_qname, *spp);
         what = lWhat("%T(ALL)", QU_Type);
         alp = sge_gdi(SGE_QUEUE_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);

         if (!lp) {
            fprintf(stderr, MSG_QUEUE_XISNOTAQUEUENAME_S, *spp);
            spp++;
            continue;
         }

         /* ok. we don't need the argument (queue name) anymore. move on to next option
            so we don't have to do it before every 'continue' */
         spp++; 

         ep = lFirst(lp);
         
         /* write queue to temp file */
         if (cull_write_qconf(0, FALSE, NULL, NULL, fname, ep)) {
            if (sge_error_and_exit(MSG_QUEUE_UNABLETOWRITEOLDQUEUE))
               continue;
         }
         old_version = lGetUlong(ep, QU_version);

         lp = lFreeList(lp);
         
         /* edit temp file */
         status = sge_edit(fname);
         if (status < 0) {
            unlink(fname);
            if (sge_error_and_exit(MSG_PARSE_EDITFAILED))
               continue;
         }

         if (status > 0) {
            unlink(fname);
            printf(MSG_FILE_FILEUNCHANGED);
            printf("\n");
            continue;
         }

         /* read it in again */
         ep = cull_read_in_qconf(NULL, fname, 0, 0, NULL, NULL);
         unlink(fname);
         if (!ep) {
            if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE)) {
               continue;
            }
         }
         lSetUlong(ep, QU_version, old_version);
         
         /* send it to qmaster */
         lp = lCreateList("Queue to modify", QU_Type);
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_QUEUE_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));

         alp = lFreeList(alp);
         lp = lFreeList(lp);
         
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /*  "-mqattr attr_name value queue_list" */
      /*  "-Mqattr filename queue_list" */
      if (!strcmp("-mqattr", *spp) ||
          !strcmp("-Mqattr", *spp)) {
         int exit = 0;
         int fields[150], from_file;
         lListElem *add_qp;
         lList *qlp = NULL;
         /* no adminhost/manager check needed here */
        
         fields[0] = NoName;

         from_file = !strcmp(*spp, "-Mqattr");
         spp = sge_parser_get_next(spp); /* option str */

         if (from_file) {
            /* "-Mqattr" */
            if (sge_next_is_an_opt(spp))  {
               fprintf(stderr, MSG_ANSWER_MISSINGFILENAMEASOPTIONARG_S,"qconf");  
               SGE_EXIT(1);
            } 
            ep = cull_read_in_qconf(NULL, *spp, 0, 0, NULL, fields);
         } else {
            lListElem *cfep;
            lList *cflp = NULL;
            int tag;
            /* "-mqattr" */

            /* build up a config name/value list as 
               it is used by read_queue_work() */
            cfep = lAddElemStr(&cflp, CF_name, *spp, CF_Type);
            spp = sge_parser_get_next(spp);
            lSetString(cfep, CF_value, *spp);

            /* transform config list into a queue element */
            ep = lCreateElem(QU_Type);
            if (read_queue_work(&alp, &cflp, fields, ep, 0, 0, &tag, 0)) {
               for_each (aep, alp) 
                  fprintf(stderr, "%s", lGetString(aep, AN_text));
               SGE_EXIT(1);
            }

            if (lGetNumberOfElem(cflp)>0) {
               fprintf(stderr, MSG_QCONF_XISNOTAQUEUEATTRIB_SS, "qconf", 
                  lGetString(lFirst(cflp), CF_name));
               SGE_EXIT(1);
            }
         }

         /* add QU_qname to int vector and transform 
            it into an lEnumeration */ 
         if (add_nm_to_set(fields, QU_qname)<0) {
            /* someone tries to modify the queues name */
            fprintf(stderr, MSG_QCONF_CANTCHANGEQUEUENAME_S,"qconf");  
            SGE_EXIT(1);
         }
         if (!(what = lIntVector2What(QU_Type, fields))) {
            fprintf(stderr, MSG_QCONF_INTERNALFAILURE_S,"qconf");  
            SGE_EXIT(1);
         }

         while (!sge_next_is_an_opt(spp)) { /* queue name */
            spp = sge_parser_get_next(spp);
            if (!qlp)
               qlp = lCreateList("qlist", QU_Type);
            add_qp = lCopyElem(ep);
            lSetString(add_qp, QU_qname, *spp);
            lAppendElem(qlp, add_qp);
         }
         if (!qlp) {
            fprintf(stderr, MSG_QCONF_MQATTR_MISSINGQUEUELIST_S, 
               "qconf -mqattr");  
            SGE_EXIT(1);
         }
   
         alp = sge_gdi(SGE_QUEUE_LIST, SGE_GDI_MOD, &qlp, NULL, what);
         what = lFreeWhat(what);
         for_each (aep, alp) {
            printf("%s", lGetString(aep, AN_text));
            if (lGetUlong(aep, AN_status)!=STATUS_OK)
               exit = 1;
         }
         alp = lFreeList(alp);
         
         qlp = lFreeList(qlp);
         ep = lFreeElem(ep);
         spp++;

         if (exit) 
            SGE_EXIT(1);
         continue;
      }

/*-----------------------------------------------------------------------------*/

   if (!strcmp("-mattr", *spp) || !strcmp("-Mattr", *spp) ||
       !strcmp("-aattr", *spp) || !strcmp("-Aattr", *spp) ||   
       !strcmp("-rattr", *spp) || !strcmp("-Rattr", *spp) ||   
       !strcmp("-dattr", *spp) || !strcmp("-Dattr", *spp)) {   
     
/* *INDENT-OFF* */ 
      static object_info_entry info_entry[] = {
         {SGE_QUEUE_LIST,    SGE_OBJ_QUEUE,    QU_Type,  SGE_ATTR_QNAME,         QU_qname, read_queue_work, cull_read_in_qconf},
         {SGE_EXECHOST_LIST, SGE_OBJ_EXECHOST, EH_Type,  SGE_ATTR_HOSTNAME,      EH_name,  read_host_work,  cull_read_in_host},
         {SGE_PE_LIST,       SGE_OBJ_PE,       PE_Type,  SGE_ATTR_PE_NAME,       PE_name,  read_pe_work,    cull_read_in_pe},
         {SGE_CKPT_LIST,     SGE_OBJ_CKPT,     CK_Type,  SGE_ATTR_CKPT_NAME,     CK_name,  read_ckpt_work,  cull_read_in_ckpt},
#if 0
         {SGE_CALENDAR_LIST, SGE_OBJ_CALENDAR, CAL_Type, SGE_ATTR_CALENDAR_NAME, CAL_name, read_cal_work,   cull_read_in_cal},
#endif
         {0,                 NULL,             0,        NULL,                   0,        NULL,            NULL}
      }; 
/* *INDENT-ON* */
      
      int from_file;
      int index;
      int ret = 0;
      int sub_command = 0;
   
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
         SGE_EXIT(1);
      } 

      /* */
      ret = qconf_modify_attribute(&alp, from_file, &spp, &ep, 
            sub_command, &(info_entry[index])); 

      /* Error handling */
      if (ret || lGetNumberOfElem(alp)) {
         int exit = 0;

         for_each(aep, alp) {
            FILE *std_x;
            
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
            SGE_EXIT(1);
         }
      }
      continue;
   }

/*-----------------------------------------------------------------------------*/
      /* "-Mq fname" */
      if (!strcmp("-Mq", *spp)) {
         /* no adminhost/manager check needed here */
        
         spp = sge_parser_get_next(spp);

         ep = cull_read_in_qconf(NULL, *spp, 0, 0, NULL, NULL);
         if (!ep)
            if (sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE))
               continue;

         /* send it to qmaster */
         lp = lCreateList("Queue to modify", QU_Type);
         lAppendElem(lp, ep);

         alp = sge_gdi(SGE_QUEUE_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));

         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-msconf"  modify scheduler configuration */

      if (!strcmp("-msconf", *spp)) {
         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

         /* get the scheduler configuration .. */
         what = lWhat("%T(ALL)", SC_Type);
         alp = sge_gdi(SGE_SC_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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
         if (sge_get_recoverable(ep) == STATUS_OK)
            fprintf(stderr, MSG_SCHEDD_CHANGEDSCHEDULERCONFIGURATION);
         else
            fprintf(stderr, "%s", lGetString(ep, AN_text));
         newlp = lFreeList(newlp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-mstree"  modify sharetree */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-mstree", *spp)) {
         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

         /* get the sharetree .. */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);
 
         ep = lFirst(lp);
         if (!(ep=edit_sharetree(ep)))
            continue;

         lp = lFreeList(lp);

         newlp = lCreateList("sharetree modify", STN_Type);
         lAppendElem(newlp, ep);

         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_MOD, &newlp, NULL, what);
         what = lFreeWhat(what);
         ep = lFirst(alp);
         if (sge_get_recoverable(ep) == STATUS_OK)
            fprintf(stderr, MSG_TREE_CHANGEDSHARETREE);
         else
            fprintf(stderr, "%s", lGetString(ep, AN_text));
         newlp = lFreeList(newlp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-mu userset,..." */

      if (!strcmp("-mu", *spp)) {
         /* check for adminhost and manager privileges */
         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

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

      if (!strcmp("-Mu", *spp)) {
         char* file = NULL;
         const char* usersetname = NULL;
         lList *acl=NULL;

         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }     

          
         /* get userset from file */
         ep = NULL;
         ep = cull_read_in_userset(NULL, file ,0 ,0 ,0 ); 
         if (ep == NULL) {
            sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE); 
         }
         usersetname = lGetString(ep,US_name);
 
         /* get userset from qmaster */
         where = lWhere("%T( %I==%s )", US_Type, US_name,usersetname );
         what = lWhat("%T(ALL)", US_Type);
         alp = sge_gdi(SGE_USERSET_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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
         if (sge_get_recoverable(aep) != STATUS_OK) {
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

      if (!strcmp("-Au", *spp)) {
         lList *acl=NULL;
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
         ep = cull_read_in_userset(NULL, file ,0 ,0 ,0 ); 
         if (ep == NULL) {
            sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE); 
         }

         acl = lCreateList("usersetlist list to add", US_Type); 
         lAppendElem(acl,ep);

         alp = sge_gdi(SGE_USERSET_LIST, SGE_GDI_ADD, &acl, NULL, NULL);
         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-muser", *spp)) {
         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

         spp = sge_parser_get_next(spp);
        
         /* get user */
         where = lWhere("%T( %I==%s )", UP_Type, UP_name, *spp);
         what = lWhat("%T(ALL)", UP_Type);
         alp = sge_gdi(SGE_USER_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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

         /* look whether name has changed. If so we have to delete the
            user with the old name */
         if (strcmp(lGetString(ep, UP_name), lGetString(newep, UP_name))) {
            alp = sge_gdi(SGE_USER_LIST, SGE_GDI_DEL, &lp, NULL, NULL);
            aep = lFirst(alp);
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            alp = lFreeList(alp);
         }

         lp = lFreeList(lp);
         /* send it to qmaster */
         lp = lCreateList("User list to modify", UP_Type); 
         lAppendElem(lp, newep);

         alp = sge_gdi(SGE_USER_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-mprj projectname" */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-mprj", *spp)) {
         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

         spp = sge_parser_get_next(spp);
        
         /* get project */
         where = lWhere("%T( %I==%s )", UP_Type, UP_name, *spp);
         what = lWhat("%T(ALL)", UP_Type);
         alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);
                  
         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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
            sge_get_recoverable(aep);
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            alp = lFreeList(alp);
         }*/

         /* send it to qmaster */
         lp = lCreateList("Project list to modify", UP_Type); 
         lAppendElem(lp, newep);

         alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_MOD, &lp, NULL, NULL);

         aep = lFirst(alp);
         sge_get_recoverable(aep);
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         
         alp = lFreeList(alp);
         lp = lFreeList(lp);

         spp++;
         continue;
      }




/*----------------------------------------------------------------------------*/

      /* "-Muser file" */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-Muser", *spp)) {
         char* file = NULL;
         const char* username = NULL;
   
         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }

         /* get user from file */
         newep = NULL;
         newep = cull_read_in_userprj(NULL, file ,0 ,1 ,0 ); 
         if (newep == NULL) {
            sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE); 
         } 
         username = lGetString(newep, UP_name); 
                 
         /* get user */
         where = lWhere("%T( %I==%s )", UP_Type, UP_name, username);
         what = lWhat("%T(ALL)", UP_Type);
         alp = sge_gdi(SGE_USER_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);
                  
         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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
         if (sge_get_recoverable(aep) != STATUS_OK) {
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

/*----------------------------------------------------------------------------*/

      /* "-Mprj file" */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-Mprj", *spp)) {
         char* file = NULL;
         const char* projectname = NULL;
   
         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }

         /* get project from file */
         newep = NULL;
         newep = cull_read_in_userprj(NULL, file ,0 ,0 ,0 ); 
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
         if (sge_get_recoverable(aep) != STATUS_OK) {
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
         if (sge_get_recoverable(aep) != STATUS_OK) {
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
      /* "-sc complex_name_list" */

      if (!strcmp("-sc", *spp)) {
         spp = sge_parser_get_next(spp);

         /* *spp might look like name1,name2,... */

         cp = sge_strtok(*spp, ",");
         print_cmplx(cp);
         while ((cp=sge_strtok(NULL, ","))) {
            fprintf(stderr, "\n");
            print_cmplx(cp);
         }

         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-scl" */

      if (!strcmp("-scl", *spp)) {
         show_object_list(SGE_COMPLEX_LIST, CX_Type, CX_name, "complex"); 
         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-scal calendar_name" */
      if (!strcmp("-scal", *spp)) {
         spp = sge_parser_get_next(spp);

         /* get the existing pe entry .. */
         where = lWhere("%T( %I==%s )", CAL_Type, CAL_name, *spp);
         what = lWhat("%T(ALL)", CAL_Type);
         alp = sge_gdi(SGE_CALENDAR_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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
         write_cal(0, 0, ep);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-scall" */

      if (!strcmp("-scall", *spp)) {
         show_object_list(SGE_CALENDAR_LIST, CAL_Type, CAL_name, "calendar"); 
         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-sconf host_list" || "-mconf host_list" || "-aconf host_list" || "-Aconf file_list" */
      /* file list is also host list */
      if (!strcmp("-sconf", *spp) || 
          !strcmp("-aconf", *spp) || 
          !strcmp("-mconf", *spp) ||
          !strcmp("-Aconf", *spp)) {
         int action = 0;
         char *host_list = NULL;
         int ret, first = 1;
         lListElem *hep;
         const char *host;

         if (!strcmp("-aconf", *spp)) {
            sge_gdi_is_manager(me.user_name);
            sge_gdi_is_adminhost(me.qualified_hostname);
            action = 1;
         }
         else if (!strcmp("-mconf", *spp)) {
            sge_gdi_is_manager(me.user_name);
            if (sge_gdi_is_adminhost(me.qualified_hostname) != 0) {
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
            case 0:
               break;
            case -1:           
               fprintf(stderr, MSG_ANSWER_GETUNIQUEHNFAILEDRESX_S,
                  cl_errstr(-1));
               FREE(host_list);
               lFreeElem(hep);
               SGE_EXIT(1);
               break;
            default:
               fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_SS, 
                     lGetHost(hep, EH_name), cl_errstr(ret));
               FREE(host_list);
               lFreeElem(hep);
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
         lFreeElem(hep);

         spp++;
         continue;
      }
      
/*-----------------------------------------------------------------------------*/
      /* "-sckpt ckpt_name" */
      if (!strcmp("-sckpt", *spp)) {
         spp = sge_parser_get_next(spp);

         /* get the existing ckpt entry .. */
         where = lWhere("%T( %I==%s )", CK_Type, CK_name, *spp);
         what = lWhat("%T(ALL)", CK_Type);
         alp = sge_gdi(SGE_CKPT_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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
         write_ckpt(0, 0, ep);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-sckptl" */
      if (!strcmp("-sckptl", *spp)) {
         show_object_list(SGE_CKPT_LIST, CK_Type, CK_name,
               "ckpt interface definition");
         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-sconfl" */

      if (!strcmp("-sconfl", *spp)) {
         show_object_list(SGE_CONFIG_LIST, CONF_Type, CONF_hname, "config"); 
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-dconf config_list" */
      if (!strcmp("-dconf", *spp)) {
         char *host_list = NULL;
         lListElem *hep;
         const char *host;
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
               case 0:
                  break;
               case -1:
                  fprintf(stderr, MSG_ANSWER_GETUNIQUEHNFAILEDRESX_S,
                          cl_errstr(-1));
                  FREE(host_list);
                  lFreeElem(hep);
                  SGE_EXIT(1);
                  break;
               default:
                  fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, cp);
                  FREE(host_list);
                  lFreeElem(hep);
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
            lFreeElem(hep);
         }
         else
            fprintf(stderr, MSG_ANSWER_NEEDHOSTNAMETODELLOCALCONFIG);

         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-se exec_server" */
      if (!strcmp("-se", *spp)) {
         spp = sge_parser_get_next(spp);

         /* resolve host */
         hep = lCreateElem(EH_Type);
         lSetHost(hep, EH_name, *spp);
         
         switch (sge_resolve_host(hep, EH_name)) {
         case 0:
            break;
         case -1:
            fprintf(stderr, MSG_ANSWER_GETUNIQUEHNFAILEDRESX_S,
               cl_errstr(-1));
            lFreeElem(hep);
            SGE_EXIT(1);
            break;
         default:
            fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetHost(hep, EH_name));
            lFreeElem(hep);
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
         if (sge_get_recoverable(aep) != STATUS_OK) {
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
         write_host(0, 0, ep, EH_name, NULL);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-secl" */
      if (!strcmp("-secl", *spp)) {
         show_eventclients();
         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-sel" */
      if (!strcmp("-sel", *spp)) {
         show_object_list(SGE_EXECHOST_LIST, EH_Type, EH_name, 
               "execution host"); 
         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-sep" */
      if (!strcmp("-sep", *spp)) {
         show_processors();
         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-sh" */
      if (!strcmp("-sh", *spp)) {
         show_object_list(SGE_ADMINHOST_LIST, AH_Type, AH_name, 
               "administrative host"); 
         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-sm" */

      if (!strcmp("-sm", *spp)) {
         show_object_list(SGE_MANAGER_LIST, MO_Type, MO_name, "manager"); 
         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-sp pe_name" */
      if (!strcmp("-sp", *spp)) {
         spp = sge_parser_get_next(spp);

         /* get the existing pe entry .. */
         where = lWhere("%T( %I==%s )", PE_Type, PE_name, *spp);
         what = lWhat("%T(ALL)", PE_Type);
         alp = sge_gdi(SGE_PE_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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
         write_pe(0, 0, ep);

         spp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-spl" */
      if (!strcmp("-spl", *spp)) {
         show_object_list(SGE_PE_LIST, PE_Type, PE_name,
               "parallel environment");
         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-so" */

      if (!strcmp("-so", *spp)) {
         show_object_list(SGE_OPERATOR_LIST, MO_Type, MO_name, "operator"); 
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-sq [destin_id[,destin_id,...]]" */

      if (!strcmp("-sq", *spp)) {
         if(*(spp+1) && strncmp("-", *(spp+1), 1)) {
            /* queues specified */
            while(*(spp+1) && strncmp("-", *(spp+1), 1)) {
               spp++;
               if (strncmp("-", *spp, 1)) {
                  lString2List(*spp, &arglp, QR_Type, QR_name, ", ");

                  for_each(argep, arglp) {
                     where = lWhere("%T( %I==%s )", QU_Type, QU_qname, lGetString(argep, QR_name));
                     what = lWhat("%T(ALL)", QU_Type);
                     alp = sge_gdi(SGE_QUEUE_LIST, SGE_GDI_GET, &lp, where, what);
                     where = lFreeWhere(where);
                     what = lFreeWhat(what);

                     if(lp)
                        cull_write_qconf(0, 1, NULL, NULL, NULL, lFirst(lp));
                     else {
                        fprintf(stderr, MSG_OBJ_UNABLE2FINDQ_S, lGetString(argep, QR_name));
                        sge_parse_return = 1;
                     }
                     aep = lFirst(alp);
                     if(sge_get_recoverable(aep) != STATUS_OK) {
                        fprintf(stderr, "%s", lGetString(aep, AN_text));
                     }
                     lFreeList(alp);

                     printf("\n");
                  }
                  arglp = lFreeList(arglp);
               }
            }
         } else {
            /* no queue specified, show template queue */
            where = lWhere("%T( %I==%s )", QU_Type, QU_qname, SGE_TEMPLATE_NAME);
            what = lWhat("%T(ALL)", QU_Type);
            alp = sge_gdi(SGE_QUEUE_LIST, SGE_GDI_GET, &lp, where, what);
            where = lFreeWhere(where);
            what = lFreeWhat(what);

            if(lp)
               cull_write_qconf(0, 1, NULL, NULL, NULL, lFirst(lp));
            else {
               aep = lFirst(alp);
               sge_get_recoverable(aep);
               fprintf(stderr, "%s", lGetString(aep, AN_text));
            }
            printf("\n");
            lFreeList(alp);
         }
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-ssconf" */

      if (!strcmp("-ssconf", *spp)) {

         /* get the scheduler configuration .. */
         what = lWhat("%T(ALL)", SC_Type);
         alp = sge_gdi(SGE_SC_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);
 
         if (write_sched_configuration(0, 0, lFirst(lp)) == NULL) {
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

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-sstnode", *spp)) {
         int found = 0;

         spp = sge_parser_get_next(spp);

         /* get the sharetree .. */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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

            lListElem *node;
            const char *nodepath;
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

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-rsstnode", *spp)) {
         int found = 0;

         spp = sge_parser_get_next(spp);

         /* get the sharetree .. */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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
            const char *nodepath;
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

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-sstree", *spp)) {
         /* get the sharetree .. */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
            spp++;
            continue;
         }
         alp = lFreeList(alp);
 
         ep = lFirst(lp);

         write_sharetree(NULL, ep, NULL, stdout, 0, 1, 1);

         lp = lFreeList(lp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-bonsai" */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-bonsai", *spp)) {
         /* get the sharetree .. */
         what = lWhat("%T(ALL)", STN_Type);
         alp = sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, &lp, NULL, what);
         what = lFreeWhat(what);

         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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

      /* "-sql" */
      if (!strcmp("-sql", *spp)) {
         show_object_list(SGE_QUEUE_LIST, QU_Type, QU_qname, "queue"); 
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-ss" */
      if (!strcmp("-ss", *spp)) {

         show_object_list(SGE_SUBMITHOST_LIST, SH_Type, SH_name, 
               "submit host"); 
         spp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-sss" - show scheduler state */

      if (!strcmp("-sss", *spp)) {
         /* ... */
         show_object_list(SGE_EVENT_LIST, EV_Type, EV_host, "scheduling host"); 

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-su [list_name[,list_name,...]]" */

      if (!strcmp("-su", *spp)) {
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

      /* "-shgrpl" */
      if (!strcmp("-shgrpl", *spp)) {
         show_object_list(SGE_HOST_GROUP_LIST, GRP_Type, GRP_group_name, "host group list");  
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-sumapl" */
      if (!strcmp("-sumapl", *spp)) {
         show_object_list(SGE_USER_MAPPING_LIST, UME_Type, UME_cluster_user, "user mapping entries");  
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/



      /* "-Mumap user filename" */
      if (!strcmp("-Mumap", *spp)) {
         char* file = NULL;
        
         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }
         mod_user_map_entry_from_file(  file );
         
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/



      /* "-Mhgrp user filename" */
      if (!strcmp("-Mhgrp", *spp)) {
         char* file = NULL;
         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }
         mod_host_group_entry_from_file( file );
         
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-sumap user"  */
      if (!strcmp("-sumap", *spp)) {
         spp = sge_parser_get_next(spp);
         show_user_map_entry(*spp);
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-shgrp group"  */
      if (!strcmp("-shgrp", *spp)) {
         spp = sge_parser_get_next(spp);
         show_host_group_entry(*spp);
         spp++;
         continue;
      }



/*----------------------------------------------------------------------------*/

      /* "-mumap user"  */
      if (!strcmp("-mumap", *spp)) {
         char* user = NULL;

         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

         spp = sge_parser_get_next(spp);
         user = *spp;
         sge_gdi_is_manager(me.user_name);
         mod_user_map_entry( user );
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-mhgrp user"  */
      if (!strcmp("-mhgrp", *spp)) {
         char* group = NULL;

         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

         spp = sge_parser_get_next(spp);
         group = *spp;
         sge_gdi_is_manager(me.user_name);
         mod_host_group_entry( group );
         spp++;
         continue;
      }

         
/*----------------------------------------------------------------------------*/

      /* "-dumap user "  */
      if (!strcmp("-dumap", *spp)) {
         char* user = NULL;

         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);
         user = *spp; 
         
         sge_gdi_is_manager(me.user_name);
         del_user_map_entry( user );
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-dhgrp user "  */
      if (!strcmp("-dhgrp", *spp)) {
         char* group = NULL;
   
         /* no adminhost/manager check needed here */

         spp = sge_parser_get_next(spp);
         group = *spp; 
         
         sge_gdi_is_manager(me.user_name);
         del_host_group_entry( group );
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-Aumap user mapfile"  */
      if (!strcmp("-Aumap", *spp)) {
         char* file = NULL;

         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }
    
         add_user_map_entry_from_file( file );
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-aumap user"  */
      if (!strcmp("-aumap", *spp)) {
         char* user = NULL;

         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

         spp = sge_parser_get_next(spp);
         user = *spp;
         sge_gdi_is_manager(me.user_name);
         add_user_map_entry( user, NULL, NULL );
         spp++;
         continue;
      }


/*----------------------------------------------------------------------------*/

      /* "-ahgrp group"  */
      if (!strcmp("-ahgrp", *spp)) {
         char* group = NULL;

         sge_gdi_is_adminhost(me.qualified_hostname);
         sge_gdi_is_manager(me.user_name);

         spp = sge_parser_get_next(spp);
         group = *spp;
         sge_gdi_is_manager(me.user_name);
         add_host_group_entry( group );
         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/

      /* "-Ahgrp file"  */
      if (!strcmp("-Ahgrp", *spp)) {
         char* file = NULL;

         /* no adminhost/manager check needed here */

         if (!sge_next_is_an_opt(spp)) {
            spp = sge_parser_get_next(spp);
            file = *spp;
         } else {
            sge_error_and_exit(MSG_FILE_NOFILEARGUMENTGIVEN); 
         }
    
         add_host_group_entry_from_file(  file );
         spp++;
         continue;
      }

#endif

/*----------------------------------------------------------------------------*/

      /* "-suser username" */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-suser", *spp)) {
         const char*  user = NULL;
         lList* uList = NULL;
         lListElem* uep = NULL;

         spp = sge_parser_get_next(spp);

         lString2List(*spp, &uList, ST_Type, STR , ", ");
         for_each(uep,uList)
         {
            user = lGetString(uep,STR);
            /* get user */
            where = lWhere("%T( %I==%s )", UP_Type, UP_name, user);
            what = lWhat("%T(ALL)", UP_Type);
            alp = sge_gdi(SGE_USER_LIST, SGE_GDI_GET, &lp, where, what);
            where = lFreeWhere(where);
            what = lFreeWhat(what);
                        
            printf("\n");            
            aep = lFirst(alp);
            if (sge_get_recoverable(aep) != STATUS_OK) {
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
            write_userprj(&alp, ep, NULL, stdout, 0, 1);
            alp = lFreeList(alp);
         }
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-sprj projectname" */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-sprj", *spp)) {
         spp = sge_parser_get_next(spp);

         /* get project */
         where = lWhere("%T( %I==%s )", UP_Type, UP_name, *spp);
         what = lWhat("%T(ALL)", UP_Type);
         alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_GET, &lp, where, what);
         where = lFreeWhere(where);
         what = lFreeWhat(what);
                  
         aep = lFirst(alp);
         if (sge_get_recoverable(aep) != STATUS_OK) {
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
         write_userprj(&alp, ep, NULL, stdout, 0, 0);
         alp = lFreeList(alp);

         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-ss" */
      if (!strcmp("-ss", *spp)) {
         show_object_list(SGE_SUBMITHOST_LIST, SH_Type, SH_name, 
               "submit"); 
         spp++;
         continue;
      }
/*----------------------------------------------------------------------------*/
      /* "-sul" */

      if (!strcmp("-sul", *spp)) {
         show_object_list(SGE_USERSET_LIST, US_Type, US_name, 
               "userset list"); 
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

       /* "-suserl" */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-suserl", *spp)) {
         show_object_list(SGE_USER_LIST, UP_Type, UP_name, 
               "user list"); 
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

       /* "-sprjl" */

      if (feature_is_enabled(FEATURE_SGEEE) && !strcmp("-sprjl", *spp)) {
         show_object_list(SGE_PROJECT_LIST, UP_Type, UP_name, 
               "project list"); 
         spp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-tsm" trigger scheduler monitoring */

      if (!strcmp("-tsm", *spp)) {
         /* no adminhost/manager check needed here */

         alp = gdi_tsm(NULL, NULL);
         for_each(aep, alp) {
            fprintf(stderr, "%s", lGetString(aep, AN_text));
         }
         alp = lFreeList(alp);
         
         spp++;
         continue;
      } 

/*----------------------------------------------------------------------------*/
 
      /* "-switch featuresetname" */
 
      if (!strcmp("-switch", *spp)) {
         /* no adminhost/manager check needed here */
 
         spp = sge_parser_get_next(spp);

         feature_activate(feature_get_featureset_id(*spp));

         alp = sge_gdi(SGE_FEATURESET_LIST, SGE_GDI_MOD, &Master_FeatureSet_List, NULL, NULL);
 
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
   char *cp2;
   lListElem *ep;
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

/***********************************************************************/

static int sge_edit(
char *fname 

) {

   SGE_STRUCT_STAT before, after;
   pid_t pid;
   int status;
   int ws = 0;

   DENTER(TOP_LAYER, "sge_edit");;

   if (SGE_STAT(fname, &before)) {
      ERROR((SGE_EVENT, MSG_FILE_EDITFILEXDOESNOTEXIST_S, fname));
      DEXIT;
      return (-1);
   }

   chown(fname, me.uid, me.gid);

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
   }
   else {
      const char *cp = NULL;

      sge_set_def_sig_mask(0, NULL);   
      sge_unblock_all_signals();
      setuid(getuid());
      setgid(getgid());

      cp = sge_getenv("EDITOR");
      if (!cp || strlen(cp) == 0)
         cp = DEFAULT_EDITOR;
         
      execlp(cp, cp, fname, (char *) 0);
      ERROR((SGE_EVENT, MSG_QCONF_CANTSTARTEDITORX_S, cp));
      SGE_EXIT(1);
   }

   DEXIT;
   return (-1);
}

/****************************************************************************/
static int sge_next_is_an_opt(
char **pptr 

) {
   DENTER(TOP_LAYER, "sge_next_is_an_opt");

   if (!*(pptr+1)) {
      DEXIT;
/*       return -1; */
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
   return 1; /* to prevent warning */
}

static int add_host_of_type(
lList *arglp,
u_long32 target 
) {
   lListElem *argep, *ep;
   lList *lp, *alp;
   const char *host;
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
      if (sge_resolve_host(argep, nm)) {
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
      if (sge_get_recoverable(ep) == STATUS_OK)
         if (target == SGE_SUBMITHOST_LIST)
            fprintf(stdout, MSG_QCONF_ADDEDTO_SUBMITHOST_LIST_S, host);
         else
            fprintf(stdout, MSG_QCONF_ADDEDTO_ADMINHOST_LIST_S, host);    
      else 
         fprintf(stderr, "%s", lGetString(ep, AN_text));

      lFreeList(lp);
      lFreeList(alp);
   }

   DEXIT;
   return ret;
}

/* ------------------------------------------------------------ */

static int del_host_of_type(
lList *arglp,
u_long32 target 
) {
   lListElem *argep, *ep;
   lList *lp, *alp;
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

      /* resolve hostname */
      if (sge_resolve_host(argep, nm)) {
         fprintf(stderr, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetHost(argep, nm));
         continue;
      }

      /* make a new host element */
      lp = lCreateList("host to add", type);
      ep = lCopyElem(argep);
      lAppendElem(lp, ep);

      /* delete element */
      alp = sge_gdi(target, SGE_GDI_DEL, &lp, NULL, NULL);

      /* print results */
      ep = lFirst(alp);
      sge_get_recoverable(ep);
		fprintf(stderr, "%s\n", lGetString(ep, AN_text));

      lFreeList(alp);
      lFreeList(lp);
   }

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ */

static lListElem* edit_exechost(
lListElem *ep 
) {
   int status;
   lListElem *hep;

   /* used for generating filenames */
   char *filename;

   DENTER(TOP_LAYER, "edit_exechost");

   filename = write_host(0, 1, ep, EH_name, NULL);
   status = sge_edit(filename);

   if (status < 0) {
      unlink(filename);
      free(filename);
      if (sge_error_and_exit(MSG_PARSE_EDITFAILED))
         return NULL;
   }

   if (status > 0) {
      unlink(filename);
      free(filename);
      if (sge_error_and_exit(MSG_FILE_FILEUNCHANGED))
         return NULL;
   }
   
   hep = cull_read_in_host(NULL, filename, CULL_READ_MINIMUM, EH_name, 
      NULL, NULL);
   unlink(filename);
   free(filename);

   DEXIT;
   return hep;
}

/* ------------------------------------------------------------ */

static lList* edit_sched_conf(
lList *confl 
) {
   int status;
   lListElem *aep;
   char *fname;
   lList *alp=NULL, *newconfl;


   DENTER(TOP_LAYER, "edit_sched_conf");

   if ((fname = write_sched_configuration(0, 1, lFirst(confl))) == NULL) {
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
   
   if (!(newconfl = read_sched_configuration(fname, 0, &alp))) {
      aep = lFirst(alp);
      fprintf(stderr, MSG_QCONF_CANTREADCONFIG_S, lGetString(aep, AN_text));
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
   lListElem *aep, *newep;
   char fname[SGE_PATH_MAX];
   lList *alp=NULL;


   DENTER(TOP_LAYER, "edit_userprj");

   if (!sge_tmpnam(fname)) {
      fprintf(stderr, MSG_FILE_CANTCREATETEMPFILE);
      SGE_EXIT(1);
   }

   if (write_userprj(&alp, ep, fname, NULL, 0, user)) {
      aep = lFirst(alp);
      fprintf(stderr, "%s\n", lGetString(aep, AN_text));
      SGE_EXIT(1);
   }
   lFreeList(alp);

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

   newep = cull_read_in_userprj(NULL, fname, 0, user, NULL);
   
   if (!newep) {
      fprintf(stderr, MSG_QCONF_CANTREADX_S, user?MSG_OBJ_USER:MSG_JOB_PROJECT);
      unlink(fname);
      SGE_EXIT(1);
   }

   unlink(fname);

   DEXIT;
   return newep;
}

/****************************************************************/
static lListElem *edit_sharetree(
lListElem *ep 
) {
   int status;
   lListElem *aep, *newep;
   char fname[SGE_PATH_MAX], errstr[1024];
   lList *alp=NULL;


   DENTER(TOP_LAYER, "edit_sharetree");

   if (!sge_tmpnam(fname)) {
      fprintf(stderr, MSG_FILE_CANTCREATETEMPFILE);
      SGE_EXIT(1);
   }

   if (!ep) {
      ep = getSNTemplate();
   }

   if (write_sharetree(&alp, ep, fname, NULL, 0, 1, 1)) {
      aep = lFirst(alp);
      fprintf(stderr, "%s\n", lGetString(aep, AN_text));
      SGE_EXIT(1);
   }
   lFreeList(alp);

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
   
   if (!(newep = read_sharetree(fname, NULL, 0, errstr, 1, NULL))) {
      fprintf(stderr, MSG_QCONF_CANTREADSHARETREEX_S, errstr);
      unlink(fname);
      SGE_EXIT(1);
   }
   
   unlink(fname);

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
   lEnumeration *what;
   lCondition *where;
   lList *alp = NULL, *lp = NULL;
   lListElem *ep;
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

   case QU_qname:
      where = lWhere("%T(%I!=%s)",
         type, keynm, SGE_TEMPLATE_NAME);
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

   ep = lFirst(alp);
   if (sge_get_recoverable(ep) != STATUS_OK) {
      fprintf(stderr, "%s\n", lGetString(ep, AN_text));
      return 0;
   }



   if (lp) {
      for_each (ep, lp) {
         const char *line;
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
   
   lFreeList(alp);
   lFreeList(lp);
   
   DEXIT;
   return 0;
}

static int show_eventclients()
{
   lEnumeration *what;
   lList *alp = NULL, *lp = NULL;
   lListElem *ep;

   DENTER(TOP_LAYER, "show_eventclients");

   what = lWhat("%T(%I %I %I)", EV_Type, EV_id, EV_name, EV_host);

   alp = sge_gdi(SGE_EVENT_LIST, SGE_GDI_GET, &lp, NULL, what);
   what = lFreeWhat(what);

   ep = lFirst(alp);
   if (sge_get_recoverable(ep) != STATUS_OK) {
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
   
   lFreeList(alp);
   lFreeList(lp);
   
   DEXIT;
   return 0;
}



static int show_processors()
{
   lEnumeration *what;
   lCondition *where;
   lList *alp = NULL, *lp = NULL;
   lListElem *ep;
   const char *cp;
   u_long32 sum = 0;

   DENTER(TOP_LAYER, "show_processors");

   what = lWhat("%T(%I %I %I)", EH_Type, EH_name, EH_processors, EH_load_list);
   where = lWhere("%T(!(%Ic=%s || %Ic=%s))", EH_Type, EH_name, 
                  SGE_TEMPLATE_NAME, EH_name, SGE_GLOBAL_NAME);

   alp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_GET, &lp, where, what);
   what = lFreeWhat(what);
   where = lFreeWhere(where);

   ep = lFirst(alp);
   if (sge_get_recoverable(ep) != STATUS_OK) {
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
         lListElem *arch_elem;

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
   
   lFreeList(alp);
   lFreeList(lp);
   
   DEXIT;
   return 0;
}






/***p** src/add_user_map_entry_from_file() **********************************
*
*  NAME
*     add_user_map_entry_from_file() -- Option -Aumap 
*
*  SYNOPSIS
*
*     #include "parse_qconf.h"
*     #include <src/parse_qconf.h>
* 
*     static int add_user_map_entry_from_file(char* filename); 
*       
*
*  FUNCTION
*     This function is used in the -Aumap Option. It will open the file with mapping 
*     entries and send it to the qmaster. The entries in the file are used to define
*     the mapping for a cluster user. 
*
*
*  INPUTS
*     char* filename - file with user mapping definition
*
*  RESULT
*     int 0 on success, -1 on error
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     /()
*     
****************************************************************************
*/
#ifndef __SGE_NO_USERMAPPING__
static int add_user_map_entry_from_file(
char *filename 
) {
   lListElem* genericElem = NULL;
   lList* addList         = NULL;
   lList* alp             = NULL; 
   DENTER(TOP_LAYER, "add_user_map_entry_from_file");


   /* read in file */
   if (filename == NULL) {
      fprintf(stderr, MSG_FILE_ERRORREADINGINFILE);
      DEXIT;
      return(-1);
   }

   genericElem = cull_read_in_ume(NULL, filename, 1, 0, 0 );
   if (genericElem == NULL) {
      fprintf(stderr, MSG_FILE_ERRORREADINGINFILE);
      DEXIT;
      return(-1);
   }

   if (lGetString(genericElem, UME_cluster_user) == NULL) {
      fprintf(stderr, MSG_UM_NOCLUSTERUSERENTRYINFILEX_S,filename );
      genericElem = lFreeElem(genericElem);
      DEXIT;
      return(-1);
   }

   /* get list from master */   
   addList = get_user_mapping_list_from_master(lGetString(genericElem, UME_cluster_user));
   if (addList != NULL) {
     addList = lFreeList(addList);
     fprintf(stderr, MSG_ANSWER_USERMAPENTRYALREADYEXISTS_S,lGetString(genericElem, UME_cluster_user));
     genericElem = lFreeElem(genericElem);
     DEXIT;
     return(-1); 
   } 

   /*  create UME_list , add Element in UME_list */ 
   addList = lCreateList("user mapping entry",UME_Type);
   lAppendElem(addList, genericElem);

   
   /*  send new list to qmaster */
   alp = sge_gdi(SGE_USER_MAPPING_LIST, SGE_GDI_ADD, &addList, NULL, NULL);
   show_gdi_request_answer(alp);

   alp = lFreeList(alp);
   addList = lFreeList(addList);

   DEXIT;
   return 0; 
}
#endif

/***p** src/show_gdi_request_answer() **********************************
*
*  NAME
*     show_gdi_request_answer() -- print answer list to stderr
*
*  SYNOPSIS
*
*     void show_gdi_request_answer(alp);  
*
*  FUNCTION
*     
*
*
*  INPUTS
*     lList* alp - pointer to answer list for SGE_USER_MAP_LIST request
*
*  RESULT
*     nothing
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     /()
*     
****************************************************************************
*/
#ifndef __SGE_NO_USERMAPPING__
void show_gdi_request_answer(
lList *alp 
) {
   lListElem *aep         = NULL;
   DENTER(TOP_LAYER, "show_gdi_request_answer");
   if (alp != NULL) {
    
    
      for_each(aep,alp) {
        sge_get_recoverable(aep); 
        /* fprintf(stderr, "%s", lGetString(aep, AN_text)); */
      }
      aep = lLast(alp);
      fprintf(stderr, "%s", lGetString(aep, AN_text));
   }
   DEXIT;
}

#endif




/***p** src/add_user_map_entry() **********************************
*
*  NAME
*     add_user_map_entry() -- Option -aumap
*
*  SYNOPSIS
*
*     #include "parse_qconf.h"
*     #include <src/parse_qconf.h>
* 
*     static int add_user_map_entry(char* user, 
*                                         char* mapname, 
*                                         char* hostname);
*       
*
*  FUNCTION
*     This function is used in the -aumap Option. A default mapping
*     file is generated. Then the $EDITOR command is started and
*     the user can modify the file. At least the file will be read
*     in again and send to the qmaster. The parameters mapname and 
*     hostname can be NULL.
*     If they are not NULL, the parameters are writen to the default
*     mapping file. 
*
*
*  INPUTS
*     char* user     - name of the cluster user to define the mapping 
*     char* mapname  - a mapping name for the cluster user
*     char* hostname - the hostname with user account for mapname
*
*  RESULT
*     int 0 on success, -1 on error
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     /()
*     
****************************************************************************
*/
#ifndef __SGE_NO_USERMAPPING__
static int add_user_map_entry(
char *user,
char *mapname,
char *hostname 
) {
   lListElem* genericElem = NULL;
   lList* addList         = NULL;
   lList* alp             = NULL; 
   char* filename         = NULL;
   int   status           = 0;
   DENTER(TOP_LAYER, "add_user_map_entry");

   /* get list from master */   
   addList = get_user_mapping_list_from_master(user);
   if (addList != NULL) {
     addList = lFreeList(addList);
     fprintf(stderr, MSG_ANSWER_USERMAPENTRYALREADYEXISTS_S,user );
     DEXIT;
     return(-1); 
   } 

   /* create standard file and write it to tmp */
   genericElem = sge_create_ume(user,mapname,hostname);
   filename = write_ume(2,1,genericElem);
   lFreeElem(genericElem);
    
   /* edit this file */
   status = sge_edit(filename);
   if (status < 0) {
       unlink(filename);
       fprintf(stderr, MSG_PARSE_EDITFAILED);
       DEXIT;
       return(-1);
   }

   if (status > 0) {
       unlink(filename);
       fprintf(stderr, MSG_FILE_FILEUNCHANGED);
       DEXIT;
       return(-1);
   }

   /* read it in again */
   genericElem = cull_read_in_ume(NULL, filename, 1, 0, 0 );
   unlink(filename);
   if (genericElem == NULL) {
      fprintf(stderr, MSG_FILE_ERRORREADINGINFILE);
      DEXIT;
      return(-1);
   }

   if (strcmp(lGetString(genericElem, UME_cluster_user), user) != 0) {
     
     fprintf(stderr, MSG_ANSWER_CLUSTERUNAMEXDIFFFROMY_SS,lGetString(genericElem, UME_cluster_user),user );
     genericElem = lFreeElem(genericElem);
     DEXIT;
     return(-1); 
   } 
   
   addList = lCreateList("user mapping entry",UME_Type);
   lAppendElem(addList, genericElem);

   alp = sge_gdi(SGE_USER_MAPPING_LIST, SGE_GDI_ADD, &addList, NULL, NULL);
   show_gdi_request_answer(alp);

   alp = lFreeList(alp);
   addList = lFreeList(addList);

   DEXIT;
   return 0; 
}


/***p** src/add_host_group_entry() **********************************
*
*  NAME
*     add_host_group_entry() -- Option qconf -ahgrp
*
*  SYNOPSIS
*
*     #include "parse_qconf.h"
*     #include <src/parse_qconf.h>
* 
*     static int add_host_group_entry( char* group );
*       
*
*  FUNCTION
*     This function is used in the qconf -ahgrp Option. A default host
*     group file is generated. Then the $EDITOR command is started and
*     the user can modify the file. At least the file will be read
*     in again and send to the qmaster.  
*
*  INPUTS
*     char* group - name of the host group to define
*
*  RESULT
*     int 0 on success, -1 on error
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     /()
*     
****************************************************************************
*/
static int add_host_group_entry(
char *group 
) {
   lListElem* genericElem = NULL;
   lList* addList         = NULL;
   lList* alp             = NULL; 
   char* filename         = NULL;
   int   status           = 0;
   DENTER(TOP_LAYER, "add_host_group_entry");

   /* get list from master */   
   addList = get_host_group_list_from_master(group);
   if (addList != NULL) {
     addList = lFreeList(addList);
     fprintf(stderr, MSG_ANSWER_HOSTGROUPENTRYALLREADYEXISTS_S,group );
     DEXIT;
     return(-1); 
   } 

   /* create standard file and write it to tmp */
   genericElem = lCreateElem(GRP_Type);
   lSetString(genericElem, GRP_group_name, group);
   filename = write_host_group(2,1,genericElem);
   lFreeElem(genericElem);
    
   /* edit this file */
   status = sge_edit(filename);
   if (status < 0) {
       unlink(filename);
       fprintf(stderr, MSG_PARSE_EDITFAILED);
       DEXIT;
       return(-1);
   }

   if (status > 0) {
       unlink(filename);
       fprintf(stderr, MSG_FILE_FILEUNCHANGED);
       DEXIT;
       return(-1);
   }

   /* read it in again */
   genericElem = cull_read_in_host_group(NULL, filename, 1, 0, 0 );
   unlink(filename);
   if (genericElem == NULL) {
      fprintf(stderr, MSG_FILE_ERRORREADINGINFILE);
      DEXIT;
      return(-1);
   }

   if (strcmp(lGetString(genericElem, GRP_group_name), group) != 0) {
     
     fprintf(stderr, MSG_ANSWER_HOSTGROUPNAMEXDIFFFROMY_SS,lGetString(genericElem, GRP_group_name),group );
     genericElem = lFreeElem(genericElem);
     DEXIT;
     return(-1); 
   } 
   
   addList = lCreateList("host group list",GRP_Type);
   lAppendElem(addList, genericElem);

   alp = sge_gdi(SGE_HOST_GROUP_LIST, SGE_GDI_ADD, &addList, NULL, NULL);
   show_gdi_request_answer(alp);

   alp = lFreeList(alp);
   addList = lFreeList(addList);

   DEXIT;
   return 0; 
}


/***p** src/mod_user_map_entry() **********************************
*
*  NAME
*     mod_user_map_entry() -- Option -mumap 
*
*  SYNOPSIS
*
*     #include "parse_qconf.h"
*     #include <src/parse_qconf.h>
* 
*     static int mod_user_map_entry (char* user);  
*
*  FUNCTION
*     This function is used in the -mumap Option. It asks the qmaster
*     for the mapping entries for the cluster user, given in the 
*     parameter user. Then it write a temporary file with the entries
*     for the user. After that the $EDITOR is called and the user
*     can modify the file. At least the modified entries are sent to
*     the qmaster.
*
*
*  INPUTS
*     char* user     - name of the cluster user to define the mapping 
*    
*
*  RESULT
*     int 0 on success, -1 on error
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     /()
*     
****************************************************************************
*/
static int mod_user_map_entry(
char *user 
) {
  lListElem* genericElem = NULL;
   lList* addList         = NULL;
   lList* alp             = NULL; 
   char* filename         = NULL;
   int   status           = 0;
   DENTER(TOP_LAYER, "mod_user_map_entry");


   /* get list from master */   
   addList = get_user_mapping_list_from_master(user);
   if (addList == NULL) {
     fprintf(stderr, MSG_ANSWER_USERMAPENTRYXNOTFOUND_S,user );
     DEXIT;
     return(-1); 
   } 

   /* write list to file */
   genericElem = lFirst(addList);
   filename = write_ume(2,1,genericElem);
   addList = lFreeList(addList);   
 
   /* edit this file */
   status = sge_edit(filename);
   if (status < 0) {
       unlink(filename);
       fprintf(stderr, MSG_PARSE_EDITFAILED );
       DEXIT; 
       return(-1); 
   }

   if (status > 0) {
       unlink(filename);
       fprintf(stderr, MSG_FILE_FILEUNCHANGED );
       DEXIT;
       return(-1); 
   }

   /* read it in again */
   genericElem = cull_read_in_ume(NULL, filename, 1, 0, 0 );
   unlink(filename);
   
   if (genericElem == NULL) {
      fprintf(stderr, MSG_FILE_ERRORREADINGINFILE );
      DEXIT;
      return (-1); 
   }
   if (strcmp(lGetString(genericElem, UME_cluster_user), user) != 0) {
     
     fprintf(stderr, MSG_ANSWER_CLUSTERUNAMEXDIFFFROMY_SS,lGetString(genericElem, UME_cluster_user),user );
     genericElem = lFreeElem(genericElem);
     DEXIT;
     return(-1); 
   }  
   addList = lCreateList("user mapping entry",UME_Type);
   lAppendElem(addList, genericElem);

   alp = sge_gdi(SGE_USER_MAPPING_LIST, SGE_GDI_MOD, &addList, NULL, NULL);
   show_gdi_request_answer(alp);

   alp = lFreeList(alp);
   addList = lFreeList(addList);
   DEXIT;
   return 0;  
}

/***p** src/mod_host_group_entry() **********************************
*
*  NAME
*     mod_host_group_entry() -- Option qconf -mhgrp 
*
*  SYNOPSIS
*
*     #include "parse_qconf.h"
*     #include <src/parse_qconf.h>
* 
*     static int mod_host_group_entry ( char* group );  
*
*  FUNCTION
*     This function is used in the qconf -mhgrp Option. It asks the qmaster
*     for the existing host group entries in the host group, given in the 
*     parameter "group". Then it write a temporary file with the entries
*     for the group. After that, the $EDITOR is called and the user
*     can modify the file. At least the modified entries are sent to
*     the qmaster.
*
*
*  INPUTS
*     char* group  - name of the host group to modify 
*    
*
*  RESULT
*     int 0 on success, -1 on error
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     /()
*     
****************************************************************************
*/
int mod_host_group_entry(
char *group 
) { 
   lListElem* genericElem = NULL;
   lList* addList         = NULL;
   lList* alp             = NULL; 
   char* filename         = NULL;
   int   status           = 0;
   DENTER(TOP_LAYER, "mod_host_group_entry");


   /* get list from master */   
   addList = get_host_group_list_from_master(group);
   if (addList == NULL) {
     fprintf(stderr, MSG_ANSWER_HOSTGROUPENTRYXNOTFOUND_S,group );
     DEXIT;
     return(-1); 
   } 

   /* write list to file */
   genericElem = lFirst(addList);
   filename = write_host_group(2,1,genericElem);
   addList = lFreeList(addList);   
 
   /* edit this file */
   status = sge_edit(filename);
   if (status < 0) {
       unlink(filename);
       fprintf(stderr, MSG_PARSE_EDITFAILED );
       DEXIT; 
       return(-1); 
   }

   if (status > 0) {
       unlink(filename);
       fprintf(stderr, MSG_FILE_FILEUNCHANGED );
       DEXIT;
       return(-1); 
   }

   /* read it in again */
   genericElem = cull_read_in_host_group(NULL, filename, 1, 0, 0 );
   unlink(filename);
   
   if (genericElem == NULL) {
      fprintf(stderr, MSG_FILE_ERRORREADINGINFILE );
      DEXIT;
      return (-1); 
   }
   if (strcmp(lGetString(genericElem, GRP_group_name), group) != 0) {
     
     fprintf(stderr, MSG_ANSWER_HOSTGROUPNAMEXDIFFFROMY_SS,lGetString(genericElem,GRP_group_name ),group );
     genericElem = lFreeElem(genericElem);
     DEXIT;
     return(-1); 
   }  
   addList = lCreateList("host group list", GRP_Type);
   lAppendElem(addList, genericElem);

   alp = sge_gdi(SGE_HOST_GROUP_LIST , SGE_GDI_MOD, &addList, NULL, NULL);
   show_gdi_request_answer(alp);

   alp = lFreeList(alp);
   addList = lFreeList(addList);
   DEXIT;
   return 0;  
}


/***p** src/del_host_group_entry() **********************************
*
*  NAME
*     del_host_group_entry() -- Option qconf -dhgrp
*
*  SYNOPSIS
*
*     #include "parse_qconf.h"
*     #include <src/parse_qconf.h>
*   
*     static int del_host_group_entry( char* group );  
*
*  FUNCTION
*     This function is used in the qconf -dhgrp Option. It creates a
*     GRP_Type list for the host group to delete and will send
*     the list to the qmaster in order to remove all entries for
*     the host group specified in the parameter group.
*
*
*  INPUTS
*     char* group   - name of the host group to delete
*    
*
*  RESULT
*     int 0 on success, -1 on error
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     /()
*     
****************************************************************************
*/
static int del_host_group_entry(
char *group 
) { 
   lListElem* genericElem = NULL;
   lList* delList         = NULL;
   lList* alp             = NULL; 
   DENTER(TOP_LAYER, "del_host_group_entry");

   genericElem = lCreateElem(GRP_Type);
   lSetString(genericElem, GRP_group_name, group);

   if (genericElem == NULL) {
      fprintf(stderr, MSG_PARSE_NULLPOINTERRECEIVED); 
      DEXIT;
      return (-1);
   }
  
   delList = lCreateList("host group",GRP_Type);
   lAppendElem(delList, genericElem);

   alp = sge_gdi(SGE_HOST_GROUP_LIST , SGE_GDI_DEL, &delList, NULL, NULL);
   show_gdi_request_answer(alp);

   alp = lFreeList(alp);
   delList = lFreeList(delList);
   DEXIT;
   return 0; 
}


/***p** src/add_host_group_entry_from_file() **********************************
*
*  NAME
*     add_host_group_entry_from_file() -- Option -Ahgrp 
*
*  SYNOPSIS
*
*     #include "parse_qconf.h"
*     #include <src/parse_qconf.h>
* 
*     static int add_host_group_entry_from_file (char* filename); 
*       
*
*  FUNCTION
*     This function is used in the -Ahgrp option of qconf. It will open the file with 
*     host group entries and send it to the qmaster. The entries in the file are used 
*     to define host groups for user mapping.
*
*
*  INPUTS
*     char* filename - file with host group definition
*
*  RESULT
*     int 0 on success, -1 on error
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     /()
*     
****************************************************************************
*/
static int add_host_group_entry_from_file(
char *filename 
) {
   lListElem* genericElem = NULL;
   lList* addList         = NULL;
   lList* alp             = NULL; 
   DENTER(TOP_LAYER, "add_host_group_entry_from_file");

   /* read in file */
   if (filename == NULL) {
      fprintf(stderr, MSG_FILE_ERRORREADINGINFILE);
      DEXIT;
      return(-1);
   }

   genericElem = cull_read_in_host_group(NULL, filename, 1, 0, 0 );
   if (genericElem == NULL) {
      fprintf(stderr, MSG_FILE_ERRORREADINGINFILE);
      DEXIT;
      return(-1);
   }

   if (lGetString(genericElem, GRP_group_name) == NULL) {
      fprintf(stderr, MSG_HGRP_NOHOSTGROUPENTRYINFILEX_S,filename );
      genericElem = lFreeElem(genericElem);
      DEXIT;
      return(-1);
   } 


   /* get list from master */   
   addList = get_host_group_list_from_master(lGetString(genericElem, GRP_group_name));
   if (addList != NULL) {
     fprintf(stderr, MSG_ANSWER_HOSTGROUPENTRYALLREADYEXISTS_S,lGetString(genericElem, GRP_group_name));
     addList = lFreeList(addList);
     genericElem = lFreeElem(genericElem);
     DEXIT;
     return(-1); 
   } 

   /*  create GRP_list , add Element in GRP_list */ 
   addList = lCreateList("host group list",GRP_Type);
   lAppendElem(addList, genericElem);

   /*  send new list to qmaster */
   alp = sge_gdi(SGE_HOST_GROUP_LIST, SGE_GDI_ADD, &addList, NULL, NULL);
   show_gdi_request_answer(alp);

   alp = lFreeList(alp);
   addList = lFreeList(addList);

   DEXIT;
   return 0; 
}


/***p** src/mod_host_group_entry_from_file() **********************************
*
*  NAME
*     mod_host_group_entry_from_file() -- Option -Mhgrp 
*
*  SYNOPSIS
*
*     #include "parse_qconf.h"
*     #include <src/parse_qconf.h>
* 
*     static int mod_host_group_entry_from_file (char* filename);
*
*  FUNCTION
*     This function is used in the -Mhgrp Option in qconf. It will open the file
*     with mapping entries and send it to the qmaster. 
*
*
*  INPUTS
*     char* filename - file with host group definition 
*    
*
*  RESULT
*     int 0 on success, -1 on error
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     /()
*     
****************************************************************************
*/
static int mod_host_group_entry_from_file(
char *filename 
) {
   lListElem* genericElem = NULL;
   lList* addList         = NULL;
   lList* alp             = NULL; 
   DENTER(TOP_LAYER, "mod_host_group_entry_from_file");

   /* read in file */
   if (filename == NULL) {
      sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE);
   }

   genericElem = cull_read_in_host_group(NULL, filename, 1, 0, 0 );
   if (genericElem == NULL) {
      fprintf(stderr, MSG_FILE_ERRORREADINGINFILE );
      DEXIT;
      return(-1); 
   }

   if (lGetString(genericElem, GRP_group_name) == NULL) {
      fprintf(stderr, MSG_HGRP_NOHOSTGROUPENTRYINFILEX_S,filename );
      genericElem = lFreeElem(genericElem);
      DEXIT;
      return(-1);
   } 

   /* get list from master */   
   addList = get_host_group_list_from_master(lGetString(genericElem, GRP_group_name));
   if (addList == NULL) {
     
     fprintf(stderr,MSG_ANSWER_HOSTGROUPENTRYXNOTFOUND_S ,lGetString(genericElem, GRP_group_name));
     genericElem = lFreeElem(genericElem);
     addList = lFreeList(addList);
     DEXIT;
     return(-1); 
   } 
   addList = lFreeList(addList);
   
   addList = lCreateList("host group list",GRP_Type);
   lAppendElem(addList, genericElem);
 
   alp = sge_gdi(SGE_HOST_GROUP_LIST, SGE_GDI_MOD, &addList, NULL, NULL);
   show_gdi_request_answer(alp);

   alp = lFreeList(alp);
   addList = lFreeList(addList);

   DEXIT;
   return 0;  
}


/***p** src/mod_user_map_entry_from_file() **********************************
*
*  NAME
*     mod_user_map_entry_from_file() -- Option -Mumap 
*
*  SYNOPSIS
*
*     #include "parse_qconf.h"
*     #include <src/parse_qconf.h>
* 
*     static int mod_user_map_entry_from_file (char* filename);
*
*  FUNCTION
*     This function is used in the -Mumap Option. It will open the file with mapping 
*     entries and send it to the qmaster. The entries in the file are used to define
*     the mapping for the cluster user specified in the parameter user. 
*
*
*  INPUTS
*     char* filename - file with user mapping definition 
*    
*
*  RESULT
*     int 0 on success, -1 on error
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     /()
*     
****************************************************************************
*/
static int mod_user_map_entry_from_file(
char *filename 
) {
   lListElem* genericElem = NULL;
   lList* addList         = NULL;
   lList* alp             = NULL; 
   DENTER(TOP_LAYER, "mod_user_map_entry_from_file");

   /* read in file */
   if (filename == NULL) {
      sge_error_and_exit(MSG_FILE_ERRORREADINGINFILE);
   }

   genericElem = cull_read_in_ume(NULL, filename, 1, 0, 0 );
   if (genericElem == NULL) {
      fprintf(stderr, MSG_FILE_ERRORREADINGINFILE );
      DEXIT;
      return(-1); 
   }

   if (lGetString(genericElem, UME_cluster_user) == NULL) {
      fprintf(stderr, MSG_UM_NOCLUSTERUSERENTRYINFILEX_S,filename );
      genericElem = lFreeElem(genericElem);
      DEXIT;
      return(-1);
   }

   /* get list from master */   
   addList = get_user_mapping_list_from_master(lGetString(genericElem, UME_cluster_user));
   if (addList == NULL) {
     fprintf(stderr, MSG_ANSWER_USERMAPENTRYXNOTFOUND_S,lGetString(genericElem, UME_cluster_user) );
     addList = lFreeList(addList);
     genericElem = lFreeElem(genericElem);
     DEXIT;
     return(-1); 
   } 

   addList = lFreeList(addList);
   
   addList = lCreateList("user mapping entry",UME_Type);
   lAppendElem(addList, genericElem);
 
   alp = sge_gdi(SGE_USER_MAPPING_LIST, SGE_GDI_MOD, &addList, NULL, NULL);
   show_gdi_request_answer(alp);

   alp = lFreeList(alp);
   addList = lFreeList(addList);

   DEXIT;
   return 0;  
}






/***p** src/del_user_map_entry() **********************************
*
*  NAME
*     del_user_map_entry() -- Option -dumap
*
*  SYNOPSIS
*
*     #include "parse_qconf.h"
*     #include <src/parse_qconf.h>
*   
*     static int del_user_map_entry (char* user);  
*
*  FUNCTION
*     This function is used in the -dumap Option. It creates a
*     UME_Type list for the user mapping to delete and will send
*     the list to the qmaster in order to remove all entries for
*     the cluster user specified in the parameter user.
*
*
*  INPUTS
*     char* user     - name of the cluster user to delete the mapping
*    
*
*  RESULT
*     int 0 on success, -1 on error
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     /()
*     
****************************************************************************
*/
static int del_user_map_entry(
char *user 
) {   
   lListElem* genericElem = NULL;
   lList* delList         = NULL;
   lList* alp             = NULL; 
   DENTER(TOP_LAYER, "del_user_map_entry");

   genericElem = sge_create_ume(user,NULL,NULL);

   lSetString(genericElem, UME_cluster_user, user);
   if (genericElem == NULL) {
      fprintf(stderr, MSG_PARSE_NULLPOINTERRECEIVED); 
      DEXIT;
      return (-1);
   }
  
   delList = lCreateList("user mapping entry",UME_Type);
   lAppendElem(delList, genericElem);

   alp = sge_gdi(SGE_USER_MAPPING_LIST, SGE_GDI_DEL, &delList, NULL, NULL);
   show_gdi_request_answer(alp);

   alp = lFreeList(alp);
   delList = lFreeList(delList);
   DEXIT;
   return 0; 
}

#endif

/***************************************************************************
  -ac|-Ac|-mc|-Mc|-dc option 
  May be its cleaner to split the options in several routines. 
 ***************************************************************************/
static int add_chg_cmplx(
char *cmplx_name,
int add,                        /* == 0 modify ==1 add ==2 delete */
char *fname                     /* != NULL if we read the complex from file */
) {
   char *tmpname = NULL; 
   const char *cp = NULL;
   stringT str;
   lCondition *where;
   lEnumeration *what;
   lList *alp = NULL, *lp = NULL;
   lListElem *ep;
   int status, op;
   lList *answer = NULL;

   DENTER(TOP_LAYER, "add_chg_cmplx");

   /* test cmplx_name */
   if (verify_filename(cmplx_name)) {
      fprintf(stderr, MSG_QCONF_XISNOVALIDCOMPLEXNAME_S, cmplx_name);
      DEXIT;
      return -1;
   }

   /* look whether this complex already exist */
   where = lWhere("%T(%I==%s)", CX_Type, CX_name, cmplx_name);
   if (!where)
      fprintf(stderr, MSG_ANSWER_MALFORMEDREQUEST);

   if (add)  /* add/delete only need complex names */
      what = lWhat("%T(%I)", CX_Type, CX_name);
   else
      what = lWhat("%T(ALL)", CX_Type);

   if (!what)
      fprintf(stderr, MSG_ANSWER_MALFORMEDREQUEST);


   alp = sge_gdi(SGE_COMPLEX_LIST, SGE_GDI_GET, &lp, where, what);

   what = lFreeWhat(what);
   where = lFreeWhere(where);

   ep = lFirst(alp);
   if (sge_get_recoverable(ep) != STATUS_OK) {
      fprintf(stderr, "%s\n", lGetString(ep, AN_text));
   }
   else {
      if (add==1) {  /* add */
         if (lp) {
            fprintf(stderr, MSG_ANSWER_COMPLEXXALREADYEXISTS_S, cmplx_name);
            return -1;
         }
      }
      else {
         if (!lp) {  /* modify / delete */
            fprintf(stderr, MSG_ANSWER_COMPLEXXDOESNOTEXIST_S, cmplx_name);
            return -1;
         } 
      }
   }
   if (!add)                    /* store complex which we want to modify */
      ep = lCopyElem(lFirst(lp));
   lFreeList(alp);
   lFreeList(lp);

   if (add != 2) {      /* delete needs no editing */
      if (!fname) {     /* in case of -Ac|-Mc we have already a file */
         /* make a temporary file for editing */
         if (!(tmpname = sge_tmpnam(str))) {
            if (sge_error_and_exit(MSG_FILE_CANTCREATETEMPFILE))
               return -1;
         }
         close(creat(tmpname, 0755));
      }
      if (add) {                /* if we modify an element we have it already */
         /* Write an empty complex table */
         ep = lCreateElem(CX_Type);
         lSetString(ep, CX_name, cmplx_name);
      }

      if (!fname)
         write_cmplx(0, tmpname, lGetList(ep, CX_entries), NULL, NULL);

      lFreeElem(ep);

      if (fname)
         tmpname = fname;
      else {
         status = sge_edit(tmpname);
         if (status < 0) {
            DTRACE;
            unlink(tmpname);
            if (sge_error_and_exit(MSG_PARSE_EDITFAILED))
               return -1;
         }
         DTRACE;

         if (status > 0) {
            unlink(tmpname);
            if (sge_error_and_exit(MSG_FILE_FILEUNCHANGED))
               return -1;
         }
      }

      if (!(ep = read_cmplx(tmpname, cmplx_name, &answer))) {
         answer = lFreeList(answer);
         if (!fname)
            unlink(tmpname); 
         DEXIT;
         return -1;
      }
      if (!fname) 
         unlink(tmpname);
   }

   lp = lCreateList("complex to modify, add or delete", CX_Type);
   if (add == 2) /* delete */
      lAddElemStr( &lp, CX_name, cmplx_name, CX_Type);
   else
      lAppendElem(lp, ep); 


   if (add==0)
      op = SGE_GDI_MOD;
   else if (add==1)
      op = SGE_GDI_ADD;
   else
      op = SGE_GDI_DEL;

   alp = sge_gdi(SGE_COMPLEX_LIST, op, &lp, NULL, NULL);

   /* report results */
   ep = lFirst(alp);
   if (sge_get_recoverable(ep) == STATUS_OK) {

      switch (add) {
         case 0:
            cp = MSG_MULTIPLY_MODIFIEDIN;
            break;
         case 1:
            cp = MSG_MULTIPLY_ADDEDTO;
            break;
         case 2:
            cp = MSG_MULTIPLY_DELETEDFROM;
      }
         
      fprintf(stderr, MSG_ANSWER_XYCOMPLEXLIST_SS, cmplx_name, cp);
   }
   else
      fprintf(stderr, "%s\n", lGetString(ep, AN_text));
   
   lFreeList(lp);
   lFreeList(alp);

   DEXIT;
   return 0;
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
   lListElem *argep, *ep;
   int fail=0;
   const char *acl_name;
   lList *alpp=NULL;
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
      if (!ep) {
         fprintf(stderr, MSG_SGETEXT_DOESNOTEXIST_SS, "access list", acl_name);
         fail = 1;
      }
      else {
         if (first_time)
            first_time = 0;
         else
            printf("\n");
         write_userset(&alpp, ep, NULL, stdout, 0);
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
   lListElem *argep, *ep, *aep, *changed_ep;
   int status;
   const char *userset_name;
   lList *alp, *lp;
   char fname[SGE_PATH_MAX];
   int cmd;

   DENTER(TOP_LAYER, "edit_usersets");

   /* get all usersets named in arglp, put them into usersets */
   if (sge_client_get_acls(NULL, arglp, &usersets)) {
      DEXIT;
      return -1;
   }

   if (!sge_tmpnam(fname)) {
      DEXIT;
      return -2;
   }

   for_each (argep, arglp) {
      alp = NULL;
      userset_name = lGetString(argep, US_name);

      ep=lGetElemStr(usersets, US_name, userset_name);
      if (!ep) {
         ep = lAddElemStr(&usersets, US_name, userset_name, US_Type);
         /* initialize type field in case of sge */
         if (feature_is_enabled(FEATURE_SGEEE))
            lSetUlong(ep, US_type, US_ACL|US_DEPT);
         cmd = SGE_GDI_ADD;
      } else 
         cmd = SGE_GDI_MOD;

      if (write_userset(&alp, ep, fname, NULL, 0)) {
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

      changed_ep = cull_read_in_userset(NULL, fname, 0, 0, NULL);

      if (!changed_ep) {
         fprintf(stderr, MSG_FILE_ERRORREADINGUSERSETFROMFILE_S, fname);
         continue;   /* May be the user made a mistake. Just proceed with 
                        the next */
      }

      /* Create List; append Element; and do a modification gdi call */
      lp = lCreateList("userset list", US_Type);
      lAppendElem(lp, changed_ep);
      alp = sge_gdi(SGE_USERSET_LIST, cmd, &lp, NULL, NULL);
      lFreeList(lp);

      for_each(aep, alp) 
         fprintf(stdout, "%s", lGetString(aep, AN_text));
   }

   DEXIT;
   return 0;
}

/***************************************************************************
  -sc option 
 ***************************************************************************/
static int print_cmplx(
const char *cmplx_name 
) {
   lCondition *where;
   lEnumeration *what;
   lList *alp = NULL, *lp = NULL;
   lListElem *ep;
   int fail=0;

   DENTER(TOP_LAYER, "print_cmplx");

   /* get complex */
   where = lWhere("%T(%I==%s)", CX_Type, CX_name, cmplx_name);
   what = lWhat("%T(ALL)", CX_Type);
   alp = sge_gdi(SGE_COMPLEX_LIST, SGE_GDI_GET, &lp, where, what);
   what = lFreeWhat(what);
   where = lFreeWhere(where);

   ep = lFirst(alp);
   if (sge_get_recoverable(ep) != STATUS_OK) {
      fprintf(stderr, "%s\n", lGetString(ep, AN_text));
      fail = 1;
   }
   else {
      if (!(ep = lFirst(lp))) {
         fprintf(stderr, MSG_COMPLEX_COMPLEXXNOTDEFINED_S , cmplx_name);
         DEXIT;
         return 1;
      }
      write_cmplx(0, NULL, lGetList(ep, CX_entries), stdout, NULL);
   }

   lFreeList(alp);
   lFreeList(lp);

   DEXIT;
   return fail;
}

/***************************************************************************
  -sconf option 
 ***************************************************************************/
static int print_config(
const char *config_name 
) {
   lCondition *where;
   lEnumeration *what;
   lList *alp = NULL, *lp = NULL;
   lListElem *ep;
   int fail=0;
   const char *cfn;
   
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
   if (sge_get_recoverable(ep) != STATUS_OK) {
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
      write_configuration(0, NULL, NULL, ep, stdout, 0L);
   }

   lFreeList(alp);
   lFreeList(lp);

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
   lListElem *ep;
   int fail = 0;
   
   DENTER(TOP_LAYER, "delete_config");

   lAddElemHost(&lp, CONF_hname, config_name, CONF_Type);
   alp = sge_gdi(SGE_CONFIG_LIST, SGE_GDI_DEL, &lp, NULL, NULL);

   ep = lFirst(alp);
   fprintf(stderr, "%s\n", lGetString(ep, AN_text));

   fail = !(sge_get_recoverable(ep) == STATUS_OK);

   lFreeList(alp);
   lFreeList(lp);

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
   stringT str;
   lCondition *where;
   lEnumeration *what;
   lList *alp = NULL, *lp = NULL;
   lListElem *ep;
   int failed=0;
   char *tmpname;
   int status;
   
   DENTER(TOP_LAYER, "add_modify_config");

   where = lWhere("%T(%I == %s)", CONF_Type, CONF_hname, cfn);
   what = lWhat("%T(ALL)", CONF_Type);
   alp = sge_gdi(SGE_CONFIG_LIST, SGE_GDI_GET, &lp, where, what);
   what = lFreeWhat(what);
   where = lFreeWhere(where);

   failed = FALSE;
   ep = lFirst(alp);
   if (sge_get_recoverable(ep) != STATUS_OK) {
      fprintf(stderr, "%s\n", lGetString(ep, AN_text));
      lFreeList(alp);
      lFreeList(lp);
      DEXIT;
      return 1;
   }

   lFreeList(alp);

   ep = lCopyElem(lFirst(lp));
   lFreeList(lp);

   if (ep && (flags == 1)) {
      fprintf(stderr, MSG_ANSWER_CONFIGXALREADYEXISTS_S, cfn);
      lFreeElem(ep);
      DEXIT;
      return 2;
   }
   if (!ep && (flags == 2)) {
      fprintf(stderr, MSG_ANSWER_CONFIGXDOESNOTEXIST_S, cfn);
      lFreeElem(ep);
      DEXIT;
      return 3;
   }
   

   if (!filename) {
      /* make temp file */
      if (!(tmpname = sge_tmpnam(str))) {
         sge_error_and_exit( MSG_FILE_CANTCREATETEMPFILE);
      }
      close(creat(tmpname, 0755));
      DPRINTF(("tmpname is: %s\n", tmpname));
   
      /* get config or make an empty config entry if none exists */
      if (ep == NULL) {
         ep = lCreateElem(CONF_Type);
         lSetHost(ep, CONF_hname, cfn);
      }   
 
      write_configuration(0, NULL, tmpname, ep, NULL, 0L);
   
      lFreeElem(ep);
      status = sge_edit(tmpname);
      
      if (status != 0) {
         unlink(tmpname);
         failed = TRUE;
      }
      if (status < 0) {
         fprintf(stderr, MSG_PARSE_EDITFAILED);
         DEXIT;
         return failed;
      }
      else if (status > 0) {
         fprintf(stderr,MSG_ANSWER_CONFIGUNCHANGED  );
         DEXIT;
         return failed;
      }
      if (!(ep = read_configuration(tmpname, cfn, 0L))) {
         fprintf(stderr, MSG_ANSWER_ERRORREADINGTEMPFILE);
         unlink(tmpname);
         failed = TRUE;
         DEXIT;
         return failed;
      }
      unlink(tmpname);
   }
   else {
      ep = read_configuration(filename, cfn, 0L);
      if (!ep) {
         fprintf(stderr, MSG_ANSWER_ERRORREADINGCONFIGFROMFILEX_S, filename);
         failed = TRUE;
         DEXIT;
         return failed;
      }      
   }

   lp = lCreateList("modified configuration", CONF_Type); 
   lAppendElem(lp, ep);
     
   alp = sge_gdi(SGE_CONFIG_LIST, SGE_GDI_MOD, &lp, NULL, NULL);
   lFreeList(lp);
            
   /* report results */
   ep = lFirst(alp);                   
         
   failed = !(sge_get_recoverable(ep) == STATUS_OK);
         
   fprintf(stderr, "%s\n", lGetString(ep, AN_text));
     
   lFreeList(alp);
   
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
char *user 
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
   if (perm_return == TRUE) {
     /* user is manager */
     if (alp != NULL) {
        lFreeList(alp);
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
      lFreeList(alp);
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
char *host 
) {
   lCondition *where;
   lEnumeration *what;
   lList *alp;
   lListElem *aep;
   lList *lp = NULL;
   lListElem  *ep;

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
      lFreeList(alp);
      SGE_EXIT(1);
   }
   lFreeList(alp);

   ep = lGetElemHost(lp, AH_name, host);

   if (!ep) {
      /*
      ** host is no adminhost
      */
      lFreeList(lp);
      fprintf(stderr, MSG_ANSWER_DENIEDHOSTXISNOADMINHOST_S, host);
      SGE_EXIT(1);
   }

   lFreeList(lp);
   DEXIT;
   return 0;
}


/****** src/qconf_modify_attribute() **********************************
*
*  NAME
*     qconf_modify_attribute() -- sends a modify request to the master 
*
*  SYNOPSIS
*
*     static int qconf_modify_attribute (
*        lList **alpp, 
*        int from_file,
*        char ***spp, 
*        int sub_command,
*        struct object_info_entry *info_entry
*     );  
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
*                  
*
*  RESULT
*     [alpp] Masters answer for the gdi request
*     1 for error
*     0 for success
*     
*
*  EXAMPLE
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*       
**************************************************************************
*/

static int qconf_modify_attribute(
lList **alpp,
int from_file,
char ***spp,
lListElem **epp,
int sub_command,
struct object_info_entry *info_entry 
) {
   DENTER(TOP_LAYER, "qconf_modify_attribute"); 
   
   if (info_entry->cull_read_in_object) {
      int fields[150];
      lListElem *add_qp;
      lList *qlp = NULL;
      lEnumeration *what;

      fields[0] = NoName;

      if (from_file) {
         if (sge_next_is_an_opt(*spp))  {
            SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_ANSWER_MISSINGFILENAMEASOPTIONARG_S, 
               "qconf"));
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, NUM_AN_ERROR);
            return 1;
         }                
         *epp = info_entry->cull_read_in_object(NULL, **spp, 0, 
            info_entry->nm_name, NULL, fields);
      } else {
         lListElem *cfep;
         lList *cflp = NULL;
         int tag;

         /* build up a config name/value list 
            as it is used by read_queue_work() */
         cfep = lAddElemStr(&cflp, CF_name, **spp, CF_Type);
         *spp = sge_parser_get_next(*spp);
         lSetString(cfep, CF_value, **spp);

         /* transform config list into a queue element */
         *epp = lCreateElem(info_entry->cull_descriptor);
         if (info_entry->read_objectname_work(alpp, &cflp, fields, *epp, 0, 
               info_entry->nm_name, &tag, sub_command==SGE_GDI_REMOVE?1:0)) {
            return 1;
         }                           
         if (lGetNumberOfElem(cflp) > 0) {
            SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_QCONF_XISNOTAOBJECTATTRIB_SSS, "qconf", 
                    lGetString(lFirst(cflp), CF_name), info_entry->object_name));
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, NUM_AN_ERROR);
            return 1;
         }
      }
      /* add object name to int vector and transform
         it into an lEnumeration */
      if (add_nm_to_set(fields, info_entry->nm_name) < 0) {
         SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_QCONF_CANTCHANGEOBJECTNAME_SS, "qconf", 
            info_entry->attribute_name));
         sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, NUM_AN_ERROR);
         return 1;
      }

      if (!(what = lIntVector2What(info_entry->cull_descriptor, fields))) {
         SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_QCONF_INTERNALFAILURE_S, "qconf"));
         return 1;
      }              
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
               SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_QCONF_INTERNALFAILURE_S, "qconf"));
               sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, NUM_AN_ERROR);
               return 1;
         }
         lAppendElem(qlp, add_qp);
      }
      if (!qlp) {
         SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_QCONF_MQATTR_MISSINGOBJECTLIST_S, 
            "qconf"));
         sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, NUM_AN_ERROR);
         return 1;
      }

      *alpp = sge_gdi(info_entry->target, SGE_GDI_MOD | sub_command, &qlp, 
         NULL, what);
      what = lFreeWhat(what);
      qlp = lFreeList(qlp);
      *epp = lFreeElem(*epp);
      (*spp)++;
   }

   DEXIT;
   return 0;
}

#ifndef __SGE_NO_USERMAPPING__
/***p** src/get_user_mapping_list_from_master() **********************************
*
*  NAME
*     get_user_mapping_list_from_master() -- get user mapping entry list from master 
*
*  SYNOPSIS
*
*     #include "parse_qconf.h"
*     #include <src/parse_qconf.h>
* 
*     static lList*     get_user_mapping_list_from_master(char* user);
*       
*
*  FUNCTION
*     The function performs a SGE_GDI_GET request to the qmaster in
*     order to get the user mapping entry list for the specified user.
*
*  INPUTS
*     char* user - name of the cluster user
*
*  RESULT
*     lList*     - pointer to UME_Type list (caller must free the list)
*     NULL       - on error
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     /()
*     
****************************************************************************
*/
lList* get_user_mapping_list_from_master(
const char *user 
) {
   lList *alp = NULL; 
   lList *umlp = NULL;
   DENTER(TOP_LAYER, "get_user_mapping_list_from_master");

   if (user != NULL) {
     lEnumeration *what = NULL;
     lCondition *where = NULL;
     lListElem *ep = NULL;

     what = lWhat("%T(ALL)", UME_Type); 
     where = lWhere("%T(%I==%s)", UME_Type, UME_cluster_user, user);
     alp = sge_gdi(SGE_USER_MAPPING_LIST, SGE_GDI_GET, &umlp, where, what);
     what = lFreeWhat(what);
     where = lFreeWhere(where);

 
     ep = lFirst(alp);
     if (sge_get_recoverable(ep) != STATUS_OK) {
       fprintf(stderr, "%s\n", lGetString(ep, AN_text));
       if (sge_get_recoverable(ep) == STATUS_ENOIMP) {
         alp = lFreeList(alp);
         umlp = lFreeList(umlp);
         SGE_EXIT(1); 
       }  
       alp = lFreeList(alp);
       umlp = lFreeList(umlp);
       
       DEXIT;
       return NULL;
     }
     alp = lFreeList(alp);
   }

   DEXIT;
   return umlp; 
}

/****** src/get_host_group_list_from_master() **********************************
*
*  NAME
*     get_host_group_list_from_master() -- get GRP_List from master per GDI request 
*
*  SYNOPSIS
*
*     #include "parse_qconf.h"
*     #include <src/parse_qconf.h>
* 
*     static lList*     get_host_group_list_from_master(char* group);
*       
*  FUNCTION
*     The function performs a SGE_GDI_GET request to the qmaster in
*     order to get the GRP_Type list for the specified group.
*
*  INPUTS
*     char* group - name of the host group 
*
*  RESULT
*     lList*      - pointer to GRP_Type list (caller must free the list)
*     NULL        - on error
*
*  EXAMPLE
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*     src/()
*     src/()
*     
****************************************************************************
*/
lList*  get_host_group_list_from_master(
const char *group 
) {
   lList *alp = NULL; 
   lList *umlp = NULL;
   DENTER(TOP_LAYER, "get_host_group_list_from_master");

   if (group != NULL) {
     lEnumeration *what = NULL;
     lCondition *where = NULL;
     lListElem *ep = NULL;

     what = lWhat("%T(ALL)", GRP_Type); 
     where = lWhere("%T(%I==%s)", GRP_Type, GRP_group_name, group);
     alp = sge_gdi(SGE_HOST_GROUP_LIST, SGE_GDI_GET, &umlp, where, what);
     what = lFreeWhat(what);
     where = lFreeWhere(where);

 
     ep = lFirst(alp);
     if (sge_get_recoverable(ep) != STATUS_OK) {
       fprintf(stderr, "%s\n", lGetString(ep, AN_text));
       if (sge_get_recoverable(ep) == STATUS_ENOIMP) {
         alp = lFreeList(alp);
         umlp = lFreeList(umlp);
         SGE_EXIT(1); 
       }  
       alp = lFreeList(alp);
       umlp = lFreeList(umlp);
       
       DEXIT;
       return NULL;
     }
     alp = lFreeList(alp);
   }

   DEXIT;
   return umlp; 
}

#endif /* __SGE_NO_USERMAPPING__ */
/***p** src/show_user_map_entry() **********************************
*
*  NAME
*     show_user_map_entry() -- print user map entries of cluster user 
*
*  SYNOPSIS
*
*     #include "parse_qconf.h"
*     #include <src/parse_qconf.h>
* 
*     static int show_user_map_entry(char *user);
*       
*
*  FUNCTION
*     print current user mapping entries to stdout. The parameter
*     user specifies the name of the cluster user.
*
*  INPUTS
*     char* user - name of the cluster user to show the mapping
*
*  RESULT
*     int TRUE on success, FALSE on error
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     /()
*     
****************************************************************************
*/
#ifndef __SGE_NO_USERMAPPING__
static int show_user_map_entry(char *user)
{ 
    lList *umlp = NULL;
    lListElem *ep = NULL;
 
    DENTER(TOP_LAYER, "show_user_map_entry");
  
    if (user == NULL) {
       DEXIT;
       return FALSE;
    }
      
    umlp = get_user_mapping_list_from_master(user);   

    if ((ep = lFirst(umlp)) == NULL) {
      fprintf(stderr, MSG_ERROR_USERXNOTDEFINED_S , user);
      lFreeList(umlp);
      DEXIT;
      return FALSE; 
    }

    for_each(ep, umlp) {
      write_ume((int)0,(int)0, ep); 
    }    

    lFreeList(umlp);

    DEXIT;
    return TRUE; 
}
#endif

#ifndef __SGE_NO_USERMAPPING__




/****** src/show_host_group_entry() **********************************
*
*  NAME
*     show_host_group_entry() -- print houst group to stdout 
*
*  SYNOPSIS
*
*     "parse_qconf.c"
* 
*     static int show_host_group_entry(char* group);
*       
*
*  FUNCTION
*     This function uses gdi request to get group from qmaster and
*     prints the given group on stdout.
*      
*  INPUTS
*     char* group - name of group
*  RESULT
*
*  EXAMPLE
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*     src/()
*     src/()
*     
****************************************************************************
*/
static int show_host_group_entry(
char *group 
) { 
    lList *grp_lp = NULL;
    lListElem *ep = NULL;
 
    DENTER(TOP_LAYER, "show_host_group_entry");
  
    if (group == NULL) {
       DEXIT;
       return FALSE;
    }
      
    grp_lp = get_host_group_list_from_master(group);   

    if ((ep = lFirst(grp_lp)) == NULL) {
      fprintf(stderr, MSG_ERROR_GROUPXNOTDEFINED_S , group );
      lFreeList(grp_lp);
      DEXIT;
      return FALSE; 
    }

    for_each(ep, grp_lp) {
      write_host_group((int)0,(int)0, ep); 
    }    

    lFreeList(grp_lp);

    DEXIT;
    return TRUE; 
}
#endif
