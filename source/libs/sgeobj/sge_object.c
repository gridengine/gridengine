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
#include "sge_calendar.h"
#include "sge_ckpt.h"
#include "sge_conf.h"
#include "sge_host.h"
#include "sge_hgroup.h"
#include "sge_job.h"
#include "sge_ja_task.h"
#include "sge_pe_task.h"
#include "sge_manop.h"
#include "sge_pe.h"
#include "sge_queue.h"
#include "sge_schedd_conf.h"
#include "sge_sharetree.h"
#include "sge_cuser.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_gdi.h"
#include "sge_hostname.h"
#include "sge_answer.h"
#include "sge_range.h"
#include "sge_object.h"
#include "sge_centry.h"
#include "sge_parse_num_par.h"
#include "sge_utility.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#define OBJECT_LAYER TOP_LAYER

/* Datastructure for internal storage of object/message related information */
typedef struct {
   lList **list;                          /* master list                    */
   const char *type_name;                 /* type name, e.g. "JOB"          */
   lDescr *descr;                         /* descriptor, e.g. JB_Type       */
   const int key_nm;                      /* nm of key attribute            */
} object_description;

/* One entry per event type */
static object_description object_base[SGE_TYPE_ALL] = {
   /* master list                    name                 descr      key               */
   { &Master_Adminhost_List,         "ADMINHOST",         AH_Type,   AH_name           },
   { &Master_Calendar_List,          "CALENDAR",          CAL_Type,  CAL_name          },
   { &Master_Ckpt_List,              "CKPT",              CK_Type,   CK_name           },
   { &Master_Config_List,            "CONFIG",            CONF_Type, CONF_hname        },
   { NULL,                           "GLOBAL_CONFIG",     NULL,      NoName            },
   { &Master_Exechost_List,          "EXECHOST",          EH_Type,   EH_name           },
   { NULL,                           "JATASK",            JAT_Type,  JAT_task_number   },
   { NULL,                           "PETASK",            PET_Type,  PET_id            },
   { &Master_Job_List,               "JOB",               JB_Type,   JB_job_number     },
   { &Master_Job_Schedd_Info_List,   "JOB_SCHEDD_INFO",   SME_Type,  NoName            },
   { &Master_Manager_List,           "MANAGER",           MO_Type,   MO_name           },
   { &Master_Operator_List,          "OPERATOR",          MO_Type,   MO_name           },
   { &Master_Sharetree_List,         "SHARETREE",         STN_Type,  STN_name          },
   { &Master_Pe_List,                "PE",                PE_Type,   PE_name           },
   { &Master_Project_List,           "PROJECT",           UP_Type,   UP_name           },
   { &Master_Queue_List,             "QUEUE",             QU_Type,   QU_qname          },
   { &Master_Sched_Config_List,      "SCHEDD_CONF",       SC_Type,   NoName            },
   { NULL,                           "SCHEDD_MONITOR",    NULL,      NoName            },
   { NULL,                           "SHUTDOWN",          NULL,      NoName            },
   { NULL,                           "QMASTER_GOES_DOWN", NULL,      NoName            },
   { &Master_Submithost_List,        "SUBMITHOST",        SH_Type,   SH_name           },
   { &Master_User_List,              "USER",              UP_Type,   UP_name           },
   { &Master_Userset_List,           "USERSET",           US_Type,   US_name           },
   { &Master_HGroup_List,            "HOSTGROUP",         HGRP_Type,  HGRP_name        },
   { &Master_CEntry_List,            "COMPLEX_ENTRY",     CE_Type,    CE_name          },
#ifndef __SGE_NO_USERMAPPING__
   { &Master_Cuser_List,             "USERMAPPING",       CU_Type,  CU_name  }
#endif
};

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
*     It would be better to have the necessary data in the object 
*     description, e.g. via a field property CULL_PRIMARY_KEY.
*
*     Function interface breaks style guide - either we would have to 
*     pass an object, or call it descr_get_primary_key.
*******************************************************************************/
int 
object_get_primary_key(const lDescr *descr)
{
   int ret = NoName;
   int i;

   for (i = 0; descr[i].nm != NoName; i++) {
      if (descr[i].mt & CULL_PRIMARY_KEY) {
         ret = descr[i].nm;
         break;
      }
   }

#if 0
      ret = MR_user; /* JG: TODO: is this really the primary key? */
      ret = PA_origin; /* JG: TODO: this is most probably no primary key! */
#endif

   return ret;
}
 
