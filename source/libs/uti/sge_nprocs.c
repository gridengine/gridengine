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
#include "msg_utilib.h"

#if defined(NECSX4) || defined(NECSX5)
#   include <sys/rsg.h>
#   include <sys/types.h>
#   include <fcntl.h>
#   include <sys/syssx.h>
#endif

#if defined(DARWIN)
#   include <mach/host_info.h>
#   include <mach/mach_host.h>
#   include <mach/mach_init.h>
#   include <mach/machine.h>
#endif

#if defined(__sgi)
#   include <sys/types.h>
#   include <sys/sysmp.h>
#endif

#if defined(ALPHA)
#   include <sys/sysinfo.h>
#   include <machine/hal_sysinfo.h>
#endif

#if defined(SOLARIS) || defined(AIX) || defined(LINUX)
#   include <unistd.h>
#endif

#if defined(__hpux)
    /* needs to be copiled with std C compiler ==> no -Aa switch no gcc */
#   include <stdlib.h>
#   include <sys/pstat.h>
#   include <unistd.h>
#endif

#if defined(CRAY)
#   include <unistd.h>
#endif

#if defined(FREEBSD)
#   include <sys/types.h>
#   include <sys/sysctl.h>
#endif

#if defined(NETBSD)
#   include <sys/param.h>
#   include <sys/sysctl.h>
#endif

#ifdef NPROCS_TEST
#   include <stdio.h>
#   include <unistd.h>
void main(int, char**);
#endif

#ifndef NPROCS_TEST
#include "sge_os.h"
#else 
int sge_nprocs (void);
#endif

#if defined(NECSX4) || defined(NECSX5)
int sge_nprocs_rsg(rsg_id) {
   int nprocs;
   int fd;
   rsg_info_t info;
   char fsg_dev_string[256];

   nprocs = 0;
   sprintf(fsg_dev_string, "/dev/rsg/%d", rsg_id);
   fd = open(fsg_dev_string, O_RDONLY);
   if (fd > 0) {
      if (ioctl(fd, RSG_INFO, &info) == -1) {
         close(fd);
         return 0;
      }
      close(fd);

      nprocs += info.cprb.init_cpu;
   }
   return nprocs;
}
#endif

