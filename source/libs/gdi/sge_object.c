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

#include "sge_string.h"

#include "sge_all_listsL.h"
#include "sge_answer.h"
#include "sge_queue.h"

#include "gdi_utility.h"

#include "msg_common.h"
#include "msg_gdilib.h"

#include "sge_object.h"

/****** gdi/object/object_has_type() *******************************************
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
*     gdi/object/object_get_primary_key()
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

/****** gdi/object/object_get_primary_key() ************************************
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

   if (descr == EH_Type) {
      ret = EH_name;
   } else if (descr == AH_Type) {
      ret = AH_name;
   } else if (descr == SH_Type) {
      ret = SH_name;
   } else if (descr == QU_Type) {
      ret = QU_qname;
   } else if (descr == JB_Type) {
      ret = JB_job_number;
   } else if (descr == JAT_Type) {
      ret = JAT_task_number;
   } else if (descr == PET_Type) {
      ret = PET_id;
   } else if (descr == RN_Type) {
      ret = RN_min;
   } else if (descr == PE_Type) {
      ret = PE_name;
   } else if (descr == VA_Type) {
      ret = VA_variable;
   }

   return ret;
}
 
/****** gdi/object/object_get_field_contents() *********************************
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
*     gdi/object/--GDI-Object-Handling
*     gdi/object/object_set_field_contents()
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
         result = lGetPosBool(object, pos) ? "true" : "false";
         break;
      case lIntT:
         sge_dstring_sprintf(buffer, "%d", lGetPosInt(object, pos));
         break;
      case lStringT:
         str = lGetPosString(object, pos);
         sge_dstring_sprintf(buffer, "%s", str != NULL ? str : "none");
         break;
      case lHostT:
         str = lGetPosHost(object, pos);
         sge_dstring_sprintf(buffer, "%s", str != NULL ? str : "none");
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

/****** gdi/object/object_set_field_contents() *********************************
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
*     gdi/object/--GDI-Object-Handling
*     gdi/object/object_get_field_contents()
*******************************************************************************/
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
         if (sge_strnullcasecmp(value, "true") == 0) {
            lSetPosBool(object, pos, true);
         } else if (sge_strnullcasecmp(value, "false") == 0) {
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