/****** sgeobj/object/object_get_name_prefix() ********************************
*  NAME
*     object_get_name_prefix() -- get prefix of cull attribute name
*
*  SYNOPSIS
*     const char *
*     object_get_name_prefix(const lDescr *descr, dstring *buffer) 
*
*  FUNCTION
*     Returns the prefix that is used in attribute names characterizing 
*     the object type (e.g. "QU_" for the QU_Type).
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
*     For types not handled in object_get_primary_key, NULL will be 
*     returned.
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

   nm = descr[0].nm;

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

/****** sgeobj/object/object_append_field_to_dstring() ************************
*  NAME
*     object_append_field_to_dstring() -- object field to string
*
*  SYNOPSIS
*     const char *
*     object_append_field_to_dstring(const lListElem *object, 
*                                    lList **answer_list, 
*                                    dstring *buffer, const int nm,
*                                    char string_quotes) 
*
*  FUNCTION
*     Returns a string representation of a given object attribute.
*     If errors arrise they are returned in the given answer_list.
*     Data will be created in the given dynamic string buffer.
*     For some fields a special handling is implemented, e.g. mapping
*     bitfields to string lists.
*
*  INPUTS
*     const lListElem *object - object to use
*     lList **answer_list     - used to return error messages
*     dstring *buffer         - buffer used to format the result
*     const int nm            - attribute to output
*     char string_quotes      - character to be used for string quoting 
*                               '\0' means no quoting
*
*  RESULT
*     const char * - string representation of the attribute value 
*                    (pointer to the string in the dynamic string 
*                    buffer, or NULL if an error occured.
*
*  NOTES
*     For sublists, subobjects and references NULL is returned.
*
*  BUGS
*     For the handled special cases, the dstring is cleared,
*     the default handling appends to the dstring buffer.
*
*  SEE ALSO
*     sgeobj/object/--GDI-object-Handling
*     sgeobj/object/object_parse_field_from_string()
*******************************************************************************/
const char * 
object_append_field_to_dstring(const lListElem *object, lList **answer_list, 
                               dstring *buffer, const int nm, 
                               char string_quotes)
{
   const char *result = NULL;
   char tmp_buf[MAX_STRING_SIZE];
   dstring tmp_dstring;
   bool quote_special_case = false;

   DENTER(OBJECT_LAYER, "object_append_field_to_dstring");

   SGE_CHECK_POINTER_NULL(object);

   /* handle special cases 
    * these special cases are for instance bitfields that shall be 
    * output as readable strings.
    * Usually such special cases are evidence of bad data base design.
    * Example: QU_qtype is a bitfield of types (BATCH, PARALLEL, ...)
    *          Instead, multiple boolean fields should be created:
    *          QU_batch, QU_parallel, ...
    */

   sge_dstring_init(&tmp_dstring, tmp_buf, sizeof(tmp_buf));
    
   switch (nm) {
      case CE_valtype:
         result = map_type2str(lGetUlong(object, nm));
         quote_special_case = true;
         break;
      case CE_relop:
         result = map_op2str(lGetUlong(object, nm));
         quote_special_case = true;
         break;
      case CE_requestable:
         result = map_req2str(lGetUlong(object, nm));
         break;
      case CE_consumable:
         if (lGetBool(object, nm)) {
            result = "YES";
         } else {
            result = "NO";
         }
         quote_special_case = true;
         break;
      case QU_qtype:
         queue_print_qtype_to_dstring(object, &tmp_dstring, false);
         result = sge_dstring_get_string(buffer); 
         quote_special_case = true;
         break;
      case US_type:
         result = userset_get_type_string(object, answer_list, &tmp_dstring);
         quote_special_case = true;
         break;
   }

   /* we had a special case - append to result dstring */
   if (result != NULL) {
      if (quote_special_case && string_quotes != '\0') {
         result = sge_dstring_sprintf_append(buffer, "%c%s%c", string_quotes, 
                                             result, string_quotes);
      } else { 
         result = sge_dstring_append(buffer, result);
      }
   } else {
      result = object_append_raw_field_to_dstring(object, answer_list, buffer,
                                                  nm, string_quotes);
   }

   return result;
}

/****** sgeobj/object/object_append_raw_field_to_dstring() *********************
*  NAME
*     object_append_raw_field_to_dstring() -- object field to string
*
*  SYNOPSIS
*     const char *
*     object_append_raw_field_to_dstring(const lListElem *object, 
*                                        lList **answer_list, 
*                                        dstring *buffer, const int nm,
*                                        char string_quotes) 
*
*  FUNCTION
*     Returns a string representation of a given object attribute.
*     If errors arrise they are returned in the given answer_list.
*     Data will be created in the given dynamic string buffer.
*     For some fields a special handling is implemented, e.g. mapping
*     bitfields to string lists.
*
*  INPUTS
*     const lListElem *object - object to use
*     lList **answer_list     - used to return error messages
*     dstring *buffer         - buffer used to format the result
*     const int nm            - attribute to output
*     char string_quotes      - character to be used for string quoting 
*                               '\0' means no quoting
*
*  RESULT
*     const char * - string representation of the attribute value 
*                    (pointer to the string in the dynamic string 
*                    buffer, or NULL if an error occured.
*
*  NOTES
*     For sublists, subobjects and references NULL is returned.
*
*  BUGS
*     For the handled special cases, the dstring is cleared,
*     the default handling appends to the dstring buffer.
*
*  SEE ALSO
*     sgeobj/object/--GDI-object-Handling
*     sgeobj/object/object_parse_field_from_string()
*******************************************************************************/
const char *
object_append_raw_field_to_dstring(const lListElem *object, lList **answer_list,
                                   dstring *buffer, const int nm,
                                   char string_quotes)
{
   const char *str;
   const char *result = NULL;
   int pos;

   DENTER(OBJECT_LAYER, "object_append_raw_field_to_dstring");

   pos = lGetPosViaElem(object, nm);

   if (pos < 0) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, MSG_NMNOTINELEMENT_S, 
                              lNm2Str(nm));
   } else {
      const lDescr *descr;
      int type;

      descr = lGetElemDescr(object);
      type = lGetPosType(descr, pos);

      /* no special case: read and copy data from object */
      switch (type) {
         case lFloatT:
            result = sge_dstring_sprintf_append(buffer, "%f", lGetPosFloat(object, pos));
            break;
         case lDoubleT:
            result = sge_dstring_sprintf_append(buffer, "%lf", lGetPosDouble(object, pos));
            break;
         case lUlongT:
            result = sge_dstring_sprintf_append(buffer, U32CFormat, lGetPosUlong(object, pos));
            break;
         case lLongT:
            result = sge_dstring_sprintf_append(buffer, "%ld", lGetPosLong(object, pos));
            break;
         case lCharT:
            result = sge_dstring_sprintf_append(buffer, "%c", lGetPosChar(object, pos));
            break;
         case lBoolT:
            result = sge_dstring_sprintf_append(buffer, "%s", 
                                lGetPosBool(object, pos) ? TRUE_STR : FALSE_STR);
            break;
         case lIntT:
            result = sge_dstring_sprintf_append(buffer, "%d", lGetPosInt(object, pos));
            break;
         case lStringT:
            str = lGetPosString(object, pos);
            if (string_quotes != '\0') {
               result = sge_dstring_sprintf_append(buffer, "%c%s%c", 
                                      string_quotes,
                                      str != NULL ? str : NONE_STR,
                                      string_quotes);
            } else {
               result = sge_dstring_sprintf_append(buffer, "%s", 
                                      str != NULL ? str : NONE_STR);
            }
            break;
         case lHostT:
            str = lGetPosHost(object, pos);
            if (string_quotes != '\0') {
               result = sge_dstring_sprintf_append(buffer, "%c%s%c", 
                                      string_quotes,
                                      str != NULL ? str : NONE_STR,
                                      string_quotes);
            } else {
               result = sge_dstring_sprintf_append(buffer, "%s", 
                                      str != NULL ? str : NONE_STR);
            }
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
   }

   DEXIT;
   return result;
}

