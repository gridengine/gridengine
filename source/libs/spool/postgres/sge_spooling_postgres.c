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

#include <libpq-fe.h>

#include "sgermon.h"
#include "sge_log.h"

#include "sge_feature.h"

#include "sge_answer.h"
#include "sge_dstring.h"

#include "sge_object.h"

#include "spool/sge_spooling_database.h"
#include "spool/sge_spooling_sql.h"

#include "msg_common.h"
#include "spool/msg_spoollib.h"
#include "spool/postgres/msg_spoollib_postgres.h"

#include "spool/postgres/sge_spooling_postgres.h"

/****** spool/postgres/--Postgres-Spooling *************************************
*
*  NAME
*     Postgres Spooling -- spooling to PostgreSQL database
*
*  FUNCTION
*     This module provides an implementation of the spooling framework 
*     accessing a PostgreSQL database.
*
*  NOTES
*     The module uses the PostgreSQL c-interface in the standard way (waiting
*     for the result of each statement to execute).
*
*     Most probably, notable performance improvements could be achieved by
*     using libpq's "Asynchronous Query Processing" approach.
*
*  SEE ALSO
*     spool/--Spooling
*     spool/database/--Database-Spooling
*     spool/sql/--SQL-Spooling
****************************************************************************
*/
static const char *spooling_method = "postgres";

const char *
get_spooling_method(void)
{
   return spooling_method;
}

static lListElem *
spool_postgres_read_object(lList **answer_list, PGconn *connection, 
                           PGresult *res, int record,
                           int id_field, int key_nm,
                           spooling_field *fields, bool with_history,
                           const lDescr *descr,
                           const char *parent_id, const char *parent_key);
static bool
spool_postgres_read_list(lList **answer_list, PGconn *connection,
                         spooling_field *fields, bool with_history, 
                         lList **list, const lDescr *descr,
                         const char *parent_id, const char *parent_key);

static bool
spool_postgres_write_object(lList **answer_list, PGconn *connection, 
                            bool *transaction_started, 
                            spooling_field *fields, bool with_history, 
                            const lListElem *object, const char *key, 
                            const char *parent_id, const char *parent_key);

static bool
spool_postgres_write_sublist(lList **answer_list, PGconn *connection, 
                             bool *transaction_started, 
                             spooling_field *fields, bool with_history, 
                             const lList *list, 
                             const char *parent_id, const char *parent_key);

static bool
spool_postgres_write_sublists(lList **answer_list, PGconn *connection, 
                              bool *transaction_started, 
                              spooling_field *fields, bool with_history, 
                              const lListElem *object, 
                              const char *parent_id, const char *parent_key);

static bool
spool_postgres_delete_object(lList **answer_list, PGconn *connection, 
                             bool *transaction_started, 
                             spooling_field *fields, bool with_history, 
                             const char *key, 
                             const char *parent_id, const char *parent_key);

static bool
spool_postgres_delete_sublists(lList **answer_list, PGconn *connection, 
                               bool *transaction_started, 
                               spooling_field *fields, bool with_history, 
                               const char *key, 
                               const char *parent_id, const char *parent_key);

static bool
spool_postgres_delete_all_sublists(lList **answer_list, PGconn *connection, 
                                   bool *transaction_started, 
                                   spooling_field *fields, bool with_history, 
                                   const char *parent_id, const char *parent_key);

static bool 
spool_postgres_start_transaction(lList **answer_list, PGconn *connection, 
                                 bool *transaction_started);

static bool 
spool_postgres_stop_transaction(lList **answer_list, PGconn *connection, 
                                bool *transaction_started, bool commit);

static const char *
spool_postgres_create_new_id(lList **answer_list, PGconn *connection, 
                             bool *transaction_started, 
                             const spooling_field *fields);

static bool
spool_postgres_invalidate(lList **answer_list, PGconn *connection, 
                          bool *transaction_started, 
                          const spooling_field *fields, const char *id);


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
                              MSG_POSTGRES_INVALIDARGSTOCREATESPOOLINGCONTEXT);
   } else {
      lListElem *rule, *type;
      
      /* create spooling context */
      context = spool_create_context(answer_list, "postgresql spooling");
      
      /* create rule and type for all objects spooled in the spool dir */
      rule = spool_context_create_rule(answer_list, context, 
                                       "default rule", 
                                       args,
                                       spool_postgres_default_startup_func,
                                       spool_postgres_default_shutdown_func,
                                       spool_postgres_default_list_func,
                                       spool_postgres_default_read_func,
                                       spool_postgres_default_write_func,
                                       spool_postgres_default_delete_func,
                                       spool_postgres_default_verify_func);

      spool_database_initialize(answer_list, rule);

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
*                                         lListElem *rule)
*
*  FUNCTION
*     Tries to connect to the configured database.
*
*     Reads and checks (SGE) version information - the current SGE version
*     must equal the SGE version the database was created for.
*
*     Reads additional information from the database, e.g. whether historical
*     data shall be stored.
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
   bool ret = true;
   const char *url;
   PGconn *connection;

   DENTER(TOP_LAYER, "spool_postgres_default_startup_func");

   /* connect to database */
   url = lGetString(rule, SPR_url);
   connection = PQconnectdb(url);
   if (PQstatus(connection) == CONNECTION_BAD) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_POSTGRES_OPENFAILED_SS, 
                              url, PQerrorMessage(connection));
      ret = false;
   } else {
      /* retrieve version and history information */
      PGresult *res;

      spool_database_set_handle(rule, connection);
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_INFO, 
                              MSG_POSTGRES_OPENSUCCEEDED_S, 
                              url);
      /* check version and retrieve info */
      res = PQexec(connection, 
                   "SELECT * FROM sge_info ORDER BY last_change DESC LIMIT 1");
      if (res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_POSTGRES_CANNOTREADDBINFO_S, 
                                 PQerrorMessage(connection));
         ret = false;
      } else {
         char buffer[256];
         dstring ds;
         const char *db_version;
         const char *my_version;
        
         /* check version */
         sge_dstring_init(&ds, buffer, sizeof(buffer));
         my_version = feature_get_product_name(FS_SHORT_VERSION, &ds);

         db_version = PQgetvalue(res, 0, PQfnumber(res, "version"));

         if (strcmp(db_version, my_version) != 0) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_POSTGRES_WRONGVERSION_SS, 
                                    db_version, my_version);
            ret = false;
         } else {
            /* check and store history settings */
            const char *history = PQgetvalue(res, 0, 
                                             PQfnumber(res, "with_history"));
            if (*history == 'f') {
               spool_database_set_history(rule, false);
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_INFO,
                                       MSG_POSTGRES_HISTORYDISABLED);
            } else {
               spool_database_set_history(rule, true);
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_INFO,
                                       MSG_POSTGRES_HISTORYENABLED);
            }
         }
      }

      PQclear(res);
   }

   /* on error shutdown database connection */
   if (ret == false) {
      PQfinish(connection);
      spool_database_set_handle(rule, NULL);
   }

   DEXIT;
   return ret;
}

