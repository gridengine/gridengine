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
/*
   This is the module for handling users and projects.
   We save users to <spool>/qmaster/$USER_DIR and
                                    $PROJECT_DIR
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "sgermon.h"
#include "sge_gdi_intern.h"
#include "sge_answerL.h"
#include "sge_confL.h"
#include "sge_usageL.h"
#include "sge_userprjL.h"
#include "sge_usersetL.h"
#include "config.h"
#include "sge_feature.h"
#include "read_object.h"
#include "read_write_userprj.h"
#include "sge_log.h"
#include "sge_exit.h"
#include "sge_stdio.h"
#include "sge_language.h"
#include "msg_common.h"
#include "sge_spoolmsg.h"

static intprt_type intprt_as_usage[] = { UA_name, UA_value, 0 };
static intprt_type intprt_as_acl[] = { US_name, 0 };

static int read_userprj_work(lList **alpp, lList **clpp, int fields[], lListElem *ep, int spool, int user, int *tag, int parsing_type);

/************************************************************************
  write_userprj

  write a user or a project element human readable to file or fp. 
  If fname == NULL use fpout for writing.
  if spool!=0 store all information else store user relevant information
  schedd_conf file looks like this:

  name     auser
  oticket  15
  fshare   22
  usage    cpu=1.12,io=2.14
  long_term_usage    cpu=11.12,io=25.51
  acl      tralla,trulla
  xacl     trilla,trullala
  20       cpu=3.5,io=2.14
  21       cpu=2.4,io=2.24

 ************************************************************************/
int write_userprj(
lList **alpp,
lListElem *ep,
char *fname,
FILE *fpout,
int spool,
int user        /* =1 user, =0 project */
) {
   FILE *fp; 
   const char *delis[] = {"=", ",", "", NULL};
   lListElem *ju;
   char filename[SGE_PATH_MAX];
   int ret;

   DENTER(TOP_LAYER, "write_userprj");

   if (!ep) {
      CRITICAL((SGE_EVENT, MSG_RWUSERPRJ_EPISNULLNOUSERORPROJECTELEMENT));
      if (!alpp) {
         SGE_EXIT(1);
      } else {
         sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
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
            sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
            DEXIT;
            return -1;
         }
      }
   } else {
      fp = fpout;
   }

   if (spool && sge_spoolmsg_write(fp, COMMENT_CHAR,
             feature_get_product_name(FS_VERSION)) < 0) {
      goto FPRINTF_ERROR;
   } 

   FPRINTF((fp, "name %s\n", lGetString(ep, UP_name)));

   if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
      lList *pul;

      FPRINTF((fp, "oticket " u32 "\n", lGetUlong(ep, UP_oticket)));
      FPRINTF((fp, "fshare " u32 "\n", lGetUlong(ep, UP_fshare)));

      if (spool) {
         FPRINTF((fp, "usage "));
         ret = uni_print_list(fp, NULL, 0, lGetList(ep, UP_usage), 
            intprt_as_usage, delis,0);
         if (ret < 0) {
            goto FPRINTF_ERROR;
         }
         FPRINTF((fp, "\n"));
         FPRINTF((fp, "usage_time_stamp " u32 "\n", 
                        lGetUlong(ep, UP_usage_time_stamp)));
         FPRINTF((fp, "long_term_usage "));
         ret = uni_print_list(fp, NULL, 0, lGetList(ep, UP_long_term_usage),
            intprt_as_usage, delis,0);
         if (ret < 0) {
            goto FPRINTF_ERROR;
         }
         FPRINTF((fp, "\n"));
         FPRINTF((fp, "project "));
         if ((pul=lGetList(ep, UP_project))) {
            FPRINTF((fp, "{\n"));
            for_each (ju, pul) {
               const char *upp_name = lGetString(ju, UPP_name);

               if (strpbrk(upp_name, " \t`~!@#$%^&*()-_=+{}[]|:;'<>?,.")) {
                  FPRINTF((fp, "   \"%s\" {\n", upp_name));
               } else {
                  FPRINTF((fp, "   %s {\n", upp_name));
               }
               FPRINTF((fp, "      usage "));
               ret = uni_print_list(fp, NULL, 0, lGetList(ju, UPP_usage),
                  intprt_as_usage, delis,0);
               if (ret < 0) {
                  goto FPRINTF_ERROR;
               }
               FPRINTF((fp, "\n"));
               FPRINTF((fp, "      long_term_usage "));
               ret = uni_print_list(fp, NULL, 0, lGetList(ju, 
                  UPP_long_term_usage), intprt_as_usage, delis,0);
               if (ret < 0) {
                  goto FPRINTF_ERROR;
               }
               FPRINTF((fp, "\n"));
               FPRINTF((fp, "   }\n"));
            }
            FPRINTF((fp, "}\n"));
         } else {
            FPRINTF((fp, "NONE\n"));
         }
      }
   }

   if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR) && user) {
      const char *dproj = lGetString(ep, UP_default_project);
      FPRINTF((fp, "default_project %s\n", dproj ? dproj : "NONE"));
   }
   
   if (!user) {
      FPRINTF((fp, "acl "));
      ret = uni_print_list(fp, NULL, 0, lGetList(ep, UP_acl), intprt_as_acl, 
                     delis, 0);
      if (ret < 0) {
         goto FPRINTF_ERROR;
      }

      FPRINTF((fp, "\nxacl "));
      ret = uni_print_list(fp, NULL, 0, lGetList(ep, UP_xacl), intprt_as_acl, 
                     delis, 0);
      if (ret < 0) {
         goto FPRINTF_ERROR;
      }
      FPRINTF((fp, "\n"));
   }
                  
   if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR) && spool) {
      for_each (ju, lGetList(ep, UP_debited_job_usage)) {
         FPRINTF((fp, u32" ", lGetUlong(ju, UPU_job_number)));
         ret = uni_print_list(fp, NULL, 0, lGetList(ju, UPU_old_usage_list), 
                        intprt_as_usage, delis,0);
         if (ret < 0) {
            goto FPRINTF_ERROR;
         }
         FPRINTF((fp, "\n"));
      }
   }

   if (fname) {
      fclose(fp);
   }

   DEXIT;
   return 0;

