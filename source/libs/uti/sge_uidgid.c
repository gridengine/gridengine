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
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

#include "sgermon.h"
#include "sge_uidgid.h"
#include "sge_switch_user.h"
#include "sge_getpwnam.h"
#include "sge_sysconf.h"
#include "sge_unistd.h"
#include "sge_log.h"
#include "msg_utilib.h"

#ifdef QIDL
#include <pthread.h>
#endif

/****** uti/uidgid/sge_user2uid() *********************************************
*  NAME
*     sge_user2uid() -- resolves user name to uid  
*
*  SYNOPSIS
*     int sge_user2uid(const char *user, uid_t *uidp, int retries) 
*
*  FUNCTION
*     Resolves a username ('user') to its uid (stored in 'uidp').
*     'retries' defines the number of (e.g. NIS/DNS) retries.
*     If 'uidp' is NULL the user name is resolved without saving it.
*
*  INPUTS
*     const char *user - username 
*     uid_t *uidp      - uid pointer 
*     int retries      - number of retries 
*
*  RESULT
*     int - exit state 
*         0 - OK
*         1 - Error
******************************************************************************/
int sge_user2uid(const char *user, uid_t *uidp, int retries) 
{
   struct passwd *pw;

   DENTER(CULL_LAYER, "sge_user2uid");

   do {
      DPRINTF(("name: %s retries: %d\n", user, retries));

      if (!retries--) {
         DEXIT;
         return 1;
      }
      pw = getpwnam(user);
   } while (pw == NULL);

   if (uidp) {
      *uidp = pw->pw_uid;
   }

   DEXIT; 
   return 0;
}

/****** uti/uidgid/sge_group2gid() ********************************************
*  NAME
*     sge_group2gid() -- ??? 
*
*  SYNOPSIS
*     int sge_group2gid(const char *gname, gid_t *gidp, int retries) 
*
*  FUNCTION
*     Resolves a groupname ('gname') to its gid (stored in 'gidp').
*     'retries' defines the number of (e.g. NIS/DNS) retries.
*     If 'gidp' is NULL the group name is resolved without saving it.
*
*  INPUTS
*     const char *gname - group name 
*     gid_t *gidp       - gid pointer 
*     int retries       - number of retries  
*
*  RESULT
*     int - exit state 
*         0 - OK
*         1 - Error
******************************************************************************/
int sge_group2gid(const char *gname, gid_t *gidp, int retries) 
{
   struct group *gr;

   DENTER(CULL_LAYER, "sge_group2gid");

   do {
      if (!retries--) {
         DEXIT;
         return 1;
      }
      gr = getgrnam(gname);
   } while (gr == NULL);
   
   if (gidp) {
      *gidp = gr->gr_gid;
   }

   DEXIT; 
   return 0;
}

/****** uti/uidgid/sge_uid2user() *********************************************
*  NAME
*     sge_uid2user() -- Resolves uid to user name. 
*
*  SYNOPSIS
*     int sge_uid2user(uid_t uid, char *dst, size_t sz, int retries) 
*
*  FUNCTION
*     Resolves uid to user name. if 'dst' is NULL the function checks
*     only if the uid is resolvable. 
*
*  INPUTS
*     uid_t uid   - user id 
*     char *dst   - buffer for the username 
*     size_t sz   - buffersize 
*     int retries - number of retries 
*
*  RESULT
*     int - error state
*         0 - OK
*         1 - Error
******************************************************************************/
int sge_uid2user(uid_t uid, char *dst, size_t sz, int retries)
{
   struct passwd *pw;
   static uid_t last_uid;
   static char last_username[255] = ""; 
#ifdef QIDL
   static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
#endif

   DENTER(CULL_LAYER, "sge_uid2user");

#ifdef QIDL
   pthread_mutex_lock(&lock);
#endif

   if (!last_username[0] || last_uid != uid) {

      /* max retries that are made resolving user name */
      while (!(pw = getpwuid(uid))) {
         if (!retries--) {
            ERROR((SGE_EVENT, MSG_SYSTEM_GETPWUIDFAILED_US , 
                  uid, strerror(errno)));
            DEXIT;
#ifdef QIDL
            pthread_mutex_unlock(&lock);
#endif
            return 1;
         }
         sleep(1);
      }

      /* cache user name */
      strcpy(last_username, pw->pw_name);
      last_uid = uid;
   }
   if (dst)
      strncpy(dst, last_username, sz);

#ifdef QIDL
   pthread_mutex_unlock(&lock);
#endif
   DEXIT; 
   return 0;
}

