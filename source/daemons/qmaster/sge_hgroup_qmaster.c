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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "sge.h"
#include "sgermon.h"
#include "sge_conf.h"
#include "sge_log.h"
#include "sge_c_gdi.h"
#include "sge_str.h"
#include "sge_string.h"
#include "sge_utility.h"
#include "sge_cuser.h"
#include "sge_hgroup_qmaster.h"
#include "sge_answer.h"
#include "sge_unistd.h"
#include "sge_cqueue.h"
#include "sge_qinstance.h"
#include "sge_hostname.h"
#include "sge_hgroup.h"
#include "sge_href.h"
#include "sge_event_master.h"
#include "sge_cqueue_qmaster.h"

#include "sge_persistence_qmaster.h"
#include "spool/sge_spooling.h"

#include "msg_common.h"
#include "msg_qmaster.h"

static bool
hgroup_mod_hostlist(lListElem *hgroup, lList **answer_list,
                    lListElem *reduced_elem, int sub_command,
                    lList **add_hosts, lList **rem_hosts,
                    lList **occupant_groups);

static void 
hgroup_commit(lListElem *hgroup);

static void 
hgroup_rollback(lListElem *this_elem);

static bool
hgroup_mod_hostlist(lListElem *hgroup, lList **answer_list,
                    lListElem *reduced_elem, int sub_command,
                    lList **add_hosts, lList **rem_hosts,
                    lList **occupant_groups)
{
   bool ret = true;

   DENTER(TOP_LAYER, "hgroup_mod_hostlist");
   if (hgroup != NULL && reduced_elem != NULL) {
      int pos = lGetPosViaElem(reduced_elem, HGRP_host_list);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);
         lList *old_href_list = lCopyList("", lGetList(hgroup, HGRP_host_list));
         lList *master_list = *(hgroup_list_get_master_list());
         lList *href_list = NULL;
         lList *add_groups = NULL;
         lList *rem_groups = NULL;

         if (ret) {
            ret &= href_list_resolve_hostnames(list, answer_list);
         }
         if (ret) {
            attr_mod_sub_list(answer_list, hgroup, HGRP_host_list, HR_name,
                              reduced_elem, sub_command, SGE_ATTR_HOSTLIST,
                              SGE_OBJ_HGROUP, 0);
            href_list = lGetList(hgroup, HGRP_host_list);
         }
         if (ret) {
            ret &= href_list_find_diff(href_list, answer_list, old_href_list,
                                       add_hosts, rem_hosts, &add_groups,
                                       &rem_groups);
         }
         if (ret && add_groups != NULL) {
            ret &= hgroup_list_exists(master_list, answer_list, add_groups);
         }
         if (ret) {
            ret &= href_list_find_effective_diff(answer_list, add_groups,
                                                 rem_groups, master_list,
                                                 add_hosts, rem_hosts);
         }


         /*
          * Try to find cycles in the definition
          */
         if (ret) {
            ret &= hgroup_find_all_referencees(hgroup, answer_list,
                                               master_list, occupant_groups);
            ret &= href_list_add(occupant_groups, answer_list,
                                 lGetHost(hgroup, HGRP_name));
            if (ret) {
               if (*occupant_groups != NULL && add_groups != NULL) {
                  lListElem *add_group = NULL;

                  for_each(add_group, add_groups) {
                     const char *name = lGetHost(add_group, HR_name);

                     if (href_list_has_member(*occupant_groups, name)) {
                        break;
                     }
                  }
                  if (add_group == NULL) {
                     /*
                      * No cycle found => success
                      */
                     ;
                  } else {
                     SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_HGROUP_CYCLEINDEF_SS,
                                            lGetHost(add_group, HR_name),
                                            lGetHost(hgroup, HGRP_name)));
                     answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX,
                                     ANSWER_QUALITY_ERROR);
                     ret = false;
                  }
               }
            }
         }

         /*
          * Make sure that:
          *   - added hosts where not already part the old hostlist
          *   - removed hosts are not part of the new hostlist
          */
         if (ret) {
            lList *tmp_hosts = NULL;

            ret &= href_list_find_all_references(old_href_list, answer_list,
                                                 master_list, &tmp_hosts, NULL);
            ret &= href_list_remove_existing(add_hosts, answer_list, tmp_hosts);
            tmp_hosts = lFreeList(tmp_hosts);

            ret &= href_list_find_all_references(href_list, answer_list,
                                                 master_list, &tmp_hosts, NULL);
            ret &= href_list_remove_existing(rem_hosts, answer_list, tmp_hosts);
            tmp_hosts = lFreeList(tmp_hosts);
         }

