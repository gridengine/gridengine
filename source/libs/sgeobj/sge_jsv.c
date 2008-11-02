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
 *   Copyright: 2008 by Sun Microsystems, Inc.
 *
 *   All Rights Reserved.
 *
 ************************************************************************/
/*___INFO__MARK_END__*/                                   

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "sge.h"

#include "rmon/sgermon.h"

#include "lck/sge_mtutil.h"
#include "lck/sge_lock.h"

#include "uti/sge_dstring.h"
#include "uti/sge_log.h"
#include "uti/sge_stdio.h"
#include "uti/sge_string.h"
#include "uti/sge_stdlib.h"
#include "uti/sge_time.h"
#include "uti/sge_unistd.h"

#include "gdi/sge_gdi_ctx.h"

#include "sgeobj/sge_answer.h"
#include "sgeobj/sge_job.h"
#include "sgeobj/sge_jsv.h"
#include "sgeobj/sge_jsv_script.h"
#include "sgeobj/sge_str.h"

#define JSV_QUIT_TIMEOUT (10)

/*
 * This mutex is used to secure the jsv_list below.
 */
static pthread_mutex_t jsv_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Each element in this list contains data which shows the state
 * of the corresponding JSV. The order of the elements in the list 
 * is important because the JSV's are executed in that order.
 */
static lList *jsv_list = NULL;   /* JSV_Type */

/* <jsi_url> := [ <type> ":" ] [ <user> "@" ] <path> */
bool 
jsv_url_parse(dstring *jsv_url, lList **answer_list, dstring *type, dstring *user, dstring *path, bool in_client)
{
   bool success = true;

   DENTER(TOP_LAYER, "jsv_url_parse");
   if (jsv_url != NULL) {
      dstring tmp = DSTRING_INIT;
      const char *t, *u, *p;

      sge_dstring_split(jsv_url, ':', type, &tmp);
      sge_dstring_split(&tmp, '@', user, path);
      sge_dstring_strip_white_space_at_eol(type);
      sge_dstring_strip_white_space_at_eol(user);
      sge_dstring_strip_white_space_at_eol(path);
      t = sge_dstring_get_string(type);
      u = sge_dstring_get_string(user);
      p = sge_dstring_get_string(path);

      /*
       * either the type is not specified (the type "script" is used)
       * or "script" has to be specified. In future we might support "sharedlib" ...
       */
      if ((t == NULL) ||
          (t != NULL && strcmp(t, "script") == 0)) {

         if (in_client && u != NULL) {
            answer_list_add_sprintf(answer_list, STATUS_EEXIST, ANSWER_QUALITY_ERROR, 
                                    "JSV script URL does not allow user "SFQ" in clients", u);
         } else {
            if (p != NULL) {
               if (sge_is_file(p) && sge_is_executable(p)) {
                  if (u != NULL) {
                     struct passwd *pw;
                     struct passwd pw_struct;
                     char *buffer;
                     int size;

                     size = get_pw_buffer_size();
                     buffer = sge_malloc(size);
                     pw = sge_getpwnam_r(u, &pw_struct, buffer, size);
                     buffer = sge_free(buffer);
                     if (pw == NULL) {
                        answer_list_add_sprintf(answer_list, STATUS_EEXIST, ANSWER_QUALITY_ERROR,  
                                                "User specified in JSV URL "SFQ" does not exist", u);
                        success = false;
                     }
                  } else {
                     /* we have a type and a path; user is optional */
                     success = true;
                  }
               } else {
                  answer_list_add_sprintf(answer_list, STATUS_EEXIST, ANSWER_QUALITY_ERROR,
                                          "JSV script "SFQ" is either no file or not executable", p);
                  success = false;
               }

            } else {
               answer_list_add_sprintf(answer_list, STATUS_EEXIST, ANSWER_QUALITY_ERROR,
                                       "the given JSV URL "SFQ" is invalid", sge_dstring_get_string(jsv_url));
               success = false;
            }
         }
      } else {
         answer_list_add_sprintf(answer_list, STATUS_EEXIST, ANSWER_QUALITY_ERROR, 
                                 "invalid type "SFQ" specified in jsv url", t);
         success = false;
      }
   }
   DRETURN(success);
}

