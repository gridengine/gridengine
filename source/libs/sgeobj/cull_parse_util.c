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
#include <stdlib.h>

#include "cull.h"
#include "cull_parse_util.h"
#include "sgermon.h"
#include "sge_string.h"
#include "sge_centry.h"
#include "parse_qsubL.h"
#include "sge_stdio.h"

/*
** NAME
**   cull_parse_string_list
** PARAMETER
**   pstrlist             -  pointer to string list to be interpreted
**   listname             -  listname to be given to the cull output list
**   descr                -  cull list descriptor, i.e. list type
**   interpretation_rule  -  array of integers terminated by 0, each int
**                           represents one list field to be set, when thru,
**                           a new list element is created and interpretation
**                           starts anew with first entry
**   pplist               -  list pointer-pointer to be set to output cull list
** RETURN
**
** EXTERNAL
**
** DESCRIPTION
**   parses a list of strings
**   this list might have been created by string_list
*/
/*
** problem: if NULL ends string list, then NULL cannot be used
** for fields that should not be set
*/
int cull_parse_string_list(
char **pstrlist,
const char *listname,
lDescr *descr,
intprt_type *interpretation_rule,
lList **pplist 
) {
   lList *list;
   lListElem *ep;
   intprt_type *rule;
   int type;

   DENTER(BASIS_LAYER, "cull_parse_string_list");
   if (!pstrlist || !descr || !interpretation_rule || !pplist) {
      DPRINTF(("cull_parse_string_list: NULL pointer received\n"));
      DEXIT;
      return -1;
   }
   if (*interpretation_rule == 0) {
      DPRINTF(("cull_parse_string_list: zero interpretation rule\n"));
      DEXIT;
      return -2;
   }
   list = lCreateList(listname, descr);
   if (!list) {
      DPRINTF(("cull_parse_string_list: cannot create list\n"));
      DEXIT;
      return -3;      
   }
   ep = lCreateElem(descr);
   if (!ep) {
      DPRINTF(("cull_parse_string_list: cannot create element\n"));
      lFreeList(list);
      DEXIT;
      return -4;      
   }
   lAppendElem(list, ep);
   /*
   ** process the string list till it ends
   */
   for (rule = interpretation_rule; *pstrlist; pstrlist++) {
      if (*rule == 0) {
         rule = interpretation_rule;
         ep = lCreateElem(descr);
         if (!ep) {
            DPRINTF(("cull_parse_string_list: cannot create another element\n"));
            lFreeList(list);
            DEXIT;
            return -5;      
         }
         lAppendElem(list, ep);
      }
      type = lGetType(descr, *rule);
      switch (type) {
         /*
         ** the format specifiers were copied from some cull routines
         ** where similar things happen
         */
         case lFloatT:
            {
               lFloat f;

               if (sscanf(*pstrlist, "%f", &f) != 1) {
                  DPRINTF(("cull_parse_string_list: " \
                           "error interpreting float: %s\n", *pstrlist));
                  lFreeList(list);
                  DEXIT;
                  return -6;
               }
               lSetFloat(ep, *rule, f);
            }
            break;

         case lDoubleT:
            {
               lDouble dd;

               if (sscanf(*pstrlist, "%99lg", &dd) != 1) {
                  DPRINTF(("cull_parse_string_list: " \
                           "error interpreting double: %s\n", *pstrlist));
                  lFreeList(list);
                  DEXIT;
                  return -7;
               }
               lSetDouble(ep, *rule, dd);
            }
            break;
       
         case lUlongT:
            {
               lUlong ul;

               if (sscanf(*pstrlist, u32, &ul) != 1) {
                  DPRINTF(("cull_parse_string_list: " \
                           "error interpreting ulong: %s\n", *pstrlist));
                  lFreeList(list);
                  DEXIT;
                  return -8;
               }
               lSetUlong(ep, *rule, ul);
            }
            break;


         case lLongT:
            {
               lLong l;

               if (sscanf(*pstrlist, "%ld", &l) != 1) {
                  DPRINTF(("cull_parse_string_list: " \
                           "error interpreting long: %s\n", *pstrlist));
                  lFreeList(list);
                  DEXIT;
                  return -9;
               }
               lSetLong(ep, *rule, l);
            }
            break;

         case lCharT:
            {
               lChar c;

               if (sscanf(*pstrlist, "%c", &c) != 1) {
                  DPRINTF(("cull_parse_string_list: " \
                           "error interpreting char: %s\n", *pstrlist));
                  lFreeList(list);
                  DEXIT;
                  return -10;
               }
               lSetChar(ep, *rule, c);
            }
            break;

         case lIntT:
            {
               lInt i;

               if (sscanf(*pstrlist, "%d", &i) != 1) {
                  DPRINTF(("cull_parse_string_list: " \
                           "error interpreting int: %s\n", *pstrlist));
                  lFreeList(list);
                  DEXIT;
                  return -11;
               }
               lSetInt(ep, *rule, i);
            }
            break;

         case lStringT:
            lSetString(ep, *rule, *pstrlist);
            break;
     
         case lHostT:
            lSetHost(ep, *rule, *pstrlist);
            break;

         case lListT:
            /*
            ** list types are skipped at the moment
            */
            DPRINTF(("skipped list type")); 
            break;

         default:
            DPRINTF(("encountered unknown list field type %d\n", type));
            lFreeList(list);
            DEXIT;
            return -12;

      } /* end switch */
      rule++;
   } /* end for */

   if (*rule != 0) {
      DPRINTF(("invalid number of entries specified\n"));
      lFreeList(list);
      DEXIT;
      return -13;
   } 
   *pplist = list;
   DPRINTF(("list parsed: \n"));

   DEXIT;
   return 0;
}


