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

#include "sge_conf.h"
#include "sge.h"
#include "commlib.h"
#include "sge_ja_task.h"
#include "sge_requestL.h"
#include "sge_pe.h"
#include "sgermon.h"
#include "sge_prog.h"
#include "sge_host.h"
#include "slots_used.h"
#include "sge_feature.h"
#include "sge_c_gdi.h"
#include "sge_host_qmaster.h"
#include "sge_queue_qmaster.h"
#include "subordinate_qmaster.h"
#include "sge_userset_qmaster.h"
#include "sge_select_queue.h"
#include "sge_calendar_qmaster.h"
#include "sge_event_master.h"
#include "sge_queue_event_master.h"
#include "sge_parse_num_par.h"
#include "sge_signal.h"
#include "sge_string.h"
#include "sge_log.h"
#include "sge_time.h"
#include "sge_host.h"
#include "sge_qmod_qmaster.h"
#include "config_file.h"
#include "sge_userprj_qmaster.h"
#include "sge_job_qmaster.h"
#include "sge_unistd.h"
#include "sge_hostname.h"
#include "sge_answer.h"
#include "sge_queue.h"
#include "sge_job.h"
#include "sge_ckpt.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_complex.h"
#include "sge_calendar.h"
#include "sge_complex_schedd.h"
#include "sge_utility.h"
#include "sge_todo.h"
#include "sge_utility_qmaster.h"
#include "sge_static_load.h"
#include "sge_stringL.h"

#include "sge_persistence_qmaster.h"
#include "spool/sge_spooling.h"

#include "msg_common.h"
#include "msg_qmaster.h"

#ifdef QIDL
#include "qidl_c_gdi.h"
#endif

static int mod_queue_attributes(lList **alpp, lListElem *new_queue, lListElem *qep, int add, int sub_command);


static void queue_clear_unknown(lListElem *qep);

static u_long32 queue_get_queue_number(void);


/****** qmaster/queue/queue_clear_unknown() ***********************************
*  NAME
*     queue_clear_unknown() -- clear the u state of a queue 
*
*  SYNOPSIS
*     static void queue_clear_unknown(lListElem *queue) 
*
*  FUNCTION
*     Clear the unknown state of "queue" if there are load values 
*     available for the corresponding host. 
*
*  INPUTS
*     lListElem *queue - QU_Type list 
*
*  RESULT
*     static void - None 
*******************************************************************************/
static void queue_clear_unknown(lListElem *queue) 
{
   lListElem *ep, *hep;
   const char *hostname = lGetHost(queue, QU_qhostname);
   const char *queue_name = lGetString(queue, QU_qname);

   DENTER(TOP_LAYER, "queue_clear_unknown");

   hep = host_list_locate(Master_Exechost_List, hostname);
   if (hep == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_UNABLE2FINDHOST_S, hostname));
      DEXIT;
      return;
   }

   for_each (ep, lGetList(hep, EH_load_list)) {
      if (!sge_is_static_load_value(lGetString(ep, HL_name))) {
         u_long32 state = lGetUlong(queue, QU_state);

         CLEARBIT(QUNKNOWN, state);
         lSetUlong(queue, QU_state, state);
         DPRINTF(("QUEUE %s CLEAR UNKNOWN STATE\n", queue_name));
         DEXIT;
         return;
      }
   }

   DPRINTF(("no non-static load value for host %s of queue %s\n",
            hostname, queue_name));

   DEXIT;
   return;
}

/****** qmaster/queue/queue_get_queue_number() ********************************
*  NAME
*     queue_get_queue_number() -- Returns a unique (unused) queue number 
*
*  SYNOPSIS
*     static u_long32 queue_get_queue_number(void) 
*
*  FUNCTION
*     This function returns a unique queue number which is not used
*     until now. 
*
*  INPUTS
*     void - None
*
*  RESULT
*     static u_long32 - unique queue id
*******************************************************************************/
static u_long32 queue_get_queue_number(void)
{
   static u_long32 queue_number=1;
   int start = queue_number;

   DENTER(TOP_LAYER, "queue_get_queue_number");
   while (1) {
      if (queue_number > MAX_SEQNUM) {
         queue_number = 1;
      }

      if (!lGetElemUlong(Master_Queue_List, QU_queue_number, queue_number)) {
         DEXIT;
         return queue_number++;
      }

      queue_number++;

      if (start == queue_number) {
         /* out of queue numbers -> real awkward */
         DEXIT;
         return 0; 
      }
   }
}