FPRINTF_ERROR:
   sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0); 
   DEXIT;
   return -1;   
}

/****
 **** read_userprj_work
 ****
 ****/
static int read_userprj_work(
lList **alpp,
lList **clpp,
int fields[],
lListElem *ep,
int spool,
int user,
int *tag,
int parsing_type 
) {
   char *rest;
   const char *name;
   u_long32 job_number;
   lListElem *upu = NULL;
   lListElem *cp = NULL;
   lListElem *next = NULL;

   DENTER(TOP_LAYER, "read_userprj_work");

   /* --------- UP_name */
   if (!set_conf_string(alpp, clpp, fields, "name", ep, UP_name)) {
      DEXIT;
      return -1;
   }

   if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
      /* --------- UP_oticket */
      if (!set_conf_ulong(alpp, clpp, fields, "oticket", ep, UP_oticket)) {
         DEXIT;
         return -1;
      }

      /* --------- UP_fshare */
      if (!set_conf_ulong(alpp, clpp, fields, "fshare", ep, UP_fshare)) {
         DEXIT;
         return -1;
      }

      /* --------- UP_default_project */
      if (user) {
         if (!set_conf_string(alpp, clpp, fields, "default_project", ep,
                UP_default_project)) {
            DEXIT;
            return -1;
         }
         NULL_OUT_NONE(ep, UP_default_project);
      }

      if (spool) {

         lList *sclp = NULL, *ssclp = NULL;
         const char *val;

         /* --------- UP_usage */
         if (!set_conf_deflist(alpp, clpp, fields, "usage", ep, 
                  UP_usage, UA_Type, intprt_as_usage)) {
            DEXIT;
            return -1;
         }

         /* --------- UP_usage_time_stamp */
         if (!set_conf_ulong(alpp, clpp, fields, "usage_time_stamp", ep, 
                  UP_usage_time_stamp)) {
            DEXIT;
            return -1;
         }

         /* --------- UP_long_term_usage */
         if (!set_conf_deflist(alpp, clpp, fields, "long_term_usage", ep, 
                  UP_long_term_usage, UA_Type, intprt_as_usage)) {
            DEXIT;
            return -1;
         }

         if ((sclp=get_conf_sublist(fields?NULL:alpp, *clpp, CF_name,
                  CF_sublist, "project"))) {

            lListElem *upp;

            next = lFirst(sclp);
            while ((cp = next)) {
               next = lNext(cp);
               name = lGetString(cp, CF_name);
               add_nm_to_set(fields, UP_project);

               if ((lGetList(cp, CF_sublist))) {

                  ssclp = lCopyList(NULL, lGetList(cp, CF_sublist));

                  /* --------- UPP_name */
                  if (!(upp = lAddSubStr(ep, UPP_name, name, UP_project, UPP_Type))) {
                     sprintf(SGE_EVENT, MSG_PROJECT_FOUNDPROJECTXTWICE_S, name);
                     sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX,
                            NUM_AN_ERROR);
                     DEXIT;
                     return -1;
                  }
                  add_nm_to_set(fields, UPP_name);

                  /* --------- UPP_usage */
                  if (!set_conf_deflist(alpp, &ssclp, fields, "usage", upp, 
                           UPP_usage, UA_Type, intprt_as_usage)) {
                     DEXIT;
                     return -1;
                  }

                  /* --------- UPP_long_term_usage */
                  if (!set_conf_deflist(alpp, &ssclp, fields,
                            "long_term_usage", upp, UPP_long_term_usage,
                            UA_Type, intprt_as_usage)) {
                     DEXIT;
                     return -1;
                  }

                  if (ssclp)
                     lFreeList(ssclp);

                  lSetList(cp, CF_sublist, NULL);  /* frees old list */

               } else if ((val = lGetString(cp, CF_value))) {

                  if (!strcasecmp("NONE", val)) {
                     sprintf(SGE_EVENT, MSG_PROJECT_INVALIDPROJECTX_S, name);
                     sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX,
                            NUM_AN_ERROR);
                     DEXIT;
                     return -1;
                  }

               }

               /* lFreeElem(lDechainElem(sclp, cp)); */
            }

         }

         lDelElemStr(clpp, CF_name, "project");

      }
   }

   if (!user) {
      /* --------- UP_acl */
      if (!set_conf_list(alpp, clpp, fields, "acl", ep, UP_acl, US_Type, US_name)) {
         DEXIT;
         return -1;
      }

      /* --------- UP_xacl */
      if (!set_conf_list(alpp, clpp, fields, "xacl", ep, UP_xacl, US_Type, US_name)) {
         DEXIT;
         return -1;
      }
   }

   /* --------- now read in the jobs */
   if (spool) {
      next = lFirst(*clpp);
      while ((cp = next)) {
         next = lNext(cp);
         name = lGetString(cp, CF_name);
         job_number = (u_long32)strtol(name, &rest, 10);
         if (*rest != '\0') {
            sprintf(SGE_EVENT, MSG_JOB_FOUNDJOBWITHWRONGKEY_S, name);
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, NUM_AN_ERROR);
            DEXIT;
            return -1;
         }
         add_nm_to_set(fields, UP_debited_job_usage);

         if (!(upu = lAddSubUlong(ep, UPU_job_number, job_number, 
                                    UP_debited_job_usage, UPU_Type))) {
            sprintf(SGE_EVENT, MSG_JOB_FOUNDJOBXTWICE_U, u32c(job_number));
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, NUM_AN_ERROR);
            DEXIT;
            return -1;
         }

         if (!set_conf_deflist(alpp, clpp, fields, name, upu, 
                  UPU_old_usage_list, UA_Type, intprt_as_usage)) {
            DEXIT;
            return -1;
         }
      }
   }

   DEXIT;
   return 0;
}

