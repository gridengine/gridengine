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
#include <time.h>

#include <db.h>

#include "sgermon.h"
#include "sge_log.h"

#include "sge_answer.h"
#include "sge_dstring.h"
#include "sge_profiling.h"
#include "sge_unistd.h"

#include "sge_object.h"

#include "sge_host.h"
#include "sge_job.h"
#include "sge_ja_task.h"
#include "sge_pe_task.h"

#include "spool/sge_spooling_utilities.h"
#include "spool/sge_spooling_database.h"

#include "msg_common.h"
#include "spool/msg_spoollib.h"
#include "spool/berkeleydb/msg_spoollib_berkeleydb.h"

#include "spool/berkeleydb/sge_spooling_berkeleydb.h"

/* how often will the transaction log be cleared */
#define BERKELEYDB_CLEAR_INTERVAL 300

/* how often will the database be checkpointed (cache written to disk) */
#define BERKELEYDB_CHECKPOINT_INTERVAL 60

static const char *spooling_method = "berkeleydb";

const char *get_spooling_method(void)
{
   return spooling_method;
}

typedef struct bdb_info {
   DB_ENV *env;
   DB     *db;
   DB_TXN *txn;
   time_t last_clear;
   time_t last_checkpoint;
} bdb_info;

bdb_info *bdb_create(void) {
   bdb_info *info = (bdb_info *) malloc(sizeof(bdb_info));

   info->env = NULL;
   info->db  = NULL;
   info->txn = NULL;
   info->last_clear = 0;
   info->last_checkpoint = 0;

   return info;
}

/****** spool/berkeleydb/spool_berkeleydb_create_context() ********************
*  NAME
*     spool_berkeleydb_create_context() -- create a berkeleydb spooling context
*
*  SYNOPSIS
*     lListElem* 
*     spool_berkeleydb_create_context(lList **answer_list, const char *args)
*
*  FUNCTION
*     Create a spooling context for the berkeleydb spooling.
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
*     spool/berkeleydb/--BerkeleyDB-Spooling
*******************************************************************************/
lListElem *
spool_berkeleydb_create_context(lList **answer_list, const char *args)
{
   lListElem *context = NULL;

   DENTER(TOP_LAYER, "spool_berkeleydb_create_context");

   /* check input parameter (*/
   if (args == NULL) {
/*      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_POSTGRES_INVALIDARGSTOCREATESPOOLINGCONTEXT); */
   } else {
      lListElem *rule, *type;
      bdb_info *db;
      
      /* create spooling context */
      context = spool_create_context(answer_list, "berkeleydb spooling");
      
      /* create rule and type for all objects spooled in the spool dir */
      rule = spool_context_create_rule(answer_list, context, 
                                       "default rule", 
                                       args,
                                       spool_berkeleydb_default_startup_func,
                                       spool_berkeleydb_default_shutdown_func,
                                       spool_berkeleydb_default_maintenance_func,
                                       spool_berkeleydb_default_trigger_func,
                                       NULL,
                                       spool_berkeleydb_default_list_func,
                                       spool_berkeleydb_default_read_func,
                                       spool_berkeleydb_default_write_func,
                                       spool_berkeleydb_default_delete_func,
                                       spool_default_validate_func,
                                       spool_default_validate_list_func);

      db = bdb_create();
      lSetRef(rule, SPR_clientdata, db);
      type = spool_context_create_type(answer_list, context, SGE_TYPE_ALL);
      spool_type_add_rule(answer_list, type, rule, true);
   }

   DEXIT;
   return context;
}

bool spool_berkeleydb_create_environment(lList **answer_list, 
                                         bdb_info *db, const char *url)
{ 
   bool ret = true;
   int dbret;

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
      dbret = db_env_create(&(db->env), 0);
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
         dbret = db->env->set_lk_detect(db->env, DB_LOCK_DEFAULT);
         PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         if (dbret != 0) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_BERKELEY_COULDNTESETUPLOCKDETECTION_S,
                                    db_strerror(dbret));
            ret = false;
         } else {
            PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
            dbret = db->env->open(db->env, url, 
                              DB_PRIVATE /* single process has access */
                              | DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG | 
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
         }
      }
   }   

   DEXIT;
   return ret;
}