static lListElem *
jsv_create(const char *name, const char *context, lList **answer_list, const char *jsv_url,
           const char *type, const char *user, const char *scriptfile)
{
   lListElem *new_jsv = NULL;

   DENTER(TOP_LAYER, "jsv_create");
   if (name != NULL && scriptfile != NULL) {
      new_jsv = lCreateElem(JSV_Type);

      if (new_jsv != NULL) {
         SGE_STRUCT_STAT st;

         if (SGE_STAT(scriptfile, &st) == 0) {
            char pid_buffer[256];

            sprintf(pid_buffer, pid_t_fmt, (pid_t)-1);
            lSetString(new_jsv, JSV_name, name);
            lSetString(new_jsv, JSV_context, context);
            lSetString(new_jsv, JSV_url, jsv_url);
            lSetString(new_jsv, JSV_type, type);
            lSetString(new_jsv, JSV_user, user);
            lSetString(new_jsv, JSV_command, scriptfile);
            lSetString(new_jsv, JSV_pid, pid_buffer);
            lSetBool(new_jsv, JSV_send_env, false);
            lSetRef(new_jsv, JSV_in, NULL);
            lSetRef(new_jsv, JSV_out, NULL);
            lSetRef(new_jsv, JSV_err, NULL);
            lSetBool(new_jsv, JSV_has_to_restart, false);
            lSetList(new_jsv, JSV_incomplete, lCreateList("", JSV_Type));
            lSetList(new_jsv, JSV_complete, lCreateList("", JSV_Type));
            lSetUlong(new_jsv, JSV_last_mod, st.st_mtime);

            sge_mutex_lock("jsv_list", SGE_FUNC, __LINE__, &jsv_mutex);

            if (jsv_list == NULL) {
               jsv_list = lCreateList("", JSV_Type);
            }
            if (jsv_list != NULL) {
               lInsertElem(jsv_list, NULL, new_jsv);
            } else {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR,
                                       "No memory to create JSV list");
            }

            sge_mutex_unlock("jsv_list", SGE_FUNC, __LINE__, &jsv_mutex);

         } else {
            lFreeElem(&new_jsv);
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR,
                                    "stat call for JSV script "SFN" failed", scriptfile);
         }
      } else {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR,
                                    "no memory to create JSV element");
      } 
   }
   DRETURN(new_jsv);
}

static pid_t
jsv_get_pid(lListElem *jsv) {
   pid_t pid = -1;
   const char *pid_string = NULL;

   DENTER(TOP_LAYER, "jsv_get_pid");
   pid_string = lGetString(jsv, JSV_pid);
   if (pid_string != NULL) {
      sscanf(pid_string, pid_t_fmt, &pid);
   }
   DRETURN(pid);
}

static void 
jsv_set_pid(lListElem *jsv, pid_t pid)
{
   char pid_buffer[256]; /* it is sure that pid is smaller than 256 characters */

   DENTER(TOP_LAYER, "jsv_set_pid");
   sprintf(pid_buffer, pid_t_fmt, pid);
   lSetString(jsv, JSV_pid, pid_buffer);
   DRETURN_VOID;
}


static bool
jsv_is_started(lListElem *jsv) {
   return (jsv_get_pid(jsv) != -1) ? true : false; 
}

static bool
jsv_start(lListElem *jsv, lList **answer_list) {
   bool ret = true;

   DENTER(TOP_LAYER, "jsv_start");
   if (jsv != NULL && jsv_is_started(jsv) == false) {
      const char *scriptfile = lGetString(jsv, JSV_command);
      const char *user = lGetString(jsv, JSV_user);
      pid_t pid = -1;
      FILE *fp_in = NULL;
      FILE *fp_out = NULL;
      FILE *fp_err = NULL;
      SGE_STRUCT_STAT st;

      if (SGE_STAT(scriptfile, &st) == 0 && user != NULL) {
         /* set the last modification time of the script */
         lSetUlong(jsv, JSV_last_mod, st.st_mtime);

         pid = sge_peopen_threadsafe("/bin/sh", 0, scriptfile, user, NULL,
                                     &fp_in, &fp_out, &fp_err, false);

         if (pid != -1) {
            jsv_set_pid(jsv, pid);
            lSetRef(jsv, JSV_in, fp_in);
            lSetRef(jsv, JSV_out, fp_out);
            lSetRef(jsv, JSV_err, fp_err);

            /* we need it non blocking */
            fcntl(fileno(fp_out), F_SETFL, O_NONBLOCK);

            INFO((SGE_EVENT, "JSV process has been started"));
         } else {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR,
                                    "Unable to start JSV script "SFQ, scriptfile);
            ret = false;
         }
      } else {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR,
                                 "Unable to find JSV script "SFQ, scriptfile);
         ret = false;
      }
   }
   DRETURN(ret);
}

