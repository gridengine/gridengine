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
#include <errno.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#if defined(SGE_MT)
#include <pthread.h>
#endif

#include "sge_bootstrap.h"
#include "sge_hostname.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_language.h"
#include "sge_time.h" 
#include "sge_unistd.h"
#include "sge_string.h"
#include "sge_prog.h"
#include "sge_uidgid.h"


#include "msg_utilib.h"
#include "sge_mtutil.h"

#define ALIAS_DELIMITER "\n\t ,;"

#ifndef h_errno
extern int h_errno;
#endif

extern void trace(char *);


/* MT-NOTE: hostlist used only in qmaster, commd and some utilities */
static host *hostlist = NULL;

/* MT-NOTE: localhost used only in commd */
static host *localhost = NULL;

#ifdef ENABLE_NGC
#else
static int sge_host_copy_entry(struct hostent *heto, struct hostent *hefrom);
static host *sge_host_create(void);
static int sge_host_alias(host *h1, host *h2);
#endif
static int matches_addr(struct hostent *he, char *addr);
static host *sge_host_search_pred_alias(host *h);

static int matches_name(struct hostent *he, const char *name);    
static void sge_host_delete(host *h);

/* this globals are used for profiling */
unsigned long gethostbyname_calls = 0;
unsigned long gethostbyname_sec = 0;
unsigned long gethostbyaddr_calls = 0;
unsigned long gethostbyaddr_sec = 0;


#if 0 /*defined(SOLARIS)*/
int gethostname(char *name, int namelen);
#endif

#ifdef GETHOSTBYNAME_M
/* guards access to the non-MT-safe gethostbyname system call */
static pthread_mutex_t hostbyname_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#ifdef GETHOSTBYADDR_M
/* guards access to the non-MT-safe gethostbyaddr system call */
static pthread_mutex_t hostbyaddr_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif


/****** libs/uti/uti_state_get_localhost() *************************************
*  NAME
*     uti_state_get_localhost() - read access to uti lib global variables
*
*  FUNCTION
*     Provides access to either global variable or per thread global variable.
*
*******************************************************************************/
host *uti_state_get_localhost(void)
{
   /* so far called only by commd */ 
   return localhost;
}

#define MAX_RESOLVER_BLOCKING 15

/****** uti/host/sge_gethostbyname_retry() *************************************
*  NAME
*     sge_gethostbyname_retry() -- gethostbyname() wrapper
*
*  SYNOPSIS
*     struct hostent *sge_gethostbyname_retry(const char *name)
*
*  FUNCTION
*     Wraps sge_gethostbyname() function calls, and retries if the host is not
*     found.
*
*     return value must be released by function caller (don't forget the 
*     char*‹‹ array lists inside of struct hostent)
*
*  NOTES
*     MT-NOTE: see sge_gethostbyname()
*******************************************************************************/
struct hostent *sge_gethostbyname_retry(
const char *name 
) {
   int i;
   struct hostent *he;

   DENTER(TOP_LAYER, "sge_gethostbyname_retry");

   if (!name || name[0] == '\0') {
      DPRINTF(("hostname to resolve is NULL or has zero length\n"));
      DEXIT;
      return NULL;
   }

   he = sge_gethostbyname(name);
   if (he == NULL) {
      for (i = 0; i < MAX_NIS_RETRIES && he == NULL; i++) {
         DPRINTF(("Couldn't resolve hostname %s\n", name));
         sleep(1);
         he = sge_gethostbyname(name);
      }
   }

   DEXIT;
   return he;
}

/****** uti/host/sge_gethostbyname() ****************************************
*  NAME
*     sge_gethostbyname() -- gethostbyname() wrapper
*
*  SYNOPSIS
*     struct hostent *sge_gethostbyname(const char *name)
*
*  FUNCTION
*     Wrapps gethostbyname() function calls, measures time spent 
*     in gethostbyname() and logs when very much time has passed.
*
*     return value must be released by function caller (don't forget the 
*     char*‹‹ array lists inside of struct hostent)
*
*
*  NOTES
*     MT-NOTE: sge_gethostbyname() is MT safe
*     MT-NOTE: sge_gethostbyname() uses a mutex to guard access to the
*     MT-NOTE: gethostbyname() system call on all platforms other than Solaris,
*     MT-NOTE: Linux, AIX, Tru64, HP-UX, and MacOS 10.2 and greater.  Therefore,
*     MT-NOTE: except on the aforementioned platforms, MT calls to
*     MT-NOTE: gethostbyname() must go through sge_gethostbyname() to be MT safe.
*******************************************************************************/
struct hostent *sge_gethostbyname(const char *name)
{
   struct hostent *he = NULL;
   time_t now;
   int time;
   int l_errno;
   
   DENTER(TOP_LAYER, "sge_gethostbyname");

   /* This method goes to great lengths to slip a reentrant gethostbyname into
    * the code without making changes to the rest of the source base.  That
    * basically means that we have to make some redundant copies to
    * return to the caller.  This method doesn't appear to be highly utilized,
    * so that's probably ok.  If it's not ok, the interface can be changed
    * later. */
   
