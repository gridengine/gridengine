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

#include "sge_answer.h"
#include "sge_object.h"

#include "msg_common.h"
#include "spool/msg_spoollib.h"

#include "spool/sge_spooling_sql.h"

bool
spool_sql_create_insert_statement(lList **answer_list, 
                                  dstring *field_dstring, 
                                  dstring *value_dstring, 
                                  spooling_field *fields, 
                                  const lListElem *object, 
                                  bool *data_written)
{
   bool ret = true;
   bool first_field = true;
   int i;
   const lDescr *descr;

   DENTER(TOP_LAYER, "spool_sql_create_insert_statement");

   *data_written = false;

   descr = lGetElemDescr(object);

   for (i = 0; fields[i].nm != NoName && ret; i++) {
      int pos, type;

      
      pos = lGetPosInDescr(descr, fields[i].nm);
      if (pos < 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_ATTRIBUTENOTINOBJECT_S, 
                                 lNm2Str(fields[i].nm));
         ret = false;
         continue;
      }

      type = mt_get_type(descr[pos].mt);
      if (type != lListT) {
         if (!first_field) {
            sge_dstring_append(field_dstring, ", ");
            sge_dstring_append(value_dstring, ", ");
         }
         if (fields[i].name == NULL) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                    ANSWER_QUALITY_ERROR,
                                    "invalid field name for field "SFQ"\n", 
                                    lNm2Str(fields[i].nm));
            ret = false;
            continue;
         } else {
            sge_dstring_append(field_dstring, fields[i].name);
            object_append_raw_field_to_dstring(object, answer_list, value_dstring, 
                                               fields[i].nm, '\'');
         }
         *data_written = true;
         first_field = false;
      }
   }

   DEXIT;
   return ret;
}

bool
spool_sql_create_update_statement(lList **answer_list, 
                                  dstring *update_dstring, 
                                  spooling_field *fields, 
                                  const lListElem *object,
                                  bool *data_written)
{
   bool ret = true;
   bool first_field = true;
   int i;
   const lDescr *descr;

   DENTER(TOP_LAYER, "spool_sql_create_update_statement");

   *data_written = false;

   descr = lGetElemDescr(object);

   for (i = 0; fields[i].nm != NoName && ret; i++) {
      int pos, type;
      pos = lGetPosInDescr(descr, fields[i].nm);
      if (pos < 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_ATTRIBUTENOTINOBJECT_S, 
                                 lNm2Str(fields[i].nm));
         ret = false;
         continue;
      }

      type = mt_get_type(descr[pos].mt);
      if (type != lListT) {
         if (lListElem_is_pos_changed(object, pos)) {
            if (!first_field) {
               sge_dstring_append(update_dstring, ", ");
            }
            sge_dstring_sprintf_append(update_dstring, "%s = ", fields[i].name);
            object_append_raw_field_to_dstring(object, answer_list, 
                                               update_dstring, fields[i].nm, 
                                               '\'');
            *data_written = true;
            first_field = false;
         }
      }
   }
  
   DEXIT;
   return ret;
}


