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
#include <jni.h>
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
#include "sge_log.h"
#include "sge_error_class.h"
#include "jgdi_common.h"
#include "jgdi.h"
#include "sge_qhost.h"
#include "sge_host.h"
#include "msg_common.h"
#include "jgdi_wrapper.h"
#include "jgdi_logging.h"

/* DICKER TODO: Aendere das aber flott */ 
int shut_me_down = 0;


typedef struct jgdi_report_handler_str jgdi_report_handler_t;

struct jgdi_report_handler_str {
   JNIEnv  *env;
   jgdi_result_t result;
   jobject qhost_result;
   jobject qhost_info;
};

static report_handler_t* jgdi_report_handler_create(JNIEnv *env, jobject qhost_result, lList **alpp);
static int jgdi_report_finished(report_handler_t* handler, lList **alpp);
static int jgdi_report_started(report_handler_t* handler, lList **alpp);
static int jgdi_report_host_begin(report_handler_t* handler, const char* host_name, lList **alpp);
static int jgdi_report_host_string_value(report_handler_t* handler, const char* name, const char *value, lList **alpp);
static int jgdi_report_host_ulong_value(report_handler_t* handler, const char* name, u_long32 value, lList **alpp);
static int jgdi_report_host_finished(report_handler_t* handler, const char* host_name, lList **alpp);
static int jgdi_report_resource_value(report_handler_t* handler, const char* dominance, const char* name, const char* value, lList **alpp);
static int jgdi_report_queue_string_value(report_handler_t* handler, const char* qname, const char* name, const char *value, lList **alpp);
static int jgdi_report_queue_ulong_value(report_handler_t* handler, const char* qname, const char* name, u_long32 value, lList **alpp);
static int jgdi_destroy_report_handler(report_handler_t** handler, lList **alpp);

