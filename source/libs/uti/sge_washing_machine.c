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

/****** uti/sge/washing_machine_set_type() ************************************
*  NAME
*     washing_machine_set_type() -- set display mod of 'wasching machine' 
*
*  SYNOPSIS
*     void washing_machine_set_type(washing_machine_t type) 
*
*  FUNCTION
*     With 'WASHING_MACHINE_ROTATING_BAR' each call of 
*     washing_machine_next_turn() will show a rotating bar.
*     In 'WASHING_MACHINE_DOTS'-mode each call will show more dots in a line.
*
*  INPUTS
*     washing_machine_t type - display type
*        WASHING_MACHINE_ROTATING_BAR
*        WASHING_MACHINE_DOTS 
******************************************************************************/
void washing_machine_set_type(washing_machine_t type) 
{
   wtype = type;
}

/****** uti/sge/washing_machine_next_turn() ***********************************
*  NAME
*     washing_machine_next_turn() -- show next turn 
*
*  SYNOPSIS
*     void washing_machine_next_turn(void) 
*
*  FUNCTION
*     Show next turn of rotating washing machine.
******************************************************************************/
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

/****** uti/sge/washing_machine_end_turn() ************************************
*  NAME
*     washing_machine_end_turn() -- remove washing machine from display 
*
*  SYNOPSIS
*     void washing_machine_end_turn(void) 
*
*  FUNCTION
*     Last turn of washing machine.
******************************************************************************/
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
