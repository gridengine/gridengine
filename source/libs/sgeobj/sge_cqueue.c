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

#include "sgermon.h"
#include "sge_string.h"
#include "sge_log.h"
#include "cull_list.h"
#include "symbols.h"
#include "sge.h"

#include "sge_gdi.h"

#include "sge_dstring.h"
#include "sge_object.h"
#include "sge_answer.h"
#include "sge_attr.h"
#include "sge_centry.h"
#include "sge_cqueue.h"
#include "sge_qinstance.h"
#include "sge_queue.h"
#include "sge_stringL.h"
#include "sge_userprj.h"
#include "sge_userset.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#define CQUEUE_LAYER TOP_LAYER

/* *INDENT-OFF* */

list_attribute_struct cqueue_attribute_array[] = {
   { CQ_seq_no,                  QI_seq_no,                 AULNG_href,    AULNG_value,      NoName,     SGE_ATTR_SEQ_NO,            false},
   { CQ_nsuspend,                QI_nsuspend,               AULNG_href,    AULNG_value,      NoName,     SGE_ATTR_NSUSPEND,          false},
   { CQ_job_slots,               QI_job_slots,              AULNG_href,    AULNG_value,      NoName,     SGE_ATTR_SLOTS,             false},
   { CQ_fshare,                  QI_fshare,                 AULNG_href,    AULNG_value,      NoName,     SGE_ATTR_FSHARE,            true},
   { CQ_oticket,                 QI_oticket,                AULNG_href,    AULNG_value,      NoName,     SGE_ATTR_OTICKET,           true},

   { CQ_tmpdir,                  QI_tmpdir,                 ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_TMPDIR,            false},
   { CQ_shell,                   QI_shell,                  ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_SHELL,             false},
   { CQ_calendar,                QI_calendar,               ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_CALENDAR,          false},
   { CQ_priority,                QI_priority,               ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_PRIORITY,          false},
   { CQ_processors,              QI_processors,             ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_PROCESSORS,        false},
   { CQ_prolog,                  QI_prolog,                 ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_PROLOG,            false},
   { CQ_epilog,                  QI_epilog,                 ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_EPILOG,            false},
   { CQ_shell_start_mode,        QI_shell_start_mode,       ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_SHELL_START_MODE,  false},
   { CQ_starter_method,          QI_starter_method,         ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_STARTER_METHOD,    false},
   { CQ_suspend_method,          QI_suspend_method,         ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_SUSPEND_METHOD,    false},
   { CQ_resume_method,           QI_resume_method,          ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_RESUME_METHOD,     false},
   { CQ_terminate_method,        QI_terminate_method,       ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_TERMINATE_METHOD,  false},
   { CQ_initial_state,           QI_initial_state,          ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_INITIAL_STATE,     false},
   
   { CQ_rerun,                   QI_rerun,                  ABOOL_href,    ABOOL_value,      NoName,     SGE_ATTR_RERUN,             false},

   { CQ_s_fsize,                 QI_s_fsize,                AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_FSIZE,           false},
   { CQ_h_fsize,                 QI_h_fsize,                AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_FSIZE,           false},
   { CQ_s_data,                  QI_s_data,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_DATA,            false},
   { CQ_h_data,                  QI_h_data,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_DATA,            false},
   { CQ_s_stack,                 QI_s_stack,                AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_STACK,           false},
   { CQ_h_stack,                 QI_h_stack,                AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_STACK,           false},
   { CQ_s_core,                  QI_s_core,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_CORE,            false},
   { CQ_h_core,                  QI_h_core,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_CORE,            false},
   { CQ_s_rss,                   QI_s_rss,                  AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_RSS,             false},
   { CQ_h_rss,                   QI_h_rss,                  AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_RSS,             false},
   { CQ_s_vmem,                  QI_s_vmem,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_VMEM,            false},
   { CQ_h_vmem,                  QI_h_vmem,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_VMEM,            false},

   { CQ_s_rt,                    QI_s_rt,                   ATIME_href,    ATIME_value,      NoName,     SGE_ATTR_S_RT,              false},
   { CQ_h_rt,                    QI_h_rt,                   ATIME_href,    ATIME_value,      NoName,     SGE_ATTR_H_RT,              false},
   { CQ_s_cpu,                   QI_s_cpu,                  ATIME_href,    ATIME_value,      NoName,     SGE_ATTR_S_CPU,             false},
   { CQ_h_cpu,                   QI_h_cpu,                  ATIME_href,    ATIME_value,      NoName,     SGE_ATTR_H_CPU,             false},

   { CQ_suspend_interval,        QI_suspend_interval,       AINTER_href,   AINTER_value,     NoName,     SGE_ATTR_SUSPEND_INTERVAL,  false},
   { CQ_min_cpu_interval,        QI_min_cpu_interval,       AINTER_href,   AINTER_value,     NoName,     SGE_ATTR_MIN_CPU_INTERVAL,  false},
   { CQ_notify,                  QI_notify,                 AINTER_href,   AINTER_value,     NoName,     SGE_ATTR_NOTIFY,            false},

   { CQ_qtype,                   QI_qtype,                  AQTLIST_href,  AQTLIST_value,    NoName,     SGE_ATTR_QTYPE,             false},

   { CQ_ckpt_list,               QI_ckpt_list,              ASTRLIST_href, ASTRLIST_value,   ST_name,    SGE_ATTR_CKPT_LIST,         false},
   { CQ_pe_list,                 QI_pe_list,                ASTRLIST_href, ASTRLIST_value,   ST_name,    SGE_ATTR_PE_LIST,           false},
 
   { CQ_owner_list,              QI_owner_list,             AUSRLIST_href, AUSRLIST_value,   US_name,    SGE_ATTR_OWNER_LIST,        false},
   { CQ_acl,                     QI_acl,                    AUSRLIST_href, AUSRLIST_value,   US_name,    SGE_ATTR_USER_LISTS,        false},
   { CQ_xacl,                    QI_xacl,                   AUSRLIST_href, AUSRLIST_value,   US_name,    SGE_ATTR_XUSER_LISTS,       false},

   { CQ_projects,                QI_projects,               APRJLIST_href, APRJLIST_value,   UP_name,    SGE_ATTR_PROJECTS,          true},
   { CQ_xprojects,               QI_xprojects,              APRJLIST_href, APRJLIST_value,   UP_name,    SGE_ATTR_XPROJECTS,         true},

   { CQ_consumable_config_list,  QI_consumable_config_list, ACELIST_href,  ACELIST_value,    CE_name,    SGE_ATTR_COMPLEX_VALUES,    false},
   { CQ_load_thresholds,         QI_load_thresholds,        ACELIST_href,  ACELIST_value,    CE_name,    SGE_ATTR_LOAD_THRESHOLD,    false},
   { CQ_suspend_thresholds,      QI_suspend_thresholds,     ACELIST_href,  ACELIST_value,    CE_name,    SGE_ATTR_SUSPEND_THRESHOLD, false},

   { CQ_subordinate_list,        QI_subordinate_list,       ASOLIST_href,  ASOLIST_value,    SO_qname,   SGE_ATTR_SUBORDINATE_LIST,  false},

   { NoName,                     NoName,                    NoName,        NoName,           NoName,     NULL,                       false}
};

