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

#include "japi/drmaa.h"
#include "japi/sun_sge_drmaa_SGESession.h"
#include "uti/sge_signal.h"

#define BUFFER_LENGTH 1024
#define TEMPLATE_LIST_LENGTH 1024

static void throw_exception (int errno, char *message);

static drmaa_job_template_t *job_templates[TEMPLATE_LIST_LENGTH];
   
JNIEXPORT jstring JNICALL Java_sun_sge_drmaa_SGESession_nativeGetSignalName
  (JNIEnv *env, jobject object, jint sig_num)
{
   char *sig_name = sge_sig2str ((int)sig_num);
   return (*env)->NewStringUTF (env, sig_name);
}

JNIEXPORT jstring JNICALL Java_sun_sge_drmaa_SGESession_nativeGetContact
  (JNIEnv *env, jobject object)
{
   char error[BUFFER_LENGTH];
   int errno = DRMAA_ERRNO_SUCCESS;
   char contact[BUFFER_LENGTH];
   
   errno = drmaa_get_contact (contact, BUFFER_LENGTH, error, BUFFER_LENGTH);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (errno, error);
      
      return NULL;
   }
   
   return (*env)->NewStringUTF (env, contact);
}

JNIEXPORT jstring JNICALL Java_sun_sge_drmaa_SGESession_nativeGetDRMSInfo
  (JNIEnv *env, jobject object)
{
   char error[BUFFER_LENGTH];
   int errno = DRMAA_ERRNO_SUCCESS;
   char system[BUFFER_LENGTH];
   
   errno = drmaa_get_DRM_system (system, BUFFER_LENGTH, error, BUFFER_LENGTH);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (errno, error);
      
      return NULL;
   }
   
   return (*env)->NewStringUTF (env, system);
}

JNIEXPORT jint JNICALL Java_sun_sge_drmaa_SGESession_nativeGetJobProgramStatus
  (JNIEnv *env, jobject object, jstring jobId) {
   char error[BUFFER_LENGTH];
   int errno = DRMAA_ERRNO_SUCCESS;
   int status = 0;
   char *job_id = (*env)->GetStringUTFChars(env, jobId, NULL);

   errno = drmaa_job_ps (job_id, &status, error, BUFFER_LENGTH);

   (*env)->ReleaseStringUTFChars(env, jobId, job_id);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (errno, error);

      return -1;
   }
  
  return status;
}

JNIEXPORT void JNICALL Java_sun_sge_drmaa_SGESession_nativeInit
  (JNIEnv *env, jobject object, jstring contactString) {
   char error[BUFFER_LENGTH];
   int errno = DRMAA_ERRNO_SUCCESS;
   char *contact = (*env)->GetStringUTFChars(env, contactString, NULL);

   errno = drmaa_init (contact, error, BUFFER_LENGTH);

   (*env)->ReleaseStringUTFChars(env, contactString, contact);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (errno, error);
   }  
}

JNIEXPORT jobjectArray JNICALL Java_sun_sge_drmaa_SGESession_nativeRunBulkJobs
  (JNIEnv *env, jobject object, jint id, jint start, jint end, jint step)
{
   char error[BUFFER_LENGTH];
   int errno = DRMAA_ERRNO_SUCCESS;
   char buffer[BUFFER_LENGTH];
   drmaa_job_template_t *jt = job_templates[id];
   drmaa_job_ids_t *ids = NULL;
   int num_elem = 0;
   int count = 0, counter = 0;
   char **id_strings = NULL;
   jobjectArray ret_vel = NULL;
   
   errno = drmaa_run_bulk_jobs (&ids, jt, start, stop, end, error, BUFFER_LENGTH);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (errno, error);
      
      return NULL;
   }
   
   num_elem = (end - start) / step;
   id_strings = (char **)malloc (num_elem * sizeof (char *));
   
   for (count = 0; count < num_elem; count += step) {
      drmaa_get_next_job_id (ids, buffer, BUFFER_LENGTH);
      id_strings[counter++] = strdup (buffer);
   }
   
   /* Create Java array */
   
   for (count = 0; count < counter; count++) {
      free (id_strings[count]);
   }
   
   free (id_strings);
   
   return ret_val;
}

