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
#include <string.h>
#include <sys/types.h>

#include "sge.h"
#include "sge_max_nis_retries.h"
#include "sge_log.h"
#include "sgermon.h"
#include "sge_jobL.h"
#include "sge_queueL.h"
#include "sge_hostL.h"
#include "sge_userprjL.h"
#include "sge_peL.h"
#include "sge_eventL.h"
#include "sge_answerL.h"
#include "sge_usersetL.h"
#include "sge_queue_qmaster.h"
#include "sge_m_event.h"
#include "sge_userset.h"
#include "read_write_userset.h"
#include "read_write_queue.h"
#include "sge_userset_qmaster.h"
#include "sge_feature.h"
#include "utility.h"
#include "sge_conf.h"
#include "gdi_utility_qmaster.h"
#include "valid_queue_user.h"
#include "msg_utilib.h"
#include "msg_common.h"
#include "msg_qmaster.h"

extern lList *Master_Queue_List;
extern lList *Master_Project_List;
extern lList *Master_Userset_List;
extern lList *Master_Pe_List;

extern lList *Master_Userset_List;
extern lList *Master_Pe_List;
extern lList *Master_Exechost_List;

static void sge_change_queue_version_acl(const char *acl_name);
static lList* do_depts_conflict(lListElem *new, lListElem *old);
static int verify_userset_deletion(lList **alpp, const char *userset_name);
static int dept_is_valid_defaultdepartment(lListElem *dept, lList **answer_list);
static int acl_is_valid_acl(lListElem *acl, lList **answer_list);

/*********************************************************************
   sge_add_userset() - Master code
   
   adds an userset list to the global userset_list
 *********************************************************************/
int sge_add_userset(
lListElem *ep,
lList **alpp,
lList **userset_list,
char *ruser,
char *rhost 
) {
   const char *userset_name;
   int pos, ret;
   lListElem *found;
   char fname[SGE_PATH_MAX], real_fname[SGE_PATH_MAX];

   DENTER(TOP_LAYER, "sge_add_acl");

   if ( !ep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* ep is no acl element, if ep has no US_name */
   if ((pos = lGetPosViaElem(ep, US_name)) < 0) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(US_name), SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   userset_name = lGetPosString(ep, pos);
   if (!userset_name) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* Name has to be a valid filename without pathchanges, because we use it
      for storing user/project to disk */
   if (verify_str_key(alpp, userset_name, MSG_OBJ_USERSET)) {
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* search for userset with this name */
   found = sge_locate_userset(userset_name, *userset_list);

   /* no double entries */
   if (found) {
      ERROR((SGE_EVENT, MSG_SGETEXT_ALREADYEXISTS_SS, MSG_OBJ_USERSET, userset_name));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* ensure userset is at least an ACL (qconf -au ) */
   if (!lGetUlong(ep, US_type)) 
      lSetUlong(ep, US_type, US_ACL);

   /* interpret user/group names */
   ret=sge_verify_userset_entries(lGetList(ep, US_entries), alpp, 0);
   if ( ret != STATUS_OK ) {
      DEXIT;
      return ret;
   }

   /* only in sge/budget mode needed */
   if (feature_is_enabled(FEATURE_SGEEE)) {
      /*
      ** check for users defined in more than one userset if they
      ** are used as departments
      */
      ret = sge_verify_department_entries(*userset_list, ep, alpp);
      if (ret!=STATUS_OK) {
         DEXIT;
         return ret;
      }
   }

   /* update on file */
   sprintf(fname , "%s/.%s", USERSET_DIR, userset_name);
   sprintf(real_fname , "%s/%s", USERSET_DIR, userset_name);
   if ((ret= write_userset(alpp, ep, fname, 0, 1))) {
      /* answer list gets filled in write_userset() */
      DEXIT;
      return ret;
   } else {
      if (rename(fname, real_fname) == -1) {
         DEXIT;
         return 1;
      } else {
         strcpy(fname, real_fname);
      }      
   }
   /* change queue versions */
   /* isn't this unnecessary? since the userset hast just been created
    * it CANNOT by used by any queue in its (x)acl.  (Archie)
    */
   sge_change_queue_version_acl(userset_name);

   /* update in interal lists */
   if (!*userset_list)
      *userset_list = lCreateList("global userset list", US_Type);
   lAppendElem(*userset_list, lCopyElem(ep));

   /* it's better to send the event only when the internal lists have
    * been updated. The CORBA part relies on this fact...
    */
   sge_add_event(NULL, sgeE_USERSET_ADD, 0, 0, userset_name, ep);

   INFO((SGE_EVENT, MSG_SGETEXT_ADDEDTOLIST_SSSS,
            ruser, rhost, userset_name, MSG_OBJ_USERSET));
   sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   DEXIT;
   return STATUS_OK;
}

/******************************************************************
   sge_del_userset() - Qmaster code

   deletes an userset list from the global userset_list
 ******************************************************************/
int sge_del_userset(
lListElem *ep,
lList **alpp,
lList **userset_list,
char *ruser,
char *rhost 
) {
   lListElem *found;
   int pos, ret;
   const char *userset_name;

   DENTER(TOP_LAYER, "sge_del_userset");

   if ( !ep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* ep is no userset element, if ep has no US_name */
   if ((pos = lGetPosViaElem(ep, US_name)) < 0) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(US_name), SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   userset_name = lGetPosString(ep, pos);
   if (!userset_name) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* search for userset with this name and remove it from the list */
   if (!(found = sge_locate_userset(userset_name, *userset_list))) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, MSG_OBJ_USERSET, userset_name));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* ensure there are no (x)acl lists in 
      a queue/pe/project/.. refering to this userset */   
   if ((ret=verify_userset_deletion(alpp, userset_name))!=STATUS_OK) {
      /* answerlist gets filled by verify_userset_deletion() in case of errors */
      DEXIT;
      return ret;
   }

   lFreeElem(lDechainElem(*userset_list, found));
   /* remove userset file */
   sge_unlink(USERSET_DIR, userset_name);
   sge_add_event(NULL, sgeE_USERSET_DEL, 0, 0, userset_name, NULL);

   /* change queue versions */
   sge_change_queue_version_acl(userset_name);

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS,
            ruser, rhost, userset_name, MSG_OBJ_USERSET));
   sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   DEXIT;
   return STATUS_OK;
}

