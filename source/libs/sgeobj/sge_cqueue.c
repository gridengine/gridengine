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

#include <fnmatch.h>

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
#include "sge_str.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_feature.h"
#include "sge_href.h"
#include "sge_hgroup.h"
#include "sge_href.h"
#include "sge_pe.h"
#include "sge_ckpt.h"
#include "sge_userprj.h"
#include "sge_calendar.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#define CQUEUE_LAYER TOP_LAYER

/* *INDENT-OFF* */

list_attribute_struct cqueue_attribute_array[] = {
   { CQ_seq_no,                  QI_seq_no,                 AULNG_href,    AULNG_value,      NoName,     SGE_ATTR_SEQ_NO,            false,  false, NULL},
   { CQ_nsuspend,                QI_nsuspend,               AULNG_href,    AULNG_value,      NoName,     SGE_ATTR_NSUSPEND,          false,  false, NULL},
   { CQ_job_slots,               QI_job_slots,              AULNG_href,    AULNG_value,      NoName,     SGE_ATTR_SLOTS,             false,  false, NULL},
   { CQ_fshare,                  QI_fshare,                 AULNG_href,    AULNG_value,      NoName,     SGE_ATTR_FSHARE,            true,   false, NULL},
   { CQ_oticket,                 QI_oticket,                AULNG_href,    AULNG_value,      NoName,     SGE_ATTR_OTICKET,           true,   false, NULL},

   { CQ_tmpdir,                  QI_tmpdir,                 ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_TMPDIR,            false,  false, NULL},
   { CQ_shell,                   QI_shell,                  ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_SHELL,             false,  false, NULL},
   { CQ_calendar,                QI_calendar,               ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_CALENDAR,          false,  false, cqueue_verify_calendar},
   { CQ_priority,                QI_priority,               ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_PRIORITY,          false,  true,  cqueue_verify_priority},
   { CQ_processors,              QI_processors,             ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_PROCESSORS,        false,  false, NULL},
   { CQ_prolog,                  QI_prolog,                 ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_PROLOG,            false,  false, NULL},
   { CQ_epilog,                  QI_epilog,                 ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_EPILOG,            false,  false, NULL},
   { CQ_shell_start_mode,        QI_shell_start_mode,       ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_SHELL_START_MODE,  false,  true,  cqueue_verify_shell_start_mode},
   { CQ_starter_method,          QI_starter_method,         ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_STARTER_METHOD,    false,  false, NULL},
   { CQ_suspend_method,          QI_suspend_method,         ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_SUSPEND_METHOD,    false,  false, NULL},
   { CQ_resume_method,           QI_resume_method,          ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_RESUME_METHOD,     false,  false, NULL},
   { CQ_terminate_method,        QI_terminate_method,       ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_TERMINATE_METHOD,  false,  false, NULL},
   { CQ_initial_state,           QI_initial_state,          ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_INITIAL_STATE,     false,  true,  cqueue_verify_initial_state},
   
   { CQ_rerun,                   QI_rerun,                  ABOOL_href,    ABOOL_value,      NoName,     SGE_ATTR_RERUN,             false,  false, NULL},

   { CQ_s_fsize,                 QI_s_fsize,                AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_FSIZE,           false,  false, NULL},
   { CQ_h_fsize,                 QI_h_fsize,                AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_FSIZE,           false,  false, NULL},
   { CQ_s_data,                  QI_s_data,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_DATA,            false,  false, NULL},
   { CQ_h_data,                  QI_h_data,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_DATA,            false,  false, NULL},
   { CQ_s_stack,                 QI_s_stack,                AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_STACK,           false,  false, NULL},
   { CQ_h_stack,                 QI_h_stack,                AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_STACK,           false,  false, NULL},
   { CQ_s_core,                  QI_s_core,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_CORE,            false,  false, NULL},
   { CQ_h_core,                  QI_h_core,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_CORE,            false,  false, NULL},
   { CQ_s_rss,                   QI_s_rss,                  AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_RSS,             false,  false, NULL},
   { CQ_h_rss,                   QI_h_rss,                  AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_RSS,             false,  false, NULL},
   { CQ_s_vmem,                  QI_s_vmem,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_VMEM,            false,  false, NULL},
   { CQ_h_vmem,                  QI_h_vmem,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_VMEM,            false,  false, NULL},

   { CQ_s_rt,                    QI_s_rt,                   ATIME_href,    ATIME_value,      NoName,     SGE_ATTR_S_RT,              false,  false, NULL},
   { CQ_h_rt,                    QI_h_rt,                   ATIME_href,    ATIME_value,      NoName,     SGE_ATTR_H_RT,              false,  false, NULL},
   { CQ_s_cpu,                   QI_s_cpu,                  ATIME_href,    ATIME_value,      NoName,     SGE_ATTR_S_CPU,             false,  false, NULL},
   { CQ_h_cpu,                   QI_h_cpu,                  ATIME_href,    ATIME_value,      NoName,     SGE_ATTR_H_CPU,             false,  false, NULL},

   { CQ_suspend_interval,        QI_suspend_interval,       AINTER_href,   AINTER_value,     NoName,     SGE_ATTR_SUSPEND_INTERVAL,  false,  false, NULL},
   { CQ_min_cpu_interval,        QI_min_cpu_interval,       AINTER_href,   AINTER_value,     NoName,     SGE_ATTR_MIN_CPU_INTERVAL,  false,  false, NULL},
   { CQ_notify,                  QI_notify,                 AINTER_href,   AINTER_value,     NoName,     SGE_ATTR_NOTIFY,            false,  false, NULL},

   { CQ_qtype,                   QI_qtype,                  AQTLIST_href,  AQTLIST_value,    NoName,     SGE_ATTR_QTYPE,             false,  false, NULL},

   { CQ_ckpt_list,               QI_ckpt_list,              ASTRLIST_href, ASTRLIST_value,   ST_name,    SGE_ATTR_CKPT_LIST,         false,  false, cqueue_verify_ckpt_list},
   { CQ_pe_list,                 QI_pe_list,                ASTRLIST_href, ASTRLIST_value,   ST_name,    SGE_ATTR_PE_LIST,           false,  false, cqueue_verify_pe_list},
 
   { CQ_owner_list,              QI_owner_list,             AUSRLIST_href, AUSRLIST_value,   US_name,    SGE_ATTR_OWNER_LIST,        false,  false, NULL},
   { CQ_acl,                     QI_acl,                    AUSRLIST_href, AUSRLIST_value,   US_name,    SGE_ATTR_USER_LISTS,        false,  false, cqueue_verify_user_list},
   { CQ_xacl,                    QI_xacl,                   AUSRLIST_href, AUSRLIST_value,   US_name,    SGE_ATTR_XUSER_LISTS,       false,  false, cqueue_verify_user_list},

   { CQ_projects,                QI_projects,               APRJLIST_href, APRJLIST_value,   UP_name,    SGE_ATTR_PROJECTS,          true,   false, cqueue_verify_project_list},
   { CQ_xprojects,               QI_xprojects,              APRJLIST_href, APRJLIST_value,   UP_name,    SGE_ATTR_XPROJECTS,         true,   false, cqueue_verify_project_list},

   { CQ_consumable_config_list,  QI_consumable_config_list, ACELIST_href,  ACELIST_value,    CE_name,    SGE_ATTR_COMPLEX_VALUES,    false,  false, NULL},
   { CQ_load_thresholds,         QI_load_thresholds,        ACELIST_href,  ACELIST_value,    CE_name,    SGE_ATTR_LOAD_THRESHOLD,    false,  false, NULL},
   { CQ_suspend_thresholds,      QI_suspend_thresholds,     ACELIST_href,  ACELIST_value,    CE_name,    SGE_ATTR_SUSPEND_THRESHOLD, false,  false, NULL},

   /* EB: TODO: add verify function */
   { CQ_subordinate_list,        QI_subordinate_list,       ASOLIST_href,  ASOLIST_value,    SO_name,    SGE_ATTR_SUBORDINATE_LIST,  false,  false, NULL},

   { NoName,                     NoName,                    NoName,        NoName,           NoName,     NULL,                       false,  false, NULL}
};