static bool
jsv_stop(lListElem *jsv, lList **answer_list, bool try_soft_quit) {
   bool ret = true;
   pid_t pid = -1;

   DENTER(TOP_LAYER, "jsv_stop");

   /* stop is only possible if it was started before */
   pid = jsv_get_pid(jsv);
   if (pid != -1) {
      struct timeval t;

      /* 
       * send a quit command to stop the JSV softly.
       * give it then JSV_QUIT_TIMEOUT seconds before
       * the kill is triggered (sge_peclose)
       */
      if (try_soft_quit) {
         jsv_send_command(jsv, answer_list, "QUIT");
         t.tv_sec = JSV_QUIT_TIMEOUT;
      }
  
      /*
       * kill the JSV process
       */ 
      sge_peclose(pid, lGetRef(jsv, JSV_in), 
                  lGetRef(jsv, JSV_out), lGetRef(jsv, JSV_err), 
                  (try_soft_quit ? &t : NULL));

      INFO((SGE_EVENT, "JSV process has been stopped"));

      jsv_set_pid(jsv, -1);
   }   
   DRETURN(ret);
}

static bool
jsv_is_send_ready(lListElem *jsv, lList **answer_list) {
   bool ret = true;
   const int timeout = 5;
   struct timeval timeleft;
   fd_set writefds;
   int fd;
   int lret;

   DENTER(TOP_LAYER, "jsv_is_send_ready");

   FD_ZERO(&writefds);
   fd = fileno((FILE *) lGetRef(jsv, JSV_in));
   FD_SET(fd, &writefds);
   timeleft.tv_sec = timeout;
   timeleft.tv_usec = 0;
   lret = select(fd + 1, NULL, &writefds, NULL, &timeleft);
   if (lret != -1 && FD_ISSET(fd, &writefds)) {
      ret = true;
      DPRINTF(("ready\n"));
   } else {
      ret = false; /* either not ready or broken pipe */
      DPRINTF(("not ready\n"));
   }
   DRETURN(ret);
}

static bool
jsv_send_data(lListElem *jsv, lList **answer_list, const char *buffer, size_t size) {
   bool ret = true;

   DENTER(TOP_LAYER, "jsv_send_data");

   if (jsv_is_send_ready(jsv, answer_list)) {
#if 0
      int lret = write(fileno((FILE *) lGetRef(jsv, JSV_in)), buffer, size); 
#else
      int lret = fprintf(lGetRef(jsv, JSV_in), buffer);
      fflush(lGetRef(jsv, JSV_in));
#endif

      if (lret != size) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR,
                                 "unable to send data to JSV script.");
         ret = false;
      }
   } else {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR,
                              "not ready to send to JSV script.");
      ret = false;
   } 

   DRETURN(ret);
}

bool
jsv_send_command(lListElem *jsv, lList **answer_list, const char *message) 
{
   bool ret = true;
   dstring buffer = DSTRING_INIT;
   const char *new_message = NULL;

   DENTER(TOP_LAYER, "jsv_send_command");
   sge_dstring_sprintf(&buffer, "%s\n", message);
   new_message = sge_dstring_get_string(&buffer);
   DPRINTF(("JSV(%s) >> %s\n", lGetString(jsv, JSV_context), message));
   ret = jsv_send_data(jsv, answer_list, new_message, strlen(new_message));
   sge_dstring_free(&buffer);
   DRETURN(ret);
}

