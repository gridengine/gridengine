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
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "cull.h"
#include "sge_gdi_intern.h"
#include "sge_parse_num_par.h"
#include "sgermon.h"
#include "sge_log.h"
#include "config.h"
#include "sge_string.h"
#include "sge_answer.h"
#include "sge_conf.h"
#include "parse.h"

#include "msg_gdilib.h"

/* 
**
** DESCRIPTION
**    read_config_list() reads the file into a configuration list 
**    of type 'delimitor' putting the name of the configuraion
**    attribute into the 'nm1'-field and the value into 'nm2'.
**    'delimitor' is used to separate the name and value.
**    It may be NULL as it gets passed to sge_strtok().
**    If 'flag' has set the RCL_NO_VALUE bitmask then
**    also lines are accepted having only a name but no value.
**    
**
**
** RETURN
**    A NULL pointer as answer list signals success.
*/
int read_config_list(FILE *fp, lList **lpp, lList **alpp, lDescr *dp, int nm1,
                     int nm2, int nm3, char *delimitor, int flag, char *buffer,
                     int buffer_size) 
{
   lListElem *ep;
   char *name; 
   char *value;
   char *tmp;
   char *s;
   struct saved_vars_s *last = NULL;
   int force_value;
  
   DENTER(TOP_LAYER, "read_config_list");

   force_value = ((flag&RCL_NO_VALUE)==0);

   while (fgets(buffer, buffer_size, fp)) {
      if (last) {
         sge_free_saved_vars(last);
         last = NULL;
      }
      /*
      ** skip empty and comment lines
      */
      if (buffer[0] == '#')
         continue;
      for(s = buffer; *s == ' ' || *s == '\t'; s++)
         ;
      if (*s == '\n')
         continue;

      /*
      ** get name and value
      */
      if (*s != '"' || !(name = sge_strtok_r(s+1, "\"", &last))) {
         if (!(name = sge_strtok_r(buffer, delimitor, &last)))
            break;
      }
      value = sge_strtok_r(NULL, "\n", &last);

      /* handle end of sub-list */
      if (nm3 && strcmp(name, "}") == 0 && !value) {
         break;
      }

      if (!value && force_value)
         break;

      /* skip leading delimitors */
      if (value) {
         while (*value && (delimitor? 
            (NULL != strchr(delimitor, *value)):
            isspace((int) *value)))
            value++;
      }

      if(!value || !(*value)) {
         if (force_value) {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_CONFIGNOARGUMENTGIVEN_S , name));
            answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            DEXIT;
            goto Error;
         }
         value = NULL;
      } else {
         /* skip trailing delimitors */
         tmp = &value[strlen(value)-1];

         while (tmp>value && (delimitor?
            (NULL != strchr(delimitor, *tmp)):
            isspace((int) *tmp))) {
            *tmp = '\0';
            tmp--;
         }
      }

      /* handle sub-list */
      if (nm3 && value && strcmp(value, "{") == 0) {
         lList *slpp = NULL;
         if (read_config_list(fp, &slpp, alpp, dp, nm1, nm2, nm3, delimitor, flag, buffer, buffer_size)<0) {
            DEXIT;
            goto Error;
         }
         ep = lAddElemStr(lpp, nm1, name, dp);
         if (!ep) { 
            ERROR((SGE_EVENT, MSG_GDI_CONFIGADDLISTFAILED_S , name));
            answer_list_add(alpp, SGE_EVENT, STATUS_EDISK, ANSWER_QUALITY_ERROR);
            DEXIT;
            goto Error;
         } 
         lSetList(ep, nm3, slpp);
      } else {
         ep = lAddElemStr(lpp, nm1, name, dp);
         if (!ep) { 
            ERROR((SGE_EVENT, MSG_GDI_CONFIGADDLISTFAILED_S , name));
            answer_list_add(alpp, SGE_EVENT, STATUS_EDISK, ANSWER_QUALITY_ERROR);
            DEXIT;
            goto Error;
         } 

         lSetString(ep, nm2, value);
      }
   }
   
   if (last)
      sge_free_saved_vars(last);
   DEXIT;
   return 0; 

