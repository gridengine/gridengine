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
#include <ctype.h>

#include "sge_unistd.h"
#include "sgermon.h"
#include "sge.h"
#include "sgeobj/sge_resource_quota.h"
#include "sge_answer.h"
#include "spool/classic/read_write_resource_quota.h"
#include "sge_string.h"
#include "uti/sge_log.h"
#include "config.h"
#include "read_object.h"
#include "sge_stdio.h"
#include "msg_common.h"
#include "spool/classic/msg_spoollib_classic.h"
#include "sge_feature.h"
#include "sge_spool.h"
#include "sge_io.h"
#include "sge_resource_utilization.h"
#include "sgeobj/sge_strL.h"
#include "uti/sge_unistd.h"

/****** read_write_resource_quota/sge_read_RQRF_obj() ******************************
*  NAME
*     sge_read_RQRF_obj() -- parse elem from string
*
*  SYNOPSIS
*     static int sge_read_RQRF_obj(lListElem *ep, int nm, const char *buffer, 
*     lList **alpp) 
*
*  FUNCTION
*     Converts a string buffer to RQRF Type Element
*
*  INPUTS
*     lListElem *ep      - Store for the parsed Element
*     int nm             - Type of Element to parse
*     const char *buffer - buffer to parse
*     lList **alpp        - Answer List
*
*  RESULT
*     static int - 1 on success
*                  0 on error
*
*  NOTES
*     MT-NOTE: sge_read_RQRF_obj() is MT safe 
*
*******************************************************************************/
static int sge_read_RQRF_obj(lListElem *ep, int nm, const char *buffer,
                             lList **alpp) 
{
   lListElem *filter = NULL;
   int ret = 0;

   DENTER(TOP_LAYER, "sge_read_RQRF_obj");

   if (rqs_parse_filter_from_string(&filter, buffer, alpp)) { 
      ret = 1;
      lSetObject(ep, nm, filter);
   }

   DRETURN(ret);
}