   now = sge_get_gmt();
   gethostbyname_calls++;       /* profiling */
   
#ifdef GETHOSTBYNAME_R6
   /* This is for Linux */
   DPRINTF (("Getting host by name - Linux\n"));
   {
      struct hostent re;
      char buffer[4096];

      /* No need to malloc he because it will end up pointing to re. */
      gethostbyname_r (name, &re, buffer, 4096, &he, &l_errno);
      
      /* Since re contains pointers into buffer, and both re and the buffer go
       * away when we exit this code block, we make a deep copy to return. */
      /* Yes, I do mean to check if he is NULL and then copy re!  No, he
       * doesn't need to be freed first. */
      if (he != NULL) {
         he = sge_copy_hostent (&re);
      }
   }
#endif
#ifdef GETHOSTBYNAME_R5
   /* This is for Solaris */
   DPRINTF (("Getting host by name - Solaris\n"));
   {
      char buffer[4096];
      struct hostent *help_he = NULL;

      he = (struct hostent *)malloc (sizeof (struct hostent));
      /* On Solaris, this function returns the pointer to my struct on success
       * and NULL on failure. */
      help_he = gethostbyname_r (name, he, buffer, 4096, &l_errno);
      
      /* Since he contains pointers into buffer, and buffer goes away when we
       * exit this code block, we make a deep copy to return. */
      if (help_he != NULL) {
         struct hostent *new_he = sge_copy_hostent (he);
         FREE (he);
         he = new_he;
      } else {
         FREE (he);
         he = NULL;
      }
   }
#endif
#ifdef GETHOSTBYNAME_R3
   /* This is for AIX < 4.3, HPUX < 11, and Tru64 */
   DPRINTF (("Getting host by name - 3 arg\n"));
   
   {
      struct hostent_data he_data;
      
      he = (struct hostent *)malloc (sizeof (struct hostent));
      if (gethostbyname_r (name, he, &he_data) < 0) {
         /* If this function fails, free he so that we can test if it's NULL
          * later in the code. */
         FREE (he);
         he = NULL;
      }
      /* The location of the error code is actually undefined.  I'm just
       * assuming that it's in h_errno since that's where it is in the unsafe
       * version.
       * h_errno is, of course, not thread safe, but if there's an error we're
       * already screwed, so we won't worry to much about it.
       * An alternative would be to set errno to HOST_NOT_FOUND. */
      l_errno = h_errno;
      
      /* Since he contains pointers into he_data, and he_data goes away when we
       * exit this code block, we make a deep copy to return. */
      if (he != NULL) {
         struct hostent *new_he = sge_copy_hostent (he);
         FREE (he);
         he = new_he;
      }
   }
#endif
#ifdef GETHOSTBYNAME
   /* This is for AIX >= 4.3, HPUX >= 11, and Mac OS X >= 10.2 */
   DPRINTF (("Getting host by name - Thread safe\n"));
   he = gethostbyname(name);
   /* The location of the error code is actually undefined.  I'm just
    * assuming that it's in h_errno since that's where it is in the unsafe
    * version.
    * h_errno is, of course, not thread safe, but if there's an error we're
    * already screwed, so we won't worry too much about it.
    * An alternative would be to set errno to HOST_NOT_FOUND. */
   l_errno = h_errno;
   if (he != NULL) {
      struct hostent *new_he = sge_copy_hostent (he);
      /* do NOT free he, there was no malloc() */
      he = new_he;
   }
#endif
#ifdef GETHOSTBYNAME_M
   /* This is for Mac OS < 10.2, IRIX, and everyone else
    *  - Actually, IRIX 6.5.17 supports a reentrant getaddrinfo(), but it's not
    *    worth the effort. */
   DPRINTF (("Getting host by name - Mutex guarded\n"));

   sge_mutex_lock("hostbyname", SGE_FUNC, __LINE__, &hostbyname_mutex);

   he = gethostbyname(name);
   l_errno = h_errno;

   if (he != NULL) {
      struct hostent *new_he = sge_copy_hostent (he);
      /* do NOT free he, there was no malloc() */
      he = new_he;
   }
   sge_mutex_unlock("hostbyname", SGE_FUNC, __LINE__, &hostbyname_mutex);

#endif

   time = sge_get_gmt() - now;
   gethostbyname_sec += time;   /* profiling */

   /* warn about blocking gethostbyname() calls */
   if (time > MAX_RESOLVER_BLOCKING) {
      WARNING((SGE_EVENT, "gethostbyname(%s) took %d seconds and returns %s\n", 
            name, time, he?"success":
          (l_errno == HOST_NOT_FOUND)?"HOST_NOT_FOUND":
          (l_errno == TRY_AGAIN)?"TRY_AGAIN":
          (l_errno == NO_RECOVERY)?"NO_RECOVERY":
          (l_errno == NO_DATA)?"NO_DATA":
          (l_errno == NO_ADDRESS)?"NO_ADDRESS":"<unknown error>"));
   }

   DEXIT;
   return he;
}

/****** uti/host/sge_copy_hostent() ****************************************
*  NAME
*     sge_copy_hostent() -- make a deep copy of a struct hostent
*
*  SYNOPSIS
*     struct hostent *sge_copy_hostent (struct hostent *orig)
*
*  FUNCTION
*     Makes a deep copy of a struct hostent so that sge_gethostbyname() can
*     free it's buffer for the gethostbyname_r() calls on Linux and Solaris.
*
*  NOTES
*     MT-NOTE: sge_copy_hostent() is MT safe
*******************************************************************************/
struct hostent *sge_copy_hostent(struct hostent *orig)
{
   struct hostent *copy = (struct hostent *)malloc (sizeof (struct hostent));
   char **p = NULL;
   int count = 0;

   DENTER (TOP_LAYER, "sge_copy_hostent");
   
   /* Easy stuff first */
   copy->h_name = strdup (orig->h_name);
   copy->h_addrtype = orig->h_addrtype;
   copy->h_length = orig->h_length;
   
   /* Count the number of entries */
   count = 0;
   for (p = orig->h_addr_list; *p != 0; p++) {
      count++;
   }
   
