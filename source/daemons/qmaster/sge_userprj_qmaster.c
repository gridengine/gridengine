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
/*
   This is the module for handling users and projects.
   We save users to <spool>/qmaster/$USER_DIR and
                                    $PROJECT_DIR
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "sge.h"
#include "sgermon.h"
#include "sge_prog.h"
#include "sge_conf.h"
#include "sge_time.h"
#include "sge_manop.h"
#include "sge_gdi_request.h"
#include "sge_gdi.h"
#include "sge_usageL.h"
#include "sge_userprj_qmaster.h"
#include "sge_userset_qmaster.h"
#include "sge_sharetree_qmaster.h"
#include "sge_event_master.h"
#include "cull_parse_util.h"
#include "config.h"
#include "sge_log.h"
#include "sge_answer.h"
#include "sge_qinstance.h"
#include "sge_userprj.h"
#include "sge_host.h"
#include "sge_userset.h"
#include "sge_sharetree.h"
#include "sge_utility.h"
#include "sge_utility_qmaster.h"
#include "sge_cqueue.h"
#include "sge_suser.h"
#include "sge_lock.h"

#include "sge_persistence_qmaster.h"
#include "spool/sge_spooling.h"

#include "msg_common.h"
#include "msg_qmaster.h"


static int do_add_auto_user(lListElem*, int, lList**);


int userprj_mod(
lList **alpp,
lListElem *modp,
lListElem *ep,
int add,
const char *ruser,
const char *rhost,
gdi_object_t *object,
int sub_command 
) {
   int user_flag = (object->target==SGE_USER_LIST)?1:0;
   int pos;
   const char *userprj;
   u_long32 uval;
   u_long32 up_new_version;
   lList *lp;
   const char *obj_name;
   int make_auto_user_permanent = 0;

   DENTER(TOP_LAYER, "userprj_mod");
  
   obj_name = user_flag ? MSG_OBJ_USER : MSG_OBJ_PRJ;  

   /* ---- UP_name */
   userprj = lGetString(ep, UP_name);
   if (add) {
      if (!strcmp(userprj, "default")) {
         ERROR((SGE_EVENT, MSG_UP_NOADDDEFAULT_S, obj_name));
         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         goto Error;
      }
      /* may not add user with same name as an existing project */
      if (lGetElemStr(user_flag?Master_Project_List:Master_User_List, 
                        UP_name, userprj)) {
         ERROR((SGE_EVENT, MSG_UP_ALREADYEXISTS_SS,user_flag ? MSG_OBJ_PRJ : MSG_OBJ_USER , userprj));

         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         goto Error;
      }
      if (verify_str_key(alpp, userprj, obj_name))
         goto Error;
      lSetString(modp, UP_name, userprj);
   }

   /* ---- UP_oticket */
   if ((pos=lGetPosViaElem(ep, UP_oticket))>=0) {
      uval = lGetPosUlong(ep, pos);
      lSetUlong(modp, UP_oticket, uval);
      make_auto_user_permanent = 1;
   }

   /* ---- UP_fshare */
   if ((pos=lGetPosViaElem(ep, UP_fshare))>=0) {
      uval = lGetPosUlong(ep, pos);
      lSetUlong(modp, UP_fshare, uval);
      make_auto_user_permanent = 1;
   }

   /* ---- UP_delete_time */
   if ((pos=lGetPosViaElem(ep, UP_delete_time))>=0) {
      uval = lGetPosUlong(ep, pos);
      lSetUlong(modp, UP_delete_time, uval);
   }

   up_new_version = lGetUlong(modp, UP_version)+1;

   /* ---- UP_usage */
   if ((pos=lGetPosViaElem(ep, UP_usage))>=0) {
      lp = lGetPosList(ep, pos);
      lSetList(modp, UP_usage, lCopyList("usage", lp));
      lSetUlong(modp, UP_version, up_new_version);
   }

   /* ---- UP_project */
   if ((pos=lGetPosViaElem(ep, UP_project))>=0) {
      lp = lGetPosList(ep, pos);
      lSetList(modp, UP_project, lCopyList("project", lp));
      lSetUlong(modp, UP_version, up_new_version);
   }

   if (user_flag) {
      /* ---- UP_default_project */
      if ((pos=lGetPosViaElem(ep, UP_default_project))>=0) {
         const char *dproj;

         /* make sure default project exists */
         if ((dproj = lGetPosString(ep, pos))) {
            if (!Master_Project_List ||
                !userprj_list_locate(Master_Project_List, dproj)) {
               ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, MSG_OBJ_PRJ, dproj));
               answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
               goto Error;
            }
         }

         lSetString(modp, UP_default_project, dproj);
         make_auto_user_permanent = 1;
      }

