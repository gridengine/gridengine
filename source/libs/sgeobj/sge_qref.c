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

#include "basis_types.h"
#include "sgermon.h" 
#include "sge_string.h"
#include "sge_str.h"
#include "sge_log.h"
#include "sge_hostname.h"

#include "sge_answer.h"
#include "sge_centry.h"
#include "sge_cqueue.h"
#include "sge_href.h"
#include "sge_hgroup.h"
#include "sge_object.h"
#include "sge_qinstance.h"
#include "sge_qref.h"
#include "commlib.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#define QREF_LAYER TOP_LAYER

static bool
qref_list_resolve_cqueue_names(const lList *cq_qref_list, 
                               lList **answer_list,
                               lList **qref_list, 
                               bool *found_something,
                               const lList *cqueue_list,
                               bool resolve_cqueue);

static bool
qref_list_resolve_qinstance_names(const lList *cq_qref_list, 
                                  lList **answer_list,
                                  dstring *host_or_hgroup,
                                  lList **qref_list, 
                                  bool *found_something,
                                  const lList *cqueue_list);

static bool
qref_list_resolve_qdomain_names(const lList *cq_qref_list,
                                lList **answer_list,
                                dstring *host_or_hgroup,
                                lList **qref_list,
                                bool *found_something,
                                const lList *cqueue_list,
                                const lList *hgroup_list,
                                bool resolve_qdomain);

static bool
qref_list_resolve_cqueue_names(const lList *cq_qref_list, 
                               lList **answer_list,
                               lList **qref_list, 
                               bool *found_something,
                               const lList *cqueue_list,
                               bool resolve_cqueue)
{
   bool ret = true;
   const lListElem *cq_qref = NULL;

   DENTER(QREF_LAYER, "qref_list_resolve_cqueue_names");
   for_each(cq_qref, cq_qref_list) {
      const char *cq_name = lGetString(cq_qref, QR_name);

      if (resolve_cqueue) {
         const lListElem *cqueue = NULL;
         const lList *qinstance_list = NULL;
         const lListElem *qinstance = NULL;
        
         cqueue = lGetElemStr(cqueue_list, CQ_name, cq_name); 
         qinstance_list = lGetList(cqueue, CQ_qinstances);
         for_each(qinstance, qinstance_list) {
            dstring buffer = DSTRING_INIT;
            const char *qi_name = qinstance_get_name(qinstance, &buffer);

            lAddElemStr(qref_list, QR_name, qi_name, QR_Type);
            sge_dstring_free(&buffer);
            *found_something = true;
         }
      } else {
         lAddElemStr(qref_list, QR_name, cq_name, QR_Type);
      }
   }
   DEXIT;
   return ret;
}

static bool
qref_list_resolve_qinstance_names(const lList *cq_qref_list, 
                                  lList **answer_list,
                                  dstring *host_or_hgroup,
                                  lList **qref_list, 
                                  bool *found_something,
                                  const lList *cqueue_list)
{
   bool ret = true;
   const lListElem *cq_qref = NULL;

   DENTER(QREF_LAYER, "qref_list_resolve_qinstance_names");
   for_each(cq_qref, cq_qref_list) {
      const char *cqueue_name = NULL;
      const char *hostname_pattern = NULL;
      const lListElem *cqueue = NULL;
      const lList *qinstance_list = NULL;
      lList *qi_ref_list = NULL;
      lListElem *qi_qref = NULL;

      cqueue_name = lGetString(cq_qref, QR_name);
      cqueue = lGetElemStr(cqueue_list, CQ_name, cqueue_name);
      hostname_pattern = sge_dstring_get_string(host_or_hgroup);
      qinstance_list = lGetList(cqueue, CQ_qinstances);
      qinstance_list_find_matching(qinstance_list, answer_list,
                                   hostname_pattern, &qi_ref_list);

      for_each(qi_qref, qi_ref_list) {
         const char *qi_name = lGetString(qi_qref, QR_name);

         lAddElemStr(qref_list, QR_name, qi_name, QR_Type);
         *found_something = true;
      }
      qi_ref_list = lFreeList(qi_ref_list);
   }
   DEXIT;
   return ret;
}