/**************************************************************
   sge_mod_userset() - Qmaster code

   modifies an userset in the global list
 **************************************************************/
int sge_mod_userset(
lListElem *ep,
lList **alpp,
lList **userset_list,
char *ruser,
char *rhost 
) {
   const char *userset_name;
   int pos, ret;
   lListElem *found;
   char fname[SGE_PATH_MAX], real_fname[SGE_PATH_MAX];

   DENTER(TOP_LAYER, "sge_mod_userset");

   if ( !ep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* ep is no userset element, if ep has no US_name */
   if ((pos = lGetPosViaElem(ep, US_name)) < 0) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(US_name), SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   userset_name = lGetPosString(ep, pos);
   if (!userset_name) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* no double entries */
   found = sge_locate_userset(userset_name, *userset_list);
   if (!found) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, MSG_OBJ_USERSET, 
             userset_name));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* interpret user/group names */
   ret=sge_verify_userset_entries(lGetList(ep, US_entries), alpp, 0);
   if (ret!=STATUS_OK) {
      DEXIT;
      return ret;
   }

   if (feature_is_enabled(FEATURE_SGEEE)) {
      /* make sure acl is valid */
      ret = acl_is_valid_acl(ep, alpp);
      if (ret != STATUS_OK) {
         DEXIT;
         return ret;
      }

      /*
      ** check for users defined in more than one userset if they
      ** are used as departments
      */
      ret = sge_verify_department_entries(*userset_list, ep, alpp);
      if (ret!=STATUS_OK) {
         DEXIT;
         return ret;
      }
   }

   /* delete old userset */
   lRemoveElem(*userset_list, found);

   /* insert modified userset */
   lAppendElem(*userset_list, lCopyElem(ep));

   /* update on file */
   sprintf(fname, "%s/.%s", USERSET_DIR, userset_name);
   sprintf(real_fname, "%s/%s", USERSET_DIR, userset_name);
   if (write_userset(alpp, ep, fname, NULL, 1)) {
      DEXIT;
      return STATUS_EDISK;
   } else {
      if (rename(fname, real_fname) == -1) {
         DEXIT;
         return 1;
      } else {
         strcpy(fname, real_fname);
      }    
   }
   sge_add_event(NULL, sgeE_USERSET_MOD, 0, 0, userset_name, ep);

   /* change queue versions */
   sge_change_queue_version_acl(userset_name);

   INFO((SGE_EVENT, MSG_SGETEXT_MODIFIEDINLIST_SSSS,
            ruser, rhost, userset_name, MSG_OBJ_USERSET));
   sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   DEXIT;
   return STATUS_OK;
}


/*********************************************************************
   Qmaster code

   increase version of queues using given userset as access list (acl or xacl)

   having changed a complex we have to increase the versions
   of all queues containing this complex;
 **********************************************************************/
