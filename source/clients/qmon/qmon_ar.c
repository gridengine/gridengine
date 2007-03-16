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



#define QALTER_RUNNING


#include <stdio.h>
#include <stdlib.h>
#include <Xm/ToggleB.h>

#include <Xmt/Xmt.h>
#include <Xmt/Create.h>
#include <Xmt/Icon.h>
#include <Xmt/Dialogs.h>
#include <Xmt/Chooser.h>
#include <Xmt/InputField.h>

#include "Matrix.h"
#include "Tab.h"
#include "qmon_rmon.h"
#include "qmon_cull.h"
#include "qmon_ar.h"
#include "qmon_arsub.h"
#include "qmon_arcustom.h"
#include "qmon_comm.h"
#include "qmon_appres.h"
#include "qmon_submit.h"
#include "qmon_timer.h"
#include "qmon_globals.h"
#include "qmon_browser.h"
#include "qmon_message.h"
#include "qmon_init.h"
#include "qmon_request.h"
#include "qmon_util.h"
#include "basis_types.h"
#include "sge_signal.h"
#include "sge_time.h"
#include "sge_all_listsL.h"
#include "sge_answer.h"
#include "sge_utility.h"
#include "sge_feature.h"
#include "qmon_matrix.h"
#include "sge_range.h"
#include "parse.h"
#include "sge_dstring.h"
#include "sge_schedd_text.h"
#include "sgeee.h"
#include "sge_support.h"
#include "sge_range.h"
#include "sge_object.h"
#include "sge_centry.h"
#include "sge_advance_reservation.h"
#include "sge_gdi_ctx.h"

extern sge_gdi_ctx_class_t *ctx;

enum {
   AR_DISPLAY_MODE_RUNNING,
   AR_DISPLAY_MODE_PENDING
};

typedef struct _tAskHoldInfo {
   Boolean blocked;
   XmtButtonType button;
   Widget AskHoldDialog;
   Widget AskHoldFlags;
   Widget AskHoldTasks;
} tAskHoldInfo;
   

static Widget qmon_ar = 0;
static Widget ar_running_ars = 0;
static Widget ar_pending_ars = 0;
static Widget current_matrix = 0;
static Widget ar_customize = 0;
static Widget ar_delete = 0;
static Widget ar_select_all = 0;
static Widget ar_qalter = 0;
static Widget force_toggle = 0;


/*-------------------------------------------------------------------------*/
static void qmonCreateARControl(Widget parent);
static void qmonARToMatrix(Widget w, lListElem *ar, int mode, int row);
static void qmonARFolderChange(Widget w, XtPointer cld, XtPointer cad);
static void qmonDeleteARCB(Widget w, XtPointer cld, XtPointer cad);
static void qmonSelectAllARCB(Widget w, XtPointer cld, XtPointer cad);
static void qmonARPopdown(Widget w, XtPointer cld, XtPointer cad);
static void qmonARStartUpdate(Widget w, XtPointer cld, XtPointer cad);
static void qmonARStopUpdate(Widget w, XtPointer cld, XtPointer cad);
static void qmonARHandleEnterLeave(Widget w, XtPointer cld, XEvent *ev, Boolean *ctd);
static void qmonARShowBrowserInfo(dstring *info, lListElem *jep);
/* static void qmonARChangeState(Widget w, XtPointer cld, XtPointer cad); */
static void qmonARQalter(Widget w, XtPointer cld, XtPointer cad);
static void qmonARNoEdit(Widget w, XtPointer cld, XtPointer cad);
static Pixel qmonARStateToColor(Widget w, lListElem *jep);
static Boolean qmonDeleteARForMatrix(Widget w, Widget matrix, lList **local);
static lList* qmonARBuildSelectedList(Widget matrix, lDescr *dp, int nm);
/* static void qmonResizeCB(Widget w, XtPointer cld, XtPointer cad); */
/* static void qmonARSort(Widget w, XtPointer cld, XtPointer cad); */
/*-------------------------------------------------------------------------*/
static int field_sort_by;
static int field_sort_direction;
static int arnum_sort_direction;