/****** spool/postgres/spool_postgres_default_shutdown_func() **************
*  NAME
*     spool_postgres_default_shutdown_func() -- shutdown database connection 
*
*  SYNOPSIS
*     bool 
*     spool_postgres_default_shutdown_func(lList **answer_list, 
*                                         lListElem *rule);
*
*  FUNCTION
*     Shuts down the database connection.
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *rule - the rule containing data necessary for
*                             the shutdown (e.g. path to the spool directory)
*
*  RESULT
*     bool - true, if the shutdown succeeded, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/postgres/--Spooling-Postgres
*     spool/spool_shutdown_context()
*******************************************************************************/
bool
spool_postgres_default_shutdown_func(lList **answer_list, 
                                    const lListElem *rule)
{
   bool ret = true;
   const char *url;
   PGconn *connection;

   DENTER(TOP_LAYER, "spool_postgres_default_shutdown_func");

   url = lGetString(rule, SPR_url);

   connection = spool_database_get_handle(rule);
   if (connection == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_WARNING, 
                              MSG_POSTGRES_NOCONNECTIONTOCLOSE_S, url);
      ret = false;
   } else {
      PQfinish(connection);
      spool_database_set_handle(rule, NULL);
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_INFO, 
                              MSG_POSTGRES_CLOSEDCONNECTION_S, 
                              url);
   }

   DEXIT;
   return ret;
}

/****** spool/postgres/spool_postgres_read_list() ***********************
*  NAME
*     spool_postgres_read_list() -- read a list of objects
*
*  SYNOPSIS
*     static bool 
*     spool_postgres_read_list(lList **answer_list, PGconn *connection, 
*                              spooling_field *fields, bool with_history, 
*                              lList **list, const lDescr *descr, 
*                              const char *parent_id, const char *parent_key) 
*
*  FUNCTION
*
*  INPUTS
*     lList **answer_list    - to return error messages
*     PGconn *connection     - database connection
*     spooling_field *fields - field information
*     bool with_history      - spool historical data?
*     lList **list           - tartet list
*     const lDescr *descr    - type of objects to read/store
*     const char *parent_id  - database internal id of parent object
*     const char *parent_key - primary key value of parent object
*
*  RESULT
*     bool - true on success, 
*            else false - error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool
spool_postgres_read_list(lList **answer_list, PGconn *connection,
                         spooling_field *fields, bool with_history, 
                         lList **list, const lDescr *descr,
                         const char *parent_id, const char *parent_key)
{
   bool ret = true;

   PGresult *res;
   dstring sql_dstring;
   char sql_buffer[MAX_STRING_SIZE];
   const char *table_name;

   DENTER(TOP_LAYER, "spool_postgres_read_list");

   sge_dstring_init(&sql_dstring, sql_buffer, sizeof(sql_buffer));
   table_name = spool_database_get_table_name(fields);

   /* JG: TODO: creating SQL could be moved to sge_spooling_sql */
   if (with_history) {
      sge_dstring_sprintf(&sql_dstring, 
         "SELECT * FROM %s WHERE %s = TRUE", table_name,
         spool_database_get_valid_field(fields));
      if (parent_id != NULL) {
         sge_dstring_sprintf_append(&sql_dstring, " AND %s = %s",
                                    spool_database_get_parent_id_field(fields),
                                    parent_id);
      }
   } else {
      sge_dstring_sprintf(&sql_dstring, "SELECT * FROM %s", table_name);
      if (parent_id != NULL) {
         sge_dstring_sprintf_append(&sql_dstring, " WHERE %s = %s",
                                    spool_database_get_parent_id_field(fields),
                                    parent_id);
      }
   }

   DPRINTF(("SQL: %s\n", sge_dstring_get_string(&sql_dstring)));

   res = PQexec(connection, sge_dstring_get_string(&sql_dstring));
   if (res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_POSTGRES_COMMANDFAILED_SS, 
                              sge_dstring_get_string(&sql_dstring),
                              PQerrorMessage(connection));
      ret = false;
   } else {
      int num_records = PQntuples(res);
      if (num_records > 0) {
         int i;
         int id_field;
         int key_nm;

         if (*list == NULL) {
            *list = lCreateList(table_name, descr);
         }

         /* store the field index from result set in fields structure, 
          * (mis)use width field 
          */
         id_field = PQfnumber(res, spool_database_get_id_field(fields));
         DPRINTF(("id_field of table %s is %s and has index %d\n",
                  table_name, spool_database_get_id_field(fields),
                  id_field));
         for (i = 0; fields[i].nm != NoName; i++) {
            fields[i].width = PQfnumber(res, fields[i].name);
            DPRINTF(("field id of %s is %d\n", fields[i].name, 
                     fields[i].width));
         }

         key_nm = spool_database_get_key_nm(fields);

         /* read all objects */
         for (i = 0; i < num_records && ret; i++) {
            lListElem *ep;
           
            ep = spool_postgres_read_object(answer_list, connection, res, i, 
                                            id_field, key_nm, fields, 
                                            with_history, descr, 
                                            parent_id, parent_key);
            if (ep == NULL) {
               /* error messages come from spool_postgres_read_object */
               ret = false;
            } else {
               lAppendElem(*list, ep);
            }
         }
      }
   }

   PQclear(res);

   DEXIT;
   return ret;
}

