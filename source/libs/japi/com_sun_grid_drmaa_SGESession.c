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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <strings.h>

#include "japi/drmaa.h"
#include "japi/com_sun_grid_drmaa_SGESession.h"
#include "lck/sge_mtutil.h"
#include "rmon/sgermon.h"
#include "uti/sge_log.h"
#include "uti/sge_signal.h"

#define BUFFER_LENGTH 1024
#define TEMPLATE_LIST_LENGTH 1024

static pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
static drmaa_job_template_t **job_templates = NULL;
static int list_length = 0;

static void throw_exception (JNIEnv *env, int errno, char *message);
static drmaa_job_template_t *remove_from_list (int id);
static drmaa_job_template_t *get_from_list (int id);
static int insert_into_list (drmaa_job_template_t *jt);
   
JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeControl
  (JNIEnv *env, jobject object, jstring jobId, jint action)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   const char *job_id = NULL;
   
   job_id = (*env)->GetStringUTFChars(env, jobId, NULL);

   errno = drmaa_control (job_id, action, error, DRMAA_ERROR_STRING_BUFFER);

   (*env)->ReleaseStringUTFChars(env, jobId, job_id);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);
   }  
}

JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeExit
  (JNIEnv *env, jobject object)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   
   errno = drmaa_exit (error, DRMAA_ERROR_STRING_BUFFER);

   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);
   }  
}

JNIEXPORT jstring JNICALL Java_com_sun_grid_drmaa_SGESession_nativeGetContact
  (JNIEnv *env, jobject object)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   char contact[DRMAA_CONTACT_BUFFER];
   
   errno = drmaa_get_contact (contact, DRMAA_CONTACT_BUFFER, error,
                              DRMAA_ERROR_STRING_BUFFER);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);
      
      return NULL;
   }
   
   return (*env)->NewStringUTF (env, contact);
}

JNIEXPORT jstring JNICALL Java_com_sun_grid_drmaa_SGESession_nativeGetDRMSInfo
  (JNIEnv *env, jobject object)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   char system[DRMAA_DRM_SYSTEM_BUFFER];
   
   errno = drmaa_get_DRM_system (system, DRMAA_DRM_SYSTEM_BUFFER, error,
                                 DRMAA_ERROR_STRING_BUFFER);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);
      
      return NULL;
   }
   
   return (*env)->NewStringUTF (env, system);
}

JNIEXPORT jint JNICALL Java_com_sun_grid_drmaa_SGESession_nativeGetJobProgramStatus
  (JNIEnv *env, jobject object, jstring jobId)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   int status = 0;
   const char *job_id = (*env)->GetStringUTFChars(env, jobId, NULL);

   errno = drmaa_job_ps (job_id, &status, error, DRMAA_ERROR_STRING_BUFFER);

   (*env)->ReleaseStringUTFChars(env, jobId, job_id);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);

      return -1;
   }
  
  return status;
}

JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeInit
  (JNIEnv *env, jobject object, jstring contactString)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   const char *contact = NULL;

   if (contactString != NULL) {
      contact = (*env)->GetStringUTFChars(env, contactString, NULL);
   }

   errno = drmaa_init (contact, error, DRMAA_ERROR_STRING_BUFFER);

   (*env)->ReleaseStringUTFChars(env, contactString, contact);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);
   }  
}