#if 0 /* SVD040202 - commented out because we only make user permanent if
         the delete_time is adjusted */
      /* if one of the attributes has been edited, make the user object permanent */
      if (!add && make_auto_user_permanent) {
         lSetUlong(modp, UP_delete_time, 0);
      }
#endif

   }
   else {
      /* ---- UP_acl */
      if ((pos=lGetPosViaElem(ep, UP_acl))>=0) {
         lp = lGetPosList(ep, pos);
         lSetList(modp, UP_acl, lCopyList("acl", lp));

         if (userset_list_validate_acl_list(lGetList(ep, UP_acl), alpp)!=STATUS_OK) {
            /* answerlist gets filled by userset_list_validate_acl_list() in case of errors */
            goto Error;
         }
      }

      /* ---- UP_xacl */
      if ((pos=lGetPosViaElem(ep, UP_xacl))>=0) {
         lp = lGetPosList(ep, pos);
         lSetList(modp, UP_xacl, lCopyList("xacl", lp));
         if (userset_list_validate_acl_list(lGetList(ep, UP_xacl), alpp)!=STATUS_OK) {
            /* answerlist gets filled by userset_list_validate_acl_list() in case of errors */
            goto Error;
         }
      }

      if (lGetPosViaElem(modp, UP_xacl)>=0 || 
          lGetPosViaElem(modp, UP_acl)>=0) {
         if (multiple_occurances( alpp,
               lGetList(modp, UP_acl),
               lGetList(modp, UP_xacl),
               US_name, userprj, 
               "project")) {
            goto Error;
         }
      }
   }

   DEXIT;
   return 0;

Error:
   DEXIT;
   return STATUS_EUNKNOWN;
}

int userprj_success(lListElem *ep, lListElem *old_ep, gdi_object_t *object) 
{
   int user_flag = (object->target==SGE_USER_LIST)?1:0;
   
   DENTER(TOP_LAYER, "userprj_success");
   sge_add_event( 0, old_ep?
                 (user_flag?sgeE_USER_MOD:sgeE_PROJECT_MOD) :
                 (user_flag?sgeE_USER_ADD:sgeE_PROJECT_ADD), 
                 0, 0, lGetString(ep, UP_name), NULL, NULL, ep);
   lListElem_clear_changed_info(ep);

   DEXIT;
   return 0;
}

int userprj_spool(
lList **alpp,
lListElem *upe,
gdi_object_t *object 
) {
   lList *answer_list = NULL;
   bool dbret;

   int user_flag = (object->target==SGE_USER_LIST)?1:0;

   DENTER(TOP_LAYER, "userprj_spool");

   /* write user or project to file */
   dbret = spool_write_object(alpp, spool_get_default_context(), upe, 
                              lGetString(upe, object->key_nm), 
                              user_flag ? SGE_TYPE_USER : SGE_TYPE_PROJECT);
   answer_list_output(&answer_list);

   if (!dbret) {
      answer_list_add_sprintf(alpp, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_PERSISTENCE_WRITE_FAILED_S,
                              lGetString(upe, object->key_nm));
   }

   DEXIT;
   return dbret ? 0 : 1;
}


/***********************************************************************
   master code: delete a user or project
 ***********************************************************************/