/****** sgeobj/object/object_parse_field_from_string() ************************
*  NAME
*     object_parse_field_from_string() -- set object attr. from str
*
*  SYNOPSIS
*     bool 
*     object_parse_field_from_string(lListElem *object, 
*                                    lList **answer_list, 
*                                    const int nm, const char *value) 
*
*  FUNCTION
*     Sets a new value for a certain object attribute.
*     The new value is passed as parameter in string format.
*     For some fields a special handling is implemented, e.g. mapping
*     string lists to bitfields.
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
*     Sublists, subobjects and references cannot be set with this 
*     function.
*
*  SEE ALSO
*     sgeobj/object/--GDI-object-Handling
*     sgeobj/object/object_append_field_to_dstring()
******************************************************************************/
bool 
object_parse_field_from_string(lListElem *object, lList **answer_list, 
                               const int nm, const char *value)
{
   bool ret = true;

   DENTER(OBJECT_LAYER, "object_parse_field_from_string");

   SGE_CHECK_POINTER_FALSE(object);

   /* handle special cases */
   switch (nm) {
      case CE_valtype:
         {
            /* JG: TODO: we should better have a function map_str2type 
             * and isn't there some sort of framework for mapping 
             * strings to ints and vice versa? See QU_qtype implementation.
             */
            u_long32 type = 0;
            int i;

            for (i=TYPE_FIRST; !type && i<=TYPE_DOUBLE; i++) {
               if (!strcasecmp(value, map_type2str(i)))  
                  type = i;
            }

            if (type == 0) {
               /* error output necessary? Should be created by parsing function */
               ret = false;
            } else {
               lSetUlong(object, nm, type);
            }
         }
         break;
      case CE_relop:
         {
            u_long32 op = 0;
            int i;

            for (i=TYPE_FIRST; !op && i<=TYPE_DOUBLE; i++) {
               if (!strcasecmp(value, map_op2str(i)))  
                  op = i;
            }

            if (op == 0) {
               /* error output necessary? Should be created by parsing function */
               ret = false;
            } else {
               lSetUlong(object, nm, op);
            }
         }
         break;
      case CE_requestable:
         {
            u_long32 requestable = REQU_NO;

            if (strcasecmp(value, "y") == 0 || strcasecmp(value, "yes") == 0) {
               requestable = REQU_YES;
            } else if (strcasecmp(value, "n") == 0 || strcasecmp(value, "no") == 0) {
               requestable = REQU_NO;
            } else if (strcasecmp(value, "f") == 0 || strcasecmp(value, "forced") == 0) {
               requestable = REQU_FORCED;
            } else {
               ret = false;
            }
            if (ret) {
               lSetBool(object, nm, requestable);
            }
         }
         break;
      case CE_consumable:
         {
            bool cond = false;
            if (strcasecmp(value, "y") == 0 || strcasecmp(value, "yes") == 0) {
               cond = true;
            } else if (strcasecmp(value, "n") == 0 || strcasecmp(value, "no") == 0) {
               ;
            } else {
               ret = false;
            }
            lSetBool(object, nm, cond);
         }
         break;
      case QU_qtype:
         ret = queue_parse_qtype_from_string(object, answer_list, value);
         break;
      case US_type:
         ret = userset_set_type_string(object, answer_list, value);
         break;
      case AMEM_value:
         ret = object_parse_mem_from_string(object, answer_list, nm, value);
         break;
      case ATIME_value:
         ret = object_parse_time_from_string(object, answer_list, nm, value);
         break;
      case AINTER_value:
         ret = object_parse_inter_from_string(object, answer_list, nm, value);
         break;
      default:
         ret = object_parse_raw_field_from_string(object, answer_list, nm, 
                                                  value);
         break;
   }

   DEXIT;
   return ret;
}

/****** sgeobj/object/object_parse_raw_field_from_string() ************************
*  NAME
*     object_parse_raw_field_from_string() -- set object attr. from str
*
*  SYNOPSIS
*     bool 
*     object_parse_raw_field_from_string(lListElem *object, 
*                                        lList **answer_list, 
*                                        const int nm, const char *value) 
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
*     Sublists, subobjects and references cannot be set with this 
*     function.
*
*  SEE ALSO
*     sgeobj/object/--GDI-object-Handling
*     sgeobj/object/object_append_field_to_dstring()
******************************************************************************/
bool 
object_parse_raw_field_from_string(lListElem *object, lList **answer_list, 
                                   const int nm, const char *value)
{
   bool ret = true;
   int pos;

   DENTER(OBJECT_LAYER, "object_parse_raw_field_from_string");

   pos = lGetPosViaElem(object, nm);
   if (pos < 0) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, MSG_NMNOTINELEMENT_S, 
                              lNm2Str(nm));
      ret = false;
   } else {
      const lDescr *descr;
      int type;

      descr = lGetElemDescr(object);
      type = lGetPosType(descr, pos);

      /* read data */
      switch (type) {
         case lFloatT:
            ret = object_parse_float_from_string(object, answer_list, nm, value);
            break;
         case lDoubleT:
            ret = object_parse_double_from_string(object, answer_list, nm, value);
            break;
         case lUlongT:
            ret = object_parse_ulong32_from_string(object, answer_list, nm, value);
            break;
         case lLongT:
            ret = object_parse_long_from_string(object, answer_list, nm, value);
            break;
         case lCharT:
            ret = object_parse_char_from_string(object, answer_list, nm, value);
            break;
         case lBoolT:
            ret = object_parse_bool_from_string(object, answer_list, nm, value);
            break;
         case lIntT:
            ret = object_parse_int_from_string(object, answer_list, nm, value);
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
   }

   DEXIT;
   return ret;
}

