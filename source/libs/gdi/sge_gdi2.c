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
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pwd.h>

#ifndef WIN32NATIVE
#	include <unistd.h>
#endif
#include <stdlib.h>

#include "basis_types.h"
#include "sge.h"
#include "sge_stdlib.h"
#include "commlib.h"
#include "sge_gdiP.h"
#include "sge_gdi_request.h"
#include "sge_any_request.h"
#include "sge_multiL.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sge_uidgid.h"
#include "sge_parse_num_par.h"
#include "sge_profiling.h"
#include "sge_spool.h"
#include "qm_name.h"
#include "sge_unistd.h"
#include "sge_security.h"
#include "sge_hostname.h"
#include "sge_answer.h"
#include "uti/setup_path.h"
#ifdef KERBEROS
#  include "krb_lib.h"
#endif
#include "msg_common.h"
#include "msg_gdilib.h"
#include "sge_env.h"
#include "sge_bootstrap.h"

#include "sge_gdi_ctx.h"
#include "sge_gdi2.h"
#include "sge_time.h"

                                      
#if 0
static int sge_gdi2_log_flush_func(cl_raw_list_t* list_p);
static int gdi2_check_isalive(sge_gdi_ctx_class_t* ctx);
#endif

static void dump_send_info(const char* comp_host, const char* comp_name, int comp_id, cl_xml_ack_type_t ack_type, unsigned long tag, unsigned long* mid );


static bool gdi2_send_multi_sync(sge_gdi_ctx_class_t* ctx, lList **alpp, 
                                 state_gdi_multi *state, sge_gdi_request **answer, 
                                 lList **malpp);


static int sge_get_gdi2_request(int *commlib_error,
                                sge_gdi_ctx_class_t *ctx,
                               sge_gdi_request** arp,
                               unsigned long request_mid);
static int sge_send_gdi2_request(int sync, sge_gdi_ctx_class_t *ctx,
                         sge_gdi_request *ar,
                         u_long32 *mid, unsigned long response_id, lList **alpp);
static int sge_send_receive_gdi2_request(int *commlib_error,
                                        sge_gdi_ctx_class_t *ctx, 
                                        sge_gdi_request *out,
                                        sge_gdi_request **in,
                                        lList **alpp);
static int sge_get_gdi2_request_async(int *commlib_error,
                               sge_gdi_ctx_class_t *ctx,
                               sge_gdi_request** arp,
                               unsigned long request_mid,
                               bool is_sync);
static void dump_receive_info(cl_com_message_t** message, cl_com_endpoint_t** sender);