/****** uti/uidgid/sge_gid2group() ********************************************
*  NAME
*     sge_gid2group() -- Resolves gid to user name. 
*
*  SYNOPSIS
*     int sge_gid2group(gid_t gid, char *dst, size_t sz, int retries) 
*
*  FUNCTION
*     Resolves gid to user name. if 'dst' is NULL the function checks
*     only if the gid is resolvable. 
*
*  INPUTS
*     uid_t gid   - group id 
*     char *dst   - buffer for the group name 
*     size_t sz   - buffersize 
*     int retries - number of retries 
*
*  RESULT
*     int - error state
*         0 - OK
*         1 - Error
******************************************************************************/
int sge_gid2group(gid_t gid, char *dst, size_t sz, int retries)
{
   struct group *gr;
   static gid_t last_gid;
   static char last_groupname[255] = ""; 
#ifdef QIDL
   static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
#endif

   DENTER(CULL_LAYER, "sge_gid2group");

#ifdef QIDL
   pthread_mutex_lock(&lock);
#endif

   if (!last_groupname[0] || last_gid != gid) {

      /* max retries that are made resolving group name */
      while (!(gr = getgrgid(gid))) {
         if (!retries--) {
            ERROR((SGE_EVENT, MSG_SYSTEM_GETGRGIDFAILED_US , 
                  gid, strerror(errno)));
#ifdef QIDL
            pthread_mutex_unlock(&lock);
#endif
            DEXIT;
            return 1;
         }
         sleep(1);
      }

      /* cache group name */
      strcpy(last_groupname, gr->gr_name);
      last_gid = gid;
   }
   if (dst)
      strncpy(dst, last_groupname, sz);

#ifdef QIDL
   pthread_mutex_unlock(&lock);
#endif
   DEXIT; 
   return 0;
}

/****** uti/uidgid/sge_set_uid_gid_addgrp() ***********************************
*  NAME
*     sge_set_uid_gid_addgrp() -- Set uid and gid of calling process
*
*  SYNOPSIS
*     int sge_set_uid_gid_addgrp(const char *user, 
*                                const char *intermediate_user,
*                                int min_gid, int min_uid, int add_grp,
*                                char *err_str, int use_qsub_gid, 
*                                gid_t qsub_gid)
*
*  FUNCTION
*     Set uid and gid of calling process. This can be done only by root.
*
*  INPUTS
*     const char *user              - ???
*     const char *intermediate_user - ???
*     int min_gid                   - ???
*     int min_uid                   - ???
*     int add_grp                   - ???
*     char *err_str                 - ???
*     int use_qsub_gid              - ???
*     gid_t qsub_gid                - ???
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - we can't switch to user since we are not root
*         1 - we can't switch to user or we can't set add_grp
******************************************************************************/
int sge_set_uid_gid_addgrp(const char *user, const char *intermediate_user,
                           int min_gid, int min_uid, int add_grp, char *err_str,
                           int use_qsub_gid, gid_t qsub_gid)
{
#ifndef WIN32 /* var not needed */
   int status;
#endif
   struct passwd *pw;
 
   switch2start_user();
 
   if (getuid() != 0) {
      sprintf(err_str, MSG_SYSTEM_CHANGEUIDORGIDFAILED );
      return -1;
   }
 
   if (intermediate_user) {
      user = intermediate_user;            
   }
 
   if (!(pw = sge_getpwnam(user))) {
      sprintf(err_str, MSG_SYSTEM_GETPWNAMFAILED_S , user);
      return 1;
   }
 
   /*
    *  Should we use the primary group of qsub host? (qsub_gid)
    */
   if (use_qsub_gid) {
      pw->pw_gid = qsub_gid;
   }
 
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
      if (sge_add_group(add_grp) == -1) {
         sprintf(err_str, MSG_SYSTEM_ADDGROUPIDFORSGEFAILED );
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
   } else {
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

/****** uti/uidgid/sge_add_group() ********************************************
*  NAME
*     sge_add_group() -- Add a gid to the list of additional group ids
*
*  SYNOPSIS
*     int sge_add_group(gid_t add_grp_id)
*
*  FUNCTION
*     Add a gid to the list of additional group ids. If 'add_grp_id' is 0
*     don't add value to group id list (but return sucessfully).
*
*  INPUTS
*     gid_t add_grp_id - new gid
*
*  RESULT
*     int - error state
*         0 - Success
*        -1 - Error
******************************************************************************/
int sge_add_group(gid_t add_grp_id)
{
   u_long32 max_groups;
   gid_t *list;
   int groups;
 
   if (add_grp_id == 0) {
      return 0;
   }
 
   max_groups = sge_sysconf(sge_sysconf_NGROUPS_MAX);
   if (max_groups <= 0) {
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
      return -1;
   }
 
   groups = getgroups(max_groups, list);
 
   if (groups < max_groups) {
      list[groups] = add_grp_id;
      groups++;
      groups = setgroups(groups, list);
      if (groups == -1) {
         free(list);
         return -1;
      }
   } else {
      free(list);
      return -1;
   }                      

   free(list);
   return 0;
}  
  
