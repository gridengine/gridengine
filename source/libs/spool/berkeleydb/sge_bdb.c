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

#include <errno.h>
#include <string.h>

/* rmon */
#include "sgermon.h"
#include "sge_log.h"

/* uti */
#include "sge_profiling.h"
#include "sge_string.h"
#include "sge_unistd.h"

/* cull */
#include "cull.h"

/* sgeobj */
#include "sge_answer.h"
#include "sge_cqueue.h"
#include "sge_ja_task.h"
#include "sge_job.h"
#include "sge_object.h"

/* local */
#include "msg_common.h"
#include "spool/berkeleydb/msg_spoollib_berkeleydb.h"

#include "spool/berkeleydb/sge_bdb.h"

#if 1
static const int pack_part = CULL_SPOOL | CULL_SUBLIST | CULL_SPOOL_PROJECT | 
                             CULL_SPOOL_USER;
#else
static const int pack_part = 0;
#endif

/****** spool/berkeleydb/spool_berkeleydb_check_version() **********************
*  NAME
*     spool_berkeleydb_check_version() -- check version of shared libs 
*
*  SYNOPSIS
*     bool 
*     spool_berkeleydb_check_version(lList **answer_list) 
*
*  FUNCTION
*     Checks if major and minor version number returned by the db_version()
*     library call of Berkeley DB matches the version numbers set at compile
*     time.
*
*     The major and minor number must be equal, the patch level may differ.
*
*  INPUTS
*     lList **answer_list - used to return info and error messages
*
*  RESULT
*     bool - true, on success, else false
*
*  NOTES
*     MT-NOTE: spool_berkeleydb_check_version() is MT safe 
*
*******************************************************************************/
bool
spool_berkeleydb_check_version(lList **answer_list)
{
   bool ret = true;
   const char *version;
   int major, minor;

   DENTER(TOP_LAYER, "spool_berkeleydb_check_version");
   
   version = db_version(&major, &minor, NULL);

   answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                           ANSWER_QUALITY_INFO, 
                           MSG_BERKELEY_USINGBDBVERSION_S,
                           version);

   if (major != DB_VERSION_MAJOR || minor != DB_VERSION_MINOR) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_WRONGBDBVERSIONEXPECTING_SDD,
                              version, DB_VERSION_MAJOR, DB_VERSION_MINOR);
      ret = false;
   }
  
   DEXIT;
   return ret;
}

/****** spool/berkeleydb/spool_berkeleydb_create_environment() *****************
*  NAME
*     spool_berkeleydb_create_environment() -- ??? 
*
*  SYNOPSIS
*     bool spool_berkeleydb_create_environment(lList **answer_list, struct 
*     bdb_info *info, const char *url) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **answer_list   - ??? 
*     struct bdb_info *info - ??? 
*     const char *url       - ??? 
*
*  RESULT
*     bool - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: spool_berkeleydb_create_environment() is not MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool spool_berkeleydb_create_environment(lList **answer_list, 
                                         struct bdb_info *info, const char *url)
{ 
   bool ret = true;
   int dbret;

   DB_ENV *env;

   DENTER(TOP_LAYER, "spool_berkeleydb_create_environment");

   /* check database directory */
   if (!sge_is_directory(url)) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_DATABASEDIRDOESNTEXIST_S,
                              url);
      ret = false;
   } else {
      PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      dbret = db_env_create(&env, 0);
      PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      if (dbret != 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_BERKELEY_COULDNTCREATEENVIRONMENT_S,
                                 db_strerror(dbret));
         ret = false;
      } else {
         /* do deadlock detection internally */
         PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         dbret = env->set_lk_detect(env, DB_LOCK_DEFAULT);
         PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         if (dbret != 0) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_BERKELEY_COULDNTESETUPLOCKDETECTION_S,
                                    db_strerror(dbret));
            ret = false;
         } else {
            PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
            dbret = env->open(env, url, 
                              DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG | 
                              DB_INIT_MPOOL | DB_INIT_TXN | DB_RECOVER | 
                              DB_THREAD, 
                              S_IRUSR | S_IWUSR);
            PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
            if (dbret != 0){
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_BERKELEY_COULDNTCREATEENVIRONMENT_S,
                                       url,
                                       db_strerror(dbret));
               ret = false;
            }

            bdb_set_env(info, env, true);
         }
      }
   }   

   DEXIT;
   return ret;
}

