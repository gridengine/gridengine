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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#if defined(FREEBSD) || defined(DARWIN)
#include <sys/time.h>
#endif
#include <sys/resource.h>
#include <sys/wait.h>


#include "sge_string.h"
#include "setup.h"
#include "sge_unistd.h"
#include "sge_all_listsL.h"
#include "usage.h"
#include "sig_handlers.h"
#include "commlib.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"

#include "msg_clients_common.h"
#include "msg_common.h"

#include "sge_answer.h"
#include "sge_mirror.h"
#include "sge_event.h"
#include "sge_event_client.h"
#include "sge_time.h"
#include "sge_unistd.h"
#include "sge_feature.h"
#include "sge_spool.h"
#include "qevent.h"


#if defined(SOLARIS) || defined(ALPHA)
/* ALPHA4 only has wait3() prototype if _XOPEN_SOURCE_EXTENDED is defined */
pid_t wait3(int *, int, struct rusage *);
#endif


u_long Global_jobs_running = 0;
u_long Global_jobs_registered = 0;
qevent_options *Global_qevent_options;


static void qevent_show_usage(void);
static void qevent_testsuite_mode(void);
static char* qevent_get_event_name(int event);
static void qevent_trigger_scripts( int qevent_event, qevent_options *option_struct, lListElem *event );
static void qevent_start_trigger_script(int qevent_event, const char* script_file, lListElem *event  );
static qevent_options* qevent_get_option_struct(void);
static void qevent_set_option_struct(qevent_options *option_struct);



int main(int argc, char *argv[]);


static void  qevent_set_option_struct(qevent_options *option_struct) {
   Global_qevent_options=option_struct;
}


static qevent_options* qevent_get_option_struct(void) {
   return Global_qevent_options;
}

static void qevent_dump_pid_file(void) {
   sge_write_pid("qevent.pid");
}


bool print_event(sge_object_type type, sge_event_action action, 
                lListElem *event, void *clientdata)
{
   char buffer[1024];
   dstring buffer_wrapper;

   sge_dstring_init(&buffer_wrapper, buffer, sizeof(buffer));

   fprintf(stdout, "%s\n", event_text(event, &buffer_wrapper));
   fflush(stdout);
   /* create a callback error to test error handling */
   if(type == SGE_TYPE_GLOBAL_CONFIG) {
      return false;
   }
   
   return true;
}

bool print_jatask_event(sge_object_type type, sge_event_action action, 
                lListElem *event, void *clientdata)
{
   int pos;
   char buffer[1024];
   u_long32 timestamp;
   dstring buffer_wrapper;


   sge_dstring_init(&buffer_wrapper, buffer, sizeof(buffer));
   
   timestamp = sge_get_gmt();

   DPRINTF(("%s\n", event_text(event, &buffer_wrapper)));
/*    fprintf(stdout,"%s\n",event_text(event, &buffer_wrapper)); */
   if ((pos=lGetPosViaElem(event, ET_type))>=0) {
      u_long32 type = lGetUlong(event, ET_type);
      if (type == sgeE_JATASK_MOD) { 
         lList *jat = lGetList(event,ET_new_version);
         u_long job_id  = lGetUlong(event, ET_intkey);
         u_long task_id = lGetUlong(event, ET_intkey2);
         lListElem *ep = lFirst(jat);
         u_long job_status = lGetUlong(ep, JAT_status);
         int task_running = (job_status==JRUNNING || job_status==JTRANSFERING);

         if (task_running) {
            fprintf(stdout,"JOB_START (%ld.%ld:ECL_TIME="U32CFormat")\n", job_id ,task_id,u32c(timestamp));
            fflush(stdout);  
            Global_jobs_running++;
         }
/*         lWriteElemTo(event, stdout); 
         fflush(stdout); */
      }
      if (type == sgeE_JOB_FINAL_USAGE) { 
         /* lList *jat = lGetList(event,ET_new_version); */
         u_long job_id = lGetUlong(event, ET_intkey);
         u_long task_id = lGetUlong(event, ET_intkey2);
         /* lWriteElemTo(event, stdout); */
         fprintf(stdout,"JOB_FINISH (%ld.%ld:ECL_TIME="U32CFormat")\n", job_id, task_id,u32c(timestamp));
         Global_jobs_running--;
         fflush(stdout);  
      }
      if (type == sgeE_JOB_ADD) { 
         lList *jat = lGetList(event,ET_new_version);
         u_long job_id  = lGetUlong(event, ET_intkey);
         u_long task_id = lGetUlong(event, ET_intkey2);
         lListElem *ep = lFirst(jat);
         const char* job_project = lGetString(ep, JB_project);
         if (job_project == NULL) {
            job_project = "NONE";
         }
         fprintf(stdout,"JOB_ADD (%ld.%ld:ECL_TIME="U32CFormat":project=%s)\n", job_id, task_id, u32c(timestamp),job_project);
         Global_jobs_registered++;
         fflush(stdout);  
      }
      if (type == sgeE_JOB_DEL) { 
         u_long job_id  = lGetUlong(event, ET_intkey);
         u_long task_id = lGetUlong(event, ET_intkey2);
         fprintf(stdout,"JOB_DEL (%ld.%ld:ECL_TIME="U32CFormat")\n", job_id, task_id,u32c(timestamp));
         Global_jobs_registered--;
         fflush(stdout);  
      }



   }
   /* create a callback error to test error handling */
   if(type == SGE_TYPE_GLOBAL_CONFIG) {
      return false;
   }
   
   return true;
}