/****** sgeobj/object/object_delete_range_id() ********************************
*  NAME
*     object_delete_range_id() -- del certain id from an range_list
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
*
*  NOTES
*     MT-NOTE: object_set_range_id() is MT safe
******************************************************************************/
int 
object_set_range_id(lListElem *object, int rnm, u_long32 start, u_long32 end,
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


/****** sgeobj/object/object_type_get_master_list() **************************
*  NAME
*     object_type_get_master_list() -- get master list for object type
*
*  SYNOPSIS
*     lList** object_type_get_master_list(const sge_object_type type) 
*
*  FUNCTION
*     Returns a pointer to the master list holding objects of the 
*     given type.
*
*  INPUTS
*     const sge_object_type type - the object type 
*
*  RESULT
*     lList** - the corresponding master list, or NULL, if the object 
*               type has no associated master list
*
*  EXAMPLE
*     object_type_get_master_list(SGE_TYPE_JOB) will return a pointer 
*     to the Master_Job_List.
*
*     object_type_get_master_list(SGE_TYPE_SHUTDOWN) will return NULL,
*     as this object type has no associated master list.
*
*  NOTES
*
*  SEE ALSO
*     sgeobj/object/object_type_get_master_list()
*     sgeobj/object/object_type_get_name()
*     sgeobj/object/object_type_get_descr()
*     sgeobj/object/object_type_get_key_nm()
*******************************************************************************/
lList **object_type_get_master_list(const sge_object_type type)
{
   lList **ret = NULL;

   DENTER(OBJECT_LAYER, "object_type_get_master_list");
   if(type < 0 || type >= SGE_TYPE_ALL) {
      ERROR((SGE_EVENT, MSG_OBJECT_INVALID_OBJECT_TYPE_SI, SGE_FUNC, type));
   } else {
      ret = object_base[type].list;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/object/object_type_free_master_list() ***************************
*  NAME
*     object_type_free_master_list() -- free the master list 
*
*  SYNOPSIS
*     bool 
*     object_type_free_master_list(const sge_object_type type) 
*
*  FUNCTION
*     Frees the masterlist for a certain type of objects.
*
*  INPUTS
*     const sge_object_type type - the object type
*
*  RESULT
*     bool - true, if the list existed and could be freed, else false
*
*  NOTES
*
*  SEE ALSO
*     sgeobj/object/--object-Typedefs
*******************************************************************************/
bool object_type_free_master_list(const sge_object_type type)
{
   lList **list;
   bool ret = false;

   DENTER(OBJECT_LAYER, "object_type_free_master_list");
   list = object_type_get_master_list(type);
   if (list != NULL) {
      lFreeList(*list);
      *list = NULL;
      ret = true;
   }
   DEXIT;
   return ret;
}


/****** sgeobj/object/object_type_get_name() *********************************
*  NAME
*     object_type_get_name() -- get a printable name for event type
*
*  SYNOPSIS
*     const char* object_type_get_name(const sge_object_type type) 
*
*  FUNCTION
*     Returns a printable name for an event type.
*
*  INPUTS
*     const sge_object_type type - the event type
*
*  RESULT
*     const char* - string describing the type
*
*  EXAMPLE
*     object_type_get_name(SGE_TYPE_JOB) will return "JOB"
*
*  SEE ALSO
*     sgeobj/object/object_type_get_master_list()
*     sgeobj/object/object_type_get_descr()
*     sgeobj/object/object_type_get_key_nm()
*******************************************************************************/
const char *object_type_get_name(const sge_object_type type)
{
   const char *ret = "unknown";

   DENTER(OBJECT_LAYER, "object_type_get_name");
   if(type < 0 || type > SGE_TYPE_ALL) {
      ERROR((SGE_EVENT, MSG_OBJECT_INVALID_OBJECT_TYPE_SI, SGE_FUNC, type));
   } else if(type == SGE_TYPE_ALL) {
      ret = "default";
   } else {
      ret = object_base[type].type_name;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/object/object_type_get_descr() ********************************
*  NAME
*     object_type_get_descr() -- get the descriptor for an event type
*
*  SYNOPSIS
*     const lDescr* object_type_get_descr(const sge_object_type type) 
*
*  FUNCTION
*     Returns the CULL element descriptor for the object type 
*     associated with the given event.
*
*  INPUTS
*     const sge_object_type type - the event type
*
*  RESULT
*     const lDescr* - the descriptor, or NULL, if no descriptor is 
*                     associated with the type
*
*  EXAMPLE
*     object_type_get_descr(SGE_TYPE_JOB) will return the descriptor 
*        JB_Type,
*     object_type_get_descr(SGE_TYPE_SHUTDOWN) will return NULL
*
*  SEE ALSO
*     sgeobj/object/object_type_get_master_list()
*     sgeobj/object/object_type_get_name()
*     sgeobj/object/object_type_get_key_nm()
*******************************************************************************/
const lDescr *object_type_get_descr(const sge_object_type type)
{
   const lDescr *ret = NULL;

   DENTER(TOP_LAYER, "object_type_get_descr");

   if(type < 0 || type >= SGE_TYPE_ALL) {
      ERROR((SGE_EVENT, MSG_OBJECT_INVALID_OBJECT_TYPE_SI, SGE_FUNC, type));
   } else {
      ret = object_base[type].descr;
   }

   DEXIT;
   return ret;
}

/****** sgeobj/object/object_type_get_key_nm() *******************************
*  NAME
*     object_type_get_key_nm() -- get the primary key attribute for type
*
*  SYNOPSIS
*     int object_type_get_key_nm(const sge_object_type type) 
*
*  FUNCTION
*     Returns the primary key attribute for the object type associated 
*     with the given event type.
*
*  INPUTS
*     const sge_object_type type - event type
*
*  RESULT
*     int - the key number (struct element nm of the descriptor), or
*           -1, if no object type is associated with the event type
*
*  EXAMPLE
*     object_type_get_key_nm(SGE_TYPE_JOB) will return JB_job_number
*     object_type_get_key_nm(SGE_TYPE_SHUTDOWN) will return -1
*
*  SEE ALSO
*     sgeobj/object/object_type_get_master_list()
*     sgeobj/object/object_type_get_name()
*     sgeobj/object/object_type_get_descr()
*******************************************************************************/
int object_type_get_key_nm(const sge_object_type type)
{
   int ret = NoName;

   DENTER(OBJECT_LAYER, "object_type_get_key_nm");
   if(type < 0 || type >= SGE_TYPE_ALL) {
      ERROR((SGE_EVENT, MSG_OBJECT_INVALID_OBJECT_TYPE_SI, SGE_FUNC, type));
   } else {
      ret = object_base[type].key_nm;
   }
   DEXIT;
   return ret;
}

bool
object_parse_bool_from_string(lListElem *this_elem, lList **answer_list,
                              int name, const char *string)
{
   bool ret = true;

   DENTER(OBJECT_LAYER, "object_parse_bool_from_string");
   if (this_elem != NULL && string != NULL) {
      int pos = lGetPosViaElem(this_elem, name);

      if (!strcasecmp(string, "true") || !strcasecmp(string, "t") || 
          !strcmp(string, "1") ||
          !strcasecmp(string, "yes") || !strcasecmp(string, "y")) {
         lSetPosBool(this_elem, pos, true);
      } else if (!strcasecmp(string, "false") || !strcasecmp(string, "f") ||
                 !strcmp(string, "0") ||
                 !strcasecmp(string, "no") || !strcasecmp(string, "n")) {
         lSetPosBool(this_elem, pos, false);
      } else {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_ERRORPARSINGVALUEFORNM_SS,
                                 string, lNm2Str(name));
         ret = false;
      }
   } else {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_ERRORPARSINGVALUEFORNM_SS,
                              "<null>", lNm2Str(name));
      ret = false;
   }
   DEXIT;
   return ret;
}

bool
object_parse_time_from_string(lListElem *this_elem, lList **answer_list,
                              int name, const char *string)
{
   bool ret = true;

   DENTER(OBJECT_LAYER, "object_parse_time_from_string");
   if (this_elem != NULL && string != NULL) {
      int pos = lGetPosViaElem(this_elem, name);

      if (parse_ulong_val(NULL, NULL, TYPE_TIM, string, NULL, 0)) {
         lSetPosString(this_elem, pos, string);
      } else {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_ERRORPARSINGVALUEFORNM_SS,
                                 string, lNm2Str(name));
         ret = false;
      }
   } else {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                              ANSWER_QUALITY_ERROR,
                              MSG_ERRORPARSINGVALUEFORNM_SS,
                              "<null>", lNm2Str(name));
      ret = false;
   }
   DEXIT;
   return ret;
}

bool
object_parse_inter_from_string(lListElem *this_elem, lList **answer_list,
                               int name, const char *string)
{
   bool ret = true;

   DENTER(OBJECT_LAYER, "object_parse_inter_from_string");
   if (this_elem != NULL && string != NULL) {
      int pos = lGetPosViaElem(this_elem, name);

      if (parse_ulong_val(NULL, NULL, TYPE_TIM, string, NULL, 0)) {
         lSetPosString(this_elem, pos, string);
      } else {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_ERRORPARSINGVALUEFORNM_SS,
                                 string, lNm2Str(name));
         ret = false;
      }
   } else {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                              ANSWER_QUALITY_ERROR,
                              MSG_ERRORPARSINGVALUEFORNM_SS,
                              "<null>", lNm2Str(name));
      ret = false;
   }
   DEXIT;
   return ret;
}