   DPRINTF (("%d names in h_addr_list\n", count));
   
   copy->h_addr_list = (char **) malloc (sizeof (char *) * (count + 1));
   
   /* Copy the entries */
   count = 0;
   for (p = orig->h_addr_list; *p != 0; p++) {
      int tmp_size = sizeof (in_addr_t);
      
      copy->h_addr_list[count] = (char *)malloc (tmp_size);
      memcpy (copy->h_addr_list[count++], *p, tmp_size);
   }
   
   copy->h_addr_list[count] = NULL;
   
   /* Count the number of entries */
   count = 0;
   for (p = orig->h_aliases; *p != 0; p++) {
      count++;
   }
   
   DPRINTF (("%d names in h_aliases\n", count));
   
   copy->h_aliases = (char **) malloc (sizeof (char *) * (count + 1));
   
   /* Copy the entries */
   count = 0;
   for (p = orig->h_aliases; *p != 0; p++) {
      int tmp_size = (strlen(*p) + 1) * sizeof(char);

      copy->h_aliases[count] = (char *)malloc (tmp_size);
      memcpy (copy->h_aliases[count++], *p, tmp_size);
   }
   
   copy->h_aliases[count] = NULL;
   
   DEXIT;
   return copy;
}

/****** uti/host/sge_gethostbyaddr() ****************************************
*  NAME
*     sge_gethostbyaddr() -- gethostbyaddr() wrapper
*
*  SYNOPSIS
*     struct hostent *sge_gethostbyaddr(const struct in_addr *addr)
*
*  FUNCTION
*     Wrapps gethostbyaddr() function calls, measures time spent 
*     in gethostbyaddr() and logs when very much time has passed.
*
*     return value must be released by function caller (don't forget the 
*     char*‹‹ array lists inside of struct hostent)
*
*  NOTES
*     MT-NOTE: sge_gethostbyaddr() is MT safe
*     MT-NOTE: sge_gethostbyaddr() uses a mutex to guard access to the
*     MT-NOTE: gethostbyaddr() system call on all platforms other than Solaris,
*     MT-NOTE: Linux, and HP-UX.  Therefore, except on the aforementioned
*     MT-NOTE: platforms, MT calls to gethostbyaddr() must go through
*     MT-NOTE: sge_gethostbyaddr() to be MT safe.
*******************************************************************************/
struct hostent *sge_gethostbyaddr(const struct in_addr *addr)
{
   struct hostent *he = NULL;
   time_t now;
   int time;
   int l_errno;

   DENTER(TOP_LAYER, "sge_gethostbyaddr");

   /* This method goes to great lengths to slip a reentrant gethostbyaddr into
    * the code without making changes to the rest of the source base.  That
    * basically means that we have to make some redundant copies to
    * return to the caller.  This method doesn't appear to be highly utilized,
    * so that's probably ok.  If it's not ok, the interface can be changed
    * later. */

   gethostbyaddr_calls++;      /* profiling */
   now = sge_get_gmt();

#ifdef GETHOSTBYADDR_R8
   /* This is for Linux */
   DPRINTF (("Getting host by addr - Linux\n"));
   {
      struct hostent re;
      char buffer[4096];

      /* No need to malloc he because it will end up pointing to re. */
      gethostbyaddr_r ((const char *)addr, 4, AF_INET, &re, buffer, 4096, &he, &l_errno);
      
      /* Since re contains pointers into buffer, and both re and the buffer go
       * away when we exit this code block, we make a deep copy to return. */
      /* Yes, I do mean to check if he is NULL and then copy re!  No, he
       * doesn't need to be freed first. */
      if (he != NULL) {
         he = sge_copy_hostent (&re);
      }
   }
#endif
#ifdef GETHOSTBYADDR_R7
   /* This is for Solaris */
   DPRINTF (("Getting host by addr - Solaris\n"));
   {
      char buffer[4096];
      struct hostent *help_he = NULL;
      he = (struct hostent *)malloc (sizeof (struct hostent));
      /* On Solaris, this function returns the pointer to my struct on success
       * and NULL on failure. */
      help_he = gethostbyaddr_r ((const char *)addr, 4, AF_INET, he, buffer, 4096, &l_errno);
      
      /* Since he contains pointers into buffer, and buffer goes away when we
       * exit this code block, we make a deep copy to return. */
      if (help_he != NULL) {
         struct hostent *new_he = sge_copy_hostent (help_he);
         FREE (he);
         he = new_he;
      } else {
         FREE (he);
         he = NULL;
      }
   }
#endif
#ifdef GETHOSTBYADDR_R5
   /* This is for HPUX < 11 */
   DPRINTF (("Getting host by addr - 3 arg\n"));
   
