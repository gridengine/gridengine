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
#include "japi/com_sun_grid_drmaa_SGESession.h"
#include "uti/sge_signal.h"

#define BUFFER_LENGTH 1024
#define TEMPLATE_LIST_LENGTH 1024

static pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
static drmaa_job_template_t **job_templates = NULL;
static int list_length = 0;

static void throw_exception (int errno, char *message);
   
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
   char *job_id = NULL;
   jobject job_info = NULL;
   jmethodID meth = NULL;
   jclass clazz = NULL;
   jobjectArray resource = NULL;
   char* resource_entries[BUFFER_LENGTH];
   int status = -1;
   drmaa_attr_values_t *rusage = NULL;
   jstring tmp_str = NULL;
   int signaled = 0;
   int count = 0;
   int length = 0;
   
   job_id = (*env)->GetStringUTFChars (env, jobId, NULL);
   
   errno = drmaa_wait (job_id, buffer, BUFFER_LENGTH, &status, timeout, &rusage,
                       error, BUFFER_LENGTH);
   
   (*env)->ReleaseStringUTFChars (env, jobId, job_id);
      
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (errno, error);
   
      return NULL;
   }

   while ((errno = drmaa_get_next_attr_value (rusage, buffer, BUFFER_LENGTH))
                                                      == DRMAA_ERRNO_SUCCESS) {
      resource_entries[count++] = strdup (buffer);
   }

   length = count;
   
   clazz = FindClass (env, "Ljava/lang/String;");
   resources = (*env)->NewObjectArray(env, length, clazz, NULL);
   
   for (count = 0; count < length; count++) {
      tmp_str = (*env)->NewStringUTF (env, resource_entries[count]);
      (*env)->SetObjectArrayElement(env, resources, count++, tmp_str);
   }

   errno = drmaa_wifsignaled (&signaled, status, error, BUFFER_LENGTH);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (errno, error);
   
      return NULL;
   }
   else if (signaled != 0) {
      errno = drmaa_wtermsig (buffer, BUFFER_LENGTH, status, error, BUFFER_LENGTH);

      if (errno != DRMAA_ERRNO_SUCCESS) {
         throw_exception (errno, error);

         return NULL;
      }
      
      tmp_str = (*env)->NewStringUTF (env, buffer);
   }
   
   clazz = FindClass (env, "Lsun/sge/drmaa/SGEJobInfo;");
   meth = GetMethodID (env, clazz, "<init>",
                       "(Ljava/lang/String;I[Ljava/lang/String;Ljava/lang/String;)V");
   job_info = NewObject (env, meth, jobId, status, resources, tmp_str);
   
   return job_info;
}

JNIEXPORT jint JNICALL Java_com_sun_grid_drmaa_SGESession_nativeAllocateJobTemplate
  (JNIEnv *env, jobject object)
{
   char error[BUFFER_LENGTH];
   int errno = DRMAA_ERRNO_SUCCESS;
   drmaa_job_template_t *jt = NULL;
   
   errno = drmaa_allocate_job_template(&jt, error, BUFFER_LENGTH);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (errno, error);
      return -1;
   }
   
   return insert_into_list (jt);
}

JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeSetAttributeValue
  (JNIEnv *env, jobject object, jint id, jstring nameStr, jstring valueStr)
{
   char error[BUFFER_LENGTH];
   int errno = DRMAA_ERRNO_SUCCESS;
   drmaa_job_template_t *jt = get_from_list (id);
   char *name = NULL;
   char *value = NULL;
   
   if (jt == NULL) {
      throw_exception (DRMAA_INTERNAL_ERROR, "Requested job template does not exist");
      return;
   }
   
   name = (*env)->GetStringUTFChars (env, nameStr, NULL);
   value = (*env)->GetStringUTFChars (env, valueStr, NULL);
   
   errno = drmaa_set_attribute (jt, name, value, error, BUFFER_LENGTH);

   (*env)->ReleaseStringUTFChars (env, nameStr, name);
   (*env)->ReleaseStringUTFChars (env, valueStr, value);

   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (errno, error);
   
      return;
   }
}

JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeSetAttributeValues
  (JNIEnv *env, jobject object, jint id, jstring nameStr, jobjectArray values)
{
   char error[BUFFER_LENGTH];
   int errno = DRMAA_ERRNO_SUCCESS;
   drmaa_job_template_t *jt = get_from_list (id);
   char *name = NULL;
   char **value = NULL;
   jsize length = (*env)->GetArrayLength(env, values);
   jobject tmp_obj = NULL;
   jsize count = 0;
   
   if (jt == NULL) {
      throw_exception (DRMAA_INTERNAL_ERROR, "Requested job template does not exist");
      return;
   }

   /* Get the strings out of the Strings. */
   name = (*env)->GetStringUTFChars (env, nameStr, NULL);
   value = (char**)malloc (length * sizeof (char *));
   
   for (count = 0; count < length; count++) {
      tmp_obj = GetObjectArrayElement(env, values, count);
      value[count] = (*env)->GetStringUTFChars(env, (jstring)tmp_obj, NULL);
   }
   
   errno = drmaa_set_vector_attribute (jt, name, value, error, BUFFER_LENGTH);

   /* Release the strings. */
   (*env)->ReleaseStringUTFChars (env, nameStr, name);
   
   for (count = 0; count < length; count++) {
      tmp_obj = GetObjectArrayElement(env, values, count);
      (*env)->ReleaseStringUTFChars(env, (jstring)tmp_obj, value[count]);
   }

   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (errno, error);
   
      return;
   }
}

JNIEXPORT jobjectArray JNICALL Java_com_sun_grid_drmaa_SGESession_nativeGetAttributeNames
  (JNIEnv *env, jobject object, jint id)
{
   char error[BUFFER_LENGTH];
   int errno = DRMAA_ERRNO_SUCCESS;
   drmaa_job_template_t *jt = get_from_list (id);
   jObjectArray retval = NULL;
   drmaa_names_t *names = NULL;
   char *name_array[BUFFER_LENGTH];
   int count = 0;
   int max = 0;
   jclass clazz = NULL;
   jstring tmp_str = NULL;
   
   errno = drmaa_get_attribute_names (names, error, BUFFER_LENGTH);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (errno, error);
   
      return NULL;
   }
   
   while (drmaa_get_next_attr_value(names, buffer, BUFFER_LENGTH)
                                                       == DRMAA_ERRNO_SUCCESS) {
      name_array[count++] = strdup (buffer);
   }
   
   errno = drmaa_get_vector_attribute_names (names, error, BUFFER_LENGTH);
   
   if (errno != DRMAA_ERRNO_SUCCESS) {
      throw_exception (errno, error);
   
      return NULL;
   }
   
   while (drmaa_get_next_attr_value(names, buffer, BUFFER_LENGTH)
                                                       == DRMAA_ERRNO_SUCCESS) {
      name_array[count++] = strdup (buffer);
   }
   
   max = count;
   
   clazz = FindClass (env, "Ljava/lang/String;");
   retval = (*env)->NewObjectArray(env, max, clazz, NULL);

   for (count = 0; count < max; count++) {
      tmp_str = (*env)->NewStringUTF (env, name_array[count]);
      (*env)->SetObjectArrayElement(env, retval, count, tmp_str);
      FREE (name_array[count]);
   }
}

