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
#include <stdlib.h>
#include <math.h>

#include <Xm/ToggleB.h>
#include <Xmt/Xmt.h>
#include <Xmt/Create.h>
#include <Xmt/Layout.h>
#include <Xmt/Chooser.h>
#include <Xmt/Dialog.h>
#include <Xmt/Dialogs.h>
#include <Xmt/MsgLine.h>
#include <Xmt/InputField.h>

#include "ListTree.h"
#include "Matrix.h"
#include "Tab.h"

#include "qmon_rmon.h"
#include "qmon_cull.h"
#include "qmon_util.h"
#include "qmon_init.h"
#include "qmon_timer.h"
#include "qmon_comm.h"
#include "qmon_browser.h"
#include "qmon_cq.h"
#include "qmon_qaction.h"
#include "qmon_appres.h"
#include "sgeee.h"
#include "qmon_message.h"
#include "qmon_widgets.h"
#include "qmon_manop.h"
#include "qmon_project.h"
#include "qmon_globals.h"
#include "sge_all_listsL.h"
#include "sge_gdi.h"
#include "sge_support.h"
#include "sge_answer.h"
#include "sge_cqueue.h"
#include "sge_cqueue_qstat.h"
#include "sge_cqueue_qconf.h"
#include "sge_select_queue.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"

static Widget qmon_cq = 0;
static Widget cq_tree = 0;
static Widget cq_message = 0;
static Widget cluster_queue_settings = 0;
static Widget qinstance_settings = 0;

static Boolean dirty = False;
static Widget current_matrix = 0;


/*-------------------------------------------------------------------------*/
static void qmonCQPopdown(Widget w, XtPointer cld, XtPointer cad);
static void qmonCQUpdate(Widget w, XtPointer cld, XtPointer cad);
static void qmonCQHighlight(Widget w, XtPointer cld, XtPointer cad);
static void qmonCQMenu(Widget w, XtPointer cld, XtPointer cad);
static Widget qmonCQCreate(Widget shell);
static void qmonCQActivate(Widget w, XtPointer cld, XtPointer cad);
static void qmonCQFindNode(Widget w, XtPointer cld, XtPointer cad);
static void qmonCQFindNextNode(Widget w, XtPointer cld, XtPointer cad);

static ListTreeItem* CullToTree(Widget tree, ListTreeItem *parent, lList *shac);
static void qmonQinstanceHandleEnterLeave(Widget w, XtPointer cld, XEvent *ev, Boolean *ctd);
static void qmonCQHandleEnterLeave(Widget w, XtPointer cld, XEvent *ev, Boolean *ctd);
static String qmonQinstanceShowBrowserInfo(lListElem *qep);
static String qmonCQShowBrowserInfo(lListElem *qep);
static void qmonCQDelete(Widget matrix, XtPointer cld, XtPointer cad);
static void qmonCQChangeState(Widget w, XtPointer cld, XtPointer cad);
static void qmonCQFolderChange(Widget w, XtPointer cld, XtPointer cad);
static void qmonCQUpdateCQMatrix(void);
static void qmonCQUpdateQIMatrix(void);
static void qmonCQModify(Widget w, XtPointer cld, XtPointer cad);
/* static void qmonCQSetValues(ListTreeItem *item); */

#if 0
static ListTreeItem* cq_add_aulng(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name);
static ListTreeItem* cq_add_abool(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name);
static ListTreeItem* cq_add_amem(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name);
static ListTreeItem* cq_add_atime(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name);
static ListTreeItem* cq_add_ainter(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name);
static ListTreeItem* cq_add_astr(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name);
static ListTreeItem* cq_add_astrlist(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name);
static ListTreeItem* cq_add_ausrlist(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name);
static ListTreeItem* cq_add_aprjlist(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name);
static ListTreeItem* cq_add_acelist(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name);
static ListTreeItem* cq_add_asolist(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name);
static ListTreeItem* cq_add_aqtlist(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name);
#endif

