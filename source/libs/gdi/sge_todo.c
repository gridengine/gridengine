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

#include "sge.h"
#include "sgermon.h"
#include "basis_types.h"
#include "sge_ja_task.h"
#include "sge_answer.h"
#include "sge_log.h"
#include "sge_suserL.h"
#include "sge_job.h"
#include "sge_suser.h"
#include "sge_queue.h"
#include "sge_pe.h"
#include "sge_ckpt.h"
#include "sge_todo.h"
#include "sge_utility.h"
#include "sge_string.h"
#include "sge_userset.h"
#include "config_file.h"
#include "sge_m_event.h"
#include "gdi_utility.h"
#include "sge_signal.h"
#include "sge_userprj.h"
#include "sge_pe_task.h"
#include "sge_manop.h"
#include "sge_host.h"
#include "sge_complex.h"
#include "sge_sharetree.h"
#include "sge_answer.h"
#include "version.h"
#include "sge_schedd_conf.h"
#include "sge_conf.h"
#include "sge_calendar.h"
#include "sge_report.h"

#include "msg_common.h"
#include "msg_qmaster.h"
#include "msg_gdilib.h"
#include "msg_sgeobjlib.h"

static int job_update_master_list_usage(lListElem *event);

/****** gdi/queue/queue_list_set_unknown_state_to() ************************
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

/* JG: TODO: is it really needed? */
void sge_add_queue_event(u_long32 type, lListElem *qep)
{
   DENTER(TOP_LAYER, "sge_add_queue_event");
   sge_add_event(NULL, 0, type, 0, 0, lGetString(qep, QU_qname), qep);
   DEXIT;
   return;
}