/****** spool/berkeleydb/spool_berkeleydb_default_startup_func() **************
*  NAME
*     spool_berkeleydb_default_startup_func() -- setup 
*
*  SYNOPSIS
*     bool 
*     spool_berkeleydb_default_startup_func(lList **answer_list, 
*                                         const char *args, bool check)
*
*  FUNCTION
*
*  INPUTS
*     lList **answer_list   - to return error messages
*     const lListElem *rule - the rule containing data necessary for
*                             the startup (e.g. path to the spool directory)
*     bool check            - check the spooling database
*
*  RESULT
*     bool - true, if the startup succeeded, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/berkeleydb/--BerkeleyDB-Spooling
*     spool/spool_startup_context()
*******************************************************************************/
bool
spool_berkeleydb_default_startup_func(lList **answer_list, 
                                      const lListElem *rule, bool check)
{
   bool ret = true;
   const char *url;

   bdb_info *db;

   int dbret;

   DENTER(TOP_LAYER, "spool_berkeleydb_default_startup_func");

   url = lGetString(rule, SPR_url);
   db = (bdb_info *)lGetRef(rule, SPR_clientdata);

   ret = spool_berkeleydb_create_environment(answer_list, db, url);

   if (ret) {
      PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      dbret = db_create(&(db->db), db->env, 0);
      PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      if (dbret != 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_BERKELEY_COULDNTCREATEDBHANDLE_S,
                                 db_strerror(dbret));
         ret = false;
         db->db = NULL;
      } else {
         if (check) {
#if 0
            PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
            /* Cache sizes: default: 256K    262144
             *                         1M   1048576
             *                         4M   4194304
             *                        16M  16777216
             */
            dbret = db->db->set_cachesize(db->db, 0, 1048576, 0);
            PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
            if (dbret != 0) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_BERKELEY_COULDNTSETCACHE_SS,
                                       url, db_strerror(dbret));
               ret = false;
            /* if check = false, we needn't open the databases - we are in 
             * maintenance mode.
             */
            } else {
#endif
               PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
               dbret = db->db->open(db->db, "sge", NULL, DB_BTREE, DB_THREAD, 0);
               PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
               if (dbret != 0) {
                  answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                          ANSWER_QUALITY_ERROR, 
                                          MSG_BERKELEY_COULDNTOPENDB_SS,
                                          url, db_strerror(dbret));
                  ret = false;
               }
#if 0
            }
#endif
         }
      }
   }

#if 1
   prof_set_level_name(SGE_PROF_CUSTOM0, "packing", NULL);
#endif

   DEXIT;
   return ret;
}

/****** spool/berkeleydb/spool_berkeleydb_default_shutdown_func() **************
*  NAME
*     spool_berkeleydb_default_shutdown_func() -- shutdown spooling context
*
*  SYNOPSIS
*     bool 
*     spool_berkeleydb_default_shutdown_func(lList **answer_list, 
*                                          lListElem *rule);
*
*  FUNCTION
*     Shuts down the context, e.g. the database connection.
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
*     spool/berkeleydb/--Spooling-BerkeleyDB
*     spool/spool_shutdown_context()
*******************************************************************************/
bool
spool_berkeleydb_default_shutdown_func(lList **answer_list, 
                                    const lListElem *rule)
{
   bool ret = true;
   const char *url;

   bdb_info *db;

   DENTER(TOP_LAYER, "spool_berkeleydb_default_shutdown_func");

   url = lGetString(rule, SPR_url);
   db = (bdb_info *)lGetRef(rule, SPR_clientdata);

   if (db == NULL || db->env == NULL || db->db == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_NOCONNECTIONOPEN_S,
                              url);
      ret = false;
   } else {
      int dbret;
      PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      dbret = db->db->close(db->db, 0);
      PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      db->db = NULL;
      if (dbret != 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_BERKELEY_COULDNTCLOSEDB_SS,
                                 url, db_strerror(dbret));
         ret = false;
      }  else {
         PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         dbret = db->env->close(db->env, 0);
         PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         db->env = NULL;
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
   }

   DEXIT;
   return ret;
}

