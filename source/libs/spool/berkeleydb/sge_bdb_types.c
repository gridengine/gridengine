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

/* rmon */
#include "sgermon.h"
#include "sge_log.h"

/* local */
#include "spool/berkeleydb/sge_bdb_types.h"

/* JG: TODO: use incomplete type for bdb_info in header, see 
 * sge_qmaster_timed_event.h
 */

struct bdb_connection {
   DB_ENV *    env;                       /* thread specific database environment */
   DB *        db;                        /* thread specific database object */
   DB_TXN *    txn;                       /* transaction handle, always per thread */
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
*     bdb_create(const char *params) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const char *params - ??? 
*
*  RESULT
*     struct bdb_info * - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: bdb_create() is MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
struct bdb_info *
bdb_create(const char *params) 
{
   int ret;
   struct bdb_info *info = (struct bdb_info *) malloc(sizeof(struct bdb_info));

   pthread_mutex_init(&(info->mtx), NULL);
   ret = pthread_key_create(&(info->key), bdb_destroy_connection);
   if (ret != 0) {
      fprintf(stderr, "can't initialize key for thread local storage: %s\n", strerror(ret));
   }
   info->params = params;
   info->env = NULL;
   info->db  = NULL;

   info->next_clear = 0;
   info->next_checkpoint = 0;

   return info;
}

static void
bdb_init_connection(struct bdb_connection *con)
{
   con->env = NULL;
   con->db  = NULL;
   con->txn = NULL;
}

static void
bdb_destroy_connection(void *connection)
{
   /* Nothing to do here in principle.
    * Transactions and database connections shall be closed by calling the shutdown
    * function.
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

DB_ENV *
bdb_get_env(struct bdb_info *info)
{
   DB_ENV *env = NULL;

   if (info->env != NULL) {
      env = info->env;
   } else {
      GET_SPECIFIC(struct bdb_connection, con, bdb_init_connection, info->key, "bdb_get_env");
      env = con->env;
   }

   return env;
}

DB *
bdb_get_db(struct bdb_info *info)
{
   DB *db = NULL;

   if (info->db != NULL) {
      db = info->db;
   } else {
      GET_SPECIFIC(struct bdb_connection, con, bdb_init_connection, info->key, "bdb_get_db");
      db = con->db;
   }

   return db;
}

DB_TXN *
bdb_get_txn(struct bdb_info *info)
{
   GET_SPECIFIC(struct bdb_connection, con, bdb_init_connection, info->key, "bdb_get_txn");
   return con->txn;
}

void
bdb_set_env(struct bdb_info *info, DB_ENV *env, bool global)
{
   if (global) {
      info->env  = env;
   } else {
      GET_SPECIFIC(struct bdb_connection, con, bdb_init_connection, info->key, "bdb_set_env");
      con->env = env;
   }
}

void
bdb_set_db(struct bdb_info *info, DB *db, bool global)
{
   if (global) {
      info->db  = db;
   } else {
      GET_SPECIFIC(struct bdb_connection, con, bdb_init_connection, info->key, "bdb_set_db");
      con->db = db;
   }
}

void
bdb_set_txn(struct bdb_info *info, DB_TXN *txn)
{
   GET_SPECIFIC(struct bdb_connection, con, bdb_init_connection, info->key, "bdb_set_txn");
   con->txn = txn;
}

