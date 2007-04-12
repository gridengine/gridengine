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
#include <errno.h>
#include <pwd.h>

#include "uti/sge_stdlib.h"
#include "uti/sge_stdio.h"

#include "sge_advance_reservation_qmaster.h"

#include "sge_persistence_qmaster.h"
#include "rmon/sgermon.h"
#include "uti/sge_log.h"
#include "sge_answer.h"
#include "spool/sge_spooling.h"
#include "sgeobj/sge_conf.h"

#include "msg_common.h"
#include "msg_qmaster.h"
#include "sgeobj/msg_sgeobjlib.h"
#include "sgeobj/sge_qinstance.h"
#include "sgeobj/sge_hgroup.h"
#include "sgeobj/sge_userset.h"
#include "sched/sge_resource_utilization.h"

#include "sge_lock.h"
#include "sge_mtutil.h"
#include "uti/sge_time.h"
#include "uti/sge_uidgid.h"
#include "sge_utility.h"
#include "sge_range.h"
#include "sgeobj/sge_job.h"
#include "sgeobj/sge_cqueue.h"
#include "sgeobj/sge_qinstance_state.h"
#include "sgeobj/sge_host.h"
#include "sgeobj/sge_schedd_conf.h"
#include "sgeobj/sge_centry.h"
#include "sgeobj/sge_pe.h"
#include "sgeobj/sge_str.h"

#include "sched/sge_select_queue.h"
#include "sched/schedd_monitor.h"
#include "sched/sge_job_schedd.h"
#include "sched/sge_serf.h"
#include "sched/valid_queue_user.h"

#include "evm/sge_event_master.h"
#include "evm/sge_queue_event_master.h"

#include "sge_utility_qmaster.h"
#include "sge_cqueue_qmaster.h"

#include "evm/sge_event_master.h"
#include "sge_reporting_qmaster.h"

typedef struct {
   u_long32 ar_id;
   bool changed;
   pthread_mutex_t ar_id_mutex;
} ar_id_t;

ar_id_t ar_id_control = {0, false, PTHREAD_MUTEX_INITIALIZER};

static bool ar_reserve_queues(lList **alpp, lListElem *ar);
static u_long32 sge_get_ar_id(sge_gdi_ctx_class_t *ctx, monitoring_t *monitor);
static u_long32 guess_highest_ar_id(void);

/****** sge_advance_reservation_qmaster/ar_mod() *******************************
*  NAME
*     ar_mod() -- gdi callback function for adding modifing advance reservations
*
*  SYNOPSIS
*     int ar_mod(sge_gdi_ctx_class_t *ctx, lList **alpp, lListElem *new_ar, 
*     lListElem *ar, int add, const char *ruser, const char *rhost, 
*     gdi_object_t *object, int sub_command, monitoring_t *monitor) 
*
*  FUNCTION
*     This function is called from the framework that
*     add/modify/delete generic gdi objects.
*     The purpose of this function is it to add new advance reservation
*     objects.
*     Modifing is currently not supported.
*
*  INPUTS
*     sge_gdi_ctx_class_t *ctx - gdi context pointer
*     lList **alpp             - the answer_list
*     lListElem *new_ar        - if a new ar object will be created by this
*                                function, then new_ar is a newly initialized
*                                CULL object.
*     lListElem *ar            - a reduced ar object that contains all of the
*                                requested values
*     int add                  - 1 for add requests
*                                0 for mod requests
*     const char *ruser        - username who invoked this GDI request
*     const char *rhost        - hostname of where the GDI request was invoked
*     gdi_object_t *object     - structure of the GDI framework that contains
*                                additional informations to perform the request
*     int sub_command          - GDI sub command
*     monitoring_t *monitor    - monitoring structure
*
*  RESULT
*     int - 0 on success
*           STATUS_EUNKNOWN if an error occured
*
*  NOTES
*     MT-NOTE: ar_mod() is not MT safe 
*******************************************************************************/
int ar_mod(sge_gdi_ctx_class_t *ctx, lList **alpp, lListElem *new_ar,
           lListElem *ar, int add, const char *ruser, 
           const char *rhost, gdi_object_t *object, int sub_command,
           monitoring_t *monitor)
{
   object_description *object_base = object_type_get_object_description();
   u_long32 ar_id;
   u_long32 max_advance_reservations =  mconf_get_max_advance_reservations();

   DENTER(TOP_LAYER, "ar_mod");

   if (!ar_validate(ar, alpp, true)) {
      goto ERROR;
   }

   if (add) {
      /* get new ar ids until we find one that is not yet used */
      do {
         ar_id = sge_get_ar_id(ctx, monitor);
      } while (ar_list_locate(*object_base[SGE_TYPE_AR].list, ar_id));
      lSetUlong(new_ar, AR_id, ar_id);
      /*
      ** set the owner of new_ar, don't overwrite it with
      ** attr_mod_str(alpp, ar, new_ar, AR_owner, object->object_name);
      */
      lSetString(new_ar, AR_owner, ruser);
      lSetString(new_ar, AR_group, ctx->get_groupname(ctx));
   } else {
      ERROR((SGE_EVENT, MSG_NOTYETIMPLEMENTED_S, "advance reservation modification"));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      goto ERROR; 
   }

   if (max_advance_reservations > 0 &&
       max_advance_reservations <= lGetNumberOfElem(*object_base[SGE_TYPE_AR].list)) {
      ERROR((SGE_EVENT, MSG_AR_MAXARSPERCLUSTER_U, sge_u32c(max_advance_reservations)));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      goto ERROR; 
   }

   /*    AR_name, SGE_STRING */
   attr_mod_zerostr(ar, new_ar, AR_name, object->object_name);
   /*   AR_account, SGE_STRING */
   attr_mod_zerostr(ar, new_ar, AR_account, object->object_name);
   /*   AR_submission_time, SGE_ULONG */
   lSetUlong(new_ar, AR_submission_time, sge_get_gmt());   
   /*   AR_start_time, SGE_ULONG          required */
   attr_mod_ulong(ar, new_ar, AR_start_time, object->object_name);
   /*   AR_end_time, SGE_ULONG            required */
   attr_mod_ulong(ar, new_ar, AR_end_time, object->object_name);
   /*   AR_duration, SGE_ULONG */
   attr_mod_ulong(ar, new_ar, AR_duration, object->object_name);
   /*   AR_verify, SGE_ULONG              just verify the reservation or final case */
   attr_mod_ulong(ar, new_ar, AR_verify, object->object_name);
   /*   AR_error_handling, SGE_ULONG      how to deal with soft and hard exceptions */
   attr_mod_ulong(ar, new_ar, AR_error_handling, object->object_name);
   /*   AR_state, SGE_ULONG               state of the AR */
   attr_mod_ulong(ar, new_ar, AR_state, object->object_name);
   /*   AR_checkpoint_name, SGE_STRING    Named checkpoint */
   attr_mod_zerostr(ar, new_ar, AR_checkpoint_name, object->object_name);
   /*   AR_resource_list, SGE_LIST */
   attr_mod_sub_list(alpp, new_ar, AR_resource_list, AR_name, ar, sub_command, SGE_ATTR_COMPLEX_VALUES, SGE_OBJ_AR, 0); 
   /*   AR_queue_list, SGE_LIST */
   attr_mod_sub_list(alpp, new_ar, AR_queue_list, AR_name, ar, sub_command, SGE_ATTR_QUEUE_LIST, SGE_OBJ_AR, 0); 
   /*   AR_mail_options, SGE_ULONG   */
   attr_mod_ulong(ar, new_ar, AR_mail_options, object->object_name);
   /*   AR_mail_list, SGE_LIST */
   attr_mod_sub_list(alpp, new_ar, AR_mail_list, AR_name, ar, sub_command, SGE_ATTR_MAIL_LIST, SGE_OBJ_AR, 0); 
   /*   AR_pe, SGE_STRING */
   attr_mod_zerostr(ar, new_ar, AR_pe, object->object_name);
   /*   AR_master_queue_list, SGE_LIST */
   attr_mod_sub_list(alpp, new_ar ,AR_master_queue_list, AR_name, ar, sub_command, SGE_ATTR_QUEUE_LIST, SGE_OBJ_AR, 0); 
   /*   AR_pe_range, SGE_LIST */
   attr_mod_sub_list(alpp, new_ar, AR_pe_range, AR_name, ar, sub_command, SGE_ATTR_PE_LIST, SGE_OBJ_AR, 0);
   /*   AR_acl_list, SGE_LIST */
   attr_mod_sub_list(alpp, new_ar, AR_acl_list, AR_name, ar, sub_command, SGE_ATTR_USER_LISTS, SGE_OBJ_AR, 0);
   /*   AR_xacl_list, SGE_LIST */
   attr_mod_sub_list(alpp, new_ar, AR_xacl_list, AR_name, ar, sub_command, SGE_ATTR_XUSER_LISTS, SGE_OBJ_AR, 0); 
   /*   AR_type, SGE_ULONG     */
   attr_mod_ulong(ar, new_ar, AR_type, object->object_name);


   /* try to reserve the queues */
   if (!ar_reserve_queues(alpp, new_ar)) {
      goto ERROR;
   }

   INFO((SGE_EVENT, MSG_AR_GRANTED_U, sge_u32c(ar_id)));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   DRETURN(0);

ERROR:
   DRETURN(STATUS_EUNKNOWN);
}

