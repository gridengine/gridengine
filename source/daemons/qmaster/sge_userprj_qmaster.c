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
#include "sge_answerL.h"
#include "sge_conf.h"
#include "sge_hostL.h"
#include "sge_usageL.h"
#include "sge_userprjL.h"
#include "sge_usersetL.h"
#include "sge_queueL.h"
#include "sge_share_tree_nodeL.h"
#include "read_write_userprj.h"
#include "sge_userprj_qmaster.h"
#include "sge_userset_qmaster.h"
#include "sge_sharetree_qmaster.h"
#include "sge_m_event.h"
#include "cull_parse_util.h"
#include "config.h"
#include "sge_log.h"
#include "gdi_utility_qmaster.h"
#include "msg_common.h"
#include "msg_utilib.h"
#include "msg_qmaster.h"

extern lList *Master_Sharetree_List;
extern lList *Master_Project_List;
extern lList *Master_User_List;
extern lList *Master_Queue_List;
extern lList *Master_Exechost_List;
extern lList *Master_User_List;

int userprj_mod(
lList **alpp,
lListElem *modp,
lListElem *ep,
int add,
char *ruser,
char *rhost,
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

   DENTER(TOP_LAYER, "userprj_mod");
  
   obj_name = user_flag ? MSG_OBJ_USER : MSG_OBJ_PRJ;  

   /* ---- UP_name */
   userprj = lGetString(ep, UP_name);
   if (add) {
      if (!strcmp(userprj, "default")) {
         ERROR((SGE_EVENT, MSG_UP_NOADDDEFAULT_S, obj_name));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
         goto Error;
      }
      /* may not add user with same name as an existing project */
      if (lGetElemStr(user_flag?Master_Project_List:Master_User_List, 
                        UP_name, userprj)) {
         ERROR((SGE_EVENT, MSG_UP_ALREADYEXISTS_SS,user_flag ? MSG_OBJ_PRJ : MSG_OBJ_USER , userprj));

         sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
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
   }

   /* ---- UP_fshare */
   if ((pos=lGetPosViaElem(ep, UP_fshare))>=0) {
      uval = lGetPosUlong(ep, pos);
      lSetUlong(modp, UP_fshare, uval);
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
                !sge_locate_user_prj(dproj, Master_Project_List)) {
               ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, MSG_OBJ_PRJ, dproj));
               sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
               goto Error;
            }
         }
         lSetString(modp, UP_default_project, dproj);
      }
   }
   else {
      /* ---- UP_acl */
      if ((pos=lGetPosViaElem(ep, UP_acl))>=0) {
         lp = lGetPosList(ep, pos);
         lSetList(modp, UP_acl, lCopyList("acl", lp));

         if (verify_acl_list(alpp, lGetList(ep, UP_acl), "user_lists", 
               "project", lGetString(ep, UP_name))!=STATUS_OK) {
            /* answerlist gets filled by verify_acl_list() in case of errors */
            goto Error;
         }
      }

      /* ---- UP_xacl */
      if ((pos=lGetPosViaElem(ep, UP_xacl))>=0) {
         lp = lGetPosList(ep, pos);
         lSetList(modp, UP_xacl, lCopyList("xacl", lp));
         if (verify_acl_list(alpp, lGetList(ep, UP_xacl), "xuser_lists", 
               "project", lGetString(ep, UP_name))!=STATUS_OK) {
            /* answerlist gets filled by verify_acl_list() in case of errors */
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

int userprj_success(
lListElem *ep,
lListElem *old_ep,
gdi_object_t *object 
) {
   int user_flag = (object->target==SGE_USER_LIST)?1:0;
   
   DENTER(TOP_LAYER, "userprj_success");

   sge_add_event( NULL, old_ep?
         (user_flag?sgeE_USER_MOD:sgeE_PROJECT_MOD) :
         (user_flag?sgeE_USER_ADD:sgeE_PROJECT_ADD), 
         0, 0, lGetString(ep, UP_name), ep);

   DEXIT;
   return 0;
}

int userprj_spool(
lList **alpp,
lListElem *upe,
gdi_object_t *object 
) {
   char fname[1000];
   int user_flag = (object->target==SGE_USER_LIST)?1:0;

   DENTER(TOP_LAYER, "userprj_spool");

   /* write user or project to file */
   sprintf(fname , "%s/%s", 
      user_flag?USER_DIR : PROJECT_DIR, 
      lGetString(upe, object->key_nm));

   if (write_userprj(alpp, upe, fname, NULL, 1, user_flag)) {
      /* answer list gets filled in write_userprj() */
      DEXIT;
      return 1;
   }

   DEXIT;
   return 0;
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
   char fname[SGE_PATH_MAX];
   lListElem *ep;
   lListElem *myep;

   DENTER(TOP_LAYER, "sge_del_userprj");

   if ( !up_ep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   name = lGetString(up_ep, UP_name);

   if (!(ep=sge_locate_user_prj(name, *upl))) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, user? MSG_OBJ_USER : MSG_OBJ_PRJ, name));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* ensure this u/p object is not referenced in actual share tree */
   if (getNode(Master_Sharetree_List, name, user ? STT_USER : STT_PROJECT, 0)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANT_DELETE_UP_IN_SHARE_TREE_SS, user?MSG_OBJ_USER:MSG_OBJ_PRJ, name));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   if (user==0) { /* ensure this project is not referenced in any queue */
      lListElem *ep;

      /* check queues */
      for_each (ep, Master_Queue_List) {
         if (lGetElemStr(lGetList(ep, QU_projects), UP_name, name)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_PROJECTSTILLREFERENCED_SSSS, name, 
                  MSG_OBJ_PRJS, MSG_OBJ_QUEUE, lGetString(ep, QU_qname)));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EEXIST;
         }
         if (lGetElemStr(lGetList(ep, QU_xprojects), UP_name, name)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_PROJECTSTILLREFERENCED_SSSS, name, 
                  MSG_OBJ_XPRJS, MSG_OBJ_QUEUE, lGetString(ep, QU_qname)));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EEXIST;
         }
      }

      /* check hosts */
      for_each (ep, Master_Exechost_List) {
         if (lGetElemStr(lGetList(ep, EH_prj), UP_name, name)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_PROJECTSTILLREFERENCED_SSSS, name, 
                  MSG_OBJ_PRJS, MSG_OBJ_EH, lGetHost(ep, EH_name)));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EEXIST;
         }
         if (lGetElemStr(lGetList(ep, EH_xprj), UP_name, name)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_PROJECTSTILLREFERENCED_SSSS, name, 
                  MSG_OBJ_XPRJS, MSG_OBJ_EH, lGetHost(ep, EH_name)));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EEXIST;
         }
      }

      /* check global configuration */
      if (lGetElemStr(conf.projects, UP_name, name)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_PROJECTSTILLREFERENCED_SSSS, name, 
               MSG_OBJ_PRJS, MSG_OBJ_CONF, MSG_OBJ_GLOBAL));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EEXIST;
      }
      if (lGetElemStr(conf.xprojects, UP_name, name)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_PROJECTSTILLREFERENCED_SSSS, name, 
               MSG_OBJ_XPRJS, MSG_OBJ_CONF, MSG_OBJ_GLOBAL));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EEXIST;
      }

      /* check user list for reference */
      if ((myep = lGetElemStr(Master_User_List, UP_default_project, name))) {
         ERROR((SGE_EVENT, MSG_USERPRJ_PRJXSTILLREFERENCEDINENTRYX_SS, name, lGetString (myep, UP_name)));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EEXIST;
      }

   }

   lRemoveElem(*upl, ep);

   /* delete user or project file */
   sprintf(fname , "%s/%s", user ? USER_DIR : PROJECT_DIR, name);
   if (unlink(fname)) {
      ERROR((SGE_EVENT, MSG_FILE_RM_S, fname));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
      DEXIT;
      return STATUS_EDISK;
   }

   sge_add_event( NULL, user?sgeE_USER_DEL:sgeE_PROJECT_DEL, 0, 0, name, NULL);

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS,
         ruser, rhost, name, user?MSG_OBJ_USER:MSG_OBJ_PRJ));
   sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);

   DEXIT;
   return STATUS_OK;
}

lListElem *sge_locate_user_prj(
const char *up_name,
lList *upl 
) {
   return lGetElemStr(upl, UP_name, up_name);
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
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }

   DEXIT;
   return STATUS_OK;
}