/****** gdi/ckpt/validate_ckpt() ******************************************
*  NAME
*     validate_ckpt -- validate all ckpt interface parameters
*
*  SYNOPSIS
*     int validate_ckpt(lListElem *ep, lList **alpp);
*
*  FUNCTION
*     This function will test all ckpt interface parameters.
*     If all are valid then it will return successfull.
*
*
*  INPUTS
*     ep     - element which sould be verified.
*     answer - answer list where the function stored error messages
*
*
*  RESULT
*     [answer] - error messages will be added to this list
*     STATUS_OK - success
*     STATUS_EUNKNOWN or STATUS_EEXIST - error
******************************************************************************/
int validate_ckpt(lListElem *ep, lList **alpp)
{
   static char* ckpt_interfaces[] = {
      "USERDEFINED",
      "HIBERNATOR",
      "TRANSPARENT",
      "APPLICATION-LEVEL",
      "CPR",
      "CRAY-CKPT"
   };
   static struct attr {
      int nm;
      char *text;
   } ckpt_commands[] = {
      { CK_ckpt_command, "ckpt_command" },
      { CK_migr_command, "migr_command" },
      { CK_rest_command, "restart_command"},
      { CK_clean_command, "clean_command"},
      { NoName,           NULL} };

   int i;
   int found = 0;
   const char *s, *interface;

   DENTER(TOP_LAYER, "validate_ckpt_obj");

   if (!ep) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* -------- CK_name */
   if (verify_str_key(alpp, lGetString(ep, CK_name), "checkpoint interface")) {
      DEXIT;
      return STATUS_EUNKNOWN;
   }



   /*
   ** check if ckpt obj can be added
   ** check allowed interfaces and license
   */
   if ((interface = lGetString(ep, CK_interface))) {
      found = 0;
      for (i=0; i < (sizeof(ckpt_interfaces)/sizeof(char*)); i++) {
         if (!strcasecmp(interface, ckpt_interfaces[i])) {
            found = 1;
            break;
         }
      }

      if (!found) {
         ERROR((SGE_EVENT, MSG_SGETEXT_NO_INTERFACE_S, interface));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EEXIST;
      }

#ifdef PW
      /* license check */
      if (!set_licensed_feature("ckpt")) {
         if (!strcasecmp(interface, "HIBERNATOR") ||
             !strcasecmp(interface, "CPR") ||
             !strcasecmp(interface, "APPLICATION-LEVEL") ||
             !strcasecmp(interface, "CRAY-CKPT")) {
            ERROR((SGE_EVENT, MSG_SGETEXT_NO_CKPT_LIC));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         }
      }
#endif
   }

   for (i=0; ckpt_commands[i].nm!=NoName; i++) {
      if (replace_params(lGetString(ep, ckpt_commands[i].nm),
               NULL, 0, ckpt_variables)) {
         ERROR((SGE_EVENT, MSG_OBJ_CKPTENV_SSS,
               ckpt_commands[i].text, lGetString(ep, CK_name), err_msg));
         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EEXIST;
      }
   }

   /* -------- CK_queue_list */
   if (queue_reference_list_validate(alpp, lGetList(ep, CK_queue_list), MSG_OBJ_QLIST,
               MSG_OBJ_CKPTI, lGetString(ep, CK_name))!=STATUS_OK) {
      DEXIT;
      return STATUS_EEXIST;
   }

   /* -------- CK_signal */
   if ((s=lGetString(ep, CK_signal)) &&
         strcasecmp(s, "none") &&
         sge_sys_str2signal(s)==-1) {
      ERROR((SGE_EVENT, MSG_CKPT_XISNOTASIGNALSTRING_S , s));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   DEXIT;
   return STATUS_OK;
}


/****** gdi/pe/pe_validate() ***************************************************
*  NAME
*     pe_validate() -- validate a parallel environment
*
*  SYNOPSIS
*     int pe_validate(int startup, lListElem *pep, lList **alpp)
*
*  FUNCTION
*     Ensures that a new pe is not a duplicate of an already existing one
*     and checks consistency of the parallel environment:
*        - pseudo parameters in start and stop proc
*        - validity of the allocation rule
*        - correctness of the queue list, the user list and the xuser list
*
*
*  INPUTS
*     int startup    - are we in qmaster startup phase?
*     lListElem *pep - the pe to check
*     lList **alpp   - answer list pointer, if an answer shall be created, else
*                      NULL - errors will in any case be output using the
*                      Grid Engine error logging macros.
*
*  RESULT
*     int - STATUS_OK, if everything is ok, else other status values,
*           see libs/gdi/sge_answer.h
*******************************************************************************/
int pe_validate(int startup, lListElem *pep, lList **alpp)
{
   const char *s;
   const char *pe_name;
   int ret;

   DENTER(TOP_LAYER, "pe_validate");

   pe_name = lGetString(pep, PE_name);
   if (pe_name && verify_str_key(alpp, pe_name, MSG_OBJ_PE)) {
      ERROR((SGE_EVENT, "Invalid character in pe name of pe "SFQ, pe_name));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* register our error function for use in replace_params() */
   config_errfunc = error;

   /* -------- start_proc_args */
   s = lGetString(pep, PE_start_proc_args);
   if (s && replace_params(s, NULL, 0, pe_variables )) {
      ERROR((SGE_EVENT, MSG_PE_STARTPROCARGS_SS, pe_name, err_msg));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }


   /* -------- stop_proc_args */
   s = lGetString(pep, PE_stop_proc_args);
   if (s && replace_params(s, NULL, 0, pe_variables )) {
      ERROR((SGE_EVENT, MSG_PE_STOPPROCARGS_SS, pe_name, err_msg));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* -------- allocation_rule */
   s = lGetString(pep, PE_allocation_rule);
   if (!s)  {
      ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(PE_allocation_rule), "validate_pe"));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   if (replace_params(s, NULL, 0, pe_alloc_rule_variables )) {
      ERROR((SGE_EVENT, MSG_PE_ALLOCRULE_SS, pe_name, err_msg));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* -------- PE_queue_list */
   if ((ret=queue_reference_list_validate(alpp, lGetList(pep, PE_queue_list), MSG_OBJ_QLIST,
               MSG_OBJ_PE, pe_name))!=STATUS_OK && !startup) {
      DEXIT;
      return ret;
   }

   /* -------- PE_user_list */
   if ((ret=userset_list_validate_acl_list(alpp, lGetList(pep, PE_user_list), MSG_OBJ_USERLIST,
               MSG_OBJ_PE, pe_name))!=STATUS_OK) {
      DEXIT;
      return ret;
   }

   /* -------- PE_xuser_list */
   if ((ret=userset_list_validate_acl_list(alpp, lGetList(pep, PE_xuser_list), MSG_OBJ_XUSERLIST,
               MSG_OBJ_PE, pe_name))!=STATUS_OK) {
      DEXIT;
      return ret;
   }

   DEXIT;
   return STATUS_OK;
}

/****** gdi/userprj/userprj_update_master_list() *****************************
*  NAME
*     userprj_update_master_list() -- update the master lists of users and proje
cts
*
*  SYNOPSIS
*     int userprj_update_master_list(sge_event_type type,
*                                    sge_event_action action,
*                                    lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the global master lists of users and projects
*     based on an event.
*     The function is called from the event mirroring interface.
*     Depending on the event received, either the user list or
*     the project list is updated.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*     Eventmirror/sge_mirror_update_master_list_str_key()
*******************************************************************************/
int userprj_update_master_list(sge_event_type type, sge_event_action action,
                              lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   int     key_nm;

   const char *key;


   DENTER(TOP_LAYER, "userprj_update_master_list");

   list_descr = UP_Type;
   key_nm = UP_name;

   switch(type) {
      case SGE_EMT_PROJECT:
         list = &Master_Project_List;
         break;
      case SGE_EMT_USER:
         list = &Master_User_List;
         break;
      default:
         return FALSE;
   }

   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_str_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}

/****** gdi/job/job_update_master_list_usage() *********************************
*  NAME
*     job_update_master_list_usage() -- update usage for a jobs tasks
*
*  SYNOPSIS
*     int job_update_master_list_usage(lListElem *event)
*
*  FUNCTION
*     Events containing usage reports are sent for a jobs tasks.
*     This can be array tasks (where a non array job has a single
*     array task) or tasks of a parallel job.
*     This function decides which type of task has to receive
*     the updated usage report and passes the event
*     information to the corresponding update functions.
*
*  INPUTS
*     lListElem *event - event object containing the new usage list
*
*  RESULT
*     int - TRUE, if the operation succeeds, else FALSE
*
*  SEE ALSO
*     gdi/ja_task/pe_task_update_master_list_usage()
*     gdi/pe_task/pe_task_update_master_list_usage()
*******************************************************************************/
static int job_update_master_list_usage(lListElem *event)
{
   int ret = TRUE;
   u_long32 job_id, ja_task_id;
   const char *pe_task_id;

   DENTER(TOP_LAYER, "job_update_master_list_usage");

   job_id = lGetUlong(event, ET_intkey);
   ja_task_id = lGetUlong(event, ET_intkey2);
   pe_task_id = lGetString(event, ET_strkey);

   if(ja_task_id == 0) {
      ERROR((SGE_EVENT, MSG_JOB_RECEIVEDINVALIDUSAGEEVENTFORJOB_S, job_get_id_string(job_id, ja_task_id, pe_task_id)));
      ret = FALSE;
   }

   if(pe_task_id != NULL) {
      ret = pe_task_update_master_list_usage(event);   /* usage for a pe task */
   } else {
      ret = ja_task_update_master_list_usage(event);   /* usage for a ja task */
   }

   DEXIT;
   return ret;
}

/****** gdi/job/job_update_master_list() *****************************
*  NAME
*     job_update_master_list() -- update the master list of jobs
*
*  SYNOPSIS
*     int job_update_master_list(sge_event_type type,
*                                     sge_event_action action,
*                                     lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the global master list of jobs
*     based on an event.
*     The function is called from the event mirroring interface.
*
*     A jobs array tasks are not updated by this function,
*     as they are maintained by separate events.
*     In addition, some scheduler specific attributes, that
*     are only used in scheduler, are not updated.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*     gdi/job/job_update_master_list_usage()
*******************************************************************************/
int job_update_master_list(sge_event_type type, sge_event_action action,
                           lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   u_long32 job_id;
   lListElem *job = NULL;
   lList *ja_tasks = NULL;

   DENTER(TOP_LAYER, "job_update_master_list");

   list = &Master_Job_List;
   list_descr = JB_Type;
   job_id = lGetUlong(event, ET_intkey);
   job = job_list_locate(*list, job_id);

   if(action == SGE_EMA_MOD) {
      u_long32 event_type = lGetUlong(event, ET_type);

      if(job == NULL) {
         ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBFORUPDATEIN_SS,
                job_get_id_string(job_id, 0, NULL), "job_update_master_list"));
         DEXIT;
         return FALSE;
      }

#if 0
      if (event_type == sgeE_JOB_FINISH) {
         /* no direct impact on master list */
         ;
      } else
#endif
      if (event_type == sgeE_JOB_USAGE || event_type == sgeE_JOB_FINAL_USAGE ) {
         /* special handling needed for JOB_USAGE and JOB_FINAL_USAGE events.
         * they are sent for jobs, ja_tasks and pe_tasks and only contain
         * the usage list.
         * Preferable would probably be to send MOD events for the different
         * object types.
         */
         int ret = job_update_master_list_usage(event);
         DEXIT;
         return ret;
      } else {
         /* this is the true modify event.
          * we may not update several fields:
          * - JB_ja_tasks is the task list - it is maintained by JATASK events
          * - JB_jobclass, JB_host and JB_category are scheduler internal attributes
          *   they may not be overwritten.
          *   Better would be to move them from JB_Type to some scheduler specific
          *   object.
          */

          lListElem *modified_job;

          modified_job = lFirst(lGetList(event, ET_new_version));
          if(job != NULL && modified_job != NULL) {
            lXchgList(modified_job, JB_ja_tasks, &ja_tasks);
            lSetString(modified_job, JB_jobclass, lGetString(job, JB_jobclass));
            lSetHost(modified_job, JB_host, lGetHost(job, JB_host));
            lSetRef(modified_job, JB_category, lGetRef(job, JB_category));
          }
      }
   }

   if(sge_mirror_update_master_list(list, list_descr, job, job_get_id_string(job_id, 0, NULL), action, event) != SGE_EM_OK) {
      lFreeList(ja_tasks);
      DEXIT;
      return FALSE;
   }

   /* restore ja_task list after modify event */
   if(action == SGE_EMA_MOD && ja_tasks != NULL) {
      /* we have to search the replaced job */
      job = job_list_locate(*list, job_id);
      if(job == NULL) {
         ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBFORUPDATEIN_SS,
                job_get_id_string(job_id, 0, NULL), "job_update_master_list"));
         lFreeList(ja_tasks);
         DEXIT;
         return FALSE;
      }

      lXchgList(job, JB_ja_tasks, &ja_tasks);
      ja_tasks = lFreeList(ja_tasks);
   }

   DEXIT;
   return TRUE;
}

