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
#include "sge_answer.h"
#include "parse_job_cull.h"
#include "parse_qsubL.h"
#include "parse_qsub.h"
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

/****** sge/opt/opt_list_append_opts_from_default_files() *********************
*  NAME
*     opt_list_append_opts_from_default_files() -- parse default files 
*
*  SYNOPSIS
*     void opt_list_append_opts_from_default_files(
*                                         lList **pcmdline, 
*                                         lList **answer_list
*                                         char **envp) 
*
*  FUNCTION
*     This function reads the 3 defaults files if they exist and parses them
*     into an options list. 
*
*  INPUTS
*     lList **pcmdline - pointer to SPA_Type list, if list is NULL, it is
*                        created if the files contain any options 
*     lList* - answer list, AN_Type or NULL if everything ok
*        possible errors:
*           STATUS_ENOSUCHUSER - could not retrieve passwd info on me.user_name
*           STATUS_EDISK       - home directory for user is missing or cwd 
*                                cannot be read or file could not be opened 
*                                (is just a warning)
*           STATUS_EEXIST      - (parse_script_file), (is just a warning)
*           STATUS_EUNKNOWN    - (parse_script_file), error opening or 
*                                reading from existing file, (is just a warning)
*                                plus all other error stati returned by 
*                                parse_script_file, see there
*     char **envp      - environment pointer 
*
*******************************************************************************/
void opt_list_append_opts_from_default_files(lList **pcmdline, 
                                             lList **answer_list,
                                             char **envp) 
{
   lList *alp;
   lListElem *aep;
   struct passwd *pwd;
   char str[256 + 1];
   char cwd[SGE_PATH_MAX + 1];
   char **pstr;
   char **ppstr;
   SGE_STRUCT_STAT buf;
   int do_exit = 0;
#ifdef HAS_GETPWNAM_R
   struct passwd pw_struct;
   char buffer[2048];
#endif
   
   DENTER(TOP_LAYER, "opt_list_append_opts_from_default_files");

   if (*answer_list) {
      *answer_list = lFreeList(*answer_list);
   }

   /* the sge root defaults file */
   def_files[0] = malloc(strlen(path_state_get_sge_root()) + 
                         strlen(SGE_COMMON_DEF_REQ_FILE) + 2);
   sprintf(def_files[0], "%s/%s", path_state_get_sge_root(), SGE_COMMON_DEF_REQ_FILE);

   /*
    * the defaults file in the user's home directory
    */

#ifdef HAS_GETPWNAM_R
   pwd = sge_getpwnam_r(uti_state_get_user_name(), &pw_struct, buffer, sizeof(buffer));
#else
   pwd = sge_getpwnam(uti_state_get_user_name());
#endif
   if (!pwd) {
      sprintf(str, MSG_USER_INVALIDNAMEX_S, uti_state_get_user_name());
      answer_list_add(answer_list, str, STATUS_ENOSUCHUSER, 
                      ANSWER_QUALITY_ERROR);
      return;
   }
   if (!pwd->pw_dir) {
      sprintf(str, MSG_USER_NOHOMEDIRFORUSERX_S, uti_state_get_user_name());
      answer_list_add(answer_list, str, STATUS_EDISK, ANSWER_QUALITY_ERROR);
      return;
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
      answer_list_add(answer_list, str, STATUS_EDISK, ANSWER_QUALITY_ERROR);
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
         answer_list_add(answer_list, lGetString(aep, AN_text), status, 
                         quality);
      }

      if (do_exit) {
         for (pstr = def_files; *pstr; free(*pstr++)) {
            ;
         }
         return;
      }
   }

   for (pstr = def_files; *pstr; free(*pstr++)) {
      ;
   }

   DEXIT;
   return;
}

