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

#include <Xmt/Xmt.h>
#include <Xmt/Create.h>
#include <Xmt/Layout.h>
#include <Xmt/Chooser.h>
#include <Xmt/Dialog.h>
#include <Xmt/Dialogs.h>
#include <Xmt/InputField.h>
#include <Xmt/MsgLine.h>

#include "qmon_rmon.h"
#include "qmon_cull.h"
#include "qmon_util.h"
#include "qmon_timer.h"
#include "qmon_comm.h"
#include "qmon_fticket.h"
#include "qmon_message.h"
#include "qmon_widgets.h"
#include "qmon_manop.h"
#include "qmon_job.h"
#include "qmon_queue.h"
#include "qmon_globals.h"
#include "qmon_init.h"
#include "qmon_project.h"
#include "qmon_appres.h"
#include "sge_all_listsL.h"
#include "sge_gdi.h"
#include "sge_answer.h"

#include "Matrix.h"

typedef struct _tFOTInfo {
   Widget   layout;
   Widget   matrix;
   u_long32 list_type;
   lDescr   *dp;
   int      field0;
   int      field1;
} tFOTInfo;
   
static tFOTInfo fticket_info;
static tFOTInfo oticket_info;

typedef struct _tFREntry {
   int fticket_user;
   int fticket_userset;
   int fticket_project;
   int fticket_job;
   int fticket_jobclass;
} tFREntry;

static tScaleWeight WeightData[5];

static tFREntry ratio_data;

static Widget qmon_fticket = 0;
static Widget qmon_oticket = 0;
static Widget fticket_matrix = 0;
static Widget oticket_matrix = 0;
static Widget fticket_type = 0;
static Widget oticket_type = 0;
static Widget fticket_message = 0;
static Widget oticket_message = 0;

static Widget fticket_ratio = 0;

static XtResource ratio_resources[] = {
   { "fticket_user", "fticket_user", XtRInt,
      sizeof(int),
      XtOffsetOf(tFREntry, fticket_user),
      XtRImmediate, NULL },

   { "fticket_userset", "fticket_userset", XtRInt,
      sizeof(int),
      XtOffsetOf(tFREntry, fticket_userset),
      XtRImmediate, NULL },

   { "fticket_project", "fticket_project", XtRInt,
      sizeof(int),
      XtOffsetOf(tFREntry, fticket_project),
      XtRImmediate, NULL },

   { "fticket_job", "fticket_job", XtRInt,
      sizeof(int),
      XtOffsetOf(tFREntry, fticket_job),
      XtRImmediate, NULL },

   { "fticket_jobclass", "fticket_jobclass", XtRInt,
      sizeof(int),
      XtOffsetOf(tFREntry, fticket_jobclass),
      XtRImmediate, NULL }

};

/*-------------------------------------------------------------------------*/
static void set_functional_share_percentage(Widget mw);
static void qmonFOTicketPopdown(Widget w, XtPointer cld, XtPointer cad);
static void qmonFTShowMore(Widget w, XtPointer cld, XtPointer cad);
static void qmonFTLeaveCell(Widget w, XtPointer cld, XtPointer cad);
static void qmonFOTEnterCell(Widget w, XtPointer cld, XtPointer cad);
static void qmonFTTraverseCell(Widget w, XtPointer cld, XtPointer cad);
static void qmonFOTOpenLink(Widget w, XtPointer cld, XtPointer cad);
static Widget qmonFTicketCreate(Widget parent);
static Widget qmonOTicketCreate(Widget parent);
static void qmonFTSwitchType(Widget w, XtPointer cld, XtPointer cad);
static void qmonFTUpdate(Widget w, XtPointer cld, XtPointer cad);
static void qmonOTUpdate(Widget w, XtPointer cld, XtPointer cad);
static void qmonOTSwitchType(Widget w, XtPointer cld, XtPointer cad);
static void qmonFOTCullToMatrix(Widget matrix, lList *lp, int nm0, int nm1);
static Boolean qmonFOTMatrixToCull(Widget matrix, lList *lp, int nm0, int nm1);
static void qmonFTOkay(Widget w, XtPointer cld, XtPointer cad);
static void qmonOTOkay(Widget w, XtPointer cld, XtPointer cad);
static void CullToParameters(tFREntry *data, lListElem *sep);
static void ParametersToCull(lListElem *sep, tFREntry *data);