bool 
spool_berkeleydb_open_database(lList **answer_list, struct bdb_info *info, 
                               const char *url, bool create)
{
   bool ret = true;

   DB_ENV *env;
   DB *db;

   int dbret;

   DENTER(TOP_LAYER, "spool_berkeleydb_open_database");

   env = bdb_get_env(info);

   if (env == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_NOCONNECTIONOPEN_S,
                              url);
      ret = false;
   } else {
      PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      dbret = db_create(&db, env, 0);
      PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      if (dbret != 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_BERKELEY_COULDNTCREATEDBHANDLE_S,
                                 db_strerror(dbret));
         ret = false;
         db = NULL;
      } else {
         int flags = DB_THREAD;
         int mode  = 0;

         if (create) {
            flags |= DB_CREATE;
            mode =  S_IRUSR | S_IWUSR;
         }

         ret = spool_berkeleydb_start_transaction(answer_list, info);
         if (ret) {
            DB_TXN *txn = bdb_get_txn(info);
            PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
            dbret = db->open(db, txn, "sge", NULL, 
                             DB_BTREE, flags, mode);
            PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
            ret = spool_berkeleydb_end_transaction(answer_list, info, true);
         }
         if (dbret != 0) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    create ? MSG_BERKELEY_COULDNTCREATEDB_SS : 
                                             MSG_BERKELEY_COULDNTOPENDB_SS,
                                    "sge", db_strerror(dbret));
            ret = false;
         }
      }

      bdb_set_db(info, db, true);
      DPRINTF(("db = %p\n", db));
   }

   DEXIT;
   return ret;
}

bool 
spool_berkeleydb_close_database(lList **answer_list, struct bdb_info *info,
                                const char *url)
{
   bool ret = true;

   DB_ENV *env;
   DB *db;

   DENTER(TOP_LAYER, "spool_berkeleydb_close_database");

   env = bdb_get_env(info);
   db = bdb_get_db(info);

   if (env == NULL || db == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_NOCONNECTIONOPEN_S,
                              url);
      ret = false;
   } else {
      int dbret;

      PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      dbret = db->close(db, 0);
      PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      if (dbret != 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_BERKELEY_COULDNTCLOSEDB_SS,
                                 url, db_strerror(dbret));
         ret = false;
      }  else {
         PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         dbret = env->close(env, 0);
         PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         if (dbret != 0) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_BERKELEY_COULDNTCLOSEENVIRONMENT_SS,
                                    url, db_strerror(dbret));
            ret = false;
         } else {
           answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                   ANSWER_QUALITY_INFO, 
                                   MSG_BERKELEY_CLOSEDDB_S,
                                   url);
         }
      }

      db = NULL;
      env = NULL;
      bdb_set_db(info, db, true);
      bdb_set_env(info, env, true);
   }

   DEXIT;
   return ret;
}

/****** sge_bdb/spool_berkeleydb_start_transaction() ***************************
*  NAME
*     spool_berkeleydb_start_transaction() -- start a transaction
*
*  SYNOPSIS
*     bool 
*     spool_berkeleydb_start_transaction(lList **answer_list, struct bdb_info *info) 
*
*  FUNCTION
*     Starts a transaction.
*     Transactions are bound to a certain thread, multiple threads can start
*     transactions in parallel.
*
*  INPUTS
*     lList **answer_list   - used to return error messages
*     struct bdb_info *info - database handle
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     MT-NOTE: spool_berkeleydb_start_transaction() is MT safe 
*
*  SEE ALSO
*     spool/berkeleydb/spool_berkeleydb_stop_transaction()
*******************************************************************************/
bool
spool_berkeleydb_start_transaction(lList **answer_list, struct bdb_info *info)
{
   bool ret = true;

   DB_ENV *env;
   DB_TXN *txn;

   DENTER(TOP_LAYER, "spool_berkeleydb_start_transaction");

   env = bdb_get_env(info);
   txn = bdb_get_txn(info);

   if (env == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_NOCONNECTIONOPEN_S,
                              "");
      ret = false;
   } else {
      if (txn != NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_BERKELEY_TXNALREADYOPEN);
         ret = false;
      } else {
         int dbret;

         PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         dbret = env->txn_begin(env, NULL, &txn, 0);
         PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         if (dbret != 0) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_BERKELEY_ERRORSTARTINGTRANSACTION_S,
                                    db_strerror(dbret));
            ret = false;
            txn = NULL;
         }
      }

      bdb_set_txn(info, txn);
   }

   DEXIT;
   return ret;
}

