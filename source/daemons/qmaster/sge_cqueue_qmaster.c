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
#include "sge_event_master.h"
#include "sge_persistence_qmaster.h"

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

   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_seq_no);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_seq_no, lCopyList("", list));
      }
   }

   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_nsuspend);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_nsuspend, lCopyList("", list));
      }
   }

   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_job_slots);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_job_slots, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_fshare);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_fshare, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_oticket);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_oticket, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_rerun);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_rerun, lCopyList("", list));
      }
   }

   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_s_fsize);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_s_fsize, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_h_fsize);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_h_fsize, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_s_data);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_s_data, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_h_data);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_h_data, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_s_stack);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_s_stack, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_h_stack);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_h_stack, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_s_core);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_s_core, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_h_core);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_h_core, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_s_rss);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_s_rss, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_h_rss);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_h_rss, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_s_vmem);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_s_vmem, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_h_vmem);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_h_vmem, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_s_rt);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_s_rt, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_h_rt);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_h_rt, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_s_cpu);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_s_cpu, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_h_cpu);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_h_cpu, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_suspend_interval);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_suspend_interval, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_min_cpu_interval);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_min_cpu_interval, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_notify);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_notify, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_tmpdir);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_tmpdir, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_pe_list);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_pe_list, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_ckpt_list);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_ckpt_list, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_owner_list);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_owner_list, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_acl);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_acl, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_xacl);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_xacl, lCopyList("", list));
      }
   }
   
   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_projects);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);

         lSetList(cqueue, CQ_projects, lCopyList("", list));
      }
   }

   if (ret) {
      DTRACE;
      pos = lGetPosViaElem(reduced_elem, CQ_xprojects);

      if (pos >= 0) {
         lList *list = lGetPosList(reduced_elem, pos);
   
         lSetList(cqueue, CQ_xprojects, lCopyList("", list));
      }
   }
   
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