/*-------------------------------------------------------------------------*/
/* P U B L I C                                                             */
/*-------------------------------------------------------------------------*/
void qmonFTicketPopup(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget shell;

   DENTER(GUI_LAYER, "qmonFTicketPopup");

   /* set busy cursor */
   XmtDisplayBusyCursor(w);

   if (!qmon_fticket) {
      shell = XmtGetTopLevelShell(w);
      qmon_fticket = qmonFTicketCreate(shell);
      XmtAddDeleteCallback(shell, XmDO_NOTHING, 
                              qmonFOTicketPopdown, (XtPointer)qmon_fticket);
   } 

   XmtChooserSetState(fticket_type, 0, True);

   xmui_manage(qmon_fticket);

/*    XtManageChild(qmon_fticket); */
/*    XmtRaiseShell(XtParent(qmon_fticket));  */

   /* set default cursor */
   XmtDisplayDefaultCursor(w);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
void qmonOTicketPopup(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget shell;

   DENTER(GUI_LAYER, "qmonOTicketPopup");

   /* set busy cursor */
   XmtDisplayBusyCursor(w);

   if (!qmon_oticket) {
      shell = XmtGetTopLevelShell(w);
      qmon_oticket = qmonOTicketCreate(shell);
      XmtAddDeleteCallback(shell, XmDO_NOTHING, 
                              qmonFOTicketPopdown, (XtPointer)qmon_oticket);
   } 

   XmtChooserSetState(oticket_type, 0, True);
   
   xmui_manage(qmon_oticket);

/*    XtManageChild(qmon_oticket); */
/*    XmtRaiseShell(XtParent(qmon_oticket));  */

   /* set default cursor */
   XmtDisplayDefaultCursor(w);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
/* P R I V A T E                                                           */
/*-------------------------------------------------------------------------*/
static void qmonFOTicketPopdown(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget ticket = (Widget)cld;

   DENTER(GUI_LAYER, "qmonFOTicketPopdown");

   XtUnmanageChild(ticket);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonFOTOpenLink(
Widget w,
XtPointer cld,
XtPointer cad 
) {
   tFOTInfo *info = (tFOTInfo *) cld;

   DENTER(GUI_LAYER, "qmonFOTOpenLink");

   switch (info->list_type) {
      case SGE_USER_LIST:
         qmonPopupManopConfig(w, NULL, NULL); 
         break;

      case SGE_USERSET_LIST:
         qmonPopupManopConfig(w, NULL, NULL); 
         break;

      case SGE_PROJECT_LIST:
         qmonPopupProjectConfig(w, NULL, NULL); 
         break;

      case SGE_JOB_LIST:
         qmonJobPopup(w, NULL, NULL); 
         break;

      case SGE_CQUEUE_LIST:
         qmonQueuePopup(w, NULL, NULL); 
         break;
   }
  
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonFTOkay(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *lp = NULL;
   lList *alp = NULL;
   lListElem *sep = NULL;
   lEnumeration *what = NULL;
   lCondition *where = NULL;

   DENTER(GUI_LAYER, "qmonFTOkay");

   qmonMirrorMultiAnswer(l2s(fticket_info.list_type), &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }
   lp = qmonMirrorList(fticket_info.list_type);

   /*
   ** filter queues
   */
   if (fticket_info.list_type == SGE_CQUEUE_LIST) {
      where = lWhere("%T(%I != %s)", QU_Type, QU_qname, "template");
      what = lWhat("%T(ALL)", QU_Type);
      lp = lSelect("Queues without template", lp, where, what);
      where = lFreeWhere(where);
      what = lFreeWhat(what);
   } 

   if (qmonFOTMatrixToCull(fticket_matrix, lp, fticket_info.field0, 
                        fticket_info.field1)) {

      /* Send a reduced job list for functional shares so qmaster won't
         complain about modifying priority of running jobs  */

      if (fticket_info.dp == JB_Type) {
         lList *tlp = NULL;
         what = lWhat("%T(%I %I)", JB_Type, JB_job_number,
                            JB_priority);
         tlp = lSelect("", lp, NULL, what);
         /*
         ** there is a weakness in the lEnumeration Definition which leads
         ** to problems if a reduced list is run through lSelect with
         ** an lWhat build from the total descriptor of a special type
         ** e.g.
         ** what_from_whole_descr = lWhat("%T(%I %I)", JB_Type, 
         **                               JB_job_number, JB_priority);
         ** rl = lSelect("rl", l, null, what_from_whole_descr);
         ** rl2 = lSelect("rl", l, null, what_from_whole_descr);
         ** returns NULL which is probably not what we expect.
         ** The correct thing to do would be:
         ** what_from_reduced_descr = lWhat("%T(%I %I)", lGetListDescr(rl),
         **                            JB_job_number, JB_priority);
         ** rl2 = lSelect("rl", l, null, what_from_reduced_descr);
         ** 
         ** An alternative solution is to use what_all or NULL (==what_all)
         ** 
         */                         

         alp = sge_gdi(fticket_info.list_type, SGE_GDI_MOD,
                       tlp ? &tlp : &lp, NULL, NULL);
         if (tlp)
            lFreeList(tlp);
      } else {
         what = lWhat("%T(ALL)", fticket_info.dp);
         alp = sge_gdi(fticket_info.list_type, SGE_GDI_MOD, &lp, NULL, what);
      }

      qmonMessageBox(w, alp, 0);

      if (alp && lGetUlong(lFirst(alp), AN_status) != STATUS_OK)
         XmtMsgLinePrintf(fticket_message, "Failure");
      else
         XmtMsgLinePrintf(fticket_message, "Success");
      XmtMsgLineClear(fticket_message, DISPLAY_MESSAGE_DURATION); 

      
      if (rmon_mlgetl(&DEBUG_ON, GUI_LAYER) & INFOPRINT) {
         printf("---Functional Policy-------------------\n");
         lWriteListTo(lp, stdout);
         printf("_______________________________________\n");
      }

      alp = lFreeList(alp);
      lFreeWhat(what); 
   }

   /*
   **
   ** save ratio changes
   **
   */
   XmtDialogGetDialogValues(fticket_ratio, &ratio_data);

   qmonMirrorMultiAnswer(SC_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }
      
   lp = qmonMirrorList(SGE_SC_LIST);
   sep = lFirst(lp);
   if (sep) {
      ParametersToCull(sep, &ratio_data);
      
      what = lWhat("%T(ALL)", SC_Type);
      alp = qmonModList(SGE_SC_LIST, 
                        qmonMirrorListRef(SGE_SC_LIST),
                        0, &lp, NULL, what);

      qmonMessageBox(w, alp, 0);

      if (alp && lGetUlong(lFirst(alp), AN_status) != STATUS_OK)
         XmtMsgLinePrintf(fticket_message, "Failure");
      else
         XmtMsgLinePrintf(fticket_message, "Success");
      XmtMsgLineClear(fticket_message, DISPLAY_MESSAGE_DURATION); 
   
      if (rmon_mlgetl(&DEBUG_ON, GUI_LAYER) & INFOPRINT) {
         printf("---Functional Ratios-------------------\n");
         lWriteListTo(lp, stdout);
         printf("_______________________________________\n");
      }

      lFreeList(alp);
      lFreeWhat(what);
   }

/*    XtUnmanageChild(qmon_fticket); */

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static Widget qmonFTicketCreate(
Widget parent 
) {
   Widget fticket_layout, fticket_cancel, fticket_update, 
            fticket_more, fticket_ok, fticket_new, fticket_jcsc,
            fticket_usc, fticket_ussc, fticket_psc, fticket_jsc,
            fticket_usc_t, fticket_ussc_t, fticket_psc_t, fticket_jsc_t,
            fticket_jcsc_t, fticket_main_link;
   static int usc_index = 0;
   static int ussc_index = 1;
   static int psc_index = 2;
   static int jsc_index = 3;
   static int jcsc_index = 4;

   DENTER(GUI_LAYER, "qmonFTicketCreate");
   
   fticket_layout = XmtBuildQueryDialog( parent, "qmon_fticket",
                           NULL, 0,
                           "fticket_cancel", &fticket_cancel,
                           "fticket_ok", &fticket_ok,
                           "fticket_update", &fticket_update,
                           "fticket_matrix", &fticket_matrix,
                           "fticket_type", &fticket_type,
                           "fticket_more", &fticket_more,
                           "fticket_ratio", &fticket_ratio,
                           "fticket_new", &fticket_new,
                           "fticket_main_link", &fticket_main_link,
                           "fticket_message", &fticket_message,
                           NULL);

   fticket_ratio = XmtCreateLayout(fticket_layout, "fticket_ratio", NULL, 0);
   XmtDialogBindResourceList(fticket_ratio, ratio_resources, 
                              XtNumber(ratio_resources));
   XmtCreateQueryChildren(fticket_ratio, 
                          "fticket_usc", &fticket_usc,
                          "fticket_ussc", &fticket_ussc,
                          "fticket_psc", &fticket_psc,
                          "fticket_jsc", &fticket_jsc,
                          "fticket_jcsc", &fticket_jcsc,
                          "fticket_usc_t", &fticket_usc_t,
                          "fticket_ussc_t", &fticket_ussc_t,
                          "fticket_psc_t", &fticket_psc_t,
                          "fticket_jsc_t", &fticket_jsc_t,
                          "fticket_jcsc_t", &fticket_jcsc_t,
                          NULL);

   /*
   ** initialize the tFOTInfo structs
   */
   fticket_info.layout = fticket_layout;
   fticket_info.matrix = fticket_matrix;

   /*
   ** initialize the WeightData structures
   */
   WeightData[0].lock = 0;
   WeightData[0].scale = fticket_usc;
   WeightData[0].toggle = fticket_usc_t;
   WeightData[0].nr = XtNumber(WeightData);
   XtVaSetValues(fticket_usc, XmNuserData, (XtPointer) &usc_index, NULL); 
   XtVaSetValues(fticket_usc_t, XmNuserData, (XtPointer) &usc_index, NULL); 

   WeightData[1].lock = 0;
   WeightData[1].scale = fticket_ussc;
   WeightData[1].toggle = fticket_ussc_t;
   XtVaSetValues(fticket_ussc, XmNuserData, (XtPointer) &ussc_index, NULL); 
   XtVaSetValues(fticket_ussc_t, XmNuserData, (XtPointer) &ussc_index, NULL); 

   WeightData[2].lock = 0;
   WeightData[2].scale = fticket_psc;
   WeightData[2].toggle = fticket_psc_t;
   XtVaSetValues(fticket_psc, XmNuserData, (XtPointer) &psc_index, NULL); 
   XtVaSetValues(fticket_psc_t, XmNuserData, (XtPointer) &psc_index, NULL); 

   WeightData[3].lock = 0;
   WeightData[3].scale = fticket_jsc;
   WeightData[3].toggle = fticket_jsc_t;
   XtVaSetValues(fticket_jsc, XmNuserData, (XtPointer) &jsc_index, NULL); 
   XtVaSetValues(fticket_jsc_t, XmNuserData, (XtPointer) &jsc_index, NULL); 

   WeightData[4].lock = 0;
   WeightData[4].scale = fticket_jcsc;
   WeightData[4].toggle = fticket_jcsc_t;
   XtVaSetValues(fticket_jcsc, XmNuserData, (XtPointer) &jcsc_index, NULL); 
   XtVaSetValues(fticket_jcsc_t, XmNuserData, (XtPointer) &jcsc_index, NULL); 


   /*
   ** callbacks
   */
   XtAddCallback(fticket_usc, XmNvalueChangedCallback,
                     qmonScaleWeight, (XtPointer)WeightData);
   XtAddCallback(fticket_ussc, XmNvalueChangedCallback,
                     qmonScaleWeight, (XtPointer)WeightData);
   XtAddCallback(fticket_psc, XmNvalueChangedCallback,
                     qmonScaleWeight, (XtPointer)WeightData);
   XtAddCallback(fticket_jsc, XmNvalueChangedCallback,
                     qmonScaleWeight, (XtPointer)WeightData);
   XtAddCallback(fticket_jcsc, XmNvalueChangedCallback,
                     qmonScaleWeight, (XtPointer)WeightData);

   XtAddCallback(fticket_usc_t, XmNvalueChangedCallback,
                     qmonScaleWeightToggle, (XtPointer)WeightData);
   XtAddCallback(fticket_ussc_t, XmNvalueChangedCallback,
                     qmonScaleWeightToggle, (XtPointer)WeightData);
   XtAddCallback(fticket_psc_t, XmNvalueChangedCallback,
                     qmonScaleWeightToggle, (XtPointer)WeightData);
   XtAddCallback(fticket_jsc_t, XmNvalueChangedCallback,
                     qmonScaleWeightToggle, (XtPointer)WeightData);
   XtAddCallback(fticket_jcsc_t, XmNvalueChangedCallback,
                     qmonScaleWeightToggle, (XtPointer)WeightData);
                     

   XtAddCallback(fticket_main_link, XmNactivateCallback, 
                     qmonMainControlRaise, NULL);
   XtAddCallback(fticket_cancel, XmNactivateCallback, 
                     qmonFOTicketPopdown, (XtPointer)fticket_layout);
   XtAddCallback(fticket_ok, XmNactivateCallback, 
                     qmonFTOkay, NULL);
   XtAddCallback(fticket_update, XmNactivateCallback, 
                     qmonFTUpdate, NULL);
   XtAddCallback(fticket_more, XmNactivateCallback, 
                     qmonFTShowMore, (XtPointer)fticket_ratio);
   XtAddCallback(fticket_new, XmNactivateCallback, 
                     qmonFOTOpenLink, (XtPointer) &fticket_info);
   XtAddCallback(fticket_type, XmtNvalueChangedCallback, 
                     qmonFTSwitchType, NULL);
   XtAddCallback(fticket_matrix, XmNleaveCellCallback, 
                     qmonFTLeaveCell, NULL);
   XtAddCallback(fticket_matrix, XmNenterCellCallback, 
                     qmonFOTEnterCell, NULL);
   XtAddCallback(fticket_matrix, XmNtraverseCellCallback, 
                     qmonFTTraverseCell, NULL);

   XtAddEventHandler(XtParent(fticket_layout), StructureNotifyMask, False, 
                        SetMinShellSize, NULL);

   DEXIT;
   return fticket_layout;
}

/*-------------------------------------------------------------------------*/
static void qmonFTTraverseCell(
Widget w,
XtPointer cld,
XtPointer cad 
) {
/*    XbaeMatrixEnterCellCallbackStruct *cbs = */
/*       (XbaeMatrixEnterCellCallbackStruct*) cad; */
   XbaeMatrixTraverseCellCallbackStruct *cbs =
      (XbaeMatrixTraverseCellCallbackStruct*) cad;
   String str;

   DENTER(GUI_LAYER, "qmonFTEnterCell");
   str = XbaeMatrixGetCell(w, cbs->next_row, 0);

   if (!str ||  str[0] == '\0') {
      DEXIT;
      return;
   }

   str = XbaeMatrixGetCell(w, cbs->row, 0);
   if (!str || str[0] == '\0') {
      XbaeMatrixEditCell(w, 1, 1);
   }
  
   set_functional_share_percentage(w);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonFOTEnterCell(
Widget w,
XtPointer cld,
XtPointer cad 
) {
   XbaeMatrixEnterCellCallbackStruct *cbs =
      (XbaeMatrixEnterCellCallbackStruct*) cad;
   String str;

   DENTER(GUI_LAYER, "qmonFTEnterCell");
   str = XbaeMatrixGetCell(w, cbs->row, 0);

   if (!str ||  str[0] == '\0') {
      cbs->doit = False;
   }

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonFTLeaveCell(
Widget w,
XtPointer cld,
XtPointer cad 
) {
   XbaeMatrixLeaveCellCallbackStruct *cbs = 
         (XbaeMatrixLeaveCellCallbackStruct *)cad;

   char *str;

   DENTER(GUI_LAYER, "qmonFTLeaveCell");
  
   /*
   ** check if this is a valid line
   */
   str = XbaeMatrixGetCell(w, cbs->row, 0); 

   if (!str || *str == '\0') {
      DEXIT;
      return;
   }

   /*
   ** check if its a valid number
   */
   strtol(cbs->value, &str, 10);

   if (!str || *str != '\0') {
      cbs->doit = False;
   }
   else {
      set_functional_share_percentage(w);
   }   

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void set_functional_share_percentage(
Widget mw 
) {
   double total_fshare = 0;
   String str;
   int row, max_fill, last_row, max_rows;
   double percentage;
   char buf[10];
   double total_percentage = 0;
   
   DENTER(GUI_LAYER, "set_functional_share_percentage");

   /*
   ** calculate total share, but don't add last row (the sum)
   */
   max_rows = XbaeMatrixNumRows(mw);
   for (row=0; row<max_rows-1; row++) {
      str = XbaeMatrixGetCell(mw, row, 0);
      if (!str || *str == '\0')
         break;
      str = XbaeMatrixGetCell(mw, row, 1);
      total_fshare += atof(str);
   }
   
   /*
   ** fill the percentage fields
   */
   max_fill = row;
   for (row=0; row<max_fill; row++) {
      str = XbaeMatrixGetCell(mw, row, 1);
      percentage = atof(str) * 100;
      if (total_fshare)
         percentage /= total_fshare;
      else
         percentage = 0;
      sprintf(buf, "%.1f %%", percentage); 
      XbaeMatrixSetCell(mw, row, 2, buf);
      total_percentage += percentage;
   }

   last_row = max_rows-1;
   /*
   ** set the total share
   */
   XbaeMatrixSetRowBackgrounds(mw, last_row, &JobSuspPixel, 1);
   XbaeMatrixSetCell(mw, last_row, 0, XmtLocalize2(mw, "Sum", "qmon_fticket", "Sum"));
   sprintf(buf, "%.0f", total_fshare);
   XbaeMatrixSetCell(mw, last_row, 1, buf);
   sprintf(buf, "%.1f %%", total_percentage);
   XbaeMatrixSetCell(mw, last_row, 2, buf);

   DPRINTF(("total_percentage: %f\n", total_percentage));

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonFTShowMore(
Widget w,
XtPointer cld,
XtPointer cad 
) {
   static Boolean managed = False;
   Widget ratio = (Widget)cld;
   static Pixmap open;
   static Pixmap closed; 

   DENTER(GUI_LAYER, "qmonFTShowMore");
  
   if (!open) {
      open = qmonGetIcon("leftarrow");
      closed = qmonGetIcon("rightarrow");
   }
  
   if (!managed) {
      managed = True;
      XtVaSetValues(w, XmNlabelPixmap, open, NULL);
      XtManageChild(ratio);
   }
   else {
      managed = False;
      XtVaSetValues(w, XmNlabelPixmap, closed, NULL);
      XtUnmanageChild(ratio);
   }

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonFTSwitchType(
Widget w,
XtPointer cld,
XtPointer cad 
) {
   XmtChooserCallbackStruct *cbs = (XmtChooserCallbackStruct*) cad;

   DENTER(GUI_LAYER, "qmonFTSwitchType");

   switch (cbs->state) {
      case FOT_USER:
         fticket_info.field0 = UP_name;
         fticket_info.field1 = UP_fshare;
         fticket_info.list_type = SGE_USER_LIST;
         fticket_info.dp = UP_Type;
         break;
      
      case FOT_PROJECT:
         fticket_info.field0 = UP_name;
         fticket_info.field1 = UP_fshare;
         fticket_info.list_type = SGE_PROJECT_LIST;
         fticket_info.dp = UP_Type;
         break;
      
      case FOT_USERSET:
         fticket_info.field0 = US_name;
         fticket_info.field1 = US_fshare;
         fticket_info.list_type = SGE_USERSET_LIST;
         fticket_info.dp = US_Type;
         break;

      case FOT_JOB:
         fticket_info.field0 = JB_job_number;
         fticket_info.field1 = JB_priority;
         fticket_info.list_type = SGE_JOB_LIST;
         fticket_info.dp = JB_Type;
         break;

      case FOT_JOBCLASS:
         fticket_info.field0 = QU_qname;
         fticket_info.field1 = QU_fshare;
         fticket_info.list_type = SGE_CQUEUE_LIST;
         fticket_info.dp = QU_Type;
         break;
   }

   /*
   ** update the data
   */
   qmonFTUpdate(w, NULL, NULL);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonFTUpdate(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *lp = NULL;
   lList *alp = NULL;
   lCondition *where = NULL;
   lEnumeration *what = NULL;

   DENTER(GUI_LAYER, "qmonFTUpdate");
   
   qmonMirrorMultiAnswer(SC_T | l2s(fticket_info.list_type), &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }
   lp = qmonMirrorList(fticket_info.list_type);

   /*
   ** filter usersets
   */
   if (fticket_info.list_type == SGE_USERSET_LIST) {
      where = lWhere("%T(%I m= %u)", US_Type, US_type, US_DEPT);
      what = lWhat("%T(ALL)", US_Type);
      lp = lSelect("Departments", lp, where, what);
      lFreeWhere(where);
      lFreeWhat(what);
   } 

   /*
   ** filter queues
   */
   if (fticket_info.list_type == SGE_CQUEUE_LIST) {
      where = lWhere("%T(%I != %s)", QU_Type, QU_qname, "template");
      what = lWhat("%T(ALL)", QU_Type);
      lp = lSelect("Queues without template", lp, where, what);
      lFreeWhere(where);
      lFreeWhat(what);
   } 
   lPSortList(lp, "%I+", fticket_info.field0);
   qmonFOTCullToMatrix(fticket_info.matrix, lp, 
                        fticket_info.field0, fticket_info.field1);
   if (fticket_info.list_type == SGE_USERSET_LIST || 
         fticket_info.list_type == SGE_CQUEUE_LIST)
      lp = lFreeList(lp);

   /*
   ** set the percentage
   */
   set_functional_share_percentage(fticket_info.matrix);

   /*
   ** update the ratio data
   */
   lp = qmonMirrorList(SGE_SC_LIST);
   CullToParameters(&ratio_data, lFirst(lp));
   XmtDialogSetDialogValues(fticket_ratio, &ratio_data);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonOTUpdate(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *lp = NULL;
   lList *alp = NULL;
   lCondition *where = NULL;
   lEnumeration *what = NULL;
   u_long32 selector = 0;

   DENTER(GUI_LAYER, "qmonOTUpdate");
   
   switch (oticket_info.list_type) {
      case SGE_USERSET_LIST:
         selector = USERSET_T;
         break;
      case SGE_CQUEUE_LIST:
         selector = QUEUE_T;
         break;
         
      case SGE_JOB_LIST:
         selector = JOB_T;
         break;
         
      case SGE_USER_LIST:
         selector = USER_T;
         break;
         
      case SGE_PROJECT_LIST:
         selector = PROJECT_T;
         break;
   }        
   qmonMirrorMultiAnswer(selector, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }
      
   lp = qmonMirrorList(oticket_info.list_type);

   /*
   ** filter usersets
   */
   if (oticket_info.list_type == SGE_USERSET_LIST) {
      where = lWhere("%T(%I m= %u)", US_Type, US_type, US_DEPT);
      what = lWhat("%T(ALL)", US_Type);
      lp = lSelect("Departments", lp, where, what);
      where = lFreeWhere(where);
      what = lFreeWhat(what);
   } 

   /*
   ** filter queues
   */
   if (oticket_info.list_type == SGE_CQUEUE_LIST) {
      where = lWhere("%T(%I != %s)", QU_Type, QU_qname, "template");
      what = lWhat("%T(ALL)", QU_Type);
      lp = lSelect("Queues without template", lp, where, what);
      where = lFreeWhere(where);
      what = lFreeWhat(what);
   } 
   lPSortList(lp, "%I+", oticket_info.field0);
   qmonFOTCullToMatrix(oticket_info.matrix, lp, 
                        oticket_info.field0, oticket_info.field1);
   if (oticket_info.list_type == SGE_USERSET_LIST || 
         oticket_info.list_type == SGE_CQUEUE_LIST)
      lp = lFreeList(lp);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonOTOkay(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *lp = NULL;
   lList *alp = NULL;
   lEnumeration *what = NULL;
   lCondition *where = NULL;

   DENTER(GUI_LAYER, "qmonOTOkay");

   qmonMirrorMultiAnswer(oticket_info.list_type, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }
   lp = qmonMirrorList(oticket_info.list_type);
   /*
   ** filter queues
   */
   if (oticket_info.list_type == SGE_CQUEUE_LIST) {
      where = lWhere("%T(%I != %s)", QU_Type, QU_qname, "template");
      what = lWhat("%T(ALL)", QU_Type);
      lp = lSelect("Queues without template", lp, where, what);
      lFreeWhere(where);
      lFreeWhat(what);
   } 

   if (qmonFOTMatrixToCull(oticket_matrix, lp, oticket_info.field0, 
         oticket_info.field1)){

      /* Send a reduced job list for override tickets so it won't
         complain about modifying priority of running jobs  */

      if (oticket_info.dp == JB_Type) {
         lList *tlp = NULL;
         what = lWhat("%T(%I %I)", JB_Type, JB_job_number,
                            JB_override_tickets);
         tlp = lSelect("", lp, NULL, what);
         alp = sge_gdi(oticket_info.list_type, SGE_GDI_MOD,
                       tlp ? &tlp : &lp, NULL, NULL);
         if (tlp)
            lFreeList(tlp);
      } else {
         what = lWhat("%T(ALL)", oticket_info.dp);
         alp = sge_gdi(oticket_info.list_type, SGE_GDI_MOD, &lp, NULL, what);
      }

      qmonMessageBox(w, alp, 0);
      
      if (alp && lGetUlong(lFirst(alp), AN_status) != STATUS_OK)
         XmtMsgLinePrintf(oticket_message, "Failure");
      else
         XmtMsgLinePrintf(oticket_message, "Success");
      XmtMsgLineClear(oticket_message, DISPLAY_MESSAGE_DURATION); 

      if (rmon_mlgetl(&DEBUG_ON, GUI_LAYER) & INFOPRINT) {
         printf("----Override Policy--------------------\n");
         lWriteListTo(lp, stdout);
         printf("_______________________________________\n");
      }

      lFreeList(alp);
      lFreeWhat(what);
   }

/*    XtUnmanageChild(qmon_oticket); */

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static Widget qmonOTicketCreate(
Widget parent 
) {
   Widget oticket_layout, oticket_cancel, oticket_update, 
            oticket_ok, oticket_new, oticket_main_link;

   DENTER(GUI_LAYER, "qmonOTicketCreate");
   
   oticket_layout = XmtBuildQueryDialog( parent, "qmon_oticket",
                           NULL, 0,
                           "oticket_cancel", &oticket_cancel,
                           "oticket_update", &oticket_update,
                           "oticket_matrix", &oticket_matrix,
                           "oticket_ok", &oticket_ok,
                           "oticket_type", &oticket_type,
                           "oticket_new", &oticket_new,
                           "oticket_main_link", &oticket_main_link,
                           "oticket_message", &oticket_message,
                           NULL);

   /*
   ** initialize the tFOTInfo structs
   */
   oticket_info.layout = oticket_layout;
   oticket_info.matrix = oticket_matrix;

   XtAddCallback(oticket_main_link, XmNactivateCallback, 
                     qmonMainControlRaise, NULL);
   XtAddCallback(oticket_cancel, XmNactivateCallback, 
                     qmonFOTicketPopdown, (XtPointer)oticket_layout);
   XtAddCallback(oticket_type, XmtNvalueChangedCallback, 
                     qmonOTSwitchType, NULL);
   XtAddCallback(oticket_new, XmNactivateCallback, 
                     qmonFOTOpenLink, (XtPointer) &oticket_info);
   XtAddCallback(oticket_ok, XmNactivateCallback, 
                     qmonOTOkay, NULL);
   XtAddCallback(oticket_update, XmNactivateCallback, 
                     qmonOTUpdate, NULL);
   XtAddCallback(oticket_matrix, XmNenterCellCallback, 
                     qmonFOTEnterCell, NULL);



   XtAddEventHandler(XtParent(oticket_layout), StructureNotifyMask, False, 
                        SetMinShellSize, NULL);

   DEXIT;
   return oticket_layout;
}

/*-------------------------------------------------------------------------*/
static void qmonOTSwitchType(
Widget w,
XtPointer cld,
XtPointer cad 
) {
   XmtChooserCallbackStruct *cbs = (XmtChooserCallbackStruct*) cad;

   DENTER(GUI_LAYER, "qmonOTSwitchType");

   switch (cbs->state) {
      case FOT_USER:
         oticket_info.field0 = UP_name;
         oticket_info.field1 = UP_oticket;
         oticket_info.list_type = SGE_USER_LIST;
         oticket_info.dp = UP_Type;
         break;
      
      case FOT_PROJECT:
         oticket_info.field0 = UP_name;
         oticket_info.field1 = UP_oticket;
         oticket_info.list_type = SGE_PROJECT_LIST;
         oticket_info.dp = UP_Type;
         break;
      
      case FOT_USERSET:
         oticket_info.field0 = US_name;
         oticket_info.field1 = US_oticket;
         oticket_info.list_type = SGE_USERSET_LIST;
         oticket_info.dp = US_Type;
         break;

      case FOT_JOB:
         oticket_info.field0 = JB_job_number;
         oticket_info.field1 = JB_override_tickets;
         oticket_info.list_type = SGE_JOB_LIST;
         oticket_info.dp = JB_Type;
         break;

      case FOT_JOBCLASS:
         oticket_info.field0 = QU_qname;
         oticket_info.field1 = QU_oticket;
         oticket_info.list_type = SGE_CQUEUE_LIST;
         oticket_info.dp = QU_Type;
         break;
   }

   /*
   ** update the data
   */
   qmonOTUpdate(w, NULL, NULL);


   DEXIT;
}



/*-------------------------------------------------------------------------*/
static void qmonFOTCullToMatrix(
Widget matrix,
lList *lp,
int nm0,
int nm1 
) {
   lListElem *ep;
   StringConst name;
   int tickets;
   char buf[128];
   char buf2[128];
   int row;
   int max_rows;

   DENTER(GUI_LAYER, "qmonFOTCullToMatrix");

   /*
   ** delete old matrix entries
   */
   XtVaSetValues(matrix, XmNcells, NULL, NULL);
   if (!lp) {
      DEXIT;
      return;
   }   

   max_rows = XbaeMatrixNumRows(matrix);

   for (ep = lFirst(lp), row = 0; ep; ep = lNext(ep), row++) {
      if (nm0 == JB_job_number) {
         sprintf(buf2, "%d", (int)lGetUlong(ep, nm0));
         name = buf2;
      }
      else
         name = lGetString(ep, nm0);
      tickets = (int)lGetUlong(ep, nm1);
      sprintf(buf, "%d", tickets);
      if (row == max_rows-1) {
         XbaeMatrixAddRows(matrix,
                           max_rows-1,
                           NULL,       /* empty rows           */
                           NULL,       /* no lables            */
                           NULL,       /* no different colors  */
                           1);         /* we add 1 row         */
         max_rows++;
      }
      /* FIX_CONST_GUI */
      XbaeMatrixSetCell(matrix, row, 0, name ? (const String) name : "");
      XbaeMatrixSetCell(matrix, row, 1, buf);
   }

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static Boolean qmonFOTMatrixToCull(
Widget matrix,
lList *lp,
int nm0,
int nm1 
) {
   lListElem *ep;
   String col1, col2;
   int row;
   int max_rows;

   DENTER(GUI_LAYER, "qmonFOTMatrixToCull");

   if (!lp) {
      DEXIT;
      return False;
   }

   max_rows = XbaeMatrixNumRows(matrix);

   for (row=0; row<max_rows-1; row++) {
      col1 = XbaeMatrixGetCell(matrix, row, 0);
      col2 = XbaeMatrixGetCell(matrix, row, 1);
      if (col1 && col1[0] != '\0') {
         if (nm0 == JB_job_number)
            ep = lGetElemUlong(lp, nm0, (u_long32) atoi(col1));
         else
            ep = lGetElemStr(lp, nm0, col1);
         lSetUlong(ep, nm1, col2 ? atoi(col2) : 0);
      }
      else
         continue;
   }

   DEXIT;
   return True;
}

/*-------------------------------------------------------------------------*/
static void ParametersToCull(
lListElem *sep,
tFREntry *data 
) {
   DENTER(GUI_LAYER, "ParametersToCull");
   
   /* USER */
   lSetDouble(sep, SC_weight_user, ((double)data->fticket_user)/1000);
      
   /* USERSET */
   lSetDouble(sep, SC_weight_department, ((double)data->fticket_userset)/1000);

   /* PROJECT */
   lSetDouble(sep, SC_weight_project, ((double)data->fticket_project)/1000);

   /* JOB */
   lSetDouble(sep, SC_weight_job, ((double)data->fticket_job)/1000);

   /* JOBCLASS */
   lSetDouble(sep, SC_weight_jobclass, ((double)data->fticket_jobclass)/1000);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void CullToParameters(
tFREntry *data,
lListElem *sep 
) {

   DENTER(GUI_LAYER, "CullToParameters");

   /* USER */
   data->fticket_user = (int)(lGetDouble(sep, SC_weight_user)*1000);
   
   /* USERSET */
   data->fticket_userset = (int)(lGetDouble(sep, SC_weight_department)*1000);
   
   /* PROJECT */
   data->fticket_project = (int)(lGetDouble(sep, SC_weight_project)*1000);
   
   /* JOB */
   data->fticket_job = (int)(lGetDouble(sep, SC_weight_job)*1000);
   
   /* JOBCLASS */
   data->fticket_jobclass = (int)(lGetDouble(sep, SC_weight_jobclass)*1000);
   
   if (data->fticket_user+data->fticket_userset+data->fticket_project+
         data->fticket_job + data->fticket_jobclass != 1000) {
      data->fticket_user = 200;
      data->fticket_userset = 200;
      data->fticket_project = 200;
      data->fticket_job = 200;
      data->fticket_jobclass = 200;
   }

   /*
   ** set the UsageWeightData
   */
   WeightData[0].weight = data->fticket_user;
   WeightData[1].weight = data->fticket_userset;
   WeightData[2].weight = data->fticket_project;
   WeightData[3].weight = data->fticket_job;
   WeightData[4].weight = data->fticket_jobclass;
   
   DEXIT;
}

         
