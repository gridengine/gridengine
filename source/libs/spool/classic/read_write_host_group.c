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
#include "sge_stringL.h"
#include "read_write_host_group.h"
#include "sge_string.h"
#include "sge_log.h"
#include "config.h"
#include "read_object.h"
#include "sge_stdio.h"
#include "msg_common.h"
#include "sge_feature.h"
#include "sge_spool.h"
#include "sge_io.h"
#include "sge_answer.h"
#include "sge_hostgroup.h"
#include "sge_conf.h"

static int read_host_group_work(lList **alpp, lList **clpp, int fields[], lListElem *ep, int spool, int flag, int *tag, int parsing_type);



/****** src/cull_read_in_host_group() **********************************
*
*  NAME
*     cull_read_in_host_group() -- read in host group file entry 
*
*  SYNOPSIS
*
*  #include "read_write_host_group.h"
*
*  lListElem *cull_read_in_host_group(dirname, filename, spool, flag, tag)
*  char *dirname;
*  char *filename;
*  int spool;
*  int flag;
*  int *tag;
*       
*
*  FUNCTION
*     This function reads in a host group file for host group definition     
*
*  INPUTS
*     char *dirname  - directory of host group file
*     char *filename - filename of host group file (cluster user name)
*     int spool      - not used
*     int flag       - not used
*     int *tag       - not used
*  
*  RESULT
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
*     src/()
*     
****************************************************************************
*/
lListElem *cull_read_in_host_group(
const char *dirname,
const char *filename,
int spool,
int flag,
int *tag 
) {  
   lListElem *ep;
   struct read_object_args args = { GRP_Type, "main host group list", read_host_group_work };
   int intern_tag = 0;

   DENTER(TOP_LAYER, "cull_read_in_host_group");

   ep = read_object(dirname, filename, spool, 0,RCL_NO_VALUE, &args, tag?tag:&intern_tag, NULL);
  
   DEXIT;
   return ep;
}




/****** src/read_host_group_work() **********************************
*
*  NAME
*     read_host_group_work() -- fill list element with data from file 
*
*  SYNOPSIS
*
*     static int read_host_group_work(lList **alpp, 
*                                          lList **clpp, 
*                                          int fields[], 
*                                          lListElem *ep,
*                                          int spool,
*                                          int flag,
*                                          int *tag);
*     
*       
*
*  FUNCTION
*     This function is a worker function for cull_read_in_host_group()
*
*  INPUTS
*     lList **alpp   - anser list
*     lList **clpp   - list with parsed file contens
*     int fields[]   - ??? (not used)
*     lListElem *ep  - list element to fill (GRP_Type) 
*     int spool      - read only user information (not used)
*     int flag       - ??? (not used)
*     int *tag       - can return result (not used)
*
*  RESULT
*     static int     - 0 ok , -1 is error
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
*     src/cull_read_in_host_group()
*     
****************************************************************************
*/
/* ------------------------------------------------------------

   read_host_group_work()

   spool:
      1 write for spooling
      0 write only user controlled fields

*/
static int read_host_group_work(
lList **alpp,   /* anser list */
lList **clpp,   /* parsed file */
int fields[],   /* not needed */
lListElem *ep,  /* list element to fill of type GRP_Type */
int spool,      /* look above */
int flag,       /* user flag */
int *tag,       /* user return value */
int parsing_type 
) {
   lListElem* confListElem = NULL;
   const char*      fileEntry = NULL;
   const char*      fileValue = NULL;
   int        back = 0;

   DENTER(TOP_LAYER, "read_host_group_work");
   /* try to fill the ep pointer with all data */
   /* a return of -1 is for error state */ 

   lSetString(ep, GRP_group_name, ""); 
   
   while ((confListElem=lFirst(*clpp)) != NULL) {
      fileEntry = lGetString( confListElem , CF_name);
      if (fileEntry != NULL) {
         fileValue = lGetString(confListElem, CF_value);
         if ((strcmp(fileEntry,"group_name") == 0) && 
             (fileValue != NULL) ) {
            /* group_name keyword found */
            lSetString(ep,GRP_group_name , fileValue );
         } else {
            /* host name  or group name */
            if (fileEntry[0] == '@') {
               /* group name found */
               if (sge_add_subgroup2group(alpp,NULL, ep, &fileEntry[1],FALSE) == FALSE) {
                 DPRINTF(("Error adding subgroup '%s'\n", &fileEntry[1]));
                 SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ANSWER_SUBGROUPXINGROUPYNOTACCEPTED_SS, &fileEntry[1], lGetString(ep, GRP_group_name)));
                 answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR); 
                 back = -1;
               }
            } else {
               /* host name found */
               if (sge_add_member2group( ep, fileEntry) == FALSE) {
                 DPRINTF(("Error adding host '%s'\n", fileEntry));
                 SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ANSWER_HOSTXINGROUPYNOTACCEPTED_SS,fileEntry , lGetString(ep, GRP_group_name)));
                 answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR); 
                 back = -1;
               }
            } 
         }
      }
      lDelElemStr(clpp, CF_name, fileEntry);   /* remove the hole element */
   }
   DEXIT;
   return back;   /* everythis was very well, ->perfecto !! */
}



