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
#include <string.h>
#include <pwd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>  

#include "sge.h"
#include "sgermon.h"
#include "sge_hostname.h"
#include "sge_log.h"
#include "sge_stdlib.h"
#include "sge_string.h"
#include "sge_unistd.h"

#include "sge_prog.h"

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
   "commdcntl" ,       /* 38  */
   "spoolinit" ,       /* 39  */
   "japi"      ,       /* 40  */
   "japi_ec"   ,       /* 41  */
   "drmaa"             /* 42  */
};
 
typedef struct {
   char*           sge_formal_prog_name;  /* taken from prognames[] */
   char*           qualified_hostname;
   char*           unqualified_hostname;
   u_long32        who;                   /* Qxxx defines  QUSERDEFINED  */
   u_long32        uid;
   u_long32        gid;
   int             daemonized;
   char*           user_name;
   char*           default_cell;
   sge_exit_func_t exit_func;
   int             exit_on_error;
} prog_state_t;

static pthread_once_t prog_once = PTHREAD_ONCE_INIT;
static pthread_key_t  prog_state_key;

static void          prog_once_init(void);
static void          prog_state_destroy(void *theState);
static prog_state_t* prog_state_getspecific(pthread_key_t aKey);
static void          prog_state_init(prog_state_t *theState);

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
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   return prog_state->sge_formal_prog_name;
}

const char *uti_state_get_qualified_hostname(void)
{
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   return prog_state->qualified_hostname;
}

const char *uti_state_get_unqualified_hostname(void)
{
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   return prog_state->unqualified_hostname;
}

u_long32 uti_state_get_mewho(void)
{
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   return prog_state->who;
}

u_long32 uti_state_get_uid(void)
{
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   return prog_state->uid;
}

u_long32 uti_state_get_gid(void)
{
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   return prog_state->gid;
}

int uti_state_get_daemonized(void)
{
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   return prog_state->daemonized;
}

const char *uti_state_get_user_name(void)
{
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   return prog_state->user_name;
}

const char *uti_state_get_default_cell(void)
{
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   return prog_state->default_cell;
}

int uti_state_get_exit_on_error(void)
{
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   return prog_state->exit_on_error;
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
   prog_state_t *prog_state = NULL;

   prog_state = prog_state_getspecific(prog_state_key);

   prog_state->sge_formal_prog_name = sge_strdup(prog_state->sge_formal_prog_name, s);

   return;
}

void uti_state_set_qualified_hostname(const char *s)
{
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   prog_state->qualified_hostname = sge_strdup(prog_state->qualified_hostname, s);

   return;
}

void uti_state_set_unqualified_hostname(const char *s)
{
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   prog_state->unqualified_hostname = sge_strdup(prog_state->unqualified_hostname, s);

   return;
}

void uti_state_set_daemonized(int daemonized)
{
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   prog_state->daemonized = daemonized;

   return;
}

void uti_state_set_mewho(u_long32 who)
{
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   prog_state->who = who;

   return;
}

static void uti_state_set_uid(u_long32 uid)
{
   prog_state_t *prog_state = NULL;

   prog_state = prog_state_getspecific(prog_state_key);

   prog_state->uid = uid;

   return;
}

static void uti_state_set_gid(u_long32 gid)
{
   prog_state_t *prog_state = NULL;

   prog_state = prog_state_getspecific(prog_state_key);

   prog_state->gid = gid;

   return;
}

static void uti_state_set_user_name(const char *s)
{
   prog_state_t *prog_state = NULL;

   prog_state = prog_state_getspecific(prog_state_key);

   prog_state->user_name = sge_strdup(prog_state->user_name, s);

   return;
}

static void uti_state_set_default_cell(const char *s)
{
   prog_state_t *prog_state = NULL;

   prog_state = prog_state_getspecific(prog_state_key);

   prog_state->default_cell = sge_strdup(prog_state->default_cell, s);

   return;
}

