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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "commd.h"
#include "commd_error.h"
#include "host.h"
#include "debug_malloc.h"
#include "sgermon.h"
#include "sge_log.h"
#include "msg_commd.h"
#include "sge_language.h"
#include "sge_stat.h" 
#include "sge_time.h" 

#include "sge_arch.h" 

#ifndef h_errno
extern int h_errno;
#endif

extern void trace(char *);

static host *hostlist = NULL;

host *localhost = NULL;

static int copy_hostent(struct hostent *heto, struct hostent *hefrom);
static int matches_addr(struct hostent *he, char *addr);
static host *search_pred_alias(host *h);
void host_initialize(void);

/* cpp functions */
static char **copy_cpp(char **cpp, int n);
static void free_cpp(char **cpp);
static char **cpp_casecmp(const char *cp, char **cpp);
static char **cpp_memncmp(const char *cp, char **cpp, int n);

/* resolver library wrappers */
static struct hostent *sge_gethostbyname(const char *name);
static struct hostent *sge_gethostbyaddr(const struct in_addr *addr);

#if defined(SOLARIS)
int gethostname(char *name, int namelen);
#endif

#define MAX_RESOLVER_BLOCKING 15

/* should use gethostbyname_r() instead */
static struct hostent *sge_gethostbyname(const char *name)
{
   struct hostent *he;
   time_t now;
   int time;

   DENTER(TOP_LAYER, "sge_gethostbyname");

   now = sge_get_gmt();
   he = gethostbyname(name);
   time = sge_get_gmt() - now;

   /* warn about blocking gethostbyname() calls */
   if (time > MAX_RESOLVER_BLOCKING) {
      WARNING((SGE_EVENT, "gethostbyname(%s) took %d seconds and returns %s\n", 
            name, time, he?"success":
          (h_errno == HOST_NOT_FOUND)?"HOST_NOT_FOUND":
          (h_errno == TRY_AGAIN)?"TRY_AGAIN":
          (h_errno == NO_RECOVERY)?"NO_RECOVERY":
          (h_errno == NO_DATA)?"NO_DATA":
          (h_errno == NO_ADDRESS)?"NO_ADDRESS":"<unknown error>"));
   }

   DEXIT;
   return he;
}

/* should use gethostbyaddr_r() instead */
static struct hostent *sge_gethostbyaddr(const struct in_addr *addr)
{
   struct hostent *he;
   time_t now;
   int time;

   DENTER(TOP_LAYER, "sge_gethostbyaddr");

   now = sge_get_gmt();
#if defined(CRAY)
   he = gethostbyaddr((const char *)addr, sizeof(struct in_addr), AF_INET);
#else   
   he = gethostbyaddr((const char *)addr, 4, AF_INET);
#endif
   time = sge_get_gmt() - now;

   /* warn about blocking gethostbyaddr() calls */
   if (time > MAX_RESOLVER_BLOCKING) {
      WARNING((SGE_EVENT, "gethostbyaddr(%s) took %d seconds and returns %s\n", 
            inet_ntoa(*addr), time, he?"success":
          (h_errno == HOST_NOT_FOUND)?"HOST_NOT_FOUND":
          (h_errno == TRY_AGAIN)?"TRY_AGAIN":
          (h_errno == NO_RECOVERY)?"NO_RECOVERY":
          (h_errno == NO_DATA)?"NO_DATA":
          (h_errno == NO_ADDRESS)?"NO_ADDRESS":"<unknown error>"));
   }

   DEXIT;
   return he;
}

/**********************************************************************/
void host_initialize()
{
   char localname[MAXHOSTLEN];

   DENTER(TOP_LAYER, "host_initialize");

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
   localhost = newhost_name(localname, NULL);
   if (!localhost) {
      ERROR((SGE_EVENT, MSG_NET_RESOLVINGLOCALHOSTFAILED ));
      DEXIT;
      return;
   }
   DEXIT;
}

