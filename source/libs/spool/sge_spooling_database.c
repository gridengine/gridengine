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
#include "sge_log.h"

#include "sge_feature.h"

#include "sge_answer.h"
#include "sge_object.h"

#include "sge_complex.h"
#include "sge_conf.h"
#include "sge_hgroup.h"
#include "sge_host.h"
#include "sge_job.h"
#include "sge_ja_task.h"
#include "sge_pe_task.h"
#include "sge_pe.h"
#include "sge_queue.h"
#include "sge_schedd_conf.h"
#include "sge_userprj.h"
#include "sge_userset.h"

#include "spool/sge_spooling.h"
#include "spool/sge_spooling_utilities.h"

#include "msg_common.h"
#include "spool/msg_spoollib.h"

#include "spool/sge_spooling_database.h"

/****** spool/database/--Database-Spooling *************************************
*
*  NAME
*     Database Spooling -- spooling to databases
*
*  FUNCTION
*     This module provides data structures and functions useful for the 
*     implementation of spooling methods conformant to the spooling framework, 
*     that spool into a database.
*
*     Database can be SQL databases, file based databased, etc.
*
*  SEE ALSO
*     spool/--Spooling
*     spool/sql/--SQL-Spooling
****************************************************************************
*/
/****** spool/database/-Database-Spooling-Typedefs *****************************
*
*  NAME
*     Typedefs -- type definitions for the database spooling
*
*  SYNOPSIS
*     typedef struct sge_database_info {...} sge_database_info;
*     typedef struct {...} table_description;
*
*  FUNCTION
*     These typedefs are for internal use only!
*     Please use the access functions named under SEE ALSO.
*
*     The sge_database_info structure is used to store information
*     like a database system dependent handle,
*     whether to spool with historical data or not, etc.
*     An instance of sge_database_info is stored in the SPR_clientdata
*     attribute of spooling rules.
*
*     The table_description structure contains information describing
*     the table layout for SGE CULL datatypes, e.g. the names of table columns 
*     holding id's, timestamps for spooling with historical data etc.
*  
*     In addition, it contains information information like the CULL attribute
*     name (nm) for the primary key field,
*     and a mapping table from primary key to database internal id, that is 
*     cached within the spooling module.
*
*     Instances of table_description are stored in the clientdata attribute
*     of the spooling_field structure describing which fields are spooled,
*     in the element with index 0.
*
*     Example for a spooling_field structure including table_description:
*     
*     -> fields[0].nm         EH_name
*                 .name       "EH_name"
*                 .sub_fields NULL
*                 .clientdata table_description.table_name    "sge_exechost"
*                                              .field_name_id "EH__id"
*                                              ...
*                                              .key_nm        EH_name
*                                              .id_list
*     ...
*        fields[5].nm         EH_load_list
*                 .sub_fields spooling_field[0].nm HL_name
*                                              .name "HL_name"
*                                              .clientdata table_description
*
*  SEE ALSO
*     spool/database/spool_database_initialize()
*     spool/database/spool_database_get_handle()
*     spool/database/spool_database_set_handle()
*     spool/database/spool_database_set_history()
*     spool/database/spool_database_get_history()
*     spool/database/spool_database_get_table_name()
*     spool/database/spool_database_get_id_field()
*     spool/database/spool_database_get_parent_id_field()
*     spool/database/spool_database_get_valid_field()
*     spool/database/spool_database_get_created_field()
*     spool/database/spool_database_get_deleted_field()
*     spool/database/spool_database_get_key_nm()
*     spool/database/spool_database_get_fields()
*     spool/database/spool_database_store_id()
*     spool/database/spool_database_get_id()
*     spool/database/spool_database_delete_id()
*     spool/database/spool_database_tag_id()
*     spool/database/spool_database_get_id_list()
*     spool/database/spool_database_object_changed()
****************************************************************************
*/
typedef struct sge_database_info {
   bool with_history;   /* store historical data */
   void *handle;        /* database specific handle or structure */
   spooling_field *fields[SGE_TYPE_ALL];
} sge_database_info;

typedef struct {
   const char *table_name;
   const char *field_name_id;
   const char *field_name_parent_id;
   const char *field_name_valid;
   const char *field_name_created;
   const char *field_name_deleted;
   int key_nm;
   lList *id_list;
} table_description;

static lList **
spool_database_get_field_id_list(const spooling_field *fields);

static bool 
spool_database_assign_table_description(lList **answer_list, spooling_field *fields, const char *table_name, const lDescr *descr, bool sublevel);

static table_description *
spool_database_create_table_description(lList **answer_list, const char *table_name, const char *prefix, int key_nm, bool sublevel);

static const char *
spool_database_get_sub_table_name(const char *prefix, int nm);

#if 0
static bool
spool_database_set_table_description(spooling_field *fields, int nm, table_description *description);
#endif

