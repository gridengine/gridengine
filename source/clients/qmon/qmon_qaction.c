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
#include <ctype.h>

#include <X11/IntrinsicP.h>

#include <Xm/Xm.h>
#include <Xm/List.h>

#include <Xmt/Xmt.h>
#include <Xmt/Hash.h>
#include <Xmt/Create.h>
#include <Xmt/Chooser.h>
#include <Xmt/Layout.h>
#include <Xmt/Dialog.h>
#include <Xmt/Dialogs.h>
#include <Xmt/MsgLine.h>
#include <Xmt/InputField.h>
#include <Xmt/Procedures.h>


/*----------------------------------------------------------------------------*/
#include "sge_all_listsL.h"
#include "Matrix.h"
#include "Tab.h"
#include "commlib.h"
#include "sge_parse_num_par.h"
#include "sge_complex_schedd.h"
#include "qmon_proto.h"
#include "qmon_rmon.h"
#include "qmon_util.h"
#include "qmon_cull.h"
#include "qmon_qaction.h"
#include "qmon_comm.h"
#include "qmon_widgets.h"
#include "qmon_load.h"
#include "qmon_quarks.h"
#include "qmon_timer.h"
#include "qmon_message.h"
#include "qmon_init.h"
#include "qmon_manop.h"
#include "qmon_queue.h"
#include "qmon_cplx.h"
#include "qmon_globals.h"
#include "qmon_project.h"
#include "AskForTime.h"
#include "AskForItems.h"
#include "sge_host.h"
#include "sge_feature.h"
#include "sge_answer.h"
#include "sge_cqueue.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_qinstance.h"

/*-------------------------------------------------------------------------*/

/* 
 * typedefs for all the resources connected to the dialogs 
 * additional typedefs for transfer cull <--> structs
 */

/* queue configuration parameters -> gdilib/sge_queueL.h */

typedef struct _tQCEntry {
/*---- general queue config -----*/
   char qname[100];
   char qhostname[100];
   int  qtype;
   char processors[256];
   int  priority;
   int  job_slots;
   int  rerun;
   char tmpdir[256];
   char calendar[256];
   char shell[256];
   int  shell_start_mode;
   char notify[20];
   int  seq_no;
   int initial_state;
/*---- methods -----*/
   const char *prolog;
   const char *epilog;
   const char *starter_method;
   const char *suspend_method;
   const char *resume_method;
   const char *terminate_method;
/*---- checkpointing -----*/
   char min_cpu_interval[20];
   lList *ckpt_list;
/*---- pe -----*/
   lList *pe_list;
/*---- load thresholds ----*/
   lList* load_thresholds;
/*---- suspend thresholds ----*/
   lList* suspend_thresholds;
   char suspend_interval[20];
   int    nsuspend;
/*---- queue limits  ----*/
   lList* limits_hard;
   lList* limits_soft;
   /*
   char s_rt[20];
   char h_rt[20];
   char s_cpu[20];
   char h_cpu[20];
   char s_fsize[20];
   char h_fsize[20];
   char s_data[20];
   char h_data[20];
   char s_stack[20];
   char h_stack[20];
   char s_core[20];
   char h_core[20];
   char s_rss[20];
   char h_rss[20];
   char s_vmem[20];
   char h_vmem[20];
   */
/*---- complexes  ----*/
   lList *complexes;
   lList *consumable_config_list;
/*---- user access ---*/
   lList *acl;
   lList *xacl;
/*---- project access ---*/
   lList *prj;
   lList *xprj;
/*---- owner list ----*/
   lList *owner_list;
/*---- subordinate list ----*/
   lList *subordinates;
}  tQCEntry;

   
typedef struct _tAction {
   String label;
   XtCallbackProc callback;
} tAction;

enum {
   QC_ADD,
   QC_MODIFY,
   QC_DELETE
};


/*---- queue configuration -----*/
XtResource qc_resources[] = {
   { "qname", "qname", XmtRBuffer, 
      XmtSizeOf(tQCEntry, qname), 
      XtOffsetOf(tQCEntry, qname[0]), 
      XtRImmediate, NULL },

   { "qhostname", "qhostname", XmtRBuffer, 
      XmtSizeOf(tQCEntry, qhostname), 
      XtOffsetOf(tQCEntry, qhostname[0]), 
      XtRImmediate, NULL },

   { "qtype", "qtype", XtRInt,
      sizeof(int), XtOffsetOf(tQCEntry, qtype),
      XtRImmediate, NULL },
      
   { "processors", "processors", XmtRBuffer, 
      XmtSizeOf(tQCEntry, processors), 
      XtOffsetOf(tQCEntry, processors[0]), 
      XtRImmediate, NULL },
      
   { "priority", "priority", XtRInt,
      sizeof(int), XtOffsetOf(tQCEntry, priority),
      XtRImmediate, NULL },
   
   { "job_slots", "job_slots", XtRInt,
      sizeof(int), XtOffsetOf(tQCEntry, job_slots),
      XtRImmediate, NULL },
      
   { "rerun", "rerun", XtRInt,
      sizeof(int), XtOffsetOf(tQCEntry, rerun),
      XtRImmediate, NULL },
      
   { "tmpdir", "tmpdir", XmtRBuffer, 
      XmtSizeOf(tQCEntry, tmpdir), 
      XtOffsetOf(tQCEntry, tmpdir[0]), 
      XtRImmediate, NULL },
      
   { "calendar", "calendar", XmtRBuffer, 
      XmtSizeOf(tQCEntry, calendar), 
      XtOffsetOf(tQCEntry, calendar[0]), 
      XtRImmediate, NULL },
      
   { "shell", "shell", XmtRBuffer, 
      XmtSizeOf(tQCEntry, shell), 
      XtOffsetOf(tQCEntry, shell[0]), 
      XtRImmediate, NULL },
      
   { "notify", "notify", XmtRBuffer, 
      XmtSizeOf(tQCEntry, notify), 
      XtOffsetOf(tQCEntry, notify[0]), 
      XtRImmediate, NULL },

   { "seq_no", "seq_no", XtRInt,
      sizeof(int), XtOffsetOf(tQCEntry, seq_no),
      XtRInt, 0 },

   { "initial_state", "initial_state", XtRInt,
      sizeof(int), XtOffsetOf(tQCEntry, initial_state),
      XtRInt, 0 },

   { "shell_start_mode", "shell_start_mode", XtRInt,
      sizeof(int), XtOffsetOf(tQCEntry, shell_start_mode),
      XtRInt, 0 },

/*---- methods -----*/
   { "prolog", "prolog", XtRString,
      sizeof(char *), XtOffsetOf(tQCEntry, prolog),
      XtRImmediate, NULL },

   { "epilog", "epilog", XtRString,
      sizeof(char *), XtOffsetOf(tQCEntry, epilog),
      XtRImmediate, NULL },

   { "starter_method", "starter_method", XtRString,
      sizeof(char *), XtOffsetOf(tQCEntry, starter_method),
      XtRImmediate, NULL },

   { "suspend_method", "suspend_method", XtRString,
      sizeof(char *), XtOffsetOf(tQCEntry, suspend_method),
      XtRImmediate, NULL },

   { "resume_method", "resume_method", XtRString,
      sizeof(char *), XtOffsetOf(tQCEntry, resume_method),
      XtRImmediate, NULL },

   { "terminate_method", "terminate_method", XtRString,
      sizeof(char *), XtOffsetOf(tQCEntry, terminate_method),
      XtRImmediate, NULL },

/*---- checkpointing -----*/
   { "min_cpu_interval", "min_cpu_interval", XmtRBuffer,
      XmtSizeOf(tQCEntry, min_cpu_interval),
      XtOffsetOf(tQCEntry, min_cpu_interval[0]),
      XtRImmediate, NULL },

   { "ckpt_list", "ckpt_list", QmonRSTR_Type,
      sizeof(lList *),
      XtOffsetOf(tQCEntry, ckpt_list),
      XtRImmediate, NULL },

/*---- pe -----*/
   { "pe_list", "pe_list", QmonRSTR_Type,
      sizeof(lList *),
      XtOffsetOf(tQCEntry, pe_list),
      XtRImmediate, NULL },
   
/*---- load thresholds ----*/
   { "load_thresholds", "load_thresholds", QmonRCE2_Type,
      sizeof(lList *),
      XtOffsetOf(tQCEntry, load_thresholds),
      XtRImmediate, NULL },

/*---- suspend thresholds ----*/
   { "suspend_thresholds", "suspend_thresholds", QmonRCE2_Type,
      sizeof(lList *),
      XtOffsetOf(tQCEntry, suspend_thresholds),
      XtRImmediate, NULL },

   { "suspend_interval", "suspend_interval", XmtRBuffer,
      XmtSizeOf(tQCEntry, suspend_interval), 
      XtOffsetOf(tQCEntry, suspend_interval[0]),
      XtRImmediate, NULL },

   { "nsuspend", "nsuspend", XtRInt,
      sizeof(int), XtOffsetOf(tQCEntry, nsuspend),
      XtRImmediate, NULL },

/*---- queue limits  ----*/
   { "limits_hard", "limits_hard", QmonRVA_Type,
      sizeof(lList *),
      XtOffsetOf(tQCEntry, limits_hard),
      XtRImmediate, NULL },

   { "limits_soft", "limits_soft", QmonRVA_Type,
      sizeof(lList *),
      XtOffsetOf(tQCEntry, limits_soft),
      XtRImmediate, NULL },
      
/*---- complexes ----*/
   { "complex_list", "complex_list", QmonRCX_Type,
      sizeof(lList *),
      XtOffsetOf(tQCEntry, complexes),
      XtRImmediate, NULL },
      
   { "consumable_config_list", "consumable_config_list", QmonRCE2_Type,
      sizeof(lList *),
      XtOffsetOf(tQCEntry, consumable_config_list),
      XtRImmediate, NULL },
      
/*---- subordinates ----*/
   { "subordinate_list", "subordinate_list", QmonRSO_Type,
      sizeof(lList *),
      XtOffsetOf(tQCEntry, subordinates),
      XtRImmediate, NULL },
      
/*---- access  ----*/
   { "acl", "acl", QmonRUS_Type,
      sizeof(lList *),
      XtOffsetOf(tQCEntry, acl),
      XtRImmediate, NULL },
   
   { "xacl", "xacl", QmonRUS_Type,
      sizeof(lList *),
      XtOffsetOf(tQCEntry, xacl),
      XtRImmediate, NULL },
      
/*---- projects  ----*/
   { "prj", "prj", QmonRUP_Type,
      sizeof(lList *),
      XtOffsetOf(tQCEntry, prj),
      XtRImmediate, NULL },
   
   { "xprj", "xprj", QmonRUP_Type,
      sizeof(lList *),
      XtOffsetOf(tQCEntry, xprj),
      XtRImmediate, NULL },
      
/*---- owners  ----*/
   { "owner_list", "owner_list", QmonRUS_Type,
      sizeof(lList *),
      XtOffsetOf(tQCEntry, owner_list),
      XtRImmediate, NULL }
      
};