/****** sge_advance_reservation_qmaster/ar_spool() *****************************
*  NAME
*     ar_spool() -- gdi callback funktion to spool an advance reservation
*
*  SYNOPSIS
*     int ar_spool(sge_gdi_ctx_class_t *ctx, lList **alpp, lListElem *ep, 
*     gdi_object_t *object) 
*
*  FUNCTION
*     This function is called from the framework that
*     add/modify/delete generic gdi objects.
*     After an object was modified/added successfully it
*     is necessary to spool the current state to the filesystem.
*
*  INPUTS
*     sge_gdi_ctx_class_t *ctx - GDI context
*     lList **alpp             - answer_list
*     lListElem *ep            - element to spool
*     gdi_object_t *object     - structure from the GDI framework
*
*  RESULT
*     [alpp] - error messages will be added to this list
*     0 - success
*     STATUS_EEXIST - an error occured
*
*  NOTES
*     MT-NOTE: ar_spool() is MT safe 
*******************************************************************************/
int ar_spool(sge_gdi_ctx_class_t *ctx, lList **alpp, lListElem *ep, gdi_object_t *object)
{
   lList *answer_list = NULL;
   bool dbret;
   bool job_spooling = ctx->get_job_spooling(ctx);
   dstring buffer = DSTRING_INIT;

   DENTER(TOP_LAYER, "ar_spool");

   sge_dstring_sprintf(&buffer, sge_u32, lGetUlong(ep, AR_id));
   dbret = spool_write_object(&answer_list, spool_get_default_context(), ep, 
                              sge_dstring_get_string(&buffer), SGE_TYPE_AR,
                              job_spooling);
   answer_list_output(&answer_list);

   if (!dbret) {
      answer_list_add_sprintf(alpp, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_PERSISTENCE_WRITE_FAILED_S,
                              sge_dstring_get_string(&buffer));
   } 
   sge_dstring_free(&buffer);

   DRETURN(dbret ? 0 : 1);
}