int sge_del_userprj(
lListElem *up_ep,
lList **alpp,
lList **upl,    /* list to change */
char *ruser,
char *rhost,
int user        /* =1 user, =0 project */
) {
   const char *name;
   lListElem *ep;
   lListElem *myep;

   DENTER(TOP_LAYER, "sge_del_userprj");

   if ( !up_ep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   name = lGetString(up_ep, UP_name);
   

   if (!(ep=userprj_list_locate(*upl, name))) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, user? MSG_OBJ_USER : MSG_OBJ_PRJ, name));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* ensure this u/p object is not referenced in actual share tree */
   if (getNode(Master_Sharetree_List, name, user ? STT_USER : STT_PROJECT, 0)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANT_DELETE_UP_IN_SHARE_TREE_SS, user?MSG_OBJ_USER:MSG_OBJ_PRJ, name));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   if (user==0) { /* ensure this project is not referenced in any queue */
      lListElem *cqueue;
      lListElem *ep;

      /* check queues */
      for_each (cqueue, *(object_type_get_master_list(SGE_TYPE_CQUEUE))) {
         lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
         lListElem *qinstance;

         for_each(qinstance, qinstance_list) {
            if (userprj_list_locate(lGetList(qinstance, QU_projects), name)) {
               ERROR((SGE_EVENT, MSG_SGETEXT_PROJECTSTILLREFERENCED_SSSS, name, 
                     MSG_OBJ_PRJS, MSG_OBJ_QUEUE, 
                     lGetString(qinstance, QU_qname)));
               answer_list_add(alpp, SGE_EVENT, 
                               STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
               DEXIT;
               return STATUS_EEXIST;
            }
            if (userprj_list_locate(lGetList(qinstance, QU_xprojects), name)) {
               ERROR((SGE_EVENT, MSG_SGETEXT_PROJECTSTILLREFERENCED_SSSS, name, 
                     MSG_OBJ_XPRJS, MSG_OBJ_QUEUE, 
                     lGetString(qinstance, QU_qname)));
               answer_list_add(alpp, SGE_EVENT, 
                               STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
               DEXIT;
               return STATUS_EEXIST;
            }
         }
      }

      /* check hosts */
      for_each (ep, Master_Exechost_List) {
         if (userprj_list_locate(lGetList(ep, EH_prj), name)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_PROJECTSTILLREFERENCED_SSSS, name, 
                  MSG_OBJ_PRJS, MSG_OBJ_EH, lGetHost(ep, EH_name)));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         }
         if (userprj_list_locate(lGetList(ep, EH_xprj), name)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_PROJECTSTILLREFERENCED_SSSS, name, 
                  MSG_OBJ_XPRJS, MSG_OBJ_EH, lGetHost(ep, EH_name)));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         }
      }

      /* check global configuration */
      if (userprj_list_locate(conf.projects, name)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_PROJECTSTILLREFERENCED_SSSS, name, 
               MSG_OBJ_PRJS, MSG_OBJ_CONF, MSG_OBJ_GLOBAL));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EEXIST;
      }
      if (userprj_list_locate(conf.xprojects, name)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_PROJECTSTILLREFERENCED_SSSS, name, 
               MSG_OBJ_XPRJS, MSG_OBJ_CONF, MSG_OBJ_GLOBAL));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EEXIST;
      }

      /* check user list for reference */
      if ((myep = lGetElemStr(Master_User_List, UP_default_project, name))) {
         ERROR((SGE_EVENT, MSG_USERPRJ_PRJXSTILLREFERENCEDINENTRYX_SS, name, lGetString (myep, UP_name)));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EEXIST;
      }
   }

   /* delete user or project file */
   if (!sge_event_spool(alpp, 0, user ? sgeE_USER_DEL : sgeE_PROJECT_DEL,
                        0, 0, name, NULL, NULL,
                        NULL, NULL, NULL, true, true)) {

      DEXIT;
      return STATUS_EDISK;
   }

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS,
         ruser, rhost, name, user?MSG_OBJ_USER:MSG_OBJ_PRJ));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);

   lRemoveElem(*upl, ep);

   DEXIT;
   return STATUS_OK;
}

int verify_userprj_list(
lList **alpp,
lList *name_list,
lList *userprj_list,
const char *attr_name, /* e.g. "xprojects" */
const char *obj_descr, /* e.g. "host"      */
const char *obj_name   /* e.g. "fangorn"  */
) {
   lListElem *up;

   DENTER(TOP_LAYER, "verify_userprj_list");

   for_each (up, name_list) {
      if (!lGetElemStr(userprj_list, UP_name, lGetString(up, UP_name))) {
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWNPROJECT_SSSS, lGetString(up, UP_name), 
               attr_name, obj_descr, obj_name));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }

   DEXIT;
   return STATUS_OK;
}