static table_description table_base[SGE_TYPE_ALL] = {
   { "sge_adminhost", "AH__id", NULL, "AH__valid", "AH__created", "AH_deleted", AH_name },
   { "sge_calendar", NULL , NULL, NULL, NULL, NULL, NoName},
   { "sge_ckpt", NULL , NULL, NULL, NULL, NULL, NoName},
   { "sge_complex", NULL , NULL, NULL, NULL, NULL, NoName},
   { "sge_config", NULL , NULL, NULL, NULL, NULL, NoName},
   { NULL , NULL, NULL, NULL, NULL, NULL, NoName},
   { "sge_exechost", "EH__id" , NULL, "EH__valid", "EH__created", "EH__deleted", EH_name },
   { "sge_jatask", NULL , NULL, NULL, NULL, NULL, NoName},
   { "sge_petask", NULL , NULL, NULL, NULL, NULL, NoName},
   { "sge_job", NULL , NULL, NULL, NULL, NULL, NoName},
   { "sge_job_schedd_info", NULL, NULL , NULL, NULL, NULL, NoName},
   { "sge_manager", NULL , NULL, NULL, NULL, NULL, NoName},
   { "sge_operator", NULL , NULL, NULL, NULL, NULL, NoName},
   { "sge_sharetree", NULL , NULL, NULL, NULL, NULL, NoName},
   { "sge_pe", NULL , NULL, NULL, NULL, NULL, NoName},
   { "sge_project", NULL , NULL, NULL, NULL, NULL, NoName},
   { "sge_queue", NULL , NULL, NULL, NULL, NULL, NoName},
   { "sge_sched_config", NULL, NULL , NULL, NULL, NULL, NoName},
   { NULL , NULL, NULL, NULL, NULL, NULL, NoName},
   { NULL , NULL, NULL, NULL, NULL, NULL, NoName},
   { NULL , NULL, NULL, NULL, NULL, NULL, NoName},
   { "sge_submithost", NULL , NULL, NULL, NULL, NULL, NoName},
   { "sge_user", NULL , NULL, NULL, NULL, NULL, NoName},
   { "sge_userset", NULL , NULL, NULL, NULL, NULL, NoName},
   { "sge_hgroup", NULL , NULL, NULL, NULL, NULL, NoName},
#ifndef __SGE_NO_USERMAPPING__
   { "sge_cuser", NULL , NULL, NULL, NULL, NULL, NoName}
#endif
};

const spool_instr spool_database_sub_instr = {
   CULL_SUBLIST,
   true,
   false,
   &spool_database_sub_instr,
   NULL
};

const spool_instr spool_database_instr = {
   CULL_SPOOL,
   true,
   false,
   &spool_database_sub_instr,
   NULL
};

const spool_instr spool_database_sharetree_instr = {
   CULL_SPOOL,
   true,
   false,
   &spool_database_sharetree_instr,
   NULL
};

const spool_instr spool_database_complex_sub_instr = {
   CULL_SPOOL,
   true,
   false,
   NULL,
   NULL
};

const spool_instr spool_database_complex_instr = {
   CULL_SPOOL,
   true,
   false,
   &spool_database_complex_sub_instr,
   NULL
};

const spool_instr spool_database_userprj_sub_instr = {
   CULL_SUBLIST,
   true,
   false,
   &spool_database_userprj_sub_instr,
   NULL
};

const spool_instr spool_database_project_instr = {
   CULL_SPOOL | CULL_SPOOL_PROJECT,
   true,
   false,
   &spool_database_userprj_sub_instr,
   NULL
};

const spool_instr spool_database_user_instr = {
   CULL_SPOOL | CULL_SPOOL_USER,
   true,
   false,
   &spool_database_userprj_sub_instr,
   NULL
};


/****** spool/database/spool_database_initialize() **********************
*  NAME
*     spool_database_initialize() -- initialize database spooling information 
*
*  SYNOPSIS
*     bool 
*     spool_database_initialize(lList **answer_list, lListElem *rule) 
*
*  FUNCTION
*     Initializes internal information needed for database spooling.
*
*  INPUTS
*     lList **answer_list - to return error messages
*     lListElem *rule     - rule that will hold the created data structures
*
*  RESULT
*     bool - true on success, 
*            else false - error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool 
spool_database_initialize(lList **answer_list, lListElem *rule)
{
   bool ret = true;
   sge_object_type i;
   sge_database_info *info;

   DENTER(TOP_LAYER, "spool_database_initialize");

   info = (sge_database_info *)malloc(sizeof(sge_database_info));
   info->with_history = false;
   info->handle = NULL;
  
   for (i = SGE_TYPE_ADMINHOST; i < SGE_TYPE_ALL && ret; i++) {
      const lDescr *descr = object_type_get_descr(i);

      /* evaluate which fields to spool */
      switch (i) {
         case SGE_TYPE_ADMINHOST:
         case SGE_TYPE_CALENDAR:
         case SGE_TYPE_CKPT:
         case SGE_TYPE_CONFIG:
         case SGE_TYPE_EXECHOST:
         case SGE_TYPE_JOB:
#if 0
         case SGE_TYPE_JATASK:
         case SGE_TYPE_PETASK:
         case SGE_TYPE_JOB_SCHEDD_INFO:
#endif
         case SGE_TYPE_MANAGER:
         case SGE_TYPE_OPERATOR:
         case SGE_TYPE_PE:
         case SGE_TYPE_QUEUE:
         case SGE_TYPE_SCHEDD_CONF:
         case SGE_TYPE_SUBMITHOST:
         case SGE_TYPE_USERSET:
         case SGE_TYPE_HGROUP:
#ifndef __SGE_NO_USERMAPPING__
         case SGE_TYPE_CUSER:
#endif
            info->fields[i] = spool_get_fields_to_spool(answer_list, 
                                                        object_type_get_descr(i),
                                                        &spool_database_instr);
            if (info->fields[i] == NULL) {
               ret = false;
               continue;
            }
            ret = spool_database_assign_table_description(answer_list, info->fields[i], table_base[i].table_name, descr, false);
            break;
         case SGE_TYPE_COMPLEX:
            info->fields[i] = spool_get_fields_to_spool(answer_list, 
                                                        object_type_get_descr(i),
                                                        &spool_database_complex_instr);
            if (info->fields[i] == NULL) {
               ret = false;
               continue;
            }
            ret = spool_database_assign_table_description(answer_list, info->fields[i], table_base[i].table_name, descr, false);
            break;
         case SGE_TYPE_SHARETREE:
            info->fields[i] = spool_get_fields_to_spool(answer_list, 
                                                        object_type_get_descr(i),
                                                        &spool_database_sharetree_instr);
            if (info->fields[i] == NULL) {
               ret = false;
               continue;
            }
            ret = spool_database_assign_table_description(answer_list, info->fields[i], table_base[i].table_name, descr, false);
            break;

         case SGE_TYPE_PROJECT:
            info->fields[i] = spool_get_fields_to_spool(answer_list, 
                                                        object_type_get_descr(i),
                                                        &spool_database_project_instr);
            if (info->fields[i] == NULL) {
               ret = false;
               continue;
            }
            ret = spool_database_assign_table_description(answer_list, info->fields[i], table_base[i].table_name, descr, false);
            break;
         case SGE_TYPE_USER:
            info->fields[i] = spool_get_fields_to_spool(answer_list, 
                                                        object_type_get_descr(i),
                                                        &spool_database_user_instr);
            if (info->fields[i] == NULL) {
               ret = false;
               continue;
            }
            ret = spool_database_assign_table_description(answer_list, info->fields[i], table_base[i].table_name, descr, false);
            break;

         default:
            info->fields[i] = NULL;
            break;
      }
   }

   lSetRef(rule, SPR_clientdata, info);

   DEXIT;
   return ret;
}