/****** spool/postgres/spool_postgres_read_object() *********************
*  NAME
*     spool_postgres_read_object() -- read an object
*
*  SYNOPSIS
*     static lListElem * 
*     spool_postgres_read_object(lList **answer_list, PGconn *connection, 
*                                PGresult *res, int record, 
*                                int id_field, int key_nm, 
*                                spooling_field *fields, bool with_history, 
*                                const lDescr *descr, 
*                                const char *parent_id, const char *parent_key) 
*
*  FUNCTION
*     Reads a single object from a given PostgreSQL result set.
*
*  INPUTS
*     lList **answer_list    - to return error messages
*     PGconn *connection     - database connection
*     PGresult *res          - a resultset from a previous query
*     int record             - record number to read from
*     int id_field           - field id for the internal database id attribute
*     int key_nm             - the primary key cull attribute
*     spooling_field *fields - field information
*     bool with_history      - spool historical data?
*     const lDescr *descr    - CULL type of the object to read 
*     const char *parent_id  - database internal id of parent object
*     const char *parent_key - primary key value of parent object
*
*  RESULT
*     lListElem * - on success the new element, 
*                   else NULL - error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static lListElem *
spool_postgres_read_object(lList **answer_list, PGconn *connection, 
                           PGresult *res, int record, 
                           int id_field, int key_nm,
                           spooling_field *fields, bool with_history,
                           const lDescr *descr,
                           const char *parent_id, const char *parent_key)
{
   lListElem *ep;
   int i;
   const char *id = NULL;
   const char *key = NULL;
   dstring key_dstring;
   char key_buffer[MAX_STRING_SIZE];

   DENTER(TOP_LAYER, "spool_postgres_read_object");

   /* create the raw object */
   ep = lCreateElem(descr);
  
   /* loop over all spooled attributes and fill object */
   for (i = 0; fields[i].nm != NoName && ep != NULL; i++) {
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
      if (type == lListT) {
         /* unspool sublist, if spooling information is available */
         spooling_field *sub_fields = fields[i].sub_fields;
         if (sub_fields != NULL && sub_fields[0].clientdata != NULL) {
            if (key == NULL || id == NULL) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_CANTREADSUBLISTASPRIMARYKEYVALUEUNKNOWN_S , 
                                       lNm2Str(fields[i].nm));
               ep = lFreeElem(ep);
               continue;
            } else {
               bool ret;
               lList *sub_list = NULL;
               const lDescr *sub_descr = object_get_subtype(fields[i].nm);

               DPRINTF(("reading sublist %s for parent %s:%s\n", 
                        lNm2Str(fields[i].nm), key, id));
               ret = spool_postgres_read_list(answer_list, connection,
                         sub_fields, with_history, 
                         &sub_list, sub_descr,
                         id, key);
               if (!ret) {
                  /* on error stop */
                  ep = lFreeElem(ep);
                  continue;
               } else {
                  if (sub_list != NULL) {
                     /* if any data was read for the sublist, store it */
                     lSetPosList(ep, pos, sub_list);
                  }
               }
            }
         }   
      } else {
         /* read attribute value from database resultset */
         const char *value;

         value = PQgetvalue(res, record, fields[i].width);
         if (value != NULL) {
            object_parse_field_from_string(ep, answer_list, fields[i].nm, value);

            /* we just parsed the key field - store key and id */
            if (fields[i].nm == key_nm) {
               sge_dstring_init(&key_dstring, key_buffer, sizeof(key_buffer));
               if (parent_key != NULL) {
                  key = sge_dstring_sprintf(&key_dstring, "%s|%s", 
                                            parent_key, value);
               } else {
                  key = sge_dstring_sprintf(&key_dstring, "%s", value);
               }
               id  = strdup(PQgetvalue(res, record, id_field));
               spool_database_store_id(answer_list, fields, parent_key, key, id,
                                       false);
               DPRINTF(("object with key %s has id %s\n", key, id));
            }
         }
      }
   }

   /* cleanup */
   if (id != NULL) {
      free((char *)id);
   }
   
   DEXIT;
   return ep;
}