#if 0
int sge_gdi2_connect(const char* progname, const char* url,
                     const char* username, const char* credentials,
                     sge_gdi_ctx_class_t** ctx, lList **alpp) {
                        

   int ret = 0;
   dstring bw = DSTRING_INIT;
   
   const sge_path_state_class_t *path_state = NULL;
   const sge_bootstrap_state_class_t *bootstrap_state = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi2_connect");
   
   /* TODO: profiling init, is this possible */
   sge_prof_set_enabled(false);
   sge_prof_setup();

   /* TODO: global state setup must be removed */
   gdi_mt_init();

   *ctx = sge_gdi_ctx_class_create_from_bootstrap(progname, url, username, credentials);
   if(*ctx == NULL ) {
      answer_list_add(alpp,"sge_gdi_ctx_class_create failed", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return FALSE;
   }
   
   /* context setup is complete => setup the commlib */
   ret = cl_com_setup_commlib(CL_NO_THREAD,CL_LOG_OFF, sge_gdi2_log_flush_func);
   if (ret != CL_RETVAL_OK) {
      answer_list_add(alpp,"cl_com_setup_commlib failed", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      sge_gdi_ctx_class_destroy(ctx);
      DEXIT;
      return FALSE;
   }
   
   path_state = (*ctx)->get_sge_path_state(*ctx);
   bootstrap_state = (*ctx)->get_bootstrap_state(*ctx);
   
   /* set the alias file */
   /* TODO cl_com_set_alias_file needs (char*)! why? */
   ret = cl_com_set_alias_file((char*)(path_state->get_alias_file(path_state)));
   if (ret != CL_RETVAL_OK) {
      answer_list_add(alpp,"cl_com_set_alias_file failed", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      sge_gdi_ctx_class_destroy(ctx);      
      DEXIT;
      return FALSE;
   }

   /* setup the resolve method */
   {
      cl_host_resolve_method_t resolve_method = CL_SHORT;
      if( bootstrap_state->get_ignore_fqdn(bootstrap_state) == false ) {
         resolve_method = CL_LONG;
      }
      
      /* TODO cl_com_set_resolve_method needs (char*)! why? */
      ret = cl_com_set_resolve_method(resolve_method, (const char*)bootstrap_state->get_default_domain(*bootstrap_state));
      if( ret != CL_RETVAL_OK ) {
         answer_list_add(alpp,"cl_com_set_resolve_method failed", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         sge_gdi_ctx_class_destroy(ctx);
         ERROR((SGE_EVENT, cl_get_error_text(ret)) );
         DEXIT;
         return FALSE;
      }
      
   }
   
   /* TODO set a general_communication_error */   
   ret = cl_com_set_error_func(general_communication_error);
   if (ret != CL_RETVAL_OK) {
      char buf[1024];
      sprintf(buf, "cl_com_set_error_func failed: %s", cl_get_error_text(ret));
      answer_list_add(alpp, buf, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      sge_gdi_ctx_class_destroy(ctx);
      DEXIT;
      return FALSE;
   }*/
   
   /* TODO set tag name function */
   ret = cl_com_set_tag_name_func(sge_dump_message_tag);
   if (ret != CL_RETVAL_OK) {
      char buf[1024];
      sprintf(buf, "cl_com_set_tag_name_func failed: %s", cl_get_error_text(ret));
      answer_list_add(alpp, buf, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      sge_gdi_ctx_class_destroy(ctx);
      DEXIT;
      return FALSE;
   }
   
   (*ctx)->handle = cl_com_get_handle((*ctx)->progname ,0);
   
   if( (*ctx)->handle == NULL ) {
      /* handle does not exits, create one */
      int commlib_error = 0;      
      DTRACE;
      (*ctx)->handle = cl_com_create_handle( &commlib_error,
                                             (*ctx)->communication_framework,
                                             CL_CM_CT_MESSAGE,
                                             CL_FALSE,
                                             (*ctx)->toport,
                                             CL_TCP_DEFAULT,
                                             (*ctx)->progname,
                                             (*ctx)->from->comp_id,
                                             1, 0 );
                                           
      if( (*ctx)->handle == NULL ) {
         ERROR((SGE_EVENT, MSG_GDI_CANT_CONNECT_HANDLE_SSUUS, 
               (*ctx)->to->comp_host,
               (*ctx)->progname,
               (*ctx)->from->comp_id, 
               (*ctx)->toport,
               cl_get_error_text(commlib_error)));
         sge_gdi2_close(*ctx);
         DEXIT;
         return FALSE;
      }
   }

/*    TODO: gdi_state_set_made_setup(1); */

   /* check if master is alive */
/*    if (gdi_state_get_isalive()) { */
   {
      const char * master = sge_get_master(1);
      DPRINTF(("sge_get_master(1) = %s\n", master)); 
      if (gdi2_check_isalive(*ctx) != CL_RETVAL_OK) {
         DEXIT;
         return FALSE;
      }
   }

/*    } */

   DEXIT;
   return TRUE;
}

/*
** requires a valid sge_gdi_ctx_t no check in function
*/
static int gdi2_check_isalive(sge_gdi_ctx_t* ctx) 
{
   int alive = CL_RETVAL_OK;
   cl_com_SIRM_t* status = NULL;
   int ret;
 
   DENTER(TOP_LAYER, "gdi2_check_isalive");

   DPRINTF(("ctx->to->comp_host, ctx->to->comp_name, ctx->to->comp_id: %s/%s/%d\n", ctx->to->comp_host, ctx->to->comp_name, ctx->to->comp_id));
   ret = cl_commlib_get_endpoint_status(ctx->handle,(char*)ctx->to->comp_host, ctx->to->comp_name, ctx->to->comp_id, &status);
   if (ret != CL_RETVAL_OK) {
      DPRINTF(("cl_commlib_get_endpoint_status() returned "SFQ"\n", cl_get_error_text(ret)));
      alive = ret;
   } else {
      DEBUG((SGE_EVENT,MSG_GDI_QMASTER_STILL_RUNNING));   
      alive = CL_RETVAL_OK;
   }

   if (status != NULL) {
      DEBUG((SGE_EVENT,MSG_GDI_ENDPOINT_UPTIME_UU, sge_u32c( status->runtime) , sge_u32c( status->application_status) ));
      cl_com_free_sirm_message(&status);
   }
 
   DEXIT;
   return alive;
}

int sge_gdi2_close(sge_gdi_ctx_t *ctx) {
 
   DENTER(TOP_LAYER, "sge_gdi2_close");
   
   if( ctx->handle != NULL ) {
      cl_commlib_shutdown_handle(ctx->handle, CL_TRUE);
      ctx->handle = NULL;
   }
   
   if( ctx->ssl_setup != NULL ) {
      FREE(ctx->ssl_setup);
      ctx->ssl_setup = NULL;
   }
   
   if( ctx->security_mode != NULL ) {
      FREE(ctx->security_mode);
      ctx->security_mode = NULL;
   }
   
   if( ctx->default_domain != NULL ) {
     FREE(ctx->default_domain);
     ctx->default_domain = NULL;
   }
   
   if( ctx->to != NULL ) {
      cl_com_free_endpoint(&(ctx->to));
   }
   
   if( ctx->from != NULL ) {
      cl_com_free_endpoint(&(ctx->from));
   }
   
   FREE(ctx->aliasfile);
   ctx->aliasfile = NULL;
   
   FREE(ctx->progname);
   ctx->progname = NULL;
   FREE(ctx->groupname);
   ctx->groupname = NULL;
   FREE(ctx->username);
   ctx->username = NULL;

   sge_prog_state_class_destroy(&(ctx->sge_prog_state_object));

   DEXIT;
   return true;
}

static int sge_gdi2_init_ctx_bootstrap(sge_gdi_ctx_t* ctx, 
                                       const char* progname, 
                                       const char* url, 
                                       const char* username, 
                                       const char* credentials) 
{
   char sge_root[BUFSIZ];
   char sge_cell[BUFSIZ];
   char sge_qmaster_port[BUFSIZ];
   char *token = NULL;
   char sge_url[BUFSIZ];
   
   int ret = true;

   const int num_bootstrap = 3;
   const char *name[] = { "default_domain",
                          "ignore_fqdn",
                          "security_mode"
                        };

   char value[num_bootstrap][1025];

   dstring bootstrap_file = DSTRING_INIT;
   dstring error_dstring = DSTRING_INIT;
   
   dstring qmaster_file = DSTRING_INIT;
   char*   resolved_name = NULL;
   char    master_name[CL_MAXHOSTLEN];
   char    err_str[BUFSIZ];
   struct  saved_vars_s *url_ctx = NULL;
   sge_env_state_class_t *sge_env = NULL;   
   int sge_qmaster_p = 0;
   int sge_execd_p = 0;
   
   DENTER(TOP_LAYER, "sge_gdi2_init_ctx_bootstrap");

   sge_env = sge_env_state_class_create();
   if (!sge_env) {
      DPRINTF(("sge_env_state_class_create failed\n"));
      DEXIT;
      return FALSE;
   }

   DPRINTF(("url = %s\n", url));
   
   sscanf(url, "bootstrap://%s", sge_url);
   DPRINTF(("sge_url = %s\n", sge_url));
   token = sge_strtok_r(sge_url, "@", &url_ctx);
   strcpy(sge_root, token);
   token = sge_strtok_r(NULL, ":", &url_ctx);
   strcpy(sge_cell, token);
   token = sge_strtok_r(NULL, NULL, &url_ctx);
   strcpy(sge_qmaster_port, token);
   sge_free_saved_vars(url_ctx);
   
   sge_qmaster_port = atoi(sge_qmaster_port);

   ctx->toport = atoi(sge_qmaster_port);
   
   sge_env->set_sge_root(sge_env, sge_root); 
   sge_env->set_sge_cell(sge_env, sge_cell); 
   sge_env->set_sge_qmaster_port(sge_env, sge_cell); 
   
   DPRINTF(("sge_root = %s, sge_cell = %s, port = %s\n", sge_root, sge_cell, sge_qmaster_port));
   
   sge_dstring_sprintf(&bootstrap_file,"%s/%s/%s/bootstrap", 
                        sge_root, sge_cell, COMMON_DIR );
   /* read bootstrapping information */   
   ret = sge_get_confval_array(sge_dstring_get_string(&bootstrap_file), 
                               num_bootstrap, name, 
                               value, &error_dstring);
                                    
   sge_dstring_free(&bootstrap_file);
   
   /* TODO: fill prog_state, progname or prog number ??? */
   ctx->prog_state_object = prog_state_class_create(QCONF); 

   if (ret) {
      /* TODO provide a error message */
      sge_dstring_free(&error_dstring);
      DEXIT;
      return FALSE;
   } else {
      sge_dstring_free(&error_dstring);
      
      /* store bootstrapping information in the context */
      ctx->default_domain = sge_strdup(NULL, value[0]);
      ctx->security_mode = sge_strdup(NULL, value[2]);
      {
         u_long32 uval;
         parse_ulong_val(NULL, &uval, TYPE_BOO, value[1], 
                         NULL, 0);
         ctx->ignore_fqdn = uval ? true : false;                        
      }
   }
   
   /* read act qmaster file and initialize to endpoint */
   sge_dstring_sprintf(&qmaster_file, "%s/%s/common/act_qmaster", sge_root, sge_cell );
   
   ret = get_qm_name(master_name, sge_dstring_get_string(&qmaster_file), err_str);
   
   DPRINTF(("master_name = %s\n", master_name));
   
   sge_dstring_free(&qmaster_file);
   
   if (ret) {
      ERROR((SGE_EVENT, MSG_GDI_READMASTERNAMEFAILED_S , err_str));
      DEXIT;
      return FALSE;
   }

   ret = cl_com_cached_gethostbyname(master_name, &resolved_name, NULL, NULL, NULL );
   if (ret != CL_RETVAL_OK) {
      DEXIT;
      return ret;
   }
   
   DPRINTF(("resolved_name = %s\n", resolved_name));

   ctx->to = cl_com_create_endpoint(resolved_name, prognames[QMASTER] , 1);
   if (ctx->to == NULL) {
      FREE(resolved_name); 
      DEXIT;
      return FALSE;
   }

   /* get own hostname and initialize from endpoint */
   {
      char hostname[CL_MAXHOSTLEN];

      /* Fetch hostnames */
/*       SGE_ASSERT((gethostname(hostname, CL_MAXHOSTLEN) == 0)); */
      gethostname(hostname, CL_MAXHOSTLEN);

      DPRINTF(("my hostname = %s\n", hostname));
      
      resolved_name = NULL;
      ret = cl_com_cached_gethostbyname(hostname, &resolved_name, NULL, NULL, NULL );
      if (ret != CL_RETVAL_OK) {
         DEXIT;
         return FALSE;
      }
   
      DPRINTF(("resolved_name = %s\n", resolved_name));

      ctx->from = cl_com_create_endpoint(resolved_name,(char*) ctx->progname, 0 );
      if (ctx->from == NULL) {
         FREE(resolved_name); 
         DEXIT;
         return FALSE;
      }
      
      /* TODO: component id depends on progname */
      ctx->from->comp_id = 0;
      
   }
   
   /* init alias file */   
   {
      dstring tmp_alias_file = DSTRING_INIT;
      sge_dstring_sprintf(&tmp_alias_file, "%s/%s/%s/%s", sge_root, sge_cell, COMMON_DIR, ALIAS_FILE );
      
      ctx->aliasfile = sge_strdup(NULL, sge_dstring_get_string(&tmp_alias_file));
      sge_dstring_free(&tmp_alias_file);
   }

   /* init security */
   {
      if( strcasecmp( value[2], "csp" ) == 0 ) {
         ctx->communication_framework = CL_CT_SSL;
         // TODO call sge_ssl_setup_security_path with parameter
         //      sge_root and sge_cell
      }
      
   }
   
   DEXIT;
   return TRUE;
   
}

static int sge_gdi2_log_flush_func(cl_raw_list_t* list_p) {
   int ret_val;
   cl_log_list_elem_t* elem = NULL;
   DENTER(COMMD_LAYER, "sge_gdi2_log_flush_func");

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

#endif


lList* sge_gdi2(sge_gdi_ctx_class_t *ctx, u_long32 target, u_long32 cmd, lList **lpp, lCondition *cp,
               lEnumeration *enp) 
{
   lList *alp = NULL;
   lList *mal = NULL;
   u_long32 id;
   state_gdi_multi state = STATE_GDI_MULTI_INIT;

   DENTER(TOP_LAYER, "sge_gdi2");

   PROF_START_MEASUREMENT(SGE_PROF_GDI);

   if ((id = sge_gdi2_multi(ctx, &alp, SGE_GDI_SEND, target, cmd, lpp, 
                              cp, enp, &mal, &state, true)) == -1) {
      PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
      DEXIT;
      return alp;
   }

   alp = sge_gdi_extract_answer(cmd, target, id, mal, lpp);

   lFreeList(&mal);

   PROF_STOP_MEASUREMENT(SGE_PROF_GDI);

   DEXIT;
   return alp;
}

int sge_gdi2_multi(sge_gdi_ctx_class_t* ctx, lList **alpp, int mode, u_long32 target, u_long32 cmd,
                  lList **lp, lCondition *cp, lEnumeration *enp, lList **malpp, 
                  state_gdi_multi *state, bool do_copy) 
{
  return sge_gdi2_multi_sync(ctx, alpp, mode, target, cmd, lp, cp, enp, malpp, 
                            state, do_copy, true);
}


int sge_gdi2_multi_sync(sge_gdi_ctx_class_t* ctx, lList **alpp, int mode, u_long32 target, u_long32 cmd,
                  lList **lp, lCondition *cp, lEnumeration *enp, lList **malpp, 
                  state_gdi_multi *state, bool do_copy, bool do_sync) 
{
   sge_gdi_request *request = NULL;
   sge_gdi_request *answer = NULL;
   int ret;
   int operation;
   uid_t uid;
   gid_t gid;
   char username[128];
   char groupname[128];

   DENTER(TOP_LAYER, "sge_gdi2_multi_sync");

   PROF_START_MEASUREMENT(SGE_PROF_GDI);

   operation = SGE_GDI_GET_OPERATION(cmd);

   if ((!lp || !*lp) && !(operation == SGE_GDI_PERMCHECK || operation == SGE_GDI_GET 
       || operation == SGE_GDI_TRIGGER || 
       (operation == SGE_GDI_DEL && target == SGE_SHARETREE_LIST))) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_POINTER_NULLPOINTERPASSEDTOSGEGDIMULIT ));
      goto error;
   }

   if (!(request = new_gdi_request())) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_CANTCREATEGDIREQUEST ));
      goto error;
   }
   
   request->lp = NULL;
   request->op = cmd;
   request->target = target;
   request->version = GRM_GDI_VERSION;
   request->alp = NULL;
   switch (operation) {
   case SGE_GDI_MOD:
      if (enp && lp != NULL) {
         if (do_copy) {
            request->lp = lSelect("lp", *lp, NULL, enp);
         } else {
            request->lp = *lp;
            *lp = NULL;
         }
         break;
      }
      /* no break */
   default:
      if (lp != NULL) {
         if (do_copy) {
            request->lp = lCopyList("lp", *lp);
         } else {
               request->lp = *lp;
               *lp = NULL;
         }
      }
      break;
   }
   if ((operation == SGE_GDI_GET) || (operation == SGE_GDI_PERMCHECK)) {
      request->cp =  lCopyWhere(cp);
      request->enp = lCopyWhat(enp);
   } else {
      request->cp =  NULL;
      request->enp = NULL; 
   }

#if 0
   /* 
   ** user info
   */
   uid = ctx->get_uid(ctx);
   
   if (sge_uid2user(uid, username, sizeof(username), MAX_NIS_RETRIES)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_FUNC_GETPWUIDXFAILED_IS , 
              (int)uid, strerror(errno)));
      goto error;
   }
   DPRINTF(("uid = %d, username = %s\n", uid, username));
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
   gid = ctx->get_gid(ctx);
   if (sge_gid2group(gid, groupname, sizeof(groupname), MAX_NIS_RETRIES)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_GETGRGIDXFAILEDERRORX_U,
                             sge_u32c(gid)));
      goto error; 
   }
   DPRINTF(("gid = %d, groupname = %s\n", gid, groupname));

   if (sge_set_auth_info(request, uid, username, gid, groupname) == -1) {
      goto error;
   }   