Error:
   if (last)
      sge_free_saved_vars(last);
   return -1;
}

/*
**
** DESCRIPTION
**    'get_conf_sublist' searches in the configurations list 
**    for a config value with 'key' as name. 'name_nm' is
**    used as field for the key and 'value_nm' is taken
**    for value.
** 
**    If an alpp gets passed then an answer element 
**    gets added in case of error.
**
** RETURN
**    Returns sub-list value or NULL if such a list
**    does not exist.
*/
lList *get_conf_sublist(lList **alpp, lList *lp, int name_nm, int value_nm,
                        const char *key) 
{
   lList *value;
   lListElem *ep;

   DENTER(CULL_LAYER, "get_conf_sublist");
   
   if (!(ep=lGetElemStr(lp, name_nm, key))) {
      if (alpp) {
         char error[1000];
         sprintf(error, MSG_GDI_CONFIGMISSINGARGUMENT_S , key);
         answer_list_add(alpp, error, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      }
      DEXIT;
      return NULL;
   }

   value = lGetList(ep, value_nm);
   /* DPRINTF(("%s = %s\n", key, value?value:"<null ptr>")); */

   DEXIT;
   return value;
}

/*
**
** DESCRIPTION
**    'get_conf_value' searches in the configurations list 
**    for a config value with 'key' as name. 'name_nm' is
**    used as field for the key and 'value_nm' is taken
**    for value.
** 
**    If an alpp gets passed then an answer element 
**    gets added in case of error.
**
** RETURN
**    Returns string value or NULL if such a value
**    does not exist.
*/
char *get_conf_value(lList **alpp, lList *lp, int name_nm, int value_nm,
                           const char *key) {
   char *value;
   lListElem *ep;

   DENTER(CULL_LAYER, "get_conf_value");
   
   if (!(ep=lGetElemStr(lp, name_nm, key))) {
      if (alpp) {
         char error[1000];
         sprintf(error, MSG_GDI_CONFIGMISSINGARGUMENT_S , key);
         answer_list_add(alpp, error, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      }
      DEXIT;
      return NULL;
   }

   /* FIX_CONST */
   value = (char*) lGetString(ep, value_nm);
   DPRINTF(("%s = %s\n", key, value?value:"<null ptr>"));

   DEXIT;
   return value;
}

/****
 **** set_conf_string
 ****
 **** 'set_conf_string' searches in the configuration list
 **** (pointed to by clpp) for a string-config value with 'key'
 **** as name.
 **** If the value is found, it is stored in the lListElem
 **** 'ep' in the fied specified by 'name_nm'.
 **** If the config-string is not found, an error message
 **** is created.
 ****
 **** The function returns FALSE on error, otherwise TRUE.
 ****/
int set_conf_string(
lList **alpp,
lList **clpp,
int fields[],
const char *key,
lListElem *ep,
int name_nm 
) {
#ifdef __INSIGHT__
/* JG: NULL is OK for fields */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   const char *str;
   int pos;
   int dataType;

   DENTER(CULL_LAYER, "set_conf_string");

   if(!(str=get_conf_value(fields?NULL:alpp, *clpp, CF_name, CF_value, key))) {
      DEXIT;
      return fields?TRUE:FALSE;
   }

   pos = lGetPosViaElem(ep, name_nm);
   dataType = lGetPosType(lGetElemDescr(ep),pos);
   switch (dataType) {
      case lStringT:
         DPRINTF(("set_conf_string: lStringT data type (Type: %s)\n",lNm2Str(name_nm)));
         lSetString(ep, name_nm, str);
         break;
      case lHostT:
         DPRINTF(("set_conf_string: lHostT data type (Type: %s)\n",lNm2Str(name_nm)));
         lSetHost(ep, name_nm, str);
         break;
      default:
         DPRINTF(("!!!!!!!!!set_conf_string: unexpected data type !!!!!!!!!!!!!!!!!\n"));
         break;
   } 
   lDelElemStr(clpp, CF_name, key);
   add_nm_to_set(fields, name_nm);

   DEXIT;
   return TRUE;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}

/****
 **** set_conf_bool
 ****
 **** 'set_conf_bool' searches in the configuration list
 **** (pointed to by clpp) for a string-config value with 'key'
 **** as name.
 **** If the value is found, it is stored in the lListElem
 **** 'ep' in the field specified by 'name_nm'.
 **** the strings true and false are stored as the constants TRUE/FALSE
 **** in a u_long32 field
 **** If the config-string is not found, an error message
 **** is created.
 ****
 **** The function returns FALSE on error, otherwise TRUE.
 ****/
int set_conf_bool(
lList **alpp,
lList **clpp,
int fields[],
const char *key,
lListElem *ep,
int name_nm 
) {
#ifdef __INSIGHT__
/* JG: NULL is OK for fields */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   const char *str;

   DENTER(CULL_LAYER, "set_conf_boolean");

   if(!(str=get_conf_value(fields?NULL:alpp, *clpp, CF_name, CF_value, key))) {
      DEXIT;
      return fields?TRUE:FALSE;
   }
   if (!strcasecmp(str, "true"))
      lSetBool(ep, name_nm, TRUE);
   else
      lSetBool(ep, name_nm, FALSE);
      
   lDelElemStr(clpp, CF_name, key);
   add_nm_to_set(fields, name_nm);

   DEXIT;
   return TRUE;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}


/****
 **** set_conf_ulong
 ****
 **** 'set_conf_ulong' searches in the configuration list
 **** (pointed to by clpp) for a ulong-config value with 'key'
 **** as name.
 **** If the value is found, it is stored in the lListElem
 **** 'ep' in the fied specified by 'name_nm'.
 **** If the config-value is not found, or it is not an
 **** integer, an error message is created.
 ****
 **** The function returns FALSE on error, otherwise TRUE.
 ****/
int set_conf_ulong(
lList **alpp,
lList **clpp,
int fields[],
const char *key,
lListElem *ep,
int name_nm 
) {
#ifdef __INSIGHT__
/* JG: NULL is OK for fields */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   const char *str;
   u_long32 uval;

   DENTER(CULL_LAYER, "set_conf_ulong");

   if(!(str=get_conf_value(fields?NULL:alpp, *clpp, CF_name, CF_value, key))) {
      DEXIT;
      return fields?TRUE:FALSE;
   }
   if (sscanf(str, u32, &uval)!=1) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_CONFIGARGUMENTNOTINTEGER_SS ,
               key, str)); 
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR); 
      DEXIT;
      return FALSE;
   }
   lSetUlong(ep, name_nm, uval);
   lDelElemStr(clpp, CF_name, key);
   add_nm_to_set(fields, name_nm);

   DEXIT;
   return TRUE;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}