static void sge_change_queue_version_acl(
const char *acl_name 
) {
   lListElem *qep;

   DENTER(TOP_LAYER, "sge_change_queue_version");

   for_each(qep, Master_Queue_List) {
      if (lGetElemStr(lGetList(qep, QU_acl), US_name, acl_name) ||
          lGetElemStr(lGetList(qep, QU_xacl), US_name, acl_name)) {
         sge_change_queue_version(qep, 0, 0);
         cull_write_qconf(1, 0, QUEUE_DIR, lGetString(qep, QU_qname), NULL, qep);
         DPRINTF(("increasing version of queue "SFQ" because acl "
                       SFQ" changed\n", lGetString(qep, QU_qname), acl_name));
      }
   }

   DEXIT;
   return;
}


/***********************************************************
   sge_locate_userset() - searches an userset
 ***********************************************************/
lListElem *sge_locate_userset(
const char *name,
lList *lp 
) {
   lListElem *ep;

   DENTER(TOP_LAYER, "sge_locate_userset");

   for_each(ep, lp)
      if (!strcasecmp(name, lGetString(ep, US_name))) {
         DEXIT;
         return ep;
      }

   DEXIT;
   return NULL;
}

/******************************************************
   sge_verify_userset_entries()
      resolves user set/department

   usep
      cull list (UE_Type)
   alpp
      may be NULL
      is used to build up an answer
      element in case of error

   returns
      STATUS_OK         - on success
      STATUS_ESEMANTIC  - on error
 ******************************************************/
int sge_verify_userset_entries(
lList *userset_entries,
lList **alpp,
int start_up 
) {
   lListElem *ep;
   int name_pos;

   DENTER(TOP_LAYER, "sge_verify_userset_entries");

   /*
      resolve cull names to positions
      for faster access in loop
   */
   name_pos = lGetPosInDescr(UE_Type, UE_name);

   for_each(ep, userset_entries)
      if (!lGetPosString(ep, name_pos)) {
         ERROR((SGE_EVENT, MSG_US_INVALIDUSERNAME));
         sge_add_answer(alpp, SGE_EVENT, STATUS_ESEMANTIC, 0);
         DEXIT;
         return STATUS_ESEMANTIC;
      }

   DEXIT;
   return STATUS_OK;
}

/******************************************************
   sge_verify_department_entries()
      resolves user set/department

   userset_list
      the current master user list (US_Type)
   new_userset
      the new userset element
   alpp
      may be NULL
      is used to build up an answer
      element in case of error

   returns
      STATUS_OK         - on success
      STATUS_ESEMANTIC  - on error
 ******************************************************/
int sge_verify_department_entries(
lList *userset_list,
lListElem *new_userset,
lList **alpp 
) {
   lListElem *up;
   lList *depts;
   lList *answers = NULL;
   lCondition *where = NULL;
   lEnumeration *what = NULL;

   DENTER(TOP_LAYER, "sge_verify_department_entries");

   /*
    * make tests for the defaultdepartment
    */
   if (!strcmp(lGetString(new_userset, US_name), DEFAULT_DEPARTMENT)) {
      if (!dept_is_valid_defaultdepartment(new_userset, alpp)) {
         DEXIT;
         return STATUS_ESEMANTIC;
      }
   }

   if (!(lGetUlong(new_userset, US_type) & US_DEPT)) {
      DEXIT;
      return STATUS_OK;
   }
   
   /*
   ** get the department usersets and only those that have a different
   ** name than the new one.
   */
   what = lWhat("%T(ALL)", US_Type);
   where = lWhere("%T(%I m= %u && %I != %s)", US_Type, US_type, US_DEPT,
                     US_name, lGetString(new_userset, US_name));
   depts = lSelect("Departments", userset_list, where, what);
   lFreeWhere(where);
   lFreeWhat(what);

   if (!depts) {
      DEXIT;
      return STATUS_OK;
   }

   /*
   ** Loop over departments and check if a user in the new
   ** element is already contained in another department list.
   ** This requires expanding the group entries.
   */
   for_each(up, depts) {
      answers = do_depts_conflict(new_userset, up); 
      if (answers)
         break;
   }
   
   lFreeList(depts);
   
   if (answers) {
      *alpp = answers;
      DEXIT;
      return STATUS_ESEMANTIC;
   }

   DEXIT;
   return STATUS_OK;
}

