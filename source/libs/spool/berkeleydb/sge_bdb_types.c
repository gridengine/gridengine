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
#include <string.h>
#include <errno.h>

/* common */
#include "basis_types.h"

/* libs */
#include "rmon/sgermon.h"
#include "uti/sge_log.h"
#include "lck/sge_mtutil.h"

/* local */
#include "spool/berkeleydb/msg_spoollib_berkeleydb.h"
#include "spool/berkeleydb/sge_bdb_types.h"

/* JG: TODO: use incomplete type for bdb_info in header, see 
 * sge_qmaster_timed_event.h
 */

struct bdb_connection {
   DB_ENV *    env;                 /* thread specific database environment */
   DB *        db;                  /* thread specific database object */
   DB_TXN *    txn;                 /* transaction handle, always per thread */
};

static void
bdb_init_connection(struct bdb_connection *con);

static void 
bdb_destroy_connection(void *connection);

/****** spool/berkeleydb/bdb_create() *****************************************
*  NAME
*     bdb_create() -- create Berkeley DB specific data structures
*
*  SYNOPSIS
*     struct bdb_info * 
*     bdb_create(const char *server, const char *path) 
*
*  FUNCTION
*     Creates and initializes an object describing the connection to a 
*     Berkeley DB database and holding database and transaction handles.
*
*     Transaction handles are thread specific, in case of spooling using the
*     RPC client/server mechanism, the database environment and the database
*     handle are also thread specific.
*     
*     The parameter server is optional. If it is given, the Berkeley DB RPC
*     mechanism will be used. It then defines the name of the host, where the
*     Berkeley DB RPC server (berkeley_db_svc_) is running.
*
*     Path is either
*     - the absolute path to a Database in case of local spooling
*     - the database name (last component of path) in case of RPC mechanism
*
*  INPUTS
*     const char *server - hostname of Berkeley DB rpc server, or NULL
*     const char *path   - path to the database
*
*  RESULT
*     struct bdb_info * - pointer to a newly created and initialized structure
*
*  NOTES
*     MT-NOTE: bdb_create() is MT safe 
*******************************************************************************/
struct bdb_info *
bdb_create(const char *server, const char *path) 
{
   int ret;
   struct bdb_info *info = (struct bdb_info *) malloc(sizeof(struct bdb_info));

   pthread_mutex_init(&(info->mtx), NULL);
   ret = pthread_key_create(&(info->key), bdb_destroy_connection);
   if (ret != 0) {
      fprintf(stderr, "can't initialize key for thread local storage: %s\n", strerror(ret));
   }
   info->server = server;
   info->path   = path;
   info->env    = NULL;
   info->db     = NULL;

   info->next_clear = 0;
   info->next_checkpoint = 0;

   return info;
}

/*
* initialize thread local storage for a connection.
*  NOTES
*     MT-NOTE: bdb_init_connection() is MT safe 
*/
static void
bdb_init_connection(struct bdb_connection *con)
{
   con->env = NULL;
   con->db  = NULL;
   con->txn = NULL;
}

/*
* destroy the thread local storage for a connection
*  NOTES
*     MT-NOTE: bdb_destroy_connection() is MT safe 
*/
static void
bdb_destroy_connection(void *connection)
{
   /* Nothing to do here in principle.
    * Transactions and database connections shall be closed by calling the 
    * shutdown function.
    * But we can generate an error, if there is still something open.
    */
   struct bdb_connection *con = (struct bdb_connection *)connection;

   DENTER(TOP_LAYER, "bdb_destroy_connection");

   if (con->txn != NULL) {
      /* error */
   }

   if (con->db != NULL) {
      /* error */
   }

   if (con->env != NULL) {
      /* error */
   }
   
   DEXIT;
}

/****** spool/berkeleydb/bdb_get_server() **************************************
*  NAME
*     bdb_get_server() -- get server name for a Berkeley DB
*
*  SYNOPSIS
*     const char * 
*     bdb_get_server(struct bdb_info *info) 
*
*  FUNCTION
*     Returns the hostname of a Berkeley DB RPC server, if the RPC mechanism
*     is used, else NULL.
*
*  INPUTS
*     struct bdb_info *info - the database object
*
*  RESULT
*     const char * - hostname or NULL
*
*  NOTES
*     MT-NOTE: bdb_get_server() is MT safe 
*******************************************************************************/
const char *
bdb_get_server(struct bdb_info *info)
{
   return info->server;
}