/****
 **** set_conf_double
 ****
 **** 'set_conf_double' searches in the configuration list
 **** (pointed to by clpp) for a double-config value with 'key'
 **** as name.
 **** If the value is found, it is stored in the lListElem
 **** 'ep' in the field specified by 'name_nm'.
 **** If operation_nm != 0, the double value can be preceded by
 **** +,-,= or nothing. This sets a flag specified by operation_nm.
 **** If the config-value is not found, or it is not a
 **** double, an error message is created.
 ****
 **** The function returns FALSE on error, otherwise TRUE.
 ****/
int set_conf_double(
lList **alpp,
lList **clpp,
int fields[],
const char *key,
lListElem *ep,
int name_nm,
int operation_nm 
) {
#ifdef __INSIGHT__
/* JG: NULL is OK for fields */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   const char *str;
   double dval;

   DENTER(CULL_LAYER, "set_conf_double"); 

   if(!(str=get_conf_value(fields?NULL:alpp, *clpp, CF_name, CF_value, key))) {
      DEXIT;
      return fields?TRUE:FALSE;
   }
   /*
   ** only treated if operation_nm != 0
   */
   if (operation_nm) {
      while (isspace((int) *str))
         str++;
      switch (*str) {
         case '+':   
            lSetUlong(ep, operation_nm, MODE_ADD);
            str++;
            break;
         case '-':
            lSetUlong(ep, operation_nm, MODE_SUB);
            str++;
            break;
         case '=':
            lSetUlong(ep, operation_nm, MODE_SET);
            str++;
            break;
         default:
            lSetUlong(ep, operation_nm, MODE_RELATIVE);
      }
   }

   if ( (sscanf(str, "%lf", &dval)!=1) || ( strncasecmp(str,"inf",3) == 0 ) ) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_CONFIGARGUMENTNOTDOUBLE_SS , 
               key, str));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      DEXIT;
      return FALSE;
   }

   lSetDouble(ep, name_nm, dval);
   lDelElemStr(clpp, CF_name, key);
   add_nm_to_set(fields, name_nm);

   DEXIT;
   return TRUE;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}

