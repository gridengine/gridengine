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
#include <fnmatch.h>

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/ToggleB.h>

#include <Xmt/Xmt.h>
#include <Xmt/Dialog.h>
#include <Xmt/Create.h>
#include <Xmt/Layout.h>
#include <Xmt/Converters.h>
#include <Xmt/Procedures.h>
#include <Xmt/WidgetType.h>
#include <Xmt/Include.h>
#include <Xmt/InputField.h>

#include "Matrix.h" 
#include "qmon_rmon.h"
#include "qmon_cull.h"
#include "qmon_jobcustom.h"
#include "qmon_job.h"
#include "qmon_util.h"
#include "qmon_request.h"
#include "qmon_comm.h"
#include "qmon_appres.h"
#include "qmon_widgets.h"
#include "qmon_matrix.h"
#include "sge.h"
#include "symbols.h"
#include "complex.h"
#include "sge_sched.h"      
#include "commlib.h"
#include "sge_gdi_intern.h"
#include "sge_time.h"
#include "sge_all_listsL.h"
#include "IconList.h"
#include "sge_feature.h"
#include "parse_range.h"
#include "sge_hash.h"
#include "qstat_util.h"
#include "job.h"
#include "qmon_preferences.h"
#include "qmon_message.h"
#include "utility.h"

/*-------------------------------------------------------------------------*/
/* Prototypes */
static void okCB(Widget w, XtPointer cld, XtPointer cad);
static void cancelCB(Widget w, XtPointer cld, XtPointer cad);
static void addToSelected(Widget w, XtPointer cld, XtPointer cad);
static void rmFromSelected(Widget w, XtPointer cld, XtPointer cad);

