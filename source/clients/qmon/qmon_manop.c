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

#include <Xm/Xm.h>
#include <Xm/List.h>

#include <Xmt/Xmt.h>
#include <Xmt/Help.h>
#include <Xmt/Create.h>
#include <Xmt/Layout.h>
#include <Xmt/Chooser.h>
#include <Xmt/Dialog.h>
#include <Xmt/Dialogs.h>
#include <Xmt/InputField.h>

#include "sge_all_listsL.h"
#include "sge_gdi.h"
#include "qmon_proto.h"
#include "qmon_rmon.h"
#include "qmon_cull.h"
#include "qmon_util.h"
#include "qmon_appres.h"
#include "qmon_manop.h"
#include "qmon_comm.h"
#include "qmon_globals.h"
#include "qmon_timer.h"
#include "qmon_message.h"
#include "qmon_ticket.h"
#include "qmon_matrix.h"
#include "sge_feature.h"

#include "Tab.h"

/*-------------------------------------------------------------------------*/
static Widget qmonCreateManopConfig(Widget parent);
static void qmonManopHelp(Widget w, XtPointer cld, XtPointer cad);
static void qmonManopFillList(void);
static void qmonManopAdd(Widget w, XtPointer cld, XtPointer cad);
static void qmonManopDelete(Widget w, XtPointer cld, XtPointer cad);
static void qmonManopFolderChange(Widget w, XtPointer cld, XtPointer cad);
static Widget qmonCreateUserConfig(Widget parent, XtPointer cld);

static void updateUsersetList(void);
static Widget qmonCreateUsersetConfig(Widget parent, XtPointer cld);
static void qmonUsersetFillConf(Widget w, lListElem *ep);
static void qmonSelectUserset(Widget w, XtPointer cld, XtPointer cad);
static void qmonUsersetType(Widget w, XtPointer cld, XtPointer cad);
static void qmonUsersetAdd(Widget w, XtPointer cld, XtPointer cad);
static void qmonUsersetModify(Widget w, XtPointer cld, XtPointer cad);
static void qmonUsersetDelete(Widget w, XtPointer cld, XtPointer cad);
static Widget qmonCreateUsersetAsk(Widget parent);
static void qmonUsersetOk(Widget w, XtPointer cld, XtPointer cad);
static void qmonUsersetCancel(Widget w, XtPointer cld, XtPointer cad);
static void qmonUsersetUserAdd(Widget w, XtPointer cld, XtPointer cad);
static void qmonUsersetName(Widget w, XtPointer cld, XtPointer cad);

static void qmonUserAskForProject(Widget w, XtPointer cld, XtPointer cad);
static void qmonPopdownManopConfig(Widget w, XtPointer cld, XtPointer cad);
/*-------------------------------------------------------------------------*/
static Widget qmon_manop = NULL;
static Widget manager_list = NULL;
static Widget manager_name = NULL;
static Widget operator_list = NULL;
static Widget operator_name = NULL;
static Widget user_matrix = NULL;
static Widget user_name = NULL;
static Widget manop_modify = NULL;

static int dialog_mode = SGE_MANAGER_LIST;

static Widget userset_layout = 0;
static Widget userset_names = 0;
static Widget userset_user_list = 0;
static Widget userset_ask_layout = 0;
static Widget uset_user_list = 0;
static Widget uset_user = 0;
static Widget uset_name = 0;
static Widget uset_type = 0;
static Widget uset_user_remove = 0;
static Widget userset_type = 0;
static int add_mode = 0;
static int userset_type_state = US_ACL;



