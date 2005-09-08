/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of thiz file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of thiz file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use thiz file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under thiz License is provided on an "AS IS" basis,
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
#include "sge_error_class.h"
#include "sge_profiling.h"
#include "sge_csp_path.h"

#if  1
/* TODO: throw this out asap */
#include "sge_gdi.h"
void gdi_once_init(void);
#endif
#include "sge_gdi_ctx.h"
#include "sge_gdi2.h"

typedef struct {
   sge_env_state_class_t* sge_env_state_obj;
   sge_prog_state_class_t* sge_prog_state_obj;
   sge_path_state_class_t* sge_path_state_obj;
   sge_bootstrap_state_class_t* sge_bootstrap_state_obj;
   cl_com_handle_t* com_handle;
   
   char* master;
   char* username;
   char* groupname;
   uid_t uid;
   gid_t gid;
   
   lList *alp;
   
} sge_gdi_ctx_t;

static bool sge_gdi_ctx_setup(sge_gdi_ctx_class_t *thiz, 
                              int prog_number, 
                              const char* username,
                              const char *sge_root, 
                              const char *sge_cell, 
                              int sge_qmaster_port, 
                              int sge_execd_port,
                              sge_error_class_t *eh);
static void sge_gdi_ctx_destroy(void *theState);

static sge_env_state_class_t* get_sge_env_state(sge_gdi_ctx_class_t *thiz);
static sge_prog_state_class_t* get_sge_prog_state(sge_gdi_ctx_class_t *thiz);
static sge_path_state_class_t* get_sge_path_state(sge_gdi_ctx_class_t *thiz);
static sge_bootstrap_state_class_t* get_sge_bootstrap_state(sge_gdi_ctx_class_t *thiz);
static cl_com_handle_t* get_com_handle(sge_gdi_ctx_class_t *thiz);
static void set_com_handle(sge_gdi_ctx_class_t *thiz, cl_com_handle_t*com_handle);

static const char* get_progname(sge_gdi_ctx_class_t *thiz);
static const char* get_master(sge_gdi_ctx_class_t *thiz);
static const char* get_username(sge_gdi_ctx_class_t *thiz);
static const char* get_cell_root(sge_gdi_ctx_class_t *thiz);
static const char* get_groupname(sge_gdi_ctx_class_t *thiz);
static uid_t ctx_get_uid(sge_gdi_ctx_class_t *thiz);
static gid_t ctx_get_gid(sge_gdi_ctx_class_t *thiz);



static bool sge_gdi_ctx_class_connect(sge_gdi_ctx_class_t *thiz, sge_error_class_t *eh);
static bool sge_gdi_ctx_class_is_alive(sge_gdi_ctx_class_t *thiz, sge_error_class_t* eh);
static lList* sge_gdi_ctx_class_gdi(sge_gdi_ctx_class_t *thiz, int target, int cmd, lList **lpp,
                                 lCondition *where, lEnumeration *what, sge_error_class_t *eh);
static int sge_gdi_ctx_class_gdi_multi(sge_gdi_ctx_class_t* ctx, lList **alpp, int mode, u_long32 target, u_long32 cmd,
                  lList **lp, lCondition *cp, lEnumeration *enp, lList **malpp, 
                  state_gdi_multi *state, bool do_copy);
                  
static int sge_gdi_ctx_log_flush_func(cl_raw_list_t* list_p);

static void sge_gdi_ctx_class_dprintf(sge_gdi_ctx_class_t *ctx);