bool
spool_berkeleydb_end_transaction(lList **answer_list, struct bdb_info *info, 
                                 bool commit)
{
   bool ret = true;
   int dbret;

   DB_ENV *env;
   DB_TXN *txn;

   DENTER(TOP_LAYER, "spool_berkeleydb_end_transaction");

   env = bdb_get_env(info);
   txn = bdb_get_txn(info);

   if (env == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_NOCONNECTIONOPEN_S,
                              "");
      ret = false;
   } else {
      if (txn == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_BERKELEY_TXNNOTOPEN);
         ret = false;
      } else {
         if (commit) {
            PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
            dbret = txn->commit(txn, 0);
            PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         } else {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_WARNING, 
                                    MSG_BERKELEY_ABORTINGTRANSACTION);
            PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
            dbret = txn->abort(txn);
            PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         }

         if (dbret != 0) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_BERKELEY_ERRORENDINGTRANSACTION_S,
                                    db_strerror(dbret));
            ret = false;
         }

         txn = NULL;
         bdb_set_txn(info, txn);
      }
   }

   DEXIT;
   return ret;
}

bool
spool_berkeleydb_clear_log(lList **answer_list, struct bdb_info *info, 
                           const char *url)
{
   bool ret = true;
   int dbret;
   char **list;

   DB_ENV *env;

   DENTER(TOP_LAYER, "spool_berkeleydb_clear_log");

   env = bdb_get_env(info);
   if (env == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_NOCONNECTIONOPEN_S,
                              url);
      ret = false;
   } else {
      PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      dbret = env->log_archive(env, &list, DB_ARCH_ABS);
      PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      if (dbret != 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_BERKELEY_CANNOTRETRIEVELOGARCHIVE_S,
                                 db_strerror(dbret));
         ret = false;
      } else {
         if (list != NULL) {  
            char **file;

            for (file = list; *file != NULL; file++) {
               if (remove(*file) != 0) {
                  dstring error_dstring = DSTRING_INIT;

                  answer_list_add_sprintf(answer_list, STATUS_EDISK, 
                                          ANSWER_QUALITY_ERROR, 
                                          MSG_ERRORDELETINGFILE_SS,
                                          *file,
                                          sge_strerror(errno, &error_dstring));
                  sge_dstring_free(&error_dstring);
                  ret = false;
                  break;
               }
            }
            free(list);
         }
      }
   }

   DEXIT;
   return ret;
}

bool
spool_berkeleydb_checkpoint(lList **answer_list, struct bdb_info *info)
{
   bool ret = true;
   int dbret;

   DB_ENV *env;

   DENTER(TOP_LAYER, "spool_berkeleydb_clear_log");

   env = bdb_get_env(info);
   if (env == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_NOCONNECTIONOPEN_S,
                              "");
      ret = false;
   } else {
      PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      dbret = env->txn_checkpoint(env, 0, 0, 0);
      PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      if (dbret != 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_BERKELEY_CANNOTCHECKPOINT_S,
                                 db_strerror(dbret));
         ret = false;
      } 
   }

   DEXIT;
   return ret;
}