/*-------------------------------------------------------------------------*/
void qmonPopupManopConfig(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget shell;
   
   DENTER(GUI_LAYER, "qmonPopupManopConfig");

   /* set busy cursor */
   XmtDisplayBusyCursor(w);

   /*
   ** create the dialog, if it doesn't exist
   */
   if (!qmon_manop) {
      shell = XmtGetTopLevelShell(w);
      qmon_manop = qmonCreateManopConfig(shell);
      XmtAddDeleteCallback(shell, XmDO_NOTHING, 
                              qmonPopdownManopConfig, NULL);
      XtAddEventHandler(XtParent(qmon_manop), StructureNotifyMask, False, 
                        SetMinShellSize, NULL);
   } 
   
   qmonTimerAddUpdateProc(USERSET_T, "updateUsersetList", updateUsersetList);
   qmonStartTimer(MANAGER_T | OPERATOR_T | USERSET_T | USER_T);
   qmonManopFillList();

   xmui_manage(qmon_manop);

   /* set default cursor */
   XmtDisplayDefaultCursor(w);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonPopdownManopConfig(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   DENTER(GUI_LAYER, "qmonPopdownManopConfig");

   qmonStopTimer(MANAGER_T | OPERATOR_T | USER_T | USERSET_T);
   qmonTimerRmUpdateProc(USERSET_T, "updateUsersetList");

   xmui_unmanage(qmon_manop);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static Widget qmonCreateManopConfig(
Widget parent 
) {
   Widget manop_layout, manop_add, manop_delete, manop_done, manop_help,
          manop_main_link, manop_folder, user_layout, manop_tickets;

   DENTER(GUI_LAYER, "qmonCreateManopConfig");
   
   manop_layout = XmtBuildQueryDialog( parent, "qmon_manop",
                           NULL, 0,
                           "manop_folder", &manop_folder,
                           "manop_add", &manop_add,
                           "manop_modify", &manop_modify,
                           "manop_tickets", &manop_tickets,
                           "manop_delete", &manop_delete,
                           "manop_done", &manop_done,
                           "manop_help", &manop_help,
                           "manop_main_link", &manop_main_link,
                           "manager_list", &manager_list,
                           "manager_name", &manager_name,
                           "operator_list", &operator_list,
                           "operator_name", &operator_name,
                           NULL);
   
   userset_layout = qmonCreateUsersetConfig(manop_folder, NULL);
   XtManageChild(userset_layout);

   if (feature_is_enabled(FEATURE_SGEEE)) {
      user_layout = qmonCreateUserConfig(manop_folder, NULL);
      XtAddCallback(manop_tickets, XmNactivateCallback, 
                        qmonPopupTicketOverview, NULL);
   } else {
      XtUnmanageChild(manop_tickets);
   }
                               
   XtAddCallback(manop_main_link, XmNactivateCallback, 
                     qmonMainControlRaise, NULL);
   XtAddCallback(manop_folder, XmNvalueChangedCallback, 
                     qmonManopFolderChange, NULL);
   XtAddCallback(manop_done, XmNactivateCallback, 
                     qmonPopdownManopConfig, NULL);
   XtAddCallback(manop_add, XmNactivateCallback, 
                     qmonManopAdd, (XtPointer) 1); 
   XtAddCallback(manop_modify, XmNactivateCallback, 
                     qmonManopAdd, NULL); 
   XtAddCallback(manager_name, XmNactivateCallback /* XmtNinputCallback */, 
                     qmonManopAdd, NULL);
   XtAddCallback(operator_name, XmNactivateCallback /* XmtNinputCallback */, 
                     qmonManopAdd, NULL);
   XtAddCallback(manop_delete, XmNactivateCallback, 
                     qmonManopDelete, NULL); 
   XtAddCallback(manop_help, XmNactivateCallback, 
                     qmonManopHelp, NULL);
   
   XtAddEventHandler(XtParent(manop_layout), StructureNotifyMask, False, 
                        SetMinShellSize, NULL);

   DEXIT;
   return manop_layout;
}


/*-------------------------------------------------------------------------*/
static Widget qmonCreateUserConfig(
Widget parent,
XtPointer cld 
) {
   Widget user_layout;

   DENTER(GUI_LAYER, "qmonCreateUserConfig");
   
   user_layout = XmtCreateLayout(parent, "user_layout", NULL, 0);
   XmtCreateQueryChildren( user_layout,
                           "user_name", &user_name,
                           "user_matrix", &user_matrix,
                           NULL);

#if 0
   if (feature_is_enabled(FEATURE_SGEEE)) {
      XtUnmanageChild(user_tickets);
   }
   else {
      XtAddCallback(user_tickets, XmNactivateCallback,
                     qmonPopupTicketOverview, NULL);
   }
#endif
                               
   XtAddCallback(user_name, XmtNinputCallback, 
                     qmonManopAdd, NULL);
   XtAddCallback(user_matrix, XmNlabelActivateCallback,
                  qmonUserAskForProject, NULL);


   DEXIT;
   return user_layout;
}

/*-------------------------------------------------------------------------*/
static void qmonManopFillList(void)
{
   lList *lp, *cl;
   lListElem *ep;
   
   DENTER(GUI_LAYER, "qmonManopFillList");

   /* manager list */
   lp = qmonMirrorList(SGE_MANAGER_LIST);
   lPSortList(lp, "%I+", MO_name);
   UpdateXmListFromCull(manager_list, XmFONTLIST_DEFAULT_TAG, lp, MO_name);
   
   /* operator list */
   lp = qmonMirrorList(SGE_OPERATOR_LIST);
   lPSortList(lp, "%I+", MO_name);
   UpdateXmListFromCull(operator_list, XmFONTLIST_DEFAULT_TAG, lp, MO_name);

   /* userset list */
   updateUsersetList();

   /* user list */
   if (feature_is_enabled(FEATURE_SGEEE)) {
      lp = qmonMirrorList(SGE_USER_LIST);
      lPSortList(lp, "%I+", UP_name);
      /*
      ** set UP_default_project to NONE
      */
      cl = lCopyList("cl", lp);
      for_each (ep, cl) {
         if (ep && !lGetString(ep, UP_default_project)) 
            lSetString(ep, UP_default_project, "NONE");
      }
      qmonSet2xN(user_matrix, cl, UP_name, UP_default_project);
      lFreeList(cl);
   }


   DEXIT;
}

/*-------------------------------------------------------------------------*/
void updateManopList(void)
{
   DENTER(GUI_LAYER, "updateManopList");

   if (qmon_manop && XtIsManaged(qmon_manop))
      qmonManopFillList();

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonManopDelete(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   int type = dialog_mode;
   lList *lp = NULL;
   lList *alp = NULL;
   lEnumeration *what = NULL;
   lDescr *dp;
   int nm;
   int rows, i;
   

   DENTER(GUI_LAYER, "qmonManopDelete");

   switch (type) {
      case SGE_MANAGER_LIST:
         lp = XmStringToCull(manager_list, MO_Type, MO_name, SELECTED_ITEMS);
         dp = MO_Type;
         nm = MO_name;
         break;
      case SGE_OPERATOR_LIST:
         lp = XmStringToCull(operator_list, MO_Type, MO_name, SELECTED_ITEMS);
         dp = MO_Type;
         nm = MO_name;
         break;

      case SGE_USER_LIST:
         rows = XbaeMatrixNumRows(user_matrix);
         for (i=0; i<rows; i++) {
            String s = XbaeMatrixGetCell(user_matrix, i, 0);
            if (XbaeMatrixIsRowSelected(user_matrix, i) && s &&
                    strcmp(s, ""))
               lAddElemStr(&lp, UP_name, s, UP_Type); 
         } 
         XbaeMatrixDeselectAll(user_matrix);
         dp = UP_Type;
         nm = UP_name;
         break;

      case SGE_USERSET_LIST:
         qmonUsersetDelete(w, NULL, NULL);
         DEXIT;
         return;
      default:
         DEXIT;
         return;
   }
    
      
   if (lp) {
      what = lWhat("%T(ALL)", dp);
      alp = qmonDelList(type, qmonMirrorListRef(type), nm, &lp, NULL, what);

      qmonMessageBox(w, alp, 0);
   
      updateManopList();

      alp = lFreeList(alp);
      lp = lFreeList(lp);
      lFreeWhat(what);
   } 
   
   DEXIT;
}
         
/*-------------------------------------------------------------------------*/
static void qmonManopAdd(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   int type = dialog_mode;
   lList *lp = NULL;
   lList *alp = NULL;
   lEnumeration *what = NULL;
   String user = NULL;
   Widget username;
   int nm;
   lDescr *dp;

   DENTER(GUI_LAYER, "qmonManopAdd");

   switch (type) {
      case SGE_MANAGER_LIST:
         dp = MO_Type;
         nm = MO_name;
         username = manager_name;
         break;
      case SGE_OPERATOR_LIST:
         dp = MO_Type;
         nm = MO_name;
         username = operator_name;
         break;

      case SGE_USER_LIST:
         dp = UP_Type;
         nm = UP_name;
         username = user_name;
         break;

      case SGE_USERSET_LIST:
         if ((long)cld == 1)
            qmonUsersetAdd(w, NULL, NULL);
         else
            qmonUsersetModify(w, NULL, NULL);
         DEXIT;
         return;
      default:
         DEXIT;
         return;
   }

   user = XmtInputFieldGetString(username);
   
   if (user && user[0] != '\0' && user[0] != ' ') { 
      what = lWhat("%T(ALL)", dp);
      if (!(lp = lCreateElemList("AH", dp, 1))) {
         fprintf(stderr, "lCreateElemList failed\n");
         lFreeWhat(what);
         DEXIT;
         return;
      }
      lSetString(lFirst(lp), nm, user);
/*       if (type == SGE_USER_LIST) */
/*          lSetString(lFirst(lp), UP_default_project, "NONE"); */
      
      alp = qmonAddList(type, qmonMirrorListRef(type), 
                           nm, &lp, NULL, what);
         
      qmonMessageBox(w, alp, 0);

      updateManopList();
         
      XmtInputFieldSetString(username, "");

      lFreeWhat(what);
      lp = lFreeList(lp);
      alp = lFreeList(alp);
   }
   DEXIT;
}
         
/*-------------------------------------------------------------------------*/
static void qmonManopFolderChange(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *alp = NULL;
   XmTabCallbackStruct *cbs = (XmTabCallbackStruct *) cad;

   DENTER(GUI_LAYER, "qmonManopFolderChange");
   
   DPRINTF(("%s\n", XtName(cbs->tab_child)));

   if (!strcmp(XtName(cbs->tab_child), "manager_layout"))
      dialog_mode = SGE_MANAGER_LIST;

   if (!strcmp(XtName(cbs->tab_child), "operator_layout"))
      dialog_mode = SGE_OPERATOR_LIST;

   if (!strcmp(XtName(cbs->tab_child), "user_layout"))
      dialog_mode = SGE_USER_LIST;

   if (!strcmp(XtName(cbs->tab_child), "userset_layout"))
      dialog_mode = SGE_USERSET_LIST;

   /*
   ** fetch changed lists and update dialogues
   */
   qmonMirrorMultiAnswer(MANAGER_T | OPERATOR_T | USERSET_T | USER_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }
   updateManopList();

   /*
   ** set Modify button sensitivity
   */
   XtSetSensitive(manop_modify, (dialog_mode==SGE_USERSET_LIST) ? True:False);
      
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonManopHelp(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget widget = 0;
   
   DENTER(GUI_LAYER, "qmonManopHelp");

   switch(dialog_mode) {
      case SGE_MANAGER_LIST:
         widget = manager_list; 
         break;
      case SGE_OPERATOR_LIST:
         widget = operator_list; 
         break;
      case SGE_USER_LIST:
         widget = user_matrix; 
         break;
      case SGE_USERSET_LIST:
         widget = userset_layout; 
         break;
      default:
         DEXIT;
         return;
   }

   XmtHelpDisplayContextHelp(widget);  

   DEXIT;
}



/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
static void updateUsersetList(void)
{
   lList *al;
   XmString *selectedItems;
   Cardinal selectedItemCount;
   Cardinal itemCount;
   
   DENTER(GUI_LAYER, "updateUsersetList");

   al = qmonMirrorList(SGE_USERSET_LIST);
   lPSortList(al, "%I+", US_name);
   UpdateXmListFromCull(userset_names, XmFONTLIST_DEFAULT_TAG, al, 
                           US_name);
   XtVaGetValues( userset_names,
                  XmNselectedItems, &selectedItems,
                  XmNselectedItemCount, &selectedItemCount,
                  XmNitemCount, &itemCount,
                  NULL);

   if (selectedItemCount)
      XmListSelectItem(userset_names, selectedItems[0], True);
   else if (itemCount)
      XmListSelectPos(userset_names, 1, True);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonUsersetFillConf(
Widget w,
lListElem *ep 
) {
   lList *ul;

   DENTER(GUI_LAYER, "qmonUsersetFillConf");
   
   if (!ep) {
      /*
      ** clear the userset_user_list
      */
      XtVaSetValues( userset_user_list, 
                  XmNitems, NULL,
                  XmNitemCount, 0,
                  NULL);
      DEXIT;
      return;
   }

   if (feature_is_enabled(FEATURE_SGEEE)) {
      userset_type_state = (int)lGetUlong(ep, US_type);
      XmtChooserSetState(userset_type, userset_type_state, False);
   }
   ul = lGetList(ep, US_entries);
   lPSortList(ul, "%I+", UE_name);
   UpdateXmListFromCull(userset_user_list, XmFONTLIST_DEFAULT_TAG, ul, UE_name);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonSelectUserset(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   XmListCallbackStruct *cbs = (XmListCallbackStruct*) cad;
   char *usetname;
   lListElem *ep;
   
   DENTER(GUI_LAYER, "qmonSelectUserset");

   if (!XmStringGetLtoR(cbs->item, XmFONTLIST_DEFAULT_TAG, &usetname)) {
      fprintf(stderr, "XmStringGetLtoR failed\n");
      DEXIT;
      return;
   }

   ep = lGetElemStr(qmonMirrorList(SGE_USERSET_LIST), US_name, usetname);
   if (usetname && !strcmp(usetname, DEFAULT_DEPARTMENT)) 
      XtSetSensitive(manop_modify, False);
   else
      XtSetSensitive(manop_modify, True);

   XtFree((char*) usetname);
   
   qmonUsersetFillConf(userset_user_list, ep);
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonUsersetType(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   DENTER(GUI_LAYER, "qmonUsersetType");

   XmtChooserSetState(w, userset_type_state, False);
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static Widget qmonCreateUsersetConfig(
Widget parent,
XtPointer cld 
) {
   Widget userset_layout, shell;

   DENTER(TOP_LAYER, "qmonCreateUsersetConfig2");
   userset_layout = XmtCreateLayout( parent, "userset_layout", NULL, 0);
   XmtCreateQueryChildren( userset_layout, 
                           "userset_names", &userset_names,
                           "userset_user_list", &userset_user_list,
                           "userset_type", &userset_type,
                           NULL);


   /*
   ** create ask layout
   */
   shell = XmtGetShell(userset_layout);
   userset_ask_layout = qmonCreateUsersetAsk(shell);

   XtAddCallback(userset_names, XmNbrowseSelectionCallback, 
                     qmonSelectUserset, NULL);
#if 0
   XtAddCallback(userset_done, XmNactivateCallback, 
                     qmonPopdownManopConfig, NULL);
   XtAddCallback(userset_add, XmNactivateCallback, 
                     qmonUsersetAdd, NULL); 
   XtAddCallback(userset_delete, XmNactivateCallback, 
                     qmonUsersetDelete, NULL); 
   XtAddCallback(userset_modify, XmNactivateCallback, 
                     qmonUsersetModify, NULL); 
#endif

   if (!feature_is_enabled(FEATURE_SGEEE)) {
      XmString xstr;
      
/*       XtUnmanageChild(userset_tickets); */
      XtUnmanageChild(userset_type);
      xstr = XmtCreateLocalizedXmString(shell, "@{Access Lists}");
      XtVaSetValues( XtParent(userset_names),
                     XmtNlayoutCaption, xstr,
                     NULL);
      XmStringFree(xstr);

      xstr = XmtCreateLocalizedXmString(shell, "@{Access List}");
      XtVaSetValues( uset_name,
                     XmtNlayoutCaption, xstr,
                     NULL);
      XmStringFree(xstr);
   }
   else {
/*       XtAddCallback(userset_tickets, XmNactivateCallback, */
/*                      qmonPopupTicketOverview, NULL); */
      XtAddCallback(userset_type, XmNvalueChangedCallback,
                     qmonUsersetType, NULL);
                     
   }


   DEXIT;
   return userset_layout;
}

   
/*-------------------------------------------------------------------------*/
static Widget qmonCreateUsersetAsk(
Widget parent 
) {
   Widget uset_ok, uset_cancel;

   DENTER(TOP_LAYER, "qmonCreateUsersetAsk");
   
   userset_ask_layout = XmtBuildQueryDialog( parent, "userset_ask_shell",
                           NULL, 0,
                           "uset_ok", &uset_ok,
                           "uset_cancel", &uset_cancel,
                           "uset_user_list", &uset_user_list,
                           "uset_user_remove", &uset_user_remove,
                           "uset_user", &uset_user,
                           "uset_name", &uset_name,
                           "uset_type", &uset_type,
                           NULL);

   if (!feature_is_enabled(FEATURE_SGEEE)) {
      XtUnmanageChild(uset_type);
   }

   XtAddCallback(uset_ok, XmNactivateCallback,
                  qmonUsersetOk, NULL);
   XtAddCallback(uset_cancel, XmNactivateCallback,
                  qmonUsersetCancel, NULL);
   XtAddCallback(uset_user, XmNinputCallback,
                  qmonUsersetUserAdd, NULL);

#if 0
   if (feature_is_enabled(FEATURE_SGEEE)) {
      XtAddCallback(uset_name, XmNinputCallback,
                  qmonUsersetName, NULL);
   }
#endif   

   XtAddEventHandler(XtParent(userset_ask_layout), StructureNotifyMask, False, 
                        SetMinShellSize, NULL);
   DEXIT;
   return userset_ask_layout;
}

/*-------------------------------------------------------------------------*/
static void qmonUsersetAdd(w, cld, cad)
Widget w;
XtPointer cld, cad;
{

   DENTER(GUI_LAYER, "qmonUsersetAdd");

   XmtInputFieldSetString(uset_name, "");

   if (feature_is_enabled(FEATURE_SGEEE)) {
      XmtChooserSetState(uset_type, US_ACL, False);
   }
   XtVaSetValues( uset_name,
                  XmNeditable, True,
                  NULL);
   XtVaSetValues( uset_user_list,
                  XmNitems, NULL,
                  XmNitemCount, 0,
                  NULL);

   add_mode = 1;

   XtManageChild(userset_ask_layout);
   XmtSetInitialFocus(userset_ask_layout, uset_name);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonUsersetModify(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   XmString *usetname;
   Cardinal usetnum;
   String usetstr;
   XmString *usetusers;
   Cardinal usetusernum;

   DENTER(GUI_LAYER, "qmonUsersetModify");
   
   /*
   ** on opening the dialog fill in the old values
   */
   XtVaGetValues( userset_names,
                  XmNselectedItems, &usetname,
                  XmNselectedItemCount, &usetnum,
                  NULL);
   XtVaGetValues( userset_user_list,
                  XmNitems, &usetusers,
                  XmNitemCount, &usetusernum,
                  NULL);

   if (usetnum == 1 && 
         XmStringGetLtoR(usetname[0], XmFONTLIST_DEFAULT_TAG, &usetstr)) {
      XmtInputFieldSetString(uset_name, usetstr);
      XtFree((char *)usetstr);
      XtVaSetValues( uset_name,
                     XmNeditable, False,
                     NULL);
      XtVaSetValues( uset_user_list,
                     XmNitems, usetusers,
                     XmNitemCount, usetusernum,
                     NULL);

      if (feature_is_enabled(FEATURE_SGEEE)) {
         XmtChooserSetState(uset_type, userset_type_state, False);
      }

      add_mode = 0;

      XtManageChild(userset_ask_layout);
      XmtSetInitialFocus(userset_ask_layout, uset_user);
   }

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonUsersetName(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   String name = NULL;

   DENTER(GUI_LAYER, "qmonUsersetName");

   name = XmtInputFieldGetString(uset_name);

   if (name && !strcmp(name, DEFAULT_DEPARTMENT)) {
      XtSetSensitive(uset_user, False);
      XtSetSensitive(uset_user_list, False);
      XtSetSensitive(uset_user_remove, False);
      XmtChooserSetState(uset_type, US_DEPT, False);
      XtSetSensitive(uset_type, False);
   }
   else {
      XtSetSensitive(uset_user, True);
      XtSetSensitive(uset_user_list, True);
      XtSetSensitive(uset_user_remove, True);
      XtSetSensitive(uset_type, True);
   }   

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonUsersetUserAdd(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   String user = NULL;

   DENTER(GUI_LAYER, "qmonUsersetUserAdd");

   user = XmtInputFieldGetString(uset_user);

   if (user && user[0] != '\0') {
      XmListAddItemUniqueSorted(uset_user_list, user);
      XmtInputFieldSetString(uset_user, "");
   }

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonUsersetOk(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   String usetname = NULL;
   XmString xusetname = NULL;
   lList *lp = NULL;
   lList *ul = NULL;
   lList *alp = NULL;
   lListElem *up = NULL;
   lEnumeration *what = NULL;
   Boolean status = False;
   int usettype = 0;

   DENTER(GUI_LAYER, "qmonUsersetOk");

   /*
   ** get the dialog data and
   */
   usetname = XmtInputFieldGetString(uset_name);

   if (feature_is_enabled(FEATURE_SGEEE)) {
      usettype = XmtChooserGetState(uset_type); 
   }

   /*
   ** usetname required, show warning dialog
   */
   if (usetname && usetname[0] != '\0') {
      ul = XmStringToCull(uset_user_list, UE_Type, UE_name, ALL_ITEMS);
      /*
      ** do gdi stuff here
      */ 
      if (add_mode) {
         lAddElemStr(&lp, US_name, usetname, US_Type);
      }
      else {
         up = lCopyElem(lGetElemStr(qmonMirrorList(SGE_USERSET_LIST), 
                           US_name, usetname));
         lp = lCreateList("userset", US_Type);
         lAppendElem(lp, up);
      }

      lSetList(lFirst(lp), US_entries, ul);
      if (feature_is_enabled(FEATURE_SGEEE)) {
         lSetUlong(lFirst(lp), US_type, usettype); 
      }

      what = lWhat("%T(ALL)", US_Type);

      if (add_mode) {
         alp = qmonAddList(SGE_USERSET_LIST, 
                        qmonMirrorListRef(SGE_USERSET_LIST),
                        US_name, &lp, NULL, what);
      }
      else {
         alp = qmonModList(SGE_USERSET_LIST, 
                           qmonMirrorListRef(SGE_USERSET_LIST),
                           US_name, &lp, NULL, what);
      }

      if (!qmonMessageBox(w, alp, 0))
         status = True;

      updateUsersetList();

      xusetname = XmtCreateXmString(usetname);
      XmListSelectItem(userset_names, xusetname, True);
      XmStringFree(xusetname);

      lFreeWhat(what);
      lp = lFreeList(lp);
      alp = lFreeList(alp);
   }  
   else { 
      qmonMessageShow(w, True, "Userset List Name required !");
   }       
   
   if (status) 
      XtUnmanageChild(userset_ask_layout);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonUsersetCancel(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   DENTER(GUI_LAYER, "qmonUsersetCancel");

   XtUnmanageChild(userset_ask_layout);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonUsersetDelete(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *lp = NULL;
   lList *alp = NULL;
   Cardinal itemCount = 0;
   lEnumeration *what = NULL;
   Boolean answer;

   DENTER(GUI_LAYER, "qmonUsersetDelete");

   lp = XmStringToCull(userset_names, US_Type, US_name, SELECTED_ITEMS);
      
   if (lp) {
      XmtAskForBoolean(w, "xmtBooleanDialog", 
                  "@{userset.askdel.Do you really want to delete\nthe selected userset lists ?}", 
                  "@{Delete}", "@{Cancel}", NULL, XmtNoButton, XmDIALOG_WARNING, 
                  False, &answer, NULL);
         
      if (answer) { 
         what = lWhat("%T(ALL)", US_Type);
         alp = qmonDelList(SGE_USERSET_LIST, 
                           qmonMirrorListRef(SGE_USERSET_LIST),
                           US_name, &lp, NULL, what);

         qmonMessageBox(w, alp, 0);
         
         lFreeWhat(what);
         lp = lFreeList(lp);
         alp = lFreeList(alp);

         updateUsersetList();
         XtVaGetValues( userset_names,
                        XmNitemCount, &itemCount,
                        NULL);
         if (itemCount)
            XmListSelectPos(userset_names, 1, True);
         else
            qmonUsersetFillConf(userset_names, NULL);
      }
   }
   DEXIT;

}

/*-------------------------------------------------------------------------*/
static void qmonUserAskForProject(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   XbaeMatrixLabelActivateCallbackStruct *cbs = 
                            (XbaeMatrixLabelActivateCallbackStruct *) cad;
   Boolean status = False;
   lList *pl = NULL;
   lListElem *cep = NULL;
   int n, i;
   StringConst *strs = NULL;
   static char buf[BUFSIZ];
   lList *lp = NULL;
   lList *alp = NULL;
   lListElem *ep;
   static lEnumeration *what = NULL;
   
   DENTER(GUI_LAYER, "qmonUserAskForProject");

   if (!what) {
      what = lWhat("%T(%I %I)", UP_Type, UP_name, UP_default_project);
   }

   if (cbs->column != 1) {
        DEXIT;
        return;
   }
   
   qmonMirrorMultiAnswer(PROJECT_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }
   pl = qmonMirrorList(SGE_PROJECT_LIST);
   n = lGetNumberOfElem(pl);
   if (n>0) {
      strs = (StringConst*)XtMalloc(sizeof(String)*(n+1)); 
      strs[0] = "NONE";
      for (cep=lFirst(pl), i=0; i<n; cep=lNext(cep), i++) {
        /*
        ** we get only references don't free, the strings
        */
        strs[i+1] = lGetString(cep, UP_name);
      }
    
      strcpy(buf, "");
      /* FIX_CONST_GUI */
      status = XmtAskForItem(w, NULL, "@{Select a project}",
                        "@{Available projects}", (String*)strs, n+1,
                        False, buf, BUFSIZ, NULL); 
      
      if (status) {
         int rows, i;
         rows = XbaeMatrixNumRows(user_matrix);
         for (i=0; i<rows; i++) {
            String s = XbaeMatrixGetCell(user_matrix, i, 0);
            if (XbaeMatrixIsRowSelected(user_matrix, i) && s &&
                    strcmp(s, "")) {
               ep = lAddElemStr(&lp, UP_name, s, UP_Type); 
               lSetString(ep, UP_default_project, buf);
            }
         } 
         XbaeMatrixDeselectAll(user_matrix);

         /*
         ** send to master as modify request
         */
         for_each (ep, lp) {
            if (ep && lGetString(ep, UP_default_project) && 
                !strcasecmp(lGetString(ep, UP_default_project), "NONE")) 
               lSetString(ep, UP_default_project, NULL);
         }
         if (lp) {
             alp = qmonModList(SGE_USER_LIST, 
                               qmonMirrorListRef(SGE_USER_LIST),
                               UP_name, &lp, NULL, what);
             qmonMessageBox(w, alp, 0);
             updateManopList();

             alp = lFreeList(alp);
             lp = lFreeList(lp);
         }
      }
      /*
      ** don't free referenced strings, they are in the pl list
      */
      XtFree((char*)strs);
   }
   else
      qmonMessageShow(w, True, 
            "Please configure a project first.");
   
   DEXIT;
}