   {
      struct hostent_data he_data;
      
      he = (struct hostent *)malloc (sizeof (struct hostent));
      if (gethostbyaddr_r ((const char *)addr, 4, AF_INET, he, &he_data) < 0) {
         /* If this function fails, free he so that we can test if it's NULL
          * later in the code. */
         FREE (he);
         he = NULL;
      }
      /* The location of the error code is actually undefined.  I'm just
       * assuming that it's in h_errno since that's where it is in the unsafe
       * version.
       * h_errno is, of course, not thread safe, but if there's an error we're
       * already screwed, so we won't worry to much about it.
       * An alternative would be to set errno to HOST_NOT_FOUND. */
      l_errno = h_errno;
      
      /* Since he contains pointers into he_data, and he_data goes away when we
       * exit this code block, we make a deep copy to return. */
      if (he != NULL) {
         struct hostent *new_he = sge_copy_hostent (he);
         FREE (he);
         he = new_he;
      }
   }
#endif
#ifdef GETHOSTBYADDR
   /* This is for HPUX >= 11 */
   DPRINTF (("Getting host by addr - Thread safe\n"));
   he = gethostbyaddr((const char *)addr, 4, AF_INET);
   /* The location of the error code is actually undefined.  I'm just
    * assuming that it's in h_errno since that's where it is in the unsafe
    * version.
    * h_errno is, of course, not thread safe, but if there's an error we're
    * already screwed, so we won't worry too much about it.
    * An alternative would be to set errno to HOST_NOT_FOUND. */
   l_errno = h_errno;
   if (he != NULL) {
      struct hostent *new_he = sge_copy_hostent(he);
      /* do not free he, there was no malloc() */
      he = new_he;
   }
#endif
#ifdef GETHOSTBYADDR_M
   /* This is for everone else. */
   DPRINTF (("Getting host by addr - Mutex guarded\n"));
   
   sge_mutex_lock("hostbyaddr", SGE_FUNC, __LINE__, &hostbyaddr_mutex);

#if defined(CRAY)
   he = gethostbyaddr((const char *)addr, sizeof(struct in_addr), AF_INET);
#else   
   he = gethostbyaddr((const char *)addr, 4, AF_INET);
#endif

   l_errno = h_errno;
   if (he != NULL) {
      struct hostent *new_he = sge_copy_hostent(he);
      /* do not free he, there was no malloc() */
      he = new_he;
   }
   sge_mutex_unlock("hostbyaddr", SGE_FUNC, __LINE__, &hostbyaddr_mutex);
#endif

   time = sge_get_gmt() - now;
   gethostbyaddr_sec += time;   /* profiling */

   /* warn about blocking gethostbyaddr() calls */
   if (time > MAX_RESOLVER_BLOCKING) {
      WARNING((SGE_EVENT, "gethostbyaddr() took %d seconds and returns %s\n", time, he?"success":
          (l_errno == HOST_NOT_FOUND)?"HOST_NOT_FOUND":
          (l_errno == TRY_AGAIN)?"TRY_AGAIN":
          (l_errno == NO_RECOVERY)?"NO_RECOVERY":
          (l_errno == NO_DATA)?"NO_DATA":
          (l_errno == NO_ADDRESS)?"NO_ADDRESS":"<unknown error>"));
   }

   DEXIT;
   return he;
}

/****** sge_hostname/sge_host_list_initialize() ********************************
*  NAME
*     sge_host_list_initialize() -- intialize host list
*
*  SYNOPSIS
*     void sge_host_list_initialize(void) 
*
*  NOTES
*     MT-NOTE: gethostbyaddr() is not MT safe, due to access to global variables
*
*******************************************************************************/
#ifdef ENABLE_NGC
#else
void sge_host_list_initialize(void)
{
   char localname[MAXHOSTLEN];

   DENTER(TOP_LAYER, "sge_host_list_initialize");

   if (localhost) {
      DEXIT;
      return;
   }   

   if (gethostname(localname, MAXHOSTLEN)) {
      ERROR((SGE_EVENT, MSG_NET_GETHOSTNAMEFAILED ));
      DEXIT;
      return;
   }
   DPRINTF(("adding localhost %s\n", localname));

   localhost = (host *) 1;      /* avoid recursion */
   localhost = sge_host_new_name(localname, NULL);
   if (!localhost) {
      ERROR((SGE_EVENT, MSG_NET_RESOLVINGLOCALHOSTFAILED ));
      DEXIT;
      return;
   }
   DEXIT;
}
#endif

/****** sge_hostname/sge_host_create() *****************************************
*  NAME
*     sge_host_create() -- create a new host in host list
*
*  SYNOPSIS
*     static host* sge_host_create(void) 
*
*  RESULT
*     static host* - returns host created
*
*  NOTES
*     MT-NOTE: sge_host_create() is not MT safe due to access to global variable
*
*******************************************************************************/
#ifdef ENABLE_NGC
#else
static host *sge_host_create(void)
{
   host *new = (host *) malloc(sizeof(host));

   if (!new)
      return NULL;

   memset(new, 0, sizeof(host));
   new->alias = NULL;
   new->next = hostlist;
   hostlist = new;

   return new;
}
#endif

/****** sge_hostname/sge_host_delete() *****************************************
*  NAME
*     sge_host_delete() -- delete host in host list with all aliases 
*
*  SYNOPSIS
*     static void sge_host_delete(host *h) 
*
*  INPUTS
*     host *h - host to be deleted
*
*  NOTES
*     MT-NOTE: sge_host_delete() is not MT safe due to access to global variable
*
*******************************************************************************/
static void sge_host_delete(host *h) 
{
   host *last = NULL, *hl = hostlist;
   host *predalias, *nextalias;

   if (!h)
      return;

   predalias = sge_host_search_pred_alias(h);
   nextalias = h->alias;

   while (hl && hl != h) {
      last = hl;
      hl = hl->next;
   }
   if (hl) {
      if (last)
         last->next = hl->next;
      else
         hostlist = hostlist->next;
   }
   sge_strafree(h->he.h_aliases);
   sge_strafree(h->he.h_addr_list);
   free(h);

   sge_host_delete(predalias);
   sge_host_delete(nextalias);
}