/****
 **** set_conf_deflist
 ****
 **** 'set_conf_deflist' searches in the configuration list
 **** (pointed to by clpp) for a definition-list-config 
 **** value with 'key' as name.
 **** If the value is found, it is stored in the lListElem
 **** 'ep' in the fied specified by 'name_nm'.
 **** The definition-list is tokenized with 
 **** cull_parse_definition_list().
 **** The sub-list is created of type 'descr' and 
 **** interpreted by 'interpretation_rule'.
 ****
 **** The function returns FALSE on error, otherwise TRUE.
 ****/
int set_conf_deflist(
lList **alpp,
lList **clpp,
int fields[],
const char *key,
lListElem *ep,
int name_nm,
lDescr *descr,
intprt_type *interpretation_rule 

) {
#ifdef __INSIGHT__
/* JG: NULL is OK for fields */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   lList *tmplp = NULL;
   char *str;

   DENTER(CULL_LAYER, "set_conf_deflist");


   if(!(str=get_conf_value(fields?NULL:alpp, *clpp, CF_name, CF_value, key))) {
      DEXIT;
      return fields?TRUE:FALSE;
   }

   {
      int ret;

      if ((ret = cull_parse_definition_list(str, &tmplp, key, descr, 
            interpretation_rule))) {
         DEXIT;
         return FALSE;
      }
   }

   lSetList(ep, name_nm, tmplp);
   lDelElemStr(clpp, CF_name, key);
   add_nm_to_set(fields, name_nm);

   DEXIT;
   return TRUE;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}

/****
 **** set_conf_timestring
 ****
 **** 'set_conf_timestring' searches in the configuration list
 **** (pointed to by clpp) for a timestring-config value 
 **** with 'key' as name.
 **** If the value is found, it is stored in the lListElem
 **** 'ep' in the fied specified by 'name_nm'.
 **** If the config-string is not found, or it is not a
 **** valid time-string, an error message is created.
 ****
 **** The function returns FALSE on error, otherwise TRUE.
 ****/
int set_conf_timestr(
lList **alpp,
lList **clpp,
int fields[],
const char *key,
lListElem *ep,
int name_nm 
) {
#ifdef __INSIGHT__
/* JG: NULL is OK for fields */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   const char *str;

   DENTER(CULL_LAYER, "set_conf_timestring");

   if (key == NULL) {
      DEXIT;
      return FALSE;
   }

   if(!(str=get_conf_value(fields?NULL:alpp, *clpp, CF_name, CF_value, key))) {
      DEXIT;
      return fields?TRUE:FALSE;
   }
   if(!parse_ulong_val(NULL, NULL, TYPE_TIM, str, NULL, 0)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_CONFIGARGUMENTNOTTIME_SS , key, str));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      DEXIT;
      return FALSE;
   }

   lSetString(ep, name_nm, str);
   lDelElemStr(clpp, CF_name, key);
   add_nm_to_set(fields, name_nm);

   DEXIT;
   return TRUE;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}

