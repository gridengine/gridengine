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
#include <string.h>

#include "sgermon.h"
#include "sge_gdi_intern.h"
#include "sge_requestL.h"
#include "sge_resource.h"
#include "cull_parse_util.h"
#include "sge_string.h"
#include "sge_log.h"
#include "sge_range.h"
#include "sge_complex.h"

#include "msg_sgeobjlib.h"

static void sge_show_ce_type_list(lList *cel, const char *indent, const char *separator);



/***********************************************************************
   parse -l request and add to resource list

   allowed requests:

   -l[<range-expr>] attr[=value],...
   str comes without the leading -l and contains a blank if there is no
   range

   What we want to generate is:

resources
(RE_Type)
   |
   | |----------------|
   |_| range_list     | --- .... (RN_Type)
   | |----------------|
   | | complex_entries| --- .... (CE_Type)
   | |----------------|
   |
   |- ....
   |
   ...

   hard_soft         - for name of resources list 
 ***********************************************************************/
lList *
sge_parse_resources(lList *resources, const char *range_str,
                    const char *str, const char *hard_soft) 
{
   lListElem *request_el;
   lList *complex_attributes=NULL;
   lListElem *complex_attribute;
   const char *cp;
   const char *attr;
   char *value;
   char name[256];

   DENTER(TOP_LAYER, "sge_parse_resources");

   /* alloc. space for resource list, attrib. list and resource elem. */
   if (!resources) {
      strcpy(name,hard_soft);
      strcat(name,"_resources");
      if (!(resources = lCreateList(name,RE_Type))) {
         ERROR((SGE_EVENT, MSG_PARSE_NOALLOCREQRES));
         DEXIT;
         return NULL;
      }
   }
   if (!(complex_attributes = lCreateList("attributes",CE_Type))) {
      ERROR((SGE_EVENT, MSG_PARSE_NOALLOCATTRLIST));
      lFreeList(resources);
      DEXIT;
      return NULL;
   }
   if (!(request_el = lCreateElem(RE_Type))) {
      ERROR((SGE_EVENT, MSG_PARSE_NOALLOCRESELEM));
      lFreeList(resources);
      lFreeList(complex_attributes);
      DEXIT;
      return NULL;
   }
   
   if (range_str && 
         (*range_str != ' ') && 
         (*range_str != '\0')) {   /* there comes a range */
      lList *rl = NULL;
      range_list_parse_from_string(&rl, NULL, range_str,
                                   0, 0, INF_ALLOWED);
      if (rl == NULL) {
         ERROR((SGE_EVENT, MSG_PARSE_NOVALIDSLOTRANGE_S, range_str));
         lFreeList(resources);
         lFreeList(complex_attributes);
         DEXIT;
         return NULL;
      }
      lSetList(request_el, RE_ranges, rl);
   }

   /* str now points to the attr=value pairs */

   while ((cp = sge_strtok(str, ", "))) {
      str = NULL;       /* for the next strtoks */

      if (!(complex_attribute = lCreateElem(CE_Type))) {
         ERROR((SGE_EVENT, MSG_PARSE_NOALLOCATTRELEM));
         lFreeList(resources);
         lFreeList(complex_attributes);
         lFreeElem(request_el);
         DEXIT;
         return NULL;
      }
      
      /*
      ** recursive strtoks didnt work
      */
      attr = cp;
      if ((value = strchr(cp, '=')))
         *value++ = 0;

      lSetString(complex_attribute, CE_name, attr);
      lSetString(complex_attribute, CE_stringval, value);

      lAppendElem(complex_attributes, complex_attribute);
   }

   lSetList(request_el, RE_entries, complex_attributes);
   lAppendElem(resources, request_el);

   DEXIT;
   return resources;
}

/*
** NAME
**   unparse_resources - makes a string from a list of resource requests
** PARAMETER
**   fp      - file pointer or NULL if output to buffer is requested
**   buff    - buffer to store output or NULL if output to fp is requested
**   max_len - maximum number of bytes to write to the buffer
**   rlp     - RE_Type list to be converted to string
**  
** RETURN
**   
** EXTERNAL
**
** DESCRIPTION
**   
*/
int unparse_resources(FILE *fp, char *buff, u_long32 max_len, lList *rlp) 
{
   intprt_type attr_fields[] = { CE_name, CE_stringval, 0 };
   const char *attr_delis[] = {"=", ",", "\n"};
   lListElem *rep;
   int ret;
   u_long32 cb = 0;

   DENTER(BASIS_LAYER, "unparse_resources");
  
   if (buff)
      buff[0] = '\0';

   /* sort alphabetically at least */
   lPSortList(rlp, "%I+", CE_name);
   for_each(rep, rlp) {
      if (lGetList(rep, RE_ranges)) {
         dstring range_string = DSTRING_INIT;
         const char *tmp_string;

         range_list_print_to_string(lGetList(rep, RE_ranges), 
                                    &range_string, 1);
         tmp_string = sge_dstring_get_string(&range_string);
         if (buff != NULL) {
            if (strlen(tmp_string) > max_len) {
               DEXIT;
               return -3;
            } else {
               strcat(buff, tmp_string);
            }
         } else {
            fprintf(fp, tmp_string);
         }

         if (!fp && buff) {
            cb = strlen(buff);
            buff += cb;
            max_len -= cb;
         }
      }

      if (!fp && buff) {
         if (!max_len) {
            DEXIT;
            return -1;
         }
         strcpy(buff, " ");
         buff++;
         max_len--;
      }
      else {
         fprintf(fp, " ");
      }


      ret = uni_print_list(fp, buff, max_len, 
         lGetList(rep, RE_entries), attr_fields, 
         attr_delis, 0);
      if (ret) {
         DEXIT;
         return ret;
      }
      if (!fp && buff) {
          cb = strlen(buff);
          buff += cb;
          max_len -= cb;
      }
   }
   DPRINTF(("buff: %s\n", buff));
   DEXIT;
   return 0;
}