/****** uti/hostname/sge_host_new_addr() **************************************
*  NAME
*     sge_host_new_addr() -- Create new host from internet address 
*
*  SYNOPSIS
*     host* sge_host_new_addr(const struct in_addr *addr) 
*
*  FUNCTION
*     Create new host from internet address in hostorder
*
*  INPUTS
*     const struct in_addr addr - inet-address 
*
*  NOTES
*     MT-NOTE: sge_host_new_addr() is not MT safe as it uses MT unsafe 
*     MT-NOTE: sge_host_create()
*
*  RESULT
*     host* - host entry
******************************************************************************/
#ifdef ENABLE_NGC
#else
host *sge_host_new_addr(const struct in_addr *addr) 
{
   host *new = sge_host_create();
   struct hostent *he;

   if (!new)
      return NULL;

   he = sge_gethostbyaddr(addr);
   
   if (!he) {
      sge_host_delete(new);
      return NULL;
   }

   if (sge_host_copy_entry(&new->he, he)) {
      sge_host_delete(new);
      return NULL;
   }

   return new;
}
#endif

/****** uti/hostname/sge_host_new_name() **************************************
*  NAME
*     sge_host_new_name() -- Create new host from hostname 
*
*  SYNOPSIS
*     host* sge_host_new_name(const char *name, int *not_really_new) 
*
*  FUNCTION
*     Create new host from hostname. Be careful with resolving 
*     anomalies. *not_really_new may be set to 1 if host was already
*     known and sge_host_search() found it. 
*
*  INPUTS
*     const char *name    - hostname
*     int *not_really_new - 
*
*  NOTES
*     MT-NOTE: sge_host_new_name() is not MT safe as it uses sge_host_copy_entry()
*
*  RESULT
*     host* - host entry
******************************************************************************/
#ifdef ENABLE_NGC
#else
host *sge_host_new_name(const char *name, int *not_really_new) 
{
   host *new, *h;
   struct hostent *he;
   char **cpp;

   DENTER(TOP_LAYER, "sge_host_new_name");

   he = sge_gethostbyname(name);
   if (!he) {
      DEXIT;
      return NULL;
   } 

   /* use address to make sure we are unique */

   for (cpp = he->h_addr_list; *cpp; cpp++) {
      if ((h = sge_host_search(NULL, *cpp))) {
         /* make a new host entry, but host is already known */
         if (not_really_new)
            *not_really_new = 1;
         DEXIT;
         return h;
      }
   }

   if (!(new = sge_host_create())) {
      DEXIT;
      return NULL;
   }

   if (sge_host_copy_entry(&new->he, he)) {
      sge_host_delete(new);
      DEXIT;
      return NULL;
   }

   DPRINTF(("adding host object %s\n", name));
   DEXIT;
   return new;
}
#endif

/****** sge_hostname/sge_host_search_pred_alias() ******************************
*  NAME
*     sge_host_search_pred_alias() -- search host who's aliasptr points to host 
*
*  SYNOPSIS
*     static host* sge_host_search_pred_alias(host *h) 
*
*  INPUTS
*     host *h - host we search for
*
*  RESULT
*     static host* - found host if contained in host list
*
*  NOTES
*     MT-NOTE: sge_host_search_pred_alias() is not MT safe due to access to 
*     MT-NOTE: global variable
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static host *sge_host_search_pred_alias(host *h) 
{
   host *hl = hostlist;

   while (hl && hl->alias != h)
      hl = hl->next;

   return hl;
}

/****** sge_hostname/sge_host_alias() ******************************************
*  NAME
*     sge_host_alias() -- alias two hosts
*
*  SYNOPSIS
*     static int sge_host_alias(host *h1, host *h2) 
*
*  FUNCTION
*     Alias 2 hostnames. This is neccesary for hosts with more than one  
*     interface and if we cant use DNS to specify more interfaces for one
*     host. The 2 hosts have to be created before we call this function.
*
*  INPUTS
*     host *h1 - first host 
*     host *h2 - second host 
*
*  RESULT
*     static int - always 0 ?? 
*
*  NOTES
*     MT-NOTE: sge_host_alias() is not MT safe as it uses unsafe 
*     MT-NOTE: sge_host_search_pred_alias()
*
*******************************************************************************/
#ifdef ENABLE_NGC
#else
static int sge_host_alias(host *h1, host *h2) 
{
   host *h3;

   while (h1->alias) {          /* end chain for host1 */
      if (h1 == h2) {
         return 0;              /* already aliased */
      }
      h1 = h1->alias;
   }

   while ((h3 = sge_host_search_pred_alias(h2)))         /* start of chain for host2 */
      h2 = h3;

   h1->alias = h2;              /* chain */

   return 0;
}
#endif

/****** sge_hostname/matches_name() ********************************************
*  NAME
*     matches_name() -- ??? 
*
*  SYNOPSIS
*     static int matches_name(struct hostent *he, const char *name) 
*
*  INPUTS
*     struct hostent *he - ???
*     const char *name   - ??? 
*
*  RESULT
*     static int - ???
*
*  NOTES
*     MT-NOTE: matches_name() is MT safe
*******************************************************************************/
static int matches_name(struct hostent *he, const char *name)    
{
   if (!name)
      return 1;
   if (!strcasecmp(name, he->h_name))
      return 1;
   if (sge_stracasecmp(name, he->h_aliases))
      return 1;
   return 0;
}

/****** sge_hostname/matches_addr() ********************************************
*  NAME
*     matches_addr() -- ??? 
*
*  SYNOPSIS
*     static int matches_addr(hostent *he, char *addr) 
*
*  INPUTS
*     struct hostent *he - ??? 
*     char *addr  - ??? 
*
*  NOTES
*     MT-NOTE: matches_addr() is MT safe
*******************************************************************************/
static int matches_addr(struct hostent *he, char *addr) 
{
   if (!addr) {
      return 1;
   }
   if (sge_stramemncpy(addr, he->h_addr_list, he->h_length)) {
      return 1;
   }
   return 0;
}