/****** sge/opt/opt_list_append_opts_from_qsub_cmdline() **********************
*  NAME
*     opt_list_append_opts_from_qsub_cmdline() -- parse opts from cmd line 
*
*  SYNOPSIS
*     void opt_list_append_opts_from_qsub_cmdline(lList **opts_cmdline, 
*                                                 lList **answer_list, 
*                                                 char **argv, 
*                                                 char **envp) 
*
*  FUNCTION
*     Parse options from the qsub commandline given by "argv" and store
*     the parsed objects in "opts_cmdline". If an error occures store
*     the error/warning messages in the "answer_list". 
*     "envp" is a pointer to the process environment.
*     
*
*  INPUTS
*     lList **opts_cmdline - command line options 
*     lList **answer_list  - AN_Type list 
*     char **argv          - Argumente 
*     char **envp          - Environment 
*
*  RESULT
*     void - None
*******************************************************************************/
void opt_list_append_opts_from_qsub_cmdline(lList **opts_cmdline,
                                            lList **answer_list,
                                            char **argv,
                                            char **envp)
{
   if (*answer_list != NULL) {
      *answer_list = lFreeList(*answer_list);
   }
   *answer_list = cull_parse_cmdline(argv, envp, opts_cmdline, FLG_USE_PSEUDOS);
}

/****** sge/opt/opt_list_append_opts_from_qalter_cmdline() ********************
*  NAME
*     opt_list_append_opts_from_qalter_cmdline() -- parse opts from cmd line 
*
*  SYNOPSIS
*     void opt_list_append_opts_from_qalter_cmdline(lList **opts_cmdline, 
*                                                   lList **answer_list, 
*                                                   char **argv, 
*                                                   char **envp) 
*
*  FUNCTION
*     Parse options from the qalter commandline given by "argv" and store
*     the parsed objects in "opts_cmdline". If an error occures store
*     the error/warning messages in the "answer_list". 
*     "envp" is a pointer to the process environment.
*     
*
*  INPUTS
*     lList **opts_cmdline - command line options 
*     lList **answer_list  - AN_Type list 
*     char **argv          - Argumente 
*     char **envp          - Environment 
*
*  RESULT
*     void - None
*******************************************************************************/
void opt_list_append_opts_from_qalter_cmdline(lList **opts_cmdline,
                                              lList **answer_list,
                                              char **argv,
                                              char **envp)
{
   if (*answer_list != NULL) {
      *answer_list = lFreeList(*answer_list);
   }
   *answer_list = cull_parse_cmdline(argv, envp, opts_cmdline, 
                                     FLG_USE_PSEUDOS | FLG_QALTER);
}

/****** sge/opt/opt_list_append_opts_from_script() ****************************
*  NAME
*     opt_list_append_opts_from_script() -- parse opts from scriptfile 
*
*  SYNOPSIS
*     void opt_list_append_opts_from_script(lList **opts_scriptfile, 
*                                           lList **answer_list, 
*                                           const lList *opts_cmdline, 
*                                           char **envp) 
*
*  FUNCTION
*     This function parses the commandline options which are embedded
*     in scriptfile (jobscript) and stores the parsed objects in
*     opts_scriptfile. The filename of the scriptfile has to be
*     contained in the list "opts_cmdline" which has been previously i
*     created with opt_list_append_opts_from_*_cmdline(). "answer_list"
*     will be used to store error/warning messages.
*     "envp" is a pointer to the process environment.
*
*  INPUTS
*     lList **opts_scriptfile   - embedded command line options 
*     lList **answer_list       - AN_Type list 
*     const lList *opts_cmdline - Argumente 
*     char **envp               - Environment 
*
*  RESULT
*     void - None
*******************************************************************************/
void opt_list_append_opts_from_script(lList **opts_scriptfile, 
                                      lList **answer_list,
                                      const lList *opts_cmdline,
                                      char **envp) 
{ 
   lListElem *script_option = NULL;
   lListElem *c_option = NULL;
   const char *scriptfile = NULL;
   const char *prefix = NULL;
 
   script_option = lGetElemStr(opts_cmdline, SPA_switch, STR_PSEUDO_SCRIPT);
   if (script_option != NULL) {
      scriptfile = lGetString(script_option, SPA_argval_lStringT);
   }
   c_option = lGetElemStr(opts_cmdline, SPA_switch, "-C");
   if (c_option != NULL) {
      prefix = lGetString(c_option, SPA_argval_lStringT);
   } else {
      prefix = default_prefix;
   }
   if (*answer_list) {
      *answer_list = lFreeList(*answer_list);
   }
   *answer_list = parse_script_file(scriptfile, prefix, opts_scriptfile, 
                                    envp, FLG_DONT_ADD_SCRIPT);
}