bool 
spool_database_check_version(lList **answer_list, const char *version)
{
   bool ret = true;

   char buffer[256];
   dstring ds;
   const char *my_version;

   sge_dstring_init(&ds, buffer, sizeof(buffer));
   my_version = feature_get_product_name(FS_SHORT_VERSION, &ds);

   if (strcmp(version, my_version) != 0) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_POSTGRES_WRONGVERSION_SS, 
                              version, my_version);
      ret = false;
   }

   return ret;
}

/****** spool/database/spool_database_get_handle() **********************
*  NAME
*     spool_database_get_handle() -- get database handle 
*
*  SYNOPSIS
*     void * 
*     spool_database_get_handle(const lListElem *rule) 
*
*  FUNCTION
*     Returns the database handle associated with a rule.
*     Database handle is some database specific pointer that is used to 
*     address the database.
*
*  INPUTS
*     const lListElem *rule - the rule from which to read the database handle
*
*  RESULT
*     void * - pointer to the database handle, or NULL on error
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
void *
spool_database_get_handle(const lListElem *rule)
{
   sge_database_info *info;
   
   info = (sge_database_info *)lGetRef(rule, SPR_clientdata);
   return info->handle;
}

/****** spool/database/spool_database_set_handle() **********************
*  NAME
*     spool_database_set_handle() -- set database handle
*
*  SYNOPSIS
*     bool 
*     spool_database_set_handle(const lListElem *rule, void *handle) 
*
*  FUNCTION
*     Stores a database specific handle (pointer) in a certain rule.
*
*  INPUTS
*     const lListElem *rule - the rule to use
*     void *handle          - the handle to store
*
*  RESULT
*     bool - true on success, else false
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool 
spool_database_set_handle(const lListElem *rule, void *handle)
{
   bool ret = true;
   sge_database_info *info;
   
   info = (sge_database_info *)lGetRef(rule, SPR_clientdata);
   info->handle = handle;

   return ret;
}

/****** spool/database/spool_database_set_history() *********************
*  NAME
*     spool_database_set_history() -- set history information
*
*  SYNOPSIS
*     void spool_database_set_history(const lListElem *rule, bool value) 
*
*  FUNCTION
*     Sets for a certain rule the information, whether spooling shall be done
*     with or without historical data.
*
*  INPUTS
*     const lListElem *rule - the rule to use
*     bool value            - true  = spooling with history,
*                             false = spooling without history
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
void 
spool_database_set_history(const lListElem *rule, bool value)
{
   sge_database_info *info = (sge_database_info *)lGetRef(rule, SPR_clientdata);
   info->with_history = value;
}

/****** spool/database/spool_database_get_history() *********************
*  NAME
*     spool_database_get_history() -- get history information
*
*  SYNOPSIS
*     bool 
*     spool_database_get_history(const lListElem *rule) 
*
*  FUNCTION
*     Returns the information, whether spooling shall be done with or without
*     historical information.
*
*  INPUTS
*     const lListElem *rule - the rule to read from
*
*  RESULT
*     bool - true:  spool with historical data
*            false: don't spool historical data
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool
spool_database_get_history(const lListElem *rule)
{
   sge_database_info *info = (sge_database_info *)lGetRef(rule, SPR_clientdata);
   return info->with_history;
}

/****** spool/database/spool_database_get_table_name() ******************
*  NAME
*     spool_database_get_table_name() -- get the database table name 
*
*  SYNOPSIS
*     const char* spool_database_get_table_name(const spooling_field *fields) 
*
*  FUNCTION
*     Returns the name of the database table used for spooling data of the type
*     described in the parameter fields.
*
*  INPUTS
*     const spooling_field *fields - structure containing spooling information
*
*  RESULT
*     const char* - name of the database table
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
const char *
spool_database_get_table_name(const spooling_field *fields)
{
   table_description *tdescr = (table_description *)fields[0].clientdata;
   return tdescr->table_name;
}

/****** spool/database/spool_database_get_id_field() ********************
*  NAME
*     spool_database_get_id_field() -- return name of the id field
*
*  SYNOPSIS
*     const char* 
*     spool_database_get_id_field(const spooling_field *fields) 
*
*  FUNCTION
*     Returns the name of the database field holding the internal record
*     identifier.
*     This id field is used to reference parent objects in related tables.
*
*  INPUTS
*     const spooling_field *fields - structure containing spooling information
*
*  RESULT
*     const char* - name of the id field
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
const char *
spool_database_get_id_field(const spooling_field *fields)
{
   table_description *tdescr = (table_description *)fields[0].clientdata;
   return tdescr->field_name_id;
}

/****** spool/database/spool_database_get_parent_id_field() *************
*  NAME
*     spool_database_get_parent_id_field() -- return name of parent id field
*
*  SYNOPSIS
*     const char* 
*     spool_database_get_parent_id_field(const spooling_field *fields) 
*
*  FUNCTION
*     Returns the name of the database field holding the internal record
*     identifier of parent objects.
*
*  INPUTS
*     const spooling_field *fields - structure containing spooling information
*
*  RESULT
*     const char* - name of the parent id field
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
const char *
spool_database_get_parent_id_field(const spooling_field *fields)
{
   table_description *tdescr = (table_description *)fields[0].clientdata;
   return tdescr->field_name_parent_id;
}

/****** spool/database/spool_database_get_valid_field() *****************
*  NAME
*     spool_database_get_valid_field() -- return name of valid field
*
*  SYNOPSIS
*     const char* 
*     spool_database_get_valid_field(const spooling_field *fields) 
*
*  FUNCTION
*     Returns the name of the database field holding the "valid" information.
*     This boolean field informs about validity of a record in case of spooling
*     with historical information.
*
*  INPUTS
*     const spooling_field *fields - structure containing spooling information
*
*  RESULT
*     const char* - name of the valid field
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
const char *
spool_database_get_valid_field(const spooling_field *fields)
{
   table_description *tdescr = (table_description *)fields[0].clientdata;
   return tdescr->field_name_valid;
}

/****** spool/database/spool_database_get_created_field() ***************
*  NAME
*     spool_database_get_created_field() -- return name of created field
*
*  SYNOPSIS
*     const char* 
*     spool_database_get_created_field(const spooling_field *fields) 
*
*  FUNCTION
*     Returns the name of the database field holding the "created" information.
*     This timestamp field holds the information, when a certain record was 
*     created or modified for the last time.
*
*  INPUTS
*     const spooling_field *fields - structure containing spooling information
*
*  RESULT
*     const char* - name of the created field
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
const char *
spool_database_get_created_field(const spooling_field *fields)
{
   table_description *tdescr = (table_description *)fields[0].clientdata;
   return tdescr->field_name_created;
}

/****** spool/database/spool_database_get_deleted_field() ***************
*  NAME
*     spool_database_get_deleted_field() -- return name of deleted field
*
*  SYNOPSIS
*     const char* 
*     spool_database_get_deleted_field(const spooling_field *fields) 
*
*  FUNCTION
*     Returns the name of the database field holding the "deleted" information.
*     This timestamp field holds the information, when a certain record was 
*     deleted (only in case of spooling with historical information).
*
*  INPUTS
*     const spooling_field *fields - structure containing spooling information
*
*  RESULT
*     const char* - name of the deleted field
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
const char *
spool_database_get_deleted_field(const spooling_field *fields)
{
   table_description *tdescr = (table_description *)fields[0].clientdata;
   return tdescr->field_name_deleted;
}

/****** spool/database/spool_database_get_key_nm() **********************
*  NAME
*     spool_database_get_key_nm() -- return primary key field 
*
*  SYNOPSIS
*     int 
*     spool_database_get_key_nm(const spooling_field *fields) 
*
*  FUNCTION
*     Returns the nm (CULL attribute identifier) of the primary key attribute
*     for the datatype represented by the given fields structure.
*
*  INPUTS
*     const spooling_field *fields - structure containing spooling information
*
*  RESULT
*     int - the nm of the primary key attribute
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int 
spool_database_get_key_nm(const spooling_field *fields)
{
   table_description *tdescr = (table_description *)fields[0].clientdata;
   return tdescr->key_nm;
}

/****** spool/database/spool_database_get_field_id_list() ***************
*  NAME
*     spool_database_get_field_id_list() -- get key->id mapping list
*
*  SYNOPSIS
*     static lList ** 
*     spool_database_get_field_id_list(const spooling_field *fields) 
*
*  FUNCTION
*     Returns the key->id mapping list for a certain spooled object type
*     described in the fields structure.
*
*  INPUTS
*     const spooling_field *fields - structure containing spooling information
*
*  RESULT
*     static lList ** - pointer to the key->id mapping list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static lList **
spool_database_get_field_id_list(const spooling_field *fields)
{
   table_description *tdescr = (table_description *)fields[0].clientdata;
   return &tdescr->id_list;
}

/****** spool/database/spool_database_get_fields() **********************
*  NAME
*     spool_database_get_fields() -- return field information for object type
*
*  SYNOPSIS
*     spooling_field * 
*     spool_database_get_fields(const lListElem *rule, sge_object_type type) 
*
*  FUNCTION
*     Returns the spooling information for a certain object type.
*
*  INPUTS
*     const lListElem *rule - the rule for the spooling method
*     sge_object_type type  - object type 
*
*  RESULT
*     spooling_field * - spooling information
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
spooling_field *
spool_database_get_fields(const lListElem *rule, sge_object_type type)
{
   sge_database_info *info = (sge_database_info *)lGetRef(rule, SPR_clientdata);
   return info->fields[type];
}

spooling_field *
spool_database_get_sub_fields(spooling_field *fields, int nm)
{
   spooling_field *ret = NULL;

   if (fields != NULL) {
      int i;

      for (i = 0; fields[i].nm != NoName; i++) {
         if (fields[i].nm == nm) {
            ret = fields[i].sub_fields;
            break;
         }
      }
   }

   return ret;
}

/****** spool/database/spool_database_store_id() ************************
*  NAME
*     spool_database_store_id() -- store an id in the key->id mapping
*
*  SYNOPSIS
*     bool 
*     spool_database_store_id(lList **answer_list, const spooling_field *fields,
*                             const char *parent_key, const char *key, 
*                             const char *id, bool tag) 
*
*  FUNCTION
*     Stores an id in the key->id mapping for the given object type (fields).
*     If tag is set to true, the created object is tagged for later analyis.
*
*  INPUTS
*     lList **answer_list          - to return error messages
*     const spooling_field *fields - structure containing spooling information
*     const char *parent_key       - key of a parent object, may be NULL
*     const char *key              - key to store
*     const char *id               - id to store
*     bool tag                     - shall the mapping entry be tagged?
*
*  RESULT
*     bool - true on success, 
*            else false - error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool 
spool_database_store_id(lList **answer_list, const spooling_field *fields, 
                        const char *parent_key, const char *key, 
                        const char *id, bool tag)
{
   bool ret = true;
   lList **id_list;
   lListElem *ep;
  
   /* get id list for this field */
   id_list = spool_database_get_field_id_list(fields);

   /* if we have a parent key: search or create it and create ep in sublist
    * else create ep in top level list id_list
    */
   if (parent_key != NULL) {
      lListElem *parent_ep;

      parent_ep = lGetElemStr(*id_list, SPM_key, parent_key);
      if (parent_ep == NULL) {
         parent_ep = lAddElemStr(id_list, SPM_key, parent_key, SPM_Type);
      }
      ep = lAddSubStr(parent_ep, SPM_key, key, SPM_sublist, SPM_Type);
   } else {
      ep = lAddElemStr(id_list, SPM_key, key, SPM_Type);
   }

   /* finish ep */
   lSetString(ep, SPM_id, id);
   if (tag) {
      lSetBool(ep, SPM_tag, true);
   }

   return ret;
}

