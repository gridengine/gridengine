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

#include "sge_thread_jvm.h"

#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "rmon/sgermon.h"

#include "lck/sge_mtutil.h"

#include "sge_thread_ctrl.h"

#define THREAD_CONTROL_MUTEX "thread_control_mutex"

#define THREAD_LAYER BASIS_LAYER

thread_control_t Thread_Control = {
   PTHREAD_MUTEX_INITIALIZER,
   PTHREAD_COND_INITIALIZER,
   false
};

/* EB: TODO: ST: add adoc comments */
bool
sge_thread_has_shutdown_started(void)
{
   bool res = false;

   DENTER(THREAD_LAYER, "sge_thread_has_shutdown_started");
   sge_mutex_lock(THREAD_CONTROL_MUTEX, SGE_FUNC, __LINE__, &Thread_Control.mutex);
   res = Thread_Control.shutdown_started;
   sge_mutex_unlock(THREAD_CONTROL_MUTEX, SGE_FUNC, __LINE__, &Thread_Control.mutex);
   DEXIT;
   return res;
}

void
sge_thread_notify_all_waiting(void)
{
   DENTER(THREAD_LAYER, "sge_thread_notify_all_waiting");

   sge_mutex_lock(THREAD_CONTROL_MUTEX, SGE_FUNC, __LINE__, &Thread_Control.mutex);

   Thread_Control.shutdown_started = true;
   pthread_cond_signal(&Thread_Control.cond_var);

   sge_mutex_unlock(THREAD_CONTROL_MUTEX, SGE_FUNC, __LINE__, &Thread_Control.mutex);

   DRETURN_VOID;
}

void 
sge_thread_wait_for_signal(void)
{
   DENTER(THREAD_LAYER, "sge_thread_wait_for_signal");

   sge_mutex_lock(THREAD_CONTROL_MUTEX, SGE_FUNC, __LINE__, &Thread_Control.mutex);

   while (Thread_Control.shutdown_started == false) {
      pthread_cond_wait(&Thread_Control.cond_var, &Thread_Control.mutex);
   }

   sge_mutex_unlock(THREAD_CONTROL_MUTEX, SGE_FUNC, __LINE__, &Thread_Control.mutex);

   DEXIT;
   return;
}

