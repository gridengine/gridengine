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

#include <stdlib.h>

#include "sge_lock.h"

#include <stdio.h>
#include "sgermon.h"

/****** sge_lock/Introduction ****************************************************
*  NAME
*     Grid Engine Locking API
*
*  FUNCTION
*     The Grid Engine Locking API is a mediator between a lock service provider
*     and a lock client. A lock service provider offers a particular lock
*     implementation by registering a set of callbacks. A lock client does acquire
*     and release a lock using the respective API functions.
*
*     A lock service provider (usually a daemon) needs to register three
*     different callbacks:
*
*       + a lock callback, which is used by the API lock function
*
*       + an unlock callback, which is used by the API unlock function
*
*       + an ID callback, which is used by the API locker ID function
*     
*     Lock service provider has to register these callbacks *before* lock client
*     uses the lock/unlock API functions. Otherwise the lock/unlock operations do
*     have no effect at all.
*
*     Locktype denotes the entity which will be locked/unlocked (e.g. Global
*     Lock. Lockmode denotes in which mode the locktype will be locked/unlocked.
*     Locker ID unambiguously identifies a lock client.
*
*     Adding a new locktype does recquire two steps:
*
*     1. Add an enumerator to 'sge_locktype_t'. Do not forget to update
*        'NUM_OF_TYPES'.
*
*     2. Add a description to 'locktype_names'.
*
*  SEE ALSO
*     sge_lock/sge_lock.h
*******************************************************************************/


/* 'locktype_names' has to be in sync with the definition of 'sge_locktype_t' */
static const char* locktype_names[NUM_OF_TYPES] = {
   "global"  /* LOCK_GLOBAL */
};

static void (*lock_callback) (sge_locktype_t, sge_lockmode_t, sge_locker_t);
static void (*unlock_callback) (sge_locktype_t, sge_lockmode_t, sge_locker_t); 
static sge_locker_t (*id_callback) (void);


/****** sge_lock/sge_lock() ****************************************************
*  NAME
*     sge_lock() -- Acquire lock
*
*  SYNOPSIS
*     void sge_lock(sge_locktype_t aType, sge_lockmode_t aMode, sge_locker_t 
*     anID) 
*
*  FUNCTION
*     Acquire lock. If the lock is already held, block the caller until lock
*     becomes available again.
*
*     Instead of using this function directly the convenience macro
*     'SGE_LOCK(type, mode)' could (and should) be used. 
*     
*
*  INPUTS
*     sge_locktype_t aType - lock to acquire
*     sge_lockmode_t aMode - lock mode
*     sge_locker_t anID    - locker id
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: sge_lock() is MT safe 
*******************************************************************************/
void sge_lock(sge_locktype_t aType, sge_lockmode_t aMode, sge_locker_t anID)
{
   DENTER(TOP_LAYER, "sge_lock");

   if (NULL != lock_callback) {
      lock_callback(aType, aMode, anID);
   }

   DEXIT;
   return;
} /* sge_lock */

/****** sge_lock/sge_unlock() **************************************************
*  NAME
*     sge_unlock() -- Release lock
*
*  SYNOPSIS
*     void sge_unlock(sge_locktype_t aType, sge_lockmode_t aMode, sge_locker_t 
*     anID) 
*
*  FUNCTION
*     Release lock. 
*
*     Instead of using this function directly the convenience macro
*     'SGE_UNLOCK(type, mode)' could (and should) be used. 
*
*  INPUTS
*     sge_locktype_t aType - lock to release
*     sge_lockmode_t aMode - lock mode in which the lock has been acquired 
*     sge_locker_t anID    - locker id
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: sge_unlock() is MT safe 
*******************************************************************************/
void sge_unlock(sge_locktype_t aType, sge_lockmode_t aMode, sge_locker_t anID)
{
   DENTER(TOP_LAYER, "sge_unlock");

   if (NULL != unlock_callback) {
      unlock_callback(aType, aMode, anID);
   }

   DEXIT;
   return;
} /* sge_unlock */

/****** sge_lock/sge_locker_id() ***********************************************
*  NAME
*     sge_locker_id() -- Locker identifier 
*
*  SYNOPSIS
*     sge_locker_t sge_locker_id(void) 
*
*  FUNCTION
*     Return an unambiguous identifier for the locker.  
*
*  INPUTS
*     void - none 
*
*  RESULT
*     sge_locker_t - locker identifier 
*
*  NOTES
*     There is a 1 to 1 mapping between a locker id an a thread. However the 
*     locker id and the thread id may be different.
*
*     MT-NOTE: sge_locker_id() is MT safe
*******************************************************************************/
sge_locker_t sge_locker_id(void)
{
   sge_locker_t id = 0;

   DENTER(TOP_LAYER, "sge_locker_id");

   if (NULL != id_callback) {
      id = (sge_locker_t)id_callback();
   }

   DEXIT;
   return id;
} /* sge_locker_id */