/*-------------------------------------------------------------------------*/
/* sge_automatic_user_cleanup_handler - handles automatically deleting     */
/* GEEE automatic user objects which have expired.                         */
/*-------------------------------------------------------------------------*/
void
sge_automatic_user_cleanup_handler(te_event_t anEvent)
{
   lListElem *user, *next;
   u_long32 now = sge_get_gmt();
   char *root = "root";
   const char *qmaster_host = uti_state_get_qualified_hostname();

   DENTER(TOP_LAYER, "sge_automatic_user_cleanup_handler");

   SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE);

   /*
    * Check each user for deletion time. We don't use for_each()
    * because we are deleting entries.
    */
   for (user=lFirst(Master_User_List); user; user=next) {
      u_long32 delete_time = lGetUlong(user, UP_delete_time);
      next = lNext(user);
#define EXTEND_AUTO_USER_DELETE_TIME
#ifdef EXTEND_AUTO_USER_DELETE_TIME
      if (delete_time > 0 && conf.auto_user_delete_time > 0) {
         /*
          * The delete time will be (re)set to a value of 1 when a new
          * job is submitted.  If the delete time is 1 and there are
          * no more jobs for this user, we set the actual time
          * that the job will be deleted.
          */
         const char *name = lGetString(user, UP_name);
         lList *answer_list = NULL;
         if (delete_time == 1 && suser_get_job_counter(suser_list_find(Master_SUser_List, lGetString(user, UP_name))) == 0) {
            delete_time = now + conf.auto_user_delete_time;
            lSetUlong(user, UP_delete_time, delete_time);
            /*
             * Note: If we don't spool the delete time change,
             * we run the risk of never deleting user entries if
             * the qmaster is restarted at an interval which
             * is less than the configured auto_user_delete_time.
             */
            sge_event_spool(&answer_list, 0, sgeE_USER_MOD,
                            0, 0, name, NULL, NULL,
                            user, NULL, NULL, true, true);
         }
         if (delete_time > 1 && delete_time < now) {
            if (sge_del_userprj(user, &answer_list, &Master_User_List, root,
                                 (char *)qmaster_host, 1) != STATUS_OK) {
               /* only try to delete it once ... */
               lSetUlong(user, UP_delete_time, 0);
            }
         }
         /* output low level error messages */
         answer_list_output(&answer_list);
      }
#else
      if (delete_time > 0 && delete_time < now) {
         if (sge_del_userprj(user, NULL, &Master_User_List, root,
                              (char *)qmaster_host, 1) != STATUS_OK) {
            /* only try to delete it once ... */
            lSetUlong(user, UP_delete_time, 0);
         }
      }
#endif
   }

   SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);

   DEXIT;
   return;
}

/*-------------------------------------------------------------------------*/
/* sge_add_auto_user - handles automatically adding GEEE user objects      */
/*    called in sge_gdi_add_job                                            */
/*-------------------------------------------------------------------------*/
int
sge_add_auto_user(char *user, char *host, sge_gdi_request *request, lList **alpp)
{
   int manager_added = 0, admin_host_added = 0;
   lListElem *uep, *ep = NULL;
   int status = STATUS_OK;

   DENTER(TOP_LAYER, "sge_add_auto_user");

   uep = userprj_list_locate(Master_User_List, user);

   /*
    * if permanent user already exists or we don't need to
    * extend the life of the automatic user, we're done
    */
   if (uep && lGetUlong(uep, UP_delete_time) <= 1)
   {
      DEXIT;
      return STATUS_OK;
   }

   /*
    * User object will be added or modifed by this user, so temporarily
    * make the user a manager and the host an admin host.
    */
   if (!manop_is_manager(user))
   {
      lAddElemStr(&Master_Manager_List, MO_name, user, MO_Type);
      manager_added = 1;
   }

   if (!host_list_locate(Master_Adminhost_List, host))
   {
      lAddElemHost(&Master_Adminhost_List, AH_name, host, AH_Type);
      admin_host_added = 1;
   }

   ep = lCreateElem(UP_Type); /* create the user element to be added */ 

   if (uep)
   {
      /* modify user element (extend life) */
      lSetString(ep, UP_name, user);

      if (conf.auto_user_delete_time > 0)
      {
#ifdef EXTEND_AUTO_USER_DELETE_TIME
         lSetUlong(ep, UP_delete_time, 1);
#else
         lSetUlong(ep, UP_delete_time, sge_get_gmt() + conf.auto_user_delete_time);
#endif
      }
      else
      {
         lSetUlong(ep, UP_delete_time, 0);
      }
   }
   else
   {
      /* add automatic user element */
      lSetString(ep, UP_name, user);
      lSetUlong(ep, UP_oticket, conf.auto_user_oticket);
      lSetUlong(ep, UP_fshare, conf.auto_user_fshare);

      if (!conf.auto_user_default_project || !strcasecmp(conf.auto_user_default_project, "none"))
         lSetString(ep, UP_default_project, NULL);
      else
         lSetString(ep, UP_default_project, conf.auto_user_default_project);

      if (conf.auto_user_delete_time > 0)
      {
#ifdef EXTEND_AUTO_USER_DELETE_TIME
         lSetUlong(ep, UP_delete_time, 1);
#else
         lSetUlong(ep, UP_delete_time, sge_get_gmt() + conf.auto_user_delete_time);
#endif
      }
      else
      {
         lSetUlong(ep, UP_delete_time, 0);
      }
   }

   status = do_add_auto_user(ep, ((NULL != uep) ? SGE_GDI_MOD : SGE_GDI_ADD), alpp); 

   /* clean up the manager and admin host */
   if (manager_added) {
      lDelElemStr(&Master_Manager_List, MO_name, user);
   }

   if (admin_host_added) {
      lDelElemHost(&Master_Adminhost_List, AH_name, host);
   }
   
   ep = lFreeElem(ep);

   DEXIT;
   return status;
}