/****** spool/berkeleydb/spool_berkeleydb_default_maintenance_func() ************
*  NAME
*     spool_berkeleydb_default_maintenance_func() -- maintain database
*
*  SYNOPSIS
*     bool 
*     spool_berkeleydb_default_maintenance_func(lList **answer_list, 
*                                    lListElem *rule
*                                    const spooling_maintenance_command cmd,
*                                    const char *args);
*
*  FUNCTION
*     Maintains the database:
*        - initialization
*        - ...
*
*  INPUTS
*     lList **answer_list   - to return error messages
*     const lListElem *rule - the rule containing data necessary for
*                             the maintenance (e.g. path to the spool 
*                             directory)
*     const spooling_maintenance_command cmd - the command to execute
*     const char *args      - arguments to the maintenance command
*
*  RESULT
*     bool - true, if the maintenance succeeded, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/berkeleydb/--Spooling-BerkeleyDB
*     spool/spool_maintain_context()
*******************************************************************************/
bool
spool_berkeleydb_default_maintenance_func(lList **answer_list, 
                                    const lListElem *rule, 
                                    const spooling_maintenance_command cmd,
                                    const char *args)
{
   bool ret = true;
   const char *url;

   bdb_info *db;
   int dbret;

   DENTER(TOP_LAYER, "spool_berkeleydb_default_maintenance_func");

   url = lGetString(rule, SPR_url);
   db = (bdb_info *)lGetRef(rule, SPR_clientdata);

   switch (cmd) {
      case SPM_init:
         PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         dbret = db->db->open(db->db, "sge", NULL, DB_BTREE, 
                                    DB_CREATE | DB_THREAD, S_IRUSR | S_IWUSR);
         PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         if (dbret != 0) {
           answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                   ANSWER_QUALITY_ERROR, 
                                   MSG_BERKELEY_COULDNTCREATEDB_SS,
                                   url, db_strerror(dbret));
            ret = false;
         }
         break;
      default:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 "unknown maintenance command %d\n", cmd);
         ret = false;
         break;
         
   }

   DEXIT;
   return ret;
}

bool
spool_berkeleydb_clear_log(lList **answer_list, bdb_info *db)
{
   bool ret = true;
   int dbret;
   char **list;

   DENTER(TOP_LAYER, "spool_berkeleydb_clear_log");

   PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
   dbret = db->env->log_archive(db->env, &list, DB_ARCH_ABS);
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
               answer_list_add_sprintf(answer_list, STATUS_EDISK, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_ERRORDELETINGFILE_SS,
                                       *file,
                                       strerror(errno));
               ret = false;
               break;
            }
         }
         free(list);
      }
   }

   DEXIT;
   return ret;
}

bool
spool_berkeleydb_checkpoint(lList **answer_list, bdb_info *db)
{
   bool ret = true;
   int dbret;

   DENTER(TOP_LAYER, "spool_berkeleydb_clear_log");

   PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
   dbret = db->env->txn_checkpoint(db->env, 0, 0, 0);
   PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
   if (dbret != 0) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_CANNOTCHECKPOINT_S,
                              db_strerror(dbret));
      ret = false;
   } 

   DEXIT;
   return ret;
}

bool
spool_berkeleydb_default_trigger_func(lList **answer_list, const lListElem *rule)
{
   bool ret = true;

   bdb_info *db;

   DENTER(TOP_LAYER, "spool_berkeleydb_default_trigger_func");

   db = (bdb_info *)lGetRef(rule, SPR_clientdata);
   if (db == NULL || db->env == NULL || db->db == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_WARNING, 
                              MSG_BERKELEY_NOCONNECTIONOPEN_S,
                              lGetString(rule, SPR_url));
      ret = false;
   } else {
      time_t now = time(0);
      if ((db->last_clear + BERKELEYDB_CLEAR_INTERVAL) <= now) {
         ret = spool_berkeleydb_clear_log(answer_list, db);
         if (ret) {
            db->last_clear = now;
         }
      }
      if ((db->last_checkpoint + BERKELEYDB_CHECKPOINT_INTERVAL <= now)) {
         ret = spool_berkeleydb_checkpoint(answer_list, db);
         if (ret) {
            db->last_checkpoint = now;
         }
      }
   }
   
   DEXIT;
   return ret;
}