/****** sge_advance_reservation_qmaster/ar_success() ***************************
*  NAME
*     ar_success() -- does something after a successfully add or modify request
*
*  SYNOPSIS
*     int ar_success(sge_gdi_ctx_class_t *ctx, lListElem *ep, lListElem 
*     *old_ep, gdi_object_t *object, lList **ppList, monitoring_t *monitor) 
*
*  FUNCTION
*     This function is called from the framework that
*     add/modify/delete generic gdi objects.
*     After an object was modified/added and spooled successfully 
*     it is possibly necessary to perform additional tasks.
*     For example it is necessary to send some events to
*     other deamon.
*
*  INPUTS
*     sge_gdi_ctx_class_t *ctx - GDI context
*     lListElem *ep            - new added object
*     lListElem *old_ep        - old object before modifications or NULL
*                                for add requests
*     gdi_object_t *object     - structure from the GDI framework
*     lList **ppList           - ???
*     monitoring_t *monitor    - monitoring structure
*
*  RESULT
*     int - 0
*
*  NOTES
*     MT-NOTE: ar_success() is not MT safe 
*******************************************************************************/
int ar_success(sge_gdi_ctx_class_t *ctx, lListElem *ep, lListElem *old_ep,
               gdi_object_t *object, lList **ppList, monitoring_t *monitor)
{
   te_event_t ev;
   dstring buffer = DSTRING_INIT;
   u_long32 timestamp = 0; 

   DENTER(TOP_LAYER, "ar_success");

   ev = te_new_event((time_t)lGetUlong(ep, AR_start_time), TYPE_AR_EVENT, ONE_TIME_EVENT, lGetUlong(ep, AR_id), AR_RUNNING, NULL);
   te_add_event(ev);
   te_free_event(&ev);

   /* with old_ep it is possible to identify if it is an add or modify request */
   timestamp = sge_get_gmt();
   if (old_ep == NULL) {
      reporting_create_new_ar_record(NULL, ep, timestamp);
      reporting_create_ar_attribute_record(NULL, ep, timestamp);
   } else {
      reporting_create_ar_attribute_record(NULL, ep, timestamp);
   }

   /*
   ** return element with correct id
   */
   if (ppList != NULL) {
      if (*ppList == NULL) {
         *ppList = lCreateList("", AR_Type);
      }   
      lAppendElem(*ppList, lCopyElem(ep)); 
   }

   /*
   ** send sgeE_AR_MOD/sgeE_AR_ADD event
   */
   sge_dstring_sprintf(&buffer, sge_u32, lGetUlong(ep, AR_id));
   sge_add_event(0, old_ep?sgeE_AR_MOD:sgeE_AR_ADD, lGetUlong(ep, AR_id), 0, 
                 sge_dstring_get_string(&buffer), NULL, NULL, ep);
/*    lListElem_clear_changed_info(ep); */
   sge_dstring_free(&buffer);

   DRETURN(0);
}

/****** sge_advance_reservation_qmaster/ar_del() *******************************
*  NAME
*     ar_del() -- removed advance reservation from master list
*
*  SYNOPSIS
*     int ar_del(sge_gdi_ctx_class_t *ctx, lListElem *ep, lList **alpp, lList 
*     **ar_list, char *ruser, char *rhost) 
*
*  FUNCTION
*     This function removes a advance reservation from the master list and
*     performs the necessary cleanup.
*
*  INPUTS
*     sge_gdi_ctx_class_t *ctx - GDI context
*     lListElem *ep            - element that should be removed
*     lList **alpp             - answer list
*     lList **ar_list          - list from where the element should be removed
*                                (normally a reference to the master ar list)
*     char *ruser              - user who invoked this GDI request
*     char *rhost              - host where the request was invoked
*
*  RESULT
*     int - 0 on success
*           STATUS_EUNKNOWN on failure
*
*  NOTES
*     MT-NOTE: ar_del() is not MT safe 
*******************************************************************************/
int ar_del(sge_gdi_ctx_class_t *ctx, lListElem *ep, lList **alpp, lList **ar_list, 
           char *ruser, char *rhost)
{
   const char *ar_name;
   u_long32 ar_id = 0;
   lListElem *found;
   bool removed_one = false;
   dstring buffer = DSTRING_INIT;

   DENTER(TOP_LAYER, "ar_del");

   /*
    When the AR end time is reached at first all jobs referring to the
    AR will be deleted and at second the AR itself will be deleted.
    No jobs can request the AR handle any longer.
   */
   if (!ep || !ruser || !rhost) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      sge_dstring_free(&buffer);
      DRETURN(STATUS_EUNKNOWN);
   }

   /* ep is no ar element, if ep has no AR_id */
   if (lGetPosViaElem(ep, AR_id, SGE_NO_ABORT) < 0) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(AR_id), SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      sge_dstring_free(&buffer);
      DRETURN(STATUS_EUNKNOWN);
   }

   ar_name = lGetString(ep, AR_name);
   ar_id = lGetUlong(ep, AR_id);
   if (!ar_name && ar_id == 0) {
      CRITICAL((SGE_EVENT, MSG_AR_XISINVALIDARID_U, sge_u32c(ar_id)));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      sge_dstring_free(&buffer);
      DRETURN(STATUS_EUNKNOWN);
   }

   /* search for ar with this name and remove it from the list */
   if (ar_name) {
      found = lGetElemStr(*ar_list, AR_name, ar_name);
   } else {
      found = ar_list_locate(*ar_list, ar_id);
   }

   while (found) {
      removed_one = true;

      ar_id = lGetUlong(found, AR_id);
      sge_dstring_sprintf(&buffer, sge_U32CFormat, ar_id);

      /* remove timer for this advance reservation */
      te_delete_one_time_event(TYPE_CALENDAR_EVENT, ar_id, AR_RUNNING, NULL);
      te_delete_one_time_event(TYPE_CALENDAR_EVENT, ar_id, AR_EXITED, NULL);

      /* unblock reserved queues */
      ar_do_reservation(found, false);

      found = lDechainElem(*ar_list, found);

      sge_event_spool(ctx, alpp, 0, sgeE_AR_DEL, 
                      ar_id, 0, sge_dstring_get_string(&buffer), NULL, NULL,
                      NULL, NULL, NULL, true, true);

      INFO((SGE_EVENT, "%s@%s deleted advance reservation %s",
               ruser, rhost, sge_dstring_get_string(&buffer)));
      answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);

      lFreeElem(&found);
      if (ar_name) {
         found = lGetElemStr(*ar_list, AR_name, ar_name);
      }
   }

   if (!removed_one) {
      if (!ar_name) {
         sge_dstring_sprintf(&buffer, sge_u32, ar_id);
      } else {
         sge_dstring_sprintf(&buffer, "%s", ar_name);
      }

      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, MSG_OBJ_AR, sge_dstring_get_string(&buffer)));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      sge_dstring_free(&buffer);
      DRETURN(STATUS_EEXIST);
   }

   /* remove all orphaned queue intances, which are empty. */
   cqueue_list_del_all_orphaned(ctx, *(object_type_get_master_list(SGE_TYPE_CQUEUE)), alpp);

   sge_dstring_free(&buffer);
   DRETURN(0);
}

