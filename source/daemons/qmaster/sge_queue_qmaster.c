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
#include "def.h"
#include "commlib.h"
#include "sge_jobL.h"
#include "sge_ja_task.h"
#include "sge_hostL.h"
#include "sge_complexL.h"
#include "sge_queueL.h"
#include "sge_requestL.h"
#include "sge_calendarL.h"
#include "sge_pe.h"
#include "sge_userprjL.h"
#include "sge_usersetL.h"
#include "sge_peL.h"
#include "sge_ckptL.h"
#include "sgermon.h"
#include "sge_prog.h"
#include "sge_host.h"
#include "sge_complex.h"
#include "slots_used.h"
#include "sge_feature.h"
#include "sge_c_gdi.h"
#include "sge_host_qmaster.h"
#include "sge_queue_qmaster.h"
#include "subordinate_qmaster.h"
#include "sge_userset_qmaster.h"
#include "sge_select_queue.h"
#include "sge_calendar_qmaster.h"
#include "sge_m_event.h"
#include "read_write_host.h"
#include "read_write_queue.h"
#include "sge_parse_num_par.h"
#include "complex_history.h"
#include "sge_signal.h"
#include "opt_history.h"
#include "sge_string.h"
#include "sge_log.h"
#include "sge_time.h"
#include "resolve_host.h"
#include "gdi_utility_qmaster.h"
#include "sge_qmod_qmaster.h"
#include "config_file.h"
#include "sge_userprj_qmaster.h"
#include "read_write_job.h"
#include "sge_job.h"
#include "sge_unistd.h"
#include "sge_hostname.h"
#include "sge_answer.h"
#include "sge_queue.h"

#include "msg_common.h"
#include "msg_qmaster.h"

#ifdef QIDL
#include "qidl_c_gdi.h"
#endif

static int mod_queue_attributes(lList **alpp, lListElem *new_queue, lListElem *qep, int add, int sub_command);


static void clear_unknown(lListElem *qep);

static u_long32 sge_get_queue_number(void);

extern lList *Master_Ckpt_List;

extern lList *Master_Complex_List;
extern lList *Master_Job_List;
extern lList *Master_Project_List;

/* ------------------------------------------------------------

   written for: qmaster at setup timeA

   sets unknown state for all queues
*/
void set_queues_to_unknown()
{
   u_long32 state;
   lListElem *qep;

   DENTER(TOP_LAYER, "set_queues_to_unknown");

   for_each(qep, Master_Queue_List) {
      state = lGetUlong(qep, QU_state);
      SETBIT(QUNKNOWN, state);
      lSetUlong(qep, QU_state, state);
   }

   DEXIT;
   return;
}        

/* ------------------------------------------------------------

   written for: qmaster when adding new queues

   clears unknown state for queue if load values are available
*/
static void clear_unknown(
lListElem *qep 
) {
   lListElem *ep, *hep;

   DENTER(TOP_LAYER, "clear_unknown");

   if (!(hep = sge_locate_host(lGetHost(qep, QU_qhostname), SGE_EXECHOST_LIST))) {
      ERROR((SGE_EVENT, MSG_JOB_UNABLE2FINDHOST_S, lGetHost(qep, QU_qhostname)));
      DEXIT;
      return;
   }

   for_each (ep, lGetList(hep, EH_load_list)) {
      if (!sge_is_static_load_value(lGetString(ep, HL_name))) {
         u_long32 state = lGetUlong(qep, QU_state);
         CLEARBIT(QUNKNOWN, state);
         lSetUlong(qep, QU_state, state);
         DPRINTF(("QUEUE %s CLEAR UNKNOWN STATE\n", lGetString(qep, QU_qname)));
         DEXIT;
         return;
      }
   }

   DPRINTF(("no non-static load value for host %s of queue %s\n",
         lGetHost(qep, QU_qhostname), lGetString(qep, QU_qname)));

   DEXIT;
   return;
}