sge_gdi_ctx_class_t *sge_gdi_ctx_class_create(int prog_number, const char* username, const char *sge_root, const char *sge_cell, int sge_qmaster_port, int sge_execd_port, sge_error_class_t *eh)
{
   sge_gdi_ctx_class_t *ret = (sge_gdi_ctx_class_t *)sge_malloc(sizeof(sge_gdi_ctx_class_t));

   DENTER(GDI_LAYER, "sge_gdi_ctx_class_create");

   if (!ret) {
      eh->error(eh, STATUS_EMALLOC, ANSWER_QUALITY_ERROR, MSG_MEMORY_MALLOCFAILED);
      DEXIT;
      return NULL;
   }

   ret->connect = sge_gdi_ctx_class_connect;
   ret->is_alive = sge_gdi_ctx_class_is_alive;
   ret->gdi = sge_gdi_ctx_class_gdi;
   ret->gdi_multi = sge_gdi_ctx_class_gdi_multi;
   
   ret->get_sge_env_state = get_sge_env_state;
   ret->get_sge_prog_state = get_sge_prog_state;
   ret->get_sge_path_state = get_sge_path_state;
   ret->get_sge_bootstrap_state = get_sge_bootstrap_state;
   ret->get_progname = get_progname;
   ret->get_master = get_master;
   ret->get_username = get_username;
   ret->get_cell_root = get_cell_root;
   ret->get_groupname = get_groupname;
   ret->get_uid = ctx_get_uid;
   ret->get_gid = ctx_get_gid;   
   ret->get_com_handle = get_com_handle;   
   ret->set_com_handle = set_com_handle;
   ret->dprintf = sge_gdi_ctx_class_dprintf;

   ret->sge_gdi_ctx_handle = (sge_gdi_ctx_t*)sge_malloc(sizeof(sge_gdi_ctx_t));
   memset(ret->sge_gdi_ctx_handle, 0, sizeof(sge_gdi_ctx_t));

   if (!ret->sge_gdi_ctx_handle) {
      eh->error(eh, STATUS_EMALLOC, ANSWER_QUALITY_ERROR, MSG_MEMORY_MALLOCFAILED);
      sge_gdi_ctx_class_destroy(&ret);
      DEXIT;
      return NULL;
   }

   if (!sge_gdi_ctx_setup(ret, prog_number, username, sge_root, sge_cell, sge_qmaster_port, sge_execd_port, eh)) {
      sge_gdi_ctx_class_destroy(&ret);
      DEXIT;
      return NULL;
   }

   DEXIT;
   return ret;
}   

void sge_gdi_ctx_class_destroy(sge_gdi_ctx_class_t **pst)
{
   DENTER(GDI_LAYER, "sge_gdi_ctx_class_destroy");

   if (!pst && !*pst) {
      DEXIT;
      return;
   }   
      
   sge_gdi_ctx_destroy((*pst)->sge_gdi_ctx_handle);
   FREE(*pst);
   *pst = NULL;

   DEXIT;
}

static bool sge_gdi_ctx_setup(sge_gdi_ctx_class_t *thiz, int prog_number, const char* username, const char *sge_root, const char *sge_cell, int sge_qmaster_port, int sge_execd_port, sge_error_class_t *eh)
{
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *)thiz->sge_gdi_ctx_handle;

   DENTER(GDI_LAYER, "sge_gdi_ctx_setup");
 
   es->sge_env_state_obj = sge_env_state_class_create(sge_root, sge_cell, sge_qmaster_port, sge_execd_port, eh);
   if (!es->sge_env_state_obj) {
      DEXIT;
      return false;
   }   

   es->sge_prog_state_obj = sge_prog_state_class_create(es->sge_env_state_obj, prog_number, eh);
   if (!es->sge_prog_state_obj) {
      DEXIT;
      return false;
   }   

   es->sge_path_state_obj = sge_path_state_class_create(es->sge_env_state_obj, eh);
   if (!es->sge_path_state_obj) {
      DEXIT;
      return false;
   }

   es->sge_bootstrap_state_obj = sge_bootstrap_state_class_create(es->sge_path_state_obj, eh);
   if (!es->sge_bootstrap_state_obj) {
      DEXIT;
      return false;
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

      if(!pwd) {
         eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "sge_getpwnam failed for username %s", username);
         sge_gdi_ctx_destroy(es);
         DEXIT;
         return false;
      }
      es->uid = pwd->pw_uid;
      es->gid = pwd->pw_gid;
   }
   
#if defined( INTERIX )
   /* Strip Windows domain name from user name */
   {
      char *plus_sign;

      plus_sign = strstr(username, "+");
      if(plus_sign!=NULL) {
         plus_sign++;
         strcpy(username, plus_sign);
      }
   }
#endif
   
   es->username = strdup(username);

   
   if (_sge_gid2group(es->gid, &(es->gid), &(es->groupname), MAX_NIS_RETRIES)) {
      eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, MSG_GDI_GETGRGIDXFAILEDERRORX_U, sge_u32c(es->gid));
      return false;
   }

   DEXIT;
   return true;
}