/****** sge_advance_reservation_qmaster/sge_get_ar_id() ************************
*  NAME
*     sge_get_ar_id() -- returns the next possible unused id
*
*  SYNOPSIS
*     static u_long32 sge_get_ar_id(sge_gdi_ctx_class_t *ctx, monitoring_t 
*     *monitor) 
*
*  FUNCTION
*     returns the next possible unused advance reservation id.
*
*  INPUTS
*     sge_gdi_ctx_class_t *ctx - gdi context
*     monitoring_t *monitor    - monitoring structure
*
*  RESULT
*     static u_long32 - ar id
*
*  NOTES
*     MT-NOTE: sge_get_ar_id() is MT safe 
*******************************************************************************/
static u_long32 sge_get_ar_id(sge_gdi_ctx_class_t *ctx, monitoring_t *monitor)
{
   u_long32 ar_id;
   bool is_store_ar = false;

   DENTER(TOP_LAYER, "sge_get_ar_id");

   sge_mutex_lock("ar_id_mutex", "sge_get_ar_id", __LINE__, 
                  &ar_id_control.ar_id_mutex);
 
   ar_id_control.ar_id++;
   ar_id_control.changed = true;
   if (ar_id_control.ar_id > MAX_SEQNUM) {
      DPRINTF(("highest ar number MAX_SEQNUM %d exceeded, starting over with 1\n", MAX_SEQNUM));
      ar_id_control.ar_id = 1;
      is_store_ar = true;
   }
   ar_id = ar_id_control.ar_id;

   sge_mutex_unlock("ar_id_mutex", "sge_get_ar_id", __LINE__, 
                  &ar_id_control.ar_id_mutex);
  
   if (is_store_ar) {
      sge_store_ar_id(ctx, NULL, monitor);
   }
  
   DRETURN(ar_id);
}

/****** sge_advance_reservation_qmaster/sge_store_ar_id() **********************
*  NAME
*     sge_store_ar_id() -- store ar id
*
*  SYNOPSIS
*     void sge_store_ar_id(sge_gdi_ctx_class_t *ctx, te_event_t anEvent, 
*     monitoring_t *monitor) 
*
*  FUNCTION
*     At qmaster shutdown it's necessary to store the latest highest ar id to
*     reinitialize the counter at the next qmaster start. This is done by a event
*     timer in specific intervall.
*
*  INPUTS
*     sge_gdi_ctx_class_t *ctx - GDI context
*     te_event_t anEvent       - event that triggered this function
*     monitoring_t *monitor    - monitoring structure
*
*  NOTES
*     MT-NOTE: sge_store_ar_id() is not MT safe 
*******************************************************************************/
void sge_store_ar_id(sge_gdi_ctx_class_t *ctx, te_event_t anEvent, monitoring_t *monitor) {
   u_long32 ar_id = 0;
   bool changed = false;

   DENTER(TOP_LAYER, "sge_store_ar_id");
   
   sge_mutex_lock("ar_id_mutex", "sge_store_ar_id", __LINE__, 
                  &ar_id_control.ar_id_mutex);
   if (ar_id_control.changed) {
      ar_id = ar_id_control.ar_id;
      ar_id_control.changed = false;
      changed = true;
   }   
   sge_mutex_unlock("ar_id_mutex", "sge_store_ar_id", __LINE__, 
                  &ar_id_control.ar_id_mutex);     

   /* here we got a race condition that can (very unlikely)
      cause concurrent writing of the sequence number file  */ 
   if (changed) {
      FILE *fp = fopen(ARSEQ_NUM_FILE, "w");

      if (fp == NULL) {
         ERROR((SGE_EVENT, MSG_NOSEQFILECREATE_SSS, "ar", ARSEQ_NUM_FILE, strerror(errno)));
      } else {
         FPRINTF((fp, sge_u32"\n", ar_id));
         FCLOSE(fp);
      }   
   }
   DRETURN_VOID;

FPRINTF_ERROR:
FCLOSE_ERROR:
   ERROR((SGE_EVENT, MSG_NOSEQFILECLOSE_SSS, "ar", ARSEQ_NUM_FILE, strerror(errno)));
   DRETURN_VOID;
}

/****** sge_advance_reservation_qmaster/sge_init_ar_id() ***********************
*  NAME
*     sge_init_ar_id() -- init ar id counter
*
*  SYNOPSIS
*     void sge_init_ar_id(void) 
*
*  FUNCTION
*     Called during startup and sets the advance reservation id counter. 
*
*  NOTES
*     MT-NOTE: sge_init_ar_id() is MT safe 
*******************************************************************************/
void sge_init_ar_id(void) 
{
   FILE *fp = NULL;
   u_long32 ar_id = 0;
   u_long32 guess_ar_id = 0;
  
   DENTER(TOP_LAYER, "sge_init_ar_id");
   
   if ((fp = fopen(ARSEQ_NUM_FILE, "r"))) {
      if (fscanf(fp, sge_u32, &ar_id) != 1) {
         ERROR((SGE_EVENT, MSG_NOSEQNRREAD_SSS, "ar", ARSEQ_NUM_FILE, strerror(errno)));
      }
      FCLOSE(fp);
FCLOSE_ERROR:
      fp = NULL;
   } else {
      WARNING((SGE_EVENT, MSG_NOSEQFILEOPEN_SSS, "ar", ARSEQ_NUM_FILE, strerror(errno)));
   }  
   
   guess_ar_id = guess_highest_ar_id();
   ar_id = MAX(ar_id, guess_ar_id);
   
   sge_mutex_lock("ar_id_mutex", "sge_init_ar_id", __LINE__, 
                  &ar_id_control.ar_id_mutex);
   ar_id_control.ar_id = ar_id;
   ar_id_control.changed = true;
   sge_mutex_unlock("ar_id_mutex", "sge_init_ar_id", __LINE__, 
                  &ar_id_control.ar_id_mutex);   
                  
   DRETURN_VOID;
}