/****** read_write_resource_quota/cull_read_in_rqs_list() *******************
*  NAME
*     cull_read_in_rqs_list() -- converts a file to a resource quota set list
*
*  SYNOPSIS
*     lList* cull_read_in_rqs_list(const char *fname, lList **alpp) 
*
*  FUNCTION
*     Reads in spooled resource quota sets
*
*  INPUTS
*     const char *fname - file to read
*     lList **alpp      - answer list
*
*  RESULT
*     lList* - List of RQS_Type
*
*  NOTES
*     MT-NOTE: cull_read_in_rqs_list() is MT safe 
*
*******************************************************************************/
lList *cull_read_in_rqs_list(const char *fname, lList **alpp) 
{
   FILE *fp;
   lList *lp = NULL;
   lListElem *ep = NULL;
   char buffer[10000];
   char *cp = NULL;
   struct saved_vars_s *last = NULL;
   char *name; 
   char *value;
   lList *lp_rules = NULL;
   
   DENTER(TOP_LAYER, "cull_read_in_rqs_list");

   lp = lCreateList("rqs_list", RQS_Type);

   if (!(fp = fopen(fname, "r"))) {
      ERROR((SGE_EVENT, MSG_FILE_NOOPEN_SS, fname, strerror(errno)));
      answer_list_add(alpp, SGE_EVENT, STATUS_EDISK, ANSWER_QUALITY_ERROR);
      lFreeList(&lp);
      DRETURN(NULL);
   }

   while (fgets(buffer, sizeof(buffer), fp)) {

      if (last) {
         sge_free_saved_vars(last);
         last = NULL;
      }

      /* skip empty and comment lines */
      if (buffer[0] == '#')
         continue;
      for(cp = buffer; *cp == ' ' || *cp == '\t'; cp++)
         ;
      if (*cp == '\n')
         continue;

      /* get name and value */
      if (*cp != '"' || !(name = sge_strtok_r(cp+1, "\"", &last))) {
         if (!(name = sge_strtok_r(buffer, NULL, &last)))
            break;
      }
      value = sge_strtok_r(NULL, "\n", &last);

      /* skip leading spaces */
      if (value) {
         while (*value && isspace((int)*value))
            value++;
      }

      /* handle begin of resource quota set */
      if (strcmp(name, "{") == 0 && !value) {
         if (ep != NULL) {
            ERROR((SGE_EVENT, MSG_RESOURCE_QUOTA_RULESETNOTFINISHED));
            answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            break;
         } else {
            ep = lCreateElem(RQS_Type); 
            lp_rules = lCreateList("rqr_list", RQR_Type);
            continue;
         }
      }

      /* handle end of resource_quota set */
      else if (strcmp(name, "}") == 0 && !value) {
         if (ep == NULL) {
            ERROR((SGE_EVENT, MSG_RESOURCE_QUOTA_RULESETNOTSTARTED));
            answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            break;
         } else {
            lSetList(ep, RQS_rule, lp_rules);
            lAppendElem(lp, ep);
            ep = NULL;
            continue;
         }
      }

      /* RQS_name */
      else if (strcmp(name, "name") == 0) {
         lSetString(ep, RQS_name, value); 
      }
      
      /* RQS_description */
      else if (strcmp(name, "description") == 0) {
         lSetString(ep, RQS_description, value); 
      }

      /* RQS_enabled */
      else if (strcmp(name, "enabled") == 0) {
         object_parse_bool_from_string(ep, alpp, RQS_enabled, value);
      }
      /* RQS_limit */
      else if (strcmp(name, "limit") == 0) {
         lListElem *rule = lCreateElem(RQR_Type);
         struct saved_vars_s *context = NULL;
         const char dlmt[] = "\t \v\r";
         char *field;

         for (field = sge_strtok_r(value, dlmt, &context); field; field = sge_strtok_r(NULL, dlmt, &context)) {
            /* RQR_name */
            if (strcmp(field, "name") == 0) {
               field = sge_strtok_r(NULL, dlmt, &context);
               lSetString(rule, RQR_name, field);
            }
            /* RQR_filter_users */
            else if (strcmp(field, "users") == 0) {
               field = sge_strtok_r(NULL, dlmt, &context);
               sge_read_RQRF_obj(rule, RQR_filter_users, field, alpp); 
            } 
            /* RQR_filter_projects */
            else if (strcmp(field, "projects") == 0) {
               field = sge_strtok_r(NULL, dlmt, &context);
               sge_read_RQRF_obj(rule, RQR_filter_projects, field, alpp); 
            }
            /* RQR_filter_pes */
            else if (strcmp(field, "pes") == 0) {
               field = sge_strtok_r(NULL, dlmt, &context);
               sge_read_RQRF_obj(rule, RQR_filter_pes, field, alpp); 
            }
            /* RQR_filter_queues */
            else if (strcmp(field, "queues") == 0) {
               field = sge_strtok_r(NULL, dlmt, &context);
               sge_read_RQRF_obj(rule, RQR_filter_queues, field, alpp); 
            }
            /* RQR_filter_hosts */
            else if (strcmp(field, "hosts") == 0) {
               field = sge_strtok_r(NULL, dlmt, &context);
               sge_read_RQRF_obj(rule, RQR_filter_hosts, field, alpp); 
            }
            /* RQR_limit */
            else if (strcmp(field, "to") == 0) {
               static int object_rule[] = { RQRL_name, RQRL_value, 0 };
               lList* limit_list = NULL;  
               
               field = sge_strtok_r(NULL, dlmt, &context);
               if (!cull_parse_definition_list(field, &limit_list, "", RQRL_Type, object_rule)) {
                  lSetList(rule, RQR_limit, limit_list);
               } else {
                  ERROR((SGE_EVENT, MSG_RESOURCE_QUOTA_ERRORPARSINGRULELIMIT));
                  answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
               }
               
            } else {
               ERROR((SGE_EVENT, MSG_RESOURCE_QUOTA_UNKNOWNCONFIGURATIONATTRIBUTE, field));
               answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            }
         }

         if (context)   
            sge_free_saved_vars(context);
         lAppendElem(lp_rules, rule);
      }
   }
   sge_free_saved_vars(last);
   last = NULL;

   FCLOSE(fp);

   DRETURN(lp);

FCLOSE_ERROR:
   ERROR((SGE_EVENT, MSG_FILE_NOCLOSE_SS, fname, strerror(errno)));
   answer_list_add(alpp, SGE_EVENT, STATUS_EDISK, ANSWER_QUALITY_ERROR);
   DRETURN(NULL);
}

