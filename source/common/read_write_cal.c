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

#include "sge.h"
#include "sge_calendarL.h"
#include "sge_confL.h"
#include "sge_answerL.h"
#include "read_write_cal.h"
#include "sge_string.h"
#include "sge_log.h"
#include "sge_tmpnam.h"
#include "config.h"
#include "read_object.h"
#include "sgermon.h"
#include "sge_stdio.h"
#include "msg_common.h"
#include "sge_spoolmsg.h"
#include "sge_feature.h"

/****
 **** cull_read_in_cal
 ****/
lListElem *cull_read_in_cal(
char *dirname,
char *filename,
int spool,
int flag,
int *tag,
int fields[] 
) {
   lListElem *ep;
   struct read_object_args args = { CAL_Type, "calendar", read_cal_work };
   int intern_tag = 0;

   DENTER(TOP_LAYER, "cull_read_in_cal");

   ep = read_object(dirname, filename, spool, 0, 0,&args, tag?tag:&intern_tag, fields);
  
   DEXIT;
   return ep;
}


/* ------------------------------------------------------------

   read_cal_work()

   spool:
      1 write for spooling
      0 write only user controlled fields

*/
int read_cal_work(
lList **alpp,
lList **clpp,
int fields[],
lListElem *ep,
int spool,
int flag,
int *tag,
int parsing_type 
) {
   DENTER(TOP_LAYER, "read_cal_work");

   /* --------- CAL_name */
   if (!set_conf_string(alpp, clpp, fields, "calendar_name", ep, CAL_name)) {
      DEXIT;
      return -1;
   }

   /* --------- CAL_year_calendar */
   if (!set_conf_string(alpp, clpp, fields, "year", ep, CAL_year_calendar)) {
      DEXIT;
      return -1;
   }

   /* --------- CAL_week_calendar */
   if (!set_conf_string(alpp, clpp, fields, "week", ep, CAL_week_calendar)) {
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}


/* ------------------------------------------------------------

   returns tmpfile name in case of creating a tempfile

   spool:
      1 write for spooling
      0 write only user controlled fields

   how:
      0 use stdout
      1 write into tmpfile
      2 write into spoolfile

*/
char *write_cal(
int spool,
int how,
lListElem *ep 
) {
   FILE *fp;
   char *s, filename[SGE_PATH_MAX], real_filename[SGE_PATH_MAX];

   DENTER(TOP_LAYER, "write_cal");

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
         sprintf(filename, "%s/.%s", CAL_DIR, lGetString(ep, CAL_name));
         sprintf(real_filename, "%s/%s", CAL_DIR, lGetString(ep, CAL_name));
      }

      fp = fopen(filename, "w");
      if (!fp) {
         CRITICAL((SGE_EVENT, MSG_FILE_ERRORWRITING_S, filename));
         DEXIT;
         return NULL;
      }
      break;
   default:
      DEXIT;
      return NULL;
   }

   if (spool && sge_spoolmsg_write(fp, COMMENT_CHAR,
             feature_get_product_name(FS_VERSION)) < 0) {
      goto FPRINTF_ERROR;
   }  

   FPRINTF((fp, "calendar_name    %s\n", lGetString(ep, CAL_name)));
   FPRINTF((fp, "year             %s\n", 
           (s=lGetString(ep, CAL_year_calendar))?s:"NONE"));
   FPRINTF((fp, "week             %s\n", 
           (s=lGetString(ep, CAL_week_calendar))?s:"NONE"));

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


/* -----------------------------
   
   build up a generic cal object

   returns 
      NULL on error

*/
lListElem* sge_generic_cal(char *cal_name)
{
   lListElem *calp;

   DENTER(TOP_LAYER, "sge_generic_cal");

   calp = lCreateElem(CAL_Type);

   lSetString(calp, CAL_name, cal_name?cal_name:"template");

   DEXIT;
   return calp;
}