#if 1 /* debug */
         if (ret) {
            href_list_debug_print(*add_hosts, "add_hosts: ");
            href_list_debug_print(*rem_hosts, "rem_hosts: ");
         }
#endif

         /*
          * Cleanup
          */
         old_href_list = lFreeList(old_href_list);
         add_groups = lFreeList(add_groups);
         rem_groups = lFreeList(rem_groups);
      }
   }
   DEXIT;
   return ret;
}

static void 
hgroup_commit(lListElem *hgroup) 
{
   lList *cqueue_master_list = *(cqueue_list_get_master_list());
   lList *cqueue_list = lGetList(hgroup, HGRP_cqueue_list);
   lListElem *next_cqueue = NULL;
   lListElem *cqueue = NULL;

   DENTER(TOP_LAYER, "hgroup_commit");
   next_cqueue = lFirst(cqueue_list);
   while ((cqueue = next_cqueue)) {
      const char *name = lGetString(cqueue, CQ_name);
      lListElem *org_queue = lGetElemStr(cqueue_master_list, CQ_name, name);

      next_cqueue = lNext(cqueue);
      cqueue_commit(cqueue);
      lDechainElem(cqueue_list, cqueue);
      lAppendElem(cqueue_master_list, cqueue);
      lRemoveElem(cqueue_master_list, org_queue);
   }
   lSetList(hgroup, HGRP_cqueue_list, NULL);
   DEXIT;
}

static void 
hgroup_rollback(lListElem *this_elem) 
{
   DENTER(TOP_LAYER, "hgroup_rollback");
   lSetList(this_elem, HGRP_cqueue_list, NULL);
   DEXIT;
}

int 
hgroup_mod(lList **answer_list, lListElem *hgroup, lListElem *reduced_elem,
           int add, const char *remote_user, const char *remote_host, 
           gdi_object_t *object, int sub_command) 
{
   bool ret = true;
   int pos;

   DENTER(TOP_LAYER, "hgroup_mod");

   if (ret) {
      /* Did we get a hostgroupname?  */
      pos = lGetPosViaElem(reduced_elem, HGRP_name);
      if (pos >= 0) {
         const char *name = lGetPosHost(reduced_elem, pos);

         if (add) {
            /* Check groupname for new hostgroups */
            if (!verify_str_key(answer_list, &name[1], "hostgroup")) {
               lSetHost(hgroup, HGRP_name, name); 
            } else {
               ERROR((SGE_EVENT, MSG_HGRP_GROUPXNOTGUILTY_S, name));
               answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, 
                               ANSWER_QUALITY_ERROR);
               ret = false;
            }
         } else {
            const char *old_name = lGetHost(hgroup, HGRP_name);

            /* Reject modify requests which try to change the groupname */
            if (sge_hostcmp(old_name, name)) {
               ERROR((SGE_EVENT, MSG_HGRP_NONAMECHANGE));
               answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX,
                               ANSWER_QUALITY_ERROR);
               ret = false;
            }
         }
      } else {
         ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS, 
                lNm2Str(HGRP_name), SGE_FUNC));
         answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN, 
                         ANSWER_QUALITY_ERROR);
         ret = false;
      }
   }

   /*
    * Is there a list of host references
    */
   if (ret) {
      pos = lGetPosViaElem(reduced_elem, HGRP_host_list);
      if (pos >= 0) {
         lList *add_hosts = NULL;
         lList *rem_hosts = NULL;
         lList *occupant_groups = NULL;
   
         DPRINTF(("got new HGRP_host_list\n")); 

         if (ret) {
            ret &= hgroup_mod_hostlist(hgroup, answer_list, reduced_elem, 
                                       sub_command, &add_hosts, &rem_hosts, 
                                       &occupant_groups);
         }
         if (ret) {
            lList *cqueue_master_list = *(cqueue_list_get_master_list());
            lListElem *cqueue;

            for_each (cqueue, cqueue_master_list) {
               if (cqueue_is_a_href_referenced(cqueue, occupant_groups)) {
                  lList *cqueue_list = lGetList(hgroup, HGRP_cqueue_list);
                  lListElem *new_cqueue = NULL;

                  if (cqueue_list == NULL) {
                     cqueue_list = lCreateList("", CQ_Type);
                     lSetList(hgroup, HGRP_cqueue_list, cqueue_list);
                  }
                  new_cqueue = lCopyElem(cqueue);
                  lAppendElem(cqueue_list, new_cqueue);
                 
                  if (ret) {
                     ret &= cqueue_handle_qinstances(new_cqueue, answer_list, 
                                                     reduced_elem,
                                                     add_hosts, rem_hosts);
                  }
                  if (!ret) {
                     break;
                  }
               }
            }
            if (!ret) { 
               hgroup_rollback(hgroup);
            }
         } 

         add_hosts = lFreeList(add_hosts);
         rem_hosts = lFreeList(rem_hosts);
         occupant_groups = lFreeList(occupant_groups);
      }  
   } 

   DEXIT;
   if (ret) {
      return 0;
   } else {
      return STATUS_EUNKNOWN;
   }
}