/****** uti/hostname/sge_host_search() ****************************************
*  NAME
*     sge_host_search() -- Search for host 
*
*  SYNOPSIS
*     host* sge_host_search(const char *name, char *addr) 
*
*  FUNCTION
*     Search for host. If 'name' is not NULL the returned host has 
*     the given or the alias that matches this name. 'name' is
*     not case sensitive. 'addr' is in hostorder.
*     If 'addr' is not NULL the returned host has 'addr' in his
*     addrlist. If 'addr' is aliased this function returns the
*     hostname of the first alias (mainname) 
*
*  INPUTS
*     const char *name - hostname 
*     char *addr       - address 
*
*  NOTES
*     MT-NOTE: sge_host_search() is not MT safe due to access to hostlist 
*     MT-NOTE: global variable
*
*  RESULT
*     host* - host entry
******************************************************************************/
host *sge_host_search(const char *name, char *addr) 
{
   host *hl = hostlist, *h1;
   struct hostent *he;

   while (hl) {
      he = &hl->he;

      if (((name && !strcasecmp(hl->mainname, name)) || 
            matches_name(he, name)) && matches_addr(he, addr)) {
         while ((h1 = sge_host_search_pred_alias(hl)))   /* start of alias chain */
            hl = h1;
         return hl;
      }
      hl = hl->next;
   }

   return NULL;
}

/****** uti/hostname/sge_host_print() *****************************************
*  NAME
*     sge_host_print() -- Print host entry info file 
*
*  SYNOPSIS
*     void sge_host_print(host *h, FILE *fp) 
*
*  FUNCTION
*     Print host entry info file  
*
*  INPUTS
*     host *h  - host entry 
*     FILE *fp - file 
*
*  NOTES
*     MT-NOTE: sge_host_print() is NOT MT safe ( because of inet_ntoa() call)
******************************************************************************/
void sge_host_print(host *h, FILE *fp) 
{
   struct hostent *he;
   char **cpp;

   he = &h->he;
   fprintf(fp, "h_name: %s\n", he->h_name);
   fprintf(fp, "mainname: %s\n", h->mainname);
   fprintf(fp, "h_aliases:\n");
   for (cpp = he->h_aliases; *cpp; cpp++) {
      fprintf(fp, "  %s\n", *cpp);
   }
   fprintf(fp, "h_addrtype: %d\n", he->h_addrtype);
   fprintf(fp, "h_length: %d\n", he->h_length);
   fprintf(fp, "h_addr_list:\n");
   for (cpp = he->h_addr_list; *cpp; cpp++) {
      fprintf(fp, "  %s\n", inet_ntoa(*(struct in_addr *) *cpp)); /* inet_ntoa() is not MT save */
   }
   if (h->alias) {
      fprintf(fp, "aliased to %s\n", h->alias->he.h_name);
   }
}

/****** uti/hostname/sge_host_list_print() ************************************
*  NAME
*     sge_host_list_print() -- Print hostlist into file 
*
*  SYNOPSIS
*     void sge_host_list_print(FILE *fp) 
*
*  FUNCTION
*     Print hostlist into file 
*
*  INPUTS
*     FILE *fp - filename 
*  
*  NOTES
*     MT-NOTE: sge_host_list_print() is not MT safe due to access to hostlist 
*     MT-NOTE: global variable
******************************************************************************/
void sge_host_list_print(FILE *fp) 
{
   host *hl = hostlist;

   while (hl) {
      sge_host_print(hl, fp);
      hl = hl->next;
      if (hl)
         fprintf(fp, "\n");
   }
}

/****** sge_hostname/sge_host_copy_entry() *************************************
*  NAME
*     sge_host_copy_entry() -- ??? 
*
*  SYNOPSIS
*     static int sge_host_copy_entry(hostent *heto, hostent *hefrom) 
*
*  INPUTS
*     struct hostent *heto   - ??? 
*     struct hostent *hefrom - ??? 
*
*  RESULT
*     static int - ???
*
*  NOTES
*     MT-NOTE: sge_host_copy_entry() is MT safe
*******************************************************************************/
#ifdef ENABLE_NGC
#else
static int sge_host_copy_entry(struct hostent *heto, struct hostent *hefrom) 
{

   if (heto->h_name)
      free((char *) heto->h_name);
   sge_strafree(heto->h_aliases);
   sge_strafree(heto->h_addr_list);

   memcpy(heto, hefrom, sizeof(struct hostent));
   heto->h_name = strdup((char *) hefrom->h_name);
   heto->h_aliases = sge_stradup(hefrom->h_aliases, 0);
   heto->h_addr_list = sge_stradup(hefrom->h_addr_list, hefrom->h_length);

   if (!heto->h_name || !heto->h_aliases || !heto->h_addr_list)
      return -1;

   return 0;
}
#endif

