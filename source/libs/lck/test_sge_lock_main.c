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

#include "test_sge_lock_main.h"

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "sge_lock.h"


static pthread_mutex_t *locks;

static void setup(void);
static void teardown(void);
static sge_locker_t id_callback(void);
static void lock_callback(sge_locktype_t aType, sge_lockmode_t aMode, sge_locker_t anID);
static void unlock_callback(sge_locktype_t aType, sge_lockmode_t aMode, sge_locker_t anID);


/****** test_sge_lock_main/main() **********************************************
*  NAME
*     main() -- Generic skeleton for lock API test programs. 
*
*  SYNOPSIS
*     int main(int argc, char *argv[]) 
*
*  FUNCTION
*     Generic driver for lock API test programs utilizing pthreads and pthread
*     mutexes. Setup locking API by registering the needed callbacks. Determine
*     the number of threads needed. Create threads and execute the provided thread
*     function. Wait until all the threads have finished. Teardown the locking API.
*
*     If a program wants to utilize this skeleton it has to implement the functions
*     of a small API defined in 'sge_lock_main.h'.
*
*  INPUTS
*     int argc     - number of arguments 
*     char *argv[] - argument array 
*
*  RESULT
*     int - always 0
*
*  SEE ALSO
*     lck/test_sge_lock_main.h
*******************************************************************************/
int main(int argc, char *argv[])
{
   pthread_t *t;
   int i,j;

   DENTER_MAIN(TOP_LAYER, "main");

   setup();

   i = get_thrd_demand();
   t = (pthread_t *)malloc(i * sizeof(pthread_t));

   DPRINTF(("%s Create %d threads\n", SGE_FUNC, i));

   for(j = 0; j < i; j++) {
      pthread_create(&(t[j]), NULL, get_thrd_func(), get_thrd_func_arg());
   }

   for(j = 0; j < i; j++) {
      DPRINTF(("%s Join thread %u\n", SGE_FUNC, (unsigned int)t[j]));
      pthread_join(t[j], NULL);
   }

   teardown();
   free(t);

   DEXIT;
   return 0;
} /* main */

/****** test_sge_lock_main/setup() *********************************************
*  NAME
*     setup() -- Setup the locking API 
*
*  SYNOPSIS
*     static void setup(void) 
*
*  FUNCTION
*     Create and initialize the pthread mutexes needed. Register the callbacks
*     required by the locking API. 
*
*  INPUTS
*     void - none 
*
*  RESULT
*     static void - none 
*
*  SEE ALSO
*     test_sge_lock_main/teardown()
*******************************************************************************/
static void setup(void)
{
   pthread_mutexattr_t attr;
   int n, i;

   DENTER(TOP_LAYER, "setup");

   pthread_mutexattr_init(&attr);
   pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

   n = sge_num_locktypes();
   locks = (pthread_mutex_t *)malloc(n * sizeof(pthread_mutex_t));

   for(i = 0; i < n; i++) {
      pthread_mutex_init(&(locks[i]), &attr);
   }

   sge_set_lock_callback(lock_callback);
   sge_set_unlock_callback(unlock_callback);
   sge_set_id_callback(id_callback);

   DEXIT;
   return;
} /* setup */

/****** test_sge_lock_main/teardown() ******************************************
*  NAME
*     teardown() -- Teardown the locking API 
*
*  SYNOPSIS
*     static void teardown(void) 
*
*  FUNCTION
*     Destroy and free the pthread mutexes created during the setup. 
*
*  INPUTS
*     void - none 
*
*  RESULT
*     static void - none 
*
*  SEE ALSO
*     test_sge_lock_main/setup()
*******************************************************************************/
static void teardown(void)
{
   int n, i;

   DENTER(TOP_LAYER, "teardown");

   n = sge_num_locktypes();

   for (i = 0; i < n; i++) {
      pthread_mutex_destroy(&(locks[i]));
   }
   
   free(locks);

   DEXIT;
   return;
} /* teardown */

/****** test_sge_lock_main/id_callback() ***************************************
*  NAME
*     id_callback() -- Locker ID callback implementation
*
*  SYNOPSIS
*     static sge_locker_t id_callback(void) 
*
*  FUNCTION
*     Return if of current thread as locker id. 
*
*  INPUTS
*     void - none 
*
*  RESULT
*     static sge_locker_t - locker id 
*
*  SEE ALSO
*     test_sge_lock_main/setup()
*     lck/sge_lock.c
*******************************************************************************/
static sge_locker_t id_callback(void)
{
   return (sge_locker_t)pthread_self();
} /* id_callback */

/****** test_sge_lock_main/lock_callback() *************************************
*  NAME
*     lock_callback() -- Lock callback implementation 
*
*  SYNOPSIS
*     static void lock_callback(sge_locktype_t aType, sge_lockmode_t aMode, 
*     sge_locker_t anID) 
*
*  FUNCTION
*     Lock pthread mutex determined by 'aType' in mode 'aMode'.
*
*  INPUTS
*     sge_locktype_t aType - lock type 
*     sge_lockmode_t aMode - lock mode 
*     sge_locker_t anID    - locker id 
*
*  RESULT
*     static void - none
*
*  SEE ALSO
*     test_sge_lock_main/setup()
*     lck/sge_lock.c
*******************************************************************************/
static void lock_callback(sge_locktype_t aType, sge_lockmode_t aMode, sge_locker_t anID)
{
   DENTER(TOP_LAYER, "lock_callback");

   DLOCKPRINTF(("Locker %d tries to lock %s\n", (int)anID, sge_type_name(aType)));

   if (pthread_mutex_lock(&(locks[aType])) != 0)
   {
      DLOCKPRINTF(("Locker %d failed to lock %s\n", (int)anID, sge_type_name(aType)));
      abort();
   }

   DLOCKPRINTF(("Locker %d locked %s\n", (int)anID, sge_type_name(aType)));

   DEXIT;
   return;
} /* lock_callback */

/****** test_sge_lock_main/unlock_callback() ***********************************
*  NAME
*     unlock_callback() -- Unlock callback implementation 
*
*  SYNOPSIS
*     static void unlock_callback(sge_locktype_t aType, sge_lockmode_t aMode, 
*     sge_locker_t anID) 
*
*  FUNCTION
*     Unlock pthread mutex determined by 'aType'. 
*
*  INPUTS
*     sge_locktype_t aType - lock type 
*     sge_lockmode_t aMode - lock mode 
*     sge_locker_t anID    - locker id 
*
*  RESULT
*     static void - none
*
*  SEE ALSO
*     test_sge_lock_main/setup()
*     lck/sge_lock.c
*******************************************************************************/
static void unlock_callback(sge_locktype_t aType, sge_lockmode_t aMode, sge_locker_t anID)
{
   DENTER(TOP_LAYER, "unlock_callback");

   if (pthread_mutex_unlock(&(locks[aType])) != 0)
   {
      DLOCKPRINTF(("Locker %d failed to unlock %s\n", (int)anID, sge_type_name(aType)));
      abort();
   }

   DLOCKPRINTF(("Locker %d unlocked %s\n", (int)anID, sge_type_name(aType)));

   DEXIT;
   return;
} /* lock_callback */