JNIEXPORT jobjectArray JNICALL Java_com_sun_grid_drmaa_SGESession_nativeRunBulkJobs
  (JNIEnv *env, jobject object, jint id, jint start, jint end, jint step)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   char buffer[DRMAA_JOBNAME_BUFFER];
   drmaa_job_template_t *jt = job_templates[id];
   drmaa_job_ids_t *ids = NULL;
   int num_elem = 0;
   int count = 0;
   int counter = 0;
   char **id_strings = NULL;
   jobjectArray ret_val = NULL;
   jclass clazz = NULL;
   jstring tmp_str = NULL;
   
   errno = drmaa_run_bulk_jobs (&ids, jt, start, end, step, error,
                                DRMAA_ERROR_STRING_BUFFER);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);
      
      return NULL;
   }
   
   num_elem = (end - start) / step;
   id_strings = (char **)malloc (num_elem * sizeof (char *));
   
   for (count = start; count < end; count += step) {
      if (drmaa_get_next_job_id (ids, buffer, DRMAA_JOBNAME_BUFFER)
                                                       == DRMAA_ERRNO_SUCCESS) {
         id_strings[counter++] = strdup (buffer);
      }
   }

   num_elem = counter;
   
   /* Create Java array */
   clazz = (*env)->FindClass (env, "Ljava/lang/String;");
   ret_val = (*env)->NewObjectArray(env, num_elem, clazz, NULL);

   for (count = 0; count < num_elem; count++) {
      tmp_str = (*env)->NewStringUTF (env, id_strings[count]);
      (*env)->SetObjectArrayElement(env, ret_val, count, tmp_str);
      free (id_strings[count]);
   }
   
   free (id_strings);
   
   return ret_val;
}

JNIEXPORT jstring JNICALL Java_com_sun_grid_drmaa_SGESession_nativeRunJob
  (JNIEnv *env, jobject object, jint id)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   char job_id[DRMAA_JOBNAME_BUFFER];
   drmaa_job_template_t *jt = job_templates[id];
   
   errno = drmaa_run_job (job_id, DRMAA_JOBNAME_BUFFER, jt, error,
                          DRMAA_ERROR_STRING_BUFFER);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);
      
      return NULL;
   }
   
   return (*env)->NewStringUTF (env, job_id);
}

JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeSynchronize
  (JNIEnv *env, jobject object, jobjectArray ids, jlong timeout,
   jboolean dispose)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   const char **job_ids = NULL;
   jsize length = (*env)->GetArrayLength(env, ids);
   jobject tmp_obj = NULL;
   jsize count = 0;

   job_ids = (const char**)malloc ((length + 1) * sizeof (char *));
   
   for (count = 0; count < length; count++) {
      tmp_obj = (*env)->GetObjectArrayElement(env, ids, count);
      job_ids[count] = (*env)->GetStringUTFChars(env, (jstring)tmp_obj, NULL);
   }
   
   job_ids[count] = NULL;
   
   errno = drmaa_synchronize (job_ids, timeout, dispose, error,
                              DRMAA_ERROR_STRING_BUFFER);
   
   for (count = 0; count < length; count++) {
      tmp_obj = (*env)->GetObjectArrayElement(env, ids, count);
      (*env)->ReleaseStringUTFChars(env, (jstring)tmp_obj, job_ids[count]);
   }
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);
   }
}

