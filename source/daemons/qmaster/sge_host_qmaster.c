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

#include "sge.h"
#include "sge_conf.h"
#include "symbols.h"
#include "sge_prog.h"
#include "sge_time.h"
#include "sge_feature.h"
#include "sge_idL.h"
#include "sge_ja_task.h"
#include "commlib.h"
#include "sge_host.h"
#include "sge_manop.h"
#include "sge_host_qmaster.h"
#include "sge_gdi_request.h"
#include "sge_utility.h"
#include "sge_event_master.h"
#include "sge_queue_event_master.h"
#include "sge_static_load.h"
#include "sge_job_schedd.h"
#include "sge_c_gdi.h"
#include "mail.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_parse_num_par.h"
#include "configuration_qmaster.h"
#include "sge_qmod_qmaster.h"
#include "sge_cqueue_qmaster.h"
#include "sort_hosts.h"
#include "sge_userset_qmaster.h"
#include "sge_userprj_qmaster.h"
#include "time_event.h"
#include "sge_complex_schedd.h"
#include "reschedule.h"
#include "sge_string.h"
#include "sge_security.h"
#include "sge_unistd.h"
#include "sge_hostname.h"
#include "sge_answer.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_job.h"
#include "sge_report.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_utility_qmaster.h"
#include "qmaster_to_execd.h"
#include "sge_todo.h"
#include "sge_centry.h"
#include "sge_href.h"
#include "sge_cqueue.h"
#include "sge_str.h"
#include "sge_load.h"

#include "sge_persistence_qmaster.h"
#include "sge_reporting_qmaster.h"
#include "spool/sge_spooling.h"

#include "msg_common.h"
#include "msg_qmaster.h"

static void master_kill_execds(sge_gdi_request *request, sge_gdi_request *answer);

static void host_trash_nonstatic_load_values(lListElem *host);

static void notify(lListElem *lel, sge_gdi_request *answer, int kill_jobs, int force);

static int verify_scaling_list(lList **alpp, lListElem *host); 

/****** qmaster/host/host_trash_nonstatic_load_values() ***********************
*  NAME
*     host_trash_nonstatic_load_values() -- Trash old load values 
*
*  SYNOPSIS
*     static void host_trash_nonstatic_load_values(lListElem *host) 
*
*  FUNCTION
*     Trash old load values in "host" element 
*
*  INPUTS
*     lListElem *host - EH_Type element 
*
*  RESULT
*     void - None
*******************************************************************************/
static void host_trash_nonstatic_load_values(lListElem *host) 
{
   lList *load_attr_list;
   lListElem *next_load_attr;
   lListElem *load_attr;

   load_attr_list = lGetList(host, EH_load_list);
   next_load_attr = lFirst(load_attr_list);
   while ((load_attr = next_load_attr)) {
      const char *load_attr_name = lGetString(load_attr, HL_name);

      next_load_attr = lNext(load_attr);
      if (!sge_is_static_load_value(load_attr_name)) {
         lRemoveElem(load_attr_list, load_attr);
      }
   }
   if (lGetNumberOfElem(load_attr_list) == 0) {
      lSetList(host, EH_load_list, NULL);
   }
}

/* ------------------------------------------ 

   adds the host to host_list with type 

   -1 error 
   0 ok
   1 host does exist

 */
int sge_add_host_of_type(
const char *hostname,
u_long32 target 
) {
   int ret;
   int dataType;
   int pos;
   lListElem *ep;
   gdi_object_t *object;

   DENTER(TOP_LAYER, "sge_add_host_of_type");

   if (!hostname) {
      DEXIT;
      return -1;
   }
  
   object = get_gdi_object(target);
   
   /* prepare template */
   ep = lCreateElem(object->type);
   pos = lGetPosInDescr(object->type,object->key_nm);
   dataType = lGetPosType(object->type , pos);
   switch (dataType) {
      case lStringT:
         lSetString(ep, object->key_nm, hostname);
         break;
      case lHostT:
         lSetHost(ep, object->key_nm, hostname);
         break;
      default:
         DPRINTF(("sge_add_host_of_type: unexpected datatype\n"));
   }
   ret = sge_gdi_add_mod_generic(NULL, ep, 1, object, uti_state_get_user_name(), 
      uti_state_get_qualified_hostname(), 0);
   lFreeElem(ep);       

   DEXIT;
   return (ret == STATUS_OK) ? 0 : -1;
}

bool
host_list_add_missing_href(lList *this_list, 
                           lList **answer_list, const lList *href_list)
{
   bool ret = true;
   lListElem *href = NULL;

   DENTER(TOP_LAYER, "host_list_add_missing_href");
   for_each(href, href_list) {
      const char *hostname = lGetHost(href, HR_name);
      lListElem *host = host_list_locate(this_list, hostname);

      if (host == NULL) {
         ret &= (sge_add_host_of_type(hostname, SGE_EXECHOST_LIST) == 0);
      }
   }
   DEXIT;
   return ret;
}

