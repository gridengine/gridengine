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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>

#include "sge_max_nis_retries.h"
#include "cull.h"
#include "commlib.h"
#include "commd_message_flags.h"
#include "sge_prognames.h"
#include "sge_gdi.h"
#include "sge_gdi_intern.h"
#include "sge_uidgid.h"
#include "sgermon.h"
#include "pack_job_delivery.h"
#include "sge_qexec.h"
#include "sge_qexecL.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_stringL.h"
#include "msg_gdilib.h"
#include "sge_security.h"

#include "jb_now.h"

static lList *remote_task_list = 0;
static char lasterror[1024];

/* option flags for rcv_from_execd() */
#define OPT_SYNCHRON 1

#define LOCATE_RTASK(tid) lGetElemUlong(remote_task_list, RT_tid, tid)

static char program_name[1024] = "libgdi.a";
static int rcv_from_execd(int options, int tag); 

const char *qexec_last_err(void)
{
   return lasterror;
}

/****** gdi/sge_qexecve() ************************************************
*  NAME
*     sge_qexecve() -- start a task in a tightly integrated parallel job
*
*  SYNOPSIS
*     sge_tid_t sge_qexecve(const char *hostname, const char *path, 
*                           const char *argv[], const lList *env_lp, 
*                           int is_qrsh) 
*
*  FUNCTION
*     Starts a task in a tightly integrated job.
*     Builds a job object describing the task, 
*     connects to the commd on the targeted execution host,
*     deliveres the job object and waits for an answer.
*     The answer from the execution daemon on the execution host
*     contains a task id that is returned to the caller of the function.
*
*  INPUTS
*     const char *hostname - name of the host on which to start the task
*     const char *path     - complete path of the command to start
*     const char *argv[]   - argument vector for the command to start
*     const lList *env_lp  - list containing environment variable settings
*                            for the task that override the default environment
*     int is_qrsh          - is the task to be started a qrsh -inherit task?
*                            0 means 0, != 0 means yes
*
*  RESULT
*     sge_tid_t - the task id, if the task can be executed,
*                 a value <= 0 indicates an error.
*
*******************************************************************************/
sge_tid_t sge_qexecve(const char *hostname, const char *path, const char *cwd,
                      const char *argv[], const lList *env_lp, 
                      int is_qrsh)
{
char localhost[1000];
char myname[256];
const char *s;
   int ret, i, uid;
   sge_tid_t tid = 0;
   lListElem *rt, *jep, *arg_ep, *env_ep, *jgelem=NULL, *taskep;
   lList *env, *arg_lp = NULL;
   sge_pack_buffer pb;
   u_long32 jobid;
   u_long32 dummymid;
   int no_ack = 0;

   DENTER(TOP_LAYER, "sge_qexecve");

   /* use local host if no hostname is given */
   if (!hostname) {
      if (gethostname(localhost, sizeof(localhost) - 1)) {
         sprintf(lasterror, MSG_GDI_RETRIEVINGLOCALHOSTNAMEFAILED_S , strerror(errno));
         DEXIT;
         return -1;
      }
      hostname = localhost;
   }

   /* resolve user */
   if (sge_uid2user((uid=getuid()), myname, sizeof(myname)-1, MAX_NIS_RETRIES)) {
      sprintf(lasterror, MSG_GDI_RESOLVINGUIDTOUSERNAMEFAILED_IS , 
            uid, strerror(errno));
      DEXIT;
      return -1;
   }
   
   {  
      lListElem *ep;
      if ( !(s=getenv("JOB_ID")) && 
      (!(ep=lGetElemStr(env_lp, VA_variable, "JOB_ID")) || !(s=lGetString(ep, VA_value)))) {
         sprintf(lasterror, MSG_GDI_MISSINGJOBIDENVIRONMENT );
         DEXIT;
         return -1;
      }
   }
   if (sscanf(s, u32, &jobid)!=1) {
      sprintf(lasterror, MSG_GDI_STRINGINJOBIDISINVALID_S , s);
      DEXIT;
      return -1;
   }

   /* ---- build up job structure (see gdilib/sge_jobL.h) */
   jep = lCreateElem(JB_Type);

   lSetUlong(jep, JB_job_number, jobid); 
   lSetString(jep, JB_owner, myname);

   if(cwd != NULL) {
      lSetString(jep, JB_cwd, cwd);
   }

   if(is_qrsh) {
      lSetUlong(jep, JB_now, JB_NOW_QRSH);
      lSetString(jep, JB_script_file, JB_NOW_STR_QRSH);
   } else {
      /* fill in absolute exec path to task executable */
      lSetString(jep, JB_script_file, path);

      /* build up task argument list */
      for (i = 1; argv[i]; i++) {
         if (!arg_lp) {
            arg_lp = lCreateList("task arguments", ST_Type);
            lSetList(jep, JB_job_args, arg_lp);
         }
         arg_ep = lCreateElem(ST_Type);
         lSetString(arg_ep, STR, argv[i]);
         lAppendElem(arg_lp, arg_ep);
      }
   }

   env = lCopyList("env_list", env_lp);

   /* user may propose a task id */
   if ((env_ep=lGetElemStr(env, VA_variable, OVERWRITE_TASK_ID_NAME))) {
      s = lGetString(env_ep, VA_value);
      /* user proposes a task id */
      if (!(tid=atoi(s))) {
         sprintf(lasterror, MSG_GDI_PROPOSEDTASKIDINVALID_SS , OVERWRITE_TASK_ID_NAME, s);
         DEXIT;
         return -1;
      }
      if (LOCATE_RTASK(tid)) {
         sprintf(lasterror, MSG_GDI_TASKEXISTS_S , s);
         DEXIT;
         return -1;
      }
   }

   /* user may overwrite the program name that is transfered to the execd */
   if ((env_ep=lGetElemStr(env, VA_variable, OVERWRITE_PROGRAM_NAME))) {
      strcpy(program_name, lGetString(env_ep, VA_value));
      lDelElemStr(&env, VA_variable, OVERWRITE_PROGRAM_NAME);
   }

   if ((env_ep=lGetElemStr(env, VA_variable, OVERWRITE_STDOUT))) {
      lAddSubStr(jep, PN_path, lGetString(env_ep, VA_value), JB_stdout_path_list, PN_Type);
      lDelElemStr(&env, VA_variable, OVERWRITE_STDOUT);
   } else 
      lAddSubStr(jep, PN_path, "/dev/null", JB_stdout_path_list, PN_Type);

   if ((env_ep=lGetElemStr(env, VA_variable, OVERWRITE_STDERR))) {
      lAddSubStr(jep, PN_path, lGetString(env_ep, VA_value), JB_stderr_path_list, PN_Type);
      lDelElemStr(&env, VA_variable, OVERWRITE_STDERR);
   } else 
      lAddSubStr(jep, PN_path, "/dev/null", JB_stderr_path_list, PN_Type);

   if ((env_ep=lGetElemStr(env, VA_variable, OVERWRITE_MERGE))) {
      lSetUlong(jep, JB_merge_stderr, strcasecmp(lGetString(env_ep, VA_value), "true"));
      lDelElemStr(&env, VA_variable, OVERWRITE_MERGE);
   }

   /* put an element into JB_ja_tasks list */
   {
      lListElem *tep;
      tep = lAddSubUlong(jep, JAT_task_number, 1, JB_ja_tasks, JAT_Type);
   }

   if ((env_ep=lGetElemStr(env, VA_variable, OVERWRITE_QUEUE))) {
      for_each(taskep, lGetList(jep, JB_ja_tasks)) {
         jgelem = lAddSubStr(taskep, JG_qname, lGetString(env_ep, VA_value), 
            JAT_granted_destin_identifier_list, JG_Type);
      }
      /* JG_qhostname is better to be filled in by execd */
      lSetUlong(jgelem, JG_slots, 1);
      lDelElemStr(&env, VA_variable, OVERWRITE_QUEUE);
   }

   if ((env_ep=lGetElemStr(env, VA_variable, OVERWRITE_NO_ACK))) {
      DPRINTF(("requested to get no ack\n"));
      no_ack = 1;
   }

   lSetList(jep, JB_env_list, env);
   env = NULL;

   set_commlib_param(CL_P_COMMDHOST, 0, hostname, NULL); 

   if(init_packbuffer(&pb, 0, 0) != PACK_SUCCESS) {
      lFreeElem(jep);
      lFreeList(env);
      sprintf(lasterror, MSG_GDI_OUTOFMEMORY);
      DEXIT;
      return -1;
   }

   pack_job_delivery(&pb, jep, NULL, NULL);

   ret = send_message_pb(1, prognames[EXECD], 0, hostname,
            TAG_JOB_EXECUTION, &pb, &dummymid);

   clear_packbuffer(&pb);

   lFreeElem(jep);
   lFreeList(env);

   if (ret) {
      sprintf(lasterror, MSG_GDI_SENDTASKTOEXECDFAILED_SS , hostname, cl_errstr(ret));
      DEXIT;
      return -1;
   }
  
   /* add list into our remote task list */
   rt = lAddElemUlong(&remote_task_list, RT_tid, tid, RT_Type);
   lSetString(rt, RT_hostname, hostname);

   if (no_ack) {
      lSetUlong(rt, RT_state, RT_STATE_EXITED);
      lSetUlong(rt, RT_status, 0); /* assume (hope) everything worked fine */
   } else 
      lSetUlong(rt, RT_state, RT_STATE_WAIT4ACK);

   if (!tid) { /* get it from execd */
      /* fprintf(stderr, "receiving tid\n"); fflush(stderr); */
      rcv_from_execd(OPT_SYNCHRON, TAG_JOB_EXECUTION);
      tid = lGetUlong(rt, RT_tid);
   }

   DEXIT;
   return tid;
}