JNIEXPORT jobject JNICALL Java_com_sun_grid_drmaa_SGESession_nativeWait
  (JNIEnv *env, jobject object, jstring jobId, jlong timeout)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   char buffer[DRMAA_JOBNAME_BUFFER];
   char signal[DRMAA_SIGNAL_BUFFER];
   const char *job_id = NULL;
   jobject job_info = NULL;
   jmethodID meth = NULL;
   jclass clazz = NULL;
   jobjectArray resources = NULL;
   char* resource_entries[BUFFER_LENGTH];
   int status = -1;
   drmaa_attr_values_t *rusage = NULL;
   jstring tmp_str = NULL;
   int signaled = 0;
   int count = 0;
   int length = 0;
   
   job_id = (*env)->GetStringUTFChars (env, jobId, NULL);
   
   errno = drmaa_wait (job_id, buffer, DRMAA_JOBNAME_BUFFER, &status, timeout,
                       &rusage, error, DRMAA_ERROR_STRING_BUFFER);
   
   (*env)->ReleaseStringUTFChars (env, jobId, job_id);
      
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);
   
      return NULL;
   }

   while ((errno = drmaa_get_next_attr_value (rusage, buffer, BUFFER_LENGTH))
                                                      == DRMAA_ERRNO_SUCCESS) {
      resource_entries[count++] = strdup (buffer);
   }

   length = count;
   
   clazz = (*env)->FindClass (env, "Ljava/lang/String;");
   resources = (*env)->NewObjectArray(env, length, clazz, NULL);
   
   for (count = 0; count < length; count++) {
      tmp_str = (*env)->NewStringUTF (env, resource_entries[count]);
      (*env)->SetObjectArrayElement(env, resources, count, tmp_str);
      
      free (resource_entries[count]);
   }

   errno = drmaa_wifsignaled (&signaled, status, error,
                              DRMAA_ERROR_STRING_BUFFER);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);
   
      return NULL;
   }
   else if (signaled != 0) {
      errno = drmaa_wtermsig (signal, DRMAA_SIGNAL_BUFFER, status, error,
                              DRMAA_ERROR_STRING_BUFFER);

      if (errno != DRMAA_ERRNO_SUCCESS) {
         throw_exception (env, errno, error);

         return NULL;
      }
      
      tmp_str = (*env)->NewStringUTF (env, signal);
   }
   
   clazz = (*env)->FindClass (env, "Lcom/sun/grid/drmaa/SGEJobInfo;");
   meth = (*env)->GetMethodID (env, clazz, "<init>",
                 "(Ljava/lang/String;I[Ljava/lang/String;Ljava/lang/String;)V");
   job_info = (*env)->NewObject (env, clazz, meth, jobId, status, resources,
                                 tmp_str);
   
   return job_info;
}

JNIEXPORT jint JNICALL Java_com_sun_grid_drmaa_SGESession_nativeAllocateJobTemplate
  (JNIEnv *env, jobject object)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   drmaa_job_template_t *jt = NULL;
   
   errno = drmaa_allocate_job_template(&jt, error, DRMAA_ERROR_STRING_BUFFER);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);
      return -1;
   }
   
   return insert_into_list (jt);
}

JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeSetAttributeValue
  (JNIEnv *env, jobject object, jint id, jstring nameStr, jstring valueStr)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   drmaa_job_template_t *jt = get_from_list (id);
   const char *name = NULL;
   const char *value = NULL;
   
   if (jt == NULL) {
      throw_exception (env, DRMAA_ERRNO_INTERNAL_ERROR,
                       MSG_JDRMAA_BAD_JOB_TEMPLATE);
      return;
   }
   
   name = (*env)->GetStringUTFChars (env, nameStr, NULL);
   value = (*env)->GetStringUTFChars (env, valueStr, NULL);
   
   errno = drmaa_set_attribute (jt, name, value, error,
                                DRMAA_ERROR_STRING_BUFFER);

   (*env)->ReleaseStringUTFChars (env, nameStr, name);
   (*env)->ReleaseStringUTFChars (env, valueStr, value);

   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);
   
      return;
   }
}

JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeSetAttributeValues
  (JNIEnv *env, jobject object, jint id, jstring nameStr, jobjectArray values)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   drmaa_job_template_t *jt = get_from_list (id);
   const char *name = NULL;
   const char **value = NULL;
   jsize length = (*env)->GetArrayLength(env, values);
   jobject tmp_obj = NULL;
   jsize count = 0;
   
   if (jt == NULL) {
      throw_exception (env, DRMAA_ERRNO_INTERNAL_ERROR,
                       MSG_JDRMAA_BAD_JOB_TEMPLATE);
      return;
   }

   /* Get the strings out of the Strings. */
   name = (*env)->GetStringUTFChars (env, nameStr, NULL);
   value = (const char**)malloc ((length + 1) * sizeof (char *));
   
   for (count = 0; count < length; count++) {
      tmp_obj = (*env)->GetObjectArrayElement(env, values, count);
      value[count] = (*env)->GetStringUTFChars(env, (jstring)tmp_obj, NULL);
   }

   value[count] = NULL;
   
   errno = drmaa_set_vector_attribute (jt, name, value, error,
                                       DRMAA_ERROR_STRING_BUFFER);

   /* Release the strings. */
   (*env)->ReleaseStringUTFChars (env, nameStr, name);
   
   for (count = 0; count < length; count++) {
      tmp_obj = (*env)->GetObjectArrayElement(env, values, count);
      (*env)->ReleaseStringUTFChars(env, (jstring)tmp_obj, value[count]);
   }

   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);
   
      return;
   }
}