/****** uti/os/sge_nprocs() ***************************************************
*  NAME
*     sge_nprocs() -- Number of processors in this machine 
*
*  SYNOPSIS
*     int sge_nprocs() 
*
*  FUNCTION
*     Use this function to get the number of processors in 
*     this machine 
*
*  RESULT
*     int - number of procs
* 
*  NOTES
*     MT-NOTE: sge_nprocs() is MT safe (SOLARIS, NEC, IRIX, ALPHA, HPUX, LINUX)
******************************************************************************/
int sge_nprocs()
{
   int nprocs=1; /* default */
#if defined(NECSX4) || defined(NECSX5)
   int fd;
   int fsg_id;
   rsg_info_t info;
   char fsg_dev_string[256];
#endif
 
/* NEC SX 4/16, NEC SX 4/32 */
#if defined(NECSX4) || defined(NECSX5)
/*
 * Using RSG values alone is unreliable.
 */
#if 0
   nprocs = 0;
   for (fsg_id=0; fsg_id<32; fsg_id++) {
      sprintf(fsg_dev_string, "/dev/rsg/%d", fsg_id);
      fd = open(fsg_dev_string, O_RDONLY);
      if (fd > 0) {
         if (ioctl(fd, RSG_INFO, &info) == -1) {
            close(fd);
            continue;
         }
         close(fd);

         nprocs += info.cprb.init_cpu;
      }
   }
   if (nprocs == 0) {
      nprocs=1;
   }
#elif 1
#if defined(CNFGAPNUM)
   /*
    * SUPER-UX >= 11.x provides a function.
    */
   nprocs = syssx(CNFGAPNUM);
#else
   {
      /*
       * As with sge_loadmem(), get RB info and tally
       * it up.
       */
      char       fsg_dev_string[256];
      int        fd, fsg_id;
      rsg_id_t   id;
      rsg_info_t info;
      cpurb_t    cpurbs[MAXRBNUM];
      int        i;

      /* initialize */
      for (i = 0; i < MAXRBNUM; i++) {
         memset(&cpurbs[i], 0, sizeof(cpurb_t));
      }

      /* read in RB info (don't be fooled by RSG names) */
      for (fsg_id = 0; fsg_id < MAXRSGNUM; fsg_id++) {
         sprintf(fsg_dev_string, "/dev/rsg/%d", fsg_id);
         fd = open(fsg_dev_string, O_RDONLY);
         if (fd >= 0) {
            if ((ioctl(fd, RSG_ID, &id) == -1) ||
                  (ioctl(fd, RSG_INFO, &info) == -1)) {
               close(fd);
               continue;
            }
            close(fd);

            /* copy for later use */
            memcpy(&cpurbs[id.cprbid], &info.cprb, sizeof(cpurb_t));
         }
      }

      nprocs = 0;
      for (i = 0; i < MAXRBNUM; i++) {
         nprocs += cpurbs[i].init_cpu;
      }
   }
#endif /* CNFGAPNUM */
#endif

#endif

#if defined(DARWIN)
  struct host_basic_info cpu_load_data;

  mach_msg_type_number_t host_count = sizeof(cpu_load_data)/sizeof(integer_t);
  mach_port_t host_priv_port = mach_host_self();

  host_info(host_priv_port, HOST_BASIC_INFO , (host_info_t)&cpu_load_data, &host_count);

  nprocs =  cpu_load_data.avail_cpus;

#endif


#ifdef __sgi
   nprocs = sysmp(MP_NPROCS);
#endif

#if defined(ALPHA)
   int start=0;

   getsysinfo(GSI_CPUS_IN_BOX,(char*)&nprocs,sizeof(nprocs),&start);
#endif

#if defined(SOLARIS) || defined(AIX) || defined(LINUX)
   nprocs = sysconf(_SC_NPROCESSORS_ONLN);
#endif

#if defined(__hpux)
   union pstun pstatbuf;
   struct pst_dynamic dinfo;

   pstatbuf.pst_dynamic = &dinfo;
   if (pstat(PSTAT_DYNAMIC,pstatbuf,sizeof(dinfo),NULL,NULL)==-1) {
          perror(MSG_PERROR_PSTATDYNAMIC);
          exit(1);
   }
   nprocs = dinfo.psd_proc_cnt;
#endif

#ifdef CRAY
   nprocs = sysconf(_SC_CRAY_NCPU);
#endif

#if defined(FREEBSD)
   size_t nprocs_len = sizeof(nprocs);

   if (sysctlbyname("hw.ncpu", &nprocs, &nprocs_len, NULL, 0) == -1) {
      nprocs = -1;
   }
#endif

#if defined(NETBSD)
   int mib[2];
   size_t nprocs_len;

   nprocs_len = sizeof(nprocs);
   mib[0]     = CTL_HW;
   mib[1]     = HW_NCPU;

   if (sysctl(mib, sizeof(mib)/sizeof(int), &nprocs, &nprocs_len, NULL, 0) == -1) {
     nprocs = -1;
   }
#endif

#if defined(INTERIX)
/* TODO: HP: don't set nprocs==-1 to 0, overwrite it with value from
 *       external load sensor.
 */
   nprocs = -1;
#endif

   if (nprocs <= 0) {
      nprocs = 1;
   }

   return nprocs;
}

#ifdef NPROCS_TEST
void main(
int argc,
char **argv 
) {
   printf(MSG_INFO_NUMBOFPROCESSORS_I, sge_nprocs());
   printf("\n");
   exit(0);
}
#endif
