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

#include <Xmt/Xmt.h>
#include <Xmt/Create.h>
#include <Xmt/Icon.h>
#include <Xmt/Layout.h>
#include <Xmt/Dialog.h>
#include <Xmt/Dialogs.h>
#include <Xmt/InputField.h>
#include <Xmt/MsgLine.h>

#include "sge_gdi.h"
#include "sge_all_listsL.h"
#include "qmon_rmon.h"
#include "qmon_cull.h"
#include "qmon_globals.h"
#include "qmon_appres.h"
#include "qmon_util.h"
#include "qmon_init.h"
#include "qmon_ticket.h"
#include "qmon_share.h"
#include "qmon_fticket.h"
#include "qmon_comm.h"
#include "qmon_message.h"
#include "qmon_manop.h"
#include "qmon_timer.h"

typedef struct _TOVEntry {
   Cardinal      total_share_tree_tickets;
   Cardinal      total_functional_tickets;
   Cardinal      max_deadline_initiation_tickets;
   Cardinal      deadline_initiation_tickets;
   Cardinal      override_tickets;
} tTOVEntry;

static tTOVEntry cdata = {0, 0, 0, 0, 0};
static Boolean data_changed = False;

static Widget qmon_tov = 0;
static Widget tov_layout = 0;
static Widget tov_message = 0;

XtResource tov_resources[] = {
   { "total_share_tree_tickets", "total_share_tree_tickets", XtRCardinal,
      sizeof(Cardinal),
      XtOffsetOf(tTOVEntry, total_share_tree_tickets),
      XtRImmediate, (XtPointer) 0 },

   { "label_share_tree_tickets", "label_share_tree_tickets", XtRCardinal,
      sizeof(Cardinal),
      XtOffsetOf(tTOVEntry, total_share_tree_tickets),
      XtRImmediate, (XtPointer) 0 },

   { "total_functional_tickets", "total_functional_tickets", XtRCardinal,
      sizeof(Cardinal),
      XtOffsetOf(tTOVEntry, total_functional_tickets),
      XtRImmediate, (XtPointer) 0 },

   { "label_functional_tickets", "label_functional_tickets", XtRCardinal,
      sizeof(Cardinal),
      XtOffsetOf(tTOVEntry, total_functional_tickets),
      XtRImmediate, (XtPointer) 0 },

   { "max_deadline_initiation_tickets", "max_deadline_initiation_tickets", 
      XtRInt, sizeof(Cardinal),
      XtOffsetOf(tTOVEntry, max_deadline_initiation_tickets),
      XtRImmediate, (XtPointer) 0 },

   { "deadline_initiation_tickets", "deadline_initiation_tickets", XtRCardinal,
      sizeof(Cardinal),
      XtOffsetOf(tTOVEntry, deadline_initiation_tickets),
      XtRImmediate, (XtPointer) 0 },

   { "override_tickets", "override_tickets", XtRCardinal,
      sizeof(Cardinal),
      XtOffsetOf(tTOVEntry, override_tickets),
      XtRImmediate, (XtPointer) 0 }

};

/*-------------------------------------------------------------------------*/
static void qmonPopdownTicketOverview(Widget w, XtPointer cld, XtPointer cad);
static Widget qmonCreateTicketOverview(Widget parent);
static void qmonTOVInput(Widget w, XtPointer cld, XtPointer cad);
static void qmonTOVUpdate(Widget w, XtPointer cld, XtPointer cad);
static void qmonTOVApply(Widget w, XtPointer cld, XtPointer cad);
static Boolean qmonTOVEntryToCull(lListElem *scep, tTOVEntry *tov_data);
static Boolean qmonCulltoTOVEntry(tTOVEntry *tov_data, lListElem *scep);
static int qmonTOVUpdateFill(Widget w, lList **alpp);

/*-------------------------------------------------------------------------*/
/* P U B L I C                                                             */
/*-------------------------------------------------------------------------*/
void qmonPopupTicketOverview(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *alp = NULL;
   DENTER(GUI_LAYER, "qmonPopupTicketOverview");

   /* set busy cursor */
   XmtDisplayBusyCursor(w);

   if (!qmon_tov) {
      qmon_tov = qmonCreateTicketOverview(AppShell);
      /* 
      ** set the close button callback 
      ** set the icon and iconName
      */
      XmtCreatePixmapIcon(qmon_tov, qmonGetIcon("toolbar_ticket"), None);
      XtVaSetValues(qmon_tov, XtNiconName, "qmon:Ticket Overview", NULL);
      XmtAddDeleteCallback(qmon_tov, XmDO_NOTHING, 
                              qmonPopdownTicketOverview, NULL);
      XtAddEventHandler(qmon_tov, StructureNotifyMask, False, 
                        SetMinShellSize, NULL);
   }

   /*
   ** set the Ticket Overview Data
   ** cdata should always contain the actual data
   */
   if (qmonTOVUpdateFill(qmon_tov, &alp)) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }

   /*
   ** popup and raise the dialog
   */
   xmui_manage(qmon_tov);

   /* set normal cursor */
   XmtDisplayDefaultCursor(w);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