static String PrintUlong(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintDouble(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintPriority(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintString(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintTime(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintStartTime(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintBool(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintGrantedQueue(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintJobTaskId(lListElem *ep, lListElem *jat, lList *jal, int nm);
/* static String PrintJobId(lListElem *ep, lListElem *jat, */
/*                               lList *jal,  int nm); */
/* static String PrintTaskId(lListElem *ep, lListElem *jat, */
/*                               lList *jal,  int nm); */
static String PrintStatus(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintHold(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintMailOptions(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintPathList(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintCPU(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintMEM(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintIO(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintMailList(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintPERange(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintJobArgs(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintPredecessors(lListElem *ep, lListElem *jat, lList *jal, int nm);
static String PrintRestart(lListElem *ep, lListElem *jat, lList *jal, int nm);                           
static void SetJobLabels(Widget w);
static void qmonWhatSetItems(Widget list, int how);
static void qmonJobFilterSet(Widget w, XtPointer cld, XtPointer cad);
static void qmonJobFilterClear(Widget w, XtPointer cld, XtPointer cad);
static void qmonJobFilterEditResource(Widget w, XtPointer cld, XtPointer cad);
static void qmonJobFilterRemoveResource(Widget w, XtPointer cld, XtPointer cad);

static int is_owner_ok(lListElem *jep, lList *owner_list);
static int is_job_runnable_on_queues(lListElem *jep, lList *queue_list, lList *exec_host_list, lList *complex_list);
/*-------------------------------------------------------------------------*/
/* selectable items, the names must match the names in sge_jobL.h          */
/* there are fixed columns at the beginning of the natrix                  */
/*-------------------------------------------------------------------------*/

static HashTable JobColumnPrintHashTable = NULL;
static HashTable NameMappingHashTable = NULL;

#define FIRST_FIELD     6
#define SGE_FIELDS      13

static tJobField job_items[] = {
   { 1, JB_job_number, "@{Id}", 12, 20, PrintJobTaskId }, 
/*    { 1, JB_ja_tasks, "@{TaskId(s)}", 8, 20, PrintTaskId },  */
   { 1, JB_priority, "@{Priority}", 8, 20, PrintPriority },
   { 1, JB_job_name, "@{Name}", 10, 50, PrintString },
   { 1, JB_owner, "@{Owner}", 10, 50, PrintString },
   { 1, JAT_status, "@{Status}", 8, 30, PrintStatus },
   { 1, 0, "@{Queue}", 10, 500, PrintGrantedQueue},
   { 0, JB_job_file, "@{Script}", 10, 30, PrintString },
   { 0, JB_submission_time, "@{SubmitTime}", 12, 30, PrintTime },
   { 0, JAT_start_time, "@{StartTime}", 12, 30, PrintStartTime },
   { 0, JB_execution_time, "@{ScheduleTime}", 12, 30, PrintTime },
   { 0, JB_account, "@{AccountString}", 15, 50, PrintString },
   { 0, JB_cell, "@{Cell}", 10, 30, PrintString },
   { 0, JB_cwd, "@{CWD}", 10, 30, PrintString },
   { 0, JB_stderr_path_list, "@{StderrPaths}", 15, 100, PrintPathList },
   { 0, JAT_hold, "@{Hold}", 10, 30, PrintHold },
   { 0, JB_merge_stderr, "@{MergeOutput}", 11, 30, PrintBool },
   { 0, JB_mail_options, "@{MailOptions}", 11, 30, PrintMailOptions },
   { 0, JB_mail_list, "@{MailTo}", 15, 30, PrintMailList },
   { 0, JB_notify, "@{Notify}", 6, 30, PrintBool },
   { 0, JB_stdout_path_list, "@{StdoutPaths}", 15, 50, PrintPathList },
   { 0, JB_restart, "@{Restart}", 10, 30, PrintRestart },
   { 0, JB_job_args, "@{JobArgs}", 15, 30, PrintJobArgs},
   { 0, JB_pe, "@{PE}", 10, 30, PrintString },
   { 0, JB_pe_range, "@{PERange}", 15, 30, PrintPERange },
   { 0, JB_jid_predecessor_list, "@{Predecessors}", 12, 30, PrintPredecessors },
/**** SGE specific fields *****/
   { 0, JAT_ticket, "@{Ticket}", 10, 30, PrintUlong},
   { 0, JAT_oticket, "@{OTicket}", 10, 30, PrintUlong},
   { 0, JAT_dticket, "@{DTicket}", 10, 30, PrintUlong },
   { 0, JAT_fticket, "@{FTicket}", 10, 30, PrintUlong },
   { 0, JAT_sticket, "@{STicket}", 10, 30, PrintUlong },
   { 0, JAT_share, "@{Share}", 10, 30, PrintDouble },
   { 0, JB_override_tickets, "@{OverrideTickets}", 15, 30, PrintUlong },
   { 0, JB_project, "@{Project}", 10, 30, PrintString },
   { 0, JB_department, "@{Department}", 10, 30, PrintString },
   { 0, JB_deadline, "@{Deadline}", 10, 30, PrintTime },
   { 0, JAT_scaled_usage_list, "@{CPU}", 10, 30, PrintCPU },
   { 0, JAT_scaled_usage_list, "@{MEM}", 10, 30, PrintMEM },
   { 0, JAT_scaled_usage_list, "@{IO}", 10, 30, PrintIO },
};

/*
** this list restricts the selected jobs, queues displayed
*/
static lList *jobfilter_resources = NULL;
static lList *jobfilter_owners = NULL;
static lList *jobfilter_fields = NULL;
static int jobfilter_compact = 1;


static Widget jcu = 0;
static Widget jobfield = 0;
static Widget jobfilter_ar = 0;
static Widget jobfilter_sr = 0;
static Widget jobfilter_owner = 0;
static Widget jobfilter_arrays_compressed = 0;
static Widget jobfield_available = 0;
static Widget jobfield_selected = 0;

/*-------------------------------------------------------------------------*/
static String PrintUlong(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   char buf[BUFSIZ];
   String str;

   DENTER(GUI_LAYER, "PrintUlong");

   if (nm >= JAT_LOWERBOUND && nm <= JAT_UPPERBOUND) {
      jat = lFirst(lGetList(ep, JB_ja_tasks));
      sprintf(buf, "%d", (int)lGetUlong(jat, nm));
   }
   else
      sprintf(buf, "%d", (int)lGetUlong(ep, nm));

   str = XtNewString(buf);

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintDouble(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   char buf[BUFSIZ];
   String str;

   DENTER(GUI_LAYER, "PrintDouble");

   if (nm >= JAT_LOWERBOUND && nm <= JAT_UPPERBOUND) {
      jat = lFirst(lGetList(ep, JB_ja_tasks));
      sprintf(buf, "%f", lGetDouble(jat, nm));
   }
   else
      sprintf(buf, "%f", lGetDouble(ep, nm));

   str = XtNewString(buf);

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintBool(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   String str;

   DENTER(GUI_LAYER, "PrintBool");

   if (lGetUlong(ep, nm))
      str = XtNewString("True");
   else
      str = XtNewString("False");

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintJobArgs(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   String str;
   lList *job_args;
   lListElem *jep = NULL;
   char buf[ 10 * BUFSIZ];

   DENTER(GUI_LAYER, "PrintJobArgs");

   job_args = lGetList(ep, nm);
   
   strcpy(buf, "");
   for_each(jep, job_args) {
      sprintf(buf, "%s %s", buf, lGetString(jep, STR));
   }
   str = XtNewString(buf);

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintPredecessors(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   String str;
   lList *pred;
   lListElem *jep = NULL;
   char buf[ 10 * BUFSIZ];

   DENTER(GUI_LAYER, "PrintPredecessors");

   pred = lGetList(ep, nm);
   
   strcpy(buf, "");
   for_each(jep, pred) {
      sprintf(buf, "%s %d", buf, (int) lGetUlong(jep, JRE_job_number));
   }
   str = XtNewString(buf);

   DEXIT;
   return str;
}


/*-------------------------------------------------------------------------*/
static String PrintPERange(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   char buf[BUFSIZ];
   String str;

   DENTER(GUI_LAYER, "PrintPERange");

   show_ranges(buf, 0, NULL, lGetList(ep, nm));

   str = XtNewString(buf);

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintPathList(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   String str;
   lList *path_list = NULL;

   DENTER(GUI_LAYER, "PrintPathList");

   path_list = lGetList(ep, nm); 

   str = XtNewString(write_pair_list(path_list, PATH_TYPE));

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintCPU(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   String str;
   lListElem *up = NULL;
   char buf[1024];
   u_long32 running;

   DENTER(GUI_LAYER, "PrintCPU");

   if (!jat)
      jat = lFirst(jal);

   running = lGetUlong(jat, JAT_status)==JRUNNING ||
               lGetUlong(jat, JAT_status)==JTRANSITING;

   /* scaled cpu usage */
   if (!(up = lGetSubStr(jat, UA_name, USAGE_ATTR_CPU, JAT_scaled_usage_list)))
      sprintf(buf, "%-10.10s ", running?"NA":"");
   else {
      int secs, minutes, hours, days;

      secs = lGetDouble(up, UA_value);

      days    = secs/(60*60*24);
      secs   -= days*(60*60*24);

      hours   = secs/(60*60);
      secs   -= hours*(60*60);

      minutes = secs/60;
      secs   -= minutes*60;

      sprintf(buf, "%d:%2.2d:%2.2d:%2.2d", days, hours, minutes, secs);
   }

   str = XtNewString(buf);

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintMEM(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   String str;
   lListElem *up = NULL;
   char buf[1024];
   u_long32 running;

   DENTER(GUI_LAYER, "PrintMEM");

   if (!jat)
      jat = lFirst(jal);

   running = lGetUlong(jat, JAT_status)==JRUNNING ||
               lGetUlong(jat, JAT_status)==JTRANSITING;

   /* scaled mem usage */
   if (!(up = lGetSubStr(jat, UA_name, USAGE_ATTR_MEM,
      JAT_scaled_usage_list)))
      sprintf(buf, "%-7.7s", running?"NA":"");
   else
      sprintf(buf, "%-5.5f", lGetDouble(up, UA_value));

   str = XtNewString(buf);

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintIO(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   String str;
   lListElem *up = NULL;
   char buf[1024];
   u_long32 running;

   DENTER(GUI_LAYER, "PrintIO");

   if (!jat)
      jat = lFirst(jal);

   running = lGetUlong(jat, JAT_status)==JRUNNING ||
               lGetUlong(jat, JAT_status)==JTRANSITING;

   /* scaled io usage */
   if (!(up = lGetSubStr(jat, UA_name, USAGE_ATTR_IO,
      JAT_scaled_usage_list)))
      sprintf(buf, "%-7.7s", running?"NA":"");
   else
      sprintf(buf, "%-5.5f", lGetDouble(up, UA_value));

   str = XtNewString(buf);

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintMailList(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   String str;
   lList *mail_list = NULL;

   DENTER(GUI_LAYER, "PrintMailList");

   mail_list = lGetList(ep, nm); 

   str = XtNewString(write_pair_list(mail_list, MAIL_TYPE));

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintMailOptions(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   char buf[BUFSIZ];
   String str;
   u_long32 ca;
   static u_long32 mail_at[] = { MAIL_AT_BEGINNING, MAIL_AT_EXIT,
                                 MAIL_AT_ABORT, MAIL_AT_SUSPENSION};
   static char mailsym[] ="beas";
   int i;

   DENTER(GUI_LAYER, "PrintMailOptions");

   ca = lGetUlong(ep, nm);
   strcpy(buf, "");
   
   if (ca) {
      for (i=0; i<4; i++) {
         if (ca & mail_at[i])
            sprintf(buf, "%s%c", buf, mailsym[i]);
      }
   }
   else
      strcpy(buf, "None"); 
   
   str = XtNewString(buf);

   DEXIT;
   return str;
}
   
#if 0
/*-------------------------------------------------------------------------*/
static String PrintCheckpointAttr(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   char buf[BUFSIZ];
   String str;
   u_long32 ca;
   u_long32 interval;


   DENTER(GUI_LAYER, "PrintCheckpointAttr");

   ca = lGetUlong(ep, JB_checkpoint_attr);
   interval = lGetUlong(ep, JB_checkpoint_interval);

   strcpy(buf, "NOT AVAIL");
   
   str = XtNewString(buf);

   DEXIT;
   return str;
}
#endif
   
/*-------------------------------------------------------------------------*/
static String PrintGrantedQueue(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   String str = NULL;
   lList *ql = NULL;
   lListElem *qgep = NULL;
   int n, len;
   String queue;

   DENTER(GUI_LAYER, "PrintGrantedQueue");

   /*
   ** display the queue of the first task in the task list
   */

#if 0   
   if (jat) {
      n = 1;
      queue = lGetString(jat, JAT_master_queue);
   }
   else {
      n = 0;
   }
   
   if (n == 0) 
      str = XtNewString("*pending*");
   else
      str = XtNewString(queue);
#endif

   if (jal && !jat) {
      jat = lFirst(jal);
   }

   if (jat) {
      ql = lGetList(jat, JAT_granted_destin_identifier_list);
      n = lGetNumberOfElem(ql);
   }
   else {
      n = 0;
   }
   
   if (n == 0 && lGetUlong(jat, JAT_status) == JIDLE)
      str = XtNewString("*pending*");
   else
      str = XtNewString("-");

   if (n >= 1) {
      if (lGetString(ep, JB_pe)) {
         str = XtNewString("P:");
      }
      else
         str = XtNewString("");

      for_each(qgep, ql) {
         queue = lGetString(qgep, JG_qname);
         len = 1 + (str?strlen(str):0) + (queue?strlen(queue):0);
         str = XtRealloc((char*)str, len+1); 
         strcat(str, queue);
         strcat(str, " ");
      }
   }      

   DEXIT;
   return str;
}

#if 0
/*-------------------------------------------------------------------------*/
static String PrintTaskId(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   char buf[BUFSIZ];
   String str;

   DENTER(GUI_LAYER, "PrintTaskId");

   /*
   ** prepare task ids, if the job contains only one job array task the job id!    ** is sufficient
   */
   if (jat) {
      sprintf(buf, u32, lGetUlong(jat, JAT_task_number));
   }
   else if (jal) {
      buf[0] = '\0';
      get_taskrange_str(jal, buf);
   }

   str = XtNewString(buf);

   DEXIT;
   return str;
}
 
/*-------------------------------------------------------------------------*/
static String PrintJobId(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   char buf[BUFSIZ];
   String str;

   DENTER(GUI_LAYER, "PrintJobId");

   sprintf(buf, u32, lGetUlong(ep, JB_job_number));

   str = XtNewString(buf);

   DEXIT;
   return str;
}
#endif

/*-------------------------------------------------------------------------*/
static String PrintJobTaskId(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   char buf[BUFSIZ];
   char buf2[BUFSIZ];
   String str;

   DENTER(GUI_LAYER, "PrintJobTaskId");
   /*
   ** prepare task ids, if the job contains only one job array task the job id!    ** is sufficient
   */
   if (is_array(ep)) {
      if (jat) {
         sprintf(buf2, u32, lGetUlong(jat, JAT_task_number));
      }
      else if (jal) {
         buf2[0] = '\0';
         get_taskrange_str(jal, buf2);
      }
      sprintf(buf, u32 ".%s", lGetUlong(ep, JB_job_number), buf2);
   }
   else {
      sprintf(buf, u32 , lGetUlong(ep, JB_job_number));
   }

   DPRINTF(("PrintJobTaskId: %s\n", buf));

   str = XtNewString(buf);

   DEXIT;
   return str;
}
 
/*-------------------------------------------------------------------------*/
static String PrintStatus(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   lListElem *qep;
   lList *ql = NULL;
   int n;
   char buf[128] = "";
   String str;
   u_long32 tstate = 0;
   u_long32 tstatus = 0;

   DENTER(GUI_LAYER, "PrintStatus");

   if (jal && !jat)
      jat = lFirst(jal);

   if (jat) {
      ql = lGetList(jat, JAT_granted_destin_identifier_list);
      n = lGetNumberOfElem(ql);
   }
   else {
      n = 0;
   }
   
#if 1
   /* move status info into state info */
   tstatus = lGetUlong(jat, JAT_status);
   tstate = lGetUlong(jat, JAT_state);

   if (tstatus==JRUNNING) {
      tstate |= JRUNNING;
      tstate &= ~JTRANSITING;
   } else if (tstatus==JTRANSITING) {
      tstate |= JTRANSITING;
      tstate &= ~JRUNNING;
   } else if (tstatus==JFINISHED) {
      tstate |= JEXITING;
      tstate &= ~(JRUNNING|JTRANSITING);
   }

   /* check suspension of queue */
   if (n>0) {
      qep = lGetElemStr(qmonMirrorList(SGE_QUEUE_LIST), QU_qname, 
                           lGetString(lFirst(ql), JG_qname));
      if (qep && (lGetUlong(qep, QU_state) & (QSUSPENDED|QSUSPENDED_ON_SUBORDINATE|QCAL_SUSPENDED))) {
         tstate &= ~JRUNNING;                   /* unset bit JRUNNING */
         tstate |= JSUSPENDED_ON_SUBORDINATE;   /* set bit JSUSPENDED_ON_SUBORDINATE */
         lSetUlong(jat, JAT_state, tstate);
      }
   }
      
   if (lGetList(ep, JB_jid_predecessor_list) || lGetUlong(jat, JAT_hold)) {
      tstate |= JHELD;
   }

   if (lGetUlong(jat, JAT_job_restarted)) {
      tstate &= ~JWAITING;
      tstate |= JMIGRATING;
   }

   /* write states into string */
   sge_get_states(JB_job_number, buf, tstate);

#else
   if ((tstate & JRUNNING) == JRUNNING)
      strcpy(buf, "RUNNING");
   else if ((tstate & JTRANSITING) == JTRANSITING)
      strcpy(buf, "TRANSITING");
   else if ((tstate & JFINISHED) == JFINISHED)
      strcpy(buf, "FINISHED");
   else 
      strcpy(buf, "IDLE");

   if (lGetList(ep, JB_jid_predecessor_list) || 
       (jat &&  lGetUlong(jat, JAT_hold)) ||
       (jal && lGetUlong(lFirst(jal), JAT_hold))) {
      strcpy(buf, "HOLD");
   }
#endif

   str = XtNewString(buf);

   DEXIT;
   return str;
}
   
/*-------------------------------------------------------------------------*/
static String PrintHold(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   char buf[BUFSIZ];
   String str;
   int value = 0;
   char* hold_string[] = {"u", "o", "s"};
   int i;

   DENTER(GUI_LAYER, "PrintHold");

   if (jat)
      value = (int)lGetUlong(jat, nm);
   else if (jal)
      value = (int)lGetUlong(lFirst(jal), nm);
   strcpy(buf, "");
   for (i=0; i<3; i++) {
      if (value & (1<<i))
         strcat(buf, hold_string[i]);
   }

   str = XtNewString(buf);

   DEXIT;
   return str;
}
   
/*-------------------------------------------------------------------------*/
static String PrintPriority(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   char buf[BUFSIZ];
   String str;

   DENTER(GUI_LAYER, "PrintPriority");

   sprintf(buf, "%d", (int)lGetUlong(ep, nm) - BASE_PRIORITY);

   str = XtNewString(buf);

   DEXIT;
   return str;
}
   
/*-------------------------------------------------------------------------*/
static String PrintString(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {

   String str;
   String temp;
   
   DENTER(GUI_LAYER, "PrintString");

   temp = lGetString(ep, nm); 
   
   str = XtNewString(temp?temp:"");

   DEXIT;
   return str;
}
   
/*-------------------------------------------------------------------------*/
static String PrintTime(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {

   String str;
   
   DENTER(GUI_LAYER, "PrintTime");

   if (lGetUlong(ep, nm))
      str = XtNewString(sge_ctime(lGetUlong(ep, nm)));
   else
      str = XtNewString("");

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintRestart(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {
   String str;

   DENTER(GUI_LAYER, "PrintRestart");

   switch (lGetUlong(ep, nm)) {
      case 1:
         str = XtNewString("True");
         break;
      case 2:
         str = XtNewString("False");
         break;
      default:
         str = XtNewString("Depends on queue");
   }

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintStartTime(
lListElem *ep,
lListElem *jat,
lList *jal,
int nm 
) {

   String str;

   DENTER(GUI_LAYER, "PrintTime");

   if (jat && lGetUlong(jat, nm))
      str = XtNewString(sge_ctime(lGetUlong(jat, nm)));
   else
      str = XtNewString("");

   DEXIT;
   return str;
}
                                                                              
   
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
String* PrintJobField(
lListElem *ep,
lListElem *jat,
lList *jal,
int cols 
) {
   int i;
   String* row = NULL;
   int col;
   String entry;

   DENTER(GUI_LAYER, "PrintJobField");

   /*
   ** check col against XtNumber(job_items)
   ** if cols > XtNumber(job_items) we screw the whole thing up
   */

   row = (String*)XtMalloc(sizeof(String)*cols);

   if (ep) {
      for (i=0, col=0; row && i<XtNumber(job_items) && col<cols; i++) {
         if (job_items[i].show) {
            if (job_items[i].printJobField &&
               (entry = job_items[i].printJobField(ep, jat, jal, job_items[i].nm))) { 
               row[col] = entry;
            }
            else {
               row[col] = XtNewString("");
            }
            col++;
         }
      }
   }
   else {
      for (col=0; row && col<cols; col++) {
         row[col] = XtNewString("");
      }
   }

   DEXIT;
   return row;
}

/*-------------------------------------------------------------------------*/
static void okCB(w, cld, cad)
Widget w;
XtPointer cld,cad;
{
   int nr_fields = 0;
   int i, j;
   XmString *strlist;
   Widget run_m;
   Widget pen_m;
   Widget zombie_m;
   String owner_str = NULL;
   tJobField *job_item = NULL;
   
   DENTER(GUI_LAYER, "okCB");

   XtVaGetValues( jobfield_selected, 
                  XmNitemCount, &nr_fields, 
                  XmNitems, &strlist,
                  NULL);

   /*
   ** reset show flag
   */ 
   for (j=FIRST_FIELD; j<XtNumber(job_items); j++) {           
      job_items[j].show = 0; 
   }



   for (i=0; i<nr_fields; i++) {
      if (strlist[i]) {
         String text;
         XmStringGetLtoR(strlist[i], XmFONTLIST_DEFAULT_TAG, &text);
         if (HashTableLookup(NameMappingHashTable, 
                             (void*)XrmStringToQuark(text),
                             (void **) &job_item)) {
            job_item->show = 1;
         }
         XtFree(text);
         DPRINTF(("job_item[%d].name: <%s> show = %d\n",
                  i, job_items[i].name, job_items[i].show));
      }
   }        

   qmonJobReturnMatrix(&run_m, &pen_m, &zombie_m);
   SetJobLabels(run_m);
   SetJobLabels(pen_m);
   SetJobLabels(zombie_m);
   
   /*
   ** get the strings to do a wildmatch against
   */
   owner_str = XmtInputFieldGetString(jobfilter_owner);
   jobfilter_owners = lFreeList(jobfilter_owners);
   lString2List(owner_str, &jobfilter_owners, ST_Type, STR, NULL); 

   jobfilter_compact = XmToggleButtonGetState(jobfilter_arrays_compressed);

   updateJobList();

   xmui_unmanage(jcu);

   DEXIT;
}
   
/*-------------------------------------------------------------------------*/
static void cancelCB(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   DENTER(GUI_LAYER, "cancelCB");
   
   qmonWhatSetItems(jobfield_selected, FILL_SELECTED);
   xmui_unmanage(jcu);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void saveCB(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *lp = NULL;
   lList *alp = NULL;
   int j;

   DENTER(GUI_LAYER, "saveCB");
   
   okCB(w, NULL, NULL);

   if (!qmon_preferences)
      qmon_preferences = lCreateElem(PREF_Type);

   lSetList(qmon_preferences, PREF_job_filter_resources, 
                     lCopyList("", jobfilter_resources));
   lSetList(qmon_preferences, PREF_job_filter_owners, 
                     lCopyList("", jobfilter_owners));
   lSetUlong(qmon_preferences, PREF_job_filter_compact, jobfilter_compact);
   for (j=FIRST_FIELD; j<XtNumber(job_items); j++) {           
      if (job_items[j].show)
         lAddElemStr(&lp, STR, job_items[j].name, ST_Type); 
   }
   lSetList(qmon_preferences, PREF_job_filter_fields, lp);

   alp = qmonWritePreferences();

   qmonMessageBox(w, alp, 0);

   alp = lFreeList(alp);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void addToSelected(w, cld, cad)
Widget w;
XtPointer cld,cad;
{
   XmString *items;
   Cardinal itemCount;
   int i;

   DENTER(GUI_LAYER, "addToSelected");

   XtVaGetValues( jobfield_available,
                  XmNselectedItems, &items,
                  XmNselectedItemCount, &itemCount,
                  NULL);

   for (i=0; i<itemCount; i++) {
      if (! XmListItemExists(jobfield_selected, items[i]))
         XmListAddItem(jobfield_selected, items[i], 0); 
   }
                     
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void rmFromSelected(w, cld, cad)
Widget w;
XtPointer cld,cad;
{
   XmString *items;
   Cardinal itemCount;

   DENTER(GUI_LAYER, "rmFromSelected");

   XtVaGetValues( jobfield_selected,
                  XmNselectedItems, &items,
                  XmNselectedItemCount, &itemCount,
                  NULL);
   
   if (itemCount)
      XmListDeleteItems(jobfield_selected, items, itemCount);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void SetJobLabels(
Widget w 
) {
   int cols;
   int col;
   int i;
   String *labels = NULL;
   short *widths = NULL;
   int *max_lengths = NULL;
   int additional = 0;
   Screen *screen = XtScreen(w);

   DENTER(GUI_LAYER, "SetJobLabels");

   for (i=FIRST_FIELD; i<XtNumber(job_items); i++) {
      if (job_items[i].show)
         additional++;
   }

   XtVaGetValues( w,
                  XmNcolumns, &cols,
                  NULL);

   /*
   ** delete the additional columns
   */
   if (cols > FIRST_FIELD)
      XbaeMatrixDeleteColumns(w, FIRST_FIELD, cols - FIRST_FIELD);

   if (additional > 0) {
      labels = (String*)XtMalloc(sizeof(String)*additional);
      widths = (short*)XtMalloc(sizeof(short)*additional);
      max_lengths = (int*)XtMalloc(sizeof(int)*additional);
      for (i=FIRST_FIELD, col=0; i<XtNumber(job_items) && col<additional; i++) {
         if (job_items[i].show) {
            String str, s, category, tag, defaultstr, free_me = NULL;
            str = job_items[i].name; 
            /* if this string has a tag, localize it first */
            if ((str[0] == '@') && (str[1] == '{')) {
               s = XtNewString(str);
               free_me = s;
               s += 2;
               category = NULL;
               tag = NULL;
               defaultstr = s;
               while(*s) {
                  if (*s == '.' && !category) {
                     *s = '\0';
                     category = defaultstr;
                     defaultstr = s+1;
                  }
                  if (*s == '.' && !tag) {
                     *s = '\0';
                     tag = defaultstr;
                     defaultstr = s+1;
                  }
                  if (*s == '}') {
                     *s = '\0';
                     s++;
                     break;
                  }
                  s++;
               }
               if (!tag)
                  tag = defaultstr;
/*                if (!tag[0]) goto error; */
/*                if (category && !category[0]) goto error; */
               s = _XmtLocalize(screen, defaultstr, category, tag);
/* printf("--> '%s'\n", s);                  */
            }
            else {
               s = (String)str;
            }
    
            labels[col] = XtNewString(s);
            widths[col] = job_items[i].width;
            max_lengths[col] = job_items[i].max_length;
            if (free_me) XtFree(free_me);
            col++;
         } 
      }
      XbaeMatrixAddColumns(w,
                           FIRST_FIELD,
                           NULL,
                           labels,
                           widths,
                           max_lengths,
                           NULL,
                           NULL,
                           NULL,
                           additional);
      for(i=0; i<additional; i++) {
         XtFree(labels[i]);
      }   
      XtFree((char*)labels);
      XtFree((char*)widths);
      XtFree((char*)max_lengths);
   }
                           

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonWhatSetItems(
Widget list,
int how 
) {
/*    XmString *items; */
/*    Cardinal itemCount = 0; */
/*    int i, j; */
   int num_jobs, i;

   DENTER(GUI_LAYER, "qmonWhatSetItems");

   num_jobs = XtNumber(job_items);
   if (!feature_is_enabled(FEATURE_SGEEE))
      num_jobs -= SGE_FIELDS;

#if 0
   if (how == FILL_ALL)
      itemCount = num_jobs - FIRST_FIELD;
   else
      for (i=FIRST_FIELD; i<num_jobs; i++) {
         if (job_items[i].show)
            itemCount++;
      } 

   items = (XmString*)XtMalloc(sizeof(XmString)*itemCount);
   for (i=FIRST_FIELD,j=0; i<num_jobs && j<itemCount; i++) {
      if (how == FILL_ALL) {
         items[j] = XmtCreateLocalizedXmString(list, job_items[i].name); 
         j++;
      }
      else {
         if (job_items[i].show) {
            items[j] = XmtCreateLocalizedXmString(list, job_items[i].name);
            j++;
         }
      }
   }
         
   XtVaSetValues( list, 
                  XmNitems, items,
                  XmNitemCount, itemCount,
                  NULL);
   XmStringTableFree(items, itemCount);

#else   

   for (i=FIRST_FIELD; i<num_jobs; i++) {
      if (how == FILL_ALL) {
         DPRINTF(("XmListAddItemUniqueSorted: '%s'\n", job_items[i].name));
         XmListAddItemUniqueSorted(list, job_items[i].name);
      }
      else {
         if (job_items[i].show) {
            XmListAddItemUniqueSorted(list, job_items[i].name);
         }
      }
   }
         
#endif

   DEXIT;
}


/*-------------------------------------------------------------------------*/
void qmonPopupJCU(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   DENTER(TOP_LAYER, "qmonPopupJCU");

   if (!jcu)
      qmonCreateJCU(w, NULL);
   xmui_manage(jcu);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
void qmonCreateJCU(
Widget parent,
XtPointer cld 
) {
   Widget   jcu_ok, jcu_cancel, jcu_save, jcu_folder,
            jobfield_add, jobfield_remove, jobfilter_clear;
   Widget run_m;
   Widget pen_m;
   Widget zombie_m;
   int i;
   char buf[BUFSIZ * 10];
   int first_time = 1;
 
   DENTER(GUI_LAYER, "qmonCreateJCU");

   /*
   ** the resource file for qc_dialog_shell must contain 
   ** qc_dialog as unmanaged XmtLayout child otherwise
   ** it is managed by XmtBuildQueryDialog and managed again
   ** in qmonPopupJCU
   */
   jcu = XmtBuildQueryDialog(parent, "jcu_shell",
                           NULL, 0,
                           "jcu_folder", &jcu_folder,
                           "jobfield_selected", &jobfield_selected,
                           "jobfield_available", &jobfield_available,
                           "jobfield_add", &jobfield_add,
                           "jobfield_remove", &jobfield_remove,
                           "jcu_ok", &jcu_ok,
                           "jcu_cancel", &jcu_cancel,
                           "jcu_save", &jcu_save,
                           "jobfilter_ar", &jobfilter_ar,
                           "jobfilter_sr", &jobfilter_sr,
                           "jobfilter_owner", &jobfilter_owner,
                           "jobfilter_arrays_compressed", 
                                    &jobfilter_arrays_compressed,
                           "jobfilter_clear", &jobfilter_clear,
                           NULL);
   /*
   ** create JobColumnPrintHashTable
   */
   JobColumnPrintHashTable = HashTableCreate(5);
   for (i=0; i<sizeof(job_items)/sizeof(tJobField); i++) {
      HashTableStore(JobColumnPrintHashTable,
                     (void *)XrmStringToQuark(job_items[i].name),
                     (void *)&job_items[i]);
   }

   /*
   ** create NameMappingHashTable
   */
   NameMappingHashTable = HashTableCreate(5);
   for (i=0; i<sizeof(job_items)/sizeof(tJobField); i++) {
      String text;
      XmString xstr = XmtCreateLocalizedXmString(jcu, job_items[i].name);
      XmStringGetLtoR(xstr, XmFONTLIST_DEFAULT_TAG, &text);
      HashTableStore(NameMappingHashTable,
                     (void *) XrmStringToQuark(text),
                     (void *)&job_items[i]);
      XmStringFree(xstr);
      XtFree(text);
   }

   /*
   ** reference to preferences
   */
   if (qmon_preferences) {
      lListElem *field;
      lListElem *ep;
      tJobField *job_item = NULL;

      jobfilter_resources = lCopyList("", lGetList(qmon_preferences, 
                                          PREF_job_filter_resources));
      jobfilter_owners = lCopyList("", lGetList(qmon_preferences, 
                                          PREF_job_filter_owners));
      jobfilter_compact = lGetUlong(qmon_preferences, PREF_job_filter_compact);
      jobfilter_fields = lCopyList("", lGetList(qmon_preferences,
                                          PREF_job_filter_fields));

/* lWriteListTo(jobfilter_resources, stdout); */
/* lWriteListTo(jobfilter_owners, stdout); */
/* lWriteListTo(jobfilter_fields, stdout); */


      /*
      ** preset the owners
      */
      strcpy(buf, "");
      for_each(ep, jobfilter_owners) {
         if (first_time) {
            first_time = 0;
            strcpy(buf, lGetString(ep, STR));
         }
         else
            sprintf(buf, "%s,%s", buf, lGetString(ep, STR));
      }
      XmtInputFieldSetString(jobfilter_owner, buf);


      /*
      ** set the fields which shall be shown
      */
      for_each(field, jobfilter_fields) {
         if (HashTableLookup(JobColumnPrintHashTable, 
             (void*)XrmStringToQuark(lGetString(field, STR)),
                             (void **) &job_item)) {
            job_item->show = 1;
         }
      }
   }

   /*
   ** preset the resources 
   */
   qmonJobFilterSet(jcu, NULL, NULL);

   /*
   ** preset the fields
   */
   qmonWhatSetItems(jobfield_available, FILL_ALL); 
   qmonWhatSetItems(jobfield_selected, FILL_SELECTED); 

   /*
   ** preset the column labels
   */
   qmonJobReturnMatrix(&run_m, &pen_m, &zombie_m);
   SetJobLabels(run_m);
   SetJobLabels(pen_m);
   SetJobLabels(zombie_m);
   
                           
   XtAddCallback(jcu_ok, XmNactivateCallback,
                        okCB, NULL);
   XtAddCallback(jcu_cancel, XmNactivateCallback,
                        cancelCB, NULL);
   XtAddCallback(jcu_save, XmNactivateCallback,
                        saveCB, NULL);
   XtAddCallback(jobfield_add, XmNactivateCallback,
                        addToSelected, (XtPointer) jobfield);
   XtAddCallback(jobfield_remove, XmNactivateCallback,
                        rmFromSelected, (XtPointer) jobfield);

   /*
   ** jobfilter callbacks
   */
   XtAddCallback(jobfilter_ar, XmNactivateCallback,
                        qmonJobFilterEditResource, (XtPointer)0);
   XtAddCallback(jobfilter_sr, XmNactivateCallback,
                        qmonJobFilterEditResource, (XtPointer)1);
   XtAddCallback(jobfilter_sr, XmNremoveCallback, 
                     qmonJobFilterRemoveResource, NULL);
   XtAddCallback(jobfilter_clear, XmNactivateCallback,
                        qmonJobFilterClear, NULL);
   XtAddCallback(jcu_folder, XmNvalueChangedCallback, 
                     qmonJobFilterSet, NULL);

   XmToggleButtonSetState(jobfilter_arrays_compressed, jobfilter_compact, False);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonJobFilterClear(w, cld, cad)
Widget w;
XtPointer cld, cad;
{


   DENTER(GUI_LAYER, "qmonJobFilterClear");

   jobfilter_resources = lFreeList(jobfilter_resources);
   qmonRequestDraw(jobfilter_sr, jobfilter_resources, 1);

   jobfilter_owners = lFreeList(jobfilter_owners);
   XmtInputFieldSetString(jobfilter_owner, "");
   
   XmToggleButtonSetState(jobfilter_arrays_compressed, 1, False);

   DEXIT;

}

/*-------------------------------------------------------------------------*/
static void qmonJobFilterSet(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *arl = NULL;
   lListElem *ep = NULL;
   lListElem *rp = NULL;

   DENTER(GUI_LAYER, "qmonJobFilterSet");

   arl = qmonGetResources(qmonMirrorList(SGE_COMPLEX_LIST), ALL_RESOURCES);

   for_each (ep, jobfilter_resources) {
      rp = lGetElemStr(arl, CE_name, lGetString(ep, CE_name));
      if (!rp)
         rp = lGetElemStr(arl, CE_shortcut, lGetString(ep, CE_name));
      if (rp) {
         lSetString(ep, CE_name, lGetString(rp, CE_name));
         lSetUlong(ep, CE_valtype, lGetUlong(rp, CE_valtype));
      }
   }
      
   /*
   ** do the drawing
   */
   qmonRequestDraw(jobfilter_ar, arl, 0);
   qmonRequestDraw(jobfilter_sr, jobfilter_resources, 1);

   arl = lFreeList(arl);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonJobFilterEditResource(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   XmIconListCallbackStruct *cbs = (XmIconListCallbackStruct*) cad;
   long how = (long)cld;
   lList *arl = NULL;
   int type;
   char stringval[MAXHOSTLEN];
   Boolean status = False;
   String name, value, strval;
   Boolean found = False;
   lListElem *fill_in_request = NULL;
   

   DENTER(GUI_LAYER, "qmonJobFilterEditResource");

   arl = qmonGetResources(qmonMirrorList(SGE_COMPLEX_LIST), ALL_RESOURCES);


   if (!how) {
      fill_in_request = lGetElemStr(arl, CE_name, cbs->element->string[0]);
   }
   else {
      for_each (fill_in_request, jobfilter_resources) {
         name = lGetString(fill_in_request, CE_name);
         value = lGetString(fill_in_request, CE_stringval);
         if (cbs->element->string[0] && name && 
            !strcmp(cbs->element->string[0], name) &&
            cbs->element->string[2] && value &&
            !strcmp(cbs->element->string[2], value) ) {
               found = True;
               break;
         }
      }       
            
      if (!found) {
         fill_in_request = NULL; 
      }
   }

   if (!fill_in_request) {
      DEXIT;
      return;
   }

   type = lGetUlong(fill_in_request, CE_valtype);
   strval = lGetString(fill_in_request, CE_stringval);
   if (strval)
      strncpy(stringval, strval, MAXHOSTLEN-1);
   else
      strcpy(stringval, "");

   status = qmonRequestInput(w, type, cbs->element->string[0], 
                              stringval, sizeof(stringval));
   /* 
   ** put the value in the CE_Type elem 
   */
   if (status) {
      lSetString(fill_in_request, CE_stringval, stringval);
    
      /* put it in the hard or soft resource list if necessary */
      if (!how) {
         if (!jobfilter_resources) {
            jobfilter_resources = lCreateList("jobfilter_resources", CE_Type);
         }
         if (!lGetElemStr(jobfilter_resources, CE_name, cbs->element->string[0]))
            lAppendElem(jobfilter_resources, lCopyElem(fill_in_request));
      }
      qmonRequestDraw(jobfilter_sr, jobfilter_resources, 1);
   }
   arl = lFreeList(arl);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonJobFilterRemoveResource(w, cld, cad)
Widget w;
XtPointer cld, cad;
{

   XmIconListCallbackStruct *cbs = (XmIconListCallbackStruct*) cad;
   lListElem *dep = NULL;
   Boolean found = False;
   String name, value;

   DENTER(GUI_LAYER, "qmonJobFilterRemoveResource");

   if (jobfilter_resources) {
      for_each (dep, jobfilter_resources) {
         name = lGetString(dep, CE_name);
         value = lGetString(dep, CE_stringval);
         if (cbs->element->string[0] && name && 
            !strcmp(cbs->element->string[0], name) &&
            cbs->element->string[2] && value &&
            !strcmp(cbs->element->string[2], value) ) {
               found = True;
               break;
         }
      }       
            
      if (found) {
         lRemoveElem(jobfilter_resources, dep);
         if (lGetNumberOfElem(jobfilter_resources) == 0)
            jobfilter_resources = lFreeList(jobfilter_resources);
         qmonRequestDraw(jobfilter_sr, jobfilter_resources, 1);
      }
   }
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
lList* qmonJobFilterResources(void)
{
   return jobfilter_resources;
}

/*-------------------------------------------------------------------------*/
lList* qmonJobFilterOwners(void)
{
   return jobfilter_owners;
}

/*-------------------------------------------------------------------------*/
int qmonJobFilterArraysCompressed(void)
{
   return jobfilter_compact;
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/* this function destroys the delivered queue list partially               */
/*-------------------------------------------------------------------------*/
int match_queue(
lList **queue_list,
lList *request_list,
lList *complex_list,
lList *exec_host_list 
) {
   lListElem *qep;
   lListElem *dep;
   lList *ce;

   DENTER(GUI_LAYER, "match_queue");

   if (!exec_host_list) {
      DPRINTF(("empty exec_host_list\n"));
      DEXIT;
      return False;
   } 

/* lWriteListTo(request_list, stderr); */

   /*
   ** remove the queues not fulfilling the request_list
   */
   qep = lFirst(*queue_list);
   while (qep) {
      lList *ccl[3];
      lListElem *ep;

      DPRINTF(("QUEUE %s\n", lGetString(qep, QU_qname)));
      ccl[0] = lGetList(lGetElemStr(exec_host_list, EH_name, "global"), EH_consumable_config_list);
      ccl[1] = (ep=lGetElemHost(exec_host_list, EH_name, lGetString(qep, QU_qhostname)))?
               lGetList(ep, EH_consumable_config_list):NULL;
      ccl[2] = lGetList(qep, QU_consumable_config_list);

      ce = NULL;
      set_qs_state(QS_STATE_EMPTY);
      queue_complexes2scheduler(&ce, qep, exec_host_list, complex_list, 0);
      set_qs_state(QS_STATE_FULL);
      if (!sge_select_queue(ce, request_list, 1, NULL, 0, 1, ccl)) {
         dep = qep;
         qep = lNext(qep);
         lRemoveElem(*queue_list, dep);
      }
      else
         qep = lNext(qep);
      ce = lFreeList(ce);
   }
   /*
   ** remove the template queue if present
   */
   lDelElemStr(queue_list, QU_qname, SGE_TEMPLATE_NAME);

   /*
   ** fill in requests
   */
   for_each(dep, request_list) {
      if (sge_fill_requests(lGetList(dep, RE_entries), complex_list, 0, 1, 0)) {
         DPRINTF(("failure in sge_fill_requests()\n"));
         DEXIT;
         return False;
      }
   } 

/* lWriteListTo(request_list, stderr); */

   /*
   ** remove the queues not fulfilling the request_list
   */
   qep = lFirst(*queue_list);
   while (qep) {
      lList *ccl[3];
      lListElem *ep;

      ccl[0] = lGetList(lGetElemStr(exec_host_list, EH_name, "global"), EH_consumable_config_list);
      ccl[1] = (ep=lGetElemHost(exec_host_list, EH_name, lGetString(qep, QU_qhostname)))?
               lGetList(ep, EH_consumable_config_list):NULL;
      ccl[2] = lGetList(qep, QU_consumable_config_list);

      ce = NULL;
      set_qs_state(QS_STATE_EMPTY);
      queue_complexes2scheduler(&ce, qep, exec_host_list, complex_list, 0);
      set_qs_state(QS_STATE_FULL);
      
      if (!sge_select_queue(ce, request_list, 1, NULL, 0, 1, ccl)) {
         dep = qep;
         qep = lNext(qep);
         lRemoveElem(*queue_list, dep);
      }
      else
         qep = lNext(qep);
      
      ce = lFreeList(ce);
   }

   if (lGetNumberOfElem(*queue_list) == 0)
      *queue_list = lFreeList(*queue_list);

   DEXIT;
   return True;
}
         

/*-------------------------------------------------------------------------*/
/* this function destroys the delivered job list partially               */
/*-------------------------------------------------------------------------*/
int match_job(
lList **job_list,
lList *owner_list,
lList *queue_list, 
lList *complex_list,
lList *exec_host_list,
lList *request_list 
) {
   lListElem *jep;
   lListElem *dep;
   int show;

   DENTER(GUI_LAYER, "match_job");

   jep = lFirst(*job_list);
   while (jep) {
      /*
      ** first of all we assume that the job should be displayed
      */
      show = 1;
      
      /*
      ** check if job fulfills user_list and set show flag
      */
      if (show && owner_list)
         show = is_owner_ok(jep, owner_list);

      /*
      ** is job runnable on queues fulfilling requests, dito
      */
      if (show && request_list) 
         show = is_job_runnable_on_queues(jep, queue_list, exec_host_list,
                                             complex_list);

      if (show) {
         jep = lNext(jep);
      }
      else {
         dep = jep;
         jep = lNext(jep);
         lRemoveElem(*job_list, dep);
      }
   } 

   if (lGetNumberOfElem(*job_list) == 0)
      *job_list = lFreeList(*job_list);

   DEXIT;
   return True;
}
         
/*-------------------------------------------------------------------------*/
static int is_owner_ok(
lListElem *jep,
lList *owner_list 
) {
   lListElem *op;

   if (!owner_list)
      return True;

   for_each(op, owner_list) {
      if (!fnmatch(lGetString(op, STR), lGetString(jep, JB_owner), 0)) {
         return True;
      }
   }
   return False;
}

/*-------------------------------------------------------------------------*/
static int is_job_runnable_on_queues(
lListElem *jep,
lList *queue_list,
lList *exec_host_list,
lList *complex_list 
) {
   lListElem *qep;   
   lList *ce;
   Boolean was_not_in_loop = True;

   /*
   ** if the job has no requests all queues fit
   */
   if (!lGetList(jep, JB_hard_resource_list)) {
      return True;
   }

   /*
   ** see if queues fulfill the request_list of the job
   */
   for_each(qep, queue_list) {
      lList *ccl[3];
      lListElem *ep;
      ccl[0] = lGetList(lGetElemStr(exec_host_list, EH_name, "global"), EH_consumable_config_list);
      ccl[1] = (ep=lGetElemHost(exec_host_list, EH_name, lGetString(qep, QU_qhostname)))?
               lGetList(ep, EH_consumable_config_list):NULL;
      ccl[2] = lGetList(qep, QU_consumable_config_list);

      ce = NULL;
      queue_complexes2scheduler(&ce, qep, exec_host_list, complex_list, 0);
      if (sge_select_queue(ce, lGetList(jep, JB_hard_resource_list), 1, 
                                       NULL, 0, 1, ccl)) {
         ce = lFreeList(ce);
         return True;
      } 
      ce = lFreeList(ce);
      was_not_in_loop = False;
   } 
   
   return was_not_in_loop;
}