JNIEXPORT jobjectArray JNICALL Java_com_sun_grid_drmaa_SGESession_nativeGetAttributeNames
  (JNIEnv *env, jobject object, jint id)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   drmaa_job_template_t *jt = get_from_list (id);
   char buffer[BUFFER_LENGTH];
   jobjectArray retval = NULL;
   drmaa_attr_names_t *names = NULL;
   char *name_array[BUFFER_LENGTH];
   int count = 0;
   int max = 0;
   jclass clazz = NULL;
   jstring tmp_str = NULL;
   
   errno = drmaa_get_attribute_names (&names, error, DRMAA_ERROR_STRING_BUFFER);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);
   
      return NULL;
   }
   
   while (drmaa_get_next_attr_name(names, buffer, BUFFER_LENGTH)
                                                       == DRMAA_ERRNO_SUCCESS) {
      name_array[count++] = strdup (buffer);
   }
   
   errno = drmaa_get_vector_attribute_names (&names, error, 
                                             DRMAA_ERROR_STRING_BUFFER);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (env, errno, error);
   
      return NULL;
   }
   
   while (drmaa_get_next_attr_name(names, buffer, BUFFER_LENGTH)
                                                       == DRMAA_ERRNO_SUCCESS) {
      name_array[count++] = strdup (buffer);
   }
   
   max = count;
   
   clazz = (*env)->FindClass (env, "Ljava/lang/String;");
   retval = (*env)->NewObjectArray(env, max, clazz, NULL);

   for (count = 0; count < max; count++) {
      tmp_str = (*env)->NewStringUTF (env, name_array[count]);
      (*env)->SetObjectArrayElement(env, retval, count, tmp_str);
      free (name_array[count]);
   }
   
   return retval;
}

JNIEXPORT jobjectArray JNICALL Java_com_sun_grid_drmaa_SGESession_nativeGetAttribute
  (JNIEnv *env, jobject object, jint id, jstring name)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   drmaa_job_template_t *jt = get_from_list (id);
   jobjectArray retval = NULL;
   drmaa_attr_names_t *names = NULL;
   drmaa_attr_values_t *values = NULL;
   char buffer[BUFFER_LENGTH];
   const char *name_str = (*env)->GetStringUTFChars(env, name, NULL);
   bool is_vector = false;
   jclass clazz = NULL;
   jstring tmp_str = NULL;
   
   errno = drmaa_get_vector_attribute_names(&names, error,  
                                            DRMAA_ERROR_STRING_BUFFER);
   
   if (errno == DRMAA_ERRNO_SUCCESS) {
      while (drmaa_get_next_attr_name(names, buffer, BUFFER_LENGTH)
                                                       == DRMAA_ERRNO_SUCCESS) {
         if (strcmp (buffer, name_str) == 0) {
            is_vector = true;
            break;
         }
      }
   }
   else {
      (*env)->ReleaseStringUTFChars(env, name, name_str);
      throw_exception (env, errno, error);
   
      return NULL;
   }
   
   drmaa_release_attr_names (names);
   
   clazz = (*env)->FindClass (env, "Ljava/lang/String;");
   
   if (is_vector) {
      errno = drmaa_get_vector_attribute (jt, name_str, &values, error,
                                          DRMAA_ERROR_STRING_BUFFER);
      (*env)->ReleaseStringUTFChars(env, name, name_str);
      
      if (errno != DRMAA_ERRNO_SUCCESS) {
         throw_exception (env, errno, error);
         drmaa_release_attr_values (values);

         return NULL;
      }
      else {
         char *names[BUFFER_LENGTH];
         int count = 0;
         int max = 0;
         
         while (drmaa_get_next_attr_value(values, buffer, BUFFER_LENGTH)
                                                       == DRMAA_ERRNO_SUCCESS) {
            names[count++] = strdup (buffer);
         }
         
         max = count;
         
         retval = (*env)->NewObjectArray(env, max, clazz, NULL);
         
         for (count = 0; count < max; count++) {
            tmp_str = (*env)->NewStringUTF (env, names[count]);
            (*env)->SetObjectArrayElement(env, retval, count, tmp_str);
            free (names[count]);
         }
      
         drmaa_release_attr_values (values);
      }
   }
   else {
      errno = drmaa_get_attribute (jt, name_str, buffer, BUFFER_LENGTH, error,
                                   DRMAA_ERROR_STRING_BUFFER);
      (*env)->ReleaseStringUTFChars(env, name, name_str);
      
      if (errno != DRMAA_ERRNO_SUCCESS) {
         throw_exception (env, errno, error);

         return NULL;
      }
      else {
         retval = (*env)->NewObjectArray(env, 1, clazz, NULL);
         tmp_str = (*env)->NewStringUTF (env, buffer);
         (*env)->SetObjectArrayElement(env, retval, 0, tmp_str);
      }
   }
   
   return retval;
}

JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeDeleteJobTemplate
  (JNIEnv *env, jobject object, jint id)
{
   char error[DRMAA_ERROR_STRING_BUFFER];
   int errno = DRMAA_ERRNO_SUCCESS;
   drmaa_job_template_t *jt = remove_from_list (id);
   
   if (jt != NULL) {
      errno = drmaa_delete_job_template (jt, error, DRMAA_ERROR_STRING_BUFFER);
      
      if (errno != DRMAA_ERRNO_SUCCESS) {
         throw_exception (env, errno, error);

         return;
      }      
   }
   else {
      throw_exception (env, DRMAA_ERRNO_INTERNAL_ERROR,
                       MSG_JDRMAA_BAD_JOB_TEMPLATE);
      return;
   }
}

static void throw_exception (JNIEnv *env, int errno, char *message)
{
   jclass newExcCls = NULL;
   
   DENTER (TOP_LAYER, "throw_exception");
   
   switch (errno) {
      case DRMAA_ERRNO_INTERNAL_ERROR:
         newExcCls = (*env)->FindClass(env,
                                       "Lorg/ggf/drmaa/InternalException;");
         break;
      case DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE:
         newExcCls = (*env)->FindClass(env,
                                   "Lorg/ggf/drmaa/DRMCommunicationException;");
         break;
      case DRMAA_ERRNO_AUTH_FAILURE:
         newExcCls = (*env)->FindClass(env,
                                      "Lorg/ggf/drmaa/AuthorizationException;");
         break;
      case DRMAA_ERRNO_INVALID_ARGUMENT:
         newExcCls = (*env)->FindClass(env,
                                    "Lorg/ggf/drmaa/InvalidArgumentException;");
         break;
      case DRMAA_ERRNO_NO_ACTIVE_SESSION:
         newExcCls = (*env)->FindClass(env,
                                    "Lorg/ggf/drmaa/NoActiveSessionException;");
         break;
      case DRMAA_ERRNO_NO_MEMORY:
         newExcCls = (*env)->FindClass(env, "Ljava/lang/OutOfMemoryError;");
         break;
      case DRMAA_ERRNO_INVALID_CONTACT_STRING:
         newExcCls = (*env)->FindClass(env,
                               "Lorg/ggf/drmaa/InvalidContactStringException;");
         break;
      case DRMAA_ERRNO_DEFAULT_CONTACT_STRING_ERROR:
         newExcCls = (*env)->FindClass(env,
                               "Lorg/ggf/drmaa/DefaultContactStringException;");
         break;
      case DRMAA_ERRNO_DRMS_INIT_FAILED:
         newExcCls = (*env)->FindClass(env,
                                       "Lorg/ggf/drmaa/DRMSInitException;");
         break;
      case DRMAA_ERRNO_ALREADY_ACTIVE_SESSION:
         newExcCls = (*env)->FindClass(env,
                               "Lorg/ggf/drmaa/SessionAlreadyActiveException;");
         break;
      case DRMAA_ERRNO_DRMS_EXIT_ERROR:
         newExcCls = (*env)->FindClass(env,
                                       "Lorg/ggf/drmaa/DRMSExitException;");
         break;
      case DRMAA_ERRNO_INVALID_ATTRIBUTE_FORMAT:
         newExcCls = (*env)->FindClass(env,
                             "Lorg/ggf/drmaa/InvalidAttributeFormatException;");
         break;
      case DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE:
         newExcCls = (*env)->FindClass(env,
                              "Lorg/ggf/drmaa/InvalidAttributeValueException;");
         break;
      case DRMAA_ERRNO_CONFLICTING_ATTRIBUTE_VALUES:
         newExcCls = (*env)->FindClass(env,
                         "Lorg/ggf/drmaa/ConflictingAttributeValuesException;");
         break;
      case DRMAA_ERRNO_TRY_LATER:
         newExcCls = (*env)->FindClass(env,
                                       "Lorg/ggf/drmaa/TryLaterException;");
         break;
      case DRMAA_ERRNO_DENIED_BY_DRM:
         newExcCls = (*env)->FindClass(env,
                                       "Lorg/ggf/drmaa/DeniedByDRMException;");
         break;
      case DRMAA_ERRNO_INVALID_JOB:
         newExcCls = (*env)->FindClass(env,
                                       "Lorg/ggf/drmaa/InvalidJobException;");
         break;
      case DRMAA_ERRNO_RESUME_INCONSISTENT_STATE:
         newExcCls = (*env)->FindClass(env,
                                  "Lorg/ggf/drmaa/InconsistentStateException;");
         break;
      case DRMAA_ERRNO_SUSPEND_INCONSISTENT_STATE:
         newExcCls = (*env)->FindClass(env,
                                  "Lorg/ggf/drmaa/InconsistentStateException;");
         break;
      case DRMAA_ERRNO_HOLD_INCONSISTENT_STATE:
         newExcCls = (*env)->FindClass(env,
                                  "Lorg/ggf/drmaa/InconsistentStateException;");
         break;
      case DRMAA_ERRNO_RELEASE_INCONSISTENT_STATE:
         newExcCls = (*env)->FindClass(env,
                                  "Lorg/ggf/drmaa/InconsistentStateException;");
         break;
      case DRMAA_ERRNO_EXIT_TIMEOUT:
         newExcCls = (*env)->FindClass(env,
                                       "Lorg/ggf/drmaa/ExitTimeoutException;");
         break;
      case DRMAA_ERRNO_NO_RUSAGE:
         newExcCls = (*env)->FindClass(env,
                               "Lorg/ggf/drmaa/DNoResourceUsageDataException;");
         break;
      default:
         break;
   }

   if (newExcCls == 0) {
      /* If we can't find the specific exception, try again using
       * DRMAAException */
      newExcCls = (*env)->FindClass(env, "Lorg/ggf/drmaa/DRMAAException;");
   }
   
   /* This isn't an "else if" because we also need to check the result of the
    * previous "if." */
   if (newExcCls == 0) {
      /* If we really can't find the right exception, default to something we
       * really expect to be able to find. */
      newExcCls = (*env)->FindClass(env, "Ljava/lang/RuntimeException;");
      
      /* If it's still not found, give up. */
      if (newExcCls == 0) {
         CRITICAL ((SGE_EVENT, "Unable to find exception for DRMAA error: %d\n",
                    drmaa_strerror (errno)));
         DEXIT;
         return;
      }
   }

   /* InconsistentStateException has a different constructor. */
   if ((errno == DRMAA_ERRNO_RESUME_INCONSISTENT_STATE) ||
       (errno == DRMAA_ERRNO_SUSPEND_INCONSISTENT_STATE) ||
       (errno == DRMAA_ERRNO_HOLD_INCONSISTENT_STATE) ||
       (errno == DRMAA_ERRNO_RELEASE_INCONSISTENT_STATE)) {
      jmeth meth = NULL;
      jthrowable e = NULL;
      int state = 0;
      jfieldID id = NULL;
      
      switch (errno) {
         case DRMAA_ERRNO_RESUME_INCONSISTENT_STATE:
            id = (*env)->GetStaticFieldID(env, newExcCls, "RESUME", "I");
            break;
         case DRMAA_ERRNO_SUSPEND_INCONSISTENT_STATE:
            id = (*env)->GetStaticFieldID(env, newExcCls, "SUSPEND", "I");
            break;
         case DRMAA_ERRNO_HOLD_INCONSISTENT_STATE:
            id = (*env)->GetStaticFieldID(env, newExcCls, "HOLD", "I");
            break;
         case DRMAA_ERRNO_RELEASE_INCONSISTENT_STATE:
            id = (*env)->GetStaticFieldID(env, newExcCls, "RELEASE", "I");
            break;
      }
      
      state = GetStaticIntField(env, newExcCls, id);
         
      meth = (*env)->GetMethodID (env, newExcCls, "<init>",
                                  "(ILjava/lang/String;)V");
      e = (*env)->NewObject (env, newExcCls, meth, state, message);
      /* Throw the exception. */
      (*env)->Throw(env, e);
   }
   else {
      /* Throw the exception. */
      (*env)->ThrowNew(env, newExcCls, message);
   }
   
   DEXIT;
}