/****** sge_advance_reservation_qmaster/guess_highest_ar_id() ******************
*  NAME
*     guess_highest_ar_id() -- guesses the histest ar id
*
*  SYNOPSIS
*     static u_long32 guess_highest_ar_id(void) 
*
*  FUNCTION
*     Iterates over all granted advance reservations in the cluster and determines
*     the highest id
*
*  RESULT
*     static u_long32 - determined id
*
*  NOTES
*     MT-NOTE: guess_highest_ar_id() is MT safe 
*******************************************************************************/
static u_long32 guess_highest_ar_id(void)
{
   lListElem *ar;
   u_long32 maxid = 0;
   lList *master_ar_list = *(object_type_get_master_list(SGE_TYPE_AR)); 

   DENTER(TOP_LAYER, "guess_highest_ar_id");   

   /* this function is called during qmaster startup and not while it is running,
      we do not need to monitor this lock */
   SGE_LOCK(LOCK_GLOBAL, LOCK_READ);
   
   ar = lFirst(master_ar_list);
   if (ar) { 
      int pos;
      pos = lGetPosViaElem(ar, AR_id, SGE_NO_ABORT); 
      
      for_each(ar, master_ar_list) {
         maxid = MAX(maxid, lGetPosUlong(ar, pos));
      }   
   }

   SGE_UNLOCK(LOCK_GLOBAL, LOCK_READ);

   DRETURN(maxid);
}   

/****** sge_advance_reservation_qmaster/sge_ar_event_handler() *****************
*  NAME
*     sge_ar_event_handler() -- advance reservation event handler
*
*  SYNOPSIS
*     void sge_ar_event_handler(sge_gdi_ctx_class_t *ctx, te_event_t anEvent, 
*     monitoring_t *monitor) 
*
*  FUNCTION
*     Registered function in the times event framework. For every granted a trigger
*     for the start time of the advance reservation is registered. When the function is
*     executed at start time it regististers a additional timer for the end time of
*     the advance reservation.
*
*  INPUTS
*     sge_gdi_ctx_class_t *ctx - GDI context
*     te_event_t anEvent       - triggered timed event
*     monitoring_t *monitor    - monitoring structure
*
*  NOTES
*     MT-NOTE: sge_ar_event_handler() is MT safe 
*******************************************************************************/
void sge_ar_event_handler(sge_gdi_ctx_class_t *ctx, te_event_t anEvent, monitoring_t *monitor)
{
   lListElem *ar;
   u_long32 ar_id = te_get_first_numeric_key(anEvent);
   u_long32 state = te_get_second_numeric_key(anEvent);
   te_event_t ev;

   DENTER(TOP_LAYER, "sge_ar_event_handler");

   
   /*
    To guarantee all jobs are removed from the cluster when AR end time is
    reached it is necessary to consider the DURATION_OFFSET for Advance Reservation also.
    This means all jobs submitted to a AR will have a resulting runtime limit of AR duration - DURATION_OFFSET.
    Jobs requesting a longer runtime will not be scheduled.
    The AR requester needs to keep this in mind when he creates a new AR.
    */
   MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE), monitor);

   if (!(ar = ar_list_locate(*(object_type_get_master_list(SGE_TYPE_AR)), ar_id))) {
      ERROR((SGE_EVENT, MSG_EVE_TE4AR_U, sge_u32c(ar_id)));   
      SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);      
      DRETURN_VOID;
   }
   
   if (state == AR_EXITED) {
      lListElem *elem = NULL;
      dstring buffer = DSTRING_INIT;
      time_t timestamp = (time_t) sge_get_gmt();

      sge_dstring_sprintf(&buffer, sge_U32CFormat, ar_id);

      /* unblock reserved queues */
      ar_do_reservation(ar, false);

      reporting_create_ar_log_record(NULL, ar, ARL_TERMINATED, 
                                     ar_get_string_from_event(ARL_TERMINATED),
                                     timestamp);  

      for_each(elem, lGetList(ar, AR_granted_slots)) {
         const char *queue_name = lGetString(elem, JG_qname);
         u_long32 slots = lGetUlong(elem, JG_slots);
         dstring cqueue_name = DSTRING_INIT;
         dstring host_or_hgroup = DSTRING_INIT;
         bool has_hostname;
         bool has_domain;
   
         cqueue_name_split(queue_name, &cqueue_name, &host_or_hgroup,
                           &has_hostname, &has_domain);
        
         reporting_create_ar_acct_record(NULL, ar,
                                         sge_dstring_get_string(&cqueue_name),
                                         sge_dstring_get_string(&host_or_hgroup),
                                         slots, timestamp); 

         sge_dstring_free(&cqueue_name);
         sge_dstring_free(&host_or_hgroup);
      } 

      /* AR TBD CLEANUP 
       * for now we only remove the AR object 
       */
      DPRINTF(("AR: exited, removing AR %s\n", sge_dstring_get_string(&buffer)));
      lRemoveElem(*(object_type_get_master_list(SGE_TYPE_AR)), &ar);
      sge_event_spool(ctx, NULL, 0, sgeE_AR_DEL, 
                      0, 0, sge_dstring_get_string(&buffer), NULL, NULL,
                      NULL, NULL, NULL, true, true);

      /* remove all orphaned queue intances, which are empty. */
      cqueue_list_del_all_orphaned(ctx, *(object_type_get_master_list(SGE_TYPE_CQUEUE)), NULL);

      sge_dstring_free(&buffer);

   } else {
      /* AR_RUNNING */
      DPRINTF(("AR: started, changing state of AR "sge_u32"\n", ar_id));

      lSetUlong(ar, AR_state, state);
      ev = te_new_event((time_t)lGetUlong(ar, AR_end_time), TYPE_AR_EVENT, ONE_TIME_EVENT, ar_id, AR_EXITED, NULL);
      te_add_event(ev);
      te_free_event(&ev);

      reporting_create_ar_log_record(NULL, ar, ARL_STARTTIME_REACHED, 
                                     ar_get_string_from_event(ARL_STARTTIME_REACHED),
                                     sge_get_gmt());  
   }

   SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);

   DRETURN_VOID;
}

