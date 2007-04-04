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
#include <grp.h>
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
#include "sge_env.h"
#include "sge_prog.h"
#include "setup_path.h"
#include "sge_bootstrap.h"
#include "commlib.h"
#include "sge_answer.h"
#include "msg_gdilib.h"
#include "msg_utilib.h"
#include "qm_name.h"
#include "sge_uidgid.h"
#include "sge_profiling.h"
#include "sge_csp_path.h"
#include "sge_feature.h"
#include "sge_conf.h"
#include "sge_object.h"
#include "lck/sge_lock.h"

/*
** need this for lInit(nmv)
*/
extern lNameSpace nmv[];

#if  1
/* TODO: throw this out asap */
#include "sge_gdi.h"
#include "sge_gdi_request.h"
void gdi_once_init(void);
void gdi_init_mt(void);
void feature_mt_init(void);
void sc_mt_init(void);
#endif

#include "gdi/sge_gdi_ctx.h"
#include "gdi/sge_gdi2.h"

#ifdef NEW_GDI_STATE 
typedef struct {
   /* gdi request base */
   u_long32 request_id;     /* incremented with each GDI request to have a unique request ID
                               it is ensured that the request ID is also contained in answer */
   gdi_send_t *async_gdi; /* used to store a async GDI request.*/
} gdi_state_t;
#endif


typedef struct {
   sge_env_state_class_t* sge_env_state_obj;
   sge_prog_state_class_t* sge_prog_state_obj;
   sge_path_state_class_t* sge_path_state_obj;
   sge_bootstrap_state_class_t* sge_bootstrap_state_obj;
   
   char* component_name;
   char* master;
   char* component_username;
   char* username;
   char* groupname;
   uid_t uid;
   gid_t gid;

   char *ssl_private_key;
   char *ssl_certificate;
   
   lList *alp;
   int last_commlib_error;
   sge_error_class_t *eh;

   bool is_setup;

#ifdef NEW_GDI_STATE
   /* gdi_state */
   gdi_state_t gdi_state;
#endif   

} sge_gdi_ctx_t;

#ifndef NEW_GDI_STATE
static void gdi_free_request(gdi_send_t **async_gdi); 

static pthread_key_t  gdi_state_key;
static pthread_once_t gdi_once_control = PTHREAD_ONCE_INIT;

typedef struct {
   /* gdi request base */
   u_long32 request_id;     /* incremented with each GDI request to have a unique request ID
                               it is ensured that the request ID is also contained in answer */
   int      made_setup;
   int      isalive;

   char     cached_master_name[CL_MAXHOSTLEN];

   gdi_send_t *async_gdi; /* used to store a async GDI request.*/
} gdi_state_t;


static void gdi_state_destroy(void* state) {
   free(state);
}

void gdi_once_init(void) {
   pthread_key_create(&gdi_state_key, &gdi_state_destroy);
} 
 
static void gdi_state_init(gdi_state_t* state) {
   state->request_id = 0;
   state->made_setup = 0;
   state->isalive = 0;
   strcpy(state->cached_master_name, "");

   state->async_gdi = NULL;
}

/****** gid/gdi_setup/gdi_mt_init() ************************************************
*  NAME
*     gdi_mt_init() -- Initialize GDI state for multi threading use.
*
*  SYNOPSIS
*     void gdi_mt_init(void) 
*
*  FUNCTION
*     Set up GDI. This function must be called at least once before any of the
*     GDI functions is used. This function is idempotent, i.e. it is safe to
*     call it multiple times.
*
*     Thread local storage for the GDI state information is reserved. 
*
*  INPUTS
*     void - NONE 
*
*  RESULT
*     void - NONE
*
*  NOTES
*     MT-NOTE: gdi_mt_init() is MT safe 
*
*******************************************************************************/
void gdi_mt_init(void)
{
   pthread_once(&gdi_once_control, gdi_once_init);
}