/****** qmaster/queue/queue_set_initial_state() *******************************
*  NAME
*     queue_set_initial_state() -- set "initial" state 
*
*  SYNOPSIS
*     int queue_set_initial_state(lListElem *queue, char *rhost) 
*
*  FUNCTION
*     Change the "queue" state according to the "initial" state 
*     configuration (QU_initial_state).
*
*  INPUTS
*     lListElem *queue - QU_Type element 
*     char *rhost      - hostname (used to write to SGE_EVENT)
*
*  RESULT
*     int - was the queue changed?
*        0 - no queue change
*        1 - queue state was changed due to initial state
*******************************************************************************/
int queue_set_initial_state(lListElem *queue, char *rhost)
{
   const char *is = lGetString(queue, QU_initial_state);
   int changed = 0;

   DENTER(TOP_LAYER, "queue_set_initial_state");

   if (is != NULL && strcmp(is, "default")) {
      int enable = !strcmp(is, "enabled");
      u_long32 state = lGetUlong(queue, QU_state);

      if ((enable && (state & QDISABLED)) ||
          (!enable && !(state & QDISABLED))) {
         const char *queue_name = lGetString(queue, QU_qname);

         if (!rhost) {
            if (enable) {
               WARNING((SGE_EVENT, MSG_QUEUE_ADDENABLED_S, queue_name));
            } else {
               WARNING((SGE_EVENT, MSG_QUEUE_ADDDISABLED_S, queue_name));
            }
         } else {
            if (enable) {
               WARNING((SGE_EVENT, MSG_QUEUE_EXECDRESTARTENABLEQ_SS,
                        rhost, queue_name));
            } else {
               WARNING((SGE_EVENT, MSG_QUEUE_EXECDRESTARTDISABLEQ_SS,
                        rhost, queue_name));
            }
         }

         if (enable) {
            CLEARBIT(QDISABLED, state);
         } else {
            SETBIT(QDISABLED, state);
         }
         lSetUlong(queue, QU_state, state);
         changed = 1;
      }
   }

   DEXIT;
   return changed;
}