bool analyze_jatask_event(sge_object_type type, sge_event_action action, 
                lListElem *event, void *clientdata)
{
   int pos;
   char buffer[1024];
   u_long32 timestamp;
   dstring buffer_wrapper;


   sge_dstring_init(&buffer_wrapper, buffer, sizeof(buffer));
   
   timestamp = sge_get_gmt();

   if ((pos=lGetPosViaElem(event, ET_type))>=0) {
      u_long32 type = lGetUlong(event, ET_type);

      if (type == sgeE_JATASK_MOD) { 
         lList *jat = lGetList(event,ET_new_version);
         lListElem *ep = lFirst(jat);
         u_long job_status = lGetUlong(ep, JAT_status);
         int task_running = (job_status==JRUNNING || job_status==JTRANSFERING);
         if (task_running) {
         }
      }

      if (type == sgeE_JOB_FINAL_USAGE) { 
      }

      if (type == sgeE_JOB_ADD) { 
         /* lList *jat = lGetList(event,ET_new_version);
         u_long job_id  = lGetUlong(event, ET_intkey);
         u_long task_id = lGetUlong(event, ET_intkey2);
         lListElem *ep = lFirst(jat); */
      }

      if (type == sgeE_JOB_DEL) { 
         qevent_trigger_scripts(QEVENT_JB_END, qevent_get_option_struct(), event);
      }

      if (type == sgeE_JATASK_DEL) { 
         qevent_trigger_scripts(QEVENT_JB_TASK_END,qevent_get_option_struct() , event);
      }


   }
   /* create a callback error to test error handling */
   if(type == SGE_TYPE_GLOBAL_CONFIG) {
      return false;
   }
   
   return true;
}



static void qevent_trigger_scripts( int qevent_event, qevent_options *option_struct, lListElem *event) {

   int i=0;
   DENTER(TOP_LAYER, "qevent_trigger_scripts");
   if (option_struct->trigger_option_count > 0) {
      INFO((SGE_EVENT, "trigger for event "SFN"\n", qevent_get_event_name(qevent_event) ));
      for (i=0;i<option_struct->trigger_option_count;i++) {
         if ( (option_struct->trigger_option_events)[i] == qevent_event ) {
            qevent_start_trigger_script(qevent_event ,(option_struct->trigger_option_scripts)[i], event);
         }
      }
   }
   DEXIT;
}