/**********************************************************************/
host *create_host()
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

/********************************/
/* delete host with all aliases */
/********************************/
void delete_host(
host *h 
) {
   host *last = NULL, *hl = hostlist;
   host *predalias, *nextalias;

   if (!h)
      return;

   predalias = search_pred_alias(h);
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
   free_cpp(h->he.h_aliases);
   free_cpp(h->he.h_addr_list);
   free(h);

   delete_host(predalias);
   delete_host(nextalias);
}

/**********************************************************************/
/* create new host from internet address in hostorder */
host *newhost_addr( const struct in_addr *addr) 
{
   host *new = create_host();
   struct hostent *he;

   if (!new)
      return NULL;

   he = sge_gethostbyaddr(addr);
   
   if (!he) {
      delete_host(new);
      return NULL;
   }

   if (copy_hostent(&new->he, he)) {
      delete_host(new);
      return NULL;
   }

   return new;
}

/*************************************************/
/* create new host from hostname                 */
/*   be careful with resolving anomalies.        */
/*************************************************/
host *newhost_name(
const char *name,
int *not_really_new 
) {
   host *new, *h;
   struct hostent *he;
   char **cpp;

   DENTER(TOP_LAYER, "newhost_name");

   he = sge_gethostbyname(name);
   if (!he) {
      DEXIT;
      return NULL;
   } 

   /* use address to make sure we are unique */

   for (cpp = he->h_addr_list; *cpp; cpp++) {
      if ((h = search_host(NULL, *cpp))) {
         /* make a new host entry, but host is already known */
         if (not_really_new)
            *not_really_new = 1;
         DEXIT;
         return h;
      }
   }

   if (!(new = create_host())) {
      DEXIT;
      return NULL;
   }

   if (copy_hostent(&new->he, he)) {
      delete_host(new);
      DEXIT;
      return NULL;
   }

   DPRINTF(("adding host object %s\n", name));
   DEXIT;
   return new;
}

/*********************************************/
/* search host who's aliasptr points to host */
/*********************************************/
static host *search_pred_alias(
host *h 
) {
   host *hl = hostlist;

   while (hl && hl->alias != h)
      hl = hl->next;

   return hl;
}

/***********************************************************************/
/* Alias 2 hostnames. This is neccesary for hosts with more than one   */
/* interface and if we cant use DNS to specify more interfaces for one */
/* host. The 2 hosts have to be created before we call this function.  */
/***********************************************************************/
int alias_host(
host *h1,
host *h2 
) {
   host *h3;

   while (h1->alias) {          /* end chain for host1 */
      if (h1 == h2)
         return 0;              /* already aliased */
      h1 = h1->alias;
   }

   while ((h3 = search_pred_alias(h2)))         /* start of chain for host2 */
      h2 = h3;

   h1->alias = h2;              /* chain */

   return 0;
}

/**********************************************************************/
int alias_hoststr(
char *host1,
char *host2 
) {
   host *h1, *h2;

   h1 = search_host(host1, NULL);
   h2 = search_host(host2, NULL);
   if (!h1 || !h2)
      return -1;

   return alias_host(h1, h2);
}

/**********************************************************************/
int matches_name(
struct hostent *he,
const char *name 
) {
   if (!name)
      return 1;
   if (!strcasecmp(name, he->h_name))
      return 1;
   if (cpp_casecmp(name, he->h_aliases))
      return 1;
   return 0;
}

/**********************************************************************/
static int matches_addr(
struct hostent *he,
char *addr 
) {
   if (!addr)
      return 1;
   if (cpp_memncmp(addr, he->h_addr_list, he->h_length))
      return 1;
   return 0;
}

