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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "sge.h"
#include "sge_jobL.h"
#include "sgermon.h"
#include "cull_file.h"
#include "read_write_job.h"



/****
 **** cull_read_job
 ****
 **** reads a job from the file 'filename'
 ****/
lListElem *cull_read_job(
char *filename 
) {
   lListElem *jep;

   DENTER(TOP_LAYER, "cull_read_job");

   jep = lReadElemFromDisk(NULL, filename, JB_Type, "job");

   DEXIT;
   return jep;
}

/****
 **** cull_write_job_to_disk
 ****
 **** writes the job 'jep' to a file. The filename
 **** is created of 'JOB_DIR'/ followed by the jobnumber.
 ****/
int cull_write_job_to_disk(
lListElem *jep 
) {
   int ret;
    
   DENTER(TOP_LAYER, "cull_write_job_to_disk");

   ret = cull_write_jobtask_to_disk(jep, 0);

   DEXIT;
   return ret;
}

/****
 **** cull_write_jobtask_to_disk
 ****
 **** writes the job 'jep' to a file. The filename
 **** is created of 'JOB_DIR'/ followed by the jobnumber and the i
 **** given ja taskid.
 ****/

int cull_write_jobtask_to_disk(
lListElem *jep,
u_long32 ja_taskid 
) {
   int ret;
   stringT tmpstr, real_tmpstr;
   u_long32 jobid;
    
   DENTER(TOP_LAYER, "cull_write_jobtask_to_disk");

   jobid = lGetUlong(jep, JB_job_number);
   /* create filename */
   if (ja_taskid == 0) {
      sprintf(tmpstr, "%s/."u32"", JOB_DIR, jobid);
      sprintf(real_tmpstr, "%s/"u32"", JOB_DIR, jobid);
      DPRINTF(("writing job "u32" to disk\n", jobid));
   } else {
      sprintf(tmpstr, "%s/."u32"."u32"", JOB_DIR, jobid, ja_taskid);
      sprintf(real_tmpstr, "%s/"u32"."u32"", JOB_DIR, jobid, ja_taskid);
      DPRINTF(("writing job "u32"."u32" to disk\n", jobid, ja_taskid));
   }

   /* write job */
   ret = lWriteElemToDisk(jep, tmpstr, NULL, "job");

   if (!ret && (rename(tmpstr, real_tmpstr) == -1)) {
      DEXIT;
      return 1; 
   }

   DEXIT; 
   return ret;
}

/****
 **** cull_write_zombiejob_to_disk
 ****
 **** writes the job 'jep' to a file. The filename
 **** is created of 'ZOMBIE_DIR'/ followed by the jobnumber and the 
 **** given ja taskid.
 ****/

int cull_write_zombiejob_to_disk(
lListElem *jep 
) {
   int ret;
   stringT tmpstr, real_tmpstr;
   u_long32 jobid;

   DENTER(TOP_LAYER, "cull_write_zombiejob_to_disk");

   jobid = lGetUlong(jep, JB_job_number);
   /* create filename */
   sprintf(tmpstr, "%s/."u32"", ZOMBIE_DIR, jobid);
   sprintf(real_tmpstr, "%s/"u32"", ZOMBIE_DIR, jobid);
   DPRINTF(("writing job "u32" to disk\n", jobid));

   /* write job */
   ret = lWriteElemToDisk(jep, tmpstr, NULL, "job");

   if (!ret && (rename(tmpstr, real_tmpstr) == -1)) {
      DEXIT;
      return 1;
   }   
   
   DEXIT;
   return ret;
}