bool
object_parse_mem_from_string(lListElem *this_elem, lList **answer_list,
                             int name, const char *string)
{
   bool ret = true;

   DENTER(OBJECT_LAYER, "object_parse_mem_from_string");
   if (this_elem != NULL && string != NULL) {
      int pos = lGetPosViaElem(this_elem, name);

      if (parse_ulong_val(NULL, NULL, TYPE_MEM, string, NULL, 0)) {
         lSetPosString(this_elem, pos, string);
      } else {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_ERRORPARSINGVALUEFORNM_SS,
                                 string, lNm2Str(name));
         ret = false;
      }
   } else {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                              ANSWER_QUALITY_ERROR,
                              MSG_ERRORPARSINGVALUEFORNM_SS,
                              "<null>", lNm2Str(name));
      ret = false;
   }
   DEXIT;
   return ret;
}

bool
object_parse_ulong32_from_string(lListElem *this_elem, lList **answer_list,
                                 int name, const char *string)
{
   bool ret = true;
   
   DENTER(OBJECT_LAYER, "object_parse_ulong32_from_string");
   if (this_elem != NULL && string != NULL) {
      int pos = lGetPosViaElem(this_elem, name);
      u_long32 value;

      if (strlen(string) == 0) {
         /*
          * Empty string will be parsed as '0'
          */
         lSetPosUlong(this_elem, pos, 0);
      } else if (sscanf(string, u32, &value) == 1) {
         lSetPosUlong(this_elem, pos, value);
      } else {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_ERRORPARSINGVALUEFORNM_SS,
                                 string, lNm2Str(name));
         ret = false;
      }
   } else {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_ERRORPARSINGVALUEFORNM_SS,
                              "<null>", lNm2Str(name));
      ret = false;
   }
   DEXIT;
   return ret;
}

