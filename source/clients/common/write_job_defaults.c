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

#include "def.h"
#include "symbols.h"
#include "sge_gdi_intern.h"
#include "sge_ja_task.h"
#include "sge_answer.h"
#include "sge_string.h"
#include "sge_time.h"
#include "parse_qsubL.h"
#include "sge_stringL.h"
#include "sge_identL.h"
#include "sge_job_refL.h"
#include "sge_resource.h"
#include "sgermon.h"                       
#include "sge_answer.h"
#include "unparse_job_cull.h"
#include "write_job_defaults.h"
#include "msg_common.h"

static lList *write_defaults_file(lList *lp, char *filename, int flags);

/*
** NAME
**   write_job_defaults   -  defaults file from job structure
** PARAMETER
**   job       -  Job, JB_Type
**   filename  -  file to write defaults to
**   flags     -  FLG_FULL_CMDLINE or 0
**
** RETURN
**   answer list, AN_Type, or NULL if everything ok, 
**   the following stati can occur:
**   
** EXTERNAL
**   
** DESCRIPTION
**   unparses a job element into a default file
**   containing the option switches 
*/
lList *write_job_defaults(
lListElem *job,
char *filename,
int flags 
) {
   lList *alp;
   lListElem *aep;
   lList *cmdline = NULL;
   int do_exit = 0;
   int status;
   
   DENTER(TOP_LAYER, "write_job_defaults");

   alp = cull_unparse_job_parameter(&cmdline, job, flags);
   for_each(aep, alp) {
      answer_exit_if_not_recoverable(aep);
      status = answer_get_status(aep); 
      if ((status != STATUS_OK) && (status != STATUS_EEXIST)) {
         do_exit = 1;
      }
   }
   if (do_exit) {
      DEXIT;
      return alp;
   }
   lFreeList(alp);

   alp = write_defaults_file(cmdline, filename, flags);
   for_each(aep, alp) {
      answer_exit_if_not_recoverable(aep);
      status = answer_get_status(aep);
      if ((status != STATUS_OK) && (status != STATUS_EEXIST)) {
         do_exit = 1;
      }
   }
    if (do_exit) {
      DEXIT;
      return alp;
   }
   lFreeList(alp);
   
   DEXIT;
   return NULL;
}

/*
** NAME
**   write_defaults_file   - writing job options
** PARAMETER
**   lp         - list of options, SPA_Type
**   filename   - name of file to write options to, 
**                or NULL, in this case stdout is used
**   flags      - FLG_FULL_CMDLINE or 0
**
** RETURN
**   answer list, AN_Type, or NULL if everything ok, 
**   the following stati can occur:
**   STATUS_EDISK     - error opening or writing to filename
**   STATUS_ESYNTAX   - the original command line string was
**                      not saved
**   
** EXTERNAL
**   
** DESCRIPTION
**   this function writes the given options to a file without
**   unparsing them. The following fields are written:
**   SPA_switch, then a blank (except for -l) and then
**   SPA_switch_arg, if the option had an argument on the commandline.
**   The switch -l is recognised separately, and no blank is
**   inserted between -l and the argument. This is a restriction to
**   the general usability of this function.
*/
static lList *write_defaults_file(
lList *lp,
char *filename,
int flags 
) {
   lListElem *ep;
   const char *cp;
   FILE *fp;
   lList *answer = NULL;
   int i = 0;
   char str[256 + 1];
   
   DENTER(BASIS_LAYER, "write_defaults_file");
   
   if (!filename) {
      fp = stdout;
   }
   else {
      fp = fopen(filename, "w");
      if (!fp) {
         sprintf(str, MSG_FILE_ERROROPENFILEXFORWRITING_S, filename);
         answer_list_add(&answer, str, STATUS_EDISK, ANSWER_QUALITY_ERROR);
         DEXIT;
         return answer;
      }
   }
   for_each(ep, lp) {
      cp = lGetString(ep, SPA_switch);
      /*
      ** only real options are written to file
      ** except if the caller asks for full commandline
      */
      if (!cp) {
         continue;
      }
      if ((*cp != '-') && !(flags & FLG_FULL_CMDLINE)) {
         continue;
      }
      /*
      ** problem: exception for -l makes this function slightly
      ** less generally appliccable
      */
      if ((strlen(cp) > 1) && !strncmp(cp, "-l", 2)) {
         i = fprintf(fp, "%s", cp);
         i++;
      }
      else if (*cp == '-') {
         i = fprintf(fp, "%s ", cp);
      }
      if ((*cp == '-') && (i != (int) strlen(cp) + 1)) {
         sprintf(str, MSG_FILE_ERRORWRITETOFILEX_S, filename);
         answer_list_add(&answer, str, STATUS_EDISK, ANSWER_QUALITY_ERROR);
         if (filename)
            fclose(fp);
         DEXIT;
         return answer;
      }
      if (lGetUlong(ep, SPA_occurrence) & BIT_SPA_OCC_ARG) {
         cp = lGetString(ep, SPA_switch_arg);
         if (!cp) {
            sprintf(str, MSG_ANSWER_ARGUMENTMISSINGFORX_S , 
                    lGetString(ep, SPA_switch));
            answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            if (filename)
               fclose(fp);
            DEXIT;
            return answer;
         }
         i = fprintf(fp, "%s ", cp);
         if (i != (int) strlen(cp) + 1) {
            sprintf(str, MSG_FILE_ERRORWRITETOFILEX_S, filename);
            answer_list_add(&answer, str, STATUS_EDISK, ANSWER_QUALITY_ERROR);
            if (filename)
               fclose(fp);
            DEXIT;
            return answer;
         }
      }
   }

   fprintf(fp, "\n");

   if (filename) {
      fclose(fp);
   }

   DEXIT;
   return answer;
}