static u_long32 sge_get_queue_number()
{
   static u_long32 queue_number=1;
   int start = queue_number;

   while (1) {
      if (queue_number > MAX_SEQNUM)
         queue_number = 1;

      if (!lGetElemUlong(Master_Queue_List, QU_queue_number, queue_number))
         return queue_number++;

      queue_number++;

      if (start == queue_number) {
         return 0;      /* out of queue numbers -> real awkward */
      }
   }
}

lListElem *sge_locate_queue(
const char *cp 
) {
   DENTER(BASIS_LAYER, "sge_locate_queue");

   if (!cp) {
      DEXIT;
      return NULL;
   }

   DEXIT;
   return (lGetElemStr(Master_Queue_List, QU_qname, cp));
}

void sge_clear_tags()
{
   lListElem *qep;

   for_each(qep, Master_Queue_List)
      lSetUlong(qep, QU_tagged, 0);
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
         ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetHost(qep, QU_qhostname)));
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
      if (verify_complex_list(alpp, "queue", qname, lGetList(qep, QU_complex_list))!=STATUS_OK)
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
   if (lGetPosViaElem(qep, QU_qtype)>=0) {
      /* special case for transfer queues:
         The hostname entered is the name of the host the qstd runs on. But qstd
         gives us loadvalues from the queueing system that lies behind. So we need
         a pseudohost object storing this load values. The name of the pseudohost 
         is pseudo.<queuename> */
      u_long32 qtype;

      qtype = lGetUlong(qep, QU_qtype);
      if (qtype & TQ) {
         lListElem *hel;
         char pseudohostname[MAXHOSTLEN], qhostname[MAXHOSTLEN];
         
         /* add the exechost for the real hostname */
         strcpy(qhostname, lGetHost(new_queue, QU_qhostname));
         if (!sge_locate_host(qhostname, SGE_EXECHOST_LIST))
            sge_add_host_of_type(qhostname, SGE_EXECHOST_LIST);

         /* enter the pseudohostname to the queue */
         lSetHost(new_queue, QU_qhostname, pseudohostname);

         /* enter the real host name to the pseudo host object */
         hel = sge_locate_host(pseudohostname, SGE_EXECHOST_LIST);
         lSetString(hel, EH_real_name, qhostname);
      }
      if (sub_command == SGE_GDI_APPEND || sub_command == SGE_GDI_CHANGE) {
         qtype = lGetUlong(new_queue, QU_qtype) | qtype;
      } else if (sub_command == SGE_GDI_REMOVE) {
         qtype = lGetUlong(new_queue, QU_qtype) & (~qtype);
      }
      if (!qtype) {
         ERROR((SGE_EVENT, MSG_AT_LEASTONEQTYPE));
         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EEXIST;  
      }
      lSetUlong(new_queue, QU_qtype, qtype);
   }

   /* ---- QU_rerun */
   attr_mod_ulong(qep, new_queue, QU_rerun, "rerun");

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
      if (verify_acl_list(alpp, lGetList(qep, QU_acl), "user_lists", 
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
      if (verify_acl_list(alpp, lGetList(qep, QU_xacl), "xuser_lists", 
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
      if (nc && !(new_cal = sge_locate_calendar(nc))) {
         
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

      if (add)
         queue_initial_state(new_queue, NULL);
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
      lSetUlong(new_queue, QU_queue_number, sge_get_queue_number());
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
   old_queue = sge_locate_queue(qname);

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
      cull_write_qconf(1, 0, QUEUE_DIR, lGetString(new_queue, QU_qname), NULL, new_queue)) {
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
   sge_add_queue(new_queue);

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
      if (!sge_locate_host(lGetHost(new_queue, QU_qhostname), SGE_EXECHOST_LIST) &&
         !(lGetUlong(new_queue, QU_qtype) & TQ))
         sge_add_host_of_type(lGetHost(new_queue, QU_qhostname), SGE_EXECHOST_LIST);
      clear_unknown(new_queue);
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
            sge_add_jatask_event(sgeE_JATASK_MOD, job, ja_task);
            job_write_spool_file(job, lGetUlong(ja_task, JAT_task_number), NULL, SPOOL_DEFAULT);
         }
      }
   }            

   /* send an additional event concerning this queue to schedd */
   /* the first one is sent whithin sge_change_queue_version() */
   sge_add_queue_event(sgeE_QUEUE_MOD, new_queue);

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
   u_long32 qtype;

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

   /* delete queue, referenced in Master_Ckpt_List is not allowed */
   {
      lList*     qlist;
      lListElem* qliste;
      lListElem* ckpte;
      const char* chkpt_qname = NULL;
      const char* chkpt_name = NULL;


      for_each (ckpte, Master_Ckpt_List) {
         qlist = lGetList(ckpte, CK_queue_list);
         for_each (qliste, qlist) {
            chkpt_qname = lGetString(qliste, QR_name);
            if ((chkpt_qname != NULL) && (qname != NULL)) {
               if (strcmp(chkpt_qname, qname) == 0) {
                  chkpt_name = lGetString(ckpte, CK_name );
                  if (chkpt_name == NULL) {
                     chkpt_name = MSG_OBJ_UNKNOWN;
                  } 
                  ERROR((SGE_EVENT, MSG_UNABLETODELQUEUEXREFERENCEDINCHKPTY_SS, qname,chkpt_name ));
                  answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, 0);  
                  DEXIT;
                  return STATUS_ESEMANTIC;
               }
            } 
         }
      }
   } 

   /* delete queue, referenced in Master_Pe_List is not allowed */
   {
      lList*     qlist;
      lListElem* qliste;
      lListElem* pee;
      const char* pe_qname = NULL;
      const char* pe_name = NULL;

      for_each (pee,Master_Pe_List ) {
         qlist = lGetList(pee, PE_queue_list);
         for_each (qliste, qlist) {
            pe_qname = lGetString(qliste, QR_name);
            if ((pe_qname != NULL) && (qname != NULL)) {
               if (strcmp(pe_qname, qname) == 0) {
                  pe_name = lGetString(pee, PE_name );
                  if (pe_name == NULL) {
                     pe_name = MSG_OBJ_UNKNOWN;
                  } 
                  ERROR((SGE_EVENT, MSG_UNABLETODELQUEUEXREFERENCEDINPEY_SS, qname,pe_name ));
                  answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, 0);  
                  DEXIT;
                  return STATUS_ESEMANTIC;
               }
            } 
         }
      }
   }  





   if (!(oqep = sge_locate_queue(qname))) {
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

   qtype = lGetUlong(oqep, QU_qtype);

   if (sge_del_queue(qname)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, MSG_OBJ_QUEUE, qname));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, 0); 
      DEXIT;
      return STATUS_EEXIST;
   }

   /* generate a sgeE_QUEUE_DEL event and queue it into the event list */
   sge_add_event(NULL, sgeE_QUEUE_DEL, 0, 0, qname, NULL);
  
   sge_unlink(QUEUE_DIR, qname); 

   unsuspend_all(sos_list_before, 0);
   lFreeList(sos_list_before); 

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS,
         ruser, rhost, qname, MSG_OBJ_QUEUE));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   DEXIT;
   return STATUS_OK;

}


