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

#include "sge_limit_rule_qmaster.h"
#include "msg_common.h"
#include "msg_qmaster.h"
#include "msg_sgeobjlib.h"
#include "sge_persistence_qmaster.h"
#include "sge_utility_qmaster.h"
#include "sge.h"
#include "uti/sge_log.h"
#include "rmon/sgermon.h"
#include "sgeobj/sge_limit_rule.h"
#include "sgeobj/sge_answer.h"
#include "sgeobj/sge_utility.h"
#include "sgeobj/sge_job.h"
   #include "sgeobj/sge_ja_task.h"
#include "spool/sge_spooling.h"
#include "evm/sge_event_master.h"

static bool
limit_rule_reinit_consumable_actual_list(lListElem *lirs, lList **answer_list);

/****** sge_limit_rule_qmaster/lirs_mod() **************************************
*  NAME
*     lirs_mod() -- gdi callback function for modifing limitation rule sets
*
*  SYNOPSIS
*     int lirs_mod(lList **alpp, lListElem *new_lirs, lListElem *lirs, int add, 
*     const char *ruser, const char *rhost, gdi_object_t *object, int 
*     sub_command, monitoring_t *monitor) 
*
*  FUNCTION
*     This function will be called from the framework which will
*     add/modify/delete generic gdi objects.
*     The purpose of this function is it to add new lirs 
*     objects or modify existing limitation rule sets. 
*
*  INPUTS
*     lList **alpp          - referenct to an answer list
*     lListElem *new_lirs   - if a new lirs object will be created by this
*                             function, then new_lirs is a newly initialized
*                             CULL object.
*                             if this function was called due to a modify request
*                             than new_lirs will contain the old data
*     lListElem *lirs       - a reduced lirs object which contails all
*                             necessary information to create a new object
*                             or modify parts of an existing one
*     int add               - 1 if a new element should be added to the master list
*                             0 to modify an existing object
*     const char *ruser     - username who invoked this gdi request
*     const char *rhost     - hostname of where the gdi request was invoked
*     gdi_object_t *object  - structure of the gdi framework which contains
*                             additional information to perform the request
*     int sub_command       - how should we handle sublist elements
*              SGE_GDI_CHANGE - modify sublist elements
*              SGE_GDI_APPEND - add elements to a sublist
*              SGE_GDI_REMOVE - remove sublist elements
*              SGE_GDI_SET - replace the complete sublist                        
*     monitoring_t *monitor - monitoring structure
*
*  RESULT
*     int - 0 on success
*           STATUS_EUNKNOWN if an error occured
*
*  NOTES
*     MT-NOTE: sge_del_limit_rule_set() is MT safe 
*******************************************************************************/
int lirs_mod(lList **alpp, lListElem *new_lirs, lListElem *lirs, int add, const char *ruser, 
           const char *rhost, gdi_object_t *object, int sub_command, monitoring_t *monitor)
{
   const char *lirs_name = NULL; 
   bool rules_changed = false;

   DENTER(TOP_LAYER, "lirs_mod");

   /* ---- LIRS_name */
   if (add) {
      if (attr_mod_str(alpp, lirs, new_lirs, LIRS_name, object->object_name))
         goto ERROR;
   }
   lirs_name = lGetString(new_lirs, LIRS_name);

   /* Name has to be a valid name */
   if (add && verify_str_key(alpp, lirs_name, MAX_VERIFY_STRING, MSG_OBJ_LIRS) != STATUS_OK) {
      DRETURN(STATUS_EUNKNOWN);
   }

   /* ---- LIRS_description */
   attr_mod_str(alpp, lirs, new_lirs, LIRS_description, "description");

   /* ---- LIRS_enabled */
   attr_mod_bool(lirs, new_lirs, LIRS_enabled, "enabled");

   /* ---- LIRS_rule */
   if (lGetPosViaElem(lirs, LIRS_rule, SGE_NO_ABORT)>=0) {
      rules_changed = true;
      if (sub_command == SGE_GDI_SET_ALL) {
         normalize_sublist(lirs, LIRS_rule);
         attr_mod_sub_list(alpp, new_lirs, LIRS_rule,
            LIR_name, lirs, sub_command, SGE_ATTR_LIMITRULES, SGE_OBJ_LIRS, 0);    
      } else {
         /* *attr cases */
         lList *rule_list = lGetList(lirs, LIRS_rule);
         lListElem *rule = NULL;

         for_each(rule, rule_list) {
            lList *new_rule_list = lGetList(new_lirs, LIRS_rule);
            lListElem *new_rule = NULL;

            new_rule = limit_rule_locate(new_rule_list, lGetString(rule, LIR_name));
            if (new_rule != NULL) {
               /* ---- LIR_limit */
               attr_mod_sub_list(alpp, new_rule, LIR_limit, LIRL_name, rule, sub_command, SGE_ATTR_LIMITRULES, SGE_OBJ_LIRS, 0);
            } else {
               ERROR((SGE_EVENT, MSG_OBJECT_VALUEMISSING));
               answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC,
                               ANSWER_QUALITY_ERROR);
                
            }
         }
      }
   }
   if (!limit_rule_set_verify_attributes(new_lirs, alpp, true)) {
      DRETURN(STATUS_EUNKNOWN);
   }
   if (rules_changed && lGetBool(new_lirs, LIRS_enabled) == true) {
      limit_rule_reinit_consumable_actual_list(new_lirs, alpp);
   }
  
   DRETURN(0);