#ifdef TEST
#include "sge_all_listsL.h"
int main(int argc, char **argv, char **envp);

int main(
int argc,
char **argv,
char **envp 
) {
   lList *cmdline = NULL;
   lListElem *job = NULL;
   lList *alp;
   lListElem *aep;
   u_long32 status = STATUS_OK;
   int do_exit = 0;

   DENTER_MAIN(TOP_LAYER, "test_write_defaults");

#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   sge_init_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL); 
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */


   lInit(nmv);
   sge_setup(QSUB);
   alp = cull_parse_cmdline(argv + 1, envp, &cmdline, FLG_USE_PSEUDOS);
   for_each(aep, alp) {
      answer_exit_if_not_recoverable(aep);
      status = answer_get_status(aep);
      if ((status != STATUS_OK) && (status != STATUS_EEXIST)) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         do_exit = 1;
      }
      else {
         printf("%s\n", lGetString(aep, AN_text));
      }
   }
   lFreeList(alp);
   if (do_exit) {
      SGE_EXIT(status);
   }

   alp = cull_parse_job_parameter(cmdline, &job);
   for_each(aep, alp) {
      answer_exit_if_not_recoverable(aep);
      status = answer_get_status(aep);
      if (status != STATUS_OK) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         do_exit = 1;
      }
      else {
         printf("%s\n", lGetString(aep, AN_text));
      }
   }
   lFreeList(alp);
   if (do_exit) {
      SGE_EXIT(status);
   }

   alp = write_job_defaults(job, "wdtest.dat", FLG_FULL_CMDLINE);
   for_each(aep, alp) {
      answer_exit_if_not_recoverable(aep);
      status = answer_get_status(aep);
      if (status != STATUS_OK) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         do_exit = 1;
      }
      else {
         printf("%s\n", lGetString(aep, AN_text));
      }
   }
   lFreeList(alp);
   if (do_exit) {
      SGE_EXIT(status);
   }

   return 0;
}

#endif

#ifdef TEST
#include "sge_all_listsL.h"
int main(int argc, char **argv, char **envp);

int main(
int argc,
char **argv,
char **envp 
) {
   lList *answer;
   lList *cmdline;


#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   install_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_lang_init(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */


   lInit(nmv);

   answer = parse_script_file(*(argv + 1), "", &cmdline, envp, FLG_USE_NO_PSEUDOS);
   if (answer) {
      lDumpList(stdout, answer, 0);
      exit(1);
   }
   lDumpList(stdout, cmdline, 4);

   answer = write_defaults_file(cmdline, NULL, 0);
   if (answer) {
      lDumpList(stdout, answer, 0);
      exit(1);
   }
   return 0;
}

#endif

