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
#endif

#if defined(DARWIN)
#   include <mach/host_info.h>
#   include <mach/mach_host.h>
#   include <mach/mach_init.h> 
#   include <mach/machine.h>
#endif         

/* IRIX 5, 6 */
#if defined(__sgi)
#   include <sys/types.h>
#   include <sys/sysmp.h>
#endif

#if defined(ALPHA)
#   include <sys/sysinfo.h>
#   include <machine/hal_sysinfo.h>
#endif

#if defined(SOLARIS) || defined(AIX)
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

#if defined(LINUX) 
#   include <stdio.h>
#   include <errno.h>
#   include <string.h>
#   define LINUX_PROCFILE "/proc/cpuinfo"
#   define LINUX_KEYWORD "processor"
#endif

#if defined(ALINUX)
#  include <stdlib.h>
#  define ALINUX_PROCFILE "/proc/cpuinfo"
#  define ALINUX_KEYWORD "cpus detected"   
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
#include "sge_nprocs.h"
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

/* use this function to get the number of processors in this machine */
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
#endif

#if defined(DARWIN)
  struct host_basic_info cpu_load_data;

  int host_count = sizeof(cpu_load_data)/sizeof(integer_t);
  mach_port_t host_priv_port = mach_host_self();

  host_info(host_priv_port, HOST_BASIC_INFO , (host_info_t)&cpu_load_data, &host_count);

  nprocs =  cpu_load_data.avail_cpus;

#endif


/* IRIX 5, 6 */
#ifdef __sgi
   nprocs = sysmp(MP_NPROCS);
#endif

#if defined(ALPHA)
   int start=0;

   getsysinfo(GSI_CPUS_IN_BOX,(char*)&nprocs,sizeof(nprocs),&start);
#endif

#if defined(SOLARIS) || defined(AIX)
   nprocs = sysconf(_SC_NPROCESSORS_ONLN);
#endif

#if defined(__hpux) && !defined(HPCONVEX)
   union pstun pstatbuf;
   struct pst_dynamic dinfo;

   pstatbuf.pst_dynamic = &dinfo;
   if (pstat(PSTAT_DYNAMIC,pstatbuf,sizeof(dinfo),NULL,NULL)==-1) {
          perror(MSG_PERROR_PSTATDYNAMIC);
          exit(1);
   }
   nprocs = dinfo.psd_proc_cnt;
#endif

#if defined(HPCONVEX)
   nprocs = 1;
#endif

#ifdef CRAY
   nprocs = sysconf(_SC_CRAY_NCPU);
#endif


#if defined(LINUX) 
   int success = 0;
   char buffer[1024];
   int keylen;
   FILE *fp;

#  if defined(ALINUX)
   /* cat /proc/cpuinfo | grep "cpus detected" | cut -f 2 -d ":" */  
   char *token;
 
   if ((fp = fopen(ALINUX_PROCFILE, "r"))) {
      keylen = strlen(ALINUX_KEYWORD);
      while (fgets(buffer, sizeof(buffer)-1, fp)) {
         token = strtok(buffer, ":");
         if (!strncmp(buffer, ALINUX_KEYWORD, keylen)) {
            token = strtok(NULL, " \t\n");
            nprocs = atoi(token);
            success = 1;
            break;
         }
      }
      fclose(fp);
   }                         
#  endif

   if (!success) {
      /* cat /proc/cpuinfo|grep processor|wc -l */

      if ((fp = fopen(LINUX_PROCFILE, "r"))) {
         nprocs = 0;
         keylen = strlen(LINUX_KEYWORD);
         while (fgets(buffer, sizeof(buffer)-1, fp)) {
            if (!strncmp(buffer, LINUX_KEYWORD, keylen)) {
               nprocs++;
               success = 1;
            }
         }
         fclose(fp);
      }
   }

   if (!success) {
      nprocs = 1; 
   }
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
   exit(0);
}
#endif