static bool
spool_berkeleydb_start_transaction(lList **answer_list, bdb_info *db)
{
   bool ret = true;
   int dbret;

   DENTER(TOP_LAYER, "spool_berkeleydb_start_transaction");

   if (db->txn != NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_TXNALREADYOPEN);
      ret = false;
   } else {
      PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      dbret = db->env->txn_begin(db->env, NULL, &(db->txn), 0);
      PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      if (dbret != 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_BERKELEY_ERRORSTARTINGTRANSACTION_S,
                                 db_strerror(dbret));
         ret = false;
         db->txn = NULL;
      }
   }

   DEXIT;
   return ret;
}

static bool
spool_berkeleydb_end_transaction(lList **answer_list, bdb_info *db, 
                                 bool commit)
{
   bool ret = true;
   int dbret;

   DENTER(TOP_LAYER, "spool_berkeleydb_end_transaction");

   if (db->txn == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_TXNNOTOPEN);
      ret = false;
   } else {
      if (commit) {
         PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         dbret = db->txn->commit(db->txn, 0);
         PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      } else {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_BERKELEY_ABORTINGTRANSACTION);
         PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         dbret = db->txn->abort(db->txn);
         PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      }

      db->txn = NULL;

      if (dbret != 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_BERKELEY_ERRORENDINGTRANSACTION_S,
                                 db_strerror(dbret));
         ret = false;
      }
   }

   DEXIT;
   return ret;
}

static bool 
spool_berkeleydb_read_list(lList **answer_list, bdb_info *db,
                           lList **list, const lDescr *descr,
                           const char *key)
{
   bool ret = true;
   int dbret;

   sge_pack_buffer pb;
   DBT key_dbt, data_dbt;
   DBC *dbc;

   DPRINTF(("querying objects with keys %s*\n", key));

   PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
   db->db->cursor(db->db, db->txn, &dbc, 0);
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
         lListElem *object;
         DPRINTF(("read object with key "SFQ"\n", key_dbt.data));
         PROF_START_MEASUREMENT(SGE_PROF_CUSTOM0);
         init_packbuffer_from_buffer(&pb, data_dbt.data, data_dbt.size, 0);
         cull_unpack_elem(&pb, &object, descr);
         /* we may not free the packbuffer: it references the buffer
          * delivered from the database
          * clear_packbuffer(&pb);
          */
         PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM0);
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

   return ret;
}