/****** spool/database/spool_database_get_id() **************************
*  NAME
*     spool_database_get_id() -- get the id for a certain key
*
*  SYNOPSIS
*     const char * 
*     spool_database_get_id(lList **answer_list, const spooling_field *fields, 
*                           const char *parent_key, const char *key, bool tag) 
*
*  FUNCTION
*     Searches the key->id mapping for the given object type (fields) for the
*     given key.
*     If the key is found, the id is returned.
*     If tag = true, the mapping entry is tagged for later analyis.
*
*  INPUTS
*     lList **answer_list          - to return error messages
*     const spooling_field *fields - structure containing spooling information
*     const char *parent_key       - key of a parent object, may be NULL
*     const char *key              - key
*     bool tag                     - shall the mapping entry be tagged?
*
*  RESULT
*     const char * - the id, if the given key was found,
*                    else NULL.
*                    If errors occured, NULL is returned and error messages
*                    are appended to answer_list.
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
const char *
spool_database_get_id(lList **answer_list, const spooling_field *fields, 
                      const char *parent_key, const char *key, bool tag)
{
   const char *id = NULL;
   lList **id_list;
   
   /* get id list for this field */
   id_list = spool_database_get_field_id_list(fields);
   if (*id_list != NULL) {
      lListElem *ep = NULL;

      /* if we have a parent_key, search key in parent's sublist 
       * else search it in the toplevel list id_list
       */
      if (parent_key != NULL) {
         lListElem *parent_ep = lGetElemStr(*id_list, SPM_key, parent_key);
         if (parent_ep != NULL) {
            ep = lGetSubStr(parent_ep, SPM_key, key, SPM_sublist);
         }
      } else {
         ep = lGetElemStr(*id_list, SPM_key, key);
      }

      /* if we found an entry, read id and optionally tag the entry */
      if (ep != NULL) {
         id = lGetString(ep, SPM_id);

         if (tag) {
            lSetBool(ep, SPM_tag, true);
         }
      }
   }

   return id;
}

