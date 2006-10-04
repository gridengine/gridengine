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

#include <ctype.h>
#include <string.h>
#include "jni.h"
#include "jgdi.h"
#include "basis_types.h"
#include "cull.h"
#include "commlib.h"
#include "sgermon.h"
#include "sge_all_listsL.h"
#include "sge_answer.h"
#include "sge_prog.h"
#include "sge_bootstrap.h"
#include "sge_gdi.h"
#include "sge_gdi_ctx.h"
#include "sge_gdi2.h"
#include "cl_errors.h"
#include "setup.h"
#include "sge_log.h"
#include "sge_error_class.h"
#include "jgdi_common.h"
#include "sge_event_client2.h"
#include "jgdi_event.h"
#include "sge_mirror.h"
#include "sge_eventL.h"
#include "jgdi_wrapper.h"
#include "jgdi_logging.h"
#define MAX_EVC_ARRAY_SIZE 1024
static pthread_mutex_t sge_evc_mutex = PTHREAD_MUTEX_INITIALIZER;
static sge_evc_class_t* sge_evc_array[MAX_EVC_ARRAY_SIZE];
static jgdi_result_t process_event(JNIEnv *env,  jobject eventList, lListElem *ev, lList** alpp);
                                             
static jgdi_result_t fill_job_usage_event(JNIEnv *env, jobject event_obj, lListElem *ev, lList **alpp);
static jgdi_result_t fill_job_event(JNIEnv *env, jobject event_obj, lListElem *ev, lList **alpp);
#define JGDI_LAYER      TOP_LAYER



/*
 * Class:     com_sun_grid_jgdi_jni_AbstractEventClient
 * Method:    closeNative
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_jgdi_jni_AbstractEventClient_closeNative(JNIEnv *env, jobject evcobj, jint evc_index)
{
   sge_evc_class_t *evc = NULL;
   rmon_ctx_t rmon_ctx;

   DENTER(TOP_LAYER, "Java_com_sun_grid_jgdi_jni_EventClient_closeNative");
   
   jgdi_init_rmon_ctx(env, JGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   pthread_mutex_lock(&sge_evc_mutex);
   evc = sge_evc_array[evc_index];
   sge_evc_array[evc_index] = NULL;
   pthread_mutex_unlock(&sge_evc_mutex);
   if (evc) {
      sge_evc_class_destroy(&evc);
   } else {
      THROW_ERROR((env, JGDI_ILLEGAL_STATE, "evc is NULL"));
   }   

   rmon_set_thread_ctx(NULL);
   jgdi_destroy_rmon_ctx(&rmon_ctx);
   
   DEXIT;
}

/*
 * Class:     com_sun_grid_jgdi_jni_AbstractEventClient
 * Method:    initNative
 * Signature: (Lcom/sun/grid/jgdi/jni/JGDI;I)I
 */