bool
object_parse_int_from_string(lListElem *this_elem, lList **answer_list,
                             int name, const char *string)
{
   bool ret = true;

   DENTER(OBJECT_LAYER, "object_parse_int_from_string");
   if (this_elem != NULL && string != NULL) {
      int pos = lGetPosViaElem(this_elem, name);
      int value;

      if (sscanf(string, "%d", &value) == 1) {
         lSetPosInt(this_elem, pos, value);
      } else {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_ERRORPARSINGVALUEFORNM_SS,
                                 string, lNm2Str(name));
         ret = false;
      }
   } else {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_ERRORPARSINGVALUEFORNM_SS,
                              "<null>", lNm2Str(name));
      ret = false;
   }
   DEXIT;
   return ret;
}

bool
object_parse_char_from_string(lListElem *this_elem, lList **answer_list,
                             int name, const char *string)
{
   bool ret = true;

   DENTER(OBJECT_LAYER, "object_parse_char_from_string");
   if (this_elem != NULL && string != NULL) {
      int pos = lGetPosViaElem(this_elem, name);
      char value;

      if (sscanf(string, "%c", &value) == 1) {
         lSetPosChar(this_elem, pos, value);
      } else {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_ERRORPARSINGVALUEFORNM_SS,
                                 string, lNm2Str(name));
         ret = false;
      }
   } else {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_ERRORPARSINGVALUEFORNM_SS,
                              "<null>", lNm2Str(name));
      ret = false;
   }
   DEXIT;
   return ret;
}

bool
object_parse_long_from_string(lListElem *this_elem, lList **answer_list,
                             int name, const char *string)
{
   bool ret = true;

   DENTER(OBJECT_LAYER, "object_parse_long_from_string");
   if (this_elem != NULL && string != NULL) {
      int pos = lGetPosViaElem(this_elem, name);
      long value;

      if (sscanf(string, "%ld", &value) == 1) {
         lSetPosLong(this_elem, pos, value);
      } else {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_ERRORPARSINGVALUEFORNM_SS,
                                 string, lNm2Str(name));
         ret = false;
      }
   } else {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_ERRORPARSINGVALUEFORNM_SS,
                              "<null>", lNm2Str(name));
      ret = false;
   }
   DEXIT;
   return ret;
}

bool
object_parse_double_from_string(lListElem *this_elem, lList **answer_list,
                                int name, const char *string)
{
   bool ret = true;

   DENTER(OBJECT_LAYER, "object_parse_double_from_string");
   if (this_elem != NULL && string != NULL) {
      int pos = lGetPosViaElem(this_elem, name);
      double value;

      if (sscanf(string, "%lf", &value) == 1) {
         lSetPosDouble(this_elem, pos, value);
      } else {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_ERRORPARSINGVALUEFORNM_SS,
                                 string, lNm2Str(name));
         ret = false;
      }
   } else {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_ERRORPARSINGVALUEFORNM_SS,
                              "<null>", lNm2Str(name));
      ret = false;
   }
   DEXIT;
   return ret;
}

bool
object_parse_float_from_string(lListElem *this_elem, lList **answer_list,
                               int name, const char *string)
{
   bool ret = true;

   DENTER(OBJECT_LAYER, "object_parse_float_from_string");
   if (this_elem != NULL && string != NULL) {
      int pos = lGetPosViaElem(this_elem, name);
      float value;

      if (sscanf(string, "%f", &value) == 1) {
         lSetPosFloat(this_elem, pos, value);
      } else {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_ERRORPARSINGVALUEFORNM_SS,
                                 string, lNm2Str(name));
         ret = false;
      }
   } else {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_ERRORPARSINGVALUEFORNM_SS,
                              "<null>", lNm2Str(name));
      ret = false;
   }
   DEXIT;
   return ret;
}

bool
object_set_any_type(lListElem *this_elem, int name, void *value)
{
   int ret = true;
   int pos = lGetPosViaElem(this_elem, name);
   int type = lGetPosType(lGetElemDescr(this_elem), pos);

   DENTER(OBJECT_LAYER, "object_set_any_type");
   if (type == lStringT) {
      ret = lSetPosString(this_elem, pos, *((const char **)value));
   } else if (type == lHostT) {
      ret = lSetPosHost(this_elem, pos, *((const char **)value));
   } else if (type == lUlongT) {
      ret = lSetPosUlong(this_elem, pos, *((lUlong*)value));
   } else if (type == lDoubleT) {
      ret = lSetPosDouble(this_elem, pos, *((lDouble*)value));
   } else if (type == lFloatT) {
      ret = lSetPosFloat(this_elem, pos, *((lFloat*)value));
   } else if (type == lLongT) {
      ret = lSetPosLong(this_elem, pos, *((lLong*)value));
   } else if (type == lCharT) {
      ret = lSetPosChar(this_elem, pos, *((lChar*)value));
   } else if (type == lBoolT) {
      ret = lSetPosBool(this_elem, pos, *((lBool*)value));
   } else if (type == lIntT) {
      ret = lSetPosInt(this_elem, pos, *((int*)value));
   } else if (type == lObjectT) {
      ret = lSetPosObject(this_elem, pos, *((lListElem **)value));
   } else if (type == lRefT) {
      ret = lSetPosRef(this_elem, pos, *((lRef*)value));
   } else {
      /* not possible */
      ret = false;
   }
   DEXIT;
   return ret;
}

void 
object_get_any_type(lListElem *this_elem, int name, void *value)
{
   int pos = lGetPosViaElem(this_elem, name);
   int type = lGetPosType(lGetElemDescr(this_elem), pos);

   DENTER(OBJECT_LAYER, "object_get_any_type");
   if (value != NULL) {
      if (type == lStringT) {
         *((const char **)value) = lGetPosString(this_elem, pos);
      } else if (type == lHostT) {
         *((const char **)value) = lGetPosHost(this_elem, pos);
      } else if (type == lUlongT) {
         *((lUlong*)value) = lGetPosUlong(this_elem, pos);
      } else if (type == lDoubleT) {
         *((lDouble*)value) = lGetPosDouble(this_elem, pos);
      } else if (type == lFloatT) {
         *((lFloat*)value) = lGetPosFloat(this_elem, pos);
      } else if (type == lLongT) {
         *((lLong*)value) = lGetPosLong(this_elem, pos);
      } else if (type == lCharT) {
         *((lChar*)value) = lGetPosChar(this_elem, pos);
      } else if (type == lBoolT) {
         *((lBool*)value) = lGetPosBool(this_elem, pos);
      } else if (type == lIntT) {
         *((int*)value) = lGetPosInt(this_elem, pos);
      } else if (type == lObjectT) {
         *((lListElem **)value) = lGetPosObject(this_elem, pos);
      } else if (type == lRefT) {
         *((lRef *)value) = lGetPosRef(this_elem, pos);
      } else {
         /* not possible */
      }
   }
   DEXIT;
}

