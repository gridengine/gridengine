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

#include "def.h"
#include "sge.h"
#include "sge_conf.h"
#include "symbols.h"
#include "sge_prognames.h"
#include "sge_time.h"
#include "sge_me.h"
#include "sge_feature.h"
#include "sge_hostL.h"
#include "sge_answerL.h"
#include "sge_load_reportL.h"
#include "sge_eventL.h"
#include "sge_usersetL.h"
#include "sge_queueL.h"
#include "sge_identL.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_userprjL.h"
#include "commlib.h"
#include "sge_host.h"
#include "read_write_host.h"
#include "sge_host_qmaster.h"
#include "sge_queue_qmaster.h"
#include "gdi_utility_qmaster.h"
#include "sge_m_event.h"
#include "sge_static_load.h"
#include "complex_history.h"
#include "opt_history.h"
#include "sec.h"
#include "read_write_job.h"
#include "read_write_queue.h"
#include "sge_job_schedd.h"
#include "sge_c_gdi.h"
#include "mail.h"
#include "sgermon.h"
#include "sge_log.h"
#include "resolve_host.h"
#include "sge_parse_num_par.h"
#include "configuration_qmaster.h"
#include "utility.h"
#include "sge_qmod_qmaster.h"
#include "sort_hosts.h"
#include "sge_userset_qmaster.h"
#include "sge_userprj_qmaster.h"
#include "time_event.h"
#include "sge_complexL.h"
#include "sge_complex_schedd.h"
#include "reschedule.h"
#include "sge_string.h"
#include "sge_security.h"
#include "msg_common.h"
#include "msg_utilib.h"
#include "msg_qmaster.h"

extern lList *Master_Queue_List;
extern lList *Master_Job_List;
extern lList *Master_Exechost_List;
extern lList *Master_Adminhost_List;
extern lList *Master_Submithost_List;
extern lList *Master_Config_List;
extern lList *Master_Complex_List;
extern lList *Master_Project_List;

static int sge_has_active_queue(char *uniquie);

static void master_kill_execds(sge_gdi_request *request, sge_gdi_request *answer);

static int notify_kill_job(lListElem *lel, int kill_jobs, const char *target);

static void notify(lListElem *lel, sge_gdi_request *answer, int kill_jobs, int force);

static int verify_scaling_list(lList **alpp, lListElem *hep); 

static int sge_unlink_object(lListElem *ep, int nm);

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
   ret = sge_gdi_add_mod_generic(NULL, ep, 1, object, me.user_name, 
      me.qualified_hostname, 0);
   lFreeElem(ep);       

   DEXIT;
   return (ret == STATUS_OK) ? 0 : -1;
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
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
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
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   host = lGetPosHost(hep, pos);
   if (!host) {
      ERROR((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* check if host is in host list */
   found_host = 1;
   if ((ep=sge_locate_host(host, target))==NULL) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, name, host));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      found_host = 0;
   }

   if (found_host) {
      strcpy(unique, host);     /* no need to make unique anymore */
   }
   else {
      /* may be host was not the unique hostname.
         Get the unique hostname and try to find it again. */
      if (getuniquehostname(host, unique, 0)!=CL_OK) {
         ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, host));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
      /* again check if host is in host list. This time use the unique
         hostname */
      if ((ep=sge_locate_host(unique, target))==NULL) {
         ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, name, host));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
         DEXIT;
         return STATUS_EEXIST;
      }
   }

   /* 
      check if someone tries to delete 
      the qmaster host from admin host list
   */
   if (target==SGE_ADMINHOST_LIST && 
         !hostcmp(unique, me.qualified_hostname)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTDELADMINQMASTER_S, 
          me.qualified_hostname));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   if (target==SGE_EXECHOST_LIST && sge_has_active_queue(unique)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTDELEXECACTIVQ_S, unique));
      sge_add_answer(alpp, SGE_EVENT, STATUS_ESEMANTIC, 0);
      DEXIT;
      return STATUS_ESEMANTIC;
   }

   if (target==SGE_EXECHOST_LIST && !strcasecmp(unique, "global")) {
      ERROR((SGE_EVENT, MSG_OBJ_DELGLOBALHOST));
      sge_add_answer(alpp, SGE_EVENT, STATUS_ESEMANTIC, 0);
      DEXIT;
      return STATUS_ESEMANTIC;
   }

   /* remove host file */
   sge_unlink_object(ep, nm);

   /* send event */
   switch(target) {
      case SGE_ADMINHOST_LIST:
         sge_add_event(NULL, sgeE_ADMINHOST_DEL, 0, 0, host, NULL);
         break;
      case SGE_EXECHOST_LIST:
         sge_add_event(NULL, sgeE_EXECHOST_DEL, 0, 0, host, NULL);
         break;
      case SGE_SUBMITHOST_LIST:
         sge_add_event(NULL, sgeE_SUBMITHOST_DEL, 0, 0, host, NULL);
         break;
   }

   /* delete found host element */
   lRemoveElem(*host_list, ep);

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS, 
         ruser, rhost, unique, name));
   sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   DEXIT;
   return STATUS_OK;
}