/*
** NAME
**   cull_parse_definition_list
** PARAMETER
**   str                 - string to be parsed
**   lpp                 - list pointer-pointer to be set the list,
**                         or NULL if first token is NONE
**   name                - name for the cull list
**   descr               - type of the cull list
**   interpretation_rule - fields to be filled, normally 2 entries
** RETURN
**
** EXTERNAL
**
** DESCRIPTION
**   parses list of form var=value,var=value,...
*/
int cull_parse_definition_list(
char *str,
lList **lpp,
const char *name,
lDescr *descr,
intprt_type *interpretation_rule 
) {
   char **pstr;
   int ret;

   DENTER(BASIS_LAYER, "cull_parse_definition_list");
   if (!str || !lpp) {
      DEXIT;
      return -1;
   }
   pstr = string_list(str, " ,=\t\n\f\r\v", NULL);
   if (!pstr) {
      DEXIT;
      return -2;
   }
   if (!strcasecmp("NONE", pstr[0])) {
      *lpp = NULL;
      free(pstr);
      DEXIT;
      return 0;
   }
   ret = cull_parse_string_list(pstr, name, descr, interpretation_rule, lpp);
   free(pstr);
   if (ret) {
      DEXIT;
      return -3;
   }
   DEXIT;
   return 0;
}


