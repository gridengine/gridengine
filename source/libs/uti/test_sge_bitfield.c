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

#define MALLINFO

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/times.h>

#ifdef MALLINFO
#include <malloc.h>
#endif

#include "uti/sge_bitfield.h"

const int test_bf_max_size = 100;
int test_bf_loops    = 1000;

static void test_nullpointer_actions()
{
   bitfield b1;

   /* bitfields of size 0 */
   b1 = sge_bitfield_new(0);
   b1 = sge_bitfield_free(b1);

   b1 = sge_bitfield_new(10);

   /* pass nullpointer */
   sge_bitfield_free(NULL);

   sge_bitfield_copy(b1, NULL);
   sge_bitfield_copy(NULL, b1);
   sge_bitfield_copy(NULL, NULL);

   sge_bitfield_bitwise_copy(b1, NULL);
   sge_bitfield_bitwise_copy(NULL, b1);
   sge_bitfield_bitwise_copy(NULL, NULL);

   sge_bitfield_set(NULL, 0);
   sge_bitfield_get(NULL, 0);
   sge_bitfield_clear(NULL, 0);
   sge_bitfield_reset(NULL);
   sge_bitfield_changed(NULL);
   sge_bitfield_print(NULL, stdout);
   sge_bitfield_print(b1, NULL); /* shall output to stdout */
   printf("\n");

   /* free bitfield */ 
   b1 = sge_bitfield_free(b1);
}

int main(int argc, char *argv[])
{
   int i, k;
   bitfield *b1, *b2;

#ifdef MALLINFO
   struct mallinfo meminfo;
#endif

   long clk_tck = 0;
   const char *header_format = "%5s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s\n";
   const char *data_format =   "%5d %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8d\n";

   /* evaluate commandline args */
   if (argc > 1) {
      test_bf_loops = atoi(argv[1]);
   }

   /* create array to hold bitfields, to be used per test loop */
   b1 = (bitfield *)malloc(test_bf_loops * sizeof(bitfield));
   b2 = (bitfield *)malloc(test_bf_loops * sizeof(bitfield));
   printf("performing %d loops per action\n", test_bf_loops);

   /* test nullpointer actions */
   printf("\ntesting nullpointer actions\n");
   test_nullpointer_actions();

   /* test performance */
   clk_tck = sysconf(_SC_CLK_TCK); /* JG: TODO: sge_sysconf? */
   printf("\ntesting performance of all actions\n");
   printf(header_format, "size", "create", "free", 
          "copy", "bwcopy", 
          "set", "get", 
          "clear", "reset", "changed", "mem");

   for (i = 0; i <= test_bf_max_size; i += 10) {
      struct tms tms_buffer;
      clock_t now, start;
      double prof_create = 0.0;
      double prof_free = 0.0;
      double prof_copy = 0.0;
      double prof_bwcopy = 0.0;
      double prof_set = 0.0;
      double prof_get = 0.0;
      double prof_clear = 0.0;
      double prof_reset = 0.0;
      double prof_changed = 0.0;

      int loops, index;

      srand(time(0));

      /* creation of bitfields */
      start = times(&tms_buffer);
      for (loops = 0; loops < test_bf_loops; loops++) {
         b1[loops] = sge_bitfield_new(i);
         b2[loops] = sge_bitfield_new(i);
      }   
      now = times(&tms_buffer);
      prof_create = (now - start) * 1.0 / clk_tck;

      /* copy */
      start = times(&tms_buffer);
      for (loops = 0; loops < test_bf_loops; loops++) {
         sge_bitfield_copy(b1[loops], b2[loops]);
      }   
      now = times(&tms_buffer);
      prof_copy = (now - start) * 1.0 / clk_tck;

      /* bitwise copy */
      start = times(&tms_buffer);
      for (loops = 0; loops < test_bf_loops; loops++) {
         sge_bitfield_bitwise_copy(b2[loops], b2[loops]);
      }   
      now = times(&tms_buffer);
      prof_bwcopy = (now - start) * 1.0 / clk_tck;

      /* set/get/clear */
      index = rand() % (i + 1) - 1;
      start = times(&tms_buffer);
      for (loops = 0; loops < test_bf_loops; loops++) {
         sge_bitfield_set(b1[loops], index);
      }   
      now = times(&tms_buffer);
      prof_set = (now - start) * 1.0 / clk_tck;

      start = times(&tms_buffer);
      for (loops = 0; loops < test_bf_loops; loops++) {
         sge_bitfield_get(b1[loops], index);
      }   
      now = times(&tms_buffer);
      prof_get = (now - start) * 1.0 / clk_tck;

      start = times(&tms_buffer);
      for (loops = 0; loops < test_bf_loops; loops++) {
         sge_bitfield_clear(b1[loops], index);
      }
      now = times(&tms_buffer);
      prof_clear = (now - start) * 1.0 / clk_tck;

      /* reset */
      start = times(&tms_buffer);
      for (loops = 0; loops < test_bf_loops; loops++) {
         sge_bitfield_reset(b1[loops]);
      }
      now = times(&tms_buffer);
      prof_reset = (now - start) * 1.0 / clk_tck;
      
      /* changed */
      start = times(&tms_buffer);
      for (loops = 0; loops < test_bf_loops; loops++) {
         sge_bitfield_changed(b1[loops]);
      }
      now = times(&tms_buffer);
      prof_changed = (now - start) * 1.0 / clk_tck;

      /* evaluate memory usage */
#ifdef MALLINFO
      meminfo = mallinfo();
#endif

      /* freeing of bitfields */
      start = times(&tms_buffer);
      for (loops = 0; loops < test_bf_loops; loops++) {
         b1[loops] = sge_bitfield_free(b1[loops]);
         b2[loops] = sge_bitfield_free(b2[loops]);
      }
      now = times(&tms_buffer);
      prof_free = (now - start) * 1.0 / clk_tck;

      printf(data_format, i, prof_create, prof_free,
             prof_copy, prof_bwcopy,
             prof_set, prof_get, prof_clear, 
             prof_reset, prof_changed,
#ifdef MALLINFO
          (meminfo.usmblks + meminfo.uordblks) / 1024
#else
          0
#endif
             );
   }

   /* free memory used for bitfields */
   free(b1);
   free(b2);
   return EXIT_SUCCESS;
}
