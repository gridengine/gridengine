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

#include "rmon_h.h"
#include "rmon_err.h"
#include "rmon_client_number.h"
#include "rmon_rmon.h"

#define FREE    0
#define USED    1

unsigned char client_number[MAX_CLIENTS];

void rmon_init_cn(void) {
   int i = 0;

   while (i < MAX_CLIENTS)
      client_number[i++] = FREE;
}

/**********************************************************/

u_long rmon_get_cn()
{
   u_long i;

   for (i = 0; i < MAX_CLIENTS && client_number[i] != FREE; i++);

   if (i == MAX_CLIENTS)
      return 0xffffffff;

   client_number[i] = USED;
   return i;
}

/**********************************************************/

void rmon_free_cn(
u_long i 
) {
   client_number[i] = FREE;
}