/*
** NAME
**   cull_merge_definition_list
** PARAMETER
**   lpp_old       - pointer to list which is to be changed, pointer cannot be NULL
**                   if list is NULL then list is created if lp_new contains any
                     elements
**   lp_new        - list to update *lpp_old with, or NULL
**   nm_var        - field containing the variable, must be in both lists, can
**                   be any valid type except list
**   nm_value      - field containing the value, must be in both lists, can
**                   be any valid type except list
** RETURN
**
** EXTERNAL
**
** DESCRIPTION
**   merges two lists of form var=value,var=value,...
**   does NOT remove duplicate entries within one list,
**   for this purpose see cull_compress_definition_list
*/
int cull_merge_definition_list(
lList **lpp_old,
lList *lp_new,
int nm_var,
int nm_value 
) {
   lListElem *ep_old, *ep_new;
   int type;
   int is_there;

   DENTER(CULL_LAYER, "cull_merge_definition_list");
   
   if (!lp_new) {
      DEXIT;
      return 0;
   }
   
   if (!lpp_old) {
      DEXIT;
      return -1;
   }
   /*
   ** problem: type equal check missing here, look at lAddList
   ** problem: also check if (both) have nm_var and nm_value
   */
   
   if (lGetType(lGetListDescr(*lpp_old), nm_var) != 
       lGetType(lGetListDescr(lp_new), nm_var)) {
      DPRINTF(("cull_merge_definition_list: conflicting types for nm_var\n"));
      DEXIT;
      return -2;
   }
   if (lGetType(lGetListDescr(*lpp_old), nm_value) != 
       lGetType(lGetListDescr(lp_new), nm_value)) {
      DPRINTF(("cull_merge_definition_list: conflicting types for nm_value\n"));
      DEXIT;
      return -3;
   }

   if (!*lpp_old) {
      *lpp_old = lCreateList("copied list", lGetListDescr(lp_new));
      if (!*lpp_old) {
         DPRINTF(("memory allocation fault\n"));
         DEXIT;
         return -4;
      }
   }

   
   /*
   ** look for each variable if it is already in the list
   ** if it is, change the value to the new value
   ** if it isn't, append it
   */
   for_each(ep_new, lp_new) {
      is_there = 0;
      for_each(ep_old, *lpp_old) {
         type = lGetType(lGetListDescr(lp_new), nm_var);
         switch (type) {
         case lFloatT:
            if (lGetFloat(ep_new, nm_var) == lGetFloat(ep_old, nm_var)) {
               is_there = 1;
            }
            break;
         case lDoubleT:
            if (lGetDouble(ep_new, nm_var) == lGetDouble(ep_old, nm_var)) {
               is_there = 1;
            }
            break;
         case lUlongT:
            if (lGetUlong(ep_new, nm_var) == lGetUlong(ep_old, nm_var)) {
               is_there = 1;
            }
            break;
         case lLongT:
            if (lGetLong(ep_new, nm_var) == lGetLong(ep_old, nm_var)) {
               is_there = 1;
            }
            break;
         case lCharT:
            if (lGetChar(ep_new, nm_var) == lGetChar(ep_old, nm_var)) {
               is_there = 1;
            }
            break;
         case lIntT:
            if (lGetInt(ep_new, nm_var) == lGetInt(ep_old, nm_var)) {
               is_there = 1;
            }
            break;
         case lStringT:
            if ( /* lGetString(ep_new, nm_var) &&  ??? andreas */
                !sge_strnullcmp(lGetString(ep_new, nm_var), lGetString(ep_old, nm_var))) {
               is_there = 1;
            }
            break;
         case lHostT:
            if ( !sge_strnullcmp(lGetHost(ep_new, nm_var), lGetHost(ep_old, nm_var))) {
               is_there = 1;
            }
            break;

         case lListT:
            DPRINTF(("cull_merge_definition_list: " \
               "list type not implemented with this function\n"));
            DEXIT;
            return -4;
         default:
            DPRINTF(("cull_merge_definition_list: invalid type\n"));
            DEXIT;
            return -5;
         } /* end switch */
         if (is_there) {
            break;
         }
      }
      if (ep_new == ep_old) {
         /*
         ** do nothing
         */
      }
      else if (is_there) {
         type = lGetType(lGetListDescr(lp_new), nm_value);
         switch (type) {
         case lFloatT:
            lSetFloat(ep_old, nm_value, lGetFloat(ep_new, nm_value));
            break;
         case lDoubleT:
            lSetDouble(ep_old, nm_value, lGetDouble(ep_new, nm_value));
            break;
         case lUlongT:
            lSetUlong(ep_old, nm_value, lGetUlong(ep_new, nm_value));
            break;
         case lLongT:
            lSetLong(ep_old, nm_value, lGetLong(ep_new, nm_value));
            break;
         case lCharT:
            lSetChar(ep_old, nm_value, lGetChar(ep_new, nm_value));
            break;
         case lIntT:
            lSetInt(ep_old, nm_value, lGetInt(ep_new, nm_value));
            break;
         case lStringT:
            lSetString(ep_old, nm_value, lGetString(ep_new, nm_value));
            break;
         case lHostT:
            lSetHost(ep_old, nm_value, lGetHost(ep_new, nm_value));
            break;

         case lListT:
            DPRINTF(("cull_merge_definition_list: " \
               "list type not implemented with function\n"));
            DEXIT;
            return -6;
         default:
            DPRINTF(("cull_merge_definition_list: invalid type to set\n"));
            DEXIT;
            return -7;
         } /* end switch */
      }
      else {
         lAppendElem(*lpp_old, lCopyElem(ep_new));
      }
   }

   DEXIT;
   return 0;
}