static bool
qref_list_resolve_qdomain_names(const lList *cq_qref_list, 
                                lList **answer_list,
                                dstring *host_or_hgroup,
                                lList **qref_list,
                                bool *found_something,
                                const lList *cqueue_list,
                                const lList *hgroup_list,
                                bool resolve_qdomain)
{
   bool ret = true;
   const char *hgroup_pattern = NULL;
   lList *href_list = NULL;
   lListElem *cq_qref = NULL;

   DENTER(QREF_LAYER, "qref_list_resolve_qdomain_names");
   hgroup_pattern = sge_dstring_get_string(host_or_hgroup);
   /*
    * Find all hostgroups which match 'hgroup_pattern'
    * Possibly resolve them.
    */
   if (resolve_qdomain) {
      hgroup_list_find_matching_and_resolve(hgroup_list, answer_list,
                                            hgroup_pattern, &href_list);
   } else {
      hgroup_list_find_matching(hgroup_list, answer_list,
                                hgroup_pattern, &href_list);
   }
   for_each(cq_qref, cq_qref_list) {
      const char *cqueue_name = lGetString(cq_qref, QR_name);
      const lListElem *cqueue = NULL;
      const lList *qinstance_list = NULL;
      const lListElem *href = NULL;

      cqueue = lGetElemStr(cqueue_list, CQ_name, cqueue_name);
      qinstance_list = lGetList(cqueue, CQ_qinstances);
      for_each(href, href_list) {
         if (resolve_qdomain) {
            const char *hostname = lGetHost(href, HR_name);
            const lListElem *qinstance = NULL;

            qinstance = lGetElemHost(qinstance_list, 
                                     QU_qhostname, hostname);
            if (qinstance != NULL) {
               dstring buffer = DSTRING_INIT;
               const char *qinstance_name = NULL;

               qinstance_name = qinstance_get_name(qinstance, 
                                                   &buffer);
               lAddElemStr(qref_list, QR_name, 
                           qinstance_name, QR_Type);
               sge_dstring_free(&buffer);
               *found_something = true;
            }
         } else {
            dstring buffer = DSTRING_INIT;
            const char *hgroup_name = lGetHost(href, HR_name);
            const char *qinstance_name = NULL;

            qinstance_name = sge_dstring_sprintf(&buffer, SFN"@"SFN,
                                                 cqueue_name, hgroup_name);
            lAddElemStr(qref_list, QR_name,
                        qinstance_name, QR_Type);
            sge_dstring_free(&buffer);
            *found_something = true;
         }
      }
   }
   href_list = lFreeList(href_list);
   DEXIT;
   return ret;
}