sge_tid_t sge_qwaittid(
sge_tid_t tid,
int *status,
int options 
) {
   lListElem *rt = NULL;
   int ret, rcv_opt = 0;

   DENTER(TOP_LAYER, "sge_qwaittid");

   if (!(options&WNOHANG))
      rcv_opt |= OPT_SYNCHRON;

   if (tid && !(rt=LOCATE_RTASK(tid))) {
      sprintf(lasterror, MSG_GDI_TASKNOTEXIST_I , (int) tid);
      DEXIT;
      return -1;
   }


   while ((rt && /* definite one searched */
            lGetUlong(rt, RT_state)!=RT_STATE_EXITED && /* not exited */
            lGetUlong(rt, RT_state)==RT_STATE_WAIT4ACK) /* waiting for ack */
        || (!rt && /* anybody searched */
            !lGetElemUlong(remote_task_list, RT_state, RT_STATE_EXITED) && /* none exited */
            lGetElemUlong(remote_task_list, RT_state, RT_STATE_WAIT4ACK))) /* but one is waiting for ack */ {
      /* wait for incoming messeges about exited tasks */
      if ((ret=rcv_from_execd(rcv_opt, TAG_TASK_EXIT))) {
         DEXIT;
         return (ret<0)?-1:0;
      }
   }

   if (status)
      *status = lGetUlong(rt, RT_status);
   lSetUlong(rt, RT_state, RT_STATE_WAITED);

   DEXIT;
   return 0;
}

