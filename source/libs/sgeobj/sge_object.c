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

#include <stdlib.h>

#include "sgermon.h"
#include "sge_log.h"

#include "sge_stdlib.h"
#include "sge_string.h"

#include "sge_all_listsL.h"
#include "sge_answer.h"
#include "sge_queue.h"
#include "sge_range.h"
#include "sge_object.h"

#include "gdi_utility.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"


/****** sgeobj/object/object_has_type() ***************************************
*  NAME
*     object_has_type() -- has an object a certain type?
*
*  SYNOPSIS
*     bool 
*     object_has_type(const lListElem *object, const lDescr *descr) 
*
*  FUNCTION
*     Checks if an object has a certain type.
*     The check is done by looking up the primary key field in descr and
*     checking, if this key field is contained in the given object.
*
*  INPUTS
*     const lListElem *object - object to check
*     const lDescr *descr     - type to check against
*
*  RESULT
*     bool - true, if the object has the given type, else false
*
*  NOTES
*     As looking up the primary key of an object is only implemented
*     for selected types, this function also will work only for these
*     object types.
*
*  SEE ALSO
*     sgeobj/object/object_get_primary_key()
*******************************************************************************/
bool 
object_has_type(const lListElem *object, const lDescr *descr) 
{
   bool ret = false;
 
   /*
    * we assume that "object" is of the given type when the 
    * primary key is contained in the element
    *
    * --> make sure that your object is handled in object_get_primary_key() 
    */
   if (object != NULL &&
       lGetPosInDescr(object->descr, object_get_primary_key(descr)) != -1) {
      ret = true;
   }

   return ret;
} 

/****** sgeobj/object/object_get_type() ***************************************
*  NAME
*     object_get_type() -- return type (descriptor) for object
*
*  SYNOPSIS
*     const lDescr * object_get_type(const lListElem *object) 
*
*  FUNCTION
*     Returns the cull type (descriptor) for a certain object.
*     This descriptor can be different from the objects descriptor,
*     as the objects descriptor can come from an object created from
*     communication, can be a partial descriptor etc.
*
*  INPUTS
*     const lListElem *object - the object to analyze
*
*  RESULT
*     const lDescr * - the object type / descriptor
*
*******************************************************************************/
const lDescr * 
object_get_type(const lListElem *object)
{
   const lDescr *ret = NULL;

   if (object_has_type(object, AH_Type)) {
      ret = AH_Type;
   } else if (object_has_type(object, CAL_Type)) {
      ret = CAL_Type;
   } else if (object_has_type(object, CK_Type)) {
      ret = CK_Type;
   } else if (object_has_type(object, EH_Type)) {
      ret = EH_Type;
   } else if (object_has_type(object, JAT_Type)) {
      ret = JAT_Type;
   } else if (object_has_type(object, JB_Type)) {
      ret = JB_Type;
   } else if (object_has_type(object, PE_Type)) {
      ret = PE_Type;
   } else if (object_has_type(object, PET_Type)) {
      ret = PET_Type;
   } else if (object_has_type(object, QU_Type)) {
      ret = QU_Type;
   } else if (object_has_type(object, QR_Type)) {
      ret = QR_Type;
   } else if (object_has_type(object, RN_Type)) {
      ret = RN_Type;
   } else if (object_has_type(object, SH_Type)) {
      ret = SH_Type;
   } else if (object_has_type(object, VA_Type)) {
      ret = VA_Type;
   }

   return ret;
}

/****** sgeobj/object/object_get_subtype() ************************************
*  NAME
*     object_get_subtype() -- get type of a sublist
*
*  SYNOPSIS
*     const lDescr * object_get_subtype(int nm) 
*
*  FUNCTION
*     returns the data type (descriptor) of a certain sublist.
*
*  INPUTS
*     int nm - name of the sublist
*
*  RESULT
*     const lDescr * - type of the sublist
*
*  NOTES
*     Only partially implemented.
*     The function has to be extended as needed.
*     Better would be to have some global data structure containing this
*     information.
*
*******************************************************************************/
const lDescr * 
object_get_subtype(int nm)
{
   const lDescr *ret = NULL;

   switch(nm) {
      case CK_queue_list:
         ret = QR_Type;
         break;
      case QU_load_thresholds:
      case QU_suspend_thresholds:
         ret = CE_Type;
         break;
      case QU_acl:
      case QU_xacl:
      case QU_owner_list:
         ret = US_Type;
         break;
      case QU_subordinate_list:
         ret = SO_Type;
         break;
      case QU_complex_list:
         ret = CX_Type;
         break;
      case QU_consumable_config_list:
         ret = CE_Type;
         break;
      case QU_projects:
      case QU_xprojects:
         ret = UP_Type;
         break;
   }

   return ret;
}

