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

#include <libpq-fe.h>

#include "sgermon.h"
#include "sge_log.h"

#include "sge_answer.h"
#include "sge_dstring.h"

#include "sge_object.h"

#include "msg_common.h"
#include "spool/msg_spoollib.h"
#include "spool/postgres/msg_spoollib_postgres.h"

#include "spool/postgres/sge_spooling_postgres.h"

static const char *spooling_method = "postgres";

const char *get_spooling_method(void)
{
   return spooling_method;
}


/****** spool/postgres/spool_postgres_create_context() ********************
*  NAME
*     spool_postgres_create_context() -- create a postgres spooling context
*
*  SYNOPSIS
*     lListElem* 
*     spool_postgres_create_context(lList **answer_list, const char *args)
*
*  FUNCTION
*     Create a spooling context for the postgres spooling.
* 
*  INPUTS
*     lList **answer_list - to return error messages
*     int argc     - number of arguments in argv
*     char *argv[] - argument vector
*
*  RESULT
*     lListElem* - on success, the new spooling context, else NULL
*
*  SEE ALSO
*     spool/--Spooling
*     spool/postgres/--Spooling-Postgres
*******************************************************************************/
lListElem *
spool_postgres_create_context(lList **answer_list, const char *args)
{
   lListElem *context = NULL;

   DENTER(TOP_LAYER, "spool_postgres_create_context");

   /* check input parameter (*/
   if (args == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_SPOOL_INVALIDARGSTOCREATESPOOLINGCONTEXT);
   } else {
      lListElem *rule, *type;
      
      /* create spooling context */
      context = spool_create_context(answer_list, "postgresql spooling");
      
      /* create rule and type for all objects spooled in the spool dir */
      rule = spool_context_create_rule(answer_list, context, 
                                       "default rule", 
                                       args,
                                       spool_postgres_default_startup_func,
                                       NULL,
                                       spool_postgres_default_list_func,
                                       spool_postgres_default_read_func,
                                       spool_postgres_default_write_func,
                                       spool_postgres_default_delete_func,
                                       spool_postgres_default_verify_func);
      type = spool_context_create_type(answer_list, context, SGE_TYPE_ALL);
      spool_type_add_rule(answer_list, type, rule, true);
   }

   DEXIT;
   return context;
}

/****** spool/postgres/spool_postgres_default_startup_func() **************
*  NAME
*     spool_postgres_default_startup_func() -- setup 
*
*  SYNOPSIS
*     bool 
*     spool_postgres_default_startup_func(lList **answer_list, 
*                                         const char *args)
*
*  FUNCTION
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *rule - the rule containing data necessary for
*                             the startup (e.g. path to the spool directory)
*
*  RESULT
*     bool - true, if the startup succeeded, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/postgres/--Spooling-Postgres
*     spool/spool_startup_context()
*******************************************************************************/
bool
spool_postgres_default_startup_func(lList **answer_list, 
                                    const lListElem *rule)
{
   const char *url;

   DENTER(TOP_LAYER, "spool_postgres_default_startup_func");

   url = lGetString(rule, SPR_url);

   DEXIT;
   return true;
}

/****** spool/postgres/spool_postgres_default_list_func() *****************
*  NAME
*     spool_postgres_default_list_func() -- read lists through postgres spooling
*
*  SYNOPSIS
*     bool 
*     spool_postgres_default_list_func(
*                                      lList **answer_list, 
*                                      const lListElem *type, 
*                                      const lListElem *rule, lList **list, 
*                                      const sge_object_type event_type) 
*
*  FUNCTION
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to be used 
*     lList **list                    - target list
*     const sge_object_type event_type - object type
*
*  RESULT
*     bool - true, on success, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/postgres/--Spooling-Postgres
*     spool/spool_read_list()
*******************************************************************************/
bool
spool_postgres_default_list_func(lList **answer_list, 
                                 const lListElem *type, 
                                 const lListElem *rule, lList **list, 
                                 const sge_object_type event_type)
{
   DENTER(TOP_LAYER, "spool_postgres_default_list_func");

   DEXIT;
   return true;
}