bool 
spool_berkeleydb_read_list(lList **answer_list, struct bdb_info *info,
                           lList **list, const lDescr *descr,
                           const char *key)
{
   bool ret = true;
   int dbret;

   DB *db;
   DB_TXN *txn;

   DBT key_dbt, data_dbt;
   DBC *dbc;

   DENTER(TOP_LAYER, "spool_berkeleydb_read_list");

   db  = bdb_get_db(info);
   txn = bdb_get_txn(info);

   if (db == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_NOCONNECTIONOPEN_S,
                              "");
      ret = false;
   } else {
      DPRINTF(("querying objects with keys %s*\n", key));

      PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      db->cursor(db, txn, &dbc, 0);
      PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);

      /* initialize query to first record for this object type */
      memset(&key_dbt, 0, sizeof(key_dbt));
      memset(&data_dbt, 0, sizeof(data_dbt));
      key_dbt.data = (void *)key;
      key_dbt.size = strlen(key) + 1;
      PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      dbret = dbc->c_get(dbc, &key_dbt, &data_dbt, DB_SET_RANGE);
      PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      while (true) {
         if (dbret != 0 && dbret != DB_NOTFOUND) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_BERKELEY_QUERYERROR_SS,
                                    key,
                                    db_strerror(dbret));
            ret = false;
            break;
         } else if (dbret == DB_NOTFOUND) {
            DPRINTF(("last record reached\n"));
            break;
         } else if (key_dbt.data != NULL && 
                    strncmp(key_dbt.data, key, strlen(key)) 
                    != 0) {
            DPRINTF(("current key is %s\n", key_dbt.data));
            DPRINTF(("last record of this object type reached\n"));
            break;
         } else {
            sge_pack_buffer pb;
            lListElem *object = NULL;
            int cull_ret;

            DPRINTF(("read object with key "SFQ", size %d\n", 
                     key_dbt.data, data_dbt.size));
            cull_ret = init_packbuffer_from_buffer(&pb, data_dbt.data, 
                                                   data_dbt.size, 0);
            if (cull_ret != PACK_SUCCESS) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_BERKELEY_UNPACKINITERROR_SS,
                                       key_dbt.data,
                                       cull_pack_strerror(cull_ret));
               ret = false;
               break;
            }
            DPRINTF(("init_packbuffer succeeded\n"));
            cull_ret = cull_unpack_elem_partial(&pb, &object, descr, pack_part);
            if (cull_ret != PACK_SUCCESS) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_BERKELEY_UNPACKERROR_SS,
                                       key_dbt.data,
                                       cull_pack_strerror(cull_ret));
               ret = false;
               break;
            }
            /* we may not free the packbuffer: it references the buffer
             * delivered from the database
             * clear_packbuffer(&pb);
             */
            if (object != NULL) {
               if (*list == NULL) {
                  *list = lCreateList(key, descr);
               }
               lAppendElem(*list, object);
            }

            /* get next record */
   /*          memset(&key_dbt, 0, sizeof(key_dbt)); */
   /*          memset(&data_dbt, 0, sizeof(data_dbt)); */
            PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
            dbret = dbc->c_get(dbc, &key_dbt, &data_dbt, DB_NEXT);
            PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         }
      }
      PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      dbc->c_close(dbc);
      PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
   }

   DEXIT;
   return ret;
}

bool 
spool_berkeleydb_write_object(lList **answer_list, struct bdb_info *info,
                              const lListElem *object, const char *key)
{
   bool ret = true;

   DENTER(TOP_LAYER, "spool_berkeleydb_write_object");

#if 0
   /* JG: TODO: we shouldn't spool free elements */
   if (object->status == FREE_ELEM) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_CANTSPOOLFREEELEM_S,
                              key);
      ret = false;
   } else 
#endif
   {
      sge_pack_buffer pb;
      DBT key_dbt, data_dbt;
      int dbret;
      int cull_ret;
      cull_ret = init_packbuffer(&pb, 8192, 0);
      if (cull_ret != PACK_SUCCESS) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_BERKELEY_PACKINITERROR_SS,
                                 key,
                                 cull_pack_strerror(cull_ret));
         ret = false;
      } else {
         cull_ret = cull_pack_elem_partial(&pb, object, pack_part);
         if (cull_ret != PACK_SUCCESS) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_BERKELEY_PACKERROR_SS,
                                    key,
                                    cull_pack_strerror(cull_ret));
            ret = false;
         } else { 
            DB *db = bdb_get_db(info);
            DB_TXN *txn = bdb_get_txn(info);
            DPRINTF(("db = %p, txn = %p\n", db, txn));

            memset(&key_dbt, 0, sizeof(key_dbt));
            memset(&data_dbt, 0, sizeof(data_dbt));
            key_dbt.data = (void *)key;
            key_dbt.size = strlen(key) + 1;
            data_dbt.data = pb.head_ptr;
            data_dbt.size = pb.bytes_used;

            DPRINTF(("storing object with key "SFQ", size = %d\n", key, 
                      data_dbt.size));

            /* Store a key/data pair. */
            PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
            dbret = db->put(db, txn, &key_dbt, &data_dbt, 0);
            PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
            if (dbret != 0) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_BERKELEY_PUTERROR_SS,
                                       key, db_strerror(dbret));
               ret = false;
            }
         }

         clear_packbuffer(&pb);
      }
   }
   DEXIT;
   return ret;
}