/*
** NAME
**   cull_compress_definition_list
** PARAMETER
**   lp            - list to compress
**   nm_var        - field containing the variable, can
**                   be any valid type except list
**   nm_value      - field containing the value, can
**                   be any valid type except list
** RETURN
**
** EXTERNAL
**
** DESCRIPTION
**   removes duplicate entries within a definition list
**   e.g. a=1,a=2 -> a=2
**   removes the unnecessary elements
*/
int cull_compress_definition_list(
lList *lp,
int nm_var,
int nm_value,
int double_keys 
) {
   lListElem *ep_one, *ep_other;
   int is_there;
   int type;

   DENTER(BASIS_LAYER, "cull_compress_definition_list");

   for_each(ep_one, lp) {
      for (ep_other = lFirst(lp); ep_other; ) {
         if (ep_one == ep_other) {
            break;
         }
         is_there = 0;
         type = lGetType(lGetListDescr(lp), nm_var);
         switch (type) {
         case lFloatT:
            if (lGetFloat(ep_one, nm_var) == lGetFloat(ep_other, nm_var)) {
               is_there = 1;
            }
            break;
         case lDoubleT:
            if (lGetDouble(ep_one, nm_var) == lGetDouble(ep_other, nm_var)) {
               is_there = 1;
            }
            break;
         case lUlongT:
            if (lGetUlong(ep_one, nm_var) == lGetUlong(ep_other, nm_var)) {
               is_there = 1;
            }
            break;
         case lLongT:
            if (lGetLong(ep_one, nm_var) == lGetLong(ep_other, nm_var)) {
               is_there = 1;
            }
            break;
         case lCharT:
            if (lGetChar(ep_one, nm_var) == lGetChar(ep_other, nm_var)) {
               is_there = 1;
            }
            break;
         case lIntT:
            if (lGetInt(ep_one, nm_var) == lGetInt(ep_other, nm_var)) {
               is_there = 1;
            }
            break;
         case lStringT:
            if (double_keys) {
               if (!sge_strnullcmp(lGetString(ep_one, nm_var), lGetString(ep_other, nm_var)) 
                && !sge_strnullcmp(lGetString(ep_one, nm_value), lGetString(ep_other, nm_value)))
                  is_there = 1;
            } else {
               if (lGetString(ep_one, nm_var) && 
                   !sge_strnullcmp(lGetString(ep_one, nm_var), 
                      lGetString(ep_other, nm_var))) 
                  is_there = 1;
            }
            break;

         case lHostT:
            if (double_keys) {
               if (!sge_strnullcmp(lGetHost(ep_one, nm_var), lGetHost(ep_other, nm_var)) 
                && !sge_strnullcmp(lGetHost(ep_one, nm_value), lGetHost(ep_other, nm_value)))
                  is_there = 1;
            } else {
               if (lGetHost(ep_one, nm_var) && 
                   !sge_strnullcmp(lGetHost(ep_one, nm_var), lGetHost(ep_other, nm_var))) 
                  is_there = 1;
            }
            break;

         case lListT:
            DPRINTF(("cull_compress_definition_list: " \
               "list type not implemented with this function\n"));
            DEXIT;
            return -4;
         default:
            DPRINTF(("cull_compress_definition_list: invalid type\n"));
            DEXIT;
            return -5;
         } /* end switch */
         
         ep_other = lNext(ep_other);
         if (is_there) {
            /*
            ** ep_other must always point to a valid element, 
            ** or next "increase" fails, the element that comes later on
            ** in the list is kept, because ep_other < ep_one always
            */
            lRemoveElem(lp, lPrev(ep_other));
         }
      }
   }
   DEXIT;
   return 0;
}