/****** sgeobj/object/object_get_primary_key() ********************************
*  NAME
*     object_get_primary_key() -- get primary key for object type
*
*  SYNOPSIS
*     int object_get_primary_key(const lDescr *descr) 
*
*  FUNCTION
*     Returns the primary key field for a given object type.
*
*  INPUTS
*     const lDescr *descr - the object type (descriptor)
*
*  RESULT
*     int - name (nm) of the primary key field or 
*           NoName (-1) on error.
*
*  NOTES
*     Only implemented for selected object types.
*     It would be better to have the necessary data in the object description,
*     e.g. via a field property CULL_PRIMARY_KEY.
*
*     Function interface breaks style guide - either we would have to 
*     pass an object, or call it descr_get_primary_key.
*******************************************************************************/
int 
object_get_primary_key(const lDescr *descr)
{
   int ret = NoName;

   if (descr == AH_Type) {
      ret = AH_name;
   } else if (descr == CAL_Type) {
      ret = CAL_name;
   } else if (descr == CK_Type) {
      ret = CK_name;
   } else if (descr == EH_Type) {
      ret = EH_name;
   } else if (descr == JB_Type) {
      ret = JB_job_number;
   } else if (descr == JAT_Type) {
      ret = JAT_task_number;
   } else if (descr == PE_Type) {
      ret = PE_name;
   } else if (descr == PET_Type) {
      ret = PET_id;
   } else if (descr == QU_Type) {
      ret = QU_qname;
   } else if (descr == QR_Type) {
      ret = QR_name;
   } else if (descr == RN_Type) {
      ret = RN_min;
   } else if (descr == SH_Type) {
      ret = SH_name;
   } else if (descr == VA_Type) {
      ret = VA_variable;
   }

   return ret;
}
 
/****** sgeobj/object/object_get_name_prefix() ********************************
*  NAME
*     object_get_name_prefix() -- get prefix of cull attribute name
*
*  SYNOPSIS
*     const char * object_get_name_prefix(const lDescr *descr, dstring *buffer) 
*
*  FUNCTION
*     Returns the prefix that is used in attribute names characterizing the 
*     object type (e.g. "QU_" for the QU_Type).
*
*  INPUTS
*     const lDescr *descr - object type to use
*     dstring *buffer     - buffer that is used to return the result
*
*  RESULT
*     const char * - the prefix or
*                    NULL, if an error occured
*
*  EXAMPLE
*     object_get_name_prefix(QU_Type, buffer) = "QU_"
*     object_get_name_prefix(JB_Type, buffer) = "JB_"
*
*  NOTES
*     The function relies on object_get_primary_key. This function only
*     is implemented for some object types.
*     For types not handled in object_get_primary_key, NULL will be returned.
*
*  SEE ALSO
*     sgeobj/object/object_get_primary_key()
*******************************************************************************/
const char * 
object_get_name_prefix(const lDescr *descr, dstring *buffer)
{
   int nm;

   if (descr == NULL || buffer == NULL) {
      return NULL;
   }

   nm = object_get_primary_key(descr);

   if (nm != NoName) {
      const char *name = lNm2Str(nm);

      if (name != NULL) {
         char *underscore = strchr(name, '_');

         if (underscore != NULL) {
            sge_dstring_sprintf(buffer, "%.*s", underscore - name + 1, name);
            return sge_dstring_get_string(buffer);
         }
      }
   }

   return NULL;
}

