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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

/* for service provider info (SPI) entries and projects */
#if defined(IRIX6)
#   include <sys/extacct.h>
#   include <proj.h>
#endif

#if defined(CRAY)
#include <sys/time.h>
#include <errno.h>
#   if !defined(SIGXCPU)
#       define SIGXCPU SIGCPULIM
#   endif
    /* for killm category on Crays */
#   include <sys/category.h>
struct rusage {
   struct timeval ru_stime;
   struct timeval ru_utime;
};
    /* for job/session stuff */
#   include <sys/session.h>
    /* times() */
#   include <sys/times.h>
#endif

#if defined(NECSX4) || defined(NECSX5)
#  include <string.h> 
#  include <sys/stat.h>
#  include <fcntl.h>
#  include <errno.h>
#  include <sys/types.h>
#  include <sys/disp.h>
#  include <sys/rsg.h> 

#  define NEC_UNDEF_VALUE (-999)
#endif 

#include "basis_types.h"
#include "config_file.h"
#include "err_trace.h"
#include "setenv.h"
#include "sge_uidgid.h"
#include "setosjobid.h"

#if defined(NECSX4) || defined(NECSX5)
static void print_scheduling_parameters(dispset2_t attr);
#endif

#if defined(NECSX4) || defined(NECSX5)

static void print_scheduling_parameters(
dispset2_t attr 
) {
   char message_str[256];

   sprintf(message_str, "basepr     %d", attr.basepri);
   shepherd_trace(message_str);
   sprintf(message_str, "modcpu     %d", attr.modcpu);
   shepherd_trace(message_str);
   sprintf(message_str, "tickcnt    %d", attr.tickcnt);
   shepherd_trace(message_str);
   sprintf(message_str, "dcyfctr    %d", attr.dcyfctr);
   shepherd_trace(message_str);
   sprintf(message_str, "dcyintvl   %d", attr.dcyintvl);
   shepherd_trace(message_str);
   sprintf(message_str, "tmslice    %d", attr.tmslice);
   shepherd_trace(message_str);
   sprintf(message_str, "mempri     %d", attr.mempri);
   shepherd_trace(message_str);
   sprintf(message_str, "szefctmrt  %d", attr.szefctmrt);
   shepherd_trace(message_str);
   sprintf(message_str, "priefctmrt %d", attr.priefctmrt);
   shepherd_trace(message_str);
   sprintf(message_str, "minmrt     %d", attr.minmrt);
   shepherd_trace(message_str);
   sprintf(message_str, "agrange    %d", attr.agrange);
   shepherd_trace(message_str);
   sprintf(message_str, "spinherit  %d", attr.spinherit);
   shepherd_trace(message_str);
   sprintf(message_str, "concpu     %d", attr.concpu);
   shepherd_trace(message_str);
}

#endif 