static int insert_into_list (drmaa_job_template_t *jt)
{
   int count = 0;
   drmaa_job_template_t **tmp_list = NULL;
   int tmp_length = 0;
   
   sge_mutex_lock("JTList", "insert_into_list", __LINE__, &list_mutex);

   /* If we haven't initialized the template list yet, do so. */
   if (job_templates == NULL) {
      list_length = TEMPLATE_LIST_LENGTH;
      job_templates = (drmaa_job_template_t **)malloc
                                (sizeof (drmaa_job_template_t *) * list_length);
      memset (job_templates, 0, list_length * sizeof (drmaa_job_template_t *));
   }

   /* Search for an empty slot. */
   for (count = 0; count < list_length; count++) {
      if (job_templates[count] == NULL) {
         /* Insert the template and return the index. */
         job_templates[count] = jt;
   
         sge_mutex_unlock("JTList", "insert_into_list", __LINE__, &list_mutex);

         return count;
      }
   }

   /* If there are no empty slots, double the size of the list. */
   tmp_length = list_length * 2;
   tmp_list = (drmaa_job_template_t **)malloc (sizeof (drmaa_job_template_t *) *
                                                                    tmp_length);
   memcpy (tmp_list, job_templates, list_length *
                                               sizeof (drmaa_job_template_t *));
   memset (&tmp_list[count], 0, list_length * sizeof (drmaa_job_template_t *));
   
   list_length = tmp_length;
   free (job_templates);
   job_templates = tmp_list;
   
   /* Insert the template and return the index. */
   job_templates[count] = jt;
   
   sge_mutex_unlock("JTList", "insert_into_list", __LINE__, &list_mutex);
   
   return count;
}

static drmaa_job_template_t *get_from_list (int id)
{
   drmaa_job_template_t *retval = NULL;
   
   if ((job_templates != NULL) && (id >= 0)) {
      sge_mutex_lock("JTList", "get_from_list", __LINE__, &list_mutex);
      if (id < list_length) {
         retval = job_templates[id];
      }
      sge_mutex_unlock("JTList", "get_from_list", __LINE__, &list_mutex);
   }
   
   return retval;
}

static drmaa_job_template_t *remove_from_list (int id)
{
   drmaa_job_template_t *retval = NULL;
   
   if ((job_templates != NULL) && (id >= 0)) {
      sge_mutex_lock("JTList", "remove_from_list", __LINE__, &list_mutex);
      if (id < list_length) {
         retval = job_templates[id];
         job_templates[id] = NULL;
      }
      sge_mutex_unlock("JTList", "remove_from_list", __LINE__, &list_mutex);
   }
   
   return retval;
}