JNIEXPORT jint JNICALL Java_com_sun_grid_jgdi_jni_AbstractEventClient_initNative(JNIEnv *env, jobject evcobj, jobject jgdi, jint reg_id)
{
   char* argv[] = { "" };
   int argc = 1;

   sge_evc_class_t *evc = NULL;
   int evc_index = -1;
   int i;
   sge_gdi_ctx_class_t *sge_gdi_ctx = NULL;
   lList *alp = NULL;
   
   jgdi_result_t ret = JGDI_SUCCESS;
   rmon_ctx_t rmon_ctx;
   
   DENTER_MAIN(TOP_LAYER, "Java_com_sun_grid_jgdi_jni_AbstractEventClient_initNative");

   jgdi_init_rmon_ctx(env, JGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   if ((ret = getGDIContext(env, jgdi, &sge_gdi_ctx, &alp)) != JGDI_SUCCESS) {
      ret = JGDI_ILLEGAL_STATE;
      goto error;
   }

   evc = sge_evc_class_create(sge_gdi_ctx, reg_id, &alp); 
   if (!evc) {
      throw_error_from_answer_list(env, JGDI_ERROR, alp);
      DEXIT;
      return -1;
   }

   pthread_mutex_lock(&sge_evc_mutex);
   for (i=0; i<MAX_EVC_ARRAY_SIZE; i++) {
      if (sge_evc_array[i] == NULL) {
         sge_evc_array[i] = evc;
         evc_index = i;
         break;
      }   
   }
   pthread_mutex_unlock(&sge_evc_mutex);

   if (evc_index < 0) {
       sge_evc_class_destroy(&evc);
       ret = JGDI_ERROR;
       answer_list_add(&alp, "Too much jgdi connections", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
   }

error:

   if (ret != JGDI_SUCCESS) {
      throw_error_from_answer_list(env, ret, alp);
   }
   lFreeList(&alp);
   
   rmon_set_thread_ctx(NULL);
   jgdi_destroy_rmon_ctx(&rmon_ctx);
   
   DEXIT;
   return evc_index;
}

jgdi_result_t getEVC(JNIEnv *env, jobject evcobj, sge_evc_class_t **evc, lList **alpp) 
{
   int index = 0;
   jgdi_result_t ret = JGDI_SUCCESS;
   
   DENTER( JGDI_LAYER, "getEVC" );

   if ((ret = AbstractEventClient_getEVCIndex(env, evcobj, &index, alpp)) != JGDI_SUCCESS) {
      DEXIT;
      return ret;
   }
   *evc = sge_evc_array[index];
   DEXIT;
   return ret;
}


/*
 * Class:     com_sun_grid_jgdi_jni_AbstractEventClient
 * Method:    register
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_jgdi_jni_AbstractEventClient_register(JNIEnv *env, jobject evcobj)
{
   lList *alp = NULL;
   sge_evc_class_t *sge_evc = NULL;
   jgdi_result_t ret = JGDI_SUCCESS;
   rmon_ctx_t rmon_ctx;
   
   DENTER(JGDI_LAYER, "Java_com_sun_grid_jgdi_jni_AbstractEventClient_register");

   jgdi_init_rmon_ctx(env, JGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   if ((ret = getEVC(env, evcobj, &sge_evc, &alp)) != JGDI_SUCCESS) {
      throw_error_from_answer_list(env, ret, alp);
      DEXIT;
      return;
   }
   
   if( !sge_evc->ec_register(sge_evc, FALSE, &alp) ) {
      if (answer_list_has_error(&alp)) {
         throw_error_from_answer_list(env, JGDI_ERROR, alp);
      } else {
         throw_error(env, JGDI_ERROR,"ec_register returned false");
      }
   } else {
      ev_registration_id id = sge_evc->ec_get_id(sge_evc);
      DPRINTF(("event client with id %d successfully registered\n", id));
      if((ret = AbstractEventClient_setId(env, evcobj, (jint)id, &alp)) != JGDI_SUCCESS) {
         throw_error_from_answer_list(env, ret, alp);
      }
   }
   lFreeList(&alp);
   
   rmon_set_thread_ctx(NULL);
   jgdi_destroy_rmon_ctx(&rmon_ctx);
   
   DEXIT;
}

/*
 * Class:     com_sun_grid_jgdi_jni_AbstractEventClient
 * Method:    register
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_jgdi_jni_AbstractEventClient_deregister(JNIEnv *env, jobject evcobj)
{
   lList *alp = NULL;
   sge_evc_class_t *sge_evc = NULL;
   jgdi_result_t ret = JGDI_SUCCESS;
   ev_registration_id id = 0;
   rmon_ctx_t rmon_ctx;
   
   DENTER(JGDI_LAYER, "Java_com_sun_grid_jgdi_jni_AbstractEventClient_deregister");

   jgdi_init_rmon_ctx(env, JGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   if ((ret = getEVC(env, evcobj, &sge_evc, &alp)) != JGDI_SUCCESS) {
      throw_error_from_answer_list(env, ret, alp);
      DEXIT;
      return;
   }
   
   id = sge_evc->ec_get_id(sge_evc);

   if( !sge_evc->ec_deregister(sge_evc) ) {
      if (answer_list_has_error(&alp)) {
         throw_error_from_answer_list(env, JGDI_ERROR, alp);
      } else {
         throw_error(env, JGDI_ERROR,"ec_deregister returned false");
      }
   } else {
      DPRINTF(("event client with id %d successfully deregistered\n", id));
   }
   lFreeList(&alp);
   rmon_set_thread_ctx(NULL);
   jgdi_destroy_rmon_ctx(&rmon_ctx);
   DEXIT;
}


/*
 * Class:     com_sun_grid_jgdi_jni_EventClient
 * Method:    subscribeAllNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_jgdi_jni_AbstractEventClient_subscribeAllNative(JNIEnv *env, jobject evcobj)
{
   lList *alp = NULL;
   sge_evc_class_t *sge_evc = NULL;
   jgdi_result_t ret = JGDI_SUCCESS;
   rmon_ctx_t rmon_ctx;

   DENTER(JGDI_LAYER, "Java_com_sun_grid_jgdi_jni_AbstractEventClient_subscribeAllNative");

   jgdi_init_rmon_ctx(env, JGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   if ((ret = getEVC(env, evcobj, &sge_evc, &alp)) != JGDI_SUCCESS) {
      throw_error_from_answer_list(env, ret, alp);
      DEXIT;
      return;
   }
   
   if (!sge_evc->ec_subscribe_all(sge_evc)) {
      THROW_ERROR((env, JGDI_ERROR, "ec_subscribeAll failed"));
      DEXIT;
      return;
   }
   
   rmon_set_thread_ctx(NULL);
   jgdi_destroy_rmon_ctx(&rmon_ctx);
   
   DEXIT;
}

JNIEXPORT void JNICALL Java_com_sun_grid_jgdi_jni_AbstractEventClient_nativeCommit(JNIEnv *env, jobject evcobj)
{
   lList *alp = NULL;
   sge_evc_class_t *sge_evc = NULL;
   jgdi_result_t ret = JGDI_SUCCESS;
   rmon_ctx_t rmon_ctx;

   DENTER(JGDI_LAYER, "Java_com_sun_grid_jgdi_jni_AbstractEventClient_nativeCommit");

   jgdi_init_rmon_ctx(env, JGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   if ((ret = getEVC(env, evcobj, &sge_evc, &alp)) != JGDI_SUCCESS) {
      throw_error_from_answer_list(env, ret, alp);
      goto error;
   }
   
   if (!sge_evc->ec_commit(sge_evc, &alp)) {
      throw_error_from_answer_list(env, JGDI_ERROR, alp);
      goto error;
   }

error:

   rmon_set_thread_ctx(NULL);
   jgdi_destroy_rmon_ctx(&rmon_ctx);
   
   DEXIT;
}


/*
 * Class:     com_sun_grid_jgdi_jni_EventClient
 * Method:    unsubscribeAllNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_jgdi_jni_AbstractEventClient_unsubscribeAllNative(JNIEnv *env, jobject evcobj)
{
   lList *alp = NULL;
   sge_evc_class_t *sge_evc = NULL;
   jgdi_result_t ret = JGDI_SUCCESS;
   rmon_ctx_t rmon_ctx;

   DENTER(JGDI_LAYER, "Java_com_sun_grid_jgdi_jni_AbstractEventClient_unsubscribeAllNative");

   jgdi_init_rmon_ctx(env, JGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   if ((ret = getEVC(env, evcobj, &sge_evc, &alp)) != JGDI_SUCCESS) {
      throw_error_from_answer_list(env, ret, alp);
      DEXIT;
      return;
   }

   if (!sge_evc->ec_unsubscribe_all(sge_evc)) {
      THROW_ERROR((env, JGDI_ERROR, "ec_unsubscribeAll failed"));
      DEXIT;
      return;
   }
   
   rmon_set_thread_ctx(NULL);
   jgdi_destroy_rmon_ctx(&rmon_ctx);
   DEXIT;
}



/*
 * Class:     com_sun_grid_jgdi_jni_EventClient
 * Method:    fillEvents
 * Signature: (Ljava/util/List;)V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_jgdi_jni_AbstractEventClient_fillEvents(JNIEnv *env, jobject evcobj, jobject eventList)
{
   lList *elist = NULL;
   lList *alp = NULL;
   lListElem *ev = NULL;
   sge_evc_class_t *sge_evc = NULL;
   jgdi_result_t ret = JGDI_SUCCESS;
   jobject logger = NULL;
   rmon_ctx_t rmon_ctx;
   
   DENTER(JGDI_LAYER, "Java_com_sun_grid_jgdi_jni_AbstractEventClient_fillEvents");

   jgdi_init_rmon_ctx(env, JGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   if ((ret = getEVC(env, evcobj, &sge_evc, &alp)) != JGDI_SUCCESS) {
      throw_error_from_answer_list(env, ret, alp);
      DEXIT;
      return;
   }

   sge_evc->ec_get(sge_evc, &elist, FALSE);

   jgdi_log_printf(env, JGDI_EVENT_LOGGER, FINE,
                   "BEGIN ------------- event list --------------");
   
   jgdi_log_list(env, JGDI_EVENT_LOGGER, FINE, elist);
   
   jgdi_log_printf(env, JGDI_EVENT_LOGGER, FINE,
                   "END ------------- event list --------------");

   for_each(ev, elist) {
      
      ret = process_event(env, eventList, ev, &alp);
      
      if(ret != JGDI_SUCCESS) {
         
         if (logger == NULL) {
            logger = jgdi_get_logger(env, JGDI_EVENT_LOGGER);
         }
         if (logger != NULL && jgdi_is_loggable(env, logger, WARNING) ) {
            
            dstring ds = DSTRING_INIT;
            
            answer_list_to_dstring(alp, &ds);
            lFreeList(&alp);
            jgdi_log(env, logger, WARNING, sge_dstring_get_string(&ds));
            sge_dstring_free(&ds);
         }
      }
   }
   
   {
      jint size = 0;
      if((ret = List_size(env, eventList, &size, &alp)) != JGDI_SUCCESS) {
         throw_error_from_answer_list(env, JGDI_ILLEGAL_STATE, alp);
         lFreeList(&alp);
         DEXIT;
         return;
      }
      DPRINTF(("Received %d events\n", size));
   }
   
   rmon_set_thread_ctx(NULL);
   jgdi_destroy_rmon_ctx(&rmon_ctx);
      
   DEXIT;
}

static jgdi_result_t process_event(JNIEnv *env,  jobject eventList, lListElem *ev, lList** alpp) {
   
   jgdi_result_t ret = JGDI_SUCCESS;
   u_long32 event_type = lGetUlong(ev, ET_type);
   jobject event = NULL;
   jlong timestamp = 0;
   jint  evtId = 0;
   
   DENTER(JGDI_LAYER, "process_event");
   
   evtId = lGetUlong(ev, ET_number);
   timestamp = lGetUlong(ev, ET_timestamp);
   
   switch(event_type) {

      case sgeE_JOB_MOD_SCHED_PRIORITY:     /*35 + event job modify priority */
      
      case sgeE_JOB_SCHEDD_INFO_LIST:       /*39 + send job schedd info list at registration */
      case sgeE_JOB_SCHEDD_INFO_ADD:        /*40 - event jobs schedd info added */
      case sgeE_JOB_SCHEDD_INFO_DEL:        /*41 - event jobs schedd info deleted */
      case sgeE_JOB_SCHEDD_INFO_MOD:        /*42 + event jobs schedd info modified */

      case sgeE_NEW_SHARETREE:               /*51 + replace possibly existing share tree */

      case sgeE_QINSTANCE_ADD:              /*65 + event queue instance add */
      case sgeE_QINSTANCE_DEL:              /*66 + event queue instance delete */
      case sgeE_QINSTANCE_MOD:              /*67 + event queue instance mod */
      case sgeE_QINSTANCE_SOS:              /*68 + event queue instance sos */
      case sgeE_QINSTANCE_USOS:             /*69 + event queue instance usos */


        answer_list_add_sprintf(alpp, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR,
                               "handling event %d yet not implemented", event_type);
        ret = JGDI_ILLEGAL_STATE;
        break;
        
      case sgeE_JOB_FINAL_USAGE:            /*37 + event job final usage report after job end */
           ret = EventFactoryBase_static_createJobFinalUsageEvent(env, timestamp, evtId, &event, alpp);
           if(ret == JGDI_SUCCESS) {
              ret = fill_job_usage_event(env, event, ev, alpp);
           }
           break;
      case sgeE_JOB_USAGE:                  /*36 + event job online usage */
           ret = EventFactoryBase_static_createJobUsageEvent(env, timestamp, evtId, &event, alpp);
           if(ret == JGDI_SUCCESS) {
              ret = fill_job_usage_event(env, event, ev, alpp);
           }
           break;
      case sgeE_JOB_FINISH:                 /*38 + job finally finished or aborted (user view) */
           ret = EventFactoryBase_static_createJobFinishEvent(env, timestamp, evtId, &event, alpp);
           if(ret == JGDI_SUCCESS) {
              ret = fill_job_event(env, event, ev, alpp);
           }
           break;
      case sgeE_QMASTER_GOES_DOWN:
           ret = EventFactoryBase_static_createQmasterGoesDownEvent(env, timestamp, evtId, &event, alpp);
           break;
      case sgeE_SCHEDDMONITOR:
           ret = EventFactoryBase_static_createSchedulerRunEvent(env, timestamp, evtId, &event, alpp);
           break;
      case sgeE_SHUTDOWN:        
           ret = EventFactoryBase_static_createShutdownEvent(env, timestamp, evtId, &event, alpp);
           break;
      default:
         ret = process_generic_event(env, &event, ev, alpp);

   }
   
   if(ret == JGDI_SUCCESS) {
      jboolean addResult = false;
      ret = List_add(env, eventList, event, &addResult, alpp);
   }
   
   DEXIT;
   return ret;
   
}