/* ------------------------------------------------------------

   sge_del_host - deletes a host from the host_list

   if the invoking process is the qmaster the host list is
   spooled to disk

*/
int sge_del_host(
lListElem *hep,
lList **alpp,
char *ruser,
char *rhost,
u_long32 target 
) {
   int pos;
   lListElem *ep;
   const char *host;
   char unique[MAXHOSTLEN];
   lList **host_list = NULL;
   int nm = 0;
   char *name = NULL;
   int found_host;

   DENTER(TOP_LAYER, "sge_del_host");

   if ( !hep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   switch ( target ) {
   case SGE_EXECHOST_LIST:
      host_list = &Master_Exechost_List;
      nm = EH_name;
      name = "execution host";
      break;
   case SGE_ADMINHOST_LIST:
      host_list = &Master_Adminhost_List;
      nm = AH_name;
      name = "administrative host";
      break;
   case SGE_SUBMITHOST_LIST:
      host_list = &Master_Submithost_List;
      nm = SH_name;
      name = "submit host";
      break;
   default:
     DEXIT;
     return STATUS_EUNKNOWN;
   }
   /* ep is no host element, if ep has no nm */
   if ((pos = lGetPosViaElem(hep, nm)) < 0) {
      ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(nm), SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   host = lGetPosHost(hep, pos);
   if (!host) {
      ERROR((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* check if host is in host list */
   found_host = 1;
   if ((ep=host_list_locate(*host_list, host))==NULL) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, name, host));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      found_host = 0;
   }

   if (found_host) {
      strcpy(unique, host);     /* no need to make unique anymore */
   }
   else {
      /* may be host was not the unique hostname.
         Get the unique hostname and try to find it again. */
#ifdef ENABLE_NGC
      if (getuniquehostname(host, unique, 0)!=CL_RETVAL_OK)
#else
      if (getuniquehostname(host, unique, 0)!=CL_OK)
#endif
      {
         ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, host));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
      /* again check if host is in host list. This time use the unique
         hostname */
      if ((ep=host_list_locate(*host_list, unique))==NULL) {
         ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, name, host));
         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EEXIST;
      }
   }

   /* 
      check if someone tries to delete 
      the qmaster host from admin host list
   */
   if (target==SGE_ADMINHOST_LIST && 
         !sge_hostcmp(unique, uti_state_get_qualified_hostname())) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTDELADMINQMASTER_S, 
          uti_state_get_qualified_hostname()));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   if (target == SGE_EXECHOST_LIST && 
       host_is_referenced(hep, NULL, 
                          *(object_type_get_master_list(SGE_TYPE_CQUEUE)))) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTDELEXECACTIVQ_S, unique));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_ESEMANTIC;
   }

   if (target==SGE_EXECHOST_LIST && !strcasecmp(unique, "global")) {
      ERROR((SGE_EVENT, MSG_OBJ_DELGLOBALHOST));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_ESEMANTIC;
   }

   /* remove host file and send event */
   switch(target) {
      case SGE_ADMINHOST_LIST:
         {
            lList *answer_list = NULL;
            sge_event_spool(&answer_list, 0, sgeE_ADMINHOST_DEL, 
                            0, 0, host, NULL, NULL,
                            NULL, NULL, NULL, true, true);
            answer_list_output(&answer_list);
         }
         break;
      case SGE_EXECHOST_LIST:
         {
            lList *answer_list = NULL;
            sge_event_spool(&answer_list, 0, sgeE_EXECHOST_DEL, 
                            0, 0, host, NULL, NULL,
                            NULL, NULL, NULL, true, true);
            answer_list_output(&answer_list);
         }
         break;
      case SGE_SUBMITHOST_LIST:
         {
            lList *answer_list = NULL;
            sge_event_spool(&answer_list, 0, sgeE_SUBMITHOST_DEL, 
                            0, 0, host, NULL, NULL,
                            NULL, NULL, NULL, true, true);
            answer_list_output(&answer_list);
         }
         break;
   }

   /* delete found host element */
   lRemoveElem(*host_list, ep);

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS, 
         ruser, rhost, unique, name));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   DEXIT;
   return STATUS_OK;
}

/* ------------------------------------------------------------ */

