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
 *   Copyright: 2003 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#ifndef NO_JNI

#include "sge_thread_jvm.h"
#include "basis_types.h"
#include "sge_qmaster_threads.h"
#include "sgermon.h"
#include "sge_mt_init.h"
#include "sge_prog.h"
#include "sge_log.h"
#include "sge_unistd.h"
#include "sge_answer.h"
#include "setup_qmaster.h"
#include "sge_security.h"
#include "sge_manop.h"
#include "sge_mtutil.h"
#include "sge_lock.h"
#include "sge_qmaster_process_message.h"
#include "sge_event_master.h"
#include "sge_persistence_qmaster.h"
#include "sge_reporting_qmaster.h"
#include "sge_qmaster_timed_event.h"
#include "sge_host_qmaster.h"
#include "sge_userprj_qmaster.h"
#include "sge_give_jobs.h"
#include "sge_all_listsL.h"
#include "sge_calendar_qmaster.h"
#include "sge_time.h"
#include "lock.h"
#include "qmaster_heartbeat.h"
#include "shutdown.h"
#include "sge_spool.h"
#include "cl_commlib.h"
#include "sge_uidgid.h"
#include "sge_bootstrap.h"
#include "msg_common.h"
#include "msg_qmaster.h"
#include "msg_daemons_common.h"
#include "msg_utilib.h"  /* remove once 'sge_daemonize_qmaster' did become 'sge_daemonize' */
#include "sge.h"
#include "sge_qmod_qmaster.h"
#include "reschedule.h"
#include "sge_job_qmaster.h"
#include "sge_profiling.h"
#include "sgeobj/sge_conf.h"
#include "qm_name.h"
#include "setup_path.h"
#include "uti/sge_os.h"
#include "sge_advance_reservation_qmaster.h"
#include "sge_sched_process_events.h"
#include "sge_follow.h"
#include "uti/sge_string.h"
#include "uti/sge_thread_ctrl.h"
#include "configuration_qmaster.h"
#include "sgeobj/config.h"
#include "uti/sge_dstring.h"

#include "sge_thread_main.h"


#include <jni.h>

#ifdef LINUX
#ifndef __USE_GNU
#define __USE_GNU
#endif
#endif

#include <dlfcn.h>
#include <string.h>

#ifdef LINUX
#ifdef __USE_GNU
#undef __USE_GNU
#endif
#endif

#ifdef SOLARIS
#include <link.h>
#endif


static JavaVM* myjvm = NULL;
static pthread_mutex_t myjvm_mutex = PTHREAD_MUTEX_INITIALIZER;

static JNIEnv* create_vm(const char *libjvm_path, int argc, char** argv);
static int invoke_main(JNIEnv* env, jclass main_class, int argc, char** argv);

typedef int (*JNI_CreateJavaVM_Func)(JavaVM **pvm, void **penv, void *args);

#ifdef DARWIN
int JNI_CreateJavaVM_Impl(JavaVM **pvm, void **penv, void *args);
#endif

static bool
sge_run_jvm(sge_gdi_ctx_class_t *ctx, void *anArg, monitoring_t *monitor);

static void
sge_qmaster_thread_jvm_cleanup_monitor(monitoring_t *monitor)
{
   DENTER(TOP_LAYER, "sge_qmaster_thread_jvm_cleanup_monitor");
   sge_monitor_free(monitor);
   DRETURN_VOID;
}

/*-------------------------------------------------------------------------*
 * NAME
 *   shutdown_main - invoke the shutdown method of com.sun.grid.jgdi.management.JGDIAgent
 * PARAMETER
 *  env  - the JNI environment
 *  argc - number of arguments for the main method
 *  argv - the arguments for the main method
 *
 * RETURN
 *
 *  exit code of the JVM
 *
 * EXTERNAL
 *
 * DESCRIPTION
 *-------------------------------------------------------------------------*/
