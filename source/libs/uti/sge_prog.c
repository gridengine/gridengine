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

#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>  

#include "sge.h"
#include "sgermon.h"
#include "sge_prog.h"
#include "sge_log.h"
#include "sge_stdlib.h"
#include "sge_string.h"
#include "sge_unistd.h"

/* Must match Qxxx defines in sge_prog.h */
const char *prognames[] =
{
   "unknown"	,
   "qalter"		,       /* 1   */
   "qconf"		,       /* 2   */
   "qdel"		,       /* 3   */
   "qhold"		,       /* 4   */
   "qidle"		,       /* 5   */
   "qmaster"	,       /* 6   */
   "qmod"		,       /* 7   */
   "qmove"		,       /* 8   */
   "qmsg"		,       /* 9   */
   "qresub"    ,       /* 10  */
   "qrls"		,       /* 11  */
   "qselect"	,       /* 12  */
   "qsh"		   ,       /* 13  */
   "qrsh"		,       /* 14  */
   "qlogin"		,       /* 15  */
   "qsig"		,       /* 16  */
   "qstat"		,       /* 17  */
   "qsub"		,       /* 18  */
   "execd"		,       /* 19  */
   "qevent"    ,       /* 20  */
   "unknown"	,       /* 21  */
   "unknown"	,       /* 22  */
   "qusage"		,       /* 23  */
   "dcmd"		,       /* 24  */
   "dsh"		   ,       /* 25  */
   "qmon"		,       /* 26  */
   "schedd"		,       /* 27  */
   "qsched"		,       /* 28  */
   "qacct"		,       /* 29  */
   "qstd"		,       /* 30  */
   "commd"		,       /* 31  */
   "shadowd"	,       /* 32  */
   "yyy"	   	,       /* 33, obsolete */
   "tasker"	  	,       /* 34  */
   "qidl"		,       /* 35  */
   "unknown"	,       /* 36  */
   "qhost"		,       /* 37  */
   "commdcntl"         /* 38  */
};

typedef struct {
   char        *sge_formal_prog_name;      /* taken from prognames[] */
   char        *qualified_hostname;
   char        *unqualified_hostname;
   u_long32    who;                        /* Qxxx defines           */
   u_long32    uid;
   u_long32    gid;
   u_long32    daemonized;
   char        *user_name;
   char        *default_cell;
} sge_me_type;
 
static sge_me_type me;   

static void sge_show_me(void); 
static void uti_state_set_sge_formal_prog_name(const char *s);
static void uti_state_set_mewho(u_long32 who);
static void uti_state_set_uid(u_long32 uid);
static void uti_state_set_gid(u_long32 gid);
static void uti_state_set_user_name(const char *s);
static void uti_state_set_default_cell(const char *s);

/****** libs/uti/uti_state_get_????() ************************************
*  NAME
*     uti_state_get_????() - read access to utilib global variables
*
*  FUNCTION
*     Provides access to either global variable or per thread global variable.
*
******************************************************************************/
const char *uti_state_get_sge_formal_prog_name(void)
{
   return me.sge_formal_prog_name;
}

const char *uti_state_get_qualified_hostname(void)
{
   return me.qualified_hostname;
}

const char *uti_state_get_unqualified_hostname(void)
{
   return me.unqualified_hostname;
}

u_long32 uti_state_get_mewho(void)
{
   return me.who;
}

u_long32 uti_state_get_uid(void)
{
   return me.uid;
}

u_long32 uti_state_get_gid(void)
{
   return me.gid;
}

u_long32 uti_state_get_daemonized(void)
{
   return me.daemonized;
}

const char *uti_state_get_user_name(void)
{
   return me.user_name;
}

const char *uti_state_get_default_cell(void)
{
   return me.default_cell;
}

/****** libs/uti/uti_state_set_????() ************************************
*  NAME
*     uti_state_set_????() - write access to utilib global variables
*
*  FUNCTION
*     Provides access to either global variable or per thread global variable.
*
******************************************************************************/

void uti_state_set_qualified_hostname(const char *s)
{
   me.qualified_hostname = sge_strdup(me.qualified_hostname, s);
}

void uti_state_set_daemonized(u_long32 daemonized)
{
   me.daemonized = daemonized;
}

static void uti_state_set_sge_formal_prog_name(const char *s)
{
   me.sge_formal_prog_name = sge_strdup(me.sge_formal_prog_name, s);
}

static void uti_state_set_mewho(u_long32 who)
{
   me.who = who;
}

