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
#include "rmon_h.h"
#include "rmon_rmon.h"
#include "rmon_monitoring_level.h"
#include "rmon_def.h"

int rmon_mlnlayer()
{
   return N_LAYER;
}

/* *************************************************************** 

   NAME
   mlcheck() checks consistency of a monitoring level 

   RETURN VALUE
   0    consistency error
   1    ok

   int rmon_mlcheck( m )
   monitoring_level *m;
   {
   int j;

   for (j=0; j<N_LAYER; j++)  
   if (NO_LEVEL
   }
 */

/* *************************************************************** 

   NAME
   rmon_mlcmp() compares first and second monitoring level

 */
int rmon_mlcmp(
monitoring_level *first,
monitoring_level *second 
) {
   int j;
   int ret;

#undef FUNC
#define FUNC "rmon_mlcmp"

   DENTER;

   for (j = 0; j < N_LAYER; j++)
      if ((ret = (second->ml[j] - first->ml[j])) != 0)
         return ret;

   DEXIT;

   return 0;
}

/* *************************************************************** 

   NAME
   rmon_mliszero() tests if the monitoring level is zero 

 */
int rmon_mliszero(
monitoring_level *m 
) {
   int j;

#undef FUNC
#define FUNC "rmon_mliszero"

   DENTER;

   for (j = 0; j < N_LAYER; j++)
      if (m->ml[j] != 0) {
         DEXIT;
         return 0;
      }

   DEXIT;

   return 1;
}

/* *************************************************************** 

   NAME
   rmon_mland() performs

   d(estination) &= s(ource)

   RETURN VALUE
   returns 0 if destination is 0

 */

int rmon_mland(
monitoring_level *d,
monitoring_level *s 
) {
   int j;
   u_long sum = 0;

#undef FUNC
#define FUNC "rmon_mland"

   DENTER;

   if (s) {
      for (j = 0; j < N_LAYER; j++) {
         d->ml[j] &= s->ml[j];
         sum |= d->ml[j];
      }
      DEXIT;
      return sum;
   }
   else {
      DPRINTF(("WARNING: mland( dest, NULL ) is not supported\n"));
      DEXIT;
      return 0;
   }
}

/* *************************************************************** 

   NAME
   rmon_mlor() performs

   d(estination) |= s(ource)

 */

void rmon_mlor(
monitoring_level *d,
monitoring_level *s 
) {
   int j;

#undef FUNC
#define FUNC "rmon_mlor"

   DENTER;

   if (s)
      for (j = 0; j < N_LAYER; j++)
         d->ml[j] |= s->ml[j];

   DEXIT;
   return;
}

/* *************************************************************** 

   NAME
   rmon_mlcpy() performs

   d(estination) = s(ource)

 */

void rmon_mlcpy(
monitoring_level *d,
monitoring_level *s 
) {
   int j;

#undef FUNC
#define FUNC "rmon_mlcpy"

   DENTER;

   for (j = 0; j < N_LAYER; j++)
      d->ml[j] = s->ml[j];

   DEXIT;

   return;
}

/* *************************************************************** 

   NAME
   rmon_mlclr() performs

   d(estination) = 0

 */

void rmon_mlclr(
monitoring_level *d 
) {
   int j;

#undef FUNC
#define FUNC "rmon_mlclr"

   DENTER;

   for (j = 0; j < N_LAYER; j++)
      d->ml[j] = 0;

   DEXIT;

   return;
}

/* *************************************************************** 

   NAME
   rmon_mlset() sets in d(estination) the bitmask given in  
   to_set

 */

void rmon_mlset(
monitoring_level *d,
u_long to_set 
) {
   int j;

#undef FUNC
#define FUNC "rmon_mlset"

   DENTER;

   for (j = 0; j < N_LAYER; j++)
      d->ml[j] |= to_set;

   DEXIT;
   return;
}

/* *************************************************************** 

   NAME
   rmon_mlgetl() returns the bitmask for layer i 

 */
u_long rmon_mlgetl(
monitoring_level *s,
int i 
) {
   return s->ml[i];
}

/* *************************************************************** 

   NAME
   rmon_mlputl() sets the bitmask for layer i 

 */
void rmon_mlputl(
monitoring_level *s,
int i,
u_long mask 
) {
   s->ml[i] = mask;
}

/* *************************************************************** 

   NAME
   rmon_mlprint() prints a monitoring level as a list of decimal numbers 

 */
void rmon_mlprint(
monitoring_level *ml 
) {
   int i;

#undef FUNC
#define FUNC "rmon_mlprint"

   DENTER;

   for (i = 0; i < N_LAYER; i++)
      DPRINTF(("%2ld ", ml->ml[i]));

   DEXIT;
   return;
}
