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
#include "sge_usermapL.h"
#include "sge_stringL.h"
#include "sge_confL.h"
#include "sge_answerL.h"
/*#include "sge_usersetL.h"*/
#include "read_write_ume.h"
#include "sge_string.h"
#include "sge_log.h"
#include "sge_tmpnam.h"
#include "config.h"
#include "read_object.h"
#include "sge_stdio.h"
#include "msg_common.h"


static int read_ume_work(lList **alpp, lList **clpp, int fields[], lListElem *ep, int spool, int flag, int *tag, int parsing_type);



/****** src/cull_read_in_ume() **********************************
*
*  NAME
*     cull_read_in_ume() -- read in user mapping file entry 
*
*  SYNOPSIS
*
*     #include "read_write_ume.h"
*     #include <src/read_write_ume.h>
* 
*     lListElem *cull_read_in_ume(char *dirname,
*                                       char *filename, 
*                                       int spool, 
*                                       int flag, 
*                                       int *tag); 
*       
*
*  FUNCTION
*     This function reads in a user mapping file for a cluster user. 
*
*  INPUTS
*     char *dirname  - directory of mapping file
*     char *filename - filename of mapping file (cluster user name)
*     int spool      - not used
*     int flag       - not used
*     int *tag       - not used
*
*  RESULT
*     lListElem*     - UME_Type list element.
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
*     src/write_ume()
*     
****************************************************************************
*/
lListElem *cull_read_in_ume(
char *dirname,
char *filename,
int spool,
int flag,
int *tag 
) {  
   lListElem *ep;
   struct read_object_args args = { UME_Type, "user mapping entry list", read_ume_work };
   int intern_tag = 0;

   DENTER(TOP_LAYER, "cull_read_in_ume");

   ep = read_object(dirname, filename, spool, 0, 0,&args, tag?tag:&intern_tag, NULL);
  
   DEXIT;
   return ep;
}




/****** src/read_ume_work() **********************************
*
*  NAME
*     read_ume_work() -- fill list element with data from file 
*
*  SYNOPSIS
*
*     #include "read_write_ume.h"
*     #include <src/read_write_ume.h>
* 
*     static int read_ume_work(lList **alpp, 
*                                   lList **clpp, 
*                                   int fields[], 
*                                   lListElem *ep,
*                                   int spool,
*                                   int flag,
*                                   int *tag);
*     
*       
*
*  FUNCTION
*     This function is a worker function for cull_read_in_ume()
*
*  INPUTS
*     lList **alpp   - anser list
*     lList **clpp   - list with parsed file contens
*     int fields[]   - ??? (not used)
*     lListElem *ep  - list element to fill (UME_Type) 
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
*     src/cull_read_in_ume()
*     
****************************************************************************
*/
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
lListElem *ep,  /* list element to fill of type UME_Type */
int spool,      /* look above */
int flag,       /* user flag */
int *tag,       /* user return value */
int parsing_type 
) {
   lListElem* confListElem = NULL;
   lList*     mapList = NULL;
   char* mappedUser = NULL;

   DENTER(TOP_LAYER, "read_ume_work");
   /* try to fill the ep pointer with all data */
   /* a return of -1 is for error state */ 

   lSetString(ep, UME_cluster_user, ""); 

   while ((confListElem=lFirst(*clpp)) != NULL) {
      mappedUser = lGetString( confListElem , CF_name);
      if (mappedUser != NULL) {
         char* hostList = NULL;
         if (strcmp(mappedUser,"cluster_user") == 0) {
            /* cluster_user keyword found */
            hostList = lGetString(confListElem, CF_value);
            if (hostList != NULL) {
              DPRINTF(("cluster_user entry is '%s'\n", hostList));
              lSetString(ep, UME_cluster_user, hostList);
              if (mapList == NULL) {
                mapList = lCreateList("user mapping", UM_Type);
              } 
            }
         } else {
            /* normal mapping entry */
            hostList = lGetString( confListElem, CF_value);
            if (hostList != NULL) {
               lList* stringList = NULL;
               lString2List(hostList, &stringList, ST_Type, STR, ", ");
               if (stringList != NULL) {
                  lListElem* mapElem = lCreateElem(UM_Type); 
                  if (mapList == NULL) {
                    mapList = lCreateList("user mapping", UM_Type);
                  }
                  lSetString(mapElem, UM_mapped_user, mappedUser);
                  lSetList  (mapElem, UM_host_list  , stringList);
                  lAppendElem(mapList, mapElem);
                  DPRINTF(("Mapped User Name: '%s' has hostnames '%s'\n", mappedUser, hostList));
               }
            }
         }
      }
      lDelElemStr(clpp, CF_name, mappedUser);   /* remove the hole element */
   }
   lSetList  (ep     , UME_mapping_list, mapList);

   DEXIT;
   return 0;   /* everythis was very well, ->perfecto !! */
}