#else
   uid = ctx->get_uid(ctx);
   gid = ctx->get_gid(ctx);
   strncpy(username, ctx->get_username(ctx), sizeof(username));
   strncpy(groupname, ctx->get_groupname(ctx), sizeof(groupname));
   
   if (sge_set_auth_info(request, uid, username, gid, groupname) == -1) {
      goto error;
   }   
#endif

   /*
   ** append the new gdi request to the request list
   */
   ret = request->sequence_id = ++state->sequence_id;
   
   if (state->first) {
      state->last->next = request;
      state->last = request;
   }
   else {
      state->first = state->last = request;
   }
   
   if (mode == SGE_GDI_SEND) {
#ifdef GDI2   
       gdi2_receive_multi_async(&answer, malpp, true);
       if (do_sync) {
#endif
         if (!gdi2_send_multi_sync(ctx, alpp, state, &answer, malpp)) {
            goto error;
         }
#ifdef GDI2   
       }
       else { 
          /* if this is null, we did not get an answer..., which means we return 0;*/
         if (*malpp == NULL) {
            ret = 0;
          }

          if (!gdi_send_multi_async(alpp, state)) {
             goto error;
          }
       }
#endif

   }

   PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
   DEXIT;
   return ret;

   error:
      if (alpp != NULL) {
         answer_list_add(alpp, SGE_EVENT, STATUS_NOQMASTER, ANSWER_QUALITY_ERROR);
      }   
      answer = free_gdi_request(answer);
      state->first = free_gdi_request(state->first);
      state->last = NULL;
      state->sequence_id = 0;
      PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
      DEXIT;
      return -1;
}