/****** spool/postgres/spool_postgres_default_list_func() *****************
*  NAME
*     spool_postgres_default_list_func() -- read lists through postgres spooling
*
*  SYNOPSIS
*     bool 
*     spool_postgres_default_list_func(lList **answer_list, 
*                                      const lListElem *type, 
*                                      const lListElem *rule, lList **list, 
*                                      const sge_object_type event_type) 
*
*  FUNCTION
*     Read a list of objects from database.
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
   bool ret = true;

   DENTER(TOP_LAYER, "spool_postgres_default_list_func");

   switch (event_type) {
      case SGE_TYPE_ADMINHOST:
      case SGE_TYPE_EXECHOST:
         break;
      default:
#if 0
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_SPOOL_SPOOLINGOFXNOTSUPPORTED_S, 
                                 object_type_get_name(event_type));
#endif
         ret = false;
         break;
   }

   if (ret) {
      spooling_field *fields;
      const lDescr *descr;
      lList **master_list;
      bool with_history;
      PGconn *connection;

      fields      = spool_database_get_fields(rule, event_type);
      descr       = object_type_get_descr(event_type);
      master_list = object_type_get_master_list(event_type);
      with_history = spool_database_get_history(rule);
      connection   = spool_database_get_handle(rule);

      ret = spool_postgres_read_list(answer_list, connection,
                                     fields, with_history, 
                                     master_list, descr,
                                     NULL, NULL);
   }

   DEXIT;
   return ret;
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

   switch (event_type) {
      default:
#if 0
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_SPOOL_SPOOLINGOFXNOTSUPPORTED_S, 
                                 object_type_get_name(event_type));
#endif
         break;
   }

   DEXIT;
   return ep;
}

/****** spool/postgres/spool_postgres_start_transaction() ***************
*  NAME
*     spool_postgres_start_transaction() -- start a transaction
*
*  SYNOPSIS
*     static bool 
*     spool_postgres_start_transaction(lList **answer_list, PGconn *connection, 
*                                      bool *transaction_started) 
*
*  FUNCTION
*     Starts a transaction.
*
*  INPUTS
*     lList **answer_list       - to return error messages
*     PGconn *connection        - database connection
*     bool *transaction_started - do we already have a transaction open?
*                                 On success, it is set to true.
*
*  RESULT
*     static bool - true on success, 
*                   else false - error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool 
spool_postgres_start_transaction(lList **answer_list, PGconn *connection, 
                                 bool *transaction_started)
{
   bool ret = true;

   DENTER(TOP_LAYER, "spool_postgres_start_transaction");

   if (*transaction_started) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_CANTOPENTRANSACTIONALREADYOPEN); 
      ret = false;
   } else {
      const char *command = "BEGIN";
      PGresult *res;

      DPRINTF(("SQL: %s\n", command));
      res = PQexec(connection, command);
      if (res == NULL || PQresultStatus(res) != PGRES_COMMAND_OK) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_POSTGRES_COMMANDFAILED_SS, 
                                 command,
                                 PQerrorMessage(connection));
         ret = false;
      } else {
         *transaction_started = true;
      }

      PQclear(res);
   }

   DEXIT;
   return ret;
}

/****** spool/postgres/spool_postgres_stop_transaction() ****************
*  NAME
*     spool_postgres_stop_transaction() -- close a transaction 
*
*  SYNOPSIS
*     static bool 
*     spool_postgres_stop_transaction(lList **answer_list, PGconn *connection, 
*                                     bool *transaction_started, bool commit) 
*
*  FUNCTION
*     Closes a transaction. Dependent on the commit parameter, the transaction 
*     will either be commited or rolled back.
*
*  INPUTS
*     lList **answer_list       - to return error messages
*     PGconn *connection        - database connection
*     bool *transaction_started - do we have a transaction open?
*                                 On success, it is set to false.
*     bool commit               - true = commit, false = rollback
*
*  RESULT
*     static bool - true on success, 
*                   else false - error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool 
spool_postgres_stop_transaction(lList **answer_list, PGconn *connection, 
                                bool *transaction_started, bool commit)
{
   bool ret = true;

   DENTER(TOP_LAYER, "spool_postgres_stop_transaction");

   if (!*transaction_started) {  
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_CANTCLOSETRANSACTIONNONEOPEN); 
      ret = false;
   } else {
      const char *command;
      PGresult *res;
      
      if (commit) {
         command = "COMMIT";
      } else {
         command = "ROLLBACK";
      }

      DPRINTF(("SQL: %s\n", command));
      res = PQexec(connection, command);
      if (res == NULL || PQresultStatus(res) != PGRES_COMMAND_OK) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_POSTGRES_COMMANDFAILED_SS, 
                                 command,
                                 PQerrorMessage(connection));
         ret = false;
      } else {
         *transaction_started = false;
      }

      PQclear(res);
   }

   DEXIT;
   return ret;
}

/****** spool/postgres/spool_postgres_create_new_id() *******************
*  NAME
*     spool_postgres_create_new_id() -- create database internal record id
*
*  SYNOPSIS
*     static const char * 
*     spool_postgres_create_new_id(lList **answer_list, PGconn *connection, 
*                                  bool *transaction_started, 
*                                  const spooling_field *fields) 
*
*  FUNCTION
*     Relations between tables are usually built using unique id's.
*     These id's are typically created and maintained by database specific
*     features - in PostgreSQL it's the SEQUENCE.
*
*     This function retrieves a new id for a certain data type (table).
*
*     If necessary, a new transaction is opened.
*
*  INPUTS
*     lList **answer_list          - to return error messages
*     PGconn *connection           - database connection
*     bool *transaction_started    - do we already have a transaction open?
*     const spooling_field *fields - field information
*
*  RESULT
*     const char * - the new id on success,
*                    else NULL - error messages are returned in answer_list
*
*  NOTES
*     It is in the responsibility of the caller to free memory allocated
*     for the new id.
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static const char *
spool_postgres_create_new_id(lList **answer_list, PGconn *connection, 
                             bool *transaction_started, 
                             const spooling_field *fields)
{
   const char *id = NULL;

   DENTER(TOP_LAYER, "spool_postgres_create_new_id");

   /* if necessary, start a transaction */
   if (!*transaction_started) {
      spool_postgres_start_transaction(answer_list, connection, 
                                       transaction_started);
   }

   /* spool_postgres_start_transaction might have failed */
   if (*transaction_started) {
      char sql_buf[MAX_STRING_SIZE];
      dstring sql_dstring;
      PGresult *res;

      sge_dstring_init(&sql_dstring, sql_buf, sizeof(sql_buf));

      sge_dstring_sprintf(&sql_dstring, 
                          "SELECT nextval('%s_seq')", 
                          spool_database_get_table_name(fields));

      DPRINTF(("SQL: %s\n", sge_dstring_get_string(&sql_dstring)));
      res = PQexec(connection, sge_dstring_get_string(&sql_dstring));
      if (res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_POSTGRES_COMMANDFAILED_SS, 
                                 sge_dstring_get_string(&sql_dstring),
                                 PQerrorMessage(connection));
      } else {
         id = PQgetvalue(res, 0, PQfnumber(res, "nextval"));
         id = strdup(id);
      }

      PQclear(res);
   }

   DEXIT;
   return id;
}

/****** spool/postgres/spool_postgres_invalidate() **********************
*  NAME
*     spool_postgres_invalidate() -- invalidate a database record
*
*  SYNOPSIS
*     static bool spool_postgres_invalidate(lList **answer_list, PGconn 
*     *connection, bool *transaction_started, const spooling_field *fields, 
*     const char *id) 
*
*  FUNCTION
*     If we spool with historical information, records are not deleted, but
*     set to invalid.
*
*     This functions sets the database record for an object given by its
*     database internal id to invalid and in addition stores a timestamp
*     for the invalidation time.
*
*  INPUTS
*     lList **answer_list          - to return error messages
*     PGconn *connection           - database connection
*     bool *transaction_started    - do we already have a transaction open?
*     const spooling_field *fields - field information
*     const char *id               - database internal id of the object
*
*  RESULT
*     bool - true on success, 
*            else false - error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool
spool_postgres_invalidate(lList **answer_list, PGconn *connection, 
                          bool *transaction_started, 
                          const spooling_field *fields, const char *id)
{
   bool ret = true;

   DENTER(TOP_LAYER, "spool_postgres_invalidate");

   /* if necessary, start a transaction */
   if (!*transaction_started) {
      ret = spool_postgres_start_transaction(answer_list, connection, 
                                             transaction_started);
   }

   if (ret) {
      char sql_buf[MAX_STRING_SIZE];
      dstring sql_dstring;
      const char *valid_field;
      PGresult *res;

      valid_field = spool_database_get_valid_field(fields);

      sge_dstring_init(&sql_dstring, sql_buf, sizeof(sql_buf));
      sge_dstring_sprintf(&sql_dstring, 
                          "UPDATE %s SET %s = FALSE, %s = 'now' "
                          "WHERE %s = %s AND %s = TRUE", 
                          spool_database_get_table_name(fields),
                          valid_field,
                          spool_database_get_deleted_field(fields),
                          spool_database_get_id_field(fields),
                          id,
                          valid_field);

      DPRINTF(("SQL: %s\n", sge_dstring_get_string(&sql_dstring)));
      res = PQexec(connection, sge_dstring_get_string(&sql_dstring));
      if (res == NULL || PQresultStatus(res) != PGRES_COMMAND_OK) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_POSTGRES_COMMANDFAILED_SS, 
                                 sge_dstring_get_string(&sql_dstring),
                                 PQerrorMessage(connection));
         ret = false;
      }

      PQclear(res);
   }

   DEXIT;
   return ret;
}

