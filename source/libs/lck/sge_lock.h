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
   LOCK_GLOBAL  = 0,  /* global lock */
   NUM_OF_TYPES = 1
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
