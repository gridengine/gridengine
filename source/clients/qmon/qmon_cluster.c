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

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/TextF.h>

#include <Xmt/Xmt.h>
#include <Xmt/Create.h>
#include <Xmt/Layout.h>
#include <Xmt/Chooser.h>
#include <Xmt/Dialog.h>
#include <Xmt/Dialogs.h>
#include <Xmt/MsgLine.h>
#include <Xmt/InputField.h>
#include <Xmt/Procedures.h>

/*****/
#include <Xmt/LayoutP.h>
/*****/


#include "sge_all_listsL.h"
#include "sge_gdi.h"
#include "commlib.h"
#include "def.h"
#include "parse_range.h"
#include "qmon_proto.h"
#include "qmon_rmon.h"
#include "qmon_cull.h"
#include "qmon_util.h"
#include "qmon_appres.h"
#include "qmon_timer.h"
#include "qmon_comm.h"
#include "qmon_quarks.h"
#include "qmon_widgets.h"
#include "qmon_message.h"
#include "qmon_cluster.h"
#include "qmon_globals.h"
#include "sge_feature.h"
#include "resolve_host.h"
#include "AskForItems.h"
#include "AskForTime.h"

/*-------------------------------------------------------------------------*/
typedef struct _tCClEntry {
   String qmaster_spool_dir;
   String execd_spool_dir;
   String binary_path;
   String mailer;
   String xterm;
   String load_sensor;
   String prolog;
   String epilog;
   String qlogin_daemon;
   String qlogin_command;
#if 0
   String starter_method;
   String suspend_method;
   String resume_method;
   String terminate_method;
#endif
   String administrator_mail;
   int shell_start_mode;
   String login_shells;
   String default_domain;
   int min_uid;
   int min_gid;
   String load_report_time;
   String stat_log_time;
   String max_unheard;
   String reschedule_unknown;
   int loglevel;
   int ignore_fqdn;
   int logmail;
   int max_aj_instances;
   int max_aj_tasks;
   int max_u_jobs;
   lList *cluster_users;
   lList *cluster_xusers;
   lList *cluster_projects;
   lList *cluster_xprojects;
   int enforce_project;
   int enforce_user;
   String qmaster_params;
   String schedd_params;
   String execd_params;
   String shepherd_cmd;
   String rsh_daemon;
   String rsh_command;
   String rlogin_daemon;
   String rlogin_command;
   String set_token_cmd;
   String pag_cmd;
   String token_extend_time;
   String gid_range;
   String admin_user;
   int zombie_jobs;
} tCClEntry;

XtResource ccl_resources[] = {
   { "qmaster_spool_dir", "qmaster_spool_dir", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, qmaster_spool_dir), 
      XtRImmediate, NULL },

   { "execd_spool_dir", "execd_spool_dir", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, execd_spool_dir), 
      XtRImmediate, NULL },

   { "binary_path", "binary_path", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, binary_path), 
      XtRImmediate, NULL },

   { "mailer", "mailer", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, mailer), 
      XtRImmediate, NULL },

   { "xterm", "xterm", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, xterm),
      XtRImmediate, NULL },

   { "load_sensor", "load_sensor", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, load_sensor),
      XtRImmediate, NULL },

   { "prolog", "prolog", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, prolog), 
      XtRImmediate, NULL },

   { "epilog", "epilog", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, epilog), 
      XtRImmediate, NULL },

   { "qlogin_daemon", "qlogin_daemon", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, qlogin_daemon), 
      XtRImmediate, NULL },

   { "qlogin_command", "qlogin_command", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, qlogin_command), 
      XtRImmediate, NULL },

#if 0
   { "starter_method", "starter_method", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, starter_method), 
      XtRImmediate, NULL },

   { "suspend_method", "suspend_method", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, suspend_method), 
      XtRImmediate, NULL },

   { "resume_method", "resume_method", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, resume_method), 
      XtRImmediate, NULL },

   { "terminate_method", "terminate_method", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, terminate_method), 
      XtRImmediate, NULL },
#endif

   { "login_shells", "login_shells", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, login_shells), 
      XtRImmediate, NULL },

   { "default_domain", "default_domain", XtRString,
      sizeof(String), XtOffsetOf(tCClEntry, default_domain),
      XtRImmediate, NULL }, 

   { "administrator_mail", "administrator_mail", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, administrator_mail),
      XtRImmediate, NULL },

   { "min_uid", "min_uid", XtRInt,
      sizeof(int), XtOffsetOf(tCClEntry, min_uid),
      XtRImmediate, NULL },
   
   { "min_gid", "min_gid", XtRInt,
      sizeof(int), XtOffsetOf(tCClEntry, min_gid),
      XtRImmediate, NULL },

   { "load_report_time", "load_report_time", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, load_report_time), 
      XtRImmediate, NULL },

   { "stat_log_time", "stat_log_time", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, stat_log_time), 
      XtRImmediate, NULL },

   { "max_unheard", "max_unheard", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, max_unheard), 
      XtRImmediate, NULL },
      
   { "reschedule_unknown", "reschedule_unknown", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, reschedule_unknown), 
      XtRImmediate, NULL },
      
   { "loglevel", "loglevel", XtRInt, 
      sizeof(int), XtOffsetOf(tCClEntry, loglevel), 
      XtRImmediate, NULL },
      
   { "ignore_fqdn", "ignore_fqdn", XtRInt, 
      sizeof(int), XtOffsetOf(tCClEntry, ignore_fqdn), 
      XtRImmediate, NULL },
      
   { "logmail", "logmail", XtRInt, 
      sizeof(int), XtOffsetOf(tCClEntry, logmail), 
      XtRImmediate, NULL },

   { "max_aj_instances", "max_aj_instances", XtRInt, 
      sizeof(int), XtOffsetOf(tCClEntry, max_aj_instances), 
      XtRImmediate, NULL },

   { "max_aj_tasks", "max_aj_tasks", XtRInt, 
      sizeof(int), XtOffsetOf(tCClEntry, max_aj_tasks), 
      XtRImmediate, NULL },

   { "max_u_jobs", "max_u_jobs", XtRInt, 
      sizeof(int), XtOffsetOf(tCClEntry, max_u_jobs), 
      XtRImmediate, NULL },

   { "cluster_users", "cluster_users", QmonRUS_Type,
      sizeof(lList *), XtOffsetOf(tCClEntry, cluster_users),
      XtRImmediate, NULL },

   { "cluster_xusers", "cluster_xusers", QmonRUS_Type,
      sizeof(lList *), XtOffsetOf(tCClEntry, cluster_xusers),
      XtRImmediate, NULL },

   { "cluster_projects", "cluster_projects", QmonRUP_Type,
      sizeof(lList *), XtOffsetOf(tCClEntry, cluster_projects),
      XtRImmediate, NULL },

   { "cluster_xprojects", "cluster_xprojects", QmonRUP_Type,
      sizeof(lList *), XtOffsetOf(tCClEntry, cluster_xprojects),
      XtRImmediate, NULL },

   { "enforce_project", "enforce_project", XtRInt, 
      sizeof(int), XtOffsetOf(tCClEntry, enforce_project), 
      XtRImmediate, NULL },

   { "enforce_user", "enforce_user", XtRInt, 
      sizeof(int), XtOffsetOf(tCClEntry, enforce_user), 
      XtRImmediate, NULL },

   { "shell_start_mode", "shell_start_mode", XtRInt, 
      sizeof(int), XtOffsetOf(tCClEntry, shell_start_mode), 
      XtRImmediate, NULL },

   { "qmaster_params", "qmaster_params", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, qmaster_params), 
      XtRImmediate, NULL },

   { "schedd_params", "schedd_params", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, schedd_params), 
      XtRImmediate, NULL },

   { "execd_params", "execd_params", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, execd_params), 
      XtRImmediate, NULL },

   { "shepherd_cmd", "shepherd_cmd", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, shepherd_cmd), 
      XtRImmediate, NULL },

   { "rsh_daemon", "rsh_daemon", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, rsh_daemon), 
      XtRImmediate, NULL },

   { "rsh_command", "rsh_command", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, rsh_command), 
      XtRImmediate, NULL },

   { "rlogin_daemon", "rlogin_daemon", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, rlogin_daemon), 
      XtRImmediate, NULL },

   { "rlogin_command", "rlogin_command", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, rlogin_command), 
      XtRImmediate, NULL },

   { "set_token_cmd", "set_token_cmd", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, set_token_cmd), 
      XtRImmediate, NULL },

   { "pag_cmd", "pag_cmd", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, pag_cmd), 
      XtRImmediate, NULL },

   { "token_extend_time", "token_extend_time", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, token_extend_time), 
      XtRImmediate, NULL },
      
   { "gid_range", "gid_range", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, gid_range), 
      XtRImmediate, NULL },
      
   { "admin_user", "admin_user", XtRString, 
      sizeof(String), XtOffsetOf(tCClEntry, admin_user), 
      XtRImmediate, NULL },
      
   { "zombie_jobs", "zombie_jobs", XtRInt,
      sizeof(int), XtOffsetOf(tCClEntry, zombie_jobs),
      XtRImmediate, NULL },

};


static Widget qmon_cluster = 0;
static Widget cluster_host_list = 0;
static Widget cluster_conf_list = 0;
static Widget cluster_global_layout = 0;
static Widget cluster_host = 0;

static Widget cluster_qmaster_spool_dir = 0;
static Widget cluster_qmaster_spool_dir_label = 0;
static Widget cluster_execd_spool_dir = 0;
static Widget cluster_execd_spool_dir_label = 0;
static Widget cluster_admin_mail = 0;
static Widget cluster_admin_mail_label = 0;
static Widget cluster_login_shells = 0;
static Widget cluster_login_shells_label = 0;
static Widget cluster_default_domain = 0;
static Widget cluster_min_uid = 0;
static Widget cluster_min_gid = 0;
static Widget cluster_max_aj_instances = 0;
static Widget cluster_max_aj_tasks = 0;
static Widget cluster_max_u_jobs = 0;
static Widget cluster_zombie_jobs = 0;
static Widget cluster_load_report_time = 0;
static Widget cluster_load_report_timePB = 0;
static Widget cluster_stat_log_time = 0;
static Widget cluster_stat_log_timePB = 0;
static Widget cluster_max_unheard = 0;
static Widget cluster_max_unheardPB = 0;
static Widget cluster_reschedule_unknown = 0;
static Widget cluster_reschedule_unknownPB = 0;
static Widget cluster_shell_start_mode = 0;
static Widget cluster_loglevel = 0;
static Widget cluster_ignore_fqdn = 0;
static Widget cluster_enforce_project = 0;
static Widget cluster_enforce_user = 0;