bool
spool_berkeleydb_write_pe_task(lList **answer_list, struct bdb_info *info,
                               const lListElem *object, 
                               u_long32 job_id, u_long32 ja_task_id,
                               const char *pe_task_id)
{
   bool ret = true;
   dstring dbkey_dstring;
   char dbkey_buffer[MAX_STRING_SIZE];
   const char *dbkey;

   sge_dstring_init(&dbkey_dstring, 
                    dbkey_buffer, sizeof(dbkey_buffer));

   dbkey = sge_dstring_sprintf(&dbkey_dstring, "%s:%8d.%8d %s", 
                               object_type_get_name(SGE_TYPE_PETASK),
                               job_id, ja_task_id, pe_task_id);

   ret = spool_berkeleydb_write_object(answer_list, info, 
                                       object, dbkey);

   return ret;
}

bool
spool_berkeleydb_write_ja_task(lList **answer_list, struct bdb_info *info,
                               const lListElem *object, 
                               u_long32 job_id, u_long32 ja_task_id)
{
   bool ret = true;
   dstring dbkey_dstring;
   char dbkey_buffer[MAX_STRING_SIZE];
   const char *dbkey;
   lList *tmp_list = NULL;

   sge_dstring_init(&dbkey_dstring,
                    dbkey_buffer, sizeof(dbkey_buffer));

   dbkey = sge_dstring_sprintf(&dbkey_dstring, "%s:%8d.%8d", 
                               object_type_get_name(SGE_TYPE_JATASK),
                               job_id, ja_task_id);

   lXchgList((lListElem *)object, JAT_task_list, &tmp_list);
   ret = spool_berkeleydb_write_object(answer_list, info, 
                                       object, dbkey);
   lXchgList((lListElem *)object, JAT_task_list, &tmp_list);

   /* JG: TODO: do we have to spool all petasks?
    */

   return ret;
}

bool
spool_berkeleydb_write_job(lList **answer_list, struct bdb_info *info,
                           const lListElem *object, 
                           u_long32 job_id, bool only_job)
{
   bool ret = true;
   dstring dbkey_dstring;
   char dbkey_buffer[MAX_STRING_SIZE];
   const char *dbkey;
   lList *tmp_list = NULL;

   sge_dstring_init(&dbkey_dstring,
                    dbkey_buffer, sizeof(dbkey_buffer));

   dbkey = sge_dstring_sprintf(&dbkey_dstring, "%s:%8d", 
                               object_type_get_name(SGE_TYPE_JOB), 
                               job_id);

   lXchgList((lListElem *)object, JB_ja_tasks, &tmp_list);
   
   ret = spool_berkeleydb_write_object(answer_list, info, 
                                       object, dbkey);

   lXchgList((lListElem *)object, JB_ja_tasks, &tmp_list);

   if (ret && !only_job) {
      lListElem *ja_task;
      for_each(ja_task, lGetList(object, JB_ja_tasks)) {
         ret = spool_berkeleydb_write_ja_task(answer_list, info,
                                              ja_task,
                                              job_id, 
                                              lGetUlong(ja_task, 
                                                        JAT_task_number));
         if (!ret) {
            break;
         }
      }
   }

   return ret;
}

bool
spool_berkeleydb_write_cqueue(lList **answer_list, struct bdb_info *info, 
                              const lListElem *object, const char *key)
{
   bool ret = true;
   dstring dbkey_dstring;
   char dbkey_buffer[MAX_STRING_SIZE];
   const char *dbkey;
   lList *tmp_list = NULL;

   sge_dstring_init(&dbkey_dstring,
                    dbkey_buffer, sizeof(dbkey_buffer));

   dbkey = sge_dstring_sprintf(&dbkey_dstring, "%s:%s", 
                               object_type_get_name(SGE_TYPE_CQUEUE), 
                               key);

   lXchgList((lListElem *)object, CQ_qinstances, &tmp_list);
   
   ret = spool_berkeleydb_write_object(answer_list, info, 
                                       object, dbkey);

   lXchgList((lListElem *)object, CQ_qinstances, &tmp_list);

   return ret;
}