static int dept_is_valid_defaultdepartment(lListElem *dept, 
                                           lList **answer_list)
{
   int ret = 1;
   DENTER(TOP_LAYER, "dept_is_valid_defaultdepartment");

   if (dept != NULL) {
      /* test 'type' */ 
      if (!(lGetUlong(dept, US_type) & US_DEPT)) {
         ERROR((SGE_EVENT, MSG_QMASTER_DEPTFORDEFDEPARTMENT));
         sge_add_answer(answer_list, SGE_EVENT, STATUS_ESEMANTIC, 0);
         ret = 0;
      }
      /* test user list */
      if (lGetNumberOfElem(lGetList(dept, US_entries)) > 0 ) {
         ERROR((SGE_EVENT, MSG_QMASTER_AUTODEFDEPARTMENT));
         sge_add_answer(answer_list, SGE_EVENT, STATUS_ESEMANTIC, 0);
         ret = 0;
      }
   } 
   DEXIT;
   return ret;
}

static int acl_is_valid_acl(lListElem *acl,
                            lList **answer_list) 
{
   int ret = 1;
   DENTER(TOP_LAYER, "acl_is_valid_acl"); 
   
   if (acl != NULL) {
      if (!(lGetUlong(acl, US_type) & US_DEPT)) {
         if (lGetUlong(acl, US_fshare) > 0) {
            ERROR((SGE_EVENT, MSG_QMASTER_ACLNOSHARE));
            sge_add_answer(answer_list, SGE_EVENT, STATUS_ESEMANTIC, 0);
            ret = 0;
         }
         if (lGetUlong(acl, US_oticket) > 0) {
            ERROR((SGE_EVENT, MSG_QMASTER_ACLNOTICKET));
            sge_add_answer(answer_list, SGE_EVENT, STATUS_ESEMANTIC, 0);
            ret = 0;
         } 
      }
   }
   DEXIT;
   return ret;
}

static lList* do_depts_conflict(
lListElem *new,
lListElem *old 
) {
   lList *new_users = NULL;
   lList *old_users = NULL; 
   lListElem *np;
   lList *alp = NULL;
   const char *nname;
   
   DENTER(TOP_LAYER, "do_depts_conflict");
   
   new_users = lGetList(new, US_entries);
   old_users = lGetList(old, US_entries);

   if (!old_users || !new_users) {
      DEXIT;
      return NULL;
   }
   
   /*
   ** groups are encoded with the first letter @, e.g. @sge
   */
   for_each(np, new_users) {
      nname = lGetString(np, UE_name);
      if (nname && nname[0] == '@') { 
         if (sge_contained_in_access_list(NULL, &nname[1], old, &alp)) {
            DEXIT;
            return alp;
         }
      }   
      else {
         if (sge_contained_in_access_list(nname, NULL, old, &alp)) {
            DEXIT;
            return alp;
         }
      }   
   }      

   DEXIT;
   return NULL;
}

/* 

   return
      0   no matching department found
      1   set department
*/
int set_department(
lList **alpp,
lListElem *job,
lList *userset_list 
) {
   lListElem *dep;
   const char *owner, *group; 

   DENTER(TOP_LAYER, "set_department");

   /* first try to find a department searching the user name directly
      in a department */
   owner = lGetString(job, JB_owner);
   for_each (dep, userset_list) {
      /* use only departments */
      if (!(lGetUlong(dep, US_type) & US_DEPT)) 
         continue;

      if (sge_contained_in_access_list(owner, NULL, dep, NULL)) {
         lSetString(job, JB_department, lGetString(dep, US_name));   
         DPRINTF(("user %s got department "SFQ"\n", 
            owner, lGetString(dep, US_name)));

         DEXIT;
         return 1;
      }
   }

   /* the user does not appear in any department - now try to find
      our group in the department */
   group = lGetString(job, JB_group);
   for_each (dep, userset_list) {
      /* use only departments */ 
      if (!(lGetUlong(dep, US_type) & US_DEPT))
         continue;

      if (sge_contained_in_access_list(NULL, group, dep, NULL)) {
         lSetString(job, JB_department, lGetString(dep, US_name));   
         DPRINTF(("user %s got department \"%s\"\n", owner, lGetString(dep, US_name)));

         DEXIT;
         return 1;
      }
   }   
   

   /*
   ** attach default department if present
   ** if job has no department we reach this
   */
   if(lGetElemStr(userset_list, US_name, DEFAULT_DEPARTMENT)) {
      lSetString(job, JB_department, DEFAULT_DEPARTMENT);
      DPRINTF(("user %s got department "SFQ"\n", owner, DEFAULT_DEPARTMENT));
      DEXIT;
      return 1;
   }
   
   ERROR((SGE_EVENT, MSG_SGETEXT_NO_DEPARTMENT4USER_SS, owner, group));
   sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);

   DEXIT;
   return 0;
}