static Widget cluster_users = 0;
static Widget cluster_xusers = 0;
static Widget cluster_usersPB = 0;
static Widget cluster_xusersPB = 0;

static Widget cluster_projects_col = 0;
static Widget cluster_xprojects_col = 0;
static Widget cluster_projects = 0;
static Widget cluster_xprojects = 0;
static Widget cluster_projectsPB = 0;
static Widget cluster_xprojectsPB = 0;

static Widget cluster_qmaster_params = 0;
static Widget cluster_schedd_params = 0;
static Widget cluster_execd_params = 0;
static Widget cluster_shepherd_cmd = 0;
static Widget cluster_rsh_daemon = 0;
static Widget cluster_rsh_command = 0;
static Widget cluster_rlogin_daemon = 0;
static Widget cluster_rlogin_command = 0;
static Widget cluster_set_token_cmd = 0;
static Widget cluster_pag_cmd = 0;
static Widget cluster_token_extend_time = 0;
static Widget cluster_gid_range = 0;
static Widget cluster_admin_user = 0;
static Widget cluster_admin_user_label = 0;
static Widget cluster_default_domain_label = 0;
static Widget cluster_qmaster_params_label = 0;
static Widget cluster_schedd_params_label = 0;
static Widget cluster_set_token_cmd_label = 0;
static Widget cluster_pag_cmd_label = 0;
static Widget cluster_token_extend_time_label = 0;

