#ifndef __PARSE_RANGE_H
#define __PARSE_RANGE_H
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

 

#include "cull.h"

/*  parse_range.c */
#define PARSE_N_BUILD      0
#define JUST_PARSE         1
#define INF_ALLOWED        1
#define INF_NOT_ALLOWED    0

lList *parse_ranges(char *str, int just_parse, int step_allowed, lList **alpp, lList **rl, int inf_allowed);
int unparse_ranges(FILE *fp, char *buff, u_long32 max_len, lList *lp);
void show_ranges(char *buffer, int step_allowed, FILE *fp, lList *rlp);
int id_in_range(u_long32 id, lList *rlp);

#endif /* __PARSE_RANGE_H */

