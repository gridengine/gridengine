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

#include "cull.h"
#include "sgermon.h"
#include "sge_resource.h"
#include "cull_parse_util.h"
#include "sge_string.h"
#include "sge_log.h"
#include "sge_range.h"
#include "sge_centry.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

/*
**
** NOTES
**    MT-NOTE: all function in this module are not MT safe
*/

static void show_ce_type_list(lList *cel, const char *indent, const char *separator);



/***********************************************************************
   parse -l request and add to resource list

   allowed requests:

   -l attr[=value],...
   str comes without the leading -l and contains a blank if there is no
   range

   What we want to generate is a list of CE_Type containing the parsed
   resources (complex_entries).

   hard_soft         - for name of resources list 
   check_value       - if true, it makes sure, that each attribut request has
                       also a value assigned. 
 ***********************************************************************/
lList *
sge_parse_resources(lList *complex_attributes,
                    const char *str, const char *hard_soft,
                    bool check_value) 
{
   const char *cp;

   DENTER(TOP_LAYER, "sge_parse_resources");

   /* allocate space for attribute list if no list is passed */
   if (complex_attributes == NULL) {
      if ((complex_attributes = lCreateList("attributes", CE_Type)) == NULL) {
         ERROR((SGE_EVENT, MSG_PARSE_NOALLOCATTRLIST));
         DEXIT;
         return NULL;
      }
   }

   /* str now points to the attr=value pairs */
   while ((cp = sge_strtok(str, ", "))) {
      lListElem *complex_attribute;
      const char *attr;
      char *value;

      str = NULL;       /* for the next strtoks */

      if ((complex_attribute = lCreateElem(CE_Type)) == NULL) {
         ERROR((SGE_EVENT, MSG_PARSE_NOALLOCATTRELEM));
         lFreeList(complex_attributes);
         DEXIT;
         return NULL;
      }
      
      /*
      ** recursive strtoks didnt work
      */
      attr = cp;
      if ((value = strchr(cp, '='))) {
         *value++ = 0;
      }

      if (attr == NULL || *attr == '\0') {
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWN_RESOURCE_S, ""));
         lFreeList(complex_attributes);
         DEXIT;
         return NULL;
      }

      if ((check_value) && (value == NULL || *value == '\0')) {
         ERROR((SGE_EVENT, MSG_CPLX_VALUEMISSING_S, attr));
         lFreeList(complex_attributes);
         DEXIT;
         return NULL;
      }

      lSetString(complex_attribute, CE_name, attr);
      lSetString(complex_attribute, CE_stringval, value);

      lAppendElem(complex_attributes, complex_attribute);
   }

   DEXIT;
   return complex_attributes;
}

/*
** NAME
**   unparse_resources - makes a string from a list of resource requests
** PARAMETER
**   fp      - file pointer or NULL if output to buffer is requested
**   buff    - buffer to store output or NULL if output to fp is requested
**   max_len - maximum number of bytes to write to the buffer
**   rlp     - CE_Type list to be converted to string
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
   int ret;

   DENTER(BASIS_LAYER, "unparse_resources");
  
   if (buff)
      buff[0] = '\0';

   /* sort alphabetically at least */
   lPSortList(rlp, "%I+", CE_name);

   ret = uni_print_list(fp, buff, max_len, rlp, attr_fields, attr_delis, 0);
   if (ret) {
      DEXIT;
      return ret;
   }

   DPRINTF(("buff: %s\n", buff));
   DEXIT;
   return 0;
}

void sge_show_ce_type_list(lList *rel) 
{
   DENTER(TOP_LAYER, "sge_show_ce_type_list");

   show_ce_type_list(rel, "", ",");

   DEXIT;
   return;
}

/*************************************************************/
/* cel CE_Type List */
static void show_ce_type_list(lList *cel, const char *indent, 
                                  const char *separator) 
{
   bool first = true;
   lListElem *ce;
   const char *s;
   
   DENTER(TOP_LAYER, "show_ce_type_list");

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
/* rel CE_Type List */
void sge_show_ce_type_list_line_by_line(const char *label, 
                                        const char *indent, 
                                        lList *rel) 
{
   DENTER(TOP_LAYER, "sge_show_ce_type_list_line_by_line");

   printf("%s", label);
   show_ce_type_list(rel, indent, "\n");
   printf("\n");

   DEXIT;
   return;
}

/* rlp CE_Type */
void sge_compress_resources(lList *rlp) 
{
   DENTER(TOP_LAYER, "sge_compress_resources");

   cull_compress_definition_list(rlp, CE_name, CE_stringval, 0);

   DEXIT;
   return;
}
