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
#include "rmon_rmon.h"
#include "rmon_convert.h"

char ptr[XXL];

/***************************************************************/

char *rmon_convertint(
char *l_ptr,
u_long *i 
) {
#undef  FUNC
#define FUNC "rmon_convertint"
   u_long j;

   j = htonl(*i);
   bcopy(((char *) &j) + INTOFF, l_ptr, INTSIZE);
   l_ptr += INTSIZE;
   return (l_ptr);
}

/***************************************************************/

char *rmon_convertstr(
char *l_ptr,
char *str 
) {
#undef  FUNC
#define FUNC "rmon_convertstr"

   bcopy(str, l_ptr, STRINGSIZE);
   l_ptr += STRINGSIZE;
   return (l_ptr);
}

/***************************************************************/

char *rmon_convertml(
char *l_ptr,
monitoring_level *ml 
) {
#undef  FUNC
#define FUNC "rmon_convertml"

   int i, n;
   u_long layer;

   n = rmon_mlnlayer();
   for (i = 0; i < n; i++) {
      layer = rmon_mlgetl(ml, i);
      l_ptr = rmon_convertint(l_ptr, &layer);
   }
   return (l_ptr);
}

/***************************************************************/

char *rmon_convertlstr(
char *l_ptr,
char *str 
) {
#undef  FUNC
#define FUNC "rmon_convertstr"

   bcopy(str, l_ptr, 3 * STRINGSIZE);
   l_ptr += 3 * STRINGSIZE;
   return (l_ptr);
}

/***************************************************************/

char *rmon_unconvertint(
char *l_ptr,
u_long *i 
) {
#undef  FUNC
#define FUNC "rmon_unconvertint"
   u_long j = 0;

   bcopy(l_ptr, (((char *) &j) + INTOFF), INTSIZE);
   l_ptr += INTSIZE;
   *i = ntohl(j);
   return (l_ptr);
}

/***************************************************************/

char *rmon_unconvertstr(
char *l_ptr,
char *str 
) {
#undef  FUNC
#define FUNC "rmon_unconvertstr"

   bcopy(l_ptr, str, STRINGSIZE);
   l_ptr += STRINGSIZE;
   return (l_ptr);
}

/***************************************************************/

char *rmon_unconvertlstr(
char *l_ptr,
char *str 
) {
#undef  FUNC
#define FUNC "rmon_unconvertstr"

   bcopy(l_ptr, str, 3 * STRINGSIZE);
   l_ptr += 3 * STRINGSIZE;
   return (l_ptr);
}

/***************************************************************/

char *rmon_unconvertml(
char *l_ptr,
monitoring_level *ml 
) {
#undef  FUNC
#define FUNC "rmon_convertml"

   int n, i;
   u_long layer;

   n = rmon_mlnlayer();

   for (i = 0; i < n; i++) {
      l_ptr = rmon_unconvertint(l_ptr, &layer);
      rmon_mlputl(ml, i, layer);
   }
   return (l_ptr);
}
