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

#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "lck/test_sge_lock_main.h"
#include "lck/sge_lock.h"
#include "lck/sge_mtutil.h"
#include "uti/sge_stdlib.h"

static void setup(void);

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
   int i;
   int j;
   int thrd_count;
   struct timeval before;
   struct timeval after;
   double time_new;
   
   DENTER_MAIN(TOP_LAYER, "main");

   setup();

   thrd_count = get_thrd_demand();
   t = (pthread_t *)malloc(thrd_count * sizeof(pthread_t));

   for (i = 1; i <= thrd_count; i++) {
      for(j = 0; j < i; j++) {
         t[j] = 0;
      }
     
      set_thread_count(i);
      
      gettimeofday(&before, NULL); 

      printf("\n%s Create %d threads\n\n", SGE_FUNC, i);

      for(j = 0; j < i; j++) {
         pthread_create(&(t[j]), NULL, get_thrd_func(), get_thrd_func_arg());
      }
      for(j = 0; j < i; j++) {
         pthread_join(t[j], NULL);
      }

      gettimeofday(&after, NULL);

      time_new = after.tv_usec - before.tv_usec;
      time_new = after.tv_sec - before.tv_sec + (time_new/1000000);

      printf("the test took: %fs\n", time_new);
   }
   FREE(t);

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
   DENTER(TOP_LAYER, "setup");

   DEXIT;
   return;
} /* setup */