/****** libs/gdi/gdi_state_get_????() ************************************
*  NAME
*     gdi_state_get_????() - read access to gdilib global variables
*
*  FUNCTION
*     Provides access to either global variable or per thread global
*     variable.
*
******************************************************************************/
u_long32 gdi_state_get_next_request_id(void)
{
   GET_SPECIFIC(gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_get_next_request_id");
   gdi_state->request_id++;
   return gdi_state->request_id;
}

#ifdef USE_GDI_STATE
static void gdi_state_set_made_setup(int i)
{
   GET_SPECIFIC(gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_set_made_setup");
   gdi_state->made_setup = i;
}
#endif


int gdi_state_get_made_setup(void)
{
   GET_SPECIFIC(gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_get_made_setup");
   return gdi_state->made_setup;
}

char *gdi_state_get_cached_master_name(void)
{
   GET_SPECIFIC(gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_get_cached_master_name");
   return gdi_state->cached_master_name;
}

gdi_send_t*
gdi_state_get_last_gdi_request(void) {
   GET_SPECIFIC(gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, 
                "gdi_state_get_last_gdi_request");

   return gdi_state->async_gdi;
}

void 
gdi_state_clear_last_gdi_request(void) 
{
   GET_SPECIFIC(gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, 
                "gdi_state_clear_last_gdi_request");
   gdi_free_request(&(gdi_state->async_gdi));
}

static void
gdi_free_request(gdi_send_t **async_gdi) 
{
   (*async_gdi)->out.first = free_gdi_request((*async_gdi)->out.first);
   FREE(*async_gdi);
}

bool
gdi_set_request(const char* rhost, const char* commproc, u_short id, 
                state_gdi_multi *out, u_long32 gdi_request_mid) 
{
   gdi_send_t *async_gdi = NULL;
 
   async_gdi = (gdi_send_t*) malloc(sizeof(gdi_send_t));
   if (async_gdi == NULL) {
      return false;
   }
 
   sge_strlcpy(async_gdi->rhost, rhost, CL_MAXHOSTLEN);
   sge_strlcpy(async_gdi->commproc, commproc, CL_MAXHOSTLEN);
   async_gdi->id = id;
   async_gdi->gdi_request_mid = gdi_request_mid;

   /* copy gdi request and set the past in one to NULL */
   async_gdi->out.first = out->first;
   out->first = NULL;
   async_gdi->out.last = out->last;
   out->last = NULL;
   async_gdi->out.sequence_id = out->sequence_id;
   out->sequence_id = 0;
 
   { /* set thread specific value */
      GET_SPECIFIC(gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, 
                   "gdi_set_request");
      if (gdi_state->async_gdi != NULL) {
         gdi_free_request(&(gdi_state->async_gdi));
      }
      gdi_state->async_gdi = async_gdi;
   }
  
   return true;
}

#else

static void gdi_free_request(gdi_send_t **async_gdi); 

u_long32 gdi_state_get_next_request_id(sge_gdi_ctx_class_t *ctx)
{
   sge_gdi_ctx_t *gdi_ctx = (sge_gdi_ctx_t*)ctx->sge_gdi_ctx_handle;
   gdi_state_t gdi_state = gdi_ctx->gdi_state;
   gdi_state.request_id++;
   return gdi_state.request_id;
}

gdi_send_t* gdi_state_get_last_gdi_request(sge_gdi_ctx_class_t *ctx) {
   sge_gdi_ctx_t *gdi_ctx = (sge_gdi_ctx_t*)ctx->sge_gdi_ctx_handle;
   gdi_state_t gdi_state = gdi_ctx->gdi_state;
   return gdi_state.async_gdi;
}

void gdi_state_clear_last_gdi_request(sge_gdi_ctx_class_t *ctx) 
{
   sge_gdi_ctx_t *gdi_ctx = (sge_gdi_ctx_t*)ctx->sge_gdi_ctx_handle;
   gdi_state_t gdi_state = gdi_ctx->gdi_state;
   gdi_free_request(&(gdi_state.async_gdi));
}

static void gdi_free_request(gdi_send_t **async_gdi) 
{
   (*async_gdi)->out.first = free_gdi_request((*async_gdi)->out.first);
   FREE(*async_gdi);
}

bool
gdi_set_request(sge_gdi_ctx_class_t *ctx, const char* rhost, const char* commproc, u_short id, 
                state_gdi_multi *out, u_long32 gdi_request_mid) 
{
   sge_gdi_ctx_t *gdi_ctx = (sge_gdi_ctx_t*)ctx->sge_gdi_ctx_handle;
   gdi_state_t gdi_state = gdi_ctx->gdi_state;
   gdi_send_t *async_gdi = NULL;
 
   DENTER(TOP_LAYER, "gdi_set_request");

   async_gdi = (gdi_send_t*)malloc(sizeof(gdi_send_t));
   if (async_gdi == NULL) {
      DRETURN(false);
   }
 
   sge_strlcpy(async_gdi->rhost, rhost, CL_MAXHOSTLEN);
   sge_strlcpy(async_gdi->commproc, commproc, CL_MAXHOSTLEN);
   async_gdi->id = id;
   async_gdi->gdi_request_mid = gdi_request_mid;

   /* copy gdi request and set the past in one to NULL */
   async_gdi->out.first = out->first;
   out->first = NULL;
   async_gdi->out.last = out->last;
   out->last = NULL;
   async_gdi->out.sequence_id = out->sequence_id;
   out->sequence_id = 0;
 
   /* set context specific value */
   if (gdi_state.async_gdi != NULL) {
      gdi_free_request(&(gdi_state.async_gdi));
   }
   gdi_state.async_gdi = async_gdi;

#if 0
   /* AA: async GDI DEBUG */
   if (gdi_state.async_gdi) {
      sge_gdi_request *req = gdi_state.async_gdi->out.first;
      int count = 0;
      while (req) {
         printf("***************request %d is: (%s/%s/%d) request_id="sge_u32"\n",
                 count, req->host, req->commproc, req->id, req->request_id);
         count++;
         req=req->next;
      }           
   }   
#endif   
  
   DRETURN(true);
}

#endif /* NEW_GDI_STATE */




typedef struct {
   sge_gdi_ctx_class_t* ctx;   
} sge_gdi_ctx_thread_local_t;

static pthread_once_t sge_gdi_ctx_once = PTHREAD_ONCE_INIT;
static pthread_key_t  sge_gdi_ctx_key;
static void sge_gdi_thread_local_ctx_once_init(void);
static void sge_gdi_thread_local_ctx_destroy(void* theState);
static void sge_gdi_thread_local_ctx_init(sge_gdi_ctx_thread_local_t* theState);



static void sge_gdi_thread_local_ctx_once_init(void)
{
   pthread_key_create(&sge_gdi_ctx_key, sge_gdi_thread_local_ctx_destroy);
}

static void sge_gdi_thread_local_ctx_destroy(void* theState) {
   sge_gdi_ctx_thread_local_t *tl = (sge_gdi_ctx_thread_local_t*)theState;
   tl->ctx = NULL;
   free(theState);
}

static void sge_gdi_thread_local_ctx_init(sge_gdi_ctx_thread_local_t* theState)
{
   memset(theState, 0, sizeof(sge_gdi_ctx_thread_local_t));
}


sge_gdi_ctx_class_t* sge_gdi_get_thread_local_ctx() {

   pthread_once(&sge_gdi_ctx_once, sge_gdi_thread_local_ctx_once_init);
   {
      GET_SPECIFIC(sge_gdi_ctx_thread_local_t, tl, sge_gdi_thread_local_ctx_init, sge_gdi_ctx_key,
                "sge_gdi_get_thread_local_ctx");
      return tl->ctx;
   }   
}

void sge_gdi_set_thread_local_ctx(sge_gdi_ctx_class_t* ctx) {
 
   DENTER(TOP_LAYER, "sge_gdi_set_thread_local_ctx");
   
   pthread_once(&sge_gdi_ctx_once, sge_gdi_thread_local_ctx_once_init);
   { 
      GET_SPECIFIC(sge_gdi_ctx_thread_local_t, tl, sge_gdi_thread_local_ctx_init, sge_gdi_ctx_key,
                "set_thread_local_ctx");
      tl->ctx = ctx;

      if (ctx != NULL) {
         sge_bootstrap_state_set_thread_local(ctx->get_sge_bootstrap_state(ctx));
         log_state_set_log_context(ctx);
      } else {
         sge_bootstrap_state_set_thread_local(NULL);
         log_state_set_log_context(NULL);
      }
   }   
   DRETURN_VOID;
}

static bool sge_gdi_ctx_setup(sge_gdi_ctx_class_t *thiz, 
                              int prog_number,
                              const char* component_name,
                              const char* username,
                              const char* groupname,
                              const char *sge_root, 
                              const char *sge_cell, 
                              int sge_qmaster_port, 
                              int sge_execd_port);
static void sge_gdi_ctx_destroy(void *theState);

static void sge_gdi_ctx_set_is_setup(sge_gdi_ctx_class_t *thiz, bool is_setup);
static bool sge_gdi_ctx_is_setup(sge_gdi_ctx_class_t *thiz);
static void sge_gdi_ctx_class_get_errors(sge_gdi_ctx_class_t *thiz, lList **alpp, bool clear_errors);
static void sge_gdi_ctx_class_error(sge_gdi_ctx_class_t *thiz, int error_type, int error_quality, const char* fmt, ...);
static sge_env_state_class_t* get_sge_env_state(sge_gdi_ctx_class_t *thiz);
static sge_prog_state_class_t* get_sge_prog_state(sge_gdi_ctx_class_t *thiz);
static sge_path_state_class_t* get_sge_path_state(sge_gdi_ctx_class_t *thiz);
static sge_bootstrap_state_class_t* get_sge_bootstrap_state(sge_gdi_ctx_class_t *thiz);
static int reresolve_qualified_hostname(sge_gdi_ctx_class_t *thiz);
static cl_com_handle_t* get_com_handle(sge_gdi_ctx_class_t *thiz);
static const char* get_component_name(sge_gdi_ctx_class_t *thiz);
static const char* get_progname(sge_gdi_ctx_class_t *thiz);
static u_long32 get_who(sge_gdi_ctx_class_t *thiz);
static bool is_daemonized(sge_gdi_ctx_class_t *thiz);
static void set_daemonized(sge_gdi_ctx_class_t *thiz, bool daemonized);
static bool get_job_spooling(sge_gdi_ctx_class_t *thiz);
static void set_job_spooling(sge_gdi_ctx_class_t *thiz, bool job_spooling);
static u_long32 get_gdi_thread_count(sge_gdi_ctx_class_t *thiz);
static const char* get_master(sge_gdi_ctx_class_t *thiz, bool reread);
static u_long32 get_sge_qmaster_port(sge_gdi_ctx_class_t *thiz);
static u_long32 get_sge_execd_port(sge_gdi_ctx_class_t *thiz);
static const char* get_spooling_method(sge_gdi_ctx_class_t *thiz);
static const char* get_spooling_lib(sge_gdi_ctx_class_t *thiz);
static const char* get_spooling_params(sge_gdi_ctx_class_t *thiz);
static const char* get_username(sge_gdi_ctx_class_t *thiz);
static const char* get_cell_root(sge_gdi_ctx_class_t *thiz);
static const char* get_sge_root(sge_gdi_ctx_class_t *thiz);
static const char* get_groupname(sge_gdi_ctx_class_t *thiz);
static uid_t ctx_get_uid(sge_gdi_ctx_class_t *thiz);
static gid_t ctx_get_gid(sge_gdi_ctx_class_t *thiz);
static const char* get_qualified_hostname(sge_gdi_ctx_class_t *thiz);
static const char* get_unqualified_hostname(sge_gdi_ctx_class_t *thiz);
static const char* get_default_cell(sge_gdi_ctx_class_t *thiz);
static const char* get_admin_user(sge_gdi_ctx_class_t *thiz);
static const char* get_binary_path(sge_gdi_ctx_class_t *thiz);
static const char* get_qmaster_spool_dir(sge_gdi_ctx_class_t *thiz);
static const char* get_bootstrap_file(sge_gdi_ctx_class_t *thiz);
static const char* get_act_qmaster_file(sge_gdi_ctx_class_t *thiz);
static const char* get_acct_file(sge_gdi_ctx_class_t *thiz);
static const char* get_reporting_file(sge_gdi_ctx_class_t *thiz);
static const char* get_shadow_master_file(sge_gdi_ctx_class_t *thiz);
static sge_exit_func_t get_exit_func(sge_gdi_ctx_class_t *thiz);
static void set_exit_func(sge_gdi_ctx_class_t *thiz, sge_exit_func_t exfunc);
static void set_private_key(sge_gdi_ctx_class_t *thiz, const char *pkey);
static void set_certificate(sge_gdi_ctx_class_t *thiz, const char *cert);
static const char* get_private_key(sge_gdi_ctx_class_t *thiz);
static const char* get_certificate(sge_gdi_ctx_class_t *thiz);
static int ctx_get_last_commlib_error(sge_gdi_ctx_class_t *thiz);
static void ctx_set_last_commlib_error(sge_gdi_ctx_class_t *thiz, int cl_error);

static int sge_gdi_ctx_class_prepare_enroll(sge_gdi_ctx_class_t *thiz);
static int sge_gdi_ctx_class_connect(sge_gdi_ctx_class_t *thiz);
static int sge_gdi_ctx_class_is_alive(sge_gdi_ctx_class_t *thiz);
static lList* sge_gdi_ctx_class_gdi(sge_gdi_ctx_class_t *thiz, int target, int cmd, lList **lpp,
                                 lCondition *where, lEnumeration *what);
static int sge_gdi_ctx_class_gdi_multi(sge_gdi_ctx_class_t* ctx, lList **alpp, int mode, u_long32 target, u_long32 cmd,
                  lList **lp, lCondition *cp, lEnumeration *enp, lList **malpp, 
                  state_gdi_multi *state, bool do_copy);
                  
static lList* sge_gdi_ctx_class_gdi_tsm(sge_gdi_ctx_class_t *thiz, const char *schedd_name, const char *cell);
static lList* sge_gdi_ctx_class_gdi_kill(sge_gdi_ctx_class_t *thiz, lList *id_list, const char *cell, 
                                          u_long32 option_flags, u_long32 action_flag);
static bool sge_gdi_ctx_class_gdi_get_mapping_name(sge_gdi_ctx_class_t *thiz, const char *requestedHost, char *buf, int buflen);
static bool sge_gdi_ctx_class_gdi_check_permission(sge_gdi_ctx_class_t *thiz, lList **alpp, int option);

static int sge_gdi_ctx_log_flush_func(cl_raw_list_t* list_p);

static void sge_gdi_ctx_class_dprintf(sge_gdi_ctx_class_t *ctx);

static bool sge_gdi_ctx_class_gdi_receive_multi_async(sge_gdi_ctx_class_t* thiz, 
                                            sge_gdi_request **answerp, 
                                            lList **malpp, 
                                            bool is_sync);
static int sge_gdi_ctx_class_gdi_multi_sync(sge_gdi_ctx_class_t* thiz, 
                                            lList **alpp, 
                                            int mode, 
                                            u_long32 target, 
                                            u_long32 cmd,
                                            lList **lpp, 
                                            lCondition *cp, 
                                            lEnumeration *enp, 
                                            lList **malpp, 
                                            state_gdi_multi *state, 
                                            bool do_copy,
                                            bool do_sync);


sge_gdi_ctx_class_t *sge_gdi_ctx_class_create(int prog_number, const char* component_name,
                                              const char* username, const char *groupname,
                                              const char *sge_root, const char *sge_cell, 
                                              int sge_qmaster_port, int sge_execd_port, lList **alpp)
{
   sge_gdi_ctx_class_t *ret = (sge_gdi_ctx_class_t *)sge_malloc(sizeof(sge_gdi_ctx_class_t));
   sge_gdi_ctx_t *gdi_ctx = NULL;

   DENTER(TOP_LAYER, "sge_gdi_ctx_class_create");

   if (!ret) {
      answer_list_add_sprintf(alpp, STATUS_EMALLOC, ANSWER_QUALITY_ERROR, MSG_MEMORY_MALLOCFAILED);
      DRETURN(NULL);
   }

   ret->get_errors = sge_gdi_ctx_class_get_errors;
   ret->prepare_enroll = sge_gdi_ctx_class_prepare_enroll;
   ret->connect = sge_gdi_ctx_class_connect;
   ret->is_alive = sge_gdi_ctx_class_is_alive;
   ret->gdi = sge_gdi_ctx_class_gdi;
   ret->gdi_multi = sge_gdi_ctx_class_gdi_multi;
   ret->tsm = sge_gdi_ctx_class_gdi_tsm;
   ret->kill = sge_gdi_ctx_class_gdi_kill;
   ret->gdi_check_permission = sge_gdi_ctx_class_gdi_check_permission;
   ret->gdi_get_mapping_name = sge_gdi_ctx_class_gdi_get_mapping_name;
   ret->gdi_multi_sync = sge_gdi_ctx_class_gdi_multi_sync;
   ret->gdi_receive_multi_async = sge_gdi_ctx_class_gdi_receive_multi_async;
   
   ret->get_sge_env_state = get_sge_env_state;
   ret->get_sge_prog_state = get_sge_prog_state;
   ret->get_sge_path_state = get_sge_path_state;
   ret->get_sge_bootstrap_state = get_sge_bootstrap_state;
   ret->reresolve_qualified_hostname = reresolve_qualified_hostname;
   ret->get_component_name = get_component_name;
   ret->get_progname = get_progname;
   ret->get_who = get_who;
   ret->is_daemonized = is_daemonized;
   ret->set_daemonized = set_daemonized;
   ret->get_job_spooling = get_job_spooling;
   ret->set_job_spooling = set_job_spooling;
   ret->get_gdi_thread_count = get_gdi_thread_count;
   ret->get_qualified_hostname = get_qualified_hostname;
   ret->get_unqualified_hostname = get_unqualified_hostname;
   ret->get_master = get_master;
   ret->get_sge_qmaster_port = get_sge_qmaster_port;
   ret->get_sge_execd_port = get_sge_execd_port;
   ret->get_username = get_username;
   ret->get_spooling_method = get_spooling_method;
   ret->get_spooling_lib = get_spooling_lib;
   ret->get_spooling_params = get_spooling_params;
   ret->get_admin_user = get_admin_user;
   ret->get_binary_path = get_binary_path;
   ret->get_qmaster_spool_dir = get_qmaster_spool_dir;
   ret->get_bootstrap_file = get_bootstrap_file;
   ret->get_act_qmaster_file = get_act_qmaster_file;
   ret->get_acct_file = get_acct_file;
   ret->get_reporting_file = get_reporting_file;
   ret->get_shadow_master_file = get_shadow_master_file;
   ret->get_default_cell = get_default_cell;
   ret->get_cell_root = get_cell_root;
   ret->get_sge_root = get_sge_root;
   ret->get_groupname = get_groupname;
   ret->get_uid = ctx_get_uid;
   ret->get_gid = ctx_get_gid;   
   ret->get_com_handle = get_com_handle;   

   ret->set_exit_func = set_exit_func;
   ret->get_exit_func = get_exit_func;

   ret->set_private_key = set_private_key;
   ret->set_certificate = set_certificate;
   ret->get_private_key = get_private_key;
   ret->get_certificate = get_certificate;

   ret->dprintf = sge_gdi_ctx_class_dprintf;

   ret->sge_gdi_ctx_handle = (sge_gdi_ctx_t*)sge_malloc(sizeof(sge_gdi_ctx_t));
   memset(ret->sge_gdi_ctx_handle, 0, sizeof(sge_gdi_ctx_t));

   if (!ret->sge_gdi_ctx_handle) {
      answer_list_add_sprintf(alpp, STATUS_EMALLOC, ANSWER_QUALITY_ERROR, MSG_MEMORY_MALLOCFAILED);
      sge_gdi_ctx_class_destroy(&ret);
      DRETURN(NULL);
   }

   /*
   ** create error handler of context
   */
   gdi_ctx = (sge_gdi_ctx_t*)ret->sge_gdi_ctx_handle;
   gdi_ctx->eh = sge_error_class_create();
   if (!gdi_ctx->eh) {
      answer_list_add_sprintf(alpp, STATUS_EMALLOC, ANSWER_QUALITY_ERROR, MSG_MEMORY_MALLOCFAILED);
      DRETURN(NULL);
   }


   if (!sge_gdi_ctx_setup(ret, prog_number, component_name, username, groupname, sge_root, sge_cell, sge_qmaster_port, sge_execd_port)) {
      sge_gdi_ctx_class_get_errors(ret, alpp, true);
      sge_gdi_ctx_class_destroy(&ret);
      DRETURN(NULL);
   }

   DRETURN(ret);
}

void sge_gdi_ctx_class_destroy(sge_gdi_ctx_class_t **pst)
{
   DENTER(TOP_LAYER, "sge_gdi_ctx_class_destroy");

   if (!pst || !*pst) {
      DRETURN_VOID;
   }   
      
   /* free internal context structure */   
   sge_gdi_ctx_destroy((*pst)->sge_gdi_ctx_handle);
   FREE(*pst);
   *pst = NULL;

   DRETURN_VOID;
}

static void sge_gdi_ctx_class_get_errors(sge_gdi_ctx_class_t *thiz, lList **alpp, bool clear_errors)
{
   sge_gdi_ctx_t *gdi_ctx = NULL;

   DENTER(TOP_LAYER, "sge_gdi_ctx_class_get_errors");

   if (!thiz || !thiz->sge_gdi_ctx_handle) {
      DRETURN_VOID;
   }   
   
   gdi_ctx = (sge_gdi_ctx_t*)thiz->sge_gdi_ctx_handle;
      
   sge_error_to_answer_list(gdi_ctx->eh, alpp, clear_errors);

   DRETURN_VOID;
}

static void sge_gdi_ctx_class_error(sge_gdi_ctx_class_t *thiz, int error_type, int error_quality, const char* fmt, ...)
{
   sge_gdi_ctx_t *gdi_ctx = NULL;

   DENTER(TOP_LAYER, "sge_gdi_ctx_class_error");

   if (!thiz || !thiz->sge_gdi_ctx_handle) {
      DRETURN_VOID;
   }   
   
   gdi_ctx = (sge_gdi_ctx_t*)thiz->sge_gdi_ctx_handle;
      
   if (gdi_ctx->eh) {
      if (fmt != NULL) {
         va_list arg_list;
         va_start(arg_list, fmt);
         gdi_ctx->eh->verror(gdi_ctx->eh, error_type, error_quality, fmt, arg_list);
      }
   }   

   DRETURN_VOID;
}

static void sge_gdi_ctx_set_is_setup(sge_gdi_ctx_class_t *thiz, bool is_setup)
{
   sge_gdi_ctx_t *gdi_ctx = NULL;

   DENTER(TOP_LAYER, "sge_gdi_ctx_set_is_setup");

   if (!thiz || !thiz->sge_gdi_ctx_handle) {
      DRETURN_VOID;
   }   
   
   gdi_ctx = (sge_gdi_ctx_t*)thiz->sge_gdi_ctx_handle;
      
   gdi_ctx->is_setup = is_setup;

   DRETURN_VOID;
}


static bool sge_gdi_ctx_is_setup(sge_gdi_ctx_class_t *thiz)
{
   sge_gdi_ctx_t *gdi_ctx = NULL;

   DENTER(TOP_LAYER, "sge_gdi_ctx_is_setup");

   if (!thiz || !thiz->sge_gdi_ctx_handle) {
      DRETURN(false);
   }   
   
   gdi_ctx = (sge_gdi_ctx_t*)thiz->sge_gdi_ctx_handle;
      
   DRETURN(gdi_ctx->is_setup);
}


static bool sge_gdi_ctx_setup(sge_gdi_ctx_class_t *thiz, int prog_number, const char* component_name,
                              const char* username, const char *groupname,
                              const char *sge_root, const char *sge_cell, int sge_qmaster_port, int sge_execd_port)
{
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *)thiz->sge_gdi_ctx_handle;
   sge_error_class_t *eh = es->eh;

   DENTER(TOP_LAYER, "sge_gdi_ctx_setup");
 
   /*
   ** grrrhhh: we have to handle these inits
   ** per process initialization
   ** void gdi_once_init(void) {
   **    // uti 
   **    uidgid_mt_init();       <--- not yet
   **    bootstrap_mt_init();
   **    feature_mt_init();      <--- not yet
   **    sge_prof_setup();
   **    // gdi 
   **    gdi_init_mt();          <--- not yet
   **    path_mt_init();
   **    // schedd conf
   **    sc_mt_init()
   ** }
   */

/*    gdi_once_init(); */
   
   /* TODO: profiling init, is this possible */
   sge_prof_setup();
   feature_mt_init();
#ifndef NEW_GDI_STATE   
   gdi_mt_init();
#endif   
   sc_mt_init();
   obj_mt_init();
   bootstrap_mt_init();

   /* TODO: shall we do that here ? */
   lInit(nmv);

   es->sge_env_state_obj = sge_env_state_class_create(sge_root, sge_cell, sge_qmaster_port, sge_execd_port, eh);
   if (!es->sge_env_state_obj) {
      DRETURN(false);
   }   

   es->sge_prog_state_obj = sge_prog_state_class_create(es->sge_env_state_obj, prog_number, eh);
   if (!es->sge_prog_state_obj) {
      DRETURN(false);
   }   

   es->sge_path_state_obj = sge_path_state_class_create(es->sge_env_state_obj, eh);
   if (!es->sge_path_state_obj) {
      DRETURN(false);
   }

   es->sge_bootstrap_state_obj = sge_bootstrap_state_class_create(es->sge_path_state_obj, eh);
   if (!es->sge_bootstrap_state_obj) {
      DRETURN(false);
   }
   
   if (feature_initialize_from_string(es->sge_bootstrap_state_obj->get_security_mode(es->sge_bootstrap_state_obj))) {
      DRETURN(false);
   }   
   
   if(component_name == NULL) {
      es->component_name = strdup(prognames[prog_number]);
   } else {
      es->component_name = strdup(component_name);
   }
   
   /* set uid and gid */
   {      
      struct passwd *pwd;
#ifdef HAS_GETPWNAM_R
      struct passwd pw_struct;
      char buffer[2048];
#endif

#ifdef HAS_GETPWNAM_R
      pwd = sge_getpwnam_r(username, &pw_struct, buffer, sizeof(buffer));
#else
      pwd = sge_getpwnam(username);
#endif

      if (!pwd) {
         eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "sge_getpwnam failed for username %s", username);
         DRETURN(false);
      }
      es->uid = pwd->pw_uid;
      if (groupname != NULL) {
         gid_t gid;
         if (sge_group2gid(groupname, &gid, MAX_NIS_RETRIES) == 1) {
            eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "sge_group2gid failed for groupname %s", groupname);
            DRETURN(false);
         }
         es->gid = gid;
      } else {
         es->gid = pwd->pw_gid;
      }   
   }
   
   es->username = strdup(username);

#if defined( INTERIX )
   /* Strip Windows domain name from user name */
   {
      char *plus_sign;

      plus_sign = strstr(es->username, "+");
      if (plus_sign!=NULL) {
         plus_sign++;
         strcpy(es->username, plus_sign);
      }
   }
#endif
   
   /*
   ** groupname
   */
   if (groupname != NULL) {
      es->groupname = strdup(groupname);
   } else {   
      if (_sge_gid2group(es->gid, &(es->gid), &(es->groupname), MAX_NIS_RETRIES)) {
         eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, MSG_GDI_GETGRGIDXFAILEDERRORX_U, sge_u32c(es->gid));
         DRETURN(false);
      }
   }

   /*
   ** set the component_username and check if login is needed
   */
   {
      struct passwd *pwd = NULL;
      char buffer[2048];
      struct passwd pwentry;
      if (getpwuid_r((uid_t)getuid(), &pwentry, buffer, sizeof(buffer), &pwd) == 0) {
         es->component_username = sge_strdup(es->component_username, pwd->pw_name);
      } else {
         eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "getpwuid_r failed");
         DRETURN(false);
      }

#if 0
      /*
      ** TODO: Login to system somehow and send something like a token with request
      **       similar like the secret key in CSP mode
      */
      DPRINTF(("es->username: '%s', es->component_username: '%s'\n", es->username, es->component_username));      
      if (strcmp(es->username, es->component_username) != 0) {
#if 1      
         eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "!!!! Alert login needed !!!!!");
         DRETURN(false);
#else
         eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "!!!! First time login !!!!!");