/****** src/write_ume() **********************************
*
*  NAME
*     write_ume() -- save user mapping entrie (UME_Type) 
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
*     lListElem *umep - UME_Type list element to store
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
lListElem *ep 
) {
   FILE *fp;
   char filename[SGE_PATH_MAX], real_filename[SGE_PATH_MAX];
   lList* mapList = NULL;
   lListElem* mapElem = NULL;
   DENTER(TOP_LAYER, "write_ume");
  
   DPRINTF(("writeing user mapping entry list\n"));
   strcpy(filename, lGetString(ep, UME_cluster_user));
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
         sprintf(filename, "%s/.%s", UME_DIR, lGetString(ep, UME_cluster_user));
         sprintf(real_filename, "%s/%s", UME_DIR, 
            lGetString(ep, UME_cluster_user));
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

   if (spool == 2) { 
     FPRINTF((fp,MSG_UM_CONFIGTEXT1));
     FPRINTF((fp,MSG_UM_CONFIGTEXT2));
     FPRINTF((fp,MSG_UM_CONFIGTEXT3));
   }

   FPRINTF((fp, "cluster_user        %s\n\n", 
      lGetString(ep, UME_cluster_user))); 

   if (spool == 2) {
     FPRINTF((fp,MSG_UM_CONFIGTEXT2));
     FPRINTF((fp,MSG_UM_CONFIGTEXT4));
     FPRINTF((fp,MSG_UM_CONFIGTEXT5));
     FPRINTF((fp,MSG_UM_CONFIGTEXT2));
     FPRINTF((fp,MSG_UM_CONFIGTEXT6));
   }    

   mapList = lGetList(ep, UME_mapping_list);
   if (mapList != NULL) {
      for_each ( mapElem, mapList ) {
         char* mapName = NULL;
         lList* hostList = NULL;
 
         mapName = lGetString( mapElem , UM_mapped_user);
         hostList = lGetList(mapElem, UM_host_list) ;
         if ((mapName != NULL) && (hostList != NULL)) {
            lListElem* hostElem = NULL;
            int i = 20;
            FPRINTF((fp, "%s", mapName));
            
            i = i - strlen(mapName);
            /* this is to get all host lists in colum 20 */

            do {
               FPRINTF((fp, " "));
               i=i-1;
            } while (i> 0);
           
            for_each(hostElem , hostList) {
               char* tmpHost = NULL;
               tmpHost = lGetString( hostElem , STR);
               if (tmpHost != NULL) {
                  FPRINTF((fp, "%s", tmpHost));
                  if (lNext(hostElem)) {
                     FPRINTF((fp, ", "));
                  } else {
                     FPRINTF((fp, "\n"));
                  }
               }
            }
         }
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





/****** src/sge_create_ume() **********************************
*
*  NAME
*     sge_create_ume() --  returns new UME_Type element
*
*  SYNOPSIS
*
*     #include "read_write_ume.h"
*     #include <src/read_write_ume.h>
* 
*     lListElem* sge_create_ume(char *ume_name, char* mapName, char* hostname)  
*
*  FUNCTION
*     returns new UME_Type element
*
*  INPUTS
*     char *ume_name - UME_cluster_user name
*     char* mapName  - UM_mapped_user name
*     char* hostname - UM_host_list entry
*     
*  RESULT
*     lListElem*  -  pointer to UME_Type element
*     NULL        -  on error
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
*     src/write_ume()
*     
****************************************************************************
*/
lListElem* sge_create_ume(char *ume_name, char* mapName, char* hostname)
{ 
   lList*     mapList = NULL;
   lList*     hostList = NULL;
   lListElem* mapElem = NULL;
   lListElem* umeElem = NULL;

   DENTER(TOP_LAYER, "sge_generic_ume");  
 
   if (ume_name == NULL) {
      DEXIT;
      return NULL;
   }
  
   if (mapName != NULL) {

     if (hostname != NULL) {
        hostList = lCreateList("host list", ST_Type);
        lAddElemStr(&hostList, STR, hostname ,ST_Type);
     }

     mapList = lCreateList("user mapping", UM_Type);

     mapElem = lCreateElem(UM_Type);
     lSetString(mapElem, UM_mapped_user, mapName);
     
     if (hostList != NULL) { 
        lSetList  (mapElem, UM_host_list  , hostList);
     }
     lAppendElem(mapList, mapElem);
   }

   umeElem = lCreateElem(UME_Type);
   lSetString(umeElem , UME_cluster_user, ume_name);
   
   if (mapList != NULL) {
     lSetList  (umeElem , UME_mapping_list, mapList );
   }

   DEXIT;
   return umeElem;
}