static void qevent_start_trigger_script(int qevent_event, const char* script_file, lListElem *event ) {
   u_long jobid, taskid;
   const char* event_name;
   int pid;
   char buffer[MAX_STRING_SIZE];
   char buffer2[MAX_STRING_SIZE];

   DENTER(TOP_LAYER, "qevent_start_trigger_script");

   jobid  = lGetUlong(event, ET_intkey);
   taskid = lGetUlong(event, ET_intkey2);
   event_name = qevent_get_event_name(qevent_event);
   

   /* test if script is executable and guilty file */
   if (!sge_is_file(script_file)) {
      ERROR((SGE_EVENT, "no script file: "SFQ"\n", script_file));
      DEXIT;
      return;
   }

   /* is file executable ? */
   if (!sge_is_executable(script_file)) {  
      ERROR((SGE_EVENT, "file not executable: "SFQ"\n", script_file));
      DEXIT;
      return;
   } 

   pid = fork();
   if (pid < 0) {
      ERROR((SGE_EVENT, "fork() error\n"));
      DEXIT;
      return;
   }

   if (pid > 0) {
      int pid2;
      int exit_status;

      #if !(defined(HPUX) || defined(HP10_01) || defined(HPCONVEX) || defined(CRAY) || defined(SINIX))
         struct rusage rusage;
      #endif

      #if defined(SVR3) || defined(_BSD)
         union wait status;
      #else
         int status;
      #endif
      #if defined(HPUX) || defined(HP10_01) || defined(HPCONVEX) || defined(CRAY) || defined(SINIX)
         pid2 = waitpid(pid, &status, 0);
      #else
         pid2 = wait3(&status, 0, &rusage);
      #endif
      #if defined(SVR3) || defined(_BSD)
         exit_status = status.w_retcode;
      #else
         exit_status = status;
      #endif

      if ( WEXITSTATUS(exit_status) == 0 ) {
         INFO((SGE_EVENT,"exit status of script: "U32CFormat"\n", u32c(WEXITSTATUS(exit_status))));
      } else {
         ERROR((SGE_EVENT,"exit status of script: "U32CFormat"\n", u32c(WEXITSTATUS(exit_status))));
      }
      DEXIT;
      return;
   } else {
      
      /*      SETPGRP;  */
      /*      sge_close_all_fds(NULL); */
      sprintf(buffer  ,""U32CFormat"",u32c(jobid));
      sprintf(buffer2 ,""U32CFormat"",u32c(taskid)); 
      execlp( script_file , sge_basename( script_file, '/' ), event_name, buffer, buffer2, 0 );
   }
   exit(1);
}

static void qevent_show_usage(void) {
   dstring ds;
   char buffer[256];
   
   sge_dstring_init(&ds, buffer, sizeof(buffer));

   fprintf(stdout, "%s\n", feature_get_product_name(FS_SHORT_VERSION, &ds));
   fprintf(stdout, "%s\n", MSG_SRC_USAGE );

   fprintf(stdout,"qevent [-h|-help] -ts|-testsuite\n");
   fprintf(stdout,"qevent [-h|-help] -trigger EVENT SCRIPT [ -trigger EVENT SCRIPT, ... ]\n\n");
   
   fprintf(stdout,"   -h,  -help             show usage\n");
   fprintf(stdout,"   -ts, -testsuite        run in testsuite mode\n");
   fprintf(stdout,"   -trigger EVENT SCRIPT  start SCRIPT (executable) when EVENT occurs\n");
   fprintf(stdout,"\n");
   fprintf(stdout,"SCRIPT - path to a executable shell script\n");
   fprintf(stdout,"         1. command line argument: event name\n");
   fprintf(stdout,"         2. command line argument: jobid\n");
   fprintf(stdout,"         3. command line argument: taskid\n");
   fprintf(stdout,"EVENT  - One of the following event category:\n");
   fprintf(stdout,"         %s      - job end event\n", qevent_get_event_name(QEVENT_JB_END));
   fprintf(stdout,"         %s - job task end event\n", qevent_get_event_name(QEVENT_JB_TASK_END));
}