/*************************************************************/
/* reqlist RQ_Type List */
void sge_show_resource_list(lList *reqlist) 
{
   lListElem *req;

   DENTER(TOP_LAYER, "sge_show_resource_list");

   for_each (req, reqlist) {
      sge_show_re_type_list(lGetList(req, RQ_requests));
   }

   DEXIT;
   return;
}

/*************************************************************/
/* rel RE_Type List */
void sge_show_re_type_list(lList *rel) 
{
   lListElem* res;
   lList* range;

   DENTER(TOP_LAYER, "sge_show_re_type_list");

   for_each (res, rel) {
      range = lGetList(res, RE_ranges);
      if (range) {
         dstring range_string = DSTRING_INIT;

         range_list_print_to_string(range, &range_string, 1);
         printf("%s ", sge_dstring_get_string(&range_string));
         sge_dstring_free(&range_string);
      }
      sge_show_ce_type_list(lGetList(res, RE_entries), "", ",");
   }

   DEXIT;
   return;
}

/*************************************************************/
/* cel CE_Type List */
static void sge_show_ce_type_list(lList *cel, const char *indent, 
                                  const char *separator) 
{
   bool first = true;
   lListElem *ce;
   const char *s;
   
   DENTER(TOP_LAYER, "sge_show_ce_type_list");

   /* walk through complex entries */
   for_each (ce, cel) { 
      if (first) {
         first = false;
      } else {
         printf("%s", separator);
         printf("%s", indent);
      }
    
      s = lGetString(ce, CE_stringval); 
      if(s) {
         printf("%s=%s", lGetString(ce, CE_name), s);
      } else {
         printf("%s", lGetString(ce, CE_name));
      }
   }
      
   DEXIT;
   return;
}

/*************************************************************/
/* rel RE_Type List */
void sge_show_re_type_list_line_by_line(const char *label, 
                                        const char *indent, 
                                        lList *rel) 
{
   lListElem* res;
   lList* range;
   int first = 1;

   DENTER(TOP_LAYER, "sge_show_re_type_list_line_by_line");

   
   for_each (res, rel) {
      range = lGetList(res, RE_ranges);
      if (range) {
         dstring range_string = DSTRING_INIT;

         range_list_print_to_string(range, &range_string, 1);
         printf("%s%s\n", indent, sge_dstring_get_string(&range_string));
         sge_dstring_free(&range_string);
      }
      if (first) {
         printf("%s", label);
         sge_show_ce_type_list(lGetList(res, RE_entries), indent, "\n");
      }

      printf("\n");
   }

   DEXIT;
   return;
}

/* rlp RE_Type */
void sge_compress_resources(lList *rlp) 
{
   lListElem *rep, *rep_first_no_ranges = NULL;
   lList *lp_ranges;
   lList *lp;

   DENTER(TOP_LAYER, "sge_compress_resources");

   for_each(rep, rlp) {
      lp_ranges = lGetList(rep, RE_ranges);
      if (!lp_ranges) {
         if (!rep_first_no_ranges) {
            rep_first_no_ranges = rep;
            cull_compress_definition_list(lGetList(rep, RE_entries), 
                                          CE_name, CE_stringval, 0);
         } else {
            lp = lCopyList("resource list compressed", 
               lGetList(rep_first_no_ranges, RE_entries));
            cull_merge_definition_list(&lp, lGetList(rep, RE_entries), 
                                       CE_name, CE_stringval);
            lSetList(rep_first_no_ranges, RE_entries, lp);
            /*
             * the loop is not executed correctly if you just remove an element
             * this works because rep is never the first element here
             */
            rep = lPrev(rep);
            lRemoveElem(rlp, lNext(rep));
         }
      }      
   }

   /*
    * the element with no ranges should be the first element
    * and there should be only one
    */
   if (rep_first_no_ranges) {
      lInsertElem(rlp, NULL, lDechainElem(rlp, rep_first_no_ranges));
   }

   DEXIT;
   return;
}