/****** qmaster/sge_userprj_qmaster/do_add_auto_user() *************************
*  NAME
*     do_add_auto_user() -- add auto user to SGE_USER_LIST
*
*  SYNOPSIS
*     static int do_add_auto_user(lListElem*, int, lList**) 
*
*  NOTES
*     MT-NOTE: do_add_auto_user() is not MT safe 
*
*******************************************************************************/
static int do_add_auto_user(lListElem* anUser, int aMode, lList** anAnswer)
{
   int flag, res = STATUS_EUNKNOWN;
   gdi_object_t *userList = NULL;
   lList *tmpAnswer = NULL;

   DENTER(TOP_LAYER, "do_add_auto_user");

   flag = (SGE_GDI_MOD == aMode) ? 0 : 1; /* add or modify user? */

   userList = get_gdi_object(SGE_USER_LIST);

   res = sge_gdi_add_mod_generic(&tmpAnswer, anUser, flag, userList, "", "", 0);

   if ((STATUS_OK != res) && (NULL != tmpAnswer))
   {
      lListElem *err   = lFirst(tmpAnswer);
      const char *text = lGetString(err, AN_text);
      u_long32 status  = lGetUlong(err, AN_status);
      u_long32 quality = lGetUlong(err, AN_quality);

      answer_list_add(anAnswer, text, status, quality);

      lFreeList(tmpAnswer);
   }

   DEXIT;
   return res;
}

/****** sge_userprj_qmaster/sge_userprj_spool() ********************************
*  NAME
*     sge_userprj_spool() -- updates the spooled user and projects
*
*  SYNOPSIS
*     void sge_userprj_spool(void) 
*
*  FUNCTION
*     The usage is only stored every 2 min. To have the acual usage stored when
*     the qmaster is going down, we have to through all user/projects and store
*     them again.
*
*  NOTES
*     MT-NOTE: sge_userprj_spool() is not MT safe, because it is working on global
*              master lists (only reading)
*
*******************************************************************************/
void sge_userprj_spool(void) {
   lListElem *elem = NULL;
   lList *answer_list = NULL;
   const char *name = NULL;
   u_long32 now = sge_get_gmt();

   DENTER(TOP_LAYER, "sge_userprj_spool");

   SGE_LOCK(LOCK_GLOBAL, LOCK_READ);
   

   for_each(elem, Master_User_List) {
      name = lGetString(elem, UP_name);
      sge_event_spool(&answer_list, now, 0, 0, 0, name, NULL, NULL,
                      elem, NULL, NULL, false, true);
   }

   for_each(elem, Master_Project_List) {
      name = lGetString(elem, UP_name);
      sge_event_spool(&answer_list, now, 0, 0, 0, name, NULL, NULL,
                      elem, NULL, NULL, false, true);   
   }

   SGE_UNLOCK(LOCK_GLOBAL, LOCK_READ);
   
   answer_list_output(&answer_list);
   
   DEXIT;
   return;
}

