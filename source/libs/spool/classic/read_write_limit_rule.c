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
#include "sgeobj/sge_limit_rule.h"
#include "sge_answer.h"
#include "spool/classic/read_write_limit_rule.h"
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

/****** read_write_limit_rule/sge_read_LIRF_obj() ******************************
*  NAME
*     sge_read_LIRF_obj() -- parse elem from string
*
*  SYNOPSIS
*     static int sge_read_LIRF_obj(lListElem *ep, int nm, const char *buffer, 
*     lList **alpp) 
*
*  FUNCTION
*     Converts a string buffer to LIRF Type Element
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
*     MT-NOTE: sge_read_LIRF_obj() is MT safe 
*
*******************************************************************************/
static int sge_read_LIRF_obj(lListElem *ep, int nm, const char *buffer,
                             lList **alpp) 
{
   lListElem *filter = NULL;
   int ret = 0;

   DENTER(TOP_LAYER, "sge_read_LIRF_obj");

   if (LIRF_object_parse_from_string(&filter, buffer, alpp)) { 
      ret = 1;
      lSetObject(ep, nm, filter);
   }

   DRETURN(ret);
}

/****** read_write_limit_rule/cull_read_in_limit_rule_sets() *******************
*  NAME
*     cull_read_in_limit_rule_sets() -- converts a file to a limit rule set list
*
*  SYNOPSIS
*     lList* cull_read_in_limit_rule_sets(const char *fname, lList **alpp) 
*
*  FUNCTION
*     Reads in spooled limitation rule sets
*
*  INPUTS
*     const char *fname - file to read
*     lList **alpp      - answer list
*
*  RESULT
*     lList* - List of LIRS_Type
*
*  NOTES
*     MT-NOTE: cull_read_in_limit_rule_sets() is MT safe 
*
*******************************************************************************/
lList *cull_read_in_limit_rule_sets(const char *fname, lList **alpp) 
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
   
   DENTER(TOP_LAYER, "cull_read_in_limit_rule_sets");

   lp = lCreateList("limit_rule_sets", LIRS_Type);

   if (!(fp = fopen(fname, "r"))) {
      ERROR((SGE_EVENT, MSG_FILE_NOOPEN_SS, fname, strerror(errno)));
      if (alpp) {
         answer_list_add(alpp, SGE_EVENT, STATUS_EDISK, ANSWER_QUALITY_ERROR);
         lFreeList(&lp);
         DEXIT;
         return NULL;
      }
      else
         SGE_EXIT(1);
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

      /* handle begin of limitation rule set */
      if (strcmp(name, "{") == 0 && !value) {
         if (ep != NULL) {
            ERROR((SGE_EVENT, MSG_LIMITRULE_RULESETNOTFINISHED));
            answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            break;
         } else {
            ep = lCreateElem(LIRS_Type); 
            lp_rules = lCreateList("limit_rules", LIR_Type);
            continue;
         }
      }

      /* handle end of limitation rule set */
      else if (strcmp(name, "}") == 0 && !value) {
         if (ep == NULL) {
            ERROR((SGE_EVENT, MSG_LIMITRULE_RULESETNOTSTARTED));
            answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            break;
         } else {
            lSetList(ep, LIRS_rule, lp_rules);
            lAppendElem(lp, ep);
            ep = NULL;
            continue;
         }
      }

      /* LIRS_name */
      else if (strcmp(name, "name") == 0) {
         lSetString(ep, LIRS_name, value); 
      }
      
      /* LIRS_description */
      else if (strcmp(name, "description") == 0) {
         lSetString(ep, LIRS_description, value); 
      }

      /* LIRS_enabled */
      else if (strcmp(name, "enabled") == 0) {
         object_parse_bool_from_string(ep, alpp, LIRS_enabled, value);
      }
      /* LIRS_limit */
      else if (strcmp(name, "limit") == 0) {
         lList *tmp_list = NULL;
         lListElem *tmp_ep = NULL;
         lListElem *rule = lCreateElem(LIR_Type);

         lString2List(value, &tmp_list, ST_Type, ST_name, "\t \v\r");
         for_each(tmp_ep, tmp_list) {
            const char *field = lGetString(tmp_ep, ST_name);

            /* LIR_name */
            if (strcmp(field, "name") == 0) {
               tmp_ep = lNext(tmp_ep);
               lSetString(rule, LIR_name, lGetString(tmp_ep, ST_name));
            }
            /* LIR_filter_users */
            else if (strcmp(field, "users") == 0) {
               tmp_ep = lNext(tmp_ep);
               sge_read_LIRF_obj(rule, LIR_filter_users, lGetString(tmp_ep, ST_name), alpp); 
            } 
            /* LIR_filter_projects */
            else if (strcmp(field, "projects") == 0) {
               tmp_ep = lNext(tmp_ep);
               sge_read_LIRF_obj(rule, LIR_filter_projects, lGetString(tmp_ep, ST_name), alpp); 
            }
            /* LIR_filter_pes */
            else if (strcmp(field, "pes") == 0) {
               tmp_ep = lNext(tmp_ep);
               sge_read_LIRF_obj(rule, LIR_filter_pes, lGetString(tmp_ep, ST_name), alpp); 
            }
            /* LIR_filter_queues */
            else if (strcmp(field, "queues") == 0) {
               tmp_ep = lNext(tmp_ep);
               sge_read_LIRF_obj(rule, LIR_filter_queues, lGetString(tmp_ep, ST_name), alpp); 
            }
            /* LIR_filter_hosts */
            else if (strcmp(field, "hosts") == 0) {
               tmp_ep = lNext(tmp_ep);
               sge_read_LIRF_obj(rule, LIR_filter_hosts, lGetString(tmp_ep, ST_name), alpp); 
            }
            /* LIR_limit */
            else if (strcmp(field, "to") == 0) {
               static int object_rule[] = { LIRL_name, LIRL_value, 0 };
               lList* limit_list = NULL;  
               
               tmp_ep = lNext(tmp_ep);
               if (!cull_parse_definition_list( (char*)lGetString(tmp_ep, ST_name), 
                    &limit_list, "", LIRL_Type, object_rule)) {
                  lSetList(rule, LIR_limit, limit_list);
               } else {
                  ERROR((SGE_EVENT, MSG_LIMITRULE_ERRORPARSINGRULELIMIT));
                  answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
               }
               
            } else {
               ERROR((SGE_EVENT, MSG_LIMITRULE_UNKNOWNCONFIGURATIONATTRIBUTE, field));
               answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            }
         }

         lAppendElem(lp_rules, rule);
         lFreeList(&tmp_list);
      }
   }
   sge_free_saved_vars(last);
   last = NULL;

   FCLOSE(fp);

   DEXIT;
   return lp;
