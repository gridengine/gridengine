#ifndef __SGE_OBJECT_H
#define __SGE_OBJECT_H
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

#include "cull.h"

#include "sge_dstring.h"

/****** sgeobj/object/--Object-Handling ***************************************
*
*  NAME
*     Object Handling -- utilities for sgeobj object access
*
*  FUNCTION
*     This module provides utility functions for accessing CULL 
*     objects, e.g. getting a string representation for fields, 
*     setting field contents from string representation etc.
*
*  NOTES
*     Functions like attr_mod_double from gdi_utility should also 
*     be moved here.
*
*  SEE ALSO
*     sgeobj/Object/object_has_type()
*     sgeobj/Object/object_get_type()
*     sgeobj/Object/object_get_subtype()
*     sgeobj/Object/object_get_primary_key()
*     sgeobj/Object/object_get_name_prefix()
*     sgeobj/Object/object_append_field_to_dstring()
*     sgeobj/Object/object_parse_field_from_string()
*     sgeobj/Object/object_type_get_master_list()
*     sgeobj/Object/object_type_free_master_list()
*     sgeobj/Object/object_type_get_name()
*     sgeobj/Object/object_type_get_descr()
*     sgeobj/Object/object_type_get_key_nm()
******************************************************************************/

#define NULL_OUT_NONE(ep, nm) \
   if (lGetString(ep, nm) != NULL && \
       strcasecmp(lGetString(ep, nm), "none") == 0) { \
      lSetString(ep, nm, NULL); \
   }

/****** sgeobj/object/--Object-Typedefs ***************************************
*
*  NAME
*     Object-Typedefs -- typedefs for generic object handling
*
*  SYNOPSIS
*     The enumeration sge_object_type defines different object and 
*     message types.
*
*     The following types are defined:
*        SGE_TYPE_ADMINHOST
*        SGE_TYPE_CALENDAR
*        SGE_TYPE_CKPT
*        SGE_TYPE_CONFIG
*        SGE_TYPE_GLOBAL_CONFIG
*        SGE_TYPE_EXECHOST
*        SGE_TYPE_JATASK
*        SGE_TYPE_PETASK
*        SGE_TYPE_JOB
*        SGE_TYPE_JOB_SCHEDD_INFO
*        SGE_TYPE_MANAGER
*        SGE_TYPE_OPERATOR
*        SGE_TYPE_SHARETREE
*        SGE_TYPE_PE
*        SGE_TYPE_PROJECT
*        SGE_TYPE_CQUEUE
*        SGE_TYPE_QINSTANCE
*        SGE_TYPE_SCHEDD_CONF
*        SGE_TYPE_SCHEDD_MONITOR
*        SGE_TYPE_SHUTDOWN
*        SGE_TYPE_QMASTER_GOES_DOWN
*        SGE_TYPE_SUBMITHOST
*        SGE_TYPE_USER
*        SGE_TYPE_USERSET
*        SGE_TYPE_CUSER
*        SGE_TYPE_CENTRY
*
*     If usermapping is enabled, an additional object type is defined:
*        SGE_TYPE_HGROUP
*  
*     The last value defined as obect type is SGE_TYPE_ALL. 
*****************************************************************************/
typedef enum {
   SGE_TYPE_ADMINHOST = 0,
   SGE_TYPE_CALENDAR,
   SGE_TYPE_CKPT,
   SGE_TYPE_CONFIG,
   SGE_TYPE_GLOBAL_CONFIG,
   SGE_TYPE_EXECHOST,
   SGE_TYPE_JATASK,
   SGE_TYPE_PETASK,
   SGE_TYPE_JOB,
   SGE_TYPE_JOB_SCHEDD_INFO,
   SGE_TYPE_MANAGER,
   SGE_TYPE_OPERATOR,
   SGE_TYPE_SHARETREE,
   SGE_TYPE_PE,
   SGE_TYPE_PROJECT,
   SGE_TYPE_CQUEUE,
   SGE_TYPE_QINSTANCE,
   SGE_TYPE_SCHEDD_CONF,
   SGE_TYPE_SCHEDD_MONITOR,
   SGE_TYPE_SHUTDOWN,
   SGE_TYPE_QMASTER_GOES_DOWN,
   SGE_TYPE_SUBMITHOST,
   SGE_TYPE_USER,
   SGE_TYPE_USERSET,
   SGE_TYPE_HGROUP,
   SGE_TYPE_CENTRY,

   /*
    * Don't forget to edit
    *
    *    'mirror_base' in libs/mir/sge_mirror.c
    *    'object_base' in libs/sgeobj/sge_object.c
    *    'table_base' in libs/spool/sge_spooling_database.c
    *
    * if something is changed here!
    */
#ifndef __SGE_NO_USERMAPPING__
   SGE_TYPE_CUSER,
#endif

   SGE_TYPE_ALL            /* must be last entry */
} sge_object_type;