/* *INDENT-ON* */

lList *Master_CQueue_List = NULL;

bool
cqueue_name_split(const char *name, 
                  dstring *cqueue_name, dstring *host_domain, 
                  bool *has_hostname, bool *has_domain)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_name_split");
   if (name != NULL && cqueue_name != NULL && 
       host_domain != NULL && has_hostname != NULL && has_domain != NULL) {
      int part = 0;
      const char *tmp_string;

      while (*name != '\0') {
         if (part == 1) {
            part = 2;
         } else if (part == 0 && *name == '@') {
            part = 1;
         }
         if (part == 0) {
            sge_dstring_sprintf_append(cqueue_name, "%c", name[0]);
         } else if (part == 2) {
            sge_dstring_sprintf_append(host_domain, "%c", name[0]);
         }
         name++;
      } 
      tmp_string = sge_dstring_get_string(host_domain);
      *has_hostname = false;
      *has_domain = false;
      if (tmp_string != NULL) {
         if (tmp_string[0] == '@') {
            *has_domain = true;
         } else {
            *has_hostname = true;
         }
      } 
   }
   DEXIT;
   return ret;
}

lEnumeration *
enumeration_create_reduced_cq(bool fetch_all_qi, bool fetch_all_nqi)
{
   lEnumeration *ret;
   dstring format_string = DSTRING_INIT;
   lDescr *descr = CQ_Type;
   int name_array[100];
   int names = -1;
   int attr;

   DENTER(CQUEUE_LAYER, "enumeration_create_reduced_cq");
   for_each_attr(attr, descr) {
      if (names == -1) {
         sge_dstring_sprintf(&format_string, "%s", "%T(");
      }
      if ((attr == CQ_name) ||
          (fetch_all_qi && attr == CQ_qinstances) ||
          (fetch_all_nqi && attr != CQ_qinstances)) {
         names++;
         name_array[names] = attr;
         sge_dstring_sprintf_append(&format_string, "%s", "%I");
      }
   }
   sge_dstring_sprintf_append(&format_string, "%s", ")");
   ret = _lWhat(sge_dstring_get_string(&format_string), CQ_Type, name_array, ++names);
   sge_dstring_free(&format_string);
   
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

   DENTER(CQUEUE_LAYER, "cqueue_create");
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
cqueue_is_href_referenced(const lListElem *this_elem, const lListElem *href)
{
   bool ret = false;

   if (this_elem != NULL && href != NULL) {
      const char *href_name = lGetHost(href, HR_name);
      
      if (href_name != NULL) {
         lList *href_list = lGetList(this_elem, CQ_hostlist);
         lListElem *tmp_href = lGetElemHost(href_list, HR_name, href_name);

         if (tmp_href != NULL) {
            ret = true;
         }
      }
   }
   return ret;
} 

bool 
cqueue_is_a_href_referenced(const lListElem *this_elem, const lList *href_list)
{
   bool ret = false;
  
   if (this_elem != NULL && href_list != NULL) { 
      lListElem *href;

      for_each(href, href_list) {
         if (cqueue_is_href_referenced(this_elem, href)) {
            ret = true;
            break;
         }
      }
   }
   return ret;
} 

bool
cqueue_set_template_attributes(lListElem *this_elem, lList **answer_list)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_set_template_attributes");
   if (this_elem != NULL) {
      if (ret) {
         lList *href_list = NULL;

         href_list_add(&href_list, answer_list, "NONE");
         lSetList(this_elem, CQ_hostlist, href_list);
      }

      /*
       * initialize u_long32 values
       */
      if (ret) {
         const u_long32 value[] = {
            7, 1, 1, 0, 0, 0 
         }; 
         const int attr[] = {
            CQ_seq_no, CQ_nsuspend, CQ_job_slots, CQ_fshare, CQ_oticket, NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, AULNG_href, 
                                                HOSTREF_DEFAULT, AULNG_Type);

            lSetUlong(attr_elem, AULNG_value, value[index]);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }

      /*
       * initialize bool values
       */
      if (ret) {
         lList *attr_list = NULL;
         lListElem *attr = lAddElemHost(&attr_list, ABOOL_href, 
                                        HOSTREF_DEFAULT, ABOOL_Type);

         lSetBool(attr, ABOOL_value, 7);
         lSetList(this_elem, CQ_rerun, attr_list);
      }

      /*
       * initialize memory values
       */
      if (ret) {
         const char *value[] = {
            "INFINITY", "INFINITY", "INFINITY", "INFINITY",
            "INFINITY", "INFINITY", "INFINITY", "INFINITY",
            "INFINITY", "INFINITY", "INFINITY", "INFINITY",
            NULL
         }; 
         const int attr[] = {
            CQ_s_fsize, CQ_h_fsize, CQ_s_data, CQ_h_data,
            CQ_s_stack, CQ_h_stack, CQ_s_core, CQ_h_core,
            CQ_s_rss, CQ_h_rss, CQ_s_vmem, CQ_h_vmem,
            NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, AMEM_href, 
                                                HOSTREF_DEFAULT, AMEM_Type);

            lSetString(attr_elem, AMEM_value, value[index]);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }
      
      /*
       * initialize time values
       */
      if (ret) {
         const char *value[] = {
            "INFINITY", "INFINITY", "INFINITY", "INFINITY",
            NULL
         }; 
         const int attr[] = {
            CQ_s_rt, CQ_h_rt, CQ_s_cpu, CQ_h_cpu,
            NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, ATIME_href, 
                                                HOSTREF_DEFAULT, ATIME_Type);

            lSetString(attr_elem, ATIME_value, value[index]);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }

      /*
       * initialize interval values
       */
      if (ret) {
         const char *value[] = {
            "00:05:00", "00:05:00", "00:00:60",
            NULL
         }; 
         const int attr[] = {
            CQ_suspend_interval, CQ_min_cpu_interval, CQ_notify,
            NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, AINTER_href, 
                                                HOSTREF_DEFAULT, AINTER_Type);

            lSetString(attr_elem, AINTER_value, value[index]);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }

      /*
       * initialize string values
       */
      if (ret) {
         const char *value[] = {
            "/tmp", "/bin/csh", "NONE",
            "0", "UNDEFINED", "NONE",
            "NONE", "posix_compliant", "NONE",
            "NONE", "NONE", "NONE",
            "default", 
            NULL
         }; 
         const int attr[] = {
            CQ_tmpdir, CQ_shell, CQ_calendar,
            CQ_priority, CQ_processors, CQ_prolog,
            CQ_epilog, CQ_shell_start_mode, CQ_starter_method,
            CQ_suspend_method, CQ_resume_method, CQ_terminate_method,
            CQ_initial_state,
            NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, ASTR_href, 
                                                HOSTREF_DEFAULT, ASTR_Type);

            lSetString(attr_elem, ASTR_value, value[index]);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }

      /*
       * initialize string-list values
       */
      if (ret) {
         const int attr[] = {
            CQ_pe_list, CQ_ckpt_list,
            NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, ASTRLIST_href, 
                                                HOSTREF_DEFAULT, ASTRLIST_Type);

            lSetList(attr_elem, ASTRLIST_value, NULL);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
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
               DPRINTF(("Removing attribute list for "SFQ"\n", name));
               lRemoveElem(org_list, elem);
            }
         }
      }

      /*
       * Do modifications for all given elements of domain/host-configuration list
       */
      for_each(mod_elem, mod_list) {
         const char *name = lGetHost(mod_elem, sublist_host_name);
         lListElem *org_elem = lGetElemHost(org_list, sublist_host_name, name);

         /*
          * Create element if it does not exist
          */
         if (org_elem == NULL) {
            if (org_list == NULL) {
               org_list = lCreateList("", lGetElemDescr(mod_elem));
               lSetList(this_elem, attribute_name, org_list);
            } 
            org_elem = lCreateElem(lGetElemDescr(mod_elem));
            lSetHost(org_elem, sublist_host_name, name);
   /* EB: TODO: error message and error handling */
            lAppendElem(org_list, org_elem);
         }

         /*
          * Modify sublist according to subcommand
          */
         if (org_elem != NULL) {
            if (subsub_key != NoName) {
               attr_mod_sub_list(answer_list, org_elem, sublist_value_name, 
                              subsub_key, mod_elem, sub_command, 
                              attribute_name_str, object_name_str, 0);
            } else {
               object_replace_any_type(org_elem, sublist_value_name, mod_elem);
            }
         }
      }
   }
 
   DEXIT;
   return ret;
}