FCLOSE_ERROR:
   ERROR((SGE_EVENT, MSG_FILE_NOCLOSE_SS, fname, strerror(errno)));
   answer_list_add(alpp, SGE_EVENT, STATUS_EDISK, ANSWER_QUALITY_ERROR);
   DEXIT;
   return NULL;
}

/****** read_write_limit_rule/sge_create_limit_rule_filter_scope() *************
*  NAME
*     sge_create_limit_rule_filter_scope() -- create string of LIRF_Type
*
*  SYNOPSIS
*     static bool sge_create_limit_rule_filter_scope(dstring *buffer, const 
*     lListElem *ep) 
*
*  FUNCTION
*     This function creates a dstring of a LIRF_Type for spooling
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
*     MT-NOTE: sge_create_limit_rule_filter_scope() is MT safe 
*
*******************************************************************************/
static bool sge_create_limit_rule_filter_scope(dstring *buffer, const lListElem *ep) {
   sge_dstring_clear(buffer);
   return LIRF_object_append_to_dstring(ep, buffer, NULL);
}

/****** read_write_limit_rule/_write_limit_rule_set() **************************
*  NAME
*     _write_limit_rule_set() -- Write limit rule set to dstring
*
*  SYNOPSIS
*     static int _write_limit_rule_set(int spool, const lListElem *ep, dstring 
*     *ruleset) 
*
*  FUNCTION
*     Internal function to convert a rule set element into a string for spooling
*
*  INPUTS
*     int spool           - 1 if elem is spooled 
*                           0 if elem is not spooled
*     const lListElem *ep - LIRS Elem
*     dstring *ruleset    - buffer for created string
*
*  RESULT
*     static int - 1 on success
*                  0 on error
*
*  NOTES
*     MT-NOTE: _write_limit_rule_set() is not MT safe 
*
*  SEE ALSO
*     write_limit_rule_set()
*     write_limit_rule_sets()
*******************************************************************************/
static int _write_limit_rule_set(int spool, const lListElem *ep, dstring *ruleset) {

   lListElem *rule = NULL;

   if (ep == NULL) {
      return 0;
   }

   sge_dstring_append(ruleset, "{\n");

   /* --------- LIRS_name */
   sge_dstring_sprintf_append(ruleset,"   name         %s\n", lGetString(ep, LIRS_name));

   /* --------- LIRS_description*/
   if (lGetString(ep, LIRS_description) != NULL) {
     sge_dstring_sprintf_append(ruleset, "   description  %s\n", lGetString(ep, LIRS_description));
   }

   /* --------- LIRS_enabled*/
   sge_dstring_sprintf_append(ruleset, "   enabled      %s\n",
                             lGetBool(ep, LIRS_enabled) ? "TRUE" : "FALSE");

   /* --------- LIRS_rule*/
   for_each(rule, lGetList(ep, LIRS_rule)) {
      lList* tlp = NULL;
      lListElem *tep = NULL;
      dstring buffer = DSTRING_INIT;

      sge_dstring_append(ruleset, "   limit       ");

      /* --------- LIR_name*/
      if (lGetString(rule, LIR_name) != NULL) {
         sge_dstring_sprintf_append(ruleset, " name %s", lGetString(rule, LIR_name));
      }

      /* --------- LIR_filter_users*/
      if ((tep=lGetObject(rule, LIR_filter_users)) != NULL) {
         sge_create_limit_rule_filter_scope(&buffer, tep);
         sge_dstring_sprintf_append(ruleset, " users %s", sge_dstring_get_string(&buffer));
      }

      /* --------- LIR_filter_projects*/
      if ((tep=lGetObject(rule, LIR_filter_projects)) != NULL) {
         sge_create_limit_rule_filter_scope(&buffer, tep);
         sge_dstring_sprintf_append(ruleset, " projects %s", sge_dstring_get_string(&buffer));
      }

      /* --------- LIR_filter_pes*/
      if ((tep=lGetObject(rule, LIR_filter_pes)) != NULL) {
         sge_create_limit_rule_filter_scope(&buffer, tep);
         sge_dstring_sprintf_append(ruleset, " pes %s", sge_dstring_get_string(&buffer));
      } 
      
      /* --------- LIR_filter_queues*/
      if ((tep=lGetObject(rule, LIR_filter_queues)) != NULL) {
         sge_create_limit_rule_filter_scope(&buffer, tep);
         sge_dstring_sprintf_append(ruleset, " queues %s", sge_dstring_get_string(&buffer));
      } 

      /* --------- LIR_filter_hosts*/
      if ((tep=lGetObject(rule, LIR_filter_hosts)) != NULL) {
         sge_create_limit_rule_filter_scope(&buffer, tep);
         sge_dstring_sprintf_append(ruleset, " hosts %s", sge_dstring_get_string(&buffer));
      } 


      /* --------- LIR_limit*/
      tlp = lGetList(rule, LIR_limit);
      sge_dstring_append(ruleset, " to ");
      {
         bool first = true;
         for_each(tep, tlp) {
            if (!first) {
               sge_dstring_append(ruleset, ",");
            } else {
               first = false;
            }
            sge_dstring_sprintf_append(ruleset, "%s=%s", lGetString(tep, LIRL_name), lGetString(tep, LIRL_value));
         }
      }

      sge_dstring_append(ruleset, "\n");
      sge_dstring_free(&buffer);
   }
   
   sge_dstring_append(ruleset, "}\n");

   return 1;
}

