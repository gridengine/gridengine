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
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#if defined(NECSX4) || defined(NECSX5)
#   include <limits.h>
#endif

#include "sge_set_uid_gid.h"
#include "basis_types.h"
#include "sge_getpwnam.h"
#include "sge_switch_user.h"
#include "sge_sysconf.h"
#include "msg_utilib.h"
#include "config_file.h"
#include "msg_execd.h"

/*---------------------------------------------------------------------
 * Set userid and gid of caller to the uid/gid of a specific user.
 * This can be done only by root.
 * Returns: 0  OK
 *         -1  we can't switch to user since we are not root
 *         1   we can't switch to user or we can't set add_grp
 *---------------------------------------------------------------------*/

int setuidgidaddgrp(
const char *user,
const char *intermediate_user,
int min_gid,
int min_uid,
int add_grp,
char *err_str,
int use_qsub_gid,
gid_t qsub_gid 
) {

#ifndef WIN32 /* var not needed */
   int status;
#endif
   struct passwd *pw;

   switch2start_user();

   if (getuid() != 0) {
      sprintf(err_str, MSG_SYSTEM_CHANGEUIDORGIDFAILED );
      return -1;
   }

   if (intermediate_user)
      user = intermediate_user;

   if (!(pw = sge_getpwnam(user))) {
      sprintf(err_str, MSG_SYSTEM_GETPWNAMFAILED_S , user);
      return 1;
   }

   /*
    *  Should we use the primary group of qsub host? (qsub_gid)
    */
   if (use_qsub_gid)
      pw->pw_gid = qsub_gid;

   if ( !intermediate_user) {
      /* 
       *  It should not be necessary to set min_gid/min_uid to 0
       *  for being able to run prolog/epilog/pe_start/pe_stop 
       *  as root 
       */
      if (pw->pw_gid < min_gid) {
         sprintf(err_str, MSG_SYSTEM_GIDLESSTHANMINIMUM_SUI ,
                 user, (gid_t) pw->pw_gid, min_gid);
         return 1;
      }
      if (setgid(pw->pw_gid)) {
         sprintf(err_str,MSG_SYSTEM_SETGIDFAILED_U , (gid_t) pw->pw_gid);
         return 1;
      }
   } else {
      if (setegid(pw->pw_gid)) {
         sprintf(err_str, MSG_SYSTEM_SETEGIDFAILED_U , (gid_t) pw->pw_gid);
         return 1;
      }
   }

#ifndef WIN32 /* initgroups not called */
   status = initgroups(pw->pw_name, pw->pw_gid);

   /* Why am I doing it this way?  Good question,
      an even better question would be why vendors
      can't get their act together on what is returned,
      at least get it right in the man pages!
      on error heres what I get:
      (subject to change with OS releases)
      OS      return       errno
      AIX     1            1
      ULTRIX  1            1
      OSF/1   1            1
      IRIX   -1            1   (returns #groups if successful)
      SUNOS  -1            1
      SOLARIS-1
      UGH!!!
    */

#if defined(SVR3) || defined(sun)
   if (status < 0) {
      sprintf(err_str, MSG_SYSTEM_INITGROUPSFAILED_I , status);
      return 1;
   }
#else
   if (status) {
      sprintf(err_str, MSG_SYSTEM_INITGROUPSFAILED_I , status);
      return 1;
   }
#endif
#endif /* WIN32 */

#if defined(SOLARIS) || defined(ALPHA) || defined(LINUX)     
   /* add Additional group id to current list of groups */
   if (add_grp) {
      if (add_group(add_grp, err_str) == -1) {
         return 1;
      }
   }
#endif
   
   if (!intermediate_user) {
      if (pw->pw_uid < min_uid) {
         sprintf(err_str, MSG_SYSTEM_UIDLESSTHANMINIMUM_SUI ,
                 user, pw->pw_uid, min_uid);
         return 1;
      }

      if (use_qsub_gid) {
         if (setgid(pw->pw_gid)) {
            sprintf(err_str, MSG_SYSTEM_SETGIDFAILED_U, pw->pw_uid);
            return 1; 
         }
      }

      if (setuid(pw->pw_uid)) {
         sprintf(err_str, MSG_SYSTEM_SETUIDFAILED_U , pw->pw_uid);
         return 1;
      }
   }
   else {
      if (use_qsub_gid) {
         if (setgid(pw->pw_gid)) {
            sprintf(err_str, MSG_SYSTEM_SETGIDFAILED_U , pw->pw_uid);
            return 1;
         }
      }

      if (seteuid(pw->pw_uid)) {
         sprintf(err_str, MSG_SYSTEM_SETEUIDFAILED_U , pw->pw_uid);
         return 1;
      }
   }

   return 0;
}

/*-------------------------------------------------------------------------
 * add_group
 * add newgid to list of additional group ids
 * if newgid = 0 don't add value to group id list (but return sucessfully)
 * return 0 in case if success
 *       -1 in case of failure
 * if err_str points to a string buffer, error messages are passed back here
 *-------------------------------------------------------------------------*/
int add_group( gid_t add_grp_id, char *err_str) {
   u_long32 max_groups;
   gid_t *list;
   int groups;

   if(err_str != NULL) {
      err_str[0] = 0;
   }

   if (add_grp_id == 0)
      return 0;

   max_groups = sge_sysconf(sge_sysconf_NGROUPS_MAX);
   if (max_groups <= 0) {
      if(err_str != NULL) {
         sprintf(err_str, MSG_SYSTEM_ADDGROUPIDFORSGEFAILED_UUS, getuid(), geteuid(), MSG_SYSTEM_INVALID_NGROUPS_MAX);
      }
      return -1;
   }   

/* 
 * INSURE detects a WRITE_OVERFLOW when getgroups was invoked (LINUX).
 * Is this a bug in the kernel or in INSURE?
 */
#if defined(LINUX)
   list = (gid_t*) malloc(2*max_groups*sizeof(gid_t));
#else
   list = (gid_t*) malloc(max_groups*sizeof(gid_t));
#endif
   if (list == NULL) {
      if(err_str != NULL) {
         int error = errno;
         sprintf(err_str, MSG_SYSTEM_ADDGROUPIDFORSGEFAILED_UUS, getuid(), geteuid(), strerror(error));
      }
      return -1;
   }   
  
   groups = getgroups(max_groups, list);
   if (groups == -1) {
      if(err_str != NULL) {
         int error = errno;
         sprintf(err_str, MSG_SYSTEM_ADDGROUPIDFORSGEFAILED_UUS, getuid(), geteuid(), strerror(error));
      }
      free(list);
      return -1;
   }   

   if (groups < max_groups) {
      list[groups] = add_grp_id;
      groups++;
      groups = setgroups(groups, list);
      if (groups == -1) {
         if(err_str != NULL) {
            int error = errno;
            sprintf(err_str, MSG_SYSTEM_ADDGROUPIDFORSGEFAILED_UUS, getuid(), geteuid(), strerror(error));
         }
         free(list);
         return -1;
      }
   }
   else {
      if(err_str != NULL) {
         sprintf(err_str, MSG_SYSTEM_ADDGROUPIDFORSGEFAILED_UUS, getuid(), geteuid(), MSG_SYSTEM_USER_HAS_TOO_MANY_GIDS);
      }
      free(list);
      return -1;
   }
      
   free(list);
   return 0;
}
