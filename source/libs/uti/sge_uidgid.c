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

#include "sge_uidgid.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <pthread.h>

#include "basis_types.h"
#include "sgermon.h"
#include "sge_unistd.h"
#include "sge_log.h"
#include "sge_mtutil.h"
#include "msg_common.h"
#include "msg_utilib.h"


#define UIDGID_LAYER CULL_LAYER 

enum { SGE_MAX_USERGROUP_BUF = 255 };

typedef struct {
   pthread_mutex_t mutex;  /* TODO: use RW-lock instead */
   uid_t uid;
   gid_t gid;
   bool  initialized;
} admin_user_t;

struct uidgid_state_t {
   uid_t last_uid;
   char  last_username[SGE_MAX_USERGROUP_BUF]; 
   gid_t last_gid;
   char  last_groupname[SGE_MAX_USERGROUP_BUF]; 
};

static admin_user_t admin_user = {PTHREAD_MUTEX_INITIALIZER, -1, -1, false};

static pthread_once_t uidgid_once = PTHREAD_ONCE_INIT;
static pthread_key_t  uidgid_state_key;

static void uidgid_once_init(void);
static void uidgid_state_destroy(void* theState);
static void uidgid_state_init(struct uidgid_state_t* theState);

static void set_admin_user(uid_t, gid_t);
static int  get_admin_user(uid_t*, gid_t*);

static int get_group_buffer_size(void);

static uid_t       uidgid_state_get_last_uid(void);
static const char* uidgid_state_get_last_username(void);
static gid_t       uidgid_state_get_last_gid(void);
static const char* uidgid_state_get_last_groupname(void);

static void uidgid_state_set_last_uid(uid_t uid);
static void uidgid_state_set_last_username(const char *user);
static void uidgid_state_set_last_gid(gid_t gid);
static void uidgid_state_set_last_groupname(const char *group);


/****** uti/uidgid/uidgid_mt_init() ************************************************
*  NAME
*     uidgid_mt_init() -- Initialize user and group oriented functions for multi
*                         threading use.
*
*  SYNOPSIS
*     void uidgid_mt_init(void) 
*
*  FUNCTION
*     Set up user and group oriented functions. This function must be called at
*     least once before any of the user and group functions can be used. This
*     function is idempotent, i.e. it is safe to call it multiple times.
*
*     Thread local storage for the user and group state information is reserved. 
*
*  INPUTS
*     void - NONE 
*
*  RESULT
*     void - NONE
*
*  NOTES
*     MT-NOTE: uidgid_mt_init() is MT safe 
*
*******************************************************************************/
void uidgid_mt_init(void)
{
   pthread_once(&uidgid_once, uidgid_once_init);
}

/****** uti/uidgid/sge_is_start_user_superuser() ********************************
*  NAME
*     sge_is_real_user_superuser() -- Check the SGE/EE real user
*
*  SYNOPSIS
*     int sge_is_real_user_superuser(void)
*
*  FUNCTION
*     Check the real user id to determine if it is the superuser. If so, return
*     1, else return 0. This function relies on getuid == 0 for UNIX.  On
*     INTERIX, this function determines if the user is the built-in local admin 
*     user or if the user is in the +Administrators group.
*
*  INPUTS
*     NONE
*
*  RESULT
*     int - 0 or 1 
*         0 - Not running as superuser
*         1 - Running as superuser
*
*  NOTES
*     MT-NOTE: sge_is_start_user_superuser() is MT safe.
* 
*  SEE ALSO
*     uti/uidgid/sge_switch2admin_user()
*     uti/uidgid/sge_set_admin_username()
*     uti/uidgid/sge_switch2start_user()
*     uti/uidgid/sge_run_as_user()
******************************************************************************/
int sge_is_start_user_superuser(void)
{
   int ret = 0; 
   DENTER(UIDGID_LAYER, "sge_is_real_user_superuser");

#if defined(INTERIX) || defined(WIN32)
   {
      char  user_name[128];
      uid_t uid = getuid();

      if(!sge_uid2user(uid, user_name, sizeof(user_name)-1, MAX_NIS_RETRIES)) {
         ret = wl_is_start_user_superuser(user_name); 
      }
   }
#else
   if (getuid() == 0)
     ret = 1;
   else
     ret = 0; 
#endif

   DEXIT;
   return ret;
} /* sge_is_start_user_superuser() */           

