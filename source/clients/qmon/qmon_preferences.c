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

#include <Xmt/Xmt.h>
#include <Xmt/Util.h>

#include "sge.h"
#include "qmon_prefL.h"
#include "sge_stringL.h"
#include "qmon_preferences.h"
#include "sge_string.h"
#include "config.h"
#include "spool/classic/read_object.h"
#include "sgermon.h"
#include "sge_log.h"
#include "version.h"
#include "sge_feature.h"
#include "sge_answer.h"
#include "sge_complex.h"

#include "msg_common.h"

static int read_pref_work(lList **alpp, lList **clpp, int fields[], lListElem *ep, int spool, int flag, int *tag, int parsing_type);

static lListElem *cull_read_in_pref(char *dirname, char *filename, int spool, int flag, int *tag);

static lList* write_pref(char *filename, lListElem *ep);

/*
** the qmon preferences are kept here
*/
lListElem *qmon_preferences = NULL;

void qmonReadPreferences(void)
{
   DENTER(TOP_LAYER, "qmonReadPreferences");

   qmon_preferences = cull_read_in_pref(XmtGetHomeDir(), ".qmon_preferences",
                                        0, 0, NULL);
   DEXIT;
}

lList* qmonWritePreferences(void)
{
   char filename[SGE_PATH_MAX];
   lList *alp;

   DENTER(TOP_LAYER, "qmonWritePreferences");

   sprintf(filename, "%s/.qmon_preferences", XmtGetHomeDir());
   alp = write_pref(filename, qmon_preferences);

   DEXIT;
   return alp;
}

/****
 **** cull_read_in_pref
 ****/
static lListElem *cull_read_in_pref(
char *dirname,
char *filename,
int spool,
int flag,
int *tag 
) {
   lListElem *ep;
   struct read_object_args args = { PREF_Type, "pref", read_pref_work };
   int intern_tag = 0;

   DENTER(TOP_LAYER, "cull_read_in_pref");

   ep = read_object(dirname, filename, spool, 0, 0,&args, tag?tag:&intern_tag, NULL);
  
   DEXIT;
   return ep;
}


/* ------------------------------------------------------------

   read_pref_work()

   spool:
      1 write for spooling
      0 write only user controlled fields

*/
static int read_pref_work(
lList **alpp,
lList **clpp,
int fields[],
lListElem *ep,
int spool,
int flag,
int *tag,
int parsing_type 
) {
   static intprt_type intprt_as_cplx_entry[] = { 
      CE_name, 
      CE_stringval,
      0
   }; 

   DENTER(TOP_LAYER, "read_pref_work");

   /* --------- PREF_job_filter_resources */
   if (!set_conf_deflist(alpp, clpp, fields, "job_filter_resources", ep, 
                      PREF_job_filter_resources, 
                        CE_Type, intprt_as_cplx_entry)) {
      DEXIT;
      return -1;
   }
   
   /* --------- PREF_job_filter_owners */
   if (!set_conf_list(alpp, clpp, fields, "job_filter_owners", ep, 
                      PREF_job_filter_owners, ST_Type, STR)) {
      DEXIT;
      return -1;
   }
   
   /* --------- PREF_job_filter_fields */
   if (!set_conf_list(alpp, clpp, fields, "job_filter_fields", ep, 
                      PREF_job_filter_fields, ST_Type, STR)) {
      DEXIT;
      return -1;
   }
   
   /* --------- PREF_job_filter_compact */
   set_conf_bool(NULL, clpp, fields, "job_filter_compact", ep, 
                 PREF_job_filter_compact);

   /* --------- PREF_queue_filter_resources */
   if (!set_conf_deflist(alpp, clpp, fields, "queue_filter_resources", ep, 
                      PREF_queue_filter_resources,
                      CE_Type, intprt_as_cplx_entry)) {
      DEXIT;
      return -1;
   }
   
   DEXIT;
   return 0;
}


/* ------------------------------------------------------------
   write preferences to file
*/
static lList* write_pref(
char *filename,
lListElem *ep 
) {
   lList *answer = NULL;
   FILE *fp;
   lListElem *sep;
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "write_pref");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if (!filename)
      fp = stdout;
   else
      fp = fopen(filename, "w");

   if (!fp) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_FILE_ERRORWRITETOFILEX_S, filename));
      answer_list_add(&answer, SGE_EVENT, STATUS_EDISK, ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;
   }

   /* --------- HEADER COMMENT */
   fprintf(fp, "# %s AUTOGENERATED FILE DO NOT EDIT !\n",
               feature_get_product_name(FS_LONG_VERSION, &ds));

   /* --------- PREF_job_filter_resources */
   fprintf(fp, "job_filter_resources   ");
   sep = lFirst(lGetList(ep, PREF_job_filter_resources));
   if (sep) {
      do {
         fprintf(fp, "%s=%s", lGetString(sep, CE_name), 
                     lGetString(sep, CE_stringval));
         sep = lNext(sep);
         if (sep) 
            fprintf(fp, ",");
      } while (sep);
      fprintf(fp, "\n");
   }
   else
      fprintf(fp, "NONE\n");

   /* --------- PREF_job_filter_owners */
   fprintf(fp, "job_filter_owners      ");
   sep = lFirst(lGetList(ep, PREF_job_filter_owners));
   if (sep) {
      do {
         fprintf(fp, "%s", lGetString(sep, STR));
         sep = lNext(sep);
         if (sep) 
            fprintf(fp, " ");
      } while (sep);
      fprintf(fp, "\n");
   }
   else
      fprintf(fp, "NONE\n");

   /* --------- PREF_job_filter_fields */
   fprintf(fp, "job_filter_fields      ");
   sep = lFirst(lGetList(ep, PREF_job_filter_fields));
   if (sep) {
      do {
         fprintf(fp, "%s", lGetString(sep, STR));
         sep = lNext(sep);
         if (sep) 
            fprintf(fp, " ");
      } while (sep);
      fprintf(fp, "\n");
   }
   else
      fprintf(fp, "NONE\n");

   /* --------- PREF_queue_filter_resources */
   fprintf(fp, "queue_filter_resources ");
   sep = lFirst(lGetList(ep, PREF_queue_filter_resources));
   if (sep) {
      do {
         fprintf(fp, "%s=%s", lGetString(sep, CE_name), 
                     lGetString(sep, CE_stringval));
         sep = lNext(sep);
         if (sep) 
            fprintf(fp, ",");
      } while (sep);
      fprintf(fp, "\n");
   }
   else
      fprintf(fp, "NONE\n");

   /* --------- PREF_job_filter_compact */
   fprintf(fp, "job_filter_compact     %s\n", 
           lGetBool(ep, PREF_job_filter_compact) ?  "TRUE" : "FALSE");

   if (fp != stdout)
      fclose(fp);

   DEXIT;
   return NULL;
}