/*
** NAME
**   cull_parse_simple_list
** PARAMETER
**   str                 - string to be parsed
**   lpp                 - list pointer-pointer to be set the list,
**                         or NULL if first token is NONE
**   name                - name for the cull list
**   descr               - type of the cull list
**   interpretation_rule - fields to be filled, normally 1 entry
** RETURN
**
** EXTERNAL
**
** DESCRIPTION
**   parses a simple list like a,b,c...
**   string value NONE as first token is interpreted as no list
*/
int cull_parse_simple_list(
char *str,
lList **lpp,
char *name,
lDescr *descr,
intprt_type *interpretation_rule 
) {
   char **pstr;
   int ret;

   DENTER(BASIS_LAYER, "cull_parse_simple_list");
   if (!str || !lpp) {
      DPRINTF(("cull_parse_simple_list: NULL pointer received\n"));
      DEXIT;
      return -1;
    }

   /*
   ** = is not a delimiter here
   */
   pstr = string_list(str, " ,\t\n\f\r\v",  NULL);
   if (!pstr) {
      DPRINTF(("cull_parse_simple_list: could not parse string\n"));
      DEXIT;
      return -2;
   }
   if (!strcasecmp("NONE", pstr[0])) {
      *lpp = NULL;
      free(pstr);
      DPRINTF(("cull_parse_simple_list: String is NONE, no list, not an error\n"));
      DEXIT;
      return 0;
   }
   
   ret = cull_parse_string_list(pstr, name, descr, interpretation_rule, lpp);
   free(pstr);
   if (ret) {
      DPRINTF(("cull_parse_simple_list: cull_parse_string_list returns %d\n", ret));
      DEXIT;
      return -3;
   }
   DEXIT;
   return 0;
}