int host_mod(
lList **alpp,
lListElem *new_host,
lListElem *ep,
int add,
const char *ruser,
const char *rhost,
gdi_object_t *object,
int sub_command 
) {
   const char *host;
   int nm;
   int pos;
   int dataType;

   DENTER(TOP_LAYER, "host_mod");

   nm = object->key_nm;

   /* ---- [EAS]H_name */
   if (add) {
      if (attr_mod_str(alpp, ep, new_host, nm, object->object_name)) {
         goto ERROR;
      }
   }
   pos = lGetPosViaElem(new_host, nm);
   dataType = lGetPosType(lGetElemDescr(new_host),pos);
   if (dataType == lHostT) {
      host = lGetHost(new_host, nm);
   } else {
      host = lGetString(new_host, nm);
   }
   if (nm == EH_name) {
      /* ---- EH_scaling_list */
      if (lGetPosViaElem(ep, EH_scaling_list)>=0) {
         attr_mod_sub_list(alpp, new_host, EH_scaling_list, HS_name, ep,
            sub_command, SGE_ATTR_LOAD_SCALING, SGE_OBJ_EXECHOST, 0);
         if (verify_scaling_list(alpp, new_host)!=STATUS_OK)
            goto ERROR;
      }

      /* ---- EH_consumable_config_list */
      if (attr_mod_threshold(alpp, ep, new_host, EH_consumable_config_list, 
            CE_name, sub_command, SGE_ATTR_COMPLEX_VALUES, SGE_OBJ_EXECHOST)) { 
         goto ERROR;
      }

      /* ---- EH_acl */
      if (lGetPosViaElem(ep, EH_acl)>=0) {
         DPRINTF(("got new EH_acl\n"));
         /* check acl list */
         if (userset_list_validate_acl_list(lGetList(ep, EH_acl), alpp)!=STATUS_OK)
            goto ERROR;
         attr_mod_sub_list(alpp, new_host, EH_acl, US_name, ep,
            sub_command, SGE_ATTR_USER_LISTS, SGE_OBJ_EXECHOST, 0);
      }

      /* ---- EH_xacl */
      if (lGetPosViaElem(ep, EH_xacl)>=0) {
         DPRINTF(("got new EH_xacl\n"));
         /* check xacl list */
         if (userset_list_validate_acl_list(lGetList(ep, EH_xacl), alpp)!=STATUS_OK)
            goto ERROR;
         attr_mod_sub_list(alpp, new_host, EH_xacl, US_name, ep,
            sub_command, SGE_ATTR_XUSER_LISTS, SGE_OBJ_EXECHOST, 0);
      }

      if (feature_is_enabled(FEATURE_SGEEE)) {

         /* ---- EH_prj */
         if (lGetPosViaElem(ep, EH_prj)>=0) {
            DPRINTF(("got new EH_prj\n"));
            /* check prj list */
            if (verify_userprj_list(alpp, lGetList(ep, EH_prj),
                     Master_Project_List, "projects",
                     object->object_name, host)!=STATUS_OK)
               goto ERROR;
         attr_mod_sub_list(alpp, new_host, EH_prj, UP_name, ep,
            sub_command, SGE_ATTR_PROJECTS, SGE_OBJ_EXECHOST, 0);    
         }

         /* ---- EH_xprj */
         if (lGetPosViaElem(ep, EH_xprj)>=0) {
            DPRINTF(("got new EH_xprj\n"));
            /* check xprj list */
            if (verify_userprj_list(alpp, lGetList(ep, EH_xprj), 
                     Master_Project_List, "xprojects",
                     object->object_name, host)!=STATUS_OK)
               goto ERROR;
         attr_mod_sub_list(alpp, new_host, EH_xprj, UP_name, ep,
            sub_command, SGE_ATTR_XPROJECTS, SGE_OBJ_EXECHOST, 0);   
         }
      }

      if (feature_is_enabled(FEATURE_SGEEE)) {
         /* ---- EH_usage_scaling_list */
         if (lGetPosViaElem(ep, EH_usage_scaling_list)>=0) {
            attr_mod_sub_list(alpp, new_host, EH_usage_scaling_list, HS_name, ep,
            sub_command, SGE_ATTR_USAGE_SCALING, SGE_OBJ_EXECHOST, 0); 
         }

         /* ---- EH_resource_capability_factor */
         attr_mod_double(ep, new_host, EH_resource_capability_factor, 
            "resource_capability_factor");
      }

      if (lGetPosViaElem(ep, EH_report_variables)>=0) {
         attr_mod_sub_list(alpp, new_host, EH_report_variables, STU_name, ep,
            sub_command, "report_variables", SGE_OBJ_EXECHOST, 0);
         /* JG: TODO: we have to check for valid centry names */
      }

   }

   DEXIT;
   return 0;

ERROR:
   DEXIT;
   return STATUS_EUNKNOWN;
}

int host_spool(
lList **alpp,
lListElem *ep,
gdi_object_t *object 
) {
   int pos;
   int dataType;
   const char *key;
   sge_object_type host_type = SGE_TYPE_ADMINHOST;

   DENTER(TOP_LAYER, "host_spool");

   pos = lGetPosViaElem(ep, object->key_nm );
   dataType = lGetPosType(lGetElemDescr(ep),pos);
   if (dataType == lHostT ) { 
      key = lGetHost(ep, object->key_nm);
   } else {
      key = lGetString(ep, object->key_nm);
   }
     
   switch (object->key_nm) {
      case AH_name:
         host_type = SGE_TYPE_ADMINHOST;
         break;
      case EH_name:
         host_type = SGE_TYPE_EXECHOST;
         break;
      case SH_name:
         host_type = SGE_TYPE_SUBMITHOST;
         break;
   }
     
   if (!spool_write_object(alpp, spool_get_default_context(), ep, key, host_type)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS, object->object_name, key));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return 1;
   }

   DEXIT;
   return 0;
}