/************************************************************************/
/* search for a host                                                    */
/* If name is not NULL the returned host has the given name or an alias */
/* that matches this name. Name is not case sensitive.                  */
/* addr is in hostorder                                                 */
/* If addr is not NULL the returned host has addr in his addrlist.      */
/* If addr is aliased return hostname of first alias (main name)        */
/************************************************************************/
host *search_host(
const char *name,
char *addr 
) {
   host *hl = hostlist, *h1;
   struct hostent *he;

   while (hl) {
      he = &hl->he;

      if (((name && !strcasecmp(hl->mainname, name)) || 
            matches_name(he, name)) && matches_addr(he, addr)) {
         while ((h1 = search_pred_alias(hl)))   /* start of alias chain */
            hl = h1;
         return hl;
      }
      hl = hl->next;
   }

   return NULL;
}

/**********************************************************************/
void print_host(
host *h,
FILE *fp 
) {
   struct hostent *he;
   char **cpp;

   he = &h->he;
   fprintf(fp, "h_name: %s\n", he->h_name);
   fprintf(fp, "mainname: %s\n", h->mainname);
   fprintf(fp, "h_aliases:\n");
   for (cpp = he->h_aliases; *cpp; cpp++)
      fprintf(fp, "  %s\n", *cpp);
   fprintf(fp, "h_addrtype: %d\n", he->h_addrtype);
   fprintf(fp, "h_length: %d\n", he->h_length);
   fprintf(fp, "h_addr_list:\n");
   for (cpp = he->h_addr_list; *cpp; cpp++) {
      fprintf(fp, "  %s\n", inet_ntoa(*(struct in_addr *) *cpp));
   }
   if (h->alias)
      fprintf(fp, "aliased to %s\n", h->alias->he.h_name);
}

/**********************************************************************/
void print_hostlist(
FILE *fp 
) {
   host *hl = hostlist;

   while (hl) {
      print_host(hl, fp);
      hl = hl->next;
      if (hl)
         fprintf(fp, "\n");
   }
}

/**********************************************************************/
static int copy_hostent(
struct hostent *heto,
struct hostent *hefrom 
) {
   if (heto->h_name)
      free((char *) heto->h_name);
   free_cpp(heto->h_aliases);
   free_cpp(heto->h_addr_list);

   memcpy(heto, hefrom, sizeof(struct hostent));
   heto->h_name = strdup((char *) hefrom->h_name);
   heto->h_aliases = copy_cpp(hefrom->h_aliases, 0);
   heto->h_addr_list = copy_cpp(hefrom->h_addr_list, hefrom->h_length);

   if (!heto->h_name || !heto->h_aliases || !heto->h_addr_list)
      return -1;

   return 0;
}

/****************************************************************************/
/* Copy list of character pointers including the strings these pointers     */
/* refer to. If n==0 strings are '\0'-delimited, if n!=0 we use n as length */
/* of the strings                                                           */
/****************************************************************************/
static char **copy_cpp(
char **cpp,
int n 
) {
   int count = 0, len;
   char **cpp1, **cpp2, **cpp3;

   /* first count entries */
   cpp2 = cpp;
   while (*cpp2) {
      cpp2++;
      count++;
   }

   /* alloc space */
   cpp1 = (char **) malloc((count + 1) * sizeof(char **));
   if (!cpp1)
      return NULL;

   /* copy  */
   cpp2 = cpp;
   cpp3 = cpp1;
   while (*cpp2) {
      /* alloc space */
      if (n)
         len = n;
      else
         len = strlen(*cpp2) + 1;

      *cpp3 = (char *) malloc(len);
      if (!(*cpp3)) {
         while ((--cpp3) >= cpp1)
            free(*cpp3);
         free(cpp1);
         return NULL;
      }

      /* copy string */
      memcpy(*cpp3, *cpp2, len);
      cpp3++;
      cpp2++;
   }

   *cpp3 = NULL;

   return cpp1;
}