/* ------------------------------------------------------------ */

int host_mod(
lList **alpp,
lListElem *new_host,
lListElem *ep,
int add,
char *ruser,
char *rhost,
gdi_object_t *object,
int sub_command 
) {
   const char *host;
   int nm;
   int pos;
   int dataType;

   DENTER(TOP_LAYER, "host_mod");

   nm = object->key_nm;

#ifdef PW
   /* license checking when adding submit hosts */
   if (add && nm == SH_name) {
      int ret;
      extern u_long32 pw_num_submit;

      if ((ret=sge_count_uniq_hosts(Master_Adminhost_List,
            Master_Submithost_List)) < 0) {
         /* s.th.'s wrong, but we can't blame it on the user so we
          * keep truckin'
          */
         ERROR((SGE_EVENT, MSG_SGETEXT_CANTCOUNT_HOSTS_S, SGE_FUNC));
      } else {
         if (pw_num_submit < ret+1) {
            /* we've a license violation */
            ERROR((SGE_EVENT, MSG_SGETEXT_TOOFEWSUBMHLIC_II, (int) pw_num_submit, ret+1));
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESUBHLIC, 0);
            DEXIT;
            return STATUS_ESUBHLIC;
         }
      }
   }
#endif   

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
      /* ---- EH_complex_list */
      if (lGetPosViaElem(ep, EH_complex_list)>=0) {
         DPRINTF(("got new EH_complex_list\n"));
         /* check complex list */
         if (verify_complex_list(alpp, object->object_name, host, 
               lGetList(ep, EH_complex_list))!=STATUS_OK)
            goto ERROR;

         attr_mod_sub_list(alpp, new_host, EH_complex_list, CX_name, ep, 
            sub_command, SGE_ATTR_COMPLEX_LIST, SGE_OBJ_EXECHOST, 0);
      }

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
         if (verify_acl_list(alpp, lGetList(ep, EH_acl), "user_lists",
                  object->object_name, host)!=STATUS_OK)
            goto ERROR;
         attr_mod_sub_list(alpp, new_host, EH_acl, US_name, ep,
            sub_command, SGE_ATTR_USER_LISTS, SGE_OBJ_EXECHOST, 0);
      }

      /* ---- EH_xacl */
      if (lGetPosViaElem(ep, EH_xacl)>=0) {
         DPRINTF(("got new EH_xacl\n"));
         /* check xacl list */
         if (verify_acl_list(alpp, lGetList(ep, EH_xacl), "xuser_lists",
                  object->object_name, host)!=STATUS_OK)
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
   DENTER(TOP_LAYER, "host_spool");

   if (!write_host(1, 2, ep, object->key_nm, NULL)) {
      pos = lGetPosViaElem(ep, object->key_nm );
      dataType = lGetPosType(lGetElemDescr(ep),pos);
      if (dataType != lHostT ) { 
         ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS, object->object_name, lGetString(ep, object->key_nm)));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      } else {
         ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS, object->object_name, lGetHost(ep, object->key_nm)));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      }
      DEXIT;
      return 1;
   }
   DEXIT;
   return 0;
}