int host_success(lListElem *ep, lListElem *old_ep, gdi_object_t *object) 
{
   lListElem* jatep;
   DENTER(TOP_LAYER, "host_success");

   switch(object->key_nm) {
      case EH_name:
      {
         lListElem *jep;
         const char *host = lGetHost(ep, EH_name);
         int slots, global_host = !strcmp("global", host);

         lSetList(ep, EH_consumable_actual_list, NULL);
         debit_host_consumable(NULL, ep, Master_CEntry_List, 0);
         for_each (jep, Master_Job_List) {
            slots = 0;
            for_each (jatep, lGetList(jep, JB_ja_tasks)) {
               slots += nslots_granted(lGetList(jatep, JAT_granted_destin_identifier_list), 
                  global_host?NULL:host);
            }
            if (slots)
               debit_host_consumable(jep, ep, Master_CEntry_List, slots);
         }

         sge_change_queue_version_exechost(host);
         sge_add_event(NULL, 0, old_ep?sgeE_EXECHOST_MOD:sgeE_EXECHOST_ADD, 
                       0, 0, host, NULL, NULL, ep);
         lListElem_clear_changed_info(ep);
      }
      break;

      case AH_name:
         sge_add_event(NULL, 0, old_ep?sgeE_ADMINHOST_MOD:sgeE_ADMINHOST_ADD, 
                       0, 0, lGetHost(ep, AH_name), NULL, NULL, ep);
         lListElem_clear_changed_info(ep);
      break;

      case SH_name:
         sge_add_event(NULL, 0, old_ep?sgeE_SUBMITHOST_MOD:sgeE_SUBMITHOST_ADD, 
                       0, 0, lGetHost(ep, SH_name), NULL, NULL, ep);
         lListElem_clear_changed_info(ep);
      break;
   }

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ */

void sge_mark_unheard(
lListElem *hep,
const char *target    /* prognames[QSTD|EXECD] */
) {
   const char *host;

   DENTER(TOP_LAYER, "sge_mark_unheard");

   host = lGetHost(hep, EH_name);

#ifdef ENABLE_NGC
   /* TODO: check this */
   CRITICAL((SGE_EVENT,"set_last_heard_from() not suppored by NGC"));
#else
   /* tell commlib, that this guys will vanish */
   if (target)
      set_last_heard_from(target, 1, host, 0);
   else {
      set_last_heard_from(prognames[EXECD], 1, host, 0);
      set_last_heard_from(prognames[QSTD], 1, host, 0);
   }
#endif

   host_trash_nonstatic_load_values(hep);
   cqueue_list_set_unknown_state(
         *(object_type_get_master_list(SGE_TYPE_CQUEUE)),
         host, true, true);

   DEXIT;
   return;
}

/* ----------------------------------------

   updates global and host specific load values
   using the load report list lp
*/
void sge_update_load_values(
char *rhost,
lList *lp 
) {
   u_long32 now;
   const char *report_host = NULL;
   lListElem *ep, **hepp = NULL;
   lListElem *lep;
   lListElem *global_ep = NULL, *host_ep = NULL;
   bool added_non_static = false, statics_changed = false;

   DENTER(TOP_LAYER, "sge_update_load_values");

   /* JG: TODO: this time should better come with the report.
    *           it is the time when the reported values were valid.
    */
   now = sge_get_gmt();

   /* loop over all received load values */
   for_each(ep, lp) {
      const char *name, *value, *host;
      u_long32 global, is_static;

      /* get name, value and other info */
      name = lGetString(ep, LR_name);
      value = lGetString(ep, LR_value);
      host  = lGetHost(ep, LR_host);
      global = lGetUlong(ep, LR_global);
      is_static = lGetUlong(ep, LR_static);

      /* erroneous load report */
      if (!name || !value || !host)
         continue;

      /* handle global or exec host? */
      if(global) {
         hepp = &global_ep;
      } else {
         hepp = &host_ep;
      }

#if 0
      /* AH: this section needs to be rewritten:
         - hepp must refer either to global_ep or host_ep
         - this code sends exec host modify events for each load value for simulated
      */
      /* we get load values for another host */
      if(*hepp && sge_hostcmp(lGetHost(*hepp, EH_name), host)) {
         /* output error from previous host, if any */
         if (report_host) {
            INFO((SGE_EVENT, MSG_CANT_ASSOCIATE_LOAD_SS, rhost, report_host));
         }
         /*
         ** if static load values (eg arch) have changed
         ** then spool
         */
         if (statics_changed && host_ep) {
            write_host(1, 2, host_ep, EH_name, NULL);
         }

         /* if non static load values arrived, this indicates that 
         ** host is not unknown 
         */
         if (added_non_static) {
            lListElem *qep;
            u_long32 state;
            const char* tmp_hostname;


            tmp_hostname = lGetHost(host_ep, EH_name);
            cqueue_list_set_unknown_state(
                  *(object_type_get_master_list(SGE_TYPE_CQUEUE)),
                  tmp_hostname, true, false);
         }

         sge_add_event(NULL, 0, sgeE_EXECHOST_MOD, 0, 0, lGetHost(*hepp, EH_name), NULL, *hepp);

         added_non_static = false;
         statics_changed = false;
         *hepp = NULL;
         report_host = NULL;
      }
#endif

      /* update load value list of rhost */
      if ( !*hepp) {
         *hepp = host_list_locate(Master_Exechost_List, host);
         if (!*hepp) {
            if (!global) {
               report_host = lGetHost(ep, LR_host); /* this is our error indicator */
            }
            DPRINTF(("got load value for UNKNOWN host "SFQ"\n", 
                      lGetHost(ep, LR_host)));
            continue;
         }
      } 

      /* replace old load value or add a new one */
      lep = lGetSubCaseStr(*hepp, HL_name, name, EH_load_list);  

      if (!lep) {
         lep = lAddSubStr(*hepp, HL_name, name, EH_load_list, HL_Type);
         DPRINTF(("%s: adding load value: "SFQ" = "SFQ"\n", 
               lGetHost(ep, LR_host), name, value));

         if (is_static) {
            statics_changed = true;
         } else {
            if (!global)
               added_non_static = true; /* triggers clearing of unknown state */
         }
      }
      else {
         const char *oldval;

         oldval = lGetString(lep, HL_value);
         if (sge_is_static_load_value(name) && 
             (oldval != value) && (!oldval || strcmp(value, oldval))) {
            statics_changed = true;

            DPRINTF(("%s: updating STATIC lv: "SFQ" = "SFQ" oldval: "SFQ"\n", 
                    lGetHost(ep, LR_host), name, value, oldval));
         }
      }
      /* copy value */
      lSetString(lep, HL_value, value); 
      lSetUlong(lep, HL_last_update, now);
   }

   /* output error from previous host, if any */
   if (report_host) {
      INFO((SGE_EVENT, MSG_CANT_ASSOCIATE_LOAD_SS, rhost, report_host));
   }

   /* if non static load values arrived, this indicates that 
   ** host is not unknown 
   */
   if (added_non_static) {
      const char* tmp_hostname;

      tmp_hostname = lGetHost(host_ep, EH_name);
      cqueue_list_set_unknown_state(
         *(object_type_get_master_list(SGE_TYPE_CQUEUE)),
         tmp_hostname, true, false);
   }

   if (global_ep) {
      lList *answer_list = NULL;
      sge_event_spool(&answer_list, 0, sgeE_EXECHOST_MOD, 
                      0, 0, SGE_GLOBAL_NAME, NULL, NULL,
                      global_ep, NULL, NULL, true, false);
      answer_list_output(&answer_list);
      reporting_create_host_record(&answer_list, global_ep, now);
      answer_list_output(&answer_list);
   }

   /*
   ** if static load values (eg arch) have changed
   ** then spool
   */
   if (host_ep) {
      lList *answer_list = NULL;
      sge_event_spool(&answer_list, 0, sgeE_EXECHOST_MOD, 
                      0, 0, lGetHost(host_ep, EH_name), NULL, NULL,
                      host_ep, NULL, NULL, true, statics_changed);
      answer_list_output(&answer_list);
      reporting_create_host_record(&answer_list, host_ep, now);
      answer_list_output(&answer_list);
   }

   DEXIT;
   return;
}

/* ----------------------------------------

   trash old load values 
   
*/
void sge_load_value_garbage_collector(
u_long32 now 
) {
   extern int new_config;
   lListElem *hep, *ep, *nextep; 
   lList *h_list;
   const char *host;
   int host_unheard;
   u_short id = 1;
   const char *comproc;
   u_long32 timeout; 
   int nstatics, nbefore;
   static u_long32 next_garbage_collection = 0;
   lListElem *global_host_elem   = NULL;
   lListElem *template_host_elem = NULL;
#ifdef ENABLE_NGC
   unsigned long last_heard_from;
#endif

   const void *iterator = NULL;


   DENTER(TOP_LAYER, "sge_load_value_garbage_collector");

   if (next_garbage_collection && next_garbage_collection > now) {
      DEXIT;
      return;
   }

   
   next_garbage_collection = now + 15; 

   comproc = prognames[EXECD];
   /* get "global" element pointer */
   global_host_elem   = host_list_locate(Master_Exechost_List, SGE_GLOBAL_NAME);    
   /* get "template" element pointer */
   template_host_elem = host_list_locate(Master_Exechost_List, SGE_TEMPLATE_NAME); 
   /* take each host including the "global" host */
   for_each(hep, Master_Exechost_List) {   
      if (hep == template_host_elem)
         continue;

      host = lGetHost(hep, EH_name);

      /* do not trash load values of simulated hosts */
      if(simulate_hosts == 1) {
         const lListElem *simhost = lGetSubStr(hep, CE_name, "simhost", EH_consumable_config_list);
         if(simhost != NULL) {
            const char *real_host = lGetString(simhost, CE_stringval);
            if(real_host != NULL && sge_hostcmp(real_host, host) != 0) {
               DPRINTF(("skip trashing load values for host %s simulated by %s\n", host, real_host));
               continue;
            }
         }
      }

      timeout = MAX(load_report_interval(hep)*3, conf.max_unheard); 
#ifdef ENABLE_NGC
      cl_commlib_get_last_message_time((cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name() ,0)),
                                        (char*)host, (char*)comproc,id, &last_heard_from);
      if ( (hep != global_host_elem )  && (now > last_heard_from + timeout))
#else
      if ( (hep != global_host_elem )  && (now > last_heard_from(comproc, &id, host) + timeout)) 
#endif
      {
         host_unheard = 1;
#if 0
         DPRINTF(("id = %d, comproc = %s, host = %s, timeout = "u32", "
               "now = "u32", last_heard = "u32"\n", id, comproc, host, 
               u32c(timeout), u32c(now), 
               u32c(last_heard_from(comproc, &id, host))));
#endif
      } else {
         host_unheard = 0;
      }

      /* take each load value */
      nstatics = 0;
      nbefore = lGetNumberOfElem(lGetList(hep, EH_load_list));     

      ep=lFirst(lGetList(hep, EH_load_list));
      while (ep) {
         nextep=lNext(ep);

         if (sge_is_static_load_value(lGetString(ep, HL_name))) {
            nstatics++;
         } else {
            if ((lGetUlong(ep, HL_last_update)+timeout<now || host_unheard)) {
               DPRINTF(("host %s: trashing load value "SFQ": %s\n", 
                        host, 
                        lGetString(ep, HL_name),
                        (lGetUlong(ep, HL_last_update)+timeout < now)? "HL_last_update":"last_heard_from"));
              lDelSubStr(hep, HL_name, lGetString(ep, HL_name), EH_load_list);
            }  
         }
         ep = nextep;
      }
      h_list   = lGetList(hep, EH_load_list);
      if ( (nstatics == lGetNumberOfElem(h_list)) &&
           (nbefore   > lGetNumberOfElem(h_list))    ) {
         lListElem *cqueue;

         /* load reports were trashed and only the 
            static load values remained: 
            set all queues residing at this host in unknown state */
         for_each(cqueue, *(object_type_get_master_list(SGE_TYPE_CQUEUE))) {
            lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
            lListElem *qinstance;

            qinstance = lGetElemHostFirst(qinstance_list, QU_qhostname, 
                                          host, &iterator);
            while (qinstance != NULL) {
               qinstance_state_set_unknown(qinstance, true);
               qinstance_add_event(qinstance, sgeE_QINSTANCE_MOD);

               DPRINTF(("%s: trashed all (%d) non-static load values -> unknown\n", 
                        lGetString(qinstance, QU_qname), 
                        nbefore - lGetNumberOfElem(lGetList(hep, EH_load_list))));

               /* initiate timer for this host because they turn into 'unknown' state */
               reschedule_unknown_trigger(hep); 
               qinstance = lGetElemHostNext(qinstance_list, QU_qhostname, 
                                            host, &iterator); 
            }
         }
      } 
   }

   new_config = 0;

   DEXIT;
   return;
}