bool
cqueue_list_find_all_matching_references(const lList *this_list,
                                         lList **answer_list,
                                         const char *cqueue_pattern,
                                         lList **qref_list)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_list_find_all_matching_references");
   if (this_list != NULL && cqueue_pattern != NULL && qref_list != NULL) {
      lListElem *cqueue;

      for_each(cqueue, this_list) {
         const char *cqueue_name = lGetString(cqueue, CQ_name);
         
         if (!fnmatch(cqueue_pattern, cqueue_name, 0)) {
            if (*qref_list == NULL) {
               *qref_list = lCreateList("", QR_Type);
            }
            if (*qref_list != NULL) {
               lAddElemStr(qref_list, QR_name, cqueue_name, QR_Type);
            }
         }
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_xattr_pre_gdi(lList *this_list, lList **answer_list) 
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_xattr_pre_gdi");
   if (this_list != NULL) {
      lListElem *cqueue = NULL;
   
      for_each(cqueue, this_list) {
         const char *name = lGetString(cqueue, CQ_name);
         dstring cqueue_name = DSTRING_INIT;
         dstring host_domain = DSTRING_INIT;
         bool has_hostname = false;
         bool has_domain = false;

         cqueue_name_split(name, &cqueue_name, &host_domain,
                           &has_hostname, &has_domain);
         if (has_domain || has_hostname) {
            int index = 0;

            /*
             * Change QI/QD name to CQ name
             */
            lSetString(cqueue, CQ_name, sge_dstring_get_string(&cqueue_name));

            /*
             * Make sure that there is only a default entry
             * and change that default entry to be a QD/QI entry
             */
            while (cqueue_attribute_array[index].cqueue_attr != NoName && ret) {
               int pos = lGetPosViaElem(cqueue,
                                  cqueue_attribute_array[index].cqueue_attr);

               if (pos >= 0) {
                  lList *list = lGetPosList(cqueue, pos);
                  lListElem *elem = NULL;

                  for_each(elem, list) {
                     const char *attr_hostname = lGetHost(elem, 
                                       cqueue_attribute_array[index].href_attr);

                     if (strcmp(HOSTREF_DEFAULT, attr_hostname)) {
                        SGE_ADD_MSG_ID(sprintf(SGE_EVENT,
                                       MSG_CQUEUE_NONDEFNOTALLOWED));
                        answer_list_add(answer_list, SGE_EVENT,
                                        STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR); 
                        ret = false;
                     } else {
                        lSetHost(elem, cqueue_attribute_array[index].href_attr,
                                 sge_dstring_get_string(&host_domain));
                     }
                  }
               }
               index++;
            }
         }
         sge_dstring_free(&host_domain);
         sge_dstring_free(&cqueue_name);
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_mod_hostlist(lListElem *cqueue, lList **answer_list,
                    lListElem *reduced_elem, int sub_command, 
                    lList **add_hosts, lList **rem_hosts)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_mod_hostlist");
   if (cqueue != NULL && reduced_elem != NULL) {
      int pos = lGetPosViaElem(reduced_elem, CQ_hostlist);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);
         lList *old_href_list = lCopyList("", lGetList(cqueue, CQ_hostlist));
         lList *master_list = *(hgroup_list_get_master_list());
         lList *href_list = NULL;
         lList *add_groups = NULL;
         lList *rem_groups = NULL;

         /*
          * Make sure that all given hostnames exist
          */
         if (ret) {
            ret &= href_list_resolve_hostnames(list, answer_list);
         }

         /*
          * Find: added/removed hosts/groups
          */
         if (ret) {
            ret &= href_list_find_diff(list, answer_list, old_href_list, 
                                       add_hosts, rem_hosts, &add_groups,
                                       &rem_groups);
         }

         /*
          * Make sure that all added groups exist
          */
         if (ret && add_groups != NULL) {
            ret &= hgroup_list_exists(master_list, answer_list, add_groups);
         }

         /*
          * Remove added elements which were removed somewhere else
          * and remove removed elements which where added somewhere else 
          */
         if (ret) {
            ret &= href_list_find_effective_diff(answer_list, add_groups, 
                                                 rem_groups, master_list, 
                                                 add_hosts, rem_hosts);
         }

         /*
          * Modify hostlist
          */
         if (ret) {
            attr_mod_sub_list(answer_list, cqueue, CQ_hostlist, HR_name, 
                              reduced_elem, sub_command, SGE_ATTR_HOST_LIST,
                              SGE_OBJ_CQUEUE, 0);         
            href_list = lGetList(cqueue, CQ_hostlist);
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

#if 1 /* EB: debug */
         if (ret) {
            href_list_debug_print(*add_hosts, "add_hosts: ");
            href_list_debug_print(*rem_hosts, "rem_hosts: ");
         }
#endif

         old_href_list = lFreeList(old_href_list);
         add_groups = lFreeList(add_groups);
         rem_groups = lFreeList(rem_groups);
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_handle_qinstances(lListElem *cqueue, lList **answer_list,
                         lListElem *reduced_elem, lList *add_hosts,
                         lList *rem_hosts) 
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_handle_qinstances");

   if (ret) { 
      ret &= cqueue_mark_qinstances(cqueue, answer_list, rem_hosts);
   }
   if (ret) {
      bool refresh_all_values = (rem_hosts != NULL) || (add_hosts != NULL);
      bool has_changed;
      bool is_ambiguous;

      ret &= cqueue_mod_qinstances(cqueue, answer_list, reduced_elem,
                                   refresh_all_values,
                                   &has_changed, &is_ambiguous);
      /* EB: TODO: evaluate is_ambiguous -> error? set queue instance state?*/
   }
   if (ret) {
      bool is_ambiguous;

      ret &= cqueue_add_qinstances(cqueue, answer_list, add_hosts,
                                   &is_ambiguous);
      /* EB: TODO: evaluate is_ambiguous -> error? set queue instance state? */
   }
   DEXIT;
   return ret;
}


bool 
cqueue_mod_qinstances(lListElem *cqueue, lList **answer_list,
                      lListElem *reduced_elem, bool refresh_all_values,
                      bool *is_ambiguous,
                      bool *has_changed)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_mod_qinstances");

   if (cqueue != NULL && reduced_elem != NULL) {
      int index = 0;

      while (cqueue_attribute_array[index].cqueue_attr != NoName && ret) {
         if (cqueue_attribute_array[index].is_sgeee_attribute == false ||
             feature_is_enabled(FEATURE_SGEEE)) {
            int pos = lGetPosViaElem(reduced_elem,
                                     cqueue_attribute_array[index].cqueue_attr);

            if (pos >= 0 || refresh_all_values) {
               lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
               lListElem *qinstance = NULL;

               for_each(qinstance, qinstance_list) {
                  if (lGetUlong(qinstance, QI_tag) != SGE_QI_TAG_DEL) {
                     const char *hostname = lGetHost(qinstance, QI_hostname);
                     bool tmp_is_ambiguous = false;
                     bool tmp_has_changed = false;

                     ret &= qinstance_modify_attribute(qinstance,
                                answer_list, cqueue,
                                cqueue_attribute_array[index].qinstance_attr,
                                cqueue_attribute_array[index].cqueue_attr,
                                cqueue_attribute_array[index].href_attr,
                                cqueue_attribute_array[index].value_attr,
                                cqueue_attribute_array[index].primary_key_attr,
                                &tmp_is_ambiguous, &tmp_has_changed);
                     if (!ret) {
                        break;
                     }
                     if (tmp_has_changed) {
                        lSetUlong(qinstance, QI_tag, SGE_QI_TAG_MOD);
                        DPRINTF(("Qinstance of host "SFQ" has been changed\n", hostname));
                        if (has_changed != NULL) {
                           *has_changed = true;
                        }
                     }
                     if (tmp_is_ambiguous) {
                        DPRINTF(("Qinstance of host "SFQ" has been ambiguous configuration\n", hostname));
                        if (is_ambiguous != NULL) {
                           *is_ambiguous = true;
                        }
                        /* EB: TODO: set CA state */
                     }
                  }
               }
            }
         }
         index++;
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_mark_qinstances(lListElem *cqueue, lList **answer_list, lList *del_hosts)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_mark_qinstances");
   if (cqueue != NULL && del_hosts != NULL) {
      lListElem *href = NULL;

      for_each(href, del_hosts) {
         const char *hostname = lGetHost(href, HR_name);
         lList *list = lGetList(cqueue, CQ_qinstances);
         lListElem* qinstance = lGetElemHost(list, QI_hostname, hostname);

         if (qinstance != NULL) {
            /*
             * We cannot remove the qinstance object here because we
             * need it later on for spooling and event handling:
             *    - QI spoolfiles have to be removed
             *    - delete events for QI objects
             */
            lSetUlong(qinstance, QI_tag, SGE_QI_TAG_DEL);
            DPRINTF(("qinstance "SFN" tagged for deletion\n", hostname));
         } else {
            DPRINTF(("qinstance "SFN" not found for deletion\n", hostname));
         }
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_add_qinstances(lListElem *cqueue, lList **answer_list, lList *add_hosts,
                      bool *is_ambiguous)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_add_qinstances");
   if (cqueue != NULL && add_hosts != NULL) {
      lListElem *href = NULL;

      for_each(href, add_hosts) {
         const char *hostname = lGetHost(href, HR_name);
         lList *list = lGetList(cqueue, CQ_qinstances);
         lListElem* new_qinstance;
         bool tmp_is_ambiguous = false;

         if (list == NULL) {
            list = lCreateList("", QI_Type);
            lSetList(cqueue, CQ_qinstances, list);
         }
         new_qinstance = qinstance_create(cqueue, answer_list,
                                          hostname, &tmp_is_ambiguous);
         if (tmp_is_ambiguous) {
            DPRINTF(("qinstance %s has ambiguous configuaration\n", hostname));
            /* EB: TODO: Set ambiguous state */
            if (is_ambiguous != NULL) { 
               *is_ambiguous = true;
            }
         }
         lSetUlong(new_qinstance, QI_tag, SGE_QI_TAG_ADD);
         lAppendElem(list, new_qinstance);
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_mod_attributes(lListElem *cqueue, lList **answer_list,
                      lListElem *reduced_elem, int sub_command)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_mod_attributes");
   if (cqueue != NULL && reduced_elem != NULL) {
      int index = 0;

      while (cqueue_attribute_array[index].cqueue_attr != NoName && ret) {
         int pos = lGetPosViaElem(reduced_elem,
                                  cqueue_attribute_array[index].cqueue_attr);

         if (pos >= 0) {
            ret &= cqueue_mod_sublist(cqueue, answer_list, reduced_elem,
                             sub_command,
                             cqueue_attribute_array[index].cqueue_attr,
                             cqueue_attribute_array[index].href_attr,
                             cqueue_attribute_array[index].value_attr,
                             cqueue_attribute_array[index].primary_key_attr,
                             cqueue_attribute_array[index].name,
                             SGE_OBJ_CQUEUE);
         }
         index++;
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_attributes(lListElem *cqueue, lList **answer_list,
                         lListElem *reduced_elem, bool in_master)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_verify_attributes");
   if (cqueue != NULL && reduced_elem != NULL) {
      int index = 0;

      while (cqueue_attribute_array[index].cqueue_attr != NoName && ret) {
         if (!cqueue_attribute_array[index].is_sgeee_attribute ||
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

               if (ret && 
                   cqueue_attribute_array[index].verify_function != NULL &&
                   (cqueue_attribute_array[index].verify_client || in_master)) {
                  for_each(elem, list) {
                     ret &= cqueue_attribute_array[index].
                                    verify_function(cqueue, answer_list, elem);
                  }
               }
            }
         }
         index++;
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_priority(lListElem *cqueue, lList **answer_list, 
                       lListElem *attr_elem)
{
   bool ret = true;
   
   DENTER(CQUEUE_LAYER, "cqueue_verify_priority");
   if (cqueue != NULL && attr_elem != NULL) {
      const char *priority_string = lGetString(attr_elem, ASTR_value);

      if (priority_string != NULL) {
         const int priority = atoi(priority_string);

         if (priority < -20 || priority > 20 ) {
            answer_list_add(answer_list, MSG_CQUEUE_PRIORITYNOTINRANGE, 
                            STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            ret = false;
         }
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_pe_list(lListElem *cqueue, lList **answer_list, 
                       lListElem *attr_elem)
{
   bool ret = true;
   
   DENTER(CQUEUE_LAYER, "cqueue_verify_pe_list");
   if (cqueue != NULL && attr_elem != NULL) {
      lList *pe_list = lGetList(attr_elem, ASTRLIST_value);

      if (pe_list != NULL) {
         const lList *master_list = *(pe_list_get_master_list());

         if (!pe_list_do_all_exist(master_list, answer_list, pe_list)) {
            ret = false;
         }
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_ckpt_list(lListElem *cqueue, lList **answer_list, 
                        lListElem *attr_elem)
{
   bool ret = true;
   
   DENTER(CQUEUE_LAYER, "cqueue_verify_ckpt_list");
   if (cqueue != NULL && attr_elem != NULL) {
      lList *ckpt_list = lGetList(attr_elem, ASTRLIST_value);

      if (ckpt_list != NULL) {
         const lList *master_list = *(ckpt_list_get_master_list());

         if (!ckpt_list_do_all_exist(master_list, answer_list, ckpt_list)) {
            ret = false;
         }
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_user_list(lListElem *cqueue, lList **answer_list, 
                        lListElem *attr_elem)
{
   bool ret = true;
   
   DENTER(CQUEUE_LAYER, "cqueue_verify_user_list");
   if (cqueue != NULL && attr_elem != NULL) {
      lList *user_list = lGetList(attr_elem, AUSRLIST_value);

      if (user_list != NULL) {
         if (userset_list_validate_acl_list(user_list, answer_list)) {
            ret = false;
         }
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_verify_project_list(lListElem *cqueue, lList **answer_list,
                           lListElem *attr_elem)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_verify_project_list");
   if (cqueue != NULL && attr_elem != NULL) {
      lList *project_list = lGetList(attr_elem, APRJLIST_value);

      if (project_list != NULL) {
         const lList *master_list = *(prj_list_get_master_list());

         if (!prj_list_do_all_exist(master_list, answer_list, project_list)) {
            ret = false;
         }
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_calendar(lListElem *cqueue, lList **answer_list, 
                       lListElem *attr_elem)
{
   bool ret = true;
   
   DENTER(CQUEUE_LAYER, "cqueue_verify_calendar");
   if (cqueue != NULL && attr_elem != NULL) {
      const char *name = lGetString(attr_elem, ASTR_value);

      if (name != NULL && strcasecmp("none", name)) {
         lListElem *calendar = calendar_list_locate(Master_Calendar_List, name);

         if (calendar == NULL) {
            sprintf(SGE_EVENT, MSG_CQUEUE_UNKNOWNCALENDAR_S, name);
            answer_list_add(answer_list, SGE_EVENT, 
                            STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            ret = false;
         }
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_initial_state(lListElem *cqueue, lList **answer_list, 
                            lListElem *attr_elem)
{
   bool ret = true;
   
   DENTER(CQUEUE_LAYER, "cqueue_verify_initial_state");
   if (cqueue != NULL && attr_elem != NULL) {
      const char *names[] = {"default", "enabled", "disabled", NULL};
      const char *name = lGetString(attr_elem, ASTR_value);
      bool found = false;
      int i = 0;

      while (names[i] != NULL) {
         if (!strcasecmp(name, names[i])) {
            found = true;
         }
         i++;
      }
      if (!found) {
         sprintf(SGE_EVENT, MSG_CQUEUE_UNKNOWNINITSTATE_S, name);
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         ret = false;
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_shell_start_mode(lListElem *cqueue, lList **answer_list, 
                               lListElem *attr_elem)
{
   bool ret = true;
   
   DENTER(CQUEUE_LAYER, "cqueue_verify_shell_start_mode");
   if (cqueue != NULL && attr_elem != NULL) {
      const char *names[] = {
         "unix_behavior", "posix_compliant", "script_from_stdin", 
         NULL
      };
      const char *name = lGetString(attr_elem, ASTR_value);
      bool found = false;
      int i = 0;

      while (names[i] != NULL) {
         if (!strcasecmp(name, names[i])) {
            found = true;
         }
         i++;
      }
      if (!found) {
         sprintf(SGE_EVENT, MSG_CQUEUE_UNKNOWNSTARTMODE_S, name);
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         ret = false;
      }
   }
   DEXIT;
   return ret;
}