bool
jsv_list_add(const char *name, const char *context, lList **answer_list, const char *jsv_url) 
{
   lListElem *new_jsv = NULL;
   dstring input = DSTRING_INIT;
   dstring type = DSTRING_INIT;
   dstring user = DSTRING_INIT;
   dstring path = DSTRING_INIT; 
   bool in_client = false;
   bool ret = true;

   DENTER(TOP_LAYER, "jsv_list_add");

   sge_dstring_append(&input, jsv_url);
   in_client = (strcmp(context, JSV_CONTEXT_CLIENT) == 0) ? true : false;
   jsv_url_parse(&input, answer_list, &type, &user, &path, in_client);

   new_jsv = jsv_create(name, context, answer_list, jsv_url, 
                        sge_dstring_get_string(&type),
                        sge_dstring_get_string(&user),
                        sge_dstring_get_string(&path));
   if (new_jsv == NULL) {
      ret = false;
   } 

   /* cleanup */
   sge_dstring_free(&input);
   sge_dstring_free(&type);
   sge_dstring_free(&user);
   sge_dstring_free(&path);

   DRETURN(ret);
}

bool
jsv_list_update(const char *name, const char *context,
                lList **answer_list, const char *jsv_url) 
{
   bool ret = false;

   DENTER(TOP_LAYER, "jsv_list_update");

   if (name != NULL && context != NULL) {
      bool already_exists = false;

      /*
       * Update the information stored in a JSV element. If the URL did
       * not change then it is not necessary do anything. But if something changed
       * then update the information and set a flag (JSV_has_to_restart)
       * that indicates that a JSV process which might be already running, is 
       * restarted before the next job verification is done. If the setting
       * is changed to none then stop the process and remove data structures.
       */
      {
         const void *iterator = NULL;
         lListElem *jsv = NULL;
         lListElem *jsv_next = NULL;
         bool not_parsed = true;
         bool use_old_url = (jsv_url == NULL) ? true : false;

         sge_mutex_lock("jsv_list", SGE_FUNC, __LINE__, &jsv_mutex);

         DTRACE;

         ret = true;
         jsv_next = lGetElemStrFirst(jsv_list, JSV_context, context, &iterator);
         while ((ret == true) && ((jsv = jsv_next) != NULL)) {
            dstring input = DSTRING_INIT;
            dstring type = DSTRING_INIT;
            dstring user = DSTRING_INIT;
            dstring path = DSTRING_INIT; 
            const char *old_jsv_url = NULL; 
            bool in_client = false;

            /* position pointer to the next element */
            jsv_next = lGetElemStrNext(jsv_list, JSV_context, context, &iterator);

            old_jsv_url = lGetString(jsv, JSV_url);
            if (use_old_url) {
               jsv_url = old_jsv_url;
            }

            /* 
             * stop the process if the configuration changed 
             * or if the file was modified 
             */
            if (strcmp(old_jsv_url, jsv_url) != 0) {
               DTRACE;

               /* 
                * did we get a JSV URL or "none" as new setting? 
                * depending on that either change the jsv settings and trigger a restart
                * or trigger termination of jsv process and remove the jsv cull structure
                */
               if (strcmp(jsv_url, "none") != 0) {
                  DTRACE;

                  if (not_parsed) {
                     sge_dstring_append(&input, jsv_url);
                     in_client = (strcmp(context, JSV_CONTEXT_CLIENT) == 0) ? true : false;
                     jsv_url_parse(&input, answer_list, &type, &user, &path, in_client);
                     not_parsed = false;
                  }

                  lSetString(jsv, JSV_type, sge_dstring_get_string(&type));
                  lSetString(jsv, JSV_user, sge_dstring_get_string(&user));
                  lSetString(jsv, JSV_command, sge_dstring_get_string(&path));
                  lSetString(jsv, JSV_url, jsv_url);
                  INFO((SGE_EVENT, "JSV setting changed (context = "SFQ")", context));
                  jsv_stop(jsv, answer_list, true);
               } else {
                  DTRACE;

                  jsv_stop(jsv, answer_list, true);
                  lRemoveElem(jsv_list, &jsv);
                  INFO((SGE_EVENT, "JSV process will be stopped. Functionality is disabled now (context = "SFQ")", context));
               }
            } else {
               SGE_STRUCT_STAT st;

               DTRACE;
   
               if (SGE_STAT(lGetString(jsv, JSV_command), &st) == 0 && 
                   lGetUlong(jsv, JSV_last_mod) != st.st_mtime) {
                  INFO((SGE_EVENT, "Modification time of JSV script changed (context = "SFQ")", context));
                  jsv_stop(jsv, answer_list, true);    
               }
            }

            DTRACE;

            /*
             * independent if the jsv was modified or deleted - don't create it below
             */
            already_exists = true;
   
            /* cleanup */
            sge_dstring_free(&input);
            sge_dstring_free(&type);
            sge_dstring_free(&user);
            sge_dstring_free(&path);
         }

         sge_mutex_unlock("jsv_list", SGE_FUNC, __LINE__, &jsv_mutex);
      }

      /*
       * create a new JSV element if no one exists
       */
      if (ret && !already_exists && jsv_url != NULL && strcmp(jsv_url, "none") != 0) {
         ret = jsv_list_add(name, context, answer_list, jsv_url);
      }
   }
   DRETURN(ret);
}