/****** uti/hostname/sge_host_list_read_aliasfile() ***************************
*  NAME
*     sge_host_list_read_aliasfile() -- Read hostname aliasfile 
*
*  SYNOPSIS
*     int sge_host_list_read_aliasfile(char *fname) 
*
*  FUNCTION
*     Read hostname aliasfile which has following format:
*
*        hostname alias alias ...
*        ... 
*
*  INPUTS
*     char *fname - filename 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error
* 
*  NOTES:
*     MT-NOTE: sge_host_list_read_aliasfile() is not MT safe 
*     MT-NOTE: due to strtok(), hostlist global variable access 
******************************************************************************/
#ifdef ENABLE_NGC
#else
int sge_host_list_read_aliasfile(char *fname) 
{
#ifdef __INSIGHT__   
/* JG: really not a memory leak! */
_Insight_set_option("suppress", "LEAK_SCOPE");
#endif
   FILE *fp;
   char *mainname, *name, buf[10000];
   host *h1, *h2;
   SGE_STRUCT_STAT sb;

   DENTER(TOP_LAYER, "sge_host_list_read_aliasfile");

   if (!fname) {                 /* saves work if aliasfile not specified */
      DEXIT;
      return 0;
   }

   if (SGE_STAT(fname, &sb)) {
      DEXIT;
      return -1;      
   }

   fp = fopen(fname, "r");
   if (!fp) {
      DEXIT;
      return -1;
   }

   while (fgets(buf, sizeof(buf), fp)) {
      if (buf[0] == '#')
         continue;

      mainname = strtok(buf, ALIAS_DELIMITER);
      if (!mainname)
         continue;

      if (!(h1 = sge_host_search(mainname, NULL))) {
         if (!(h1 = sge_host_new_name(mainname, NULL))) {
            ERROR((SGE_EVENT, MSG_SYSTEM_BADMAINNAME_SS, mainname, fname));
            continue; /* skip this line */
         }
      }

      /* Insert main hostname */
      strcpy(h1->mainname, mainname);

      /* iterate on aliases */
      while ((name = strtok(NULL, ALIAS_DELIMITER))) {

         if (!(h2 = sge_host_search(name, NULL))) {
            if (!(h2 = sge_host_new_name(name, NULL))) {
               ERROR((SGE_EVENT, MSG_SYSTEM_BADALIASNAME_SS, name, fname));
               continue; /* skip this alias name */
            }
         }

         if (h1 != h2) {                   /* be sure to not alias to itself 
                                           could happen if 2 aliases resolve to
                                           the same */
            sge_host_alias(h1, h2);
            strcpy(h2->mainname, mainname);
         }
      }
   }

   fclose(fp);
   DEXIT;
   return 0;
#ifdef __INSIGHT__   
_Insight_set_option("suppress", "LEAK_SCOPE");
#endif
}
#endif

/****** uti/hostname/sge_host_get_aliased_name() ******************************
*  NAME
*     sge_host_get_aliased_name() -- Return mainname 
*
*  SYNOPSIS
*     const char* sge_host_get_aliased_name(const char *name) 
*
*  FUNCTION
*     Return mainname to be used. Add host to internal host list 
*     if necessary. 
*
*  INPUTS
*     const char *name - hostname 
*
*  RESULT
*     const char* - mainname
* 
*  NOTES:
*     MT-NOTE: sge_host_get_aliased_name() is not MT safe
******************************************************************************/
#ifdef ENABLE_NGC
#else
const char *sge_host_get_aliased_name(const char *name)
{
   host *h = sge_host_search(name, NULL);

   if (!h) {
      h = sge_host_new_name(name, NULL);
   }
   return h?sge_host_get_mainname(h):NULL;
}
#endif

void sge_free_hostent( struct hostent** he_to_del ) {
   struct hostent *he = NULL; 
   char** help = NULL;

   /* nothing to free */
   if (he_to_del == NULL) {
      return;
   }

   /* pointer points to NULL, nothing to free */
   if (*he_to_del == NULL) {
      return;
   }

   he = *he_to_del;

   /* free unique host name */
   free(he->h_name);
   he->h_name = NULL;

   /* free host aliases */  
   if (he->h_aliases != NULL) {
      help = he->h_aliases;
      while(*help) {
         free(*help++);
      }
      free(he->h_aliases);
   }
   he->h_aliases = NULL;

   /* free addr list */
   if (he->h_addr_list != NULL) {
      help = he->h_addr_list;
      while(*help) {
         free(*help++);
      }
      free(he->h_addr_list);
   }
   he->h_addr_list = NULL;

   /* free hostent struct */
   free(*he_to_del);
   *he_to_del = NULL;
}


/****** uti/hostname/sge_host_list_refresh() **********************************
*  NAME
*     sge_host_list_refresh() -- Refresh hostlist 
*
*  SYNOPSIS
*     void sge_host_list_refresh(void) 
*
*  FUNCTION
*     This function looks for changes in the resolve tables. 
* 
*  NOTES:
*     MT-NOTE: sge_host_list_refresh() is not MT safe
******************************************************************************/
#ifdef ENABLE_NGC
#else
void sge_host_list_refresh(void)
{
   host *hl = hostlist;
   struct hostent *he;

   DENTER(TOP_LAYER, "sge_host_list_refresh");

   while (hl) {
      he = sge_gethostbyname(hl->he.h_name);
      if (!he) {
         hl->deleted = 1;
      } else
         sge_host_copy_entry(&hl->he, he);
      hl = hl->next;
   }

   DEXIT;
   return;
}
#endif

/****** uti/hostname/sge_host_get_mainname() **********************************
*  NAME
*     sge_host_get_mainname() -- Return mainname considering aliases 
*
*  SYNOPSIS
*     char* sge_host_get_mainname(host *h) 
*
*  FUNCTION
*     Return the mainname of a host considering aliases. 
*
*  INPUTS
*     host *h - host
*
*  RESULT
*     char* - mainname 
*
*  NOTES:
*     MT-NOTE: sge_host_get_mainname() is not MT safe
******************************************************************************/
char *sge_host_get_mainname(host *h) 
{
   host *h1;

   h1 = h;
   while ((h = sge_host_search_pred_alias(h)))   /* search start of alias chain */
      h1 = h;

   if (h1->mainname[0])
      return h1->mainname;

   return (char *) h1->he.h_name;
}