sge_gdi_ctx_class_t *sge_gdi_ctx_class_create_from_bootstrap(int   prog_number,
                                                             const char* url,
                                                             const char* username,
                                                             const char* credentials,
                                                             sge_error_class_t *eh) 
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
   
   DENTER(GDI_LAYER, "sge_gdi_ctx_class_create_from_bootstrap");

   /* parse the url */
   DPRINTF(("url = %s\n", url));
   sscanf(url, "bootstrap://%s", sge_url);
   DPRINTF(("sge_url = %s\n", sge_url));
   
   /* search for sge_root */
   token = sge_strtok_r(sge_url, "@", &url_ctx);   
   if( token == NULL ) {
      eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "invalid url, sge_root not found");
      sge_free_saved_vars(url_ctx);
      DEXIT;
      return NULL;
   }   
   strcpy(sge_root, token);
   
   /* search for sge_cell */
   token = sge_strtok_r(NULL, ":", &url_ctx);
   
   if( token == NULL ) {
      eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "invalid url, sge_cell not found");
      sge_free_saved_vars(url_ctx);
      DEXIT;
      return NULL;
   }
   strcpy(sge_cell, token);
   
   /* get the qmaster port */
   token = sge_strtok_r(NULL, NULL, &url_ctx);

   if( token == NULL ) {
      eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "invalid url, qmaster_port not found");
      sge_free_saved_vars(url_ctx);
      DEXIT;
      return NULL;
   }
   strcpy(sge_qmaster_port, token);
   
   sge_qmaster_p = atoi(sge_qmaster_port);
   
   if( sge_qmaster_p <= 0 ) {
      eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "invalid url, invalid sge_qmaster_port port %s", sge_qmaster_port);
      sge_free_saved_vars(url_ctx);
      DEXIT;
      return NULL;
   }

   sge_free_saved_vars(url_ctx);
   
   /* TODO we need a way to define the execd port */
   
   ret = sge_gdi_ctx_class_create(prog_number, username, sge_root, sge_cell, sge_qmaster_p, sge_execd_p, eh);
   
   DEXIT;
   return ret; 
}


static void sge_gdi_ctx_destroy(void *theState)
{
   sge_gdi_ctx_t *s = (sge_gdi_ctx_t *)theState;

   DENTER(GDI_LAYER, "sge_gdi_ctx_destroy");

   if (s->com_handle != NULL ) {
      cl_commlib_shutdown_handle(s->com_handle, CL_TRUE);
      s->com_handle = NULL;
   }
   sge_env_state_class_destroy(&(s->sge_env_state_obj));
   sge_prog_state_class_destroy(&(s->sge_prog_state_obj));
   sge_path_state_class_destroy(&(s->sge_path_state_obj));
   sge_bootstrap_state_class_destroy(&(s->sge_bootstrap_state_obj));
   sge_free(s->master);
   sge_free(s->username);
   sge_free((char*)s);

   DEXIT;
}