ERROR:
   DRETURN(STATUS_EUNKNOWN);
}

/****** sge_limit_rule_qmaster/lirs_spool() ************************************
*  NAME
*     lirs_spool() -- gdi callback funktion to spool a lirs object
*
*  SYNOPSIS
*     int lirs_spool(lList **alpp, lListElem *ep, gdi_object_t *object) 
*
*  FUNCTION
*     This function will be called from the framework which will
*     add/modify/delete generic gdi objects.
*     After an object was modified/added successfully it
*     is necessary to spool the current state to the filesystem.
*
*  INPUTS
*     lList **alpp         - reference to an answer list.
*     lListElem *ep       - lirs object which should be spooled
*     gdi_object_t *object - structure of the gdi framework which contains 
*                            additional information to perform the request
*                            (function pointers, names, CULL-types)
*
*  RESULT
*     [alpp] - error messages will be added to this list
*     0 - success
*     STATUS_EEXIST - an error occured
*
*  NOTES
*     MT-NOTE: sge_del_limit_rule_set() is MT safe 
*******************************************************************************/
int lirs_spool(lList **alpp, lListElem *ep, gdi_object_t *object)
{
   lList *answer_list = NULL;
   bool dbret;

   DENTER(TOP_LAYER, "lirs_spool");

   dbret = spool_write_object(&answer_list, spool_get_default_context(), ep, 
                              lGetString(ep, LIRS_name), SGE_TYPE_LIRS);
   answer_list_output(&answer_list);

   if (!dbret) {
      answer_list_add_sprintf(alpp, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_PERSISTENCE_WRITE_FAILED_S,
                              lGetString(ep, LIRS_name));
   }

   DEXIT;
   return dbret ? 0 : 1;
}

/****** sge_limit_rule_qmaster/lirs_success() **********************************
*  NAME
*     lirs_success() -- does something after an successfull modify
*
*  SYNOPSIS
*     int lirs_success(lListElem *ep, lListElem *old_ep, gdi_object_t *object, 
*     lList **ppList, monitoring_t *monitor) 
*
*  FUNCTION
*     This function will be called from the framework which will
*     add/modify/delete generic gdi objects.
*     After an object was modified/added and spooled successfully 
*     it is possibly necessary to perform additional tasks.
*     For example it is necessary to send some events to
+     other deamon.
*
*  INPUTS
*     lListElem *ep         - new lirs object
*     lListElem *old_ep     - old lirs object before modification or
*                             NULL if a new object was added
*     gdi_object_t *object  - structure of the gdi framework which contains 
*                             additional information to perform the request
*                             (function pointers, names, CULL-types) 
*     lList **ppList        - ??? 
*     monitoring_t *monitor - monitoring structure
*
*  RESULT
*     int - 0 success
*
*  NOTES
*     MT-NOTE: sge_del_limit_rule_set() is MT safe 
*******************************************************************************/
int lirs_success(lListElem *ep, lListElem *old_ep, gdi_object_t *object, lList **ppList, monitoring_t *monitor)
{
   const char *lirs_name;

   DENTER(TOP_LAYER, "lirs_success");

   lirs_name = lGetString(ep, LIRS_name);

   sge_add_event( 0, old_ep?sgeE_LIRS_MOD:sgeE_LIRS_ADD, 0, 0, 
                 lirs_name, NULL, NULL, ep);
   lListElem_clear_changed_info(ep);

   DRETURN(0);
}