int verify_acl_list(
lList **alpp,
lList *acl_list,
const char *attr_name, /* e.g. "user_lists" */
const char *obj_descr, /* e.g. "queue"      */
const char *obj_name   /* e.g. "fangorn.q"  */
) {
   lListElem *usp;

   DENTER(TOP_LAYER, "verify_acl_list");

   for_each (usp, acl_list) {
      if (!lGetElemStr(Master_Userset_List, US_name, lGetString(usp, US_name))) {
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWNUSERSET_SSSS, lGetString(usp, US_name), 
               attr_name, obj_descr, obj_name));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }

   DEXIT;
   return STATUS_OK;
}


static int verify_userset_deletion(
lList **alpp,
const char *userset_name 
) {
   int ret = STATUS_OK;
   lListElem *ep;

   DENTER(TOP_LAYER, "verify_userset_deletion");

   for_each (ep, Master_Queue_List) {
      if (lGetElemStr(lGetList(ep, QU_acl), US_name, userset_name)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_USERSETSTILLREFERENCED_SSSS, userset_name, 
               MSG_OBJ_USERLIST, MSG_OBJ_QUEUE, lGetString(ep, QU_qname)));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         ret = STATUS_EUNKNOWN;
      }
      if (lGetElemStr(lGetList(ep, QU_xacl), US_name, userset_name)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_USERSETSTILLREFERENCED_SSSS, userset_name, 
               MSG_OBJ_XUSERLIST, MSG_OBJ_QUEUE, lGetString(ep, QU_qname)));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         ret = STATUS_EUNKNOWN;
      }

   }

   for_each (ep, Master_Pe_List) {
      if (lGetElemStr(lGetList(ep, PE_user_list), US_name, userset_name)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_USERSETSTILLREFERENCED_SSSS, userset_name, 
               MSG_OBJ_USERLIST, MSG_OBJ_PE, lGetString(ep, PE_name)));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         ret = STATUS_EUNKNOWN;
      }
      if (lGetElemStr(lGetList(ep, PE_xuser_list), US_name, userset_name)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_USERSETSTILLREFERENCED_SSSS, userset_name, 
               MSG_OBJ_XUSERLIST, MSG_OBJ_PE, lGetString(ep, PE_name)));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         ret = STATUS_EUNKNOWN;
      }
   }

   if (feature_is_enabled(FEATURE_SGEEE)) {
      for_each (ep, Master_Project_List) {
         if (lGetElemStr(lGetList(ep, UP_acl), US_name, userset_name)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_USERSETSTILLREFERENCED_SSSS, userset_name, 
                  MSG_OBJ_USERLIST, MSG_OBJ_PRJ, lGetString(ep, UP_name)));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            ret = STATUS_EUNKNOWN;
         }
         if (lGetElemStr(lGetList(ep, UP_xacl), US_name, userset_name)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_USERSETSTILLREFERENCED_SSSS, userset_name, 
                  MSG_OBJ_XUSERLIST, MSG_OBJ_PRJ, lGetString(ep, UP_name)));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            ret = STATUS_EUNKNOWN;
         }
      }
   }

   /* hosts */
   for_each (ep, Master_Exechost_List) {
      if (lGetElemStr(lGetList(ep, EH_acl), US_name, userset_name)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_USERSETSTILLREFERENCED_SSSS, userset_name,
               MSG_OBJ_USERLIST, MSG_OBJ_EH, lGetHost(ep, EH_name)));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EEXIST;
      }
      if (lGetElemStr(lGetList(ep, EH_xacl), US_name, userset_name)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_USERSETSTILLREFERENCED_SSSS, userset_name,
               MSG_OBJ_XUSERLIST, MSG_OBJ_EH, lGetHost(ep, EH_name)));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EEXIST;
      }
   }

   /* global configuration */
   if (lGetElemStr(conf.user_lists, US_name, userset_name)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_USERSETSTILLREFERENCED_SSSS, userset_name, 
            MSG_OBJ_USERLIST, MSG_OBJ_CONF, MSG_OBJ_GLOBAL));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      ret = STATUS_EUNKNOWN;
   }

   if (lGetElemStr(conf.xuser_lists, US_name, userset_name)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_USERSETSTILLREFERENCED_SSSS, userset_name, 
            MSG_OBJ_XUSERLIST, MSG_OBJ_CONF, MSG_OBJ_GLOBAL));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      ret = STATUS_EUNKNOWN;
   }

   DEXIT;
   return ret;
}
