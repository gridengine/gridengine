
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
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sgermon.h"
#include "sge.h"
#include "sge_cqueue.h"
#include "sge_stringL.h"
#include "sge_answer.h"
#include "read_write_cqueue.h"
#include "sge_string.h"
#include "sge_log.h"
#include "config.h"
#include "read_object.h"
#include "sge_stdio.h"
#include "sge_io.h"
#include "sge_conf.h"
#include "sge_attr.h"

#include "msg_common.h"

static int 
read_cqueue_work(lList **alpp, lList **clpp, int fields[], lListElem *ep, 
                 int spool, int flag, int *tag, int parsing_type);

lListElem *cull_read_in_cqueue(const char *dirname, const char *filename, 
                               int spool, int flag, int *tag, int fields[]) 
{  
   lListElem *ep;
   struct read_object_args args = { CQ_Type, "", read_cqueue_work };
   int intern_tag = 0;

   DENTER(TOP_LAYER, "cull_read_in_cqueue");

   ep = read_object(dirname, filename, spool, 0, 0,&args, 
                    tag?tag:&intern_tag, NULL);
  
   DEXIT;
   return ep;
}

/* ------------------------------------------------------------
   spool:
      1 write for spooling
      0 write only user controlled fields

*/
static int read_cqueue_work(
lList **alpp,   /* anser list */
lList **clpp,   /* parsed file */
int fields[],   /* not needed */
lListElem *ep,  /* list element to fill of type CU_Type */
int spool,      /* look above */
int flag,       /* user flag */
int *tag,       /* user return value */
int parsing_type 
) {
   int ret = 0;

   DENTER(TOP_LAYER, "read_cqueue_work");

   /* --------- CQ_name */
   if (ret == 0) {
      ret = (!set_conf_string(alpp, clpp, fields, "qname", ep, CQ_name)) ? -1 : 0;
   }

   /* --------- CQ_seq_no */
   if (ret == 0) {
      ret = (!set_conf_ulng_attr_list(alpp, clpp, fields, "seq_no", ep, 
                                      CQ_seq_no, AULNG_Type, 
                                      AULNG_href)) ? -1 : 0;
   }
   
   /* --------- CQ_nsuspend */
   if (ret == 0) {
      ret = (!set_conf_ulng_attr_list(alpp, clpp, fields, "nsuspend", ep, 
                                      CQ_nsuspend, AULNG_Type, 
                                      AULNG_href)) ? -1 : 0;
   }

   /* --------- CQ_job_slots */
   if (ret == 0) {
      ret = (!set_conf_ulng_attr_list(alpp, clpp, fields, "slots", ep, 
                                      CQ_job_slots, AULNG_Type, 
                                      AULNG_href)) ? -1 : 0;
   }

   /* --------- CQ_fshare */
   if (ret == 0) {
      ret = (!set_conf_ulng_attr_list(alpp, clpp, fields, "fshare", ep, 
                                      CQ_fshare, AULNG_Type, 
                                      AULNG_href)) ? -1 : 0;
   }

   /* --------- CQ_oticket */
   if (ret == 0) {
      ret = (!set_conf_ulng_attr_list(alpp, clpp, fields, "oticket", ep, 
                                      CQ_oticket, AULNG_Type, 
                                      AULNG_href)) ? -1 : 0;
   }

   /* --------- CQ_rerun */
   if (ret == 0) {
      ret = (!set_conf_bool_attr_list(alpp, clpp, fields, "rerun", ep, 
                                      CQ_rerun, ABOOL_Type, 
                                      ABOOL_href)) ? -1 : 0;
   }

   /* --------- CQ_s_fsize */
   if (ret == 0) {
      ret = (!set_conf_mem_attr_list(alpp, clpp, fields, "s_fsize", ep, 
                                     CQ_s_fsize, AMEM_Type,
                                     AMEM_href)) ? -1 : 0;
   }
   
   /* --------- CQ_h_fsize */
   if (ret == 0) {
      ret = (!set_conf_mem_attr_list(alpp, clpp, fields, "h_fsize", ep, 
                                     CQ_h_fsize, AMEM_Type,
                                     AMEM_href)) ? -1 : 0;
   }

   /* --------- CQ_s_data */
   if (ret == 0) {
      ret = (!set_conf_mem_attr_list(alpp, clpp, fields, "s_data", ep, 
                                     CQ_s_data, AMEM_Type,
                                     AMEM_href)) ? -1 : 0;
   }

   /* --------- CQ_h_data */
   if (ret == 0) {
      ret = (!set_conf_mem_attr_list(alpp, clpp, fields, "h_data", ep, 
                                     CQ_h_data, AMEM_Type,
                                     AMEM_href)) ? -1 : 0;
   }

   /* --------- CQ_s_stack */
   if (ret == 0) {
      ret = (!set_conf_mem_attr_list(alpp, clpp, fields, "s_stack", ep, 
                                     CQ_s_stack, AMEM_Type,
                                     AMEM_href)) ? -1 : 0;
   }

   /* --------- CQ_h_stack */
   if (ret == 0) {
      ret = (!set_conf_mem_attr_list(alpp, clpp, fields, "h_stack", ep, 
                                     CQ_h_stack, AMEM_Type,
                                     AMEM_href)) ? -1 : 0;
   }

   /* --------- CQ_s_core */
   if (ret == 0) {
      ret = (!set_conf_mem_attr_list(alpp, clpp, fields, "s_core", ep, 
                                     CQ_s_core, AMEM_Type,
                                     AMEM_href)) ? -1 : 0;
   }

   /* --------- CQ_h_core */
   if (ret == 0) {
      ret = (!set_conf_mem_attr_list(alpp, clpp, fields, "h_core", ep, 
                                     CQ_h_core, AMEM_Type,
                                     AMEM_href)) ? -1 : 0;
   }

   /* --------- CQ_s_rss */
   if (ret == 0) {
      ret = (!set_conf_mem_attr_list(alpp, clpp, fields, "s_rss", ep, 
                                     CQ_s_rss, AMEM_Type,
                                     AMEM_href)) ? -1 : 0;
   }

   /* --------- CQ_h_rss */
   if (ret == 0) {
      ret = (!set_conf_mem_attr_list(alpp, clpp, fields, "h_rss", ep, 
                                     CQ_h_rss, AMEM_Type,
                                     AMEM_href)) ? -1 : 0;
   }

   /* --------- CQ_s_vmem */
   if (ret == 0) {
      ret = (!set_conf_mem_attr_list(alpp, clpp, fields, "s_vmem", ep, 
                                     CQ_s_vmem, AMEM_Type,
                                     AMEM_href)) ? -1 : 0;
   }

   /* --------- CQ_h_vmem */
   if (ret == 0) {
      ret = (!set_conf_mem_attr_list(alpp, clpp, fields, "h_vmem", ep, 
                                     CQ_h_vmem, AMEM_Type,
                                     AMEM_href)) ? -1 : 0;
   }

   /* --------- CQ_s_rt */
   if (ret == 0) {
      ret = (!set_conf_time_attr_list(alpp, clpp, fields, "s_rt", ep, 
                                      CQ_s_rt, ATIME_Type,
                                      ATIME_href)) ? -1 : 0;
   }
   
   /* --------- CQ_h_rt */
   if (ret == 0) {
      ret = (!set_conf_time_attr_list(alpp, clpp, fields, "h_rt", ep, 
                                      CQ_h_rt, ATIME_Type,
                                      ATIME_href)) ? -1 : 0;
   }
   
   /* --------- CQ_s_cpu */
   if (ret == 0) {
      ret = (!set_conf_time_attr_list(alpp, clpp, fields, "s_cpu", ep, 
                                      CQ_s_cpu, ATIME_Type,
                                      ATIME_href)) ? -1 : 0;
   }
   
   /* --------- CQ_h_cpu */
   if (ret == 0) {
      ret = (!set_conf_time_attr_list(alpp, clpp, fields, "h_cpu", ep, 
                                      CQ_h_cpu, ATIME_Type,
                                      ATIME_href)) ? -1 : 0;
   }
   
   /* --------- CQ_suspend_interval */
   if (ret == 0) {
      ret = (!set_conf_inter_attr_list(alpp, clpp, fields, "suspend_interval", 
                                       ep, CQ_suspend_interval, AINTER_Type,
                                       AINTER_href)) ? -1 : 0;
   }

   /* --------- CQ_min_cpu_interval */
   if (ret == 0) {
      ret = (!set_conf_inter_attr_list(alpp, clpp, fields, "min_cpu_interval", 
                                       ep, CQ_suspend_interval, AINTER_Type,
                                       AINTER_href)) ? -1 : 0;
   }

   /* --------- CQ_notify */
   if (ret == 0) {
      ret = (!set_conf_inter_attr_list(alpp, clpp, fields, "notify", ep, 
                                       CQ_suspend_interval, AINTER_Type,
                                       AINTER_href)) ? -1 : 0;
   }

   /* --------- CQ_tmpdir */
   if (ret == 0) {
      ret = (!set_conf_str_attr_list(alpp, clpp, fields, "tmpdir", ep,
                                     CQ_tmpdir, AINTER_Type,
                                     AINTER_href)) ? -1 : 0;
   }

   /* --------- CQ_pe_list */
   if (ret == 0) {
      ret = (!set_conf_strlist_attr_list(alpp, clpp, fields, "pe_list", ep,
                                         CQ_pe_list, ASTRLIST_Type, 
                                         ASTRLIST_href)) ? -1 : 0;
   }
   
   /* --------- CQ_ckpt_list */
   if (ret == 0) {
      ret = (!set_conf_strlist_attr_list(alpp, clpp, fields, "ckpt_list", ep,
                                         CQ_ckpt_list, ASTRLIST_Type, 
                                         ASTRLIST_href)) ? -1 : 0;
   }

   /* --------- CQ_owner_list */
   if (ret == 0) {
      ret = (!set_conf_usrlist_attr_list(alpp, clpp, fields, "owner_list", ep,
                                         CQ_owner_list, AUSRLIST_Type, 
                                         AUSRLIST_href)) ? -1 : 0;
   }

   /* --------- CQ_acl */
   if (ret == 0) {
      ret = (!set_conf_usrlist_attr_list(alpp, clpp, fields, "user_lists", ep,
                                         CQ_acl, AUSRLIST_Type, 
                                         AUSRLIST_href)) ? -1 : 0;
   }

   /* --------- CQ_xacl */
   if (ret == 0) {
      ret = (!set_conf_usrlist_attr_list(alpp, clpp, fields, "xuser_lists", ep,
                                         CQ_xacl, AUSRLIST_Type, 
                                         AUSRLIST_href)) ? -1 : 0;
   }

   /* --------- CQ_projects */
   if (ret == 0) {
      ret = (!set_conf_prjlist_attr_list(alpp, clpp, fields, "projects", ep,
                                         CQ_projects, APRJLIST_Type, 
                                         APRJLIST_href)) ? -1 : 0;
   }

   /* --------- CQ_xprojects */
   if (ret == 0) {
      ret = (!set_conf_prjlist_attr_list(alpp, clpp, fields, "xprojects", ep,
                                         CQ_xprojects, APRJLIST_Type, 
                                         APRJLIST_href)) ? -1 : 0;
   }

#if 0 /* EB: TODO: APIBASE */
   /* --------- CU_ruser_list */
   if (ret == 0) {
      ret = (!set_conf_str_attr_list(alpp, clpp, fields, "remote_user", ep, 
                                     CU_ruser_list, ASTR_Type, 
                                     ASTR_href)) ? -1 : 0;
   }

   /* --------- CU_ulong32 */
   if (ret == 0) {
      ret = (!set_conf_ulng_attr_list(alpp, clpp, fields, "ulong32", ep, 
                                      CU_ulong32, AULNG_Type, 
                                      AULNG_href)) ? -1 : 0;
   }
   
   /* --------- CU_bool */
   if (ret == 0) {
      ret = (!set_conf_bool_attr_list(alpp, clpp, fields, "bool", ep, 
                                      CU_bool, ABOOL_Type, 
                                      ABOOL_href)) ? -1 : 0;
   }

   /* --------- CU_time */
   if (ret == 0) {
      ret = (!set_conf_time_attr_list(alpp, clpp, fields, "time", ep, 
                                      CU_time, ATIME_Type,
                                      ATIME_href)) ? -1 : 0;
   }
   
   /* --------- CU_mem */
   if (ret == 0) {
      ret = (!set_conf_mem_attr_list(alpp, clpp, fields, "mem", ep, 
                                     CU_mem, AMEM_Type,
                                     AMEM_href)) ? -1 : 0;
   }
   
#endif

   DEXIT;
   return ret;
}