int host_success(
lListElem *ep,
lListElem *old_ep,
gdi_object_t *object 
) {
   lListElem* jatep;
   DENTER(TOP_LAYER, "host_success");

   switch(object->key_nm) {
      case EH_name:
      {
         lListElem *jep;
         const char *host = lGetHost(ep, EH_name);
         int slots, global_host = !strcmp("global", host);

         lSetList(ep, EH_consumable_actual_list, NULL);
         debit_host_consumable(NULL, ep, Master_Complex_List, 0);
         for_each (jep, Master_Job_List) {
            slots = 0;
            for_each (jatep, lGetList(jep, JB_ja_tasks)) {
               slots += nslots_granted(lGetList(jatep, JAT_granted_destin_identifier_list), 
                  global_host?NULL:host);
            }
            if (slots)
               debit_host_consumable(jep, ep, Master_Complex_List, slots);
         }

         sge_change_queue_version_exechost(host);
         sge_add_event(NULL, old_ep?sgeE_EXECHOST_MOD:sgeE_EXECHOST_ADD, 0, 0, host, ep);
         if (!is_nohist())
            write_host_history(ep);

      }
      break;

      case AH_name:
         sge_add_event(NULL, old_ep?sgeE_ADMINHOST_MOD:sgeE_ADMINHOST_ADD, 0, 0, lGetHost(ep, AH_name), ep);
      break;

      case SH_name:
         sge_add_event(NULL, old_ep?sgeE_SUBMITHOST_MOD:sgeE_SUBMITHOST_ADD, 0, 0, lGetHost(ep, SH_name), ep);
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
   lListElem *qep;
   const char *host;
   u_long32 state;
   const void *iterator = NULL;


   DENTER(TOP_LAYER, "sge_mark_unheard");

   host = lGetHost(hep, EH_name);

   /* tell commlib, that this guys will vanish */
   if (target)
      set_last_heard_from(target, 1, host, 0);
   else {
      set_last_heard_from(prognames[EXECD], 1, host, 0);
      set_last_heard_from(prognames[QSTD], 1, host, 0);
   }

   /* trash old load values */
   {
      lListElem *nextep, *ep = lFirst(lGetList(hep, EH_load_list));
      while (ep) {
         nextep = lNext(ep);
         if (!sge_is_static_load_value(lGetString(ep, HL_name)))
            lDelSubStr(hep, HL_name, lGetString(ep, HL_name), EH_load_list);
         ep = nextep;
      }
   }    



   /* set all queues residing at this host to QUNKNOWN */
   qep = lGetElemHostFirst(Master_Queue_List, QU_qhostname, host, &iterator); 
   while (qep != NULL) {
      if ( !ISSET(lGetUlong(qep, QU_state), QUNKNOWN) ) {
         state = lGetUlong(qep, QU_state);
         SETBIT(QUNKNOWN, state);
         lSetUlong(qep, QU_state, state);
         sge_add_queue_event(sgeE_QUEUE_MOD, qep);
      }
      qep = lGetElemHostNext(Master_Queue_List, QU_qhostname, host, &iterator); 
   }

   DEXIT;
   return;
}


/* ------------------------------------------------------------ */

static int sge_has_active_queue(
char *unique 
) {
   DENTER(TOP_LAYER, "sge_has_active_queue");


   if (lGetElemHost(Master_Queue_List, QU_qhostname, unique) != NULL) {
      DEXIT;
      return 1;
   }

   DEXIT;
   return 0;
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
   int added_non_static = 0, statics_changed = 0;
   const void *iterator = NULL;


   DENTER(TOP_LAYER, "sge_update_load_values");
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

      /* we get load values for another host */
      if(*hepp && hostcmp(lGetHost(*hepp, EH_name), host)) {
         /* output error from previous host, if any */
         if (report_host) {
            INFO((SGE_EVENT, MSG_CANT_ASSOCIATE_LOAD_SS, rhost, report_host));
         }
         /*
         ** if static load values (eg arch) have changed
         ** then spool and write history
         */
         if (statics_changed && host_ep) {
            write_host(1, 2, host_ep, EH_name, NULL);
            if (!is_nohist()) {
               write_host_history(host_ep);
            }
         }

         /* if non static load values arrived, this indicates that 
         ** host is not unknown - unset QUNKNOWN bit for all queues
         ** on this host 
         */
         if (added_non_static) {
            lListElem *qep;
            u_long32 state;
            const char* tmp_hostname;


            tmp_hostname = lGetHost(host_ep, EH_name);
            qep = lGetElemHostFirst(Master_Queue_List, QU_qhostname, tmp_hostname , &iterator); 
            while (qep != NULL) {
               state = lGetUlong(qep, QU_state);
               CLEARBIT(QUNKNOWN, state);
               lSetUlong(qep, QU_state, state);
               sge_add_queue_event(sgeE_QUEUE_MOD, qep);
               qep = lGetElemHostNext(Master_Queue_List, QU_qhostname, tmp_hostname , &iterator); 
            }
         }

         sge_add_event(NULL, sgeE_EXECHOST_MOD, 0, 0, host, *hepp);

         added_non_static = 0;
         statics_changed = 0;
         *hepp = NULL;
         report_host = NULL;
      }

      /* update load value list of rhost */
      if ( !*hepp) {
         *hepp = sge_locate_host(host, SGE_EXECHOST_LIST);
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
            if (!is_nohist() )
               statics_changed = 1;
         } else {
            if (!global)
               added_non_static = 1; /* triggers clearing of unknown state */
         }
      }
      else {
         const char *oldval;

         oldval = lGetString(lep, HL_value);
         if (!is_nohist() && sge_is_static_load_value(name) && 
             (oldval != value) && (!oldval || strcmp(value, oldval))) {
            statics_changed = 1;

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

   /*
   ** if static load values (eg arch) have changed
   ** then spool and write history
   */
   if (statics_changed && host_ep) {
      write_host(1, 2, host_ep, EH_name, NULL);
      if (!is_nohist()) {
         write_host_history(host_ep);
      }
   }

   /* if non static load values arrived, this indicates that 
   ** host is not unknown - unset QUNKNOWN bit for all queues
   ** on this host 
   */
   if (added_non_static) {
      lListElem *qep;
      u_long32 state;
      const char* tmp_hostname;


      tmp_hostname = lGetHost(host_ep, EH_name);
      qep = lGetElemHostFirst(Master_Queue_List, QU_qhostname, tmp_hostname , &iterator); 
      while (qep != NULL) {
         state = lGetUlong(qep, QU_state);
         CLEARBIT(QUNKNOWN, state);
         lSetUlong(qep, QU_state, state);
         sge_add_queue_event(sgeE_QUEUE_MOD, qep);
         qep = lGetElemHostNext(Master_Queue_List, QU_qhostname, tmp_hostname , &iterator); 
      }
   }

   if(*hepp) {
      sge_add_event(NULL, sgeE_EXECHOST_MOD, 0, 0, lGetHost(*hepp, EH_name), *hepp);
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
   lListElem *qep, *hep, *ep, *nextep; 
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

   const void *iterator = NULL;


   DENTER(TOP_LAYER, "sge_load_value_garbage_collector");

   if (next_garbage_collection && next_garbage_collection > now) {
      DEXIT;
      return;
   }

   
   next_garbage_collection = now + 15; 

   comproc = prognames[EXECD];
   /* get "global" element pointer */
   global_host_elem   = lGetElemHost(Master_Exechost_List, EH_name, SGE_GLOBAL_NAME);    
   /* get "template" element pointer */
   template_host_elem = lGetElemHost(Master_Exechost_List, EH_name, SGE_TEMPLATE_NAME); 
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
            if(real_host != NULL && hostcmp(real_host, host) != 0) {
               DPRINTF(("skip trashing load values for host %s simulated by %s\n", host, real_host));
               continue;
            }
         }
      }

      timeout = MAX(load_report_interval(hep)*3, conf.max_unheard); 

      if ( (hep != global_host_elem )  && 
           (now > last_heard_from(comproc, &id, host) + timeout)) {
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
         /* load reports were trashed and only the 
            static load values remained: 
            set all queues residing at this host in unknown state */
 
         qep = lGetElemHostFirst(Master_Queue_List, QU_qhostname, host, &iterator);
         while (qep != NULL) {
            u_long32 state = lGetUlong(qep, QU_state);
            SETBIT(QUNKNOWN, state);
            lSetUlong(qep, QU_state, state);
            sge_add_queue_event(sgeE_QUEUE_MOD, qep);
            DPRINTF(("%s: trashed all (%d) non-static load values -> unknown\n", 
                     lGetString(qep, QU_qname), 
                     nbefore - lGetNumberOfElem(lGetList(hep, EH_load_list))));

            /* initiate timer for this host because they turn into 'unknown' state */
            reschedule_unknown_trigger(hep); 
            qep = lGetElemHostNext(Master_Queue_List, QU_qhostname, host, &iterator); 
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

static int sge_unlink_object(
lListElem *ep,
int nm 
) {
   DENTER(TOP_LAYER, "sge_unlink_object");

   switch (nm) {
   case EH_name:
      DEXIT;
      return sge_unlink(EXECHOST_DIR, lGetHost(ep,nm));
   case AH_name:
      DEXIT;
      return sge_unlink(ADMINHOST_DIR, lGetHost(ep,nm));
   case SH_name:
      DEXIT;
      return sge_unlink(SUBMITHOST_DIR, lGetHost(ep,nm));
   case US_name:
      DEXIT;
      return sge_unlink(USERSET_DIR, lGetString(ep, nm));
   }
   DEXIT;
   return -1;
}

void sge_change_queue_version_exechost(
const char *exechost_name 
) {
   int change_all = 0;
   lListElem *qep;
   const void *iterator = NULL;


   DENTER(TOP_LAYER, "sge_change_queue_version_exechost");

   /*
      in case of global host
      all queues get a new version
   */
   if (!strcasecmp(exechost_name, SGE_GLOBAL_NAME)) {
      change_all = 1;
      DPRINTF(("increasing version of all queues "
            "because host "SGE_GLOBAL_NAME" changed\n"));
   }

   if (change_all) {
      for_each(qep, Master_Queue_List) {   
         sge_change_queue_version(qep, 0, 0);
         cull_write_qconf(1, 0, QUEUE_DIR, lGetString(qep, QU_qname), NULL, qep);
      }
   } else {
      qep = lGetElemHostFirst(Master_Queue_List, QU_qhostname, exechost_name, &iterator); 
      while (qep != NULL) {
         DPRINTF(("increasing version of queue "SFQ" because exec host "
                  SFQ" changed\n", lGetString(qep, QU_qname), exechost_name));
         sge_change_queue_version(qep, 0, 0);
         cull_write_qconf(1, 0, QUEUE_DIR, lGetString(qep, QU_qname), NULL, qep);
         qep = lGetElemHostNext(Master_Queue_List, QU_qhostname, exechost_name, &iterator); 
      }
   } 


   DEXIT;
   return;
}



/*
** sge_count_uniq_hosts  --  count all uniq hosts in adm. and subm host list
**
** PARAMETERS:
**    ahl  --  administration host list  --> sorted ascending on return
**    shl  --  submit host list          --> sorted ascending on return
** RETURN VALUE:
**    -1   --  error in lSortList
**    >=0  --  the counted uniq hosts
*/
int sge_count_uniq_hosts(
lList *ahl,
lList *shl 
) {
  int ret;
  
  DENTER(TOP_LAYER, "sge_count_uniq_hosts");
  
   ret = lGetNumberOfElem(shl);
   DPRINTF(("Counted %d submit hosts\n", ret));
   DEXIT;
   return ret;
   
   
#if 0
   lListElem *shel, *ahel;
   int counter=0;
   int ret=0;

   DENTER(TOP_LAYER, "sge_count_uniq_hosts");

   /* sort the host names in ascending order */
   if (lPSortList(ahl, "%I+", AH_name)) {
      DEXIT;
      return -1;
   }
   if (lPSortList(shl, "%I+", SH_name)) {
      DEXIT;
      return -1;
   }

   /* in each interation chop-off one host (the lexically smallest)
    * if uniq or 2 if double. count each interation.
    */
   ahel = lFirst(ahl);
   shel = lFirst(shl);
   while(ahel || shel) {
      if (shel && ahel) {
         ret = hostcmp(lGetHost(ahel,AH_name),
                          lGetHost(shel,SH_name));
         if (ret < 0) ret = -1;
         if (ret > 0) ret = 1;
      } else {
         if (!shel) ret = -1;
         if (!ahel) ret =  1;
      }

      switch ( ret ) {
      case -1 :
         /* adm hostlist elem is smaller and thus is uniq */
         ahel = lNext(ahel);
         break;
      case  0 :
         /* both are equal - remove them both */
         shel = lNext(shel);
         ahel = lNext(ahel);
         break;
      case  1 :
         /* subm hostlist elem is smaller and thus is uniq */
         shel = lNext(shel);
         break;
      }

      /* counter needs to be added every time */
      counter++;
   }

   DEXIT;
   return counter;
   
#endif   
}

/****
 **** sge_gdi_kill_exechost
 ****
 **** prepares the killing of an exechost (or all).
 **** Actutally only the permission is checked here
 **** and master_kill_execds is called to do the work.
 ****/
void sge_gdi_kill_exechost(char *host, sge_gdi_request *request, sge_gdi_request *answer)
{
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];

   DENTER(GDI_LAYER, "sge_gdi_kill_exechost");

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, 0);
      DEXIT;
      return;
   }

   if (sge_manager(user)) {
      ERROR((SGE_EVENT, MSG_OBJ_SHUTDOWNPERMS)); 
      sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, 0);
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
         sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_OK, NUM_AN_INFO);
      }
   } else {
      /* only specified exechosts should be killed. */
      
      /* walk over list with execd's to kill */
      for_each(rep, request->lp) {
         if ((getuniquehostname(lGetString(rep, ID_str), host, 0)) != CL_OK) {
            WARNING((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetString(rep, ID_str)));
            sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         } else {
            if ((lel = lGetElemHost(Master_Exechost_List, EH_name, host))) {
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
               sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
            }
         }
      }
   }
   DEXIT;
}