/****** gdi/manop/manop_update_master_list() *****************************
*  NAME
*     manop_update_master_list() -- update the master list of managers/operators
*
*  SYNOPSIS
*     int manop_update_master_list(sge_event_type type,
*                                  sge_event_action action,
*                                  lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the global master lists of managers and operators
*     based on an event.
*     The function is called from the event mirroring interface.
*     Depending on the event, either the manager or the operator
*     list is updated.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*     Eventmirror/sge_mirror_update_master_list_str_key()
*******************************************************************************/
int manop_update_master_list(sge_event_type type, sge_event_action action,
                             lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   int     key_nm;

   const char *key;


   DENTER(TOP_LAYER, "manop_update_master_list");

   list_descr = MO_Type;
   key_nm = MO_name;

   switch(type) {
      case SGE_EMT_MANAGER:
         list = &Master_Manager_List;
         break;
      case SGE_EMT_OPERATOR:
         list = &Master_Operator_List;
         break;
      default:
         return FALSE;
   }

   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_str_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}

/****** gdi/host/host_update_master_list() *****************************
*  NAME
*     host_update_master_list() -- update the master hostlists
*
*  SYNOPSIS
*     int host_update_master_list(sge_event_type type,
*                                 sge_event_action action,
*                                 lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the global master lists of hosts
*     based on an event.
*     The function is called from the event mirroring interface.
*     Updates admin, submit or execution host list depending
*     on the event received.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*     Eventmirror/sge_mirror_update_master_list_host_key()
*******************************************************************************/
int host_update_master_list(sge_event_type type, sge_event_action action,
                            lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   int     key_nm;

   const char *key;


   DENTER(TOP_LAYER, "host_update_master_list");

   switch(type) {
      case SGE_EMT_ADMINHOST:
         list = &Master_Adminhost_List;
         list_descr = AH_Type;
         key_nm = AH_name;
         break;
      case SGE_EMT_EXECHOST:
         list = &Master_Exechost_List;
         list_descr = EH_Type;
         key_nm = EH_name;
         break;
      case SGE_EMT_SUBMITHOST:
         list = &Master_Submithost_List;
         list_descr = SH_Type;
         key_nm = SH_name;
         break;
      default:
         return FALSE;
   }
 
   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_host_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}