/****** uti/uidgid/sge_set_admin_username() ***********************************
*  NAME
*     sge_set_admin_username() -- Set SGE/EE admin user
*
*  SYNOPSIS
*     int sge_set_admin_username(const char *user, char *err_str)
*
*  FUNCTION
*     Set SGE/EE admin user. If 'user' is "none" then use the current
*     uid/gid. Ignore if current user is not root.
*
*  INPUTS
*     const char *user - admin user name
*     char *err_str    - error message
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Username does not exist
*        -2 - Admin user was already set
*
*  NOTES
*     MT-NOTE: sge_set_admin_username() is MT safe.
* 
*  SEE ALSO
*     uti/uidgid/sge_switch2admin_user()
*     uti/uidgid/sge_set_admin_username()
*     uti/uidgid/sge_switch2start_user()
*     uti/uidgid/sge_run_as_user()
******************************************************************************/
int sge_set_admin_username(const char *user, char *err_str)
{
   struct passwd *admin;
   int ret;
   uid_t uid;
   gid_t gid;

   DENTER(UIDGID_LAYER, "sge_set_admin_username");
 
   if (get_admin_user(&uid, &gid) != ESRCH) {
      DEXIT;
      return -2;
   }
   if (!user || user[0] == '\0') {
      if (err_str) {
         sprintf(err_str, MSG_POINTER_SETADMINUSERNAMEFAILED);
      }
      DEXIT;
      return -1;
   }
 
   ret = 0;
   if (!strcasecmp(user, "none")) {
      set_admin_user(getuid(), getgid());
   } else {
      admin = sge_getpwnam(user);
      if (admin) {
         set_admin_user(admin->pw_uid, admin->pw_gid);
      } else {
         if (err_str)
            sprintf(err_str, MSG_SYSTEM_ADMINUSERNOTEXIST_S, user);
         ret = -1;
      }
   }
   DEXIT;
   return ret;
} /* sge_set_admin_username() */           

/****** uti/uidgid/sge_switch2admin_user() ************************************
*  NAME
*     sge_switch2admin_user() -- Set euid/egid to admin uid/gid
*
*  SYNOPSIS
*     int sge_switch2admin_user(void)
*
*  FUNCTION
*     Set euid/egid to admin uid/gid. Silently ignore if our uid
*     is not root. Do nothing if out euid/egid is already the admin
*     uid/gid. If the admin user was not set with
*     sge_set_admin_username() the function will not return.
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - setegid()/seteuid() fails
*
*  NOTES
*     MT-NOTE: sge_switch2admin_user() is MT safe.
*
*  SEE ALSO
*     uti/uidgid/sge_switch2admin_user()
*     uti/uidgid/sge_set_admin_username()
*     uti/uidgid/sge_switch2start_user()
*     uti/uidgid/sge_run_as_user()
******************************************************************************/
int sge_switch2admin_user(void)
{
   uid_t uid;
   gid_t gid;
   int ret = 0;

   DENTER(UIDGID_LAYER, "sge_switch2admin_user");
 
   if (get_admin_user(&uid, &gid) == ESRCH) {
      CRITICAL((SGE_EVENT, MSG_SWITCH_USER_NOT_INITIALIZED));
      abort();
   }
 
   if (!sge_is_start_user_superuser()) {
      DPRINTF((MSG_SWITCH_USER_NOT_ROOT));
      ret = 0;
      goto exit;
   } else {
      if (getegid() != gid) {
         if (setegid(gid) == -1) {
            DTRACE;
            ret = -1;
            goto exit;
         } 
      }
 
      if (geteuid() != uid) {
         if (seteuid(uid) == -1) {
            DTRACE;
            ret = -1;
            goto exit;
         } 
      }
   }

exit:
   DPRINTF(("uid=%ld; gid=%ld; euid=%ld; egid=%ld auid=%ld; agid=%ld\n", 
            (long)getuid(), (long)getgid(), 
            (long)geteuid(), (long)getegid(),
            (long)uid, (long)gid));
   DEXIT;
   return ret;
} /* sge_switch_2admin_user() */