/****
 **** set_conf_memstr
 ****
 **** 'set_conf_memstr' searches in the configuration list
 **** (pointed to by clpp) for a memstring-config value 
 **** with 'key' as name.
 **** If the value is found, it is stored in the lListElem
 **** 'ep' in the fied specified by 'name_nm'.
 **** If the config-string is not found, or it is not a
 **** valid mem-string, an error message is created.
 ****
 **** The function returns FALSE on error, otherwise TRUE.
 ****/
int set_conf_memstr(
lList **alpp,
lList **clpp,
int fields[],
const char *key,
lListElem *ep,
int name_nm 
) {
#ifdef __INSIGHT__
/* JG: NULL is OK for fields */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   const char *str;

   DENTER(CULL_LAYER, "set_conf_memstr");

   if(!(str=get_conf_value(fields?NULL:alpp, *clpp, CF_name, CF_value, key))) {
      DEXIT;
      return fields?TRUE:FALSE;
   }
   if(!parse_ulong_val(NULL, NULL, TYPE_MEM, str, NULL, 0)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_CONFIGARGUMENTNOMEMORY_SS , key, str));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      DEXIT;
      return FALSE;
   }

   lSetString(ep, name_nm, str);
   lDelElemStr(clpp, CF_name, key);
   add_nm_to_set(fields, name_nm);

   DEXIT;
   return TRUE;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}

/****
 **** set_conf_enum
 ****
 **** 'set_conf_enum' searches in the configuration list
 **** (pointed to by clpp) for a enumstring-config value 
 **** with 'key' as name.
 **** If the value is found, it is stored in the lListElem
 **** 'ep' in the field specified by 'name_nm'.
 **** If the config-string is not found, or it is not a
 **** valid enum-string, an error message is created.
 ****
 **** The function returns FALSE on error, otherwise TRUE.
 ****/
int set_conf_enum(lList **alpp, lList **clpp, int fields[], const char *key,
                  lListElem *ep, int name_nm, const char **enum_strings) 
{
#ifdef __INSIGHT__
/* JG: NULL is OK for fields */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   const char *str;
   u_long32 uval = 0;

   DENTER(CULL_LAYER, "set_conf_enum");

   if(!(str=get_conf_value(fields?NULL:alpp, *clpp, CF_name, CF_value, key))) {
      DEXIT;
      return fields?TRUE:FALSE;
   }
   if(!sge_parse_bitfield_str(str, enum_strings, &uval, key, alpp)) {
      DEXIT;
      return FALSE;
   }
   
   if(!uval) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_CONFIGINVALIDQUEUESPECIFIED ));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      DEXIT;
      return -1;
   }

   lSetUlong(ep, name_nm, uval);
   lDelElemStr(clpp, CF_name, key);
   add_nm_to_set(fields, name_nm);


   DEXIT;
   return TRUE;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}

/****
 **** set_conf_list
 ****
 **** 'set_conf_list' searches in the configuration list
 **** (pointed to by clpp) for a list-config value with 
 **** 'key' as name.
 **** If the value is found, it is stored in the lListElem
 **** 'ep' in the fied specified by 'name_nm'.
 **** The definition-list is tokenized with 
 **** lString2List().
 **** The sub-list is created of type 'descr' and 
 **** the strings are stored in the field 'sub_name_nm'.
 ****
 **** The function returns FALSE on error, otherwise TRUE.
 ****/