/*
** NAME
**   uni_print_list
** PARAMETER
**   fp                    -   pointer to file to write string list to or NULL
**   buff                  -   buffer to write string list to, can be NULL
**                             if fp is not, data is then written to fp
**   max_len               -   maximum bytes to be written, can be 0 if
**                             fp is given
**   lp                    -   list to write to file or buffer
**   interpretation_rule   -   list elements to be written,
**                             all elements EXCEPT LISTS can be given here,
**                             to say it once more: sublists are not supported
**   pdelis                -   delimiter string to be inserted between elements
**                             pdelis[0] : string used to separate fields
**                             pdelis[1] : string used to separate records
**                             pdelis[2] : string inserted after end of list,
**                                         e.g. a newline
**   flags                 -   FLG_NO_DELIS_STRINGS - leave out delimiters
**                             before zero or NULL strings
**                             FLG_NO_DELIS_NUMBERS - leave out delimiters
**                             and the following "0" if a number is 0
**                             Be careful in using these 2 options. If more
**                             than one field is optional, this can lead to
**                             ambiguous parsing!
**
** RETURN
**
** EXTERNAL
**
** DESCRIPTION
**   prints certain fields of a list to a given stream or buffer, separated by 
**   delimiters, be careful: sublists are NOT yet implemented
*/
int uni_print_list(
FILE *fp,
char *buff,
u_long32 max_len,
lList *lp,
intprt_type *which_elements_rule,
const char *pdelis[],
unsigned long flags 
) {
   lListElem *ep;
   intprt_type *rule;
   int type;
   const lDescr *descr;
   int begin = 1;
   u_long32 cb;
   u_long32 cb_sum = 0;
   char str[256];
   const char *cp;

   DENTER(BASIS_LAYER, "uni_print_list");

   /*
   ** problem: one might allow NULL deli as no deli
   */
   if (!which_elements_rule || !pdelis) {
      DPRINTF(("uni_print_list: NULL pointer received\n"));
      DEXIT;
      return -1;
   }
   if (!fp && !buff) {
      DPRINTF(("uni_print_list: must have either file or buffer\n"));
      DEXIT;
      return -1;
   }
   if (buff && !max_len) {
      DPRINTF(("uni_print_list: zero len output required\n"));
      DEXIT;
      return -1;
   }

   if (!lp) {
      if (max_len && (cb_sum + (sizeof("NONE") - 1) > max_len)) {
         DPRINTF(("max_len too small even for zero list\n"));
         DEXIT;
         return -1;
      }
      if (fp) {
/*          cb = FPRINTF((fp, "NONE")); */
         FPRINTF_ASSIGN(cb, (fp, "NONE"));
      }
      else {
         cb = sizeof("NONE") - 1;
         strcpy(buff, "NONE");
      }
      buff += cb;
      cb_sum += cb;
      if (pdelis[2] && *pdelis[2]) {
         if (max_len && (cb_sum + strlen(pdelis[2]) > max_len)) {
            DPRINTF(("max_len too small even for zero list plus delimiter\n"));
            DEXIT;
            return -1;
         }
         if (fp) {
            cb = fprintf(fp, "%s", pdelis[2]);
         }
         else {
            cb = strlen(pdelis[2]);
            sprintf(buff, "%s", pdelis[2]);
         }
         buff += cb;
         cb_sum += cb;
      }
      DEXIT;
      return 0;
   }
   if (*which_elements_rule == 0) {
      DPRINTF(("uni_print_list: zero interpretation rule\n"));
      DEXIT;
      return -2;
   }

   descr = lGetListDescr(lp);
   if (!descr) {
      DPRINTF(("uni_print_list: list has no descriptor\n"));
      DEXIT;
      return -3;
   }

   for_each(ep, lp) {

      if (!begin && pdelis[1] && *pdelis[1]) {
         if (max_len && (cb_sum + strlen(pdelis[1]) > max_len)) {
            DPRINTF(("max_len too small\n"));
            DEXIT;
            return -1;
         }
         if (fp) {
/*             cb = FPRINTF((fp, "%s", pdelis[1])); */
            FPRINTF_ASSIGN(cb, (fp, "%s", pdelis[1]));
         }
         else {
            cb = strlen(pdelis[1]);
            sprintf(buff, "%s", pdelis[1]);
         }
         if (cb <= 0) {
            DPRINTF(("uni_print_list: error writing delimiter 1\n"));
            DEXIT;
            return -4;
         }
         buff += cb;
         cb_sum += cb;
      }

      cb = 0;
      for (rule = which_elements_rule; *rule; rule++) {
         /*
         ** before writing the delimiter, we look ahead
         */
         cp = str;
         type = lGetType(descr, *rule);

         switch (type) {
         case lFloatT:

            sprintf(str, "%.10g", lGetFloat(ep, *rule));
            break;

         case lDoubleT:
            sprintf(str, "%.10g", lGetDouble(ep, *rule));
            break;
       
         case lUlongT:
            sprintf(str, u32, lGetUlong(ep, *rule));
            break;

         case lLongT:
            sprintf(str, "%ld", lGetLong(ep, *rule));
            break;

         case lCharT:
            sprintf(str, "%c", lGetChar(ep, *rule));
            break;

         case lIntT:
            sprintf(str, "%d", lGetInt(ep, *rule));
            break;

         case lStringT:
            cp = lGetString(ep, *rule);
            if (!cp) {
               cp = "";
            }
            break;

         case lHostT:
            cp = lGetHost(ep, *rule);
            if (!cp) {
               cp = "";
            }
            break;


         case lListT:
            /*
            ** list types are skipped at the moment
            */
            *str = 0;
            DPRINTF(("skipped list type")); 
            break;

         default:
            DPRINTF(("encountered unknown list field type %d\n", type));
            DEXIT;
            return -19;
         } /* end switch */

         /*
         ** now that we know what the field is we can decide to suppress
         ** the delimiter
         ** at the moment only trailing delimiters are suppressed
         ** one might also want to suppress delimiters in eg.  [host]:path
         ** but if there are more than one optional field and delimiters are
         ** left out, then unique interpretation is lost, so be careful
         */
         if (pdelis[0] && *pdelis[0] && (rule != which_elements_rule) &&
             (!(flags & FLG_NO_DELIS_STRINGS) || (*cp && cb))  &&
             (!(flags & FLG_NO_DELIS_NUMBERS) || !L_IS_NUM_TYPE(type) 
               || strcmp(cp, "0"))) {
            if (max_len && (cb_sum + strlen(pdelis[0]) > max_len)) {
               DPRINTF(("max_len too small\n"));
               DEXIT;
               return -1;
            }
            if (fp) {
/*                cb = FPRINTF((fp, "%s", pdelis[0])); */
               FPRINTF_ASSIGN(cb, (fp, "%s", pdelis[0]));
            }
            else {
               cb = strlen(pdelis[0]);
               sprintf(buff, "%s", pdelis[0]);
            }
            if (cb <= 0) {
               DPRINTF(("uni_print_list: error writing delimiter\n"));
               DEXIT;
               return -5;
            }
            buff += cb;
            cb_sum += cb;
         }

         if (max_len && (cb_sum + strlen(cp) > max_len)) {
            DPRINTF(("max_len too small\n"));
            DEXIT;
            return -1;
         }
         if (*cp) {
            if (fp) {
               FPRINTF_ASSIGN(cb, (fp, "%s", cp));
            }
            else {
               cb = strlen(cp);
               sprintf(buff, "%s", cp);
            }
            if (cb <= 0) {
               DPRINTF(("uni_print_list: error writing to file\n"));
               DEXIT;
               return -6;
            }
            buff += cb;
            cb_sum += cb;
         }
         begin = 0;

      } /* end for this list element */
   } /* end for_each */

   if (!begin && pdelis[2] && *pdelis[2]) {
      if (max_len && (cb_sum + strlen(pdelis[2]) > max_len)) {
         DPRINTF(("max_len too small\n"));
         DEXIT;
         return -1;
      }
      if (fp) {
         FPRINTF_ASSIGN(cb, (fp, "%s", pdelis[2]));
      }
      else {
         cb = strlen(pdelis[2]);
         sprintf(buff, "%s", pdelis[2]);
      }
      if (cb <= 0) {
         DPRINTF(("uni_print_list: error writing delimiter 1\n"));
         DEXIT;
         return -4;
      }
      buff += cb;
      cb_sum += cb;
   }

   DEXIT;
   return 0;
FPRINTF_ERROR:
   DEXIT;
   return -7;
}