static bool
gdi2_send_multi_sync(sge_gdi_ctx_class_t* ctx, lList **alpp, state_gdi_multi *state, sge_gdi_request **answer, lList **malpp)
{
   sge_env_state_class_t *sge_env = ctx->get_sge_env_state(ctx);
   int commlib_error = CL_RETVAL_OK;
   sge_gdi_request *an;
   int status = 0;
   lListElem *map = NULL;
   lListElem *aep = NULL;
   
   DENTER(TOP_LAYER, "gdi2_send_multi_sync");

   /* the first request in the request list identifies the request uniquely */
   state->first->request_id = gdi_state_get_next_request_id();

#ifdef KERBEROS
   /* request that the Kerberos library forward the TGT */
   if (state->first->target == SGE_JOB_LIST && 
         SGE_GDI_GET_OPERATION(state->first->op) == SGE_GDI_ADD) {
      krb_set_client_flags(krb_get_client_flags() | KRB_FORWARD_TGT);
      krb_set_tgt_id(state->first->request_id);
   }
#endif

   status = sge_send_receive_gdi2_request(&commlib_error, ctx, state->first, answer, alpp);

#ifdef KERBEROS
   /* clear the forward TGT request */
   if (state->first->target == SGE_JOB_LIST && 
         SGE_GDI_GET_OPERATION(state->first->op) == SGE_GDI_ADD) {
      krb_set_client_flags(krb_get_client_flags() & ~KRB_FORWARD_TGT);
      krb_set_tgt_id(0);
   }
#endif

   /* Print out non-error messages */
   /* TODO SG: check for error messages and warnings */
   for_each (aep, *alpp) {
      if (lGetUlong (aep, AN_quality) == ANSWER_QUALITY_INFO) {
         INFO ((SGE_EVENT, lGetString (aep, AN_text)));
      }
   }
   
   lFreeList(alpp);
   
   if (status != 0) {

      /* failed to contact qmaster ? */
      /* So we build an answer structure */
      switch (status) {
         case -2:
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_SENDINGGDIREQUESTFAILED));
            break;
         case -3:
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_RECEIVEGDIREQUESTFAILED));
            break;
         case -4:
            /* gdi error */

            /* For the default case, just print a simple message */
            if (commlib_error == CL_RETVAL_CONNECT_ERROR ||
                commlib_error == CL_RETVAL_CONNECTION_NOT_FOUND ) {
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_UNABLE_TO_CONNECT_SUS,
                                      prognames[QMASTER],
                                      sge_env->get_sge_qmaster_port(sge_env),
                                      ctx->get_master(ctx)));
            }
            /* For unusual errors, give more detail */
            else {
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, 
                                      MSG_GDI_CANT_SEND_MESSAGE_TO_PORT_ON_HOST_SUSS,
                                      prognames[QMASTER],
                                      sge_env->get_sge_qmaster_port(sge_env), 
                                      ctx->get_master(ctx),
                                      cl_get_error_text(commlib_error)));
            }
            break;
         case -5:
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_SIGNALED ));
            break;
         default:
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_GENERALERRORXSENDRECEIVEGDIREQUEST_I , status));
            break;
      }
      DPRINTF(("re-read act_qmaster file (gdi_send_multi_sync)\n"));
#ifdef GDI2      
      /* TODO: call reinit ctx instead of sge_get_master */
      sge_get_master(true);
#endif      
      return false;
   }
 
   for (an = (*answer); an; an = an->next) { 
      int an_operation, an_sub_command;

      map = lAddElemUlong(malpp, MA_id, an->sequence_id, MA_Type);
      an_operation = SGE_GDI_GET_OPERATION(an->op);
      an_sub_command = SGE_GDI_GET_SUBCOMMAND(an->op);
      if (an_operation == SGE_GDI_GET || an_operation == SGE_GDI_PERMCHECK ||
            (an_operation==SGE_GDI_ADD 
             && an_sub_command==SGE_GDI_RETURN_NEW_VERSION )) {
         lSetList(map, MA_objects, an->lp);
         an->lp = NULL;
      }
      lSetList(map, MA_answers, an->alp);
      an->alp = NULL;
   }

   (*answer) = free_gdi_request((*answer));
   state->first = free_gdi_request(state->first);
   state->last = NULL;
   state->sequence_id = 0;
   return true;
}