/*          sge_authenticate(es->username, callback_handler); */
         
#endif
      }
#endif
   }    

   DRETURN(true);
}

sge_gdi_ctx_class_t *sge_gdi_ctx_class_create_from_bootstrap(int prog_number,
                                                             const char* component_name,
                                                             const char* url,
                                                             const char* username,
                                                             lList **alpp) 
{
   char sge_root[BUFSIZ];
   char sge_cell[BUFSIZ];
   char sge_qmaster_port[BUFSIZ];
   char *token = NULL;
   char sge_url[BUFSIZ];
   
   struct  saved_vars_s *url_ctx = NULL;
   int sge_qmaster_p = 0;
   int sge_execd_p = 0;

   sge_gdi_ctx_class_t * ret = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class_create_from_bootstrap");

   /* parse the url */
   DPRINTF(("url = %s\n", url));
   sscanf(url, "bootstrap://%s", sge_url);
   DPRINTF(("sge_url = %s\n", sge_url));
   
   /* search for sge_root */
   token = sge_strtok_r(sge_url, "@", &url_ctx);   
   if (token == NULL ) {
      answer_list_add_sprintf(alpp, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "invalid url, sge_root not found");
      sge_free_saved_vars(url_ctx);
      DRETURN(NULL);
   }   
   strcpy(sge_root, token);
   
   /* search for sge_cell */
   token = sge_strtok_r(NULL, ":", &url_ctx);
   
   if (token == NULL ) {
      answer_list_add_sprintf(alpp, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "invalid url, sge_cell not found");
      sge_free_saved_vars(url_ctx);
      DRETURN(NULL);
   }
   strcpy(sge_cell, token);
   
   /* get the qmaster port */
   token = sge_strtok_r(NULL, NULL, &url_ctx);

   if (token == NULL ) {
      answer_list_add_sprintf(alpp, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "invalid url, qmaster_port not found");
      sge_free_saved_vars(url_ctx);
      DRETURN(NULL);
   }
   strcpy(sge_qmaster_port, token);
   
   sge_qmaster_p = atoi(sge_qmaster_port);
   
   if (sge_qmaster_p <= 0 ) {
      answer_list_add_sprintf(alpp, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "invalid url, invalid sge_qmaster_port port %s", sge_qmaster_port);
      sge_free_saved_vars(url_ctx);
      DRETURN(NULL);
   }

   sge_free_saved_vars(url_ctx);
   
   /* TODO we need a way to define the execd port */
   
   ret = sge_gdi_ctx_class_create(prog_number, component_name, username, NULL,
                                  sge_root, sge_cell, sge_qmaster_p, sge_execd_p, alpp);
   
   DRETURN(ret); 
}