int 
hgroup_del(lListElem *this_elem, lList **answer_list, 
           char *remote_user, char *remote_host) 
{
   int ret = true;

   DENTER(TOP_LAYER, "hgroup_del");
   /*
    * Check all incoming parameter
    */
   if (this_elem != NULL && remote_user != NULL && remote_host != NULL) {
      const char* name = lGetHost(this_elem, HGRP_name);

      /*
       * What is the name ob the hostgroup which should be removed?
       */
      if (name != NULL) {
         lList *master_hgroup_list = *(hgroup_list_get_master_list());
#ifndef __SGE_NO_USERMAPPING__
         lList *master_cuser_list = *(cuser_list_get_master_list());
#endif
         lListElem *hgroup;

         /*
          * Does this hostgroup exist?
          */
         hgroup = hgroup_list_locate(master_hgroup_list, name);
         if (hgroup != NULL) {
            lList *href_list = NULL;
#ifndef __SGE_NO_USERMAPPING__
            lList *string_list = NULL;
#endif

            /*
             * Is it still referenced in another hostgroup?
             */
            ret &= hgroup_find_referencees(hgroup, answer_list, 
                                           master_hgroup_list, 
                                           &href_list);
            if (ret) {
               if (href_list != NULL) {
                  dstring string = DSTRING_INIT;

                  href_list_append_to_dstring(href_list, &string);
                  ERROR((SGE_EVENT, MSG_HGROUP_REFINHGOUP_SS, name,
                         sge_dstring_get_string(&string)));
                  answer_list_add(answer_list, SGE_EVENT, STATUS_EEXIST,
                                  ANSWER_QUALITY_ERROR);
                  sge_dstring_free(&string);
                  ret = false;
               }
            }
            href_list = lFreeList(href_list);


#ifndef __SGE_NO_USERMAPPING__
            /*
             * Is it still referenced in a cluster user object (user mapping)
             */ 
            ret &= cuser_list_find_hgroup_references(master_cuser_list,
                                                    answer_list, hgroup,
                                                    &string_list);
            if (ret) {
               if (string_list != NULL) {
                  dstring string = DSTRING_INIT;

                  str_list_append_to_dstring(string_list, &string, ','); 
                  ERROR((SGE_EVENT, MSG_HGROUP_REFINCUSER_SS, name,
                         sge_dstring_get_string(&string)));
                  answer_list_add(answer_list, SGE_EVENT, STATUS_EEXIST,
                                  ANSWER_QUALITY_ERROR);
                  sge_dstring_free(&string);
                  ret = false;
               }
            }
            string_list = lFreeList(string_list);
#endif
            /*
             * Try to unlink the concerned spoolfile
             */
            if (ret) {
               if (sge_event_spool(answer_list, 0, sgeE_HGROUP_DEL, 
                                   0, 0, name, NULL, NULL,
                                   NULL, NULL, NULL, true, true)) {
                  /*
                   * Let's remove the object => Success!
                   */

                  lRemoveElem(Master_HGroup_List, hgroup);

                  INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS, 
                        remote_user, remote_host, name , "host group entry"));
                  answer_list_add(answer_list, SGE_EVENT, STATUS_OK, 
                                  ANSWER_QUALITY_INFO);
               } else {
                  ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS,"host group entry",
                         name ));
                  answer_list_add(answer_list, SGE_EVENT, STATUS_EEXIST, 
                                  ANSWER_QUALITY_ERROR);
                  ret = false;
               }
            }
         } else {
            ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, 
                   "host group", name));
            answer_list_add(answer_list, SGE_EVENT, STATUS_EEXIST, 
                            ANSWER_QUALITY_ERROR);
            ret = false;
         }
      } else {
         ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS, 
                lNm2Str(HGRP_name), SGE_FUNC));
         answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN, 
                         ANSWER_QUALITY_ERROR);
         ret = false;
      } 
   } else {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN, 
                      ANSWER_QUALITY_ERROR);
      ret = false;        
   }

   DEXIT;
   if (ret) {
      return STATUS_OK;
   } else {
      return STATUS_EUNKNOWN;
   }
}

