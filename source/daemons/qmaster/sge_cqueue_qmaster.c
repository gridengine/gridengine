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
#include "sge_string.h"
#include "sge_utility.h"
#include "sge_answer.h"
#include "sge_unistd.h"
#include "sge_hgroup.h"
#include "sge_cqueue.h"
#include "sge_qinstance.h"
#include "sge_queue.h"
#include "sge_userset.h"
#include "sge_href.h"
#include "sge_stringL.h"
#include "sge_event_master.h"
#include "sge_persistence_qmaster.h"
#include "sge_attr.h"
#include "sge_complex.h"
#include "sge_userprj.h"
#include "sge_feature.h"

#include "spool/classic/read_write_ume.h"
#include "spool/sge_spooling.h"

#include "msg_common.h"
#include "msg_qmaster.h"

enum {
   SGE_QI_TAG_DEFAULT = 0,
   SGE_QI_TAG_DEL     = 1,
   SGE_QI_TAG_ADD     = 2,
   SGE_QI_TAG_MOD     = 4
};

int cqueue_mod(lList **answer_list, lListElem *cqueue, lListElem *reduced_elem, 
               int add, const char *remote_user, const char *remote_host,
               gdi_object_t *object, int sub_command) 
{
   bool ret = true;
   lList *add_hosts = NULL;
   lList *rem_hosts = NULL;


   DENTER(TOP_LAYER, "cqueue_mod");

   if (ret) {
      int pos = lGetPosViaElem(reduced_elem, CQ_name);

      if (pos >= 0) {
         const char *name = lGetPosString(reduced_elem, pos);

         if (add) {
            if (!verify_str_key(answer_list, name, "cqueue")) {
               DTRACE;
               lSetString(cqueue, CQ_name, name);
            } else {
               ERROR((SGE_EVENT, MSG_CQUEUE_NAMENOTGUILTY_S, name));
               answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX,
                               ANSWER_QUALITY_ERROR);
               ret = false;
            }
         } else {
            const char *old_name = lGetString(cqueue, CQ_name);

            if (strcmp(old_name, name)) {
               ERROR((SGE_EVENT, MSG_CQUEUE_NONAMECHANGE));
               answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX,
                               ANSWER_QUALITY_ERROR);
               ret = false;
            }
         }
      } else {
         ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
                lNm2Str(CQ_name), SGE_FUNC));
         answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN,
                         ANSWER_QUALITY_ERROR);
         ret = false;
      }
   } 

   /*
    * Find differences of hostlist configuration
    * Resolve new hostnames
    * Verify that given hostgroups exist
    * Change the hostlist
    *
    * => add_hosts, rem_hosts, add_groups, rem_groups
    */
   if (ret) {
      int pos = lGetPosViaElem(reduced_elem, CQ_hostlist);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);
         lList *old_href_list = lGetList(cqueue, CQ_hostlist);
         lList *master_list = *(hgroup_list_get_master_list());
         lList *add_groups = NULL;
         lList *rem_groups = NULL;

         ret &= href_list_find_diff(list, answer_list,
                                    old_href_list, &add_hosts,
                                    &rem_hosts, &add_groups,
                                    &rem_groups);

         if (ret) {
            lList *master_list = *(hgroup_list_get_master_list());

            if (add_hosts != NULL) {
               ret &= href_list_resolve_hostnames(add_hosts, answer_list);
            }

            if (add_groups != NULL) {
               ret &= hgroup_list_exists(master_list, answer_list, add_groups);
            }
         }
         if (ret) {
            lSetList(cqueue, CQ_hostlist, lCopyList("", list));
         }
         if (ret) {
            href_list_find_all_references(add_groups, answer_list, master_list,
                                          &add_hosts, NULL);
            href_list_find_all_references(rem_groups, answer_list, master_list,
                                          &rem_hosts, NULL);

            lWriteListTo(add_hosts, stderr);
            lWriteListTo(rem_hosts, stderr);
         }
         add_groups = lFreeList(add_groups);
         rem_groups = lFreeList(rem_groups);
      }
   }

   /*
    * Modify all cqueue attributes according to the given instructions
    */
   if (ret) {
      int index = 0;

      while (cqueue_attribute_array[index].cqueue_attr != NoName && ret) {
         int pos = lGetPosViaElem(reduced_elem, 
                                  cqueue_attribute_array[index].cqueue_attr);

         if (pos >= 0) {
            /*
             * Sublist type (CE_Type, US_Type, ...) 
             * or simple type (bool, u_long32, const char *, ...)
             */
            if (cqueue_attribute_array[index].primary_key_attr != NoName) {
               ret &= cqueue_mod_sublist(cqueue, answer_list, reduced_elem, 
                                sub_command, 
                                cqueue_attribute_array[index].cqueue_attr, 
                                cqueue_attribute_array[index].href_attr, 
                                cqueue_attribute_array[index].value_attr, 
                                cqueue_attribute_array[index].primary_key_attr, 
                                cqueue_attribute_array[index].name, 
                                SGE_OBJ_CQUEUE);
            } else {
               lList *list = lGetPosList(reduced_elem, pos);

               lSetList(cqueue, cqueue_attribute_array[index].cqueue_attr, 
                        lCopyList("", list));
            }
         }
         index++;
      }
   }

   /*
    * Check all cqueue modifications
    */
   if (ret) {
      int index = 0;

      while (cqueue_attribute_array[index].cqueue_attr != NoName && ret) {
         if (cqueue_attribute_array[index].is_sgeee_attribute == false ||
             feature_is_enabled(FEATURE_SGEEE)) {
            int pos = lGetPosViaElem(reduced_elem,
                                     cqueue_attribute_array[index].cqueue_attr);

            if (pos >= 0) {
               lList *list = lGetList(cqueue, 
                                  cqueue_attribute_array[index].cqueue_attr);
               lListElem *elem = lGetElemHost(list, 
                                    cqueue_attribute_array[index].href_attr,
                                    HOSTREF_DEFAULT);


               if (elem == NULL) {
                  /* EB: TODO: move to msg file */
                  ERROR((SGE_EVENT, SFQ" has no default value\n", 
                                    cqueue_attribute_array[index].name));
                  answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN,
                                  ANSWER_QUALITY_ERROR);
                  ret = false;
               } 
            }
         }
         index++;
      }
   }

   /*
    * Remove qinstances
    */
   if (ret) {
      lListElem *href = NULL;

      for_each(href, rem_hosts) {
         const char *hostname = lGetHost(href, HR_name);
         lList *list = lGetList(cqueue, CQ_qinstances);
         lListElem* qinstance = lGetElemHost(list, QI_hostname, hostname);

         if (qinstance != NULL) {
            DPRINTF(("Deleting qinstance for host "SFQ"\n", hostname));
            lSetUlong(qinstance, QI_tag, SGE_QI_TAG_DEL);
         } 
      }
   }

   /*
    * Modify existing ones
    */
   if (ret) {
      int index = 0; 

      while (cqueue_attribute_array[index].cqueue_attr != NoName && ret) {
         if (cqueue_attribute_array[index].is_sgeee_attribute == false ||
             feature_is_enabled(FEATURE_SGEEE)) {
            int pos = lGetPosViaElem(reduced_elem,
                                     cqueue_attribute_array[index].cqueue_attr);

            if (pos >= 0) {
               lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
               lListElem *qinstance = NULL;
               bool print_once = false;

               for_each(qinstance, qinstance_list) {
                  const char *hostname = lGetHost(qinstance, QI_hostname);
                  bool is_ambiguous = false;
                  bool has_changed = false;
                     
                  if (!print_once) {
                     DPRINTF(("Modifying qinstance "SFQ"\n", hostname));
                     print_once = true; 
                  }
                  ret &= qinstance_modify(qinstance, answer_list, cqueue,
                                cqueue_attribute_array[index].qinstance_attr,
                                cqueue_attribute_array[index].cqueue_attr,
                                cqueue_attribute_array[index].href_attr,
                                cqueue_attribute_array[index].value_attr,
                                cqueue_attribute_array[index].primary_key_attr,
                                &is_ambiguous, &has_changed); 
                  if (!ret) {
                     break;
                  } 
                  if (has_changed) {
                     DPRINTF(("qinstance %s has been changed\n", hostname));
                     lSetUlong(qinstance, QI_tag, SGE_QI_TAG_MOD);
                  }
                  if (is_ambiguous) {
                     DPRINTF(("qinstance %s has ambiguous configuaration\n", 
                              hostname));
                     /* EB: TODO: Set ambiguous state */
                  }
               }
            }
         }
         index++;
      }
   }

   /*
    * Create qinstances
    */
   if (ret) {
      lListElem *href = NULL;

      for_each(href, add_hosts) {
         const char *hostname = lGetHost(href, HR_name);
         lList *list = lGetList(cqueue, CQ_qinstances);
         lListElem* new_qinstance;
         bool is_ambiguous = false;

         if (list == NULL) {
            list = lCreateList("", QI_Type);
            lSetList(cqueue, CQ_qinstances, list);
         }
         DPRINTF(("Creating qinstance for host "SFQ"\n", hostname));
         new_qinstance = qinstance_create(cqueue, answer_list, 
                                          hostname, &is_ambiguous);
         if (is_ambiguous) {
            DPRINTF(("qinstance %s has ambiguous configuaration\n", 
                     hostname));
            /* EB: TODO: Set ambiguous state */
         }
         lSetUlong(new_qinstance, QI_tag, SGE_QI_TAG_ADD);
         lAppendElem(list, new_qinstance);
      }
   }

   add_hosts = lFreeList(add_hosts);
   rem_hosts = lFreeList(rem_hosts);

   DEXIT;
   if (ret) {
      return 0;
   } else {
      return STATUS_EUNKNOWN;
   }
}

