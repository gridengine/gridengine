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
#include "def.h"
#include "sge_complex.h"
#include "sge_complex_schedd.h"
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
static Widget cplx_names = 0;
static Widget cplx_ask_layout = 0;
static Widget cplx_ask_mx = 0;
static Widget cplx_ask_name_w = 0;
static Widget cplx_ask_aname = 0;
static Widget cplx_ask_avalue = 0;
static Widget cplx_ask_atype = 0;
static Widget cplx_ask_ashort = 0;
static Widget cplx_ask_arel = 0;
static Widget cplx_ask_areq = 0;
static Widget cplx_ask_aconsumable = 0;
static Widget cplx_ask_adefault = 0;
static Widget cplx_attributes_title = 0;


/*-------------------------------------------------------------------------*/
static void qmonPopdownCplxConfig(Widget w, XtPointer cld, XtPointer cad);
static Widget qmonCreateCplxConfig(Widget parent);
static Widget qmonCreateCplxAsk(Widget parent);
static void qmonCplxDelete(Widget w, XtPointer cld, XtPointer cad);
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
      /*
      ** create ask layout
      */
      cplx_ask_layout = qmonCreateCplxAsk(qmon_cplx);
      XtAddEventHandler(XtParent(cplx_ask_layout), StructureNotifyMask, False, 
                        SetMinShellSize, NULL);
      XtAddEventHandler(XtParent(cplx_ask_layout), StructureNotifyMask, False, 
                        SetMaxShellSize, NULL);

   } 
   XSync(XtDisplay(qmon_cplx), 0);
   XmUpdateDisplay(qmon_cplx);

   qmonMirrorMultiAnswer(COMPLEX_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      /* set default cursor */
      XmtDisplayDefaultCursor(w);
      DEXIT;
      return;
   }
   updateCplxList();

   XtManageChild(qmon_cplx);
   XRaiseWindow(XtDisplay(XtParent(qmon_cplx)), 
                  XtWindow(XtParent(qmon_cplx)));
 
   /* set default cursor */
   XmtDisplayDefaultCursor(w);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