/****** uti/uidgid/sge_switch2start_user() ************************************
*  NAME
*     sge_switch2start_user() -- set euid/egid to start uid/gid
*
*  SYNOPSIS
*     int sge_switch2start_user(void)
*
*  FUNCTION
*     Set euid/egid to the uid/gid of that user which started the
*     application which calles this function. If our euid/egid is
*     already the start uid/gid don't do anything. If the admin user
*     was not set with sge_set_admin_username() the function will
*     not return.
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - setegid()/seteuid() fails
*
*  NOTES
*     MT-NOTE: sge_switch2start_user() is MT safe.
*
*  SEE ALSO
*     uti/uidgid/sge_switch2admin_user()
*     uti/uidgid/sge_set_admin_username()
*     uti/uidgid/sge_switch2start_user()
*     uti/uidgid/sge_run_as_user()
******************************************************************************/
int sge_switch2start_user(void)
{
   uid_t uid, start_uid;
   gid_t gid, start_gid;
   int ret = 0;

   DENTER(UIDGID_LAYER, "sge_switch2start_user");
 
   if (get_admin_user(&uid, &gid) == ESRCH) {
      CRITICAL((SGE_EVENT, MSG_SWITCH_USER_NOT_INITIALIZED));
      abort();
   }
 
   start_uid = getuid();
   start_gid = getgid();

   if (start_uid) {
      DPRINTF((MSG_SWITCH_USER_NOT_ROOT));
      ret = 0;
      goto exit;
   } else {
      if (start_gid != getegid()) {
         if (setegid(start_gid) == -1) {
            DTRACE;
            ret = -1;
            goto exit;
         } 
      }
      if (start_uid != geteuid()) {
         if (seteuid(start_uid) == -1) {
            DTRACE;
            ret = -1;
            goto exit;
         } 
      }
   }

exit:
   DPRINTF(("uid=%ld; gid=%ld; euid=%ld; egid=%ld auid=%ld; agid=%ld\n", 
            (long)getuid(), (long)getgid(), 
            (long)geteuid(), (long)getegid(),
            (long)uid, (long)gid));
   DEXIT;
   return ret;
} /* sge_switch2start_user() */ 

/****** uti/uidgid/sge_run_as_user() ******************************************
*  NAME
*     sge_run_as_user() -- Set euid to uid
*
*  SYNOPSIS
*     int sge_run_as_user(void)
*
*  FUNCTION
*     Set euid to uid
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - setegid()/seteuid() failed
*
*  NOTES
*     MT-NOTE: sge_run_as_user() is MT safe
*
*  SEE ALSO
*     uti/uidgid/sge_switch2admin_user()
*     uti/uidgid/sge_set_admin_username()
*     uti/uidgid/sge_switch2start_user()
*     uti/uidgid/sge_run_as_user()
******************************************************************************/
int sge_run_as_user(void)
{
   int ret = 0;
 
   DENTER(UIDGID_LAYER, "sge_run_as_user");
 
   if (geteuid() != getuid()) {
      if (seteuid(getuid())) {
         ret = -1;
      }
   }
 
   DEXIT;
   return ret;
} /* sge_run_as_user() */

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
*  NOTES
*     MT-NOTE: sge_user2uid() is MT safe.
*
*  RESULT
*     int - exit state 
*         0 - OK
*         1 - Error
******************************************************************************/
int sge_user2uid(const char *user, uid_t *uidp, int retries) 
{
   struct passwd *pw;
   struct passwd pwentry;
   char buffer[2048];

   DENTER(UIDGID_LAYER, "sge_user2uid");

   do {
      DPRINTF(("name: %s retries: %d\n", user, retries));

      if (!retries--) {
         DEXIT;
         return 1;
      }
      if (getpwnam_r(user, &pwentry, buffer, sizeof(buffer), &pw) != 0) {
         pw = NULL;
      }
   } while (pw == NULL);

   if (uidp) {
      *uidp = pw->pw_uid;
   }

   DEXIT; 
   return 0;
} /* sge_user2uid() */

