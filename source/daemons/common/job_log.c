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
#include "sge_stat.h" 

static char job_log_file[SGE_PATH_MAX]="";

/*****************************************************************/
int is_active_job_logging(void)
{
   return job_log_file[0];
}

/*****************************************************************/
int enable_job_logging(
char *fname 
) {
   int i;
   SGE_STRUCT_STAT buf;

   DENTER(TOP_LAYER, "enable_job_logging");

   if (SGE_STAT(fname, &buf)) {
      if ((i = creat(fname, 0644)) == -1) {
         DPRINTF(("failed to create %s for job logging\n", fname));
         DEXIT;
         return -1;
      }
      close(i);
   }
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
int job_log(u_long32 job_number, const char *str, char *progname, 
            char *hostname)
{
   FILE *fp;
   time_t now;
   char dummy_str[256], date[256];

   if (!job_log_file[0])
      return 1;

   if ((fp = fopen(job_log_file, "a"))) {

      /* add host and programtype info to log message */

      now=time((time_t *)NULL);
      sprintf(dummy_str,"%s",ctime(&now));
      sscanf(dummy_str,"%[^\n]",date);

      fprintf(fp, "%s:%s:%s:"u32" %s\n", date, progname, hostname, job_number,
              str);
      fclose(fp);
      return 0;
   }
   return -1;
}

