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

#include "sge_washing_machine.h"
#include "opt_silent.h"

static washing_machine_t wtype;

void washing_machine_set_type(washing_machine_t type) 
{
   wtype = type;
}

void washing_machine_next_turn(void)
{
   static int cnt = 0;
   static char s[] = "-\\/";
   static char *sp = NULL;

   cnt++;
   if ((cnt % 100) != 1) {
      return;
   }

   switch (wtype) {
   case WASHING_MACHINE_ROTATING_BAR:
      {

         if (!silent()) {
            if (!sp || !*sp) {
               sp = s;
            }

            printf("%c\b", *sp++);
            fflush(stdout);
         }
      }
      break;
   case WASHING_MACHINE_DOTS: 
      if (!silent()) {
         printf(".");
         fflush(stdout);
      }
      break;
   default:
      break;
   }
}


void washing_machine_end_turn(void)
{
   switch (wtype) {
   case WASHING_MACHINE_ROTATING_BAR: 
      if (!silent()) {
         printf(" \b");
         fflush(stdout);
      }
      break;
   case WASHING_MACHINE_DOTS:
      if (!silent()) { 
         printf("\n");
         fflush(stdout);
      }
      break;
   default:
      break;
   }
}