static int mod_queue_attributes(
lList **alpp,
lListElem *new_queue,
lListElem *qep, /* reduced queue element */
int add,
int sub_command 
) {
   const char *qname;

   DENTER(TOP_LAYER, "mod_queue_attributes");

   /* ---- QU_qname cannot get changed - we just ignore it */
   if (add) {
      if (lGetPosViaElem(qep, QU_qname)<0) {
         CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
               lNm2Str(QU_qname), SGE_FUNC));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         goto ERROR;            
      }
      qname = lGetString(qep, QU_qname);
      if (verify_str_key(alpp, qname, "queue"))
         goto ERROR;            
      lSetString(new_queue, QU_qname, qname);
   } else {
      qname = lGetString(new_queue, QU_qname);
   }

   /* ---- QU_qhostname cannot get changed - we just ignore it */
   if (add) {
      if (lGetPosViaElem(qep, QU_qhostname) < 0) {
         CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
               lNm2Str(QU_qhostname), SGE_FUNC));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         goto ERROR;            
      }
      if (sge_resolve_host(qep, QU_qhostname)!=0) {
         const char *hname = lGetHost(qep, QU_qhostname);
         ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, hname ? hname : ""));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         goto ERROR;            
      }
      lSetHost(new_queue, QU_qhostname, lGetHost(qep, QU_qhostname));
   } else {
   
      if (lGetPosViaElem(qep, QU_qhostname) >= 0) {
         if (sge_hostcmp(lGetHost(new_queue, QU_qhostname), lGetHost(qep, QU_qhostname))) {
            ERROR((SGE_EVENT, MSG_SGETEXT_NOTPOSSIBLETOMODHOSTNAME));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR); 
            goto ERROR;
         } 
      }
   }

   /* ---- QU_complex_list */
   if (lGetPosViaElem(qep, QU_complex_list)>=0) {
      DPRINTF(("got new QU_complex_list\n"));

      /* check complex list */
      normalize_sublist(qep, QU_complex_list);
      if (complex_list_verify(lGetList(qep, QU_complex_list), alpp, "queue", 
                              qname)!=STATUS_OK)
         goto ERROR;
#if 0
      lSetList(new_queue, QU_complex_list, lCopyList("", 
            lGetList(qep, QU_complex_list)));
#endif
      attr_mod_sub_list(alpp, new_queue, QU_complex_list,
            CX_name, qep, sub_command,
            SGE_ATTR_COMPLEX_LIST, SGE_OBJ_QUEUE, 0);  
   }

   /* ---- QU_seq_no */
   attr_mod_ulong(qep, new_queue, QU_seq_no, "sequence number");
  
   /* ---- QU_load_thresholds */
   if (attr_mod_threshold(alpp, qep, new_queue, QU_load_thresholds, CE_name, 
            sub_command, SGE_ATTR_COMPLEX_VALUES, SGE_OBJ_QUEUE)) {
      goto ERROR;            
   }

   /* ---- QU_suspend_thresholds */
   if (attr_mod_threshold(alpp, qep, new_queue, QU_suspend_thresholds, CE_name,
            sub_command, SGE_ATTR_SUSPEND_THRESHOLD, SGE_OBJ_QUEUE))
      goto ERROR;            

   /* ---- QU_nsuspend */
   attr_mod_ulong(qep, new_queue, QU_nsuspend, "nsuspend");

   /* ---- QU_suspend_interval */
   if (attr_mod_time_str(alpp, qep, new_queue, QU_suspend_interval, 
            "suspend interval",0))
      goto ERROR;            

   /* ---- QU_priority */
   if (lGetPosViaElem(qep, QU_priority)>=0) {
      const char *s;
      int priority;
      
      if (!(s = lGetString(qep, QU_priority))) {
         ERROR((SGE_EVENT, MSG_QUEUE_PRIORITYRANGE));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         goto ERROR;
      }
      priority = atoi(s);
      if (priority < -20 || priority > 20 ) {
         /* out of range */
         ERROR((SGE_EVENT, MSG_QUEUE_PRIORITYRANGE));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         goto ERROR;
      }
      lSetString(new_queue, QU_priority, s);
   }

   /* ---- QU_min_cpu_interval */
   if (attr_mod_time_str(alpp, qep, new_queue, QU_min_cpu_interval, 
         "min_cpu_interval",0))
      goto ERROR;            

   /* ---- QU_processors */ 
   if (attr_mod_str(alpp, qep, new_queue, QU_processors, "processors")) goto ERROR;

   /* ---- QU_qtype */      
   if (lGetPosViaElem(qep, QU_qtype) >= 0) {
      u_long32 qtype;

      qtype = lGetUlong(qep, QU_qtype);
      if (sub_command == SGE_GDI_APPEND || sub_command == SGE_GDI_CHANGE) {
         qtype = lGetUlong(new_queue, QU_qtype) | qtype;
      } else if (sub_command == SGE_GDI_REMOVE) {
         qtype = lGetUlong(new_queue, QU_qtype) & (~qtype);
      }
      lSetUlong(new_queue, QU_qtype, qtype);
   }

   /* ---- QU_pe_list */
   if (lGetPosViaElem(qep, QU_pe_list)>=0) {
      DPRINTF(("got new QU_pe_list\n"));

      normalize_sublist(qep, QU_pe_list);
      if (!pe_list_do_all_exist(*(pe_list_get_master_list()), alpp, 
                                lGetList(qep, QU_pe_list))) {
         goto ERROR;
      }
      attr_mod_sub_list(alpp, new_queue, QU_pe_list, ST_name, qep, 
                        sub_command, SGE_ATTR_COMPLEX_LIST, SGE_OBJ_QUEUE, 0);
   }

   /* ---- QU_ckpt_list */
   if (lGetPosViaElem(qep, QU_ckpt_list)>=0) {
      DPRINTF(("got new QU_ckpt_list\n"));

      normalize_sublist(qep, QU_ckpt_list);
      if (!ckpt_list_do_all_exist(*(ckpt_list_get_master_list()), alpp, 
                                  lGetList(qep, QU_ckpt_list))) {
         goto ERROR;
      }
      attr_mod_sub_list(alpp, new_queue, QU_ckpt_list, ST_name, qep, 
                        sub_command, SGE_ATTR_COMPLEX_LIST, SGE_OBJ_QUEUE, 0);
   }

   /* ---- QU_rerun */
   attr_mod_bool(qep, new_queue, QU_rerun, "rerun");

   /* ---- QU_job_slots */
   if (lGetPosViaElem(qep, QU_job_slots)>=0) {
      lSetUlong(new_queue, QU_job_slots, lGetUlong(qep, QU_job_slots));
   }
    
   /* ---- QU_tmpdir */ 
   if (attr_mod_str(alpp, qep, new_queue, QU_tmpdir, "tmpdir")) {
      goto ERROR;
   }

   /* ---- QU_shell */  
   if (attr_mod_str(alpp, qep, new_queue, QU_shell, "shell")) {
      goto ERROR;
   }

   /* ---- QU_notify */
   if (attr_mod_time_str(alpp, qep, new_queue, QU_notify, "notify",0)) {
      goto ERROR;            
   }

   /* ---- QU_owner_list */
   if (lGetPosViaElem(qep, QU_owner_list)>=0) {
      DPRINTF(("got new QU_owner_list\n"));
#if 0
      lSetList(new_queue, QU_owner_list,
               lCopyList("", lGetList(qep, QU_owner_list)));
#endif
      attr_mod_sub_list(alpp, new_queue, QU_owner_list,
            US_name, qep, sub_command,
            SGE_ATTR_OWNER_LIST, SGE_OBJ_QUEUE, 0); 
   }

   /* ---- QU_acl */
   if (lGetPosViaElem(qep, QU_acl)>=0) {
      DPRINTF(("got new QU_acl\n"));
      /* check user_lists */
      normalize_sublist(qep, QU_acl);
      if (userset_list_validate_acl_list(alpp, lGetList(qep, QU_acl), "user_lists", 
            "queue", qname)!=STATUS_OK) 
         goto ERROR;            
#if 0
      lSetList(new_queue, QU_acl, lCopyList("", lGetList(qep, QU_acl)));
#endif
      attr_mod_sub_list(alpp, new_queue, QU_acl,
            US_name, qep, sub_command,
            SGE_ATTR_USER_LISTS, SGE_OBJ_QUEUE, 0); 
   }

   /* ---- QU_xacl */
   if (lGetPosViaElem(qep, QU_xacl)>=0) {
      DPRINTF(("got new QU_axcl\n"));
      /* check xuser_lists */
      normalize_sublist(qep, QU_xacl);
      if (userset_list_validate_acl_list(alpp, lGetList(qep, QU_xacl), "xuser_lists", 
            "queue", qname)!=STATUS_OK)
         goto ERROR;            
#if 0
      lSetList(new_queue, QU_xacl, lCopyList("", lGetList(qep, QU_xacl)));
#endif
      attr_mod_sub_list(alpp, new_queue, QU_xacl,
            US_name, qep, sub_command,
            SGE_ATTR_XUSER_LISTS, SGE_OBJ_QUEUE, 0); 
   }

   if (lGetPosViaElem(qep, QU_xacl)>=0 || lGetPosViaElem(qep, QU_acl)>=0) {
      if (multiple_occurances(
            alpp, 
            lGetList(new_queue, QU_acl), 
            lGetList(new_queue, QU_xacl), 
            US_name, 
            qname, "queue")) {
         goto ERROR;            
      }
   }

   /* ---- QU_subordinate_list */
   if (lGetPosViaElem(qep, QU_subordinate_list)>=0) {
      /* check new subordinates */
      DPRINTF(("got new QU_subordinate_list\n"));

      if (check_subordinate_list(alpp, qname, 
            lGetHost(new_queue, QU_qhostname),
            lGetUlong(new_queue, QU_job_slots), 
            lGetList(qep, QU_subordinate_list), CHECK4MOD)!=STATUS_OK)
         goto ERROR;            
#if 0
      lSetList(new_queue, QU_subordinate_list, 
            lCopyList("", lGetList(qep, QU_subordinate_list)));
#endif
      attr_mod_sub_list(alpp, new_queue, QU_subordinate_list,
            SO_qname, qep, sub_command,
            SGE_ATTR_SUBORDINATE_LIST, SGE_OBJ_QUEUE, 0); 
   }

   /* ---- QU_consumable_config_list */
   if (attr_mod_threshold(alpp, qep, new_queue, QU_consumable_config_list, CE_name,
         sub_command, SGE_ATTR_COMPLEX_VALUES, SGE_OBJ_QUEUE)) 
      goto ERROR;            
   
   if (feature_is_enabled(FEATURE_SGEEE)) {
      attr_mod_ulong(qep, new_queue, QU_fshare, "functional share");
      attr_mod_ulong(qep, new_queue, QU_oticket, "override tickets");

      /* ---- QU_projects */
      if (lGetPosViaElem(qep, QU_projects)>=0) {
         DPRINTF(("got new QU_projects\n"));
         /* check project list */
         normalize_sublist(qep, QU_projects);
         if (verify_userprj_list(alpp, lGetList(qep, QU_projects), 
                  Master_Project_List, "projects", "queue", qname)!=STATUS_OK)
            goto ERROR;
#if 0
         lSetList(new_queue, QU_projects,
                  lCopyList("", lGetList(qep, QU_projects)));
#endif
         attr_mod_sub_list(alpp, new_queue, QU_projects,
               UP_name, qep, sub_command,
               SGE_ATTR_PROJECTS, SGE_OBJ_QUEUE, 0);         
      }

      /* ---- QU_xprojects */
      if (lGetPosViaElem(qep, QU_xprojects)>=0) {
         DPRINTF(("got new QU_xprojects\n"));
         /* check project list */
         normalize_sublist(qep, QU_xprojects);
         if (verify_userprj_list(alpp, lGetList(qep, QU_xprojects), 
                  Master_Project_List, "xprojects", "queue", qname)!=STATUS_OK)
            goto ERROR;
#if 0
         lSetList(new_queue, QU_xprojects,
                  lCopyList("", lGetList(qep, QU_xprojects)));
#endif
         attr_mod_sub_list(alpp, new_queue, QU_xprojects,
               UP_name, qep, sub_command,
               SGE_ATTR_XPROJECTS, SGE_OBJ_QUEUE, 0);   
      }

      if (lGetPosViaElem(qep, QU_xprojects)>=0 || 
          lGetPosViaElem(qep, QU_projects)>=0) {
         if (multiple_occurances(
               alpp,
               lGetList(new_queue, QU_projects),
               lGetList(new_queue, QU_xprojects),
               UP_name,
               qname, "queue")) {
            goto ERROR;
         }
      }
   }

   /* ---- QU_calendar */
   if (lGetPosViaElem(qep, QU_calendar)>=0) {
      const char *nc, *oc;
      lListElem *new_cal = NULL;

      nc = lGetString(qep, QU_calendar);
      if (nc && !(new_cal = calendar_list_locate(Master_Calendar_List, nc))) {
         
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, 
                 MSG_CALENDAR_CALENDARXREFERENCEDINQUEUEYNOTEXISTS_SS,
                 nc, 
                 qname));
         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         goto ERROR;            
      }

      oc = lGetString(new_queue, QU_calendar);
      if ((oc && nc && !strcmp(nc, oc)) || (!oc && !nc)) {
         DPRINTF(("no changes with queue calendar\n"));
      } else {
         /* change state */
         lSetUlong(new_queue, QU_state, act_cal_state(new_cal, NULL)| 
            (lGetUlong(new_queue, QU_state) & ~(QCAL_SUSPENDED|QCAL_DISABLED)));
      }
   }
   attr_mod_zerostr(qep, new_queue, QU_calendar, "calendar");
   if (attr_mod_procedure(alpp, qep, new_queue, QU_prolog,   
         "prolog", prolog_epilog_variables)) {
      goto ERROR;
   }

   if (attr_mod_procedure(alpp, qep, new_queue, QU_epilog,   
         "epilog", prolog_epilog_variables)) {
      goto ERROR;
   }

   attr_mod_zerostr(qep, new_queue, QU_starter_method, "starter_method");

   /* control methods */
   if (attr_mod_ctrl_method(alpp, qep, new_queue, QU_suspend_method,   
         "suspend_method")) {
      goto ERROR;
   }
   if (attr_mod_ctrl_method(alpp, qep, new_queue, QU_resume_method,    
         "resume_method")) {
      goto ERROR;
   }
   if (attr_mod_ctrl_method(alpp, qep, new_queue, QU_terminate_method, 
      "terminate_method")) {
      goto ERROR;            
   }
   if (lGetPosViaElem(qep, QU_shell_start_mode)>=0) {
      const char *s;

      s = lGetString(qep, QU_shell_start_mode);
      if (s && strcasecmp("none", s) && 
               strcasecmp("unix_behavior", s) && 
               strcasecmp("posix_compliant", s) && 
               strcasecmp("script_from_stdin", s)) {
        
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_QMASTER_XNOVALIDSSM_S , s));

         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         goto ERROR;
      }
   }
   attr_mod_zerostr(qep, new_queue, QU_shell_start_mode, "shell_start_mode");

   if (attr_mod_str(alpp, qep, new_queue, QU_initial_state, "initial state")) {
      goto ERROR;
   }
   if (lGetPosViaElem(qep, QU_initial_state)>=0) {
      const char *s;

      s = lGetString(qep, QU_initial_state);
      if (s && strcasecmp("default", s) && 
               strcasecmp("enabled", s) && 
               strcasecmp("disabled", s)) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_QMASTER_XNOVALIDIS_S , s));
         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         goto ERROR;
      }

      if (add) {
         queue_set_initial_state(new_queue, NULL);
      }
   }

   /* resource limits */
   if (attr_mod_time_str(alpp, qep, new_queue, QU_s_rt, "soft real time",1)) {
      goto ERROR;            
   }
   if (attr_mod_time_str(alpp, qep, new_queue, QU_h_rt, "hard real time",1)) {
      goto ERROR;            
   }
   if (attr_mod_time_str(alpp, qep, new_queue, QU_s_cpu, "soft cpu limit",1)) {
      goto ERROR;            
   }
   if (attr_mod_time_str(alpp, qep, new_queue, QU_h_cpu, "hard cpu limit",1)) {
      goto ERROR;            
   }
   if (attr_mod_mem_str(alpp, qep, new_queue, QU_s_fsize, "soft file size")) {
      goto ERROR;            
   }
   if (attr_mod_mem_str(alpp, qep, new_queue, QU_h_fsize, "hard file size")) {
      goto ERROR;            
   }
   if (attr_mod_mem_str(alpp, qep, new_queue, QU_s_data, "soft data size")) {
      goto ERROR;            
   }
   if (attr_mod_mem_str(alpp, qep, new_queue, QU_h_data, "hard data size")) {
      goto ERROR;            
   }
   if (attr_mod_mem_str(alpp, qep, new_queue, QU_s_stack, "soft stack size")) {
      goto ERROR;
   }
   if (attr_mod_mem_str(alpp, qep, new_queue, QU_h_stack, "hard stack size")) {
      goto ERROR;
   }
   if (attr_mod_mem_str(alpp, qep, new_queue, QU_s_core, "soft core size")) {
      goto ERROR;            
   }
   if (attr_mod_mem_str(alpp, qep, new_queue, QU_h_core, "hard core size")) {
      goto ERROR;            
   }
   if (attr_mod_mem_str(alpp, qep, new_queue, QU_s_rss, "soft rss size")) {
      goto ERROR;            
   }
   if (attr_mod_mem_str(alpp, qep, new_queue, QU_h_rss, "hard rss size")) {
      goto ERROR;            
   }
   if (attr_mod_mem_str(alpp, qep, new_queue, QU_s_vmem, 
         "soft virtual memory size")) {
      goto ERROR;            
   }
   if (attr_mod_mem_str(alpp, qep, new_queue, QU_h_vmem, 
         "hard virtual memory size")) {
      goto ERROR;            
   }
   
   if (add) {
      /* initialization of internal fields */
      lSetUlong(new_queue, QU_state, lGetUlong(new_queue, QU_state)|QUNKNOWN);
      lSetUlong(new_queue, QU_queue_number, queue_get_queue_number());
   }

   DEXIT;
   return 0;