void
dump_list_to_file(lList *list, const char *file)
{
   FILE *f;

   if (!(f=fopen(file, "w+"))) {
      fprintf(stderr, MSG_ERROR_COULDNOTOPENSTDOUTASFILE);
      exit(1);
   }

   if (lDumpList(f, list, 0) == EOF) {
      fprintf(stderr, MSG_ERROR_UNABLETODUMPJOBLIST);
   }

   fclose(f);
}


/***************************************************
 Generate a Template for a user or project
 ***************************************************/
lListElem *getUserPrjTemplate()
{
   lListElem *ep;

   DENTER(TOP_LAYER, "getUserPrjTemplate");

   ep = lCreateElem(UP_Type);
   lSetString(ep, UP_name, "template");
   lSetString(ep, UP_default_project, NULL);
   lSetUlong(ep, UP_oticket, 0);
   lSetUlong(ep, UP_fshare, 0);
   lSetUlong(ep, UP_job_cnt, 0);
   lSetList(ep, UP_project, NULL);
   lSetList(ep, UP_usage, NULL);
   lSetList(ep, UP_long_term_usage, NULL);
   lSetList(ep, UP_acl, NULL);
   lSetList(ep, UP_xacl, NULL);

   DEXIT;
   return ep;
}

/****
 **** cull_read_in_userprj
 ****/