/****** read_write_resource_quota/sge_create_rqrf_scope() *************
*  NAME
*     sge_create_rqrf_scope() -- create string of RQRF_Type
*
*  SYNOPSIS
*     static bool sge_create_rqrf_scope(dstring *buffer, const 
*     lListElem *ep) 
*
*  FUNCTION
*     This function creates a dstring of a RQRF_Type for spooling
*
*  INPUTS
*     dstring *buffer     - buffer for the string to be created
*     const lListElem *ep - Element to be converted
*
*  RESULT
*     static bool - true on success
*                   false on error
*
*  NOTES
*     MT-NOTE: sge_create_rqrf_scope() is MT safe 
*
*******************************************************************************/
static bool sge_create_rqrf_scope(dstring *buffer, const lListElem *ep) {
   sge_dstring_clear(buffer);
   return rqs_append_filter_to_dstring(ep, buffer, NULL);
}

/****** read_write_resource_quota/_write_rqs() **************************
*  NAME
*     _write_rqs() -- Write resource_quota set to dstring
*
*  SYNOPSIS
*     static int _write_rqs(int spool, const lListElem *ep, dstring 
*     *ruleset) 
*
*  FUNCTION
*     Internal function to convert a rule set element into a string for spooling
*
*  INPUTS
*     int spool           - 1 if elem is spooled 
*                           0 if elem is not spooled
*     const lListElem *ep - RQS Elem
*     dstring *ruleset    - buffer for created string
*
*  RESULT
*     static int - 1 on success
*                  0 on error
*
*  NOTES
*     MT-NOTE: _write_rqs() is not MT safe 
*
*  SEE ALSO
*     write_rqs()
*     write_rqs_list()
*******************************************************************************/
static int _write_rqs(int spool, const lListElem *ep, dstring *ruleset) {

   lListElem *rule = NULL;

   if (ep == NULL) {
      return 0;
   }

   sge_dstring_append(ruleset, "{\n");

   /* --------- RQS_name */
   sge_dstring_sprintf_append(ruleset,"   name         %s\n", lGetString(ep, RQS_name));

   /* --------- RQS_description*/
   if (lGetString(ep, RQS_description) != NULL) {
     sge_dstring_sprintf_append(ruleset, "   description  %s\n", lGetString(ep, RQS_description));
   }

   /* --------- RQS_enabled*/
   sge_dstring_sprintf_append(ruleset, "   enabled      %s\n",
                             lGetBool(ep, RQS_enabled) ? "TRUE" : "FALSE");

   /* --------- RQS_rule*/
   for_each(rule, lGetList(ep, RQS_rule)) {
      lList* tlp = NULL;
      lListElem *tep = NULL;
      dstring buffer = DSTRING_INIT;

      sge_dstring_append(ruleset, "   limit       ");

      /* --------- RQR_name*/
      if (lGetString(rule, RQR_name) != NULL) {
         sge_dstring_sprintf_append(ruleset, " name %s", lGetString(rule, RQR_name));
      }

      /* --------- RQR_filter_users*/
      if ((tep=lGetObject(rule, RQR_filter_users)) != NULL) {
         sge_create_rqrf_scope(&buffer, tep);
         sge_dstring_sprintf_append(ruleset, " users %s", sge_dstring_get_string(&buffer));
      }

      /* --------- RQR_filter_projects*/
      if ((tep=lGetObject(rule, RQR_filter_projects)) != NULL) {
         sge_create_rqrf_scope(&buffer, tep);
         sge_dstring_sprintf_append(ruleset, " projects %s", sge_dstring_get_string(&buffer));
      }

      /* --------- RQR_filter_pes*/
      if ((tep=lGetObject(rule, RQR_filter_pes)) != NULL) {
         sge_create_rqrf_scope(&buffer, tep);
         sge_dstring_sprintf_append(ruleset, " pes %s", sge_dstring_get_string(&buffer));
      } 
      
      /* --------- RQR_filter_queues*/
      if ((tep=lGetObject(rule, RQR_filter_queues)) != NULL) {
         sge_create_rqrf_scope(&buffer, tep);
         sge_dstring_sprintf_append(ruleset, " queues %s", sge_dstring_get_string(&buffer));
      } 

      /* --------- RQR_filter_hosts*/
      if ((tep=lGetObject(rule, RQR_filter_hosts)) != NULL) {
         sge_create_rqrf_scope(&buffer, tep);
         sge_dstring_sprintf_append(ruleset, " hosts %s", sge_dstring_get_string(&buffer));
      } 


      /* --------- RQR_limit*/
      tlp = lGetList(rule, RQR_limit);
      sge_dstring_append(ruleset, " to ");
      {
         bool first = true;
         for_each(tep, tlp) {
            if (!first) {
               sge_dstring_append(ruleset, ",");
            } else {
               first = false;
            }
            sge_dstring_sprintf_append(ruleset, "%s=%s", lGetString(tep, RQRL_name), lGetString(tep, RQRL_value));
         }
      }

      sge_dstring_append(ruleset, "\n");
      sge_dstring_free(&buffer);
   }
   
   sge_dstring_append(ruleset, "}\n");

   return 1;
}