void uti_state_set_exit_on_error(int i)
{
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   prog_state->exit_on_error = i;

   return;
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
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   return prog_state->exit_func;
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
   prog_state_t *prog_state = NULL;

   pthread_once(&prog_once, prog_once_init);

   prog_state = prog_state_getspecific(prog_state_key);

   prog_state->exit_func = f;

   return;
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
   char *s = NULL;
   stringT tmp_str;
   struct passwd *paswd = NULL;
   struct hostent *hent = NULL;
 
   DENTER(TOP_LAYER, "sge_getme");
 
   pthread_once(&prog_once, prog_once_init);

   /* get program info */
   uti_state_set_mewho(program_number);
   uti_state_set_sge_formal_prog_name(prognames[program_number]);

   /* Fetch hostnames */
   SGE_ASSERT((gethostname(tmp_str, sizeof(tmp_str)) == 0));
   SGE_ASSERT(((hent = sge_gethostbyname(tmp_str,NULL)) != NULL));

   DTRACE;

   uti_state_set_qualified_hostname(hent->h_name);
   s = sge_dirname(hent->h_name, '.');
   uti_state_set_unqualified_hostname(s);
   free(s);

   DTRACE;
 
   /* Bad resolving in some networks leads to short qualified host names */
   if (!strcmp(uti_state_get_qualified_hostname(), uti_state_get_unqualified_hostname())) {
      char tmp_addr[8];
      struct hostent *hent2 = NULL;
      memcpy(tmp_addr, hent->h_addr, hent->h_length);
      DTRACE;
      SGE_ASSERT(((hent2 = sge_gethostbyaddr((const struct in_addr *)&tmp_addr, NULL)) != NULL));
      DTRACE;

      uti_state_set_qualified_hostname(hent2->h_name);
      s = sge_dirname(hent2->h_name, '.');
      uti_state_set_unqualified_hostname(s);
      free(s);
      sge_free_hostent(&hent2);
   }

   sge_free_hostent(&hent);
   DTRACE;

   /* SETPGRP; */
   uti_state_set_uid(getuid());
   uti_state_set_gid(getgid());
   SGE_ASSERT(((paswd = (struct passwd *) getpwuid(uti_state_get_uid())) != NULL));
   uti_state_set_user_name(paswd->pw_name);
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
   if (!TRACEON) {
      DEXIT;
      return;
   }
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
   return;
}     

/****** uti/prog/prog_once_init() *********************************************
*  NAME
*     prog_once_init() -- One-time executable state initialization.
*
*  SYNOPSIS
*     static prog_once_init(void) 
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
*     MT-NOTE: prog_once_init() is MT safe. 
*
*******************************************************************************/
static void prog_once_init(void)
{
   pthread_key_create(&prog_state_key, &prog_state_destroy);
   return;
} /* prog_once_init() */

/****** uti/prog/prog_state_destroy() ******************************************
*  NAME
*     prog_state_destroy() -- Free thread local storage
*
*  SYNOPSIS
*     static void prog_state_destroy(void* theState) 
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
*     MT-NOTE: prog_state_destroy() is MT safe.
*
*******************************************************************************/
static void prog_state_destroy(void *theState)
{
   prog_state_t *s = (prog_state_t *)theState;

   FREE(s->sge_formal_prog_name);
   FREE(s->sge_formal_prog_name);
   FREE(s->qualified_hostname);
   FREE(s->unqualified_hostname);
   FREE(s->user_name);
   FREE(s->default_cell);
   sge_free((char*)s);
}

/****** uti/prog/prog_state_getspecific() **************************************
*  NAME
*     prog_state_getspecific() -- Get thread local prog state 
*
*  SYNOPSIS
*     static prog_state_t* prog_state_getspecific(pthread_key_t aKey) 
*
*  FUNCTION
*     Return thread local prog state. 
*
*     If a given thread does call this function for the first time, no thread
*     local prog state is available for this particular thread. In this case the
*     thread local prog state is allocated and set.
*
*  INPUTS
*     pthread_key_t aKey - Key for thread local prog state. 
*
*  RESULT
*     static prog_state_t* - Pointer to thread local prog state
*
*  NOTES
*     MT-NOTE: prog_state_getspecific() is MT safe 
*
*******************************************************************************/
static prog_state_t* prog_state_getspecific(pthread_key_t aKey)
{
   prog_state_t *prog_state = NULL;
   int res = EINVAL;

   if ((prog_state = pthread_getspecific(aKey)) != NULL) { return prog_state; }

   prog_state = (prog_state_t*)sge_malloc(sizeof(prog_state_t));

   prog_state_init(prog_state);

   res = pthread_setspecific(prog_state_key, (const void*)prog_state);

   if (0 != res) {
      fprintf(stderr, "pthread_set_specific(%s) failed: %s\n", "prog_state_getspecific", strerror(res));
      abort();
   }
   
   return prog_state;
} /* prog_state_getspecific() */

/****** sge_prog/prog_state_init() *******************************************
*  NAME
*     prog_state_init() -- Initialize executable state.
*
*  SYNOPSIS
*     static void prog_state_init(prog_state_t *theState) 
*
*  FUNCTION
*     Initialize executable state.
*
*  INPUTS
*     struct prog_state_t* theState - Pointer to executable state structure.
*
*  RESULT
*     static void - none
*
*  NOTES
*     MT-NOTE: prog_state_init() in MT safe. 
*
*******************************************************************************/
static void prog_state_init(prog_state_t *theState)
{
   theState->sge_formal_prog_name = NULL;
   theState->qualified_hostname = NULL;
   theState->unqualified_hostname = NULL;
   theState->who = QUSERDEFINED;
   theState->uid = 0;
   theState->gid = 0;
   theState->daemonized = 0;
   theState->user_name = NULL;
   theState->default_cell = NULL;
   theState->exit_func = NULL;
   theState->exit_on_error = 1;

   return;
}