/*-------------------------------------------------------------------------*/
static void qmonPopdownClusterConfig(Widget w, XtPointer cld, XtPointer cad);
static void qmonClusterChange(Widget w, XtPointer cld, XtPointer cad);
static void qmonClusterDelete(Widget w, XtPointer cld, XtPointer cad);
static void qmonSelectHost(Widget w, XtPointer cld, XtPointer cad);
static Widget qmonCreateClusterConfig(Widget parent);
static Widget qmonClusterCreateGlobal(Widget parent);
static void qmonClearCClEntry(tCClEntry *clen); 
static void qmonInitCClEntry(tCClEntry *clen);
static void qmonCullToCClEntry(lListElem *cep, tCClEntry *clen); 
static void qmonClusterFillConf(Widget w, lListElem *ep);
static void qmonClusterOk(Widget w, XtPointer cld, XtPointer cad);
static void qmonClusterCancel(Widget w, XtPointer cld, XtPointer cad);
static Boolean qmonCClEntryToCull(Widget w, tCClEntry *clen, lList **lpp, int local);
static Boolean check_white(char *str);
static void qmonClusterAskForUsers(Widget w, XtPointer cld, XtPointer cad);
static void qmonClusterAskForProjects(Widget w, XtPointer cld, XtPointer cad);
static void qmonClusterLayoutSetSensitive(Boolean mode);
static void qmonClusterHost(Widget w, XtPointer cld, XtPointer cad);
static void qmonClusterCheckInput(Widget w, XtPointer cld, XtPointer cad);
static void qmonClusterTime(Widget w, XtPointer cld, XtPointer cad);
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
void qmonPopupClusterConfig(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget shell;
   lList *alp = NULL;

   DENTER(TOP_LAYER, "qmonPopupClusterConfig");

   /* set busy cursor */
   XmtDisplayBusyCursor(w);

   if (!qmon_cluster) {
      shell = XmtGetTopLevelShell(w);
      qmon_cluster = qmonCreateClusterConfig(shell);
      XmtAddDeleteCallback(shell, XmDO_NOTHING, 
                              qmonPopdownClusterConfig, NULL);
   } 
   XSync(XtDisplay(qmon_cluster), 0);
   XmUpdateDisplay(qmon_cluster);

   qmonMirrorMultiAnswer(CONFIG_T | EXECHOST_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      /* set default cursor */
      XmtDisplayDefaultCursor(w);
      DEXIT;
      return;
   }
   qmonTimerAddUpdateProc(CONFIG_T, "updateClusterList", updateClusterList);
   qmonStartTimer(CONFIG_T | EXECHOST_T);
   updateClusterList();
   XmListSelectPos(cluster_host_list, 1, True);
   XtManageChild(qmon_cluster);
   XRaiseWindow(XtDisplay(XtParent(qmon_cluster)), 
                  XtWindow(XtParent(qmon_cluster)));

   /* set default cursor */
   XmtDisplayDefaultCursor(w);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
void updateClusterList(void)
{
   lList *cl = NULL;
   XmString *selectedItems = NULL;
   Cardinal selectedItemCount;
   Cardinal itemCount;
   XmString xglobal;
   
   DENTER(GUI_LAYER, "updateClusterList");

   cl = qmonMirrorList(SGE_CONFIG_LIST);
   UpdateXmListFromCull(cluster_host_list, XmFONTLIST_DEFAULT_TAG, cl, CONF_hname);
   XmListMoveItemToPos(cluster_host_list, "global", 1);

   XtVaGetValues( cluster_host_list,
                  XmNselectedItems, &selectedItems,
                  XmNselectedItemCount, &selectedItemCount,
                  XmNitemCount, &itemCount,
                  NULL);

   if (selectedItemCount)
      XmListSelectItem(cluster_host_list, selectedItems[0], True);
   else if (itemCount) {
      xglobal = XmtCreateXmString("global"); 
      XmListSelectItem(cluster_host_list, xglobal, True);
      XmStringFree(xglobal);
   }
    
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonSelectHost(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   XmListCallbackStruct *cbs = (XmListCallbackStruct*) cad;
   char *hname;
   lListElem *ep;
   
   DENTER(GUI_LAYER, "qmonSelectHost");

   if (! XmStringGetLtoR(cbs->item, XmFONTLIST_DEFAULT_TAG, &hname)) {
      fprintf(stderr, "XmStringGetLtoR failed\n");
      DEXIT;
      return;
   }

   ep = lGetElemHost(qmonMirrorList(SGE_CONFIG_LIST), CONF_hname, hname);
   XtFree((char*) hname);
   qmonClusterFillConf(cluster_conf_list, ep);
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonClusterFillConf(
Widget w,
lListElem *ep 
) {
   lList *confl;
   lListElem *cep;
   XmString *items;
   int itemCount; 
   char buf[BUFSIZ*4];
   int i;

   DENTER(GUI_LAYER, "qmonClusterFillConf");
   
   if (!ep) {
      DEXIT;
      return;
   }
   confl = lGetList(ep, CONF_entries);

#if 0   
   UpdateXmListFromCull(cluster_conf_list, XmFONTLIST_DEFAULT_TAG, confl, CF_name);
#else
   itemCount = lGetNumberOfElem(confl);
   
   if (itemCount < 0) {
      DEXIT;
      return;
   }
   
   if (itemCount == 0) {
      XmListDeleteAllItems(cluster_conf_list);
      DEXIT;
      return;
   }

   if (itemCount > 0) {
      items = (XmString*) malloc(sizeof(XmString)*itemCount); 

      for(cep = lFirst(confl), i=0; cep; cep = lNext(cep), i++) {
         sprintf(buf, "%-20.20s %s", lGetString(cep, CF_name),
                        lGetString(cep, CF_value));
         items[i] = XmStringCreateLtoR(buf, "LIST");
      }
      XtVaSetValues( cluster_conf_list, 
                     XmNitems, items,
                     XmNitemCount, itemCount,
                     NULL);
      XmStringTableFree(items, itemCount);
   }
#endif

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonPopdownClusterConfig(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   DENTER(TOP_LAYER, "qmonPopdownClusterConfig");

   qmonStopTimer(CONFIG_T | EXECHOST_T);
   qmonTimerRmUpdateProc(CONFIG_T, "updateClusterList");
   XtUnmanageChild(qmon_cluster);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static Widget qmonCreateClusterConfig(
Widget parent 
) {
   Widget cluster_layout, cluster_change, cluster_change_global,
            cluster_delete, cluster_done, shell, cluster_main_link;

   DENTER(TOP_LAYER, "qmonCreateClusterConfig");
   
   cluster_layout = XmtBuildQueryDialog( parent, "qmon_cluster",
                           NULL, 0,
                           "cluster_host_list", &cluster_host_list,
                           "cluster_conf_list", &cluster_conf_list,
                           "cluster_delete", &cluster_delete,
                           "cluster_change", &cluster_change,
                           "cluster_change_global", &cluster_change_global,
                           "cluster_done", &cluster_done,
                           "cluster_main_link", &cluster_main_link,
                           NULL);

   shell = XmtGetShell(cluster_layout);
   /*
   ** create the cluster global dialog
   */
   cluster_global_layout = qmonClusterCreateGlobal(shell);

   XtAddCallback(cluster_main_link, XmNactivateCallback, 
                     qmonMainControlRaise, NULL);
   XtAddCallback(cluster_host_list, XmNbrowseSelectionCallback, 
                     qmonSelectHost, NULL);
   XtAddCallback(cluster_done, XmNactivateCallback, 
                     qmonPopdownClusterConfig, NULL);
   XtAddCallback(cluster_change, XmNactivateCallback, 
                     qmonClusterChange, (XtPointer) True); 
   XtAddCallback(cluster_change_global, XmNactivateCallback, 
                     qmonClusterChange, (XtPointer) False); 
   XtAddCallback(cluster_delete, XmNactivateCallback, 
                     qmonClusterDelete, NULL); 
   XtAddCallback(cluster_host, XmtNverifyCallback, 
                     qmonClusterHost, NULL);

   XtAddEventHandler(XtParent(cluster_layout), StructureNotifyMask, False, 
                        SetMinShellSize, NULL);
   XtAddEventHandler(XtParent(cluster_layout), StructureNotifyMask, False, 
                        SetMaxShellSize, NULL);

   DEXIT;
   return cluster_layout;
}


/*-------------------------------------------------------------------------*/
static Widget qmonClusterCreateGlobal(
Widget parent 
) {
   Widget   cluster_ok, cluster_cancel, layout, cluster_col, cluster_row;

   DENTER(TOP_LAYER, "qmonClusterCreateGlobal");
   
   layout = XmtBuildQueryDialog( parent, "global_dialog",
                           ccl_resources, XtNumber(ccl_resources),
                           "cluster_host", &cluster_host,
                           "cluster_ok", &cluster_ok,
                           "cluster_cancel", &cluster_cancel,
                           "cluster_enforce_project", &cluster_enforce_project,
                           "cluster_enforce_user", &cluster_enforce_user,
                           "cluster_usersPB", &cluster_usersPB,
                           "cluster_xusersPB", &cluster_xusersPB,
                           "cluster_users", &cluster_users,
                           "cluster_xusers", &cluster_xusers,
                           "cluster_projectsPB", &cluster_projectsPB,
                           "cluster_xprojectsPB", &cluster_xprojectsPB,
                           "cluster_projects", &cluster_projects,
                           "cluster_xprojects", &cluster_xprojects,
                           "cluster_qmaster_spool_dir", 
                                    &cluster_qmaster_spool_dir,
                           "cluster_qmaster_spool_dir_label", 
                                    &cluster_qmaster_spool_dir_label,
                           "cluster_execd_spool_dir", 
                                    &cluster_execd_spool_dir,
                           "cluster_execd_spool_dir_label", 
                                    &cluster_execd_spool_dir_label,
                           "cluster_admin_mail", &cluster_admin_mail,
                           "cluster_admin_mail_label", &cluster_admin_mail_label,
                           "cluster_login_shells", &cluster_login_shells,
                           "cluster_login_shells_label", &cluster_login_shells_label,
                           "cluster_default_domain", &cluster_default_domain,
                           "cluster_default_domain_label", &cluster_default_domain_label,
                           "cluster_min_uid", &cluster_min_uid,
                           "cluster_min_gid", &cluster_min_gid,
                           "cluster_max_aj_instances", &cluster_max_aj_instances,
                           "cluster_max_aj_tasks", &cluster_max_aj_tasks,
                           "cluster_max_u_jobs", &cluster_max_u_jobs,
                           "cluster_zombie_jobs", &cluster_zombie_jobs,
                           "cluster_load_report_time", &cluster_load_report_time,
                           "cluster_load_report_timePB", 
                                    &cluster_load_report_timePB,
                           "cluster_stat_log_time", &cluster_stat_log_time,
                           "cluster_stat_log_timePB", &cluster_stat_log_timePB,
                           "cluster_max_unheard", &cluster_max_unheard,
                           "cluster_max_unheardPB", &cluster_max_unheardPB,
                           "cluster_reschedule_unknown", &cluster_reschedule_unknown,
                           "cluster_reschedule_unknownPB", &cluster_reschedule_unknownPB,
                           "cluster_shell_start_mode", 
                                    &cluster_shell_start_mode,
                           "cluster_loglevel", &cluster_loglevel,
                           "cluster_ignore_fqdn", &cluster_ignore_fqdn,
                           "cluster_qmaster_params", &cluster_qmaster_params,
                           "cluster_qmaster_params_label", &cluster_qmaster_params_label,
                           "cluster_schedd_params", &cluster_schedd_params,
                           "cluster_schedd_params_label", &cluster_schedd_params_label,
                           "cluster_execd_params", &cluster_execd_params,
                           "cluster_shepherd_cmd", &cluster_shepherd_cmd,
                           "cluster_rsh_daemon", &cluster_rsh_daemon,
                           "cluster_rsh_command", &cluster_rsh_command,
                           "cluster_rlogin_daemon", &cluster_rlogin_daemon,
                           "cluster_rlogin_command", &cluster_rlogin_command,
                           "cluster_set_token_cmd", &cluster_set_token_cmd,
                           "cluster_set_token_cmd_label", &cluster_set_token_cmd_label,
                           "cluster_pag_cmd_label", &cluster_pag_cmd_label,
                           "cluster_pag_cmd", &cluster_pag_cmd,
                           "cluster_token_extend_time_label", 
                                    &cluster_token_extend_time_label,
                           "cluster_token_extend_time", 
                                    &cluster_token_extend_time,
                           "cluster_gid_range", 
                                    &cluster_gid_range,
                           "cluster_admin_user", 
                                    &cluster_admin_user,
                           "cluster_admin_user_label", 
                                    &cluster_admin_user_label,
                           NULL);

   if (!feature_is_enabled(FEATURE_SGEEE)) {
      XtUnmanageChild(cluster_enforce_project);
      XtUnmanageChild(cluster_enforce_user);
      XtUnmanageChild(cluster_gid_range);
      XtVaGetValues( cluster_projectsPB,
                     XmtNlayoutIn, &cluster_projects_col,
                     NULL);
      XtUnmanageChild(cluster_projects_col);
      XtVaGetValues( cluster_xprojectsPB,
                     XmtNlayoutIn, &cluster_xprojects_col,
                     NULL);
      XtUnmanageChild(cluster_xprojects_col);
   }

   if (!feature_is_enabled(FEATURE_AFS_SECURITY)) {
      XtVaGetValues( cluster_set_token_cmd,
                     XmtNlayoutIn, &cluster_row,
                     NULL);
      XtVaGetValues( cluster_row,
                     XmtNlayoutIn, &cluster_col,
                     NULL);
      XtUnmanageChild(cluster_col);
   }

   XtAddCallback(cluster_ok, XmNactivateCallback, 
                     qmonClusterOk, (XtPointer)layout);
   XtAddCallback(cluster_cancel, XmNactivateCallback, 
                     qmonClusterCancel, (XtPointer)layout);

   XtAddCallback(cluster_usersPB, XmNactivateCallback, 
                     qmonClusterAskForUsers, (XtPointer)cluster_users);
   XtAddCallback(cluster_xusersPB, XmNactivateCallback, 
                     qmonClusterAskForUsers, (XtPointer)cluster_xusers);

   if (feature_is_enabled(FEATURE_SGEEE)) {
      XtAddCallback(cluster_projectsPB, XmNactivateCallback, 
                    qmonClusterAskForProjects, (XtPointer)cluster_projects);
      XtAddCallback(cluster_xprojectsPB, XmNactivateCallback, 
                    qmonClusterAskForProjects, (XtPointer)cluster_xprojects);
   }

   XtAddCallback(cluster_load_report_timePB, XmNactivateCallback, 
                     qmonClusterTime, (XtPointer)cluster_load_report_time);
   XtAddCallback(cluster_stat_log_timePB, XmNactivateCallback, 
                     qmonClusterTime, (XtPointer)cluster_stat_log_time);
   XtAddCallback(cluster_max_unheardPB, XmNactivateCallback, 
                     qmonClusterTime, (XtPointer)cluster_max_unheard);
   XtAddCallback(cluster_reschedule_unknownPB, XmNactivateCallback, 
                     qmonClusterTime, (XtPointer)cluster_reschedule_unknown);


   /*
   ** check all the inputfields for correct input and remove leading and 
   ** trailing whitespace
   ** register callback procedures, so it can be set in the resource descr file
   */
   XmtVaRegisterCallbackProcedures(
         "TrimAndTrail", qmonClusterCheckInput, XtRWidget,
         NULL);

/* 
   XtAddCallback(cluster_qmaster_params, XmtNverifyCallback, 
                     qmonClusterCheckInput, NULL);
*/


   XtAddEventHandler(XtParent(layout), StructureNotifyMask, False, 
                        SetMinShellSize, NULL);
   XtAddEventHandler(XtParent(layout), StructureNotifyMask, False, 
                        SetMaxShellSize, NULL);
   DEXIT;
   return layout;
}

/*-------------------------------------------------------------------------*/
static void qmonClusterChange(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Boolean add  = (Boolean)(long)cld;
   static tCClEntry cluster_entry;
   lListElem *ep;
   XmString *selectedItems;
   Cardinal selectedItemCount;
   String host = NULL;
   
   DENTER(GUI_LAYER, "qmonClusterChange");

   /*
   ** reset the dialog values
   */
   qmonInitCClEntry(&cluster_entry);

   if (add) {
         qmonClusterLayoutSetSensitive(False);
         XtVaSetValues( cluster_host,
                        XmNeditable, True,
                        NULL);
         XmtInputFieldSetString(cluster_host, "");
   }
   else {
      XtVaGetValues( cluster_host_list,
                     XmNselectedItems, &selectedItems,
                     XmNselectedItemCount, &selectedItemCount,
                     NULL);
      if (selectedItemCount > 0) {
         XmStringGetLtoR(selectedItems[0], XmFONTLIST_DEFAULT_TAG, &host); 
         if ( host && !strcasecmp(host, "global"))
            qmonClusterLayoutSetSensitive(True);
         else
            qmonClusterLayoutSetSensitive(False);
      }
      if (selectedItemCount && host) {
         ep = lGetElemHost(qmonMirrorList(SGE_CONFIG_LIST), CONF_hname, host);
         qmonCullToCClEntry(ep, &cluster_entry);
         XmtInputFieldSetString(cluster_host, host);
         XtVaSetValues( cluster_host,
                        XmNeditable, False,
                        NULL);
         XtFree((char*)host);
      }
   }
      
   XmtDialogSetDialogValues(cluster_global_layout, &cluster_entry);

   /*
   ** free the allocated memory
   */
   qmonClearCClEntry(&cluster_entry);
      
   XtManageChild(cluster_global_layout);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonClusterOk(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget layout = (Widget) cld;
   tCClEntry cluster_entry;
   lList *conf_entries = NULL;
   lList *confl = NULL;
   String host = NULL;
   XmString xhost;
   lList *alp;
   lListElem *aep;
   lEnumeration *what;
   Boolean status = False;
   int local = 0;

   DENTER(GUI_LAYER, "qmonClusterOk");

   /*
   ** get the contents of the dialog fields here,
   ** build the cull list and send gdi request
   ** depending on success of gdi request close the dialog or stay open
   */

   qmonInitCClEntry(&cluster_entry);
   host = XmtInputFieldGetString(cluster_host); 

   if (!host || host[0] == '\0') {
      XtUnmanageChild(layout);
      DEXIT;
      return;
   }

   XmtDialogGetDialogValues(layout, &cluster_entry);

   if (host && !strcasecmp(host, "global"))
      local = 0;
   else
      local = 1;

   if (qmonCClEntryToCull(layout, &cluster_entry, &conf_entries, local)) {
      confl = lCreateElemList("Conf_list", CONF_Type, 1);
      lSetHost(lFirst(confl), CONF_hname, host ? host : "global");
      lSetList(lFirst(confl), CONF_entries, conf_entries);

      xhost = XmtCreateXmString(host ? host : "global");
      
      if (rmon_mlgetl(&DEBUG_ON, GUI_LAYER) & INFOPRINT) {
         printf("___CLUSTER_CONF________________________\n");
         lWriteListTo(confl, stdout);
         printf("_______________________________________\n");
      }
      /*
      ** gdi call 
      */
      what = lWhat("%T(ALL)", CONF_Type);

      alp = qmonModList(SGE_CONFIG_LIST, qmonMirrorListRef(SGE_CONFIG_LIST), CONF_hname, &confl, NULL, what);

      aep = lFirst(alp);
      if (lFirst(alp) && lGetUlong(aep, AN_status) == STATUS_OK)
         status = True;

      qmonMessageBox(w, alp, 0);

      updateClusterList();
      XmListSelectItem(cluster_host_list, xhost, True);
      XmStringFree(xhost);
   
    
      lFreeWhat(what);
      confl = lFreeList(confl);
      alp = lFreeList(alp);
   }
   
   if (status)
      XtUnmanageChild(layout);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonClusterCancel(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   Widget layout = (Widget) cld;

   DENTER(GUI_LAYER, "qmonClusterCancel");

   XtUnmanageChild(layout);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonClusterLayoutSetSensitive(Boolean mode)
{
   DENTER(GUI_LAYER, "qmonClusterLayoutSetSensitive");

   XtSetSensitive(cluster_qmaster_spool_dir, False);
   XtSetSensitive(cluster_qmaster_spool_dir_label, False);

   XtSetSensitive(cluster_execd_spool_dir, False);
   XtSetSensitive(cluster_execd_spool_dir_label, False);

   XtSetSensitive(cluster_ignore_fqdn, False);
   XtSetSensitive(cluster_default_domain, False);
   XtSetSensitive(cluster_default_domain_label, False);
   XtSetSensitive(cluster_admin_user, False);
   XtSetSensitive(cluster_admin_user_label, False);
   
   XtSetSensitive(cluster_admin_mail, mode);
   XtSetSensitive(cluster_admin_mail_label, mode);
   XtSetSensitive(cluster_login_shells, mode);
   XtSetSensitive(cluster_login_shells_label, mode);
   XtSetSensitive(cluster_min_uid, mode);
   XtSetSensitive(cluster_min_gid, mode);
   XtSetSensitive(cluster_max_aj_instances, mode);
   XtSetSensitive(cluster_max_aj_tasks, mode);
   XtSetSensitive(cluster_max_u_jobs, mode);
   XtSetSensitive(cluster_zombie_jobs, mode);
   XtSetSensitive(cluster_stat_log_time, mode);
   XtSetSensitive(cluster_stat_log_timePB, mode);
   XtSetSensitive(cluster_max_unheard, mode);
   XtSetSensitive(cluster_max_unheardPB, mode);
   XtSetSensitive(cluster_shell_start_mode, mode);
   XtSetSensitive(cluster_loglevel, mode);

   XtSetSensitive(cluster_users, mode);
   XtSetSensitive(cluster_usersPB, mode);

   XtSetSensitive(cluster_xusers, mode);
   XtSetSensitive(cluster_xusersPB, mode);


   XtSetSensitive(cluster_qmaster_params, mode);
   XtSetSensitive(cluster_qmaster_params_label, mode);
   XtSetSensitive(cluster_schedd_params, mode);
   XtSetSensitive(cluster_schedd_params_label, mode);
  
   if (feature_is_enabled(FEATURE_SGEEE)) {
      XtSetSensitive(cluster_enforce_project, mode);
      XtSetSensitive(cluster_enforce_user, mode);
      XtSetSensitive(cluster_projects, mode);
      XtSetSensitive(cluster_projectsPB, mode);
      XtSetSensitive(cluster_xprojects, mode);
      XtSetSensitive(cluster_xprojectsPB, mode);
   }

   if (feature_is_enabled(FEATURE_AFS_SECURITY)) {
      XtSetSensitive(cluster_set_token_cmd, mode);
      XtSetSensitive(cluster_set_token_cmd_label, mode);
      XtSetSensitive(cluster_pag_cmd, mode);
      XtSetSensitive(cluster_pag_cmd_label, mode);
      XtSetSensitive(cluster_token_extend_time, mode);
      XtSetSensitive(cluster_token_extend_time_label, mode);
   }

   DEXIT;
}



/*-------------------------------------------------------------------------*/
static void qmonClusterDelete(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *lp = NULL;
   lList *alp = NULL;
   lEnumeration *what = NULL;
   Cardinal itemCount = 0;
   Boolean answer;
   

   DENTER(GUI_LAYER, "qmonClusterDelete");
    
   lp = XmStringToCull(cluster_host_list, CONF_Type, CONF_hname,
                           SELECTED_ITEMS); 

   if (lp) {
      XmtAskForBoolean(w, "xmtBooleanDialog",
                     "@{cluster.askdel.Do you really want to delete the\nselected cluster configuration ?}",
                     "@{Delete}", "@{Cancel}", NULL, XmtNoButton, XmDIALOG_WARNING,
                     False, &answer, NULL);
      if (answer) {
         what = lWhat("%T(ALL)", CONF_Type);

         alp = qmonDelList(SGE_CONFIG_LIST, qmonMirrorListRef(SGE_CONFIG_LIST), CONF_hname, 
                           &lp, NULL, what);

         qmonMessageBox(w, alp, 0);

         lFreeWhat(what);
         alp = lFreeList(alp);

         updateClusterList();
         XtVaGetValues( cluster_host_list,
                        XmNitemCount, &itemCount,
                        NULL);
         if (itemCount)
            XmListSelectPos(cluster_host_list, 1, True);
         else
            XtVaSetValues( cluster_conf_list, 
                           XmNitems, NULL,
                           XmNitemCount, 0,
                           NULL);
            
         
      }
      lp = lFreeList(lp);
   }
   DEXIT;

}

/*-------------------------------------------------------------------------*/
static Boolean qmonCClEntryToCull(
Widget w,
tCClEntry *clen,
lList **lpp,
int local 
) {
   char errstr[256];
   lList *confl, *lp = NULL, *alp = NULL;
   lListElem *gep, *ep, *new, *uep;
   static String shell_start_mode[] = {
      "posix_compliant", "script_from_stdin", "unix_behavior" 
   };
   static String loglevel[] = { "log_info", "log_warning", "log_err" };
/*    static String logmail[] = { "true", "false" }; */
   static String enforce_project[] = { "true", "false" };
   static String enforce_user[] = { "true", "false" };
   static String ignore_fqdn[] = { "true", "false" };
   String str = NULL;
   char min_uid[20];
   char min_gid[20];
   char max_aj_instances[255];
   char max_aj_tasks[255];
   char max_u_jobs[255];
   char zombie_jobs[20];
   static char buf[4*BUFSIZ];
   Boolean first;
   
   DENTER(GUI_LAYER, "qmonCClEntryToCull");

   /*
   ** get the global conf list
   */
   gep = lGetElemHost(qmonMirrorList(SGE_CONFIG_LIST), CONF_hname, "global");
   if (!gep) {
      *lpp = NULL;
      DEXIT;
      return False;
   }
   /*
   ** get the global configuration list elements
   */
   confl = lGetList(gep, CONF_entries);
      
   
   if (local) {
      /*
      ** create the returned list of configuration entries
      */

      if (!(lp = lCreateList("CF_list", CF_Type))) {
         *lpp = NULL;
         DEXIT;
         return False;
      }

      ep = lGetElemStr(confl, CF_name, "execd_spool_dir");
      if (clen->execd_spool_dir && clen->execd_spool_dir[0] != '\0' 
            /* && strcmp(lGetString(ep, CF_value), clen->execd_spool_dir)*/) {
         if (check_white(clen->execd_spool_dir)) {
            strcpy(errstr, "No whitespace allowed in value for execd_spool_dir");
            goto error;
         }
         new = lCopyElem(ep);
         lSetString(new, CF_value, clen->execd_spool_dir);
         lAppendElem(lp, new);
      }

      ep = lGetElemStr(confl, CF_name, "binary_path");
      if (clen->binary_path && clen->binary_path[0] != '\0' 
       /* && strcmp(lGetString(ep, CF_value), clen->binary_path)*/) {
         if (check_white(clen->binary_path)) {
            strcpy(errstr, "No whitespace allowed in value for binary_path");
            goto error;
         }
         new = lCopyElem(ep);
         lSetString(new, CF_value, clen->binary_path);
         lAppendElem(lp, new);
      }

      ep = lGetElemStr(confl, CF_name, "mailer");
      if (clen->mailer && clen->mailer[0] != '\0'
          /* && strcmp(lGetString(ep, CF_value), clen->mailer) */) {
         if (check_white(clen->mailer)) {
            strcpy(errstr, "No whitespace allowed in value for mailer");
            goto error;
         }
         new = lCopyElem(ep);
         lSetString(new, CF_value, clen->mailer);
         lAppendElem(lp, new);
      }

      ep = lGetElemStr(confl, CF_name, "xterm");
      if (clen->xterm && clen->xterm[0] != '\0'
          /* && strcmp(lGetString(ep, CF_value), clen->xterm)*/) {
         if (check_white(clen->xterm)) {
            strcpy(errstr, "No whitespace allowed in value for xterm");
            goto error;
         }
         new = lCopyElem(ep);
         lSetString(new, CF_value, clen->xterm);
         lAppendElem(lp, new);
      }

      ep = lGetElemStr(confl, CF_name, "load_sensor");
      if (clen->load_sensor && clen->load_sensor[0] != '\0'
          /* && strcmp(lGetString(ep, CF_value), clen->load_sensor)*/) {
         if (check_white(clen->load_sensor)) {
            strcpy(errstr, "No whitespace allowed in value for load_sensor");
            goto error;
         }
         new = lCopyElem(ep);
         lSetString(new, CF_value, clen->load_sensor);
         lAppendElem(lp, new);
      }

      ep = lGetElemStr(confl, CF_name, "prolog");
      if (clen->prolog && clen->prolog[0] != '\0'
            /* && strcmp(lGetString(ep, CF_value), clen->prolog) */) {
         new = lCopyElem(ep);
         lSetString(new, CF_value, clen->prolog);
         lAppendElem(lp, new);
      }

      ep = lGetElemStr(confl, CF_name, "epilog");
      if (clen->epilog && clen->epilog[0] != '\0'
           /* && strcmp(lGetString(ep, CF_value), clen->epilog)*/) {
         new = lCopyElem(ep);
         lSetString(new, CF_value, clen->epilog);
         lAppendElem(lp, new);
      }

      ep = lGetElemStr(confl, CF_name, "qlogin_daemon");
      if (clen->qlogin_daemon && clen->qlogin_daemon[0] != '\0'
           /* && strcmp(lGetString(ep, CF_value), clen->qlogin_daemon)*/) {
         if (check_white(clen->qlogin_daemon)) {
            strcpy(errstr, "No whitespace allowed in value for qlogin_daemon");
            goto error;
         }
         new = lCopyElem(ep);
         lSetString(new, CF_value, clen->qlogin_daemon);
         lAppendElem(lp, new);
      }

      ep = lGetElemStr(confl, CF_name, "qlogin_command");
      if (clen->qlogin_command && clen->qlogin_command[0] != '\0'
           /* && strcmp(lGetString(ep, CF_value), clen->qlogin_command)*/) {
         if (check_white(clen->qlogin_command)) {
            strcpy(errstr, "No whitespace allowed in value for qlogin_command");
            goto error;
         }
         new = lCopyElem(ep);
         lSetString(new, CF_value, clen->qlogin_command);
         lAppendElem(lp, new);
      }

      ep = lGetElemStr(confl, CF_name, "load_report_time");
      if (clen->load_report_time && clen->load_report_time[0] != '\0'
           /* && strcmp(lGetString(ep, CF_value), clen->load_report_time)*/) {
         if (check_white(clen->load_report_time)) {
            strcpy(errstr, "No whitespace allowed in value for load_report_time");
            goto error;
         }
         new = lCopyElem(ep);
         lSetString(new, CF_value, clen->load_report_time);
         lAppendElem(lp, new);
      }

      if (clen->reschedule_unknown && clen->reschedule_unknown[0] != '\0'
           /* && strcmp(lGetString(ep, CF_value), clen->reschedule_unknown)*/) {
         if (check_white(clen->reschedule_unknown)) {
            strcpy(errstr, "No whitespace allowed in value for reschedule_unknown");
            goto error;
         }
         ep = lGetElemStr(confl, CF_name, "reschedule_unknown");
         if (!ep) {
            new = lCreateElem(CF_Type);
            lSetString(new, CF_name, "reschedule_unknown");
         }
         else
            new = lCopyElem(ep);
         lSetString(new, CF_value, clen->reschedule_unknown);
         lAppendElem(lp, new);
      }

      if (clen->execd_params && clen->execd_params[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "execd_params");
         if (!ep) {
            new = lCreateElem(CF_Type);
            lSetString(new, CF_name, "execd_params");
         }
         else
            new = lCopyElem(ep);
         lSetString(new, CF_value, clen->execd_params);
         lAppendElem(lp, new);
      }

      if (clen->shepherd_cmd && clen->shepherd_cmd[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "shepherd_cmd");
         if (!ep) {
            new = lCreateElem(CF_Type);
            lSetString(new, CF_name, "shepherd_cmd");
         }
         else
            new = lCopyElem(ep);
         lSetString(new, CF_value, clen->shepherd_cmd);
         lAppendElem(lp, new);
      }

      if (clen->default_domain && clen->default_domain[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "default_domain");
         if (check_white(clen->default_domain)) {
            strcpy(errstr, "No whitespace allowed in value for default_domain");
            goto error;
         }
         if (!ep) {
            new = lCreateElem(CF_Type);
            lSetString(new, CF_name, "default_domain");
         }
         else
            new = lCopyElem(ep);
         lSetString(new, CF_value, clen->default_domain);
         lAppendElem(lp, new);
      }

      if (clen->rsh_daemon && clen->rsh_daemon[0] != '\0'
           /* && strcmp(lGetString(ep, CF_value), clen->rsh_daemon)*/) {
         ep = lGetElemStr(confl, CF_name, "rsh_daemon");
         if (!ep) {
            new = lCreateElem(CF_Type);
            lSetString(new, CF_name, "rsh_daemon");
         }
         else
            new = lCopyElem(ep);
         lSetString(new, CF_value, clen->rsh_daemon);
         lAppendElem(lp, new);
      }

      if (clen->rsh_command && clen->rsh_command[0] != '\0'
           /* && strcmp(lGetString(ep, CF_value), clen->rsh_command)*/) {
         ep = lGetElemStr(confl, CF_name, "rsh_command");
         if (!ep) {
            new = lCreateElem(CF_Type);
            lSetString(new, CF_name, "rsh_command");
         }
         else
            new = lCopyElem(ep);
         lSetString(new, CF_value, clen->rsh_command);
         lAppendElem(lp, new);
      }

      if (clen->rlogin_daemon && clen->rlogin_daemon[0] != '\0'
           /* && strcmp(lGetString(ep, CF_value), clen->rlogin_daemon)*/) {
         ep = lGetElemStr(confl, CF_name, "rlogin_daemon");
         if (!ep) {
            new = lCreateElem(CF_Type);
            lSetString(new, CF_name, "rlogin_daemon");
         }
         else
            new = lCopyElem(ep);
         lSetString(new, CF_value, clen->rlogin_daemon);
         lAppendElem(lp, new);
      }

      if (clen->rlogin_command && clen->rlogin_command[0] != '\0'
           /* && strcmp(lGetString(ep, CF_value), clen->rlogin_command)*/) {
         ep = lGetElemStr(confl, CF_name, "rlogin_command");
         if (!ep) {
            new = lCreateElem(CF_Type);
            lSetString(new, CF_name, "rlogin_command");
         }
         else
            new = lCopyElem(ep);
         lSetString(new, CF_value, clen->rlogin_command);
         lAppendElem(lp, new);
      }

#if 0
      if (clen->starter_method && clen->starter_method[0] != '\0'
           /* && strcmp(lGetString(ep, CF_value), clen->starter_method)*/) {
         ep = lGetElemStr(confl, CF_name, "starter_method");
         if (!ep) {
            new = lCreateElem(CF_Type);
            lSetString(new, CF_name, "starter_method");
         }
         else
            new = lCopyElem(ep);
         lSetString(new, CF_value, clen->starter_method);
         lAppendElem(lp, new);
      }

      if (clen->suspend_method && clen->suspend_method[0] != '\0'
           /* && strcmp(lGetString(ep, CF_value), clen->suspend_method)*/) {
         ep = lGetElemStr(confl, CF_name, "suspend_method");
         if (!ep) {
            new = lCreateElem(CF_Type);
            lSetString(new, CF_name, "suspend_method");
         }
         else
            new = lCopyElem(ep);
         lSetString(new, CF_value, clen->suspend_method);
         lAppendElem(lp, new);
      }

      if (clen->resume_method && clen->resume_method[0] != '\0'
           /* && strcmp(lGetString(ep, CF_value), clen->resume_method)*/) {
         ep = lGetElemStr(confl, CF_name, "resume_method");
         if (!ep) {
            new = lCreateElem(CF_Type);
            lSetString(new, CF_name, "resume_method");
         }
         else
            new = lCopyElem(ep);
         lSetString(new, CF_value, clen->resume_method);
         lAppendElem(lp, new);
      }

      if (clen->terminate_method && clen->terminate_method[0] != '\0'
           /* && strcmp(lGetString(ep, CF_value), clen->terminate_method)*/) {
         ep = lGetElemStr(confl, CF_name, "terminate_method");
         if (!ep) {
            new = lCreateElem(CF_Type);
            lSetString(new, CF_name, "terminate_method");
         }
         else
            new = lCopyElem(ep);
         lSetString(new, CF_value, clen->terminate_method);
         lAppendElem(lp, new);
      }
#endif

      if (feature_is_enabled(FEATURE_SGEEE)) {
         if (clen->gid_range && clen->gid_range[0] != '\0') {
            if (!parse_ranges(clen->gid_range, 0, 0, &alp, NULL, INF_NOT_ALLOWED)){ 
               strcpy(errstr, "Cannot parse GID Range !");
               alp = lFreeList(alp);
               goto error;
            }

            ep = lGetElemStr(confl, CF_name, "gid_range");
            if (!ep) {
               new = lCreateElem(CF_Type);
               lSetString(new, CF_name, "gid_range");
            }
            else
               new = lCopyElem(ep);
            lSetString(new, CF_value, clen->gid_range);
            lAppendElem(lp, new);
         }
      }
   }
   else {
      /*
      ** global settings
      */
      ep = lGetElemStr(confl, CF_name, "qmaster_spool_dir");
      if (check_white(clen->qmaster_spool_dir)) {
         strcpy(errstr, "No whitespace allowed in value for qmaster_spool_dir");
         goto error;
      }
      lSetString(ep, CF_value, clen->qmaster_spool_dir);

      ep = lGetElemStr(confl, CF_name, "execd_spool_dir");
      if (check_white(clen->execd_spool_dir)) {
         strcpy(errstr, "No whitespace allowed in value for execd_spool_dir");
         goto error;
      }
      lSetString(ep, CF_value, clen->execd_spool_dir);

      ep = lGetElemStr(confl, CF_name, "binary_path");
      if (check_white(clen->binary_path)) {
         strcpy(errstr, "No whitespace allowed in value for binary_path");
         goto error;
      }
      lSetString(ep, CF_value, clen->binary_path);

      ep = lGetElemStr(confl, CF_name, "mailer");
      if (check_white(clen->mailer)) {
         strcpy(errstr, "No whitespace allowed in value for mailer");
         goto error;
      }
      lSetString(ep, CF_value, clen->mailer);

      ep = lGetElemStr(confl, CF_name, "xterm");
      if (check_white(clen->xterm)) {
         strcpy(errstr, "No whitespace allowed in value for xterm");
         goto error;
      }
      lSetString(ep, CF_value, clen->xterm);

      ep = lGetElemStr(confl, CF_name, "load_sensor");
      if (clen->load_sensor && clen->load_sensor[0] != '\0') {
         if (check_white(clen->load_sensor)) {
            strcpy(errstr, "No whitespace allowed in value for load_sensor");
            goto error;
         }
         lSetString(ep, CF_value, clen->load_sensor);
      }
      else
         lSetString(ep, CF_value, "none");

      ep = lGetElemStr(confl, CF_name, "administrator_mail");
      if (clen->administrator_mail && clen->administrator_mail[0] != '\0') {
         if (check_white(clen->administrator_mail)) {
            strcpy(errstr, "No whitespace allowed in value for administrator_mail");
            goto error;
         }
         lSetString(ep, CF_value, clen->administrator_mail);
      }
      else
         lSetString(ep, CF_value, "none");
         
      ep = lGetElemStr(confl, CF_name, "min_uid");
      sprintf(min_uid, "%d", clen->min_uid);
      lSetString(ep, CF_value, min_uid);
         
      ep = lGetElemStr(confl, CF_name, "min_gid");
      sprintf(min_gid, "%d", clen->min_gid);
      lSetString(ep, CF_value, min_gid);
         
      ep = lGetElemStr(confl, CF_name, "max_aj_instances");
      sprintf(max_aj_instances, "%d", clen->max_aj_instances);
      lSetString(ep, CF_value, max_aj_instances);
         
      ep = lGetElemStr(confl, CF_name, "max_aj_tasks");
      sprintf(max_aj_tasks, "%d", clen->max_aj_tasks);
      lSetString(ep, CF_value, max_aj_tasks);
         
      ep = lGetElemStr(confl, CF_name, "max_u_jobs");
      sprintf(max_u_jobs, "%d", clen->max_u_jobs);
      lSetString(ep, CF_value, max_u_jobs);
         
      ep = lGetElemStr(confl, CF_name, "finished_jobs");
      sprintf(zombie_jobs, "%d", clen->zombie_jobs);
      lSetString(ep, CF_value, zombie_jobs);
         
      ep = lGetElemStr(confl, CF_name, "prolog");
      if (clen->prolog && clen->prolog[0] != '\0') {
         lSetString(ep, CF_value, clen->prolog);
      }
      else
         lSetString(ep, CF_value, "none");

      ep = lGetElemStr(confl, CF_name, "epilog");
      if (clen->epilog && clen->epilog[0] != '\0') {
         lSetString(ep, CF_value, clen->epilog);
      }
      else
         lSetString(ep, CF_value, "none");
         
      ep = lGetElemStr(confl, CF_name, "qlogin_daemon");
      if (clen->qlogin_daemon && clen->qlogin_daemon[0] != '\0') {
         if (check_white(clen->qlogin_daemon)) {
            strcpy(errstr, "No whitespace allowed in value for qlogin_daemon");
            goto error;
         }
         lSetString(ep, CF_value, clen->qlogin_daemon);
      }
      else
         lSetString(ep, CF_value, "none");
         
      ep = lGetElemStr(confl, CF_name, "qlogin_command");
      if (clen->qlogin_command && clen->qlogin_command[0] != '\0') {
         if (check_white(clen->qlogin_command)) {
            strcpy(errstr, "No whitespace allowed in value for qlogin_command");
            goto error;
         }
         lSetString(ep, CF_value, clen->qlogin_command);
      }
      else
         lSetString(ep, CF_value, "none");
         
         
      ep = lGetElemStr(confl, CF_name, "login_shells");
      if (clen->login_shells && clen->login_shells[0] != '\0') {
         if (check_white(clen->login_shells)) {
            strcpy(errstr, "No whitespace allowed in value for login_shells");
            goto error;
         }
         lSetString(ep, CF_value, clen->login_shells);
      }
      else
         lSetString(ep, CF_value, "none");

      ep = lGetElemStr(confl, CF_name, "load_report_time");
      if (check_white(clen->load_report_time)) {
         strcpy(errstr, "No whitespace allowed in value for load_report_time");
         goto error;
      }
      lSetString(ep, CF_value, clen->load_report_time);

      ep = lGetElemStr(confl, CF_name, "stat_log_time");
      if (check_white(clen->stat_log_time)) {
         strcpy(errstr, "No whitespace allowed in value for stat_log_time");
         goto error;
      }
      lSetString(ep, CF_value, clen->stat_log_time);

      ep = lGetElemStr(confl, CF_name, "max_unheard");
      if (check_white(clen->max_unheard)) {
         strcpy(errstr, "No whitespace allowed in value for max_unheard");
         goto error;
      }
      lSetString(ep, CF_value, clen->max_unheard);

      if (clen->reschedule_unknown && clen->reschedule_unknown[0] != '\0') {
         if (check_white(clen->reschedule_unknown)) {
            strcpy(errstr, "No whitespace allowed in value for reschedule_unknown");
            goto error;
         }
         ep = lGetElemStr(confl, CF_name, "reschedule_unknown");
         if (!ep)
            ep = lAddElemStr(&confl, CF_name, "reschedule_unknown", CF_Type);
         lSetString(ep, CF_value, clen->reschedule_unknown);
      }
      else {
         lDelElemStr(&confl, CF_name, "reschedule_unknown");
      }

      if (clen->shell_start_mode >= 0 && 
               clen->shell_start_mode < XtNumber(shell_start_mode))
         str = shell_start_mode[clen->shell_start_mode];
      ep = lGetElemStr(confl, CF_name, "shell_start_mode");
      lSetString(ep, CF_value, str);

      if (clen->loglevel >= 0 && 
            clen->loglevel < XtNumber(loglevel))
         str = loglevel[clen->loglevel];
      ep = lGetElemStr(confl, CF_name, "loglevel");
      lSetString(ep, CF_value, str);

      if (clen->ignore_fqdn >= 0 && 
            clen->ignore_fqdn < XtNumber(ignore_fqdn))
         str = ignore_fqdn[clen->ignore_fqdn];
      ep = lGetElemStr(confl, CF_name, "ignore_fqdn");
      lSetString(ep, CF_value, str);

/*       if (clen->logmail >= 0 &&  */
/*             clen->logmail < XtNumber(logmail)) */
/*          str = logmail[clen->logmail]; */
/*       ep = lGetElemStr(confl, CF_name, "logmail"); */
/*       lSetString(ep, CF_value, str); */

      /* 
      ** (x)user_lists
      */
      strcpy(buf, "none");
      first = True;
      for_each(uep, clen->cluster_users) {
         if (first) {
            first = False;
            strcpy(buf, lGetString(uep, US_name));
         }
         else {
            strcat(buf, " "); 
            strcat(buf, lGetString(uep, US_name));
         }
      }
      ep = lGetElemStr(confl, CF_name, "user_lists");
      lSetString(ep, CF_value, buf);

      strcpy(buf, "none");
      first = True;
      for_each(uep, clen->cluster_xusers) {
         if (first) {
            first = False;
            strcpy(buf, lGetString(uep, US_name));
         }
         else {
            strcat(buf, " "); 
            strcat(buf, lGetString(uep, US_name));
         }
      }
      ep = lGetElemStr(confl, CF_name, "xuser_lists");
      lSetString(ep, CF_value, buf);

        

      if (feature_is_enabled(FEATURE_SGEEE)) {
         if (clen->enforce_project >= 0 && 
               clen->enforce_project < sizeof(enforce_project))
            str = enforce_project[clen->enforce_project];
         ep = lGetElemStr(confl, CF_name, "enforce_project");
         lSetString(ep, CF_value, str);

         if (clen->enforce_user >= 0 && 
               clen->enforce_user < sizeof(enforce_user))
            str = enforce_user[clen->enforce_user];
         ep = lGetElemStr(confl, CF_name, "enforce_user");
         lSetString(ep, CF_value, str);

         if (clen->gid_range && clen->gid_range[0] != '\0') {
            if (!parse_ranges(clen->gid_range, 0, 0, &alp, NULL, INF_NOT_ALLOWED)){ 
               strcpy(errstr, "Cannot parse GID Range !");
               alp = lFreeList(alp);
               goto error;
            }
   
            ep = lGetElemStr(confl, CF_name, "gid_range");
            if (!ep)
               ep = lAddElemStr(&confl, CF_name, "gid_range", CF_Type);
            lSetString(ep, CF_value, clen->gid_range);
         }
         else {
            lDelElemStr(&confl, CF_name, "gid_range");
         }
         /*
         ** (x)projects
         */
         strcpy(buf, "none");
         first = True;
         for_each(uep, clen->cluster_projects) {
            if (first) {
               first = False;
               strcpy(buf, lGetString(uep, UP_name));
            }
            else {
               strcat(buf, " "); 
               strcat(buf, lGetString(uep, UP_name));
            }
         }
         ep = lGetElemStr(confl, CF_name, "projects");
         lSetString(ep, CF_value, buf);

         strcpy(buf, "none");
         first = True;
         for_each(uep, clen->cluster_xprojects) {
            if (first) {
               first = False;
               strcpy(buf, lGetString(uep, UP_name));
            }
            else {
               strcat(buf, " "); 
               strcat(buf, lGetString(uep, UP_name));
            }
         }
         ep = lGetElemStr(confl, CF_name, "xprojects");
         lSetString(ep, CF_value, buf);
      }

      /*
      **  additional config parameters
      */
      if (clen->qmaster_params && clen->qmaster_params[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "qmaster_params");
         if (!ep)
            ep = lAddElemStr(&confl, CF_name, "qmaster_params", CF_Type);
         lSetString(ep, CF_value, clen->qmaster_params);
      }
      else {
         lDelElemStr(&confl, CF_name, "qmaster_params");
      }

      if (clen->schedd_params && clen->schedd_params[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "schedd_params");
         if (!ep)
            ep = lAddElemStr(&confl, CF_name, "schedd_params", CF_Type);
         lSetString(ep, CF_value, clen->schedd_params);
      }
      else {
         lDelElemStr(&confl, CF_name, "schedd_params");
      }

      if (clen->execd_params && clen->execd_params[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "execd_params");
         if (!ep)
            ep = lAddElemStr(&confl, CF_name, "execd_params", CF_Type);
         lSetString(ep, CF_value, clen->execd_params);
      }
      else {
         lDelElemStr(&confl, CF_name, "execd_params");
      }

      if (clen->shepherd_cmd && clen->shepherd_cmd[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "shepherd_cmd");
         if (!ep)
            ep = lAddElemStr(&confl, CF_name, "shepherd_cmd", CF_Type);
         lSetString(ep, CF_value, clen->shepherd_cmd);
      }
      else {
         lDelElemStr(&confl, CF_name, "shepherd_cmd");
      }

      if (clen->default_domain && clen->default_domain[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "default_domain");
         if (!ep)
            ep = lAddElemStr(&confl, CF_name, "default_domain", CF_Type);
         lSetString(ep, CF_value, clen->default_domain);
      }
      else {
         lDelElemStr(&confl, CF_name, "default_domain");
      }

      /*
      **  interactive config parameters
      */
      if (clen->rsh_daemon && clen->rsh_daemon[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "rsh_daemon");
         if (!ep)
            ep = lAddElemStr(&confl, CF_name, "rsh_daemon", CF_Type);
         lSetString(ep, CF_value, clen->rsh_daemon);
      }
      else {
         lDelElemStr(&confl, CF_name, "rsh_daemon");
      }

      if (clen->rsh_command && clen->rsh_command[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "rsh_command");
         if (!ep)
            ep = lAddElemStr(&confl, CF_name, "rsh_command", CF_Type);
         lSetString(ep, CF_value, clen->rsh_command);
      }
      else {
         lDelElemStr(&confl, CF_name, "rsh_command");
      }

      if (clen->rlogin_daemon && clen->rlogin_daemon[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "rlogin_daemon");
         if (!ep)
            ep = lAddElemStr(&confl, CF_name, "rlogin_daemon", CF_Type);
         lSetString(ep, CF_value, clen->rlogin_daemon);
      }
      else {
         lDelElemStr(&confl, CF_name, "rlogin_daemon");
      }

      if (clen->rlogin_command && clen->rlogin_command[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "rlogin_command");
         if (!ep)
            ep = lAddElemStr(&confl, CF_name, "rlogin_command", CF_Type);
         lSetString(ep, CF_value, clen->rlogin_command);
      }
      else {
         lDelElemStr(&confl, CF_name, "rlogin_command");
      }

#if 0
      if (clen->starter_method && clen->starter_method[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "starter_method");
         if (!ep)
            ep = lAddElemStr(&confl, CF_name, "starter_method", CF_Type);
         lSetString(ep, CF_value, clen->starter_method);
      }
         
      if (clen->suspend_method && clen->suspend_method[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "suspend_method");
         lSetString(ep, CF_value, clen->suspend_method);
      }
         
      if (clen->resume_method && clen->resume_method[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "resume_method");
         if (!ep)
            ep = lAddElemStr(&confl, CF_name, "resume_method", CF_Type);
         lSetString(ep, CF_value, clen->resume_method);
      }
         
      if (clen->terminate_method && clen->terminate_method[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "terminate_method");
         if (!ep)
            ep = lAddElemStr(&confl, CF_name, "terminate_method", CF_Type);
         lSetString(ep, CF_value, clen->terminate_method);
      }
#endif

      if (clen->admin_user && clen->admin_user[0] != '\0') {
         ep = lGetElemStr(confl, CF_name, "admin_user");
         if (!ep)
            ep = lAddElemStr(&confl, CF_name, "admin_user", CF_Type);
         lSetString(ep, CF_value, clen->admin_user);
      }
      else {
         lDelElemStr(&confl, CF_name, "admin_user");
      }



      if (feature_is_enabled(FEATURE_AFS_SECURITY)) {
         if (clen->set_token_cmd && clen->set_token_cmd[0] != '\0') {
            ep = lGetElemStr(confl, CF_name, "set_token_cmd");
            if (!ep)
               ep = lAddElemStr(&confl, CF_name, "set_token_cmd", CF_Type);
            lSetString(ep, CF_value, clen->set_token_cmd);
         }
         else {
            lDelElemStr(&confl, CF_name, "set_token_cmd");
         }

         if (clen->pag_cmd && clen->pag_cmd[0] != '\0') {
            ep = lGetElemStr(confl, CF_name, "pag_cmd");
            if (!ep)
               ep = lAddElemStr(&confl, CF_name, "pag_cmd", CF_Type);
            lSetString(ep, CF_value, clen->pag_cmd);
         }
         else {
            lDelElemStr(&confl, CF_name, "pag_cmd");
         }

         if (clen->token_extend_time && clen->token_extend_time[0] != '\0') {
            ep = lGetElemStr(confl, CF_name, "token_extend_time");
            if (!ep)
               ep = lAddElemStr(&confl, CF_name, "token_extend_time", CF_Type);
            lSetString(ep, CF_value, clen->token_extend_time);
         }
         else {
            lDelElemStr(&confl, CF_name, "token_extend_time");
         }

      }
      lp = lCopyList("Confl", confl);
   }

   if (lGetNumberOfElem(lp) == 0) {
      lp = lFreeList(lp);
      *lpp  = NULL;
      DEXIT;
      return True;
   }

   *lpp = lp;

   DEXIT;
   return True;

   error:
      qmonMessageShow(w, True, errstr);
      lp = lFreeList(lp);
      *lpp = NULL;
      DEXIT;
      return False;
}

/*-------------------------------------------------------------------------*/
static void qmonCullToCClEntry(
lListElem *cep,
tCClEntry *clen 
) {
   lList *confl;
   lListElem *ep;
   StringConst str = NULL;
   StringConst min_uid;
   StringConst min_gid;
   StringConst max_aj_instances;
   StringConst max_aj_tasks;
   StringConst max_u_jobs;
   StringConst zombie_jobs;

   DENTER(GUI_LAYER, "qmonCullToCClEntry");

   confl = lGetList(cep, CONF_entries);

   if ((ep = lGetElemStr(confl, CF_name, "qmaster_spool_dir")))
      clen->qmaster_spool_dir = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "execd_spool_dir")))
      clen->execd_spool_dir = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "binary_path")))
      clen->binary_path = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "mailer")))
      clen->mailer = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "xterm")))
      clen->xterm = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "load_sensor")))
      clen->load_sensor = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "administrator_mail")))
      clen->administrator_mail = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "min_uid"))) {
      min_uid = lGetString(ep, CF_value);
      clen->min_uid = min_uid ? atoi(min_uid) : 0;
   }
   if ((ep = lGetElemStr(confl, CF_name, "min_gid"))) {
      min_gid = lGetString(ep, CF_value);
      clen->min_gid = min_gid ? atoi(min_gid) : 0;
   }
   if ((ep = lGetElemStr(confl, CF_name, "max_aj_instances"))) {
      max_aj_instances = lGetString(ep, CF_value);
      clen->max_aj_instances = max_aj_instances ? atoi(max_aj_instances) : 0;
   }
   if ((ep = lGetElemStr(confl, CF_name, "max_aj_tasks"))) {
      max_aj_tasks = lGetString(ep, CF_value);
      clen->max_aj_tasks = max_aj_tasks ? atoi(max_aj_tasks) : 0;
   }
   if ((ep = lGetElemStr(confl, CF_name, "max_u_jobs"))) {
      max_u_jobs = lGetString(ep, CF_value);
      clen->max_u_jobs = max_u_jobs ? atoi(max_u_jobs) : 0;
   }
   if ((ep = lGetElemStr(confl, CF_name, "finished_jobs"))) {
      zombie_jobs = lGetString(ep, CF_value);
      clen->zombie_jobs = zombie_jobs ? atoi(zombie_jobs) : 0;
   }

   if ((ep = lGetElemStr(confl, CF_name, "prolog")))
      clen->prolog = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "epilog")))
      clen->epilog = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "qlogin_daemon")))
      clen->qlogin_daemon = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "qlogin_command")))
      clen->qlogin_command = XtNewString(lGetString(ep, CF_value));