/*-------------------------------------------------------------------------*/
/* P U B L I C                                                             */
/*-------------------------------------------------------------------------*/
void qmonARPopup(Widget w, XtPointer cld, XtPointer cad)
{
   lList *alp = NULL;

   DENTER(GUI_LAYER, "qmonARPopup");

   /* set busy cursor */
   XmtDisplayBusyCursor(w);

   if (!qmon_ar) {
      qmonMirrorMultiAnswer(AR_T, &alp);
      if (alp) {
         qmonMessageBox(w, alp, 0);
         lFreeList(&alp);
         /* set normal cursor */
         XmtDisplayDefaultCursor(w);
         DEXIT;
         return;
      }
         
      qmonCreateARControl(AppShell);
      /*
      ** create the ar customize dialog
      */
      qmonCreateARCU(qmon_ar, NULL);
      /* 
      ** set the close button callback 
      ** set the icon and iconName
      */
      XmtCreatePixmapIcon(qmon_ar, qmonGetIcon("toolbar_ar"), None);
      XtVaSetValues(qmon_ar, XtNiconName, "qmon:AR Control", NULL);
      XmtAddDeleteCallback(qmon_ar, XmDO_NOTHING, qmonARPopdown,  NULL);
      XtAddEventHandler(qmon_ar, StructureNotifyMask, False, 
                        SetMinShellSize, NULL);
      XtAddEventHandler(qmon_ar, StructureNotifyMask, False, 
                        SetMaxShellSize, NULL);
   }

   updateARListCB(qmon_ar, NULL, NULL);

   xmui_manage(qmon_ar);

   /* set normal cursor */
   XmtDisplayDefaultCursor(w);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
void qmonARReturnMatrix(
Widget *run_m,
Widget *pen_m
) {
   DENTER(GUI_LAYER, "qmonARReturnMatrix");

   *run_m = ar_running_ars;
   *pen_m = ar_pending_ars;

   DEXIT;
}

/*-------------------------------------------------------------------------*/
void updateARListCB(Widget w, XtPointer cld, XtPointer cad)
{
   lList *alp = NULL;

   /* set busy cursor */
   XmtDisplayBusyCursor(w);

   qmonMirrorMultiAnswer(AR_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      lFreeList(&alp);
      /* set default cursor */
      XmtDisplayDefaultCursor(w);
      return;
   }

   updateARList();

   /* set default cursor */
   XmtDiscardButtonEvents(w);
   XmtDisplayDefaultCursor(w);
}


/*-------------------------------------------------------------------------*/
/* P R I V A T E                                                           */
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
static void qmonARPopdown(Widget w, XtPointer cld, XtPointer cad)
{
   DENTER(GUI_LAYER, "qmonARPopdown");

   xmui_unmanage(qmon_ar);
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonARStartUpdate(Widget w, XtPointer cld, XtPointer cad)
{
   DENTER(GUI_LAYER, "qmonARStartUpdate");
  
   qmonTimerAddUpdateProc(AR_T, "updateARList", updateARList);
   qmonStartTimer(AR_T);
   
   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonARStopUpdate(Widget w, XtPointer cld, XtPointer cad)
{
   DENTER(GUI_LAYER, "qmonARStopUpdate");
  
   qmonStopTimer(AR_T);
   qmonTimerRmUpdateProc(AR_T, "updateARList");
   
   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonCreateARControl(
Widget parent 
) {
   Widget ar_submit, ar_update, ar_done,
          ar_main_link, ar_pending, ar_folder;
   static Widget pw[2];
   static Widget rw[2];
   
   DENTER(GUI_LAYER, "qmonCreateARControl");

   qmon_ar = XmtBuildQueryToplevel( parent, 
                                     "qmon_ar",
                                     "ar_running_ars", &ar_running_ars,
                                     "ar_pending_ars", &ar_pending_ars,
                                     "ar_delete", &ar_delete,
                                     "ar_select_all", &ar_select_all,
                                     "ar_qalter", &ar_qalter,
                                     "ar_update", &ar_update,
                                     "ar_customize", &ar_customize,
                                     "ar_submit", &ar_submit,
                                     "ar_done", &ar_done,
                                     "ar_main_link", &ar_main_link,
                                     "ar_folder", &ar_folder,
                                     "ar_pending", &ar_pending,
                                     "ar_force",  &force_toggle,
                                     NULL);
   pw[0] = ar_running_ars;
   pw[1] = NULL;
   rw[0] = ar_pending_ars;
   rw[1] = NULL;


   /* start the needed timers and the corresponding update routines */
   XtAddCallback(qmon_ar, XmNpopupCallback, 
                     qmonARStartUpdate, NULL);
   XtAddCallback(qmon_ar, XmNpopdownCallback,
                     qmonARStopUpdate, NULL);
                     
   XtAddCallback(ar_folder, XmNvalueChangedCallback, 
                     qmonARFolderChange, NULL);
   XmTabSetTabWidget(ar_folder, ar_pending, True);

   XtAddCallback(ar_delete, XmNactivateCallback, 
                     qmonDeleteARCB, NULL);
   XtAddCallback(ar_select_all, XmNactivateCallback, 
                     qmonSelectAllARCB, NULL);
   XtAddCallback(ar_update, XmNactivateCallback, 
                     updateARListCB, NULL);
   XtAddCallback(ar_customize, XmNactivateCallback,  
                     qmonPopupARCU, NULL); 
   XtAddCallback(ar_submit, XmNactivateCallback, 
                     qmonARSubPopup, NULL);
/*    XtAddCallback(ar_qalter, XmNactivateCallback,  */
/*                      qmonARQalter, NULL); */
   XtAddCallback(ar_done, XmNactivateCallback, 
                     qmonARPopdown, NULL);
   XtAddCallback(ar_main_link, XmNactivateCallback, 
                     qmonMainControlRaise, NULL);
/*    XtAddCallback(ar_running_ars, XmNresizeColumnCallback,  */
/*                      qmonResizeCB, NULL); */
   XtAddCallback(ar_running_ars, XmNenterCellCallback, 
                     qmonARNoEdit, NULL);
   XtAddCallback(ar_pending_ars, XmNenterCellCallback, 
                     qmonARNoEdit, NULL);
   XtAddCallback(ar_running_ars, XmNselectCellCallback, 
                     qmonMatrixSelect, (XtPointer) rw);
   XtAddCallback(ar_pending_ars, XmNselectCellCallback, 
                     qmonMatrixSelect, (XtPointer) pw);
#if 0                     
   XtAddCallback(ar_pending_ars, XmNlabelActivateCallback, 
                     qmonARSort, NULL);
   XtAddCallback(ar_running_ars, XmNlabelActivateCallback, 
                     qmonARSort, NULL);
#endif
   /* Event Handler to display additional ar info */
   XtAddEventHandler(ar_running_ars, PointerMotionMask, 
                     False, qmonARHandleEnterLeave,
                     NULL); 
   XtAddEventHandler(ar_pending_ars, PointerMotionMask, 
                     False, qmonARHandleEnterLeave,
                     NULL); 

   current_matrix = ar_pending_ars;
#if 0   
   /* initialising sort order to decreasing priority then increasing ar number */
   field_sort_by = SGEJ_priority;
   field_sort_direction = SGEJ_sort_decending;
   arnum_sort_direction = SGEJ_sort_ascending; 
#endif   

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonARToMatrix(
Widget w,
lListElem *ar,
int mode,
int row                                                                       
) {
   int rows, columns;
   int col;
   String *rowa = NULL; 
   Pixel color;
   
   DENTER(GUI_LAYER, "qmonARToMatrix");

   rows = XbaeMatrixNumRows(w);
   columns = XbaeMatrixNumColumns(w);

   /*
   ** running ars
   */
   if (mode == AR_DISPLAY_MODE_RUNNING) {
      rowa = PrintARField(ar, columns);
      if (row < rows)
         XbaeMatrixDeleteRows(w, row, 1);
      XbaeMatrixAddRows(w, row, rowa, NULL, NULL, 1);
      /*
      ** do show the different ar states we show the ar id in a different           ** color
      */
      color = qmonARStateToColor(w, ar);
      XbaeMatrixSetRowColors(w, row, &color, 1);
   }

   /*
   ** pending ars
   */
   if (mode == AR_DISPLAY_MODE_PENDING) {
      if (row < rows)
         XbaeMatrixDeleteRows(w, row, 1);
      rowa = PrintARField(ar, columns);
      XbaeMatrixAddRows(w, row, rowa, NULL, NULL, 1);
      /*
      ** do show the different ar states we show the ar id in a different           ** color
      */
      color = qmonARStateToColor(w, ar);
      XbaeMatrixSetRowColors(w, row, &color, 1);
   }

   /*
   ** free row array
   */
   if (rowa) {
      for (col=0; col<columns; col++)
         XtFree((char*)rowa[col]);
      XtFree((char*)rowa);
   }

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static Pixel qmonARStateToColor(
Widget w,
lListElem *ar 
) {
   Pixel color;
   static Pixel fg = 0;
   int state;

   DENTER(GUI_LAYER, "qmonARStateToColor");

   if (!fg) {
      XtVaGetValues(w, XmNforeground, &fg, NULL);
   }   

   if (!ar) {
      DEXIT;
      return fg;
   }
      

   state = (int)lGetUlong(ar, AR_state);

   /*
   ** the order is important
   */

   color = fg;

#if 0
   if (state & AR_RUNNING)
      color = ARRunningPixel;

   if (state & AR_WAITING)
      color = fg;

   if (state & AR_DELETED)
      color = fg;

   if (state & AR_ERROR)
      color = fg;
#endif      

   DEXIT;
   return color;

}

/*-------------------------------------------------------------------------*/
static void qmonARNoEdit(
Widget w,
XtPointer cld,
XtPointer cad  
) {
   XbaeMatrixEnterCellCallbackStruct *cbs =
         (XbaeMatrixEnterCellCallbackStruct*) cad;

   DENTER(GUI_LAYER, "qmonARNoEdit");

   cbs->doit = False;
   
   DEXIT;
}

#if 0
/*-------------------------------------------------------------------------*/
static void qmonARSort(
Widget w,
XtPointer cld,
XtPointer cad  
) {
   XbaeMatrixLabelActivateCallbackStruct *cbs = 
            (XbaeMatrixLabelActivateCallbackStruct *) cad;
   /* SGEJ_state is not a typo - PrintStatus uses both JAT_status and JAT_state */
   int column_nm[] = {SGEJ_ar_number, SGEJ_priority, SGEJ_ar_name, SGEJ_owner, SGEJ_state, SGEJ_master_queue};
   /* 
   ** Mapping the columns to these internal resources:
   ** JB_ar_number    (Ulong)
   ** JAT_prio         (Double)
   ** JB_ar_name      (String)
   ** JB_owner         (String)
   ** JAT_state        (Ulong)
   ** JAT_master_queue (String)
   */
   DENTER(GUI_LAYER, "qmonARSort");
 
   DPRINTF(("ARSort = cbs->column = %d\n", cbs->column));
   
   /* not coping beyond 6th column */
   if ( cbs->column > 5) {
      DEXIT;
      return;
   }
   /* 
   ** Here is what we are going to do for sorting:
   ** if the user clicks on a column, we'll sort ascending,
   ** doing a 2nd time toggles to decending; except
   ** we always toggle on the ar number the first time
   ** it is selected from a different column, since it is
   ** always a secondary sort key and if it is selected
   ** the user most likely wants a toggle.
   */
      
   if (column_nm[cbs->column] == field_sort_by) {
      /* toggling sort order */
      field_sort_direction = !field_sort_direction;     
   } else {
      field_sort_by = column_nm[cbs->column];
      field_sort_direction = SGEJ_sort_ascending;
      /* switching from another field to ar number also toggles */
      if (field_sort_by == SGEJ_ar_number) {
         field_sort_direction = !arnum_sort_direction;
      }
   }

   /* recording the last chosen ar number sort direction */
   if (field_sort_by == SGEJ_ar_number) {
      arnum_sort_direction = field_sort_direction;
   }
   updateARList();
    
   DEXIT;
}

#endif

/*----------------------------------------------------------------------------*/
void updateARList(void)
{
   lList *pl = NULL;
   lList *rl = NULL;
   lListElem *ar = NULL;
   lCondition *where_run = NULL;
   lEnumeration *what = NULL;
   int row = 0, pow = 0;
   int current_rows = 0, desired_rows = 0; /* used at the end to shrink tabs */
         
   DENTER(GUI_LAYER, "updateARList");
   
   /*
   ** has dialog been created
   */
   if (!qmon_ar) {
      DEXIT;
      return;
   }
   
   what = lWhat("%T(ALL)", AR_Type);
   where_run = lWhere("%T(%I == %u)", AR_Type, AR_state, AR_RUNNING);
 
   rl = lCopyList("rl", qmonMirrorList(SGE_AR_LIST));

   /*
   ** loop over the ars and the tasks
   */
   XbaeMatrixDisableRedisplay(ar_running_ars);
   XbaeMatrixDisableRedisplay(ar_pending_ars);
 
   /*
   ** reset matrices
   */
   XtVaSetValues( ar_running_ars,
                  XmNcells, NULL,
                  NULL);
   XtVaSetValues( ar_pending_ars,
                  XmNcells, NULL,
                  NULL);
   /*
   ** update the ar entries
   */
   lSplit(&rl, &pl, "rl", where_run);

#if 0
   /*
   ** sort the ars according to start time
   */
   if (lGetNumberOfElem(pl)>0 ) {
      sgeee_sort_ars_by(&arl, field_sort_by, field_sort_direction, arnum_sort_direction);
   }
#endif
   /*
   ** running ars
   */
   for_each(ar, rl) {
      qmonARToMatrix(ar_running_ars, ar, AR_DISPLAY_MODE_RUNNING, row);
      row++;                                                            
   }            

#if 0
   /*
   ** sort the ars according to start time
   */
   if (lGetNumberOfElem(pl)>0 ) {
      sgeee_sort_ars_by(&arl, field_sort_by, field_sort_direction, arnum_sort_direction);
   }
#endif
   /*
   ** pending ars
   */
   for_each(ar, pl) {
      qmonARToMatrix(ar_pending_ars, ar, AR_DISPLAY_MODE_PENDING, pow);
      pow++;                                                            
   }            

   /*
   ** free the where/what
   */
   lFreeWhere(&where_run);
   lFreeWhat(&what);
   lFreeList(&pl);
   lFreeList(&rl);

   /* shrinking excessive vertical size of tabs from previous runs of updateARList() */  
   current_rows = XbaeMatrixNumRows(ar_running_ars);
   desired_rows = MAX(row, 20);
   if (current_rows > desired_rows) {
     XbaeMatrixDeleteRows(ar_running_ars, desired_rows, current_rows - desired_rows);
   }   
   current_rows = XbaeMatrixNumRows(ar_pending_ars);
   desired_rows = MAX(pow, 20);
   if (current_rows > desired_rows) {
     XbaeMatrixDeleteRows(ar_pending_ars, desired_rows, current_rows - desired_rows);
   }

   XbaeMatrixEnableRedisplay(ar_running_ars, True);
   XbaeMatrixEnableRedisplay(ar_pending_ars, True);
 
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static Boolean qmonDeleteARForMatrix(
Widget w,
Widget matrix,
lList **local 
) {
   int i;
   int rows;
   lList *ardl = NULL;
   lList *alp = NULL;
   char *str = NULL;
   int force;

   DENTER(GUI_LAYER, "qmonDeleteARForMatrix");

/*
   if (!user_list) {
      lAddElemStr(&user_list, ST_name, "*", ST_Type);
   }   
*/   
   force = XmToggleButtonGetState(force_toggle);

   /* 
   ** create delete ar list 
   */
   rows = XbaeMatrixNumRows(matrix);
   for (i=0; i<rows; i++) {
      /* is this row selected */ 
      if (XbaeMatrixIsRowSelected(matrix, i)) {
         str = XbaeMatrixGetCell(matrix, i, 0);
         if ( str && *str != '\0' ) { 
            DPRINTF(("ARId to delete: %s\n", str));
            if (isdigit(str[0])) {
               lAddElemUlong(&ardl, AR_id, atoi(str), AR_Type);
            } else {
               lAddElemStr(&ardl, AR_name, str, AR_Type);
            }
         }
      }
   }

   if (ardl) {
      alp = ctx->gdi(ctx, SGE_AR_LIST, SGE_GDI_DEL, &ardl, NULL, NULL);
      qmonMessageBox(w, alp, 0);
      lFreeList(&alp);

      updateARList();
      XbaeMatrixDeselectAll(matrix);

      lFreeList(&ardl);
   } else {
      qmonMessageShow(w, True, "@{There are no ARs selected !}");
      DEXIT;
      return False;
   }

   DEXIT;
   return True;
}

/*-------------------------------------------------------------------------*/
static void qmonSelectAllARCB(
Widget w,
XtPointer cld,
XtPointer cad 
) {
   
   DENTER(GUI_LAYER, "qmonSelectAllARCB");

   /* set busy cursor */
   XmtDisplayBusyCursor(w);

   XbaeMatrixSelectAll(current_matrix);

   /* set normal cursor */
   XmtDisplayDefaultCursor(w);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonDeleteARCB(
Widget w,
XtPointer cld,
XtPointer cad 
) {
   Boolean status;
   
   DENTER(GUI_LAYER, "qmonDeleteARCB");

   /*
   ** we have to delete the running ars independently
   ** they are not removed from the local list only their
   ** state is set to JDELETED, this is marked with a NULL pointer
   ** instead of a valid local list address
   */

   /* set busy cursor */
   XmtDisplayBusyCursor(w);

   status = qmonDeleteARForMatrix(w, ar_running_ars, NULL);
   if (status)
      XbaeMatrixDeselectAll(ar_running_ars);

   status = qmonDeleteARForMatrix(w, ar_pending_ars,
                           qmonMirrorListRef(SGE_AR_LIST));
   if (status)
      XbaeMatrixDeselectAll(ar_pending_ars);

   updateARListCB(w, NULL, NULL);

   /* set normal cursor */
   XmtDisplayDefaultCursor(w);

   DEXIT;
}

#if 0
/*-------------------------------------------------------------------------*/
static void qmonARChangeState(
Widget w,
XtPointer cld,
XtPointer cad 
) {
   lList *jl = NULL;
   lList *rl = NULL;
   lList *alp = NULL;
   int force = 0;
   int action = (int)(long)cld;
   Widget force_toggle;
   
   DENTER(GUI_LAYER, "qmonARChangeState");

   if (action == QI_DO_CLEARERROR) {
      rl = qmonARBuildSelectedList(ar_running_ars, ST_Type, ST_name);
      
      jl = qmonARBuildSelectedList(ar_pending_ars, ST_Type, ST_name);

      if (!jl && rl) {
         jl = rl;
      }
      else if (rl) {
         lAddList(jl, &rl);
      }
   }
   else {
      force_toggle = XmtNameToWidget(w, "*ar_force");

      force = XmToggleButtonGetState(force_toggle);
      /*
      ** state changes only for running ars
      */
      jl = qmonARBuildSelectedList(ar_running_ars, ST_Type, ST_name);
   }

   if (jl) {

      alp = qmonChangeStateList(SGE_AR_LIST, jl, force, action); 

      qmonMessageBox(w, alp, 0);

      updateARList();
      XbaeMatrixDeselectAll(ar_running_ars);
      XbaeMatrixDeselectAll(ar_pending_ars);

      lFreeList(&jl);
      lFreeList(&alp);
   }
   else {
      qmonMessageShow(w, True, "@{There are no ars selected !}");
   }
   
   DEXIT;
}

#endif

/*-------------------------------------------------------------------------*/
/* descriptor must contain AR_id at least                                  */
/*-------------------------------------------------------------------------*/
static lList* qmonARBuildSelectedList(matrix, dp, nm)
Widget matrix;
lDescr *dp;
int nm;
{
   lList *jl = NULL;

#if 0
   int i;
   int rows;
   lListElem *jep = NULL;
   String str;

   DENTER(GUI_LAYER, "qmonARBuildSelectedList");

   if (nm != JB_ar_number && nm != ST_name) {
      DEXIT;
      return NULL;
   }

   rows = XbaeMatrixNumRows(matrix);
      
   for (i=0; i<rows; i++) {
      /* is this row selected */ 
      if (XbaeMatrixIsRowSelected(matrix, i)) {
         str = XbaeMatrixGetCell(matrix, i, 0);
         DPRINTF(("AR to alter: %s\n", str));
         /*
         ** list with describtion ar id's and the priority value 
         */
         if ( str && (*str != '\0') ) { 
            switch (nm) { 
               case JB_ar_number:
                  {
                     u_long32 start = 0, end = 0, step = 0;
                     lListElem *idp = NULL;
                     lList *ipp = NULL;
                     lList *jat_list = NULL;
                     lList *alp = NULL;
                     /* TODO: SG: check, if this is correct */
                     if (sge_parse_artasks(&ipp, &idp, str, &alp, false, NULL) == -1) {
                        lFreeList(&alp);
                        DEXIT;
                        return NULL;
                     }
                     lFreeList(&alp);

                     if (ipp) {
                        for_each(idp, ipp) {
                           lListElem *ip;

                           jep = lAddElemUlong(&jl, JB_ar_number, 
                                    atol(lGetString(idp, ID_str)), dp);
                           lSetList(jep, JB_ja_structure, 
                                 lCopyList("", lGetList(idp, ID_ja_structure)));
                           ar_get_submit_task_ids(jep, &start, &end, &step); 
                           for_each(ip, lGetList(idp, ID_ja_structure)) {
                              start = lGetUlong(ip, RN_min);
                              end = lGetUlong(ip, RN_max);
                              step = lGetUlong(ip, RN_step);
                              for (; start<=end; start+=step) {
                                 lAddElemUlong(&jat_list, JAT_task_number, 
                                                   start, JAT_Type);
                              }
                           }
                        }
                        if (lGetNumberOfElem(jat_list) == 0) {
                           lAddElemUlong(&jat_list, JAT_task_number, 
                                         1, JAT_Type);
                        }
                        lSetList(jep, JB_ja_tasks, jat_list);
                        lFreeList(&ipp);
                     }
                  }
                  break;
               case ST_name:
                  jep = lAddElemStr(&jl, ST_name, str, dp);
                  lSetList(jep, JB_ja_tasks, NULL);
                  break;
            }
         }
      }
   }
   DEXIT;
#endif   

   return jl;
}


/*-------------------------------------------------------------------------*/
static void qmonARQalter(Widget w, XtPointer cld, XtPointer cad)
{

#if 0

#ifdef QALTER_RUNNING
   lList *rl = NULL;
#endif
   lList *pl = NULL;
   tSubmitMode data;

   DENTER(GUI_LAYER, "qmonARQalter");
#ifdef QALTER_RUNNING
   rl = qmonARBuildSelectedList(ar_running_ars, JB_Type, JB_ar_number);
#endif
   /*
   ** qalter works for pending  & running ars
   */
   pl = qmonARBuildSelectedList(ar_pending_ars, JB_Type, JB_ar_number);

#ifdef QALTER_RUNNING
   if (!pl && rl) {
      pl = rl;
   } else if (rl) {
      lAddList(pl, &rl);
   }
#endif
   /*
   ** call the submit dialog for pending ars
   */
   if (pl && lGetNumberOfElem(pl) == 1) {
      data.mode = SUBMIT_QALTER_PENDING;
      data.ar_id = lGetUlong(lFirst(pl), JB_ar_number);
      qmonSubmitPopup(w, (XtPointer)&data, NULL);
      lFreeList(&pl);
      XbaeMatrixDeselectAll(ar_pending_ars);
#ifdef QALTER_RUNNING
      XbaeMatrixDeselectAll(ar_running_ars);
#endif
   } else {
#ifndef QALTER_RUNNING
      qmonMessageShow(w, True, "@{Select one pending ar for Qalter !}");
#else
      qmonMessageShow(w, True, "@{Select one ar for Qalter !}");
#endif
   }
   
   DEXIT;

#endif   

}



/*-------------------------------------------------------------------------*/
static void qmonARHandleEnterLeave(
Widget w,
XtPointer cld,
XEvent *ev,
Boolean *ctd 
) {
   /*
   int root_x, root_y, pos_x, pos_y;
   Window root, child;
   unsigned int keys_buttons;
   */
   char line[BUFSIZ];
   static int prev_row = -1;
   int row, col;
   String str;
   int ar_nr;
   lListElem *ar;
   
   DENTER(GUI_LAYER, "qmonARHandleEnterLeave");
   
   switch (ev->type) {
      case MotionNotify:
         if (qmonBrowserObjectEnabled(BROWSE_AR)) {
            /* 
            ** XQueryPointer(XtDisplay(w), XtWindow(w), 
            **            &root, &child, &root_x, &root_y,
            **            &pos_x, &pos_y, &keys_buttons);
            */
            if (XbaeMatrixGetEventRowColumn(w, ev, &row, &col)) {
               if ( row != prev_row ) {
                  prev_row = row;
                  /* locate the ar */
                  str = XbaeMatrixGetCell(w, row, 0);
                  if (str && *str != '\0') {
                     ar_nr = atoi(str);
                     ar = ar_list_locate(qmonMirrorList(SGE_AR_LIST), 
                                          (u_long32)ar_nr);
                     if (ar) {
                        dstring ar_info = DSTRING_INIT;
                        sprintf(line, "+++++++++++++++++++++++++++++++++++++++++++\n");  
                        qmonBrowserShow(line);
                        qmonARShowBrowserInfo(&ar_info, ar);      
                        qmonBrowserShow(sge_dstring_get_string(&ar_info));
                        sge_dstring_free(&ar_info);
                     }
                  }
               }
            }
         }
         break;
   }
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonARShowBrowserInfo(
dstring *info,
lListElem *ar 
) {

   DENTER(GUI_LAYER, "qmonARShowBrowserInfo");

   sge_dstring_sprintf(info, "%-30.30s"sge_u32"\n", "AR Id:  ", lGetUlong(ar, AR_id));
   sge_dstring_sprintf_append(info, "%-30.30s%s\n", "AR name:", lGetString(ar, AR_name));
   sge_dstring_sprintf_append(info, "%-30.30s%s\n", "AR account:",
                  lGetString(ar, AR_account) ? 
                  lGetString(ar, AR_account): "-NA-");
   sge_dstring_sprintf_append(info, "%-30.30s%s\n", "AR owner:", 
                  lGetString(ar, AR_owner));
   sge_dstring_sprintf_append(info, "%-30.30s%s\n", "AR checkpoint Object:", 
            lGetString(ar, AR_checkpoint_name) ? 
            lGetString(ar, AR_checkpoint_name) : "-NA-");

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonARFolderChange(Widget w, XtPointer cld, XtPointer cad)
{
   XmTabCallbackStruct *cbs = (XmTabCallbackStruct *) cad;

   DENTER(GUI_LAYER, "qmonARFolderChange");
   
   DPRINTF(("%s\n", XtName(cbs->tab_child)));

   if (!strcmp(XtName(cbs->tab_child), "ar_pending")) {
      current_matrix=ar_pending_ars;
   }

   if (!strcmp(XtName(cbs->tab_child), "ar_running")) {
      current_matrix=ar_running_ars;
   }

   updateARList();

   DEXIT;
}