/****** spool/postgres/spool_postgres_write_object() ********************
*  NAME
*     spool_postgres_write_object() -- write an object to database
*
*  SYNOPSIS
*     static bool 
*     spool_postgres_write_object(lList **answer_list, PGconn *connection, 
*                                 bool *transaction_started, 
*                                 spooling_field *fields, bool with_history, 
*                                 const lListElem *object, const char *key, 
*                                 const char *parent_id, const char *parent_key)
*
*  FUNCTION
*     Writes an object into the database.
*     The object itself written, then any sublists contained in the object
*     are spooled.
*
*     If necessary, a new transaction is opened.
*
*  INPUTS
*     lList **answer_list       - to return error messages
*     PGconn *connection        - database connection
*     bool *transaction_started - do we already have a transaction open?
*     spooling_field *fields    - field information
*     bool with_history         - spool historical data?
*     const lListElem *object   - the object to spool
*     const char *key           - primary key value of the object
*     const char *parent_id     - database internal id of parent object
*     const char *parent_key    - primary key value of parent object
*
*  RESULT
*     bool - true on success, 
*            else false - error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool
spool_postgres_write_object(lList **answer_list, PGconn *connection, 
                            bool *transaction_started, 
                            spooling_field *fields, bool with_history, 
                            const lListElem *object, const char *key, 
                            const char *parent_id, const char *parent_key)
{
   bool ret = true;
   bool data_written = false;

   dstring sql_dstring = DSTRING_INIT;
   const char *table_name;

   const char *id;
   const char *new_id = NULL;
   PGresult *res;

   DENTER(TOP_LAYER, "spool_postgres_write_object");

   table_name  = spool_database_get_table_name(fields);

   /* if the object is not yet known, or we have history enabled: insert */
   DPRINTF(("lookup id for key %s\n", key));

   /* JG: TODO: only retrieve the id, if the object itself changed, 
    * or before writing sublists 
    */
   id = spool_database_get_id(answer_list, fields, parent_key, key, true);

   /* new objects, or update to objects with history enabled => INSERT */
   if (id == NULL || with_history) {
      dstring field_dstring = DSTRING_INIT;
      dstring value_dstring = DSTRING_INIT;

      /* do only spool, if object is not yet known or has changed */
      data_written = false;
      if (id == NULL || spool_database_object_changed(answer_list, object, 
                                                      fields)) {
         spool_sql_create_insert_statement(answer_list, 
                                          &field_dstring, 
                                          &value_dstring, 
                                          fields, 
                                          object, &data_written);
      }

      /* if spooling with history, we have to make earlier records invalid */
      if (id != NULL && with_history && data_written) {
         ret = spool_postgres_invalidate(answer_list, connection, 
                                         transaction_started, fields, id);
      }

      if (ret && data_written) {
         if (id == NULL) {
            new_id = spool_postgres_create_new_id(answer_list, connection, 
                                                  transaction_started, fields);
            id = new_id;
         }

         if (id == NULL) {
            /* creating a new id must have failed 
             * answer_list contains error messages 
             */
            ret = false;
         } else {
            if (parent_id == NULL) {
               sge_dstring_sprintf(&sql_dstring, 
                                   "INSERT INTO %s (%s, %s) "
                                   "VALUES (%s, %s)",
                                   table_name, 
                                   spool_database_get_id_field(fields),
                                   sge_dstring_get_string(&field_dstring),
                                   id,
                                   sge_dstring_get_string(&value_dstring));
            } else {
               sge_dstring_sprintf(&sql_dstring, 
                                   "INSERT INTO %s (%s, %s, %s) "
                                   "VALUES (%s, %s, %s)",
                                   table_name, 
                                   spool_database_get_id_field(fields),
                                   spool_database_get_parent_id_field(fields),
                                   sge_dstring_get_string(&field_dstring),
                                   id,
                                   parent_id,
                                   sge_dstring_get_string(&value_dstring));
            }
         }
      }

      sge_dstring_free(&field_dstring);
      sge_dstring_free(&value_dstring);
   } else {
      /* updating existing objects without history => UPDATE */
      dstring update_dstring = DSTRING_INIT;

      /* create_update_statement will detect, if object changed at all */
      spool_sql_create_update_statement(answer_list, 
                                        &update_dstring, 
                                        fields, 
                                        object, &data_written);
      if (data_written) {
         sge_dstring_sprintf(&sql_dstring, 
                             "UPDATE %s set %s = 'now', %s where %s = %s",
                             table_name,
                             spool_database_get_created_field(fields),
                             sge_dstring_get_string(&update_dstring),
                             spool_database_get_id_field(fields),
                             id);
      }

      sge_dstring_free(&update_dstring);
   }

   /* preparation of write operation OK? -> Write data */
   if (ret && data_written) {
      /* if necessary, start a transaction */
      if (!*transaction_started) {
         ret = spool_postgres_start_transaction(answer_list, connection, 
                                                transaction_started);
      }

      if (ret) {
         DPRINTF(("SQL: %s\n", sge_dstring_get_string(&sql_dstring)));
         res = PQexec(connection, 
                      sge_dstring_get_string(&sql_dstring));
         if (res == NULL || PQresultStatus(res) != PGRES_COMMAND_OK) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_POSTGRES_COMMANDFAILED_SS, 
                                    sge_dstring_get_string(&sql_dstring),
                                    PQerrorMessage(connection));
            ret = false;
         }

         PQclear(res);
      }
   }

   sge_dstring_free(&sql_dstring);

   /* spool sublists */
   if (ret) {
      ret = spool_postgres_write_sublists(answer_list, connection, 
                                          transaction_started, 
                                          fields, with_history, 
                                          object, id, key);
   }

   /* if we created a new record and everything went well: store id */
   if (ret) {
      if (new_id != NULL) {
         DPRINTF(("storing key %s with id %s\n", key, id));
         spool_database_store_id(answer_list, fields, parent_key, key, id, true);
      }
   }

   /* if we created a new record: free the new id */
   if (new_id != NULL) {
      free((char *)new_id);
      new_id = NULL;
   }

   DEXIT;
   return ret;
}

