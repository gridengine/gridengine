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
#include "sge_str.h"
#include "sge_event_master.h"
#include "sge_persistence_qmaster.h"
#include "sge_attr.h"
#include "sge_userprj.h"
#include "sge_feature.h"
#include "sge_cqueue_qmaster.h"

#include "spool/classic/read_write_ume.h"
#include "spool/sge_spooling.h"

#include "msg_common.h"
#include "msg_qmaster.h"

int cqueue_mod(lList **answer_list, lListElem *cqueue, lListElem *reduced_elem, 
               int add, const char *remote_user, const char *remote_host,
               gdi_object_t *object, int sub_command) 
{
#define CQUEUE_MOD_DEBUG
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
    */
   if (ret) {
      ret &= cqueue_mod_hostlist(cqueue, answer_list, reduced_elem,
                                 sub_command, &add_hosts, &rem_hosts);
   }

   /*
    * Its time to do the cqueue modifications:
    *    - change the attribute lists in the cqueue object
    *    - verify the attribute lists
    */
   if (ret) {
      ret &= cqueue_mod_attributes(cqueue, answer_list, 
                                   reduced_elem, sub_command);
   }
   if (ret) {
      ret &= cqueue_verify_attributes(cqueue, answer_list, 
                                      reduced_elem, true);
   }

   /*
    * Now we have to add/mod/del all qinstances
    */ 
   if (ret) {
      ret &= cqueue_handle_qinstances(cqueue, answer_list, reduced_elem, 
                                      add_hosts, rem_hosts);
   }

   /*
    * Cleanup
    */
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
   DENTER(TOP_LAYER, "cqueue_success");
   
   /*
    * CQ modify or add event
    */
   sge_add_event(NULL, 0, old_cqueue?sgeE_CQUEUE_MOD:sgeE_CQUEUE_ADD, 0, 0, 
                 lGetString(cqueue, CQ_name), NULL, NULL, cqueue);

   /*
    * QI modify, add or delete event. Finalize operation.
    */
   cqueue_commit(cqueue);

   DEXIT;
   return 0;
}

void cqueue_rollback(lListElem *cqueue) 
{
   lList *qinstances = lGetList(cqueue, CQ_qinstances);
   lListElem *next_qinstance = NULL;
   lListElem *qinstance = NULL;

   DENTER(TOP_LAYER, "cqueue_rollback"); 

   next_qinstance = lFirst(qinstances);
   while ((qinstance = next_qinstance)) {
      u_long32 tag = lGetUlong(qinstance, QI_tag);

      next_qinstance = lNext(qinstance);

      /*
       * Reset QI tag
       */
      lSetUlong(qinstance, QI_tag, SGE_QI_TAG_DEFAULT);

      if (tag == SGE_QI_TAG_DEL) {
         /*
          * Tag has been reset some lines above.
          * There is nothing else to do.
          */
         ;
      } else if (tag == SGE_QI_TAG_ADD) {
         /*
          * Trash created qinstances
          */
         lRemoveElem(qinstances, qinstance);
      }
   }
   if (lGetNumberOfElem(qinstances) == 0) {
      lSetList(cqueue, CQ_qinstances, NULL);
   }
   DEXIT;
}
void cqueue_commit(lListElem *cqueue) 
{
   lList *qinstances = lGetList(cqueue, CQ_qinstances);
   lListElem *next_qinstance = NULL;
   lListElem *qinstance = NULL;

   DENTER(TOP_LAYER, "cqueue_commit"); 

   /*
    * QI modify, add or delete event
    */
   next_qinstance = lFirst(qinstances);
   while ((qinstance = next_qinstance)) {
      u_long32 tag = lGetUlong(qinstance, QI_tag);
      const char *name = lGetString(qinstance, QI_name);
      const char *hostname = lGetHost(qinstance, QI_hostname);

      next_qinstance = lNext(qinstance);

      /*
       * Reset QI tag
       */
      lSetUlong(qinstance, QI_tag, SGE_QI_TAG_DEFAULT);

      if (tag == SGE_QI_TAG_ADD) {
         sge_add_event(NULL, 0, sgeE_QINSTANCE_ADD, 0, 0,
                       name, hostname, NULL, qinstance);
      } else if (tag == SGE_QI_TAG_MOD) {
         sge_add_event(NULL, 0, sgeE_QINSTANCE_MOD, 0, 0,
                       name, hostname, NULL, qinstance);
      } else if (tag == SGE_QI_TAG_DEL) {
         sge_add_event(NULL, 0, sgeE_QINSTANCE_DEL, 0, 0,
                       name, hostname, NULL, NULL);

         /*
          * Now we can remove the qinstance.
          */
         lRemoveElem(qinstances, qinstance);
      }
   }
   if (lGetNumberOfElem(qinstances) == 0) {
      lSetList(cqueue, CQ_qinstances, NULL);
   }
   DEXIT;
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
            lList *qinstances = lGetList(cqueue, CQ_qinstances);
            lListElem *qinstance = NULL;
            const char *cq_name = lGetString(cqueue, CQ_name);
            dstring dir = DSTRING_INIT;

            for_each(qinstance, qinstances) {
               dstring key = DSTRING_INIT;
               const char *qi_name = lGetHost(qinstance, QI_hostname);

               sge_dstring_sprintf(&key, "%s/%s", cq_name, qi_name); 
               sge_dstring_sprintf(&dir, "%s/%s", QINSTANCES_DIR, cq_name); 
               if (sge_event_spool(answer_list, 0, sgeE_QINSTANCE_DEL,
                                   0, 0, sge_dstring_get_string(&key), 
                                   NULL, NULL, NULL, NULL, NULL, true, true)) {
                  ; /* EB: TODO: Ask JG what we can do here? */
               }
               sge_dstring_free(&key);
            }
            sge_rmdir(sge_dstring_get_string(&dir), NULL);
            sge_dstring_free(&dir);
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