bool
jsv_do_verify(sge_gdi_ctx_class_t* ctx, const char *context, lListElem **job, lList **answer_list, bool hold_global_lock) 
{
   bool ret = true;
   lListElem *jsv;
   lListElem *jsv_next;
   const void *iterator = NULL;

   DENTER(TOP_LAYER, "jsv_do_verify");

   if (context != NULL && job != NULL) {
      const char *jsv_url = NULL;

      /*
       * Depending on the context either provide a NULL pointer to 
       * jsv_list_update or the current setting of the configuration.
       * In the client context this means that the registered jsv_url
       * will be used which was provided at command start via -jsv
       * in the server context the current setting of the jsv_url
       * will be used which is part of the global configuration.
       */
      if (strcmp(context, JSV_CONTEXT_CLIENT) == 0) {
         jsv_url = NULL;
      } else {
         jsv_url = mconf_get_jsv_url();
      }

      /*
       * update the list of JSV scripts for the current thread
       */
      jsv_list_update("jsv", context, answer_list, jsv_url);

      sge_mutex_lock("jsv_list", SGE_FUNC, __LINE__, &jsv_mutex);

      /*
       * execute JSV scripts for this thread
       */
      jsv_next = lGetElemStrFirst(jsv_list, JSV_context, context, &iterator);
      while ((ret == true) && ((jsv = jsv_next) != NULL)) {
         jsv_next = lGetElemStrNext(jsv_list, JSV_context, context, &iterator);

         if (jsv_is_started(jsv) == false) {
            ret &= jsv_start(jsv, answer_list);
         }
         if (ret) {
            lListElem *old_job = *job;
            lListElem *new_job = lCopyElem(*job);

            lSetBool(jsv, JSV_restart, false);
            lSetBool(jsv, JSV_accept, false);
            lSetBool(jsv, JSV_done, false);
   
            lSetRef(jsv, JSV_old_job, old_job);
            lSetRef(jsv, JSV_new_job, new_job);

            /* 
             * During the time the job is evaluated in JSV routines we can
             * release the global lock which was accquired outside. 
             */
            if (!hold_global_lock) {
               SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE)
            }
            ret &= jsv_do_communication(ctx, jsv, answer_list);
            if (!hold_global_lock) {
               SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE)
            }

            lSetRef(jsv, JSV_old_job, NULL); 
            lSetRef(jsv, JSV_new_job, NULL); 

            if (lGetBool(jsv, JSV_accept)) {
               lFreeElem(job);
               *job = new_job;
               new_job = NULL;
               ret = true;
            } else {
               u_long32 jid = lGetUlong(new_job, JB_job_number);

               if (jid == 0) {
                  INFO((SGE_EVENT, "JSV rejected job (context = "SFQ")", context));
               } else {
                  INFO((SGE_EVENT, "JSV rejected job "sge_u32" (context = "SFQ")", jid, context));
               }
               ret = false;
               lFreeElem(&new_job);
            }
            if (lGetBool(jsv, JSV_restart)) {
               bool soft_shutdown = lGetBool(jsv, JSV_soft_shutdown) ? true : false;

               INFO((SGE_EVENT, "JSV script restart due to protocol error (context = "SFQ")", context));
               ret &= jsv_stop(jsv, answer_list, soft_shutdown);
            }
         }
      }

      sge_mutex_unlock("jsv_list", SGE_FUNC, __LINE__, &jsv_mutex);
   }
   DRETURN(ret);
}