void 
attr_mod_sub_list(lList **alpp, lListElem *this_elem, int this_elem_name,
                  int this_elem_primary_key, lListElem *delta_elem,
                  int sub_command, char *sub_list_name, char *object_name,
                  int no_info) 
{
   DENTER(OBJECT_LAYER, "attr_mod_sub_list");

   if (lGetPosViaElem(delta_elem, this_elem_name) < 0) {
      return;
   }

   if (sub_command == SGE_GDI_CHANGE ||
       sub_command == SGE_GDI_APPEND ||
       sub_command == SGE_GDI_REMOVE) {
      lList *reduced_sublist;
      lList *full_sublist;
      lListElem *reduced_element, *next_reduced_element;
      lListElem *full_element, *next_full_element;

      reduced_sublist = lGetList(delta_elem, this_elem_name);
      full_sublist = lGetList(this_elem, this_elem_name);
      next_reduced_element = lFirst(reduced_sublist);
      /*
      ** we try to find each element of the delta_elem
      ** in the sublist if this_elem. Elements which can be found
      ** will be moved into sublist of this_elem.
      */
      while ((reduced_element = next_reduced_element)) {
         int restart_loop = 0;

         next_reduced_element = lNext(reduced_element);
         next_full_element = lFirst(full_sublist);
         while ((full_element = next_full_element)) {
            int pos, type;
            const char *rstring = NULL, *fstring = NULL;

            next_full_element = lNext(full_element);

            pos = lGetPosViaElem(reduced_element, this_elem_primary_key);
            type = lGetPosType(lGetElemDescr(reduced_element), pos);            
            if (type == lStringT) {
               rstring = lGetString(reduced_element, this_elem_primary_key);
               fstring = lGetString(full_element, this_elem_primary_key);
            } else if (type == lHostT) {
               rstring = lGetHost(reduced_element, this_elem_primary_key);
               fstring = lGetHost(full_element, this_elem_primary_key);
            }

            if (!strcmp(rstring, fstring)) {
               lListElem *new_sub_elem;
               lListElem *old_sub_elem;

               next_reduced_element = lNext(reduced_element);
               new_sub_elem =
                  lDechainElem(reduced_sublist, reduced_element);
               old_sub_elem = lDechainElem(full_sublist, full_element);
               if (sub_command == SGE_GDI_CHANGE ||
                   sub_command == SGE_GDI_APPEND) {

                  if (!no_info && sub_command == SGE_GDI_APPEND) {
                     INFO((SGE_EVENT, SFQ" already exists in "SFQ" of "SFQ"\n",
                           rstring, sub_list_name, object_name));
                     answer_list_add(alpp, SGE_EVENT, STATUS_OK, 
                                     ANSWER_QUALITY_INFO);
                  }

                  lFreeElem(old_sub_elem);
                  lAppendElem(full_sublist, new_sub_elem);

                  restart_loop = 1;
                  break;
               } else if (sub_command == SGE_GDI_REMOVE) {

                  lFreeElem(old_sub_elem);
                  lFreeElem(new_sub_elem);

                  restart_loop = 1;
                  break;
               }
            }
         }
         if (restart_loop) {
            next_reduced_element = lFirst(reduced_sublist);
         }
      }
      if (sub_command == SGE_GDI_CHANGE ||
          sub_command == SGE_GDI_APPEND ||
          sub_command == SGE_GDI_REMOVE) {
         next_reduced_element = lFirst(reduced_sublist);
         while ((reduced_element = next_reduced_element)) {
            int pos, type;
            const char *rstring = NULL;
            lListElem *new_sub_elem;

            next_reduced_element = lNext(reduced_element);

            pos = lGetPosViaElem(reduced_element, this_elem_primary_key);
            type = lGetPosType(lGetElemDescr(reduced_element), pos);            
            if (type == lStringT) {
               rstring = lGetString(reduced_element, this_elem_primary_key);
            } else if (type == lHostT) {
               rstring = lGetHost(reduced_element, this_elem_primary_key);
            }

            if (!no_info && sub_command == SGE_GDI_REMOVE) {
               INFO((SGE_EVENT, SFQ" does not exist in "SFQ" of "SFQ"\n",
                     rstring, sub_list_name, object_name));
               answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
            } else {
               if (!full_sublist) {
                  if (!no_info && sub_command == SGE_GDI_CHANGE) {
                     INFO((SGE_EVENT, SFQ" of "SFQ" is empty - "
                        "Adding new element(s).\n",
                        sub_list_name, object_name));
                     answer_list_add(alpp, SGE_EVENT, STATUS_OK, 
                                     ANSWER_QUALITY_INFO);
                  }
                  lSetList(this_elem, this_elem_name, lCopyList("",
                     lGetList(delta_elem, this_elem_name)));
                  full_sublist = lGetList(this_elem, this_elem_name);
                  break;
               } else {
                  if (!no_info && sub_command == SGE_GDI_CHANGE) {
                     INFO((SGE_EVENT, "Unable to find "SFQ" in "SFQ" of "SFQ
                        " - Adding new element.\n", rstring,
                        sub_list_name, object_name));
                     answer_list_add(alpp, SGE_EVENT, STATUS_OK, 
                                     ANSWER_QUALITY_INFO);
                  }
                  new_sub_elem =
                     lDechainElem(reduced_sublist, reduced_element);
                  lAppendElem(full_sublist, new_sub_elem);
               }
            }
         }
      }
   } else if (sub_command == SGE_GDI_SET) {
      /*
      ** Overwrite the complete list
      */
      lSetList(this_elem, this_elem_name, lCopyList("",
         lGetList(delta_elem, this_elem_name)));
   }
   /*
   ** If the list does not contain any elements, we will delete
   ** the list itself
   */
   if (lGetList(this_elem, this_elem_name)
       && !lGetNumberOfElem(lGetList(this_elem, this_elem_name))) {
      lSetList(this_elem, this_elem_name, NULL);
   }
   DEXIT;
}