/****** spool/database/spool_database_delete_id() ***********************
*  NAME
*     spool_database_delete_id() -- delete a key->id mapping entry
*
*  SYNOPSIS
*     bool 
*     spool_database_delete_id(lList **answer_list, 
*                              const spooling_field *fields, 
*                              const char *parent_key, const char *key) 
*
*  FUNCTION
*     Deletes a certain key from the key-id mapping.
*
*  INPUTS
*     lList **answer_list          - to return error messages
*     const spooling_field *fields - structure containing spooling information
*     const char *parent_key       - key of a parent object
*     const char *key              - key
*
*  RESULT
*     bool - true on success, 
*            else false - error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool
spool_database_delete_id(lList **answer_list, 
                         const spooling_field *fields, 
                         const char *parent_key, const char *key)
{
   bool ret = false;
   lList **id_list;
   
   /* get id list for this field */
   id_list = spool_database_get_field_id_list(fields);

   if (*id_list != NULL) {
      /* if we have a parent_key:
       *    if no key is specified: delete parent_key
       *    else delete key entry in parent's sublist
       * else delete key in toplevel list id_list
       */
  
      if (parent_key != NULL) {
         lListElem *parent_ep = lGetElemStr(*id_list, SPM_key, parent_key);
         if (parent_ep != NULL) {
            if (key == NULL) {
               lRemoveElem(*id_list, parent_ep);
               ret = true;
            } else {
               if (lDelSubStr(parent_ep, SPM_key, key, SPM_sublist)) {
                  ret = true;
               }
            }
         }
      } else {
         /* don't use lDelElemStr - we don't write back id_list! */
         lListElem *ep = lGetElemStr(*id_list, SPM_key, key);
         if (ep != NULL) {
            lRemoveElem(*id_list, ep);
            ret = true;
         }
      }
   }

   return ret;
}