void master_notify_execds()
{
   lListElem *host;
   const char *hostname;

   for_each(host , Master_Exechost_List) {  
      hostname = lGetHost(host, EH_name);
      if (strcmp(hostname, "template") && strcmp(hostname, "global")) {
         notify_new_features(host, feature_get_active_featureset_id(), prognames[EXECD]);
      }
   }   
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
   static u_short number_one = 1;
   const char *action_str;
   u_long32 state;
   lListElem *jep;
   lList *mail_users, *gdil;
   int mail_options;
   char sge_mail_subj[1024];
   char sge_mail_body[1024];

   DENTER(TOP_LAYER, "notify");

   action_str = kill_jobs ? MSG_NOTIFY_SHUTDOWNANDKILL:MSG_NOTIFY_SHUTDOWN;

   hostname = lGetHost(lel, EH_name);

   execd_alive = last_heard_from(prognames[EXECD], &number_one, hostname);

   if (!force && !execd_alive) {
      WARNING((SGE_EVENT, MSG_OBJ_NOEXECDONHOST_S, hostname));
      sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_ESEMANTIC, 
                     NUM_AN_WARNING);
   }
   if (execd_alive || force) {
      if (notify_kill_job(lel, kill_jobs, prognames[EXECD])) {
         INFO((SGE_EVENT, MSG_COM_NONOTIFICATION_SSS, action_str, 
               (execd_alive ? "" : MSG_OBJ_UNKNOWN), hostname));
      } else {
         INFO((SGE_EVENT, MSG_COM_NOTIFICATION_SSS, action_str, 
               (execd_alive ? "" : MSG_OBJ_UNKNOWN), hostname));
      }
      DPRINTF((SGE_EVENT));
      sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_OK, NUM_AN_INFO);
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
               if(!(hostcmp(lGetHost(lFirst(gdil), JG_qhostname), hostname))) {
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
                  job_write_spool_file(jep, 0, SPOOL_DEFAULT);
               }
            }
         }
      }
   }

   sge_mark_unheard(lel, NULL); /* for both execd */

   DEXIT;
   return;
}