#if 0
   if ((ep = lGetElemStr(confl, CF_name, "starter_method")))
      clen->starter_method = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "suspend_method")))
      clen->suspend_method = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "resume_method")))
      clen->resume_method = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "terminate_method")))
      clen->terminate_method = XtNewString(lGetString(ep, CF_value));
#endif

   if ((ep = lGetElemStr(confl, CF_name, "login_shells")))
      clen->login_shells = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "default_domain")))
      clen->default_domain = XtNewString(lGetString(ep, CF_value)); 

   if ((ep = lGetElemStr(confl, CF_name, "load_report_time")))
      clen->load_report_time = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "stat_log_time")))
      clen->stat_log_time = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "max_unheard")))
      clen->max_unheard = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "reschedule_unknown")))
      clen->reschedule_unknown = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "shell_start_mode")))
      str = lGetString(ep, CF_value);
   if (str && !strcmp(str, "script_from_stdin"))
      clen->shell_start_mode = 1;
   else if (str && !strcmp(str, "unix_behavior"))
      clen->shell_start_mode = 2;
   else /* posix_compliant */
      clen->shell_start_mode = 0;


   if ((ep = lGetElemStr(confl, CF_name, "loglevel")))
      str = lGetString(ep, CF_value);
   if (str && !strcmp(str, "log_info"))
      clen->loglevel = 0;
   if (str && !strcmp(str, "log_warning"))
      clen->loglevel = 1;
   if (str && !strcmp(str, "log_err"))
      clen->loglevel = 2;

      
   if ((ep = lGetElemStr(confl, CF_name, "ignore_fqdn")))
      str = lGetString(ep, CF_value);
      if (str && !strcasecmp(str, "true"))
         clen->ignore_fqdn = 0;
      else
         clen->ignore_fqdn = 1;