/****** spool/berkeleydb/spool_berkeleydb_delete_object() **********************
*  NAME
*     spool_berkeleydb_delete_object() -- delete one or multiple objects
*
*  SYNOPSIS
*     bool 
*     spool_berkeleydb_delete_object(lList **answer_list, struct bdb_info *info,
*                                    const char *key, bool sub_objects) 
*
*  FUNCTION
*     If sub_objects = false, deletes the object specified by key.
*     If sub_objects = true, key will be used as pattern to delete multiple
*     objects.
*
*  INPUTS
*     lList **answer_list   - used to return error messages
*     struct bdb_info *info - database handle
*     const char *key       - key
*     bool sub_objects      - use key as pattern?
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     MT-NOTE: spool_berkeleydb_delete_object() is MT safe 
*******************************************************************************/
bool
spool_berkeleydb_delete_object(lList **answer_list, struct bdb_info *info, 
                               const char *key, bool sub_objects)
{
   bool ret = true;

   int dbret;
   DBT cursor_dbt, delete_dbt, data_dbt;

   DB *db;
   DB_TXN *txn;

   DENTER(TOP_LAYER, "spool_berkeleydb_delete_object");

   db = bdb_get_db(info);
   txn = bdb_get_txn(info);

   if (db == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_NOCONNECTIONOPEN_S,
                              "");
      ret = false;
   } else {
      if (sub_objects) {
         DBC *dbc;

         DPRINTF(("querying objects with keys %s*\n", key));

         PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         db->cursor(db, txn, &dbc, 0);
         PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         /* initialize query to first record for this object type */
         memset(&cursor_dbt, 0, sizeof(cursor_dbt));
         memset(&data_dbt, 0, sizeof(data_dbt));
         cursor_dbt.data = (void *)key;
         cursor_dbt.size = strlen(key) + 1;
         PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         dbret = dbc->c_get(dbc, &cursor_dbt, &data_dbt, DB_SET_RANGE);
         PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         while (true) {
            if (dbret != 0 && dbret != DB_NOTFOUND) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_BERKELEY_QUERYERROR_SS,
                                       key,
                                       db_strerror(dbret));
               ret = false;
               break;
            } else if (dbret == DB_NOTFOUND) {
               DPRINTF(("last record reached\n"));
               break;
            } else if (cursor_dbt.data != NULL && 
                       strncmp(cursor_dbt.data, key, strlen(key)) 
                       != 0) {
               DPRINTF(("current key is %s\n", cursor_dbt.data));
               DPRINTF(("last record of this object type reached\n"));
               break;
            } else {
               int delete_ret;

               /* remember key of record to delete */
               memset(&delete_dbt, 0, sizeof(delete_dbt));
               delete_dbt.data = strdup(cursor_dbt.data);
               delete_dbt.size = cursor_dbt.size;

               /* switch cursor to next position */
               memset(&cursor_dbt, 0, sizeof(cursor_dbt));
               memset(&data_dbt, 0, sizeof(data_dbt));
               PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
               dbret = dbc->c_get(dbc, &cursor_dbt, &data_dbt, DB_NEXT);
               PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);

               /* delete record with stored key */
               PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
               /* JG: TODO: use db->c_del instead? */
               delete_ret = db->del(db, txn, &delete_dbt, 0);
               PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
               if (delete_ret != 0) {
                  answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                          ANSWER_QUALITY_ERROR, 
                                          MSG_BERKELEY_DELETEERROR_SS,
                                          delete_dbt.data,
                                          db_strerror(delete_ret));
                  ret = false;
                  free(delete_dbt.data);
                  break;
               } else {
                  DPRINTF(("deleted record with key "SFQ"\n", delete_dbt.data));
               }
               free(delete_dbt.data);
            }
         }
         PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         dbc->c_close(dbc);
         PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      } else {
         memset(&cursor_dbt, 0, sizeof(cursor_dbt));
         memset(&data_dbt, 0, sizeof(data_dbt));
         cursor_dbt.data = (void *)key;
         cursor_dbt.size = strlen(key) + 1;
         PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         dbret = db->del(db, txn, &cursor_dbt, 0);
         PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         if (dbret != 0 && dbret != DB_NOTFOUND) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_BERKELEY_DELETEERROR_SS,
                                    key,
                                    db_strerror(dbret));
            ret = false;
         } else {
            DPRINTF(("deleted record with key "SFQ"\n", key));
         }
      }
   }

   DEXIT;
   return ret;
}