/****** spool/database/spool_database_get_id_list() *********************
*  NAME
*     spool_database_get_id_list() -- get id list for a parent key
*
*  SYNOPSIS
*     lList * 
*     spool_database_get_id_list(lList **answer_list, 
*                                const spooling_field *fields, 
*                                const char *parent_key) 
*
*  FUNCTION
*     Returns the list of keys that are stored for a certain parent key.
*
*  INPUTS
*     lList **answer_list          - to return error messages
*     const spooling_field *fields - structure containing spooling information
*     const char *parent_key       - the parent key
*
*  RESULT
*     lList * - a list of keys stored for the parent_key.
*               NULL, if no information was found or an error occured.
*               In case of an error, error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
lList *
spool_database_get_id_list(lList **answer_list, 
                           const spooling_field *fields, 
                           const char *parent_key)
{
   lList *ret = NULL;
   lList **id_list;
   
   /* get id list for this field */
   id_list = spool_database_get_field_id_list(fields);

   if (*id_list != NULL) {
      if (parent_key != NULL) {
         lListElem *parent_ep = lGetElemStr(*id_list, SPM_key, parent_key);
         if (parent_ep != NULL) {
            ret = lGetList(parent_ep, SPM_sublist);
         }
      } else {
         ret = *id_list;
      }
   }

   return ret;
}

/****** spool/database/spool_database_tag_id() **************************
*  NAME
*     spool_database_tag_id() -- tag an entry in the key->id mapping
*
*  SYNOPSIS
*     bool 
*     spool_database_tag_id(lList **answer_list, const spooling_field *fields, 
*                           const char *parent_key, const char *key, bool value)
*
*  FUNCTION
*     Sets the tag of the specified key->id mapping entries to value.
*
*  INPUTS
*     lList **answer_list          - to return error messages
*     const spooling_field *fields - structure containing spooling information
*     const char *parent_key       - parent key
*     const char *key              - key
*     bool value                   - the new value for the tag field
*
*  RESULT
*     bool - true on success, 
*            else false - error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool 
spool_database_tag_id(lList **answer_list, const spooling_field *fields, 
                      const char *parent_key, const char *key, bool value)
{
   bool ret = false;
   lList **id_list;
   
   /* get id list for this field */
   id_list = spool_database_get_field_id_list(fields);
   if (*id_list != NULL) {
      /* if we have a parent_key
       *    if no key is given: tag all children of parent_key
       *    else tag ep given by key in parent's sublist
       * else search element given by key in id_list and tag it
       */
      if (parent_key != NULL) {
         lListElem *parent_ep = lGetElemStr(*id_list, SPM_key, parent_key);
         if (parent_ep != NULL) {
            if (key == NULL) {
               lListElem *ep;
               for_each(ep, lGetList(parent_ep, SPM_sublist)) {
                  lSetBool(ep, SPM_tag, value);
                  ret = true;
               }
            } else {
               lListElem *ep = lGetSubStr(parent_ep, SPM_key, key, SPM_sublist);
               if (ep != NULL) {
                  lSetBool(ep, SPM_tag, value);
                  ret = true;
               }
            }
         }
      } else {
         lListElem *ep = lGetElemStr(*id_list, SPM_key, key);
         if (ep != NULL) {
            lSetBool(ep, SPM_tag, value);
            ret = true;
         }
      }
   }

   return ret;
}