/****** gdi/request/sge_send_receive_gdi2_request() ****************************
*  NAME
*     sge_send_receive_gdi2_request() -- snd and rcv a gdi structure 
*
*  SYNOPSIS
*     static int sge_send_receive_gdi_request(cl_com_endpoint_t *to, 
*                                sge_gdi_request *out, 
*                                sge_gdi_request **in) 
*
*  FUNCTION
*     sends and receives an gdi request structure 
*
*  INPUTS
*     const char *rhost          - ??? 
*     const char *commproc       - ??? 
*     u_short id           - ??? 
*     sge_gdi_request *out - ??? 
*     sge_gdi_request **in - ??? 
*
*  RESULT
*     static int - 
*        0 ok
*        -1 failed before communication
*        -2 failed sending gdi request
*        -3 failed receiving gdi request
*        -4 check_isalive() failed
*
*  NOTES
*     MT-NOTE: sge_send_receive_gdi_request() is MT safe (assumptions)
******************************************************************************/
static int sge_send_receive_gdi2_request(int *commlib_error,
                                        sge_gdi_ctx_class_t *ctx, 
                                        sge_gdi_request *out,
                                        sge_gdi_request **in,
                                        lList **alpp)
{
   int ret;
#ifdef GDI2   
   char rcv_rhost[CL_MAXHOSTLEN+1];
   char rcv_commproc[CL_MAXHOSTLEN+1];
#endif   
   u_long32 gdi_request_mid;
   
   DENTER(GDI_LAYER, "sge_send_receive_gdi2_request");

   if (!out) {
      ERROR((SGE_EVENT,
           MSG_GDI_POINTER_NULLLISTPASSEDTOSGESENDRECEIVGDIREQUEST ));
      DEXIT;
      return -1;
   }

   if (!ctx->get_master(ctx)) {
      ERROR((SGE_EVENT, MSG_GDI_POINTER_NULLRHOSTPASSEDTOSGESENDRECEIVEGDIREQUEST ));
      DEXIT;
      return -1;
   }   
   
   /* we send a gdi request and store the request id */
   ret = sge_send_gdi2_request(1, ctx, out, &gdi_request_mid, 0,
                              alpp);
   *commlib_error = ret;


   DPRINTF(("send request with id "sge_U32CFormat"\n", sge_u32c(gdi_request_mid)));
   if (ret != CL_RETVAL_OK) {
      if (!ctx->is_alive(ctx, NULL /* TODO use error handler class */ )) {
         DEXIT;
         return -4;
      } else {
         DEXIT;
         return -2;
      }
   }

#ifdef GDI2
   /* TODO check if this is necessary */
   strcpy(rcv_rhost, rhost);
   strcpy(rcv_commproc, commproc);
   while (!(ret = sge_get_gdi2_request(commlib_error, rcv_rhost, rcv_commproc, 
                                      &id, in, gdi_request_mid))) {

#endif
   while (!(ret = sge_get_gdi2_request(commlib_error, ctx, 
                                       in, gdi_request_mid))) {
      DPRINTF(("in: request_id=%d, sequence_id=%d, target=%d, op=%d\n",
            (*in)->request_id, (*in)->sequence_id, (*in)->target, (*in)->op));
      DPRINTF(("out: request_id=%d, sequence_id=%d, target=%d, op=%d\n",
               out->request_id, out->sequence_id, out->target, out->op));

      if (*in && ((*in)->request_id == out->request_id)) {
         break;
      }
      else {
         *in = free_gdi_request(*in);
         DPRINTF(("<<<<<<<<<<<<<<< GDI MISMATCH >>>>>>>>>>>>>>>>>>>\n"));
      }
   }

   if (ret) {
      if (!ctx->is_alive(ctx, NULL /* TODO use error handler class */ )) {
         DEXIT;
         return -4;
      } 
      else {
         DEXIT;
         return -3;
      }   
   }
   
   DEXIT;
   return 0;
}



/****** gdi/request/sge_send_gdi_request() ************************************
*  NAME
*     sge_send_gdi_request() -- send gdi request 
*
*  SYNOPSIS
*     int sge_send_gdi_request(int sync, const char *rhost, 
*                              const char *commproc, int id, 
*                              sge_gdi_request *ar) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     int sync             - ??? 
*     const char *rhost    - ??? 
*     const char *commproc - ??? 
*     int id               - ??? 
*     sge_gdi_request *ar  - ??? 
*
*  RESULT
*     int - 
*         0 success
*        -1 common failed sending
*        -2 not enough memory
*        -3 format error while unpacking
*        -4 no commd
*        -5 no peer enrolled   
*
*  NOTES
*     MT-NOTE: sge_send_gdi_request() is MT safe (assumptions)
*******************************************************************************/
static int sge_send_gdi2_request(int sync, sge_gdi_ctx_class_t *ctx,
                         sge_gdi_request *ar,
                         u_long32 *mid, unsigned long response_id, lList **alpp) 
{
   int ret = 0;
   sge_pack_buffer pb;
   int size;
   bool local_ret;
   lList *answer_list = NULL;

   DENTER(GDI_LAYER, "sge_send_gdi2_request");

   PROF_START_MEASUREMENT(SGE_PROF_GDI_REQUEST);

   /* 
   ** retrieve packbuffer size to avoid large realloc's while packing 
   */
   init_packbuffer(&pb, 0, 1);
   local_ret = request_list_pack_results(ar, &answer_list, &pb);
   size = pb_used(&pb);
   clear_packbuffer(&pb);

   if (local_ret) {
      /*
      ** now we do the real packing
      */
      if(init_packbuffer(&pb, size, 0) == PACK_SUCCESS) {
         local_ret = request_list_pack_results(ar, &answer_list, &pb);
      }
   }
   if (!local_ret) {
      lListElem *answer = lFirst(answer_list);

      if (answer != NULL) {
         switch (answer_get_status(answer)) {
            case STATUS_ERROR2:
               ret = -2;
               break;
            case STATUS_ERROR3:
               ret = -3;
               break;
            default:
               ret = -1;
         }
      }
   } else {
      ret = sge_gdi2_send_any_request(sync, mid, ctx, &pb,
                              TAG_GDI_REQUEST, response_id, alpp);
   }
   clear_packbuffer(&pb);
   lFreeList(&answer_list);
   PROF_STOP_MEASUREMENT(SGE_PROF_GDI_REQUEST);

   DEXIT;
   return ret;
}

/****** gdi/request/sge_get_gdi_request() *************************************
*  NAME
*     sge_get_gdi_request() -- ??? 
*
*  SYNOPSIS
*     static int sge_get_gdi_request(char *host, char *commproc, 
*                                    u_short *id, sge_gdi_request** arp) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const char *host      - ??? 
*     const char *commproc  - ??? 
*     u_short *id           - ??? 
*     sge_gdi_request** arp - ??? 
*     bool is_sync          - recieve message sync(true) or async(false)
*
*  RESULT
*     static int - 
*         0 success 
*        -1 common failed getting
*        -2 not enough memory 
*        -3 format error while unpacking
*        -4 no commd
*        -5 no peer enrolled
*
*  NOTES
*     MT-NOTE: sge_get_gdi_request() is MT safe (assumptions)
*******************************************************************************/
static int sge_get_gdi2_request(int *commlib_error,
                                sge_gdi_ctx_class_t *ctx,
                               sge_gdi_request** arp,
                               unsigned long request_mid)
{
   return sge_get_gdi2_request_async(commlib_error, ctx, arp, request_mid, true);
}

static int sge_get_gdi2_request_async(int *commlib_error,
                               sge_gdi_ctx_class_t *ctx,
                               sge_gdi_request** arp,
                               unsigned long request_mid,
                               bool is_sync)
{
   sge_pack_buffer pb;
   int tag = TAG_GDI_REQUEST; /* this is what we want */
   int ret;

   DENTER(GDI_LAYER, "sge_get_gdi2_request_async");
   if ( (*commlib_error = sge_gdi2_get_any_request(ctx, &pb, &tag, is_sync, request_mid,0)) != CL_RETVAL_OK) {
      DEXIT;
      return -1;
   }


   ret = sge_unpack_gdi_request(&pb, arp);
   switch (ret) {
   case PACK_SUCCESS:
      break;

   case PACK_ENOMEM:
      ret = -2;
      ERROR((SGE_EVENT, MSG_GDI_ERRORUNPACKINGGDIREQUEST_S, cull_pack_strerror(ret)));
      break;

   case PACK_FORMAT:
      ret = -3;
      ERROR((SGE_EVENT, MSG_GDI_ERRORUNPACKINGGDIREQUEST_S, cull_pack_strerror(ret)));
      break;

   default:
      ret = -1;
      ERROR((SGE_EVENT, MSG_GDI_ERRORUNPACKINGGDIREQUEST_S, cull_pack_strerror(ret)));
      break;
   }

   /* 
      we got the packing buffer filled by 
      sge_get_any_request and have to recycle it 
   */
   clear_packbuffer(&pb);

   DEXIT;
   return ret;
}

/*---------------------------------------------------------
 *  sge_send_any_request
 *  returns 0 if ok
 *          -4 if peer is not alive or rhost == NULL
 *          return value of gdi_send_message() for other errors
 *
 *  NOTES
 *     MT-NOTE: sge_send_gdi_request() is MT safe (assumptions)
 *---------------------------------------------------------*/
int sge_gdi2_send_any_request(int synchron, u_long32 *mid, sge_gdi_ctx_class_t *ctx, 
                         sge_pack_buffer *pb, 
                         int tag, u_long32  response_id, lList **alpp)
{
   int i;
   cl_xml_ack_type_t ack_type;
   sge_env_state_class_t  * sge_env = ctx->get_sge_env_state(ctx);
   cl_com_handle_t* handle = ctx->get_com_handle(ctx);
   unsigned long dummy_mid = 0;
   unsigned long* mid_pointer = NULL;

   const char* comp_name = prognames[QMASTER];
   const char* comp_host = ctx->get_master(ctx);
   int         to_port   = sge_env->get_sge_qmaster_port(sge_env);
   int         comp_id   = 1;  /* TODO */
   
   DENTER(GDI_LAYER, "sge_gdi2_send_any_request");

   ack_type = CL_MIH_MAT_NAK;

   if (comp_host == NULL) {
      answer_list_add(alpp, MSG_GDI_RHOSTISNULLFORSENDREQUEST, STATUS_ESYNTAX,
                      ANSWER_QUALITY_ERROR);
      DEXIT;
      return CL_RETVAL_PARAMS;
   }
   
   if (handle == NULL) {
      answer_list_add(alpp, MSG_GDI_NOCOMMHANDLE, STATUS_NOCOMMD, ANSWER_QUALITY_ERROR);
      DEXIT;
      return CL_RETVAL_HANDLE_NOT_FOUND;
   }

   if (strcmp(comp_name, (char*)prognames[QMASTER]) == 0 && comp_id == 1) {
      cl_com_append_known_endpoint_from_name((char*)comp_host, (char*)comp_name, comp_id, 
                                             to_port, CL_CM_AC_DISABLED ,CL_TRUE);
   }
   
   if (synchron) {
      ack_type = CL_MIH_MAT_ACK;
   }
  
#if 0
   SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_SENDINGMESSAGE_SIU, commproc,id,
                          (unsigned long) pb->bytes_used));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