/****** read_write_limit_rule/write_limit_rule_set() ***************************
*  NAME
*     write_limit_rule_set() -- spool limitation rule 
*
*  SYNOPSIS
*     char* write_limit_rule_set(int spool, int how, const lListElem *ep) 
*
*  FUNCTION
*     This function spools a LIRS_Type elem.
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
*     MT-NOTE: write_limit_rule_set() is MT safe 
*
*  SEE ALSO
*     _write_limit_rule_set()
*     write_limit_rule_sets()
*******************************************************************************/
char *write_limit_rule_set(int spool, int how, const lListElem *ep) {

   dstring dbuffer = DSTRING_INIT;
   FILE *fp;
   char filename[SGE_PATH_MAX], real_filename[SGE_PATH_MAX];

   DENTER(TOP_LAYER, "write_limit_rule");

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
         sprintf(filename, "%s/.%s", LIMITRULESETS, 
            lGetString(ep, LIRS_name));
         sprintf(real_filename, "%s/%s", LIMITRULESETS, 
            lGetString(ep, LIRS_name));
      }

      fp = fopen(filename, "w");
      if (!fp) {
         CRITICAL((SGE_EVENT, MSG_ERRORWRITINGFILE_SS, filename, strerror(errno)));
         DEXIT;
         return NULL;
      }
      break;
   default:
      DEXIT;
      return NULL;
   }

   _write_limit_rule_set(spool, ep, &dbuffer);
   FPRINTF((fp, "%s", sge_dstring_get_string(&dbuffer)));
   sge_dstring_free(&dbuffer);


   if (how!=0) {
      FCLOSE(fp);
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
FCLOSE_ERROR:
   sge_dstring_free(&dbuffer);
   DEXIT;
   return NULL; 
}

