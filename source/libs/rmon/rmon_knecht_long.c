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
 *  License at http://www.gridengine.sunsource.net/license.html
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
#define DEBUG
#define MAINPROGRAM
#include <unistd.h>
#include "rmon_rmon.h"
#include "rmon_daemon.h"
#include "msg_rmon.h"

#define SLEEP_TIME 1
#define LOOP_MAX 5
#define LENGTH  100

#define JOB0 0
#define JOB1 1
#define JOB2 2
#define JOB3 3
#define JOB4 4

int main(int argc, char *argv[]);
static void function_f(void);
static void function_g(void);
static void function_h(void);
static void function_i(void);

int main(
int argc,
char *argv[] 
) {
   int i, j;
   u_long job = JOB0;

#undef FUNC
#define FUNC "main"

   DOPEN("knecht_long");
   DENTER;

   rmon_daemon(rmon_mmayclose);

   for (i = 0; i < LENGTH; i++) {

      for (j = 0; j < LOOP_MAX; j++) {
         DPRINTF(("Dies ist der %d. Durchlauf von %s().\n", j, FUNC));
      }
      sleep(SLEEP_TIME);

      DJOBTRACE((job, MSG_RMON_STARTINGJOBX_D,  job));
      function_f();
      DJOBTRACE((job, MSG_RMON_JOBXHASSTOPPED_D,job));
      sleep(SLEEP_TIME);
   }

   DEXIT;
   DCLOSE;
   return 1;
}

static void function_f()
{
   int i;
   u_long job = JOB1;

#undef FUNC
#define FUNC "function_f"

   DENTER;

   for (i = 0; i < LOOP_MAX; i++) {
      DPRINTF(("Dies ist der %d. Durchlauf von %s().\n", i, FUNC));
   }

   sleep(SLEEP_TIME);

   DJOBTRACE((job, MSG_RMON_STARTINGJOBX_D,  job));

   function_g();
   DJOBTRACE((job, MSG_RMON_JOBXHASSTOPPED_D ,  job));

   sleep(SLEEP_TIME);
   DEXIT;
   return;
}

static void function_g()
{
   int i;
   u_long job = JOB2;

#undef FUNC
#define FUNC "function_g"

   DENTER;

   for (i = 0; i < LOOP_MAX; i++) {
      DPRINTF(("Dies ist der %d. Durchlauf von %s().\n", i, FUNC));
   }

   sleep(SLEEP_TIME);
   DJOBTRACE((job, MSG_RMON_STARTINGJOBX_D, job));
   function_h();
   DJOBTRACE((job, MSG_RMON_JOBXHASSTOPPED_D, job));

   sleep(SLEEP_TIME);
   DEXIT;
   return;

}

static void function_h()
{
   int i;
   u_long job = JOB3;

#undef FUNC
#define FUNC "function_h"

   DENTER;

   for (i = 0; i < LOOP_MAX; i++) {
      DPRINTF(("Dies ist der %d. Durchlauf von %s().\n", i, FUNC));
   }

   sleep(SLEEP_TIME);
   DJOBTRACE((job, MSG_RMON_STARTINGJOBX_D, job));
   function_i();
   DJOBTRACE((job, MSG_RMON_JOBXHASSTOPPED_D, job));

   sleep(SLEEP_TIME);
   DEXIT;
   return;
}

static void function_i()
{
   int i;
   u_long job = JOB4;

#undef FUNC
#define FUNC "function_i"

   DENTER;

   DJOBTRACE((job, MSG_RMON_STARTINGJOBX_D, job));

   for (i = 0; i < LOOP_MAX; i++) {
      DPRINTF(("Dies ist der %d. Durchlauf von %s().\n", i, FUNC));
   }

   DJOBTRACE((job, MSG_RMON_JOBXHASSTOPPED_D, job));

   DEXIT;
   return;
}