static void sge_gdi_ctx_destroy(void *theState)
{
   sge_gdi_ctx_t *s = (sge_gdi_ctx_t *)theState;
   cl_com_handle_t *handle = cl_com_get_handle(s->component_name, 0);

   DENTER(TOP_LAYER, "sge_gdi_ctx_destroy");

   if (handle != NULL ) {
      cl_commlib_shutdown_handle(handle, CL_TRUE);
   }
   sge_env_state_class_destroy(&(s->sge_env_state_obj));
   sge_prog_state_class_destroy(&(s->sge_prog_state_obj));
   sge_path_state_class_destroy(&(s->sge_path_state_obj));
   sge_bootstrap_state_class_destroy(&(s->sge_bootstrap_state_obj));
   sge_free(s->master);
   sge_free(s->username);
   sge_free(s->groupname);
   sge_free(s->component_name);
   sge_free(s->component_username);
   sge_free(s->ssl_certificate);
   sge_free(s->ssl_private_key);
   sge_error_class_destroy(&(s->eh));
   sge_free((char*)s);

   DRETURN_VOID;
}

static int sge_gdi_ctx_class_connect(sge_gdi_ctx_class_t *thiz) 
{
   
   int ret = 0;
   bool is_alive_check = true;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class_connect");
   
   /*
   ** TODO: must contain similar functionality as sge_gdi_setup
   */

   ret = sge_gdi_ctx_class_prepare_enroll(thiz);

   /* check if master is alive */
   if (ret == CL_RETVAL_OK && is_alive_check) {
      const char *master = thiz->get_master(thiz, true);
      DPRINTF(("thiz->get_master(thiz) = %s\n", master)); 
      ret = thiz->is_alive(thiz);
   }

   DRETURN(ret);
}