/****** sge_advance_reservation_qmaster/ar_reserve_queues() ********************
*  NAME
*     ar_reserve_queues() -- selects the queues for reserving 
*
*  SYNOPSIS
*     static bool ar_reserve_queues(lList **alpp, lListElem *ar) 
*
*  FUNCTION
*     The function executes the scheduler code to select queues matching the
*     advance reservation request for reserving. The function works on temporary
*     lists and creates the AR_granted_slots list
*
*  INPUTS
*     lList **alpp  - answer list pointer pointer
*     lListElem *ar - ar object
*
*  RESULT
*     static bool - true on success, enough resources reservable
*                   false in verify mode or not enough resources available
*
*  NOTES
*     MT-NOTE: ar_reserve_queues() is not MT safe, needs GLOBAL_LOCK
*******************************************************************************/
static bool ar_reserve_queues(lList **alpp, lListElem *ar)
{
   lList **splitted_job_lists[SPLIT_LAST];
   lList *waiting_due_to_pedecessor_list = NULL;   /* JB_Type */
   lList *waiting_due_to_time_list = NULL;         /* JB_Type */
   lList *pending_excluded_list = NULL;            /* JB_Type */
   lList *suspended_list = NULL;                   /* JB_Type */
   lList *finished_list = NULL;                    /* JB_Type */
   lList *pending_list = NULL;                     /* JB_Type */
   lList *pending_excludedlist = NULL;             /* JB_Type */
   lList *running_list = NULL;                     /* JB_Type */
   lList *error_list = NULL;                       /* JB_Type */
   lList *hold_list = NULL;                        /* JB_Type */
   lList *not_started_list = NULL;                 /* JB_Type */

   int verify_mode = lGetUlong(ar, AR_verify);
   lList *talp = NULL;

   lListElem *cqueue = NULL;
   bool ret = true;
   int i = 0;
   lListElem *dummy_job = lCreateElem(JB_Type);
   sge_assignment_t a = SGE_ASSIGNMENT_INIT;
   object_description *object_base = object_type_get_object_description();
   lList *master_cqueue_list = *object_base[SGE_TYPE_CQUEUE].list;
   lList *master_userset_list = *object_base[SGE_TYPE_USERSET].list;
   lList *master_job_list = *object_base[SGE_TYPE_JOB].list;
   lList *master_centry_list = *object_base[SGE_TYPE_CENTRY].list;
   lList *master_hgroup_list = *object_base[SGE_TYPE_HGROUP].list;

   /* These lists must be copied */
   lList *master_pe_list = lCopyList("", *object_base[SGE_TYPE_PE].list);
   lList *master_exechost_list = lCopyList("", *object_base[SGE_TYPE_EXECHOST].list);

   lList *all_queue_list = NULL;
   dispatch_t result = DISPATCH_NEVER_CAT;

   DENTER(TOP_LAYER, "ar_reserve_queues");

   assignment_init(&a, dummy_job, NULL, false);
   a.host_list        = master_exechost_list;
   a.centry_list      = master_centry_list;
   a.acl_list         = master_userset_list;
   a.hgrp_list        = master_hgroup_list;
   a.gep              = host_list_locate(master_exechost_list, SGE_GLOBAL_NAME);
   a.start            = lGetUlong(ar, AR_start_time);
   a.duration         = lGetUlong(ar, AR_duration);
   a.is_reservation   = true;
   a.is_advance_reservation = true;

   /* 
    * Current scheduler code expects all queue instances in a plain list. We use 
    * a copy of all queue instances that needs to be free'd explicitely after 
    * deciding about assignment. This is because assignment_release() sees 
    * queue_list only as a list pointer.
    */
   for_each(cqueue, master_cqueue_list) {
      lList *qinstance_list = lGetList(cqueue, CQ_qinstances);

      qinstance_list = lCopyList(NULL, qinstance_list);
      if (!all_queue_list) {
         all_queue_list = qinstance_list;
      } else {
         lAddList(all_queue_list, &qinstance_list);
      }   
   }

   /*
    * split jobs
    */
   {
      lList *job_list_copy = lCopyList("", master_job_list);

      for (i = SPLIT_FIRST; i < SPLIT_LAST; i++) {
         splitted_job_lists[i] = NULL;
      }

      splitted_job_lists[SPLIT_WAITING_DUE_TO_PREDECESSOR] = &waiting_due_to_pedecessor_list;
      splitted_job_lists[SPLIT_WAITING_DUE_TO_TIME] = &waiting_due_to_time_list;
      splitted_job_lists[SPLIT_PENDING_EXCLUDED] = &pending_excluded_list;
      splitted_job_lists[SPLIT_SUSPENDED] = &suspended_list;
      splitted_job_lists[SPLIT_FINISHED] = &finished_list;
      splitted_job_lists[SPLIT_PENDING] = &pending_list;
      splitted_job_lists[SPLIT_PENDING_EXCLUDED_INSTANCES] = &pending_excludedlist;
      splitted_job_lists[SPLIT_RUNNING] = &running_list;
      splitted_job_lists[SPLIT_ERROR] = &error_list;
      splitted_job_lists[SPLIT_HOLD] = &hold_list;
      splitted_job_lists[SPLIT_NOT_STARTED] = &not_started_list;

      /* TODO: splitted job lists must be freed */

      split_jobs(&job_list_copy, NULL, all_queue_list, 
                 mconf_get_max_aj_instances(), splitted_job_lists);

      lFreeList(&job_list_copy);                  
   }

   /*
    * Queue filtering
    *
    * TBD sort our queues where no user of AR acl have access
    * for new we just copy the all_queue_list
    */
   {
      lCondition *where;
      lEnumeration *what;
      const lDescr *dp = lGetListDescr(all_queue_list);

      /*
       * sort out queues in orphaned state
       */

      what = lWhat("%T(ALL)", dp); 
      where = lWhere("%T("
         "!(%I m= %u))",
         dp,
         QU_state, QI_ORPHANED
         );

      if (!what || !where) {
         DPRINTF(("failed creating where or what describing non available queues\n")); 
      }

      a.queue_list = lSelect("", all_queue_list, where, what);

      if (lGetList(ar, AR_acl_list) != NULL) {
         /*
          * sort out queues where AR acl have no permissions
          */
         lListElem *ep, *next_ep;
         const char *user= NULL;

         next_ep = lFirst(a.queue_list);
         while ((ep = next_ep)) {
            lListElem *acl_entry;
            
            next_ep = lNext(ep);

            for_each(acl_entry, lGetList(ar, AR_acl_list)) {
               
               user = lGetString(acl_entry, ST_name);
               if (!is_hgroup_name(user)) {
                  struct passwd *pw;
                  struct passwd pw_struct;
                  char buffer[2048];
                  stringT group;

                  pw = sge_getpwnam_r(user, &pw_struct, buffer, sizeof(buffer));
                  sge_gid2group(pw->pw_gid, group, MAX_STRING_SIZE, MAX_NIS_RETRIES);

                  DPRINTF(("user: %s\n", user));
                  DPRINTF(("group: %s\n", group));
                  
                  if (sge_has_access(user, group, ep, master_userset_list) == 0) {
                      lRemoveElem(a.queue_list, &ep);
                      break;
                  }
               } else {
                  bool skip_queue = false;
                  lList *qacl = lGetList(ep, QU_xacl);
                  /* skip preattached \@ sign */
                  const char *acl_name = ++user;

                  DPRINTF(("acl :%s", acl_name));

                  /* at first xacl */
                  if (qacl && lGetElemStr(qacl, US_name, acl_name) != NULL) {
                     skip_queue = true;
                  }

                  /* at second acl */
                  qacl = lGetList(ep, QU_acl);
                  if (!skip_queue && qacl && lGetElemStr(qacl, US_name, acl_name) == NULL) {
                     skip_queue = true;
                  }

                  if (skip_queue) {
                     lRemoveElem(a.queue_list, &ep);
                     break;
                  }
               }
            }
         }
      } else {
         lSetString(dummy_job, JB_owner, lGetString(ar, AR_owner));
         lSetString(dummy_job, JB_group, lGetString(ar, AR_group));
      }

      lFreeWhere(&where);
      lFreeWhat(&what);
   }

   /*
    * prepare resource schedule
    */
   prepare_resource_schedules(*(splitted_job_lists[SPLIT_RUNNING]),
                              *(splitted_job_lists[SPLIT_SUSPENDED]),
                              master_pe_list, a.host_list, a.queue_list, 
                              NULL, a.centry_list, a.acl_list,
                              a.hgrp_list, false);

   lSetUlong(dummy_job, JB_execution_time, lGetUlong(ar, AR_start_time));
   lSetUlong(dummy_job, JB_deadline, lGetUlong(ar, AR_end_time));
   lSetList(dummy_job, JB_hard_resource_list, lCopyList("", lGetList(ar, AR_resource_list)));
   lSetList(dummy_job, JB_hard_queue_list, lCopyList("", lGetList(ar, AR_queue_list)));
   lSetUlong(dummy_job, JB_type, lGetUlong(ar, AR_type));
   lSetString(dummy_job, JB_checkpoint_name, lGetString(ar, AR_checkpoint_name));

    /* imagine qs is empty */
    sconf_set_qs_state(QS_STATE_EMPTY);

   /* redirect scheduler monitoring into answer list */
   if (verify_mode == AR_JUST_VERIFY) {
      DPRINTF(("AR Verify Mode\n"));
      set_monitor_alpp(&talp);
   }   

   if (lGetString(ar, AR_pe)) {
      lSetString(dummy_job, JB_pe, lGetString(ar, AR_pe));
      lSetList(dummy_job, JB_pe_range, lCopyList("", lGetList(ar, AR_pe_range)));

      result = sge_select_parallel_environment(&a, master_pe_list);
   } else {
      result = sge_sequential_assignment(&a);
   }

   /* stop redirection of scheduler monitoring messages */
   if (verify_mode == AR_JUST_VERIFY) {
      /* copy error msgs from talp into alpp */
      answer_list_append_list(alpp, &talp);

      set_monitor_alpp(NULL);

      if (result == DISPATCH_OK) {
         if (!a.pe) {
            answer_list_add_sprintf(alpp, STATUS_OK, ANSWER_QUALITY_INFO, MSG_JOB_VERIFYFOUNDQ); 
         } else {
            int ngranted = nslots_granted(a.gdil, NULL);
            answer_list_add_sprintf(alpp, STATUS_OK, ANSWER_QUALITY_INFO, MSG_JOB_VERIFYFOUNDSLOTS_I, ngranted);
         }
      } else {
         answer_list_add_sprintf(alpp, STATUS_ESEMANTIC, ANSWER_QUALITY_INFO, MSG_JOB_NOSUITABLEQ_S, MSG_JOB_VERIFYVERIFY);
      }
      /* ret has to be false in verify mode, otherwise the framework adds the object to the master list */
      ret = false;
   } else {
      if (result != DISPATCH_OK) {
         answer_list_add_sprintf(alpp, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR, MSG_JOB_NOSUITABLEQ_S, SGE_OBJ_AR);
         ret = false;
      } else {
         lSetList(ar, AR_granted_slots, a.gdil);
         a.gdil = NULL;

         ar_do_reservation(ar, true);
      }
   }

   /* stop dreaming */
   sconf_set_qs_state(QS_STATE_FULL);

   /* free all job lists */
   for (i = SPLIT_FIRST; i < SPLIT_LAST; i++) {
      if (splitted_job_lists[i]) {
         lFreeList(splitted_job_lists[i]);
         splitted_job_lists[i] = NULL;
      }
   }

   lFreeList(&(a.queue_list));
   lFreeList(&all_queue_list);
   lFreeList(&master_pe_list);
   lFreeList(&master_exechost_list);
   lFreeElem(&dummy_job);

   assignment_release(&a);

   DRETURN(ret);
}

