#ifndef __SGE_SPOOLING_DATABASE_H 
#define __SGE_SPOOLING_DATABASE_H 
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

#include "spool/sge_spooling_utilities.h"

/****** spool/--Spooling-Database ************************************
*
*  NAME
*     database pooling - spooling of data into databases
*
*  FUNCTION
*     The module provides utility functions
*     for data input/output into databases.
*
*  SEE ALSO
*     spool/postgres/--Spooling-Postgres
****************************************************************************
*/

bool
spool_database_initialize(lList **answer_list, lListElem *rule);

void *
spool_database_get_handle(const lListElem *rule);
bool 
spool_database_set_handle(const lListElem *rule, void *handle);

void 
spool_database_set_history(const lListElem *rule, bool value);
bool
spool_database_get_history(const lListElem *rule);

const char *spool_database_get_table_name(const spooling_field *fields);
const char *spool_database_get_id_field(const spooling_field *fields);
const char *spool_database_get_parent_id_field(const spooling_field *fields);
const char *spool_database_get_valid_field(const spooling_field *fields);
const char *spool_database_get_created_field(const spooling_field *fields);
const char *spool_database_get_deleted_field(const spooling_field *fields);
int spool_database_get_key_nm(const spooling_field *fields);

spooling_field *
spool_database_get_fields(const lListElem *rule, sge_object_type type);

bool 
spool_database_store_id(lList **answer_list, const spooling_field *fields, 
                        const char *parent_key, const char *key, 
                        const char *id, bool tag);

const char *
spool_database_get_id(lList **answer_list, const spooling_field *fields, 
                      const char *parent_key, const char *key, bool tag);

bool
spool_database_delete_id(lList **answer_list, 
                         const spooling_field *fields, 
                         const char *parent_key, 
                         const char *key);

bool 
spool_database_tag_id(lList **answer_list, const spooling_field *fields, 
                      const char *parent_key, const char *key, bool value);

lList *
spool_database_get_id_list(lList **answer_list, 
                           const spooling_field *fields, 
                           const char *parent_key);

bool 
spool_database_object_changed(lList **answer_list, const lListElem *object, 
                              const spooling_field *fields);

#endif /* __SGE_SPOOLING_DATABASE_H */    

