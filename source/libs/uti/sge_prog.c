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
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>  
#if defined(SGE_MT)
#include <pthread.h>
#endif

#include "sge.h"
#include "sgermon.h"
#include "sge_prog.h"
#include "sge_log.h"
#include "sge_stdlib.h"
#include "sge_string.h"
#include "sge_unistd.h"

#include "sge_uti_state.h"

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
 
#if defined(SGE_MT)
/* MT-NOTE: uti_state_key is used to create per thread instances of struct uti_state_t */
pthread_key_t   uti_state_key;
#else
static struct uti_state_t uti_state_opaque = {
  NULL, NULL, NULL, QUSERDEFINED, 0, 0, 0, NULL, NULL, NULL, 1, 0, NULL };
struct uti_state_t *uti_state = &uti_state_opaque;
#endif

#if defined(SGE_MT)

static void uti_state_destroy(void* state) {
   FREE(((struct uti_state_t*)state)->sge_formal_prog_name);
   FREE(((struct uti_state_t*)state)->sge_formal_prog_name);
   FREE(((struct uti_state_t*)state)->qualified_hostname);
   FREE(((struct uti_state_t*)state)->unqualified_hostname);
   FREE(((struct uti_state_t*)state)->user_name);
   FREE(((struct uti_state_t*)state)->default_cell);
   free(state);
}

void uti_init_mt(void) {
   pthread_key_create(&uti_state_key, &uti_state_destroy);
} 
  
void uti_state_init(struct uti_state_t* state) {
   state->sge_formal_prog_name = NULL;
   state->qualified_hostname = NULL;
   state->unqualified_hostname = NULL;
   state->who = QUSERDEFINED;
   state->uid = 0;
   state->gid = 0;
   state->daemonized = 0;
   state->user_name = NULL;
   state->default_cell = NULL;
   state->exit_func = NULL;
   state->exit_on_error = 1;
   state->fqdn_cmp = 0;
   state->default_domain = NULL;

}
#endif

static void sge_show_me(void); 
static void uti_state_set_sge_formal_prog_name(const char *s);
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
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_get_sge_formal_prog_name");
   return uti_state->sge_formal_prog_name;
}

const char *uti_state_get_qualified_hostname(void)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_get_qualified_hostname");
   return uti_state->qualified_hostname;
}

const char *uti_state_get_unqualified_hostname(void)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_get_unqualified_hostname");
   return uti_state->unqualified_hostname;
}

u_long32 uti_state_get_mewho(void)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_get_mewho");
   return uti_state->who;
}

u_long32 uti_state_get_uid(void)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_get_uid");
   return uti_state->uid;
}

u_long32 uti_state_get_gid(void)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_get_gid");
   return uti_state->gid;
}

int uti_state_get_daemonized(void)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_get_daemonized");
   return uti_state->daemonized;
}

const char *uti_state_get_user_name(void)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_get_user_name");
   return uti_state->user_name;
}

const char *uti_state_get_default_cell(void)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_get_default_cell");
   return uti_state->default_cell;
}

int uti_state_get_exit_on_error(void)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_get_exit_on_error");
   return uti_state->exit_on_error;
}

/****** libs/uti/uti_state_set_????() ************************************
*  NAME
*     uti_state_set_????() - write access to utilib global variables
*
*  FUNCTION
*     Provides access to either global variable or per thread global variable.
*
******************************************************************************/

static void uti_state_set_sge_formal_prog_name(const char *s)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_set_sge_formal_prog_name");
   uti_state->sge_formal_prog_name = sge_strdup(uti_state->sge_formal_prog_name, s);
}

void uti_state_set_qualified_hostname(const char *s)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_set_qualified_hostname");
   uti_state->qualified_hostname = sge_strdup(uti_state->qualified_hostname, s);
}

void uti_state_set_unqualified_hostname(const char *s)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_set_unqualified_hostname");
   uti_state->unqualified_hostname = sge_strdup(uti_state->unqualified_hostname, s);
}

void uti_state_set_daemonized(int daemonized)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_set_daemonized");
   uti_state->daemonized = daemonized;
}

void uti_state_set_mewho(u_long32 who)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_set_mewho");
   uti_state->who = who;
}

static void uti_state_set_uid(u_long32 uid)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_set_uid");
   uti_state->uid = uid;
}

static void uti_state_set_gid(u_long32 gid)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_set_gid");
   uti_state->gid = gid;
}

static void uti_state_set_user_name(const char *s)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_set_user_name");
   uti_state->user_name = sge_strdup(uti_state->user_name, s);
}