/****** spool/postgres/spool_postgres_write_sublist() *******************
*  NAME
*     spool_postgres_write_sublist() -- spool a sublist of an object
*
*  SYNOPSIS
*     static bool 
*     spool_postgres_write_sublist(lList **answer_list, PGconn *connection, 
*                                  bool *transaction_started, 
*                                  spooling_field *fields, bool with_history, 
*                                  const lList *list, 
*                                  const char *parent_id, const char *parent_key) 
*
*  FUNCTION
*     Spool all elements of an objects sublists.
*     
*     This is a two pass operation:
*     1. All elements of the given list are written. These objects are tagged
*        in the id->key mapping.
*     2. The id->key mapping is analyzed. All untagged objects - these are 
*        objects that are no longer contained in the sublist, but still in 
*        the database - are deleted (or invalidated in case of spooling with
*        historical data).
*
*  INPUTS
*     lList **answer_list       - to return error messages
*     PGconn *connection        - database connection
*     bool *transaction_started - do we have an open transaction?
*     spooling_field *fields    - field information
*     bool with_history         - spool historical data?
*     const lList *list         - list to spool
*     const char *parent_id     - database internal id of parent object
*     const char *parent_key    - primary key value of parent object
*
*  RESULT
*     bool - true on success, 
*            else false - error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool
spool_postgres_write_sublist(lList **answer_list, PGconn *connection, 
                             bool *transaction_started, 
                             spooling_field *fields, bool with_history, 
                             const lList *list, 
                             const char *parent_id, const char *parent_key)
{
   bool ret = true;
   const lListElem *object;
   dstring key_dstring;
   char key_buffer[MAX_STRING_SIZE];

   DENTER(TOP_LAYER, "spool_postgres_write_sublist");

   sge_dstring_init(&key_dstring, key_buffer, sizeof(key_buffer));

   /* write all objects */
   for_each(object, list) {
      int key_nm;
      const char *key;
/* JG: TODO: get key_nm outside for_each */
      key_nm = spool_database_get_key_nm(fields);
      sge_dstring_sprintf(&key_dstring, "%s|", parent_key);
      key = object_append_field_to_dstring(object, answer_list, 
                                           &key_dstring, key_nm, '\0');
      ret = spool_postgres_write_object(answer_list, connection, 
                                        transaction_started, 
                                        fields, with_history, 
                                        object, key, 
                                        parent_id, parent_key);
      if (!ret) {
         break;
      }
   }

   /* after writing all objects, we have to delete no longer existing ones.
    * these are the objects, that are not tagged in the id mapping.
    */
   if (ret) {
      lList *id_list;
      lListElem *id_ep, *next_ep;

      id_list = spool_database_get_id_list(answer_list, fields, parent_key);
      /* loop over all ids - cannot use for_each, as objects are deleted */
      next_ep = lFirst(id_list);
      while ((id_ep = next_ep) != NULL) {
         next_ep = lNext(id_ep);

         if (lGetBool(id_ep, SPM_tag)) {
            /* unset tag */
            lSetBool(id_ep, SPM_tag, false);
         } else {
            /* delete or invalidate object */
            const char *key = lGetString(id_ep, SPM_key);
            if (key != NULL) {
               DPRINTF(("deleting no longer existend object %s\n", key));
               spool_postgres_delete_object(answer_list, connection, 
                                            transaction_started, 
                                            fields, with_history, 
                                            key, parent_id, parent_key);
            } else {
               /* do not break processing, but delete the faulty id object */
               DPRINTF(("inconsistend id->key mapping\n"));
               lRemoveElem(id_list, id_ep);
            }
         }
      }
   }

   DEXIT;
   return ret;
}

/****** spool/postgres/spool_postgres_write_sublists() ******************
*  NAME
*     spool_postgres_write_sublists() -- spool all sublists of an object
*
*  SYNOPSIS
*     static bool 
*     spool_postgres_write_sublists(lList **answer_list, PGconn *connection, 
*                                   bool *transaction_started, 
*                                   spooling_field *fields, bool with_history, 
*                                   const lListElem *object, 
*                                   const char *parent_id, const char *parent_key) 
*
*  FUNCTION
*     Spool all sublists of an object.
*
*  INPUTS
*     lList **answer_list       - to return error messages
*     PGconn *connection        - database connection
*     bool *transaction_started - do we already have an open transaction?
*     spooling_field *fields    - field information
*     bool with_history         - spool historical data?
*     const lListElem *object   - the object, whose lists have to be spooled
*     const char *parent_id     - database internal id of parent object
*     const char *parent_key    - primary key value of parent object
*
*  RESULT
*     bool - true on success, 
*            else false - error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool
spool_postgres_write_sublists(lList **answer_list, PGconn *connection, 
                              bool *transaction_started, 
                              spooling_field *fields, bool with_history, 
                              const lListElem *object, 
                              const char *parent_id, const char *parent_key)
{
   bool ret = true;
   const lDescr *descr;
   int i;

   DENTER(TOP_LAYER, "spool_postgres_write_sublists");

   descr = lGetElemDescr(object);

   /* check all spooled fields, if they are of type "list", spool them */
   for (i = 0; fields[i].nm != NoName && ret; i++) {
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

      if (type == lListT) {
         lList *list = lGetPosList(object, pos);
         if (list != NULL) {
            spooling_field *sub_fields = fields[i].sub_fields;
            
            if (sub_fields != NULL && sub_fields[0].clientdata != NULL) {
               DPRINTF(("spooling sublist %s for parent %s:%s\n", 
                        lNm2Str(descr[i].nm), parent_key, parent_id));
               ret = spool_postgres_write_sublist(answer_list, connection, 
                                                  transaction_started, 
                                                  sub_fields, with_history, 
                                                  list, parent_id, parent_key);
            }
         }
      }
   }

   DEXIT;
   return ret;
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
*     Algorithm:
*        search id
*        if id found
*           if spooing history
*              create insert statement
*              set old record to invalid
*              delete old id
*              create new id
*              execute insert
*              store new id
*           else
*              create and execute update statement
*        else
*           create insert statement
*           create new id
*           execute insert
*           store new id
*        
*        
*  INPUTS
*     lList **answer_list              - to return error messages
*     const lListElem *type            - object type description
*     const lListElem *rule            - rule to use
*     const lListElem *object          - object to spool
*     const char *key                  - unique key
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
   bool ret = true;

   DENTER(TOP_LAYER, "spool_postgres_default_write_func");

   switch (event_type) {
      case SGE_TYPE_ADMINHOST:
      case SGE_TYPE_EXECHOST:
         break;
      default:
#if 0
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_SPOOL_SPOOLINGOFXNOTSUPPORTED_S, 
                                 object_type_get_name(event_type));