/****** spool/database/spool_database_object_changed() ******************
*  NAME
*     spool_database_object_changed() -- check if an object changed
*
*  SYNOPSIS
*     bool 
*     spool_database_object_changed(lList **answer_list, 
*                                   const lListElem *object, 
*                                   const spooling_field *fields) 
*
*  FUNCTION
*     Checks, if an object changed.
*     The information is gained by looking at the changed bits stored in a 
*     CULL object.
*     Only the fields that will be spooled will be considered.
*     Sublists will NOT be considered.
*
*  INPUTS
*     lList **answer_list          - to return error messages
*     const lListElem *object      - the object to analyze
*     const spooling_field *fields - structure containing spooling information
*
*  RESULT
*     bool - true, if the object has changed, else false
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool 
spool_database_object_changed(lList **answer_list, const lListElem *object, 
                              const spooling_field *fields)
{
   bool ret = false;
   int i;
   const lDescr *descr;

   descr = lGetElemDescr(object);
   for (i = 0; fields[i].nm != NoName; i++) {
      int pos, type;

      pos = lGetPosInDescr(descr, fields[i].nm);
      if (pos < 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_ATTRIBUTENOTINOBJECT_S, 
                                 lNm2Str(fields[i].nm));
         continue;
      }

      type = mt_get_type(descr[pos].mt);
      if (type != lListT) {
         if (lListElem_is_pos_changed(object, pos)) {
            ret = true;
            break;
         }
      }
   }

   return ret;
}

/****** spool/database/spool_database_create_table_description() ********
*  NAME
*     spool_database_create_table_description() -- create spooling information
*
*  SYNOPSIS
*     static table_description * 
*     spool_database_create_table_description(const char *table_name, 
*                                             const char *field_name_id, 
*                                             const char *field_name_parent_id, 
*                                             const char *field_name_valid, 
*                                             const char *field_name_created, 
*                                             const char *field_name_deleted, 
*                                             int key_nm) 
*
*  FUNCTION
*     Creates a structure containing information necessary for database 
*     spooling, containing for certain object types information like the name 
*     of the database table to use, the name of certain fields within this 
*     table etc.
*
*  INPUTS
*     const char *table_name           - the table name
*     const char *field_name_id        - the name of the id field
*     const char *field_name_parent_id - the name of the parent id field
*     const char *field_name_valid     - the name of the valid field
*     const char *field_name_created   - the name of the created field
*     const char *field_name_deleted   - the name of the deleted field
*     int key_nm                       - the CULL nm of the primary key field
*
*  RESULT
*     static table_description * - the initialized information record
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static table_description *
spool_database_create_table_description(lList **answer_list, const char *table_name, const char *prefix, int key_nm, bool sublevel)
{
   table_description *description;
  
   DENTER(TOP_LAYER, "spool_database_create_table_description");
 
   description = (table_description *)malloc(sizeof(table_description));
   if (description == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                              ANSWER_QUALITY_ERROR,
                              MSG_UNABLETOALLOCATEBYTES_DS,
                              sizeof(table_description), SGE_FUNC);
   } else {
      dstring name_dstring;
      char name_buffer[30];

      sge_dstring_init(&name_dstring, name_buffer, sizeof(name_buffer));

      description->table_name           = table_name;
      description->field_name_id = strdup(sge_dstring_sprintf(&name_dstring, "%s%s", prefix, "_id"));
      if (sublevel) {
         description->field_name_parent_id = strdup(sge_dstring_sprintf(&name_dstring, "%s%s", prefix, "_parent"));
      } else {
         description->field_name_parent_id = NULL;
      }
      description->field_name_created = strdup(sge_dstring_sprintf(&name_dstring, "%s%s", prefix, "_created"));
      description->field_name_valid = strdup(sge_dstring_sprintf(&name_dstring, "%s%s", prefix, "_valid"));
      description->field_name_deleted = strdup(sge_dstring_sprintf(&name_dstring, "%s%s", prefix, "_deleted"));
      description->key_nm               = key_nm;
      description->id_list              = NULL;
   }

   DEXIT;
   return description;
}

#if 0
/****** spool/database/spool_database_set_table_description() ***********
*  NAME
*     spool_database_set_table_description() -- set table description for nm 
*
*  SYNOPSIS
*     static bool 
*     spool_database_set_table_description(spooling_field *fields, int nm, 
*                                          table_description *description) 
*
*  FUNCTION
*     Sets the table description for a certain field (sublist) in the given
*     field structure.
*
*  INPUTS
*     spooling_field *fields         - structure containing spooling information
*     int nm                         - the field (sublist) to change
*     table_description *description - the structure containing information for
*                                      the specified sublist
*
*  RESULT
*     bool - true on success, 
*            else false
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool
spool_database_set_table_description(spooling_field *fields, int nm, 
                                     table_description *description)
{
   bool ret = false;
   int i;

   for (i = 0; fields[i].nm != NoName; i++) {
      if (fields[i].nm == nm) {
         spooling_field *sub_fields = fields[i].sub_fields;
         if (sub_fields != NULL) {
            sub_fields[0].clientdata = description;
            ret = true;
         }
         break;
      }
   }

   return ret;
}
#endif

static bool 
spool_database_assign_table_description(lList **answer_list, spooling_field *fields, const char *table_name, const lDescr *descr, bool sublevel)
{
   bool ret = true;

   DENTER(TOP_LAYER, "spool_database_assign_table_description");

   /* create table description for this level */

   if (table_name == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                              ANSWER_QUALITY_ERROR,
                              MSG_NOTABLENAMEPASSEDTO_S, SGE_FUNC);
      
   } else {
      const char *prefix;
      int key_nm;
      dstring prefix_dstring;
      char prefix_buffer[10];

      sge_dstring_init(&prefix_dstring, prefix_buffer, sizeof(prefix_buffer));
      prefix = object_get_name_prefix(descr, &prefix_dstring);
      key_nm = object_get_primary_key(descr);

      if (prefix == NULL || key_nm == NoName) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_UNKNOWNPREFIXORKEYNMFORTABLE_S, 
                                 table_name);
         ret = false;
      } else {
         bool recursive_table = false;
         int i;

         /* create table description for all sublevels */
         for (i = 0; fields[i].nm != NoName && ret; i++) {
            spooling_field *sub_fields = fields[i].sub_fields;
            if (sub_fields != NULL) {
               if (sub_fields == fields) {
                  recursive_table = true;
               } else {
                  const lDescr *sub_descr = object_get_subtype(fields[i].nm);
                  if (sub_descr == NULL) {
                     answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                             ANSWER_QUALITY_ERROR,
                                             MSG_UNKNOWNOBJECTTYPEFOR_SS,
                                             lNm2Str(fields[i].nm), SGE_FUNC);
                     ret = false;
                  } else {
                     const char *sub_table_name;

                     sub_table_name = spool_database_get_sub_table_name(table_name, fields[i].nm);
                     if (sub_table_name == NULL) {
                        answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                                ANSWER_QUALITY_ERROR,
                                                MSG_UNKNOWNTABLENAMEFORSUBLIST_S,
                                                lNm2Str(fields[i].nm));
                        ret = false;
                     } else {
                        ret = spool_database_assign_table_description(answer_list, 
                                                                      sub_fields, 
                                                                      sub_table_name,
                                                                      sub_descr, 
                                                                      true);
                     }
                  }
               }
            } 
         }
   
         /* processing for subfields succeeded. Create info for this level */
         if (ret) {
            table_description *description;
            description = spool_database_create_table_description(answer_list, table_name, prefix, key_nm, sublevel || recursive_table);

            if (description == NULL) {
               /* error messages created in spool_database_create_table_description */
               ret = false;
            } else {
               fields[0].clientdata = description;
            }
         }
      }
   }

   DEXIT;
   return ret;
}