int 
hgroup_success(lListElem *hgroup, lListElem *old_hgroup, gdi_object_t *object) 
{
   const char *name = lGetHost(hgroup, HGRP_name);

   DENTER(TOP_LAYER, "hgroup_success");

   /*
    * HGRP modify or add event
    */
   sge_add_event(NULL, 0, old_hgroup?sgeE_HGROUP_MOD:sgeE_HGROUP_ADD, 0, 0, 
                 name, NULL, NULL, hgroup);
   lListElem_clear_changed_info(hgroup);

   /*
    * QI add or delete events. Finalize operation.
    */
   hgroup_commit(hgroup);
    
   DEXIT;
   return 0;
}


int 
hgroup_spool(lList **answer_list, lListElem *this_elem, gdi_object_t *object) 
{
   bool tmp_ret = true;
   const char *name = lGetHost(this_elem, HGRP_name);
   lList *cqueue_list = lGetList(this_elem, HGRP_cqueue_list);
   lListElem *cqueue = NULL;

   DENTER(TOP_LAYER, "hgroup_spool");

   /*
    * EB: TODO: Spooling
    * 
    * Spooling for CQ and HGRP is no atomar operation until now!!!
    * Transactions within the spooling framework may solve this problem.
    */
   for_each (cqueue, cqueue_list) {
      if (!spool_write_object(NULL, spool_get_default_context(), cqueue,
                              name, SGE_TYPE_CQUEUE)) {
         ERROR((SGE_EVENT, MSG_CQUEUE_ERRORWRITESPOOLFILE_S, name));
         answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX,
                         ANSWER_QUALITY_ERROR);
         tmp_ret = false;
         break;
      }
   }

   if (tmp_ret && !spool_write_object(answer_list, spool_get_default_context(), 
                                      this_elem, name, SGE_TYPE_HGROUP)) {
      ERROR((SGE_EVENT, MSG_HGRP_ERRORWRITESPOOLFORGROUP_S, name));
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, 
                      ANSWER_QUALITY_ERROR);
      tmp_ret = false;
   }

   if (!tmp_ret) {
      hgroup_rollback(this_elem);
   }
   DEXIT;
   return tmp_ret ? 0 : 1;
}