ERROR:
   DEXIT;
   return STATUS_EUNKNOWN;
}

/*-------------------------------------------------------------------------*/
/* sge_gdi_modify_queue                                                    */
/*    called in sge_c_gdi_mod                                              */
/* written for: qmaster                                                    */
/*-------------------------------------------------------------------------*/

/*
   this is our strategy:

   do common checks and search old queue
   make a copy of the old queue (this will become the new queue)
   modify new queue using reduced queue as instruction
      on error: dispose new queue
   store new queue to disc
      on error: dispose new queue
   create events
   replace old queue by new queue
*/
int sge_gdi_add_mod_queue(
lListElem *qep, /* our instructions - a reduced QU_Type element */
lList **alpp,
char *ruser,
char *rhost,
int add, /* true in case of add */
int sub_command 
) {
   const char *qname;
   lList *tmp_alp = NULL;
   lListElem *new_queue = NULL,
             *old_queue;
   lList *unsuspend = NULL,
         *suspend = NULL;
   u_long32 old_cal_state, new_cal_state;
   int old_slots_used = 0;
   int disable_susp_thresholds = 0; 

   DENTER(TOP_LAYER, "sge_gdi_add_modify_queue");

   /* DO COMMON CHECKS AND SEARCH OLD QUEUE */
   if ( !qep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* ep is no queue element, if ep has no QU_qname */
   if (lGetPosViaElem(qep, QU_qname)<0) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(QU_qname), SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   qname = lGetString(qep, QU_qname); 
   old_queue = queue_list_locate(Master_Queue_List, qname);

   if ((old_queue && add) ||
      (!old_queue && !add)) {
      if (!add)
         ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, MSG_OBJ_QUEUE, qname));
      else 
         ERROR((SGE_EVENT, MSG_SGETEXT_ALREADYEXISTS_SS, MSG_OBJ_QUEUE, qname));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* MAKE A COPY OF THE OLD QUEUE (THIS WILL BECOME THE NEW QUEUE) */
   if (!(new_queue = (add 
         ? lCreateElem(QU_Type) 
         : lCopyElem(old_queue)))) {
      ERROR((SGE_EVENT, MSG_MEM_MALLOC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* MODIFY NEW QUEUE USING REDUCED QUEUE AS INSTRUCTION */
   if (mod_queue_attributes(&tmp_alp, new_queue, qep, add, sub_command)) {
      /* ON ERROR: DISPOSE NEW QUEUE */
      /* failure: just append last elem in tmp_alp
         elements before may contain invalid success messages */
      lListElem *failure;
      failure = lLast(tmp_alp);

      lDechainElem(tmp_alp, failure);
      if (!*alpp)
         *alpp = lCreateList("answer", AN_Type);
      lAppendElem(*alpp, failure);
      lFreeList(tmp_alp);
      lFreeElem(new_queue);
      DEXIT;
      return STATUS_EUNKNOWN;
   }
   if (tmp_alp) {
      lListElem *next_failure, *failure;

      DPRINTF(("tmp_alp\n"));
      next_failure = lFirst(tmp_alp);
      while((failure = next_failure)) {
         next_failure = lNext(failure);

         lDechainElem(tmp_alp, failure);
         if (!*alpp)
            *alpp = lCreateList("answer", AN_Type);  
         lAppendElem(*alpp, failure);
      }
      lFreeList(tmp_alp);
   }

   if (!add) {
      /* create a list of subordinates that are susupended actually */
      copy_suspended(&unsuspend, 
            lGetList(old_queue, QU_subordinate_list), 
            qslots_used(old_queue), 
            lGetUlong(old_queue, QU_job_slots), 
            lGetUlong(old_queue, QU_suspended_on_subordinate));

      /* create a list of subordinates that must be susupended with the new queue */
      copy_suspended(&suspend, 
            lGetList(new_queue, QU_subordinate_list), 
            qslots_used(new_queue), 
            lGetUlong(new_queue, QU_job_slots), 
            lGetUlong(new_queue, QU_suspended_on_subordinate));

      /* remove equal entries in both lists */
      lDiffListStr(SO_qname, &suspend, &unsuspend);
   }

   /* disabling susp. thresholds (part 1) */
   if (!add) {
      u_long32 interval;
      int disabled_in_old_queue;
      int disabled_in_new_queue;

      parse_ulong_val(NULL, &interval, TYPE_TIM,
        lGetString(new_queue, QU_suspend_interval), NULL, 0);
      disabled_in_new_queue = (lGetUlong(new_queue, QU_nsuspend) == 0
          || interval == 0
          || !lGetList(new_queue, QU_suspend_thresholds));

      parse_ulong_val(NULL, &interval, TYPE_TIM,
         lGetString(old_queue, QU_suspend_interval), NULL, 0);
      disabled_in_old_queue = (lGetUlong(old_queue, QU_nsuspend) == 0
          || interval == 0
          || !lGetList(old_queue, QU_suspend_thresholds));

      if (!disabled_in_old_queue && disabled_in_new_queue) {
         disable_susp_thresholds = 1; 
      }
   }
               
   /* write on file */
   if (sge_change_queue_version(new_queue, add, 1) ||
      /* event has already been sent in sge_change_queue_version */
      !sge_event_spool(alpp, 0, sgeE_QUEUE_MOD,
                       0, 0, lGetString(new_queue, QU_qname), NULL,
                       new_queue, NULL, NULL, false, true)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS, MSG_OBJ_QUEUE, qname));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* chain out the old one */
   if (!add) {
      old_cal_state = lGetUlong(old_queue, QU_state) & (QCAL_SUSPENDED|QCAL_DISABLED);
      old_slots_used = qslots_used(old_queue);
      sge_del_queue(qname);
   } else {
      old_cal_state = 0;
   }


   /* chain in new */
   queue_list_add_queue(new_queue);

#ifdef QIDL
   if(add)
      addObjectByName(SGE_QUEUE_LIST, lGetString(qep, QU_qname));
#endif

   /* ------------------------------------------------------------
      NOW ALL CRITICAL THINGS ARE DONE 
      ------------------------------------------------------------ */


   /* ------------------------------------------------------------
      (0) handle job_slots as a consumble attribute of queue
      (1) and we have to deactivate suspensions on subordinate 
          in all queues that appear only in the old list 
      (2) we have to activate suspensions on subordinate 
          in all queues that appear only in the new list 
      (3) the state of queues that are found in both lists stays 
          unchanged 
      ------------------------------------------------------------ */
   {
      lListElem *gdil_ep, *jep;

      lSetList(new_queue, QU_consumable_actual_list, NULL);

      slots2config_list(new_queue);
      debit_queue_consumable(NULL, new_queue, Master_Complex_List, 0); 
      for_each (jep, Master_Job_List) {
         int slots = 0;
         lListElem *jatep;

         for_each(jatep, lGetList(jep, JB_ja_tasks)) {
            if (!(gdil_ep=lGetSubStr(jatep, JG_qname, qname, 
               JAT_granted_destin_identifier_list))) 
               continue;
            slots += lGetUlong(gdil_ep, JG_slots);
         }
         if (slots)
            debit_queue_consumable(jep, new_queue, Master_Complex_List, slots); 
      }
   }

   if (!add) {
      unsuspend_all(unsuspend, 0);
      lFreeList(unsuspend); 
      suspend_all(suspend, 0);
      lFreeList(suspend); 
   } else {
      lList *sos_list_after = NULL;

      /*
          we have to activate suspensions on subordinate
          in all queues where it is necessary
      */
      copy_suspended(&sos_list_after,
            lGetList(new_queue, QU_subordinate_list),
            qslots_used(new_queue), 
            lGetUlong(new_queue, QU_job_slots),
            lGetUlong(new_queue, QU_suspended_on_subordinate));
      suspend_all(sos_list_after, 0);
      lFreeList(sos_list_after);

      /*
          we have to activate suspensions on subordinate
          from other queues if the conditions are given
      */
      count_suspended_on_subordinate(new_queue);
   }

   /* send signals (on calendar) if needed */
   new_cal_state = lGetUlong(new_queue, QU_state) & (QCAL_SUSPENDED|QCAL_DISABLED);
   signal_on_calendar(new_queue, old_cal_state, new_cal_state);

   if (add) {
      if (!host_list_locate(Master_Exechost_List, 
                           lGetHost(new_queue, QU_qhostname)))
         sge_add_host_of_type(lGetHost(new_queue, QU_qhostname), SGE_EXECHOST_LIST);
      queue_clear_unknown(new_queue);
   }

   /* disabling susp. thresholds (part 2) */
   if (!add && disable_susp_thresholds) {
      lListElem *job, *ja_task;

      for_each(job, Master_Job_List) {
         for_each(ja_task, lGetList(job, JB_ja_tasks)) {
            const char *queue_name;
            u_long32 state;

            state = lGetUlong(ja_task, JAT_state);
            if (!ISSET(state, JSUSPENDED_ON_THRESHOLD)) {
               continue;
            }
            queue_name = lGetString(lFirst(lGetList(ja_task,
               JAT_granted_destin_identifier_list)), JG_qname);
            if (strcmp(queue_name, lGetString(new_queue, QU_qname))) {
               continue;
            }

            if (!ISSET(state, JSUSPENDED)) {
               sge_signal_queue(SGE_SIGCONT, new_queue, job, ja_task);
               SETBIT(JRUNNING, state);
            }

            CLEARBIT(JSUSPENDED_ON_THRESHOLD, state);
            lSetUlong(ja_task, JAT_state, state);
            sge_event_spool(alpp, 0, sgeE_JATASK_MOD,
                            lGetUlong(job, JB_job_number), 
                            lGetUlong(ja_task, JAT_task_number), NULL, NULL,
                            job, ja_task, NULL, true, true);
         }
      }
   }         

   /* send an additional event concerning this queue to schedd */
   /* the first one is sent whithin sge_change_queue_version() */
   /* JG: TODO: we should also spool here - e.g. a new queue's state has
    *           been cleared since first spooling.
    *           Perhaps simply spool later?
    */
   sge_event_spool(alpp, 0, sgeE_QUEUE_MOD,
                   0, 0, lGetString(new_queue, QU_qname), NULL,
                   new_queue, NULL, NULL, true, false);

   INFO((SGE_EVENT, add?MSG_SGETEXT_ADDEDTOLIST_SSSS:
      MSG_SGETEXT_MODIFIEDINLIST_SSSS, ruser, rhost, qname, MSG_OBJ_QUEUE));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   DEXIT;
   return STATUS_OK;
}


/*-------------------------------------------------------------------------*/
/* sge_gdi_delete_queue                                                    */
/*    called in sge_c_gdi_del                                              */
/* written for: qmaster                                                    */
/*-------------------------------------------------------------------------*/

int sge_gdi_delete_queue(
lListElem *qep,
lList **alpp,
char *ruser,
char *rhost 
) {
   int running_jobs;
   lList *sos_list_before = NULL;
   char qname[256];
   lListElem *oqep;

   DENTER(TOP_LAYER, "sge_gdi_delete_queue");

   if ( !qep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* ep is no queue element, if ep has no QU_qname */
   if (lGetPosViaElem(qep, QU_qname)<0) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(QU_qname), SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   strcpy(qname, lGetString(qep, QU_qname));

   /* deleting a template queue is not allowed */
   if (!strcmp(qname, SGE_TEMPLATE_NAME )) {
      ERROR((SGE_EVENT, MSG_SGETEXT_OPNOTALLOWED_S, MSG_QUEUE_DELQUEUETEMPLATE));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, 0);
      DEXIT;
      return STATUS_ESEMANTIC;
   }

   /* deleting a subordinate queues is not allowed */
   {
      lListElem *qep;
      
      for_each (qep, Master_Queue_List) {
         lListElem *sqep;
   
         sqep = lGetElemStr(lGetList(qep, QU_subordinate_list),
            SO_qname, qname);

         if (sqep) {
            ERROR((SGE_EVENT, MSG_NOTALLOWEDTODELSUBORDINATE_SS, qname, lGetString(qep, QU_qname)));
            answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, 0);  
            DEXIT;
            return STATUS_ESEMANTIC;
         }
      }
   }

   if (!(oqep = queue_list_locate(Master_Queue_List, qname))) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, MSG_OBJ_QUEUE, qname));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, 0); 
      DEXIT;
      return STATUS_EEXIST;
   }
   running_jobs = qslots_used(oqep);
   if (running_jobs) {
      ERROR((SGE_EVENT, MSG_SGETEXT_ACTIVEUNITS_SSIS,
         MSG_OBJ_QUEUE, qname, running_jobs, MSG_OBJ_JOBS));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* need to unsuspend all suspended queues 
      that were in the subordinate list */
   copy_suspended(&sos_list_before,
         lGetList(oqep, QU_subordinate_list),
         qslots_used(oqep),
         lGetUlong(oqep, QU_job_slots),
         lGetUlong(oqep, QU_suspended_on_subordinate));

   if (sge_del_queue(qname)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, MSG_OBJ_QUEUE, qname));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, 0); 
      DEXIT;
      return STATUS_EEXIST;
   }

   sge_event_spool(alpp, 0, sgeE_QUEUE_DEL, 
                   0, 0, qname, NULL,
                   NULL, NULL, NULL, true, true);

   unsuspend_all(sos_list_before, 0);
   lFreeList(sos_list_before); 

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS,
         ruser, rhost, qname, MSG_OBJ_QUEUE));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   DEXIT;
   return STATUS_OK;

}