/****** sge_limit_rule_qmaster/sge_del_limit_rule_set() ************************
*  NAME
*     sge_del_limit_rule_set() -- delete lirs object in Master_LIRS_List
*
*  SYNOPSIS
*     int sge_del_limit_rule_set(lListElem *ep, lList **alpp, lList 
*     **lirs_list, char *ruser, char *rhost) 
*
*  FUNCTION
*     This function will be called from the framework which will
*     add/modify/delete generic gdi objects.
*     The purpose of this function is it to delete ckpt objects. 
*
*  INPUTS
*     lListElem *ep     - element which should be deleted
*     lList **alpp      - reference to an answer list.
*     lList **lirs_list - reference to the Master_LIRS_LIST
*     char *ruser       - username of person who invoked this gdi request
*     char *rhost       - hostname of the host where someone initiated an gdi call
*
*  RESULT
*     0 - success
*     STATUS_EUNKNOWN - an error occured
*
*  NOTES
*     MT-NOTE: sge_del_limit_rule_set() is MT safe 
*******************************************************************************/
int sge_del_limit_rule_set(lListElem *ep, lList **alpp, lList **lirs_list, 
                    char *ruser, char *rhost)
{
   const char *lirs_name;
   int pos;
   lListElem *found;

   DENTER(TOP_LAYER, "sge_del_limit_rule_set");

   if ( !ep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* ep is no LIRS element, if ep has no LIRS_name */
   if ((pos = lGetPosViaElem(ep, LIRS_name, SGE_NO_ABORT)) < 0) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(LIRS_name), SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   lirs_name = lGetPosString(ep, pos);
   if (!lirs_name) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* search for lirs with this name and remove it from the list */
   if (!(found = limit_rule_set_list_locate(*lirs_list, lirs_name))) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, MSG_OBJ_LIRS, lirs_name));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   lRemoveElem(*lirs_list, &found);

   sge_event_spool(alpp, 0, sgeE_LIRS_DEL, 
                   0, 0, lirs_name, NULL, NULL,
                   NULL, NULL, NULL, true, true);

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS,
            ruser, rhost, lirs_name, MSG_OBJ_LIRS));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);

   DRETURN(STATUS_OK);
}

/****** sge_limit_rule_qmaster/limit_rule_reinit_consumable_actual_list() **************
*  NAME
*     limit_rule_reinit_consumable_actual_list() -- debit running jobs
*
*  SYNOPSIS
*     static bool limit_rule_reinit_consumable_actual_list(lListElem *lirs, lList 
*     **answer_list) 
*
*  FUNCTION
*     Newly added limitation rule sets need to be debited for all running jos
*     This is done by this function
*
*  INPUTS
*     lListElem *lirs     - limitation rule set (LIRS_Type)
*     lList **answer_list - answer list
*
*  RESULT
*     bool - always true
*
*  NOTES
*     MT-NOTE: limit_rule_reinit_consumable_actual_list() is not MT safe 
*
*******************************************************************************/
static bool
limit_rule_reinit_consumable_actual_list(lListElem *lirs, lList **answer_list) {
   bool ret = true;
   lList *master_centry_list = *(object_type_get_master_list(SGE_TYPE_CENTRY));
   lList *master_userset_list = *(object_type_get_master_list(SGE_TYPE_USERSET));
   lList *master_hgroup_list = *(object_type_get_master_list(SGE_TYPE_HGROUP));

   DENTER(TOP_LAYER, "limit_rule_reinit_consumable_actual_list");

   if (lirs != NULL) {
      lListElem *job;
      lList *job_list = *(object_type_get_master_list(SGE_TYPE_JOB));
      lListElem * rule = NULL;

      for_each(rule, lGetList(lirs, LIRS_rule)) {
         lListElem *limit = NULL;
         for_each(limit, lGetList(rule, LIR_limit)) {
            lList *usage = lGetList(limit, LIRL_usage);
            lFreeList(&usage);
         }
      }

      for_each(job, job_list) {
         lListElem *ja_task = NULL;
         lList *ja_task_list = lGetList(job, JB_ja_tasks);

         for_each(ja_task, ja_task_list) {
            lListElem *granted = NULL;
            lList *gdi_list = lGetList(ja_task, JAT_granted_destin_identifier_list);

            for_each(granted, gdi_list) {
               int tmp_slot = lGetUlong(granted, JG_slots);
               lirs_debit_consumable(lirs, job, granted, lGetString(ja_task, JAT_granted_pe), master_centry_list,
                                     master_userset_list, master_hgroup_list, tmp_slot);
            }
         }
      }
   }

   DRETURN(ret);
}
