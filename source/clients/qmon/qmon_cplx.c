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
#define AUTOMATIC_UPDATE   0

#include <stdio.h>
#include <ctype.h>

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Text.h>
#include <Xm/ScrollBar.h>

#include <Xmt/Xmt.h>
#include <Xmt/Create.h>
#include <Xmt/Layout.h>
#include <Xmt/Chooser.h>
#include <Xmt/Dialog.h>
#include <Xmt/Dialogs.h>
#include <Xmt/MsgLine.h>
#include <Xmt/InputField.h>
#include <Xmt/Procedures.h>

#include "Matrix.h"
#include "sge_all_listsL.h"
#include "sge_gdi.h"
#include "commlib.h"
#include "sge_complex_schedd.h"
#include "sge_answer.h"
#include "sge_centry.h"
#include "spool/classic/read_write_complex.h"
#include "qmon_proto.h"
#include "qmon_rmon.h"
#include "qmon_cull.h"
#include "qmon_util.h"
#include "qmon_appres.h"
#include "qmon_timer.h"
#include "qmon_comm.h"
#include "qmon_matrix.h"
#include "qmon_message.h"
#include "qmon_cplx.h"
#include "qmon_globals.h"
#include "AskForItems.h"

enum _cplx_mode {
   CPLX_ADD_MODE,
   CPLX_MODIFY_MODE
};

static int add_mode = 0;

static Widget qmon_cplx = 0;
static Widget attr_add = 0;
static Widget attr_del = 0;
static Widget attr_load = 0;
static Widget attr_save = 0;
static Widget attr_mx = 0;
static Widget attr_aname = 0;
static Widget attr_avalue = 0;
static Widget attr_atype = 0;
static Widget attr_ashort = 0;
static Widget attr_arel = 0;
static Widget attr_areq = 0;
static Widget attr_aconsumable = 0;
static Widget attr_adefault = 0;
static Widget cplx_attributes_title = 0;


/*-------------------------------------------------------------------------*/
static void qmonPopdownCplxConfig(Widget w, XtPointer cld, XtPointer cad);
static Widget qmonCreateCplxConfig(Widget parent);
static Widget qmonCreateCplxAsk(Widget parent);
static void qmonCplxSelect(Widget w, XtPointer cld, XtPointer cad);
static void qmonCplxChange(Widget w, XtPointer cld, XtPointer cad);
static void qmonCplxCancel(Widget w, XtPointer cld, XtPointer cad);
static void qmonCplxOk(Widget w, XtPointer cld, XtPointer cad);
static void qmonCplxShowCplx(Widget matrix, char *cx_name);
static void qmonCplxLoadAttr(Widget w, XtPointer cld, XtPointer cad);
static void qmonCplxSaveAttr(Widget w, XtPointer cld, XtPointer cad);
static void qmonCplxAddAttr(Widget w, XtPointer cld, XtPointer cad);
static void qmonCplxDelAttr(Widget w, XtPointer cld, XtPointer cad);
static void qmonCplxNoEdit(Widget w, XtPointer cld, XtPointer cad);
static void qmonCplxSelectAttr(Widget w, XtPointer cld, XtPointer cad);
static void qmonCplxScrollBar(Widget w, XtPointer cld, XtPointer cad);

