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
#include "sgeobj/msg_sgeobjlib.h"
#include "msg_qmaster.h"

#include "sge_lock.h"
#include "sge_mtutil.h"
#include "uti/sge_time.h"
#include "sge_utility.h"
#include "sge_range.h"

#include "sge_utility_qmaster.h"

typedef struct {
   u_long32 ar_id;
   bool changed;
   pthread_mutex_t ar_id_mutex;
} ar_id_t;

ar_id_t ar_id_control = {0, false, PTHREAD_MUTEX_INITIALIZER};

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
   /*   AR_owner, SGE_STRING */
   attr_mod_str(alpp, ar, new_ar, AR_owner, object->object_name);
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
   attr_mod_sub_list(alpp, new_ar , AR_resource_list, AR_name, ar, sub_command, SGE_ATTR_COMPLEX_VALUES, SGE_OBJ_AR, 0); 
   /*   AR_queue_list, SGE_LIST */
   attr_mod_sub_list(alpp, new_ar , AR_queue_list, AR_name, ar, sub_command, SGE_ATTR_QUEUE_LIST, SGE_OBJ_AR, 0); 
   /*   AR_mail_options, SGE_ULONG   */
   attr_mod_ulong(ar, new_ar, AR_mail_options, object->object_name);
   /*   AR_mail_list, SGE_LIST */
   attr_mod_sub_list(alpp, new_ar, AR_mail_list, AR_name, ar, sub_command, SGE_ATTR_MAIL_LIST, SGE_OBJ_AR, 0); 
   /*   AR_pe, SGE_STRING */
   attr_mod_zerostr(ar, new_ar, AR_pe, object->object_name);
   /*   AR_pe_range, SGE_LIST */
   attr_mod_sub_list(alpp, new_ar, AR_pe_range, AR_name, ar, sub_command, SGE_ATTR_PE_LIST, SGE_OBJ_AR, 0); 
   /*   AR_acl_list, SGE_LIST */
   attr_mod_sub_list(alpp, new_ar, AR_acl_list, AR_name, ar, sub_command, SGE_ATTR_USER_LISTS, SGE_OBJ_AR, 0);
   /*   AR_xacl_list, SGE_LIST */
   attr_mod_sub_list(alpp, new_ar, AR_xacl_list, AR_name, ar, sub_command, SGE_ATTR_XUSER_LISTS, SGE_OBJ_AR, 0); 
   /*   AR_type, SGE_ULONG     */
   attr_mod_ulong(ar, new_ar, AR_type, object->object_name);

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

   DENTER(TOP_LAYER, "ar_success");

   ev = te_new_event((time_t)lGetUlong(ep, AR_start_time), TYPE_AR_EVENT, ONE_TIME_EVENT, lGetUlong(ep, AR_id), AR_RUNNING, NULL);
   te_add_event(ev);
   te_free_event(&ev);

   /*
   ** return element with correct id
   */
   if (ppList != NULL) {
      if (*ppList == NULL) {
         *ppList = lCreateList("", AR_Type);
      }   
      lAppendElem(*ppList, lCopyElem(ep)); 
   }


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
      sge_dstring_sprintf(&buffer, sge_u32, ar_id);

      /* remove timer for this advance reservation */
      te_delete_one_time_event(TYPE_CALENDAR_EVENT, ar_id, AR_RUNNING, NULL);
      te_delete_one_time_event(TYPE_CALENDAR_EVENT, ar_id, AR_EXITED, NULL);

      found = lDechainElem(*ar_list, found);

      sge_event_spool(ctx, alpp, 0, sgeE_AR_DEL, 
                      0, 0, sge_dstring_get_string(&buffer), NULL, NULL,
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
      dstring buffer = DSTRING_INIT;
      sge_dstring_sprintf(&buffer, "%d", ar_id);

      /* AR TBD CLEANUP 
       * for now we only remove the AR object 
       */
      DPRINTF(("AR: exited, removing AR %s\n", sge_dstring_get_string(&buffer)));
      lRemoveElem(*(object_type_get_master_list(SGE_TYPE_AR)), &ar);
      sge_event_spool(ctx, NULL, 0, sgeE_AR_DEL, 
                      0, 0, sge_dstring_get_string(&buffer), NULL, NULL,
                      NULL, NULL, NULL, true, true);

      sge_dstring_free(&buffer);
   } else {
      /* AR_RUNNING */
      DPRINTF(("AR: started, changing state of AR "sge_u32"\n", ar_id));

      lSetUlong(ar, AR_state, state);
      ev = te_new_event((time_t)lGetUlong(ar, AR_end_time), TYPE_AR_EVENT, ONE_TIME_EVENT, ar_id, AR_EXITED, NULL);
      te_add_event(ev);
      te_free_event(&ev);
   }

   SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);

   DRETURN_VOID;
}
