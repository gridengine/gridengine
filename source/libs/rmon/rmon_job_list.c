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
#define DEBUG

#include "rmon_h.h"
#include "rmon_def.h"
#include "rmon_err.h"
#include "rmon_rmon.h"

#include "rmon_job_list.h"

job_list_type *job_list;

int rmon_insert_jl_in_jlp(
job_list_type *new,
job_list_type **jlp 
) {
#undef FUNC
#define FUNC "rmon_insert_jl_in_jlp"
   DENTER;

   if (!new) {
      DPRINTF(("invalid new job\n"));
      DEXIT;
      return 0;
   }

   /* insert for key job */
   while (*jlp && new->jobid > (*jlp)->jobid)
      jlp = &((*jlp)->next);

   if (*jlp && (*jlp)->jobid == new->jobid) {
      DPRINTF(("no double entries\n"));
      DEXIT;
      return 0;
   }

   /* link for job key */
   new->next = *jlp;
   *jlp = new;

   DEXIT;
   return 1;
}                               /* insert_jl() */

/**********************************************************/

job_list_type **rmon_search_job_in_jl(
u_long jobid,
job_list_type **jlp 
) {
#undef  FUNC
#define FUNC "rmon_search_job_in_jl"
   DENTER;

   while (*jlp && jobid > (*jlp)->jobid)
      jlp = &((*jlp)->next);

   DEXIT;
   return *jlp ? ((*jlp)->jobid == jobid ? jlp : NULL) : NULL;
}

/**********************************************************/

int rmon_delete_jl(
job_list_type **jlp 
) {
   job_list_type *jl;

#undef FUNC
#define FUNC "rmon_delete_jl"
   DENTER;

   if (!jlp) {
      DPRINTF(("invalid job list\n"));
      DEXIT;
      return 0;
   }

   while ((jl = *jlp)) {
      *jlp = jl->next;
      free(jl);
   }

   DEXIT;
   return 1;
}                               /* delete_jl */

/**********************************************************/

void rmon_print_jl(
job_list_type *jl 
) {
#undef FUNC
#define FUNC "rmon_print_jl"
   DENTER;

   if (!jl) {
      DPRINTF(("job_list = NULL !\n"));
      DEXIT;
      return;
   }

   DPRINTF(("job\n"));
   while (jl) {
      rmon_print_job(jl);
      jl = jl->next;
   }

   DEXIT;
   return;
}                               /* rmon_print_jl() */

/**********************************************************/

void rmon_print_job(
job_list_type *jl 
) {
   if (!jl) {
      DPRINTF(("job = NULL !\n"));
      return;
   }

   DPRINTF(("%6d\n", jl->jobid));

   return;
}                               /* rmon_print_job() */

/**********************************************************/

job_list_type *rmon_unchain_jl(
job_list_type **jlp 
) {
   job_list_type *jl;

#undef FUNC
#define FUNC "rmon_unchain_jl"
   DENTER;

   jl = *jlp;
   *jlp = (*jlp)->next;

   DEXIT;
   return jl;
}                               /* unchain_jl */