static bool shutdown_main(void) 
{
   char main_class_name[] = "com/sun/grid/jgdi/management/JGDIAgent";
	jmethodID shutdown_mid = NULL;
   JNIEnv *env;
   jclass main_class;
   int ret = 0;
   bool error_occured = false;
   JavaVMAttachArgs attach_args = { JNI_VERSION_1_2, NULL, NULL };

   DENTER(TOP_LAYER, "shutdown_main");

   pthread_mutex_lock(&myjvm_mutex);

   if (myjvm == NULL) {
      pthread_mutex_unlock(&myjvm_mutex);
      DRETURN(true);
   }

   ret = (*myjvm)->AttachCurrentThread(myjvm, (void**) &env, &attach_args);

   if (ret < 0) { 
      CRITICAL((SGE_EVENT, "could not attach thread to vm\n"));
      pthread_mutex_unlock(&myjvm_mutex);
      DRETURN(false);
   }   

   if (env != NULL) {
      main_class = (*env)->FindClass(env, main_class_name);
      if (main_class != NULL) {
	      shutdown_mid = (*env)->GetStaticMethodID(env, main_class, "shutdown", "()V");
         if (shutdown_mid == NULL) {
            CRITICAL((SGE_EVENT, "class has no shutdown method\n"));
            error_occured = true;
         }
      } else {
         CRITICAL((SGE_EVENT, "main_class is NULL\n"));
         error_occured = true;
      }  

      if (!error_occured) {
         (*env)->CallStaticVoidMethod(env, main_class, shutdown_mid);

         if ((*env)->ExceptionOccurred(env)) {
            (*env)->ExceptionClear(env);
            CRITICAL((SGE_EVENT, "unexpected jvm exception\n"));
            error_occured = true;
         }
      }   
   }

   ret = (*myjvm)->DetachCurrentThread(myjvm);
   if (ret < 0) { 
      CRITICAL((SGE_EVENT, "could not detach thread from vm\n"));
      error_occured = true;
   }   

   pthread_mutex_unlock(&myjvm_mutex);

   if (error_occured) {
      DRETURN(false);
   } else {
      DRETURN(true);
   }
}

static jint (JNICALL printVMErrors)(FILE *fp, const char *format, va_list args) {
   jint rc = 0;
   const char* str = NULL;
   dstring ds = DSTRING_INIT;

   DENTER(TOP_LAYER, "printVMErrors");
   str = sge_dstring_vsprintf(&ds, format, args);
   if (str != NULL) {
      rc = strlen(str);
      DPRINTF(("%s\n", str));
      CRITICAL((SGE_EVENT, "JVM message: %s", str));
   }
   sge_dstring_free(&ds);
   DRETURN(rc);
}

#if 0
static void (JNICALL exitVM)(jint code) {
    DENTER(TOP_LAYER, "exitVM");
    DPRINTF(("CALLED exitVM %d\n", (int)code));
    fflush(stdout);
    DRETURN_VOID;
}
#endif


/*-------------------------------------------------------------------------*
 * NAME
 *   create_vm - create a jvm
 * PARAMETER
 *  argc - number of arguments for the jvm
 *  argv - arguments for the jvm
 *
 * RETURN
 *
 *   NULL -  jvm could not be created
 *   else -  jvm has been created
 *
 * EXTERNAL
 *
 * DESCRIPTION
 *-------------------------------------------------------------------------*/
