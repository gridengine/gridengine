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
/*
   Module containing all stuff concerning job logging.

   Job logging should give a means of seeing exactly what is happening
   with/to jobs. 

*/

#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#include "basis_types.h"
#include "job_log.h"
#include "sgermon.h"
#include "sge_prog.h"
#include "sge_time.h"
#include "sge_unistd.h"

static char job_log_file[SGE_PATH_MAX]="";

/*****************************************************************/
int is_active_job_logging(void)
{
   return job_log_file[0];
}

/*****************************************************************/
int enable_job_logging(char *fname) 
{
   DENTER(TOP_LAYER, "enable_job_logging");

   strcpy(job_log_file, fname);

   DPRINTF(("use file %s for job logging\n", job_log_file));
   DEXIT;
   return 0;
}

/*****************************************************************/
int disable_job_logging(
char *fname 
) {
   job_log_file[0] = '\0';
   return 0;
}

/******************************************************************
   Log string to logfile

   return:
     0 = success
     1 = job logging disabled
    -1 = error
 ******************************************************************/
int job_log(u_long32 job_number, u_long32 task_number, const char *str)
{
   FILE *fp;
   time_t now;
   char dummy_str[256], date[256];
   const char *progname;
   char *hostname;

   if (!job_log_file[0])
      return 1;

   progname = prognames[me.who];
   hostname = me.unqualified_hostname;

   if ((fp = fopen(job_log_file, "a"))) {

#if 1
      /* this date format is nice to read */
      now=time((time_t *)NULL);
      sprintf(dummy_str,"%s",ctime(&now));
      sscanf(dummy_str,"%[^\n]",date);
#else
      /* this date format is better for parsing */
      sprintf(date, u32, sge_get_gmt());
#endif

      /* add host and programtype info to log message */
      if (!task_number)
         fprintf(fp, "%s:%s:%s:"u32" %s\n", date, progname, hostname, job_number, str);
      else
         fprintf(fp, "%s:%s:%s:"u32"."u32" %s\n", date, progname, hostname, job_number, task_number, str);
      fclose(fp);
      return 0;
   }
   return -1;
}