/**********************************************************************/
/* Compare string with string field and return the pointer to the     */
/* matched character pointer. Case sensitive.                         */
/**********************************************************************/
static char **cpp_casecmp(
const char *cp,
char **cpp 
) {
   while (*cpp) {
      if (!strcasecmp(*cpp, cp))
         return cpp;
      cpp++;
   }
   return NULL;
}

/************************************************************************/
/* Compare string with string field and return the pointer to the       */
/* matched character pointer. Case insensitive. Compare exactly n chars */
/************************************************************************/
static char **cpp_memncmp(
const char *cp,
char **cpp,
int n 
) {
   while (*cpp) {
      if (!memcmp(*cpp, cp, n))
         return cpp;
      cpp++;
   }
   return NULL;
}

/***********************************/
/* free list of character pointers */
/***********************************/
static void free_cpp(
char **cpp 
) {
   char **cpp1 = cpp;
   if (!cpp)
      return;

   while (*cpp1)
      free(*cpp1++);
   free(cpp);
}

/********************************************/
/* read an aliasfile:                       */
/*                                          */
/* hostname alias alias ...                 */
/* ...                                      */
/********************************************/
#define ALIAS_DELIMITER "\n\t ,;"

int read_aliasfile(
char *fname 
) {
   FILE *fp;
   char *mainname, *name, buf[10000];
   host *h1, *h2;
   SGE_STRUCT_STAT sb;

   DENTER(TOP_LAYER, "read_aliasfile");

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

      h1 = search_host(mainname, NULL);
      if (!h1)
         h1 = newhost_name(mainname, NULL);
      if (!h1) {
         fclose(fp);
         DEXIT;
         return -1;             /* main hostname could not be resolved */
      }

      /* Insert main hostname */
      strcpy(h1->mainname, mainname);

      /* iterate on aliases */
      while ((name = strtok(NULL, ALIAS_DELIMITER))) {

         h2 = search_host(name, NULL);
         if (!h2)
            h2 = newhost_name(name, NULL);

         if (h2 && (h1 != h2)) {        /* be sure to not alias to itself 
                                           could happen if 2 aliases resolve to
                                           the same */
            alias_host(h1, h2);
            strcpy(h2->mainname, mainname);
         }
      }
   }

   fclose(fp);
   DEXIT;
   return 0;
}

/*******************************************************
 return mainname to be used 
 add host to internal host list if necessary
 *******************************************************/
const char *get_aliased_name(const char *name)
{
   host *h = search_host(name, NULL);
   if (!h)
      h = newhost_name(name, NULL);
   return h?get_mainname(h):NULL;
}


/*******************************************************
 refresh hostlist. This function looks for changes 
 in the resolve tables
 *******************************************************/
void refresh_hostlist()
{
   host *hl = hostlist;
   struct hostent *he;

   while (hl) {
      he = sge_gethostbyname(hl->he.h_name);
      if (!he) {
         hl->deleted = 1;
         continue;
      }

      copy_hostent(&hl->he, he);
      hl = hl->next;
   }
}

/*****************************************************/
/* return the mainname of a host considering aliases */
/*****************************************************/
char *get_mainname(
host *h 
) {
   host *h1;

   h1 = h;
   while ((h = search_pred_alias(h)))   /* search start of alias chain */
      h1 = h;

   if (h1->mainname[0])
      return h1->mainname;

   return (char *) h1->he.h_name;
}

/*****************************************************************************/
/* used by qmaster, shadowd and some utilities in cases when commd resolving */
/* is needed to comply with SGE host aliasing but not available              */
/*****************************************************************************/
const char *resolve_hostname_local(const char *unresolved)
{  
   const char *s;
   char *apath;
   apath = get_alias_path();
   read_aliasfile(apath);
   free(apath);
   s = get_aliased_name(unresolved);
   if (s)
     DPRINTF(("%s as aliased from %s\n", s, unresolved));
   else
     DPRINTF(("no aliased name from %s\n", unresolved));

   return s;
}