/****** spool/berkeleydb/spool_berkeleydb_default_list_func() *****************
*  NAME
*     spool_berkeleydb_default_list_func() -- read lists through berkeleydb spooling
*
*  SYNOPSIS
*     bool 
*     spool_berkeleydb_default_list_func(
*                                      lList **answer_list, 
*                                      const lListElem *type, 
*                                      const lListElem *rule, lList **list, 
*                                      const sge_object_type object_type) 
*
*  FUNCTION
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to be used 
*     lList **list                    - target list
*     const sge_object_type object_type - object type
*
*  RESULT
*     bool - true, on success, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/berkeleydb/--BerkeleyDB-Spooling
*     spool/spool_read_list()
*******************************************************************************/
bool
spool_berkeleydb_default_list_func(lList **answer_list, 
                                 const lListElem *type, 
                                 const lListElem *rule, lList **list, 
                                 const sge_object_type object_type)
{
   bool ret = true;
#if 0
   bool local_transaction = false; /* did we start a transaction? */
#endif

   const lDescr *descr;
   const char *table_name;

   bdb_info *db;

   DENTER(TOP_LAYER, "spool_berkeleydb_default_list_func");

   db = (bdb_info *)lGetRef(rule, SPR_clientdata);
   descr = object_type_get_descr(object_type);
   table_name = object_type_get_name(object_type);

   if (db == NULL || db->env == NULL || db->db == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_WARNING, 
                              MSG_BERKELEY_NOCONNECTIONOPEN_S,
                              lGetString(rule, SPR_url));
      ret = false;
   } else if (descr == NULL || list == NULL ||
              table_name == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_WARNING, 
                              MSG_SPOOL_SPOOLINGOFXNOTSUPPORTED_S, 
                              object_type_get_name(object_type));
      ret = false;
   } else {
      /* if no transaction was opened from outside, open a new one */
#if 0
   /* JG: TODO: why does reading within a transaction give me the error:
    *           Not enough space
    */
      if (db->txn == NULL) {
         ret = spool_berkeleydb_start_transaction(answer_list, db);
         if (ret) {
            local_transaction = true;
         }
      }
#endif
      if (ret) {
         switch (object_type) {
            case SGE_TYPE_JATASK:
            case SGE_TYPE_PETASK:
               break;
            case SGE_TYPE_JOB:
               {
                  lListElem *job;
                  dstring key_dstring;
                  char key_buffer[MAX_STRING_SIZE];
                  const char *key;

                  sge_dstring_init(&key_dstring, key_buffer, sizeof(key_buffer));
                 
                  /* read all jobs */
                  ret = spool_berkeleydb_read_list(answer_list, db, 
                                                   list, descr,
                                                   table_name);
                  if (ret) {
                     const char *ja_task_table;
                     /* for all jobs: read ja_tasks */
                     ja_task_table = object_type_get_name(SGE_TYPE_JATASK);
                     for_each(job, *list) {
                        lList *task_list = NULL;
                        u_long32 job_id = lGetUlong(job, JB_job_number);

                        key = sge_dstring_sprintf(&key_dstring, "%s:%8d.",
                                                  ja_task_table,
                                                  job_id);
                        ret = spool_berkeleydb_read_list(answer_list, db,
                                                         &task_list, JAT_Type,
                                                         key);
                        /* reading ja_tasks succeeded */
                        if (ret) {
                           /* we actually found ja_tasks for this job */
                           if (task_list != NULL) {
                              lListElem *ja_task;
                              const char *pe_task_table;

                              lSetList(job, JB_ja_tasks, task_list);
                              pe_task_table = object_type_get_name(SGE_TYPE_PETASK);

                              for_each(ja_task, task_list) {
                                 lList *pe_task_list = NULL;
                                 u_long32 ja_task_id = lGetUlong(ja_task, 
                                                                 JAT_task_number);
                                 key = sge_dstring_sprintf(&key_dstring, "%s:%8d.%8d.",
                                                           pe_task_table,
                                                           job_id, ja_task_id);
                                 
                                 ret = spool_berkeleydb_read_list(answer_list, db,
                                                         &pe_task_list, PET_Type,
                                                         key);
                                 if (ret) {
                                    if (pe_task_list != NULL) {
                                       lSetList(ja_task, JAT_task_list, pe_task_list);
                                    }
                                 } else {
                                    break;
                                 }
                              }
                           }
                        }

                        if (!ret) {
                           break;
                        }
                     }
                  }
               }
               break;
            default:
               ret = spool_berkeleydb_read_list(answer_list, db, 
                                                list, descr,
                                                table_name);
               break;
         }
#if 0
         /* spooling is done, now end the transaction, if we have an own one */
         if (local_transaction) {
            ret = spool_berkeleydb_end_transaction(answer_list, db, ret);
         }
#endif
      }
   }

   if (ret) {
      lListElem *ep;
      spooling_validate_func validate = 
         (spooling_validate_func)lGetRef(rule, SPR_validate_func);
      spooling_validate_list_func validate_list = 
         (spooling_validate_list_func)lGetRef(rule, SPR_validate_list_func);

      /* validate each individual object */
      /* JG: TODO: is it valid to validate after reading all objects? */
      for_each(ep, *list) {
         ret = validate(answer_list, type, rule, ep, object_type);
         if (!ret) {
            /* error message has been created in the validate func */
            break;
         }
      }

      if (ret) {
         /* validate complete list */
         ret = validate_list(answer_list, type, rule, object_type);
      }
   }



   DEXIT;
   return ret;
}