/****** uti/uidgid/sge_group2gid() ********************************************
*  NAME
*     sge_group2gid() -- Resolve a group name to its gid 
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
*  NOTES
*     MT-NOTE: sge_group2gid() is MT safe.
*
*  RESULT
*     int - exit state 
*         0 - OK
*         1 - Error
******************************************************************************/
int sge_group2gid(const char *gname, gid_t *gidp, int retries) 
{
   struct group *gr;
   struct group grentry;
   char buffer[2048];

   DENTER(UIDGID_LAYER, "sge_group2gid");

   do {
      if (!retries--) {
         DEXIT;
         return 1;
      }
      if (getgrnam_r(gname, &grentry, buffer, sizeof(buffer), &gr) != 0) {
         gr = NULL;
      }
   } while (gr == NULL);
   
   if (gidp) {
      *gidp = gr->gr_gid;
   }

   DEXIT; 
   return 0;
} /* sge_group2gid() */

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
*  NOTES
*     MT-NOTE: sge_uid2user() is MT safe.
*
*  RESULT
*     int - error state
*         0 - OK
*         1 - Error
******************************************************************************/
int sge_uid2user(uid_t uid, char *dst, size_t sz, int retries)
{
   struct passwd *pw;
   struct passwd pwentry;
   char buffer[2048];
   const char *last_username;

   DENTER(UIDGID_LAYER, "sge_uid2user");

   last_username = uidgid_state_get_last_username();

   if (!last_username[0] || (uidgid_state_get_last_uid() != uid))
   {
      /* max retries that are made resolving user name */
      while (getpwuid_r(uid, &pwentry, buffer, sizeof(buffer), &pw) != 0)
      {
         if (!retries--) {
            ERROR((SGE_EVENT, MSG_SYSTEM_GETPWUIDFAILED_US, 
                  u32c(uid), strerror(errno)));
            DEXIT;
            return 1;
         }
         sleep(1);
      }
      /* cache user name */
      uidgid_state_set_last_username(pw->pw_name);
      uidgid_state_set_last_uid(uid);
   }
   if (dst) {
      strncpy(dst, uidgid_state_get_last_username(), sz);
   }

   DEXIT; 
   return 0;
} /* sge_uid2user() */

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
*  NOTES
*     MT-NOTE: sge_gid2group() is MT safe.
*
*  RESULT
*     int - error state
*         0 - OK
*         1 - Error
******************************************************************************/
int sge_gid2group(gid_t gid, char *dst, size_t sz, int retries)
{
   struct group *gr;
   struct group grentry;
   const char *last_groupname;

   DENTER(UIDGID_LAYER, "sge_gid2group");

   last_groupname = uidgid_state_get_last_groupname();

   if (!last_groupname[0] || uidgid_state_get_last_gid() != gid) {
      char *buf = NULL;
      int size = 0;
      
      size = get_group_buffer_size();
      buf = sge_malloc(size);
      
     /* max retries that are made resolving group name */
#if defined (INTERIX)
      while (getgrgid_nomembers_r(gid, &grentry, buf, size, &gr) != 0)
#else
      while (getgrgid_r(gid, &grentry, buf, size, &gr) != 0)
#endif
      {
         if (!retries--) {
            sge_free(buf);
            
            DEXIT;
            return 1;
         }
         
         sleep(1);
      }
      
      sge_free(buf);

      /* Bugfix: Issuezilla 1256
       * We need to handle the case when the OS is unable to resolve the GID to
       * a name. [DT] */
      if (gr == NULL) {
         DEXIT;
         return 1;
      }
      
      /* cache group name */
      uidgid_state_set_last_groupname(gr->gr_name);
      uidgid_state_set_last_gid(gid);
   }
   
   if (dst != NULL) {
      strncpy(dst, uidgid_state_get_last_groupname(), sz);
   }

   DEXIT; 
   return 0;
} /* sge_gid2group() */