static void uti_state_set_default_cell(const char *s)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_set_default_cell");
   uti_state->default_cell = sge_strdup(uti_state->default_cell, s);
}

void uti_state_set_exit_on_error(int i)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_set_exit_on_error");
   uti_state->exit_on_error = i;
}

/****** uti/unistd/uti_state_get_exit_func() ************************************
*  NAME
*     uti_state_get_exit_func() -- Return installed exit funciton 
*
*  SYNOPSIS
*     sge_exit_func_t uti_state_get_exit_func(void)
*
*  FUNCTION
*     Returns installed exit funciton. Exit function
*     will be called be sge_exit()
*
*  RESULT
*     sge_exit_func_t - function pointer 
*
*  SEE ALSO
*     uti/unistd/sge_exit() 
******************************************************************************/
sge_exit_func_t uti_state_get_exit_func(void)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_get_exit_func");
   return uti_state->exit_func;
}

/****** uti/unistd/uti_state_set_exit_func() ************************************
*  NAME
*     uti_state_set_exit_func() -- Installs a new exit handler 
*
*  SYNOPSIS
*     void uti_state_set_exit_func(sge_exit_func_t f)
*
*  FUNCTION
*     Installs a new exit handler. Exit function will be called be sge_exit()
*
*  INPUTS
*     sge_exit_func_t f - new function pointer 
*
*  SEE ALSO
*     uti/unistd/sge_exit() 
******************************************************************************/
void uti_state_set_exit_func(sge_exit_func_t f)
{
   GET_SPECIFIC(struct uti_state_t, uti_state, uti_state_init, uti_state_key, "uti_state_set_exit_func");
   uti_state->exit_func = f;
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
*
*  NOTES
*     MT-NOTE: sge_getme() is MT safe
******************************************************************************/
void sge_getme(u_long32 program_number)
{
#ifdef WIN32NATIVE
   char *envs;
#endif /* WIN32NATIVE */
   char *s;
   stringT tmp_str;
 
#ifndef WIN32
   struct passwd *paswd;
#endif
 
   struct hostent *hent, *hent2;
 
   DENTER(TOP_LAYER, "sge_getme");
 
   /* get program info */
   uti_state_set_mewho(program_number);
   uti_state_set_sge_formal_prog_name(prognames[program_number]);

   /* Fetch hostnames */
   SGE_ASSERT((gethostname(tmp_str, sizeof(tmp_str)) == 0));
   SGE_ASSERT(((hent = gethostbyname(tmp_str)) != NULL));

   DTRACE;

   uti_state_set_qualified_hostname(hent->h_name);
   s = sge_dirname(hent->h_name, '.');
   uti_state_set_unqualified_hostname(s);
   free(s);

   DTRACE;
 
   /* Bad resolving in some networks leads to short qualified host names */
   if (!strcmp(uti_state_get_qualified_hostname(), uti_state_get_unqualified_hostname())) {
      char tmp_addr[8];
 
      memcpy(tmp_addr, hent->h_addr, hent->h_length);
 
      DTRACE;

      SGE_ASSERT(((hent2 = gethostbyaddr(tmp_addr, hent->h_length, AF_INET)) !=
NULL));

      DTRACE;

      uti_state_set_qualified_hostname(hent->h_name);
      s = sge_dirname(hent->h_name, '.');
      uti_state_set_unqualified_hostname(s);
      free(s);
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
   SGE_ASSERT(((paswd = (struct passwd *) getpwuid(uti_state_get_uid())) != NULL));
   uti_state_set_user_name(paswd->pw_name);
#endif
 
   uti_state_set_default_cell(sge_get_default_cell());
 
   sge_show_me();
 
   DEXIT;
   return;
}
 

/****** uti/prog/sge_show_me() ************************************************
*  NAME
*     sge_show_me() -- Show content of me structure
*
*  SYNOPSIS
*     static void sge_show_me()
*
*  FUNCTION
*     Show content of me structure in debug output
*
*  NOTES
*     MT-NOTE: sge_show_me() is MT safe
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
   DPRINTF(("me.unqualified_hostname     >%s<\n", uti_state_get_unqualified_hostname()));
   DPRINTF(("me.uid                      >%d<\n", (int) uti_state_get_uid()));
   DPRINTF(("me.gid                      >%d<\n", (int) uti_state_get_gid()));
   DPRINTF(("me.daemonized               >%d<\n", uti_state_get_daemonized()));
   DPRINTF(("me.user_name                >%s<\n", uti_state_get_user_name()));
   DPRINTF(("me.default_cell             >%s<\n", uti_state_get_default_cell()));
 
   DEXIT;
}     