/****** gdi/complex/complex_update_master_list() *****************************
*  NAME
*     complex_update_master_list() -- update the master list of complexes
*
*  SYNOPSIS
*     int complex_update_master_list(sge_event_type type,
*                                    sge_event_action action,
*                                    lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the global master list of complexes based on an
*     event.
*     The function is called from the event mirroring interface.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*     Eventmirror/sge_mirror_update_master_list_str_key()
*******************************************************************************/
int complex_update_master_list(sge_event_type type, sge_event_action action,
                               lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   int     key_nm;

   const char *key;


   DENTER(TOP_LAYER, "complex_update_master_list");

   list = &Master_Complex_List;
   list_descr = CX_Type;
   key_nm = CX_name;

   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_str_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}

/****** gdi/pe/pe_update_master_list() *****************************
*  NAME
*     pe_update_master_list() -- update the master list of parallel environments
*
*  SYNOPSIS
*     int pe_update_master_list(sge_event_type type,
*                                     sge_event_action action,
*                                     lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the global master list of parallel environments
*     based on an event.
*     The function is called from the event mirroring interface.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*     Eventmirror/sge_mirror_update_master_list_str_key()
*******************************************************************************/
int pe_update_master_list(sge_event_type type, sge_event_action action,
                          lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   int     key_nm;

   const char *key;


   DENTER(TOP_LAYER, "pe_update_master_list");

   list = &Master_Pe_List;
   list_descr = PE_Type;
   key_nm = PE_name;

   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_str_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}

