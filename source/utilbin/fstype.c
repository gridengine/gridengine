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
#include <errno.h>
#include <string.h>

#if defined(DARWIN) || defined(FREEBSD)
#  include <sys/param.h>
#  include <sys/mount.h>
#elif defined(LINUX)
#  include <sys/vfs.h>
#else
#  include <sys/types.h>
#  include <sys/statvfs.h>
#endif

#if defined(SOLARIS)
#include <kstat.h>
#include <nfs/nfs.h> 
#include <nfs/nfs_clnt.h> 
#endif 

#if defined(INTERIX)
#  include "misc.h"
#endif

int main(int argc, char *argv[]) {

   int ret=1;

   if (argc < 2) {
      printf("Usage: fstype <directory>\n");
      return 1;
   }
   else
   {  
#if defined(LINUX) || defined(DARWIN) || defined(FREEBSD)
   struct statfs buf;
   ret = statfs(argv[1], &buf);
#elif defined(INTERIX)
   struct statvfs buf;
   ret = wl_statvfs(argv[1], &buf);
#elif defined(SOLARIS)
   struct statvfs buf; 
   struct mntinfo_kstat mnt_info;
   ulong_t fsid;
   kstat_ctl_t    *kc = NULL;
   kstat_t        *ksp;
   kstat_named_t  *knp;

   ret = statvfs(argv[1], &buf); 
   fsid = (minor_t)(buf.f_fsid & 0xfff);
 
   if ( strcmp(buf.f_basetype, "nfs") == 0) {
            kc = kstat_open();
            for (ksp = kc->kc_chain; ksp; ksp = ksp->ks_next) {
               if (ksp->ks_type != KSTAT_TYPE_RAW)
			         continue;
		         if (strcmp(ksp->ks_module, "nfs") != 0)
			         continue;
		         if (strcmp(ksp->ks_name, "mntinfo") != 0)
			         continue;
               if (kstat_read(kc, ksp, &mnt_info) == -1) {
                  kstat_close(kc);
                  printf("error\n");
                  return 2;
               }
               if ( fsid  == ksp->ks_instance)
                  if ( mnt_info.mik_vers >= 4 ) {
                     sprintf(buf.f_basetype, "%s%i", buf.f_basetype, mnt_info.mik_vers);
                  }
               }
            ret = kstat_close(kc);
   }
#else   
   struct statvfs buf;
   ret = statvfs(argv[1], &buf);
#endif

   if(ret!=0) {
      printf("Error: %s\n", strerror(errno));
      return 2;
   }
  
#if defined (DARWIN) || defined(FREEBSD)
   printf("%s\n", buf.f_fstypename);
#elif defined(LINUX)
   if (buf.f_type == 0x6969)
      printf("nfs\n");
   else   
      printf("%lx\n", (long unsigned int)buf.f_type);
#elif defined(INTERIX)
   printf("%s\n", buf.f_fstypename);
#else
   printf("%s\n", buf.f_basetype);
#endif
   }
   return 0;
}