static int sge_gdi_ctx_class_prepare_enroll(sge_gdi_ctx_class_t *thiz) {
   
   sge_gdi_ctx_t *gdi_ctx = (sge_gdi_ctx_t *)thiz->sge_gdi_ctx_handle;
   sge_env_state_class_t* sge_env = thiz->get_sge_env_state(thiz);
   sge_prog_state_class_t* prog_state = thiz->get_sge_prog_state(thiz);
   sge_path_state_class_t* path_state = thiz->get_sge_path_state(thiz);
   sge_bootstrap_state_class_t * bootstrap_state = thiz->get_sge_bootstrap_state(thiz);
   cl_host_resolve_method_t resolve_method = CL_SHORT;
   cl_framework_t  communication_framework = CL_CT_TCP;
   cl_com_handle_t* handle = NULL;
   const char *help = NULL;
   const char *default_domain = NULL;
   int cl_ret = CL_RETVAL_OK;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class_prepare_enroll");

   /* context setup is complete => setup the commlib
   ** TODO:
   ** there is a problem in qsub if it is used with CL_RW_THREAD, the signaling in qsub -sync
   ** makes qsub hang
   */
   
   if (cl_com_setup_commlib_complete() == CL_FALSE) {
      switch (thiz->get_who(thiz)) {
         case QMASTER:
         case QMON:
         case DRMAA:
         case SCHEDD:
            INFO((SGE_EVENT,MSG_GDI_MULTI_THREADED_STARTUP));
            cl_ret = cl_com_setup_commlib(CL_RW_THREAD, CL_LOG_OFF, sge_gdi_ctx_log_flush_func);
            break;
         default:
            INFO((SGE_EVENT,MSG_GDI_SINGLE_THREADED_STARTUP));
            cl_ret = cl_com_setup_commlib(CL_NO_THREAD, CL_LOG_OFF, sge_gdi_ctx_log_flush_func);
      }
      if (cl_ret != CL_RETVAL_OK) {
         sge_gdi_ctx_class_error(thiz, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, 
                            "cl_com_setup_commlib failed: %s", cl_get_error_text(cl_ret));
         DRETURN(cl_ret);
      }
   }

   /* set the alias file */
   cl_ret = cl_com_set_alias_file((char*)path_state->get_alias_file(path_state));
   if (cl_ret != CL_RETVAL_OK && cl_ret != ctx_get_last_commlib_error(thiz)) {
      sge_gdi_ctx_class_error(thiz, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, 
                         "cl_com_set_alias_file failed: %s", cl_get_error_text(cl_ret));
      DRETURN(cl_ret);
   }

   /* setup the resolve method */
      
   if( bootstrap_state->get_ignore_fqdn(bootstrap_state) == false ) {
      resolve_method = CL_LONG;
   }
   if ((help = bootstrap_state->get_default_domain(bootstrap_state)) != NULL) {
      if (SGE_STRCASECMP(help, NONE_STR) != 0) {
         default_domain = help;
      }
   }
   
   cl_ret = cl_com_set_resolve_method(resolve_method, (char*)default_domain);
   if (cl_ret != CL_RETVAL_OK && cl_ret != ctx_get_last_commlib_error(thiz)) {
      sge_gdi_ctx_class_error(thiz, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, 
                         "cl_com_set_resolve_method failed: %s", cl_get_error_text(cl_ret));
      DRETURN(cl_ret);
   }

   /*
   ** reresolve qualified hostname with use of host aliases 
   ** (corresponds to reresolve_me_qualified_hostname)
   */
   cl_ret = thiz->reresolve_qualified_hostname(thiz);
   if (cl_ret != CL_RETVAL_OK && cl_ret != ctx_get_last_commlib_error(thiz)) { 
      sge_gdi_ctx_class_error(thiz, STATUS_EUNKNOWN, ANSWER_QUALITY_WARNING, 
                         "reresolve hostname failed: %s", cl_get_error_text(cl_ret));
      DRETURN(cl_ret);
   } 
   
   
   /* 
   ** TODO set a general_communication_error  
   */
   cl_ret = cl_com_set_error_func(general_communication_error);
   if (cl_ret != CL_RETVAL_OK && cl_ret != ctx_get_last_commlib_error(thiz)) {
      sge_gdi_ctx_class_error(thiz, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, 
                         "cl_com_set_error_func failed: %s", cl_get_error_text(cl_ret));
      DRETURN(cl_ret);
   }
   
   /* TODO set tag name function */
   cl_ret = cl_com_set_tag_name_func(sge_dump_message_tag);
   if (cl_ret != CL_RETVAL_OK && cl_ret != ctx_get_last_commlib_error(thiz)) {
      sge_gdi_ctx_class_error(thiz, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, 
                         "cl_com_set_tag_name_func failed: %s", cl_get_error_text(cl_ret));
      DRETURN(cl_ret);
   }
   
#ifdef DEBUG_CLIENT_SUPPORT
   /* set debug client callback function to rmon's debug client callback */
   cl_ret = cl_com_set_application_debug_client_callback_func(rmon_debug_client_callback);
   if (cl_ret != CL_RETVAL_OK && cl_ret != ctx_get_last_commlib_error(thiz)) {
      sge_gdi_ctx_class_error(thiz, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, 
                         "cl_com_set_application_debug_client_callback_func failed: %s", cl_get_error_text(cl_ret));
      DRETURN(cl_ret);
   }
#endif

   handle = thiz->get_com_handle(thiz);
   if (handle == NULL) {
      /* handle does not exist, create one */
      
      int me_who = thiz->get_who(thiz);
      const char *progname = thiz->get_progname(thiz);
      const char *master = thiz->get_master(thiz, true);
      const char *qualified_hostname = thiz->get_qualified_hostname(thiz);
      u_long32 sge_qmaster_port = thiz->get_sge_qmaster_port(thiz); /* TODO: reresolving of port from /etc/services etc. */
      u_long32 sge_execd_port = thiz->get_sge_execd_port(thiz);
      int my_component_id = 0; /* 1 for daemons, 0=automatical for clients */
      
      if (master == NULL && !(me_who == QMASTER)) { 
         DRETURN(CL_RETVAL_UNKNOWN);
      }    

      /*
      ** CSP initialize
      */
      if (strcasecmp(bootstrap_state->get_security_mode(bootstrap_state), "csp") == 0 ) {
         cl_ssl_setup_t *sec_ssl_setup_config = NULL;
         cl_ssl_cert_mode_t ssl_cert_mode = CL_SSL_PEM_FILE;
         sge_csp_path_class_t *sge_csp = sge_csp_path_class_create(sge_env, prog_state, gdi_ctx->eh);

         if (thiz->get_certificate(thiz) != NULL) {
            ssl_cert_mode = CL_SSL_PEM_BYTE;
            sge_csp->set_cert_file(sge_csp, thiz->get_certificate(thiz));
            sge_csp->set_key_file(sge_csp, thiz->get_private_key(thiz));
         }   
         sge_csp->dprintf(sge_csp);

         communication_framework = CL_CT_SSL;
         cl_ret = cl_com_create_ssl_setup(&sec_ssl_setup_config,
                                       ssl_cert_mode,
                                       CL_SSL_v23,                                   /* ssl_method           */
                                       (char*)sge_csp->get_CA_cert_file(sge_csp),    /* ssl_CA_cert_pem_file */
                                       (char*)sge_csp->get_CA_key_file(sge_csp),     /* ssl_CA_key_pem_file  */
                                       (char*)sge_csp->get_cert_file(sge_csp),       /* ssl_cert_pem_file    */
                                       (char*)sge_csp->get_key_file(sge_csp),        /* ssl_key_pem_file     */
                                       (char*)sge_csp->get_rand_file(sge_csp),       /* ssl_rand_file        */
                                       (char*)sge_csp->get_reconnect_file(sge_csp),  /* ssl_reconnect_file   */
                                       (char*)sge_csp->get_crl_file(sge_csp),        /* ssl_crl_file         */
                                       sge_csp->get_refresh_time(sge_csp),           /* ssl_refresh_time     */
                                       (char*)sge_csp->get_password(sge_csp),        /* ssl_password         */
                                       sge_csp->get_verify_func(sge_csp));           /* ssl_verify_func (cl_ssl_verify_func_t)  */
         if (cl_ret != CL_RETVAL_OK && cl_ret != ctx_get_last_commlib_error(thiz)) {
            DPRINTF(("return value of cl_com_create_ssl_setup(): %s\n", cl_get_error_text(cl_ret)));
            sge_gdi_ctx_class_error(thiz, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, MSG_GDI_CANT_CONNECT_HANDLE_SSUUS, 
                               qualified_hostname,
                               progname, 0, 
                               sge_qmaster_port,
                               cl_get_error_text(cl_ret));
            sge_csp_path_class_destroy(&sge_csp);
            DRETURN(cl_ret);
         }

         /*
         ** set the CSP credential info into commlib
         */
         cl_ret = cl_com_specify_ssl_configuration(sec_ssl_setup_config);
         if (cl_ret != CL_RETVAL_OK && cl_ret != ctx_get_last_commlib_error(thiz)) {
            DPRINTF(("return value of cl_com_specify_ssl_configuration(): %s\n", cl_get_error_text(cl_ret)));
            sge_gdi_ctx_class_error(thiz, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, MSG_GDI_CANT_CONNECT_HANDLE_SSUUS, 
                               (char*)thiz->get_component_name(thiz), 0,
                               sge_qmaster_port,
                               cl_get_error_text(cl_ret));
            sge_csp_path_class_destroy(&sge_csp);
            cl_com_free_ssl_setup(&sec_ssl_setup_config);
            DRETURN(cl_ret);
         }
         sge_csp_path_class_destroy(&sge_csp);
         cl_com_free_ssl_setup(&sec_ssl_setup_config);
      }

      if ( me_who == QMASTER ||
           me_who == EXECD   ||
           me_who == SCHEDD  || 
           me_who == SHADOWD ) {
         my_component_id = 1;   
      }

      switch (me_who) {

         case EXECD:
            /* add qmaster as known endpoint */
            DPRINTF(("re-read actual qmaster file (prepare_enroll)\n"));
            cl_com_append_known_endpoint_from_name((char*)master, 
                                                   (char*)prognames[QMASTER],
                                                   1 ,
                                                   sge_qmaster_port,
                                                   CL_CM_AC_DISABLED ,
                                                   CL_TRUE);
            handle = cl_com_create_handle(&cl_ret, 
                                          communication_framework, 
                                          CL_CM_CT_MESSAGE, 
                                          CL_TRUE,
                                          sge_execd_port, 
                                          CL_TCP_DEFAULT,
                                          (char*)thiz->get_component_name(thiz),
                                          my_component_id, 
                                          1, 
                                          0);
            cl_com_set_auto_close_mode(handle, CL_CM_AC_ENABLED);
            if (handle == NULL) {
               if (cl_ret != CL_RETVAL_OK && cl_ret != ctx_get_last_commlib_error(thiz)) {
                  /*
                  ** TODO: eh error handler does no logging
                  */
                  ERROR((SGE_EVENT, MSG_GDI_CANT_GET_COM_HANDLE_SSUUS, 
                                    qualified_hostname,
                                    (char*)thiz->get_component_name(thiz),
                                    sge_u32c(my_component_id), 
                                    sge_u32c(sge_execd_port),
                                    cl_get_error_text(cl_ret)));
               }
            }
            break;

         case QMASTER:
            DPRINTF(("creating QMASTER handle\n"));
            cl_com_append_known_endpoint_from_name((char*)master, 
                                                   (char*) prognames[QMASTER],
                                                   1,
                                                   sge_qmaster_port,
                                                   CL_CM_AC_DISABLED ,
                                                   CL_TRUE);

            handle = cl_com_create_handle(&cl_ret, 
                                          communication_framework, 
                                          CL_CM_CT_MESSAGE, /* message based tcp communication */
                                          CL_TRUE, 
                                          sge_qmaster_port, /* create service on qmaster port */
                                          CL_TCP_DEFAULT,   /* use standard connect mode */
                                          (char*)thiz->get_component_name(thiz), 
                                          my_component_id,  /* this endpoint is called "qmaster" 
                                                               and has id 1 */
                                          1, 
                                          0); /* select timeout is set to 1 second 0 usec */
            if (handle == NULL) {
               if (cl_ret != CL_RETVAL_OK && cl_ret != ctx_get_last_commlib_error(thiz)) {   
                  /*
                  ** TODO: eh error handler does no logging
                  */
                  ERROR((SGE_EVENT, MSG_GDI_CANT_GET_COM_HANDLE_SSUUS, 
                                       qualified_hostname,
                                       (char*)thiz->get_component_name(thiz),
                                       sge_u32c(my_component_id), 
                                       sge_u32c(sge_qmaster_port),
                                       cl_get_error_text(cl_ret)));
               }
            } else {
               /* TODO: remove reresolving here */
               int alive_back = 0;
               char act_resolved_qmaster_name[CL_MAXHOSTLEN]; 
               cl_com_set_synchron_receive_timeout(handle, 5);
               /* TODO: reresolve master */
               master = thiz->get_master(thiz, true);
               
               if (master != NULL) {
                  /* TODO: sge_hostcmp uses implicitly bootstrap state info */
                  /* check a running qmaster on different host */
                  if (getuniquehostname(master, act_resolved_qmaster_name, 0) == CL_RETVAL_OK &&
                        sge_hostcmp(act_resolved_qmaster_name, qualified_hostname) != 0) {
                     DPRINTF(("act_qmaster file contains host "SFQ" which doesn't match local host name "SFQ"\n",
                              master, qualified_hostname));

                     cl_com_set_error_func(NULL);

                     alive_back = thiz->is_alive(thiz);
                     cl_ret = cl_com_set_error_func(general_communication_error);
                     if (cl_ret != CL_RETVAL_OK) {
                        ERROR((SGE_EVENT, cl_get_error_text(cl_ret)) );
                     }

                     if (alive_back == CL_RETVAL_OK && getenv("SGE_TEST_HEARTBEAT_TIMEOUT") == NULL ) {
                        CRITICAL((SGE_EVENT, MSG_GDI_MASTER_ON_HOST_X_RUNINNG_TERMINATE_S, master));
                        /* TODO: remove !!! */
                        SGE_EXIT(NULL, 1);
                     } else {
                        DPRINTF(("qmaster on host "SFQ" is down\n", master));
                     }
                  } else {
                     DPRINTF(("act_qmaster file contains local host name\n"));
                  }
               } else {
                  DPRINTF(("skipping qmaster alive check because act_qmaster is not availabe\n"));
               }
            }
            break;

         case QMON:
            DPRINTF(("creating QMON GDI handle\n"));
            handle = cl_com_create_handle(&cl_ret, 
                                          communication_framework, 
                                          CL_CM_CT_MESSAGE, 
                                          CL_FALSE, 
                                          sge_qmaster_port, 
                                          CL_TCP_DEFAULT,
                                          (char*)thiz->get_component_name(thiz), 
                                          my_component_id, 
                                          1, 
                                          0);
            cl_com_set_auto_close_mode(handle, CL_CM_AC_ENABLED);
            if (handle == NULL) {
               if (cl_ret != CL_RETVAL_OK && cl_ret != ctx_get_last_commlib_error(thiz)) {
                  ERROR((SGE_EVENT, MSG_GDI_CANT_CONNECT_HANDLE_SSUUS, 
                                       qualified_hostname,
                                       (char*)thiz->get_component_name(thiz),
                                       sge_u32c(my_component_id), 
                                       sge_u32c(sge_qmaster_port),
                                       cl_get_error_text(cl_ret)));
               }
            }
            break;

         default:
            /* this is for "normal" gdi clients of qmaster */
            DPRINTF(("creating %s GDI handle\n", thiz->get_component_name(thiz)));
            handle = cl_com_create_handle(&cl_ret, 
                                          communication_framework, 
                                          CL_CM_CT_MESSAGE, 
                                          CL_FALSE, 
                                          sge_qmaster_port, 
                                          CL_TCP_DEFAULT,
                                          (char*)thiz->get_component_name(thiz), 
                                          my_component_id, 
                                          1, 
                                          0);
            if (handle == NULL) {
/*             if (cl_ret != CL_RETVAL_OK && cl_ret != ctx_get_last_commlib_error(thiz)) { */
                  sge_gdi_ctx_class_error(thiz, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, 
                                     MSG_GDI_CANT_CONNECT_HANDLE_SSUUS, 
                                     thiz->get_qualified_hostname(thiz), 
                                     thiz->get_component_name(thiz), 
                                     sge_u32c(my_component_id),
                                     sge_u32c(sge_qmaster_port),
                                     cl_get_error_text(cl_ret));
/*             }        */
            }
            break;
      }
      ctx_set_last_commlib_error(thiz, cl_ret);
   }

#ifdef DEBUG_CLIENT_SUPPORT
   /* set rmon callback for message printing (after handle creation) */
   rmon_set_print_callback(gdi_rmon_print_callback_function);
#endif

   if ((thiz->get_who(thiz) == QMASTER) && (getenv("SGE_TEST_SOCKET_BIND") != NULL)) {
      /* this is for testsuite socket bind test (issue 1096 ) */
         struct timeval now;
         gettimeofday(&now,NULL);
     
         /* if this environment variable is set, we wait 15 seconds after 
            communication lib setup */
         DPRINTF(("waiting for 60 seconds, because environment SGE_TEST_SOCKET_BIND is set\n"));
         while ( handle != NULL && now.tv_sec - handle->start_time.tv_sec  <= 60 ) {
            DPRINTF(("timeout: "sge_U32CFormat"\n",sge_u32c(now.tv_sec - handle->start_time.tv_sec)));
            cl_commlib_trigger(handle, 1);
            gettimeofday(&now,NULL);
         }
         DPRINTF(("continue with setup\n"));
   }
   DRETURN(cl_ret);   
}

