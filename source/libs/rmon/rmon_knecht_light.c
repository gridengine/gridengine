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

#include "rmon_rmon.h"
#include "rmon_daemon.h"

#include <stdio.h>
#include <unistd.h>
#include "msg_rmon.h"

void rmon_function_f(void);
void rmon_function_g(void);
int main(int argc, char *argv[]);

int main(
int argc,
char *argv[] 
) {

   int n, i;
   u_long job = 0;

   char *newargv[3];

#define FUNC "main"

   DOPEN("knecht_light");
   DENTER;

   if (argc == 2)
      sscanf(argv[1], "%d", &n);
   else
      n = 1;

   rmon_daemon(rmon_mmayclose);
   DJOBTRACE((job, MSG_RMON_STARTINGJOBX_D , (u32c)job));
   for (i = 0; i < 50; i++) {
      rmon_function_f();
      DPRINTF(("Dies ist der %d. Durchlauf von %s().\n", i, FUNC));
      sleep(0);
   }
   DJOBTRACE((job, MSG_RMON_JOBXHASSTOPPED_D, (u32c) job));

   if (--n) {
      sprintf(argv[1], "%d", n);
      newargv[0] = argv[0];
      newargv[1] = argv[1];
      newargv[2] = 0;
      DEXECVP((argv[0], newargv));
   }
   DEXIT;

   sleep(2);
   DCLOSE;
   return 1;
}

void rmon_function_f()
{
   int i;
   u_long job = 1;

#undef FUNC
#define FUNC "rmon_function_f"

   DPUSH_LAYER(TOP_LAYER);
   DENTER;

   DJOBTRACE((job, MSG_RMON_STARTINGJOBX_D,(u32c) job));
   for (i = 0; i < 3; i++) {
      DPRINTF(("Dies ist der %d. Durchlauf von %s().\n", i, FUNC));
      rmon_function_g();
   }
   DJOBTRACE((job, MSG_RMON_JOBXHASSTOPPED_D, (u32c)job));

   DEXIT;
   DPOP_LAYER;
   return;
}

void rmon_function_g()
{
   int i;
   u_long job = 1;

#undef FUNC
#define FUNC "rmon_function_g"

   DPUSH_LAYER(TOP_LAYER);
   DENTER;

   DJOBTRACE((job, MSG_RMON_STARTINGJOBX_D, (u32c)job));
   for (i = 0; i < 5; i++) {
      DPRINTF(("Dies ist der %d. Durchlauf von %s().\n", i, FUNC));
   }
   DJOBTRACE((job, MSG_RMON_JOBXHASSTOPPED_D, (u32c)job));

   DEXIT;
   DPOP_LAYER;
   return;
}