#if 0
/* verify that all project names 
   in the queues project list exist */
int verify_project_list(
lList **alpp,
char *obj_name,
char *qname,
lList *project_list  /* UP_Type */
) {
   lListElem *pep;

   DENTER(TOP_LAYER, "verify_project_list");

   for_each (pep, project_list) {
      if (!lGetElemStr(Master_Project_List, UP_name, lGetString(pep, UP_name))) {
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWNPROJECT_SSS, 
               lGetString(pep, UP_name), obj_name, qname));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }

   DEXIT;
   return STATUS_OK;
}
#endif


int verify_complex_list(
lList **alpp,
const char *obj_name,
const char *qname,
lList *complex_list  /* CX_Type */
) {
   lListElem *cep;
   const char *s;
   int ret = STATUS_OK;

   DENTER(TOP_LAYER, "verify_complex_list");

   for_each (cep, complex_list) {
      s = lGetString(cep, CX_name);

      /* it is not allowed to put standard complexes into a complex list */
      if (!strcmp(s, "global") ||
          !strcmp(s, "host")   ||
          !strcmp(s, "queue")) {
         ERROR((SGE_EVENT, MSG_SGETEXT_COMPLEXNOTUSERDEFINED_SSS,
               s, obj_name, qname));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         ret = STATUS_EUNKNOWN;
      } 

      /* verify that all complex names in the queues complex list exist */
      if (!lGetElemStr(Master_Complex_List, CX_name, s)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWNCOMPLEX_SSS, 
               s, obj_name, qname));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         ret = STATUS_EUNKNOWN;
      }
   }

   DEXIT;
   return ret;
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

   /*
   ** an increase in the queue version triggers the
   ** generation of a new history file
   ** any event that is worth increasing the version count
   ** should also be worth writing a history version
   */
   if (write_history && !is_nohist() && sge_write_queue_history(qep)) {
      WARNING((SGE_EVENT, MSG_CONFIG_CANTWRITEHISTORYFORQUEUEX_S,
               lGetString(qep, QU_qname)));
      DEXIT;
      return -1;
   }

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

   qep = sge_locate_queue(qname);
   if (!qep) {
      ERROR((SGE_EVENT, MSG_QUEUE_CANTLOCATEQUEUEX_S, qname));
      DEXIT;
      return -1;
   }

   lRemoveElem(Master_Queue_List, qep);

   DEXIT;
   return 0;
}