int cqueue_success(lListElem *cqueue, lListElem *old_cqueue, 
                   gdi_object_t *object) 
{
   lListElem *qinstance = NULL;
   lList *qinstances = NULL;

   DENTER(TOP_LAYER, "cqueue_success");
   
   qinstances = lGetList(cqueue, CQ_qinstances);

   /*
    * CQ modify or add event
    */
   sge_add_event(NULL, 0, old_cqueue?sgeE_CQUEUE_MOD:sgeE_CQUEUE_ADD, 0, 0, 
                 lGetString(cqueue, CQ_name), NULL, NULL, cqueue);

   /*
    * QI modify, add or delete event
    */
   for_each(qinstance, qinstances) {
      u_long32 tag = lGetUlong(qinstance, QI_tag);

      if (tag == SGE_QI_TAG_ADD) {
         DPRINTF(("ADD QI event\n"));
         sge_add_event(NULL, 0, sgeE_QINSTANCE_ADD, 0, 0, 
                       lGetString(qinstance, QI_name), 
                       lGetHost(qinstance, QI_hostname),
                       NULL, cqueue);
      } else if (tag == SGE_QI_TAG_MOD) {
         DPRINTF(("MOD QI event\n"));
         sge_add_event(NULL, 0, sgeE_QINSTANCE_MOD, 0, 0, 
                       lGetString(qinstance, QI_name), 
                       lGetHost(qinstance, QI_hostname),
                       NULL, cqueue);
      } else if (tag == SGE_QI_TAG_DEL) {
         DPRINTF(("DEL QI event\n"));
         sge_add_event(NULL, 0, sgeE_QINSTANCE_DEL, 0, 0, 
                       lGetString(qinstance, QI_name), 
                       lGetHost(qinstance, QI_hostname),
                       NULL, NULL);
      } 
      lSetUlong(qinstance, QI_tag, SGE_QI_TAG_DEFAULT);
   }
   DEXIT;
   return 0;
}

