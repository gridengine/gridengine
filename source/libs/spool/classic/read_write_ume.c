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
#include "sge_cuser.h"
#include "sge_stringL.h"
#include "sge_answer.h"
#include "read_write_ume.h"
#include "sge_string.h"
#include "sge_log.h"
#include "config.h"
#include "read_object.h"
#include "sge_stdio.h"
#include "sge_io.h"
#include "sge_conf.h"
#include "sge_attr.h"

#include "msg_common.h"

static int read_ume_work(lList **alpp, lList **clpp, int fields[], lListElem *ep, int spool, int flag, int *tag, int parsing_type);

lListElem *cull_read_in_ume(
const char *dirname,
const char *filename,
int spool,
int flag,
int *tag,
int fields[]
) {  
   lListElem *ep;
   struct read_object_args args = { CU_Type, "user mapping entry list", read_ume_work };
   int intern_tag = 0;

   DENTER(TOP_LAYER, "cull_read_in_ume");

   ep = read_object(dirname, filename, spool, 0, 0,&args, tag?tag:&intern_tag, NULL);
  
   DEXIT;
   return ep;
}

/* ------------------------------------------------------------

   read_ume_work()

   spool:
      1 write for spooling
      0 write only user controlled fields

*/
static int read_ume_work(
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

   DENTER(TOP_LAYER, "read_ume_work");

   /* --------- CU_name */
   if (ret == 0) {
      ret = (!set_conf_string(alpp, clpp, fields, "cluster_user", ep, CU_name)) ? -1 : 0;
   }

   /* --------- CU_ruser_list */
   if (ret == 0) {
      ret = (!set_conf_hostattr_str_list(alpp, clpp, fields, "remote_user", ep, 
                                         CU_ruser_list, ASTR_Type, 
                                         ASTR_href)) ? -1 : 0;
   }

   DEXIT;
   return ret;
}

/****** src/write_ume() **********************************
*
*  NAME
*     write_ume() -- save user mapping entrie (CU_Type) 
*
*  SYNOPSIS
*
*     #include "read_write_ume.h"
*     #include <src/read_write_ume.h>
* 
*     char *write_ume(int spool, int how, lListElem *umep);
*       
*
*  FUNCTION
*     
*
*  INPUTS
*     int spool       - 0 userer controlled entries, 1 write for spooling
*                       2 write for spooling with comment
*                       (0 and 1: this parameter is not used)
*     int how         - 0 use stdout,  1 write into tmpfile, 2 write into spoolfile
*     lListElem *umep - CU_Type list element to store
*
*  RESULT
*     char* - pointer to filename
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     src/cull_read_in_ume()
*     
****************************************************************************
*/
/* ------------------------------------------------------------

   returns tmpfile name in case of creating a tempfile
   spool (not used - only spooling is active now):
      2 write for spolling with comment
      1 write for spooling
      0 write only user controlled fields

   how:
      0 use stdout
      1 write into tmpfile
      2 write into spoolfile

*/
char *write_ume(
int spool,
int how,
const lListElem *ep 
) {
   FILE *fp;
   char filename[SGE_PATH_MAX], real_filename[SGE_PATH_MAX];
   DENTER(TOP_LAYER, "write_ume");
  
   DPRINTF(("writeing user mapping entry list\n"));
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
         sprintf(filename, "%s/.%s", UME_DIR, lGetString(ep, CU_name));
         sprintf(real_filename, "%s/%s", UME_DIR, 
            lGetString(ep, CU_name));
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
      FPRINTF((fp, "cluster_user     %s\n", 
               lGetString(ep, CU_name))); 
   }
   {
      const lList *attr_str_list = lGetList(ep, CU_ruser_list);

      FPRINTF((fp, "remote_user      "));
      if (attr_str_list != NULL) {
         dstring string = DSTRING_INIT;

         attr_str_list_append_to_dstring(attr_str_list, &string);
         FPRINTF((fp, "%s\n", sge_dstring_get_string(&string)));
         sge_dstring_free(&string);
      } else {
         FPRINTF((fp, "NONE\n"));
      }
 
   }

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


