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
#include "rmon_sge_rmon.h"
#include "rmon_daemon.h"

void main(int argc, char *argv[]);
static void child(void);
static void f(int i);

void main(
int argc,
char *argv[] 
) {
   int i;

   DENTER_MAIN("knecht");

/*      rmon_daemon( rmon_mmayclose ); */

   for (i = 0; i < 10; i++) {
      DPRINTF(("Dies ist der %d. Durchlauf vom main.\n", i));
   }

   if (!fork())
      child();

   for (i = 0; i < 10; i++) {
      DPRINTF(("Dies ist der %d. Durchlauf vom parent.\n", i));
   }

   DCLOSE;

}

static void child()
{
   int i;

   DENTERL("child");

   for (i = 0; i < 10; i++) {
      DPRINTF(("Dies ist der %d. Durchlauf vom Child.\n", i));
      f(10);
   }
   DEXITL;
   DCLOSE;

   exit(0);
}

static void f(int i)
{

   DENTERL("f");

   if (i > 0) {
      DPRINTF(("i = %d\n", i));
      f(i - 1);
   }

   DEXITL;
   return;
}
