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

#include "rmon_piped_message.h"
#include "rmon_s_c_overflow.h"

#include "rmon_job_list.h"
#include "rmon_message_list.h"
#include "rmon_monitoring_level.h"

extern message_list_type *last_element;
extern u_long message_counter;
extern u_long addr, port;
extern u_long childdeath;
extern u_long all_jobs;

/*****************************************************************/

int rmon_s_c_overflow(
piped_message_type *piped_message 
) {
   message_list_type *neu;
   monitoring_level jobtracing;

#undef FUNC
#define FUNC "rmon_s_c_overflow"
   DENTER;

   if (piped_message->childdeath) {
      childdeath = 1;
      DEXIT;
      return 1;
   }

   /* send only messages of traced jobs */
   rmon_mlclr(&jobtracing);
   rmon_mlset(&jobtracing, JOBTRACE);
   if (rmon_mland(&jobtracing, &piped_message->level)
       && !all_jobs
       && !rmon_search_job_in_jl(piped_message->jobid, &job_list)) {
      DEXIT;
      return 1;
   }

   neu = (message_list_type *) malloc(sizeof(message_list_type));
   if (!neu) {
      printf(MSG_RMON_MALLOCFAILED);
      DEXIT;
      DCLOSE;
      exit(-1);
   }

   DPRINTF(("DATA: %s\n", piped_message->data));

   strcpy(neu->data, piped_message->data);
   rmon_mlcpy(&neu->level, &piped_message->level);
   neu->inet_addr = ntohl(addr);
   neu->port = port;
   neu->pid = piped_message->pid;
   neu->jobid = piped_message->jobid;
   neu->next = NULL;

   /* append */
   if (message_list == NULL)
      message_list = neu;
   else
      last_element->next = neu;
   last_element = neu;

   message_counter++;

   DEXIT;
   return 1;
}
