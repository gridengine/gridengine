#ifndef __SGE_SPOOLING_POSTGRES_H 
#define __SGE_SPOOLING_POSTGRES_H 
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

#include "spool/sge_spooling.h"

/****** spool/postgres/--Spooling-Postgres ************************************
*
*  NAME
*     postgres pooling - spooling of data in PostgreSQL database
*
*  FUNCTION
*     The module provides functions and a spooling framework instantiation
*     for data input/output into a PostgreSQL database.
*
*     Database access is based on PostgreSQL's libpq interface.
*
*     This spooling framework instantiation has been developed using 
*     version 7.3.2 of the PostgreSQL database.
*
*  SEE ALSO
*     spool/--Spooling-Database
*     spool/--Spooling-SQL-Database
****************************************************************************
*/

const char *
get_spooling_method(void);

lListElem *
spool_postgres_create_context(lList **answer_list, const char *args);

bool 
spool_postgres_default_startup_func(lList **answer_list, 
                                    const lListElem *rule, bool check);
bool 
spool_postgres_default_shutdown_func(lList **answer_list, 
                                     const lListElem *rule);
bool 
spool_postgres_default_maintenance_func(lList **answer_list, 
                                        const lListElem *rule,
                                        const spooling_maintenance_command cmd,
                                        const char *args);

bool 
spool_postgres_common_startup_func(lList **answer_list, 
                                   const lListElem *rule);

bool 
spool_postgres_default_list_func(lList **answer_list, 
                                 const lListElem *type, 
                                 const lListElem *rule, lList **list, 
                                 const sge_object_type object_type);
lListElem *
spool_postgres_default_read_func(lList **answer_list, 
                                 const lListElem *type, 
                                 const lListElem *rule, const char *key, 
                                 const sge_object_type object_type);
bool 
spool_postgres_default_write_func(lList **answer_list, 
                                  const lListElem *type, 
                                  const lListElem *rule, 
                                  const lListElem *object, const char *key, 
                                  const sge_object_type object_type);
bool 
spool_postgres_default_delete_func(lList **answer_list, 
                                   const lListElem *type, 
                                   const lListElem *rule, 
                                   const char *key, 
                                   const sge_object_type object_type);

#endif /* __SGE_SPOOLING_POSTGRES_H */    