static int get_group_buffer_size(void)
{
   enum { buf_size = 20480 };  /* default is 20 KB */
   
   int sz = buf_size;

#ifdef _SC_GETGR_R_SIZE_MAX
   if ((sz = (int)sysconf(_SC_GETGR_R_SIZE_MAX)) != 0)
   {
      sz = buf_size;
   }
#endif

   return sz;
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
*  NOTES
*     MT-NOTE: sge_set_uid_gid_addgrp() is not MT safe because it depends
*     MT-NOTE: on sge_switch2start_user() and sge_getpwnam()
*
*     TODO: This function needs to be rewritten from scratch! It calls
*     'initgroups()' which is not part of POSIX. The call to 'initgroups()'
*     shall be replaced by a combination of 'getgroups()/getegid()/setgid()'.
*      
*     This function is used by 'shepherd' only anyway. Hence it shall be
*     considered to move it from 'libuti' to 'shepherd'.
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
#if !(defined(WIN32) || defined(INTERIX)) /* var not needed */
   int status;
#endif
   struct passwd *pw;
 
   sge_switch2start_user();
 
   if (!sge_is_start_user_superuser()) {
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
                 user, u32c( pw->pw_gid), min_gid);
         return 1;
      }
      if (setgid(pw->pw_gid)) {
         sprintf(err_str,MSG_SYSTEM_SETGIDFAILED_U , u32c(pw->pw_gid) );
         return 1;
      }
   } else {
      if (setegid(pw->pw_gid)) {
         sprintf(err_str, MSG_SYSTEM_SETEGIDFAILED_U , u32c(pw->pw_gid));
         return 1;
      }
   }

#if !(defined(WIN32) || defined(INTERIX)) /* initgroups not called */
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
      if (sge_add_group(add_grp, err_str) == -1) {
         return 1;
      }
   }
#endif
 
   if (!intermediate_user) {
      if (pw->pw_uid < min_uid) {
         sprintf(err_str, MSG_SYSTEM_UIDLESSTHANMINIMUM_SUI ,
                 user, u32c(pw->pw_uid), min_uid);
         return 1;
      }
 
      if (use_qsub_gid) {
         if (setgid(pw->pw_gid)) {
            sprintf(err_str, MSG_SYSTEM_SETGIDFAILED_U, u32c(pw->pw_uid));
            return 1;
         }
      }
 
      if (setuid(pw->pw_uid)) {
         sprintf(err_str, MSG_SYSTEM_SETUIDFAILED_U , u32c(pw->pw_uid));
         return 1;
      }
   } else {
      if (use_qsub_gid) {
         if (setgid(pw->pw_gid)) {
            sprintf(err_str, MSG_SYSTEM_SETGIDFAILED_U , u32c(pw->pw_uid));
            return 1;
         }
      }
 
      if (seteuid(pw->pw_uid)) {
         sprintf(err_str, MSG_SYSTEM_SETEUIDFAILED_U , u32c(pw->pw_uid));
         return 1;
      }
   }
 
   return 0;
} /* sge_set_uid_gid_addgrp() */ 

/****** uti/uidgid/sge_add_group() ********************************************
*  NAME
*     sge_add_group() -- Add a gid to the list of additional group ids
*
*  SYNOPSIS
*     int sge_add_group(gid_t add_grp_id, char *err_str)
*
*  FUNCTION
*     Add a gid to the list of additional group ids. If 'add_grp_id' 
*     is 0 don't add value to group id list (but return sucessfully).
*     If an error occurs, a descriptive string will be written to 
*     err_str.
*
*  INPUTS
*     gid_t add_grp_id - new gid
*     char *err_str    - if points to a valid string buffer
*                        error descriptions 
*                        will be written here
*
*  NOTE
*     MT-NOTE: sge_add_group() is MT safe
*
*  RESULT
*     int - error state
*         0 - Success
*        -1 - Error
******************************************************************************/
int sge_add_group(gid_t add_grp_id, char *err_str)
{
   u_long32 max_groups;
   gid_t *list;
   int groups;

   if(err_str != NULL) {
      err_str[0] = 0;
   }

   if (add_grp_id == 0) {
      return 0;
   }

   max_groups = sge_sysconf(SGE_SYSCONF_NGROUPS_MAX);
   if (max_groups <= 0) {
      if(err_str != NULL) {
         sprintf(err_str, MSG_SYSTEM_ADDGROUPIDFORSGEFAILED_UUS, u32c(getuid()), 
                 u32c(geteuid()), MSG_SYSTEM_INVALID_NGROUPS_MAX);
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
         sprintf(err_str, MSG_SYSTEM_ADDGROUPIDFORSGEFAILED_UUS, u32c(getuid()), 
                 u32c(geteuid()), strerror(error));
      }
      return -1;
   }
 
   groups = getgroups(max_groups, list);
   if (groups == -1) {
      if(err_str != NULL) {
         int error = errno;
         sprintf(err_str, MSG_SYSTEM_ADDGROUPIDFORSGEFAILED_UUS, u32c(getuid()), 
                 u32c(geteuid()), strerror(error));
      }
      free(list);
      return -1;
   }   
#if !defined(INTERIX)
   if (groups < max_groups) {
      list[groups] = add_grp_id;
      groups++;
      groups = setgroups(groups, list);
      if (groups == -1) {
         if(err_str != NULL) {
            int error = errno;
            sprintf(err_str, MSG_SYSTEM_ADDGROUPIDFORSGEFAILED_UUS, u32c(getuid()), 
                    u32c(geteuid()), strerror(error));
         }
         free(list);
         return -1;
      }
   } else {
      if(err_str != NULL) {
         sprintf(err_str, MSG_SYSTEM_ADDGROUPIDFORSGEFAILED_UUS, u32c(getuid()), 
                 u32c(geteuid()), MSG_SYSTEM_USER_HAS_TOO_MANY_GIDS);
      }
      free(list);
      return -1;
   }                      
#endif
   free(list);
   return 0;
}  