/* ----------------------------------------

   written for: qmaster

   increase version of queue 
   generate an queue modify event

*/
int sge_change_queue_version(
lListElem *qep,
int add,
int write_history 
) {
   DENTER(TOP_LAYER, "sge_change_queue_version");

   /*
   ** problem: error handling missing here
   */

   lSetUlong(qep, QU_version, add ? 0 : lGetUlong(qep, QU_version) + 1);

   sge_add_queue_event(add?sgeE_QUEUE_ADD:sgeE_QUEUE_MOD, qep);

   DEXIT;
   return 0;
}

/* -------------------------------------------------------
   sge_del_queue - delete elements of the queue structure

   returns:
   0 ok
   -1 error;

*/
int sge_del_queue(
const char *qname 
) {
   lListElem *qep;

   DENTER(TOP_LAYER, "sge_del_queue");

   if (!qname) {
      ERROR((SGE_EVENT, MSG_QUEUE_NULLPTRPASSEDTOSGE_DEL_QUEUE));
      DEXIT;
      return -1;
   }

   qep = queue_list_locate(Master_Queue_List, qname);
   if (!qep) {
      ERROR((SGE_EVENT, MSG_QUEUE_CANTLOCATEQUEUEX_S, qname));
      DEXIT;
      return -1;
   }

   lRemoveElem(Master_Queue_List, qep);

   DEXIT;
   return 0;
}