/****** spool/berkeleydb/spool_berkeleydb_default_read_func() *****************
*  NAME
*     spool_berkeleydb_default_read_func() -- read objects through berkeleydb spooling
*
*  SYNOPSIS
*     lListElem* 
*     spool_berkeleydb_default_read_func(lList **answer_list, 
*                                      const lListElem *type, 
*                                      const lListElem *rule, const char *key, 
*                                      const sge_object_type object_type) 
*
*  FUNCTION
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const char *key                 - unique key specifying the object
*     const sge_object_type object_type - object type
*
*  RESULT
*     lListElem* - the object, if it could be read, else NULL
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/berkeleydb/--BerkeleyDB-Spooling
*     spool/spool_read_object()
*******************************************************************************/
lListElem *
spool_berkeleydb_default_read_func(lList **answer_list, 
                                 const lListElem *type, 
                                 const lListElem *rule, const char *key, 
                                 const sge_object_type object_type)
{
   lListElem *ep = NULL;

   bdb_info *db;

   DENTER(TOP_LAYER, "spool_berkeleydb_default_read_func");

   db = (bdb_info *)lGetRef(rule, SPR_clientdata);

   switch (object_type) {
      default:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_SPOOL_SPOOLINGOFXNOTSUPPORTED_S, 
                                 object_type_get_name(object_type));
         break;
   }

   DEXIT;
   return ep;
}

static bool 
spool_berkeleydb_write_object(lList **answer_list, bdb_info *db,
                              const lListElem *object, const char *key)
{
   bool ret = true;
   sge_pack_buffer pb;
   DBT key_dbt, data_dbt;
   int dbret;

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM0);
   init_packbuffer(&pb, 8192, 0);
   cull_pack_elem(&pb, object);
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM0);
   
   memset(&key_dbt, 0, sizeof(key_dbt));
   memset(&data_dbt, 0, sizeof(data_dbt));
   key_dbt.data = (void *)key;
   key_dbt.size = strlen(key) + 1;
   data_dbt.data = pb.head_ptr;
   data_dbt.size = pb.bytes_used;

   /* Store a key/data pair. */
   PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
   dbret = db->db->put(db->db, db->txn, &key_dbt, &data_dbt, 0);
   PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
   if (dbret != 0) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_BERKELEY_PUTERROR_SS,
                              key, db_strerror(dbret));
      ret = false;
   } else {
      DPRINTF(("stored object with key "SFQ", size = %d\n", key, 
               data_dbt.size));
   }

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM0);
   clear_packbuffer(&pb);
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM0);

   return ret;
}

static bool
spool_berkeleydb_write_pe_task(lList **answer_list, bdb_info *db,
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

   ret = spool_berkeleydb_write_object(answer_list, db, 
                                       object, dbkey);

   return ret;
}

static bool
spool_berkeleydb_write_ja_task(lList **answer_list, bdb_info *db,
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

   ret = spool_berkeleydb_write_object(answer_list, db, 
                                       object, dbkey);

   lXchgList((lListElem *)object, JAT_task_list, &tmp_list);

   /* JG: TODO: do we have to spool all petasks?
    */

   return ret;
}