/****** spool/berkeleydb/bdb_get_path() ****************************************
*  NAME
*     bdb_get_path() -- get the database path
*
*  SYNOPSIS
*     const char * 
*     bdb_get_path(struct bdb_info *info) 
*
*  FUNCTION
*     Returns the path to a Berkeley DB database.
*     If the RPC mechanism is used, this is the last component of the path.
*
*  INPUTS
*     struct bdb_info *info - the database object
*
*  RESULT
*     const char * - path to the database.
*
*  NOTES
*     MT-NOTE: bdb_get_path() is MT safe 
*******************************************************************************/
const char *
bdb_get_path(struct bdb_info *info)
{
   return info->path;
}

/****** spool/berkeleydb/bdb_get_env() *****************************************
*  NAME
*     bdb_get_env() -- get Berkeley DB database environment
*
*  SYNOPSIS
*     DB_ENV * bdb_get_env(struct bdb_info *info) 
*
*  FUNCTION
*     Returns the Berkeley DB database environment set earlier using 
*     bdb_set_env().
*     If the RPC mechanism is used, the environment is stored per thread.
*
*  INPUTS
*     struct bdb_info *info - the database object
*
*  RESULT
*     DB_ENV * - the database environment
*
*  NOTES
*     MT-NOTE: bdb_get_env() is MT safe 
*
*  SEE ALSO
*     spool/berkeleydb/bdb_set_env()
*******************************************************************************/
DB_ENV *
bdb_get_env(struct bdb_info *info)
{
   DB_ENV *env = NULL;

   if (info->server == NULL) {
      env = info->env;
   } else {
      GET_SPECIFIC(struct bdb_connection, con, bdb_init_connection, info->key, 
                   "bdb_get_env");
      env = con->env;
   }

   return env;
}

/****** spool/berkeleydb/bdb_get_db() ******************************************
*  NAME
*     bdb_get_db() -- get Berkeley DB database handle
*
*  SYNOPSIS
*     DB * 
*     bdb_get_db(struct bdb_info *info) 
*
*  FUNCTION
*     Return the Berkeleyd BD database handle set earlier using bdb_set_db().
*     If the RPC mechanism is used, the database handle is stored per thread.
*
*  INPUTS
*     struct bdb_info *info - the database object
*
*  RESULT
*     DB * - the database handle
*
*  NOTES
*     MT-NOTE: bdb_get_db() is MT safe 
*
*  SEE ALSO
*     spool/berkeleydb/bdb_set_db()
*******************************************************************************/
DB *
bdb_get_db(struct bdb_info *info)
{
   DB *db = NULL;

   if (info->server == NULL) {
      db = info->db;
   } else {
      GET_SPECIFIC(struct bdb_connection, con, bdb_init_connection, info->key, 
                   "bdb_get_db");
      db = con->db;
   }

   return db;
}

/****** spool/berkeleydb/bdb_get_txn() *****************************************
*  NAME
*     bdb_get_txn() -- get a transaction handle
*
*  SYNOPSIS
*     DB_TXN * 
*     bdb_get_txn(struct bdb_info *info) 
*
*  FUNCTION
*     Returns a transaction handle set earlier with bdb_set_txn().
*     Each thread can have one transaction open.
*
*  INPUTS
*     struct bdb_info *info - the database object
*
*  RESULT
*     DB_TXN * - a transaction handle
*
*  NOTES
*     MT-NOTE: bdb_get_txn() is MT safe 
*
*  SEE ALSO
*     spool/berkeleydb/bdb_set_txn()
*******************************************************************************/
DB_TXN *
bdb_get_txn(struct bdb_info *info)
{
   GET_SPECIFIC(struct bdb_connection, con, bdb_init_connection, info->key, 
                "bdb_get_txn");
   return con->txn;
}