#endif

   if (mid) {
      mid_pointer = &dummy_mid;
   }

   i = cl_commlib_send_message(handle, (char*) comp_host, (char*) comp_name, comp_id,
                                  ack_type, (cl_byte_t*)pb->head_ptr, (unsigned long) pb->bytes_used,
                                  mid_pointer,  response_id,  tag , (cl_bool_t)1, (cl_bool_t)synchron);
   
   if (i != CL_RETVAL_OK) {
      /* try again ( if connection timed out ) */
      i = cl_commlib_send_message(handle, (char*) comp_host, (char*) comp_name, comp_id,
                                  ack_type, (cl_byte_t*)pb->head_ptr, (unsigned long) pb->bytes_used,
                                  mid_pointer,  response_id,  tag , (cl_bool_t)1, (cl_bool_t)synchron);
   }

   dump_send_info(comp_host, comp_name, comp_id, ack_type, tag, mid_pointer);
   
   if (mid) {
      *mid = dummy_mid;
   }

   if (i != CL_RETVAL_OK) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT,
                             MSG_GDI_SENDMESSAGETOCOMMPROCFAILED_SSISS ,
                             (synchron ? "" : "a"),
                             comp_name,
                             comp_id,
                             comp_host,
                             cl_get_error_text(i)));
      answer_list_add(alpp, SGE_EVENT, STATUS_NOCOMMD, ANSWER_QUALITY_ERROR);
   }

   DEXIT;
   return i;
}


/*----------------------------------------------------------
 * sge_get_any_request
 *
 * returns 0               on success
 *         -1              rhost is NULL 
 *         commlib return values (always positive)
 *
 * NOTES
 *    MT-NOTE: sge_get_any_request() is MT safe (assumptions)
 *----------------------------------------------------------*/
int 
sge_gdi2_get_any_request(sge_gdi_ctx_class_t *ctx, sge_pack_buffer *pb, 
                    int *tag, int synchron, u_long32 for_request_mid, u_long32* mid) 
{
   int i;
   char host[CL_MAXHOSTLEN+1];
   cl_com_message_t* message = NULL;
   cl_com_endpoint_t* sender = NULL;
   cl_com_handle_t* handle = NULL;
   
   sge_prog_state_class_t * prog_state = ctx->get_sge_prog_state(ctx);

   const char* comp_name = prog_state->get_sge_formal_prog_name(prog_state);
   const char* comp_host = ctx->get_master(ctx);
   int         comp_id   = 1;  /* TODO */

   DENTER(GDI_LAYER, "sge_gdi2_get_any_request");

   PROF_START_MEASUREMENT(SGE_PROF_GDI);

   if (!comp_host) {
      ERROR((SGE_EVENT, MSG_GDI_RHOSTISNULLFORGETANYREQUEST ));
      PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
      DEXIT;
      return -1;
   }
   
   strcpy(host, comp_host);

   handle = ctx->get_com_handle(ctx);

   /* trigger communication or wait for a new message (select timeout) */
   cl_commlib_trigger(handle, synchron);

   i = cl_commlib_receive_message(handle, NULL, NULL, 0, (cl_bool_t) synchron, for_request_mid, &message, &sender);
   if ( i == CL_RETVAL_CONNECTION_NOT_FOUND ) {
      if ( comp_name[0] != '\0' && comp_host[0] != '\0' ) {
         /* The connection was closed, reopen it */
         i = cl_commlib_open_connection(handle, (char*)comp_host, (char*)comp_name, comp_id);
         INFO((SGE_EVENT,"reopen connection to %s,%s,"sge_U32CFormat" (2)\n", comp_host, comp_name, sge_u32c(comp_id)));
         if (i == CL_RETVAL_OK) {
            INFO((SGE_EVENT,"reconnected successfully\n"));
            i = cl_commlib_receive_message(handle, NULL, NULL, 0, (cl_bool_t) synchron, for_request_mid, &message, &sender);
         }
      } else {
         DEBUG((SGE_EVENT,"can't reopen a connection to unspecified host or commproc (2)\n"));
      }
   }

   if (i != CL_RETVAL_OK) {
      if (i != CL_RETVAL_NO_MESSAGE) {
         /* This if for errors */
         DEBUG((SGE_EVENT, MSG_GDI_RECEIVEMESSAGEFROMCOMMPROCFAILED_SISS , 
               (comp_name[0] ? comp_name : "any"), 
               (int) comp_id, 
               (comp_host[0] ? comp_host : "any"),
                cl_get_error_text(i)));
      }
      cl_com_free_message(&message);
      cl_com_free_endpoint(&sender);
      /* This is if no message is there */
      PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
      DEXIT;
      return i;
   }    

   /* ok, we received a message */
   if (message != NULL ) {
      dump_receive_info(&message, &sender);
#ifdef GDI2      
      /* TODO: there are two cases for any and addressed communication partner, 
               two functions are needed */
      if (sender != NULL && id) {
         comp_id = (u_short)sender->comp_id;
      }
#endif      
      if (tag) {
        *tag = (int)message->message_tag;
      }  
      if (mid) {
        *mid = message->message_id;
      }  


      /* fill it in the packing buffer */
      i = init_packbuffer_from_buffer(pb, (char*)message->message, message->message_length);

      /* TODO: the packbuffer must be hold, not deleted !!! */
      message->message = NULL;

      if(i != PACK_SUCCESS) {
         ERROR((SGE_EVENT, MSG_GDI_ERRORUNPACKINGGDIREQUEST_S, cull_pack_strerror(i)));
         PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
         DEXIT;
         return CL_RETVAL_READ_ERROR;
      } 

#ifdef GDI2
      /* TODO: there are two cases for any and addressed communication partner, 
               two functions are needed */
      if (sender != NULL ) {
         DEBUG((SGE_EVENT,"received from: %s,"sge_U32CFormat"\n",sender->comp_host, sge_u32c(sender->comp_id) ));
         if (rhost[0] == '\0') {
            strcpy(rhost, sender->comp_host); /* If we receive from anybody return the sender */
         }
         if (commproc[0] == '\0') {
            strcpy(commproc , sender->comp_name); /* If we receive from anybody return the sender */
         }
      }
#endif

      cl_com_free_endpoint(&sender);
      cl_com_free_message(&message);
   }
   PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
   DEXIT;
   return CL_RETVAL_OK;
}

