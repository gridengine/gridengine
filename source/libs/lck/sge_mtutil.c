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
#include <pthread.h>
#include <sys/time.h>

#include "sgermon.h"
#include "sge_log.h"
#include "msg_lcklib.h"


/****** sge_mtutil/sge_mutex_lock() ********************************************
*  NAME
*     sge_mutex_lock() -- Mutex locking wrapper with rmon monitoring
*
*  SYNOPSIS
*     void sge_mutex_lock(const char *mutex_name, const char *func, 
*     int line, pthread_mutex_t *mutex) 
*
*  FUNCTION
*     Locks the passed mutex. Before and after locking rmon DLOCKPRINTF() 
*     is used to facilitate tracking of deadlocks that are caused by 
*     mutexes.
*
*  INPUTS
*     const char *mutex_name - The name of the mutex.
*     const char *func       - The function where sge_mutex_lock() 
*                              was called from
*     int line               - The line number where sge_mutex_lock() 
*                              was called from
*     pthread_mutex_t *mutex - The mutex.
*
*  NOTES
*     MT-NOTE: sge_mutex_lock() is MT-safe
*
*  SEE ALSO
*     sge_mtutil/sge_mutex_unlock()
*******************************************************************************/
void sge_mutex_lock(const char *mutex_name, const char *func, int line, pthread_mutex_t *mutex)
{
   DENTER(BASIS_LAYER, "sge_mutex_lock");

   DLOCKPRINTF(("%s() line %d: about to lock mutex \"%s\"\n", func, line, mutex_name));

   if (pthread_mutex_lock(mutex) != 0)
   {
      CRITICAL((SGE_EVENT, MSG_LCK_MUTEXLOCKFAILED_SS, func, mutex_name));
      abort();
   }

   DLOCKPRINTF(("%s() line %d: locked mutex \"%s\"\n", func, line, mutex_name));

   DEXIT;
   return;
} /* sge_mutex_lock() */

/****** sge_mtutil/sge_mutex_unlock() ********************************************
*  NAME
*     sge_mutex_unlock() -- Mutex unlocking wrapper with rmon monitoring
*
*  SYNOPSIS
*     void sge_mutex_unlock(const char *mutex_name, const char *func, 
*     int line, pthread_mutex_t *mutex) 
*
*  FUNCTION
*     Unlocks the passed mutex. Before and after unlocking rmon DLOCKPRINTF() 
*     is used to facilitate tracking of deadlocks that are caused by 
*     mutexes.
*
*  INPUTS
*     const char *mutex_name - The name of the mutex.
*     const char *func       - The function where sge_unmutex_unlock() 
*                              was called from
*     int line               - The line number where sge_unmutex_lock() 
*                              was called from
*     pthread_mutex_t *mutex - The mutex.
*
*  NOTES
*     MT-NOTE: sge_mutex_unlock() is MT-safe
*
*  SEE ALSO
*     sge_mtutil/sge_mutex_lock()
*******************************************************************************/
void sge_mutex_unlock(const char *mutex_name, const char *func, int line, pthread_mutex_t *mutex)
{
   DENTER(BASIS_LAYER, "sge_mutex_unlock");

   if (pthread_mutex_unlock(mutex) != 0)
   {
      CRITICAL((SGE_EVENT, MSG_LCK_MUTEXUNLOCKFAILED_SS, func, mutex_name));
      abort();
   }

   DLOCKPRINTF(("%s() line %d: unlocked mutex \"%s\"\n", func, line, mutex_name));

   DEXIT;
   return;
} /* sge_mutex_unlock() */


/****** sge_mtutil/sge_relative_timespec() **************************************
*  NAME
*     sge_relative_timespec() -- set timespec to now plus timeout 
*
*  SYNOPSIS
*     static void sge_relative_timespec(signed long timeout, struct 
*     timespec *ts) 
*
*  FUNCTION
*     Based on the relative timeout passed an absolute timespec is 
*     returned. The timespec can e.g. be used for pthread_cond_timedwait().
*     Also a timout of 0 can be used. However if the timespec returned is then 
*     used with pthread_cond_timedwait() this requires the predicate is checked 
*     once at least.
*
*  INPUTS
*     signed long timeout - A relative timeout interval or 0
*
*  OUTPUTS
*     struct timespec *ts - An abstime timespec value
*
*  NOTES
*     MT-NOTE: sge_relative_timespec() is MT safe
*******************************************************************************/
void sge_relative_timespec(signed long timeout, struct timespec *ts)
{
   struct timeval now;

   /* in examples also clock_gettime(CLOCK_REALTIME, &ts) was used */
   gettimeofday(&now, NULL);
   ts->tv_sec = now.tv_sec;
   ts->tv_nsec = now.tv_usec * 1000;

   if (timeout != 0)
      ts->tv_sec += timeout;
   /* in case of DRMAA_TIMEOUT_NO_WAIT the current system time 
      can be used assumed the predicate is checked once at least */

   return;
}