/****** gdi/queue/queue_update_master_list() *****************************
*  NAME
*     queue_update_master_list() -- update the master list of queues
*
*  SYNOPSIS
*     int queue_update_master_list(sge_event_type type,
*                                  sge_event_action action,
*                                  lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the global master list of queues
*     based on an event.
*     The function is called from the event mirroring interface.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*     Eventmirror/sge_mirror_update_master_list_str_key()
*******************************************************************************/
int queue_update_master_list(sge_event_type type, sge_event_action action,
                             lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   lListElem *queue;
   int     key_nm;

   const char *key;


   DENTER(TOP_LAYER, "queue_update_master_list");

   list = &Master_Queue_List;
   list_descr = QU_Type;
   key_nm = QU_qname;

   key = lGetString(event, ET_strkey);

   queue = queue_list_locate(*list, key);

   if(action == SGE_EMA_MOD) {
      u_long32 type = lGetUlong(event, ET_type);

      if(type == sgeE_QUEUE_SUSPEND_ON_SUB || type == sgeE_QUEUE_UNSUSPEND_ON_SUB) {
         u_long32 state;

         if(queue == NULL) {
            ERROR((SGE_EVENT, MSG_QUEUE_CANTFINDQUEUEFORUPDATEIN_SS, key, "queue_update_master_list"));
            DEXIT;
            return FALSE;
         }

         state = lGetUlong(queue, QU_state);

         if (type == sgeE_QUEUE_SUSPEND_ON_SUB) {
            state |= QSUSPENDED_ON_SUBORDINATE;      /* set this bit */
         } else {
            state &= ~QSUSPENDED_ON_SUBORDINATE;     /* reset this bit */
         }
         lSetUlong(queue, QU_state, state);

         DEXIT;
         return TRUE;
      }
   }

   if(sge_mirror_update_master_list(list, list_descr, queue, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}

/****** gdi/sharetree/sharetree_update_master_list() *****************************
*  NAME
*     sharetree_update_master_list() -- update the master sharetree list
*
*  SYNOPSIS
*     int sharetree_update_master_list(sge_event_type type,
*                                      sge_event_action action,
*                                      lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the global master list for the sharetree
*     based on an event.
*     The function is called from the event mirroring interface.
*     Sharetree events always contain the whole sharetree, that
*     replaces an existing sharetree in the master list.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*******************************************************************************/
int sharetree_update_master_list(sge_event_type type, sge_event_action action,
                                 lListElem *event, void *clientdata)
{
   lList *src;

   DENTER(TOP_LAYER, "sharetree_update_master_list");

   /* remove old share tree */
   Master_Sharetree_List = lFreeList(Master_Sharetree_List);

   if ((src = lGetList(event, ET_new_version))) {
      /* install new one */
      Master_Sharetree_List = lCreateList("share tree",
        lGetElemDescr(lFirst(lGetList(event, ET_new_version))));
      lAppendElem(Master_Sharetree_List, lDechainElem(src, lFirst(src)));
   }

   DEXIT;
   return TRUE;
}

/****** gdi/schedd_conf/schedd_conf_update_master_list() *****************************
*  NAME
*     schedd_conf_update_master_list() -- update the master list of scheduler configurations
*
*  SYNOPSIS
*     int schedd_conf_update_master_list(sge_event_type type,
*                                        sge_event_action action,
*                                        lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the global master list of scheduler configurations
*     based on an event.
*     The function is called from the event mirroring interface.
*     The list only contains one element that is replaced when a
*     modify event arrives.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*******************************************************************************/
int schedd_conf_update_master_list(sge_event_type type, sge_event_action action,
                                       lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;

   lList *data_list;
   lListElem *ep = NULL;

   DENTER(TOP_LAYER, "schedd_conf_update_master_list");

   list = &Master_Sched_Config_List;
   list_descr = SC_Type;

   /* We always update the whole list (consisting of one list element) */
   if(*list != NULL) {
      *list = lFreeList(*list);
   }

   if((data_list = lGetList(event, ET_new_version)) != NULL) {
      if((ep = lFirst(data_list)) != NULL) {
         ep = lDechainElem(data_list, ep);
      }
   }

   /* if neccessary, create list and copy schedd info */
   if(ep != NULL) {
      *list = lCreateList("schedd config", list_descr);
      lAppendElem(*list, ep);
   }

   DEXIT;
   return TRUE;
}

/****** gdi/config/config_update_master_list() *****************************
*  NAME
*     config_update_master_list() -- update the master list of configurations
*
*  SYNOPSIS
*     int config_update_master_list(sge_event_type type,
*                                     sge_event_action action,
*                                     lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the global master list of host or global configurations
*     based on an event.
*     The function is called from the event mirroring interface.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*     Eventmirror/sge_mirror_update_master_list_host_key()
*******************************************************************************/
int config_update_master_list(sge_event_type type, sge_event_action action,
                              lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   int     key_nm;

   const char *key;


   DENTER(TOP_LAYER, "config_update_master_list");

   list = &Master_Config_List;
   list_descr = CONF_Type;
   key_nm = CONF_hname;

   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_host_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   /* JG: TODO: Anything else to do? Call some function parsing the config? */

   DEXIT;
   return TRUE;
}

/****** gdi/ja_task/ja_task_update_master_list_usage() *************************
*  NAME
*     ja_task_update_master_list_usage() -- update an array tasks usage
*
*  SYNOPSIS
*     int ja_task_update_master_list_usage(lListElem *event)
*
*  FUNCTION
*     Updates the scaled usage of an array task (also task data structure
*     of a non array job).
*
*  INPUTS
*     lListElem *event - event object containing the new usage list
*
*  RESULT
*     int - TRUE, if the operation succeeds, else FALSE
*
*  SEE ALSO
*     gdi/job/job_update_master_list_usage()
*     gdi/ja_task/pe_task_update_master_list_usage()
*******************************************************************************/
int ja_task_update_master_list_usage(lListElem *event)
{
   lList *tmp = NULL;
   u_long32 job_id, ja_task_id;
   lListElem *job, *ja_task;

   DENTER(TOP_LAYER, "ja_task_update_master_list_usage");

   job_id = lGetUlong(event, ET_intkey);
   ja_task_id = lGetUlong(event, ET_intkey2);

   job = job_list_locate(Master_Job_List, job_id);
   if(job == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBFORUPDATEIN_SS,
             job_get_id_string(job_id, 0, NULL), "ja_task_update_master_list_usage"));
      DEXIT;
      return FALSE;
   }

   ja_task = job_search_task(job, NULL, ja_task_id);
   if(ja_task == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDJATASKFORUPDATEIN_SS,
             job_get_id_string(job_id, ja_task_id, NULL), "ja_task_update_master_list_usage"));
      DEXIT;
      return FALSE;
   }

   lXchgList(event, ET_new_version, &tmp);
   lXchgList(ja_task, JAT_scaled_usage_list, &tmp);
   lXchgList(event, ET_new_version, &tmp);

   DEXIT;
   return TRUE;
}