u_long32 load_report_interval(
lListElem *hep 
) {
   extern int new_config;
   u_long32 timeout; 
   lListElem *cfep;
   const char *host;

   DENTER(TOP_LAYER, "load_report_interval");

   host = lGetHost(hep, EH_name);

   /* cache load report interval in exec host to 
      prevent host name resolving each epoch */
   if (new_config || !lGetUlong(hep, EH_load_report_interval)) {
      /* timeout may depend on the host */
      if (!(cfep = get_local_conf_val(host, "load_report_time"))
            || (cfep && !parse_ulong_val(NULL, &timeout, TYPE_TIM, 
               lGetString(cfep, CF_value), NULL, 0))) {
         ERROR((SGE_EVENT, MSG_OBJ_LOADREPORTIVAL_SS,
               host, lGetString(cfep, CF_value)));
         timeout = 120;
      }
      DPRINTF(("load value timeout for host %s is "u32"\n", 
                  host, timeout)); 
      lSetUlong(hep, EH_load_report_interval, timeout);
   } else { 
      timeout = lGetUlong(hep, EH_load_report_interval);
   } 
   DEXIT; 
   return timeout;
}

lListElem *get_local_conf_val(
const char *host,
const char *name 
) {
   lListElem *cfep, *ep = NULL;

   DENTER(TOP_LAYER, "get_local_conf_val");

   /* try to find load_report_time for this host in the local configuration */
   if (!select_configuration(host, Master_Config_List, &ep) && 
       (cfep=lGetSubStr(ep, CF_name, name, CONF_entries))) {
      DEXIT;
      return cfep;
   }

   /* no success - take global configuration */
   ep = NULL;
   if (!select_configuration("global", Master_Config_List, &ep) && 
       (cfep=lGetSubStr(ep, CF_name, name, CONF_entries))) {
      DEXIT;
      return cfep;
   }

   DEXIT;
   return NULL;
}

