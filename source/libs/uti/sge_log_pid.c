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
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sgermon.h"
#include "sge_log_pid.h"

/************************************************************************/
void sge_log_pid(
char *pid_log_file 
) {

   int pid;
   FILE *fp;

   DENTER(TOP_LAYER, "sge_log_pid");

   close(creat(pid_log_file, 0644));

   if ((fp = fopen(pid_log_file, "w")) != NULL) {
      pid = getpid();
      fprintf(fp, "%d\n", pid);
      fclose(fp);
   }

   DEXIT;
   return;

}


/************************************************************************/
int read_pid(
char *pid_log_file 
) {
   int pid=-1;
   FILE *fp;

   DENTER(TOP_LAYER, "read_pid");

   if ((fp = fopen(pid_log_file, "r")) != NULL) {
      fscanf(fp,"%d", &pid);
      fclose(fp);
   }

   DEXIT;
   return pid;
}