#ifdef GDI2

/****** sge_gdi_request/gdi_receive_multi_async() ******************************
*  NAME
*     gdi_receive_multi_async() -- does a async gdi send
*
*  SYNOPSIS
*     bool gdi_receive_multi_async(sge_gdi_request **answer, lList **malpp, 
*     bool is_sync) 
*
*  FUNCTION
*     The function checks, if an async send was done before. If not, it
*     return true right away, otherwise it gets the send date from the
*     thread specific storage. With that data it queries the comlib, if
*     it has a reply for the send. If not, it returns false otherwise true.
*
*     If is_sync is set, the call blocks, till the comlib recieves an answer,
*     otherwise it returns right away.
*
*  INPUTS
*     sge_gdi_request **answer - answer list for errors during send
*     lList **malpp            - message answer list
*     bool is_sync             - if true, the function waits for an answer
*
*  RESULT
*     bool - true, if everything went okay, otherwise false
*
*  NOTES
*     MT-NOTE: gdi_receive_multi_async() is MT safe 
*
*  SEE ALSO
*     sge_gdi_request/gdi_send_multi_sync
*     sge_gdi_request/gdi_send_multi_async
*******************************************************************************/
bool
gdi2_receive_multi_async(sge_gdi_request **answer, lList **malpp, bool is_sync)
{
   char *rcv_rhost;
   char *rcv_commproc;
   u_short id;
   u_long32 gdi_request_mid = 0;
   state_gdi_multi *state = NULL;

   gdi_send_t *async_gdi = NULL;

   int commlib_error = CL_RETVAL_OK;
   int ret = 0;
   sge_gdi_request *an = NULL;
   lListElem *map = NULL; 

   DENTER(GDI_LAYER, "gdi_receive_multi_async");

   /* we have to check for an ongoing gdi reqest, if there is none, we have to exit */
   if ((async_gdi = gdi_state_get_last_gdi_request()) != NULL) {
      rcv_rhost = async_gdi->rhost;
      rcv_commproc = async_gdi->commproc;
      id = async_gdi->id;
      gdi_request_mid = async_gdi->gdi_request_mid;
      state = &(async_gdi->out);
   }
   else {
      /* nothing todo... */
      return true;
   }
   
   /* recive answer */
   while (!(ret = sge_get_gdi2_request_async(&commlib_error, rcv_rhost, rcv_commproc, &id, answer, gdi_request_mid, is_sync))) {
   
      DPRINTF(("in: request_id=%d, sequence_id=%d, target=%d, op=%d\n",
            (*answer)->request_id, (*answer)->sequence_id, (*answer)->target, (*answer)->op));
      DPRINTF(("out: request_id=%d, sequence_id=%d, target=%d, op=%d\n",
               state->first->request_id, state->first->sequence_id, state->first->target, state->first->op));

      if (*answer && ((*answer)->request_id == state->first->request_id)) {
         break;
      }
      else {
         *answer = free_gdi_request(*answer);
         DPRINTF(("<<<<<<<<<<<<<<< GDI MISMATCH >>>>>>>>>>>>>>>>>>>\n"));
      }
   }
  
   /* process return code */
   if (ret) {
      if (is_sync) {
         if ( (commlib_error = gdi2_check_isalive(rcv_rhost)) != CL_RETVAL_OK) {
            /* gdi error */

            /* For the default case, just print a simple message */
            if (commlib_error == CL_RETVAL_CONNECT_ERROR ||
                commlib_error == CL_RETVAL_CONNECTION_NOT_FOUND ) {
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_UNABLE_TO_CONNECT_SUS,
                                      prognames[QMASTER],
                                      sge_u32c(sge_get_qmaster_port()), 
                                      sge_get_master(false)));
            }
            /* For unusual errors, give more detail */
            else {
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, 
                                      MSG_GDI_CANT_SEND_MESSAGE_TO_PORT_ON_HOST_SUSS,
                                      prognames[QMASTER],
                                      sge_u32c(sge_get_qmaster_port()), 
                                      sge_get_master(false),
                                      cl_get_error_text(commlib_error)));
            }
         } 
         else {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_RECEIVEGDIREQUESTFAILED));
         }   
      }
      DEXIT;
      return false;
   }
  
   for (an = (*answer); an; an = an->next) { 
      int an_operation, an_sub_command;

      map = lAddElemUlong(malpp, MA_id, an->sequence_id, MA_Type);
      an_operation = SGE_GDI_GET_OPERATION(an->op);
      an_sub_command = SGE_GDI_GET_SUBCOMMAND(an->op);
      if (an_operation == SGE_GDI_GET || an_operation == SGE_GDI_PERMCHECK ||
            (an_operation==SGE_GDI_ADD 
             && an_sub_command==SGE_GDI_RETURN_NEW_VERSION )) {
         lSetList(map, MA_objects, an->lp);
         an->lp = NULL;
      }
      lSetList(map, MA_answers, an->alp);
      an->alp = NULL;
   }

   (*answer) = free_gdi_request((*answer));
  
   gdi_state_clear_last_gdi_request();
   
   return true;
}