/****** gdi/ja_task/ja_task_update_master_list() *****************************
*  NAME
*     ja_task_update_master_list() -- update array tasks of a job
*
*  SYNOPSIS
*     int ja_task_update_master_list(sge_event_type type,
*                                    sge_event_action action,
*                                    lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the list of array tasks of a job
*     based on an event.
*     The function is called from the event mirroring interface.
*
*     An array tasks list of parallel tasks and the
*     scaled usage list are not updated in this function,
*     as this data is maintained by separate events.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*******************************************************************************/
int ja_task_update_master_list(sge_event_type type, sge_event_action action,
                               lListElem *event, void *clientdata)
{
   u_long32 job_id, ja_task_id;
   lListElem *job, *ja_task;

   lList *list;
   lDescr *list_descr;

   lList *pe_tasks = NULL;
   lList *usage = NULL;

   DENTER(TOP_LAYER, "ja_task_update_master_list");

   job_id = lGetUlong(event, ET_intkey);
   ja_task_id = lGetUlong(event, ET_intkey2);

   job = job_list_locate(Master_Job_List, job_id);
   if(job == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBFORUPDATEIN_SS,
             job_get_id_string(job_id, 0, NULL), "ja_task_update_master_list"));
      DEXIT;
      return FALSE;
   }

   ja_task = job_search_task(job, NULL, ja_task_id);

   list = lGetList(job, JB_ja_tasks);
   list_descr = JAT_Type;

   if(action == SGE_EMA_MOD) {
      /* modify event for ja_task.
       * we may not update
       * - JAT_task_list - it is maintained by PETASK events
       * - JAT_scaled_usage - it is maintained by JOB_USAGE events
       */
      if(ja_task == NULL) {
         ERROR((SGE_EVENT, MSG_JOB_CANTFINDJATASKFORUPDATEIN_SS,
                job_get_id_string(job_id, ja_task_id, NULL), "ja_task_update_master_list"));
         DEXIT;
         return FALSE;
      }

      lXchgList(ja_task, JAT_task_list, &pe_tasks);
      lXchgList(ja_task, JAT_scaled_usage_list, &usage);
   }

   /* If an array job is deleted, a delete event is sent for
    * each ja_task. If it is not yet enrolled,
    * sge_mirror_update_master_list will fail.
    * If it is not enrolled, but in the range list
    * for pending tasks, remove it from ranges.
    */
   if(action == SGE_EMA_DEL) {
      if(ja_task == NULL &&
         job_is_ja_task_defined(job, ja_task_id) &&
         (!job_is_enrolled(job, ja_task_id))
        ) {
         job_delete_not_enrolled_ja_task(job, NULL, ja_task_id);
         DEXIT;
         return TRUE;
      }
   }

   if(sge_mirror_update_master_list(&list, list_descr, ja_task, job_get_id_string(job_id, ja_task_id, NULL), action, event) != SGE_EM_OK) {
      lFreeList(pe_tasks);
      lFreeList(usage);
      DEXIT;
      return FALSE;
   }

   /* restore pe_task list after modify event */
   if(action == SGE_EMA_MOD) {
      /* we have to search the replaced ja_task */
      ja_task = job_search_task(job, NULL, ja_task_id);
      if(ja_task == NULL) {
         ERROR((SGE_EVENT, MSG_JOB_CANTFINDJATASKFORUPDATEIN_SS,
                job_get_id_string(job_id, ja_task_id, NULL), "ja_task_update_master_list"));
         lFreeList(pe_tasks);
         lFreeList(usage);
         DEXIT;
         return FALSE;
      }

      lXchgList(ja_task, JAT_task_list, &pe_tasks);
      lXchgList(ja_task, JAT_scaled_usage_list, &usage);
      pe_tasks = lFreeList(pe_tasks);
      usage = lFreeList(usage);
   }

   if(action == SGE_EMA_ADD) {
      /* first jatask add event could have created new ja_task list for job */
      if(lGetList(job, JB_ja_tasks) == NULL && list != NULL) {
         lSetList(job, JB_ja_tasks, list);
      }
      /* we must enroll the task to have it removed in the pending range list */
      job_enroll(job, NULL, ja_task_id);
   }

   DEXIT;
   return TRUE;
}

/****** gdi/ckpt/ckpt_update_master_list() *****************************
*  NAME
*     ckpt_update_master_list() -- update the master list of checkpoint environments
*
*  SYNOPSIS
*     int ckpt_update_master_list(sge_event_type type,
*                                 sge_event_action action,
*                                 lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the global master list of checkpoint environments based on an
*     event.
*     The function is called from the event mirroring interface.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*     Eventmirror/sge_mirror_update_master_list_str_key()
*******************************************************************************/
int ckpt_update_master_list(sge_event_type type, sge_event_action action,
                            lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   int     key_nm;

   const char *key;


   DENTER(TOP_LAYER, "ckpt_update_master_list");

   list = &Master_Ckpt_List;
   list_descr = CK_Type;
   key_nm = CK_name;

   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_str_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}