/****** sge_lock/sge_type_name() ***********************************************
*  NAME
*     sge_type_name() -- Lock type name 
*
*  SYNOPSIS
*     const char* sge_type_name(sge_locktype_t aType) 
*
*  FUNCTION
*     Return lock name for a given lock type.
*
*  INPUTS
*     sge_locktype_t aType - lock type for which the name should be returned. 
*
*  RESULT
*     const char* - pointer to a constant string holding the lock type name.
*                   NULL if the lock type is unknown.
*
*  NOTES
*     MT-NOTE: sge_type_name() is MT safe 
*******************************************************************************/
const char* sge_type_name(sge_locktype_t aType)
{
   const char *s = NULL;
   int i = (int)aType;

   DENTER(TOP_LAYER, "sge_type_name");

   s = (i < NUM_OF_TYPES) ? locktype_names[i] : NULL;

   DEXIT;
   return s;
} /* sge_type_name */

/****** sge_lock/sge_num_locktypes() *******************************************
*  NAME
*     sge_num_locktypes() -- Number of lock types
*
*  SYNOPSIS
*     int sge_num_locktypes(void) 
*
*  FUNCTION
*     Return number of lock types. 
*
*     This function is useful for a lock service provider to determine the number
*     of locks it is supposed to handle.
*
*  INPUTS
*     void - none 
*
*  RESULT
*     int - number of lock types.
*
*  NOTES
*     MT-NOTE: sge_num_locktypes() is MT safe 
*******************************************************************************/
int sge_num_locktypes(void)
{
   int i = 0;

   DENTER(TOP_LAYER, "sge_num_locktypes");

   i = (int)NUM_OF_TYPES;

   DEXIT;
   return i;
} /* sge_num_locktypes */

/****** sge_lock/sge_set_lock_callback() ***************************************
*  NAME
*     sge_set_lock_callback() -- Set lock callback
*
*  SYNOPSIS
*     void sge_set_lock_callback(void (*aFunc)(sge_locktype_t, sge_lockmode_t, sge_locker_t)) 
*
*  FUNCTION
*     Set the callback which is used by 'sge_lock()' to actually acquire a lock
*     from the lock service provider.
*
*     A lock service provider must call this function *before* a lock client 
*     acquires a lock via 'sge_lock()' for the first time.
*
*  INPUTS
*     aFunc - lock function pointer 
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE; sge_set_lock_callback() is NOT MT safe 
*******************************************************************************/
void sge_set_lock_callback(void (*aFunc)(sge_locktype_t, sge_lockmode_t, sge_locker_t))
{
   DENTER(TOP_LAYER, "sge_set_lock_callback");

   if (NULL != aFunc) {
      lock_callback = aFunc;
   }

   DEXIT;
   return;
} /* sge_set_lock_callback */

/****** sge_lock/sge_set_unlock_callback() *************************************
*  NAME
*     sge_set_unlock_callback() -- Set unlock callback 
*
*  SYNOPSIS
*     void sge_set_unlock_callback(void (*aFunc)(sge_locktype_t, sge_lockmode_t, sge_locker_t))
*
*  FUNCTION
*     Set the callback which is used by 'sge_unlock()' to actually release a lock
*     acquired from the lock service provider. 
*
*     A lock service provider must call this function *before* a lock client
*     releases a lock via 'sge_unlock()' for the first time.
*
*  INPUTS
*     aFunc - unlock function pointer 
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: sge_set_unlock_callback() is NOT MT safe 
*******************************************************************************/
void sge_set_unlock_callback(void (*aFunc)(sge_locktype_t, sge_lockmode_t, sge_locker_t))
{
   DENTER(TOP_LAYER, "sge_set_unlock_callback");

   if (NULL != aFunc) {
      unlock_callback = aFunc;
   }

   DEXIT;
   return;
} /* sge_set_unlock_callback */

/****** sge_lock/sge_set_id_callback() *****************************************
*  NAME
*     sge_set_id_callback() -- Set locker id callback 
*
*  SYNOPSIS
*     void sge_set_id_callback(sge_locker_t (*aFunc)(void)) 
*
*  FUNCTION
*     Set the callback which is used by 'sge_locker_id()' to actually fetch a 
*     locker id from the lock service provider. 
*
*     A lock service provider must call this function *before* a lock client
*     fetches a locker id via 'sge_locker_id()' for the first time.
*
*  INPUTS
*     aFunc - locker id function pointer
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: sge_set_id_callback() is NOT MT safe 
*******************************************************************************/
void sge_set_id_callback(sge_locker_t (*aFunc)(void))
{
   DENTER(TOP_LAYER, "sge_set_id_callback");

   if (NULL != aFunc) {
      id_callback = aFunc;
   }

   DEXIT;
   return;
} /* sge_set_id_callback */