/*-------------------------------------------------------------------------*/
static Widget qmonQCCreate(Widget parent);
static void qmonQCClone(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCResetAll(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCAction(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCToggleAction(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCAccessToggle(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCAccessAdd(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCAccessRemove(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCProjectToggle(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCProjectAdd(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCProjectRemove(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCOwnerAdd(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCCheckHost(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCCheckName(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCUpdate(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCCalendar(Widget w, XtPointer cld, XtPointer cad);

static void qmonQCTime(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCLimitInput(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCLimitNoEdit(Widget w, XtPointer cld, XtPointer cad); 
static void qmonQCLimitCheck(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCSOQ(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCAskForPE(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCAskForCkpt(Widget w, XtPointer cld, XtPointer cad);
static Boolean check_qname(char *name);
static void qmonInitQCEntry(tQCEntry *data);
static void qmonQCSetData(tQCEntry *data, StringConst qname, int how);
static Boolean qmonCullToQC(lListElem *qep, tQCEntry *data, int how);
static Boolean qmonQCToCull(tQCEntry *data, lListElem *qep);

static void qmonQCAdd(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCModify(Widget w, XtPointer cld, XtPointer cad);
static void qmonQCDelete(Widget w, XtPointer cld, XtPointer cad);

static void updateQCA(void);
static void updateQCP(void);

/*-------------------------------------------------------------------------*/
static Widget qc_dialog = 0;
static Widget qc_title = 0;
static Widget qc_subdialogs = 0;
static Widget qc_apply = 0;
static Widget qc_reset = 0;
static Widget qc_folder = 0;
static Widget qc_qname = 0;
static Widget qc_qhostname = 0;
static Widget subordinates_attached = 0;
static Widget access_list = 0;
static Widget access_allow = 0;
static Widget access_deny = 0;
static Widget access_toggle = 0;
static Widget project_list = 0;
static Widget project_allow = 0;
static Widget project_deny = 0;
static Widget project_toggle = 0;
static Widget owner_list = 0;
static Widget owner_new = 0;
static Widget ckpt_reference_list = 0;
static Widget pe_reference_list = 0;

static int dialog_mode = QC_ADD;
static int dont_close = 0;

static tAction actions[] = {
   { "Add", qmonQCAdd },
   { "Modify", qmonQCModify },
   { "Delete", qmonQCDelete }
};
   
/*
**  hold the tQCEntry data
*/
static tQCEntry current_entry; 
static tQCEntry temp_entry; 

/*
** this depends on the order of the sub dialogs in the
** option menu button !!!
*/
   
enum {
   QC_ALMOST = -2,
   QC_ALL,
   QC_GENERAL,
   QC_CHECKPOINT,
   QC_PE,
   QC_LIMIT,
   QC_LOADTHRESHOLD, 
   QC_COMPLEXES,
   QC_USERSET,
   QC_PROJECT,
   QC_OWNER,
   QC_SUBORDINATES,
   QC_SUSPENDTHRESHOLD 
};





/*-------------------------------------------------------------------------*/
/* P U B L I C                                                             */ 
/*-------------------------------------------------------------------------*/
void qmonQCPopup(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget layout;
   XmString title;
   lList *ql = NULL;
   
   DENTER(TOP_LAYER, "qmonQCPopup");

   /*
   ** popup the dialog in the specified mode
   */
   dialog_mode = cld ? QC_MODIFY: QC_ADD;
   ql = (lList*)cld;

   /* 
   ** set busy cursor 
   */
   XmtDisplayBusyCursor(w);

   /* 
   ** create the dialog if necessary 
   */
   if (!qc_dialog) {
      layout = XmtGetTopLevelShell(w);
      qc_dialog = qmonQCCreate(layout);
      /*
      ** initialize current entry & temp emtry to zero
      */
      qmonInitQCEntry(&current_entry);
      qmonInitQCEntry(&temp_entry);
   }

   /*
   ** set the dialog title
   */
   title = XmtCreateLocalizedXmString(qc_dialog, dialog_mode == QC_ADD ? 
                              "@{@fBQueue Configuration - Add}" :
                              "@{@fBQueue Configuration - Modify}");
   XtVaSetValues( qc_dialog,
                  XmNdialogTitle, title,
                  NULL);
   XtVaSetValues( qc_title,
                  XmtNlabelString, title,
                  NULL);
   XmStringFree(title);

   /*
   ** we open the qconf dialog with the template queue displayed 
   ** the Apply ad Reset buttons are set insensitive
   */
   if (!ql) 
      qmonQCSetData(&current_entry, "template", QC_ALL);
   else {
      const char *qname = lGetString(lFirst(ql), QU_qname);
      qmonQCSetData(&current_entry, qname, QC_ALL);
   }
      
   XmtDialogSetDialogValues(qc_dialog, &current_entry);
   if (dialog_mode == QC_ADD)
      XtSetSensitive(qc_apply, False);
   qmonQCUpdate(w, NULL, NULL);
   
   /*
   ** display the correct dialog
   */
   qmonQCToggleAction(qc_dialog, NULL, NULL);
   
   /* 
   ** pop the dialog up, Motif 1.1 gets confused if we don't popup the shell 
   */
   XtManageChild(qc_dialog);

   /* 
   **  stop the polling of lists totally
   */
/*    qmonStopPolling(); */

   /* set normal cursor */
   XmtDisplayDefaultCursor(w);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
void qmonQCPopdown(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   
   DENTER(TOP_LAYER, "qmonQCPopdown");

   if (qc_dialog && XtIsManaged(qc_dialog)) {
   
      XtUnmanageChild(qc_dialog);
      XtPopdown(XtParent(qc_dialog));

      /*
      ** Sync the display
      */
      XSync(XtDisplay(qc_dialog), 0);
      XmUpdateDisplay(qc_dialog);

      /*
      ** restart polling of lists
      */ 
/*       qmonStartPolling(AppContext); */

      /*
      ** force update, we need this workaround for updating the Q Control
      */
      updateQueueListCB(w, NULL, NULL);

   }
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
/* P R I V A T E                                                           */
/*-------------------------------------------------------------------------*/
static Widget qmonQCCreate(
Widget parent 
) {

   Widget dialog;
   Widget qc_cancel, qc_clone, qc_update, qc_main_link;
   Widget qtype, notify, notifyPB, calendar, calendarPB;
   Widget min_cpu_intervalPB, min_cpu_interval;
   Widget load_thresholds, load_delete;
   Widget suspend_thresholds, suspend_delete,
          suspend_interval, suspend_intervalPB;
   Widget limits_hard, limits_soft;
   Widget complexes_dialog, complexes_ccl, consumable_delete;
   Widget access_add, access_remove, access_dialog;
   Widget project_add, project_remove, project_dialog, project_config;
   Widget owner_add, owner_remove;
   Widget pe_reference_listPB, ckpt_reference_listPB;

   DENTER(TOP_LAYER, "qmonQCCreate");

   /*
   ** the resource file for qc_dialog_shell must contain 
   ** qc_dialog as unmanaged XmtLayout child otherwise
   ** it is managed by XmtBuildQueryDialog and managed again
   ** in qmonQCPopup
   */
   dialog = XmtBuildQueryDialog(parent, "qc_dialog_shell",
                              qc_resources, XtNumber(qc_resources),
                              "qc_folder", &qc_folder,
                              "qc_qname", &qc_qname,
                              "qc_qhostname", &qc_qhostname,
                              "qc_reset", &qc_reset,
                              "qc_apply", &qc_apply,
                              "qc_clone", &qc_clone,
                              "qc_cancel", &qc_cancel,
                              "qc_update", &qc_update,
                              "qc_main_link", &qc_main_link,
                              "qc_title", &qc_title,
                              /* General Configuration */
                              "qtype", &qtype,
                              "notify", &notify,
                              "notifyPB", &notifyPB,
                              "calendar", &calendar,
                              "calendarPB", &calendarPB,
                              /* checkpoint_config */
                              "min_cpu_intervalPB", &min_cpu_intervalPB,
                              "min_cpu_interval", &min_cpu_interval,
                              "ckpt_reference_listPB", &ckpt_reference_listPB,
                              "ckpt_reference_list", &ckpt_reference_list,
                              /* pe config */
                              "pe_reference_listPB", &pe_reference_listPB,
                              "pe_reference_list", &pe_reference_list,
                              /* load_threshold_config */
                              "load_thresholds", &load_thresholds,
                              "load_delete", &load_delete,
                              /* suspend_threshold_config */
                              "suspend_thresholds", &suspend_thresholds,
                              "suspend_delete", &suspend_delete,
                              "suspend_interval", &suspend_interval,
                              "suspend_intervalPB", &suspend_intervalPB,
                              /* limit_config */ 
                              "limits_hard", &limits_hard,
                              "limits_soft", &limits_soft,
                              /* complexes_config */
                              "complexes_dialog", &complexes_dialog,
                              "consumable_delete", &consumable_delete,
                              "complexes_ccl", &complexes_ccl,
                              /* subordinates_config */
                              "subordinates_attached", &subordinates_attached,
                              /* access_config */
                              "access_list", &access_list,
                              "access_allow", &access_allow,
                              "access_deny", &access_deny,
                              "access_add", &access_add,
                              "access_remove", &access_remove,
                              "access_toggle", &access_toggle,
                              "access_dialog", &access_dialog,
                              /* project_config */
                              "project_list", &project_list,
                              "project_allow", &project_allow,
                              "project_deny", &project_deny,
                              "project_add", &project_add,
                              "project_remove", &project_remove,
                              "project_toggle", &project_toggle,
                              "project_dialog", &project_dialog,
                              "project_config", &project_config,
                              /* owner_config */
                              "owner_new", &owner_new,
                              "owner_add", &owner_add,
                              "owner_remove", &owner_remove,
                              "owner_list", &owner_list,
                              NULL); 


   if (!feature_is_enabled(FEATURE_SGEEE)) {
/*       XtUnmanageChild(project_config); */
         XmTabDeleteFolder(qc_folder, project_config);
   }
   /*
   ** callbacks
   */
   XtAddCallback(qc_main_link, XmNactivateCallback, 
                  qmonMainControlRaise, NULL); 
   XtAddCallback(qc_apply, XmNactivateCallback, 
                  qmonQCAction, NULL); 
   XtAddCallback(qc_clone, XmNactivateCallback, 
                  qmonQCClone, NULL); 
   XtAddCallback(qc_update, XmNactivateCallback, 
                  qmonQCUpdate, NULL); 

   XtAddCallback(qc_cancel, XmNactivateCallback, 
                  qmonQCPopdown, NULL);
   XtAddCallback(qc_reset, XmNactivateCallback, 
                  qmonQCResetAll, NULL);

   XtAddCallback(qc_qhostname, XmtNverifyCallback, 
                     qmonQCCheckHost, NULL);
   XtAddCallback(qc_qname, XmNvalueChangedCallback, 
                     qmonQCCheckName, NULL);
   

   /* 
   ** General Config
   */
   XtAddCallback(notifyPB, XmNactivateCallback, 
                     qmonQCTime, (XtPointer)notify);
   XtAddCallback(calendarPB, XmNactivateCallback, 
                     qmonQCCalendar, (XtPointer)calendar);


   /*
   ** Load Thresholds
   */
#if 0   
   XtAddCallback(load_delete, XmNactivateCallback,
                  qmonLoadDelLines, (XtPointer) load_thresholds); 
   XtAddCallback(load_thresholds, XmNenterCellCallback,
                  qmonLoadNoEdit, NULL);
   XtAddCallback(load_thresholds, XmNselectCellCallback,
                  qmonLoadSelectEntry, NULL);
#endif
   XtAddCallback(load_thresholds, XmNlabelActivateCallback,
                  qmonLoadNames, NULL);

   /*
   ** Suspend Thresholds
   */
#if 0
   XtAddCallback(suspend_delete, XmNactivateCallback,
                  qmonLoadDelLines, (XtPointer) suspend_thresholds); 
   XtAddCallback(suspend_thresholds, XmNenterCellCallback,
                  qmonLoadNoEdit, NULL);
   XtAddCallback(suspend_thresholds, XmNselectCellCallback,
                  qmonLoadSelectEntry, NULL);
#endif
   XtAddCallback(suspend_thresholds, XmNlabelActivateCallback,
                  qmonLoadNames, NULL);
   XtAddCallback(suspend_intervalPB, XmNactivateCallback,
                  qmonQCTime, (XtPointer) suspend_interval); 

   /*
   ** Checkpointing
   */
   XtAddCallback(min_cpu_intervalPB, XmNactivateCallback,
                  qmonQCTime, (XtPointer) min_cpu_interval); 
   XtAddCallback(ckpt_reference_listPB, XmNactivateCallback,
                  qmonQCAskForCkpt, NULL); 

   /*
   ** PE
   */
   XtAddCallback(pe_reference_listPB, XmNactivateCallback,
                  qmonQCAskForPE, NULL); 


   /*
   ** Complexes & Consumables
   */
#if 0
   XtAddCallback(consumable_delete, XmNactivateCallback,
                  qmonLoadDelLines, (XtPointer) complexes_ccl); 
   XtAddCallback(complexes_ccl, XmNenterCellCallback,
                  qmonLoadNoEdit, NULL);
   XtAddCallback(complexes_ccl, XmNselectCellCallback,
                  qmonLoadSelectEntry, NULL);
#endif
   XtAddCallback(complexes_ccl, XmNlabelActivateCallback,
                  qmonLoadNames, NULL);

   /*
   ** Limits
   */
   XtAddCallback(limits_hard, XmNdefaultActionCallback, 
                     qmonQCLimitInput, NULL);
   XtAddCallback(limits_hard, XmNleaveCellCallback, 
                     qmonQCLimitCheck, NULL);
   XtAddCallback(limits_hard, XmNenterCellCallback, 
                     qmonQCLimitNoEdit, NULL);
   XtAddCallback(limits_soft, XmNdefaultActionCallback, 
                     qmonQCLimitInput, NULL);
   XtAddCallback(limits_soft, XmNleaveCellCallback, 
                     qmonQCLimitCheck, NULL);
   XtAddCallback(limits_soft, XmNenterCellCallback, 
                     qmonQCLimitNoEdit, NULL);


   /*
   ** Subordinates
   */
   XtAddCallback(subordinates_attached, XmNdefaultActionCallback, 
                     qmonQCSOQ, NULL);

   /*
   ** Access & Xacess
   */
   XtAddCallback(access_toggle, XmtNvalueChangedCallback, 
                     qmonQCAccessToggle, NULL);
   XtAddCallback(access_add, XmNactivateCallback, 
                     qmonQCAccessAdd, NULL);
   XtAddCallback(access_remove, XmNactivateCallback, 
                     qmonQCAccessRemove, NULL);
   XtAddCallback(access_dialog, XmNactivateCallback, 
                     qmonPopupManopConfig, (XtPointer)2);

   if (feature_is_enabled(FEATURE_SGEEE)) {
      /*
      ** Project & Xproject
      */
      XtAddCallback(project_toggle, XmtNvalueChangedCallback, 
                        qmonQCProjectToggle, NULL);
      XtAddCallback(project_add, XmNactivateCallback, 
                        qmonQCProjectAdd, NULL);
      XtAddCallback(project_remove, XmNactivateCallback, 
                        qmonQCProjectRemove, NULL);
      XtAddCallback(project_dialog, XmNactivateCallback, 
                        qmonPopupProjectConfig, NULL);
   }

   /*
   ** Owner
   */
#if 0
   XtAddCallback(owner_add, XmNactivateCallback, 
                     qmonQCOwnerAdd, NULL);
   XtAddCallback(owner_remove, XmNactivateCallback, 
                     DeleteItems, (XtPointer) owner_list);
#endif
   XtAddCallback(owner_new, XmNinputCallback, 
                     qmonQCOwnerAdd, NULL);
                              


#if 0
   /* 
   ** start the needed timers and the corresponding update routines 
   */
   XtAddCallback(XtParent(dialog), XmNpopupCallback, 
                     qmonQCStartUpdate, NULL);
   XtAddCallback(XtParent(dialog), XmNpopdownCallback,
                     qmonQCStopUpdate, NULL);
                     
#endif

   XtAddEventHandler(XtParent(dialog), StructureNotifyMask, False, 
                        SetMinShellSize, NULL);
   XtAddEventHandler(XtParent(dialog), StructureNotifyMask, False, 
                        SetMaxShellSize, NULL);

   DEXIT;
   return dialog;
}


/*-------------------------------------------------------------------------*/
static void qmonQCUpdate(w, cld, cad)
Widget w;
XtPointer cld, cad; 
{
   lList *alp = NULL;
   DENTER(GUI_LAYER, "qmonQCUpdate");
   
   qmonMirrorMultiAnswer(USERSET_T | PROJECT_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }
   updateQCA();
   updateQCP();

   DEXIT;
}



/*-------------------------------------------------------------------------*/
static void qmonQCAction(w, cld, cad)
Widget w;
XtPointer cld, cad; 
{
   
   DENTER(GUI_LAYER, "qmonQCAction");
   
   actions[dialog_mode].callback(w, NULL, NULL);

   if (!dont_close)
      qmonQCPopdown(w, NULL, NULL);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonQCToggleAction(w, cld, cad)
Widget w;
XtPointer cld, cad; 
{
   DENTER(GUI_LAYER, "qmonQCToggleAction");

   /*
   ** set the qc_apply button label and the corresponding callback
   */
   switch (dialog_mode) {
      case QC_ADD:
         /* manage the layout */
/*          if (!XtIsManaged(qc_layout)) */
/*             XtManageChild(qc_layout); */
         XtSetSensitive(qc_qname, True);
         XtSetSensitive(qc_qhostname, True);
         XtVaSetValues( qc_qname,
                        XmNeditable, True,
                        NULL);
         XtVaSetValues( qc_qhostname,
                        XmNeditable, True,
                        NULL);
         break;
      case QC_MODIFY:
         /* manage the layout */
/*          if (!XtIsManaged(qc_layout)) */
/*             XtManageChild(qc_layout); */
         XtSetSensitive(qc_qname, True);
         XtSetSensitive(qc_qhostname, True);
         XtVaSetValues( qc_qname,
                        XmNeditable, False,
                        NULL);
         XtVaSetValues( qc_qhostname,
                        XmNeditable, False,
                        NULL);
         break;
      case QC_DELETE:
         /* manage the layout */
/*          if (XtIsManaged(qc_layout)) */
/*             XtUnmanageChild(qc_layout); */
         XtSetSensitive(qc_qname, False);
         XtSetSensitive(qc_qhostname, False);
         break;
   }

   DEXIT;
}   


/*-------------------------------------------------------------------------*/
static void qmonQCClone(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Boolean status = False;
   lList *ql = NULL;
   lListElem *qep = NULL;
   int n, i;
   StringConst *strs = NULL;
   static char buf[BUFSIZ];
   lList *alp = NULL;
   
   DENTER(GUI_LAYER, "qmonQCClone");
   
   qmonMirrorMultiAnswer(QUEUE_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }

   /* EB: TODO: */
   ql = qmonMirrorList(SGE_CQUEUE_LIST);
   n = lGetNumberOfElem(ql);
   if (n>0) {
      strs = (StringConst*)XtMalloc(sizeof(String)*n); 
      for (qep=lFirst(ql), i=0; i<n; qep=lNext(qep), i++) {
        /*
        ** we get only references don't free, the strings
        */
        strs[i] = lGetString(qep, QU_qname);
      }
    
      strcpy(buf, "");
      /* FIX_CONST_GUI */
      status = XmtAskForItem(w, NULL, "@{Get Queue Attributes from}",
                        "@{Available Queues}", (String*)strs, n,
                        False, buf, BUFSIZ, NULL); 
      
      if (status) {
         XmtDialogGetDialogValues(qc_dialog, &current_entry);
         qmonQCSetData(&current_entry, buf, QC_ALMOST);
         XmtDialogSetDialogValues(qc_dialog, &current_entry);
      }
      /*
      ** don't free referenced strings, they are in the queue list
      */
      XtFree((char*)strs);
   }
   else
      qmonMessageShow(w, True, "@{No queues to clone}");
   
   DEXIT;
}



/*-------------------------------------------------------------------------*/
static void qmonQCCheckHost(w, cld, cad)
Widget w;
XtPointer cld, cad; 
{
   XmtInputFieldCallbackStruct *cbs = (XmtInputFieldCallbackStruct*)cad;
   static char unique[MAXHOSTLEN];
   int ret;
   String name = NULL;
   
   DENTER(GUI_LAYER, "qmonQCCheckHost");

   name = XmtInputFieldGetString(qc_qname);

   if (cbs->input && cbs->input[0]!='\0' && name && strcmp(name, "template")) {
      ret = sge_resolve_hostname((char*)cbs->input, unique, EH_name);
      switch ( ret ) {
         case COMMD_NACK_UNKNOWN_HOST:
/*             cbs->okay = False; */
/*             qmonMessageShow(w, True, "Can't resolve host '%s'\n",  */
/*                               cbs->input);   */
            break;
         case CL_OK:
            cbs->input = unique;
         default:
            DPRINTF(("sge_resolve_hostname() failed resolving: %s\n",
               cl_errstr(ret)));
      }
   }

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonQCCheckName(w, cld, cad)
Widget w;
XtPointer cld, cad; 
{
   DENTER(GUI_LAYER, "qmonQCCheckName");

   XtSetSensitive(qc_apply, True);
   
   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonQCAdd(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
  
   lList *lp = NULL;
   lList *alp = NULL;
   lEnumeration *what;
   
   DENTER(TOP_LAYER, "qmonQCAdd");

   what = lWhat("%T(ALL)", QU_Type);
   
   if (!(lp = lCreateElemList("AQ", QU_Type, 1))) {
      fprintf(stderr, "lCreateElemList failed\n");
      DEXIT;
      return;
   }

   XmtDialogGetDialogValues(qc_dialog, &current_entry);

   
   if (current_entry.qname[0] == '\0' || !check_qname(current_entry.qname)) {
      qmonMessageShow(w, True, "@{No valid Queue name specified}");
      XmtChooserSetState(qc_subdialogs, 0, False);
      XmProcessTraversal(qc_qname, XmTRAVERSE_CURRENT);
      DEXIT;
      return;
   }
  

   if (lFirst(lp) && qmonQCToCull(&current_entry, lFirst(lp))) {
      /* EB: TODO: */
      alp = qmonAddList(SGE_CQUEUE_LIST, qmonMirrorListRef(SGE_CQUEUE_LIST), 
                        QU_qname, &lp, NULL, what);
      
      qmonMessageBox(w, alp, 0);
      
      if ( lFirst(alp) && lGetUlong(lFirst(alp), AN_status) == STATUS_OK ) {
/*          updateQCQ(); */
         updateQueueList();
         /*
         ** reset dialog 
         */
         qmonQCSetData(&current_entry, "template", QC_ALL);
         XmtDialogSetDialogValues(qc_dialog, &current_entry);
         if (dialog_mode == QC_ADD)
            XtSetSensitive(qc_apply, False);
      }
   }
   lFreeWhat(what);
   lp = lFreeList(lp);
   alp = lFreeList(alp);



   DEXIT;
}
      
/*-------------------------------------------------------------------------*/
static void qmonQCModify(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lListElem *moq = NULL; 
   lList *lp = NULL;
   lList *alp = NULL;
   lEnumeration *what;
   
   DENTER(TOP_LAYER, "qmonQCModify");


   what = lWhat("%T(ALL)", QU_Type);

   if (!(lp = lCreateList("MQ", QU_Type))) {
      fprintf(stderr, "lCreateList failed\n");
      DEXIT;
      return;
   }

   XmtDialogGetDialogValues(qc_dialog, &current_entry);

   if (current_entry.qname) {
      /* EB: TODO: */
      moq = lCopyElem(cqueue_list_locate_qinstance(qmonMirrorList(SGE_CQUEUE_LIST),  
                                        current_entry.qname));
   }

   if (moq) {
      qmonQCToCull(&current_entry, moq);
      lAppendElem(lp, moq);

      if (rmon_mlgetl(&DEBUG_ON, GUI_LAYER) & INFOPRINT) {
         printf("___Changed Queue________________________\n");
         lWriteListTo(lp, stdout);
         printf("________________________________________\n");
      }
     
      /* EB: TODO: */ 
      alp = qmonModList(SGE_CQUEUE_LIST, qmonMirrorListRef(SGE_CQUEUE_LIST),
                           QU_qname, &lp, NULL, what);

      qmonMessageBox(w, alp, 0);
      
      if ( lFirst(alp) && lGetUlong(lFirst(alp), AN_status) == STATUS_OK ) {
/*          updateQCQ(); */
         updateQueueList();
         dont_close = 0;
      }
      else {
         dont_close = 1;
      }   
   }

   lFreeWhat(what);
   lp = lFreeList(lp);
   alp = lFreeList(alp);
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonQCDelete(w, cld, cad)
Widget w;
XtPointer cld, cad;
{

#if 0
  
   lList *lp = NULL;
   lList *alp = NULL;
   static lEnumeration *what = NULL;
   Boolean status, answer;
   
   DENTER(TOP_LAYER, "qmonQCDelete");

   if (!what)
      what = lWhat("%T(ALL)", QU_Type);
   
   lp = XmStringToCull(qc_queue_list, QU_Type, QU_qname, SELECTED_ITEMS);

   if (lp && (lGetNumberOfElem(lp) > 0)) {
      status = XmtAskForBoolean(w, "xmtBooleanDialog", 
                     "@{queue.askdel.Do you really want to\ndelete the selected queues ?}", 
                     "@{Delete}", "@{Cancel}", NULL, XmtNoButton, XmDIALOG_WARNING, 
                     False, &answer, NULL);
         
      if (answer) { 
         /* EB: TODO: */
         alp = qmonDelList(SGE_CQUEUE_LIST, qmonMirrorListRef(SGE_CQUEUE_LIST),
                                 QU_qname, &lp, NULL, what);

         qmonMessageBox(w, alp, 0);

         alp = lFreeList(alp);
      } 
      lp = lFreeList(lp);
/*       updateQCQ(); */
      updateQueueList();
   }

   DEXIT;
#endif
}


/*-------------------------------------------------------------------------*/
static void qmonQCSetData(
tQCEntry *data,
StringConst qname,
int how 
) {
   lListElem *qep;
   lList *ql;
   
   DENTER(GUI_LAYER, "qmonQCSetData");

   if (!data) {
      DEXIT;
      return;
   }
   
   qmonMirrorMulti(QUEUE_T);
   /* EB: TODO: */
   ql = qmonMirrorList(SGE_CQUEUE_LIST);
   qep = cqueue_list_locate_qinstance(ql, qname);
   qmonCullToQC(qep, data, how);

   DEXIT;

}   
   
/*-------------------------------------------------------------------------*/
static Boolean check_qname(
char *name 
) {
   DENTER(GUI_LAYER, "check_qname");

   if (strchr(name, '/')) {
      DEXIT;
      return False;
   }

   DEXIT;
   return True;
}


/*-------------------------------------------------------------------------*/
static void qmonInitQCEntry(
tQCEntry *data 
) {
   DENTER(GUI_LAYER, "qmonInitQCEntry");

   memset((void*)data, sizeof(tQCEntry), 0);

   DEXIT;

}

/*-------------------------------------------------------------------------*/
static Boolean qmonCullToQC(
lListElem *qep,
tQCEntry *data,
int how 
) {
   StringConst qhostname;
   lListElem *ep = NULL;
   int i;
   const char *str;
   static lCondition *where = NULL;
   static lEnumeration *what = NULL;
   static char *hard_variable[] = {
      "Wallclock Time (sec)", 
      "CPU Time (sec)", 
      "File Size (Byte)", 
      "Data Size (Byte)", 
      "Stack Size (Byte)", 
      "Corefile Size (Byte)", 
      "Resident Set Size (Byte)",
      "Virtual Memory (Byte)"
   };
   static char *soft_variable[] = {
      "Wallclock Time (sec)", 
      "CPU Time (sec)", 
      "File Size (Byte)", 
      "Data Size (Byte)", 
      "Stack Size (Byte)", 
      "Corefile Size (Byte)", 
      "Resident Set Size (Byte)",
      "Virtual Memory (Byte)"
   }; 
   static int hard_value[] = {
      QU_h_rt, QU_h_cpu, QU_h_fsize, QU_h_data, QU_h_stack, QU_h_core, 
      QU_h_rss, QU_h_vmem
   };
   static int soft_value[] = {
      QU_s_rt, QU_s_cpu, QU_s_fsize, QU_s_data, QU_s_stack, QU_s_core, 
      QU_s_rss, QU_s_vmem 
   };
   


   DENTER(GUI_LAYER, "qmonCullToQC");
   
   if (!qep || !data) 
      goto error;

DTRACE;
   /*********************/
   /* general config    */
   /*********************/
   
   if (how == QC_ALL || how == QC_GENERAL || how == QC_ALMOST) {
      if (how == QC_ALL) {
         strncpy(data->qname, lGetString(qep, QU_qname), 
                  XmtSizeOf(tQCEntry, qname));

         qmonMirrorMulti(EXECHOST_T);
         qhostname = lGetHost(qep, QU_qhostname); 
         strncpy(data->qhostname, qhostname,  XmtSizeOf(tQCEntry, qhostname));
      }

      data->qtype       = lGetUlong(qep, QU_qtype);
      strncpy(data->processors, lGetString(qep, QU_processors),  
               XmtSizeOf(tQCEntry, processors));
      if ((str = lGetString(qep, QU_priority)))
         data->priority = atoi(str);
      else
         data->priority = 0;
      data->job_slots   = lGetUlong(qep, QU_job_slots);
      data->rerun       = lGetBool(qep, QU_rerun);
      data->seq_no      = lGetUlong(qep, QU_seq_no);

      strncpy(data->tmpdir, lGetString(qep, QU_tmpdir) ? 
               lGetString(qep, QU_tmpdir) : "",  
               XmtSizeOf(tQCEntry, tmpdir));
      
      strncpy(data->calendar, lGetString(qep, QU_calendar) ? 
                lGetString(qep, QU_calendar) : "",  
                XmtSizeOf(tQCEntry, calendar)); 
      
      strncpy(data->shell, lGetString(qep, QU_shell),  
               XmtSizeOf(tQCEntry, shell));
      
      strncpy(data->notify, lGetString(qep, QU_notify),  
               XmtSizeOf(tQCEntry, notify));

      if (!strcasecmp(lGetString(qep, QU_initial_state), "disabled"))
         data->initial_state = 2; 
      else if (!strcasecmp(lGetString(qep, QU_initial_state), "enabled"))
         data->initial_state = 1; 
      else 
         data->initial_state = 0; 
      if (lGetString(qep, QU_shell_start_mode)) {
         if (!strcasecmp(lGetString(qep, QU_shell_start_mode), "unix_behavior"))
            data->shell_start_mode = 3; 
         else if (!strcasecmp(lGetString(qep, QU_shell_start_mode), "script_from_stdin"))
            data->shell_start_mode = 2; 
         else if (!strcasecmp(lGetString(qep, QU_shell_start_mode), "posix_compliant"))
            data->shell_start_mode = 1; 
         else 
            data->shell_start_mode = 0; 
      }
      data->prolog = lGetString(qep, QU_prolog);
      data->epilog = lGetString(qep, QU_epilog);
      data->starter_method = lGetString(qep, QU_starter_method);
      data->suspend_method = lGetString(qep, QU_suspend_method);
      data->resume_method = lGetString(qep, QU_resume_method);
      data->terminate_method = lGetString(qep, QU_terminate_method);
   }         

DTRACE;
   /*********************/
   /* checkpoint config */
   /*********************/
   
   if (how == QC_ALL || how == QC_CHECKPOINT || how == QC_ALMOST) {
      strncpy(data->min_cpu_interval, lGetString(qep, QU_min_cpu_interval),  
               XmtSizeOf(tQCEntry, min_cpu_interval));
   }
   if (how == QC_ALL || how == QC_CHECKPOINT || how == QC_ALMOST) {
      data->ckpt_list = lFreeList(data->ckpt_list);
      data->ckpt_list = lCopyList("ckpt_list", lGetList(qep, QU_ckpt_list));
   }
DTRACE;

   /*********************/
   /* pe config         */
   /*********************/
   
   if (how == QC_ALL || how == QC_PE || how == QC_ALMOST) {
      data->pe_list = lFreeList(data->pe_list);
      data->pe_list = lCopyList("pe_list", lGetList(qep, QU_pe_list));

   }
DTRACE;

   /*********************/
   /* limit config      */
   /*********************/
   if (how == QC_ALL || how == QC_LIMIT || how == QC_ALMOST) {
      /* we convert the limits to a VA_Type list */
      data->limits_hard = lFreeList(data->limits_hard);
      data->limits_soft = lFreeList(data->limits_soft);

      data->limits_hard = lCreateElemList("Queue Limits", VA_Type, 
                                             XtNumber(hard_variable));
      data->limits_soft = lCreateElemList("Queue Limits", VA_Type, 
                                             XtNumber(soft_variable));
       
      for (i=0, ep=lFirst(data->limits_hard); i<XtNumber(hard_variable) && 
               data->limits_hard; i++, ep=lNext(ep)) {
         lSetString(ep, VA_variable, hard_variable[i]);
         str = lGetString(qep, hard_value[i]);
         lSetString(ep, VA_value, str?str:"");
      }
      for (i=0, ep=lFirst(data->limits_soft); i<XtNumber(soft_variable) && 
               data->limits_soft; i++, ep=lNext(ep)) {
         lSetString(ep, VA_variable, soft_variable[i]);
         str = lGetString(qep, soft_value[i]);
         lSetString(ep, VA_value, str?str:"");
      }
   }
   
DTRACE;
   /**************************/
   /* load threshold config */
   /**************************/
   if (how == QC_ALL || how == QC_LOADTHRESHOLD || how == QC_ALMOST) {
      data->load_thresholds = lFreeList(data->load_thresholds);
      data->load_thresholds = lCopyList("load_thresholds", 
                                  lGetList(qep, QU_load_thresholds));

   }

   /**************************/
   /* suspend threshold config */
   /**************************/
   if (how == QC_ALL || how == QC_SUSPENDTHRESHOLD || how == QC_ALMOST) {
      const char *susp_interval;
      data->suspend_thresholds = lFreeList(data->suspend_thresholds);
      data->suspend_thresholds = lCopyList("suspend_thresholds", 
                                  lGetList(qep, QU_suspend_thresholds));
      susp_interval = lGetString(qep, QU_suspend_interval);
      strncpy(data->suspend_interval, susp_interval ? susp_interval : "",  
               XmtSizeOf(tQCEntry, suspend_interval));
      data->nsuspend = lGetUlong(qep, QU_nsuspend);
   }


DTRACE;
   /**************************/
   /* complexes config       */
   /**************************/
   if (how == QC_ALL || how == QC_COMPLEXES || how == QC_ALMOST) {
      if (!where) {
         where = lWhere("%T(%I != %s)", CE_Type, CE_name, "slots");
         what = lWhat("%T(ALL)", CE_Type);
      }
      data->consumable_config_list = lFreeList(data->consumable_config_list);
      data->consumable_config_list = lSelect("consumable_config_list", 
                                      lGetList(qep, QU_consumable_config_list),
                                      where, what);
   }


DTRACE;
   /**************************/
   /* subordinates config    */
   /**************************/
   if (how == QC_ALL || how == QC_SUBORDINATES || how == QC_ALMOST) {
      data->subordinates = lFreeList(data->subordinates);
      data->subordinates = lCopyList("subordinates", 
                                 lGetList(qep, QU_subordinate_list));
   }
   
DTRACE;
   /**************************/
   /* access config       */
   /**************************/
   if (how == QC_ALL || how == QC_USERSET || how == QC_ALMOST) {
      data->acl = lFreeList(data->acl);
      data->xacl = lFreeList(data->xacl);
      data->acl = lCopyList("acl", lGetList(qep, QU_acl));
      data->xacl = lCopyList("xacl", lGetList(qep, QU_xacl));
   }

   /**************************/
   /* project config       */
   /**************************/
   if (how == QC_ALL || how == QC_PROJECT || how == QC_ALMOST) {
      data->prj = lFreeList(data->prj);
      data->xprj = lFreeList(data->xprj);
      data->prj = lCopyList("prj", lGetList(qep, QU_projects));
      data->xprj = lCopyList("xprj", lGetList(qep, QU_xprojects));
   }


DTRACE;
   /**************************/
   /* owner config           */
   /**************************/
   if (how == QC_ALL || how == QC_OWNER || how == QC_ALMOST) {
/* lWriteListTo(data->owner_list, stdout); */
      data->owner_list = lFreeList(data->owner_list);
      data->owner_list = lCopyList("owner_list", lGetList(qep, QU_owner_list));
   }

   DEXIT;
   return True;

   error:
      fprintf(stderr, "qmonCullToQC failure\n");
      DEXIT;
      return False;
}   

   
   
/*-------------------------------------------------------------------------*/
static Boolean qmonQCToCull(
tQCEntry *data,
lListElem *qep 
) {
   lListElem *ep = NULL;
   char buf[BUFSIZ];
   
   DENTER(GUI_LAYER, "qmonQCToCull");

   if (!qep || !data || !data->qname || !data->qhostname)
      goto error;

   qinstance_state_set_unknown(qep, true);
   
   /**************************/
   /* general config         */
   /**************************/
   lSetString(qep, QU_qname, qmon_trim(data->qname));

   lSetHost(qep, QU_qhostname, data->qhostname);
   /* get the unqualified hostname */

   lSetUlong(qep, QU_qtype, data->qtype);
   lSetString(qep, QU_processors, data->processors);
   sprintf(buf, "%d", data->priority);
   lSetString(qep, QU_priority, buf);
   lSetUlong(qep, QU_job_slots, data->job_slots);
   /* initialize QU_job_slots_used */
   qinstance_set_slots_used(qep, 0);
   lSetBool(qep, QU_rerun, data->rerun);
   lSetUlong(qep, QU_seq_no, data->seq_no);

   lSetString(qep, QU_tmpdir, data->tmpdir);
   lSetString(qep, QU_calendar, data->calendar);
   lSetString(qep, QU_shell, data->shell);
   lSetString(qep, QU_notify, data->notify);

   if (data->initial_state == 0)
      lSetString(qep, QU_initial_state, "default");
   else if (data->initial_state == 1)
      lSetString(qep, QU_initial_state, "enabled");
   else if (data->initial_state == 2)
      lSetString(qep, QU_initial_state, "disabled");
   
   lSetString(qep, QU_prolog, data->prolog);
   lSetString(qep, QU_epilog, data->epilog);
   lSetString(qep, QU_starter_method, data->starter_method);
   lSetString(qep, QU_suspend_method, data->suspend_method);
   lSetString(qep, QU_resume_method, data->resume_method);
   lSetString(qep, QU_terminate_method, data->terminate_method);

   if (data->shell_start_mode == 0)
      lSetString(qep, QU_shell_start_mode, "NONE");
   else if (data->shell_start_mode == 1)
      lSetString(qep, QU_shell_start_mode, "posix_compliant");
   else if (data->shell_start_mode == 2)
      lSetString(qep, QU_shell_start_mode, "script_from_stdin");
   else if (data->shell_start_mode == 3)
      lSetString(qep, QU_shell_start_mode, "unix_behavior");
   
   /**************************/
   /* checkpoint config      */
   /**************************/
   lSetString(qep, QU_min_cpu_interval, data->min_cpu_interval);
   lSetList(qep, QU_ckpt_list, data->ckpt_list);
   data->ckpt_list = NULL;

   /**************************/
   /* pe config              */
   /**************************/
   lSetList(qep, QU_pe_list, data->pe_list);
   data->pe_list = NULL;


   /**************************/
   /* limit config           */
   /**************************/
   /* This is a bit dangerous cause it depends on the order */
   ep = lFirst(data->limits_hard);
   lSetString(qep, QU_h_rt, lGetString(ep, VA_value));
   ep = lNext(ep);
   lSetString(qep, QU_h_cpu, lGetString(ep, VA_value));
   ep = lNext(ep);
   lSetString(qep, QU_h_fsize, lGetString(ep, VA_value));
   ep = lNext(ep);
   lSetString(qep, QU_h_data, lGetString(ep, VA_value));
   ep = lNext(ep);
   lSetString(qep, QU_h_stack, lGetString(ep, VA_value));
   ep = lNext(ep);
   lSetString(qep, QU_h_core, lGetString(ep, VA_value));
   ep = lNext(ep);
   lSetString(qep, QU_h_rss, lGetString(ep, VA_value));
   ep = lNext(ep);
   lSetString(qep, QU_h_vmem, lGetString(ep, VA_value));

   ep = lFirst(data->limits_soft);
   lSetString(qep, QU_s_rt, lGetString(ep, VA_value));
   ep = lNext(ep);
   lSetString(qep, QU_s_cpu, lGetString(ep, VA_value));
   ep = lNext(ep);
   lSetString(qep, QU_s_fsize, lGetString(ep, VA_value));
   ep = lNext(ep);
   lSetString(qep, QU_s_data, lGetString(ep, VA_value));
   ep = lNext(ep);
   lSetString(qep, QU_s_stack, lGetString(ep, VA_value));
   ep = lNext(ep);
   lSetString(qep, QU_s_core, lGetString(ep, VA_value));
   ep = lNext(ep);
   lSetString(qep, QU_s_rss, lGetString(ep, VA_value));
   ep = lNext(ep);
   lSetString(qep, QU_s_vmem, lGetString(ep, VA_value));

   /**************************/
   /* load threshold config  */
   /**************************/
   /*
    * check the new list against the old list here
    */
   lSetList(qep, QU_load_thresholds, data->load_thresholds);
   data->load_thresholds = NULL;

   /**************************/
   /* suspend threshold config  */
   /**************************/
   /*
    * check the new list against the old list here
    */
   lSetList(qep, QU_suspend_thresholds, data->suspend_thresholds);
   data->suspend_thresholds = NULL;
   lSetString(qep, QU_suspend_interval, data->suspend_interval);
   lSetUlong(qep, QU_nsuspend, data->nsuspend);

   /*******************************/
   /* attached complex attributes */
   /*******************************/
   lSetList(qep, QU_consumable_config_list, data->consumable_config_list);
   data->consumable_config_list = NULL;

   /****************************/
   /* attached subordinates    */
   /****************************/
   lSetList(qep, QU_subordinate_list, data->subordinates);
   data->subordinates = NULL;


   /****************************/
   /* access lists             */
   /****************************/
   lSetList(qep, QU_acl, data->acl);
   data->acl = NULL;
   lSetList(qep, QU_xacl, data->xacl);
   data->xacl = NULL;

   /****************************/
   /* project access           */
   /****************************/
   lSetList(qep, QU_projects, data->prj);
   data->prj = NULL;
   lSetList(qep, QU_xprojects, data->xprj);
   data->xprj = NULL;

   /****************************/
   /* owner list               */
   /****************************/
   lSetList(qep, QU_owner_list, data->owner_list);
   data->owner_list = NULL;

/*    lWriteElemTo(qep, stdout); */
   
   DEXIT;
   return True;

   error:
      fprintf(stderr, "qmonQCToCull failure\n");
      DEXIT;
      return False;

}



/*-------------------------------------------------------------------------*/
static void qmonQCResetAll(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   DENTER(GUI_LAYER, "qmonQCResetAll");
   
   /*
   ** fill all pages with template values, leave qname and hostname 
   ** unchanged
   */
   qmonQCSetData(&current_entry, "template", QC_ALMOST);
   XmtDialogSetDialogValues(qc_dialog, &current_entry);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
/* C H E C K P O I N T    P A G E                                          */
/*-------------------------------------------------------------------------*/

static void qmonQCAskForCkpt(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *cl_out = NULL;
   lList *cl_in = NULL;
   int status;
   lList *alp = NULL;

   DENTER(GUI_LAYER, "qmonQCAskForCkpt");
   
   qmonMirrorMultiAnswer(CKPT_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }

   cl_in = qmonMirrorList(SGE_CKPT_LIST);
   cl_out = XmStringToCull(ckpt_reference_list, ST_Type, ST_name, ALL_ITEMS);

   status = XmtAskForItems(w, NULL, NULL, "@{Select Checkpoint Object}", 
                  cl_in, CK_name, "@{@fBAvailable Checkpoint Objects}", 
                  &cl_out, ST_Type, ST_name, "@{@fBChosen Checkpoint Objects}",
                  NULL);

   if (status) {
      UpdateXmListFromCull(ckpt_reference_list, XmFONTLIST_DEFAULT_TAG, cl_out,
                              ST_name);
   }
   cl_out = lFreeList(cl_out);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
/* P E   P A G E                                                           */
/*-------------------------------------------------------------------------*/

static void qmonQCAskForPE(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *pel_out = NULL;
   lList *pel_in = NULL;
   int status;
   lList *alp = NULL;

   DENTER(GUI_LAYER, "qmonQCAskForPE");
   
   qmonMirrorMultiAnswer(PE_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }

   pel_in = qmonMirrorList(SGE_PE_LIST);
   pel_out = XmStringToCull(pe_reference_list, ST_Type, ST_name, ALL_ITEMS);

   status = XmtAskForItems(w, NULL, NULL, "@{Select PE Object}", 
                  pel_in, PE_name, "@{@fBAvailable PE Objects}", 
                  &pel_out, ST_Type, ST_name, "@{@fBChosen PE Objects}",
                  NULL);

   if (status) {
      UpdateXmListFromCull(pe_reference_list, XmFONTLIST_DEFAULT_TAG, pel_out,
                              ST_name);
   }
   pel_out = lFreeList(pel_out);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonQCTime(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget input_field = (Widget) cld;
   char stringval[256];
   Boolean status = False;
   String current;

   DENTER(GUI_LAYER, "qmonQCTime");

   current = XmtInputFieldGetString(input_field);
   strncpy(stringval, current ? current : "", sizeof(stringval)-1);
   status = XmtAskForTime(w, NULL, "@{Enter time}",
               stringval, sizeof(stringval), NULL, True);
   if (stringval[0] == '\0')
      status = False;

   if (status)
      XmtInputFieldSetString(input_field, stringval);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonQCCalendar(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget input_field = (Widget) cld;
   char buf[256];
   Boolean status = False;
   lList *cl = NULL;
   lListElem *cep = NULL;
   StringConst *strs = NULL;
   int n, i;

   DENTER(GUI_LAYER, "qmonQCCalendar");

   qmonMirrorMulti(CALENDAR_T);
   cl = qmonMirrorList(SGE_CALENDAR_LIST);
   n = lGetNumberOfElem(cl);
   if (n>0) {
      strs = (StringConst*)XtMalloc(sizeof(String)*n); 
      for (cep=lFirst(cl), i=0; i<n; cep=lNext(cep), i++) {
        /*
        ** we get only references don't free, the strings
        */
        strs[i] = lGetString(cep, CAL_name);
      }
    
      strcpy(buf, "");
      /* FIX_CONST_GUI */
      status = XmtAskForItem(w, NULL, "@{Get calendar}",
                        "@{Available Calendars}", (String*)strs, n,
                        False, buf, 256, NULL); 


      if (status) {
         XmtInputFieldSetString(input_field, buf);
      }
      /*
      ** don't free referenced strings, they are in the queue list
      */
      XtFree((char*)strs);
   }
   else
      qmonMessageShow(w, True, "@{No calendars to select}");
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
/* L I M I T    P A G E                                                    */
/*-------------------------------------------------------------------------*/
static void qmonQCLimitNoEdit(w, cld, cad)
Widget w;
XtPointer cld, cad; 
{
   XbaeMatrixEnterCellCallbackStruct *cbs =
         (XbaeMatrixEnterCellCallbackStruct*) cad;

   DENTER(GUI_LAYER, "qmonQCLimitNoEdit");

   switch( cbs->column ) {
      case 0:
         cbs->doit = False;
         break;
      default:
         cbs->doit = True;       /* Default behaviour */
         break;
    }
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonQCLimitCheck(w, cld, cad)
Widget w;
XtPointer cld, cad; 
{
   XbaeMatrixLeaveCellCallbackStruct *cbs =
         (XbaeMatrixLeaveCellCallbackStruct*) cad;
   u_long32 uval;
   char buf[BUFSIZ];
   static Boolean show_message = True;

   DENTER(GUI_LAYER, "qmonQCLimitCheck");

   if (cbs->column == 1) {
      if (!parse_ulong_val(NULL, &uval, TYPE_TIM, cbs->value, buf, BUFSIZ-1)) {
         if (show_message)
            qmonMessageShow(w, True, "@{qaction.novalidtime.No valid time format: hh:mm:ss or\nINFINITY required !}");
         show_message = False;
         cbs->doit = False;
      }
      else
         show_message = True;
   }
  
   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonQCLimitInput(w, cld, cad)
Widget w;
XtPointer cld, cad; 
{
   XbaeMatrixDefaultActionCallbackStruct *cbs =
         (XbaeMatrixDefaultActionCallbackStruct*) cad;
   char buf[BUFSIZ];
   char stringval[BUFSIZ];
   Boolean status = False;
   String limit;
   String value;

   DENTER(GUI_LAYER, "qmonQCLimitInput");

   /*
   ** this depends on the order of the limits displayed
   */
   if (cbs->row < 2) {
      limit = XbaeMatrixGetCell(w, cbs->row, 0);
      value = XbaeMatrixGetCell(w, cbs->row, 1);
      sprintf(buf, "%s '@fB%s':", 
              XmtLocalize(w, "Enter a time value for", 
              "Enter a time value for"), limit);
      strncpy(stringval, value, BUFSIZ-1);
      status = XmtAskForTime(w, NULL, buf,
                  stringval, sizeof(stringval), NULL, True);
      if (stringval[0] == '\0')
         status = False;
   }
   else {
      limit = XbaeMatrixGetCell(w, cbs->row, 0);
      value = XbaeMatrixGetCell(w, cbs->row, 1);
      strncpy(stringval, value, BUFSIZ-1);
      status = XmtAskForMemory(w, NULL, "@{Enter a memory value}",
                  stringval, sizeof(stringval), NULL);
      if (stringval[0] == '\0')
         status = False;
   }

   if (status) {
      XbaeMatrixSetCell(w, cbs->row, 1, stringval);
   }
   
   DEXIT;
}




/*-------------------------------------------------------------------------*/
/* C O M P L E X E S    P A G E                                            */
/*-------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------*/
/* S U B O R D I N A T E S   P A G E                                       */
/*-------------------------------------------------------------------------*/
static void qmonQCSOQ(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   XbaeMatrixDefaultActionCallbackStruct *cbs =
         (XbaeMatrixDefaultActionCallbackStruct*) cad;
   Boolean status = False;
   lList *ql = NULL;
   lListElem *qep = NULL;
   int n, i;
   StringConst *strs = NULL;
   static char buf[BUFSIZ];
   
   DENTER(GUI_LAYER, "qmonQCSOQ");
   
   if (cbs->column == 0) {
      qmonMirrorMulti(QUEUE_T);
      /* EB: TODO: */
      ql = qmonMirrorList(SGE_CQUEUE_LIST);
      n = lGetNumberOfElem(ql);
      if (n>0) {
         strs = (StringConst*)XtMalloc(sizeof(String)*n); 
         for (qep=lFirst(ql), i=0; i<n; qep=lNext(qep), i++) {
           /*
           ** we get only references don't free, the strings
           */
           strs[i] = lGetString(qep, QU_qname);
         }
       
         strcpy(buf, "");
         /* FIX_CONST_GUI */
         status = XmtAskForItem(w, NULL, "@{Get Queue}",
                           "@{Available Queues}", (String*)strs, n,
                           False, buf, BUFSIZ, NULL); 
         
         if (status) {
             XbaeMatrixSetCell(w, cbs->row, cbs->column, buf);
         }
         /*
         ** don't free referenced strings, they are in the queue list
         */
         XtFree((char*)strs);
      }
      else
         qmonMessageShow(w, True, "@{No queues to select}");
   } 
   DEXIT;
}


/*-------------------------------------------------------------------------*/
/* A C C E S S L I S T     P A G E                                         */
/*-------------------------------------------------------------------------*/
static void qmonQCAccessToggle(w, cld, cad)
Widget w;
XtPointer cld, cad; 
{
   XmtChooserCallbackStruct *cbs = (XmtChooserCallbackStruct*) cad;
   
   DENTER(GUI_LAYER, "qmonQCAccessToggle");

   if (cbs->state) {
      XtSetSensitive(access_allow, False);
      XtSetSensitive(access_deny, True);
   }
   else {
      XtSetSensitive(access_allow, True);
      XtSetSensitive(access_deny, False);
   }
  
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonQCAccessAdd(w, cld, cad)
Widget w;
XtPointer cld, cad; 
{
   XmString *selectedItems;
   Cardinal selectedItemCount, i;
   Widget list;
   int state;
   String text;
   
   DENTER(GUI_LAYER, "qmonQCAccessAdd");

   XtVaGetValues( access_list,
                  XmNselectedItems, &selectedItems,
                  XmNselectedItemCount, &selectedItemCount,
                  NULL);

   if (selectedItemCount) {
      XmtLayoutDisableLayout(qc_dialog);
      state = XmtChooserGetState(access_toggle);

      if (state)
         list = access_deny;
      else
         list = access_allow;
         
      for (i=0; i<selectedItemCount; i++) {
         if (!XmStringGetLtoR(selectedItems[i], XmFONTLIST_DEFAULT_TAG, &text))
            continue;
         XmListAddItemUniqueSorted(list, text);
         XtFree(text); 
      }
      XmtLayoutEnableLayout(qc_dialog);
   }

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonQCAccessRemove(w, cld, cad)
Widget w;
XtPointer cld, cad; 
{
   XmString *selectedItems;
   Cardinal selectedItemCount;
   int state;
   Widget list;
   
   DENTER(GUI_LAYER, "qmonQCAccessRemove");

   state = XmtChooserGetState(access_toggle);

   if (state)
      list = access_deny;
   else
      list = access_allow;
         
   XtVaGetValues( list,
                  XmNselectedItems, &selectedItems,
                  XmNselectedItemCount, &selectedItemCount,
                  NULL);

   if (selectedItemCount)
      XmListDeleteItems(list, selectedItems, selectedItemCount); 

   DEXIT;
}

/*-------------------------------------------------------------------------*/
/* P R O J E C T     P A G E                                               */
/*-------------------------------------------------------------------------*/
static void qmonQCProjectToggle(w, cld, cad)
Widget w;
XtPointer cld, cad; 
{
   XmtChooserCallbackStruct *cbs = (XmtChooserCallbackStruct*) cad;
   
   DENTER(GUI_LAYER, "qmonQCProjectToggle");

   if (cbs->state) {
      XtSetSensitive(project_allow, False);
      XtSetSensitive(project_deny, True);
   }
   else {
      XtSetSensitive(project_allow, True);
      XtSetSensitive(project_deny, False);
   }
  
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonQCProjectAdd(w, cld, cad)
Widget w;
XtPointer cld, cad; 
{
   XmString *selectedItems;
   Cardinal selectedItemCount, i;
   Widget list;
   int state;
   String text;
   
   DENTER(GUI_LAYER, "qmonQCProjectAdd");

   XtVaGetValues( project_list,
                  XmNselectedItems, &selectedItems,
                  XmNselectedItemCount, &selectedItemCount,
                  NULL);

   if (selectedItemCount) {
      XmtLayoutDisableLayout(qc_dialog);
      state = XmtChooserGetState(project_toggle);

      if (state)
         list = project_deny;
      else
         list = project_allow;
         
      for (i=0; i<selectedItemCount; i++) {
         if (!XmStringGetLtoR(selectedItems[i], XmFONTLIST_DEFAULT_TAG, &text))
            continue;
         XmListAddItemUniqueSorted(list, text);
         XtFree(text); 
      }
      XmtLayoutEnableLayout(qc_dialog);
   }

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonQCProjectRemove(w, cld, cad)
Widget w;
XtPointer cld, cad; 
{
   XmString *selectedItems;
   Cardinal selectedItemCount;
   int state;
   Widget list;
   
   DENTER(GUI_LAYER, "qmonQCProjectRemove");

   state = XmtChooserGetState(project_toggle);

   if (state)
      list = project_deny;
   else
      list = project_allow;
         
   XtVaGetValues( list,
                  XmNselectedItems, &selectedItems,
                  XmNselectedItemCount, &selectedItemCount,
                  NULL);

   if (selectedItemCount)
      XmListDeleteItems(list, selectedItems, selectedItemCount); 

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonQCOwnerAdd(w, cld, cad)
Widget w;
XtPointer cld, cad; 
{
   XmString xnew = NULL;
   String new = NULL;

   DENTER(GUI_LAYER, "qmonQCOwnerAdd");
   
   new = XmtInputFieldGetString(owner_new);

   if (new && new[0] != '\0') {
      xnew = XmtCreateXmString(new);
      XmListAddItem(owner_list, xnew, 0); 
      XmStringFree(xnew);
      XmtInputFieldSetString(owner_new, "");
   }

   DEXIT;
}




/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

#if 0

/*-------------------------------------------------------------------------*/
void updateQCQ(void)
{
   lList *qlf;
   static lCondition *where = NULL;
   
   DENTER(GUI_LAYER, "updateQCQ");

   if (qc_dialog && XtIsManaged(qc_dialog)) {
      /* disable/enable redisplay while updating */
      XmtLayoutDisableLayout(qc_dialog);
      /* list with template sorted alphabetically */
      /* EB: TODO: */
      qlf = lCopyList("qlf", qmonMirrorList(SGE_CQUEUE_LIST));
      lPSortList(qlf, "%I+", QU_qname);
      UpdateXmListFromCull(qc_queue_list, XmFONTLIST_DEFAULT_TAG, qlf, 
                              QU_qname);
                              
      if (!where) {
         where = lWhere("%T(%I != %s)", QU_Type, QU_qname, "template");
      }
      qlf = lSelectDestroy(qlf, where);
      UpdateXmListFromCull(qc_queue_set, XmFONTLIST_DEFAULT_TAG, qlf, QU_qname);
      XmtLayoutEnableLayout(qc_dialog);

      qlf = lFreeList(qlf);
   }
   
   DEXIT;
}

#endif

/*-------------------------------------------------------------------------*/
static void updateQCA(void)
{
   lList *al;
   
   DENTER(GUI_LAYER, "updateQCA");
   
   /* What about sorting */
   al = qmonMirrorList(SGE_USERSET_LIST);
   lPSortList(al, "%I+", US_name);
   
   /* disable/enable redisplay while updating */
   XmtLayoutDisableLayout(qc_dialog);
   UpdateXmListFromCull(access_list, XmFONTLIST_DEFAULT_TAG, al, US_name);
   XmtLayoutEnableLayout(qc_dialog);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void updateQCP(void)
{
   lList *pl;
   
   DENTER(GUI_LAYER, "updateQCP");
   
   /* What about sorting */
   pl = qmonMirrorList(SGE_PROJECT_LIST);
   
   /* disable/enable redisplay while updating */
   XmtLayoutDisableLayout(qc_dialog);
   lPSortList(pl, "%I+", UP_name);
   UpdateXmListFromCull(project_list, XmFONTLIST_DEFAULT_TAG, pl, UP_name);
   XmtLayoutEnableLayout(qc_dialog);

   DEXIT;
}