/****** gdi/calendar/calendar_update_master_list() *****************************
*  NAME
*     calendar_update_master_list() -- update the master list of calendars
*
*  SYNOPSIS
*     int calendar_update_master_list(sge_event_type type,
*                                     sge_event_action action,
*                                     lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the global master list of calendars
*     based on an event.
*     The function is called from the event mirroring interface.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*     Eventmirror/sge_mirror_update_master_list_str_key()
*     Eventmirror/sge_mirror_update_master_list_host_key()
*******************************************************************************/
int calendar_update_master_list(sge_event_type type, sge_event_action action,
                                lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   int     key_nm;

   const char *key;


   DENTER(TOP_LAYER, "calendar_update_master_list");

   list = &Master_Calendar_List;
   list_descr = CAL_Type;
   key_nm = CAL_name;

   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_str_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}

/****** gdi/report/report_list_send() ******************************************
*  NAME
*     report_list_send() -- Send a list of reports.
*
*  SYNOPSIS
*     int report_list_send(const lList *rlp, const char *rhost,
*                          const char *commproc, int id,
*                          int synchron, u_long32 *mid)
*
*  FUNCTION
*     Send a list of reports.
*
*  INPUTS
*     const lList *rlp     - REP_Type list
*     const char *rhost    - Hostname
*     const char *commproc - Component name
*     int id               - Component id
*     int synchron         - true or false
*     u_long32 *mid        - Message id
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Unexpected error
*        -2 - No memory
*        -3 - Format error
*        other - see sge_send_any_request()
*******************************************************************************/
int report_list_send(const lList *rlp, const char *rhost,
                     const char *commproc, int id,
                     int synchron, u_long32 *mid)
{
   sge_pack_buffer pb;
   int ret, size;

   DENTER(TOP_LAYER, "report_list_send");

   /* retrieve packbuffer size to avoid large realloc's while packing */
   init_packbuffer(&pb, 0, 1);
   ret = cull_pack_list(&pb, rlp);
   size = pb_used(&pb);
   clear_packbuffer(&pb);

   /* prepare packing buffer */
   if((ret = init_packbuffer(&pb, size, 0)) == PACK_SUCCESS) {
      ret = cull_pack_list(&pb, rlp);
   }

   switch (ret) {
   case PACK_SUCCESS:
      break;

   case PACK_ENOMEM:
      ERROR((SGE_EVENT, MSG_GDI_REPORTNOMEMORY_I , size));
      clear_packbuffer(&pb);
      DEXIT;
      return -2;

   case PACK_FORMAT:
      ERROR((SGE_EVENT, MSG_GDI_REPORTFORMATERROR));
      clear_packbuffer(&pb);
      DEXIT;
      return -3;

   default:
      ERROR((SGE_EVENT, MSG_GDI_REPORTUNKNOWERROR));
      clear_packbuffer(&pb);
      DEXIT;
      return -1;
   }

   ret = sge_send_any_request(synchron, mid, rhost, commproc, id, &pb,
                              TAG_REPORT_REQUEST);
   clear_packbuffer(&pb);

   DEXIT;
   return ret;
}


int 
job_schedd_info_update_master_list(sge_event_type type, 
                                   sge_event_action action, 
                                   lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;

   lList *data_list;
   lListElem *ep = NULL;
   
   DENTER(TOP_LAYER, "job_schedd_info_update_master_list");

   list = &Master_Job_Schedd_Info_List;
   list_descr = SME_Type;

   /* We always update the whole list (consisting of one list element) */
   if(*list != NULL) {
      *list = lFreeList(*list);
   }

   if((data_list = lGetList(event, ET_new_version)) != NULL) {
      if((ep = lFirst(data_list)) != NULL) {
         ep = lDechainElem(data_list, ep);
      }
   }

   /* if neccessary, create list and copy schedd info */
   if(ep != NULL) {
      *list = lCreateList("job schedd info", list_descr);
      lAppendElem(*list, ep);
   }

   DEXIT;
   return TRUE;
}


/****** gdi/pe_task/pe_task_update_master_list_usage() *************************
*  NAME
*     pe_task_update_master_list_usage() -- update a parallel tasks usage
*
*  SYNOPSIS
*     int pe_task_update_master_list_usage(lListElem *event) 
*
*  FUNCTION
*     Updates the scaled usage of a parallel task.
*
*  INPUTS
*     lListElem *event - event object containing the new usage list
*
*  RESULT
*     int - TRUE, if the operation succeeds, else FALSE
*
*  SEE ALSO
*     gdi/job/job_update_master_list_usage()
*     gdi/ja_task/ja_task_update_master_list_usage()
*******************************************************************************/
int pe_task_update_master_list_usage(lListElem *event)
{
   lList *tmp = NULL;
   u_long32 job_id, ja_task_id;
   const char *pe_task_id;
   lListElem *job, *ja_task, *pe_task;

   DENTER(TOP_LAYER, "pe_task_update_master_list_usage");

   job_id = lGetUlong(event, ET_intkey);
   ja_task_id = lGetUlong(event, ET_intkey2);
   pe_task_id = lGetString(event, ET_strkey);
   
   job = job_list_locate(Master_Job_List, job_id);
   if(job == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBFORUPDATEIN_SS, 
             job_get_id_string(job_id, 0, NULL), "pe_task_update_master_list_usage"));
      DEXIT;
      return FALSE;
   }
   
   ja_task = job_search_task(job, NULL, ja_task_id);
   if(ja_task == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDJATASKFORUPDATEIN_SS, 
             job_get_id_string(job_id, ja_task_id, NULL), "pe_task_update_master_list_usage"));
      DEXIT;
      return FALSE;
   }

   pe_task = ja_task_search_pe_task(ja_task, pe_task_id);
   if(pe_task == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDPETASKFORUPDATEIN_SS, 
             job_get_id_string(job_id, ja_task_id, pe_task_id), "pe_task_update_master_list_usage"));
      DEXIT;
      return FALSE;
   }

   lXchgList(event, ET_new_version, &tmp);
   lXchgList(pe_task, PET_scaled_usage, &tmp);
   lXchgList(event, ET_new_version, &tmp);
   
   DEXIT;
   return TRUE;
}