void updateCplxList(void)
{
   lList *cl;
   XmString *selectedItems;
   Cardinal selectedItemCount;
   Cardinal itemCount;
   
   DENTER(GUI_LAYER, "updateCplxList");

   cl = qmonMirrorList(SGE_COMPLEX_LIST);
   lPSortList(cl, "%I+", CX_name);
   UpdateXmListFromCull(cplx_names, XmFONTLIST_DEFAULT_TAG, cl, CX_name);

   XtVaGetValues( cplx_names,
                  XmNselectedItems, &selectedItems,
                  XmNselectedItemCount, &selectedItemCount,
                  XmNitemCount, &itemCount,
                  NULL);

   if (selectedItemCount)
      XmListSelectItem(cplx_names, selectedItems[0], True);
   else if (itemCount)
      XmListSelectPos(cplx_names, 1, True);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
/* P R I V A T E                                                           */
/*-------------------------------------------------------------------------*/
static Widget qmonCreateCplxConfig(
Widget parent 
) {
   Widget cplx_layout, cplx_add, cplx_delete, cplx_modify, cplx_done,
          cplx_attributes, hsb1, hsb2, cplx_main_link;
   char buf[256];

   DENTER(GUI_LAYER, "qmonCreateCplxConfig");
   
   cplx_layout = XmtBuildQueryDialog( parent, "qmon_cplx",
                           NULL, 0,
                           "cplx_names", &cplx_names,
                           "cplx_attributes", &cplx_attributes,
                           "cplx_add", &cplx_add,
                           "cplx_modify", &cplx_modify,
                           "cplx_delete", &cplx_delete,
                           "cplx_done", &cplx_done,
                           "cplx_attributes_title", &cplx_attributes_title,
                           "cplx_main_link", &cplx_main_link,
                           NULL);

   XtAddCallback(cplx_main_link, XmNactivateCallback, 
                     qmonMainControlRaise, NULL);

   XtVaGetValues( XtParent(cplx_attributes), 
                  XmNhorizontalScrollBar, &hsb1, 
                  NULL);
   XtVaGetValues( XtParent(cplx_attributes_title), 
                  XmNhorizontalScrollBar, &hsb2, 
                  NULL);
   XtUnmanageChild(hsb2);

   XtAddCallback(hsb1, XmNdragCallback, 
                  qmonCplxScrollBar, (XtPointer) hsb2);
   XtAddCallback(hsb1, XmNincrementCallback, 
                  qmonCplxScrollBar, (XtPointer) hsb2);
   XtAddCallback(hsb1, XmNdecrementCallback, 
                  qmonCplxScrollBar, (XtPointer) hsb2);
   XtAddCallback(hsb1, XmNpageIncrementCallback, 
                  qmonCplxScrollBar, (XtPointer) hsb2);
   XtAddCallback(hsb1, XmNpageDecrementCallback, 
                  qmonCplxScrollBar, (XtPointer) hsb2);
   XtAddCallback(hsb1, XmNtoTopCallback, 
                  qmonCplxScrollBar, (XtPointer) hsb2);
   XtAddCallback(hsb1, XmNtoBottomCallback, 
                  qmonCplxScrollBar, (XtPointer) hsb2);

   XtAddCallback(cplx_names, XmNbrowseSelectionCallback, 
                     qmonCplxSelect, (XtPointer) cplx_attributes);
   XtAddCallback(cplx_done, XmNactivateCallback, 
                     qmonPopdownCplxConfig, NULL);

   XtAddCallback(cplx_add, XmNactivateCallback, 
                     qmonCplxChange, (XtPointer)CPLX_ADD_MODE);
   XtAddCallback(cplx_modify, XmNactivateCallback, 
                     qmonCplxChange, (XtPointer)CPLX_MODIFY_MODE);
   XtAddCallback(cplx_delete, XmNactivateCallback, 
                     qmonCplxDelete, NULL); 

   /*
   ** set title bar
   */
   sprintf(buf, "%-20.20s %-10.10s %-10.10s %-20.20s %-5.5s %-7.7s %-10.10s %-20.20s",
             "NAME", "SHORTCUT", "TYPE", "VALUE  ", "RELOP",
             "REQ   ", "CONSUMABLE", "DEFAULT");
   XmTextSetString(cplx_attributes_title, buf);
   DEXIT;
   return cplx_layout;
}

/*-------------------------------------------------------------------------*/
static void qmonCplxScrollBar(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct*) cad;
   Widget slave = (Widget)cld;
   int max_val = 0;

   DENTER(GUI_LAYER, "qmonCplxScrollBar");

   XtVaGetValues( slave,
                  XmNmaximum, &max_val,
                  NULL);

   if (cbs->value < max_val)   
      XmScrollBarSetValues(slave, cbs->value, 1, 1, 1, True);

/*    printf("Scrollbar: Value %d     Pixel %d\n", cbs->value, cbs->pixel); */

   DEXIT;
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

#if AUTOMATIC_UPDATE

/*-------------------------------------------------------------------------*/
static void qmonCplxStartUpdate(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   DENTER(GUI_LAYER, "qmonCplxStartUpdate");
  
   qmonTimerAddUpdateProc(COMPLEX_T, "updateCplxList", updateCplxList);
   qmonStartTimer(COMPLEX);
   
   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonCplxStopUpdate(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   DENTER(GUI_LAYER, "qmonCplxStopUpdate");
  
   qmonStopTimer(COMPLEX);
   qmonTimerRmUpdateProc(COMPLEX_T, "updateCplxList");
   
   DEXIT;
}

#endif


/*-------------------------------------------------------------------------*/
static void qmonCplxSelect(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   XmListCallbackStruct *cbs = (XmListCallbackStruct*) cad;
   Widget text_field = (Widget) cld;
   char *cxname;
   lListElem *ep;
   lList *attr_list;
   lListElem *attr;
   char buf[1024];
   XmTextPosition pos;
   
   DENTER(GUI_LAYER, "qmonCplxSelect");

   if (! XmStringGetLtoR(cbs->item, XmFONTLIST_DEFAULT_TAG, &cxname)) {
      DPRINTF(("XmStringGetLtoR failed !\n"));
      DEXIT;
      return;
   }
   ep = lGetElemStr(qmonMirrorList(SGE_COMPLEX_LIST), CX_name, cxname);
   XtFree((char*)cxname);

   /*
   ** fill cplx text field
   */
   if (ep) {
      XmTextDisableRedisplay(text_field);
      pos = 0;
      XmTextSetString(text_field, "");
      attr_list = lGetList(ep, CX_entries);
      for_each(attr, attr_list) {
            sprintf(buf, "%-20.20s %-10.10s %-10.10s %-20.20s %-5.5s %-7.7s %-10.10s %-20.20s\n",
            lGetString(attr, CE_name), lGetString(attr, CE_shortcut),
            map_type2str(lGetUlong(attr, CE_valtype)), 
            lGetString(attr, CE_stringval), 
            map_op2str(lGetUlong(attr, CE_relop)),
            lGetUlong(attr, CE_forced) ? "FORCED" : 
               (lGetUlong(attr, CE_request) ? "YES" : "NO"),
            lGetUlong(attr, CE_consumable) ? "YES" : "NO",
            lGetString(attr, CE_default) ? lGetString(attr, CE_default) : "");
            XmTextInsert(text_field, pos, buf);
            pos += strlen(buf);
      }
      XmTextShowPosition(text_field, 0);
      XmTextEnableRedisplay(text_field);
   }
   XmTextShowPosition(cplx_attributes_title, 0);
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonCplxChange(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   XmString *xcnames;
   Cardinal xcnum;
   String cstr;
   long mode = (long) cld;

   DENTER(GUI_LAYER, "qmonCplxChange");

   if (mode == CPLX_MODIFY_MODE) {
      /*
      ** on opening the dialog fill in the old values
      */
      XtVaGetValues( cplx_names,
                     XmNselectedItems, &xcnames,
                     XmNselectedItemCount, &xcnum,
                     NULL);
      
      if (xcnum == 1 && 
            XmStringGetLtoR(xcnames[0], XmFONTLIST_DEFAULT_TAG, &cstr)) {
         XmtInputFieldSetString(cplx_ask_name_w, cstr);
         XtVaSetValues( cplx_ask_name_w,
                        XmNeditable, False,
                        NULL);
         qmonCplxShowCplx(cplx_ask_mx, cstr);
         XtFree((char*)cstr);
         add_mode = CPLX_MODIFY_MODE;
      } else {
         mode = CPLX_ADD_MODE;
      }
   } 

   if (mode == CPLX_ADD_MODE) {
      XtVaSetValues( cplx_ask_name_w,
                     XmNeditable, True,
                     NULL);
      XmtInputFieldSetString(cplx_ask_name_w, "");
      XmtInputFieldSetString(cplx_ask_aname, "");
      XmtInputFieldSetString(cplx_ask_ashort, "");
      XmtChooserSetState(cplx_ask_atype, 0, False);
      XmtInputFieldSetString(cplx_ask_avalue, "");
      XmtChooserSetState(cplx_ask_arel, 0, False);
      XmtChooserSetState(cplx_ask_areq, 0, False);
      XmtChooserSetState(cplx_ask_aconsumable, 0, False);
      XmtInputFieldSetString(cplx_ask_adefault, "");
      qmonCplxShowCplx(cplx_ask_mx, NULL);
      add_mode = CPLX_ADD_MODE;
   }

   XtManageChild(cplx_ask_layout);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonCplxDelete(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *lp = NULL;
   lList *alp = NULL;
   lEnumeration *what = NULL;
   Boolean answer;
   
   DENTER(GUI_LAYER, "qmonCplxDelete");

   lp = XmStringToCull(cplx_names, CX_Type, CX_name, SELECTED_ITEMS);
   
   if (!lp) {
      DEXIT;
      return;
   }

   XmtAskForBoolean(w, "xmtBooleanDialog", 
                  "@{cplx.askdel.Do you really want to\ndelete the selected complexes ?}", 
                  "@{Delete}", "@{Cancel}", NULL, XmtNoButton, XmDIALOG_WARNING, 
                  False, &answer, NULL);
   if (answer) {         
      what = lWhat("%T(ALL)", CX_Type);
      alp = qmonDelList( SGE_COMPLEX_LIST, qmonMirrorListRef(SGE_COMPLEX_LIST), 
                                 CX_name, &lp, NULL, what);

      qmonMessageBox(w, alp, 0);
         
      updateCplxList();
      XmListSelectPos(cplx_names, 1, True);

      alp = lFreeList(alp);
      lFreeWhat(what);
   }
   lp = lFreeList(lp);
   
   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonCplxShowCplx(
Widget matrix,
char *cx_name 
) {
   lListElem *cep = NULL;
   lList *entries = NULL;
   
   DENTER(GUI_LAYER, "qmonCplxShowCplx");
   
   /* 
   ** get cx_entries 
   */   
   if (cx_name && cx_name != '\0')
      cep = lGetElemStr(qmonMirrorList(SGE_COMPLEX_LIST), CX_name, cx_name); 

   if (cep) 
      entries = lGetList(cep, CX_entries);

   /*
   ** fill the values into the matrix
   */
   qmonSetCE_Type(cplx_ask_mx, entries, CE_TYPE_FULL);
      
   DEXIT;
}



/*-------------------------------------------------------------------------*/
static Widget qmonCreateCplxAsk(
Widget parent 
) {
   Widget cplx_ask_ok, cplx_ask_cancel, cplx_ask_add,
          cplx_ask_del, cplx_ask_load, cplx_ask_save;

   DENTER(GUI_LAYER, "qmonCreateCplxAsk");
   
   cplx_ask_layout = XmtBuildQueryDialog( parent, "cplx_ask_shell",
                           NULL, 0,
                           "cplx_ask_ok", &cplx_ask_ok,
                           "cplx_ask_cancel", &cplx_ask_cancel,
                           "cplx_ask_name", &cplx_ask_name_w,
                           "cplx_ask_add", &cplx_ask_add,
                           "cplx_ask_del", &cplx_ask_del,
                           "cplx_ask_load", &cplx_ask_load,
                           "cplx_ask_save", &cplx_ask_save,
                           "cplx_ask_mx", &cplx_ask_mx,
                           "cplx_ask_aname", &cplx_ask_aname,
                           "cplx_ask_ashort", &cplx_ask_ashort,
                           "cplx_ask_avalue", &cplx_ask_avalue,
                           "cplx_ask_atype", &cplx_ask_atype,
                           "cplx_ask_arel", &cplx_ask_arel,
                           "cplx_ask_areq", &cplx_ask_areq,
                           "cplx_ask_aconsumable", &cplx_ask_aconsumable,
                           "cplx_ask_adefault", &cplx_ask_adefault,
                           NULL);
   
   /* remove the traverseCell callback */
   XtRemoveAllCallbacks(cplx_ask_mx, XmNtraverseCellCallback);

   XtAddCallback(cplx_ask_ok, XmNactivateCallback, 
                     qmonCplxOk, NULL);
   XtAddCallback(cplx_ask_cancel, XmNactivateCallback, 
                     qmonCplxCancel, NULL);
#if 0                     
   XtAddCallback(cplx_ask_load, XmNactivateCallback, 
                     qmonCplxLoadAttr, NULL);
   XtAddCallback(cplx_ask_save, XmNactivateCallback, 
                     qmonCplxSaveAttr, NULL);
   XtAddCallback(cplx_ask_add, XmNactivateCallback, 
                     qmonCplxAddAttr, (XtPointer)cplx_ask_mx);
   XtAddCallback(cplx_ask_del, XmNactivateCallback, 
                     qmonCplxDelAttr, (XtPointer)cplx_ask_mx);
#endif                     
   XtAddCallback(cplx_ask_mx, XmNenterCellCallback, 
                     qmonCplxNoEdit, NULL);
   XtAddCallback(cplx_ask_mx, XmNselectCellCallback, 
                     qmonCplxSelectAttr, NULL);

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
   return cplx_ask_layout;
}


/*-------------------------------------------------------------------------*/
static void qmonCplxCancel(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   DENTER(GUI_LAYER, "qmonPECancel");

   XtUnmanageChild(cplx_ask_layout);

   DEXIT;
}

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
   entries = qmonGetCE_Type(cplx_ask_mx);
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
   if (is_empty_word(XmtInputFieldGetString(cplx_ask_aname))) {
      qmonMessageShow(matrix, True, "Name required !");
      DEXIT;
      return;
   }
   if (is_empty_word(XmtInputFieldGetString(cplx_ask_ashort))) {
      qmonMessageShow(matrix, True, "Shortcut required !\n");
      DEXIT;
      return;
   }
   if (XmtChooserGetState(cplx_ask_atype) == 0 && 
         is_empty_word(XmtInputFieldGetString(cplx_ask_avalue))) {
      qmonMessageShow(matrix, True, "Value required !\n");
      DEXIT;
      return;
   }
   if (is_empty_word(XmtInputFieldGetString(cplx_ask_adefault))) {
      qmonMessageShow(matrix, True, "Default required !\n");
      DEXIT;
      return;
   }
      
   /*
   ** get the values
   */
   new_str[0][0] = XtNewString(XmtInputFieldGetString(cplx_ask_aname));
   new_str[0][1] = XtNewString(XmtInputFieldGetString(cplx_ask_ashort));
   new_str[0][2] = XtNewString(map_type2str(1 +
                        XmtChooserGetState(cplx_ask_atype)));
   new_str[0][3] = XtNewString(XmtInputFieldGetString(cplx_ask_avalue));
   new_str[0][4] = XtNewString(map_op2str(1 +
                        XmtChooserGetState(cplx_ask_arel)));
   req_state = XmtChooserGetState(cplx_ask_areq); 
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
      
   new_str[0][6] = XtNewString(XmtChooserGetState(cplx_ask_aconsumable) ? "YES" : "NO");
   new_str[0][7] = XtNewString(XmtInputFieldGetString(cplx_ask_adefault));
         

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

   /* reset and jump to cplx_ask_aname */
   XbaeMatrixDeselectAll(matrix);
   /* refresh the matrix */
   XbaeMatrixRefresh(matrix);
   
/*    XmtInputFieldSetString(cplx_ask_name_w, ""); */
   XmtInputFieldSetString(cplx_ask_aname, "");
   XmtInputFieldSetString(cplx_ask_ashort, "");
   XmtChooserSetState(cplx_ask_atype, 0, False);
   XmtInputFieldSetString(cplx_ask_avalue, "");
   XmtChooserSetState(cplx_ask_arel, 0, False);
   XmtChooserSetState(cplx_ask_areq, 0, False);
   XmtChooserSetState(cplx_ask_aconsumable, 0, False);
   XmtInputFieldSetString(cplx_ask_adefault, "");
   XmProcessTraversal(cplx_ask_aname, XmTRAVERSE_CURRENT);


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
   XmtInputFieldSetString(cplx_ask_aname, "");
   XmtInputFieldSetString(cplx_ask_ashort, "");
   XmtChooserSetState(cplx_ask_atype, 0, False);
   XmtInputFieldSetString(cplx_ask_avalue, "");
   XmtChooserSetState(cplx_ask_arel, 0, False);
   XmtChooserSetState(cplx_ask_areq, 0, False);
   XmtChooserSetState(cplx_ask_aconsumable, 0, False);
   XmtInputFieldSetString(cplx_ask_adefault, "");
   
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
      cep = read_cmplx(filename, "dummy", &alp);
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
      entries = lGetList(cep, CX_entries);
      qmonSetCE_Type(cplx_ask_mx, entries, CE_TYPE_FULL);
      cep = lFreeElem(cep);
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
      entries = qmonGetCE_Type(cplx_ask_mx);
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
      XmtInputFieldSetString(cplx_ask_aname, str ? str : ""); 

      /* shortcut */
      str = XbaeMatrixGetCell(w, cbs->row, 1);
      XmtInputFieldSetString(cplx_ask_ashort, str ? str : "");
      
      /* type */
      str = XbaeMatrixGetCell(w, cbs->row, 2);
      type = 0;
      for (i=TYPE_FIRST; !type && i<=TYPE_DOUBLE; i++) {
         if (!strcasecmp(str, map_type2str(i)))
            type = i-1;
      }
      XmtChooserSetState(cplx_ask_atype, type, False); 

      /* value */
      str = XbaeMatrixGetCell(w, cbs->row, 3);
      XmtInputFieldSetString(cplx_ask_avalue, str ? str : "");
      
      /* relop */
      str = XbaeMatrixGetCell(w, cbs->row, 4);
      relop = 0;
      for (i=CMPLXEQ_OP; !relop && i<=CMPLXNE_OP; i++) {
         if (!strcasecmp(str, map_op2str(i)))
            relop = i-1;
      }
      XmtChooserSetState(cplx_ask_arel, relop, False); 

      str = XbaeMatrixGetCell(w, cbs->row, 5);
      if (str && !strcmp(str, "YES")) 
         XmtChooserSetState(cplx_ask_areq, 1, False);
      else if (str && !strcmp(str, "FORCED"))
         XmtChooserSetState(cplx_ask_areq, 2, False);
      else
         XmtChooserSetState(cplx_ask_areq, 0, False);

      str = XbaeMatrixGetCell(w, cbs->row, 6);
      if (str && !strcmp(str, "YES")) 
         XmtChooserSetState(cplx_ask_aconsumable, 1, False);
      else
         XmtChooserSetState(cplx_ask_aconsumable, 0, False);

      /* default */
      str = XbaeMatrixGetCell(w, cbs->row, 7);
      XmtInputFieldSetString(cplx_ask_adefault, str ? str : ""); 
   }
   
   DEXIT;
}