/* -----------------------------------
   sge_add_queue - adds a queue to the 
     Master_Queue_List. 

   returns:
   -1 if queue already exists or other error;
   0 if successful;
*/
int sge_add_queue(
lListElem *qep 
) {
   static lSortOrder *so = NULL;

   DENTER(TOP_LAYER, "sge_add_queue");

   if (!qep) {
      ERROR((SGE_EVENT, MSG_QUEUE_NULLPTR));
      DEXIT;
      return -1;
   }

   /* create SortOrder: */
   if(!so) {
      so = lParseSortOrderVarArg(QU_Type, "%I+", QU_qname);
   };
  
   /* insert Element: */
   if(!Master_Queue_List)
      Master_Queue_List = lCreateList("Master_Queue_List", QU_Type);
   lInsertSorted(so, qep, Master_Queue_List);

   DEXIT;
   return 0;
}


void sge_add_queue_event(
u_long32 type,
lListElem *qep 
) {
   DENTER(TOP_LAYER, "sge_add_queue_event");
   sge_add_event(NULL, type, 0, 0, lGetString(qep, QU_qname), qep);
   DEXIT;
   return;
}

/* verify that all queue names in a 
   QR_Type list refer to existing queues */
int verify_qr_list(
lList **alpp,
lList *qr_list,
const char *attr_name, /* e.g. "queue_list" */
const char *obj_descr, /* e.g. "parallel environment", "ckpt interface" */
const char *obj_name   /* e.g. "pvm", "hibernator"  */
) {
   lListElem *qrep;
   int all_name_exists = 0;
   int queue_exist = 0;

   DENTER(TOP_LAYER, "verify_qr_list");

   for_each (qrep, qr_list) {
      if (!strcasecmp(lGetString(qrep, QR_name), SGE_ATTRVAL_ALL)) {
         lSetString(qrep, QR_name, SGE_ATTRVAL_ALL);
         all_name_exists = 1;
      } else if (!sge_locate_queue(lGetString(qrep, QR_name))) {
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWNQUEUE_SSSS, 
            lGetString(qrep, QR_name), attr_name, obj_descr, obj_name));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      } else {
         queue_exist = 1;
      }
      if (all_name_exists && queue_exist) {
         ERROR((SGE_EVENT, MSG_SGETEXT_QUEUEALLANDQUEUEARENOT_SSSS,
            SGE_ATTRVAL_ALL, attr_name, obj_descr, obj_name));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }

   DEXIT;
   return STATUS_OK;
}