/****** sge_gdi_request/gdi_send_multi_async() *********************************
*  NAME
*     gdi_send_multi_async() -- sends a request async
*
*  SYNOPSIS
*     static bool gdi_send_multi_async(lList **alpp, state_gdi_multi *state) 
*
*  FUNCTION
*     It sends the handed data and stores all connection info in a thread
*     local storage. This can than be used in gdi_receive_multi_async to
*     query the comlib for an anser
*
*  INPUTS
*     lList **alpp           - answer list
*     state_gdi_multi *state - date to send
*
*  RESULT
*     static bool - returns true, if everything is okay
*
*
*  NOTES
*     MT-NOTE: gdi_send_multi_async() is MT safe 
*
*  SEE ALSO
*     sge_gdi_request/gdi_receive_multi_async
*******************************************************************************/
static bool
gdi2_send_multi_async(lList **alpp, state_gdi_multi *state)
{
   int commlib_error = CL_RETVAL_OK;
   lListElem *aep = NULL;

   u_short id = 1;
   const char *rhost = sge_get_master(false);
   const char *commproc = prognames[QMASTER];
   u_long32 gdi_request_mid = 0;
   
   DENTER(GDI_LAYER, "gdi_send_multi_async");

   /* the first request in the request list identifies the request uniquely */
   state->first->request_id = gdi_state_get_next_request_id();

   commlib_error = sge_send_gdi2_request(0, rhost, commproc, id, state->first, 
                                        &gdi_request_mid, 0, alpp);
   
   /* Print out non-error messages */
   /* TODO SG: check for error messages and warnings */
   for_each (aep, *alpp) {
      if (lGetUlong (aep, AN_quality) == ANSWER_QUALITY_INFO) {
         INFO ((SGE_EVENT, lGetString (aep, AN_text)));
      }
   }
   *alpp = lFreeList (*alpp);
  
   DPRINTF(("send request with id "sge_U32CFormat"\n", sge_u32c(gdi_request_mid)));
   if (commlib_error != CL_RETVAL_OK) {
      if (( commlib_error = gdi2_check_isalive(rhost)) != CL_RETVAL_OK) {
         /* gdi error */

         /* For the default case, just print a simple message */
         if (commlib_error == CL_RETVAL_CONNECT_ERROR ||
             commlib_error == CL_RETVAL_CONNECTION_NOT_FOUND ) {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_UNABLE_TO_CONNECT_SUS,
                                   prognames[QMASTER],
                                   sge_u32c(sge_get_qmaster_port()), 
                                   sge_get_master(false)));
         }
         /* For unusual errors, give more detail */
         else {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, 
                                   MSG_GDI_CANT_SEND_MESSAGE_TO_PORT_ON_HOST_SUSS,
                                   prognames[QMASTER],
                                   sge_u32c(sge_get_qmaster_port()), 
                                   sge_get_master(false),
                                   cl_get_error_text(commlib_error)));
         }
         
      } else {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_SENDINGGDIREQUESTFAILED));
      }
      return false;
   }
  

   /* we have to store the data for the recieve....  */ 
   gdi_set_request(rhost, commproc, id, state, gdi_request_mid);
   
   return true;
}  
#endif

static void dump_receive_info(cl_com_message_t** message, cl_com_endpoint_t** sender) 
{
   DENTER(TOP_LAYER, "dump_receive_info");
   if ( message  != NULL && sender   != NULL && *message != NULL && *sender  != NULL &&
        (*sender)->comp_host != NULL && (*sender)->comp_name != NULL ) {
         char buffer[512];
         dstring ds;
         sge_dstring_init(&ds, buffer, sizeof(buffer));

      DEBUG((SGE_EVENT,"<<<<<<<<<<<<<<<<<<<<\n"));
      DEBUG((SGE_EVENT,"gdi_rcv: reseived message from %s/%s/"sge_U32CFormat": \n",(*sender)->comp_host, (*sender)->comp_name, sge_u32c((*sender)->comp_id)));
      DEBUG((SGE_EVENT,"gdi_rcv: cl_xml_ack_type_t: %s\n",            cl_com_get_mih_mat_string((*message)->message_mat)));
      DEBUG((SGE_EVENT,"gdi_rcv: message tag:       %s\n",            sge_dump_message_tag( (*message)->message_tag) ));
      DEBUG((SGE_EVENT,"gdi_rcv: message id:        "sge_U32CFormat"\n",  sge_u32c((*message)->message_id) ));
      DEBUG((SGE_EVENT,"gdi_rcv: receive time:      %s\n",            sge_ctime((*message)->message_receive_time.tv_sec, &ds)));
      DEBUG((SGE_EVENT,"<<<<<<<<<<<<<<<<<<<<\n"));
   }
   DEXIT;
}

static void dump_send_info(const char* comp_host, const char* comp_name, int comp_id, cl_xml_ack_type_t ack_type, 
                          unsigned long tag, unsigned long* mid) 
{
   char buffer[512];
   dstring ds;

   DENTER(TOP_LAYER, "dump_send_info");
   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if (comp_host != NULL && comp_name != NULL) {
      DEBUG((SGE_EVENT,">>>>>>>>>>>>>>>>>>>>\n"));
      DEBUG((SGE_EVENT,"gdi_snd: sending message to %s/%s/"sge_U32CFormat": \n", 
               (char*)comp_host,comp_name ,sge_u32c(comp_id)));
      DEBUG((SGE_EVENT,"gdi_snd: cl_xml_ack_type_t: %s\n",            cl_com_get_mih_mat_string(ack_type)));
      DEBUG((SGE_EVENT,"gdi_snd: message tag:       %s\n",            sge_dump_message_tag( tag) ));
      if (mid) {
         DEBUG((SGE_EVENT,"gdi_snd: message id:        "sge_U32CFormat"\n",  sge_u32c(*mid) ));
      } else {
         DEBUG((SGE_EVENT,"gdi_snd: message id:        not handled by caller\n"));
      }
      DEBUG((SGE_EVENT,"gdi_snd: send time:         %s\n", sge_ctime(0, &ds)));
      DEBUG((SGE_EVENT,">>>>>>>>>>>>>>>>>>>>\n"));
   } else {
      DEBUG((SGE_EVENT,">>>>>>>>>>>>>>>>>>>>\n"));
      DEBUG((SGE_EVENT,"gdi_snd: some parameters are not set\n"));
      DEBUG((SGE_EVENT,">>>>>>>>>>>>>>>>>>>>\n"));
   }
   DEXIT;
}


/****** sge_ack/sge_send_ack_to_qmaster() **************************************
*  NAME
*     sge_send_ack_to_qmaster() -- ??? 
*
*  SYNOPSIS
*     int sge_send_ack_to_qmaster(int sync, u_long32 type, u_long32 ulong_val, 
*     u_long32 ulong_val_2) 
*
*  FUNCTION
*     Sends an acknowledge to qmaster.
*
*  INPUTS
*     int sync             - ??? 
*     u_long32 type        - ??? 
*     u_long32 ulong_val   - ??? 
*     u_long32 ulong_val_2 - ??? 
*
*  RESULT
*     int - CL_OK on success
*
*  NOTES
*     MT-NOTE: sge_send_ack_to_qmaster() is MT safe (assumptions)
*******************************************************************************/
int sge_gdi2_send_ack_to_qmaster(sge_gdi_ctx_class_t *ctx, int sync, u_long32 type, u_long32 ulong_val, 
                            u_long32 ulong_val_2, lList **alpp) 
{
   int ret;
   sge_pack_buffer pb;

   /* send an ack to the qmaster for the events */
   if(init_packbuffer(&pb, 3*sizeof(u_long32), 0) != PACK_SUCCESS) {
      return CL_RETVAL_MALLOC;
   }

   packint(&pb, type);
   packint(&pb, ulong_val);
   packint(&pb, ulong_val_2);
   ret = sge_gdi2_send_any_request(sync, NULL, ctx, &pb, TAG_ACK_REQUEST, 0, alpp);
/*    ret = sge_send_any_request(sync, NULL, sge_get_master(0), prognames[QMASTER], */
/*                               1, &pb, TAG_ACK_REQUEST, 0, alpp); */
   clear_packbuffer(&pb);
   answer_list_output (alpp);

   return ret;
}