static jgdi_result_t fill_job_usage_event(JNIEnv *env, jobject event_obj, 
                                            lListElem *ev, lList **alpp) {
     jgdi_result_t ret = JGDI_SUCCESS;
     lListElem *ep = NULL;
     
     DENTER(JGDI_LAYER, "fill_job_event");
     ret = fill_job_event(env, event_obj, ev, alpp);
     
     /* Job usage events has a list of UA_Type in the ET_new_version */
     for_each(ep, lGetList(ev, ET_new_version)) {
        const char* name = lGetString(ep, UA_name);
        jdouble value = (jdouble)lGetDouble(ep, UA_value);
        
        ret = JobUsageEvent_addUsage(env, event_obj, name, value, alpp);
        if (ret != JGDI_SUCCESS) {
           break;
        }
     }
     DEXIT;
     return ret;
}

static jgdi_result_t fill_job_event(JNIEnv *env, jobject event_obj, lListElem *ev, lList **alpp) {                                               
     jgdi_result_t ret = JGDI_SUCCESS;
     u_long32 job_id = lGetUlong(ev, ET_intkey);
     u_long32 ja_task_id = lGetUlong(ev, ET_intkey2);
     const char* pe_task_id = lGetString(ev, ET_strkey);
     
     DENTER(JGDI_LAYER, "fill_job_event");
     
     ret = JobEvent_setJobId(env, event_obj, (jint)job_id, alpp);
     if(ret != JGDI_SUCCESS) {
        DEXIT;
        return ret;
     }
     ret = JobEvent_setTaskId(env, event_obj, (jint)ja_task_id, alpp);
     if(ret != JGDI_SUCCESS) {
        DEXIT;
        return ret;
     }
     ret = JobEvent_setPeTaskId(env, event_obj, pe_task_id, alpp);
     DEXIT;
     return ret;
}
   
