#ifndef __RMON_JOB_LIST_H
#define __RMON_JOB_LIST_H
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

#include <sys/types.h>



#include "rmon_def.h"

#define ADD_ALL 5
#define DEL_ALL 6

typedef struct job_list_type {
   u_long jobid;
   struct job_list_type *next;
} job_list_type;

int rmon_insert_jl_in_jlp(job_list_type *new, job_list_type **jlp);
job_list_type **rmon_search_job_in_jl(u_long jobid, job_list_type **jl);
job_list_type *rmon_unchain_jl(job_list_type **jlp);
int rmon_delete_jl(job_list_type **jlp);
void rmon_print_jl(job_list_type *jl);
void rmon_print_job(job_list_type *jl);

extern job_list_type *job_list;

#endif /* __RMON_JOB_LIST_H */