static int sge_gdi_ctx_class_is_alive(sge_gdi_ctx_class_t *thiz) 
{
   cl_com_SIRM_t* status = NULL;
   int cl_ret = CL_RETVAL_OK;
   cl_com_handle_t *handle = thiz->get_com_handle(thiz);

   /* TODO */
   const char* comp_name = prognames[QMASTER];
   const char* comp_host = thiz->get_master(thiz, false);
   int         comp_id   = 1;
 
   DENTER(TOP_LAYER, "sge_gdi_ctx_class_is_alive");
   
   if (handle == NULL) {
      sge_gdi_ctx_class_error(thiz, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR,
                "handle not found %s:0", thiz->get_component_name(thiz));
      DRETURN(CL_RETVAL_PARAMS);
   }

   DPRINTF(("to->comp_host, to->comp_name, to->comp_id: %s/%s/%d\n", comp_host?comp_host:"", comp_name?comp_name:"", comp_id));
   cl_ret = cl_commlib_get_endpoint_status(handle, (char*)comp_host, (char*)comp_name, comp_id, &status);
   if (cl_ret != CL_RETVAL_OK) {
      sge_gdi_ctx_class_error(thiz, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR,
                "cl_commlib_get_endpoint_status failed: "SFQ, cl_get_error_text(cl_ret));
   } else {
      DEBUG((SGE_EVENT,MSG_GDI_QMASTER_STILL_RUNNING));   
   }

   if (status != NULL) {
      DEBUG((SGE_EVENT,MSG_GDI_ENDPOINT_UPTIME_UU, sge_u32c( status->runtime) , 
             sge_u32c( status->application_status) ));
      cl_com_free_sirm_message(&status);
   }
 
   DRETURN(cl_ret);
}

static lList* sge_gdi_ctx_class_gdi(sge_gdi_ctx_class_t *thiz, int target, int cmd, lList **lpp,
                                 lCondition *where, lEnumeration *what)
{
   lList *alp = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class_gdi");


   /* thiz->dprintf(thiz); */
   
   alp = sge_gdi2(thiz, target, cmd, lpp, where, what);

   DRETURN(alp);

}

static int sge_gdi_ctx_class_gdi_multi(sge_gdi_ctx_class_t* thiz, lList **alpp, int mode, u_long32 target, u_long32 cmd,
                  lList **lpp, lCondition *cp, lEnumeration *enp, lList **malpp, 
                  state_gdi_multi *state, bool do_copy) 
{
   int id = 0;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class_gdi_multi");

   /* thiz->dprintf(thiz); */
   
   id = sge_gdi2_multi(thiz, alpp, mode, target, cmd, lpp, cp, enp, malpp, state, do_copy); 

   DRETURN(id);

}

static int sge_gdi_ctx_class_gdi_multi_sync(sge_gdi_ctx_class_t* thiz, 
                                            lList **alpp, 
                                            int mode, 
                                            u_long32 target, 
                                            u_long32 cmd,
                                            lList **lpp, 
                                            lCondition *cp, 
                                            lEnumeration *enp, 
                                            lList **malpp, 
                                            state_gdi_multi *state, 
                                            bool do_copy,
                                            bool do_sync) 
{
   int id = 0;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class_gdi_multi_sync");

   /* thiz->dprintf(thiz); */
   
   id = sge_gdi2_multi_sync(thiz, alpp, mode, target, cmd, lpp, cp, enp, malpp, state, do_copy, do_sync); 

   DRETURN(id);

}

static bool sge_gdi_ctx_class_gdi_receive_multi_async(sge_gdi_ctx_class_t* thiz, 
                                            sge_gdi_request **answerp, 
                                            lList **malpp, 
                                            bool is_sync) 
{
   bool ret = false;

   DENTER(TOP_LAYER, "sge_gdi_ctx_class_gdi_receive_multi_async");

   /* thiz->dprintf(thiz); */
   
   ret = gdi2_receive_multi_async(thiz, answerp, malpp, is_sync); 

   DRETURN(ret);

}


static lList* sge_gdi_ctx_class_gdi_tsm(sge_gdi_ctx_class_t *thiz, const char *schedd_name, const char *cell)
{
   lList *alp = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class_gdi_tsm");

   alp = gdi2_tsm(thiz, schedd_name, cell);

   DRETURN(alp);

}

static lList* sge_gdi_ctx_class_gdi_kill(sge_gdi_ctx_class_t *thiz, lList *id_list, const char *cell, 
                                          u_long32 option_flags, u_long32 action_flag)
{
   lList *alp = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class_gdi_kill");

   alp = gdi2_kill(thiz, id_list, cell, option_flags, action_flag);

   DRETURN(alp);

}

static bool sge_gdi_ctx_class_gdi_check_permission(sge_gdi_ctx_class_t *thiz, lList **alpp, int option)
{
   bool ret = false;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class_gdi_check_permission");

   ret = sge_gdi2_check_permission(thiz, alpp, option);

   DRETURN(ret);

}

static bool sge_gdi_ctx_class_gdi_get_mapping_name(sge_gdi_ctx_class_t *thiz, const char *requestedHost, char *buf, int buflen)
{
   bool ret = false;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class_gdi_get_mapping_name");

   ret = sge_gdi2_get_mapping_name(thiz, requestedHost, buf, buflen);

   DRETURN(ret);

}

static void sge_gdi_ctx_class_dprintf(sge_gdi_ctx_class_t *ctx)
{
   DENTER(TOP_LAYER, "sge_gdi_ctx_class_dprintf");

   if (ctx == NULL) {
      DRETURN_VOID;
   }   
   DPRINTF(("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n"));   
      
   (ctx->get_sge_env_state(ctx))->dprintf(ctx->get_sge_env_state(ctx)); 
   (ctx->get_sge_prog_state(ctx))->dprintf(ctx->get_sge_prog_state(ctx)); 
   (ctx->get_sge_path_state(ctx))->dprintf(ctx->get_sge_path_state(ctx)); 
   (ctx->get_sge_bootstrap_state(ctx))->dprintf(ctx->get_sge_bootstrap_state(ctx)); 

   DPRINTF(("master: %s\n", ctx->get_master(ctx, false)));
   DPRINTF(("uid/username: %d/%s\n", (int) ctx->get_uid(ctx), ctx->get_username(ctx)));
   DPRINTF(("gid/groupname: %d/%s\n", (int) ctx->get_gid(ctx), ctx->get_groupname(ctx)));

   DPRINTF(("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n"));   

   DRETURN_VOID;
}