static bool
spool_berkeleydb_write_job(lList **answer_list, bdb_info *db,
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
   
   ret = spool_berkeleydb_write_object(answer_list, db, 
                                       object, dbkey);

   lXchgList((lListElem *)object, JB_ja_tasks, &tmp_list);

   if (ret && !only_job) {
      lListElem *ja_task;
      for_each(ja_task, lGetList(object, JB_ja_tasks)) {
         ret = spool_berkeleydb_write_ja_task(answer_list, db,
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

/****** spool/berkeleydb/spool_berkeleydb_default_write_func() ****************
*  NAME
*     spool_berkeleydb_default_write_func() -- write objects through berkeleydb spooling
*
*  SYNOPSIS
*     bool
*     spool_berkeleydb_default_write_func(lList **answer_list, 
*                                       const lListElem *type, 
*                                       const lListElem *rule, 
*                                       const lListElem *object, 
*                                       const char *key, 
*                                       const sge_object_type object_type) 
*
*  FUNCTION
*     Writes an object through the appropriate berkeleydb spooling functions.
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const lListElem *object         - object to spool
*     const char *key                 - unique key
*     const sge_object_type object_type - object type
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/berkeleydb/--BerkeleyDB-Spooling
*     spool/spool_delete_object()
*******************************************************************************/
bool
spool_berkeleydb_default_write_func(lList **answer_list, 
                                  const lListElem *type, 
                                  const lListElem *rule, 
                                  const lListElem *object, 
                                  const char *key, 
                                  const sge_object_type object_type)
{
   bool ret = true;
   bool local_transaction = false; /* did we start a transaction? */
   bdb_info *db;

   DENTER(TOP_LAYER, "spool_berkeleydb_default_write_func");

   db = (bdb_info *)lGetRef(rule, SPR_clientdata);
   if (db == NULL || db->env == NULL || db->db == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_WARNING, 
                              MSG_BERKELEY_NOCONNECTIONOPEN_S,
                              lGetString(rule, SPR_url));
      ret = false;
   } else if (key == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_WARNING, 
                              MSG_BERKELEY_NULLVALUEASKEY,
                              lGetString(rule, SPR_url));
      ret = false;
   } else {
      /* if no transaction was opened from outside, open a new one */
      if (db->txn == NULL) {
         ret = spool_berkeleydb_start_transaction(answer_list, db);
         if (ret) {
            local_transaction = true;
         }
      }

      if (ret) {
         switch (object_type) {
            case SGE_TYPE_JOB:
            case SGE_TYPE_JATASK:
            case SGE_TYPE_PETASK:
               {
                  u_long32 job_id, ja_task_id;
                  char *pe_task_id;
                  char *dup = strdup(key);
                  bool only_job; 

                  job_parse_key(dup, &job_id, &ja_task_id, &pe_task_id, &only_job);

                  if (pe_task_id != NULL) {
                     ret = spool_berkeleydb_write_pe_task(answer_list, db,
                                                          object,
                                                          job_id, ja_task_id,
                                                          pe_task_id);
                  } else if (ja_task_id != 0) {
                     ret = spool_berkeleydb_write_ja_task(answer_list, db,
                                                          object,
                                                          job_id, ja_task_id);
                  } else {
                     ret = spool_berkeleydb_write_job(answer_list, db,
                                                      object,
                                                      job_id, only_job);
                  }
               }
               break;
            default:
               {
                  dstring dbkey_dstring;
                  char dbkey_buffer[MAX_STRING_SIZE];
                  const char *dbkey;

                  sge_dstring_init(&dbkey_dstring, 
                                   dbkey_buffer, sizeof(dbkey_buffer));

                  dbkey = sge_dstring_sprintf(&dbkey_dstring, "%s:%s", 
                                              object_type_get_name(object_type),
                                              key);

                  ret = spool_berkeleydb_write_object(answer_list, db, 
                                                      object, dbkey);
               }
               break;
         }
      }

      /* spooling is done, now end the transaction, if we have an own one */
      if (local_transaction) {
         ret = spool_berkeleydb_end_transaction(answer_list, db, ret);
      }
   }

   DEXIT;
   return ret;
}

static bool
spool_berkeleydb_delete_object(lList **answer_list, bdb_info *db, 
                               const char *key, bool sub_objects)
{
   bool ret = true;

   int dbret;
   DBT cursor_dbt, delete_dbt, data_dbt;

   if (sub_objects) {
      DBC *dbc;

      DPRINTF(("querying objects with keys %s*\n", key));

      PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
      db->db->cursor(db->db, db->txn, &dbc, 0);
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
            delete_ret = db->db->del(db->db, db->txn, &delete_dbt, 0);
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
      dbret = db->db->del(db->db, db->txn, &cursor_dbt, 0);
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
   
   return ret;
}

static bool
spool_berkeleydb_delete_pe_task(lList **answer_list, bdb_info *db,
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
   ret = spool_berkeleydb_delete_object(answer_list, db, dbkey, sub_objects);

   return ret;
}

static bool
spool_berkeleydb_delete_ja_task(lList **answer_list, bdb_info *db,
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
   ret = spool_berkeleydb_delete_object(answer_list, db, dbkey, sub_objects);

   if (ret) {
      ret = spool_berkeleydb_delete_pe_task(answer_list, db, key, 
                                            true);
   }

   return ret;
}

static bool
spool_berkeleydb_delete_job(lList **answer_list, bdb_info *db,
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
   ret = spool_berkeleydb_delete_object(answer_list, db, dbkey, sub_objects);

   if (ret) {
      ret = spool_berkeleydb_delete_ja_task(answer_list, db, key, 
                                            true);
   }

   return ret;
}

/****** spool/berkeleydb/spool_berkeleydb_default_delete_func() ***************
*  NAME
*     spool_berkeleydb_default_delete_func() -- delete object in berkeleydb spooling
*
*  SYNOPSIS
*     bool
*     spool_berkeleydb_default_delete_func(lList **answer_list, 
*                                        const lListElem *type, 
*                                        const lListElem *rule, 
*                                        const char *key, 
*                                        const sge_object_type object_type) 
*
*  FUNCTION
*     Deletes an object in the berkeleydb spooling.
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const char *key                 - unique key 
*     const sge_object_type object_type - object type
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/berkeleydb/--BerkeleyDB-Spooling
*     spool/spool_delete_object()
*******************************************************************************/
bool
spool_berkeleydb_default_delete_func(lList **answer_list, 
                                   const lListElem *type, 
                                   const lListElem *rule,
                                   const char *key, 
                                   const sge_object_type object_type)
{
   bool ret = true;
   bool local_transaction = false; /* did we start a transaction? */
   const char *table_name;
   bdb_info *db;

   dstring dbkey_dstring;
   char dbkey_buffer[MAX_STRING_SIZE];
   const char *dbkey;

   DENTER(TOP_LAYER, "spool_berkeleydb_default_delete_func");

   sge_dstring_init(&dbkey_dstring, dbkey_buffer, sizeof(dbkey_buffer));
   db = (bdb_info *)lGetRef(rule, SPR_clientdata);

      /* if no transaction was opened from outside, open a new one */
      if (db->txn == NULL) {
         ret = spool_berkeleydb_start_transaction(answer_list, db);
         if (ret) {
            local_transaction = true;
         }
      }

      if (ret) {
   switch (object_type) {
      case SGE_TYPE_JOB:
      case SGE_TYPE_JATASK:
      case SGE_TYPE_PETASK:
         {
            u_long32 job_id, ja_task_id;
            char *pe_task_id;
            char *dup = strdup(key);
            bool only_job; 

            job_parse_key(dup, &job_id, &ja_task_id, &pe_task_id, &only_job);

            if (pe_task_id != NULL) {
               dbkey = sge_dstring_sprintf(&dbkey_dstring, "%8d.%8d %s",
                                           job_id, ja_task_id, pe_task_id);
               ret = spool_berkeleydb_delete_pe_task(answer_list, db, 
                                                     dbkey, false);
            } else if (ja_task_id != 0) {
               dbkey = sge_dstring_sprintf(&dbkey_dstring, "%8d.%8d",
                                           job_id, ja_task_id);
               ret = spool_berkeleydb_delete_ja_task(answer_list, db, 
                                                     dbkey, false);
            } else {
               dbkey = sge_dstring_sprintf(&dbkey_dstring, "%8d",
                                           job_id);
               ret = spool_berkeleydb_delete_job(answer_list, db, 
                                                 dbkey, false);
            }
         }
         break;
      default:
         table_name = object_type_get_name(object_type);
         dbkey = sge_dstring_sprintf(&dbkey_dstring, "%s:%s", 
                                     table_name,
                                     key);

         ret = spool_berkeleydb_delete_object(answer_list, db, 
                                              dbkey, false);
         break;
   }
      /* spooling is done, now end the transaction, if we have an own one */
      if (local_transaction) {
         ret = spool_berkeleydb_end_transaction(answer_list, db, ret);
      }
   }

   DEXIT;
   return ret;
}
