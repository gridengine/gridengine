#ifndef __RMON_CONVERT_H
#define __RMON_CONVERT_H
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



#include "rmon_def.h"
#include "rmon_monitoring_level.h"

#define INTSIZE             4
#define ULONGSIZE           4

#if _UNICOS
#define INTOFF              4   /* big endian 64-bit machines where sizeof(int) = 8 */
#else
#define INTOFF              0   /* the rest of the world; see comments in request.c */
#endif

char *rmon_convertint(char *ptr, u_long *i);
char *rmon_convertstr(char *ptr, char *str);
char *rmon_convertlstr(char *ptr, char *str);
char *rmon_convertml(char *ptr, monitoring_level *ml);
char *rmon_unconvertint(char *ptr, u_long *i);
char *rmon_unconvertstr(char *ptr, char *str);
char *rmon_unconvertlstr(char *ptr, char *str);
char *rmon_unconvertml(char *ptr, monitoring_level *ml);

/* #define XXL (1 + 5)*ULONGSIZE+3*STRINGSIZE */
#define XXL (N_LAYER + 5)*ULONGSIZE+4*STRINGSIZE
extern char ptr[XXL];

#endif /* __RMON_CONVERT_H */