JNIEXPORT jstring JNICALL Java_sun_sge_drmaa_SGESession_nativeRunJob
  (JNIEnv *env, jobject object, jint id)
{
   char error[BUFFER_LENGTH];
   int errno = DRMAA_ERRNO_SUCCESS;
   char job_id[BUFFER_LENGTH];
   drmaa_job_template_t *jt = job_templates[id];
   
   errno = drmaa_run_job (job_id, BUFFER_LENGTH, jt, error, BUFFER_LENGTH);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (errno, error);
      
      return NULL;
   }
   
   return (*env)->NewStringUTF (env, job_id);
}

JNIEXPORT void JNICALL Java_sun_sge_drmaa_SGESession_nativeSynchronize
  (JNIEnv *env, jobject object, jobjectArray ids, jlong timeout, jboolean dispose)
{
   char error[BUFFER_LENGTH];
   int errno = DRMAA_ERRNO_SUCCESS;
   char **job_ids = NULL;
   jsize length = (*env)->GetArrayLength(env, ids);
   jobject tmp_obj = NULL;
   jsize count = 0;

   job_ids = (char**)malloc (length * sizeof (char *));
   
   for (count = 0; count < length; count++) {
      tmp_obj = GetObjectArrayElement(env, ids, count);
      job_ids[count] = (*env)->GetStringUTFChars(env, (jstring)tmp_obj, NULL);
   }
   
   errno = drmaa_synchronize (job_ids, timeout, dispose, error, BUFFER_LENGTH);
   
   for (count = 0; count < length; count++) {
      tmp_obj = GetObjectArrayElement(env, ids, count);
      (*env)->ReleaseStringUTFChars(env, (jstring)tmp_obj, job_ids[count]);
   }
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (errno, error);
   }
}

JNIEXPORT jobject JNICALL Java_sun_sge_drmaa_SGESession_nativeWait
  (JNIEnv *env, jobject object, jstring jobId, jlong timeout);
{
   char error[BUFFER_LENGTH];
   int errno = DRMAA_ERRNO_SUCCESS;
   char buffer[BUFFER_LENGTH];
   char *job_id = (*env)->GetStringUTFChars (env, jobId, NULL);
   jobject job_info = NULL;
   jmethodID meth = NULL;
   jclass clazz = NULL;
   jobject resources = NULL;
   jobjectArray resource_entries = NULL;
   int status = -1;
   drmaa_attr_values_t *rusage = NULL;
   
   errno = drmaa_wait (job_id, buffer, BUFFER_LENGTH, &status, timeout, &rusage,
                       error, BUFFER_LENGTH);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (errno, error);
   
      (*env)->ReleaseStringUTFChars (env, jobId, job_id);
      
      return NULL;
   }

   while ((errno = drmaa_get_next_attr_value (rusage, buffer, BUFFER_LENGTH))
                                                      == DRMAA_ERRNO_SUCCESS) {
      jstring tmp_str = (*env)->NewStringUTF (env, buffer);
   }
   
   clazz = FindClass (env, "Lsun/sge/drmaa/SGEJobInfo;");
   meth = GetMethodID (env, clazz, "<init>",
                       "(Ljava/lang/String;I[Ljava/lang/String;)V");
   job_info = NewObject (env, meth, jobId, status, resources);
   
   (*env)->ReleaseStringUTFChars (env, jobId, job_id);
   
   return job_info;
}

static void throw_exception (int errno, char *message)
{
   jclass newExcCls = NULL;
   bool default_err = false;
   
   switch (errno) {
      case DRMAA_ERRNO_INTERNAL_ERROR:
         (*env)->FindClass(env, "com/sun/grid/drmaa/InternalException");
         break;
      default:
         (*env)->FindClass(env, "com/sun/grid/drmaa/DRMAAException");
         default_err = true;
         break;
   }

   if ((newExcCls == 0) && !default_err) {
      /* If we can't find the specific exception, try again using
       * DRMAAException */
      (*env)->FindClass(env, "com/sun/grid/drmaa/DRMAAException");
   }
   
   /* This isn't an "else if" because we also need to check the result of the
    * previous "if." */
   if (newExcCls == 0) {
      /* If we really can't find the right exception, default to something we
       * really expect to be able to find. */
      (*env)->FindClass(env, "java/lang/RuntimeException");
      
      /* If it's still not found, give up. */
      if (newExcCls == 0) {
         CRITICAL ((SGE_EVENT, "Unable to find exception for DRMAA error: %d\n", drmaa_strerror (errno)));
         return;
      }
   }

   /* Throw the exception. */
   (*env)->ThrowNew(env, newExcCls, error);
}