/** --------- getter/setter ------------------------------------------------- */
static cl_com_handle_t* get_com_handle(sge_gdi_ctx_class_t *thiz) 
{
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   return cl_com_get_handle(es->component_name, 0);
}
   
static sge_env_state_class_t* get_sge_env_state(sge_gdi_ctx_class_t *thiz) 
{
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   return es->sge_env_state_obj;
}

static sge_prog_state_class_t* get_sge_prog_state(sge_gdi_ctx_class_t *thiz) 
{
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   return es->sge_prog_state_obj;
}

static sge_path_state_class_t* get_sge_path_state(sge_gdi_ctx_class_t *thiz) 
{
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   return es->sge_path_state_obj;
}

static sge_bootstrap_state_class_t* get_sge_bootstrap_state(sge_gdi_ctx_class_t *thiz)
{
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   return es->sge_bootstrap_state_obj;
}

static const char* get_master(sge_gdi_ctx_class_t *thiz, bool reread) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   sge_path_state_class_t* path_state = thiz->get_sge_path_state(thiz);
   sge_error_class_t *eh = es ? es->eh : NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_master");
   
   if (es->master == NULL || reread) {
      char err_str[SGE_PATH_MAX+128];
      char master_name[CL_MAXHOSTLEN];

      if (get_qm_name(master_name, path_state->get_act_qmaster_file(path_state), err_str) == -1) {         
         if (eh != NULL) {
            eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, MSG_GDI_READMASTERNAMEFAILED_S, err_str);
         }
         DRETURN(NULL);
      } 
      DPRINTF(("(re-)reading act_qmaster file. Got master host \"%s\"\n", master_name));
      /*
      ** TODO: thread locking needed here ?
      */ 
      es->master = sge_strdup(es->master,master_name);
   }   
   DRETURN(es->master);
}

static const char* get_qualified_hostname(sge_gdi_ctx_class_t *thiz) {
   sge_prog_state_class_t* prog_state = thiz->get_sge_prog_state(thiz);
   const char *qualified_hostname = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_progname");
   qualified_hostname = prog_state->get_qualified_hostname(prog_state); 
   DRETURN(qualified_hostname);
}

static const char* get_unqualified_hostname(sge_gdi_ctx_class_t *thiz) {
   sge_prog_state_class_t* prog_state = thiz->get_sge_prog_state(thiz);
   const char *unqualified_hostname = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_unqualified_hostname");
   unqualified_hostname = prog_state->get_unqualified_hostname(prog_state); 
   DRETURN(unqualified_hostname);
}

static const char* get_component_name(sge_gdi_ctx_class_t *thiz) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   const char* ret = NULL;
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_component_name");
   ret = es->component_name;
   DRETURN(ret);
}   
static const char* get_progname(sge_gdi_ctx_class_t *thiz) {
   sge_prog_state_class_t* prog_state = thiz->get_sge_prog_state(thiz);
   const char *progname = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_progname");
   progname = prog_state->get_sge_formal_prog_name(prog_state); 
   DRETURN(progname);
}

static u_long32 get_who(sge_gdi_ctx_class_t *thiz) {
   sge_prog_state_class_t* prog_state = thiz->get_sge_prog_state(thiz);
   u_long32 prognumber = 0;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_who");
   prognumber = prog_state->get_who(prog_state); 
   DRETURN(prognumber);
}

static bool is_daemonized(sge_gdi_ctx_class_t *thiz) {
   sge_prog_state_class_t* prog_state = thiz->get_sge_prog_state(thiz);
   bool is_daemonized = false;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->is_daemonized");
   is_daemonized = (prog_state->get_daemonized(prog_state)) ? true : false;
   DRETURN(is_daemonized);
}

static void set_daemonized(sge_gdi_ctx_class_t *thiz, bool daemonized) {
   sge_prog_state_class_t* prog_state = thiz->get_sge_prog_state(thiz);
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->set_daemonized");
   prog_state->set_daemonized(prog_state, daemonized);
   DRETURN_VOID;
}

static bool get_job_spooling(sge_gdi_ctx_class_t *thiz) {
   sge_bootstrap_state_class_t* bootstrap_state = thiz->get_sge_bootstrap_state(thiz);
   bool job_spooling = true;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_job_spooling");
   job_spooling = bootstrap_state->get_job_spooling(bootstrap_state);
   DRETURN(job_spooling);
}

static void set_job_spooling(sge_gdi_ctx_class_t *thiz, bool job_spooling) {
   sge_bootstrap_state_class_t* bootstrap_state = thiz->get_sge_bootstrap_state(thiz);

   DENTER(TOP_LAYER, "sge_gdi_ctx_class->set_job_spooling");
   bootstrap_state->set_job_spooling(bootstrap_state, job_spooling);
   DRETURN_VOID;
}

static const char* get_spooling_method(sge_gdi_ctx_class_t *thiz) {
   sge_bootstrap_state_class_t* bootstrap_state = thiz->get_sge_bootstrap_state(thiz);
   const char *spooling_method = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_spooling_method");
   spooling_method = bootstrap_state->get_spooling_method(bootstrap_state);
   DRETURN(spooling_method);
}

static const char* get_spooling_lib(sge_gdi_ctx_class_t *thiz) {
   sge_bootstrap_state_class_t* bootstrap_state = thiz->get_sge_bootstrap_state(thiz);
   const char *spooling_lib = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_spooling_lib");
   spooling_lib = bootstrap_state->get_spooling_lib(bootstrap_state);
   DRETURN(spooling_lib);
}

static const char* get_spooling_params(sge_gdi_ctx_class_t *thiz) {
   sge_bootstrap_state_class_t* bootstrap_state = thiz->get_sge_bootstrap_state(thiz);
   const char *spooling_params = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_spooling_params");
   spooling_params = bootstrap_state->get_spooling_params(bootstrap_state);
   DRETURN(spooling_params);
}

static u_long32 get_gdi_thread_count(sge_gdi_ctx_class_t *thiz) {
   sge_bootstrap_state_class_t* bootstrap_state = thiz->get_sge_bootstrap_state(thiz);
   u_long32 gdi_thread_count = 0;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_gdi_thread_count");
   gdi_thread_count = bootstrap_state->get_gdi_thread_count(bootstrap_state);
   DRETURN(gdi_thread_count);
}

static sge_exit_func_t get_exit_func(sge_gdi_ctx_class_t *thiz) {
   sge_prog_state_class_t* prog_state = thiz->get_sge_prog_state(thiz);
   sge_exit_func_t exit_func = NULL;
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->set_exit_func");
   exit_func = prog_state->get_exit_func(prog_state);
   DRETURN(exit_func);
}

static void set_exit_func(sge_gdi_ctx_class_t *thiz, sge_exit_func_t exit_func) {
   sge_prog_state_class_t* prog_state = thiz->get_sge_prog_state(thiz);
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->set_exit_func");
   prog_state->set_exit_func(prog_state, exit_func);
   DRETURN_VOID;
}

static void set_private_key(sge_gdi_ctx_class_t *thiz, const char *pkey) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->set_private_key");

   if ( es->ssl_private_key != NULL ) {
      FREE(es->ssl_private_key);
   }
   es->ssl_private_key = pkey ? strdup(pkey): NULL;

   DRETURN_VOID;
}

static const char* get_private_key(sge_gdi_ctx_class_t *thiz) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   const char *pkey = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_private_key");
   pkey = es->ssl_private_key;
   DRETURN(pkey);
}


static void set_certificate(sge_gdi_ctx_class_t *thiz, const char *cert) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;

   DENTER(TOP_LAYER, "sge_gdi_ctx_class->set_certificate");

   if ( es->ssl_certificate != NULL ) {
      FREE(es->ssl_certificate);
   }
   es->ssl_certificate = cert ? strdup(cert) : NULL;

   DRETURN_VOID;
}

static const char* get_certificate(sge_gdi_ctx_class_t *thiz) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   const char *cert = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_certificate");
   cert = es->ssl_certificate;
   DRETURN(cert);
}


static const char* get_default_cell(sge_gdi_ctx_class_t *thiz) {
   sge_prog_state_class_t* prog_state = thiz->get_sge_prog_state(thiz);
   const char *default_cell = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_default_cell");
   default_cell = prog_state->get_default_cell(prog_state); 
   DRETURN(default_cell);
}


static const char* get_cell_root(sge_gdi_ctx_class_t *thiz) {
   sge_path_state_class_t* path_state = thiz->get_sge_path_state(thiz);
   const char *cell_root = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_cell_root");
   cell_root = path_state->get_cell_root(path_state); 
   DRETURN(cell_root);
}

static const char* get_sge_root(sge_gdi_ctx_class_t *thiz) {
   sge_path_state_class_t* path_state = thiz->get_sge_path_state(thiz);
   const char *sge_root = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_ctx_class->get_sge_root");
   sge_root = path_state->get_sge_root(path_state); 
   DRETURN(sge_root);
}


static const char* get_admin_user(sge_gdi_ctx_class_t *thiz) {
   sge_bootstrap_state_class_t * bootstrap_state = thiz->get_sge_bootstrap_state(thiz);
   return bootstrap_state->get_admin_user(bootstrap_state);
}

static const char* get_binary_path(sge_gdi_ctx_class_t *thiz) {
   sge_bootstrap_state_class_t * bootstrap_state = thiz->get_sge_bootstrap_state(thiz);
   return bootstrap_state->get_binary_path(bootstrap_state);
}

static const char* get_qmaster_spool_dir(sge_gdi_ctx_class_t *thiz) {
   sge_bootstrap_state_class_t * bootstrap_state = thiz->get_sge_bootstrap_state(thiz);
   return bootstrap_state->get_qmaster_spool_dir(bootstrap_state);
}

static const char* get_bootstrap_file(sge_gdi_ctx_class_t *thiz) {
   sge_path_state_class_t *path_state = thiz->get_sge_path_state(thiz);
   return path_state->get_bootstrap_file(path_state);
}

static const char* get_act_qmaster_file(sge_gdi_ctx_class_t *thiz) {
   sge_path_state_class_t *path_state = thiz->get_sge_path_state(thiz);
   return path_state->get_act_qmaster_file(path_state);
}

