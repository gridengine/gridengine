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
   
   /* --------- CU_inter */
   if (ret == 0) {
      ret = (!set_conf_inter_attr_list(alpp, clpp, fields, "inter", ep, 
                                       CU_inter, AINTER_Type,
                                       AINTER_href)) ? -1 : 0;
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


