#ifndef __SGE_PARSE_NUM_PAR_H
#define __SGE_PARSE_NUM_PAR_H
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

#ifndef WIN32NATIVE
#	include <sys/time.h>
#	include <sys/resource.h>
#endif


#include "cull_list.h"

/* type wrapper */
#if defined(CRAY) || defined(LINUX)
   typedef long sge_rlim_t;
#elif defined(NECSX4) || defined(NECSX5)
   typedef long long sge_rlim_t;
#elif IRIX6
   typedef rlim64_t sge_rlim_t;
#elif SUN4 || HPUX || HP10 || AIX41
   typedef int sge_rlim_t;
#elif WIN32NATIVE
   typedef long sge_rlim_t;
#else
   typedef rlim_t sge_rlim_t;
#endif

/* compare two rlimit entities return values like strcmp() */
int rlimcmp(sge_rlim_t r1, sge_rlim_t r2);

sge_rlim_t mul_infinity(sge_rlim_t rlim, sge_rlim_t muli);

int parse_ulong_val(double *dvalp, u_long32 *uvalp, u_long32 type, char *s, char *err_str, int err_len);

int sge_parse_loglevel_val(u_long32 *uval, char *s);


int extended_parse_ulong_val(double *dvalp, u_long32 *uvalp, u_long32 type, char *s, char *err_str, int err_len, int enable_infinity);


int sge_parse_limit(sge_rlim_t *rlvalp, char *s, char *error_str, int error_len);

int sge_parse_checkpoint_attr(char *attr_str);

char *resource_descr(double dval, u_long32 type, char *buffer);

char *get_checkpoint_when(int bitmask);

int is_checkpoint_when_valid(int bitmask);

#endif /* __SGE_PARSE_NUM_PAR_H */