char *
write_cqueue(int spool, int how, const lListElem *ep) 
{
   FILE *fp;
   char filename[SGE_PATH_MAX], real_filename[SGE_PATH_MAX];

   DENTER(TOP_LAYER, "write_cqueue");
 
   switch (how) {
   case 0:
      fp = stdout;
      break;
   case 1:
   case 2:
      if (how==1) {
         if (!sge_tmpnam(filename)) {
            CRITICAL((SGE_EVENT, MSG_TMPNAM_GENERATINGTMPNAM));
            DEXIT;
            return NULL;
         }
      } else {
         sprintf(filename, "%s/.%s", CQUEUE_DIR, lGetString(ep, CQ_name));
         sprintf(real_filename, "%s/%s", CQUEUE_DIR, lGetString(ep, CQ_name));
      }

      fp = fopen(filename, "w");
      if (!fp) {
         CRITICAL((SGE_EVENT, MSG_FILE_ERRORWRITING_SS, filename, strerror(errno)));
         DEXIT;
         return NULL;
      }
      break;
   default:
      DEXIT;
      return NULL;
   }

   {
      FPRINTF((fp, "qname            %s\n", 
               lGetString(ep, CQ_name))); 
   }
   {
      const lList *ulng_attr_list = lGetList(ep, CQ_seq_no);

      FPRINTF((fp, "seq_no           "));
      if (ulng_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         ulng_attr_list_append_to_dstring(ulng_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "0\n"));
      }
   }
   {
      const lList *ulng_attr_list = lGetList(ep, CQ_nsuspend);

      FPRINTF((fp, "nsuspend         "));
      if (ulng_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         ulng_attr_list_append_to_dstring(ulng_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "0\n"));
      }
   }
   {
      const lList *ulng_attr_list = lGetList(ep, CQ_job_slots);

      FPRINTF((fp, "slots            "));
      if (ulng_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         ulng_attr_list_append_to_dstring(ulng_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "1\n"));
      }
   }
   {
      const lList *ulng_attr_list = lGetList(ep, CQ_fshare);

      FPRINTF((fp, "fshare           "));
      if (ulng_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         ulng_attr_list_append_to_dstring(ulng_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "1\n"));
      }
   }
   {
      const lList *ulng_attr_list = lGetList(ep, CQ_oticket);

      FPRINTF((fp, "oticket          "));
      if (ulng_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         ulng_attr_list_append_to_dstring(ulng_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "1\n"));
      }
   }
   {
      const lList *bool_attr_list = lGetList(ep, CQ_rerun);

      FPRINTF((fp, "rerun            "));
      if (bool_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         bool_attr_list_append_to_dstring(bool_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "FALSE\n"));
      }
   }

   {
      const lList *mem_attr_list = lGetList(ep, CQ_s_fsize);

      FPRINTF((fp, "s_fsize          "));
      if (mem_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         mem_attr_list_append_to_dstring(mem_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *mem_attr_list = lGetList(ep, CQ_h_fsize);

      FPRINTF((fp, "h_fsize          "));
      if (mem_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         mem_attr_list_append_to_dstring(mem_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *mem_attr_list = lGetList(ep, CQ_s_data);

      FPRINTF((fp, "s_data           "));
      if (mem_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         mem_attr_list_append_to_dstring(mem_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *mem_attr_list = lGetList(ep, CQ_h_data);

      FPRINTF((fp, "h_data           "));
      if (mem_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         mem_attr_list_append_to_dstring(mem_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *mem_attr_list = lGetList(ep, CQ_s_stack);

      FPRINTF((fp, "s_stack          "));
      if (mem_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         mem_attr_list_append_to_dstring(mem_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *mem_attr_list = lGetList(ep, CQ_h_stack);

      FPRINTF((fp, "h_stack          "));
      if (mem_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         mem_attr_list_append_to_dstring(mem_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *mem_attr_list = lGetList(ep, CQ_s_core);

      FPRINTF((fp, "s_core           "));
      if (mem_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         mem_attr_list_append_to_dstring(mem_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *mem_attr_list = lGetList(ep, CQ_h_core);

      FPRINTF((fp, "h_core           "));
      if (mem_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         mem_attr_list_append_to_dstring(mem_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *mem_attr_list = lGetList(ep, CQ_s_rss);

      FPRINTF((fp, "s_rss            "));
      if (mem_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         mem_attr_list_append_to_dstring(mem_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *mem_attr_list = lGetList(ep, CQ_h_rss);

      FPRINTF((fp, "h_rss            "));
      if (mem_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         mem_attr_list_append_to_dstring(mem_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *mem_attr_list = lGetList(ep, CQ_s_vmem);

      FPRINTF((fp, "s_vmem           "));
      if (mem_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         mem_attr_list_append_to_dstring(mem_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *mem_attr_list = lGetList(ep, CQ_h_vmem);

      FPRINTF((fp, "h_vmem           "));
      if (mem_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         mem_attr_list_append_to_dstring(mem_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *time_attr_list = lGetList(ep, CQ_s_rt);

      FPRINTF((fp, "s_rt             "));
      if (time_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         time_attr_list_append_to_dstring(time_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *time_attr_list = lGetList(ep, CQ_h_rt);

      FPRINTF((fp, "h_rt             "));
      if (time_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         time_attr_list_append_to_dstring(time_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *time_attr_list = lGetList(ep, CQ_s_cpu);

      FPRINTF((fp, "s_cpu            "));
      if (time_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         time_attr_list_append_to_dstring(time_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *time_attr_list = lGetList(ep, CQ_h_cpu);

      FPRINTF((fp, "h_cpu            "));
      if (time_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         time_attr_list_append_to_dstring(time_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *inter_attr_list = lGetList(ep, CQ_suspend_interval);

      FPRINTF((fp, "suspend_interval "));
      if (inter_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         inter_attr_list_append_to_dstring(inter_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "00:05:00\n"));
      }
 
   }
   {
      const lList *inter_attr_list = lGetList(ep, CQ_min_cpu_interval);

      FPRINTF((fp, "min_cpu_interval "));
      if (inter_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         inter_attr_list_append_to_dstring(inter_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "00:05:00\n"));
      }
 
   }
   {
      const lList *inter_attr_list = lGetList(ep, CQ_notify);

      FPRINTF((fp, "notify           "));
      if (inter_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         inter_attr_list_append_to_dstring(inter_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "00:00:60\n"));
      }
 
   }
   {
      const lList *str_attr_list = lGetList(ep, CQ_tmpdir);

      FPRINTF((fp, "tmpdir           "));
      if (str_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         str_attr_list_append_to_dstring(str_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "NONE\n"));
      }
 
   }
   {
      const lList *str_attr_list = lGetList(ep, CQ_pe_list);

      FPRINTF((fp, "pe_list          "));
      if (str_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         strlist_attr_list_append_to_dstring(str_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "NONE\n"));
      }
   }
   {
      const lList *str_attr_list = lGetList(ep, CQ_ckpt_list);

      FPRINTF((fp, "ckpt_list        "));
      if (str_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         strlist_attr_list_append_to_dstring(str_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "NONE\n"));
      }
   }
   {
      const lList *str_attr_list = lGetList(ep, CQ_owner_list);

      FPRINTF((fp, "owner_list       "));
      if (str_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         usrlist_attr_list_append_to_dstring(str_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "NONE\n"));
      }
   }
   {
      const lList *str_attr_list = lGetList(ep, CQ_acl);

      FPRINTF((fp, "user_lists       "));
      if (str_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         usrlist_attr_list_append_to_dstring(str_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "NONE\n"));
      }
   }
   {
      const lList *str_attr_list = lGetList(ep, CQ_xacl);

      FPRINTF((fp, "xuser_lists      "));
      if (str_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         usrlist_attr_list_append_to_dstring(str_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "NONE\n"));
      }
   }
   {
      const lList *str_attr_list = lGetList(ep, CQ_projects);

      FPRINTF((fp, "projects         "));
      if (str_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         prjlist_attr_list_append_to_dstring(str_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "NONE\n"));
      }
   }
   {
      const lList *str_attr_list = lGetList(ep, CQ_xprojects);

      FPRINTF((fp, "xprojects        "));
      if (str_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         prjlist_attr_list_append_to_dstring(str_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "NONE\n"));
      }
   }
#if 0 /* EB: TODO: APIBASE */ 
   {
      const lList *str_attr_list = lGetList(ep, CU_ruser_list);

      FPRINTF((fp, "remote_user      "));
      if (str_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         str_attr_list_append_to_dstring(str_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "NONE\n"));
      }
 
   }
   {
      const lList *ulng_attr_list = lGetList(ep, CU_ulong32);

      FPRINTF((fp, "ulong32          "));
      if (ulng_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         ulng_attr_list_append_to_dstring(ulng_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "NONE\n"));
      }
 
   }
   {
      const lList *bool_attr_list = lGetList(ep, CU_bool);

      FPRINTF((fp, "bool             "));
      if (bool_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         bool_attr_list_append_to_dstring(bool_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "NONE\n"));
      }
 
   }
   {
      const lList *time_attr_list = lGetList(ep, CU_time);

      FPRINTF((fp, "time             "));
      if (time_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         time_attr_list_append_to_dstring(time_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *mem_attr_list = lGetList(ep, CU_mem);

      FPRINTF((fp, "mem              "));
      if (mem_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         mem_attr_list_append_to_dstring(mem_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "INFINITY\n"));
      }
 
   }
   {
      const lList *inter_attr_list = lGetList(ep, CU_inter);

      FPRINTF((fp, "inter            "));
      if (inter_attr_list != NULL) {
         dstring string = DSTRING_INIT;

         inter_attr_list_append_to_dstring(inter_attr_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "00:05:00\n"));
      }
 
   }
#endif

   if (how != 0) {
      fclose(fp);
   }
   if (how == 2) {
      if (rename(filename, real_filename) == -1) {
         DEXIT;
         return NULL;
      } else {
         strcpy(filename, real_filename);
      }
   }                 
   DEXIT;
   return how==1?sge_strdup(NULL, filename):filename;

FPRINTF_ERROR:
   DEXIT;
   return NULL;  
}