static void qevent_parse_command_line(int argc, char **argv, qevent_options *option_struct) {

   
   DENTER(TOP_LAYER, "qevent_parse_command_line");

   option_struct->help_option = 0;
   option_struct->testsuite_option = 0;
   option_struct->trigger_option_count =0;

   while (*(++argv)) {
      if (!strcmp("-h", *argv) || !strcmp("-help", *argv)) {
         option_struct->help_option = 1;
         continue;
      }
      if (!strcmp("-ts", *argv) || !strcmp("-testsuite", *argv)) {
         option_struct->testsuite_option = 1;
         continue;
      }
      if (!strcmp("-trigger", *argv)) {
         int ok = 0;
         if (option_struct->trigger_option_count >= MAX_TRIGGER_SCRIPTS ) {
            sge_dstring_sprintf(option_struct->error_message,
                                "option \"-trigger\": only "U32CFormat" trigger arguments supported\n",
                                u32c(MAX_TRIGGER_SCRIPTS) );
            break; 
         }

         ++argv;
         if (*argv) {
            /* get EVENT argument */
            if (strcmp(qevent_get_event_name(QEVENT_JB_END),*argv) == 0) {
               ok = 1;
               (option_struct->trigger_option_events)[option_struct->trigger_option_count] = QEVENT_JB_END;
            } 
            if (strcmp(qevent_get_event_name(QEVENT_JB_TASK_END),*argv) == 0) {
               ok = 1;
               (option_struct->trigger_option_events)[option_struct->trigger_option_count] = QEVENT_JB_TASK_END;
            } 

            if (!ok) {
               sge_dstring_append(option_struct->error_message,"option \"-trigger\": undefined EVENT type\n");
               break; 
            }
         } else {
            sge_dstring_append(option_struct->error_message,"option \"-trigger\": found no EVENT argument\n");
            break;
         }
         ++argv;
         if (*argv) {
            /* get SCRIPT argument */

            /* check for SCRIPT file */
            if (!sge_is_file(*argv)) {
               sge_dstring_sprintf(option_struct->error_message,
                                   "option \"-trigger\": SCRIPT file %s not found\n",
                                   (*argv));
               break;
            }

            /* is file executable ? */
            if (!sge_is_executable(*argv)) {  
               sge_dstring_sprintf(option_struct->error_message,
                                   "option \"-trigger\": SCRIPT file %s not executable\n",
                                   (*argv));
               break;

            } 
 
            (option_struct->trigger_option_scripts)[option_struct->trigger_option_count] = *argv;
            (option_struct->trigger_option_count)++;
         } else {
            sge_dstring_append(option_struct->error_message,"option \"-trigger\": found no SCRIPT argument\n");
            break;
         }
         continue;
      }


      /* unkown option */
      if ( *argv[0] == '-' ) {  
         sge_dstring_append(option_struct->error_message,"unkown option: ");
         sge_dstring_append(option_struct->error_message,*argv);
         sge_dstring_append(option_struct->error_message,"\n");
      } else {
         sge_dstring_append(option_struct->error_message,"unkown argument: ");
         sge_dstring_append(option_struct->error_message,*argv);
         sge_dstring_append(option_struct->error_message,"\n");
      }
   } 
   DEXIT;
}