static bool sge_gdi_ctx_class_connect(sge_gdi_ctx_class_t *thiz, sge_error_class_t *eh) {
   
   sge_env_state_class_t* sge_env = thiz->get_sge_env_state(thiz);
   sge_prog_state_class_t* prog_state = thiz->get_sge_prog_state(thiz);
   sge_path_state_class_t* path_state = thiz->get_sge_path_state(thiz);
   sge_bootstrap_state_class_t * bootstrap_state = thiz->get_sge_bootstrap_state(thiz);
   cl_host_resolve_method_t resolve_method = CL_SHORT;
   cl_framework_t  communication_framework = CL_CT_TCP;
   cl_com_handle_t* handle = NULL;
   int ret = 0;
   
   DENTER(GDI_LAYER, "sge_gdi_ctx_class_connect");

   /* TODO: profiling init, is this possible */
   sge_prof_set_enabled(false);
   sge_prof_setup();
   
   /* context setup is complete => setup the commlib */
   ret = cl_com_setup_commlib(CL_RW_THREAD /* CL_NO_THREAD */, CL_LOG_OFF, sge_gdi_ctx_log_flush_func);
   if (ret != CL_RETVAL_OK) {
      eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "cl_com_setup_commlib failed: %s", cl_get_error_text(ret));
      DEXIT;
      return false;
   }
   

   /* set the alias file */
   ret = cl_com_set_alias_file((char*)path_state->get_alias_file(path_state));
   if (ret != CL_RETVAL_OK) {
      eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "cl_com_set_alias_file failed: %s", cl_get_error_text(ret));
      DEXIT;
      return false;
   }

   /* setup the resolve method */
      
   if( bootstrap_state->get_ignore_fqdn(bootstrap_state) == false ) {
      resolve_method = CL_LONG;
   }
   
   ret = cl_com_set_resolve_method(resolve_method, (char*)bootstrap_state->get_default_domain(bootstrap_state));
   if( ret != CL_RETVAL_OK ) {
      eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "cl_com_set_resolve_method failed: %s", cl_get_error_text(ret));
      DEXIT;
      return false;
   }
   
   /* TODO set a general_communication_error    
   ret = cl_com_set_error_func(general_communication_error);
   if (ret != CL_RETVAL_OK) {
      char buf[1024];
      sprintf(buf, "cl_com_set_error_func failed: %s", cl_get_error_text(ret));
      answer_list_add(alpp, buf, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      return false;
   } */
   
   /* TODO set tag name function 
   ret = cl_com_set_tag_name_func(sge_dump_message_tag);
   if (ret != CL_RETVAL_OK) {
      char buf[1024];
      sprintf(buf, "cl_com_set_tag_name_func failed: %s", cl_get_error_text(ret));
      answer_list_add(alpp, buf, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return false;
   } */
   
#if 0 
   /* 
   ** TODO: does require the id, if several instances are running 
   **       we don't search for an existing handle but create a new one for every connect
   */
   handle = cl_com_get_handle((char*)prog_state->get_sge_formal_prog_name(prog_state), 0);
#endif   
   
   if (handle == NULL) {
      
      /* handle does not exits, create one */
      int commlib_error = 0;   
      
      if( strcasecmp( bootstrap_state->get_security_mode(bootstrap_state), "csp" ) == 0 ) {
         cl_ssl_setup_t *sec_ssl_setup_config;
         sge_csp_path_class_t *sge_csp = sge_csp_path_class_create(sge_env, prog_state, eh);

         communication_framework = CL_CT_SSL;
#if 1         
         ret = cl_com_create_ssl_setup(&sec_ssl_setup_config,
                                       CL_SSL_v23,                            /* ssl_method           */
                                       (char*)sge_csp->get_CA_cert_file(sge_csp),    /* ssl_CA_cert_pem_file */
                                       (char*)sge_csp->get_CA_key_file(sge_csp),     /* ssl_CA_key_pem_file  */
                                       (char*)sge_csp->get_cert_file(sge_csp),       /* ssl_cert_pem_file    */
                                       (char*)sge_csp->get_key_file(sge_csp),        /* ssl_key_pem_file     */
                                       (char*)sge_csp->get_rand_file(sge_csp),       /* ssl_rand_file        */
                                       (char*)sge_csp->get_reconnect_file(sge_csp),  /* ssl_reconnect_file   */
                                       sge_csp->get_refresh_time(sge_csp),    /* ssl_refresh_time     */
                                       (char*)sge_csp->get_password(sge_csp),        /* ssl_password         */
                                       sge_csp->get_verify_func(sge_csp)); /* ssl_verify_func (cl_ssl_verify_func_t)  */
         if ( ret != CL_RETVAL_OK) {
            DPRINTF(("return value of cl_com_create_ssl_setup(): %s\n", cl_get_error_text(commlib_error)));
            eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, MSG_GDI_CANT_CONNECT_HANDLE_SSUUS, 
                prog_state->get_sge_formal_prog_name(prog_state), 0, 
                sge_env->get_sge_qmaster_port(sge_env),
                cl_get_error_text(commlib_error));
            sge_csp_path_class_destroy(&sge_csp);
            DEXIT;
            return false;
         }


         ret = cl_com_specify_ssl_configuration(sec_ssl_setup_config);
         if ( ret != CL_RETVAL_OK) {
            DPRINTF(("return value of cl_com_specify_ssl_configuration(): %s\n", cl_get_error_text(commlib_error)));
            eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, MSG_GDI_CANT_CONNECT_HANDLE_SSUUS, 
                prog_state->get_sge_formal_prog_name(prog_state), 0, 
                sge_env->get_sge_qmaster_port(sge_env),
                cl_get_error_text(commlib_error));
            sge_csp_path_class_destroy(&sge_csp);
            DEXIT;
            return false;
         }