/****** read_write_resource_quota/write_rqs() ***************************
*  NAME
*     write_rqs() -- spool resource quota set 
*
*  SYNOPSIS
*     char* write_rqs(int spool, int how, const lListElem *ep) 
*
*  FUNCTION
*     This function spools a RQS_Type elem.
*
*  INPUTS
*     int spool           - 1 for spooling
*                           0 for not spooling
*     int how             - 0 for stdout
*                           1 for tmp file
*                           2 for classic spooling file
*     const lListElem *ep - Elem to be spooled
*
*  RESULT
*     char* - filename on success
*             NULL on error
*
*  NOTES
*     MT-NOTE: write_rqs() is MT safe 
*
*  SEE ALSO
*     _write_rqs()
*     write_rqs_list()
*******************************************************************************/
char *write_rqs(int spool, int how, const lListElem *ep) {

   dstring dbuffer = DSTRING_INIT;
   FILE *fp;
   char filename[SGE_PATH_MAX], real_filename[SGE_PATH_MAX];

   DENTER(TOP_LAYER, "write_rqs");

   switch (how) {
   case 0:
      fp = stdout;
      break;
   case 1:
   case 2:
      if (how==1) {
         dstring tmp_name_error = DSTRING_INIT;
         if (sge_tmpnam(filename, &tmp_name_error) == NULL) {
            if (sge_dstring_get_string(&tmp_name_error) != NULL) {
               CRITICAL((SGE_EVENT, sge_dstring_get_string(&tmp_name_error)));
            } else {
               CRITICAL((SGE_EVENT, MSG_TMPNAM_GENERATINGTMPNAM));
            }
            sge_dstring_free(&tmp_name_error);
            DRETURN(NULL);
         }
         sge_dstring_free(&tmp_name_error);
      } else  {
         sprintf(filename, "%s/.%s", RESOURCEQUOTAS_DIR, 
            lGetString(ep, RQS_name));
         sprintf(real_filename, "%s/%s", RESOURCEQUOTAS_DIR, 
            lGetString(ep, RQS_name));
      }

      fp = fopen(filename, "w");
      if (!fp) {
         CRITICAL((SGE_EVENT, MSG_ERRORWRITINGFILE_SS, filename, strerror(errno)));
         DRETURN(NULL);
      }
      break;
   default:
      DRETURN(NULL);
   }

   _write_rqs(spool, ep, &dbuffer);
   FPRINTF((fp, "%s", sge_dstring_get_string(&dbuffer)));
   sge_dstring_free(&dbuffer);


   if (how!=0) {
      FCLOSE(fp);
   }
   if (how == 2) {
      if (rename(filename, real_filename) == -1) {
         DRETURN(NULL);
      } else {
         strcpy(filename, real_filename);
      }
   }          

   DRETURN((how==1?sge_strdup(NULL, filename):filename));
FPRINTF_ERROR:
FCLOSE_ERROR:
   ERROR((SGE_EVENT, MSG_FILE_NOCLOSE_SS, filename, strerror(errno)));
   sge_dstring_free(&dbuffer);
   DRETURN(NULL); 
}

