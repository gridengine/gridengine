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
#include "sge_queue.h"
#include "sge_userset.h"
#include "sge_href.h"
#include "sge_stringL.h"
#include "sge_event_master.h"
#include "sge_persistence_qmaster.h"
#include "sge_attr.h"
#include "sge_complex.h"
#include "sge_userprj.h"

#include "spool/classic/read_write_ume.h"
#include "spool/sge_spooling.h"

#include "msg_common.h"
#include "msg_qmaster.h"

int cqueue_mod(lList **answer_list, lListElem *cqueue, lListElem *reduced_elem, 
               int add, const char *remote_user, const char *remote_host,
               gdi_object_t *object, int sub_command) 
{
   bool ret = true;
   int pos;
   lList *add_hosts = NULL;
   lList *rem_hosts = NULL;
   lList *add_groups = NULL;
   lList *rem_groups = NULL;


   DENTER(TOP_LAYER, "cqueue_mod");

   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_name);

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
    * - find differences of hostlist configuration
    * - resolve new hostnames
    * - verify that new hostgroups exist
    * - change the hostlist
    */
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_hostlist);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);
         lList *old_href_list = lGetList(cqueue, CQ_hostlist);

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
#if 1 /* EB: debug */
            {
               dstring message = DSTRING_INIT;

               sge_dstring_append(&message, "add_hosts: ");
               href_list_append_to_dstring(add_hosts, &message);
               sge_dstring_append(&message, " ");
               sge_dstring_append(&message, "rem_hosts: ");
               href_list_append_to_dstring(rem_hosts, &message);
               sge_dstring_append(&message, " ");
               sge_dstring_append(&message, "add_groups: ");
               href_list_append_to_dstring(add_groups, &message);
               sge_dstring_append(&message, " ");
               sge_dstring_append(&message, "rem_groups: ");
               href_list_append_to_dstring(rem_groups, &message);
               sge_dstring_append(&message, "\n");
               DPRINTF(("%s", sge_dstring_get_string(&message)));
            }
#endif
            lSetList(cqueue, CQ_hostlist, lCopyList("", list));
         }
      }
   }

#if 0   
   if (ret) {
      int index;
      int array[] = {
         CQ_consumable_config_list, CQ_load_thresholds,  CQ_suspend_thresholds,
         CQ_projects,               CQ_xprojects,        CQ_acl, 
         CQ_xacl,                   CQ_owner_list,       CQ_ckpt_list, 
         CQ_pe_list,                CQ_subordinate_list, NoName
      };

      index = 0;
      while (array[index] != NoName && ret) {
         pos = lGetPosViaElem(reduced_elem, array[index]);

         if (pos >= 0) {
            ret &= cqueue_mod_sublist(cqueue, answer_list, reduced_elem, 
                                sub_command, array[index], 
                                cqueue_attr_get_href_attr(array[index]),
                                cqueue_attr_get_value_attr(array[index]), 
                                cqueue_attr_get_primary_key_attr(array[index]),
                                cqueue_attr_get_name(array[index]), 
                                SGE_OBJ_CQUEUE);
         }
         index++;
      }
   }
#endif
   
   if (ret) {
      int index;
      int array[] = { 
         CQ_suspend_interval, NoName
      };
#if 0
      int array[] = { 
         CQ_seq_no,  CQ_nsuspend, CQ_job_slots,        CQ_fshare,
         CQ_oticket, CQ_rerun,    CQ_suspend_interval, CQ_min_cpu_interval,
         CQ_notify,  CQ_tmpdir,   CQ_shell,            CQ_s_fsize,
         CQ_h_fsize, CQ_s_data,   CQ_h_data,           CQ_s_stack,
         CQ_h_stack, CQ_s_core,   CQ_h_core,           CQ_s_rss,
         CQ_h_rss,   CQ_s_vmem,   CQ_h_vmem,           CQ_s_rt,
         CQ_h_rt,    CQ_s_cpu,    CQ_h_cpu,
         NoName
      };
#endif

      index = 0;
      while (array[index] != NoName && ret) {
         pos = lGetPosViaElem(reduced_elem, array[index]);

         if (pos >= 0) {
            lList *list = lGetPosList(reduced_elem, pos);

lWriteListTo(list, stderr);
            lSetList(cqueue, array[index], lCopyList("", list));
         }
         index++;
      }
   }

   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_qtype);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);
   
         lSetList(cqueue, CQ_qtype, lCopyList("", list));
      }
   }

   add_hosts = lFreeList(add_hosts);
   rem_hosts = lFreeList(rem_hosts);
   add_groups = lFreeList(add_groups);
   rem_groups = lFreeList(rem_groups);

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
   DENTER(TOP_LAYER, "cqueue_success");
   sge_add_event(NULL, 0, old_cqueue?sgeE_CQUEUE_MOD:sgeE_CQUEUE_ADD, 0,
                 0, lGetString(cqueue, CQ_name), NULL, cqueue);
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
                                0, 0, name, NULL,
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

