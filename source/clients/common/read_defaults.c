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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  
#include <pwd.h>
#include <sys/types.h>

#include "sgermon.h"
#include "sge_gdi_intern.h"
#include "sge_answerL.h"
#include "parse_job_cull.h"
#include "read_defaults.h"
#include "setup_path.h"
#include "sge_unistd.h"
#include "msg_common.h"
#include "msg_clients_common.h"
#include "sge_feature.h"
#include "sge_uidgid.h"
#include "sge_io.h"
#include "sge_prog.h"
#include "sge_answer.h"

static char *def_files[3 + 1];

/****** sge/sge/get_all_defaults_files() *********************************
*  NAME
*     get_all_defaults_files() -- reads default files and parses them 
*
*  SYNOPSIS
*     lList* get_all_defaults_files(lList **pcmdline, char **envp) 
*
*  FUNCTION
*     This function reads the 3 defaults files if they exist and parses them
*     into an options list. 
*
*  INPUTS
*     lList **pcmdline - pointer to SPA_Type list, if list is NULL, it is
*                        created if the files contain any options 
*     char **envp      - environment pointer 
*
*  RESULT
*     lList* - answer list, AN_Type or NULL if everything ok
*        possible errors:
*           STATUS_ENOSUCHUSER - could not retrieve passwd info on me.user_name
*           STATUS_EDISK       - home directory for user is missing or cwd 
*                                cannot be read or file could not be opened 
*                                (is just a warning)
*           STATUS_EEXIST      - (von parse_script_file), (is just a warning)
*           STATUS_EUNKNOWN    - (von parse_script_file), error opening or 
*                                reading from existing file, (is just a warning)
*                                plus all other error stati returned by 
*                                parse_script_file, see there
*
*  NOTES
*     path.sge_root and me.user_name will be used by this function
*
*     problem: make user a parameter?
*******************************************************************************/
lList *get_all_defaults_files(lList **pcmdline, char **envp) 
{
   lList *answer = NULL;
   lList *alp;
   lListElem *aep;
   struct passwd *pwd;
   char str[256 + 1];
   char cwd[SGE_PATH_MAX + 1];
   char **pstr;
   char **ppstr;
   SGE_STRUCT_STAT buf;
   int do_exit = 0;
   
   DENTER(TOP_LAYER, "get_all_defaults_files");

   /* the sge root defaults file */
   
   def_files[0] = malloc(strlen(path.cell_root) + 
                         strlen(SGE_COMMON_DEF_REQ_FILE) + 2);
   sprintf(def_files[0], "%s/%s", path.cell_root, SGE_COMMON_DEF_REQ_FILE);

   /*
    * the defaults file in the user's home directory
    */
   pwd = sge_getpwnam(me.user_name);
   if (!pwd) {
      sprintf(str, MSG_USER_INVALIDNAMEX_S, me.user_name);
      answer_list_add(&answer, str, STATUS_ENOSUCHUSER, ANSWER_QUALITY_ERROR);
      return answer;
   }
   if (!pwd->pw_dir) {
      sprintf(str, MSG_USER_NOHOMEDIRFORUSERX_S, me.user_name);
      answer_list_add(&answer, str, STATUS_EDISK, ANSWER_QUALITY_ERROR);
      return answer;
   }

   def_files[1] = malloc(strlen(pwd->pw_dir) + 
                         strlen(SGE_HOME_DEF_REQ_FILE) + 2);
   strcpy(def_files[1], pwd->pw_dir);
   if (*def_files[1] && (def_files[1][strlen(def_files[1]) - 1] != '/')) {
      strcat(def_files[1], "/");
   }
   strcat(def_files[1], SGE_HOME_DEF_REQ_FILE);

   if (!sge_is_file(def_files[1])) {
      strcpy(def_files[1], pwd->pw_dir);
      if (*def_files[1] && (def_files[1][strlen(def_files[1]) - 1] != '/')) {
         strcat(def_files[1], "/");
      }
      if (feature_is_enabled(FEATURE_SGEEE)) {
         strcat(def_files[1], GRD_HOME_DEF_REQ_FILE);
      } else {
         strcat(def_files[1], COD_HOME_DEF_REQ_FILE);
      }
   }

   /*
    * the defaults file in the current working directory
    */
   if (!getcwd(cwd, sizeof(cwd))) {
      sprintf(str, MSG_FILE_CANTREADCURRENTWORKINGDIR);
      answer_list_add(&answer, str, STATUS_EDISK, ANSWER_QUALITY_ERROR);
      return answer;
   }
   
   def_files[2] = malloc(strlen(cwd) + strlen(SGE_HOME_DEF_REQ_FILE) + 2);
   strcpy(def_files[2], cwd);
   if (*def_files[2] && (def_files[2][strlen(def_files[2]) - 1] != '/')) {
      strcat(def_files[2], "/");
   }
   strcat(def_files[2], SGE_HOME_DEF_REQ_FILE);
   if (!sge_is_file(def_files[2])) {
      strcpy(def_files[2], cwd);
      if (*def_files[2] && (def_files[2][strlen(def_files[2]) - 1] != '/')) {
         strcat(def_files[2], "/");
      }
      if (feature_is_enabled(FEATURE_SGEEE)) {
         strcat(def_files[2], GRD_HOME_DEF_REQ_FILE); 
      } else {
         strcat(def_files[2], COD_HOME_DEF_REQ_FILE);
      }
   }


   def_files[3] = NULL;

   /*
    * now read all the defaults files, unaware of where they came from
    */
   for (pstr = def_files; *pstr; pstr++) {
      int already_read;

      if (SGE_STAT(*pstr, &buf)<0) {
         DPRINTF(("-- defaults file %s does not exist\n", *pstr));
         continue;
      }

      already_read = 0; 
      for (ppstr = def_files; *ppstr != *pstr; ppstr++) {
         if (!sge_filecmp(*ppstr, *pstr)) {
            DPRINTF(("-- skipping %s as defaults file - already read as %s\n", 
               *pstr, *ppstr));
            already_read = 1; 
            break;
         }
      }
      if (already_read) {
         continue;
      }
      DPRINTF(("-- defaults file: %s\n", *pstr));

      alp = parse_script_file(*pstr, "", pcmdline, envp, 
         FLG_HIGHER_PRIOR | FLG_USE_NO_PSEUDOS);

      for_each(aep, alp) {
         u_long32 status, quality;

         status = lGetUlong(aep, AN_status);
         quality = lGetUlong(aep, AN_quality);

         if (quality == ANSWER_QUALITY_ERROR) {
            DPRINTF(("%s", lGetString(aep, AN_text)));
            if (status == STATUS_EDISK) {
               /*
               ** we turn this error into a warning here
               */
               quality = ANSWER_QUALITY_WARNING;
            }
            else {
               do_exit = 1;
            }
         }
         else {
            DPRINTF(("Warning: Error: %s\n", lGetString(aep, AN_text)));
         }
         answer_list_add(&answer, lGetString(aep, AN_text), status, quality);
      }

      if (do_exit) {
         for (pstr = def_files; *pstr; free(*pstr++));
         return answer;
      }
   }

   for (pstr = def_files; *pstr; free(*pstr++));
   DEXIT;
   return answer;
}