lList **
object_type_get_master_list(const sge_object_type type);

bool 
object_type_commit_master_list(const sge_object_type type, lList **answer_list); 

bool
object_type_free_master_list(const sge_object_type type);

const char *
object_type_get_name(const sge_object_type type);

sge_object_type 
object_name_get_type(const char *name);

const lDescr *
object_type_get_descr(const sge_object_type type);

int
object_type_get_key_nm(const sge_object_type type);


/* JG: TODO: rename to object_has_descr, make function object_has_type 
             and call this function where possible */
bool 
object_has_type(const lListElem *object, const lDescr *descr);

/* JG: TODO: rename to object_get_type_descr, check all calls, if possible pass sge_object_type */
const lDescr *
object_get_type(const lListElem *object);

const lDescr *
object_get_subtype(int nm);

int 
object_get_primary_key(const lDescr *descr);

const char *
object_get_name_prefix(const lDescr *descr, dstring *buffer);

const char *
object_append_field_to_dstring(const lListElem *object, lList **answer_list, 
                               dstring *buffer, const int nm,
                               char string_quotes);
const char *
object_append_raw_field_to_dstring(const lListElem *object, lList **answer_list,
                                   dstring *buffer, const int nm,
                                   char string_quotes);

bool 
object_parse_field_from_string(lListElem *object, lList **answer_list, 
                               const int nm, const char *value);

bool 
object_parse_raw_field_from_string(lListElem *object, lList **answer_list, 
                                   const int nm, const char *value);

void
object_delete_range_id(lListElem *object, lList **answer_list, 
                       const int rnm, const u_long32 id);

int 
object_set_range_id(lListElem *object, int rnm, u_long32 start, u_long32 end,
                            u_long32 step);

bool
object_parse_bool_from_string(lListElem *this_elem, lList **answer_list,
                              int name, const char *string);

bool
object_parse_ulong32_from_string(lListElem *this_elem, lList **answer_list,
                                 int name, const char *string);

bool
object_parse_int_from_string(lListElem *this_elem, lList **answer_list,
                             int name, const char *string);

bool
object_parse_char_from_string(lListElem *this_elem, lList **answer_list,
                             int name, const char *string);

bool
object_parse_long_from_string(lListElem *this_elem, lList **answer_list,
                             int name, const char *string);

bool
object_parse_double_from_string(lListElem *this_elem, lList **answer_list,
                                int name, const char *string);

bool
object_parse_float_from_string(lListElem *this_elem, lList **answer_list,
                               int name, const char *string);

bool
object_parse_time_from_string(lListElem *this_elem, lList **answer_list,
                                 int name, const char *string);

bool
object_parse_mem_from_string(lListElem *this_elem, lList **answer_list,
                             int name, const char *string);

bool
object_parse_inter_from_string(lListElem *this_elem, lList **answer_list,
                               int name, const char *string);

bool
object_parse_list_from_string(lListElem *this_elem, lList **answer_list,
                              int name, const char *string,
                              const lDescr *descr, int nm);

bool
object_parse_celist_from_string(lListElem *this_elem, lList **answer_list,
                                int name, const char *string);

bool
object_parse_solist_from_string(lListElem *this_elem, lList **answer_list,
                                int name, const char *string);

bool
object_parse_qtlist_from_string(lListElem *this_elem, lList **answer_list,
                                int name, const char *string);

bool
object_set_any_type(lListElem *this_elem, int name, void *value);

bool
object_replace_any_type(lListElem *this_elem, int name, lListElem *org_elem);

void
object_get_any_type(lListElem *this_elem, int name, void *value);

bool 
attr_mod_sub_list(lList **alpp, lListElem *this_elem, int this_elem_name,
                  int this_elem_primary_key, lListElem *delta_elem, 
                  int sub_command, const char *sub_list_name, 
                  const char *object_name, int no_info); 

bool  
object_has_differences(lListElem *this_elem, lList **answer_list,
                       lListElem *old_elem, bool modify_changed_flag);

bool
object_list_has_differences(lList *this_elem, lList **answer_list,
                            lList *old_elem, bool modify_changed_flag);

#endif /* __SGE_OBJECT_H */