JNIEXPORT jobjectArray JNICALL Java_com_sun_grid_drmaa_SGESession_nativeGetAttribute
  (JNIEnv *env, jobject object, jint id, jstring name)
{
   char error[BUFFER_LENGTH];
   int errno = DRMAA_ERRNO_SUCCESS;
   drmaa_job_template_t *jt = get_from_list (id);
   jObjectArray retval = NULL;
   drmaa_attr_names_t *values = NULL;
   char buffer[BUFFER_LENGTH];
   char *name_str = (*env)->GetStringUTFChars(env, name, NULL);
   bool is_vector = false;
   jclass clazz = NULL;
   jstring tmp_str = NULL;
   
   errno = drmaa_get_vector_attribute_names(&values, error, BUFFER_LENGTH);
   
   if (errno == DRMAA_ERRNO_SUCCESS) {
      while (drmaa_get_next_attr_value(values, buffer, BUFFER_LENGTH)
                                                       == DRMAA_ERRNO_SUCCESS) {
         if (strcmp (buffer, name_str) == 0) {
            is_vector = true;
            break;
         }
      }
   }
   else {
      (*env)->ReleaseStringUTFChars(env, name, name_str);
      throw_exception (errno, error);
   
      return NULL;
   }
   
   drmaa_release_attr_values (values);
   
   clazz = FindClass (env, "Ljava/lang/String;");
   
   if (is_vector) {
      errno = drmaa_get_vector_attribute (jt, name_str, values, error, BUFFER_LENGTH);
      (*env)->ReleaseStringUTFChars(env, name, name_str);
      
      if (errno != DRMAA_ERRNO_SUCCESS) {
         throw_exception (errno, error);

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
            FREE (names[count]);
         }
      }
   }
   else {
      errno = drmaa_get_attribute (jt, name_str, buffer, BUFFER_LENGTH, error, BUFFER_LENGTH);
      (*env)->ReleaseStringUTFChars(env, name, name_str);
      
      if (errno != DRMAA_ERRNO_SUCCESS) {
         throw_exception (errno, error);

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
   char error[BUFFER_LENGTH];
   int errno = DRMAA_ERRNO_SUCCESS;
   drmaa_job_template_t *jt = remove_from_list (id);
   
   if (jt != NULL) {
      errno = drmaa_delete_job_template (jt, error, BUFFER_LENGTH);
      
      if (errno != DRMAA_ERRNO_SUCCESS) {
         throw_exception (errno, error);

         return;
      }      
   }
   else {
      throw_exception (DRMAA_INTERNAL_ERROR, "Requested job template does not exist");
      return;
   }
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

static int insert_into_list (drmaa_job_template_t *jt)
{
   int count = 0;
   drmaa_job_template_t **tmp_list = NULL;
   int tmp_length = 0;
   
   sge_mutex_lock("JTList", "insert_into_list", __LINE__, &list_mutex);

   /* If we haven't initialized the template list yet, do so. */
   if (job_templates == NULL) {
      list_length = TEMPLATE_LIST_LENGTH;
      job_templates = (drmaa_job_template_t **)malloc (sizeof (drmaa_job_template_t *) * list_length);
      memset (job_templates, 0, list_length * sizeof (drmaa_job_template_t *));
   }

   /* Search for an empty slot. */
   for (count = 0; count < list_length; count++) {
      if (job_template[count] == NULL) {
         /* Insert the template and return the index. */
         job_template[count] = jt;
   
         sge_mutex_unlock("JTList", "insert_into_list", __LINE__, &list_mutex);

         return count;
      }
   }

   /* If there are no empty slots, double the size of the list. */
   tmp_length = list_length * 2;
   tmp_list = (drmaa_job_template_t **)malloc (sizeof (drmaa_job_template_t *) * tmp_length);
   memcpy (tmplist, job_templates, list_length * sizeof (drmaa_job_template_t *));
   memset (&tmp_list[count], 0, list_length * sizeof (drmaa_job_template_t *));
   
   list_length = tmp_length;
   free (job_templates);
   job_templates = tmp_list;
   
   /* Insert the template and return the index. */
   job_template[count] = jt;
   
   sge_mutex_unlock("JTList", "insert_into_list", __LINE__, &list_mutex);
   
   return count;
}

static drmaa_job_template_t *get_from_list (int id) {
   drmaa_job_template_t retval = NULL;
   
   if ((job_templates != NULL) && (id >= 0)) {
      sge_mutex_lock("JTList", "get_from_list", __LINE__, &list_mutex);
      if (id < list_length) {
         retval = job_templates[id];
      }
      sge_mutex_unlock("JTList", "get_from_list", __LINE__, &list_mutex);
   }
   
   return retval;
}

static drmaa_job_template_t *remove_from_list (int id) {
   drmaa_job_template_t retval = NULL;
   
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