static JNIEnv* create_vm(const char *libjvm_path, int argc, char** argv)
{
   void *libjvm_handle = NULL;
   bool ok = true;
	JavaVM* jvm = NULL;
	JNIEnv* env = NULL;
	JavaVMInitArgs args;
   JNI_CreateJavaVM_Func sge_JNI_CreateJavaVM = NULL;
   int i = 0;
	JavaVMOption* options = NULL;
   const int extraOptionCount = 1;

   DENTER(GDI_LAYER, "create_vm");

   options = (JavaVMOption*)malloc((argc+extraOptionCount)*sizeof(JavaVMOption));

	/* There is a new JNI_VERSION_1_4, but it doesn't add anything for the purposes of our example. */
	args.version = JNI_VERSION_1_2;
	args.nOptions = argc+extraOptionCount;
   options[0].optionString = "vfprintf";
   options[0].extraInfo = (void*)printVMErrors;
#if 0   
   options[1].optionString = "exit";
   options[1].extraInfo = exitVM;
#endif   
   for(i=0; i < argc; i++) {
      /*printf("argv[%d] = %s\n", i, argv[i]);*/
      options[i+extraOptionCount].optionString = argv[i];
   }

	args.options = options;
	args.ignoreUnrecognized = JNI_FALSE;

   /* build the full name of the shared lib - append architecture dependent
    * shlib postfix 
    */
#ifdef HP1164
   /*
   ** need to switch to start user for HP
   */
   sge_switch2start_user();
#endif   

   /* open the shared lib */
   # if defined(DARWIN)
   # ifdef RTLD_NODELETE
   libjvm_handle = dlopen(libjvm_path, RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
   # else
   libjvm_handle = dlopen(libjvm_path, RTLD_NOW | RTLD_GLOBAL );
   # endif /* RTLD_NODELETE */
   # elif defined(HP11) || defined(HP1164)
   # ifdef RTLD_NODELETE
   libjvm_handle = dlopen(libjvm_path, RTLD_LAZY | RTLD_NODELETE);
   # else
   libjvm_handle = dlopen(libjvm_path, RTLD_LAZY );
   # endif /* RTLD_NODELETE */
   # else
   # ifdef RTLD_NODELETE
   libjvm_handle = dlopen(libjvm_path, RTLD_LAZY | RTLD_NODELETE);
   # else
   libjvm_handle = dlopen(libjvm_path, RTLD_LAZY);
   # endif /* RTLD_NODELETE */
   #endif

#ifdef HP1164
   /*
   ** switch back to admin user for HP
   */
   sge_switch2admin_user();
#endif

   if (libjvm_handle == NULL) {
      CRITICAL((SGE_EVENT, "could not load libjvmi %s", dlerror()));
      ok = false;
   }

   /* retrieve function pointer of get_method function in shared lib */
   if (ok) {
#ifdef DARWIN
      /*
      ** for darwin there exists no JNI_CreateJavaVM, Why not maybe a fix in the future ???
      */
      const char* JNI_CreateJavaVM_FuncName = "JNI_CreateJavaVM_Impl";
#else
      const char* JNI_CreateJavaVM_FuncName = "JNI_CreateJavaVM";
#endif
	   sge_JNI_CreateJavaVM = (JNI_CreateJavaVM_Func)dlsym(libjvm_handle, JNI_CreateJavaVM_FuncName);
      if (sge_JNI_CreateJavaVM == NULL) {
         CRITICAL((SGE_EVENT, "could not load sge_JNI_CreateJavaVM %s", dlerror()));
         ok = false;
      }
   }

   if (ok) {
/* printf("------> Running22 as uid/euid: %d/%d\n", getuid(), geteuid()); */
      if ((i = sge_JNI_CreateJavaVM(&jvm, (void **)&env, &args)) < 0) {
         CRITICAL((SGE_EVENT, "can not create JVM (error code %d)\n", i));
         env = NULL;
      }

      pthread_mutex_lock(&myjvm_mutex);
      myjvm = jvm;
      pthread_mutex_unlock(&myjvm_mutex);
   }
   free(options);
	DRETURN(env);
}

/*-------------------------------------------------------------------------*
 * NAME
 *   invoke_main - invoke the main method of com.sun.grid.jgdi.management.JGDIAgent
 * PARAMETER
 *  env  - the JNI environment
 *  argc - number of arguments for the main method
 *  argv - the arguments for the main method
 *
 * RETURN
 *
 *  exit code of the JVM
 *
 * EXTERNAL
 *
 * DESCRIPTION
 *-------------------------------------------------------------------------*/
static int invoke_main(JNIEnv* env, jclass main_class, int argc, char** argv) 
{
	jmethodID main_mid;
	jobjectArray main_args;
   int i = 0;

   DENTER(TOP_LAYER, "invoke_main");
   
	main_mid = (*env)->GetStaticMethodID(env, main_class, "main", "([Ljava/lang/String;)V");
   if (main_mid == NULL) {
      CRITICAL((SGE_EVENT, "class has no main method\n"));
      DRETURN(1);
   }

	main_args = (*env)->NewObjectArray(env, argc, (*env)->FindClass(env, "java/lang/String"), NULL);
   
   for(i=0; i < argc; i++) {
      jstring str = (*env)->NewStringUTF(env, argv[i]);
      DPRINTF(("argv[%d] = %s\n", i, argv[i]));
      (*env)->SetObjectArrayElement(env, main_args, i, str);
   }

   INFO((SGE_EVENT, "Starting up jvm thread\n"));
	(*env)->CallStaticVoidMethod(env, main_class, main_mid, main_args);
   
   if ((*env)->ExceptionOccurred(env)) {
      (*env)->ExceptionClear(env);
      CRITICAL((SGE_EVENT, "unexpected exception in invoke_main\n"));
      DRETURN(1);
   }

   DRETURN(0);
}


static bool
sge_run_jvm(sge_gdi_ctx_class_t *ctx, void *anArg, monitoring_t *monitor)
{

   dstring ds = DSTRING_INIT;
   const char *libjvm_path = NULL;
   char **jvm_argv;
   int jvm_argc = 0;
   char *main_argv[1];
   int i;
   char main_class_name[] = "com/sun/grid/jgdi/management/JGDIAgent";
   JNIEnv* env = NULL;
   jobject main_class = NULL;
   char* additional_jvm_args = NULL;
   char** additional_jvm_argv = NULL;
   int additional_jvm_argc = 0;
   lListElem *confEntry = NULL;
   const int fixed_jvm_argc = 14;
   bool ret = true;

   DENTER(TOP_LAYER, "sge_run_jvm");

   confEntry = sge_get_configuration_entry_by_name(ctx->get_qualified_hostname(ctx), "libjvm_path");
   if (confEntry != NULL) {
      const char *confVal = lGetString(confEntry, CF_value);
      if (confVal && *confVal != '\0') {
         libjvm_path = strdup(confVal);
      }   
      lFreeElem(&confEntry);
   }   

   if (libjvm_path == NULL) {
      WARNING((SGE_EVENT, "libjvm_path is NULL\n"));
      DRETURN(false);
   }  

   /*
   ** get additional jvm args from configuration
   */
   confEntry = sge_get_configuration_entry_by_name(ctx->get_qualified_hostname(ctx), "additional_jvm_args");
   if (confEntry != NULL) {
      const char *confVal = lGetString(confEntry, CF_value);
      if (confVal && *confVal != '\0') {
         additional_jvm_args = strdup(confVal);
      }
      lFreeElem(&confEntry);
   }   

   if (additional_jvm_args != NULL) {
        DPRINTF(("additional_jvm_args: >%s<\n", additional_jvm_args));
        additional_jvm_argv = string_list(additional_jvm_args, " ", NULL);

        for (i=0; additional_jvm_argv[i] != NULL; i++) {
           DPRINTF(("additional jvm arg[%d]: %s\n", i, additional_jvm_argv[i]));
           additional_jvm_argc++;
        }
        jvm_argc = fixed_jvm_argc + additional_jvm_argc;
   } else {
      additional_jvm_argv = NULL;
      jvm_argc = fixed_jvm_argc;
      additional_jvm_argc = 0;
   }

   DPRINTF(("fixed_jvm_argc + additional_jvm_argc = jvm_argc: %d + %d = %d\n", fixed_jvm_argc, additional_jvm_argc, jvm_argc));
  
   jvm_argv = (char**)sge_malloc(jvm_argc * sizeof(char*));
   /*
   ** adjust fixed_jvm_argc if an additional fixed argument line is added
   */
   jvm_argv[0] = strdup(sge_dstring_sprintf(&ds, "-Djava.class.path=%s/lib/jgdi.jar:%s/lib/juti.jar", ctx->get_sge_root(ctx), ctx->get_sge_root(ctx)));
   jvm_argv[1] = strdup(sge_dstring_sprintf(&ds, "-Djava.security.policy=%s/common/jmx/java.policy", ctx->get_cell_root(ctx)));
   jvm_argv[2] = strdup("-Djava.security.manager=com.sun.grid.jgdi.management.JGDISecurityManager");
   jvm_argv[3] = strdup(sge_dstring_sprintf(&ds, "-Djava.rmi.server.codebase=file://%s/lib/jgdi.jar", ctx->get_sge_root(ctx)));
   jvm_argv[4] = strdup(sge_dstring_sprintf(&ds, "-Djava.library.path=%s/lib/%s", ctx->get_sge_root(ctx), sge_get_arch()));
   jvm_argv[5] = strdup(sge_dstring_sprintf(&ds, "-Dcom.sun.management.jmxremote.access.file=%s/common/jmx/jmxremote.access", ctx->get_cell_root(ctx)));
   jvm_argv[6] = strdup(sge_dstring_sprintf(&ds, "-Dcom.sun.management.jmxremote.password.file=%s/common/jmx/jmxremote.password", ctx->get_cell_root(ctx)));
   jvm_argv[7] = strdup(sge_dstring_sprintf(&ds, "-Djava.security.auth.login.config=%s/common/jmx/jaas.config", ctx->get_cell_root(ctx)));
   jvm_argv[8] = strdup(sge_dstring_sprintf(&ds, "-Djava.util.logging.config.file=%s/common/jmx/logging.properties", ctx->get_cell_root(ctx)));
   jvm_argv[9] = strdup(sge_dstring_sprintf(&ds, "-Dcom.sun.grid.jgdi.sgeRoot=%s", ctx->get_sge_root(ctx)));
   jvm_argv[10] = strdup(sge_dstring_sprintf(&ds, "-Dcom.sun.grid.jgdi.sgeCell=%s", ctx->get_default_cell(ctx)));
   jvm_argv[11] = strdup(sge_dstring_sprintf(&ds, "-Dcom.sun.grid.jgdi.sgeQmasterSpoolDir=%s", ctx->get_qmaster_spool_dir(ctx)));
/*    jvm_argv[13] = strdup("-Djava.util.logging.manager=com.sun.grid.jgdi.util.JGDILogManager"); */
   jvm_argv[12] = strdup("-Djava.util.logging.manager=java.util.logging.LogManager");
   jvm_argv[13] = strdup("-Xrs");

   /*
   ** add additional_jvm_args
   */
   for (i=0; i < additional_jvm_argc; i++) {
      jvm_argv[fixed_jvm_argc+i] = strdup(additional_jvm_argv[i]);
      additional_jvm_argv[i] = NULL;
   }
   FREE(additional_jvm_argv);

   /*
   ** process arguments of main method
   */
   main_argv[0] = strdup(sge_dstring_sprintf(&ds, "bootstrap://%s@%s:"sge_u32, ctx->get_sge_root(ctx),
                         ctx->get_default_cell(ctx), ctx->get_sge_qmaster_port(ctx)));

   sge_dstring_free(&ds);


   /*
   ** for debugging
   */
   for (i=0; i<jvm_argc; i++) {
      DPRINTF(("jvm_argv[%d]: %s\n", i, jvm_argv[i]));
   }  

   for (i=0; i<sizeof(main_argv)/sizeof(char*); i++) {
      DPRINTF(("main_argv[%d]: %s\n", i, main_argv[i]));
   }  

   env = create_vm(libjvm_path, jvm_argc, jvm_argv);
   FREE(libjvm_path);

   if (env != NULL) {
      main_class = (*env)->FindClass(env, main_class_name);
      if (main_class != NULL) {
         if (invoke_main(env, main_class, sizeof(main_argv)/sizeof(char*), main_argv) != 0) {
            CRITICAL((SGE_EVENT, "invoke_main failed\n"));
            ret = false;
         }
      } else {
         CRITICAL((SGE_EVENT, "main_class is NULL\n"));
         ret = false;
      }  
   } else {
      ret = false;
   }

   pthread_mutex_lock(&myjvm_mutex);
   if (myjvm != NULL) {
      (*myjvm)->DestroyJavaVM(myjvm);
      myjvm = NULL;
   }
   pthread_mutex_unlock(&myjvm_mutex);

   /*
   ** free allocated jvm args
   */
   for (i=0; i<jvm_argc; i++) {
      FREE(jvm_argv[i]);
   }  
   FREE(jvm_argv);

   DRETURN(ret);
}

void
sge_jvm_initialize(sge_gdi_ctx_class_t *ctx)
{
   const int max_initial_jvm_threads = ctx->get_jvm_thread_count(ctx);
   cl_thread_settings_t* dummy_thread_p = NULL;
   int i;

   DENTER(TOP_LAYER, "sge_jvm_initialize");

   cl_thread_list_setup(&(Main_Control.jvm_thread_pool), "jvm thread pool");
   for (i = 0; i < max_initial_jvm_threads; i++) {
      dstring thread_name = DSTRING_INIT;      

/*       INFO((SGE_EVENT, MSG_QMASTER_THREADCOUNT_US, sge_u32c(i), threadnames[JVM_THREAD])); */
      sge_dstring_sprintf(&thread_name, "%s%03d", threadnames[JVM_THREAD], i);
      cl_thread_list_create_thread(Main_Control.jvm_thread_pool, &dummy_thread_p,
                                   NULL, sge_dstring_get_string(&thread_name), i, 
                                   sge_jvm_main, NULL, NULL);
      sge_dstring_free(&thread_name);
   }
   DRETURN_VOID;
}

void
sge_jvm_terminate(void)
{
   cl_thread_settings_t* thread = NULL;
   DENTER(TOP_LAYER, "sge_jvm_terminate");

   thread = cl_thread_list_get_first_thread(Main_Control.jvm_thread_pool);
   while (thread != NULL) {
      shutdown_main();
      DPRINTF((SFN" gets canceled\n", thread->thread_name));
      cl_thread_list_delete_thread(Main_Control.jvm_thread_pool, thread);
      thread = cl_thread_list_get_first_thread(Main_Control.jvm_thread_pool);
   }
   DPRINTF(("all "SFN" threads terminated\n", threadnames[JVM_THREAD]));
   DRETURN_VOID;
}

/****** qmaster/sge_qmaster_main/sge_qmaster_thread_worker_main() *************
*  NAME
*     sge_jvm_main() -- jvm thread function 
*
*  SYNOPSIS
*     static void* sge_jvm_main(void* arg) 
*
*  FUNCTION
*     run jvm. 
*
*  INPUTS
*     void* arg -  commlib thread configuration 
*
*  RESULT
*     void* - none 
*
*  NOTES
*     MT-NOTE: jvm_thread() is a thread function. Do NOT use this function
*     MT-NOTE: in any other way!
*
*******************************************************************************/
void *
sge_jvm_main(void *arg)
{
   cl_thread_settings_t *thread_config = (cl_thread_settings_t*)arg;
   time_t next_prof_output = 0;
   monitoring_t monitor;
   sge_gdi_ctx_class_t *ctx = NULL;
   bool jvm_started = false;
   bool do_endlessly = true;

   DENTER(TOP_LAYER, "sge_jvm_main");

   cl_thread_func_startup(thread_config);
   sge_monitor_init(&monitor, thread_config->thread_name, GDI_EXT, MT_WARNING, MT_ERROR);
   sge_qmaster_thread_init(&ctx, QMASTER, JVM_THREAD, true);

   set_thread_name(pthread_self(), "JVM Thread");
   conf_update_thread_profiling("JVM Thread");

   while (do_endlessly) {
      int execute = 0;

      thread_start_stop_profiling();

      if (jvm_started == false) {
         jvm_started = sge_run_jvm(ctx, arg, &monitor);
      }

      thread_output_profiling("JVM thread profiling summary:\n", 
                              &next_prof_output);

      sge_monitor_output(&monitor);

      /* 
      ** to prevent high cpu load if jvm is not started
      */
      if (!jvm_started) {
         sge_thread_wait_for_signal();
      }

      /* pthread cancelation point */
      do {
         pthread_cleanup_push((void (*)(void *))sge_qmaster_thread_jvm_cleanup_monitor,
                              (void *)&monitor);
         cl_thread_func_testcancel(thread_config);
         pthread_cleanup_pop(execute);
         if (sge_thread_has_shutdown_started()) {
            DPRINTF((SFN" is waiting for termination", thread_config->thread_name));
            sleep(1);
         }
      } while (sge_thread_has_shutdown_started()); 
   }

   /*
    * Don't add cleanup code here. It will never be executed. Instead register
    * a cleanup function with pthread_cleanup_push()/pthread_cleanup_pop() before 
    * the call of cl_thread_func_testcancel()
    */
   
   DEXIT;
   return NULL;
} 

#endif

