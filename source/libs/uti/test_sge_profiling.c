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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "sge_profiling.h"

int main(int argc, char *argv[])
{
   int i;
   double x, y;

   /* initialize */
   profiling_start();
   printf("after start:    %s\n", profiling_get_info_string());

   /* sleep and measure time */
   PROFILING_START_MEASUREMENT;
   sleep(5);
   PROFILING_STOP_MEASUREMENT;
   printf("after sleep(5): %s\n", profiling_get_info_string());

   /* work and measure time */
   PROFILING_START_MEASUREMENT;
   for(i = 0; i < 1000000; i++) {
      x = sin(i % 10);
      y = cos(i % 10);
   }
   PROFILING_STOP_MEASUREMENT;
   printf("after working:  %s\n", profiling_get_info_string());

   /* reset profiling, verify data */
   profiling_reset();
   printf("after reset:    %s\n", profiling_get_info_string());

   profiling_stop();
   return EXIT_SUCCESS;
}