void 
sge_change_queue_version_exechost(const char *exechost_name) 
{
   lListElem *cqueue = NULL; 
   bool change_all = (strcasecmp(exechost_name, SGE_GLOBAL_NAME) == 0);

   DENTER(TOP_LAYER, "sge_change_queue_version_exechost");

   for_each(cqueue, *(object_type_get_master_list(SGE_TYPE_CQUEUE))) {
      lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
      lListElem *qinstance = NULL;
      lListElem *next_qinstance = NULL;
      const void *iterator = NULL;

      if (change_all) {
         next_qinstance = lFirst(qinstance_list);
      } else {
         next_qinstance = lGetElemHostFirst(qinstance_list, QU_qhostname, 
                                            exechost_name, &iterator);
      }
      while ((qinstance = next_qinstance)) {
         const char *name = NULL;
         lList *answer_list = NULL;

         if (change_all) {
            next_qinstance = lNext(qinstance);
            name = SGE_GLOBAL_NAME;
         } else {
            next_qinstance = lGetElemHostNext(qinstance_list, QU_qhostname, 
                                              exechost_name, &iterator); 
            name = exechost_name;
         }
         DPRINTF((SFQ" has changed. Increasing qversion of"SFQ"\n",
                  name, lGetString(qinstance, QU_full_name)));
         qinstance_increase_qversion(qinstance);
         sge_event_spool(&answer_list, 0, sgeE_QINSTANCE_MOD, 
                         0, 0, lGetString(qinstance, QU_qname), 
                         lGetHost(qinstance, QU_qhostname), NULL,
                         qinstance, NULL, NULL, false, true);
         answer_list_output(&answer_list);
      }
   }

   DEXIT;
   return;
}

/****
 **** sge_gdi_kill_exechost
 ****
 **** prepares the killing of an exechost (or all).
 **** Actutally only the permission is checked here
 **** and master_kill_execds is called to do the work.
 ****/
void sge_gdi_kill_exechost(char *host, 
                           sge_gdi_request *request, 
                           sge_gdi_request *answer)
{
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];

   DENTER(GDI_LAYER, "sge_gdi_kill_exechost");

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, 
                      ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (!manop_is_manager(user)) {
      ERROR((SGE_EVENT, MSG_OBJ_SHUTDOWNPERMS)); 
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, 
                      ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   master_kill_execds(request, answer);
   DEXIT;
}