#endif
         ret = false;
         break;
   }

   if (ret) {
      bool with_history, transaction_started;
      spooling_field *fields;
      PGconn *connection;

      with_history = spool_database_get_history(rule);
      transaction_started = false;
      fields       = spool_database_get_fields(rule, event_type);
      connection   = spool_database_get_handle(rule);

      ret = spool_postgres_write_object(answer_list, connection, 
                                        &transaction_started, 
                                        fields, with_history, 
                                        object, key, NULL, NULL);
      /* if a transaction was started during write_object - close it */
      if (transaction_started) {
         spool_postgres_stop_transaction(answer_list, connection, 
                                         &transaction_started, ret);
      }
   }

   DEXIT;
   return ret;
}

/****** spool/postgres/spool_postgres_delete_object() *******************
*  NAME
*     spool_postgres_delete_object() -- delete an object from the database
*
*  SYNOPSIS
*     static bool 
*     spool_postgres_delete_object(lList **answer_list, PGconn *connection, 
*                                  bool *transaction_started, 
*                                  spooling_field *fields, bool with_history, 
*                                  const char *key, 
*                                  const char *parent_id, const char *parent_key) 
*
*  FUNCTION
*     Deletes an object from the database.
*     If spooling with historical data is enabled, the object is just 
*     invalidated.
*
*     The operation comprises two steps:
*     1. Delete/invalidate all sublists of the object.
*     2. Delete/invalidate the object itself.
*
*  INPUTS
*     lList **answer_list       - to return error messages
*     PGconn *connection        - database connection
*     bool *transaction_started - do we already have an open transaction?
*     spooling_field *fields    - field information
*     bool with_history         - spool historical data?
*     const char *key           - primary key of the object to delete
*     const char *parent_id     - database internal id of parent object
*     const char *parent_key    - primary key value of parent object
*
*  RESULT
*     bool - true on success, 
*            else false - error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool
spool_postgres_delete_object(lList **answer_list, PGconn *connection, 
                             bool *transaction_started, 
                             spooling_field *fields, bool with_history, 
                             const char *key, 
                             const char *parent_id, const char *parent_key)
{
   bool ret = true;

   DENTER(TOP_LAYER, "spool_postgres_delete_object");

   /* delete or invalidate all sublists for this object */
   ret = spool_postgres_delete_sublists(answer_list, connection, 
                                        transaction_started, 
                                        fields, with_history, 
                                        key, parent_id, parent_key);

   if (ret) {
      const char *table_name;
      const char *id_field, *valid_field, *deleted_field, *parent_field;

      dstring sql_dstring;
      char sql_buffer[MAX_STRING_SIZE];

      sge_dstring_init(&sql_dstring, sql_buffer, sizeof(sql_buffer));

      /* JG: TODO: don't need all fields for all possible cases */
      table_name    = spool_database_get_table_name(fields);
      id_field      = spool_database_get_id_field(fields);
      valid_field   = spool_database_get_valid_field(fields);
      deleted_field = spool_database_get_deleted_field(fields);
      parent_field  = spool_database_get_parent_id_field(fields);

      if (key != NULL) {
         /* delete a specific object */
         const char *id;
         
         /* get database internal id of the object */
         id = spool_database_get_id(answer_list, fields, parent_key, key, 
                                    false);
         if (id == NULL) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_WARNING, 
                                    MSG_OBJECTWITHPRIMARYKEYUNKNOWN_S, key);
            ret = false;
         } else {
            if (with_history) {
               sge_dstring_sprintf(&sql_dstring, 
                  "UPDATE %s SET %s = FALSE, %s = 'now' "
                  "WHERE %s = %s AND %s = TRUE", 
                  table_name, valid_field, deleted_field,
                  id_field, id, valid_field);
            } else {
               sge_dstring_sprintf(&sql_dstring, 
                  "DELETE FROM %s WHERE %s = %s", 
                  table_name, id_field, id);
            }
         }
      } else {
         /* delete all objects related to a certain parent object */
         if (parent_key == NULL || parent_id == NULL) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_PARENTKEYORIDNULL);
            ret = false;
         } else {
            /* delete all objects that reference the parent object */
            if (with_history) {
               sge_dstring_sprintf(&sql_dstring, 
                  "UPDATE %s SET %s = FALSE, %s = 'now' "
                  "WHERE %s = %s AND %s = TRUE",
                  table_name, valid_field, deleted_field,
                  parent_field, parent_id, valid_field);
            } else {
               sge_dstring_sprintf(&sql_dstring, 
                  "DELETE FROM %s where %s = %s",
                  table_name, parent_field, parent_id);
            }
         }   
      }

      if (ret) {
         /* if necessary, start a transaction */
         if (!*transaction_started) {
            ret = spool_postgres_start_transaction(answer_list, connection, 
                                                   transaction_started);
         }

         if (ret) {
            PGresult *res;

            DPRINTF(("SQL: %s\n", sge_dstring_get_string(&sql_dstring)));
            res = PQexec(connection, sge_dstring_get_string(&sql_dstring));
            if (res == NULL || PQresultStatus(res) != PGRES_COMMAND_OK) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_POSTGRES_COMMANDFAILED_SS, 
                                       sge_dstring_get_string(&sql_dstring),
                                       PQerrorMessage(connection));
               ret = false;
            }

            PQclear(res);
         }
      }

      /* delete id from internal id->key mapping */
      if (ret) {
         spool_database_delete_id(answer_list, fields, parent_key, key);
      }
   }

   DEXIT;
   return ret;
}

