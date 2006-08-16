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

#include "sgeobj/sge_centry.h"
#include "sched/sge_resource_utilization.h"
#include "rmon/sgermon.h"
#include "sge_all_listsL.h"
#include "sge_mt_init.h"

#include "sge_qeti.h"

int main(int argc, char *argv[]) 
{
   lListElem *cr;
   lList *cr_list;
   sge_qeti_t *iter;
   u_long32 pe_time;
   double uti;
   int ret = 0;
   int i = 0;

   static u_long32 qeti_expected_result[] = {
      1200,
      1100,
      1000,
      800
   };

   DENTER_MAIN(TOP_LAYER, "test_resource_utilization");

   sge_mt_init();

   lInit(nmv);

   cr = lCreateElem(RUE_Type);
   lSetString(cr, RUE_name, "slots");

   printf("adding a 200s now assignment of 8 starting at 800\n");
   utilization_add(cr, 800, 200, 8, 100, 1, PE_TAG, "pe_slots", "STARTING");   
   utilization_print(cr, "pe_slots");

   printf("adding a 100s now assignment of 4 starting at 1000\n");
   utilization_add(cr, 1000, 100, 4, 101, 1, PE_TAG, "pe_slots", "STARTING");   
   utilization_print(cr, "pe_slots");

   printf("adding a 100s reservation of 8 starting at 1100\n");
   utilization_add(cr, 1100, 100, 8, 102, 1, PE_TAG, "pe_slots", "RESERVING");   
   utilization_print(cr, "pe_slots");

   uti = utilization_max(cr, 1000, 100);
   if (uti != 4) {
      printf("failed: utilization(cr, 1000, 100) returned %f, expected 4\n", uti);
      ret++;
   } else {
      printf("sucess: utilization(cr, 1000, 100) returned %f\n", uti);
   }
   
   uti = utilization_max(cr, 1200, 150);
   if (uti != 0) {
      printf("failed: utilization(cr, 1200, 150) returned %f, expected 4\n", uti);
      ret++;
   } else {
      printf("sucess: utilization(cr, 1200, 150) returned %f\n", uti);
   }

   uti = utilization_max(cr, 700, 150);
   if (uti != 8) {
      printf("failed: utilization(cr, 700, 150) returned %f, expected 4\n", uti);
      ret++;
   } else {
      printf("sucess: utilization(cr, 700, 150) returned %f\n", uti);
   }

   /* use a QETI to iterate through times */

   /* sge_qeti_allocate() */
   cr_list = lCreateList("", RUE_Type);
   lAppendElem(cr_list, cr);
   iter = sge_qeti_allocate2(cr_list);

   /* sge_qeti_first() */
   for (pe_time = sge_qeti_first(iter), i=0; pe_time; pe_time = sge_qeti_next(iter), i++) {
      if (qeti_expected_result[i] != pe_time) {
         printf("failed: qeti returned "sge_u32", expected "sge_u32"\n", pe_time, qeti_expected_result[i]);
         ret++;
      } else {
         printf("success: QETI returned "sge_u32"\n", pe_time);
      }
   }

   sge_qeti_release(&iter);

   if (ret != 0) {
      printf("\ntest failed!\n");
   }

   return ret;
}