/******************************************************************
   We have to tell execd(s) to terminate. 

   request->lp is a lList of Type ID_Type.
      If the first ID_str is NULL, we have to kill all execd's.
      Otherwise, every ID_str describes an execd to kill.
      If ID_force is 1, we have to kill jobs, too.
      If ID_force is 0, we don't kill jobs.
 *******************************************************************/
static void master_kill_execds(
sge_gdi_request *request,
sge_gdi_request *answer 
) {
   int kill_jobs;
   lListElem *lel, *rep;
   char host[MAXHOSTLEN];
   const char *hostname;

   DENTER(TOP_LAYER, "master_kill_execds");

   if (lGetString(lFirst(request->lp), ID_str) == NULL) {
      /* this means, we have to kill every execd. */

      kill_jobs = lGetUlong(lFirst(request->lp), ID_force)?1:0;
      /* walk over exechost list and send every exechosts execd a
         notification */
      for_each(lel, Master_Exechost_List) {  
         hostname = lGetHost(lel, EH_name);
         if (strcmp(hostname, "template") && strcmp(hostname, "global")) {
            notify(lel, answer, kill_jobs, 0); 

            /* RU: */
            /* initiate timer for this host which turns into unknown state */
            reschedule_unknown_trigger(lel);
         }
      }
      if(lGetNumberOfElem(answer->alp) == 0) {
         /* no exechosts have been killed */
         DPRINTF((MSG_SGETEXT_NOEXECHOSTS));
         INFO((SGE_EVENT, MSG_SGETEXT_NOEXECHOSTS));
         answer_list_add(&(answer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
      }
   } else {
      /* only specified exechosts should be killed. */
      
      /* walk over list with execd's to kill */
      for_each(rep, request->lp) {
#ifdef ENABLE_NGC
         if ((getuniquehostname(lGetString(rep, ID_str), host, 0)) != CL_RETVAL_OK)
#else
         if ((getuniquehostname(lGetString(rep, ID_str), host, 0)) != CL_OK)
#endif
         {
            WARNING((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetString(rep, ID_str)));
            answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_WARNING);
         } else {
            if ((lel = host_list_locate(Master_Exechost_List, host))) {
               kill_jobs = lGetUlong(rep, ID_force)?1:0;
               /*
               ** if a host name is given, then a kill is forced
               ** this means that even if the host is unheard we try
               ** to kill it
               */
               notify(lel, answer, kill_jobs, 1);
               /* RU: */
               /* initiate timer for this host which turns into unknown state */ 
               reschedule_unknown_trigger(lel);
            } else {
               WARNING((SGE_EVENT, MSG_SGETEXT_ISNOEXECHOST_S, host));
               answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_WARNING);
            }
         }
      }
   }
   DEXIT;
}


/********************************************************************
 Notify execd on a host to shutdown
 ********************************************************************/
static void notify(
lListElem *lel,
sge_gdi_request *answer,
int kill_jobs,
int force 
) {
   const char *hostname;
   u_long execd_alive;
   const char *action_str;
   u_long32 state;
   lListElem *jep;
   lList *mail_users, *gdil;
   int mail_options;
   char sge_mail_subj[1024];
   char sge_mail_body[1024];
#ifdef ENABLE_NGC
   unsigned long last_heard_from;
#else
   static u_short number_one = 1;
#endif


   DENTER(TOP_LAYER, "notify");

   action_str = kill_jobs ? MSG_NOTIFY_SHUTDOWNANDKILL:MSG_NOTIFY_SHUTDOWN;

   hostname = lGetHost(lel, EH_name);

#ifdef ENABLE_NGC
   cl_commlib_get_last_message_time((cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name() ,0)),
                                        (char*)hostname, (char*)prognames[EXECD],1, &last_heard_from);
   execd_alive = last_heard_from;
#else
   execd_alive = last_heard_from(prognames[EXECD], &number_one, hostname);