/****** spool/postgres/spool_postgres_default_read_func() *****************
*  NAME
*     spool_postgres_default_read_func() -- read objects through postgres spooling
*
*  SYNOPSIS
*     lListElem* 
*     spool_postgres_default_read_func(lList **answer_list, 
*                                      const lListElem *type, 
*                                      const lListElem *rule, const char *key, 
*                                      const sge_object_type event_type) 
*
*  FUNCTION
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const char *key                 - unique key specifying the object
*     const sge_object_type event_type - object type
*
*  RESULT
*     lListElem* - the object, if it could be read, else NULL
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/postgres/--Spooling-Postgres
*     spool/spool_read_object()
*******************************************************************************/
lListElem *
spool_postgres_default_read_func(lList **answer_list, 
                                 const lListElem *type, 
                                 const lListElem *rule, const char *key, 
                                 const sge_object_type event_type)
{
   lListElem *ep = NULL;

   DENTER(TOP_LAYER, "spool_postgres_default_read_func");

   DEXIT;
   return ep;
}

/****** spool/postgres/spool_postgres_default_write_func() ****************
*  NAME
*     spool_postgres_default_write_func() -- write objects through postgres spooling
*
*  SYNOPSIS
*     bool
*     spool_postgres_default_write_func(lList **answer_list, 
*                                       const lListElem *type, 
*                                       const lListElem *rule, 
*                                       const lListElem *object, 
*                                       const char *key, 
*                                       const sge_object_type event_type) 
*
*  FUNCTION
*     Writes an object through the appropriate postgres spooling functions.
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const lListElem *object         - object to spool
*     const char *key                 - unique key
*     const sge_object_type event_type - object type
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/postgres/--Spooling-Postgres
*     spool/spool_delete_object()
*******************************************************************************/
bool
spool_postgres_default_write_func(lList **answer_list, 
                                  const lListElem *type, 
                                  const lListElem *rule, 
                                  const lListElem *object, 
                                  const char *key, 
                                  const sge_object_type event_type)
{
   DENTER(TOP_LAYER, "spool_postgres_default_write_func");

   DEXIT;
   return true;
}

/****** spool/postgres/spool_postgres_default_delete_func() ***************
*  NAME
*     spool_postgres_default_delete_func() -- delete object in postgres spooling
*
*  SYNOPSIS
*     bool
*     spool_postgres_default_delete_func(lList **answer_list, 
*                                        const lListElem *type, 
*                                        const lListElem *rule, 
*                                        const char *key, 
*                                        const sge_object_type event_type) 
*
*  FUNCTION
*     Deletes an object in the postgres spooling.
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const char *key                 - unique key 
*     const sge_object_type event_type - object type
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/postgres/--Spooling-Postgres
*     spool/spool_delete_object()
*******************************************************************************/
bool
spool_postgres_default_delete_func(lList **answer_list, 
                                   const lListElem *type, 
                                   const lListElem *rule,
                                   const char *key, 
                                   const sge_object_type event_type)
{
   DENTER(TOP_LAYER, "spool_postgres_default_delete_func");

   DEXIT;
   return true;
}

/****** spool/postgres/spool_postgres_default_verify_func() ****************
*  NAME
*     spool_postgres_default_verify_func() -- verify objects
*
*  SYNOPSIS
*     bool
*     spool_postgres_default_verify_func(lList **answer_list, 
*                                        const lListElem *type, 
*                                        const lListElem *rule, 
*                                        const lListElem *object, 
*                                        const char *key, 
*                                        const sge_object_type event_type) 
*
*  FUNCTION
*     Verifies an object.
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const lListElem *object         - object to verify
*     const sge_object_type event_type - object type
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*     It should be moved to libs/spool/spooling_utilities or even to
*     libs/sgeobj/sge_object
*
*  SEE ALSO
*     spool/postgres/--Postgres-Spooling
*******************************************************************************/
bool
spool_postgres_default_verify_func(lList **answer_list, 
                                   const lListElem *type, 
                                   const lListElem *rule,
                                   lListElem *object,
                                   const sge_object_type event_type)
{
   bool ret = true;

   DENTER(TOP_LAYER, "spool_postgres_default_verify_func");

   DEXIT;
   return ret;
}

