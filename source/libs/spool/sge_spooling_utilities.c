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

#include "sgermon.h"
#include "sge_log.h"

#include "sge_stdlib.h"
#include "sge_string.h"

#include "gdi_utility.h"
#include "sge_object.h"

#include "msg_common.h"
#include "msg_spoollib.h"

#include "sge_spooling_utilities.h"

const spool_instr spool_config_subinstr = 
{
   CULL_NAMELIST | CULL_TUPLELIST,
   true,
   true,
   NULL
};

const spool_instr spool_config_instr = 
{
   CULL_SPOOL,
   true,
   true,
   &spool_config_subinstr
};


static spooling_field *
_spool_get_fields_to_spool(lList **answer_list, const lDescr *descr, 
                           const spool_instr *instr);

/****** spool/utilities/spool_get_fields_to_spool() *********************
*  NAME
*     spool_get_fields_to_spool() -- which fields are to be spooled
*
*  SYNOPSIS
*     spooling_field * 
*     spool_get_fields_to_spool(lList **answer_list, const lDescr *descr, 
*                               const spool_instr *instr) 
*
*  FUNCTION
*     Returns an array of field descriptions (sge_spooling_field).
*     Which fields have to be spooled is retrieved from the CULL object
*     definition given in <descr> and the spooling instruction <instr>.
*
*     For each attribute the function checks if the attribute information 
*     fulfils the selection given in <instr> (e.g. attribute property 
*     CULL_SPOOL). 
*
*     If <instr> contains an in struction for sublists, the function will
*     try to figure out the CULL object definition for the sublists
*     (by calling object_get_subtype) and call itself recursively.
*
*  INPUTS
*     lList **answer_list      - answer list to report errors
*     const lDescr *descr      - object type to analyze
*     const spool_instr *instr - spooing instructions to use
*
*  RESULT
*     spooling_field * - an array of type spooling_field, or 
*                        NULL, if an error occured, error messages are returned
*                        in answer_list
*
*  NOTES
*     The returned spooling_field array has to be freed by the caller of this
*     function using the function spool_free_spooling_fields().
*
*  SEE ALSO
*     gdi/object/object_get_subtype()
*     spool/utilities/spool_free_spooling_fields()
*******************************************************************************/
spooling_field * 
spool_get_fields_to_spool(lList **answer_list, const lDescr *descr, 
                          const spool_instr *instr)
{
   spooling_field *fields;

   DENTER(TOP_LAYER, "spool_get_fields_to_spool");

   SGE_CHECK_POINTER_NULL(descr);

   fields = _spool_get_fields_to_spool(answer_list, descr, instr);

   DEXIT;
   return fields;
}

static spooling_field *
_spool_get_fields_to_spool(lList **answer_list, const lDescr *descr, 
                           const spool_instr *instr)
{
   spooling_field *fields;
   int i, j, size;
   int strip = 0;

   DENTER(TOP_LAYER, "spool_get_fields_to_spool");

   /* we don't check descr and instr, as we know they are ok
    * (it's a static function)
    */

   /* count fields to spool */
   for (i = 0, size = 0; descr[i].mt != lEndT; i++) {
      if ((descr[i].mt & instr->selection) != 0) {
         size++;
      }
   }

   /* allocate memory */
   fields = (spooling_field *)malloc((size + 1) * sizeof(spooling_field));
   if (fields == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_UNABLETOALLOCATEBYTES_DS, 
                              (size * 1) * sizeof(spooling_field), SGE_FUNC);
      DEXIT;
      return NULL;
   }

   /* initialize fields */
   for (i = 0; i < size; i++) {
      fields[i].nm         = NoName;
      fields[i].width      = 0;
      fields[i].name       = NULL;
      fields[i].sub_fields = NULL;
   }

   /* do we have to strip field prefixes, e.g. "QU_" from field names? */
   if (instr->copy_field_names && instr->strip_field_prefix) {
      dstring buffer = DSTRING_INIT;
      const char *prefix = object_get_name_prefix(descr, &buffer);
      strip = sge_strlen(prefix);
      sge_dstring_free(&buffer);
   }

   /* copy field info */
   for (i = 0, j = 0; descr[i].mt != lEndT; i++) {
      if ((descr[i].mt & instr->selection) != 0) {
         spooling_field *sub_fields = NULL;

         fields[j].nm         = descr[i].nm;
         fields[j].width      = 0;
         fields[j].name       = NULL;
         fields[j].sub_fields = NULL;

         if (instr->copy_field_names) {
            const char *name;
            name = lNm2Str(descr[i].nm);
            if(name == NULL || strlen(name) <= strip) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_NONAMEFORATTRIBUTE_D, 
                                       descr[i].nm);
               fields = spool_free_spooling_fields(fields);
               DEXIT;
               return NULL;
            }
            fields[j].name = strdup(name + strip);
            DPRINTF(("field "SFQ" will be spooled\n", fields[j].name));
         }
         
         if (mt_get_type(descr[i].mt) == lListT) {
            const lDescr *sub_descr;

            if (instr->sub_instr == NULL) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR,
                                       MSG_DONTKNOWHOWTOSPOOLSUBLIST_SS,
                                       lNm2Str(descr[i].nm), SGE_FUNC);
               fields = spool_free_spooling_fields(fields);
               DEXIT;
               return NULL;
            }
            
            sub_descr = object_get_subtype(descr[i].nm);
            if (sub_descr == NULL) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR,
                                       MSG_UNKNOWNOBJECTTYPEFOR_SS,
                                       lNm2Str(descr[i].nm), SGE_FUNC);
               fields = spool_free_spooling_fields(fields);
               DEXIT;
               return NULL;
            }

            sub_fields = _spool_get_fields_to_spool(answer_list, sub_descr, 
                                                       instr->sub_instr);
         }

         fields[j++].sub_fields = sub_fields;
      }
   }

   /* end of field array */
   fields[j].nm = NoName;

   DEXIT;
   return fields;
}

/****** spool/utilities/spool_free_spooling_fields() ********************
*  NAME
*     spool_free_spooling_fields() -- free a spooling field array
*
*  SYNOPSIS
*     spooling_field * spool_free_spooling_fields(spooling_field *fields) 
*
*  FUNCTION
*     Frees an array of spooling_field with all sublists and contained strings.
*
*  INPUTS
*     spooling_field *fields - the field array to free
*
*  RESULT
*     spooling_field * - NULL
*
*  EXAMPLE
*     fields = spool_free_spooling_fields(fields);
*******************************************************************************/
spooling_field * 
spool_free_spooling_fields(spooling_field *fields)
{
   if (fields != NULL) {
      int i;
      for (i = 0; fields[i].nm >=0; i++) {
         if (fields[i].sub_fields != NULL) {
            fields[i].sub_fields = spool_free_spooling_fields(fields[i].sub_fields);
         }

         if (fields[i].name != NULL) {
            FREE(fields[i].name);
         }
      }
      FREE(fields);
   }

   return NULL;
}