static report_handler_t* jgdi_report_handler_create(JNIEnv *env, jobject qhost_result, lList **alpp) {
   jgdi_report_handler_t* jgdi_handler = (jgdi_report_handler_t*)sge_malloc(sizeof(jgdi_report_handler_t));
   report_handler_t *ret = NULL;
   DENTER( JGDI_LAYER, "jgdi_report_handler_create" );
   
   if (jgdi_handler == NULL ) {
      answer_list_add(alpp, "malloc of jgdi_report_handler_t failed",
                            STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return NULL;
   }
   
   ret = (report_handler_t*)sge_malloc(sizeof(report_handler_t));
   if (ret == NULL ) {
      answer_list_add(alpp, "malloc of report_handler_t failed",
                            STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
      FREE(jgdi_handler);  
      DEXIT;
      return NULL;
   }
   
   memset(jgdi_handler, 0, sizeof(jgdi_report_handler_t));
   memset(ret, 0, sizeof(report_handler_t));
   
   jgdi_handler->result = JGDI_SUCCESS;
   ret->ctx = jgdi_handler;
   ret->report_started = jgdi_report_started;
   ret->report_finished = jgdi_report_finished;
   
   ret->report_host_begin = jgdi_report_host_begin;
   ret->report_host_finished = jgdi_report_host_finished;
   
   ret->report_host_string_value = jgdi_report_host_string_value;
   ret->report_host_ulong_value = jgdi_report_host_ulong_value;
   
   ret->report_resource_value = jgdi_report_resource_value;
   ret->report_queue_string_value = jgdi_report_queue_string_value;
   ret->report_queue_ulong_value = jgdi_report_queue_ulong_value;
   ret->destroy = jgdi_destroy_report_handler;
   
   jgdi_handler->qhost_result = qhost_result;
   jgdi_handler->env = env;
   
   DEXIT;
   return ret;
}

static int jgdi_destroy_report_handler(report_handler_t** handler, lList **alpp) {
   DENTER( JGDI_LAYER, "jgdi_destroy_report_handler" );
   if (*handler != NULL ) {
      
      /* We ensurce that the global ref on the qhost_info object is deleted */
      jgdi_report_handler_t* jgdi_handler = (jgdi_report_handler_t*)(*handler)->ctx;
      if (jgdi_handler->qhost_info != NULL) {
         (*(jgdi_handler->env))->DeleteGlobalRef(jgdi_handler->env, jgdi_handler->qhost_info);
         jgdi_handler->qhost_info = NULL;
      }
      
      /* Free the internal context */
      FREE((*handler)->ctx);
   }
   DEXIT;
   return QHOST_SUCCESS;
}


static int jgdi_report_finished(report_handler_t* handler, lList **alpp) {
   DENTER( JGDI_LAYER, "jgdi_report_finished" );
   DEXIT;
   return QHOST_SUCCESS;
}

static int jgdi_report_started(report_handler_t* handler, lList **alpp) {
   DENTER( JGDI_LAYER, "jgdi_report_started" );
   DEXIT;
   return QHOST_SUCCESS;
}

static int jgdi_report_host_begin(report_handler_t* handler, const char* host_name, lList **alpp) {
   jgdi_report_handler_t* jgdi_handler = (jgdi_report_handler_t*)handler->ctx;
   JNIEnv *env = jgdi_handler->env;
   jobject qhost_info = NULL;
   
   DENTER( JGDI_LAYER, "jgdi_report_host_begin" );

   DPRINTF(("Create new host info object for host %s\n", host_name));
  
   jgdi_handler->result = HostInfoImpl_init_0(env, &qhost_info, host_name, alpp);
   if (jgdi_handler->result != JGDI_SUCCESS) {
      DEXIT;
      return QHOST_ERROR;
   }
   
   jgdi_handler->qhost_info = (*env)->NewGlobalRef(env, qhost_info);
   
   (*env)->DeleteLocalRef(env, qhost_info);
   
   if (jgdi_handler->qhost_info == NULL) {
      answer_list_add(alpp , "Can not create global reference for qhost info object", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return QHOST_ERROR;
   }
   
   DEXIT;
   return QHOST_SUCCESS;
}

static int jgdi_report_host_string_value(report_handler_t* handler, const char* name, const char *value, lList** alpp) {
   
   jgdi_report_handler_t* jgdi_handler = (jgdi_report_handler_t*)handler->ctx;
   JNIEnv *env = jgdi_handler->env;
   jstring value_obj = NULL;
   
   DENTER( JGDI_LAYER, "jgdi_report_host_string_value" );
   
   DPRINTF(("add host value %s=%s\n", name, value));
   
   value_obj = (*env)->NewStringUTF(env, value);
   
   if (HostInfoImpl_addHostValue(env, jgdi_handler->qhost_info, name, value_obj, alpp) != JGDI_SUCCESS) {
     DEXIT;
     return QHOST_ERROR;
   }
   
   DEXIT;
   return QHOST_SUCCESS;
}


static int jgdi_report_host_ulong_value(report_handler_t* handler, const char* name, u_long32 value, lList** alpp) {
   jgdi_report_handler_t* jgdi_handler = (jgdi_report_handler_t*)handler->ctx;
   JNIEnv *env = jgdi_handler->env;
   jobject value_obj = NULL;
   
   DENTER( JGDI_LAYER, "jgdi_report_host_ulong_value" );

   DPRINTF(("add host value %s=%ld\n", name, value));
   
   jgdi_handler->result = Long_init_0(env, &value_obj, value, alpp);
   if (jgdi_handler->result != JGDI_SUCCESS) {
     DEXIT;
     return QHOST_ERROR;
   }
   
   if (HostInfoImpl_addHostValue(env, jgdi_handler->qhost_info, name, value_obj, alpp) != JGDI_SUCCESS) {
     DEXIT;
     return QHOST_ERROR;
   }
   DEXIT;
   return QHOST_SUCCESS;
}

static int jgdi_report_host_finished(report_handler_t* handler, const char* host_name, lList** alpp) {
   jgdi_report_handler_t* jgdi_handler = (jgdi_report_handler_t*)handler->ctx;
   JNIEnv *env = jgdi_handler->env;

   DENTER( JGDI_LAYER, "jgdi_report_host_finished" );
   
   if (jgdi_handler->qhost_info == NULL) {
      answer_list_add(alpp, "qhost_info object is not available in jgdi_handler",
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return QHOST_ERROR;
   }

   if (jgdi_handler->qhost_result == NULL) {
      DPRINTF(("jgdi_handler->qhost_result is NULL\n"));
      abort();
   }
   
   if (QHostResultImpl_addHostInfo(env, jgdi_handler->qhost_result, jgdi_handler->qhost_info, alpp) != JGDI_SUCCESS) {
     DEXIT;
     return QHOST_ERROR;
   }
   DPRINTF(("DeleteGlobalRef\n"));
   (*env)->DeleteGlobalRef(env, jgdi_handler->qhost_info);
   jgdi_handler->qhost_info = NULL;
   
   DEXIT;
   return QHOST_SUCCESS;
}

static int jgdi_report_resource_value(report_handler_t* handler, const char* dominance, const char* name, const char* value, lList** alpp) {
   jgdi_report_handler_t* jgdi_handler = (jgdi_report_handler_t*)handler->ctx;
   JNIEnv *env = jgdi_handler->env;
   jstring value_obj = NULL;
   
   DENTER( JGDI_LAYER, "jgdi_report_resource_value" );
   
   if (jgdi_handler->qhost_info == NULL) {
      answer_list_add(alpp, "jgdi_report_resource_value: qhost_info object not set", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return QHOST_ERROR;
   }
   
   value_obj = (*env)->NewStringUTF(env, value);

   if (HostInfoImpl_addResourceValue(env, jgdi_handler->qhost_info, dominance, name, value_obj, alpp) != JGDI_SUCCESS) { 
     DEXIT;
     return QHOST_ERROR;
   }

   DEXIT;
   return QHOST_SUCCESS;
}

static int jgdi_report_queue_string_value(report_handler_t* handler, const char* qname, const char* name, const char *value, lList** alpp) {
   jgdi_report_handler_t* jgdi_handler = (jgdi_report_handler_t*)handler->ctx;
   JNIEnv *env = jgdi_handler->env;
   jstring value_obj = NULL;
   char qname1[1024];
   char name1[1024];
   DENTER( JGDI_LAYER, "jgdi_report_queue_string_value" );
   
   strcpy(qname1, qname);
   strcpy(name1, name);
   value_obj = (*env)->NewStringUTF(env, value);
   
   DPRINTF(("addQueueValue: %s, %s, %s\n", qname1, name1, value));   
   if (HostInfoImpl_addQueueValue(env, jgdi_handler->qhost_info, qname1, name1, value_obj, alpp) != JGDI_SUCCESS) {
     DEXIT;
     return QHOST_ERROR;
   }
   DEXIT;
   return QHOST_SUCCESS;
}

static int jgdi_report_queue_ulong_value(report_handler_t* handler, const char* qname, const char* name, u_long32 value, lList** alpp) {
   jgdi_report_handler_t* jgdi_handler = (jgdi_report_handler_t*)handler->ctx;
   JNIEnv *env = jgdi_handler->env;
   jobject value_obj = NULL;
   DENTER( JGDI_LAYER, "jgdi_report_queue_ulong_value" );
   
   DPRINTF(("addQueueValue: %s, %s, %ld\n", qname, name, value));   
   
   jgdi_handler->result = Long_init_0(env, &value_obj, value, alpp);
   if (jgdi_handler->result != JGDI_SUCCESS) {
     DEXIT;
     return QHOST_ERROR;
   }
   if (HostInfoImpl_addQueueValue(env, jgdi_handler->qhost_info, qname, name, value_obj, alpp) != JGDI_SUCCESS) {
     DEXIT;
     return QHOST_ERROR;
   }
   DEXIT;
   return QHOST_SUCCESS;
}

/*
 * Class:     com_sun_grid_jgdi_jni_JGDIImpl
 * Method:    execQHost
 * Signature: (Lcom/sun/grid/jgdi/monitoring/QHostOptions;Lcom/sun/grid/jgdi/monitoring/QHostResultImpl;)V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_jgdi_jni_JGDIBase_execQHost
  (JNIEnv *env, jobject jgdi, jobject qhost_options, jobject qhost_result) {
   
   jclass    cls = NULL;
   jmethodID mid = NULL;
   jobject   sub_object = NULL;
   lList     *alp = NULL;
   u_long32  show = 0;
   
   struct filter_def {
      const char* getter;
      const char* signature;
      const char* getListFunc;
      lList *list;
   };
   
   struct filter_def filters [] = {
      { "getHostFilter", "()Lcom/sun/grid/jgdi/monitoring/filter/HostFilter;", "getHosts", NULL },
      { "getUserFilter", "()Lcom/sun/grid/jgdi/monitoring/filter/UserFilter;", "getUsers", NULL },
      { "getResourceFilter", "()Lcom/sun/grid/jgdi/monitoring/filter/ResourceFilter;", "getResources", NULL },
      { "getResourceAttributeFilter", "()Lcom/sun/grid/jgdi/monitoring/filter/ResourceAttributeFilter;", "getValueNames", NULL },
      { NULL, NULL, NULL, NULL }
   };
   
   int i;
   sge_gdi_ctx_class_t * ctx = NULL;
   
   jgdi_result_t ret = JGDI_SUCCESS;
   rmon_ctx_t rmon_ctx;
   
   DENTER( JGDI_LAYER, "Java_com_sun_grid_jgdi_jni_JGDIBase_execQHost" );
   
   jgdi_init_rmon_ctx(env, JGDI_QHOST_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   ret = getGDIContext(env, jgdi, &ctx, &alp);
   
   if (ret != JGDI_SUCCESS) {
      goto error;
   }
   
   sge_gdi_set_thread_local_ctx(ctx);

   cls = QHostOptions_find_class(env, &alp);
   if (cls == NULL) {
      ret = JGDI_ILLEGAL_STATE;
      goto error;
   }
   
   /* build the show bitmask */
   {
      jboolean  is_show_queue = false;
      jboolean  is_show_jobs  = false;
      
      if ((ret=QHostOptions_includeQueue(env, qhost_options, &is_show_queue, &alp)) != JGDI_SUCCESS) {
         goto error;
      }
      if((ret=QHostOptions_includeJobs(env, qhost_options, &is_show_jobs, &alp)) != JGDI_SUCCESS) {
         goto error;
      }
    
      if( is_show_queue ) {
         DPRINTF(("execQHost: show queues\n"));
         show |= QHOST_DISPLAY_QUEUES;
      }
      if( is_show_jobs ) {
         DPRINTF(("execQHost: show jobs\n"));
         show |= QHOST_DISPLAY_JOBS;
      }
   }
   
   for(i = 0; filters[i].getter != NULL; i++ ) {
      
      mid = get_methodid(env, cls, filters[i].getter, filters[i].signature, &alp);
      if (!mid) {
         ret = JGDI_ILLEGAL_STATE;
         break;
      }
      sub_object = (*env)->CallObjectMethod(env, qhost_options, mid);
      
      if (test_jni_error( env, "Java_com_sun_grid_jgdi_jni_JGDIBase_execQHost: Unexpected error while getting sub_object", &alp)) {
         ret = JGDI_ILLEGAL_STATE;
         break;
      }
      
      if (sub_object != NULL) {
         if ((ret=get_string_list(env, sub_object, filters[i].getListFunc, &(filters[i].list), ST_Type, ST_name, &alp)) != JGDI_SUCCESS) {
            break;
         }
         if(strcmp(filters[i].getter, "getResourceAttributeFilter") == 0) {
            show |= QHOST_DISPLAY_RESOURCES;
         } else if (strcmp(filters[i].getter, "getHostFilter") == 0) {
            lListElem *ep = NULL;
            /* 
            ** resolve hostnames and replace them in list
            */
            for_each(ep, filters[i].list) {
               if (sge_resolve_host(ep, ST_name) != CL_RETVAL_OK) {
                  answer_list_add_sprintf(&alp, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR,
                                          MSG_SGETEXT_CANTRESOLVEHOST_S, lGetString(ep,ST_name) );
                  ret = JGDI_ERROR;
                  break;
               }
            }
         }
      }
      /*
      printf("Build filter %s -------------------------\n", filters[i].getter);
      lWriteListTo(filters[i].list, stdout);
      */
   }
   
   /* send gdi request */
   if (ret == JGDI_SUCCESS) {
      report_handler_t *report_handler = jgdi_report_handler_create(env, qhost_result, &alp);
      
      if (report_handler != NULL) {
         do_qhost(ctx, 
                  filters[0].list,
                  filters[1].list,
                  filters[2].list,
                  filters[3].list,
                  show,
                  &alp,
                  report_handler);
         ret = ((jgdi_report_handler_t*)report_handler->ctx)->result;
         if (report_handler->destroy != NULL) {
            report_handler->destroy(&report_handler, &alp);
         }         
         FREE(report_handler);
      } else {
         ret = JGDI_ILLEGAL_STATE;
      }
   }

error:
   if (ret != JGDI_SUCCESS) {
      throw_error_from_answer_list(env, ret, alp);
   }
   for(i=0; filters[i].getter != NULL; i++) {
      lFreeList(&(filters[i].list));
   }

   lFreeList(&alp);   
   
   rmon_set_thread_ctx(NULL);
   jgdi_destroy_rmon_ctx(&rmon_ctx);
   
   DEXIT;
   
}



