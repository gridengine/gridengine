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
#include <string.h>

#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "job_log.h"
#include "job.h"
#include "mail.h"
#include "sge_prognames.h"
#include "sge_me.h"
#include "sge_rangeL.h"

extern lList *Master_Job_List;

/**********************************************************************
  return
    ==JTYPE_JOB          -  job is a real job
    ==JTYPE_JOB_ARRAY    -  job is job array
 */
int is_array(lListElem *job) 
{
   u_long32 start, end, step;
   
   job_get_ja_task_ids(job, &start, &end, &step);
   return (start != 1 || end != 1 || step != 1) ? JTYPE_JOB_ARRAY : JTYPE_JOB;
}

int job_get_number_of_ja_tasks(lListElem *job)
{
   u_long32 start, end, step;
 
   job_get_ja_task_ids(job, &start, &end, &step);
   return ((end - start) / step + 1);
}   

/***********************************************************************/
int job_get_ja_task_ids(lListElem *job, u_long32 *start, u_long32 *end, 
                        u_long32 *step) 
{
   lListElem *range_elem = NULL; /* RN_Type */

   range_elem = lFirst(lGetList(job, JB_ja_structure));
   if (range_elem) {
      u_long32 tmp_step;

      *start = lGetUlong(range_elem, RN_min);
      *end = lGetUlong(range_elem, RN_max);
      tmp_step = lGetUlong(range_elem, RN_step);
      *step = tmp_step ? tmp_step : 1;
   } else {
      *start = *end = *step = 1; 
   }
   return 0;
}

int job_set_ja_task_ids(lListElem *job, u_long32 start, u_long32 end, 
                    u_long32 step)
{
   lListElem *range_elem;

   range_elem = lFirst(lGetList(job, JB_ja_structure));
   if (!range_elem) {
      lList *range_list;

      range_elem = lCreateElem(RN_Type);
      range_list = lCreateList("task id range", RN_Type);
      if (!range_elem || !range_list) {
         range_elem = lFreeElem(range_elem);
         range_list = lFreeList(range_list);
         return 1;
      }
      lAppendElem(range_list, range_elem);
      lSetList(job, JB_ja_structure, range_list);
   }
   lSetUlong(range_elem, RN_min, start);
   lSetUlong(range_elem, RN_max, end);
   lSetUlong(range_elem, RN_step, step);
   return 0;
}


#if 0

/**********************************************************************
 Log a whole job. I dont put it in job_log, cause I dont want to
 include all stuff.

 Give useful informations (not too much)
 */
int job_log_alljobs()
{
   char str[256];
   lListElem *jep; 

   job_log(0, "printing a list of all known jobs", 
           prognames[me.who], me.unqualified_hostname);
   for_each(jep, Master_Job_List) {
      lListElem* ja_task;
      
      for_each (ja_task, lGetList(jep, JB_ja_tasks)) {
         sprintf(str, MSG_LOG_SCRIPTSTATEOWNER_SUS,
                  lGetString(jep, JB_script_file), u32c(lGetUlong(ja_task, JAT_state)), 
                  lGetString(jep, JB_owner));
         job_log(lGetUlong(jep, JB_job_number), str, prognames[me.who], 
                 me.unqualified_hostname);
      }
   }

   return 0;
}

#endif