/* P R I V A T E                                                           */
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
static void qmonPopdownTicketOverview(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Boolean status = True;
   Boolean answer = False;

   DENTER(GUI_LAYER, "qmonPopdownTicketOverview");

   if (data_changed) 
      status = XmtAskForBoolean(w, "xmtBooleanDialog", 
                     "@{ticket.asksave.Do you want to save your changes ?}", 
                     "@{Yes}", "@{No}", "@{Cancel}", XmtNoButton, 
                        XmDIALOG_QUESTION, 
                     False, &answer, NULL);

   if (status) {
      if (answer)
         qmonTOVApply(w, NULL, NULL);
      else
         data_changed = False;
      xmui_unmanage(qmon_tov);    
   }
   
   DEXIT;
}

#if 0
/*-------------------------------------------------------------------------*/
static void qmonTOVStartUpdate(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   DENTER(GUI_LAYER, "qmonTOVStartUpdate");
  
   qmonTimerAddUpdateProc(JOB_T, "updateJobList", updateJobList);
   qmonStartTimer(JOB | QUEUE | COMPLEX);
   
   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonJobStopUpdate(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   DENTER(GUI_LAYER, "qmonJobStopUpdate");
  
   qmonStopTimer(JOB_TIMER | QUEUE_TIMER | COMPLEX_TIMER);
   qmonTimerRmUpdateProc(JOB_T, "updateJobList");
   
   DEXIT;
}

#endif


/*-------------------------------------------------------------------------*/
static Widget qmonCreateTicketOverview(
Widget parent 
) {
   Widget shell, tov_done, tov_edit, tov_users_for_deadline, 
            tov_share_tree_policy, tov_functional_policy, 
            tov_override_policy, share_tree_tickets_field,
            functional_tickets_field, deadline_tickets_field,
            tov_update, tov_apply, tov_main_link;
   
   DENTER(GUI_LAYER, "qmonCreateTicketOverview");

   /* create shell */
   shell = XmtCreateChild(parent, "qmon_tov");

   /* submit control layout */
   tov_layout = XmtCreateLayout( shell, "tov_layout", 
                                    NULL, 0); 

   /* bind resource list */
   XmtDialogBindResourceList( tov_layout, 
                              tov_resources, XtNumber(tov_resources));

   /* create children */
   XmtCreateQueryChildren( tov_layout, 
                           "tov_edit", &tov_edit,
                           "share_tree_tickets_field", 
                                 &share_tree_tickets_field,
                           "functional_tickets_field", 
                                 &functional_tickets_field,
                           "deadline_tickets_field", 
                                 &deadline_tickets_field,
                           "tov_users_for_deadline", &tov_users_for_deadline,
                           "tov_share_tree_policy", &tov_share_tree_policy,
                           "tov_functional_policy", &tov_functional_policy,
                           "tov_override_policy", &tov_override_policy,
                           "tov_done", &tov_done,
                           "tov_apply", &tov_apply,
                           "tov_update", &tov_update,
                           "tov_main_link", &tov_main_link,
                           "tov_message", &tov_message,
                           NULL);
   XtManageChild(tov_layout);

#if 0
   /* start the needed timers and the corresponding update routines */
   XtAddCallback(shell, XmNpopupCallback, 
                     qmonTOVStartUpdate, NULL);
   XtAddCallback(shell, XmNpopdownCallback,
                     qmonTOVStopUpdate, NULL);
                     
#endif

   XtAddCallback(tov_main_link, XmNactivateCallback, 
                     qmonMainControlRaise, NULL);
   XtAddCallback(tov_done, XmNactivateCallback, 
                     qmonPopdownTicketOverview, NULL);
   XtAddCallback(tov_update, XmNactivateCallback, 
                     qmonTOVUpdate, NULL);
   XtAddCallback(tov_apply, XmNactivateCallback, 
                     qmonTOVApply, NULL);

   XtAddCallback(tov_users_for_deadline, XmNactivateCallback, 
                     qmonPopupManopConfig, (XtPointer)2);

   XtAddCallback(tov_share_tree_policy, XmNactivateCallback, 
                     qmonShareTreePopup, NULL);
   XtAddCallback(tov_functional_policy, XmNactivateCallback, 
                     qmonFTicketPopup, NULL);
   XtAddCallback(tov_override_policy, XmNactivateCallback, 
                     qmonOTicketPopup, NULL);

   XtAddCallback(functional_tickets_field, XmtNinputCallback,
                     qmonTOVInput, NULL);
   XtAddCallback(share_tree_tickets_field, XmtNinputCallback,
                     qmonTOVInput, NULL);
   XtAddCallback(deadline_tickets_field, XmtNinputCallback,
                     qmonTOVInput, NULL);

   DEXIT;
   return shell;
}


/*-------------------------------------------------------------------------*/
static void qmonTOVInput(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   tTOVEntry data;

   DENTER(GUI_LAYER, "qmonTOVInput");

   /*
   ** get the data that have changed and set the corresponding labels
   ** the links are the label_functional_tickets and label_share_tree_tickets
   ** resources which reference the same entries as the 
   ** total_share_tree_tickets total_functional_tickets resources
   */
   XmtDialogGetDialogValues(tov_layout, &data);

   if (data.total_share_tree_tickets != cdata.total_share_tree_tickets) {
      cdata.total_share_tree_tickets = data.total_share_tree_tickets;
      data_changed = True;
   }
   if (data.total_functional_tickets != cdata.total_functional_tickets) {
      cdata.total_functional_tickets = data.total_functional_tickets;
      data_changed = True;
   }
   if (data.max_deadline_initiation_tickets != 
                           cdata.max_deadline_initiation_tickets) {
      cdata.max_deadline_initiation_tickets = 
                              data.max_deadline_initiation_tickets;
      data_changed = True;
   }
   
   XmtDialogSetDialogValues(tov_layout, &cdata);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonTOVUpdate(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   DENTER(GUI_LAYER, "qmonTOVUpdate");
   
   qmonTOVUpdateFill(w, NULL);
      
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static int qmonTOVUpdateFill(Widget w, lList **alpp)
{
   lList *scl = NULL;
   lListElem *scep = NULL;
   
   DENTER(GUI_LAYER, "qmonTOVUpdateFill");
   /*
   ** set and get the sge conf list
   */
   if (qmonMirrorMultiAnswer(SC_T, alpp)) {
      DEXIT;
      return -1;
   }
   
   scl = qmonMirrorList(SGE_SC_LIST);
   scep = lFirst(scl);
   qmonCulltoTOVEntry(&cdata, scep);
   
   XmtDialogSetDialogValues(tov_layout, &cdata);
   
   DEXIT;
   return 0;
}   

/*-------------------------------------------------------------------------*/
static void qmonTOVApply(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *scl;
   lList *alp = NULL;
   lListElem *sep;
   lEnumeration *what;

   DENTER(GUI_LAYER, "qmonTOVApply");
   
   /*
   ** set the entries in the cdata element into the cull
   ** element
   */
   qmonMirrorMultiAnswer(SC_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }
   scl = qmonMirrorList(SGE_SC_LIST);
   sep = lFirst(scl);
   if (sep) {
      qmonTOVEntryToCull(sep, &cdata);

      what = lWhat("%T(ALL)", SC_Type);
      alp = qmonModList(SGE_SC_LIST, qmonMirrorListRef(SGE_SC_LIST),
                  SC_halftime, &scl, NULL, what); 
      
      qmonMessageBox(w, alp, 0);

      if (alp && lGetUlong(lFirst(alp), AN_status) != STATUS_OK)
         XmtMsgLinePrintf(tov_message, XmtLocalize(w, "Failure", "Failure"));
      else
         XmtMsgLinePrintf(tov_message, XmtLocalize(w, "Success", "Success"));
      XmtMsgLineClear(tov_message, DISPLAY_MESSAGE_DURATION); 

      data_changed = False;

      lFreeList(alp);
   }
   
   DEXIT;
}



/*-------------------------------------------------------------------------*/
static Boolean qmonCulltoTOVEntry(
tTOVEntry *tov_data,
lListElem *scep 
) {
   DENTER(GUI_LAYER, "qmonCulltoTOVEntry");

   if (!tov_data || !scep) {
      DEXIT;
      return False;
   }

   tov_data->total_share_tree_tickets = (Cardinal)lGetUlong(scep, 
                                               SC_weight_tickets_share);
   tov_data->total_functional_tickets = (Cardinal)lGetUlong(scep, 
                                               SC_weight_tickets_functional);
   tov_data->max_deadline_initiation_tickets = (Cardinal)lGetUlong(scep, 
                                               SC_weight_tickets_deadline);

   /*
   ** these values are no configuration values but calculated and
   ** made available for convenience
   */
   tov_data->deadline_initiation_tickets = (Cardinal)lGetUlong(scep, 
                                            SC_weight_tickets_deadline_active);
   tov_data->override_tickets = (Cardinal)lGetUlong(scep, 
                                            SC_weight_tickets_override);

   DEXIT;
   return True;
}

/*-------------------------------------------------------------------------*/
static Boolean qmonTOVEntryToCull(
lListElem *scep,
tTOVEntry *tov_data 
) {
   DENTER(GUI_LAYER, "qmonTOVEntryToCull");

   if (!tov_data || !scep) {
      DEXIT;
      return False;
   }

   lSetUlong(scep, SC_weight_tickets_share, 
               (u_long32)tov_data->total_share_tree_tickets);
   lSetUlong(scep, SC_weight_tickets_functional,
               (u_long32)tov_data->total_functional_tickets);
   lSetUlong(scep, SC_weight_tickets_deadline,
               (u_long32)tov_data->max_deadline_initiation_tickets);

   DEXIT;
   return True;
}