#endif

   if (!force && !execd_alive) {
      WARNING((SGE_EVENT, MSG_OBJ_NOEXECDONHOST_S, hostname));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ESEMANTIC, 
                     ANSWER_QUALITY_WARNING);
   }
   if (execd_alive || force) {
      if (host_notify_about_kill(lel, kill_jobs)) {
         INFO((SGE_EVENT, MSG_COM_NONOTIFICATION_SSS, action_str, 
               (execd_alive ? "" : MSG_OBJ_UNKNOWN), hostname));
      } else {
         INFO((SGE_EVENT, MSG_COM_NOTIFICATION_SSS, action_str, 
               (execd_alive ? "" : MSG_OBJ_UNKNOWN), hostname));
      }
      DPRINTF((SGE_EVENT));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   }

   if(kill_jobs) {
      /* mark killed jobs as deleted */
      for_each(jep, Master_Job_List) {   
         lListElem *jatep;
         mail_users = NULL;
         mail_options = 0;

         for_each(jatep, lGetList(jep, JB_ja_tasks)) {
            gdil = lGetList(jatep, JAT_granted_destin_identifier_list);
            if(gdil) {
               if(!(sge_hostcmp(lGetHost(lFirst(gdil), JG_qhostname), hostname))) {
                  /*   send mail to users if requested                  */
                  if (mail_users == NULL) {
                     mail_users = lGetList(jep, JB_mail_list);
                  }
                  if (mail_options == 0) {
                     mail_options = lGetUlong(jep, JB_mail_options);
                  }
                  
                  if (VALID(MAIL_AT_ABORT, mail_options) && !(lGetUlong(jatep, JAT_state) & JDELETED)) {
                     lUlong jep_JB_job_number;
                     const char* jep_JB_job_name;

                     jep_JB_job_number = lGetUlong(jep, JB_job_number);
                     jep_JB_job_name   = lGetString(jep, JB_job_name);

                     sprintf(sge_mail_subj, MSG_MAIL_JOBKILLEDSUBJ_US, 
                             u32c(jep_JB_job_number), 
                             jep_JB_job_name);
                     sprintf(sge_mail_body, MSG_MAIL_JOBKILLEDBODY_USS, 
                             u32c(jep_JB_job_number), 
                             jep_JB_job_name, 
                             hostname);
                     cull_mail(mail_users, sge_mail_subj, sge_mail_body, "job abortion");
                  }
    
                  /* this job has the killed exechost as master host */
                  state = lGetUlong(jatep, JAT_state);
                  SETBIT(JDELETED, state);
                  lSetUlong(jatep, JAT_state, state);
                  /* spool job */
                  {
                     lList *answer_list = NULL;
                     dstring buffer = DSTRING_INIT;
                     spool_write_object(&answer_list, 
                                        spool_get_default_context(), jep,
                                        job_get_key(lGetUlong(jep, JB_job_number), 
                                            lGetUlong(jatep, JAT_task_number), 
                                            NULL, &buffer), 
                                        SGE_TYPE_JOB);
                     lListElem_clear_changed_info(jatep);
                     /* JG: TODO: don't we have to send an event? */
                     answer_list_output(&answer_list);
                     sge_dstring_free(&buffer);
                  }
               }
            }
         }
      }
   }

   sge_mark_unheard(lel, NULL); /* for both execd */

   DEXIT;
   return;
}


/****
 **** sge_execd_startedup
 ****
 **** gdi call for old request starting_up.
 ****/
int sge_execd_startedup(
lListElem *host,
lList **alpp,
char *ruser,
char *rhost,
u_long32 target) {
   lListElem *hep, *cqueue;
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "sge_execd_startedup");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if( !host || !ruser || !rhost) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }
   
   hep = host_list_locate(Master_Exechost_List, rhost);
   if(!hep) {
      if (sge_add_host_of_type(rhost, SGE_EXECHOST_LIST) < 0) {
         ERROR((SGE_EVENT, MSG_OBJ_INVALIDHOST_S, rhost));
         answer_list_add(alpp, SGE_EVENT, STATUS_DENIED, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_DENIED;
      } 
   }

   hep = host_list_locate(Master_Exechost_List, rhost);
   if(!hep) {
      ERROR((SGE_EVENT, MSG_OBJ_NOADDHOST_S, rhost));
      answer_list_add(alpp, SGE_EVENT, STATUS_DENIED, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_DENIED;
   }
   lSetUlong(hep, EH_featureset_id, lGetUlong(host, EH_featureset_id));
   lSetUlong(hep, EH_report_seqno, 0);

   /*
    * reinit state of all qinstances at this host according to initial_state
    */
   for_each (cqueue, *(object_type_get_master_list(SGE_TYPE_CQUEUE))) {
      lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
      lListElem *qinstance = NULL;
      const void *iterator = NULL;

      qinstance = lGetElemHostFirst(qinstance_list, QU_qhostname, 
                                    rhost, &iterator);
      while (qinstance != NULL) {
         bool state_changed = qinstance_set_initial_state(qinstance);

         if (state_changed) {
            lList *answer_list = NULL;

            qinstance_increase_qversion(qinstance);
            sge_event_spool(&answer_list, 0, sgeE_QINSTANCE_MOD, 
                            0, 0, lGetString(qinstance, QU_qname), 
                            lGetHost(qinstance, QU_qhostname), NULL,
                            qinstance, NULL, NULL, false, true);
            answer_list_output(&answer_list); 
         }
         qinstance = lGetElemHostNext(qinstance_list, QU_qhostname,
                                      rhost, &iterator);
      }
   }
   
   DPRINTF(("=====>STARTING_UP: %s %s on >%s< is starting up\n", 
      feature_get_product_name(FS_SHORT_VERSION, &ds), "execd", rhost));

   /*
   ** loop over pseudo hosts and set EH_startup flag
   */
   lSetUlong(hep, EH_startup, 1);
   sge_add_event(NULL, 0, sgeE_EXECHOST_MOD, 0, 0, rhost, NULL, NULL, hep);
   lListElem_clear_changed_info(hep);

   INFO((SGE_EVENT, MSG_LOG_REGISTER_SS, "execd", rhost));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_ERROR);

   DEXIT;
   return STATUS_OK;
}


static int verify_scaling_list(lList **answer_list, lListElem *host) 
{
   bool ret = true;
   lListElem *hs_elem;

   DENTER(TOP_LAYER, "verify_scaling_list");
   for_each (hs_elem, lGetList(host, EH_scaling_list)) {
      const char *name = lGetString(hs_elem, HS_name);
      lListElem *centry = centry_list_locate(Master_CEntry_List, name);
   
      if (centry == NULL) {
         const char *hname = lGetHost(host, EH_name);

         ERROR((SGE_EVENT, MSG_OBJ_NOSCALING4HOST_SS, name, hname));
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         ret = false;
         break;
      }
   }
   DEXIT;
   return ret ? STATUS_OK : STATUS_EUNKNOWN;
}
