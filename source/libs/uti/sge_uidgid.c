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
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

#include "msg_utilib.h"
#include "sgermon.h"
#include "sge_uidgid.h"
#include "sge_log.h"

#ifdef QIDL
#include <pthread.h>
#endif

/* ------------------------------------------------------------

   sge_user2uid - resolves user name to uid

   returns
      0 on success
      1 on failure

   if uidp is NULL the user name is resolved
   without saving it

*/
int sge_user2uid(
const char *user,
uid_t *uidp,
int retries 
) {
   struct passwd *pw;

   DENTER(CULL_LAYER, "sge_user2uid");

   do {
      DPRINTF(("name: %s retries: %d\n", user, retries));

      if (!retries--) {
         DEXIT;
         return 1;
      }
      /* resolve user name to uid */
   } while ((pw = getpwnam(user))==NULL);

   if ( uidp )
      *uidp = pw->pw_uid;

   DEXIT; 
   return 0;
}

/* ------------------------------------------------------------

   sge_group2gid - resolves group name to gid

   returns
      0 on success
      1 on failure

   if gidp is NULL the group name is resolved
   without saving it

*/
int sge_group2gid(
const char *gname,
gid_t *gidp,
int retries 
) {
   struct group *gr;

   DENTER(CULL_LAYER, "sge_group2gid");

   do {
      if (!retries--) {
         DEXIT;
         return 1;
      }
   /* resolve group name to gid */
   } while ((gr = getgrnam(gname))==NULL);
   
   if ( gidp )
      *gidp = gr->gr_gid;

   DEXIT; 
   return 0;
}


/* ------------------------------------------------------------

   sge_uid2user - resolves uid to user name

   if dst is NULL sge_uid2user only checks 
   if id is resolvable

   returns
      0 on success
      1 on failure

*/
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
                  u32c(uid), strerror(errno)));
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

/* ------------------------------------------------------------

   sge_gid2user - resolves gid to user name

   returns
      0 on success
      1 on failure

   if dst is NULL sge_gid2group only checks 
   if id is resolvable

*/
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
                  u32c(gid), strerror(errno)));
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