/****** spool/berkeleydb/spool_berkeleydb_delete_pe_task() *********************
*  NAME
*     spool_berkeleydb_delete_pe_task() -- delete one or multiple pe task(s)
*
*  SYNOPSIS
*     bool 
*     spool_berkeleydb_delete_pe_task(lList **answer_list, struct bdb_info *info,
*                                     const char *key, bool sub_objects) 
*
*  FUNCTION
*     Deletes one or multiple pe_tasks specified by key.
*  
*     The key has the form "<job_id>.<ja_task_id> <pe_task_id>" formatted as 
*     "%8d.%8d %s".
*     If sub_objects = true, it can be used as pattern, typically used to 
*     delete all pe_tasks of a certain ja_task by setting key to 
*     "<job_id>.<ja_task_id>" or just "<job_id>" to delete all pe_tasks
*     dependent on a certain job.
*
*  INPUTS
*     lList **answer_list   - used to return error messages
*     struct bdb_info *info - database handle
*     const char *key       - key
*     bool sub_objects      - interpret key as pattern?
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     MT-NOTE: spool_berkeleydb_delete_pe_task() is MT safe 
*
*  SEE ALSO
*     spool/berkeleydb/spool_berkeleydb_delete_object()
*******************************************************************************/
bool
spool_berkeleydb_delete_pe_task(lList **answer_list, struct bdb_info *info,
                                const char *key, bool sub_objects)
{
   bool ret = true;

   dstring dbkey_dstring;
   char dbkey_buffer[MAX_STRING_SIZE];
   const char *dbkey;
   const char *table_name;

   sge_dstring_init(&dbkey_dstring, dbkey_buffer, sizeof(dbkey_buffer));
   table_name = object_type_get_name(SGE_TYPE_PETASK);
   dbkey = sge_dstring_sprintf(&dbkey_dstring, "%s:%s", table_name, key);
   ret = spool_berkeleydb_delete_object(answer_list, info, dbkey, sub_objects);

   return ret;
}

/****** spool/berkeleydb/spool_berkeleydb_delete_ja_task() *********************
*  NAME
*     spool_berkeleydb_delete_ja_task() -- delete ja_task(s)
*
*  SYNOPSIS
*     bool 
*     spool_berkeleydb_delete_ja_task(lList **answer_list, struct bdb_info *info,
*                                     const char *key, bool sub_objects) 
*
*  FUNCTION
*     Deletes one or multiple ja_tasks specified by key.
*     The ja_task(s) and all dependent pe_tasks are deleted.
*  
*     The key has the form "<job_id>.<ja_task_id>" formatted as "%8d.%8d".
*     If sub_objects = true, it can be used as pattern, typically used to 
*     delete all ja_tasks of a certain job by setting key to "<job_id>.".
*
*  INPUTS
*     lList **answer_list   - used to return error messages
*     struct bdb_info *info - database handle
*     const char *key       - key
*     bool sub_objects      - use key as pattern?
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     MT-NOTE: spool_berkeleydb_delete_ja_task() is MT safe 
*
*  SEE ALSO
*     spool/berkeleydb/spool_berkeleydb_delete_object()
*     spool/berkeleydb/spool_berkeleydb_delete_pe_task()
*******************************************************************************/
bool
spool_berkeleydb_delete_ja_task(lList **answer_list, struct bdb_info *info,
                                const char *key, bool sub_objects)
{
   bool ret = true;

   dstring dbkey_dstring;
   char dbkey_buffer[MAX_STRING_SIZE];
   const char *dbkey;
   const char *table_name;

   sge_dstring_init(&dbkey_dstring, dbkey_buffer, sizeof(dbkey_buffer));
   table_name = object_type_get_name(SGE_TYPE_JATASK);
   dbkey = sge_dstring_sprintf(&dbkey_dstring, "%s:%s", table_name, key);
   ret = spool_berkeleydb_delete_object(answer_list, info, dbkey, sub_objects);

   if (ret) {
      ret = spool_berkeleydb_delete_pe_task(answer_list, info, key, true);
   }

   return ret;
}

