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

#include "sge_unistd.h"
#include "sge.h"
#include "cull.h"
#include "sge_gdi_request.h"
#include "config.h"
#include "sge_answer.h"
#include "read_write_userset.h"
#include "read_object.h"
#include "sge_feature.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_stdio.h"
#include "sge_spool.h"
#include "sge_userset.h"

#include "msg_common.h"

/*
** corresponding enum is in sge_usersetL.h
** the order is important, cause the corresponding enum is created
** in sge_parse_enum by value = (1<<array_index) 
*/
static const char* userset_types[] = {
   "ACL",   /* US_ACL   */
   "DEPT",  /* US_DEPT  */
   ""
};

static int read_userset_work(lList **alpp, lList **clpp, int fields[], lListElem *ep, int spool, int flag, int *tag, int parsing_type);

/****
 **** cull_read_in_userset
 ****/
lListElem *cull_read_in_userset(
const char *dirname,
const char *filename,
int spool,
int flag,
int *tag 
) {
   lListElem *ep;
   struct read_object_args args = { US_Type, "userset", read_userset_work};
   int intern_tag = 0;

   DENTER(TOP_LAYER, "cull_read_in_userset");

   ep = read_object(dirname, filename, spool, 0, 0,&args, tag?tag:&intern_tag, NULL);
  
   DEXIT;
   return ep;
}



static int read_userset_work(
lList **alpp,
lList **clpp,
int fields[],
lListElem *ep,
int spool,
int flag,
int *tag,
int parsing_type 
) {
#ifdef __INSIGHT__
/* JG: NULL is OK for fields */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   DENTER(TOP_LAYER, "read_userset_work");

   /* --------- US_name */
   if (!set_conf_string(alpp, clpp, fields, "name", ep, US_name)) {
      DEXIT;
      return -1;
   }
   
   if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
      /* --------- US_type */
      if (!set_conf_enum(alpp, clpp, fields, "type", ep, US_type, userset_types)) {
         DEXIT;
         return -1;
      }

      /* --------- US_oticket */
      if (!set_conf_ulong(alpp, clpp, fields, "oticket", ep, US_oticket)) {
         DEXIT;
         return -1;
      }

      /* --------- US_fshare */
      if (!set_conf_ulong(alpp, clpp, fields, "fshare", ep, US_fshare)) {
         DEXIT;
         return -1;
      }
   }

   /* --------- US_entries */
   if (!set_conf_list(alpp, clpp, fields, "entries", ep, US_entries, 
                        UE_Type, UE_name)) {
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}

/************************************************************/
int write_userset(
lList **alpp,
const lListElem *ep,
const char *fname,
FILE *fpout,
int spool 
) {
   FILE *fp;
   int print_elements[] = { UE_name, 0 };
   const char *delis[] = {":", ",", NULL};
   const char **ptr;
   u_long32 bitmask, type;
   char filename[SGE_PATH_MAX];
   int ret;
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "write_userset");

   sge_dstring_init(&ds, buffer, sizeof(buffer));
   if (!ep) {
      if (!alpp) {
         ERROR((SGE_EVENT, MSG_USERSET_NOUSERETELEMENT));
         SGE_EXIT(1);
      } else {
         answer_list_add(alpp, MSG_USERSET_NOUSERETELEMENT, 
                         STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -1;
      }
   }
   if (fname) {
      strcpy(filename, fname);
      if (!(fp = fopen(filename, "w"))) {
         ERROR((SGE_EVENT, MSG_FILE_NOOPEN_SS, fname, strerror(errno)));
         if (!alpp) {
            SGE_EXIT(1);
         } else {
            answer_list_add(alpp, SGE_EVENT, 
                            STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return -1;
         }
      }
   } else {
      fp = fpout;
   }

   if (spool && sge_spoolmsg_write(fp, COMMENT_CHAR,
             feature_get_product_name(FS_VERSION, &ds)) < 0) {
      goto FPRINTF_ERROR;
   } 

   FPRINTF((fp, "name       %s\n", lGetString(ep, US_name)));
   if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
      /* userset type field */
      type = lGetUlong(ep, US_type);
      FPRINTF((fp, "type       "));
      bitmask = 1;
      for (ptr = userset_types; **ptr != '\0'; ptr++) {
         if (bitmask & type) {
            FPRINTF((fp,"%s ",*ptr));
         }
         bitmask <<= 1;
      };
      FPRINTF((fp,"\n"));

      FPRINTF((fp, "oticket    " u32 "\n", lGetUlong(ep, US_oticket)));
      FPRINTF((fp, "fshare     " u32 "\n", lGetUlong(ep, US_fshare)));
   }

   FPRINTF((fp, "entries    "));
   ret = uni_print_list(fp, NULL, 0, lGetList(ep, US_entries), print_elements,
                  delis, 0);
   if (ret < 0) {
      goto FPRINTF_ERROR;
   }
   FPRINTF((fp, "\n"));

   if (fname) {
      fclose(fp);
   }

   DEXIT;
   return 0;

FPRINTF_ERROR:
   DEXIT;
   return -1;
}