/****** src/write_host_group() **********************************
*
*  NAME
*     write_host_group() -- save host group entry (GRP_Type) 
*
*  SYNOPSIS
*
* 
*     char *write_host_group(int spool, int how, lListElem *hostGroupElem);
*       
*
*  FUNCTION
*     
*
*  INPUTS
*     spool          - 0 userer controlled entries, 1 write for spooling
*                      2 write for spooling with comment
*                      (0 and 1: this parameter is not used)
*     how            - 0 use stdout,  1 write into tmpfile, 2 write into spoolfile
*     hostGroupElem  - GRP_Type list element to store
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
*     src/cull_read_in_host_group()
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
char *write_host_group(
int spool,
int how,
const lListElem *ep 
) {
   FILE *fp;
   lList* subGroupList = NULL;
   lList* memberList = NULL;
   lListElem* listElem = NULL;
   char filename[SGE_PATH_MAX], real_filename[SGE_PATH_MAX];
   DENTER(TOP_LAYER, "write_host_group");
  
   DPRINTF(("writeing user host group entry list\n"));
   strcpy(filename, lGetString(ep, GRP_group_name));
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
      } else  {
         sprintf(filename, "%s/.%s", HOSTGROUP_DIR, 
            lGetString(ep, GRP_group_name));
         sprintf(real_filename, "%s/%s", HOSTGROUP_DIR, 
            lGetString(ep, GRP_group_name));
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

   if (spool == 1 && sge_spoolmsg_write(fp, COMMENT_CHAR,
                feature_get_product_name(FS_SHORT_VERSION)) < 0) {
      goto FPRINTF_ERROR;
   } 

   if (spool == 2) { 
     FPRINTF((fp,"# "SFN,MSG_HOSTGROUP_CONFIGTEXT1));
     FPRINTF((fp,"# "SFN,MSG_HOSTGROUP_CONFIGTEXT2));
     FPRINTF((fp,"# "SFN,MSG_HOSTGROUP_CONFIGTEXT3));
   }

   FPRINTF((fp, "group_name        %s\n\n", lGetString(ep, GRP_group_name))); 

   if (spool == 2) {
     FPRINTF((fp,"# "SFN,MSG_HOSTGROUP_CONFIGTEXT2));
     FPRINTF((fp,"# "SFN,MSG_HOSTGROUP_CONFIGTEXT4));
     FPRINTF((fp,"# "SFN,MSG_HOSTGROUP_CONFIGTEXT5));
     FPRINTF((fp,"# "SFN,MSG_HOSTGROUP_CONFIGTEXT6));
     FPRINTF((fp,"# "SFN,MSG_HOSTGROUP_CONFIGTEXT2));
   }    
  
   subGroupList = lGetList(ep, GRP_subgroup_list ); 
   memberList = lGetList(ep, GRP_member_list );
  
   if (subGroupList != NULL) {
      for_each ( listElem, subGroupList ) {
         const char* subGroupName = NULL;
         subGroupName = lGetString( listElem , STR);
         if (subGroupName != NULL) {
            FPRINTF((fp, "@%s\n",subGroupName));
         }
      }
   } 

   if (memberList != NULL) {
      for_each ( listElem, memberList ) {
         const char* memberName = NULL;
         memberName = lGetString( listElem , STR);
         if (memberName != NULL) {
            FPRINTF((fp, "%s\n", memberName));
         }
      }
   } 

   if (how!=0) {
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