/* *INDENT-ON* */

lList *Master_CQueue_List = NULL;

bool
cqueue_name_split(const char *name, dstring *cqueue_name, dstring *host_domain, 
                  bool *has_hostname, bool *has_domain)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_name_split");
   if (name != NULL && cqueue_name != NULL && 
       host_domain != NULL && has_hostname != NULL && has_domain != NULL) {
      const char *first = strchr(name, '@');

      if (first != NULL) {
         sge_dstring_sprintf(cqueue_name, "%s", name);
         first++;
         if (*first == '@') {
            *has_hostname = false;
            *has_domain = true;
         } else if (*first == '\0') {
            *has_hostname = false;
            *has_domain = false;
         } else {
            *has_hostname = true;
            *has_domain = false;
         }
         sge_dstring_sprintf(host_domain, "%s", first);

         fprintf(stderr, "%s\n", name);
         fprintf(stderr, "%s\n", first);
      } else {
         sge_dstring_sprintf(cqueue_name, "%s", name);
      }
   }
   DEXIT;
   return ret;
}


lList **cqueue_list_get_master_list(void)
{
   return &Master_CQueue_List;
}

lListElem *
cqueue_create(lList **answer_list, const char *name)
{
   lListElem *ret = NULL;

   DENTER(CQUEUE_LAYER, "cuser_create");
   if (name != NULL) {
      ret = lCreateElem(CQ_Type);

      if (ret != NULL) {
         lSetString(ret, CQ_name, name);
      } else {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, 
                                MSG_MEM_MEMORYALLOCFAILED_S, SGE_FUNC));
         answer_list_add(answer_list, SGE_EVENT,
                         STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_list_add_cqueue(lListElem *queue)
{
   bool ret = false;
   static lSortOrder *so = NULL;

   DENTER(TOP_LAYER, "cqueue_list_add_cqueue");

   if (queue != NULL) {
      if (so == NULL) {
         so = lParseSortOrderVarArg(CQ_Type, "%I+", CQ_name);
      }

      if (Master_CQueue_List == NULL) {
         Master_CQueue_List = lCreateList("", CQ_Type);
      }

      lInsertSorted(so, queue, Master_CQueue_List);
      ret = true;
   } 
   DEXIT;
   return ret;
}

lListElem *
cqueue_list_locate(const lList *this_list, const char *name)
{
   return lGetElemStr(this_list, CQ_name, name);
}


bool
cqueue_mod_sublist(lListElem *this_elem, lList **answer_list,
                   lListElem *reduced_elem, int sub_command,
                   int attribute_name, int sublist_host_name,
                   int sublist_value_name, int subsub_key,
                   const char *attribute_name_str, 
                   const char *object_name_str) 
{
   bool ret = true;
   int pos;

   DENTER(CQUEUE_LAYER, "cqueue_mod_sublist");
  
   pos = lGetPosViaElem(reduced_elem, attribute_name);
   if (pos >= 0) {
      lList *mod_list = lGetPosList(reduced_elem, pos);
      lList *org_list = lGetList(this_elem, attribute_name);
      lListElem *mod_elem;

      /* 
       * Delete all configuration lists except the default-configuration
       * if sub_command is SGE_GDI_SET_ALL
       */
      if (sub_command == SGE_GDI_SET_ALL) {
         lListElem *elem, *next_elem;

         next_elem = lFirst(org_list);
         while ((elem = next_elem)) {
            const char *name = lGetHost(elem, sublist_host_name);

            next_elem = lNext(elem); 
            mod_elem = lGetElemHost(mod_list, sublist_host_name, name);
            if (mod_elem == NULL) {
               const char *name = lGetHost(elem, sublist_host_name);

               DPRINTF(("Removing attribute list for "SFQ"\n", name));
               lRemoveElem(org_list, elem);
            }
         }
      }

      /*
       * Do modifications for all given domain/host-configuration list
       */
      for_each(mod_elem, mod_list) {
         const char *name = lGetHost(mod_elem, sublist_host_name);
         lListElem *org_elem = lGetElemHost(org_list, sublist_host_name, name);

         /*
          * Create default-element if it does not exist
          */
         if (org_elem == NULL && 
             (!strcmp(name, HOSTREF_DEFAULT) || sub_command == SGE_GDI_SET_ALL)) {
            if (org_list == NULL) {
               org_list = lCreateList("", lGetElemDescr(mod_elem));
               lSetList(this_elem, attribute_name, org_list);
            } 
            org_elem = lCreateElem(lGetElemDescr(mod_elem));
            lSetHost(org_elem, sublist_host_name, name);
            lAppendElem(org_list, org_elem);
         }

         /*
          * Modify sublist according to subcommand
          */
         if (org_elem != NULL) {
            attr_mod_sub_list(answer_list, org_elem, sublist_value_name, 
                              subsub_key, mod_elem, sub_command, 
                              attribute_name_str, object_name_str, 0);
         }
      }
   }
 
   DEXIT;
   return ret;
}