int main(int argc, char *argv[])
{
   qevent_options enabled_options;
   dstring errors = DSTRING_INIT;
   lList *alp = NULL;
   int ret,i,gdi_setup;


   DENTER_MAIN(TOP_LAYER, "test_sge_mirror");

   /* dump pid to file */
   qevent_dump_pid_file();

   sge_gdi_param(SET_MEWHO, QEVENT, NULL);

   gdi_setup = sge_gdi_setup(prognames[QEVENT], &alp);

   /* parse command line */
   enabled_options.error_message = &errors;
   qevent_set_option_struct(&enabled_options);
   qevent_parse_command_line(argc, argv, &enabled_options);

   

   /* check if help option is set */
   if (enabled_options.help_option) {
      qevent_show_usage();
      sge_dstring_free(enabled_options.error_message);
      SGE_EXIT(0);
      return 0;
   }

   /* are there command line parsing errors ? */
   if (sge_dstring_get_string(enabled_options.error_message)) {
      ERROR((SGE_EVENT, "%s", sge_dstring_get_string(enabled_options.error_message) ));
      qevent_show_usage();
      sge_dstring_free(enabled_options.error_message);
      SGE_EXIT(1);
   }


   /* setup event client */
   if ( gdi_setup != AE_OK) {
      sge_dstring_free(enabled_options.error_message);
      answer_exit_if_not_recoverable(lFirst(alp));
      SGE_EXIT(1);
   }

   sge_setup_sig_handlers(QEVENT);

   if ((ret = reresolve_me_qualified_hostname()) != CL_OK) {
      sge_dstring_free(enabled_options.error_message);
      SGE_EXIT(1);
   }   

   /* ok, start over ... */

   /* check for testsuite option */
   
   if (enabled_options.testsuite_option) {
      /* only for testsuite */
      qevent_testsuite_mode();
      sge_dstring_free(enabled_options.error_message);
      SGE_EXIT(0);
   }

   if (enabled_options.trigger_option_count > 0) {
      
      sge_mirror_initialize(EV_ID_ANY, "sge_mirror -trigger");

      /* put out information about -trigger option */
      for (i=0;i<enabled_options.trigger_option_count;i++) {
         INFO((SGE_EVENT, "trigger script for %s events: %s\n",
                         qevent_get_event_name((enabled_options.trigger_option_events)[i]), 
                         (enabled_options.trigger_option_scripts)[i]));
         switch((enabled_options.trigger_option_events)[i]) {
            case QEVENT_JB_END:
               sge_mirror_subscribe(SGE_TYPE_JOB, analyze_jatask_event, NULL, NULL);
               ec_subscribe(sgeE_JOB_DEL);
               ec_set_flush(sgeE_JOB_DEL,1);

               break;
            case QEVENT_JB_TASK_END:
               sge_mirror_subscribe(SGE_TYPE_JATASK, analyze_jatask_event, NULL, NULL);
               ec_subscribe(sgeE_JATASK_DEL);
               ec_set_flush(sgeE_JATASK_DEL,1);
               break;
         }        
      }

      while(!shut_me_down) {
         sge_mirror_process_events();
      }

      sge_mirror_shutdown();

      sge_dstring_free(enabled_options.error_message);
      SGE_EXIT(0);
      return 0;
   }


   ERROR((SGE_EVENT, "no option selected\n" ));
   qevent_show_usage();
   sge_dstring_free(enabled_options.error_message);
   SGE_EXIT(1);
   return 1;
}

static char* qevent_get_event_name(int event) {
  
   switch(event) {
      case QEVENT_JB_END:
         return "JB_END";
      case QEVENT_JB_TASK_END:
         return "JB_TASK_END";
   }
   return "unexpected event id";
}



void qevent_testsuite_mode(void) 
{
#if 0 /* EB: debug */
#define QEVENT_SHOW_ALL
#endif
   u_long32 timestamp;
   DENTER(TOP_LAYER, "qevent_testsuite_mode");

   sge_mirror_initialize(EV_ID_ANY, "test_sge_mirror");

#ifdef QEVENT_SHOW_ALL
   sge_mirror_subscribe(SGE_TYPE_ALL, print_event, NULL, NULL); 
#else
   sge_mirror_subscribe(SGE_TYPE_JOB, print_jatask_event, NULL, NULL);
   sge_mirror_subscribe(SGE_TYPE_JATASK, print_jatask_event, NULL, NULL);
   
   ec_set_flush(sgeE_JATASK_MOD,0);
   ec_set_flush(sgeE_JOB_FINAL_USAGE,0);
   ec_set_flush(sgeE_JOB_ADD,0);
   ec_set_flush(sgeE_JOB_DEL,0);
#endif
   
   while(!shut_me_down) {
      sge_mirror_process_events();
      timestamp = sge_get_gmt();
#ifndef QEVENT_SHOW_ALL
      fprintf(stdout,"ECL_STATE (jobs_running=%ld:jobs_registered=%ld:ECL_TIME="U32CFormat")\n",
              Global_jobs_running,Global_jobs_registered,u32c(timestamp));
      fflush(stdout);  
#endif
   }

   sge_mirror_shutdown();

   DEXIT;
}