/****** uti/uidgid/sge_getpwnam() *********************************************
*  NAME
*     sge_getpwnam() -- Return password file entry for certain user 
*
*  SYNOPSIS
*     struct passwd* sge_getpwnam(const char *name) 
*
*  FUNCTION
*     Return password file entry for certain user.
*      
*  INPUTS
*     const char *name - Username 
*
*  NOTE
*     MT-NOTE: sge_getpwnam() is not MT safe; should use getpwname_r() instead
*
*  RESULT
*     struct passwd* - see getpwnam()
*******************************************************************************/
struct passwd *sge_getpwnam(const char *name)
{
   struct passwd *pw = NULL;
   int i = MAX_NIS_RETRIES;
 
   DENTER(UIDGID_LAYER, "sge_getpwnam");

   while (i-- && !pw) {
      pw = getpwnam(name);
   }

   /* sometime on failure struct is non NULL but name is empty */
   if (pw && !pw->pw_name) { 
      pw = NULL;
   }
 
   DEXIT;
   return pw;
} /* sge_getpwnam */
  
/****** sge_uidgid/sge_getpwnam_r() ********************************************
*  NAME
*     sge_getpwnam_r() -- Return password file entry for a given user name. 
*
*  SYNOPSIS
*     struct passwd* sge_getpwnam_r(const char*, struct passwd*, char*, int) 
*
*  FUNCTION
*     Search user database for a name. This function is just a wrapper for
*     'getpwnam_r()', taking into account some additional possible errors.
*     For a detailed description see 'getpwnam_r()' man page.
*
*  INPUTS
*     const char *name  - points to user name 
*     struct passwd *pw - points to structure which will be updated upon success 
*     char *buffer      - points to memory referenced by 'pw'
*     int buflen        - size of 'buffer' in bytes 
*
*  RESULT
*     struct passwd* - Pointer to entry matching user name upon success,
*                      NULL otherwise.
*
*  NOTES
*     MT-NOTE: sge_getpwnam_r() is MT safe. 
*
*******************************************************************************/
struct passwd *sge_getpwnam_r(const char *name, struct passwd *pw, char *buffer, int buflen)
{
   struct passwd *res = NULL;
   int i = MAX_NIS_RETRIES;
 
   DENTER(UIDGID_LAYER, "sge_getpwnam_r");

   while (i-- && !res) {
      if (getpwnam_r(name, pw, buffer, buflen, &res) != 0) {
         res = NULL;
      }
   }
 
   /* sometime on failure struct is non NULL but name is empty */
   if (res && !res->pw_name) {
      res = NULL;
   }
 
   DEXIT;
   return res;
} /* sge_getpwnam_r() */

/****** libs/uti/uidgid_state_get_*() ************************************
*  NAME
*     uidgid_state_set_*() - read access to lib/uti/sge_uidgid.c global variables
*
*  FUNCTION
*     Provides access to per thread global variable.
*
******************************************************************************/
static uid_t uidgid_state_get_last_uid(void)
{ 
   GET_SPECIFIC(struct uidgid_state_t, uidgid_state, uidgid_state_init, uidgid_state_key, "uidgid_state_get_last_uid");
   return uidgid_state->last_uid;
}