/****** spool/berkeleydb/spool_berkeleydb_delete_job() *************************
*  NAME
*     spool_berkeleydb_delete_job() -- delete a job
*
*  SYNOPSIS
*     bool 
*     spool_berkeleydb_delete_job(lList **answer_list, struct bdb_info *info, 
*                                 const char *key, bool sub_objects) 
*
*  FUNCTION
*     Deletes the given job and all its ja_tasks.
*     Key usually will be the unique job id formatted with %8d, but the function
*     allows for some sort of pattern matching by specifying only parts of the
*     jobid, e.g. the key "00001" will delete all jobs from 1000 to 1999, 
*     an empty string will mean "delete all jobs", if sub_objects = true.
*
*  INPUTS
*     lList **answer_list   - used to return error messages
*     struct bdb_info *info - database handle
*     const char *key       - key (job_number)
*     bool sub_objects      - is the given key a pattern?
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     MT-NOTE: spool_berkeleydb_delete_job() is MT safe 
*
*  SEE ALSO
*     spool/berkeleydb/spool_berkeleydb_delete_object()
*     spool/berkeleydb/spool_berkeleydb_delete_ja_task()
*******************************************************************************/
bool
spool_berkeleydb_delete_job(lList **answer_list, struct bdb_info *info,
                            const char *key, bool sub_objects)
{
   bool ret = true;

   dstring dbkey_dstring;
   char dbkey_buffer[MAX_STRING_SIZE];
   const char *dbkey;
   const char *table_name;

   sge_dstring_init(&dbkey_dstring, dbkey_buffer, sizeof(dbkey_buffer));
   table_name = object_type_get_name(SGE_TYPE_JOB);
   dbkey = sge_dstring_sprintf(&dbkey_dstring, "%s:%s", table_name, key);
   ret = spool_berkeleydb_delete_object(answer_list, info, dbkey, sub_objects);

   if (ret) {
      ret = spool_berkeleydb_delete_ja_task(answer_list, info, key, true);
   }

   return ret;
}

/****** spool/berkeleydb/spool_berkeleydb_delete_cqueue() **********************
*  NAME
*     spool_berkeleydb_delete_cqueue() -- delete a cluster queue
*
*  SYNOPSIS
*     bool 
*     spool_berkeleydb_delete_cqueue(lList **answer_list, struct bdb_info *info,
*                                    const char *key) 
*
*  FUNCTION
*     Deletes a cluster queue and all its queue instances.
*
*  INPUTS
*     lList **answer_list   - used to return error messages
*     struct bdb_info *info - database handle
*     const char *key       - key (name) of cluster queue to delete
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     MT-NOTE: spool_berkeleydb_delete_cqueue() is MT safe 
*
*  SEE ALSO
*     spool/berkeleydb/spool_berkeleydb_delete_object()
*******************************************************************************/
bool
spool_berkeleydb_delete_cqueue(lList **answer_list, struct bdb_info *info,
                               const char *key)
{
   bool ret = true;

   dstring dbkey_dstring;
   char dbkey_buffer[MAX_STRING_SIZE];
   const char *dbkey;
   const char *table_name;

   sge_dstring_init(&dbkey_dstring, dbkey_buffer, sizeof(dbkey_buffer));
   table_name = object_type_get_name(SGE_TYPE_CQUEUE);
   dbkey = sge_dstring_sprintf(&dbkey_dstring, "%s:%s", table_name, key);
   ret = spool_berkeleydb_delete_object(answer_list, info, dbkey, false);

   if (ret) {
      table_name = object_type_get_name(SGE_TYPE_QINSTANCE);
      dbkey = sge_dstring_sprintf(&dbkey_dstring, "%s:%s@", table_name, key);
      ret = spool_berkeleydb_delete_object(answer_list, info, dbkey, true);
   }

   return ret;
}