/****** sge_advance_reservation_qmaster/ar_do_reservation() ********************
*  NAME
*     ar_do_reservation() -- do the reservation in the selected queue instances
*
*  SYNOPSIS
*     int ar_do_reservation(lListElem *ar, bool incslots) 
*
*  FUNCTION
*     This function does the (un)reserveration in the selected parallel environment
*     and the selected queue instances
*
*  INPUTS
*     lListElem *ar - ar object (AR_Type)
*     bool incslots - increase or decrease usage
*
*  RESULT
*     int - 0
*
*  NOTES
*     MT-NOTE: ar_do_reservation() is not MT safe 
*
*  SEE ALSO
*     sge_resource_utilization/rqs_add_job_utilization()
*******************************************************************************/
int ar_do_reservation(lListElem *ar, bool incslots)
{
   lListElem *dummy_job = lCreateElem(JB_Type);
   lListElem *qep;
   lListElem *global_host_ep = NULL;
   int pe_slots = 0;
   int tmp_slots;
   const char *granted_pe = lGetString(ar, AR_pe);
   u_long32 start_time = lGetUlong(ar, AR_start_time);
   u_long32 duration = lGetUlong(ar, AR_duration);
   object_description *object_base = object_type_get_object_description();
   lList *master_cqueue_list = *object_base[SGE_TYPE_CQUEUE].list;
   lList *master_centry_list = *object_base[SGE_TYPE_CENTRY].list;
   lList *master_exechost_list = *object_base[SGE_TYPE_EXECHOST].list;
   lList *master_pe_list = *object_base[SGE_TYPE_PE].list;

   DENTER(TOP_LAYER, "ar_do_reservation");

   lSetList(dummy_job, JB_hard_resource_list, lCopyList("", lGetList(ar, AR_resource_list)));
   lSetList(dummy_job, JB_hard_queue_list, lCopyList("", lGetList(ar, AR_queue_list)));

   global_host_ep = host_list_locate(master_exechost_list, SGE_GLOBAL_NAME);

   for_each(qep, lGetList(ar, AR_granted_slots)) {
      lListElem *host_ep = NULL;
      const char *queue_hostname = NULL;
      const char *queue_name = lGetString(qep, JG_qname);
      lListElem *queue = cqueue_list_locate_qinstance(master_cqueue_list, queue_name);

      if (!queue) {
         continue;
      }

      queue_hostname = lGetHost(queue, QU_qhostname);
      
      if (!incslots) {
         tmp_slots = -lGetUlong(qep, JG_slots);
      } else {
         tmp_slots = lGetUlong(qep, JG_slots);
      }

      pe_slots += tmp_slots;

      /* reserve global host */
      if (rc_add_job_utilization(dummy_job, 0, SCHEDULING_RECORD_ENTRY_TYPE_RESERVING,
                                 global_host_ep, master_centry_list, tmp_slots,
                                 EH_consumable_config_list, EH_resource_utilization,
                                 SGE_GLOBAL_NAME, start_time, duration, GLOBAL_TAG,
                                 false) != 0) {
         /* this info is not spooled */
         sge_add_event(0, sgeE_EXECHOST_MOD, 0, 0, 
                       SGE_GLOBAL_NAME, NULL, NULL, global_host_ep);
         lListElem_clear_changed_info(global_host_ep);
      }

      /* reserve exec host */
      host_ep = host_list_locate(master_exechost_list, queue_hostname);
      if (rc_add_job_utilization(dummy_job, 0, SCHEDULING_RECORD_ENTRY_TYPE_RESERVING,
                                 host_ep, master_centry_list, tmp_slots, EH_consumable_config_list,
                                 EH_resource_utilization, queue_hostname, start_time,
                                 duration, HOST_TAG, false) != 0) {
         /* this info is not spooled */
         sge_add_event(0, sgeE_EXECHOST_MOD, 0, 0, 
                       queue_hostname, NULL, NULL, host_ep);
         lListElem_clear_changed_info(host_ep);
      }

      /* reserve queue instance */
      rc_add_job_utilization(dummy_job, 0, SCHEDULING_RECORD_ENTRY_TYPE_RESERVING,
                             queue, master_centry_list, tmp_slots, QU_consumable_config_list,
                             QU_resource_utilization, queue_name, start_time, duration,
                             QUEUE_TAG, false);

      /* this info is not spooled */
      qinstance_add_event(queue, sgeE_QINSTANCE_MOD);
   }

   if (granted_pe != NULL) {
      lListElem *pe = pe_list_locate(master_pe_list, granted_pe);

      if (!pe) {
         ERROR((SGE_EVENT, MSG_OBJ_UNABLE2FINDPE_S, granted_pe));
      } else {
         utilization_add(lFirst(lGetList(pe, PE_resource_utilization)), start_time,
                                duration, pe_slots, 0, 0, PE_TAG, granted_pe,
                                SCHEDULING_RECORD_ENTRY_TYPE_RESERVING, false);
         sge_add_event(0, sgeE_PE_MOD, 0, 0, granted_pe, NULL, NULL, pe);
         lListElem_clear_changed_info(pe);
      }
   }

   lFreeElem(&dummy_job);

   DRETURN(0);
}
