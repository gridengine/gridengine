#ifndef _SGE_LOCK_H_
#define _SGE_LOCK_H_

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
 *   Copyright: 2003 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#include "basis_types.h"
#include "sgermon.h"


typedef enum {
   LOCK_GLOBAL = 0,                      /* global lock */
   LOCK_EVENT_CLIENT_LST = 1,            /* event client list lock */
   LOCK_JOB_REPORT_LST = 2,              /* job report list lock */
   LOCK_EXECD_CONFIG_LST = 3,            /* execd configuration list lock */
   LOCK_SCHEDD_CONFIG_LST = 4,           /* schedd configuration list lock */
   LOCK_MASTER_CONFIG_LST = 5,           /* master configuration list lock */
   LOCK_MASTER_JOB_LST = 6,              /* master job list lock */
   LOCK_MASTER_ZOMBIE_LST = 7,           /* master zombie list lock */
   LOCK_MASTER_CALENDAR_LST = 8,         /* master calendar list lock */
   LOCK_MASTER_COMPLEX_ENTRY_LST = 9,    /* master complex entry list lock */
   LOCK_MASTER_CKPT_OBJ_LST = 10,        /* master checkpoint object list lock */
   LOCK_MASTER_CLUSTER_QUEUE_LST = 11   ,/* master cluster queue list lock */
   LOCK_MASTER_QUEUE_LST = 12,           /* master queue list lock */
   LOCK_MASTER_USER_MAPPING_LST = 13,    /* master user mapping list lock */
   LOCK_MASTER_HOST_GROUP_LST = 14,      /* master host group list lock */
   LOCK_MASTER_EXEC_HOST_LST = 15,       /* master exec host list lock */
   LOCK_MASTER_ADMIN_HOST_LST = 16,      /* master admin host list lock */
   LOCK_MASTER_SUBMIT_HOST_LST = 17,     /* master submit host list lock */
   LOCK_MASTER_JOB_SCHEDD_INFO_LST = 18, /* master job schedd info list lock */
   LOCK_MASTER_MANAGER_LST = 19,         /* master manager list lock */
   LOCK_MASTER_OPERATOR_LST = 20,        /* master operator list lock */
   LOCK_MASTER_PARALLEL_ENV_LST = 21,    /* master parallel environment list lock */
   LOCK_MASTER_SHARETREE_LST = 22,       /* master sharetree list lock */
   LOCK_MASTER_USER_LST = 23,            /* master user list lock */
   LOCK_MASTER_SUBMIT_USER_LST = 24,     /* master submit user list lock */
   LOCK_MASTER_USER_SET_LST = 25,        /* master user set list lock */
   LOCK_MASTER_PROJECT_LST = 26,         /* master project list lock */
   NUM_OF_TYPES = 27
} sge_locktype_t;

#if defined(LINUX)
#undef LOCK_READ
#undef LOCK_WRITE
#endif

typedef enum {
   LOCK_READ  = 1, /* shared  */
   LOCK_WRITE = 2  /* exclusive */
} sge_lockmode_t;

typedef u_long32 sge_locker_t;

/*
 * Lock user interface
 */
void sge_lock(sge_locktype_t aType, sge_lockmode_t aMode, sge_locker_t anID);
void sge_unlock(sge_locktype_t aType, sge_lockmode_t aMode, sge_locker_t anID);
sge_locker_t sge_locker_id(void);

const char* sge_type_name(sge_locktype_t aType);
int sge_num_locktypes(void);

#if defined(SGE_LOCK)
#error "SGE_LOCK already defined!"
#endif

#define SGE_LOCK(type, mode) \
{ \
   const char *name = sge_type_name(type); \
   DLOCKPRINTF(("%s() line %d: about to lock \"%s\"\n", __FILE__, __LINE__, name)); \
   sge_lock(type, mode, sge_locker_id()); \
   DLOCKPRINTF(("%s() line %d: locked \"%s\"\n", __FILE__, __LINE__, name)); \
}

#if defined(SGE_UNLOCK)
#error "SGE_UNLOCK already defined!"
#endif

#define SGE_UNLOCK(type, mode) \
{ \
   const char *name = sge_type_name(type); \
   DLOCKPRINTF(("%s() line %d: about to unlock \"%s\"\n", __FILE__, __LINE__, name)); \
   sge_unlock(type, mode, sge_locker_id()); \
   DLOCKPRINTF(("%s() line %d: unlocked \"%s\"\n", __FILE__, __LINE__, name)); \
}

/*
 * Lock service provider interface 
 */
void sge_set_lock_callback(void (*aFunc)(sge_locktype_t, sge_lockmode_t, sge_locker_t)); 
void sge_set_unlock_callback(void (*aFunc)(sge_locktype_t, sge_lockmode_t, sge_locker_t));
void sge_set_id_callback(sge_locker_t (*aFunc)(void));

#endif /* _SGE_LOCK_H_ */