void setosjobid(pid_t sid, gid_t *add_grp_id_ptr, struct passwd *pw)
{
   FILE *fp=NULL;
   char err_str[2*SGE_PATH_MAX+128];

   sprintf(err_str, "setosjobid: uid = %d, euid = %d", getuid(), geteuid());
   shepherd_trace(err_str);

#  if defined(SOLARIS) || defined(ALPHA) || defined(LINUX)
      /* Read SgeId from config-File and create Addgrpid-File */
      {  
         char *cp;
         if ((cp = search_conf_val("add_grp_id")))
            *add_grp_id_ptr = atol(cp);
         else
            *add_grp_id_ptr = 0;
      }
      if (!(fp = fopen("addgrpid", "w")))
         shepherd_error("can't open \"addgrpid\" file");   
      fprintf(fp, gid_t_fmt"\n", *add_grp_id_ptr);
      fclose(fp);   
#  else
   {
      char osjobid[100];

         if (!(fp = fopen("osjobid", "w")))
            shepherd_error("can't open \"osjobid\" file");


         if(sge_switch2start_user() == 0) {
#     if defined(IRIX6)
      {
         /* The following block contains the operations necessary for
          * IRIX6.2 (and later) to set array session handles (ASHs) and
          * service provider info (SPI) records
          */
         struct acct_spi spi;
         int ret;
         char *cp;

         shepherd_trace("in irix code");
         /* get _local_ array session id */
         if ((ret=newarraysess())) {
            sprintf(err_str, "error: can't create ASH; errno=%d", ret);
            shepherd_error(err_str);
         }

         /* retrieve array session id we just assigned to the process and
          * write it to the os-jobid file
          */
         sprintf(osjobid, "%lld", getash());
         shepherd_trace(osjobid); 
         /* set service provider information (spi) record */
         strncpy(spi.spi_company, "SGE", 8);
         strncpy(spi.spi_initiator, get_conf_val("spi_initiator"), 8);
         strncpy(spi.spi_origin, get_conf_val("queue"),16);
         strcpy(spi.spi_spi, "Job ");
         strncat(spi.spi_spi, get_conf_val("job_id"),11);
         if ((ret=setspinfo(&spi))) {
            sprintf(err_str, "error: can't set SPI; errno=%d", ret);
            shepherd_error(err_str);
         }
         
         if ((cp = search_conf_val("acct_project"))) {
            prid_t proj; 
            if (strcasecmp(cp, "none") && ((proj = projid(cp)) >= 0)) {
               sprintf(err_str, "setting project \"%s\" to id %lld", cp, proj);
               shepherd_trace(err_str);
               if (setprid(proj) == -1)
                  shepherd_trace("failed setting project id");
            }
            else {   
               sprintf(err_str, "can't get id for project \"%s\"", cp);
               shepherd_trace(err_str);
            }
         }
         else  
            shepherd_trace("can't get configuration entry for projects");
         
      }
#     elif defined(CRAY)
      {
      	 char *cp;
          char err_str[2*SGE_PATH_MAX+128];

	      {
	         int jobid;

	         if ((jobid=setjob(pw->pw_uid, 0))<0) {
	            sprintf(err_str, "error: can't set job ID; errno = %d", errno);
	            shepherd_error(err_str);
	         }

	         if (sesscntl(jobid, S_ADDFL, S_BATCH) == -1) {
	            sprintf(err_str, "error: sesscntl(%d, S_ADDFL, S_BATCH) failed,"
		         " errno = %d", sid, errno);
	            shepherd_error(err_str);
	         } 
	         sprintf(osjobid, "%d", jobid);
	      }

	      if ((cp = search_conf_val("acct_project"))) {
	         int proj; 
	         if (strcasecmp(cp, "none") && ((proj = nam2acid(cp)) >= 0)) {
	            sprintf(err_str, "setting project \"%s\" to acid %d", cp, proj);
	            shepherd_trace(err_str);
	            if (acctid(0, proj) == -1)
		            shepherd_trace("failed setting project id (acctid)");
	         } else {   
	            sprintf(err_str, "can't get id for project \"%s\"", cp);
	            shepherd_trace(err_str);
	         }
	      } else  
	         shepherd_trace("can't get configuration entry for projects");
      }
#     elif defined(NECSX4) || defined(NECSX5)
      {
         id_t jobid = 0;
			char err_str[2*SGE_PATH_MAX+128];
		 	dispset2_t attr;	
			int value;

         /*
          * Create new Super-UX job
          */
         if (setjid() == -1) {
            sprintf(err_str, "ERROR: can't set jobid: %s[%d]", 
               strerror(errno), errno);
            shepherd_trace(err_str);
         } else {
            jobid = getjid(0);
            sprintf(err_str, "Created job with id: "u32, (u_long32) jobid);
            shepherd_trace(err_str);
         }  
         sprintf(osjobid, u32, (u_long32) jobid); 

         /*
          * We will use limits for the whole job
          */
         set_rlimits_os_job_id(jobid);

         /*
          * The job will use the resources of the configured 
          * Resource Sharing Group (rsg)
          */ 
         {
            char *rsg_id_string;
            int rsg_id;
            char fsg_dev_string[256];

            rsg_id_string  = get_conf_val("processors");
            rsg_id = atoi(rsg_id_string);
            if (rsg_id) {
               int fd;

               sprintf(fsg_dev_string, "/dev/rsg/%d", rsg_id);
               fd = open(fsg_dev_string, O_RDONLY);
               if (fd <= 0) {
                  sprintf(err_str, "ERROR: can't switch to rsg%d because "
                     "can't open device: %s[%d]", rsg_id, strerror(errno), 
                     errno);
                  shepherd_trace(err_str);
               } else {
                  if (ioctl(fd, RSG_JUMP, NULL) == -1) {
                     close(fd);
                     sprintf(err_str, "ERROR: can't switch to rsg%d: %s[%d]", 
                        rsg_id, strerror(errno), errno);
                     shepherd_trace(err_str);
                     return;
                  } else {
                     close(fd);
                     sprintf(err_str, "switched to rsg%d", rsg_id);
                     shepherd_trace(err_str);
                  }
               }
            } else {
               sprintf(err_str, "using default rsg", rsg_id);
               shepherd_trace(err_str);
            }
         } 

         /*
          * Set scheduling parameter for job
          */
         if (((attr.basepri = atoi(get_conf_val("nec_basepriority"))) != NEC_UNDEF_VALUE)
            && ((attr.modcpu = atoi(get_conf_val("nec_modcpu"))) != NEC_UNDEF_VALUE)
            && ((attr.tickcnt = atoi(get_conf_val("nec_tickcnt"))) != NEC_UNDEF_VALUE)
            && ((attr.dcyfctr = atoi(get_conf_val("nec_dcyfctr"))) != NEC_UNDEF_VALUE)
            && ((attr.dcyintvl = atoi(get_conf_val("nec_dcyintvl"))) != NEC_UNDEF_VALUE)
            && ((attr.tmslice = atoi(get_conf_val("nec_timeslice"))) != NEC_UNDEF_VALUE)
            && ((attr.mempri = atoi(get_conf_val("nec_memorypriority"))) != NEC_UNDEF_VALUE)
            && ((attr.szefctmrt = atoi(get_conf_val("nec_mrt_size_effct"))) != NEC_UNDEF_VALUE)
            && ((attr.priefctmrt = atoi(get_conf_val("nec_mrt_pri_effct"))) != NEC_UNDEF_VALUE)
            && ((attr.minmrt = atoi(get_conf_val("nec_mrt_minimum"))) != NEC_UNDEF_VALUE)
            && ((attr.agrange = atoi(get_conf_val("nec_aging_range"))) != NEC_UNDEF_VALUE)
            && ((attr.spinherit = atoi(get_conf_val("nec_slavepriority"))) != NEC_UNDEF_VALUE)
            && ((attr.concpu = atoi(get_conf_val("nec_cpu_count"))) != NEC_UNDEF_VALUE)) {
            if (dispcntl(SG_JID, getjid(0), DCNTL_SET2, &attr) == -1) {
               sprintf(err_str, "ERROR: can't set scheduling parameter: %s[%d]",
                  strerror(errno), errno);
               shepherd_trace(err_str);
            } else {
               sprintf(err_str, "control parameters for active process scheduling modified");
               shepherd_trace(err_str);
               print_scheduling_parameters(attr);
            }
         } else {
            sprintf(err_str, "we do not control active process scheduling");
            shepherd_trace(err_str);
         }
      }               
#     else
         /* write a default os-jobid to file */
         sprintf(osjobid, pid_t_fmt, sid);
#     endif
         sge_switch2admin_user();
      } 
      else /* not running as super user --> we want a default os-jobid */
         sprintf(osjobid, "0");
      
      if(fprintf(fp, "%s\n", osjobid) < 0)
         shepherd_trace("error writing osjobid file");
         
      fclose(fp); /* Close os-jobid file */   
   }
#  endif
   return;
}