/*-------------------------------------------------------------------------*/
/* P U B L I C                                                             */
/*-------------------------------------------------------------------------*/
void qmonCQPopup(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget shell;

   DENTER(GUI_LAYER, "qmonCQPopup");

   /*
   ** reset dirty flag
   */
   dirty = False;

   /* set busy cursor */
   XmtDisplayBusyCursor(w);

   if (!qmon_cq) {
      shell = XmtGetTopLevelShell(w);
      qmon_cq = qmonCQCreate(shell);
      XmtAddDeleteCallback(shell, XmDO_NOTHING, 
                              qmonCQPopdown, NULL);
   } 
   
   /*
   ** set the cluster queue tree
   */
   qmonCQUpdate(qmon_cq, (XtPointer)cq_tree, NULL);

   qmonCQUpdateCQMatrix();
   
   xmui_manage(qmon_cq);

   /* set default cursor */
   XmtDisplayDefaultCursor(w);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
/* P R I V A T E                                                           */
/*-------------------------------------------------------------------------*/
static void qmonCQPopdown(w, cld, cad)
Widget w;
XtPointer cld, cad;
{

   DENTER(GUI_LAYER, "qmonCQPopdown");

   XtUnmanageChild(qmon_cq);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static Widget qmonCQCreate(
Widget parent 
) {
   Widget cq_layout, cq_close, cq_update, cq_add, cq_mod, cq_delete,
          cq_suspend, cq_resume, cq_disable, cq_enable, cq_reschedule,
          cq_clear_error, cq_main_link, cq_next, cq_find, cq_cqfolder;

   DENTER(GUI_LAYER, "qmonCQCreate");

   cq_layout = XmtBuildQueryDialog( parent, "qmon_cq",
                           NULL, 0,
                           "cq_close", &cq_close,
                           "cq_update", &cq_update,
                           "cq_add", &cq_add,
                           "cq_mod", &cq_mod,
                           "cq_delete", &cq_delete,
                           "cq_suspend", &cq_suspend,
                           "cq_resume", &cq_resume,
                           "cq_disable", &cq_disable,
                           "cq_enable", &cq_enable,
                           "cq_reschedule", &cq_reschedule,
                           "cq_clear_error", &cq_clear_error,
                           "cq_tree", &cq_tree,
                           "cq_find", &cq_find,
                           "cq_next", &cq_next,
                           "cq_main_link", &cq_main_link,
                           "cq_message", &cq_message,
                           "cluster_queue_settings", &cluster_queue_settings,
                           "qinstance_settings", &qinstance_settings,
                           "cq_cqfolder", &cq_cqfolder,
                           NULL);
   
   current_matrix = cluster_queue_settings;

   XtAddCallback(cq_main_link, XmNactivateCallback, 
                     qmonMainControlRaise, NULL);
   XtAddCallback(cq_close, XmNactivateCallback, 
                     qmonCQPopdown, NULL);
   XtAddCallback(cq_update, XmNactivateCallback, 
                     qmonCQUpdate, (XtPointer)cq_tree);
   XtAddCallback(cq_find, XmNactivateCallback, 
                     qmonCQFindNode, (XtPointer)cq_tree);
   XtAddCallback(cq_next, XmNactivateCallback, 
                     qmonCQFindNextNode, (XtPointer)cq_tree);


   XtAddCallback(cq_tree, XtNactivateCallback, 
                     qmonCQActivate, NULL);
   XtAddCallback(cq_tree, XtNhighlightCallback, 
                     qmonCQHighlight, NULL);
   XtAddCallback(cq_tree, XtNmenuCallback, 
                     qmonCQMenu, NULL);

   XtAddCallback(cq_add, XmNactivateCallback, 
                     qmonQCPopup, NULL);
   XtAddCallback(cq_mod, XmNactivateCallback, 
                     qmonCQModify, NULL);
   XtAddCallback(cq_delete, XmNactivateCallback, 
                     qmonCQDelete, NULL);

   XtAddCallback(cq_suspend, XmNactivateCallback, 
                     qmonCQChangeState, (XtPointer)QI_DO_SUSPEND);
   XtAddCallback(cq_resume, XmNactivateCallback, 
                     qmonCQChangeState, (XtPointer)QI_DO_UNSUSPEND);
   XtAddCallback(cq_disable, XmNactivateCallback, 
                     qmonCQChangeState, (XtPointer)QI_DO_DISABLE);
   XtAddCallback(cq_enable, XmNactivateCallback, 
                     qmonCQChangeState, (XtPointer)QI_DO_ENABLE);
   XtAddCallback(cq_reschedule, XmNactivateCallback, 
                     qmonCQChangeState, (XtPointer)QI_DO_RESCHEDULE);
   XtAddCallback(cq_clear_error, XmNactivateCallback, 
                     qmonCQChangeState, (XtPointer)QI_DO_CLEARERROR);

   XtAddCallback(cq_cqfolder, XmNvalueChangedCallback, 
                     qmonCQFolderChange, NULL);

   XtAddEventHandler(qinstance_settings, PointerMotionMask, 
                     False, qmonQinstanceHandleEnterLeave,
                     NULL); 
   XtAddEventHandler(cluster_queue_settings, PointerMotionMask, 
                     False, qmonCQHandleEnterLeave,
                     NULL); 


   XtAddEventHandler(XtParent(cq_layout), StructureNotifyMask, False, 
                        SetMinShellSize, NULL);

   DEXIT;
   return cq_layout;
}


static char search_for[256] = "";
static ListTreeMultiReturnStruct *matches;

/*-------------------------------------------------------------------------*/
static void qmonCQFindNode(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget tree = (Widget)cld;
   ListTreeItem *item = NULL;
   char name[256];
   Boolean status;

   DENTER(GUI_LAYER, "qmonCQFindNode");

   /*
   ** create new_node
   */
   strcpy(name, "");
   status = XmtAskForString(w, "xmtAskForString", "@{Enter item name}", name, 256, NULL);

   if (status && name[0] != '\0') {
      item = ListTreeFindChildName(tree, ListTreeFirstItem(tree), name);
      if (!item) {
         DEXIT;
         return;
      }   
      strncpy(search_for, name, 256);
      matches = NULL;
      /*
      ** Highlight the new_node
      */
      ListTreeOpenNode(tree, item);
      ListTreeMakeItemVisible(tree, item);
      ListTreeHighlightItem(tree, item, True);
   }
   else {
      strcpy(search_for, "");
   }   

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonCQFindNextNode(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget tree = (Widget)cld;
   static int count = 0;
   ListTreeItem *current;
   int first = 0;

   DENTER(GUI_LAYER, "qmonCQFindNode");

   if (!matches && strcmp(search_for, "")) {
      first = 1;
      count = 0;
      matches = ListTreeBuildSearchList(tree, ListTreeFirstItem(tree),search_for, 1);
      /*
      {
         int i;
         for (i=0; i<matches->count; i++) 
            printf("matches->items[%d]->text: %s\n", i, matches->items[i]->text);
      }
      */
   }

   /*
   ** Highlight the new_node
   */
   if (matches && matches->count > 1 && count < matches->count) {
      if (first) {
         first = 0;
         count++;
      }
      current = matches->items[count];
      DPRINTF(("current->text: %s, current->count = %d\n", current->text, current->count));
      /*
      ** open branch and highlight item
      */
      ListTreeOpenNode(tree, current);
      ListTreeMakeItemVisible(tree, current);
      ListTreeHighlightItem(tree, current, True);

      count++;
      if (count == matches->count)
         count = 0;
   }

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonCQUpdate(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget tree = (Widget) cld;
   lList *cluster_queue_tree = NULL;
   ListTreeItem *old_root = NULL;
   ListTreeItem *root_node = NULL;
   lList *alp = NULL;

   DENTER(GUI_LAYER, "qmonCQUpdate");

   XmtDisplayBusyCursor(w);
   
   /*
   ** set and get several lists
   */
   qmonMirrorMultiAnswer(CQUEUE_T,  &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }

   cluster_queue_tree = qmonMirrorList(SGE_CQUEUE_LIST);

   ListTreeRefreshOff(tree);
   /*
   ** unchain the old tree, what about the old user_data struct ????
   */
   old_root = ListTreeFirstItem(tree);
   ListTreeUnchainItem(tree, old_root);
      
   root_node = ListTreeAddBranch(tree, NULL, "/");
   CullToTree(tree, root_node, cluster_queue_tree);

   /*
   ** open the new tree according to old tree or to
   ** a defined level
   */
   if (old_root) {
      ListTreeOpenLikeTree(tree, root_node, old_root);
      ListTreeDelete(tree, old_root);
   }
   else
      ListTreeOpenToLevel(tree, NULL, 1);
   
/*    ListTreeHighlightItem(tree, root_node, True); */
   ListTreeRefreshOn(tree);
/*    ListTreeMakeItemVisible(tree, root_node); */

   XmtDisplayDefaultCursor(w);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonCQHighlight(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   ListTreeMultiReturnStruct *ret = (ListTreeMultiReturnStruct*)cad;
   ListTreeItem *item;
   int i;

   DENTER(GUI_LAYER, "qmonCQHighlight");

   if (rmon_mlgetl(&DEBUG_ON, GUI_LAYER) & INFOPRINT) {
      printf("HIGHLIGHT: count=%d\n",ret->count);
      for (i=0; i<ret->count; i++) {
        item=ret->items[i];
        printf("%s",item->text);
        while (item->parent) {
          item=item->parent;
          printf("<--%s",item->text);
        }
        printf("\n");
      }
   }

   if (ret && ret->count > 0) {
      item = ret->items[0];
/*       qmonCQSetValues(item); */
   }

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonCQMenu(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   ListTreeItemReturnStruct *ret = (ListTreeItemReturnStruct *)cad;
   char name[BUFSIZ];

   DENTER(GUI_LAYER, "qmonCQMenu");

   if (rmon_mlgetl(&DEBUG_ON, GUI_LAYER) & INFOPRINT) {
      printf ("MENU: item=%s\n", ret->item->text);
   }

   /*
   ** set the values in the item for the node popup
   */
   strcpy(name, ret->item->text);

   printf("Menu Item: %s\n", name); 

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonCQActivate(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   ListTreeActivateStruct *ret = (ListTreeActivateStruct*)cad;
   int count;
   char buf[BUFSIZ];

   DENTER(GUI_LAYER, "qmonCQActivate");

   if (rmon_mlgetl(&DEBUG_ON, GUI_LAYER) & INFOPRINT) {
      printf("ACTIVATE: item=%s count=%d\n",ret->item->text,ret->count);
      count=0;
      while (count<ret->count) {
         printf(" path: %s\n",ret->path[count]->text);
         count++;
      }
   }
}


/*-------------------------------------------------------------------------*/
static ListTreeItem* CullToTree(
Widget tree,
ListTreeItem *parent,
lList *cql 
) {
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   ListTreeItem *cq_node = NULL;
   ListTreeItem *cq_hostnode = NULL;
#if 0   
   ListTreeItem *cq_attributes = NULL;
   ListTreeItem *cq_attributes_gen = NULL;
   ListTreeItem *cq_attributes_em = NULL;
   ListTreeItem *cq_attributes_ckpt = NULL;
   ListTreeItem *cq_attributes_pe = NULL;
   ListTreeItem *cq_attributes_lst = NULL;
   ListTreeItem *cq_attributes_hlim = NULL;
   ListTreeItem *cq_attributes_slim = NULL;
   ListTreeItem *cq_attributes_cplx = NULL;
   ListTreeItem *cq_attributes_acl = NULL;
   ListTreeItem *cq_attributes_prj = NULL;
#endif   
   ListTreeItem *cq_qinstances = NULL;
   ListTreeItem *ret = NULL;
   StringConst cq_name = NULL;

   DENTER(GUI_LAYER, "CullToTree");

   for_each(ep, cql) {
      /*
      ** create new node
      ** 
      */
      cq_name = lGetString(ep, CQ_name) ? lGetString(ep, CQ_name) : "-NA-";
      cq_node = ListTreeAddBranch(tree, parent, (char*) cq_name);
      
      /*
      ** CQ_hostlist
      */
      cq_hostnode = ListTreeAddBranch(tree, cq_node, (char*)"Hostlist");
      for_each(ep2, lGetList(ep, CQ_hostlist)) { 
         ListTreeAddLeaf(tree, cq_hostnode, (char*)lGetHost(ep2,HR_name));
      }   

      /*
      ** CQ_qinstances
      ** FIXME
      */
      cq_qinstances = ListTreeAddBranch(tree, cq_node, (char*) "Queue Instances");
      for_each(ep2, lGetList(ep, CQ_qinstances)) { 
         ListTreeAddLeaf(tree, cq_qinstances, (char*)lGetString(ep2,QU_full_name));
      }   

      
#if 0      
      /*
      ** Attributes pseudo nodes
      */
      cq_attributes = ListTreeAddBranch(tree, cq_node, (char*) "Attributes");
      cq_attributes_gen = ListTreeAddBranch(tree, cq_attributes, (char*) "General");
      cq_attributes_em = ListTreeAddBranch(tree, cq_attributes, (char*) "Execution Method");
      cq_attributes_ckpt = ListTreeAddBranch(tree, cq_attributes, (char*) "Checkpointing");
      cq_attributes_pe = ListTreeAddBranch(tree, cq_attributes, (char*) "Parallel Environment");
      cq_attributes_lst = ListTreeAddBranch(tree, cq_attributes, (char*) "Load/Suspend Thresholds");
      cq_attributes_hlim = ListTreeAddBranch(tree, cq_attributes, (char*) "Hard Limits");
      cq_attributes_slim = ListTreeAddBranch(tree, cq_attributes, (char*) "Soft Limits");
      cq_attributes_cplx = ListTreeAddBranch(tree, cq_attributes, (char*) "Complexes");
      cq_attributes_acl = ListTreeAddBranch(tree, cq_attributes, (char*) "Access");
      cq_attributes_prj = ListTreeAddBranch(tree, cq_attributes, (char*) "Project Access");
      
      
      /*--------- general config --------------------------------------------*/
      
      cq_add_aqtlist(tree, cq_attributes_gen, ep, CQ_qtype, "CQ_qtype"); 

      cq_add_aulng(tree, cq_attributes_gen, ep, CQ_seq_no, "Sequence Number");

      cq_add_aulng(tree, cq_attributes_gen, ep, CQ_job_slots, "Job Slots"); 

      cq_add_aulng(tree, cq_attributes_gen, ep, CQ_fshare, "CQ_fshare"); 

      cq_add_aulng(tree, cq_attributes_gen, ep, CQ_oticket, "CQ_oticket"); 

      cq_add_abool(tree, cq_attributes_gen, ep, CQ_rerun, "Rerun Jobs"); 

      cq_add_astr(tree, cq_attributes_gen, ep, CQ_shell_start_mode, "CQ_shell_start_mode"); 

      cq_add_ainter(tree, cq_attributes_gen, ep, CQ_notify, "CQ_notify"); 

      cq_add_astr(tree, cq_attributes_gen, ep, CQ_tmpdir, "CQ_tmpdir"); 

      cq_add_astr(tree, cq_attributes_gen, ep, CQ_shell, "CQ_shell"); 

      cq_add_astr(tree, cq_attributes_gen, ep, CQ_calendar, "CQ_calendar"); 

      cq_add_astr(tree, cq_attributes_gen, ep, CQ_priority, "CQ_priority"); 

      cq_add_astr(tree, cq_attributes_gen, ep, CQ_processors, "CQ_processors"); 

      cq_add_astr(tree, cq_attributes_gen, ep, CQ_initial_state, "CQ_initial_state"); 
      /*--------- hard limits --------------------------------------------*/
      cq_add_atime(tree, cq_attributes_hlim, ep, CQ_h_rt, "Wallclock Time (sec)"); 
      cq_add_atime(tree, cq_attributes_hlim, ep, CQ_h_cpu, "CPU Time (sec)"); 
      cq_add_amem(tree, cq_attributes_hlim, ep, CQ_h_fsize, "File Size (Byte)"); 
      cq_add_amem(tree, cq_attributes_hlim, ep, CQ_h_data, "Data Size (Byte)"); 
      cq_add_amem(tree, cq_attributes_hlim, ep, CQ_h_stack, "Stack Size (Byte)"); 
      cq_add_amem(tree, cq_attributes_hlim, ep, CQ_h_core, "Corefile Size (Byte)"); 
      cq_add_amem(tree, cq_attributes_hlim, ep, CQ_h_rss, "Resident Set Size (Byte)"); 
      cq_add_amem(tree, cq_attributes_hlim, ep, CQ_h_vmem, "Virtual Memory (Byte)"); 

      /*--------- soft limits --------------------------------------------*/
      cq_add_atime(tree, cq_attributes_slim, ep, CQ_s_rt, "Wallclock Time (sec)"); 
      cq_add_atime(tree, cq_attributes_slim, ep, CQ_s_cpu, "CPU Time (sec)"); 
      cq_add_amem(tree, cq_attributes_slim, ep, CQ_s_fsize, "File Size (Byte)"); 
      cq_add_amem(tree, cq_attributes_slim, ep, CQ_s_data, "Data Size (Byte)"); 
      cq_add_amem(tree, cq_attributes_slim, ep, CQ_s_stack, "Stack Size (Byte)"); 
      cq_add_amem(tree, cq_attributes_slim, ep, CQ_s_core, "Corefile Size (Byte)"); 
      cq_add_amem(tree, cq_attributes_slim, ep, CQ_s_rss, "Resident Set Size (Byte)"); 
      cq_add_amem(tree, cq_attributes_slim, ep, CQ_s_vmem, "Virtual Memory (Byte)"); 

      /*--------- ckpt --------------------------------------------*/
      cq_add_ainter(tree, cq_attributes_ckpt, ep, CQ_min_cpu_interval, "Min CPU Interval"); 

      cq_add_astrlist(tree, cq_attributes_ckpt, ep, CQ_ckpt_list, "CQ_ckpt_list"); 
      
      /*--------- execution method ------------------------------------------*/
      cq_add_astr(tree, cq_attributes_em, ep, CQ_prolog, "CQ_prolog"); 

      cq_add_astr(tree, cq_attributes_em, ep, CQ_epilog, "CQ_epilog"); 

      cq_add_astr(tree, cq_attributes_em, ep, CQ_starter_method, "CQ_starter_method"); 

      cq_add_astr(tree, cq_attributes_em, ep, CQ_suspend_method, "CQ_suspend_method"); 

      cq_add_astr(tree, cq_attributes_em, ep, CQ_resume_method, "CQ_resume_method"); 
      cq_add_astr(tree, cq_attributes_em, ep, CQ_terminate_method, "CQ_terminate_method"); 


      /*--------- pe  ------------------------------------------*/
      cq_add_astrlist(tree, cq_attributes_pe, ep, CQ_pe_list, "Parallel Environment"); 

      /*--------- Owners  ------------------------------------------*/
      cq_add_ausrlist(tree, cq_attributes, ep, CQ_owner_list, "Owners"); 

      /*--------- Access -------------------------------------------*/
      cq_add_ausrlist(tree, cq_attributes_acl, ep, CQ_acl, "CQ_acl"); 

      cq_add_ausrlist(tree, cq_attributes_acl, ep, CQ_xacl, "CQ_xacl"); 

      /*--------- Projects -------------------------------------------*/
      cq_add_aprjlist(tree, cq_attributes_prj, ep, CQ_projects, "CQ_projects"); 

      cq_add_aprjlist(tree, cq_attributes_prj, ep, CQ_xprojects, "CQ_xprojects"); 

      /*--------- Load/Suspend Thresholds ---------------------------------*/
      cq_add_acelist(tree, cq_attributes_lst, ep, CQ_load_thresholds, "CQ_load_thresholds"); 

      cq_add_acelist(tree, cq_attributes_lst, ep, CQ_suspend_thresholds, "CQ_suspend_thresholds"); 

      cq_add_aulng(tree, cq_attributes_lst, ep, CQ_nsuspend, "Jobs suspended per interval"); 

      cq_add_ainter(tree, cq_attributes_lst, ep, CQ_suspend_interval, "Suspend Interval"); 
      

      /*--------- Consumables ---------------------------------*/
      cq_add_acelist(tree, cq_attributes, ep, CQ_consumable_config_list, "CQ_consumable_config_list"); 

      /*--------- Subordinates ---------------------------------*/
      cq_add_asolist(tree, cq_attributes, ep, CQ_subordinate_list, "CQ_subordinate_list"); 

#endif

   }

   ret = ListTreeFirstItem(tree);


   DEXIT;
   return ret; 
}

#if 0
/*-------------------------------------------------------------------------*/
static ListTreeItem* cq_add_aulng(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name)
{
   lListElem *ep;
   ListTreeItem *item;
   
   DENTER(GUI_LAYER, "cq_add_aulng");

   item = ListTreeAddBranch(tree, parent, (char*)attribute_name);
   for_each(ep, lGetList(elem, field_nm)) {
      char *hostref = (char*) lGetHost(ep, AULNG_href);
      u_long32 u = lGetUlong(ep, AULNG_value);
      dstring ds = DSTRING_INIT;
      sge_dstring_sprintf(&ds, "%s " u32, hostref, u);
      ListTreeAddLeaf(tree, item, ds.s);
      sge_dstring_free(&ds);
   }   
   DEXIT;
   return item;
}   

/*-------------------------------------------------------------------------*/
static ListTreeItem* cq_add_abool(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name)
{
   lListElem *ep;
   ListTreeItem *item;
   
   DENTER(GUI_LAYER, "cq_add_abool");

   item = ListTreeAddBranch(tree, parent, (char*)attribute_name);
   for_each(ep, lGetList(elem, field_nm)) {
      char *hostref = (char*) lGetHost(ep, ABOOL_href);
      Bool b = lGetBool(ep, ABOOL_value);
      dstring ds = DSTRING_INIT;
      sge_dstring_sprintf(&ds, "%s %s", hostref, b ? "true" : "false");
      ListTreeAddLeaf(tree, item, ds.s);
      sge_dstring_free(&ds);
   }   
   DEXIT;
   return item;
}   

/*-------------------------------------------------------------------------*/
static ListTreeItem* cq_add_amem(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name)
{
   lListElem *ep;
   ListTreeItem *item;
   
   DENTER(GUI_LAYER, "cq_add_amem");

   item = ListTreeAddBranch(tree, parent, (char*)attribute_name);
   for_each(ep, lGetList(elem, field_nm)) {
      char *hostref = (char*) lGetHost(ep, AMEM_href);
      char *value = (char*)lGetString(ep, AMEM_value);
      dstring ds = DSTRING_INIT;
      sge_dstring_sprintf(&ds, "%s %s", hostref, value ? value : "-NA-");
      ListTreeAddLeaf(tree, item, ds.s);
      sge_dstring_free(&ds);
   }   
   DEXIT;
   return item;
}   

/*-------------------------------------------------------------------------*/
static ListTreeItem* cq_add_atime(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name)
{
   lListElem *ep;
   ListTreeItem *item;
   
   DENTER(GUI_LAYER, "cq_add_atime");

   item = ListTreeAddBranch(tree, parent, (char*)attribute_name);
   for_each(ep, lGetList(elem, field_nm)) {
      char *hostref = (char*) lGetHost(ep, ATIME_href);
      char *value = (char*)lGetString(ep, ATIME_value);
      dstring ds = DSTRING_INIT;
      sge_dstring_sprintf(&ds, "%s %s", hostref, value ? value : "-NA-");
      ListTreeAddLeaf(tree, item, ds.s);
      sge_dstring_free(&ds);
   }   
   DEXIT;
   return item;
}   

/*-------------------------------------------------------------------------*/
static ListTreeItem* cq_add_ainter(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name)
{
   lListElem *ep;
   ListTreeItem *item;
   
   DENTER(GUI_LAYER, "cq_add_ainter");

   item = ListTreeAddBranch(tree, parent, (char*)attribute_name);
   for_each(ep, lGetList(elem, field_nm)) {
      char *hostref = (char*) lGetHost(ep, AINTER_href);
      char *value = (char*)lGetString(ep, AINTER_value);
      dstring ds = DSTRING_INIT;
      sge_dstring_sprintf(&ds, "%s %s", hostref, value ? value : "-NA-");
      ListTreeAddLeaf(tree, item, ds.s);
      sge_dstring_free(&ds);
   }   
   DEXIT;
   return item;
}   

/*-------------------------------------------------------------------------*/
static ListTreeItem* cq_add_astr(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name)
{
   lListElem *ep;
   ListTreeItem *item;
   
   DENTER(GUI_LAYER, "cq_add_astr");

   item = ListTreeAddBranch(tree, parent, (char*)attribute_name);
   for_each(ep, lGetList(elem, field_nm)) {
      char *hostref = (char*) lGetHost(ep, ASTR_href);
      char *value = (char*)lGetString(ep, ASTR_value);
      dstring ds = DSTRING_INIT;
      sge_dstring_sprintf(&ds, "%s %s", hostref, value ? value : "-NA-");
      ListTreeAddLeaf(tree, item, ds.s);
      sge_dstring_free(&ds);
   }   
   DEXIT;
   return item;
}   

/*-------------------------------------------------------------------------*/
static ListTreeItem* cq_add_astrlist(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name)
{
   lListElem *ep;
   ListTreeItem *item;
   
   DENTER(GUI_LAYER, "cq_add_astrlist");

   item = ListTreeAddBranch(tree, parent, (char*)attribute_name);
   for_each(ep, lGetList(elem, field_nm)) {
      ListTreeItem *sublist;
      lListElem *ep2;
      char *hostref = (char*) lGetHost(ep, ASTRLIST_href);
      lList *value = lGetList(ep, ASTRLIST_value);
      sublist = ListTreeAddBranch(tree, item, hostref);
      for_each(ep2, value) { 
         char *name = (char*) lGetString(ep2, ST_name);
         ListTreeAddLeaf(tree, sublist, name ? name : "-NA-");
      }   
   }   
   DEXIT;
   return item;
}   

/*-------------------------------------------------------------------------*/
static ListTreeItem* cq_add_ausrlist(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name)
{
   lListElem *ep;
   ListTreeItem *item;
   
   DENTER(GUI_LAYER, "cq_add_ausrlist");

   item = ListTreeAddBranch(tree, parent, (char*)attribute_name);
   for_each(ep, lGetList(elem, field_nm)) {
      ListTreeItem *sublist;
      lListElem *ep2;
      char *hostref = (char*) lGetHost(ep, AUSRLIST_href);
      lList *value = lGetList(ep, AUSRLIST_value);
      sublist = ListTreeAddBranch(tree, item, hostref);
      for_each(ep2, value) { 
         char *name = (char*) lGetString(ep2, US_name);
         ListTreeAddLeaf(tree, sublist, name ? name : "-NA-");
      }   
   }   
   DEXIT;
   return item;
}   

/*-------------------------------------------------------------------------*/
static ListTreeItem* cq_add_aprjlist(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name)
{
   lListElem *ep;
   ListTreeItem *item;
   
   DENTER(GUI_LAYER, "cq_add_aprjlist");

   item = ListTreeAddBranch(tree, parent, (char*)attribute_name);
   for_each(ep, lGetList(elem, field_nm)) {
      ListTreeItem *sublist;
      lListElem *ep2;
      char *hostref = (char*) lGetHost(ep, APRJLIST_href);
      lList *value = lGetList(ep, APRJLIST_value);
      sublist = ListTreeAddBranch(tree, item, hostref);
      for_each(ep2, value) { 
         char *name = (char*) lGetString(ep2, UP_name);
         ListTreeAddLeaf(tree, sublist, name ? name : "-NA-");
      }   
   }   
   DEXIT;
   return item;
}   

/*-------------------------------------------------------------------------*/
static ListTreeItem* cq_add_aqtlist(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name)
{
   lListElem *ep;
   ListTreeItem *item;
   
   DENTER(GUI_LAYER, "cq_add_aqtlist");

   item = ListTreeAddBranch(tree, parent, (char*)attribute_name);
#if 0   
   for_each(ep, lGetList(elem, field_nm)) {
      ListTreeItem *sublist;
      char *hostref = (char*) lGetHost(ep, AQTLIST_href);
      u_long32 u = lGetUlong(ep, AQTLIST_value);
      sublist = ListTreeAddBranch(tree, item, hostref);
      for_each(ep2, value) { 
         char *name = lGetString(ep2, UP_name);
         ListTreeAddLeaf(tree, sublist, name ? name : "-NA-");
      }   
   }   
#endif   
   DEXIT;
   return item;
}   

/*-------------------------------------------------------------------------*/
static ListTreeItem* cq_add_acelist(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name)
{
   lListElem *ep;
   ListTreeItem *item;
   
   DENTER(GUI_LAYER, "cq_add_acelist");

   item = ListTreeAddBranch(tree, parent, (char*)attribute_name);
#if 0   
   for_each(ep, lGetList(elem, field_nm)) {
      ListTreeItem *sublist;
      char *hostref = (char*) lGetHost(ep, AQTLIST_href);
      u_long32 u = lGetUlong(ep, AQTLIST_value);
      sublist = ListTreeAddBranch(tree, item, hostref);
      for_each(ep2, value) { 
         char *name = lGetString(ep2, UP_name);
         ListTreeAddLeaf(tree, sublist, name ? name : "-NA-");
      }   
   }   
#endif   
   DEXIT;
   return item;
}   

/*-------------------------------------------------------------------------*/
static ListTreeItem* cq_add_asolist(Widget tree, ListTreeItem *parent, 
                           lListElem *elem, int field_nm, 
                           StringConst attribute_name)
{
   lListElem *ep;
   lListElem *ep2;
   ListTreeItem *item;
   
   DENTER(GUI_LAYER, "cq_add_asolist");

   item = ListTreeAddBranch(tree, parent, (char*)attribute_name);
   for_each(ep, lGetList(elem, field_nm)) {
      ListTreeItem *sublist;
      char *hostref = (char*) lGetHost(ep, AQTLIST_href);
      u_long32 u = lGetUlong(ep, AQTLIST_value);
      sublist = ListTreeAddBranch(tree, item, hostref);
      for_each(ep2, value) { 
         char *name = lGetString(ep2, UP_name);
         ListTreeAddLeaf(tree, sublist, name ? name : "-NA-");
      }   
   }   
   
   DEXIT;
   return item;
}   
#endif

/*-------------------------------------------------------------------------*/
static void qmonQinstanceHandleEnterLeave(
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
   lListElem *qp;
   String queue_info;
   
   DENTER(GUI_LAYER, "qmonQinstanceHandleEnterLeave");
   
   switch (ev->type) {
      case MotionNotify:
         if (qmonBrowserObjectEnabled(BROWSE_QUEUE)) {
            /* 
            ** XQueryPointer(XtDisplay(w), XtWindow(w), 
            **            &root, &child, &root_x, &root_y,
            **            &pos_x, &pos_y, &keys_buttons);
            */
            if (XbaeMatrixGetEventRowColumn(w, ev, &row, &col)) {
               if ( row != prev_row ) {
                  prev_row = row;
                  /* locate the quue */
                  str = XbaeMatrixGetCell(w, row, 0);
                  if (str && *str != '\0') {
                     qp = cqueue_list_locate_qinstance(qmonMirrorList(SGE_CQUEUE_LIST), str);
                     if (qp) {
                        sprintf(line, "+++++++++++++++++++++++++++++++++++++++++++\n");  
                        qmonBrowserShow(line);
                        queue_info = qmonQinstanceShowBrowserInfo(qp);      
                        qmonBrowserShow(queue_info);
                        qmonBrowserShow(line);
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
static String qmonQinstanceShowBrowserInfo(
lListElem *qep 
) {

   static char info[60000];
   lListElem *ep;
   int qtype;
   const char *str, *str2;
#define WIDTH  "%s%-30.30s"

   DENTER(GUI_LAYER, "qmonQinstanceShowBrowserInfo");

   sprintf(info, WIDTH"%s\n", "\n","Queue:", lGetString(qep, QU_full_name));

   qtype = lGetUlong(qep, QU_qtype);

   {
      dstring type_buffer = DSTRING_INIT;

      qinstance_print_qtype_to_dstring(qep, &type_buffer, false);
      sprintf(info, WIDTH"%s\n", info, "Type:", 
              sge_dstring_get_string(&type_buffer));
      sge_dstring_free(&type_buffer);
   }
   sprintf(info, WIDTH"%d\n", info, "Sequence Nr:", 
                           (int)lGetUlong(qep, QU_seq_no));

   str = lGetString(qep, QU_tmpdir);
   sprintf(info, WIDTH"%s\n", info, "tmpdir:", str ? str : ""); 
   str = lGetString(qep, QU_shell);
   sprintf(info, WIDTH"%s\n", info, "Shell:", str ? str : ""); 
   sprintf(info, WIDTH"%d\n", info, "Job Slots:", 
                     (int)lGetUlong(qep, QU_job_slots));
   sprintf(info, WIDTH"%d\n", info, "Job Slots Used:", qinstance_slots_used(qep));
   str = lGetString(qep, QU_priority);
   sprintf(info, WIDTH"%s\n", info, "Priority:", str?str:"");
   sprintf(info, WIDTH"", info, "Load Thresholds:");
   for_each(ep, lGetList(qep, QU_load_thresholds)) {
      str = lGetString(ep, CE_name);
      str2 = lGetString(ep, CE_stringval);
      sprintf(info, "%s%s = %s ", info, str?str:"", str2?str2:"");
   }
   sprintf(info, "%s\n", info); 

   sprintf(info, WIDTH"%s\n", info, "Rerun Job:", 
                     lGetBool(qep, QU_rerun) ? "True" : "False");

   str = lGetString(qep, QU_notify);
   sprintf(info, WIDTH"%s\n", info, "Notify Job Interval:",  str ? str : ""); 

   str = lGetString(qep, QU_processors);
   sprintf(info, WIDTH"%s\n", info, "Processors:", str ? str : ""); 

   str = lGetString(qep, QU_s_rt);
   sprintf(info, WIDTH"%s\n", info, "Soft Real Time:", str ? str : ""); 
   str = lGetString(qep, QU_h_rt);
   sprintf(info, WIDTH"%s\n", info, "Hard Real Time:", str ? str : ""); 
   str = lGetString(qep, QU_s_cpu);
   sprintf(info, WIDTH"%s\n", info, "Soft Cpu:", str ? str : ""); 
   str = lGetString(qep, QU_h_cpu);
   sprintf(info, WIDTH"%s\n", info, "Hard Cpu:", str ? str : "");
   str = lGetString(qep, QU_s_fsize);
   sprintf(info, WIDTH"%s\n", info, "Soft File Size:", str ? str : "");
   str = lGetString(qep, QU_h_fsize);
   sprintf(info, WIDTH"%s\n", info, "Hard File Size:", str ? str : "");
   str = lGetString(qep, QU_s_data);
   sprintf(info, WIDTH"%s\n", info, "Soft Data Size:", str ? str : "");
   str = lGetString(qep, QU_h_data);
   sprintf(info, WIDTH"%s\n", info, "Hard Data Size:", str ? str : "");
   str = lGetString(qep, QU_s_stack);
   sprintf(info, WIDTH"%s\n", info, "Soft Stack Size:", str ? str : "");
   str = lGetString(qep, QU_h_stack);
   sprintf(info, WIDTH"%s\n", info, "Hard Stack Size:", str ? str : "");
   str = lGetString(qep, QU_s_core);
   sprintf(info, WIDTH"%s\n", info, "Soft Core Size:", str ? str : "");
   str = lGetString(qep, QU_h_core);
   sprintf(info, WIDTH"%s\n", info, "Hard Core Size:", str ? str : "");
   str = lGetString(qep, QU_s_rss);
   sprintf(info, WIDTH"%s\n", info, "Soft Resident Set Size:", str ? str : "");
   str = lGetString(qep, QU_h_rss);
   sprintf(info, WIDTH"%s\n", info, "Hard Resident Set Size:", str ? str : "");

   str = lGetString(qep, QU_min_cpu_interval);
   sprintf(info, WIDTH"%s\n", info, "Min Cpu Interval:", str ? str : "");

   sprintf(info, WIDTH"", info, "Access List:");
   for_each(ep, lGetList(qep, QU_acl)) {
      sprintf(info, "%s%s ", info, lGetString(ep, US_name));
   }
   sprintf(info, "%s\n", info); 
   sprintf(info, WIDTH"", info, "No Access List:");
   for_each(ep, lGetList(qep, QU_xacl)) {
      sprintf(info, "%s%s ", info, lGetString(ep, US_name));
   }
   sprintf(info, "%s\n", info); 

   DPRINTF(("info is %d long\n", strlen(info)));
   
#undef WIDTH
   DEXIT;
   return info;
}

/*-------------------------------------------------------------------------*/
static void qmonCQHandleEnterLeave(
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
   lListElem *qp;
   String queue_info;
   
   DENTER(GUI_LAYER, "qmonCQHandleEnterLeave");
   
   switch (ev->type) {
      case MotionNotify:
         if (qmonBrowserObjectEnabled(BROWSE_QUEUE)) {
            /* 
            ** XQueryPointer(XtDisplay(w), XtWindow(w), 
            **            &root, &child, &root_x, &root_y,
            **            &pos_x, &pos_y, &keys_buttons);
            */
            if (XbaeMatrixGetEventRowColumn(w, ev, &row, &col)) {
               if ( row != prev_row ) {
                  prev_row = row;
                  /* locate the quue */
                  str = XbaeMatrixGetCell(w, row, 0);
                  if (str && *str != '\0') {
                     qp = cqueue_list_locate(qmonMirrorList(SGE_CQUEUE_LIST), str);
                     if (qp) {
                        sprintf(line, "+++++++++++++++++++++++++++++++++++++++++++\n");  
                        qmonBrowserShow(line);
                        queue_info = qmonCQShowBrowserInfo(qp);      
                        qmonBrowserShow(queue_info);
                        qmonBrowserShow(line);
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
static String qmonCQShowBrowserInfo(
lListElem *qep 
) {

   static char info[60000];
#define WIDTH  "%s%-30.30s"

   DENTER(GUI_LAYER, "qmonCQShowBrowserInfo");

   sprintf(info, WIDTH"%s\n", "\n","Cluster Queue:", lGetString(qep, CQ_name));

   sprintf(info, "%s\n", info); 

   DPRINTF(("info is %d long\n", strlen(info)));
   
#undef WIDTH
   DEXIT;
   return info;
}

/*-------------------------------------------------------------------------*/
static void qmonCQDelete(Widget w, XtPointer cld, XtPointer cad) 
{
   int i;
   int rows;
   lList *alp = NULL;
   const char *str;
   Boolean status, answer;
   Widget matrix = cluster_queue_settings;

   DENTER(GUI_LAYER, "qmonCQDelete");

/*    force = XmToggleButtonGetState(force_toggle); */

   /* 
   ** loop over selected entries and delete them
   */
   rows = XbaeMatrixNumRows(matrix);
   if (XbaeMatrixGetNumSelected(matrix) > 0) {
      status = XmtAskForBoolean(w, "xmtBooleanDialog", 
                     "@{queue.askdel.Do you really want to\ndelete the selected queues ?}", 
                     "@{Delete}", "@{Cancel}", NULL, XmtNoButton, XmDIALOG_WARNING, 
                     False, &answer, NULL);
         
      if (answer) { 
         for (i=0; i<rows; i++) {
            /* is this row selected */ 
            if (XbaeMatrixIsRowSelected(matrix, i)) {
               str = XbaeMatrixGetCell(cluster_queue_settings, i, 0);
               if ( str && *str != '\0' ) { 
                  DPRINTF(("CQ to delete: %s\n", str));
                  cqueue_delete(&alp, str);
               }
            }
         }
      }
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      /*
      ** update the matrix
      */
      qmonCQUpdateCQMatrix();
   }


   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonCQChangeState(Widget w, XtPointer cld, XtPointer cad)
{
   int rows;
   const char *str;
   int i;
   lList *ql = NULL;
   lList *alp = NULL;
   int force = 0;
   long action = (long) cld;
   Widget force_toggle;
   Widget matrix = current_matrix;
   
   DENTER(GUI_LAYER, "qmonCQChangeState");

   /* 
   ** loop over selected entries and build id list
   */
   rows = XbaeMatrixNumRows(matrix);
   if (XbaeMatrixGetNumSelected(matrix) > 0) {
      /*
      ** cld contains the action we need, check if a force is involved
      */
      force_toggle = XmtNameToWidget(w, "*cq_force"); 
      force = XmToggleButtonGetState(force_toggle);
   
      for (i=0; i<rows; i++) {
         /* is this row selected */ 
         if (XbaeMatrixIsRowSelected(matrix, i)) {
            str = XbaeMatrixGetCell(matrix, i, 0);
            if ( str && *str != '\0' ) { 
               DPRINTF(("CQ/QI to change: %s\n", str));
               lAddElemStr(&ql, ST_name, str, ST_Type);
            }
         }
      }
      if (ql) {
         /* EB: TODO: */
         alp = qmonChangeStateList(SGE_CQUEUE_LIST, ql, force, action); 
      
         qmonMessageBox(w, alp, 0);
         /*
         ** update the matrix
         */
         if (current_matrix == cluster_queue_settings) {
            qmonCQUpdateCQMatrix();
         } else {   
            qmonCQUpdateQIMatrix();
         }   
         ql = lFreeList(ql);
         alp = lFreeList(alp);
      }
   }

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonCQFolderChange(Widget w, XtPointer cld, XtPointer cad)
{
   XmTabCallbackStruct *cbs = (XmTabCallbackStruct*)cad;
   
   DENTER(GUI_LAYER, "qmonCQFolderChange");

   DPRINTF(("%s\n", XtName(cbs->tab_child)));

   if (!strcmp(XtName(cbs->tab_child), "clusterqueue_layout")) {
      current_matrix = cluster_queue_settings;
      qmonCQUpdateCQMatrix();
   } else { 
      current_matrix = qinstance_settings;
      qmonCQUpdateQIMatrix();
   }   

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonCQUpdateCQMatrix(void)
{
   lList *alp = NULL;
   lList *ehl = NULL;
   lList *cl = NULL;
   lList *ql = NULL;
   lListElem *cq = NULL;
   int row;
   char buf[BUFSIZ];

   DENTER(GUI_LAYER, "qmonCQUpdateCQMatrix");

   qmonMirrorMultiAnswer(CQUEUE_T | EXECHOST_T | CENTRY_T,  &alp);
   if (alp) {
      qmonMessageBox(cluster_queue_settings, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }

   
   ehl = qmonMirrorList(SGE_EXECHOST_LIST);
   cl = qmonMirrorList(SGE_CENTRY_LIST);
   ql = qmonMirrorList(SGE_CQUEUE_LIST);

   /*
   ** Cluster Queue Pane
   */
   XtVaSetValues(cluster_queue_settings, XmNcells, NULL, NULL);

   row = 0;
   for_each (cq, ql) {
      double load = 0.0;
      u_long32 used, total;
      u_long32 temp_disabled, available, manual_intervention;
      u_long32 suspend_manual, suspend_threshold, suspend_on_subordinate;
      u_long32 suspend_calendar, unknown, load_alarm;
      u_long32 disabled_manual, disabled_calendar, ambiguous;
      u_long32 orphaned, error;
      bool is_load_available;

      cqueue_calculate_summary(cq, ehl, cl, 
                            &load, &is_load_available, &used, &total,
                            &suspend_manual, &suspend_threshold,
                            &suspend_on_subordinate, &suspend_calendar,
                            &unknown, &load_alarm, &disabled_manual,
                            &disabled_calendar, &ambiguous, &orphaned,
                            &error, &available, &temp_disabled,
                            &manual_intervention);

      XbaeMatrixSetCell(cluster_queue_settings, row, 0, (char*)lGetString(cq, CQ_name));
      if (is_load_available) {
         sprintf(buf, "%7.2f ", load);
      } else {
         sprintf(buf, "%7s ", "-NA-");
      }
      XbaeMatrixSetCell(cluster_queue_settings, row, 1, buf);
      sprintf(buf, "%6d ", (int)used);
      XbaeMatrixSetCell(cluster_queue_settings, row, 2, buf);
      sprintf(buf, "%6d ", (int)available);
      XbaeMatrixSetCell(cluster_queue_settings, row, 3, buf);
      sprintf(buf, "%6d ", (int)total);
      XbaeMatrixSetCell(cluster_queue_settings, row, 4, buf);
      sprintf(buf, "%6d ", (int)temp_disabled);
      XbaeMatrixSetCell(cluster_queue_settings, row, 5, buf);
      sprintf(buf, "%6d ", (int)manual_intervention);
      XbaeMatrixSetCell(cluster_queue_settings, row, 6, buf);
      sprintf(buf, "%5d ", (int)suspend_manual);
      XbaeMatrixSetCell(cluster_queue_settings, row, 7, buf);
      sprintf(buf, "%5d ", (int)suspend_threshold);
      XbaeMatrixSetCell(cluster_queue_settings, row, 8, buf);
      sprintf(buf, "%5d ", (int)suspend_on_subordinate);
      XbaeMatrixSetCell(cluster_queue_settings, row, 9, buf);
      sprintf(buf, "%5d ", (int)suspend_calendar);
      XbaeMatrixSetCell(cluster_queue_settings, row, 10, buf);
      sprintf(buf, "%5d ", (int)unknown);
      XbaeMatrixSetCell(cluster_queue_settings, row, 11, buf);
      sprintf(buf, "%5d ", (int)load_alarm);
      XbaeMatrixSetCell(cluster_queue_settings, row, 12, buf);
      sprintf(buf, "%5d ", (int)disabled_manual);
      XbaeMatrixSetCell(cluster_queue_settings, row, 13, buf);
      sprintf(buf, "%5d ", (int)disabled_calendar);
      XbaeMatrixSetCell(cluster_queue_settings, row, 14, buf);
      sprintf(buf, "%5d ", (int)ambiguous);
      XbaeMatrixSetCell(cluster_queue_settings, row, 15, buf);
      sprintf(buf, "%5d ", (int)orphaned);
      XbaeMatrixSetCell(cluster_queue_settings, row, 16, buf);
      sprintf(buf, "%5d ", (int)error);
      XbaeMatrixSetCell(cluster_queue_settings, row, 17, buf);
   
      row++;
   }   
}

/*-------------------------------------------------------------------------*/
static void qmonCQUpdateQIMatrix(void)
{
   lList *alp = NULL;
   lList *ehl = NULL;
   lList *cl = NULL;
   lList *ql = NULL;
   lListElem *cq = NULL;
   int row;
   char buf[BUFSIZ];

   DENTER(GUI_LAYER, "qmonCQUpdateQIMatrix");

   qmonMirrorMultiAnswer(CQUEUE_T | EXECHOST_T | CENTRY_T,  &alp);
   if (alp) {
      qmonMessageBox(qinstance_settings, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }

   ehl = qmonMirrorList(SGE_EXECHOST_LIST);
   cl = qmonMirrorList(SGE_CENTRY_LIST);
   ql = qmonMirrorList(SGE_CQUEUE_LIST);

   /*
   ** Cluster Queue Instances Pane
   */
   XtVaSetValues(qinstance_settings, XmNcells, NULL, NULL);

   row=0;
   for_each(cq, ql) {
      lListElem *qp;
      char to_print[80];
      char arch_string[80];
      double load_avg;
      char *load_avg_str;
      char load_alarm_reason[MAX_STRING_SIZE];
      char suspend_alarm_reason[MAX_STRING_SIZE];
      bool is_load_value;
      bool has_value_from_object; 

      for_each(qp, lGetList(cq, CQ_qinstances)) {
         /* compute the load and check for alarm states */
         is_load_value = sge_get_double_qattr(&load_avg, load_avg_str, qp, ehl, cl, 
                                                &has_value_from_object);
         if (sge_load_alarm(NULL, qp, lGetList(qp, QU_load_thresholds), ehl, cl, NULL)) {
            qinstance_state_set_alarm(qp, true);
            sge_load_alarm_reason(qp, lGetList(qp, QU_load_thresholds), ehl, 
                            cl, load_alarm_reason, MAX_STRING_SIZE - 1, "load");
         }
         if (sge_load_alarm(NULL, qp, lGetList(qp, QU_suspend_thresholds), ehl, cl, NULL)) {
            qinstance_state_set_suspend_alarm(qp, true);
            sge_load_alarm_reason(qp, lGetList(qp, QU_suspend_thresholds), 
                            ehl, cl, suspend_alarm_reason, 
                            MAX_STRING_SIZE - 1, "suspend");
         }

         /* full qname */
         XbaeMatrixSetCell(qinstance_settings, row, 0, 
                           (char*)lGetString(qp, QU_full_name));
         /* qtype */
         {
            dstring type_string = DSTRING_INIT;

            qinstance_print_qtype_to_dstring(qp, &type_string, true);
            XbaeMatrixSetCell(qinstance_settings, row, 1, 
                              (char*) sge_dstring_get_string(&type_string));
            sge_dstring_free(&type_string);
         }
         /* number of used/free slots */
         sprintf(to_print, "%d/%d ", qinstance_slots_used(qp),
                  (int)lGetUlong(qp, QU_job_slots));
         sprintf(buf, "%-9.9s ", to_print);   
         XbaeMatrixSetCell(qinstance_settings, row, 2, buf);

         /* load avg */
         if (!is_load_value) {
            if (has_value_from_object) {
               sprintf(to_print, "%2.2f ", load_avg);
            } else {
               sprintf(to_print, "---  ");
            }
         } else {
            sprintf(to_print, "-NA- ");
         }
         XbaeMatrixSetCell(qinstance_settings, row, 3, to_print);
   
         /* arch */
         if (!sge_get_string_qattr(arch_string, sizeof(arch_string)-1, 
                                   LOAD_ATTR_ARCH, qp, ehl, cl)) {
            XbaeMatrixSetCell(qinstance_settings, row, 4, arch_string);
         } else {
            XbaeMatrixSetCell(qinstance_settings, row, 4, "-NA-");
         }
         {
            dstring state_string = DSTRING_INIT;

            qinstance_state_append_to_dstring(qp, &state_string);
            XbaeMatrixSetCell(qinstance_settings, row, 5,
                              (char *)sge_dstring_get_string(&state_string));
            sge_dstring_free(&state_string);
         }
         row++;
      }   
   }   

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonCQModify(Widget w, XtPointer cld, XtPointer cad)
{
   int rows = 0;
   int nr_selected_rows;
   int i;
   char *str = NULL;
   lList *alp = NULL;
   Widget matrix = cluster_queue_settings;
   
   DENTER(GUI_LAYER, "qmonCQModify");

   /* 
   ** loop over selected entries and delete them
   */
   rows = XbaeMatrixNumRows(matrix);
   /*
   ** number of selected rows
   */
   nr_selected_rows = XbaeMatrixGetNumSelected(matrix)/XbaeMatrixNumColumns(matrix);
   
   if (nr_selected_rows == 1) {
      for (i=0; i<rows; i++) {
         /* is this row selected */ 
         if (XbaeMatrixIsRowSelected(matrix, i)) {
            str = XbaeMatrixGetCell(cluster_queue_settings, i, 0);
            if ( str && *str != '\0' ) { 
               DPRINTF(("CQ to modify: %s\n", str));
               qmonQCPopup(matrix, (XtPointer)str, NULL); 
            }
         }
      }
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      /*
      ** update the matrix
      */
      qmonCQUpdateCQMatrix();
   } else {
      if (nr_selected_rows > 1) {
         qmonMessageShow(w, True, "@{Select only one queue !}");
      } else {
         qmonMessageShow(w, True, "@{To modify a queue select this queue !}");
      }
   }


   DEXIT;
}