#endif         
      }

      handle = cl_com_create_handle( &commlib_error,
                                     communication_framework,
                                     CL_CM_CT_MESSAGE,
                                     CL_FALSE,
                                     sge_env->get_sge_qmaster_port(sge_env),
                                     CL_TCP_DEFAULT,
                                     (char*)prog_state->get_sge_formal_prog_name(prog_state),
                                     0,  /* TODO get the component id from the class */
                                     1, 0 );
                                           
      if (handle == NULL) {
         eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, MSG_GDI_CANT_CONNECT_HANDLE_SSUUS, 
                prog_state->get_sge_formal_prog_name(prog_state), 0, sge_env->get_sge_qmaster_port(sge_env),
                cl_get_error_text(commlib_error));
         DEXIT;
         return false;
      }
   }

   thiz->set_com_handle(thiz, handle);

/*    TODO: gdi_state_set_made_setup(1); */

   /* check if master is alive */
   {
      const char * master = thiz->get_master(thiz);
      DPRINTF(("thiz->get_master(thiz) = %s\n", master)); 
      if (!thiz->is_alive(thiz, eh)) {
         DEXIT;
         return false;
      }
   }

   DEXIT;
   return true;   
}

static bool sge_gdi_ctx_class_is_alive(sge_gdi_ctx_class_t *thiz, sge_error_class_t* eh) 
{
   cl_com_SIRM_t* status = NULL;
   int cl_ret;
   bool ret = true;
   cl_com_handle_t *handle = thiz->get_com_handle(thiz);

   /* TODO */
   const char* comp_name = prognames[QMASTER];
   const char* comp_host = thiz->get_master(thiz);
   int         comp_id   = 1;
 
   DENTER(GDI_LAYER, "sge_gdi_ctx_class_is_alive");

   DPRINTF(("to->comp_host, to->comp_name, to->comp_id: %s/%s/%d\n", comp_host, comp_name, comp_id));
   cl_ret = cl_commlib_get_endpoint_status(handle, (char*)comp_host, (char*)comp_name, comp_id, &status);
   if (cl_ret != CL_RETVAL_OK) {
      eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR,
                "cl_commlib_get_endpoint_status failed: "SFQ, cl_get_error_text(ret));
      ret = false;
   } else {
      DEBUG((SGE_EVENT,MSG_GDI_QMASTER_STILL_RUNNING));   
      ret = true;
   }

   if (status != NULL) {
      DEBUG((SGE_EVENT,MSG_GDI_ENDPOINT_UPTIME_UU, sge_u32c( status->runtime) , 
             sge_u32c( status->application_status) ));
      cl_com_free_sirm_message(&status);
   }
 
   DEXIT;
   return ret;
}

static lList* sge_gdi_ctx_class_gdi(sge_gdi_ctx_class_t *thiz, int target, int cmd, lList **lpp,
                                 lCondition *where, lEnumeration *what, sge_error_class_t *eh)
{
   lList *alp = NULL;
   
   DENTER(GDI_LAYER, "sge_gdi_ctx_class_gdi");

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
   ** }
   */

   gdi_once_init();

   thiz->dprintf(thiz);
   
   alp = sge_gdi2(thiz, target, cmd, lpp, where, what );

   DEXIT;
   return alp;

}

static int sge_gdi_ctx_class_gdi_multi(sge_gdi_ctx_class_t* thiz, lList **alpp, int mode, u_long32 target, u_long32 cmd,
                  lList **lpp, lCondition *cp, lEnumeration *enp, lList **malpp, 
                  state_gdi_multi *state, bool do_copy) 
{
   int id = 0;
   
   DENTER(GDI_LAYER, "sge_gdi_ctx_class_gdi_multi");

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
   ** }
   */

   gdi_once_init();

   thiz->dprintf(thiz);
   
   id = sge_gdi2_multi(thiz, alpp, mode, target, cmd, lpp, cp, enp, malpp, state, do_copy); 

   DEXIT;
   return id;

}