/****** spool/berkeleydb/bdb_set_env() *****************************************
*  NAME
*     bdb_set_env() -- set the Berkeley DB environment
*
*  SYNOPSIS
*     void 
*     bdb_set_env(struct bdb_info *info, DB_ENV *env) 
*
*  FUNCTION
*     Sets the Berkeley DB environment.
*     If the RPC mechanism is used, an environment has to be created and opened
*     per thread.
*
*  INPUTS
*     struct bdb_info *info - the database object
*     DB_ENV *env           - the environment handle to set
*
*  NOTES
*     MT-NOTE: bdb_set_env() is MT safe 
*
*  SEE ALSO
*     spool/berkeleydb/bdb_get_env()
*******************************************************************************/
void
bdb_set_env(struct bdb_info *info, DB_ENV *env)
{
   if (info->server == NULL) {
      info->env  = env;
   } else {
      GET_SPECIFIC(struct bdb_connection, con, bdb_init_connection, info->key, 
                   "bdb_set_env");
      con->env = env;
   }
}

/****** spool/berkeleydb/bdb_set_db() ******************************************
*  NAME
*     bdb_set_db() -- set a Berkeley DB database handle
*
*  SYNOPSIS
*     void 
*     bdb_set_db(struct bdb_info *info, DB *db) 
*
*  FUNCTION
*     Sets the Berkeley DB database handle.
*     If the RPC mechanism is used, a database handle has to be created and 
*     opened per thread.
*
*  INPUTS
*     struct bdb_info *info - the database object
*     DB *db                - the database handle to store
*
*  NOTES
*     MT-NOTE: bdb_set_db() is MT safe 
*
*  SEE ALSO
*     spool/berkeleydb/bdb_get_db()
*******************************************************************************/
void
bdb_set_db(struct bdb_info *info, DB *db)
{
   if (info->server == NULL) {
      info->db  = db;
   } else {
      GET_SPECIFIC(struct bdb_connection, con, bdb_init_connection, info->key, 
                   "bdb_set_db");
      con->db = db;
   }
}

/****** spool/berkeleydb/bdb_set_txn() *****************************************
*  NAME
*     bdb_set_txn() -- store a transaction handle
*
*  SYNOPSIS
*     void 
*     bdb_set_txn(struct bdb_info *info, DB_TXN *txn) 
*
*  FUNCTION
*     Stores a Berkeley DB transaction handle.
*     It is always stored in thread local storage.
*
*  INPUTS
*     struct bdb_info *info - the database object
*     DB_TXN *txn           - the transaction handle to store
*
*  NOTES
*     MT-NOTE: bdb_set_txn() is MT safe 
*
*  SEE ALSO
*     spool/berkeleydb/bdb_get_txn()
*******************************************************************************/
void
bdb_set_txn(struct bdb_info *info, DB_TXN *txn)
{
   GET_SPECIFIC(struct bdb_connection, con, bdb_init_connection, info->key, 
                "bdb_set_txn");
   con->txn = txn;
}

/****** spool/berkeleydb/bdb_get_dbname() **************************************
*  NAME
*     bdb_get_dbname() -- get a meaningfull database name
*
*  SYNOPSIS
*     const char * 
*     bdb_get_dbname(struct bdb_info *info, dstring *buffer) 
*
*  FUNCTION
*     Return a meaningfull name for a database connection.
*     It contains the server name (in case of RPC mechanism) and the database
*     path.
*     A dstring buffer has to be provided by the caller.
*
*  INPUTS
*     struct bdb_info *info - the database object
*     dstring *buffer       - buffer to hold the database name
*
*  RESULT
*     const char * - the database name
*
*  NOTES
*     MT-NOTE: bdb_get_dbname() is MT safe 
*******************************************************************************/
const char *
bdb_get_dbname(struct bdb_info *info, dstring *buffer)
{
   const char *ret;
   const char *server = bdb_get_server(info);
   const char *path   = bdb_get_path(info);

   if (path == NULL) {
      ret = sge_dstring_sprintf(buffer, "%s", MSG_BERKELEY_DBNOTINITIALIZED);
   } else if (server == NULL) {
      ret = sge_dstring_sprintf(buffer, "%s", path);
   } else {
      ret = sge_dstring_sprintf(buffer, "%s:%s", server, path);
   }

   return ret;
}

void
bdb_lock_info(struct bdb_info *info)
{
   DENTER(TOP_LAYER, "bdb_lock_info");
   sge_mutex_lock("bdb mutex", SGE_FUNC, __LINE__, &(info->mtx));
   DEXIT;
}

void
bdb_unlock_info(struct bdb_info *info)
{
   DENTER(TOP_LAYER, "bdb_unlock_info");
   sge_mutex_unlock("bdb mutex", SGE_FUNC, __LINE__, &(info->mtx));
   DEXIT;
}