int cqueue_spool(lList **answer_list, lListElem *cqueue, gdi_object_t *object) 
{  
   int ret = 0;
   const char *name = lGetString(cqueue, CQ_name);

   DENTER(TOP_LAYER, "cqueue_spool");
   if (!spool_write_object(NULL, spool_get_default_context(), cqueue, 
                           name, SGE_TYPE_CQUEUE)) {
      ERROR((SGE_EVENT, MSG_CQUEUE_ERRORWRITESPOOLFILE_S, name));
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, 
                      ANSWER_QUALITY_ERROR);
      ret = 1;
   }
   DEXIT;
   return ret;
}

int cqueue_del(lListElem *this_elem, lList **answer_list, 
               char *remote_user, char *remote_host) 
{
   bool ret = true;

   DENTER(TOP_LAYER, "cqueue_del");

   if (this_elem != NULL && remote_user != NULL && remote_host != NULL) {
      const char* name = lGetString(this_elem, CQ_name);

      if (name != NULL) {
         lList *master_list = *(cqueue_list_get_master_list());
         lListElem *cqueue = cqueue_list_locate(master_list, name);

         if (cqueue != NULL) {
            if (sge_event_spool(answer_list, 0, sgeE_CQUEUE_DEL,
                                0, 0, name, NULL, NULL,
                                NULL, NULL, NULL, true, true)) {
               lRemoveElem(Master_CQueue_List, cqueue);

               INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS,
                     remote_user, remote_host, name , "cluster queue"));
               answer_list_add(answer_list, SGE_EVENT, STATUS_OK,
                               ANSWER_QUALITY_INFO);
            } else {
               ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS, "cluster queue",
                      name )); 
               answer_list_add(answer_list, SGE_EVENT, STATUS_EEXIST,
                               ANSWER_QUALITY_ERROR);
               ret = false;
            }
         } else {
            ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS,
                   "cluster queue", name));
            answer_list_add(answer_list, SGE_EVENT, STATUS_EEXIST,
                            ANSWER_QUALITY_ERROR);
            ret = false;
         }
      } else {
         ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
                lNm2Str(CQ_name), SGE_FUNC));
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