static const char *uidgid_state_get_last_username(void)
{ 
   GET_SPECIFIC(struct uidgid_state_t, uidgid_state, uidgid_state_init, uidgid_state_key, "uidgid_state_get_last_username");
   return uidgid_state->last_username;
}

static gid_t uidgid_state_get_last_gid(void)
{ 
   GET_SPECIFIC(struct uidgid_state_t, uidgid_state, uidgid_state_init, uidgid_state_key, "uidgid_state_get_last_gid");
   return uidgid_state->last_gid;
}

static const char *uidgid_state_get_last_groupname(void)
{ 
   GET_SPECIFIC(struct uidgid_state_t, uidgid_state, uidgid_state_init, uidgid_state_key, "uidgid_state_get_last_groupname");
   return uidgid_state->last_groupname;
}

/****** libs/uti/uidgid_state_set_*() ************************************
*  NAME
*     uidgid_state_set_*() - write access to lib/uti/sge_uidgid.c global variables
*
*  FUNCTION
*     Provides access to per thread global variable.
*
******************************************************************************/
static void uidgid_state_set_last_uid(uid_t uid)
{ 
   GET_SPECIFIC(struct uidgid_state_t, uidgid_state, uidgid_state_init, uidgid_state_key, "uidgid_state_set_last_uid");
   uidgid_state->last_uid = uid;
}

static void uidgid_state_set_last_username(const char *user)
{ 
   GET_SPECIFIC(struct uidgid_state_t, uidgid_state, uidgid_state_init, uidgid_state_key, "uidgid_state_set_last_username");
   strncpy(uidgid_state->last_username, user, SGE_MAX_USERGROUP_BUF-1);
}

static void uidgid_state_set_last_gid(gid_t gid)
{ 
   GET_SPECIFIC(struct uidgid_state_t, uidgid_state, uidgid_state_init, uidgid_state_key, "uidgid_state_set_last_gid");
   uidgid_state->last_gid = gid;
}

static void uidgid_state_set_last_groupname(const char *group)
{ 
   GET_SPECIFIC(struct uidgid_state_t, uidgid_state, uidgid_state_init, uidgid_state_key, "uidgid_state_set_last_groupname");
   strncpy(uidgid_state->last_groupname, group, SGE_MAX_USERGROUP_BUF-1);
}

/****** uti/uidgid/set_admin_user() ********************************************
*  NAME
*     set_admin_user() -- Set user and group id of admin user. 
*
*  SYNOPSIS
*     static void set_admin_user(uid_t theUID, gid_t theGID) 
*
*  FUNCTION
*     Set user and group id of admin user. 
*
*  INPUTS
*     uid_t theUID - user id of admin user 
*     gid_t theGID - group id of admin user 
*
*  RESULT
*     static void - none
*
*  NOTES
*     MT-NOTE: set_admin_user() is MT safe. 
*
*******************************************************************************/
static void set_admin_user(uid_t theUID, gid_t theGID)
{
   uid_t uid = theUID;
   gid_t gid = theGID;

   DENTER(UIDGID_LAYER, "set_admin_user");

   sge_mutex_lock("admin_user_mutex", SGE_FUNC, __LINE__, &admin_user.mutex);
   admin_user.uid = uid;
   admin_user.gid = gid;
   admin_user.initialized = true;
   sge_mutex_unlock("admin_user_mutex", SGE_FUNC, __LINE__, &admin_user.mutex);

   DPRINTF(("auid=%ld; agid=%ld\n", (long)uid, (long)gid));

   DEXIT;
   return;
} /* set_admin_user() */
     