static void uti_state_set_uid(u_long32 uid)
{
   me.uid = uid;
}

static void uti_state_set_gid(u_long32 gid)
{
   me.gid = gid;
}

static void uti_state_set_user_name(const char *s)
{
   me.user_name = sge_strdup(me.user_name, s);
}

static void uti_state_set_default_cell(const char *s)
{
   me.default_cell = sge_strdup(me.default_cell, s);
}


/****** uti/prog/sge_getme() *************************************************
*  NAME
*     sge_getme() -- Initialize me-struct
*
*  SYNOPSIS
*     void sge_getme(u_long32 program_number)
*
*  FUNCTION
*     Initialize me-struct according to 'program_number'
*
*  INPUTS
*     u_long32 program_number - uniq internal program number
******************************************************************************/
void sge_getme(u_long32 program_number)
{
#ifdef WIN32NATIVE
   char *envs;
#endif /* WIN32NATIVE */
   stringT tmp_str;
 
#ifndef WIN32
   struct passwd *paswd;
#endif
 
   struct hostent *hent, *hent2;
 
#ifndef WIN32NATIVE
   static int first = TRUE;
#endif /* WIN32NATIVE */
 
   DENTER(TOP_LAYER, "sge_getme");
 
   if (first) {
      memset(&me, 0, sizeof(me));
      first = FALSE;
   }
   else {
      DEXIT;
      return;
   }
 
   /* get program info */
   uti_state_set_mewho(program_number);
   uti_state_set_sge_formal_prog_name(prognames[program_number]);

   /* Fetch hostnames */
   SGE_ASSERT((gethostname(tmp_str, sizeof(tmp_str)) == 0));
   SGE_ASSERT(((hent = gethostbyname(tmp_str)) != NULL));

   DTRACE;

   uti_state_set_qualified_hostname(hent->h_name);
   me.unqualified_hostname = sge_dirname(me.qualified_hostname, '.');

   DTRACE;
 
   /* Bad resolving in some networks leads to short qualified host names */
   if (!strcmp(me.qualified_hostname, me.unqualified_hostname)) {
      char tmp_addr[8];
 
      memcpy(tmp_addr, hent->h_addr, hent->h_length);
 
      DTRACE;

      SGE_ASSERT(((hent2 = gethostbyaddr(tmp_addr, hent->h_length, AF_INET)) !=
NULL));

      DTRACE;

      uti_state_set_qualified_hostname(hent->h_name);
      FREE(me.unqualified_hostname);
      me.unqualified_hostname = sge_dirname(me.qualified_hostname, '.');
   }

   DTRACE;
 
 
   /* SETPGRP; */
                                  
#ifndef WIN32NATIVE
   uti_state_set_uid(getuid());
   uti_state_set_gid(getgid());
#else
   /* TODO: Errorhandling !! id = 0 when envs not set */
   if(envs=getenv("UID")) {
      uti_state_set_uid(atol(envs));
   }
   if(envs=getenv("GID")) {
      uti_state_set_gid(atol(envs));
   }
#endif

#ifdef WIN32 /* getpwuid not called */
   uti_state_set_user_name(sge_getenv("USERNAME"));
#else
   SGE_ASSERT(((paswd = (struct passwd *) getpwuid(me.uid)) != NULL));
   uti_state_set_user_name(paswd->pw_name);
#endif
 
   uti_state_set_default_cell(sge_get_default_cell());
 
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

/****** uti/prog/sge_show_me() ************************************************
*  NAME
*     sge_show_me() -- Show content of me structure
*
*  SYNOPSIS
*     static void sge_show_me()
*
*  FUNCTION
*     Show content of me structure in debug output
******************************************************************************/
static void sge_show_me(void)
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
 
   DPRINTF(("me.who                      >%d<\n", (int) uti_state_get_mewho()));
   DPRINTF(("me.sge_formal_prog_name     >%s<\n", uti_state_get_sge_formal_prog_name()));
   DPRINTF(("me.qualified_hostname       >%s<\n", uti_state_get_qualified_hostname()));
   DPRINTF(("me.unqualified_hostname     >%s<\n", me.unqualified_hostname));
   DPRINTF(("me.uid                      >%d<\n", (int) me.uid));
   DPRINTF(("me.gid                      >%d<\n", (int) me.gid));
   DPRINTF(("me.daemonized               >%d<\n", (int) me.daemonized));
   DPRINTF(("me.user_name                >%s<\n", me.user_name));
   DPRINTF(("me.default_cell             >%s<\n", me.default_cell));
 
   DEXIT;
}     