/****** uti/hostname/sge_host_resolve_name_local() ****************************
*  NAME
*     sge_host_resolve_name_local() -- Resolve host without commd 
*
*  SYNOPSIS
*     const char* sge_host_resolve_name_local(const char *unresolved) 
*
*  FUNCTION
*     Used by qmaster, shadowd and some utilities in cases when 
*     commd resolving is needed to comply with SGE/EE host aliasing 
*     but not available.
*
*  INPUTS
*     const char *unresolved - hostname 
*
*  RESULT
*     const char* - resolved hostname 
*
*  BUGS
*     As it uses gethostbyname() it overwrites the buffer 
*     used by this function to return struct hostent *he !
*
*  NOTES:
*     MT-NOTE: sge_host_resolve_name_local() is not MT safe
******************************************************************************/
#ifdef ENABLE_NGC
#else
const char *sge_host_resolve_name_local(const char *unresolved)
{  
   const char *s;
   char *apath;
   static int read_file = 0;

   DENTER(TOP_LAYER, "sge_host_resolve_name_local");

   if (!read_file) {
      apath = sge_get_alias_path();
      sge_host_list_read_aliasfile(apath);
      free(apath);
      read_file = 1;
   }

   s = sge_host_get_aliased_name(unresolved);
   if (s) {
      DPRINTF(("%s as aliased from %s\n", s, unresolved));
   } else {
      DPRINTF(("no aliased name from %s\n", unresolved));
   }

   DEXIT;
   return s;
}
#endif


/****** uti/hostname/sge_hostcpy() ********************************************
*  NAME
*     sge_hostcpy() -- strcpy() for hostnames.
*
*  SYNOPSIS
*     void sge_hostcpy(char *dst, const char *raw)
*
*  FUNCTION
*     strcpy() for hostnames. Honours some configuration values:
*        - Domain name may be ignored
*        - Domain name may be replaced by a 'default domain'
*        - Hostnames may be used as they are.
*
*  INPUTS
*     char *dst       - possibly modified hostname
*     const char *raw - hostname
*
*  SEE ALSO
*     uti/hostname/sge_hostcmp()
*
*  NOTES:
*     MT-NOTE: sge_hostcpy() is MT safe
******************************************************************************/
void sge_hostcpy(char *dst, const char *raw)
{
   char *s;
 
   if (bootstrap_get_ignore_fqdn()) {
 
      /* standard: simply ignore FQDN */
 
      strncpy(dst, raw, MAXHOSTLEN);
      if ((s = strchr(dst, '.'))) {
         *s = '\0';
      }
   } else if (bootstrap_get_default_domain() != NULL && 
              SGE_STRCASECMP(bootstrap_get_default_domain(), "none") != 0) {
 
      /* exotic: honor FQDN but use default_domain */
 
      if (!strchr(raw, '.')) {
         strncpy(dst, raw, MAXHOSTLEN);
         strncat(dst, ".", MAXHOSTLEN);
         strncat(dst, bootstrap_get_default_domain(), MAXHOSTLEN);
      } else {
         strncpy(dst, raw, MAXHOSTLEN);
      }
   } else {
 
      /* hardcore: honor FQDN, don't use default_domain */
 
      strncpy(dst, raw, MAXHOSTLEN);
   }
   return;
}  

/****** uti/hostname/sge_hostcmp() ********************************************
*  NAME
*     sge_hostcmp() -- strcmp() for hostnames
*
*  SYNOPSIS
*     int sge_hostcmp(const char *h1, const char*h2)
*
*  FUNCTION
*     strcmp() for hostnames. Honours some configuration values:
*        - Domain name may be ignored
*        - Domain name may be replaced by a 'default domain'
*        - Hostnames may be used as they are.
*
*  INPUTS
*     const char *h1 - 1st hostname
*     const char *h2 - 2nd hostname
*
*  RESULT
*     int - 0, 1 or -1
*
*  SEE ALSO
*     uti/hostname/sge_hostcpy()
*
*  NOTES:
*     MT-NOTE: sge_hostcmp() is MT safe
******************************************************************************/
int sge_hostcmp(const char *h1, const char*h2)
{
   int cmp = -1;
   char h1_cpy[MAXHOSTLEN+1], h2_cpy[MAXHOSTLEN+1];
 
 
   DENTER(BASIS_LAYER, "sge_hostcmp");
 
   sge_hostcpy(h1_cpy,h1);
   sge_hostcpy(h2_cpy,h2);
 
   if (h1_cpy && h2_cpy) {
     cmp = SGE_STRCASECMP(h1_cpy, h2_cpy);
 
     DPRINTF(("sge_hostcmp(%s, %s) = %d\n", h1_cpy, h2_cpy));
   }
 
   DEXIT;
   return cmp;
}

/****** uti/hostname/sge_is_hgroup_ref() **************************************
*  NAME
*     sge_is_hgroup_ref() -- Is string a valid hgroup name 
*
*  SYNOPSIS
*     bool sge_is_hgroup_ref(const char *string) 
*
*  FUNCTION
*     Is string a valid hgroup name 
*
*  INPUTS
*     const char *string - hostname or hostgroup name 
*
*  RESULT
*     bool - Result
*        true  - hostgroup
*        false - no hostgroup (hostname)
*******************************************************************************/
bool sge_is_hgroup_ref(const char *string)
{
   bool ret = false;

   if (string != NULL) {
      ret = (string[0] == HOSTGROUP_INITIAL_CHAR);
   }
   return ret;
}