jgdi_result_t create_generic_event(JNIEnv *env, jobject *event_obj, const char* beanClassName, 
                                 const char* cullTypeName, lDescr *descr, int event_action, lListElem *ev, lList **alpp)
{
   jclass beanClass = NULL;
   jobject event = NULL;
   jlong timestamp = 0;
   jint  evtId = 0;
   lListElem *ep = NULL;
   jgdi_result_t ret = JGDI_SUCCESS;
   
   DENTER(JGDI_LAYER, "handleEvent");

   evtId = lGetUlong(ev, ET_number);
   timestamp = lGetUlong(ev, ET_timestamp);

   if (!descr) {
      answer_list_add(alpp, "descr is NULL", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR); 
      DEXIT;
      return JGDI_ILLEGAL_ARGUMENT;
   }   
   if (beanClassName == NULL) {
      answer_list_add(alpp, "beanClassName is NULL", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR); 
      DEXIT;
      return JGDI_ILLEGAL_ARGUMENT;
   }

   beanClass = (*env)->FindClass(env, beanClassName);
   if (test_jni_error(env, "handleEvent: FindClass failed", alpp)) {
      DEXIT;
      return JGDI_ILLEGAL_ARGUMENT;
   }

   switch (event_action) {
      case SGE_EMA_LIST:
         DPRINTF(("Handle list event\n"));
         ret = EventFactory_static_createListEvent(env, cullTypeName, timestamp, evtId, &event, alpp);
         break;
      case SGE_EMA_ADD:
         DPRINTF(("Handle add event\n"));
         ret = EventFactory_static_createAddEvent(env, cullTypeName, timestamp, evtId, &event, alpp);
         break;
      case SGE_EMA_MOD:
         DPRINTF(("Handle mod event\n"));
         ret = EventFactory_static_createModEvent(env, cullTypeName, timestamp, evtId, &event, alpp);
         break;
      case SGE_EMA_DEL:
         DPRINTF(("Handle del event\n"));
         ret = EventFactory_static_createDelEvent(env, cullTypeName, timestamp, evtId, &event, alpp);
         break;
      default:
         answer_list_add_sprintf(alpp, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, 
                                 "Event action not yet handled %d", event_action);
                                 
         return JGDI_ILLEGAL_STATE;
   }

   if(ret != JGDI_SUCCESS) {
      DEXIT;
      return ret;
   }
   
   if ( event_action == SGE_EMA_LIST ) {
      jobject  obj = NULL;
      
      for_each(ep, lGetList(ev, ET_new_version)) {
         if ((ret = listelem_to_obj(env, ep, &obj, descr, beanClass, alpp )) != JGDI_SUCCESS) {
            DEXIT;
            return ret;
         }
         ret = ListEvent_add(env, event, obj, alpp);
         if(ret != JGDI_SUCCESS) {
            DEXIT;
            return ret;
         }
      }
   } else {
      jobject  obj = NULL;
      
      ret = ChangedObjectEvent_setPKInfo(env, event,
                                        lGetUlong(ev,ET_intkey),
                                        lGetUlong(ev,ET_intkey2),
                                        lGetString(ev,ET_strkey),
                                        lGetString(ev,ET_strkey2),
                                        alpp);
                                        
      if(ret != JGDI_SUCCESS) {
         DEXIT;
         return ret;
      }
      
      ep =lFirst(lGetList(ev, ET_new_version));
      
      /* DEL events do not have a new version */
      if (ep != NULL) {
         if ((ret = listelem_to_obj(env, ep, &obj, descr, beanClass, alpp )) != JGDI_SUCCESS) {
            DEXIT;
            return ret;
         }
         ret = ChangedObjectEvent_setChangedObject(env, event, obj, alpp);
         if(ret != JGDI_SUCCESS) {
            DEXIT;
            return ret;
         }
      } else if (event_action != SGE_EMA_DEL) {
         jclass evt_class = NULL;
         jstring evt_classname_obj = NULL;
         const char* evt_classname = NULL;
         
         ret = Object_getClass(env, obj, &evt_class, alpp);
         if (ret != JGDI_SUCCESS) {
            DEXIT;
            return ret;
         }
         
         ret = Class_getName(env, evt_class, &evt_classname_obj, alpp);
         if (ret != JGDI_SUCCESS) {
            DEXIT;
            return ret;
         }
         
         evt_classname = (*env)->GetStringUTFChars(env, evt_classname_obj, 0);
         jgdi_log_printf(env, JGDI_EVENT_LOGGER, WARNING,
                         "generic event did not contain a new version (%s)",
                         evt_classname);
                         
         (*env)->ReleaseStringUTFChars(env, evt_classname_obj, evt_classname);            
      }
   }
   
   *event_obj = event;
   
   DEXIT;
   return JGDI_SUCCESS;
}