/****** read_write_resource_quota/write_rqs_list() **************************
*  NAME
*     write_rqs_list() -- spool resource quota set list 
*
*  SYNOPSIS
*     char* write_rqs_list(int spool, int how, const lList *lp) 
*
*  FUNCTION
*     This function spools a RQS_Type List.
*
*  INPUTS
*     int spool       - 1 for spooling
*                       0 for not spooling
*     int how         - 0 for stdout
*                       1 for tmp file
*     const lList *lp - List to be spooled
*
*  RESULT
*     char* - filename on success
*             NULL on error
*
*  NOTES
*     MT-NOTE: write_rqs_list() is not MT safe 
*
*  SEE ALSO
*     _write_rqs()
*     write_rqs()
*******************************************************************************/
char *write_rqs_list(int spool, int how, const lList *lp) {

   lListElem *ep;
   dstring dbuffer = DSTRING_INIT;
   FILE *fp;
   char filename[SGE_PATH_MAX];
   /*
   char real_filename[SGE_PATH_MAX];
   */

   DENTER(TOP_LAYER, "write_rqs_list");

   switch (how) {
   case 0:
      fp = stdout;
      break;
   case 1:
   case 2:
      if (how==1) {
         dstring tmp_name_error = DSTRING_INIT;
         if (sge_tmpnam(filename, &tmp_name_error) == NULL) {
            if (sge_dstring_get_string(&tmp_name_error) != NULL) {
               CRITICAL((SGE_EVENT, sge_dstring_get_string(&tmp_name_error)));
            } else {
               CRITICAL((SGE_EVENT, MSG_TMPNAM_GENERATINGTMPNAM));
            }
            sge_dstring_free(&tmp_name_error);
            DRETURN(NULL);
         }
         sge_dstring_free(&tmp_name_error);
      } 
      /* 
      else  {
         sprintf(filename, "%s/.%s", RESOURCEQUOTAS_DIR, 
            lGetString(ep, RQS_name));
         sprintf(real_filename, "%s/%s", RESOURCEQUOTAS_DIR, 
            lGetString(ep, RQS_name));
      }
      */

      fp = fopen(filename, "w");
      if (!fp) {
         CRITICAL((SGE_EVENT, MSG_ERRORWRITINGFILE_SS, filename, strerror(errno)));
         DRETURN(NULL);
      }
      break;
   default:
      DRETURN(NULL);
   }

   for_each(ep, lp) {
      _write_rqs(spool, ep, &dbuffer);
   }

   FPRINTF((fp, "%s", sge_dstring_get_string(&dbuffer)));
   sge_dstring_free(&dbuffer);

   if (how!=0) {
      FCLOSE(fp);
   }
   /*
   if (how == 2) {
      if (rename(filename, real_filename) == -1) {
         DRETURN(NULL);
      } else {
         strcpy(filename, real_filename);
      }
   }          
   */

   DRETURN((how==1?sge_strdup(NULL, filename):filename));

FPRINTF_ERROR:
FCLOSE_ERROR:
   sge_dstring_free(&dbuffer);
   DRETURN(NULL); 
}