bool 
object_has_differences(lListElem *this_elem, lList **answer_list,
                       lListElem *old_elem, bool modify_changed_flag)
{
   bool ret = false;

   DENTER(OBJECT_LAYER, "object_has_differences");
   if (this_elem != NULL && old_elem != NULL) {
      lDescr *this_elem_descr = this_elem->descr;
      lDescr *old_elem_descr = old_elem->descr;
      lDescr *tmp_decr1 = NULL;
      lDescr *tmp_decr2 = NULL;

      /*
       * Compare each attribute withen the given elements
       */ 
      for (tmp_decr1 = this_elem_descr, tmp_decr2 = old_elem_descr; 
           tmp_decr1->nm != NoName && tmp_decr2->nm != NoName; 
           tmp_decr1++, tmp_decr2++) {
         int pos = tmp_decr1 - this_elem_descr;
         int type1 = this_elem_descr[pos].mt;
         int type2 = old_elem_descr[pos].mt;
         bool equiv = false;

         /* 
          * Compare name and type
          */
         if (tmp_decr1->nm != tmp_decr2->nm ||
             mt_get_type(type1) != mt_get_type(type2)) {
            DPRINTF(("Attribute "SFQ" of type "SFQ" cannot be compared with"
                     "attribute "SFQ" of type "SFQ".\n", 
                     lNm2Str(tmp_decr1->nm), multitypes[mt_get_type(type1)],
                     lNm2Str(tmp_decr2->nm), multitypes[mt_get_type(type2)]));
            ret = true;
            break;
         }

         /*
          * Compare value of attributes
          */
         switch (mt_get_type(type1)) {
            case lFloatT:
               equiv = (lGetPosFloat(this_elem, pos) == lGetPosFloat(old_elem, pos));
               break;
            case lDoubleT:
               equiv = (lGetPosDouble(this_elem, pos) == lGetPosDouble(old_elem, pos));
               break;
            case lUlongT:
               equiv = (lGetPosUlong(this_elem, pos) == lGetPosUlong(old_elem, pos));
               break;
            case lLongT:
               equiv = (lGetPosLong(this_elem, pos) == lGetPosLong(old_elem, pos));
               break;
            case lCharT:
               equiv = (lGetPosChar(this_elem, pos) == lGetPosChar(old_elem, pos));
               break;
            case lBoolT:
               equiv = (lGetPosBool(this_elem, pos) == lGetPosBool(old_elem, pos));
               break;
            case lIntT:
               equiv = (lGetPosInt(this_elem, pos) == lGetPosInt(old_elem, pos));
               break;
            case lStringT:
               {
                  const char *new_str = lGetPosString(this_elem, pos);
                  const char *old_str = lGetPosString(old_elem, pos);

                  if ((new_str == NULL && old_str != NULL) || 
                      (new_str != NULL && old_str == NULL)) {
                     DTRACE;
                     equiv = false;
                  } else if (new_str == old_str) {
                     equiv = true;
                  } else {
                     equiv = (strcmp(new_str, old_str) == 0);
                  }
               }
               break;
            case lHostT:
               {                                                                                  const char *new_str = lGetPosHost(this_elem, pos);
                  const char *old_str = lGetPosHost(old_elem, pos);

                  if ((new_str == NULL && old_str != NULL) || 
                      (new_str != NULL && old_str == NULL)) {
                     DTRACE;
                     equiv = false;
                  } else if (new_str == old_str) {
                     equiv = true;
                  } else {
                     equiv = (sge_hostcmp(new_str, old_str) == 0);
                  }
               }
               break;
            case lRefT:
               equiv = (lGetPosRef(this_elem, pos) == lGetPosRef(old_elem, pos));
               break;
            case lObjectT:
               {
                  lListElem *new_obj = lGetPosRef(this_elem, pos);
                  lListElem *old_obj = lGetPosRef(old_elem, pos);

                  equiv = object_has_differences(new_obj, answer_list, 
                                                 old_obj, modify_changed_flag);
               }
               break;
            case lListT:
               {
                  lList *new_list = lGetPosList(this_elem, pos);
                  lList *old_list = lGetPosList(old_elem, pos);

                  if (lGetNumberOfElem(new_list) == lGetNumberOfElem(old_list)){
                     lListElem *new_elem;
                     lListElem *old_elem;

                     for(new_elem = lFirst(new_list), old_elem = lFirst(old_list);
                         new_elem != NULL && old_elem != NULL;
                         new_elem = lNext(new_elem), old_elem = lNext(old_elem)) {

                        equiv = object_has_differences(new_elem, answer_list,
                                                       old_elem, modify_changed_flag); 
                        if (!equiv) {
                           DTRACE;
                           break;
                        }
                     }
                  } else {
                     DTRACE;
                     equiv = false;
                  }
               }
               break;
            default:
               DTRACE;
               equiv = false;
               break;
         }

         if (!equiv) {
            DPRINTF(("Attributes "SFQ" of type "SFQ" are not equivalent.\n", 
                     lNm2Str(tmp_decr1->nm), multitypes[mt_get_type(type1)]));
            DTRACE;
            ret = true;
         }
      }

      /*
       * Compare number of attributes within each element
       */
      if (tmp_decr1->nm != tmp_decr2->nm) {
         DPRINTF(("Descriptor size is not equivalent\n"));
         ret = true;
      } 
   }    
   DEXIT;
   return ret;   
}
                   