/****** read_write_limit_rule/write_limit_rule_sets() **************************
*  NAME
*     write_limit_rule_sets() -- spool limitation rules
*
*  SYNOPSIS
*     char* write_limit_rule_sets(int spool, int how, const lList *lp) 
*
*  FUNCTION
*     This function spools a LIRS_Type List.
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
*     MT-NOTE: write_limit_rule_sets() is not MT safe 
*
*  SEE ALSO
*     _write_limit_rule_set()
*     write_limit_rule_set()
*******************************************************************************/
char *write_limit_rule_sets(int spool, int how, const lList *lp) {

   lListElem *ep;
   dstring dbuffer = DSTRING_INIT;
   FILE *fp;
   char filename[SGE_PATH_MAX];
   /*
   char real_filename[SGE_PATH_MAX];
   */

   DENTER(TOP_LAYER, "write_limit_rule");

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
      } 
      /* 
      else  {
         sprintf(filename, "%s/.%s", LIMITRULESETS, 
            lGetString(ep, LIRS_name));
         sprintf(real_filename, "%s/%s", LIMITRULESETS, 
            lGetString(ep, LIRS_name));
      }
      */

      fp = fopen(filename, "w");
      if (!fp) {
         CRITICAL((SGE_EVENT, MSG_ERRORWRITINGFILE_SS, filename, strerror(errno)));
         DEXIT;
         return NULL;
      }
      break;
   default:
      DEXIT;
      return NULL;
   }

   for_each(ep, lp) {
      _write_limit_rule_set(spool, ep, &dbuffer);
   }

   FPRINTF((fp, "%s", sge_dstring_get_string(&dbuffer)));
   sge_dstring_free(&dbuffer);

   if (how!=0) {
      FCLOSE(fp);
   }
   /*
   if (how == 2) {
      if (rename(filename, real_filename) == -1) {
         DEXIT;
         return NULL;
      } else {
         strcpy(filename, real_filename);
      }
   }          
   */

   DEXIT;
   return how==1?sge_strdup(NULL, filename):filename;
FPRINTF_ERROR:
FCLOSE_ERROR:
   sge_dstring_free(&dbuffer);
   DEXIT;
   return NULL; 
}