/****** uti/uidgid/get_admin_user() ********************************************
*  NAME
*     get_admin_user() -- Get user and group id of admin user.
*
*  SYNOPSIS
*     static int get_admin_user(uid_t* theUID, gid_t* theGID) 
*
*  FUNCTION
*     Get user and group id of admin user. 'theUID' and 'theGID' will contain
*     the user and group id respectively, upon successful completion.
*
*     If the admin user has not been set by a call to 'set_admin_user()'
*     previously, an error is returned. In case of an error, the locations
*     pointed to by 'theUID' and 'theGID' remain unchanged.
*
*  OUTPUTS
*     uid_t* theUID - pointer to user id storage.
*     gid_t* theGID - pointer to group id storage.
*
*  RESULT
*     int - Returns ESRCH, if no admin user has been initialized. 
*
*  EXAMPLE
*
*     uid_t uid;
*     gid_t gid;
*
*     if (get_admin_user(&uid, &gid) == ESRCH) {
*        printf("error: no admin user\n");
*     } else {
*        printf("uid = %d, gid =%d\n", (int)uid, (int)gid);
*     }
*       
*  NOTES
*     MT-NOTE: get_admin_user() is MT safe.
*
*******************************************************************************/
static int  get_admin_user(uid_t* theUID, gid_t* theGID)
{
   uid_t uid;
   gid_t gid;
   bool init = false;
   int res = ESRCH;

   DENTER(UIDGID_LAYER, "get_admin_user");

   sge_mutex_lock("admin_user_mutex", SGE_FUNC, __LINE__, &admin_user.mutex);
   uid = admin_user.uid;
   gid = admin_user.gid;
   init = admin_user.initialized;
   sge_mutex_unlock("admin_user_mutex", SGE_FUNC, __LINE__, &admin_user.mutex);

   if (init == true) {
      *theUID = uid;
      *theGID = gid;
      res = 0;
   }

   DEXIT;
   return res;
} /* get_admin_user() */

/****** uti/uidgid/uidgid_once_init() ******************************************
*  NAME
*     uidgid_once_init() -- One-time user and group function initialization.
*
*  SYNOPSIS
*     static uidgid_once_init(void) 
*
*  FUNCTION
*     Create access key for thread local storage. Register cleanup function.
*
*     This function must be called exactly once.
*
*  INPUTS
*     void - none
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: uidgid_once_init() is MT safe. 
*
*******************************************************************************/
static void uidgid_once_init(void)
{
   pthread_key_create(&uidgid_state_key, uidgid_state_destroy);
}

/****** uti/uidgid/uidgid_state_destroy() **************************************
*  NAME
*     uidgid_state_destroy() -- Free thread local storage
*
*  SYNOPSIS
*     static void uidgid_state_destroy(void* theState) 
*
*  FUNCTION
*     Free thread local storage.
*
*  INPUTS
*     void* theState - Pointer to memroy which should be freed.
*
*  RESULT
*     static void - none
*
*  NOTES
*     MT-NOTE: uidgid_state_destroy() is MT safe.
*
*******************************************************************************/
static void uidgid_state_destroy(void* theState)
{
   free((struct uidgid_state_t *)theState);
}

/****** uti/uidgid/uidgid_state_init() *****************************************
*  NAME
*     uidgid_state_init() -- Initialize user and group function state.
*
*  SYNOPSIS
*     static void cull_state_init(struct cull_state_t* theState) 
*
*  FUNCTION
*     Initialize user and group function state.
*
*  INPUTS
*     struct cull_state_t* theState - Pointer to user and group state structure.
*
*  RESULT
*     static void - none
*
*  NOTES
*     MT-NOTE: cull_state_init() in MT safe. 
*
*******************************************************************************/
static void uidgid_state_init(struct uidgid_state_t* theState)
{
   memset(theState, 0, sizeof(struct uidgid_state_t));
}

/****** uti/uidgid/sge_is_start_user_root() **********************************
*  NAME
*     sge_is_start_user_root() -- return true/false if start user was root
*
*  SYNOPSIS
*     bool sge_is_start_user_root(void)
*
*  FUNCTION
*     return true/false if start user was root or not
*
*  RESULT
*         true - root was start user
*         false - otherwise
*
*  NOTES
*     MT-NOTE: sge_is_start_user_root() is MT safe.
*
*  SEE ALSO
*     uti/uidgid/sge_switch2admin_user()
*     uti/uidgid/sge_set_admin_username()
*     uti/uidgid/sge_switch2start_user()
*     uti/uidgid/sge_run_as_user()
******************************************************************************/
bool sge_is_start_user_root(void)
{
   uid_t start_uid;

   DENTER(UIDGID_LAYER, "sge_is_start_user_root");
 
   start_uid = getuid();

   DEXIT;
   return start_uid ? true : false;
   
} /* sge_is_start_user_root() */ 