int set_conf_list(lList **alpp, lList **clpp, int fields[], const char *key, 
                  lListElem *ep, int name_nm, lDescr *descr, int sub_name_nm) 
{
#ifdef __INSIGHT__
/* JG: NULL is OK for fields */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   lList *tmplp = NULL;
   const char *str;
   char delims[] = "\t \v\r,"; 

   DENTER(TOP_LAYER, "set_conf_list");

   if(!(str=get_conf_value(fields?NULL:alpp, *clpp, CF_name, CF_value, key))) {
      DEXIT;
      return fields?TRUE:FALSE;
   }
   lString2List(str, &tmplp, descr, sub_name_nm, delims); 

   lDelElemStr(clpp, CF_name, key);
   add_nm_to_set(fields, name_nm);

   if (tmplp && strcasecmp("NONE", lGetString(lFirst(tmplp), sub_name_nm))) {
      lSetList(ep, name_nm, tmplp);
      DEXIT;
      return TRUE;
   }
   lFreeList(tmplp);

   DEXIT;
   return TRUE;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}

/****
 **** set_conf_subordlist
 ****
 **** 'set_conf_subordlist' searches in the configuration list
 **** (pointed to by clpp) for a subordinate-list-config 
 **** value with 'key' as name.
 **** The subordinate-list looks like:
 **** name1=val1,name2,name3,name4=val2
 **** The divider between two entries are ',', space or tab,
 **** between name and value are '=' or ':'.
 **** If the list is found, it is stored in the lListElem
 **** 'ep' in the fied specified by 'name_nm'.
 **** The sub-list is created of type 'descr', the name of
 **** the subord-list-field is stored in the field
 **** 'subname_nm', it's value in 'subval_nm'.
 **** If the subordinate list is 'NONE', no list will
 **** be created (null-pointer!).
 ****
 **** The function returns FALSE on error, otherwise TRUE.
 ****/
int set_conf_subordlist(
lList **alpp,
lList **clpp,
int fields[],
const char *key,
lListElem *ep,
int name_nm,
lDescr *descr,
int subname_nm,
int subval_nm 
) {
#ifdef __INSIGHT__
/* JG: NULL is OK for fields */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   lList *tmplp = NULL;
   lListElem *tmpep;
   const char *str;
   const char *s;
   char *endptr;

   DENTER(CULL_LAYER, "set_conf_subordlist");

   if(!(str=get_conf_value(fields?NULL:alpp, *clpp, CF_name, CF_value, key))) {
      DEXIT;
      return fields?TRUE:FALSE;
   }
   lString2List(str, &tmplp, descr, subname_nm, ", \t");
   for_each (tmpep, tmplp) {
      s = sge_strtok(lGetString(tmpep, subname_nm), ":=");
      lSetString(tmpep, subname_nm, s);
      if (!(s=sge_strtok(NULL, ":=")))
         continue;
      lSetUlong(tmpep, subval_nm, strtol(s, &endptr, 10));
      if (*endptr) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_CONFIGREADFILEERRORNEAR_SS , key, endptr));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DEXIT;
         return FALSE;
      }
   }
   
   if(!strcasecmp("NONE", lGetString(lFirst(tmplp), subname_nm)))
      tmplp = lFreeList(tmplp);

   lSetList(ep, name_nm, tmplp);
   lDelElemStr(clpp, CF_name, key);
   add_nm_to_set(fields, name_nm);

   DEXIT;
   return TRUE;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}


/* 
   Append 'name_nm' into the int array 'fields'.
   In case the 'name_nm' is already contained
   in 'fields' -1 is returned. 
*/

int add_nm_to_set(
int fields[],
int name_nm 
) {
#ifdef __INSIGHT__
/* JG: NULL is OK for fields */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   int i = 0;

   DENTER(CULL_LAYER, "add_nm_to_set");

   if (!fields) {
      DEXIT;
      return 0; /* we leave here in most cases */
   }

   /* seek end and check whether 'name_nm' 
      is already in 'fields' */
   while (fields[i]!=NoName && fields[i]!=name_nm)
      i++;
   
   if (fields[i]==name_nm) {
      DEXIT;
      return -1;
   }

   fields[i] = name_nm;      
   fields[++i] = NoName;      

   DEXIT;
   return 0;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}