static void sge_gdi_ctx_class_dprintf(sge_gdi_ctx_class_t *ctx)
{
   DENTER(GDI_LAYER, "sge_gdi_ctx_class_dprintf");

   if (ctx == NULL) {
      DEXIT;
      return;
   }   
   DPRINTF(("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n"));   
      
   (ctx->get_sge_env_state(ctx))->dprintf(ctx->get_sge_env_state(ctx)); 
   (ctx->get_sge_prog_state(ctx))->dprintf(ctx->get_sge_prog_state(ctx)); 
   (ctx->get_sge_path_state(ctx))->dprintf(ctx->get_sge_path_state(ctx)); 
   (ctx->get_sge_bootstrap_state(ctx))->dprintf(ctx->get_sge_bootstrap_state(ctx)); 

   DPRINTF(("master: %s\n", ctx->get_master(ctx)));
   DPRINTF(("uid/username: %d/%s\n", (int) ctx->get_uid(ctx), ctx->get_username(ctx)));
   DPRINTF(("gid/groupname: %d/%s\n", (int) ctx->get_gid(ctx), ctx->get_groupname(ctx)));

   DPRINTF(("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n"));   

   DEXIT;
}


/** --------- getter/setter ------------------------------------------------- */
static cl_com_handle_t* get_com_handle(sge_gdi_ctx_class_t *thiz) 
{
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   return es->com_handle;
}
   
static void set_com_handle(sge_gdi_ctx_class_t *thiz, cl_com_handle_t*com_handle) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   
   if( es->com_handle != NULL ) {
      cl_commlib_shutdown_handle(es->com_handle, CL_TRUE);
   }
   es->com_handle = com_handle;
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

static const char* get_master(sge_gdi_ctx_class_t *thiz) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   sge_path_state_class_t* path_state = thiz->get_sge_path_state(thiz);
   
   DENTER(GDI_LAYER, "sge_gdi_ctx_class->get_master");
   if( es->master == NULL ) {
      char err_str[SGE_PATH_MAX+128];
      char master_name[CL_MAXHOSTLEN];

      if (get_qm_name(master_name, path_state->get_act_qmaster_file(path_state), err_str)) {         
         ERROR((SGE_EVENT, MSG_GDI_READMASTERNAMEFAILED_S , err_str));
         DEXIT;
         return NULL;
      } 
      DPRINTF(("(re-)reading act_qmaster file. Got master host \"%s\"\n", master_name));
      es->master = sge_strdup(es->master,master_name);
   }   
   DEXIT;
   return es->master;
}

static const char* get_progname(sge_gdi_ctx_class_t *thiz) {
   sge_prog_state_class_t* prog_state = thiz->get_sge_prog_state(thiz);
   const char *progname = NULL;
   
   DENTER(GDI_LAYER, "sge_gdi_ctx_class->get_progname");
   progname = prog_state->get_sge_formal_prog_name(prog_state); 
   DEXIT;
   return progname;
}

static const char* get_cell_root(sge_gdi_ctx_class_t *thiz) {
   sge_path_state_class_t* path_state = thiz->get_sge_path_state(thiz);
   const char *cell_root = NULL;
   
   DENTER(GDI_LAYER, "sge_gdi_ctx_class->get_cell_root");
   cell_root = path_state->get_cell_root(path_state); 
   DEXIT;
   return cell_root;
}


static const char* get_username(sge_gdi_ctx_class_t *thiz) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   return es->username;
}

static const char* get_groupname(sge_gdi_ctx_class_t *thiz) {
   sge_gdi_ctx_t *es = (sge_gdi_ctx_t *) thiz->sge_gdi_ctx_handle;
   return es->groupname;
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
      DEXIT;
      return CL_RETVAL_LOG_NO_LOGLIST;
   }

   if (  ( ret_val = cl_raw_list_lock(list_p)) != CL_RETVAL_OK) {
      DEXIT;
      return ret_val;
   }

   while ( (elem = cl_log_list_get_first_elem(list_p) ) != NULL) {
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
   
   if (  ( ret_val = cl_raw_list_unlock(list_p)) != CL_RETVAL_OK) {
      DEXIT;
      return ret_val;
   } 
   DEXIT;
   return CL_RETVAL_OK;
}