/****** sgeobj/object/object_get_field_contents() *****************************
*  NAME
*     object_get_field_contents() -- get object field contents as string
*
*  SYNOPSIS
*     const char *
*     object_get_field_contents(const lListElem *object, lList **answer_list, 
*                               dstring *buffer, const int nm) 
*
*  FUNCTION
*     Returns a string representation of a given object attribute.
*     If errors arrise they are returned in the given answer_list.
*     Data will be created in the given dynamic string buffer.
*
*  INPUTS
*     const lListElem *object - object to use
*     lList **answer_list     - used to return error messages
*     dstring *buffer         - buffer used to format the result
*     const int nm            - attribute to output
*
*  RESULT
*     const char * - string representation of the attribute value (pointer
*                    to the string in the dynamic string buffer, or
*                    NULL if an error occured.
*
*  NOTES
*     For sublists, subobjects and references NULL is returned.
*
*  SEE ALSO
*     sgeobj/object/--GDI-Object-Handling
*     sgeobj/object/object_set_field_contents()
*******************************************************************************/
const char * 
object_get_field_contents(const lListElem *object, lList **answer_list, 
                          dstring *buffer, const int nm)
{
   const char *str;
   const char *result = NULL;
   const lDescr *descr;
   int pos, type;

   DENTER(TOP_LAYER, "object_get_field_contents");

   SGE_CHECK_POINTER_NULL(object);

   descr = lGetElemDescr(object);
   pos = lGetPosViaElem(object, nm);

   if (pos < 0) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, MSG_NMNOTINELEMENT_S, 
                              lNm2Str(nm));
      DEXIT;
      return NULL;
   }

   type = lGetPosType(descr, pos);

   /* handle special cases 
    * these special cases are for instance bitfields that shall be 
    * output as readable strings.
    * Usually such special cases are evidence of bad data base design.
    * Example: QU_qtype is a bitfield of types (BATCH, PARALLEL, ...)
    *          Instead, multiple boolean fields should be created:
    *          QU_batch, QU_parallel, ...
    */
   switch (nm) {
      case QU_qtype:
         result = queue_get_type_string(object, answer_list, buffer);
         DEXIT;
         return result;
   }

   /* read data */
   switch (type) {
      case lFloatT:
         sge_dstring_sprintf(buffer, "%f", lGetPosFloat(object, pos));
         break;
      case lDoubleT:
         sge_dstring_sprintf(buffer, "%lf", lGetPosDouble(object, pos));
         break;
      case lUlongT:
         sge_dstring_sprintf(buffer, U32CFormat, lGetPosUlong(object, pos));
         break;
      case lLongT:
         sge_dstring_sprintf(buffer, "%ld", lGetPosLong(object, pos));
         break;
      case lCharT:
         sge_dstring_sprintf(buffer, "%c", lGetPosChar(object, pos));
         break;
      case lBoolT:
         result = lGetPosBool(object, pos) ? TRUE_STR : FALSE_STR;
         break;
      case lIntT:
         sge_dstring_sprintf(buffer, "%d", lGetPosInt(object, pos));
         break;
      case lStringT:
         str = lGetPosString(object, pos);
         sge_dstring_sprintf(buffer, "%s", str != NULL ? str : NONE_STR);
         break;
      case lHostT:
         str = lGetPosHost(object, pos);
         sge_dstring_sprintf(buffer, "%s", str != NULL ? str : NONE_STR);
         break;
      case lListT:
      case lObjectT:
      case lRefT:
         /* what do to here? */
         break;
      default:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_INVALIDCULLDATATYPE_D, type);
         break;
   }

   switch (type) {
      case lFloatT:
      case lDoubleT:
      case lUlongT:
      case lLongT:
      case lCharT:
      case lIntT:
      case lStringT:
      case lHostT:
         result = sge_dstring_get_string(buffer);
         break;
   }

   DEXIT;
   return result;
}

/****** sgeobj/object/object_set_field_contents() *****************************
*  NAME
*     object_set_field_contents() -- set object attribute contents from string
*
*  SYNOPSIS
*     bool 
*     object_set_field_contents(lListElem *object, lList **answer_list, 
*                               const int nm, const char *value) 
*
*  FUNCTION
*     Sets a new value for a certain object attribute.
*     The new value is passed as parameter in string format.
*
*  INPUTS
*     lListElem *object   - the object to change
*     lList **answer_list - used to return error messages
*     const int nm        - the attribute to change
*     const char *value   - the new value
*
*  RESULT
*     bool - true on success,
*            false, on error, error description in answer_list 
*
*  NOTES
*     Sublists, subobjects and references cannot be set with this function.
*
*  SEE ALSO
*     sgeobj/object/--GDI-Object-Handling
*     sgeobj/object/object_get_field_contents()
******************************************************************************/
bool 
object_set_field_contents(lListElem *object, lList **answer_list, const int nm, 
                          const char *value)
{
   bool ret = true;
   const lDescr *descr;
   int pos, type;

   DENTER(TOP_LAYER, "object_set_field_contents");

   SGE_CHECK_POINTER_FALSE(object);

   descr = lGetElemDescr(object);
   pos = lGetPosViaElem(object, nm);
   if (pos < 0) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, MSG_NMNOTINELEMENT_S, 
                              lNm2Str(nm));
      abort();
   }

   type = lGetPosType(descr, pos);

   /* handle special cases */
   switch (nm) {
      case QU_qtype:
         ret = queue_set_type_string(object, answer_list, value);
         DEXIT;
         return ret; 
   }

   /* read data */
   switch (type) {
      case lFloatT:
         {
            float f;
            if (sscanf(value, "%f", &f) == 1) {
               lSetPosFloat(object, pos, f);
            } else {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_ERRORPARSINGVALUEFORNM_SS,
                                       value, lNm2Str(nm));
               ret = false;
            }
         }
         break;
      case lDoubleT:
         {
            double d;
            if (sscanf(value, "%lf", &d) == 1) {
               lSetPosDouble(object, pos, d);
            } else {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_ERRORPARSINGVALUEFORNM_SS,
                                       value, lNm2Str(nm));
               ret = false;
            }
         }
         break;
      case lUlongT:
         {
            u_long32 l;
            if (sscanf(value, u32, &l) == 1) {
               lSetPosUlong(object, pos, l);
            } else {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_ERRORPARSINGVALUEFORNM_SS,
                                       value, lNm2Str(nm));
               ret = false;
            }
         }
         break;
      case lLongT:
         {
            long l;
            if (sscanf(value, "%ld", &l) == 1) {
               lSetPosLong(object, pos, l);
            } else {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_ERRORPARSINGVALUEFORNM_SS,
                                       value, lNm2Str(nm));
               ret = false;
            }
         }
         break;
      case lCharT:
         {
            char c;
            if (sscanf(value, "%c", &c) == 1) {
               lSetPosChar(object, pos, c);
            } else {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_ERRORPARSINGVALUEFORNM_SS,
                                       value, lNm2Str(nm));
               ret = false;
            }
         }
         break;
      case lBoolT:
         if(value == NULL) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_ERRORPARSINGVALUEFORNM_SS,
                                    "<null>", lNm2Str(nm));
            ret = false;
         }
         if (strncmp(value, TRUE_STR, TRUE_LEN) == 0) {
            lSetPosBool(object, pos, true);
         } else if (strncmp(value, FALSE_STR, FALSE_LEN) == 0) {
            lSetPosBool(object, pos, false);
         } else {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_ERRORPARSINGVALUEFORNM_SS,
                                    value, lNm2Str(nm));
            ret = false;
         }
         break;
      case lIntT:
         {
            int i;
            if (sscanf(value, "%d", &i) == 1) {
               lSetPosInt(object, pos, i);
            } else {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_ERRORPARSINGVALUEFORNM_SS,
                                       value, lNm2Str(nm));
               ret = false;
            }
         }
         break;
      case lStringT:
         lSetPosString(object, pos, value);
         break;
      case lHostT:
         lSetPosHost(object, pos, value);
         break;
      case lListT:
      case lObjectT:
      case lRefT:
         /* what do to here? */
         break;
      default:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_INVALIDCULLDATATYPE_D, type);
         break;
   }

   DEXIT;
   return ret;
}