#if AUTOMATIC_UPDATE
static void qmonCplxStartUpdate(Widget w, XtPointer cld, XtPointer cad);
static void qmonCplxStopUpdate(Widget w, XtPointer cld, XtPointer cad);
#endif
/*-------------------------------------------------------------------------*/
void qmonPopupCplxConfig(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget shell;
   lList *alp = NULL;
   lList *entries = NULL;

   DENTER(GUI_LAYER, "qmonPopupCplxConfig");

   /* set busy cursor */
   XmtDisplayBusyCursor(w);

   if (!qmon_cplx) {
      shell = XmtGetTopLevelShell(w);
      qmon_cplx = qmonCreateCplxConfig(shell);
      XmtAddDeleteCallback(shell, XmDO_NOTHING, 
                              qmonPopdownCplxConfig, NULL);
      XtAddEventHandler(XtParent(qmon_cplx), StructureNotifyMask, False, 
                        SetMinShellSize, NULL);

   } 
   XSync(XtDisplay(qmon_cplx), 0);
   XmUpdateDisplay(qmon_cplx);

   qmonMirrorMultiAnswer(CENTRY_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      /* set default cursor */
      XmtDisplayDefaultCursor(w);
      DEXIT;
      return;
   }

   entries = qmonMirrorList(SGE_CENTRY_LIST);

   /*
   ** fill the values into the matrix
   */
   qmonSetCE_Type(attr_mx, entries, CE_TYPE_FULL);
      
   XtManageChild(qmon_cplx);
   XRaiseWindow(XtDisplay(XtParent(qmon_cplx)), 
                  XtWindow(XtParent(qmon_cplx)));
 
   /* set default cursor */
   XmtDisplayDefaultCursor(w);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
/* P R I V A T E                                                           */
/*-------------------------------------------------------------------------*/
static Widget qmonCreateCplxConfig(
Widget parent 
) {
   Widget cplx_layout, cplx_commit, cplx_reset, cplx_main_link;

   DENTER(GUI_LAYER, "qmonCreateCplxConfig");
   
   cplx_layout = XmtBuildQueryDialog( parent, "qmon_cplx",
                           NULL, 0,
                           "cplx_commit", &cplx_commit,
                           "cplx_reset", &cplx_reset,
                           "cplx_main_link", &cplx_main_link,
                           "attr_add", &attr_add,
                           "attr_del", &attr_del,
                           "attr_load", &attr_load,
                           "attr_save", &attr_save,
                           "attr_mx", &attr_mx,
                           "attr_aname", &attr_aname,
                           "attr_ashort", &attr_ashort,
                           "attr_avalue", &attr_avalue,
                           "attr_atype", &attr_atype,
                           "attr_arel", &attr_arel,
                           "attr_areq", &attr_areq,
                           "attr_aconsumable", &attr_aconsumable,
                           "attr_adefault", &attr_adefault,
                           NULL);

   XtAddCallback(cplx_main_link, XmNactivateCallback, 
                     qmonMainControlRaise, NULL);

   XtAddCallback(cplx_commit, XmNactivateCallback, 
                     qmonPopdownCplxConfig, NULL);
   XtAddCallback(cplx_reset, XmNactivateCallback, 
                     qmonPopdownCplxConfig, NULL);

   /*
   ** register callback procedures
   */
   XmtVaRegisterCallbackProcedures(
         "ComplexAttributeDelete", qmonCplxDelAttr, XtRWidget,
         "ComplexAttributeAdd", qmonCplxAddAttr, XtRWidget,
         "ComplexAttributeLoad", qmonCplxLoadAttr, NULL,
         "ComplexAttributesSave", qmonCplxSaveAttr, NULL,
         NULL);

   DEXIT;
   return cplx_layout;
}

/*-------------------------------------------------------------------------*/
static void qmonPopdownCplxConfig(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   DENTER(GUI_LAYER, "qmonPopdownCplxConfig");

   XtUnmanageChild(qmon_cplx);
   XtPopdown(XtParent(qmon_cplx));

   DEXIT;
}

#ifdef ANDRE
/*-------------------------------------------------------------------------*/
static void qmonCplxOk(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   static lEnumeration *what = NULL;
   lList *entries = NULL;
   lList *lp = NULL;
   lList *alp = NULL;
   char *cplx_name;
   XmString xcplx_name;
   
   DENTER(GUI_LAYER, "qmonCplxOk");

   cplx_name = XmtInputFieldGetString(cplx_ask_name_w);
   
   if (!cplx_name || cplx_name[0] == '\0') {
      qmonMessageShow(w, True, "Name of Complex required !");
      DEXIT;
      return;
   }
   
   if (!(lp = lCreateElemList("CC", CX_Type, 1))) {
      DEXIT;
      return;
   }

   lSetString(lFirst(lp), CX_name, qmon_trim(cplx_name));
   entries = qmonGetCE_Type(attr_mx);
   lSetList(lFirst(lp), CX_entries, entries);

   if (!what)
      what = lWhat("%T(ALL)", CX_Type);

   if (add_mode == CPLX_ADD_MODE)
      alp = qmonAddList(SGE_COMPLEX_LIST, qmonMirrorListRef(SGE_COMPLEX_LIST), 
                        CX_name, &lp, NULL, what);
   else
      alp = qmonModList(SGE_COMPLEX_LIST, qmonMirrorListRef(SGE_COMPLEX_LIST), 
                        CX_name, &lp, NULL, what);
      
   qmonMessageBox(w, alp, 0);

   if (lGetUlong(lFirst(alp), AN_status) == STATUS_OK) {
      updateCplxList();
      xcplx_name = XmtCreateXmString(cplx_name);
      XmListSelectItem(cplx_names, xcplx_name, True);
      XmStringFree(xcplx_name);
      XtUnmanageChild(cplx_ask_layout);
   }
   lp = lFreeList(lp);
   alp = lFreeList(alp);

   DEXIT;
}

#endif


/*-------------------------------------------------------------------------*/
static void qmonCplxAddAttr(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget matrix = (Widget) cld;
   String new_str[1][8];
   int rows = 0;
   int num_columns = 8;
   int row, i;
   String str;
   int req_state;

   DENTER(GUI_LAYER, "qmonCplxAddAttr");

   /*
   ** check input 
   */
   if (is_empty_word(XmtInputFieldGetString(attr_aname))) {
      qmonMessageShow(matrix, True, "Name required !");
      DEXIT;
      return;
   }
   if (is_empty_word(XmtInputFieldGetString(attr_ashort))) {
      qmonMessageShow(matrix, True, "Shortcut required !");
      DEXIT;
      return;
   }
   if (XmtChooserGetState(attr_atype) == 0 && 
         is_empty_word(XmtInputFieldGetString(attr_avalue))) {
      qmonMessageShow(matrix, True, "Value required !");
      DEXIT;
      return;
   }
   if (is_empty_word(XmtInputFieldGetString(attr_adefault))) {
      qmonMessageShow(matrix, True, "Default required !");
      DEXIT;
      return;
   }
      
   /*
   ** get the values
   */
   new_str[0][0] = XtNewString(XmtInputFieldGetString(attr_aname));
   new_str[0][1] = XtNewString(XmtInputFieldGetString(attr_ashort));
   new_str[0][2] = XtNewString(map_type2str(1 +
                        XmtChooserGetState(attr_atype)));
   new_str[0][3] = XtNewString(XmtInputFieldGetString(attr_avalue));
   new_str[0][4] = XtNewString(map_op2str(1 +
                        XmtChooserGetState(attr_arel)));
   req_state = XmtChooserGetState(attr_areq); 
   switch (req_state) {
      case 1:  
         new_str[0][5] = XtNewString("YES");
         break;
      case 2:  
         new_str[0][5] = XtNewString("FORCED");
         break;
      default:
         new_str[0][5] = XtNewString("NO");
   }
      
   new_str[0][6] = XtNewString(XmtChooserGetState(attr_aconsumable) ? "YES" : "NO");
   new_str[0][7] = XtNewString(XmtInputFieldGetString(attr_adefault));
         

   /*
   ** add to attribute matrix, search if item already exists
   */
   rows = XbaeMatrixNumRows(matrix);

   for (row=0; row<rows; row++) {
      /* get name str */
      str = XbaeMatrixGetCell(matrix, row, 0);
      if (!str || (str && !strcmp(str, new_str[0][0])) ||
            (str && is_empty_word(str))) 
         break;
   }
   
   if (row !=rows)
      XbaeMatrixDeleteRows(matrix, row, 1);

   XbaeMatrixAddRows(matrix, row, new_str[0], NULL, NULL, 1);

   /* reset and jump to attr_aname */
   XbaeMatrixDeselectAll(matrix);
   /* refresh the matrix */
   XbaeMatrixRefresh(matrix);
   
/*    XmtInputFieldSetString(cplx_ask_name_w, ""); */
   XmtInputFieldSetString(attr_aname, "");
   XmtInputFieldSetString(attr_ashort, "");
   XmtChooserSetState(attr_atype, 0, False);
   XmtInputFieldSetString(attr_avalue, "");
   XmtChooserSetState(attr_arel, 0, False);
   XmtChooserSetState(attr_areq, 0, False);
   XmtChooserSetState(attr_aconsumable, 0, False);
   XmtInputFieldSetString(attr_adefault, "");
   XmProcessTraversal(attr_aname, XmTRAVERSE_CURRENT);


   for (i=0; i<num_columns; i++) {
      XtFree((char*)new_str[0][i]);
      new_str[0][i] = NULL;
   }
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonCplxDelAttr(
Widget w,
XtPointer cld,
XtPointer cad 
) {
   Widget matrix = (Widget) cld;
   int rows;
   int i;
   int rows_to_delete = 0;
   

   DENTER(GUI_LAYER, "qmonCplxDelAttr");

   rows = XbaeMatrixNumRows(matrix);

   for (i=0; i<rows; i++)
      if (XbaeMatrixIsRowSelected(matrix, i))
         rows_to_delete++;

   i = 0;
   while (i<rows) {
      if (XbaeMatrixIsRowSelected(matrix, i)) {
         XbaeMatrixDeleteRows(matrix, i, 1);
         rows--;
      }
      else
         i++;
   }

   XbaeMatrixAddRows(matrix, rows, NULL, NULL, NULL, rows_to_delete);

   /* reset attribute line */
   XbaeMatrixDeselectAll(matrix);
   XmtInputFieldSetString(attr_aname, "");
   XmtInputFieldSetString(attr_ashort, "");
   XmtChooserSetState(attr_atype, 0, False);
   XmtInputFieldSetString(attr_avalue, "");
   XmtChooserSetState(attr_arel, 0, False);
   XmtChooserSetState(attr_areq, 0, False);
   XmtChooserSetState(attr_aconsumable, 0, False);
   XmtInputFieldSetString(attr_adefault, "");
   
   /* refresh the matrix */
   XbaeMatrixRefresh(matrix);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonCplxLoadAttr(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   static char filename[BUFSIZ];
   static char directory[BUFSIZ];
   int status;
   lListElem *cep;
   lList *entries;
   lList *alp = NULL;
 
   DENTER(GUI_LAYER, "qmonCplxLoadAttr");

   status = XmtAskForFilename(w, "@{File Selection}",
                              "@{Please type or select a filename}",
                              NULL, NULL,
                              filename, sizeof(filename),
                              directory, sizeof(directory),
                              "*", 0,
                              NULL);

   if (status == True) {
      entries = read_cmplx(filename, "dummy", &alp);
      /* fill the matrix with the values from list */ 
      if (alp) {
         qmonMessageBox(w, alp, 0);
         alp = lFreeList(alp);
         DEXIT;
         return;
      }

      /*
      ** fill the values into the matrix
      */
      qmonSetCE_Type(attr_mx, entries, CE_TYPE_FULL);
      lFreeList(entries);
   }        
   
   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonCplxSaveAttr(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   static char filename[BUFSIZ];
   static char directory[BUFSIZ];
   int status;
   lList *entries;
   lList *alp = NULL;
 
   DENTER(GUI_LAYER, "qmonCplxSaveAttr");

   status = XmtAskForFilename(w, "@{File Selection}",
                              "@{Please type or select a filename}",
                              NULL, NULL,
                              filename, sizeof(filename),
                              directory, sizeof(directory),
                              "*", 0,
                              NULL);

   if (status == True) {
      entries = qmonGetCE_Type(attr_mx);
      /* Save Cplx Dialog Box */
      write_cmplx(0, filename, entries, NULL, &alp); 
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      entries = lFreeList(entries);
   }
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonCplxNoEdit(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   XbaeMatrixEnterCellCallbackStruct *cbs = 
            (XbaeMatrixEnterCellCallbackStruct*) cad;

   DENTER(GUI_LAYER, "qmonCplxNoEdit");

   cbs->doit = False;
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonCplxSelectAttr(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   XbaeMatrixSelectCellCallbackStruct *cbs = 
            (XbaeMatrixSelectCellCallbackStruct*) cad;
   int i;
   String str;
   int type, relop;

   DENTER(GUI_LAYER, "qmonCplxSelectAttr");

   if (cbs->num_params && !strcmp(cbs->params[0], "begin")) {
      /* name */
      str = XbaeMatrixGetCell(w, cbs->row, 0);
      XmtInputFieldSetString(attr_aname, str ? str : ""); 

      /* shortcut */
      str = XbaeMatrixGetCell(w, cbs->row, 1);
      XmtInputFieldSetString(attr_ashort, str ? str : "");
      
      /* type */
      str = XbaeMatrixGetCell(w, cbs->row, 2);
      type = 0;
      for (i=TYPE_FIRST; !type && i<=TYPE_DOUBLE; i++) {
         if (!strcasecmp(str, map_type2str(i)))
            type = i-1;
      }
      XmtChooserSetState(attr_atype, type, False); 

      /* value */
      str = XbaeMatrixGetCell(w, cbs->row, 3);
      XmtInputFieldSetString(attr_avalue, str ? str : "");
      
      /* relop */
      str = XbaeMatrixGetCell(w, cbs->row, 4);
      relop = 0;
      for (i=CMPLXEQ_OP; !relop && i<=CMPLXNE_OP; i++) {
         if (!strcasecmp(str, map_op2str(i)))
            relop = i-1;
      }
      XmtChooserSetState(attr_arel, relop, False); 

      str = XbaeMatrixGetCell(w, cbs->row, 5);
      if (str && !strcmp(str, "YES")) 
         XmtChooserSetState(attr_areq, 1, False);
      else if (str && !strcmp(str, "FORCED"))
         XmtChooserSetState(attr_areq, 2, False);
      else
         XmtChooserSetState(attr_areq, 0, False);

      str = XbaeMatrixGetCell(w, cbs->row, 6);
      if (str && !strcmp(str, "YES")) 
         XmtChooserSetState(attr_aconsumable, 1, False);
      else
         XmtChooserSetState(attr_aconsumable, 0, False);

      /* default */
      str = XbaeMatrixGetCell(w, cbs->row, 7);
      XmtInputFieldSetString(attr_adefault, str ? str : ""); 
   }
   
   DEXIT;
}
