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
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <pwd.h>
#include <strings.h>
#include <stdlib.h>

#include "msg_utilib.h"
#include "sge_switch_user.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_getpwnam.h"

#if defined(SUN4)
   int seteuid(uid_t euid);
   int setegid(gid_t egid);
#endif

#if defined(HP10) || defined(HP11)
#  define seteuid(euid) setresuid(-1, euid, -1)
#  define setegid(egid) setresgid(-1, egid, -1)
#endif

#define SWITCH_USER_NOT_INITIALIZED "Module 'sge_switch_user' not initialized"
#define SWITCH_USER_NOT_ROOT "User 'root' did not start the application"

static void set_admin_user(uid_t, gid_t);

static uid_t admin_uid = -1;
static gid_t admin_gid = -1;

static int initialized = 0;

/*--------------------------------------------------------------
 * set_admin_username
 *
 * set internal admin user to "username"
 * if username = none then use current uid/gid
 * silently ignore if current user is not root
 * return 0 in case of succes
 *        -1 if username does not exist
 *        -2 if module is already initialized
 *--------------------------------------------------------------*/
int set_admin_username(
char *user,
char *err_str  
) {
   struct passwd *admin_user;
   int ret;
   DENTER(TOP_LAYER, "set_admin_username");

   if (initialized) {
      DEXIT;
      return -2;
   }
   if (!user || user[0] == '\0') {
      if (err_str)
         sprintf(err_str, MSG_POINTER_SETADMINUSERNAMEFAILED ); 
      DEXIT;
      return -1;
   }   
      
   ret = 0;
   if (!strcasecmp(user, "none"))
      set_admin_user(getuid(), getgid());
   else {
      admin_user = sge_getpwnam(user);
      if (admin_user)
         set_admin_user(admin_user->pw_uid, admin_user->pw_gid);
      else {
         if (err_str)
            sprintf(err_str, MSG_SYSTEM_ADMINUSERNOTEXIST_S, user);
         ret = -1;
      }
   }
   DEXIT;
   return ret;
}


/*--------------------------------------------------------------
 * set_admin_user
 *
 * set internal admin uid and gid
 *--------------------------------------------------------------*/
static void set_admin_user(
uid_t a_uid, 
gid_t a_gid 
) {
   DENTER(TOP_LAYER, "set_admin_user");
   
   admin_uid = a_uid;
   admin_gid = a_gid;
   initialized = 1;
   DPRINTF(("uid=%ld, gid=%ld\n", (long) a_uid, (long) a_gid));
   DEXIT;
}


/*--------------------------------------------------------------
 * switch2admin_user
 *
 * set euid/egid to admin uid/gid
 * silently ignore if our uid is not 0 (and return 0)
 * if our euid/egid is already the admin_gid/admin_user don't do anything
 * return 0 in case of success
 *       -1 of setegid()/seteuid() fails
 *       abort in case module was not initialized
 *--------------------------------------------------------------*/
int switch2admin_user() 
{
   DENTER(TOP_LAYER, "switch2admin_user");

   if (!initialized) {
      CRITICAL((SGE_EVENT, SWITCH_USER_NOT_INITIALIZED));
      abort();
   }

   if (getuid()) {
      DPRINTF((SWITCH_USER_NOT_ROOT"\n"));   
      DEXIT;   
      return 0;   
   } 
   else {
      if (getegid() != admin_gid) {
         if (setegid(admin_gid) == -1) {
            DEXIT;
            return -1;
         }
         else
            DPRINTF(("egid=%ld\n", (long) admin_gid));
      }

      if (geteuid() != admin_uid) {
         if (seteuid(admin_uid) == -1) {
            DEXIT;
            return -1;
         }
         else
            DPRINTF(("euid=%ld\n", (long) admin_uid));
      }
   }
   DEXIT;
   return 0;
}

/*--------------------------------------------------------------
 * switch2start_user
 *
 * set euid/egid to uid/gid 
 * silently ignore if our uid is not 0 (and return 0)
 * if our euid/egid is already the start_gid/start_user don't do anything
 * return 0 in case of success
 *       -1 of setegid()/seteuid() fails
 *       abort in case module was not initialized
 *--------------------------------------------------------------*/
int switch2start_user() 
{
   uid_t start_uid = getuid();
   gid_t start_gid = getgid();

   DENTER(TOP_LAYER, "switch2start_user");

   if (!initialized) {
      CRITICAL((SGE_EVENT, SWITCH_USER_NOT_INITIALIZED));
      abort();
   }

   if (start_uid) {
      DPRINTF((SWITCH_USER_NOT_ROOT"\n"));
      DEXIT;   
      return 0;
   } 
   else {
      if (start_gid != getegid()) {
         if (setegid(start_gid) == -1) {
            DEXIT;
            return -1;
         }
         else 
            DPRINTF(("gid=%ld\n", (long) start_gid));
      }
      if (start_uid != geteuid()) {
         if (seteuid(start_uid) == -1) {
            DEXIT;
            return -1;	
         }
         else
            DPRINTF(("uid=%ld\n", (long) start_uid));
      }
   }
   DEXIT;
   return 0;
}

/*--------------------------------------------------------------
 * int run_as_user(void)
 *
 * set euid to uid
 * setuid root binary switches its euid to the callers uid
 * return 0 in case of success
 *       -1 of setegid()/seteuid() fails
 *--------------------------------------------------------------*/
int run_as_user(void) 
{
   int ret = 0;

   DENTER(TOP_LAYER, "run_as_user");

   if (geteuid() != getuid()) {
      if (seteuid(getuid()))
         ret = -1;
   }      
      
   DEXIT;
   return ret;
}