/*    if ((ep = lGetElemStr(confl, CF_name, "logmail"))) */
/*       str = lGetString(ep, CF_value); */
/*    if (str && !strcasecmp(str, "true")) */
/*       clen->logmail = 0; */
/*    else */
/*       clen->logmail = 1; */

   if ((ep = lGetElemStr(confl, CF_name, "user_lists"))) {
      clen->cluster_users = lFreeList(clen->cluster_users);
      lString2ListNone(lGetString(ep, CF_value), &clen->cluster_users, 
                           US_Type, US_name, NULL);
   }

   if ((ep = lGetElemStr(confl, CF_name, "xuser_lists"))) {
      clen->cluster_xusers = lFreeList(clen->cluster_xusers);
      lString2ListNone(lGetString(ep, CF_value), &clen->cluster_xusers, 
                           US_Type, US_name, NULL);
   }

   if (feature_is_enabled(FEATURE_SGEEE)) {
      if ((ep = lGetElemStr(confl, CF_name, "enforce_project")))
         str = lGetString(ep, CF_value);
      if (str && !strcasecmp(str, "true"))
         clen->enforce_project = 0;
      else
         clen->enforce_project = 1;

      if ((ep = lGetElemStr(confl, CF_name, "enforce_user")))
         str = lGetString(ep, CF_value);
      if (str && !strcasecmp(str, "true"))
         clen->enforce_user = 0;
      else
         clen->enforce_user = 1;

      if ((ep = lGetElemStr(confl, CF_name, "gid_range")))
         clen->gid_range = XtNewString(lGetString(ep, CF_value));

      if ((ep = lGetElemStr(confl, CF_name, "projects"))) {
         clen->cluster_projects = lFreeList(clen->cluster_projects);
         lString2ListNone(lGetString(ep, CF_value), &clen->cluster_projects, 
                              UP_Type, UP_name, NULL);
      }

      if ((ep = lGetElemStr(confl, CF_name, "xprojects"))) {
         clen->cluster_xprojects = lFreeList(clen->cluster_xprojects);
         lString2ListNone(lGetString(ep, CF_value), &clen->cluster_xprojects, 
                              UP_Type, UP_name, NULL);
      }

   }

   if ((ep = lGetElemStr(confl, CF_name, "qmaster_params")))
      clen->qmaster_params = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "schedd_params")))
      clen->schedd_params = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "execd_params")))
      clen->execd_params = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "shepherd_cmd")))
      clen->shepherd_cmd = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "rsh_daemon")))
      clen->rsh_daemon = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "rsh_command")))
      clen->rsh_command = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "rlogin_daemon")))
      clen->rlogin_daemon = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "rlogin_command")))
      clen->rlogin_command = XtNewString(lGetString(ep, CF_value));

   if ((ep = lGetElemStr(confl, CF_name, "admin_user")))
      clen->admin_user = XtNewString(lGetString(ep, CF_value));

   if (feature_is_enabled(FEATURE_AFS_SECURITY)) {
      if ((ep = lGetElemStr(confl, CF_name, "set_token_cmd")))
         clen->set_token_cmd = XtNewString(lGetString(ep, CF_value));

      if ((ep = lGetElemStr(confl, CF_name, "pag_cmd")))
         clen->pag_cmd = XtNewString(lGetString(ep, CF_value));

      if ((ep = lGetElemStr(confl, CF_name, "token_extend_time")))
         clen->token_extend_time = XtNewString(lGetString(ep, CF_value));
   }
   DEXIT;

}
 