static const char* get_acct_file(sge_gdi_ctx_class_t *thiz) {
   sge_path_state_class_t *path_state = thiz->get_sge_path_state(thiz);
   return path_state->get_acct_file(path_state);
}

static const char* get_reporting_file(sge_gdi_ctx_class_t *thiz) {
   sge_path_state_class_t *path_state = thiz->get_sge_path_state(thiz);
   return path_state->get_reporting_file(path_state);
}


static const char* get_shadow_master_file(sge_gdi_ctx_class_t *thiz) {
   sge_path_state_class_t *path_state = thiz->get_sge_path_state(thiz);
   return path_state->get_shadow_masters_file(path_state);
}


static const char* get_username(sge_gdi_ctx_class_t *thiz) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   return es->username;
}

static const char* get_groupname(sge_gdi_ctx_class_t *thiz) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   return es->groupname;
}


static u_long32 get_sge_qmaster_port(sge_gdi_ctx_class_t *thiz) {
   sge_env_state_class_t *es = thiz->get_sge_env_state(thiz);
   return es->get_sge_qmaster_port(es);
}

static u_long32 get_sge_execd_port(sge_gdi_ctx_class_t *thiz) {
   sge_env_state_class_t *es = thiz->get_sge_env_state(thiz);
   return es->get_sge_execd_port(es);
}

static int ctx_get_last_commlib_error(sge_gdi_ctx_class_t *thiz) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   return es->last_commlib_error;
}

static void ctx_set_last_commlib_error(sge_gdi_ctx_class_t *thiz, int cl_error) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   es->last_commlib_error = cl_error;
}


static uid_t ctx_get_uid(sge_gdi_ctx_class_t *thiz) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   return es->uid;
}

static gid_t ctx_get_gid(sge_gdi_ctx_class_t *thiz) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   return es->gid;
}


static int sge_gdi_ctx_log_flush_func(cl_raw_list_t* list_p) 
{
   int ret_val;
   cl_log_list_elem_t* elem = NULL;

   DENTER(COMMD_LAYER, "sge_gdi_ctx_log_flush_func");

   if (list_p == NULL) {
      DRETURN(CL_RETVAL_LOG_NO_LOGLIST);
   }

   if ((ret_val = cl_raw_list_lock(list_p)) != CL_RETVAL_OK) {
      DRETURN(ret_val);
   }

   while ((elem = cl_log_list_get_first_elem(list_p) ) != NULL) {
      char* param;
      if (elem->log_parameter == NULL) {
         param = "";
      } else {
         param = elem->log_parameter;
      }

      switch(elem->log_type) {
         case CL_LOG_ERROR: 
            if ( log_state_get_log_level() >= LOG_ERR) {
               ERROR((SGE_EVENT,  "%s %-20s=> %s %s\n", elem->log_module_name, elem->log_thread_name, elem->log_message, param ));
            } else {
               printf("%s %-20s=> %s %s\n", elem->log_module_name, elem->log_thread_name, elem->log_message, param);
            }
            break;
         case CL_LOG_WARNING:
            if ( log_state_get_log_level() >= LOG_WARNING) {
               WARNING((SGE_EVENT,"%s %-20s=> %s %s\n", elem->log_module_name, elem->log_thread_name, elem->log_message, param ));
            } else {
               printf("%s %-20s=> %s %s\n", elem->log_module_name, elem->log_thread_name, elem->log_message, param);
            }
            break;
         case CL_LOG_INFO:
            if ( log_state_get_log_level() >= LOG_INFO) {
               INFO((SGE_EVENT,   "%s %-20s=> %s %s\n", elem->log_module_name, elem->log_thread_name, elem->log_message, param ));
            } else {
               printf("%s %-20s=> %s %s\n", elem->log_module_name, elem->log_thread_name, elem->log_message, param);
            }
            break;
         case CL_LOG_DEBUG:
            if ( log_state_get_log_level() >= LOG_DEBUG) { 
               DEBUG((SGE_EVENT,  "%s %-20s=> %s %s\n", elem->log_module_name, elem->log_thread_name, elem->log_message, param ));
            } else {
               printf("%s %-20s=> %s %s\n", elem->log_module_name, elem->log_thread_name, elem->log_message, param);
            }
            break;
         case CL_LOG_OFF:
            break;
      }
      cl_log_list_del_log(list_p);
   }
   
   if ((ret_val = cl_raw_list_unlock(list_p)) != CL_RETVAL_OK) {
      DRETURN(ret_val);
   } 
   DRETURN(CL_RETVAL_OK);
}

/*
** TODO: 
** only helper function to do the setup for clients similar to sge_setup()
*/
int sge_setup2(sge_gdi_ctx_class_t **context, u_long32 progid, lList **alpp)
{
   char  user[128] = "";
   char  group[128] = "";
   const char *sge_root = NULL;
   const char *sge_cell = NULL;
   u_long32 sge_qmaster_port = 0;
   u_long32 sge_execd_port = 0;

   DENTER(TOP_LAYER, "sge_setup2");

   if (context == NULL) {
      answer_list_add_sprintf(alpp, STATUS_ESEMANTIC, ANSWER_QUALITY_CRITICAL, MSG_GDI_CONTEXT_NULL);
      DRETURN(AE_ERROR);
   }

   /*
   ** TODO:
   ** get the environment for now here  -> sge_env_class_t should be enhanced and used instead as input param
   */
   sge_root = getenv("SGE_ROOT");
   if (sge_root == NULL) {
      answer_list_add_sprintf(alpp, STATUS_ESEMANTIC, ANSWER_QUALITY_CRITICAL, MSG_SGEROOTNOTSET);
      DRETURN(AE_ERROR);
   }
   sge_cell = getenv("SGE_CELL")?getenv("SGE_CELL"):DEFAULT_CELL;
   sge_qmaster_port = sge_get_qmaster_port();
   sge_execd_port = sge_get_execd_port();

/* sge_mt_init { */
/*    sge_prof_setup(); */
   uidgid_mt_init();
/*    path_mt_init(); */
/*     bootstrap_mt_init();   */
/*    feature_mt_init(); */
/*    sc_mt_init(); */
/*    obj_mt_init();  */
/*    gdi_mt_init(); */
/* } end sge_mt_init */

   if (sge_uid2user(geteuid(), user, sizeof(user), MAX_NIS_RETRIES)) {
      answer_list_add_sprintf(alpp, STATUS_ESEMANTIC, ANSWER_QUALITY_CRITICAL, MSG_SYSTEM_RESOLVEUSER);
      DRETURN(AE_ERROR);
   }

   if (sge_gid2group(getegid(), group, sizeof(group), MAX_NIS_RETRIES)) {
      answer_list_add_sprintf(alpp, STATUS_ESEMANTIC, ANSWER_QUALITY_CRITICAL, MSG_SYSTEM_RESOLVEGROUP);
      DRETURN(AE_ERROR);
   }

   /* a dynamic eh handler is created */
   *context = sge_gdi_ctx_class_create(progid, prognames[progid], user, group,
                                       sge_root, sge_cell, sge_qmaster_port, sge_execd_port, alpp);

   if (*context == NULL) {
      DRETURN(AE_ERROR);
   }

   /* 
   ** TODO: we set the log state context here 
   **       this should be done more explicitily !!!
   */
   log_state_set_log_context(*context);

   /* 
   ** TODO: bootstrap info is used in cull functions sge_hostcpy
   **       ignore_fqdn, domain_name
   **       Therefore we have to set it into the thread ctx
   */
   sge_gdi_set_thread_local_ctx(*context);

   DRETURN(AE_OK);
}

/*
** TODO: 
** only helper function to do the setup for clients similar to sge_gdi_setup()
*/
int sge_gdi2_setup(sge_gdi_ctx_class_t **context_ref, u_long32 progid, lList **alpp)
{
   sge_gdi_ctx_class_t *ctx = NULL;
   int ret = AE_OK;
   bool alpp_was_null = true;

   DENTER(TOP_LAYER, "sge_gdi2_setup");

#ifdef USE_GDI_STATE 
   if (alpp != NULL && *alpp != NULL) {
     alpp_was_null = false;
   }

   /* TODO: gdi_state_t should be part of context */
   /* initialize libraries */
   pthread_once(&gdi_once_control, gdi_once_init);
   if (gdi_state_get_made_setup()) {
      if (alpp_was_null) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_GDI_ALREADY_SETUP));
      } else {
         answer_list_add_sprintf(alpp, STATUS_EEXIST, ANSWER_QUALITY_WARNING,
                                 MSG_GDI_GDI_ALREADY_SETUP);
      }
      DRETURN(AE_ALREADY_SETUP);
   }
#else   
   if (sge_gdi_ctx_is_setup(ctx)) {
      if (alpp_was_null) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_GDI_ALREADY_SETUP));
      } else {
         answer_list_add_sprintf(alpp, STATUS_EEXIST, ANSWER_QUALITY_WARNING,
                                 MSG_GDI_GDI_ALREADY_SETUP);
      }
      DRETURN(AE_ALREADY_SETUP);
   }
#endif
   ret = sge_setup2(context_ref, progid, alpp); 
   if (ret != AE_OK) {
      DRETURN(ret);
   }

   ctx = *context_ref;
   if (ctx->prepare_enroll(ctx) != CL_RETVAL_OK) {
      sge_gdi_ctx_class_get_errors(ctx, alpp, true);
      DRETURN(AE_QMASTER_DOWN);
   }

#ifdef USE_GDI_STATE
   /* TODO: gdi_state should be part of context */
   gdi_state_set_made_setup(1);
#else   
   sge_gdi_ctx_set_is_setup(ctx, true);
#endif

   DRETURN(AE_OK);
}

static int reresolve_qualified_hostname(sge_gdi_ctx_class_t *thiz) {
   char unique_hostname[CL_MAXHOSTLEN];
   sge_prog_state_class_t* prog_state = thiz->get_sge_prog_state(thiz);
   int ret = CL_RETVAL_OK;

   DENTER(TOP_LAYER, "gdi2_reresolve_qualified_hostname");

   ret=getuniquehostname(prog_state->get_qualified_hostname(prog_state), unique_hostname, 0);
   if( ret != CL_RETVAL_OK ) {
      DRETURN(ret);
   }
   prog_state->set_qualified_hostname(prog_state, unique_hostname);

   DRETURN(ret);
}