/****** sgeobj/qref/qref_list_add() *******************************************
*  NAME
*     qref_list_add() -- Add a queue reference to the list 
*
*  SYNOPSIS
*     bool 
*     qref_list_add(lList **this_list, 
*                   lList **answer_list, 
*                   const char *qref_string) 
*
*  FUNCTION
*     Add the queue reference "qref_string" to the QR_type list "this_list".
*     Errors will be reported via return value and "answer_list".  
*
*  INPUTS
*     lList **this_list       - QR_Type 
*     lList **answer_list     - AN_Type 
*     const char *qref_string - queue reference 
*
*  RESULT
*     bool - error state
*        true  - success
*        false - error
*******************************************************************************/
bool
qref_list_add(lList **this_list, lList **answer_list, const char *qref_string)
{
   bool ret = true;

   DENTER(QREF_LAYER, "qref_list_add");
   if (this_list != NULL && qref_string != NULL) {
      lListElem *new_elem; 

      new_elem = lAddElemStr(this_list, QR_name, qref_string, QR_Type);
      if (new_elem == NULL) {
         answer_list_add(answer_list, MSG_GDI_OUTOFMEMORY,
                         STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
         ret = false;
      }
   } else {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_INAVLID_PARAMETER_IN_S, SGE_FUNC));
      answer_list_add(answer_list, SGE_EVENT,
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/qref/qref_list_resolve() ****************************************
*  NAME
*     qref_list_resolve() -- resolves a list of queue reference patterns 
*
*  SYNOPSIS
*     bool 
*     qref_list_resolve(const lList *src_qref_list, 
*                       lList **answer_list, 
*                       lList **qref_list, 
*                       bool *found_something, 
*                       const lList *cqueue_list, 
*                       const lList *hgroup_list, 
*                       bool resolve_cqueue, 
*                       bool resolve_qdomain) 
*
*  FUNCTION
*     Resolves a list of queue reference patterns. "src_qref_list"
*     is the list of input patterns to be resolved. "qref_list" contains
*     all resolved names which matched at least one of the given
*     patterns. The master lists "cqueue_list" and "hgroup_list" are
*     needed to resolve the pattern. "resolve_cqueue" and 
*     "resolve_qdomain" can be used to define how qreferenes are 
*     resolved.
*
*     Examples:
*
*        <CQ-pattern> (e.g. "*")
*           resolve_cqueue == false 
*              => cq1 cq2
*           resolve_cqueue == true 
*              => cq1@hostA1 cq1@hostA2 cq1@hostB1 cq1@hostB2
*                 cq2@hostA1 cq2@hostA2 cq2@hostB1 cq2@hostB2
*
*        <QD-pattern> (e.q "*@@hgrp*")
*           resolve_qdomain == false
*              => cq1@@hgrpA cq1@@hgrpB cq2@@hgrpA cq2@@hgrpB
*           resolve_qdomain == true
*              => cq1@hostA1 cq1@hostA2 cq1@hostB1 cq1@hostB2
*                 cq2@hostA1 cq2@hostA2 cq2@hostB1 cq2@hostB2
*
*        <QI-pattern> (e.g "cq*@host?1")
*              => cq1@hostA1 cq1@hostB1 
*                 cq2@hostA1 cq2@hostB1
*
*  INPUTS
*     const lList *src_qref_list - QR_Type list (input: pattern) 
*     lList **answer_list        - AN_Type list 
*     lList **qref_list          - QR_Type list (output: resolved qrefs)
*     bool *found_something      - a pattern matched? (output) 
*     const lList *cqueue_list   - master CQ_Type list 
*     const lList *hgroup_list   - master HGRP_Type list 
*     bool resolve_cqueue        - resolve cqueue pattern?
*     bool resolve_qdomain       - resolve qdomain pattern? 
*
*  RESULT
*     bool - error state
*        true  - success
*        false - error
*******************************************************************************/
bool
qref_list_resolve(const lList *src_qref_list, lList **answer_list, 
                  lList **qref_list, bool *found_something,
                  const lList *cqueue_list, const lList *hgroup_list, 
                  bool resolve_cqueue, bool resolve_qdomain)
{
   bool ret = true;
   DENTER(QREF_LAYER, "qref_list_resolve");

   if (src_qref_list != NULL) {
      lListElem *qref_pattern = NULL;

      *found_something = false;
      for_each(qref_pattern, src_qref_list) {
         dstring cqueue_name = DSTRING_INIT;
         dstring host_or_hgroup = DSTRING_INIT;
         const char *name = NULL;
         bool has_hostname;
         bool has_domain;
         const char *cq_pattern = NULL;
         lList *cq_ref_list = NULL;
         bool tmp_found_something = false;
 
         /*
          * Find all existing parts of the qref-pattern
          */ 
         name = lGetString(qref_pattern, QR_name); 
         cqueue_name_split(name, &cqueue_name, &host_or_hgroup,
                           &has_hostname, &has_domain);
         cq_pattern = sge_dstring_get_string(&cqueue_name);

         /*
          * Find all CQ names which match 'cq_pattern' 
          */
         cqueue_list_find_all_matching_references(cqueue_list, answer_list,
                                                  cq_pattern, &cq_ref_list);

         /*
          * Depending on the type of pattern -> resolve QC or QI names
          */
         if (has_domain) {
            ret &= qref_list_resolve_qdomain_names(cq_ref_list, answer_list,
                                                   &host_or_hgroup, qref_list,
                                                   &tmp_found_something,
                                                   cqueue_list, hgroup_list,
                                                   resolve_qdomain);
         } else if (has_hostname) {
            ret &= qref_list_resolve_qinstance_names(cq_ref_list, answer_list,
                                                     &host_or_hgroup, qref_list,
                                                     &tmp_found_something, 
                                                     cqueue_list); 
         } else {
            ret &= qref_list_resolve_cqueue_names(cq_ref_list, answer_list,
                                                  qref_list, 
                                                  &tmp_found_something,
                                                  cqueue_list, 
                                                  resolve_qdomain);
         }
         if (tmp_found_something) {
            *found_something = true;
         } 
         cq_ref_list = lFreeList(cq_ref_list);
         sge_dstring_free(&host_or_hgroup);
         sge_dstring_free(&cqueue_name);
      } 
   }
   DEXIT;
   return ret;
}

/****** sgeobj/qref/qref_list_trash_some_elemts() *****************************
*  NAME
*     qref_list_trash_some_elemts() -- Remove some elements from list 
*
*  SYNOPSIS
*     bool 
*     qref_list_trash_some_elemts(lList **this_list, 
*                                 const char *full_name) 
*
*  FUNCTION
*     Remove all elements from "this_list" where either the cluster
*     queue name is equivalent with the cluster queue part of "fullname"
*     or if the hostname is different from the hostname part of 
*     "fullname"
*
*  INPUTS
*     lList **this_list     - QR_Type 
*     const char *full_name - queue instance name  
*
*  RESULT
*     bool - error state
*        true  - success
*        false - error
*******************************************************************************/
bool
qref_list_trash_some_elemts(lList **this_list, const char *full_name)
{
   bool ret = true;

   DENTER(TOP_LAYER, "qref_list_trash_some_elemts");
   if (this_list != NULL) {
      lListElem *qref = NULL;
      lListElem *next_qref = NULL;
      dstring cqueue_buffer = DSTRING_INIT;
      dstring host_or_hgroup_buffer = DSTRING_INIT;
      dstring cqueue_buffer1 = DSTRING_INIT;
      dstring host_or_hgroup_buffer1 = DSTRING_INIT;
      bool has_hostname1;
      bool has_domain1;
      const char *cqueue1 = NULL;
      const char *host1 = NULL;

      cqueue_name_split(full_name, &cqueue_buffer1, &host_or_hgroup_buffer1,
                        &has_hostname1, &has_domain1);
      cqueue1 = sge_dstring_get_string(&cqueue_buffer1);
      host1 = sge_dstring_get_string(&host_or_hgroup_buffer1);

      next_qref = lFirst(*this_list);
      while ((qref = next_qref) != NULL) {
         bool has_hostname;
         bool has_domain;
         const char *name = NULL;
         const char *cqueue = NULL;
         const char *host = NULL;

         next_qref = lNext(qref);

         name = lGetString(qref, QR_name);
         cqueue_name_split(name, &cqueue_buffer, &host_or_hgroup_buffer,
                           &has_hostname, &has_domain);
         cqueue = sge_dstring_get_string(&cqueue_buffer);
         host = sge_dstring_get_string(&host_or_hgroup_buffer);

         /*
          * Same cluster queue or different host?
          */
         if (!strcmp(cqueue1, cqueue) || strcmp(host1, host)) {
            lRemoveElem(*this_list, qref);
         }

         sge_dstring_clear(&cqueue_buffer);
         sge_dstring_clear(&host_or_hgroup_buffer);
      }
      if (lGetNumberOfElem(*this_list) == 0) {
         *this_list = lFreeList(*this_list);
      }

      sge_dstring_free(&cqueue_buffer);
      sge_dstring_free(&host_or_hgroup_buffer);
      sge_dstring_free(&cqueue_buffer1);
      sge_dstring_free(&host_or_hgroup_buffer1);
   }
   DEXIT;
   return ret;
}

/****** sgeobj/qref/qref_list_is_valid() **************************************
*  NAME
*     qref_list_is_valid() -- check queue reference list 
*
*  SYNOPSIS
*     bool 
*     qref_list_is_valid(const lList *this_list, lList **answer_list) 
*
*  FUNCTION
*     This function will be used to check the hard and soft queue list.
*     which will be defined during job submittion or redefined via
*     qalter and qmon.
*
*     This function will return successfull when:
*        - queues are requestable and
*        - if the contained queue-pattern matches at least one 
*          queue instance
*
*  INPUTS
*     const lList *this_list - QR_Type 
*     lList **answer_list    - AN_Type 
*
*  RESULT
*     bool - error state
*        true  - success
*        false - error
*******************************************************************************/
bool
qref_list_is_valid(const lList *this_list, lList **answer_list)
{
   bool ret = true;

   DENTER(TOP_LAYER, "qref_list_is_valid");
   if (this_list != NULL) {
      lList *master_cqueue_list = NULL;
      lList *master_hgroup_list = NULL;
      lList *master_centry_list = NULL;

      master_cqueue_list = *(object_type_get_master_list(SGE_TYPE_CQUEUE));
      master_hgroup_list = *(object_type_get_master_list(SGE_TYPE_HGROUP));
      master_centry_list = *(object_type_get_master_list(SGE_TYPE_CENTRY));

      /*
       * qname has to be requestable
       */
      if (centry_list_are_queues_requestable(master_centry_list)) {
         lListElem *qref_elem;

         /*
          * At least one qinstance has to exist for each pattern
          */
         for_each (qref_elem, this_list) {
            bool found_something = false;
            bool found_matching_qinstance = false;
            const char *qref_pattern = NULL;
            lList *resolved_qref_list = NULL;
            lList *qref_list = NULL;
            lListElem *resolved_qref = NULL;

            qref_resolve_hostname(qref_elem);
            qref_pattern = lGetString(qref_elem, QR_name);

            lAddElemStr(&qref_list, QR_name, qref_pattern, QR_Type);
            qref_list_resolve(qref_list, answer_list, &resolved_qref_list,
                              &found_something, master_cqueue_list,
                              master_hgroup_list, true, true);
            for_each(resolved_qref, resolved_qref_list) {
               const char *resolved_qref_name = NULL;

               resolved_qref_name = lGetString(resolved_qref, QR_name);
               if (cqueue_list_locate_qinstance(master_cqueue_list,
                                                resolved_qref_name) != NULL) {
                  found_matching_qinstance = true;
               }
            }
            qref_list = lFreeList(qref_list);
            resolved_qref_list = lFreeList(resolved_qref_list);
            if (!found_matching_qinstance) {
               ERROR((SGE_EVENT, MSG_QREF_QUNKNOWN_S, qref_pattern));
               answer_list_add(answer_list, SGE_EVENT,
                               STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
               ret = false;
            }
         }
      } else {
         ERROR((SGE_EVENT, MSG_QREF_QNOTREQUESTABLE));
         answer_list_add(answer_list, SGE_EVENT,
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         ret = false;
      }
   }
   DEXIT;
   return ret;
}

void
qref_list_resolve_hostname(lList *this_list) 
{
   lListElem *qref;

   DENTER(TOP_LAYER, "qref_list_resolve_hostname");
   for_each(qref, this_list) {
      qref_resolve_hostname(qref);
   }
   DEXIT;
}

/* QR_name might be a pattern */
void
qref_resolve_hostname(lListElem *this_elem)
{
   dstring cqueue_name = DSTRING_INIT;
   dstring host_or_hgroup = DSTRING_INIT;
   const char *name = NULL;
   const char *unresolved_name = NULL;
   bool has_hostname;
   bool has_domain;
   
   DENTER(TOP_LAYER, "qref_resolve_hostname");
   name = lGetString(this_elem, QR_name);
   cqueue_name_split(name, &cqueue_name, &host_or_hgroup,
                     &has_hostname, &has_domain);
   unresolved_name = sge_dstring_get_string(&host_or_hgroup);

   if (has_hostname && !sge_is_pattern(unresolved_name)) {
      char resolved_name[MAXHOSTLEN+1];
      int back = getuniquehostname(unresolved_name, resolved_name, 0);

      if (back == CL_RETVAL_OK) {
         dstring new_qref_pattern = DSTRING_INIT;
         if (sge_dstring_strlen(&cqueue_name) == 0) {
             sge_dstring_sprintf(&new_qref_pattern, "@%s",
                                 resolved_name);
         } else {
            sge_dstring_sprintf(&new_qref_pattern, "%s@%s",
                                sge_dstring_get_string(&cqueue_name),
                                resolved_name);
         }
         lSetString(this_elem, QR_name, 
                    sge_dstring_get_string(&new_qref_pattern));
         sge_dstring_free(&new_qref_pattern);
      }
   }
   sge_dstring_free(&cqueue_name);
   sge_dstring_free(&host_or_hgroup);
   DEXIT;
   return;
}

