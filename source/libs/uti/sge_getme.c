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
#include <string.h>
#include <stdlib.h>

#ifndef WIN32NATIVE
#  include <pwd.h>
#  include <unistd.h>
#  include <sys/socket.h>
#  include <netdb.h>
#else /* WIN32NATIVE */
#  include "utility.h"
#  include <winsock2.h>
#endif /* WIN32NATIVE */

#include "sge.h"
#include "sge_me.h"
#include "def.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sge_getme.h"
#include "sge_prognames.h"
#include "sge_arch.h"

sge_me_type me;

#if defined(SOLARIS)
int gethostname(char *name, int namelen);
#endif

#ifdef WIN32NATIVE
   static int first = TRUE;
#endif /* WIN32NATIVE */

static void sge_show_me(void);

/*-----------------------------------------------------------------------
 * Name:  sge_getme
 * Descr: sets me-struct
 *        if called second time return immediately
 *-----------------------------------------------------------------------*/
void sge_getme(
u_long32 program_number         /* internal program number */
) {
#ifdef WIN32NATIVE
   char *envs;	
#endif /* WIN32NATIVE */
   const char *cp;
   stringT tmp_str;

#ifndef WIN32
   struct passwd *paswd;
#endif

   struct hostent *hent, *hent2;

#ifndef WIN32NATIVE
   static int first = TRUE;
#endif /* WIN32NATIVE */

   DENTER(TOP_LAYER, "sge_getme");

   DTRACE;

   if (first) {
      memset(&me, 0, sizeof(me));
      first = FALSE;
   }
   else {
      DEXIT;
      return;
   }

   /* get program info */
   me.who = program_number;
   me.sge_formal_prog_name = sge_strdup(me.sge_formal_prog_name,
                                        prognames[me.who]);

   /* Fetch hostnames */
   SGE_ASSERT((gethostname(tmp_str, sizeof(tmp_str)) == 0));
   SGE_ASSERT(((hent = gethostbyname(tmp_str)) != NULL));

   me.qualified_hostname = strdup(hent->h_name);
   me.unqualified_hostname = sge_dirname(me.qualified_hostname, '.');

   /* Bad resolving in some networks leads to short qualified host names */
   if (!strcmp(me.qualified_hostname, me.unqualified_hostname)) {
      char tmp_addr[8];

      memcpy(tmp_addr, hent->h_addr, hent->h_length);

      SGE_ASSERT(((hent2 = gethostbyaddr(tmp_addr, hent->h_length, AF_INET)) != NULL));

      FREE(me.qualified_hostname);
      FREE(me.unqualified_hostname);

      me.qualified_hostname = strdup(hent->h_name);
      me.unqualified_hostname = sge_dirname(me.qualified_hostname, '.');
   }


   /* SETPGRP; */

#ifndef WIN32NATIVE
   me.uid = getuid();
   me.gid = getgid();
#else
   /* TODO: Errorhandling !! id = 0 when envs not set */
   if(envs=getenv("UID")) {
	   me.uid = atol(envs);
   }
   if(envs=getenv("GID")) {
	   me.gid = atol(envs);
   }
#endif

#ifdef WIN32 /* getpwuid not called */
   me.user_name = sge_strdup(NULL, sge_getenv("USERNAME"));
#else
   SGE_ASSERT(((paswd = (struct passwd *) getpwuid(me.uid)) != NULL));
   me.user_name = sge_strdup(me.user_name, paswd->pw_name);
#endif

   cp = sge_default_cell();
   me.default_cell = sge_strdup(me.default_cell, cp);

   sge_show_me();

   DEXIT;
   return;
}

#ifdef WIN32NATIVE

void sge_deleteme()
{
	FREE(me.sge_formal_prog_name);
	FREE(me.qualified_hostname);
	FREE(me.unqualified_hostname);
	FREE(me.user_name);
	FREE(me.default_cell);

	first = TRUE;
}

#endif

/*******************************************************************/
static void sge_show_me()
{
   DENTER(TOP_LAYER, "sge_show_me");

#ifdef NO_SGE_COMPILE_DEBUG
   return;
#else

#ifndef WIN32NATIVE
   if (!TRACEON) {
      DEXIT;
      return;
   }
#endif

#endif

   DPRINTF(("me.who                      >%d<\n", (int) me.who));
   DPRINTF(("me.sge_formal_prog_name     >%s<\n", me.sge_formal_prog_name));
   DPRINTF(("me.qualified_hostname       >%s<\n", me.qualified_hostname));
   DPRINTF(("me.unqualified_hostname     >%s<\n", me.unqualified_hostname));
   DPRINTF(("me.uid                      >%d<\n", (int) me.uid));
   DPRINTF(("me.gid                      >%d<\n", (int) me.gid));
   DPRINTF(("me.daemonized               >%d<\n", (int) me.daemonized));
   DPRINTF(("me.user_name                >%s<\n", me.user_name));
   DPRINTF(("me.default_cell             >%s<\n", me.default_cell));

   DEXIT;
}