/* return 
   0  reaped a task cleanly  
   1  no message (asynchronuous mode)
   -1 got an error
*/
static int rcv_from_execd(
int options,
int tag 
) {
   int ret;
   char *msg = NULL;
   u_long32 msg_len = 0;
   sge_pack_buffer pb;
   u_short from_id;
   char host[1024];
   u_short compressed;

   lListElem *rt_rcv;
   char *task_id_as_str = NULL;
   u_long32 exit_status=0;
   u_long32 tid=0;

   DENTER(TOP_LAYER, "rcv_from_execd");

   host[0] = '\0';
   from_id = 1;
   do {
      /* FIX_CONST */
      if ((ret = receive_message((char*)prognames[EXECD], &from_id, host, 
            &tag, &msg, &msg_len, (options&OPT_SYNCHRON)?1:0, &compressed))!=0 
                  && ret!=COMMD_NACK_TIMEOUT) {
         sprintf(lasterror, MSG_GDI_MESSAGERECEIVEFAILED_SI , 
               cl_errstr(ret), ret);
         DEXIT;
         return -1;
      }
   } while (options&OPT_SYNCHRON && ret == COMMD_NACK_TIMEOUT);

   if (ret==COMMD_NACK_TIMEOUT) {
      DEXIT;
      return 1;
   }

   init_packbuffer_from_buffer(&pb, msg, msg_len, compressed);     

   switch (tag) {
   case TAG_TASK_EXIT:
      unpackstr(&pb, &task_id_as_str);
      unpackint(&pb, &exit_status);
      break;
   case TAG_JOB_EXECUTION:
      unpackint(&pb, &tid);
      break;
   default:
      break;
   }

   clear_packbuffer(&pb);

   switch (tag) {
   case TAG_TASK_EXIT:
      /* change state in exited task */
      if (!(rt_rcv = lGetElemUlong(remote_task_list, RT_tid, 
            atoi(task_id_as_str)))) {
         sprintf(lasterror, MSG_GDI_TASKNOTFOUND_S , 
               task_id_as_str);
         free(task_id_as_str);
         DEXIT;
         return -1;
      }

      free(task_id_as_str);

      lSetUlong(rt_rcv, RT_status, exit_status);
      lSetUlong(rt_rcv, RT_state, RT_STATE_EXITED);
      break;

   case TAG_JOB_EXECUTION:
      /* search task without taskid */
      if (!(rt_rcv = lGetElemUlong(remote_task_list, RT_tid, 0))) {
         sprintf(lasterror, MSG_GDI_TASKNOTFOUNDNOIDGIVEN_S , task_id_as_str);
         DEXIT;
         return -1;
      }
      lSetUlong(rt_rcv, RT_tid, tid);
      break;

   default:
      break;
   }

   DEXIT;
   return 0;
}
