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
#include <sys/ipc.h>
#include <sys/sem.h>

#include "rmon_err.h"
#include "rmon_def.h"
#include "rmon_rmon.h"
#include "rmon_request.h"
#include "rmon_semaph.h"

#include "rmon_job_list.h"
#include "rmon_job_protocol.h"
#include "rmon_monitoring_level.h"

#include "rmon_s_c_monitoring_level.h"

extern u_long message_counter;
extern u_long childdeath;
extern monitoring_box_type *monitoring_box_down;
extern int semid_down;
extern u_long all_jobs;

/*****************************************************************/

int rmon_s_c_monitoring_level(
int sfd 
) {
   int n;
   monitoring_level temp_level;
   u_long n_jobs;
   job_list_type *new_job_list = NULL;

#undef FUNC
#define FUNC "rmon_s_c_monitoring_level"
   DENTER;

   rmon_mlcpy(&temp_level, &request.level);
   n_jobs = request.n_job;
   all_jobs = request.kind;

   if (!rmon_send_ack(sfd, S_ACCEPTED)) {
      DEXIT;
      return 0;
   }

   if (n_jobs > 0) {
      for (n = 0; !rmon_receive_job_list(sfd, &new_job_list, n_jobs) && n < MAX_TRY; n++);
      if (n == MAX_TRY) {
         shutdown(sfd, 2);
         close(sfd);
         DEXIT;
         return 0;
      }
   }
   DPRINTF(("job list received\n"));

   request.childdeath = childdeath;
   if (!rmon_send_ack(sfd, S_ACCEPTED)) {
      DEXIT;
      return 0;
   }

   if (!childdeath) {
      DPRINTF(("semid_down: %d\n", semid_down));

      rmon_delete_jl(&job_list);
      job_list = new_job_list;

      /* Write monitoring_level (temp_level) into shared memory */
      if (!rmon_sem_wait(semid_down)) {
         DPRINTF(("sem_wait error \n"));
         exit(-1);
      }

      rmon_mlcpy(&monitoring_box_down->level, &temp_level);
      if (all_jobs)
         rmon_mlset(&monitoring_box_down->level, JOBTRACE);
      monitoring_box_down->valid = 1;

      if (!rmon_sem_signal(semid_down)) {
         DPRINTF(("sem_signal error \n"));
         exit(-1);
      }
   }
   else
      DPRINTF(("Child is dead !\n"));

   DEXIT;
   return 1;
}                               /* s_c_monitoring_level */