/****** sge/opt/opt_list_merge_command_lines() ********************************
*  NAME
*     opt_list_merge_command_lines() -- merge commandlines together
*
*  SYNOPSIS
*     void opt_list_merge_command_lines(lList **opts_all, 
*                                       lList **opts_defaults, 
*                                       lList **opts_scriptfile, 
*                                       lList **opts_cmdline) 
*
*  FUNCTION
*     Merge "opts_defaults", "opts_scriptfile" and "opts_cmdline" into
*     "opts_all".
*
*     Options to a sge submit can come from different sources:
*      - default settings (sge/sge_request)
*      - special comments in scriptfiles (override default settings)
*      - command line options (override default settings and special 
*        comments) 
*
*  INPUTS
*     lList **opts_all        - destination commandline 
*     lList **opts_defaults   - opts from default files 
*     lList **opts_scriptfile - opts from the script 
*     lList **opts_cmdline    - commandline options 
*
*  RESULT
*     void - None
*******************************************************************************/
void opt_list_merge_command_lines(lList **opts_all,
                                  lList **opts_defaults,
                                  lList **opts_scriptfile,
                                  lList **opts_cmdline)
{
   /*
    * Order is very important here
    */
   if (*opts_defaults != NULL) {
      if (*opts_all == NULL) {
         *opts_all = *opts_defaults;
      } else {
         lAddList(*opts_all, *opts_defaults);
      }
      *opts_defaults = NULL;
   }
   if (*opts_scriptfile != NULL) {
      if (*opts_all == NULL) {
         *opts_all = *opts_scriptfile;
      } else {
         lAddList(*opts_all, *opts_scriptfile);
      }
      *opts_scriptfile = NULL;
   }
   if (*opts_cmdline != NULL) {
      if (*opts_all == NULL) {
         *opts_all = *opts_cmdline;
      } else {
         lAddList(*opts_all, *opts_cmdline);
      }
      *opts_cmdline = NULL;
   }
}

/****** sge/opt/opt_list_has_X() **********************************************
*  NAME
*     opt_list_has_X() -- is a certail option contained in list 
*
*  SYNOPSIS
*     int opt_list_has_X(lList *opts, const char *option) 
*
*  FUNCTION
*     This function returns true (1) if the given 'option' 
*     (e.g. "-help") is contained in the list 'opts'.
*
*  INPUTS
*     lList *opts        - SPA_Type list 
*     const char *option - switch name  
*
*  RESULT
*     int - found switch?
*        1 - yes
*        0 - no
*
*  SEE ALSO
*     sge/opt/opt_list_is_X_true()
*******************************************************************************/
int opt_list_has_X(lList *opts, const char *option) 
{
   lListElem *opt;
   int ret = 0;

   opt = lGetElemStr(opts, SPA_switch, option);
   if (opt != NULL) {
      ret = 1;
   }
   return ret;
}

/****** sge/opt/opt_list_is_X_true() ******************************************
*  NAME
*     opt_list_is_X_true() -- check the state of a boolean switch 
*
*  SYNOPSIS
*     int opt_list_is_X_true(lList *opts, const char *option) 
*
*  FUNCTION
*     This function returns true (1) if the given 'option'
*     (e.g. "-b") is contained in the list 'opts' and if
*     it was set to 'true'. If the value of the boolean switch
*     is false than the function will also return false (0).
*
*  INPUTS
*     lList *opts        - SPA_Type list 
*     const char *option - switch name 
*
*  RESULT
*     int - found switch with value 'true'
*        1 - yes
*        0 - no 
*
*  SEE ALSO
*     sge/opt/opt_list_has_X()
******************************************************************************/
int opt_list_is_X_true(lList *opts, const char *option) 
{
   lListElem *opt;
   int ret = 0;

   opt = lGetElemStr(opts, SPA_switch, option);
   if (opt != NULL) {
      ret = (lGetInt(opt, SPA_argval_lIntT) == 1);
   }
   return ret;
}

