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
   dstring error = DSTRING_INIT;

   /* initialize */
   if(!prof_start(&error)) {
      fprintf(stderr, sge_dstring_get_string(&error)); fflush(stderr);
      sge_dstring_clear(&error);
   }

   if(!prof_set_level_name(SGE_PROF_CUSTOM1, "test", &error)) {
      fprintf(stderr, sge_dstring_get_string(&error)); fflush(stderr);
      sge_dstring_clear(&error);
   }
   printf("after start:\n");

   printf("%s\n", prof_get_info_string(SGE_PROF_ALL, false, &error));

   /* sleep and measure time */
   PROF_START_MEASUREMENT(SGE_PROF_MIRROR);
   sleep(5);
   PROF_STOP_MEASUREMENT(SGE_PROF_MIRROR);
   printf("after sleep(5):\n");
   printf("%s\n", prof_get_info_string(SGE_PROF_ALL, false, &error));

   /* work and measure time */
   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);
   for(i = 0; i < 1000000; i++) {
      x = sin(i % 10);
      y = cos(i % 10);
   }
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
   printf("after working: \n");
   printf("%s\n", prof_get_info_string(SGE_PROF_ALL, false, &error));
   printf("with subusage: \n");
   printf("%s\n", prof_get_info_string(SGE_PROF_ALL, true, &error));

   /* reset profiling, verify data */
   if(!prof_reset(&error)) {
      fprintf(stderr, sge_dstring_get_string(&error)); fflush(stderr);
      sge_dstring_clear(&error);
   }
   printf("after reset: \n");
   printf("%s\n", prof_get_info_string(SGE_PROF_ALL, false, &error));

   if(!prof_stop(&error)) {
      fprintf(stderr, sge_dstring_get_string(&error)); fflush(stderr);
      sge_dstring_clear(&error);
   }
   return EXIT_SUCCESS;
}