/****** qmaster/queue/queue_list_set_unknown_state_to() ********************
*  NAME
*     queue_list_set_unknown_state_to() -- set/clear u state of queues
*
*  SYNOPSIS
*     void queue_list_set_unknown_state_to(lList *queue_list,
*                                          const char *hostname,
*                                          int send_events,
*                                          int new_state)
*
*  FUNCTION
*     Clears or sets the unknown state of all queues in "queue_list" which
*     reside on host "hostname". If "hostname" is NULL than all queues
*     mentioned in "queue_list" will get the new unknown state.
*
*     Modify events for all modified queues will be generated if
*     "send_events" is 1 (true).
*
*  INPUTS
*     lList *queue_list    - QU_Type list
*     const char *hostname - valid hostname or NULL
*     int send_events      - 0 or 1
*     int new_state        - new unknown state (0 or 1)
*
*  RESULT
*     void - None
*******************************************************************************/
void queue_list_set_unknown_state_to(lList *queue_list,
                                     const char *hostname,
                                     int send_events,
                                     int new_state)
{
   const void *iterator = NULL;
   lListElem *queue = NULL;
   lListElem *next_queue = NULL;

   if (hostname != NULL) {
      next_queue = lGetElemHostFirst(queue_list, QU_qhostname,
                                     hostname, &iterator);
   } else {
      next_queue = lFirst(queue_list);
   }
   while ((queue = next_queue)) {
      u_long32 state;

      if (hostname != NULL) {
         next_queue = lGetElemHostNext(queue_list, QU_qhostname,
                                       hostname, &iterator);
      } else {
         next_queue = lNext(queue);
      }
      state = lGetUlong(queue, QU_state);
      if ((ISSET(state, QUNKNOWN) > 0) != (new_state > 0)) {
         if (new_state) {
            SETBIT(QUNKNOWN, state);
         } else {
            CLEARBIT(QUNKNOWN, state);
         }
         lSetUlong(queue, QU_state, state);

         if (send_events) {
            sge_add_queue_event(sgeE_QUEUE_MOD, queue);
         }
      }
   }
}