/****
 **** fprint_cull_list
 ****
 **** Prints str and field 'fi' (must be string) of
 **** every element of lList lp to file fp separated
 **** by blanks. If fp is NULL, "NONE" will be printed.
 ****/
int fprint_cull_list(
FILE *fp,
char *str,
lList *lp,
int fi 
) {
   lListElem *ep;

   DENTER(TOP_LAYER, "fprint_cull_list");

   FPRINTF((fp, "%s", str));

   if(!lp) {
      FPRINTF((fp, "NONE\n"));
   }
   else {
      for_each(ep, lp) {
         FPRINTF((fp, "%s", lGetString(ep, fi)));
         if (lNext(ep))
            FPRINTF((fp, " "));
      }
      FPRINTF((fp, "\n"));
   }

   DEXIT;
   return 0;

FPRINTF_ERROR:
   DEXIT;
   return -1;
}                   

int fprint_thresholds(
FILE *fp,
char *name,
lList *thresholds,
int print_slots 
) {
   lListElem *lep;
   int printed = 0;
   const char *s;
   char buffer[1024];

   DENTER(TOP_LAYER, "fprint_thresholds");

   FPRINTF((fp, "%s", name));

   for_each(lep, thresholds) {
      if (print_slots || strcmp("slots", lGetString(lep, CE_name))) {
         if (printed) {
            FPRINTF((fp, ","));
         }

         if (!(s=lGetString(lep, CE_stringval))) {
            sprintf(buffer, "%f", lGetDouble(lep, CE_doubleval));
            s = buffer;
         }

         FPRINTF((fp, "%s=%s", lGetString(lep, CE_name), s));
         printed++;
      }
   }
   if (printed == 0) {
      FPRINTF((fp, "NONE\n"));
   } else {
      FPRINTF((fp, "\n"));
   }

   DEXIT;
   return 0;

FPRINTF_ERROR:
   DEXIT;
   return -1;
}            