/****** spool/postgres/spool_postgres_delete_sublists() *****************
*  NAME
*     spool_postgres_delete_sublists() -- delete sublist for object(s)
*
*  SYNOPSIS
*     static bool 
*     spool_postgres_delete_sublists(lList **answer_list, PGconn *connection, 
*                                    bool *transaction_started, 
*                                    spooling_field *fields, bool with_history, 
*                                    const char *key, 
*                                    const char *parent_id, const char *parent_key) 
*
*  FUNCTION
*     Delete a sublist for a specific object or for all objects related to a 
*     certain parent object.
*
*  INPUTS
*     lList **answer_list       - to return error messages
*     PGconn *connection        - database connection
*     bool *transaction_started - do we already have an open transaction?
*     spooling_field *fields    - field information
*     bool with_history         - spool historical data?
*     const char *key           - primary key of the object to delete, 
*                                 or NULL, if all objects related to 
*                                 parent_key shall be deleted
*     const char *parent_id     - database internal id of parent object
*     const char *parent_key    - primary key value of parent object
*
*  RESULT
*     bool - true on success, 
*            else false - error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
/* add comment with use cases */
static bool
spool_postgres_delete_sublists(lList **answer_list, PGconn *connection, 
                               bool *transaction_started, 
                               spooling_field *fields, bool with_history, 
                               const char *key, 
                               const char *parent_id, const char *parent_key)
{
   bool ret = true;

   DENTER(TOP_LAYER, "spool_postgres_delete_sublists");
   
   if (key != NULL) {
      /* we get exactly one id */
      const char *id;
      id = spool_database_get_id(answer_list, fields, parent_key, key, false);

      /* if we have no parent_object: key = parent_key for sub objects,
       * else we build the key for sub objects from parent_key and key
       */
      if (parent_key == NULL) {
         ret = spool_postgres_delete_all_sublists(answer_list, connection, 
                                                  transaction_started, 
                                                  fields, with_history, 
                                                  id, key);
      } else {
         dstring key_dstring;
         char key_buffer[MAX_STRING_SIZE];
         const char *new_key;

         sge_dstring_init(&key_dstring, key_buffer, sizeof(key_buffer));
         new_key = sge_dstring_sprintf(&key_dstring, "%s|%s", parent_key, key);
         ret = spool_postgres_delete_all_sublists(answer_list, connection, 
                                                  transaction_started, 
                                                  fields, with_history, 
                                                  id, new_key);
      }
   } else {
      /* we have to loop over all id's - this requires a parent key */
      if (parent_key == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_PARENTKEYORIDNULL);
         ret = false;
      } else {
         lList *id_list;
         lListElem *id_ep;
         dstring key_dstring;
         char key_buffer[MAX_STRING_SIZE];

         id_list = spool_database_get_id_list(answer_list, fields, parent_key);
         sge_dstring_init(&key_dstring, key_buffer, sizeof(key_buffer));
        
         for_each(id_ep, id_list) {
            const char *new_id;
            const char *new_key;

            new_id = lGetString(id_ep, SPM_id);
            new_key = sge_dstring_sprintf(&key_dstring, "%s|%s", 
                                          parent_key, 
                                          lGetString(id_ep, SPM_key));
            ret = spool_postgres_delete_all_sublists(answer_list, connection, 
                                                     transaction_started, 
                                                     fields, with_history, 
                                                     new_id, new_key);
            if (!ret) {
               break;
            }
         }
      }
   }

   DEXIT;
   return ret;
}

/****** spool/postgres/spool_postgres_delete_all_sublists() *************
*  NAME
*     spool_postgres_delete_all_sublists() -- delete all sublists
*
*  SYNOPSIS
*     static bool 
*     spool_postgres_delete_all_sublists(lList **answer_list, PGconn *connection, 
*                                        bool *transaction_started, 
*                                        spooling_field *fields, bool with_history, 
*                                        const char *parent_id, const char *parent_key) 
*
*  FUNCTION
*     Deletes all sublists for a specific object, or for all objects that are 
*     related to a certain parent object.
*
*  INPUTS
*     lList **answer_list       - to return error messages
*     PGconn *connection        - database connection
*     bool *transaction_started - do we already have a transaction open?
*     spooling_field *fields    - field information
*     bool with_history         - spool historical data?
*     const char *parent_id     - database internal id of parent object
*     const char *parent_key    - primary key value of parent object
*
*  RESULT
*     bool - true on success, 
*            else false - error messages are returned in answer_list
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool
spool_postgres_delete_all_sublists(lList **answer_list, PGconn *connection, 
                                   bool *transaction_started, 
                                   spooling_field *fields, bool with_history, 
                                   const char *parent_id, const char *parent_key)
{
   bool ret = true;
   int i;

   DENTER(TOP_LAYER, "spool_postgres_delete_all_sublists");

   for (i = 0; fields[i].nm != NoName && ret; i++) {
      spooling_field *sub_fields = fields[i].sub_fields;

      /* we found a sublist */
      if (sub_fields != NULL && sub_fields[0].clientdata != NULL) {
         /* delete all objects for this sublist that have our parent_key */
         ret = spool_postgres_delete_object(answer_list, connection, 
                                            transaction_started, 
                                            sub_fields, with_history, 
                                            NULL, parent_id, parent_key);
      }
   }

   DEXIT;
   return ret;
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
   bool ret = true;
   spooling_field *fields;

   DENTER(TOP_LAYER, "spool_postgres_default_delete_func");

   switch (event_type) {
      case SGE_TYPE_ADMINHOST:
      case SGE_TYPE_EXECHOST:
         break;
      default:
#if 0
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_SPOOL_SPOOLINGOFXNOTSUPPORTED_S, 
                                 object_type_get_name(event_type));
#endif
         ret = false;
         break;
   }

   if (ret) {
      bool with_history, transaction_started;
      spooling_field *fields;
      PGconn *connection;

      with_history = spool_database_get_history(rule);
      transaction_started = false;
      fields       = spool_database_get_fields(rule, event_type);
      connection   = spool_database_get_handle(rule);

      ret = spool_postgres_delete_object(answer_list, connection, 
                                         &transaction_started, 
                                         fields, with_history, 
                                         key, NULL, NULL);

      /* if a transaction was started, we have to close it */
      if (transaction_started) {
         spool_postgres_stop_transaction(answer_list, connection, 
                                         &transaction_started, ret);
      }
   }


   DEXIT;
   return ret;
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

   switch (event_type) {
      default:
#if 0
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_SPOOL_SPOOLINGOFXNOTSUPPORTED_S, 
                                 object_type_get_name(event_type));
         ret = false;
#endif
         break;
   }

   DEXIT;
   return ret;
}

