#ifndef __SGE_BDB_TYPES_H 
#define __SGE_BDB_TYPES_H 
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

#include <time.h>
#include <pthread.h>

#include <db.h>

/* Berkeley DB data structures:
 * We have a bdb_info object per spooling rule.
 * It holds all data that is required to use the spooling rule.
 * In case of local spooling, the DB_ENV and the DB are accessable for multiple threads.
 * If we use the RPC server, we need per thread DB_ENV and DB.
 * The transaction handle is always thread specific.
 * Thread specific data is initialized in the spooling startup function.
 */
struct bdb_info {
   pthread_mutex_t   mtx;                 /* lock access to this object */
   pthread_key_t     key;                 /* for thread specific data */
  
   const char *      server;              /* server, in case of RPC mechanism */
   const char *      path;                /* the database path */

   DB_ENV *          env;                 /* global database environment */
   DB *              db;                  /* global database object */

   time_t            next_clear;          /* time of next logfile clear */
   time_t            next_checkpoint;     /* time of next checkpoint */
};

struct bdb_info *
bdb_create(const char *server, const char *path);

const char *
bdb_get_server(struct bdb_info *info);

const char *
bdb_get_path(struct bdb_info *info);

DB_ENV *
bdb_get_env(struct bdb_info *info);

DB *
bdb_get_db(struct bdb_info *info);

DB_TXN *
bdb_get_txn(struct bdb_info *info);

void
bdb_set_env(struct bdb_info *info, DB_ENV *env);

void
bdb_set_db(struct bdb_info *info, DB *db);

void
bdb_set_txn(struct bdb_info *info, DB_TXN *txn);
#endif /* __SGE_BDB_TYPES_H */    
