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

#include "gdi_utility.h"

#include "msg_common.h"
#include "msg_spoollib.h"

#include "sge_spooling_utilities.h"

const spooling_instruction spool_config_subinstruction = 
{
   CULL_NAMELIST | CULL_TUPLELIST,
   NULL
};

const spooling_instruction spool_config_instruction = 
{
   CULL_SPOOL,
   &spool_config_subinstruction
};


static spooling_field *
_spool_get_fields_to_spool(lList **answer_list, const lListElem *ep, 
                           const lDescr *descr, 
                           const spooling_instruction *instruction);

spooling_field *
spool_get_fields_to_spool(lList **answer_list, const lListElem *ep, 
                          const spooling_instruction *instruction)
{
   const lDescr *descr;
   spooling_field *fields;

   DENTER(TOP_LAYER, "spool_get_fields_to_spool");

   SGE_CHECK_POINTER_NULL(ep);

   descr = lGetElemDescr(ep);

   fields = _spool_get_fields_to_spool(answer_list, ep, descr, instruction);

   DEXIT;
   return fields;
}

/* NOTES 
 *    getting fields of sublists will traverse first elements of sublists.
 *    so if the first element doesn't contain the sublist, but the second
 *    does, spooling may not be done correctly.
 */
static spooling_field *
_spool_get_fields_to_spool(lList **answer_list, const lListElem *ep,
                           const lDescr *descr, 
                           const spooling_instruction *instruction)
{
   spooling_field *fields;
   int i, j, size;

   DENTER(TOP_LAYER, "spool_get_fields_to_spool");

   /* we don't check ep, descr and instruction, as we know they are ok
    * (it's a static function)
    */

   /* count fields to spool */
   for (i = 0, size = 0; descr[i].mt != lEndT; i++) {
      if ((descr[i].mt & instruction->selection) != 0) {
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

   /* copy field names */
   for (i = 0, j = 0; descr[i].mt != lEndT; i++) {
      if ((descr[i].mt & instruction->selection) != 0) {
         spooling_field *sub_fields = NULL;

         fields[j].nm = descr[i].nm;
         fields[j].mt = descr[i].mt;
         fields[j].width = 0;          /* width in char's */
         
         if (mt_get_type(descr[i].mt) == lListT && 
            instruction->sub_instruction != NULL) {
            if (ep != NULL) {
               const lList *sub_list = lGetList(ep, descr[i].nm);

               if (sub_list != NULL) {
                  const lListElem *sub_ep    = lFirst(sub_list);
                  const lDescr    *sub_descr = lGetListDescr(sub_list);
            
                  sub_fields = _spool_get_fields_to_spool(answer_list, sub_ep, 
                                                          sub_descr, 
                                                          instruction->sub_instruction);
               }
            }
         }

         fields[j++].sub_fields = sub_fields;
      }
   }

   /* end of field array */
   fields[j].nm = -1;

   DEXIT;
   return fields;
}

spooling_field *
spool_free_spooling_fields(spooling_field *fields)
{
   if (fields != NULL) {
      int i;
      for (i = 0; fields[i].nm >=0; i++) {
         if (fields[i].sub_fields != NULL) {
            fields[i].sub_fields = spool_free_spooling_fields(fields[i].sub_fields);
         }
      }
      FREE(fields);
   }

   return NULL;
}