lListElem *cull_read_in_userprj(
const char *dirname,
const char *filename,
int spool,
int user,
int *tag 
) {
   lListElem *ep;
   struct read_object_args args = { UP_Type, "userprj", read_userprj_work };
   int intern_tag = 0;

   DENTER(TOP_LAYER, "cull_read_in_userprj");

   ep = read_object(dirname, filename, spool, user, 0,&args, tag?tag:&intern_tag, NULL);

   DEXIT;
   return ep;
}


#if 0

int main(int argc, char *argv[])
{
   lListElem *ep, *sep, *ssep;
   lList *alp = NULL;
  
   DENTER_MAIN(TOP_LAYER, "sge_userprj");


#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   install_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL); 
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */


   lInit(nmv);
   ep = lCreateElem(UP_Type);

   lSetString(ep, UP_name, "andreas");

   sep = lAddSubStr(ep, UA_name, "mem", UP_usage, UA_Type);
   lSetDouble(sep, UA_value, 1.5);
   sep = lAddSubStr(ep, UA_name, "io", UP_usage, UA_Type);
   lSetDouble(sep, UA_value, 2.5);
   sep = lAddSubStr(ep, UA_name, "cpu", UP_usage, UA_Type);
   lSetDouble(sep, UA_value, 3.5);

   sep = lAddSubUlong(ep, UPU_job_number, 20, UP_debited_job_usage, UPU_Type);
      ssep = lAddSubStr(sep, UA_name, "mem", UPU_old_usage_list, UA_Type);
      lSetDouble(ssep, UA_value, 1.5);
      ssep = lAddSubStr(sep, UA_name, "io", UPU_old_usage_list, UA_Type);
      lSetDouble(ssep, UA_value, 2.5);
      ssep = lAddSubStr(sep, UA_name, "cpu", UPU_old_usage_list, UA_Type);
      lSetDouble(ssep, UA_value, 3.5);

   sep = lAddSubUlong(ep, UPU_job_number, 21, UP_debited_job_usage, UPU_Type);
      ssep = lAddSubStr(sep, UA_name, "mem", UPU_old_usage_list, UA_Type);
      lSetDouble(ssep, UA_value, 1.25);
      ssep = lAddSubStr(sep, UA_name, "io", UPU_old_usage_list, UA_Type);
      lSetDouble(ssep, UA_value, 2.25);
      ssep = lAddSubStr(sep, UA_name, "cpu", UPU_old_usage_list, UA_Type);
      lSetDouble(ssep, UA_value, 3.25);

   lWriteElemTo(ep, stdout);
   if (write_userprj(NULL, ep, "/sge_home/andreas/andreas", NULL, 1, 1))
      perror("failed writing user object\n");

   lFreeElem(ep);

   if (!(ep = read_userprj("/sge_home/andreas/andreas", 1, &alp)))
      fprintf(stderr, "failed reading user object: %s\n", 
                     lGetString(lFirst(alp), AN_TEXT));
   else
      lWriteElemTo(ep, stdout);

   alp = lFreeList(alp);

   DCLOSE;
   return 0;
}
#endif