static const char *
spool_database_get_sub_table_name(const char *prefix, int nm)
{
   const char *ret = NULL;

   dstring table_dstring;
   char table_buffer[MAX_STRING_SIZE];

   sge_dstring_init(&table_dstring, table_buffer, sizeof(table_buffer));

   switch (nm) {
      case CONF_entries:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "entries");
         break;
      case CX_entries:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "entries");
         break;
#ifndef __SGE_NO_USERMAPPING__
      case CU_ruser_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "rusers");
         break;
#endif
      case EH_acl:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "acl");
         break;
      case EH_xacl:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "xacl");
         break;
      case EH_consumable_config_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "complex_values");
         break;
      case EH_complex_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "complexes");
         break;
      case EH_load_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "load");
         break;
      case EH_prj:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "prj");
         break;
      case EH_xprj:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "xprj");
         break;
      case EH_scaling_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "load_scaling");
         break;
      case EH_usage_scaling_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "usage_scaling");
         break;
      case HGRP_host_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "hosts");
         break;
      case JAT_finished_task_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "finished_tasks");
         break;
      case JAT_granted_destin_identifier_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "granted_queues");
         break;
      case JAT_scaled_usage_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "scaled_usage");
         break;
      case JAT_task_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "tasks");
         break;
      case JAT_usage_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "usage");
         break;
      case JB_context:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "context");
         break;
      case JB_env_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "environment");
         break;
      case JB_hard_queue_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "hard_queue_list");
         break;
      case JB_ja_structure:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "ja_structure");
         break;
      case JB_ja_n_h_ids:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "ja_n_h_ids");
         break;
      case JB_ja_u_h_ids:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "ja_u_h_ids");
         break;
      case JB_ja_s_h_ids:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "ja_s_h_ids");
         break;
      case JB_ja_o_h_ids:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "ja_o_h_ids");
         break;
      case JB_ja_template:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "ja_template");
         break;
      case JB_ja_tasks:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "ja_tasks");
         break;
      case JB_ja_z_ids:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "ja_z_ids");
         break;
      case JB_jid_predecessor_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "predecessor");
         break;
      case JB_jid_sucessor_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "successor");
         break;
      case JB_job_args:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "args");
         break;
      case JB_mail_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "mail_list");
         break;
      case JB_master_hard_queue_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "master_hard_queue_list");
         break;
      case JB_path_aliases:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "path_aliases");
         break;
      case JB_pe_range:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "pe_range");
         break;
      case JB_shell_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "shell");
         break;
      case JB_soft_queue_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "soft_queue_list");
         break;
      case JB_stdout_path_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "stdout");
         break;
      case JB_stderr_path_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "stderr");
         break;
      case JB_stdin_path_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "stdin");
         break;
      case PE_user_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "acl");
         break;
      case PE_xuser_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "xacl");
         break;
      case PET_granted_destin_identifier_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "granted_queues");
         break;
      case PET_scaled_usage:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "scaled_usage");
         break;
      case PET_usage:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "usage");
         break;
      case QU_load_thresholds:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "load_thresholds");
         break;
      case QU_suspend_thresholds:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "suspend_thresholds");
         break;
      case QU_acl:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "acl");
         break;
      case QU_xacl:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "xacl");
         break;
      case QU_owner_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "owner");
         break;
      case QU_subordinate_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "subordinate");
         break;
      case QU_complex_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "complexes");
         break;
      case QU_consumable_config_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "complex_values");
         break;
      case QU_projects:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "prj");
         break;
      case QU_xprojects:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "xprj");
         break;
      case QU_pe_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "pe");
         break;
      case QU_ckpt_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "ckpt");
         break;
      case SC_job_load_adjustments:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "job_load_adjustments");
         break;
      case SC_usage_weight_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "usage_weight");
         break;
      case UP_acl:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "acl");
         break;
      case UP_xacl:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "xacl");
         break;
      case UP_debited_job_usage:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "debited_job_usage");
         break;
      case UP_long_term_usage:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "long_term_usage");
         break;
      case UP_project:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "project");
         break;
      case UP_usage:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "usage");
         break;
      case UPP_usage:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "usage");
         break;
      case UPP_long_term_usage:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "long_term_usage");
         break;
      case UPU_old_usage_list:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "usage");
         break;
      case US_entries:
         ret = sge_dstring_sprintf(&table_dstring, "%s_%s", prefix, "entries");
         break;
   }

   if (ret != NULL) {
      ret = strdup(ret);
   }

   return ret;
}