/*-------------------------------------------------------------------------*/
static void qmonInitCClEntry(
tCClEntry *clen 
) {
   DENTER(GUI_LAYER, "qmonInitCClEntry");

   memset((void*)clen, sizeof(tCClEntry), 0);

   DEXIT; 
}

/*-------------------------------------------------------------------------*/
static void qmonClearCClEntry(
tCClEntry *clen 
) {
   DENTER(GUI_LAYER, "qmonFreeCClEntry");

   if (clen->qmaster_spool_dir) {
      XtFree((char*)clen->qmaster_spool_dir);
      clen->qmaster_spool_dir = NULL;
   }
   if (clen->execd_spool_dir) {
      XtFree((char*)clen->execd_spool_dir);
      clen->execd_spool_dir = NULL;
   }
   if (clen->binary_path) {
      XtFree((char*)clen->binary_path);
      clen->binary_path = NULL;
   }
   if (clen->mailer) {
      XtFree((char*)clen->mailer);
      clen->mailer = NULL;
   }
   if (clen->xterm) {
      XtFree((char*)clen->xterm);
      clen->xterm = NULL;
   }
   if (clen->load_sensor) {
      XtFree((char*)clen->load_sensor);
      clen->load_sensor = NULL;
   }
   if (clen->administrator_mail) {
      XtFree((char*)clen->administrator_mail);
      clen->mailer = NULL;
   }
   if (clen->prolog) {
      XtFree((char*)clen->prolog);
      clen->prolog = NULL;
   }
   if (clen->epilog) {
      XtFree((char*)clen->epilog);
      clen->epilog = NULL;
   }

   if (clen->qlogin_daemon) {
      XtFree((char*)clen->qlogin_daemon);
      clen->qlogin_daemon = NULL;
   }

   if (clen->qlogin_command) {
      XtFree((char*)clen->qlogin_command);
      clen->qlogin_command = NULL;
   }


#if 0
   if (clen->starter_method) {
      XtFree((char*)clen->starter_method);
      clen->starter_method = NULL;
   }
   if (clen->suspend_method) {
      XtFree((char*)clen->suspend_method);
      clen->suspend_method = NULL;
   }
   if (clen->resume_method) {
      XtFree((char*)clen->resume_method);
      clen->resume_method = NULL;
   }
   if (clen->terminate_method) {
      XtFree((char*)clen->terminate_method);
      clen->terminate_method = NULL;
   }
#endif
   clen->shell_start_mode = 0;
   if (clen->login_shells) {
      XtFree((char*)clen->login_shells);
      clen->login_shells = NULL;
   }
   if (clen->default_domain) {
      XtFree((char*)clen->default_domain);
      clen->default_domain = NULL;
   } 
   clen->min_uid = 0;
   clen->min_gid = 0;
   clen->max_aj_instances = 0;
   clen->max_aj_tasks = 0;
   clen->max_u_jobs = 0;
   if (clen->load_report_time) {
      XtFree((char*)clen->load_report_time);
      clen->load_report_time = NULL;
   }
   if (clen->stat_log_time) {
      XtFree((char*)clen->stat_log_time);
      clen->stat_log_time = NULL;
   }
   if (clen->max_unheard) {
      XtFree((char*)clen->max_unheard);
      clen->max_unheard = NULL;
   }
   if (clen->reschedule_unknown) {
      XtFree((char*)clen->reschedule_unknown);
      clen->reschedule_unknown = NULL;
   }
   clen->loglevel = 0; 
   clen->ignore_fqdn = 0; 
   clen->logmail = 0;    
   clen->cluster_users = lFreeList(clen->cluster_users);
   clen->cluster_xusers = lFreeList(clen->cluster_xusers);
   clen->cluster_projects = lFreeList(clen->cluster_projects);
   clen->cluster_xprojects = lFreeList(clen->cluster_xprojects);
   clen->enforce_project = 1;    
   clen->enforce_user = 1;    

   if (clen->qmaster_params) {
      XtFree((char*)clen->qmaster_params);
      clen->qmaster_params = NULL;
   }
   if (clen->schedd_params) {
      XtFree((char*)clen->schedd_params);
      clen->schedd_params = NULL;
   }
   if (clen->execd_params) {
      XtFree((char*)clen->execd_params);
      clen->execd_params = NULL;
   }
   if (clen->shepherd_cmd) {
      XtFree((char*)clen->shepherd_cmd);
      clen->shepherd_cmd = NULL;
   }
   if (clen->rsh_daemon) {
      XtFree((char*)clen->rsh_daemon);
      clen->rsh_daemon = NULL;
   }
   if (clen->rsh_command) {
      XtFree((char*)clen->rsh_command);
      clen->rsh_command = NULL;
   }
   if (clen->rlogin_daemon) {
      XtFree((char*)clen->rlogin_daemon);
      clen->rlogin_daemon = NULL;
   }
   if (clen->rlogin_command) {
      XtFree((char*)clen->rlogin_command);
      clen->rlogin_command = NULL;
   }
   if (clen->set_token_cmd) {
      XtFree((char*)clen->set_token_cmd);
      clen->set_token_cmd = NULL;
   }
   if (clen->pag_cmd) {
      XtFree((char*)clen->pag_cmd);
      clen->pag_cmd = NULL;
   }
   if (clen->token_extend_time) {
      XtFree((char*)clen->token_extend_time);
      clen->token_extend_time = NULL;
   }
   if (clen->admin_user) {
      XtFree((char*)clen->admin_user);
      clen->admin_user = NULL;
   }
   if (clen->gid_range) {
      XtFree((char*)clen->gid_range);
      clen->gid_range = NULL;
   }
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static Boolean check_white(
char *str 
) {
   if (!str)
      return False;

   if (strrchr(str, '\t') || strrchr(str, ' '))
      return True;
   else
      return False;
}

/*-------------------------------------------------------------------------*/
static void qmonClusterAskForUsers(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *ql_out = NULL;
   lList *ql_in = NULL;
   int status;
   Widget list = (Widget) cld;
   lList *alp = NULL;

   DENTER(GUI_LAYER, "qmonClusterAskForUsers");
   
   qmonMirrorMultiAnswer(USERSET_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }
   ql_in = qmonMirrorList(SGE_USERSET_LIST);
   ql_out = XmStringToCull(list, US_Type, US_name, ALL_ITEMS);

   status = XmtAskForItems(w, NULL, NULL, "@{Select Access Lists}", ql_in, US_name,
                  "@{@fBAvailable Access Lists}", &ql_out, US_Type, US_name, 
                  "@{@fBChosen Access Lists}", NULL);

   if (status) {
      UpdateXmListFromCull(list, XmFONTLIST_DEFAULT_TAG, ql_out,
                              US_name);
   }
   ql_out = lFreeList(ql_out);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonClusterAskForProjects(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *ql_out = NULL;
   lList *ql_in = NULL;
   int status;
   Widget list = (Widget) cld;
   lList *alp = NULL;

   DENTER(GUI_LAYER, "qmonClusterAskForProjects");
   
   qmonMirrorMultiAnswer(PROJECT_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      alp = lFreeList(alp);
      DEXIT;
      return;
   }
   ql_in = qmonMirrorList(SGE_PROJECT_LIST);
   ql_out = XmStringToCull(list, UP_Type, UP_name, ALL_ITEMS);

   status = XmtAskForItems(w, NULL, NULL, "@{Select Project}", ql_in, UP_name,
                  "@{@fBAvailable Projects}", &ql_out, UP_Type, UP_name, 
                  "@{@fBChosen Projects}", NULL);

   if (status) {
      UpdateXmListFromCull(list, XmFONTLIST_DEFAULT_TAG, ql_out,
                              UP_name);
   }
   ql_out = lFreeList(ql_out);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonClusterHost(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   XmtInputFieldCallbackStruct *cbs = (XmtInputFieldCallbackStruct*) cad;
   static char unique[MAXHOSTLEN];
   int ret;

   DENTER(GUI_LAYER, "qmonClusterHost");

   unique[0] = '\0';
   
   if (cbs->input && cbs->input[0] != '\0' && cbs->input[0] != ' ') { 
       
      DPRINTF(("cbs->input = %s\n", cbs->input));

      /* try to resolve hostname */
      ret=sge_resolve_hostname((const char*)cbs->input, unique, EH_name);

      switch ( ret ) {
         case COMMD_NACK_UNKNOWN_HOST:
            qmonMessageShow(w, True, "can't resolve host '%s'\n", cbs->input);
            cbs->okay = False;
            break;
         case CL_OK:
            cbs->input = unique;
            break;
         default:
            DPRINTF(("sge_resolve_hostname() failed resolving: %s\n",
            cl_errstr(ret)));
            cbs->okay = False;
      }
   }
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonClusterCheckInput(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
#define MAX_INPUT_LEN   4*BUFSIZ
   XmtInputFieldCallbackStruct *cbs = (XmtInputFieldCallbackStruct*) cad;
   static char buf[MAX_INPUT_LEN];
   int i;
   static char *start = NULL;

   DENTER(GUI_LAYER, "qmonClusterCheckInput");
   
   if (cbs->input && cbs->input[0] != '\0') {
      DPRINTF(("cbs->input = '%s'\n", cbs->input));
      strncpy(buf, cbs->input, MAX_INPUT_LEN);
      for (start = buf; *start && isspace(*start); start++)
         ;
      for (i=strlen(buf)-1; isspace(buf[i]) && i>0; i--)
         buf[i] = '\0';
      cbs->input = start;

      DPRINTF(("cbs->input = '%s'\n", cbs->input));
   }
   
   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonClusterTime(
Widget w,
XtPointer cld,
XtPointer cad 
) {
   Widget input_field = (Widget) cld;
   char stringval[256];
   Boolean status = False;
   String current;

   DENTER(GUI_LAYER, "qmonClusterTime");

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