int parse_list_hardsoft(
lList *cmdline,
char *option,
lListElem *job,
int hard_field,
int soft_field 
) {
   lList *hard_list = NULL;
   lList *soft_list = NULL;
   lList *lp = NULL;
   lListElem *ep = NULL;

   DENTER(TOP_LAYER, "parse_list_hardsoft");

   hard_list = lCopyList("job hard sublist", lGetList(job, hard_field));
   if (soft_field)
      soft_list = lCopyList("job soft sublist", lGetList(job, soft_field));

   while ((ep = lGetElemStr(cmdline, SPA_switch, option))) {
      lp = NULL;
      lXchgList(ep, SPA_argval_lListT, &lp);
      if (lp) {
         if (!soft_field || lGetInt(ep, SPA_argval_lIntT) < 2) {
            if (!hard_list)
               hard_list = lp;
            else
               lAddList(hard_list, lp);
         }
         else {
            if (!soft_list)
                  soft_list = lp;
            else         
               lAddList(soft_list, lp);
         }
      }
      lRemoveElem(cmdline, ep);
    }
    lSetList(job, hard_field, hard_list);
    if (soft_field)
      lSetList(job, soft_field, soft_list);

    DEXIT;
    return 0;
}

int parse_list_simple(
lList *cmdline,
char *option,
lListElem *job,
int field,
int nm_var,
int nm_value,
u_long32 flags 
) {
   lList *destlist = NULL;
   lList *lp = NULL;
   lListElem *ep;

   DENTER(TOP_LAYER, "parse_list_simple");

DPRINTF(("OPTION: %s\n", option));
   destlist = lCopyList("job sublist", lGetList(job, field));

   while ((ep = lGetElemStr(cmdline, SPA_switch, option))) {
      lp = NULL;
      lXchgList(ep, SPA_argval_lListT, &lp);
      if (flags & FLG_LIST_APPEND || flags & FLG_LIST_MERGE_DOUBLE_KEY) {
         if (lp) {  
            if (!destlist) {
               destlist = lp;
            } else {
               lAddList(destlist, lp);
               /*
               ** was freed by lAddList
               */
               if (flags & FLG_LIST_MERGE_DOUBLE_KEY) {
                  cull_compress_definition_list(destlist, nm_var, nm_value, 1);
               }
               lp = NULL;
            }
         }
      } else if (flags & FLG_LIST_MERGE) {
         if (lp) {
            if (!destlist) {
               destlist = lp; 
            } else {
               cull_merge_definition_list(&destlist, lp, nm_var, nm_value);
               lFreeList(lp);
            }
         }
      } else {
         if (destlist)
            lFreeList(destlist);
         destlist = lp;
      } 
      lRemoveElem(cmdline, ep);
   } 
   lSetList(job, field, destlist);

   DEXIT;
   return 0;
}