/****** gdi/pe_task/pe_task_update_master_list() *****************************
*  NAME
*     pe_task_update_master_list() -- update parallel tasks of an array task
*
*  SYNOPSIS
*     int pe_task_update_master_list(sge_event_type type, 
*                                    sge_event_action action, 
*                                    lListElem *event, void *clientdata) 
*
*  FUNCTION
*     Update the list of parallel tasks of an array task
*     based on an event.
*     The function is called from the event mirroring interface.
*
*     The scaled usage list of a parallel task is not updated
*     by this function, as this data is maintained by a 
*     separate event.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*******************************************************************************/
int pe_task_update_master_list(sge_event_type type, sge_event_action action,
                               lListElem *event, void *clientdata)
{
   u_long32 job_id, ja_task_id;
   const char *pe_task_id;
   
   lListElem *job, *ja_task, *pe_task;

   lList *list;
   lDescr *list_descr;

   lList *usage = NULL;

   DENTER(TOP_LAYER, "pe_task_update_master_list");

   job_id = lGetUlong(event, ET_intkey);
   ja_task_id = lGetUlong(event, ET_intkey2);
   pe_task_id = lGetString(event, ET_strkey);
   
   job = job_list_locate(Master_Job_List, job_id);
   if(job == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBFORUPDATEIN_SS, 
             job_get_id_string(job_id, 0, NULL), "pe_task_update_master_list"));
      DEXIT;
      return FALSE;
   }
   
   ja_task = job_search_task(job, NULL, ja_task_id);
   if(ja_task == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDJATASKFORUPDATEIN_SS, 
             job_get_id_string(job_id, ja_task_id, NULL), "pe_task_update_master_list"));
      DEXIT;
      return FALSE;
   }
   
   pe_task = ja_task_search_pe_task(ja_task, pe_task_id);

   list = lGetList(ja_task, JAT_task_list);
   list_descr = PET_Type;
   
   if(action == SGE_EMA_MOD) {
      /* modify event for pe_task.
       * we may not update
       * - PET_scaled_usage - it is maintained by JOB_USAGE events
       */
      if(pe_task == NULL) {
         ERROR((SGE_EVENT, MSG_JOB_CANTFINDPETASKFORUPDATEIN_SS, 
                job_get_id_string(job_id, ja_task_id, pe_task_id), "pe_task_update_master_list"));
         DEXIT;
         return FALSE;
      }
      lXchgList(pe_task, PET_scaled_usage, &usage);
   }
 
   if(sge_mirror_update_master_list(&list, list_descr, pe_task, job_get_id_string(job_id, ja_task_id, pe_task_id), action, event) != SGE_EM_OK) {
      lFreeList(usage);
      DEXIT;
      return FALSE;
   }

   /* restore pe_task list after modify event */
   if(action == SGE_EMA_MOD) {
      pe_task = ja_task_search_pe_task(ja_task, pe_task_id);
      if(pe_task == NULL) {
         ERROR((SGE_EVENT, MSG_JOB_CANTFINDPETASKFORUPDATEIN_SS, 
                job_get_id_string(job_id, ja_task_id, pe_task_id), "pe_task_update_master_list"));
         lFreeList(usage);       
         DEXIT;
         return FALSE;
      }

      lXchgList(pe_task, PET_scaled_usage, &usage);
      usage = lFreeList(usage);
   }

   /* first petask add event could have created new pe_task list for job */
   if(lGetList(ja_task, JAT_task_list) == NULL && list != NULL) {
      lSetList(ja_task, JAT_task_list, list);
   }

   DEXIT;
   return TRUE;
}

/****** gdi/userset/userset_update_master_list() *****************************
*  NAME
*     userset_update_master_list() -- update the master list of usersets
*
*  SYNOPSIS
*     int userset_update_master_list(sge_event_type type, 
*                                    sge_event_action action, 
*                                    lListElem *event, void *clientdata) 
*
*  FUNCTION
*     Update the global master list of usersets
*     based on an event.
*     The function is called from the event mirroring interface.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*     Eventmirror/sge_mirror_update_master_list_str_key()
*******************************************************************************/
int userset_update_master_list(sge_event_type type, sge_event_action action, 
                               lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   int     key_nm;
   
   const char *key;


   DENTER(TOP_LAYER, "userset_update_master_list");

   list = &Master_Userset_List;
   list_descr = US_Type;
   key_nm = US_name;

   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_str_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}

