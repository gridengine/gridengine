#ifndef __SGE_SPOOLING_SQL_H 
#define __SGE_SPOOLING_SQL_H 
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

/****** spool/--Spooling-SQL-Database ************************************
*
*  NAME
*     SQL database spooling - spooling of data into SQL databases
*
*  FUNCTION
*     The module provides utility functions
*     for data input/output into SQL databases.
*
*  SEE ALSO
*     spool/--Spooling-Database
****************************************************************************
*/

#include "basis_types.h"
#include "cull.h"
#include "sge_dstring.h"
#include "spool/sge_spooling_utilities.h"

bool
spool_sql_create_insert_statement(lList **answer_list, 
                                  dstring *field_dstring, 
                                  dstring *value_dstring, 
                                  spooling_field *fields, 
                                  const lListElem *object, 
                                  bool *data_written);

bool
spool_sql_create_update_statement(lList **answer_list, 
                                  dstring *update_dstring, 
                                  spooling_field *fields, 
                                  const lListElem *object,
                                  bool *data_written);


#endif /* __SGE_SPOOLING_SQL_H */    