/****** sgeobj/object/object_delete_range_id() ********************************
*  NAME
*     object_delete_range_id() -- deletes a certain id from an objects range_list
*
*  SYNOPSIS
*     void 
*     object_delete_range_id(lListElem *object, lList **answer_list, 
*                            const int rnm, const u_long32 id) 
*
*  FUNCTION
*     Deletes a certain id from an objects sublist that is a range list.
*
*  INPUTS
*     lListElem *object   - the object to handle
*     lList **answer_list - error messages will be put here
*     const int rnm       - attribute containing the range list
*     const u_long32 id   - id to delete
*
*******************************************************************************/
void 
object_delete_range_id(lListElem *object, lList **answer_list, 
                       const int rnm, const u_long32 id)
{
   lList *range_list = NULL;

   lXchgList(object, rnm, &range_list);
   range_list_remove_id(&range_list, answer_list, id);
   range_list_compress(range_list);
   lXchgList(object, rnm, &range_list);
}

/****** sgeobj/object/object_set_range_id() **********************************
*  NAME
*     object_set_range_id() -- store the initial range ids in "job"
*
*  SYNOPSIS
*     int object_set_range_id(lListElem *job, u_long32 start, 
*                                 u_long32 end, u_long32 step) 
*
*  FUNCTION
*     The function stores the initial range id values ("start", "end" 
*     and "step") in the range list of an object. It should only be used 
*     in functions initializing objects range lists.
*
*  INPUTS
*     lListElem *object - object to handle
*     const int rnm     - attribute containing the range list
*     u_long32 start    - first id 
*     u_long32 end      - last id 
*     u_long32 step     - step size 
*
*  RESULT
*     int - 0 -> OK
*           1 -> no memory
******************************************************************************/
int object_set_range_id(lListElem *object, int rnm, u_long32 start, u_long32 end,
                            u_long32 step)
{
   lListElem *range_elem;  /* RN_Type */
   int ret = 0;
 
   range_elem = lFirst(lGetList(object, rnm));
   if (range_elem == NULL) {
      lList *range_list;
 
      range_elem = lCreateElem(RN_Type);
      range_list = lCreateList("task id range", RN_Type);
      if (range_elem == NULL || range_list == NULL) {
         range_elem = lFreeElem(range_elem);
         range_list = lFreeList(range_list);

         /* No memory */
         ret = 1;
      } else {
         lAppendElem(range_list, range_elem);
         lSetList(object, rnm, range_list);
      }
   }
   if (range_elem != NULL) {
      lSetUlong(range_elem, RN_min, start);
      lSetUlong(range_elem, RN_max, end);
      lSetUlong(range_elem, RN_step, step);
   }
   return ret;
}          