/************************************************************************
 We send an unacknowledged request for the moment. I would have a better
 feelin if we make some sort of acknowledgement. 
 ************************************************************************/
static int notify_kill_job(
lListElem *lel,
int kill_jobs,
const char *target 
) {
   const char *hostname;
   sge_pack_buffer pb;
   int ret;
   u_long32 dummy;

   hostname = lGetHost(lel, EH_name);

   if(init_packbuffer(&pb, 256, 0) == PACK_SUCCESS) {
      packint(&pb, kill_jobs);

      if (gdi_send_message_pb(0, target, 0, hostname, TAG_KILL_EXECD,
                       &pb, &dummy))
         ret = -1;
      else
         ret = 0;

      clear_packbuffer(&pb);
   } else {
      ret = -1;
   }   

   return ret;
}

int notify_new_features(
lListElem *host,
featureset_id_t featureset,
const char *target 
) {
   const char *hostname;
   sge_pack_buffer pb;
   int ret;
   u_long32 dummy;

   hostname = lGetHost(host, EH_name);
   if(init_packbuffer(&pb, 256, 0) == PACK_SUCCESS) {
      packint(&pb, featureset);
      if (gdi_send_message_pb(0, target, 0, hostname, TAG_NEW_FEATURES, 
          &pb, &dummy)) {
         ret = -1;
      } else {
         ret = 0;
      }
      clear_packbuffer(&pb);
   } else {
      ret = -1;
   }
   return ret; 
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
   lListElem *hep, *qep;
   const void *iterator = NULL;

   DENTER(TOP_LAYER, "sge_execd_startedup");

   if( !host || !ruser || !rhost) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }
   
   hep = sge_locate_host(rhost, SGE_EXECHOST_LIST);
   if(!hep) {
      if (sge_add_host_of_type(rhost, SGE_EXECHOST_LIST) < 0) {
         ERROR((SGE_EVENT, MSG_OBJ_INVALIDHOST_S, rhost));
         sge_add_answer(alpp, SGE_EVENT, STATUS_DENIED, 0);
         DEXIT;
         return STATUS_DENIED;
      } 
   }

   hep = sge_locate_host(rhost, SGE_EXECHOST_LIST);
   if(!hep) {
      ERROR((SGE_EVENT, MSG_OBJ_NOADDHOST_S, rhost));
      sge_add_answer(alpp, SGE_EVENT, STATUS_DENIED, 0);
      DEXIT;
      return STATUS_DENIED;
   }
   lSetUlong(hep, EH_featureset_id, lGetUlong(host, EH_featureset_id));
   lSetUlong(hep, EH_report_seqno, 0);


   /* reinit state of all queues at this host according to initial_state */

   qep = lGetElemHostFirst(Master_Queue_List, QU_qhostname, rhost, &iterator); 
   while (qep != NULL) {
      if (queue_initial_state(qep, rhost)) {
         sge_change_queue_version(qep, 0, 0);
         cull_write_qconf(1, 0, QUEUE_DIR, lGetString(qep, QU_qname), NULL, qep);
      } 
      qep = lGetElemHostNext(Master_Queue_List, QU_qhostname, rhost, &iterator); 
   }

   DPRINTF(("=====>STARTING_UP: %s %s on >%s< is starting up\n", 
      feature_get_product_name(FS_SHORT_VERSION), "execd", rhost));

   /*
   ** loop over pseudo hosts and set EH_startup flag
   */
   lSetUlong(hep, EH_startup, 1);
   sge_add_event(NULL, sgeE_EXECHOST_MOD, 0, 0, rhost, hep);

   INFO((SGE_EVENT, MSG_LOG_REGISTER_SS, "execd", rhost));
   sge_add_answer(alpp, SGE_EVENT, STATUS_OK, 0);

   DEXIT;
   return STATUS_OK;
}


static int verify_scaling_list(
lList **alpp,
lListElem *hep 
) {
   lListElem *ep;
   lList *resources = NULL;
   const char *name;

   DENTER(TOP_LAYER, "verify_scaling_list");

   /* check whether this attrib is available due to complex configuration */
   for_each (ep, lGetList(hep, EH_scaling_list)) {

      if (!resources) { /* first time build resources list */
         if (!strcmp(lGetHost(hep, EH_name), "global"))
            global_complexes2scheduler(&resources, hep, Master_Complex_List, 0);
         else 
            host_complexes2scheduler(&resources, hep, Master_Exechost_List, Master_Complex_List, 0);
      }

      name = lGetString(ep, HS_name);
      if (!lGetElemStr(resources, CE_name, name)) {
         resources = lFreeList(resources);
         ERROR((SGE_EVENT, MSG_OBJ_NOSCALING4HOST_SS,
               name, lGetHost(hep, EH_name)));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, NUM_AN_ERROR);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }

   resources = lFreeList(resources);
   DEXIT;
   return STATUS_OK;
}
