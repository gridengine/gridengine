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
#include "sge_mtutil.h"
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
#include "sge_hostname.h"
#include "sge_answer.h"
#include "uti/setup_path.h"
#ifdef KERBEROS
#  include "krb_lib.h"
#endif
#include "msg_common.h"
#include "msg_gdilib.h"
#include "gdi/version.h"

#include "sge_env.h"
#include "sge_bootstrap.h"
#include "sge_feature.h"

#include "sge_time.h"

#include "sge_eventL.h"
#include "sge_idL.h"
#include "sge_hostL.h"
#include "sge_confL.h"
#include "sge_host.h"

#include "sge_permissionL.h"

#include "sge_conf.h"
#include "sge_gdi2.h"
#include "sge_security.h"


static void dump_send_info(const char* comp_host, const char* comp_name, int comp_id, cl_xml_ack_type_t ack_type, unsigned long tag, unsigned long* mid);
static bool gdi2_send_multi_sync(sge_gdi_ctx_class_t* ctx, lList **alpp, 
                                 state_gdi_multi *state, sge_gdi_request **answer, 
                                 lList **malpp);
static int sge_get_gdi2_request(sge_gdi_ctx_class_t *ctx,
                                int *commlib_error,
                                char *rhost,
                                char *rcommproc,
                                u_short *rid,
                                sge_gdi_request** arp,
                                unsigned long request_mid);
static int sge_send_gdi2_request(int sync, sge_gdi_ctx_class_t *ctx,
                         sge_gdi_request *ar,
                         u_long32 *mid, unsigned long response_id, lList **alpp);
static int sge_send_receive_gdi2_request(sge_gdi_ctx_class_t *ctx, 
                                         int *commlib_error,
                                         const char *rhost,
                                         const char *rcommproc,
                                         u_short rid,
                                         sge_gdi_request *out,
                                         sge_gdi_request **in,
                                         lList **alpp);
static int sge_get_gdi2_request_async(sge_gdi_ctx_class_t *ctx,
                                      int *commlib_error,
                                      char *rhost,
                                      char *commproc,
                                      u_short *id,
                                      sge_gdi_request** arp,
                                      unsigned long request_mid,
                                      bool is_sync);
static void dump_receive_info(cl_com_message_t** message, cl_com_endpoint_t** sender);

static int gdi2_send_message(sge_gdi_ctx_class_t * ctx, 
                             int synchron, const char *tocomproc, int toid, 
                             const char *tohost, int tag, char *buffer, 
                             int buflen, u_long32 *mid);


lList* sge_gdi2(sge_gdi_ctx_class_t *ctx, u_long32 target, u_long32 cmd, lList **lpp, lCondition *cp,
               lEnumeration *enp) 
{
   lList *alp = NULL;
   lList *mal = NULL;
   u_long32 id;
   state_gdi_multi state = STATE_GDI_MULTI_INIT;

   DENTER(GDI_LAYER, "sge_gdi2");

   PROF_START_MEASUREMENT(SGE_PROF_GDI);

   if ((id = sge_gdi2_multi(ctx, &alp, SGE_GDI_SEND, target, cmd, lpp, 
                              cp, enp, &mal, &state, true)) == -1) {
      PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
      DRETURN(alp);
   }

   sge_gdi_extract_answer(&alp, cmd, target, id, mal, lpp);

   lFreeList(&mal);

   PROF_STOP_MEASUREMENT(SGE_PROF_GDI);

   DRETURN(alp);
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

   DENTER(GDI_LAYER, "sge_gdi2_multi_sync");

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

#if defined(INTERIX)
   /*
    * Map "Administrator" to "root", so the QMaster running on Unix
    * or Linux will accept us as "root"
    */
   if (sge_is_user_superuser(username)==true) {
      strncpy(username, "root", sizeof(username));
   }
#endif  /* defined(INTERIX) */

   DPRINTF(("sge_set_auth_info: username(uid) = %s(%d), groupname = %s(%d)\n",
      username, uid, groupname, gid));

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
   } else {
      state->first = state->last = request;
   }
   
   if (mode == SGE_GDI_SEND) {
#ifdef ASYNC_GDI2   
       gdi2_receive_multi_async(ctx, &answer, malpp, true);
       if (do_sync) {
#endif
         if (!gdi2_send_multi_sync(ctx, alpp, state, &answer, malpp)) {
            goto error;
         }
#ifdef ASYNC_GDI2   
       } else { 
          /* if this is null, we did not get an answer..., which means we return 0;*/
         if (*malpp == NULL) {
            ret = 0;
          }

          if (!gdi2_send_multi_async(ctx, alpp, state)) {
             goto error;
          }
       }
#endif

   }

   PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
   DRETURN(ret);

   error:
      if (alpp != NULL) {
         answer_list_add(alpp, SGE_EVENT, STATUS_NOQMASTER, ANSWER_QUALITY_ERROR);
      }   
      answer = free_gdi_request(answer);
      state->first = free_gdi_request(state->first);
      state->last = NULL;
      state->sequence_id = 0;
      PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
      DRETURN(-1);
}

static bool
gdi2_send_multi_sync(sge_gdi_ctx_class_t* ctx, lList **alpp, state_gdi_multi *state, sge_gdi_request **answer, lList **malpp)
{
   int commlib_error = CL_RETVAL_OK;
   sge_gdi_request *an = NULL;
   int status = 0;
   lListElem *map = NULL;
   lListElem *aep = NULL;
   const char *mastername = ctx->get_master(ctx, false);
   u_long32 sge_qmaster_port = ctx->get_sge_qmaster_port(ctx);
   
   DENTER(GDI_LAYER, "gdi2_send_multi_sync");

#ifdef NEW_GDI_STATE
   state->first->request_id = gdi_state_get_next_request_id(ctx);
#else
   /* the first request in the request list identifies the request uniquely */
   state->first->request_id = gdi_state_get_next_request_id();
#endif   

#ifdef KERBEROS
   /* request that the Kerberos library forward the TGT */
   if (state->first->target == SGE_JOB_LIST && 
         SGE_GDI_GET_OPERATION(state->first->op) == SGE_GDI_ADD) {
      krb_set_client_flags(krb_get_client_flags() | KRB_FORWARD_TGT);
      krb_set_tgt_id(state->first->request_id);
   }
#endif

   status = sge_send_receive_gdi2_request(ctx,
                                          &commlib_error,
                                          mastername, prognames[QMASTER], 1,
                                          state->first,
                                          answer,
                                          alpp);

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
                                      sge_u32c(sge_qmaster_port),
                                      mastername?mastername:"<NULL>"));
            } else { /* For unusual errors, give more detail */
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, 
                                      MSG_GDI_CANT_SEND_MESSAGE_TO_PORT_ON_HOST_SUSS,
                                      prognames[QMASTER],
                                      sge_u32c(sge_qmaster_port),
                                      mastername?mastername:"<NULL>",
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
      ctx->get_master(ctx, true);

      DRETURN(false);
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
   DRETURN(true);
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
static int sge_send_receive_gdi2_request(sge_gdi_ctx_class_t *ctx, 
                                         int *commlib_error,
                                         const char *rhost,
                                         const char *rcommproc,
                                         u_short rid,
                                         sge_gdi_request *out,
                                         sge_gdi_request **in,
                                         lList **alpp)
{
   int ret;
   char rcv_rhost[CL_MAXHOSTLEN+1];
   char rcv_commproc[CL_MAXHOSTLEN+1];
   u_short id = rid;
   u_long32 gdi_request_mid;
   
   DENTER(GDI_LAYER, "sge_send_receive_gdi2_request");

   if (!out) {
      ERROR((SGE_EVENT,
           MSG_GDI_POINTER_NULLLISTPASSEDTOSGESENDRECEIVGDIREQUEST ));
      DRETURN(-1);
   }

   /* we send a gdi request and store the request id */
   ret = sge_send_gdi2_request(1, ctx, out, &gdi_request_mid, 0,
                              alpp);
   *commlib_error = ret;


   DPRINTF(("send request with id "sge_U32CFormat"\n", sge_u32c(gdi_request_mid)));
   if (ret != CL_RETVAL_OK) {
      if ((*commlib_error = ctx->is_alive(ctx)) != CL_RETVAL_OK) {
         DRETURN(-4);
      } else {
         DRETURN(-2);
      }
   }

   strcpy(rcv_rhost, rhost);
   strcpy(rcv_commproc, rcommproc);
   while (!(ret = sge_get_gdi2_request(ctx, commlib_error, rcv_rhost, rcv_commproc, 
                                      &id, in, gdi_request_mid))) {

      DPRINTF(("in: request_id=%d, sequence_id=%d, target=%d, op=%d\n",
            (*in)->request_id, (*in)->sequence_id, (*in)->target, (*in)->op));
      DPRINTF(("out: request_id=%d, sequence_id=%d, target=%d, op=%d\n",
               out->request_id, out->sequence_id, out->target, out->op));

      if (*in && ((*in)->request_id == out->request_id)) {
         break;
      } else {
         *in = free_gdi_request(*in);
         DPRINTF(("<<<<<<<<<<<<<<< GDI MISMATCH >>>>>>>>>>>>>>>>>>>\n"));
      }
   }

   if (ret) {
      if ((*commlib_error = ctx->is_alive(ctx)) != CL_RETVAL_OK) {
         DRETURN(-4);
      } else {
         DRETURN(-3);
      }   
   }
   
   DRETURN(0);
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

   /* TODO: to master only !!!!! */
   const char* commproc = prognames[QMASTER];
   const char* rhost = ctx->get_master(ctx, false);
   int         id   = 1;
   

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
      ret = sge_gdi2_send_any_request(ctx, sync, mid, rhost, commproc, id, &pb,
                              TAG_GDI_REQUEST, response_id, alpp);
   }
   clear_packbuffer(&pb);
   lFreeList(&answer_list);
   PROF_STOP_MEASUREMENT(SGE_PROF_GDI_REQUEST);

   DRETURN(ret);
}

/****** gdi/request/sge_get_gdi2_request() *************************************
*  NAME
*     sge_get_gdi_request() -- ??? 
*
*  SYNOPSIS
*     static int sge_get_gdi2_request(char *host, char *commproc, 
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
*     MT-NOTE: sge_get_gdi2_request() is MT safe (assumptions)
*******************************************************************************/
static int sge_get_gdi2_request(sge_gdi_ctx_class_t *ctx,
                                int *commlib_error,
                                char *rhost,
                                char *rcommproc,
                                u_short *rid,
                                sge_gdi_request** arp,
                                unsigned long request_mid)
{
   return sge_get_gdi2_request_async(ctx, commlib_error, rhost, rcommproc, rid, arp, request_mid, true);
}

static int sge_get_gdi2_request_async(sge_gdi_ctx_class_t *ctx,
                                      int *commlib_error,
                                      char *rhost,
                                      char *rcommproc,
                                      u_short *id,
                                      sge_gdi_request** arp,
                                      unsigned long request_mid,
                                      bool is_sync)
{
   sge_pack_buffer pb;
   int tag = TAG_GDI_REQUEST; /* this is what we want */
   int ret;

   DENTER(GDI_LAYER, "sge_get_gdi2_request_async");
   
   if ((*commlib_error = sge_gdi2_get_any_request(ctx, rhost, rcommproc, id, &pb, &tag, is_sync, request_mid,0)) != CL_RETVAL_OK) {
      DRETURN(-1);
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

   DRETURN(ret);
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
int sge_gdi2_send_any_request(sge_gdi_ctx_class_t *ctx, int synchron, u_long32 *mid,
                         const char *rhost, const char *commproc, int id,
                         sge_pack_buffer *pb, 
                         int tag, u_long32  response_id, lList **alpp)
{
   int i;
   cl_xml_ack_type_t ack_type;
   cl_com_handle_t* handle = ctx->get_com_handle(ctx);
   unsigned long dummy_mid = 0;
   unsigned long* mid_pointer = NULL;

   int         to_port   = ctx->get_sge_qmaster_port(ctx);
   
   DENTER(GDI_LAYER, "sge_gdi2_send_any_request");

   ack_type = CL_MIH_MAT_NAK;

   if (rhost == NULL) {
      answer_list_add(alpp, MSG_GDI_RHOSTISNULLFORSENDREQUEST, STATUS_ESYNTAX,
                      ANSWER_QUALITY_ERROR);
      DRETURN(CL_RETVAL_PARAMS);
   }
   
   if (handle == NULL) {
      answer_list_add(alpp, MSG_GDI_NOCOMMHANDLE, STATUS_NOCOMMD, ANSWER_QUALITY_ERROR);
      DRETURN(CL_RETVAL_HANDLE_NOT_FOUND);
   }

   if (strcmp(commproc, (char*)prognames[QMASTER]) == 0 && id == 1) {
      cl_com_append_known_endpoint_from_name((char*)rhost, (char*)commproc, id, 
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

   i = cl_commlib_send_message(handle, (char*) rhost, (char*) commproc, id,
                                  ack_type, (cl_byte_t*)pb->head_ptr, (unsigned long) pb->bytes_used,
                                  mid_pointer,  response_id,  tag , (cl_bool_t)1, (cl_bool_t)synchron);
   
   if (i != CL_RETVAL_OK) {
      /* try again ( if connection timed out ) */
      i = cl_commlib_send_message(handle, (char*) rhost, (char*) commproc, id,
                                  ack_type, (cl_byte_t*)pb->head_ptr, (unsigned long) pb->bytes_used,
                                  mid_pointer,  response_id,  tag , (cl_bool_t)1, (cl_bool_t)synchron);
   }

   dump_send_info(rhost, commproc, id, ack_type, tag, mid_pointer);
   
   if (mid) {
      *mid = dummy_mid;
   }

   if (i != CL_RETVAL_OK) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT,
                             MSG_GDI_SENDMESSAGETOCOMMPROCFAILED_SSISS ,
                             (synchron ? "" : "a"),
                             commproc,
                             id,
                             rhost,
                             cl_get_error_text(i)));
      answer_list_add(alpp, SGE_EVENT, STATUS_NOCOMMD, ANSWER_QUALITY_ERROR);
   }

   DRETURN(i);
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
sge_gdi2_get_any_request(sge_gdi_ctx_class_t *ctx, char *rhost, char *commproc, u_short *id, sge_pack_buffer *pb, 
                    int *tag, int synchron, u_long32 for_request_mid, u_long32* mid) 
{
   int i;
   ushort usid=0;
   char host[CL_MAXHOSTLEN+1];
   cl_com_message_t* message = NULL;
   cl_com_endpoint_t* sender = NULL;
   cl_com_handle_t* handle = NULL;
   

   DENTER(GDI_LAYER, "sge_gdi2_get_any_request");

   PROF_START_MEASUREMENT(SGE_PROF_GDI);

   if (id) {
      usid = (ushort)*id;
   }   

   if (!rhost) {
      ERROR((SGE_EVENT, MSG_GDI_RHOSTISNULLFORGETANYREQUEST ));
      PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
      DRETURN(-1);
   }
   
   strcpy(host, rhost);

   handle = ctx->get_com_handle(ctx);

   /* trigger communication or wait for a new message (select timeout) */
   cl_commlib_trigger(handle, synchron);

   i = cl_commlib_receive_message(handle, rhost, commproc, usid, (cl_bool_t) synchron, for_request_mid, &message, &sender);

   if ( i == CL_RETVAL_CONNECTION_NOT_FOUND ) {
      if ( commproc[0] != '\0' && rhost[0] != '\0' ) {
         /* The connection was closed, reopen it */
         i = cl_commlib_open_connection(handle, (char*)rhost, (char*)commproc, usid);
         INFO((SGE_EVENT,"reopen connection to %s,%s,"sge_U32CFormat" (2)\n", rhost, commproc, sge_u32c(usid)));
         if (i == CL_RETVAL_OK) {
            INFO((SGE_EVENT,"reconnected successfully\n"));
            i = cl_commlib_receive_message(handle, rhost, commproc, usid, (cl_bool_t) synchron, for_request_mid, &message, &sender);
         }
      } else {
         DEBUG((SGE_EVENT,"can't reopen a connection to unspecified host or commproc (2)\n"));
      }
   }

   if (i != CL_RETVAL_OK) {
      if (i != CL_RETVAL_NO_MESSAGE) {
         /* This if for errors */
         DEBUG((SGE_EVENT, MSG_GDI_RECEIVEMESSAGEFROMCOMMPROCFAILED_SISS , 
               (commproc[0] ? commproc : "any"), 
               (int) usid, 
               (commproc[0] ? commproc : "any"),
                cl_get_error_text(i)));
      }
      cl_com_free_message(&message);
      cl_com_free_endpoint(&sender);
      /* This is if no message is there */
      PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
      DRETURN(i);
   }    

   /* ok, we received a message */
   if (message != NULL ) {
      dump_receive_info(&message, &sender);

      /* TODO: there are two cases for any and addressed communication partner, 
               two functions are needed */
      if (sender != NULL && id) {
         *id = (u_short)sender->comp_id;
      }
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
         DRETURN(CL_RETVAL_READ_ERROR);
      } 

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

      cl_com_free_endpoint(&sender);
      cl_com_free_message(&message);
   }
   PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
   DRETURN(CL_RETVAL_OK);
}

#ifdef ASYNC_GDI2

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
gdi2_receive_multi_async(sge_gdi_ctx_class_t* ctx, sge_gdi_request **answer, lList **malpp, bool is_sync)
{
   char *rcv_rhost = NULL;
   char *rcv_commproc = NULL;
   u_short id = 0;
   u_long32 gdi_request_mid = 0;
   state_gdi_multi *state = NULL;
   gdi_send_t *async_gdi = NULL;
   int commlib_error = CL_RETVAL_OK;
   int ret = 0;
   sge_gdi_request *an = NULL;
   lListElem *map = NULL; 
   
   DENTER(GDI_LAYER, "gdi2_receive_multi_async");

   /* we have to check for an ongoing gdi reqest, if there is none, we have to exit */
#ifdef NEW_GDI_STATE
   if ((async_gdi = gdi_state_get_last_gdi_request(ctx)) != NULL) {
#else
   if ((async_gdi = gdi_state_get_last_gdi_request()) != NULL) {
#endif   
      rcv_rhost = async_gdi->rhost; /* rhost is a char array */
      rcv_commproc = async_gdi->commproc; /* commproc is a char array */
      id = async_gdi->id; /* id is u_short */
      gdi_request_mid = async_gdi->gdi_request_mid;
      state = &(async_gdi->out);
#if 0      
      /* AA: DEBUG async gdi */
      printf("++++++++++ async gdi: rhost/commproc/id: (%s/%s/%d) gdi_request_mid: "sge_u32"\n", 
                 rcv_rhost, rcv_commproc, id, gdi_request_mid);      
#endif
   } else {
#if 0   
      /* AA: DEBUG async gdi */
      printf("-+-+-+-+-+ no async gdi\n"); 
#endif      
      /* nothing todo... */
      DRETURN(true);
   }
   
   /* receive answer */
   while (!(ret = sge_get_gdi2_request_async(ctx, &commlib_error, rcv_rhost, rcv_commproc, &id ,answer, gdi_request_mid, is_sync))) {
      DPRINTF(("in: request_id=%d, sequence_id=%d, target=%d, op=%d\n",
            (*answer)->request_id, (*answer)->sequence_id, (*answer)->target, (*answer)->op));
      DPRINTF(("out: request_id=%d, sequence_id=%d, target=%d, op=%d\n",
               state->first->request_id, state->first->sequence_id, state->first->target, state->first->op));

      if (*answer && ((*answer)->request_id == state->first->request_id)) {
         break;
      } else {
         *answer = free_gdi_request(*answer);
         DPRINTF(("<<<<<<<<<<<<<<< GDI MISMATCH >>>>>>>>>>>>>>>>>>>\n"));
      }
   }
  
   /* process return code */
   if (ret) {
      if (is_sync) {
         if ((commlib_error = ctx->is_alive(ctx)) != CL_RETVAL_OK) {
            /* gdi error */

            /* For the default case, just print a simple message */
            if (commlib_error == CL_RETVAL_CONNECT_ERROR ||
                commlib_error == CL_RETVAL_CONNECTION_NOT_FOUND ) {
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_UNABLE_TO_CONNECT_SUS,
                                   ctx->get_progname(ctx),
                                   sge_u32c(ctx->get_sge_qmaster_port(ctx)), 
                                   ctx->get_master(ctx, false)));
            } else { /* For unusual errors, give more detail */
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, 
                                      MSG_GDI_CANT_SEND_MESSAGE_TO_PORT_ON_HOST_SUSS,
                                      ctx->get_progname(ctx),
                                      sge_u32c(ctx->get_sge_qmaster_port(ctx)), 
                                      ctx->get_master(ctx, false),
                                      cl_get_error_text(commlib_error)));
            }
         } else {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_RECEIVEGDIREQUESTFAILED));
         }
#ifdef NEW_GDI_STATE
         gdi_state_clear_last_gdi_request(ctx); 
#else
         gdi_state_clear_last_gdi_request(); 
#endif         
      }
      DRETURN(false);
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
  
#ifdef NEW_GDI_STATE
   gdi_state_clear_last_gdi_request(ctx); 
#else
   gdi_state_clear_last_gdi_request(); 
#endif         
   
   DRETURN(true);
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
bool
gdi2_send_multi_async(sge_gdi_ctx_class_t *ctx, lList **alpp, state_gdi_multi *state)
{
   int commlib_error = CL_RETVAL_OK;
   lListElem *aep = NULL;

   u_long32 gdi_request_mid = 0;
   u_short id = 1;
   const char *rhost = NULL;
   const char *commproc = prognames[QMASTER];
   
   DENTER(GDI_LAYER, "gdi2_send_multi_async");

   rhost = ctx->get_master(ctx, false);

   /* the first request in the request list identifies the request uniquely */
#ifdef NEW_GDI_STATE
   state->first->request_id = gdi_state_get_next_request_id(ctx);
#else
   state->first->request_id = gdi_state_get_next_request_id();
#endif   

   commlib_error = sge_send_gdi2_request(0, ctx, state->first, &gdi_request_mid, 0, alpp);
   
   /* Print out non-error messages */
   /* TODO SG: check for error messages and warnings */
   for_each (aep, *alpp) {
      if (lGetUlong (aep, AN_quality) == ANSWER_QUALITY_INFO) {
         INFO ((SGE_EVENT, lGetString (aep, AN_text)));
      }
   }
   lFreeList(alpp);
  
   DPRINTF(("send request with id "sge_U32CFormat"\n", sge_u32c(gdi_request_mid)));
   if (commlib_error != CL_RETVAL_OK) {
      if ( (commlib_error = ctx->is_alive(ctx)) != CL_RETVAL_OK) {
         /* gdi error */

         /* For the default case, just print a simple message */
         if (commlib_error == CL_RETVAL_CONNECT_ERROR ||
             commlib_error == CL_RETVAL_CONNECTION_NOT_FOUND ) {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_UNABLE_TO_CONNECT_SUS,
                                   ctx->get_progname(ctx),
                                   sge_u32c(ctx->get_sge_qmaster_port(ctx)), 
                                   ctx->get_master(ctx, false)));
         } else { /* For unusual errors, give more detail */
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, 
                                   MSG_GDI_CANT_SEND_MESSAGE_TO_PORT_ON_HOST_SUSS,
                                   ctx->get_progname(ctx),
                                   sge_u32c(ctx->get_sge_qmaster_port(ctx)), 
                                   ctx->get_master(ctx, false),
                                   cl_get_error_text(commlib_error)));
         }
      } else {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_SENDINGGDIREQUESTFAILED));
      }
      DRETURN(false);
   }
  

   /* we have to store the data for the recieve....  */ 
#ifdef NEW_GDI_STATE   
   gdi_set_request(ctx, rhost, commproc, id, state, gdi_request_mid);
#else
   gdi_set_request(rhost, commproc, id, state, gdi_request_mid);
#endif   
   
   DRETURN(true);
}  
#endif

static void dump_receive_info(cl_com_message_t** message, cl_com_endpoint_t** sender) 
{
   DENTER(GDI_LAYER, "dump_receive_info");
   if ( message  != NULL && sender   != NULL && *message != NULL && *sender  != NULL &&
        (*sender)->comp_host != NULL && (*sender)->comp_name != NULL ) {
         char buffer[512];
         dstring ds;
         sge_dstring_init(&ds, buffer, sizeof(buffer));

      DEBUG((SGE_EVENT,"<<<<<<<<<<<<<<<<<<<<"));
      DEBUG((SGE_EVENT,"gdi_rcv: received message from %s/%s/"sge_U32CFormat": ",(*sender)->comp_host, (*sender)->comp_name, sge_u32c((*sender)->comp_id)));
      DEBUG((SGE_EVENT,"gdi_rcv: cl_xml_ack_type_t: %s",            cl_com_get_mih_mat_string((*message)->message_mat)));
      DEBUG((SGE_EVENT,"gdi_rcv: message tag:       %s",            sge_dump_message_tag( (*message)->message_tag) ));
      DEBUG((SGE_EVENT,"gdi_rcv: message id:        "sge_U32CFormat"",  sge_u32c((*message)->message_id) ));
      DEBUG((SGE_EVENT,"gdi_rcv: receive time:      %s",            sge_ctime((*message)->message_receive_time.tv_sec, &ds)));
      DEBUG((SGE_EVENT,"<<<<<<<<<<<<<<<<<<<<"));
   }
   DEXIT;
}

static void dump_send_info(const char* comp_host, const char* comp_name, int comp_id, cl_xml_ack_type_t ack_type, 
                          unsigned long tag, unsigned long* mid) 
{
   char buffer[512];
   dstring ds;

   DENTER(GDI_LAYER, "dump_send_info");
   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if (comp_host != NULL && comp_name != NULL) {
      DEBUG((SGE_EVENT,">>>>>>>>>>>>>>>>>>>>"));
      DEBUG((SGE_EVENT,"gdi_snd: sending message to %s/%s/"sge_U32CFormat": ", 
               (char*)comp_host,comp_name ,sge_u32c(comp_id)));
      DEBUG((SGE_EVENT,"gdi_snd: cl_xml_ack_type_t: %s",            cl_com_get_mih_mat_string(ack_type)));
      DEBUG((SGE_EVENT,"gdi_snd: message tag:       %s",            sge_dump_message_tag( tag) ));
      if (mid) {
         DEBUG((SGE_EVENT,"gdi_snd: message id:        "sge_U32CFormat"",  sge_u32c(*mid) ));
      } else {
         DEBUG((SGE_EVENT,"gdi_snd: message id:        not handled by caller"));
      }
      DEBUG((SGE_EVENT,"gdi_snd: send time:         %s", sge_ctime(0, &ds)));
      DEBUG((SGE_EVENT,">>>>>>>>>>>>>>>>>>>>"));
   } else {
      DEBUG((SGE_EVENT,">>>>>>>>>>>>>>>>>>>>"));
      DEBUG((SGE_EVENT,"gdi_snd: some parameters are not set"));
      DEBUG((SGE_EVENT,">>>>>>>>>>>>>>>>>>>>"));
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
   /* TODO: to master only !!!!! */
   const char* commproc = prognames[QMASTER];
   const char* rhost = ctx->get_master(ctx, false);
   int         id   = 1;
   
   DENTER(GDI_LAYER, "sge_gdi2_send_ack_to_qmaster");

   /* send an ack to the qmaster for the events */
   if(init_packbuffer(&pb, 3*sizeof(u_long32), 0) != PACK_SUCCESS) {
      DRETURN(CL_RETVAL_MALLOC);
   }

   packint(&pb, type);
   packint(&pb, ulong_val);
   packint(&pb, ulong_val_2);
   ret = sge_gdi2_send_any_request(ctx, sync, NULL, rhost, commproc, id, &pb, TAG_ACK_REQUEST, 0, alpp);
   clear_packbuffer(&pb);
   answer_list_output (alpp);

   DRETURN(ret);
}


/*
** NAME
**   gdi_tsm   - trigger scheduler monitoring 
** PARAMETER
**   schedd_name   - scheduler name  - ignored!
**   cell          - ignored!
** RETURN
**   answer list 
** EXTERNAL
**
** DESCRIPTION
**
** NOTES
**    MT-NOTE: gdi_tsm() is MT safe (assumptions)
*/
lList *gdi2_tsm(
sge_gdi_ctx_class_t *thiz,
const char *schedd_name,
const char *cell 
) {
   lList *alp = NULL;

   DENTER(GDI_LAYER, "gdi2_tsm");

   alp = thiz->gdi(thiz, SGE_SC_LIST, SGE_GDI_TRIGGER, NULL, NULL, NULL); 

   DRETURN(alp);
}

/*
** NAME
**   gdi_kill  - send shutdown/kill request to scheduler, master, execds 
** PARAMETER
**   id_list     - id list, EH_Type or EV_Type
**   cell          - cell, ignored!!!
**   option_flags  - 0
**   action_flag   - combination of MASTER_KILL, SCHEDD_KILL, EXECD_KILL, 
**                                       JOB_KILL 
** RETURN
**   answer list
** EXTERNAL
**
** DESCRIPTION
**
** NOTES
**    MT-NOTE: gdi_kill() is MT safe (assumptions)
*/
lList *gdi2_kill(sge_gdi_ctx_class_t *thiz, lList *id_list, const char *cell, u_long32 option_flags, 
                u_long32 action_flag ) 
{
   lList *alp = NULL, *tmpalp;
   bool id_list_created = false;

   DENTER(GDI_LAYER, "gdi_kill");

   alp = lCreateList("answer", AN_Type);

   if (action_flag & MASTER_KILL) {
      tmpalp = thiz->gdi(thiz, SGE_MASTER_EVENT, SGE_GDI_TRIGGER, NULL, NULL, NULL);
      lAddList(alp, &tmpalp);
   }

   if (action_flag & SCHEDD_KILL) {
      char buffer[10];

      sprintf(buffer, "%d", EV_ID_SCHEDD);
      id_list = lCreateList("kill scheduler", ID_Type);
      id_list_created = true;
      lAddElemStr(&id_list, ID_str, buffer, ID_Type);
      tmpalp = thiz->gdi(thiz, SGE_EVENT_LIST, SGE_GDI_TRIGGER, &id_list, NULL, NULL);
      lAddList(alp, &tmpalp);  
   }

   if (action_flag & EVENTCLIENT_KILL) {
      if (id_list == NULL) {
         char buffer[10];
         sprintf(buffer, "%d", EV_ID_ANY);
         id_list = lCreateList("kill all event clients", ID_Type);
         id_list_created = true;
         lAddElemStr(&id_list, ID_str, buffer, ID_Type);
      }
      tmpalp = thiz->gdi(thiz, SGE_EVENT_LIST, SGE_GDI_TRIGGER, &id_list, NULL, NULL);
      lAddList(alp, &tmpalp);  
   }

   if ((action_flag & EXECD_KILL) || (action_flag & JOB_KILL)) {
      lListElem *hlep = NULL, *hep = NULL;
      lList *hlp = NULL;
      if (id_list != NULL) {
         /*
         ** we have to convert the EH_Type to ID_Type
         ** It would be better to change the call to use ID_Type!
         */
         for_each(hep, id_list) {
            hlep = lAddElemStr(&hlp, ID_str, lGetHost(hep, EH_name), ID_Type);
            lSetUlong(hlep, ID_force, (action_flag & JOB_KILL)?1:0);
         }
      } else {
         hlp = lCreateList("kill all hosts", ID_Type);
         hlep = lCreateElem(ID_Type);
         lSetString(hlep, ID_str, NULL);
         lSetUlong(hlep, ID_force, (action_flag & JOB_KILL)?1:0);
         lAppendElem(hlp, hlep);
      }
      tmpalp = thiz->gdi(thiz, SGE_EXECHOST_LIST, SGE_GDI_TRIGGER, &hlp, NULL, NULL);
      lAddList(alp, &tmpalp);
      lFreeList(&hlp);
   }

   if (id_list_created) {
      lFreeList(&id_list);
   }

   DRETURN(alp);
}

/****** gdi/sge/sge_gdi_get_mapping_name() ************************************
*  NAME
*     sge_gdi_get_mapping_name() -- get username for host 
*
*  SYNOPSIS
*     int sge_gdi_get_mapping_name(char* requestedHost, char* buf, 
*                                  int buflen)
*
*  FUNCTION
*     This function sends a PERM_Type list to the qmaster. The 
*     requestedHost is stored in the PERM_req_host list entry. The 
*     qmaster will fill up the PERM_Type list. The mapped user name 
*     is stored in the PERM_req_username field. The function will strcpy 
*     the name into the "buf" char array if the name is shorter than 
*     the given "buflen". On success the function returns true. 
* 
*  INPUTS
*     char* requestedHost - pointer to char array; this is the name of 
*                           the host were the caller wants to get his 
*                           username.
*     char* buf           - char array buffer to store the username
*     int   buflen        - length (sizeof) buf
*
*  RESULT
*     int true on success, false if not
*
*  SEE ALSO
*     gdilib/sge_gdi_check_permission()
******************************************************************************/
bool sge_gdi2_get_mapping_name(sge_gdi_ctx_class_t *ctx, const char *requestedHost, char *buf,
                             int buflen) 
{  
   lList* alp = NULL;
   lList* permList = NULL;
   lListElem *ep = NULL;
   const char* mapName = NULL;
   
   DENTER(GDI_LAYER, "sge_gdi2_get_mapping_name");

   if (requestedHost == NULL) {
      DRETURN(false);
   }
   
   permList = lCreateList("permissions", PERM_Type);
   ep = lCreateElem(PERM_Type);
   lAppendElem(permList,ep);
   lSetHost(ep, PERM_req_host, requestedHost); 

   alp = ctx->gdi(ctx, SGE_DUMMY_LIST, SGE_GDI_PERMCHECK ,  &permList , NULL,NULL );

   
   if (permList != NULL) {
      ep = permList->first;
      if (ep != NULL) {
         mapName = lGetString(ep, PERM_req_username ); 
      } 
   }
  
   if (mapName != NULL) {
      if ((strlen(mapName) + 1) <= buflen) {
         strcpy(buf,mapName);
         DPRINTF(("Mapping name is: '%s'\n", buf));
   
         lFreeList(&permList);
         lFreeList(&alp);
  
         DRETURN(true);
      }
   } 

   DPRINTF(("No mapname found!\n"));
   strcpy(buf,"");
   
   lFreeList(&permList);
   lFreeList(&alp);
   
   DRETURN(false);
}

/****** gdi/sge/sge_gdi_check_permission() **********************************
*
*  NAME
*     sge_gdi_check_permission() -- check permissions of gdi request 
*
*  SYNOPSIS
*     int sge_gdi_check_permission(int option);
*
*  FUNCTION
*     This function asks the qmaster for the permission (PERM_Type) 
*     list. The option flag specifies which right should be checked. 
*     It can be MANAGER_CHECK or/and OPERATOR_CHECK at this time. If 
*     the caller has access the function returns true.
* 
*  INPUTS
*     int option - check flag (MANAGER_CHECK or OPERATOR_CHECK)
*
*  RESULT
*     bool true if caller has the right, false if not (false if qmaster 
*     not reachable)
* 
*  SEE ALSO
*     gdilib/sge_gdi_get_mapping_name()
*     gdilib/PERM_LOWERBOUND
******************************************************************************/
bool sge_gdi2_check_permission(sge_gdi_ctx_class_t *ctx, lList **alpp, int option) 
{
  bool access_status = false;
  int failed_checks = 0;
  lList* alp = NULL;
  lList* permList = NULL;
  lUlong value;
  
  DENTER(GDI_LAYER, "sge_gdi2_check_permission");

  permList = NULL; 
  alp = ctx->gdi(ctx, SGE_DUMMY_LIST, SGE_GDI_PERMCHECK ,  &permList , NULL,NULL );

  if (permList == NULL) {
     DPRINTF(("Permlist is NULL\n"));
     if (alpp != NULL) {
        if (*alpp == NULL) {
           *alpp = alp;
        } else {
           lAddList(*alpp, &alp);
        }       
     }
     failed_checks++;
     DRETURN(false);
  } else {
     if (permList->first == NULL) {
       DPRINTF(("Permlist has no entries\n")); 
       failed_checks++;
     } else {
       /* check permissions */
  
       /* manager check */
       if (option & MANAGER_CHECK) { 
          value = 0;
          value = lGetUlong(permList->first, PERM_manager);
          if (value != 1) { 
             failed_checks++;
          }
          DPRINTF(("MANAGER_CHECK: %ld\n", value));
       }

       /* operator check */
       if (option & OPERATOR_CHECK) { 
          value = 0;
          value = lGetUlong(permList->first, PERM_operator);
          if (value != 1) { 
             failed_checks++;
          }
          DPRINTF(("OPERATOR_CHECK: %ld\n", value));
       }
       
     }
  }

  lFreeList(&permList);
  lFreeList(&alp);

  if (failed_checks == 0) {
    access_status = true;
  }

  DRETURN(access_status);
}


/**********************************************************************
  send a message giving a packbuffer

  same as gdi_send_message, but this is delivered a sge_pack_buffer.
  this function flushes the z_stream_buffer if compression is turned on
  and passes the result on to send_message
  Always use this function instead of gdi_send_message directly, even
  if compression is turned off.
  
    NOTES
       MT-NOTE: gdi_send_message_pb() is MT safe (assumptions)
**********************************************************************/
int gdi2_send_message_pb(sge_gdi_ctx_class_t *ctx, 
                         int synchron, const char *tocomproc, int toid, 
                         const char *tohost, int tag, sge_pack_buffer *pb, 
                         u_long32 *mid) 
{
   long ret = 0;

   DENTER(GDI_LAYER, "gdi2_send_message_pb");

   if ( !pb ) {
       DPRINTF(("no pointer for sge_pack_buffer\n"));
       ret = gdi2_send_message(ctx, synchron, tocomproc, toid, tohost, tag, NULL, 0, mid);
       DRETURN(ret);
   }

   ret = gdi2_send_message(ctx, synchron, tocomproc, toid, tohost, tag, pb->head_ptr, pb->bytes_used, mid);

   DRETURN(ret);
}

/************************************************************
   TODO: rewrite this function
   COMMLIB/SECURITY WRAPPERS
   FIXME: FUNCTIONPOINTERS SHOULD BE SET IN sge_security_initialize !!!

   Test dlopen functionality, stub libs or check if openssl calls can be added 
   without infringing a copyright

   NOTES
      MT-NOTE: gdi_send_message() is MT safe (assumptions)
*************************************************************/
static int 
gdi2_send_message(sge_gdi_ctx_class_t *sge_ctx, int synchron, const char *tocomproc, int toid, 
                 const char *tohost, int tag, char *buffer, 
                 int buflen, u_long32 *mid) 
{
   int ret;
   cl_com_handle_t* handle = NULL;
   cl_xml_ack_type_t ack_type;
   unsigned long dummy_mid;
   unsigned long* mid_pointer = NULL;
   int use_execd_handle = 0;
   u_long32 progid = sge_ctx->get_who(sge_ctx);
   
   DENTER(GDI_LAYER, "gdi2_send_message");

   /* CR- TODO: This is for tight integration of qrsh -inherit
    *       
    *       All GDI functions normally connect to qmaster, but
    *       qrsh -inhert want's to talk to execd. A second handle
    *       is created. All gdi functions should accept a pointer
    *       to a cl_com_handle_t* handle and use this handle to
    *       send/receive messages to the correct endpoint.
    */
   if ( tocomproc[0] == '\0') {
      DEBUG((SGE_EVENT,"tocomproc is empty string\n"));
   }
   switch (progid) {
      case QMASTER:
      case EXECD:
         use_execd_handle = 0;
         break;
      default:
         if (strcmp(tocomproc,prognames[QMASTER]) == 0) {
            use_execd_handle = 0;
         } else {
            if (tocomproc != NULL && tocomproc[0] != '\0') {
               use_execd_handle = 1;
            }
         }
   }
   
 
   if (use_execd_handle == 0) {
      /* normal gdi send to qmaster */
      DEBUG((SGE_EVENT,"standard gdi request to qmaster\n"));
      handle = sge_ctx->get_com_handle(sge_ctx);
   } else {
      /* we have to send a message to another component than qmaster */
      DEBUG((SGE_EVENT,"search handle to \"%s\"\n", tocomproc));
      handle = cl_com_get_handle("execd_handle", 0);
      if (handle == NULL) {
         int commlib_error = CL_RETVAL_OK;
         cl_framework_t  communication_framework = CL_CT_TCP;
         DEBUG((SGE_EVENT,"creating handle to \"%s\"\n", tocomproc));
         if (feature_is_enabled(FEATURE_CSP_SECURITY)) {
            DPRINTF(("using communication lib with SSL framework (execd_handle)\n"));
            communication_framework = CL_CT_SSL;
         }
         cl_com_create_handle(&commlib_error, communication_framework, CL_CM_CT_MESSAGE,
                              CL_FALSE, sge_get_execd_port(), CL_TCP_DEFAULT,
                              "execd_handle" , 0 , 1 , 0 );
         handle = cl_com_get_handle("execd_handle", 0);
         if (handle == NULL) {
            ERROR((SGE_EVENT,MSG_GDI_CANT_CREATE_HANDLE_TOEXECD_S, tocomproc));
            ERROR((SGE_EVENT,cl_get_error_text(commlib_error)));
         }
      }
   }

   ack_type = CL_MIH_MAT_NAK;
   if (synchron) {
      ack_type = CL_MIH_MAT_ACK;
   }
   if (mid != NULL) {
      mid_pointer = &dummy_mid;
   }

   ret = cl_commlib_send_message(handle, (char*)tohost ,(char*)tocomproc ,toid , 
                                 ack_type, (cl_byte_t*)buffer, (unsigned long)buflen,
                                 mid_pointer, 0, tag, (cl_bool_t)1, (cl_bool_t)synchron);
   if (ret != CL_RETVAL_OK) {
      /* try again ( if connection timed out) */
      ret = cl_commlib_send_message(handle, (char*)tohost ,(char*)tocomproc ,toid , 
                                    ack_type, (cl_byte_t*)buffer, (unsigned long)buflen,
                                    mid_pointer, 0, tag, (cl_bool_t)1, (cl_bool_t)synchron);
   }

   if (mid != NULL) {
      *mid = dummy_mid;
   }

   DRETURN(ret);
}


/* 
 *  TODO: rewrite this function
 *  NOTES
 *     MT-NOTE: gdi_receive_message() is MT safe (major assumptions!)
 *
 */
int 
gdi2_receive_message(sge_gdi_ctx_class_t *sge_ctx, char *fromcommproc, u_short *fromid, char *fromhost, 
                    int *tag, char **buffer, u_long32 *buflen, int synchron) 
{
   
   int ret;
   cl_com_handle_t* handle = NULL;
   cl_com_message_t* message = NULL;
   cl_com_endpoint_t* sender = NULL;
   int use_execd_handle = 0;

   u_long32 progid = sge_ctx->get_who(sge_ctx);
   u_long32 sge_execd_port = sge_ctx->get_sge_execd_port(sge_ctx);


   DENTER(GDI_LAYER, "gdi2_receive_message");

      /* CR- TODO: This is for tight integration of qrsh -inherit
    *       
    *       All GDI functions normally connect to qmaster, but
    *       qrsh -inhert want's to talk to execd. A second handle
    *       is created. All gdi functions should accept a pointer
    *       to a cl_com_handle_t* handle and use this handle to
    *       send/receive messages to the correct endpoint.
    */


   if ( fromcommproc[0] == '\0') {
      DEBUG((SGE_EVENT,"fromcommproc is empty string\n"));
   }
   switch (progid) {
      case QMASTER:
      case EXECD:
         use_execd_handle = 0;
         break;
      default:
         if (strcmp(fromcommproc,prognames[QMASTER]) == 0) {
            use_execd_handle = 0;
         } else {
            if (fromcommproc != NULL && fromcommproc[0] != '\0') {
               use_execd_handle = 1;
            }
         }
   }

   if (use_execd_handle == 0) {
      /* normal gdi send to qmaster */
      DEBUG((SGE_EVENT,"standard gdi receive message\n"));
      handle = sge_ctx->get_com_handle(sge_ctx);
   } else {
      /* we have to send a message to another component than qmaster */
      DEBUG((SGE_EVENT,"search handle to \"%s\"\n", fromcommproc));
      handle = cl_com_get_handle("execd_handle", 0);
      if (handle == NULL) {
         int commlib_error = CL_RETVAL_OK;
         cl_framework_t  communication_framework = CL_CT_TCP;
         DEBUG((SGE_EVENT,"creating handle to \"%s\"\n", fromcommproc));
         if (feature_is_enabled(FEATURE_CSP_SECURITY)) {
            DPRINTF(("using communication lib with SSL framework (execd_handle)\n"));
            communication_framework = CL_CT_SSL;
         }
         
         cl_com_create_handle(&commlib_error, communication_framework, CL_CM_CT_MESSAGE,
                              CL_FALSE, sge_execd_port, CL_TCP_DEFAULT, 
                              "execd_handle" , 0 , 1 , 0 );
         handle = cl_com_get_handle("execd_handle", 0);
         if (handle == NULL) {
            ERROR((SGE_EVENT,MSG_GDI_CANT_CREATE_HANDLE_TOEXECD_S, fromcommproc));
            ERROR((SGE_EVENT,cl_get_error_text(commlib_error)));
         }
      }
   } 

   ret = cl_commlib_receive_message(handle, fromhost, fromcommproc, *fromid, (cl_bool_t)synchron, 0, &message, &sender);

   if (ret == CL_RETVAL_CONNECTION_NOT_FOUND ) {
      if ( fromcommproc[0] != '\0' && fromhost[0] != '\0' ) {
          /* The connection was closed, reopen it */
          ret = cl_commlib_open_connection(handle,fromhost,fromcommproc, *fromid);
          INFO((SGE_EVENT,"reopen connection to %s,%s,"sge_U32CFormat" (1)\n", fromhost , fromcommproc , sge_u32c(*fromid)));
          if (ret == CL_RETVAL_OK) {
             INFO((SGE_EVENT,"reconnected successfully\n"));
             ret = cl_commlib_receive_message(handle, fromhost, fromcommproc, *fromid, (cl_bool_t) synchron, 0, &message, &sender);
          } 
      } else {
         DEBUG((SGE_EVENT,"can't reopen a connection to unspecified host or commproc (1)\n"));
      }
   }

   if (message != NULL && ret == CL_RETVAL_OK) {
      *buffer = (char *)message->message;
      message->message = NULL;
      *buflen = message->message_length;
      if (tag) {
         *tag = (int)message->message_tag;
      }

      if (sender != NULL) {
         DEBUG((SGE_EVENT,"received from: %s,"sge_U32CFormat"\n",sender->comp_host, sge_u32c(sender->comp_id)));
         if (fromcommproc != NULL && fromcommproc[0] == '\0') {
            strcpy(fromcommproc, sender->comp_name);
         }
         if (fromhost != NULL) {
            strcpy(fromhost, sender->comp_host);
         }
         if (fromid != NULL) {
            *fromid = (u_short)sender->comp_id;
         }
      }
   }
   cl_com_free_message(&message);
   cl_com_free_endpoint(&sender);

   DRETURN(ret);
   
}


/*-------------------------------------------------------------------------*
 * NAME
 *   get_configuration - retrieves configuration from qmaster
 * PARAMETER
 *   config_name       - name of local configuration or "global",
 *                       name is being resolved before action
 *   gepp              - pointer to list element containing global
 *                       configuration, CONF_Type, should point to NULL
 *                       or otherwise will be freed
 *   lepp              - pointer to list element containing local configuration
 *                       by name given by config_name, can be NULL if global
 *                       configuration is requested, CONF_Type, should point
 *                       to NULL or otherwise will be freed
 * RETURN
 *    0   on success
 *   -1   NULL pointer received
 *   -2   error resolving host
 *   -3   invalid NULL pointer received for local configuration
 *   -4   request to qmaster failed
 *   -5   there is no global configuration
 *   -6   commproc already registered
 *   -7   no permission to get configuration
 * EXTERNAL
 *
 * DESCRIPTION
 *   retrieves a configuration from the qmaster. If the configuration
 *   "global" is requested, then this function requests only this one.
 *   If not, both the global configuration and the requested local
 *   configuration are retrieved.
 *   This function was introduced to make execution hosts independent
 *   of being able to mount the local_conf directory.
 *-------------------------------------------------------------------------*/
int gdi2_get_configuration(
sge_gdi_ctx_class_t *ctx,
const char *config_name,
lListElem **gepp,
lListElem **lepp 
) {
   lCondition *where;
   lEnumeration *what;
   lList *alp = NULL;
   lList *lp = NULL;
   int is_global_requested = 0;
   int ret;
   lListElem *hep = NULL;
   int success;
   static int already_logged = 0;
   u_long32 status;
   u_long32 me = ctx->get_who(ctx);
   
   DENTER(TOP_LAYER, "gdi2_get_configuration");

   if (!config_name || !gepp) {
      DRETURN(-1);
   }

   if (*gepp) {
      lFreeElem(gepp);
   }
   if (lepp && *lepp) {
      lFreeElem(lepp);
   }

   if (!strcasecmp(config_name, "global")) {
      is_global_requested = 1;
   } else {
      hep = lCreateElem(EH_Type);
      lSetHost(hep, EH_name, config_name);

      ret = sge_resolve_host(hep, EH_name);

      if (ret != CL_RETVAL_OK) {
         DPRINTF(("get_configuration: error %d resolving host %s: %s\n", ret, config_name, cl_get_error_text(ret)));
         lFreeElem(&hep);
         ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, config_name));
         DRETURN(-2);
      }
      DPRINTF(("get_configuration: unique for %s: %s\n", config_name, lGetHost(hep, EH_name)));

      if (sge_get_com_error_flag(me, SGE_COM_ACCESS_DENIED)       == true ||
          sge_get_com_error_flag(me, SGE_COM_ENDPOINT_NOT_UNIQUE) == true) {
         lFreeElem(&hep);
         DRETURN(-6);
      }
   }

   if (!is_global_requested && !lepp) {
      ERROR((SGE_EVENT, MSG_NULLPOINTER));
      lFreeElem(&hep);
      DRETURN(-3);
   }

   if (is_global_requested) {
      /*
       * they might otherwise send global twice
       */
      where = lWhere("%T(%I c= %s)", CONF_Type, CONF_hname, SGE_GLOBAL_NAME);
      DPRINTF(("requesting global\n"));
   } else {
      where = lWhere("%T(%I c= %s || %I h= %s)", CONF_Type, CONF_hname, SGE_GLOBAL_NAME, CONF_hname,
                     lGetHost(hep, EH_name));
      DPRINTF(("requesting global and %s\n", lGetHost(hep, EH_name)));
   }
   what = lWhat("%T(ALL)", CONF_Type);
   alp = ctx->gdi(ctx, SGE_CONFIG_LIST, SGE_GDI_GET, &lp, where, what);

   lFreeWhat(&what);
   lFreeWhere(&where);

   success = ((status= lGetUlong(lFirst(alp), AN_status)) == STATUS_OK);
   if (!success) {
      if (!already_logged) {
         ERROR((SGE_EVENT, MSG_CONF_GETCONF_S, lGetString(lFirst(alp), AN_text)));
         already_logged = 1;       
      }
                   
      lFreeList(&alp);
      lFreeList(&lp);
      lFreeElem(&hep);
      DRETURN((status != STATUS_EDENIED2HOST)?-4:-7);
   }
   lFreeList(&alp);

   if (lGetNumberOfElem(lp) > (2 - is_global_requested)) {
      WARNING((SGE_EVENT, MSG_CONF_REQCONF_II, 2 - is_global_requested, lGetNumberOfElem(lp)));
   }

   if (!(*gepp = lGetElemHost(lp, CONF_hname, SGE_GLOBAL_NAME))) {
      ERROR((SGE_EVENT, MSG_CONF_NOGLOBAL));
      lFreeList(&lp);
      lFreeElem(&hep);
      DRETURN(-5);
   }
   lDechainElem(lp, *gepp);

   if (!is_global_requested) {
      if (!(*lepp = lGetElemHost(lp, CONF_hname, lGetHost(hep, EH_name)))) {
         if (*gepp) {
            WARNING((SGE_EVENT, MSG_CONF_NOLOCAL_S, lGetHost(hep, EH_name)));
         }
         lFreeList(&lp);
         lFreeElem(&hep);
         already_logged = 0;
         DRETURN(0);
      }
      lDechainElem(lp, *lepp);
   }
   
   lFreeElem(&hep);
   lFreeList(&lp);
   already_logged = 0;
   DRETURN(0);
}


int gdi2_get_conf_and_daemonize(
sge_gdi_ctx_class_t *ctx,
tDaemonizeFunc dfunc,
lList **conf_list,
volatile int* abort_flag
) {
   lListElem *global = NULL;
   lListElem *local = NULL;
   int sleep_counter = 0;
   cl_com_handle_t* handle = NULL;
   int ret_val;
   int ret;
   
   const char *qualified_hostname = ctx->get_qualified_hostname(ctx);
   const char *cell_root = ctx->get_cell_root(ctx);
   u_long32 progid = ctx->get_who(ctx);
   

   DENTER(TOP_LAYER, "gdi2_get_conf_and_daemonize");
   /*
    * for better performance retrieve 2 configurations
    * in one gdi call
    */
   DPRINTF(("qualified hostname: %s\n",  qualified_hostname));

   while ((ret = gdi2_get_configuration(ctx, qualified_hostname, &global, &local))) {
      if (ret==-6 || ret==-7) {
         /* confict: COMMPROC ALREADY REGISTERED */
         DRETURN(-1);
      }
      if (!ctx->is_daemonized(ctx)) {
         /* do not daemonize the first time to be able
            to report communication errors to stdout/stderr */
         if (!getenv("SGE_ND") && sleep_counter > 2) {
            ERROR((SGE_EVENT, MSG_CONF_NOCONFBG));
            dfunc(ctx);
         }
         handle = ctx->get_com_handle(ctx);
         ret_val = cl_commlib_trigger(handle, 1);
         switch(ret_val) {
            case CL_RETVAL_SELECT_TIMEOUT:
            case CL_RETVAL_OK:
               break;
            default:
               sleep(1);  /* for other errors */
               break;
         }
         sleep_counter++;
      } else {
         /* here we are daemonized, we do a longer sleep when there is no connection */
         DTRACE;
         handle = ctx->get_com_handle(ctx);
         ret_val = cl_commlib_trigger(handle, 1);
         switch(ret_val) {
            case CL_RETVAL_SELECT_TIMEOUT:
               sleep(30);  /* If we could not establish the connection */
               break;
            case CL_RETVAL_OK:
               break;
            default:
               sleep(30);  /* for other errors */
               break;
         }
      }
      if (abort_flag != NULL) {
         if (*abort_flag != 0) {
            DRETURN(-2);
         }
      }
   }
  
   ret = merge_configuration(NULL, progid, cell_root, global, local, NULL);
   if (ret) {
      DPRINTF(("Error %d merging configuration \"%s\"\n", ret, qualified_hostname));
   }

   /*
    * we don't keep all information, just the name and the version
    * the entries are freed
    */
   lSetList(global, CONF_entries, NULL);
   lSetList(local, CONF_entries, NULL);
   lFreeList(conf_list);
   *conf_list = lCreateList("config list", CONF_Type);
   lAppendElem(*conf_list, global);
   lAppendElem(*conf_list, local);
   DRETURN(0);
}

/*-------------------------------------------------------------------------*
 * NAME
 *   get_merged_conf - requests new configuration set from master
 * RETURN
 *   -1      - could not get configuration from qmaster
 *   -2      - could not merge global and local configuration
 * EXTERNAL
 *
 *-------------------------------------------------------------------------*/
int gdi2_get_merged_configuration(
sge_gdi_ctx_class_t *ctx,
lList **conf_list
) {
   lListElem *global = NULL;
   lListElem *local = NULL;
   const char *qualified_hostname = ctx->get_qualified_hostname(ctx);
   const char *cell_root = ctx->get_cell_root(ctx);
   u_long32 progid = ctx->get_who(ctx);
   int ret;

   DENTER(TOP_LAYER, "gdi2_get_merged_configuration");

   DPRINTF(("qualified hostname: %s\n",  qualified_hostname));
   ret = gdi2_get_configuration(ctx, qualified_hostname, &global, &local);
   if (ret) {
      ERROR((SGE_EVENT, MSG_CONF_NOREADCONF_IS, ret, qualified_hostname));
      lFreeElem(&global);
      lFreeElem(&local);
      DRETURN(-1);
   }

   ret = merge_configuration(NULL, progid, cell_root, global, local, NULL);
   if (ret) {
      ERROR((SGE_EVENT, MSG_CONF_NOMERGECONF_IS, ret, qualified_hostname));
      lFreeElem(&global);
      lFreeElem(&local);
      DRETURN(-2);
   }
   /*
    * we don't keep all information, just the name and the version
    * the entries are freed
    */
   lSetList(global, CONF_entries, NULL);
   lSetList(local, CONF_entries, NULL);

   lFreeList(conf_list);
   *conf_list = lCreateList("config list", CONF_Type);
   lAppendElem(*conf_list, global);
   lAppendElem(*conf_list, local);

   DRETURN(0);
}


static void gdi2_default_exit_func(sge_gdi_ctx_class_t **ref_ctx, int i) 
{
   sge_security_exit(i); 
   cl_com_cleanup_commlib();
}

/****** gdi/setup/sge_gdi_shutdown() ******************************************
*  NAME
*     sge_gdi_shutdown() -- gdi shutdown.
*
*  SYNOPSIS
*     int sge_gdi_shutdown()
*
*  FUNCTION
*     This function has to be called before quitting the program. It 
*     cancels registration at commd.
*
*  NOTES
*     MT-NOTES: sge_gdi_setup() is MT safe
******************************************************************************/  
int sge_gdi2_shutdown(void **context)
{
   sge_gdi_ctx_class_t **ref_ctx = (sge_gdi_ctx_class_t **)context;

   DENTER(TOP_LAYER, "sge_gdi2_shutdown");

   /* initialize libraries */
/*    pthread_once(&gdi_once_control, gdi_once_init); */
   gdi2_default_exit_func(ref_ctx, 0);

   DRETURN(0);
}

/****** sgeobj/sge_report/report_list_send() ******************************************
*  NAME
*     report_list_send() -- Send a list of reports.
*
*  SYNOPSIS
*     int report_list_send(const lList *rlp, const char *rhost,
*                          const char *commproc, int id,
*                          int synchron, u_long32 *mid)
*
*  FUNCTION
*     Send a list of reports.
*
*  INPUTS
*     const lList *rlp     - REP_Type list
*     const char *rhost    - Hostname
*     const char *commproc - Component name
*     int id               - Component id
*     int synchron         - true or false
*     u_long32 *mid        - Message id
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Unexpected error
*        -2 - No memory
*        -3 - Format error
*        other - see sge_send_any_request()
*
*  NOTES
*     MT-NOTE: report_list_send() is not MT safe (assumptions)
*******************************************************************************/
int report_list_send(sge_gdi_ctx_class_t *ctx, 
                     const lList *rlp, 
                     const char *rhost, const char *commproc, int id,
                     int synchron, u_long32 *mid)
{
   sge_pack_buffer pb;
   int ret, size;
   lList *alp = NULL;

   DENTER(TOP_LAYER, "report_list_send");

   /* retrieve packbuffer size to avoid large realloc's while packing */
   init_packbuffer(&pb, 0, 1);
   ret = cull_pack_list(&pb, rlp);
   size = pb_used(&pb);
   clear_packbuffer(&pb);

   /* prepare packing buffer */
   if((ret = init_packbuffer(&pb, size, 0)) == PACK_SUCCESS) {
      ret = cull_pack_list(&pb, rlp);
   }

   switch (ret) {
   case PACK_SUCCESS:
      break;

   case PACK_ENOMEM:
      ERROR((SGE_EVENT, MSG_GDI_REPORTNOMEMORY_I , size));
      clear_packbuffer(&pb);
      DEXIT;
      return -2;

   case PACK_FORMAT:
      ERROR((SGE_EVENT, MSG_GDI_REPORTFORMATERROR));
      clear_packbuffer(&pb);
      DEXIT;
      return -3;

   default:
      ERROR((SGE_EVENT, MSG_GDI_REPORTUNKNOWERROR));
      clear_packbuffer(&pb);
      DEXIT;
      return -1;
   }

   ret = sge_gdi2_send_any_request(ctx, synchron, mid, rhost, commproc, id, &pb, TAG_REPORT_REQUEST, 0, &alp);

   clear_packbuffer(&pb);
   answer_list_output (&alp);

   DEXIT;
   return ret;
}
/************* COMMLIB HANDLERS from sge_any_request ************************/

/* setup a communication error callback and mutex for it */
static pthread_mutex_t general_communication_error_mutex = PTHREAD_MUTEX_INITIALIZER;


/* local static struct to store communication errors. The boolean
 * values com_access_denied and com_endpoint_not_unique will never be
 * restored to false again 
 */
typedef struct sge_gdi_com_error_type {
   int  com_error;                        /* current commlib error */
   bool com_was_error;                    /* set if there was an communication error (but not CL_RETVAL_ACCESS_DENIED or CL_RETVAL_ENDPOINT_NOT_UNIQUE)*/
   int  com_last_error;                   /* last logged commlib error */
   bool com_access_denied;                /* set when commlib reports CL_RETVAL_ACCESS_DENIED */
   int  com_access_denied_counter;        /* counts access denied errors (TODO: workaround for BT: 6350264, IZ: 1893) */
   unsigned long com_access_denied_time; /* timeout for counts access denied errors (TODO: workaround for BT: 6350264, IZ: 1893) */
   bool com_endpoint_not_unique;          /* set when commlib reports CL_RETVAL_ENDPOINT_NOT_UNIQUE */
   int  com_endpoint_not_unique_counter;  /* counts access denied errors (TODO: workaround for BT: 6350264, IZ: 1893) */
   unsigned long com_endpoint_not_unique_time; /* timeout for counts access denied errors (TODO: workaround for BT: 6350264, IZ: 1893) */
} sge_gdi_com_error_t;
static sge_gdi_com_error_t sge_gdi_communication_error = {CL_RETVAL_OK,
                                                          false,
                                                          CL_RETVAL_OK,
                                                          false, 0, 0,
                                                          false, 0, 0};


/****** sge_any_request/sge_dump_message_tag() *************************************
*  NAME
*     sge_dump_message_tag() -- get tag name string
*
*  SYNOPSIS
*     const char* sge_dump_message_tag(int tag) 
*
*  FUNCTION
*     This is a function used for getting a printable string output for the
*     different message tags.
*     (Useful for debugging)
*
*  INPUTS
*     int tag - tag value
*
*  RESULT
*     const char* - name of tag
*
*  NOTES
*     MT-NOTE: sge_dump_message_tag() is MT safe 
*******************************************************************************/
const char* sge_dump_message_tag(unsigned long tag) {
   switch (tag) {
      case TAG_NONE:
         return "TAG_NONE";
      case TAG_OLD_REQUEST:
         return "TAG_OLD_REQUEST";
      case TAG_GDI_REQUEST:
         return "TAG_GDI_REQUEST";
      case TAG_ACK_REQUEST:
         return "TAG_ACK_REQUEST";
      case TAG_REPORT_REQUEST:
         return "TAG_REPORT_REQUEST";
      case TAG_FINISH_REQUEST:
         return "TAG_FINISH_REQUEST";
      case TAG_JOB_EXECUTION:
         return "TAG_JOB_EXECUTION";
      case TAG_SLAVE_ALLOW:
         return "TAG_SLAVE_ALLOW";
      case TAG_CHANGE_TICKET:
         return "TAG_CHANGE_TICKET";
      case TAG_SIGJOB:
         return "TAG_SIGJOB";
      case TAG_SIGQUEUE:
         return "TAG_SIGQUEUE";
      case TAG_KILL_EXECD:
         return "TAG_KILL_EXECD";
      case TAG_NEW_FEATURES:
         return "TAG_NEW_FEATURES";
      case TAG_GET_NEW_CONF:
         return "TAG_GET_NEW_CONF";
      case TAG_JOB_REPORT:
         return "TAG_JOB_REPORT";
      case TAG_QSTD_QSTAT:
         return "TAG_QSTD_QSTAT";
      case TAG_TASK_EXIT:
         return "TAG_TASK_EXIT";
      case TAG_TASK_TID:
         return "TAG_TASK_TID";
      case TAG_EVENT_CLIENT_EXIT:
         return "TAG_EVENT_CLIENT_EXIT";
      default:
         break;
   }
   return "TAG_NOT_DEFINED";
}


int gdi_log_flush_func(cl_raw_list_t* list_p) {
   int ret_val;
   cl_log_list_elem_t* elem = NULL;
   DENTER(COMMD_LAYER, "gdi_log_flush_func");

   if (list_p == NULL) {
      DRETURN(CL_RETVAL_LOG_NO_LOGLIST);
   }

   if (  ( ret_val = cl_raw_list_lock(list_p)) != CL_RETVAL_OK) {
      DRETURN(ret_val);
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
               ERROR((SGE_EVENT,  "%s %-20s=> %s %s", elem->log_module_name, elem->log_thread_name, elem->log_message, param ));
            } else {
               printf("%s %-20s=> %s %s\n", elem->log_module_name, elem->log_thread_name, elem->log_message, param);
            }
            break;
         case CL_LOG_WARNING:
            if ( log_state_get_log_level() >= LOG_WARNING) {
               WARNING((SGE_EVENT,"%s %-20s=> %s %s", elem->log_module_name, elem->log_thread_name, elem->log_message, param ));
            } else {
               printf("%s %-20s=> %s %s\n", elem->log_module_name, elem->log_thread_name, elem->log_message, param);
            }
            break;
         case CL_LOG_INFO:
            if ( log_state_get_log_level() >= LOG_INFO) {
               INFO((SGE_EVENT,   "%s %-20s=> %s %s", elem->log_module_name, elem->log_thread_name, elem->log_message, param ));
            } else {
               printf("%s %-20s=> %s %s\n", elem->log_module_name, elem->log_thread_name, elem->log_message, param);
            }
            break;
         case CL_LOG_DEBUG:
            if ( log_state_get_log_level() >= LOG_DEBUG) { 
               DEBUG((SGE_EVENT,  "%s %-20s=> %s %s", elem->log_module_name, elem->log_thread_name, elem->log_message, param ));
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
      DRETURN(ret_val);
   } 
   DRETURN(CL_RETVAL_OK);
}


#ifdef DEBUG_CLIENT_SUPPORT
void gdi_rmon_print_callback_function(const char *progname, const char *message, unsigned long traceid, unsigned long pid, unsigned long thread_id) {
   cl_com_handle_t* handle = NULL;

   handle = cl_com_get_handle(progname ,0);
   if (handle != NULL) {
      cl_com_application_debug(handle, message);
   }
}
#endif

/****** sge_any_request/general_communication_error() **************************
*  NAME
*     general_communication_error() -- callback for communication errors
*
*  SYNOPSIS
*     static void general_communication_error(int cl_error, 
*                                             const char* error_message) 
*
*  FUNCTION
*     This function is used by cl_com_set_error_func() to set the default
*     application error function for communication errors. On important 
*     communication errors the communication lib will call this function
*     with a corresponding error number (within application context).
*
*     This function should never block. Treat it as a kind of signal handler.
*    
*     The error_message parameter is freed by the commlib.
*
*  INPUTS
*     int cl_error              - commlib error number
*     const char* error_message - additional error text message
*
*  NOTES
*     MT-NOTE: general_communication_error() is MT safe 
*     (static struct variable "sge_gdi_communication_error" is used)
*
*
*  SEE ALSO
*     sge_any_request/sge_get_com_error_flag()
*******************************************************************************/
void general_communication_error(const cl_application_error_list_elem_t* commlib_error) {
   DENTER(TOP_LAYER, "general_communication_error");
   if (commlib_error != NULL) {
      struct timeval now;
      unsigned long time_diff = 0;

      sge_mutex_lock("general_communication_error_mutex",
                     SGE_FUNC, __LINE__, &general_communication_error_mutex);  

      /* save the communication error to react later */
      sge_gdi_communication_error.com_error = commlib_error->cl_error;

      switch (commlib_error->cl_error) {
         case CL_RETVAL_OK: {
            break;
         }
         case CL_RETVAL_ACCESS_DENIED: {
            if (sge_gdi_communication_error.com_access_denied == false) {
               /* counts access denied errors (TODO: workaround for BT: 6350264, IZ: 1893) */
               /* increment counter only once per second and allow max CL_DEFINE_READ_TIMEOUT + 2 access denied */
               gettimeofday(&now,NULL);
               if ( (now.tv_sec - sge_gdi_communication_error.com_access_denied_time) > (3*CL_DEFINE_READ_TIMEOUT) ) {
                  sge_gdi_communication_error.com_access_denied_time = 0;
                  sge_gdi_communication_error.com_access_denied_counter = 0;
               }

               if (sge_gdi_communication_error.com_access_denied_time < now.tv_sec) {
                  if (sge_gdi_communication_error.com_access_denied_time == 0) {
                     time_diff = 1;
                  } else {
                     time_diff = now.tv_sec - sge_gdi_communication_error.com_access_denied_time;
                  }
                  sge_gdi_communication_error.com_access_denied_counter += time_diff;
                  if (sge_gdi_communication_error.com_access_denied_counter > (2*CL_DEFINE_READ_TIMEOUT) ) {
                     sge_gdi_communication_error.com_access_denied = true;
                  }
                  sge_gdi_communication_error.com_access_denied_time = now.tv_sec;
               }
            }
            break;
         }
         case CL_RETVAL_ENDPOINT_NOT_UNIQUE: {
            if (sge_gdi_communication_error.com_endpoint_not_unique == false) {
               /* counts endpoint not unique errors (TODO: workaround for BT: 6350264, IZ: 1893) */
               /* increment counter only once per second and allow max CL_DEFINE_READ_TIMEOUT + 2 endpoint not unique */
               DPRINTF(("got endpint not unique"));
               gettimeofday(&now,NULL);
               if ( (now.tv_sec - sge_gdi_communication_error.com_endpoint_not_unique_time) > (3*CL_DEFINE_READ_TIMEOUT) ) {
                  sge_gdi_communication_error.com_endpoint_not_unique_time = 0;
                  sge_gdi_communication_error.com_endpoint_not_unique_counter = 0;
               }

               if (sge_gdi_communication_error.com_endpoint_not_unique_time < now.tv_sec) {
                  if (sge_gdi_communication_error.com_endpoint_not_unique_time == 0) {
                     time_diff = 1;
                  } else {
                     time_diff = now.tv_sec - sge_gdi_communication_error.com_endpoint_not_unique_time;
                  }
                  sge_gdi_communication_error.com_endpoint_not_unique_counter += time_diff;
                  if (sge_gdi_communication_error.com_endpoint_not_unique_counter > (2*CL_DEFINE_READ_TIMEOUT) ) {
                     sge_gdi_communication_error.com_endpoint_not_unique = true;
                  }
                  sge_gdi_communication_error.com_endpoint_not_unique_time = now.tv_sec;
               }
            }
            break;
         }
         default: {
            sge_gdi_communication_error.com_was_error = true;
            break;
         }
      }


      /*
       * now log the error if not already reported the 
       * least CL_DEFINE_MESSAGE_DUP_LOG_TIMEOUT seconds
       */
      if (commlib_error->cl_already_logged == CL_FALSE && 
         sge_gdi_communication_error.com_last_error != sge_gdi_communication_error.com_error) {

         /*  never log the same messages again and again (commlib
          *  will erase cl_already_logged flag every CL_DEFINE_MESSAGE_DUP_LOG_TIMEOUT
          *  seconds (30 seconds), so we have to save the last one!
          */
         sge_gdi_communication_error.com_last_error = sge_gdi_communication_error.com_error;

         switch (commlib_error->cl_err_type) {
            case CL_LOG_ERROR: {
               if (commlib_error->cl_info != NULL) {
                  ERROR((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_SS,
                         cl_get_error_text(commlib_error->cl_error),
                         commlib_error->cl_info));
               } else {
                  ERROR((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_S,
                         cl_get_error_text(commlib_error->cl_error)));
               }
               break;
            }
            case CL_LOG_WARNING: {
               if (commlib_error->cl_info != NULL) {
                  WARNING((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_SS,
                           cl_get_error_text(commlib_error->cl_error),
                           commlib_error->cl_info));
               } else {
                  WARNING((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_S,
                           cl_get_error_text(commlib_error->cl_error)));
               }
               break;
            }
            case CL_LOG_INFO: {
               if (commlib_error->cl_info != NULL) {
                  INFO((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_SS,
                        cl_get_error_text(commlib_error->cl_error),
                        commlib_error->cl_info));
               } else {
                  INFO((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_S,
                        cl_get_error_text(commlib_error->cl_error)));
               }
               break;
            }
            case CL_LOG_DEBUG: {
               if (commlib_error->cl_info != NULL) {
                  DEBUG((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_SS,
                         cl_get_error_text(commlib_error->cl_error),
                         commlib_error->cl_info));
               } else {
                  DEBUG((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_S,
                         cl_get_error_text(commlib_error->cl_error)));
               }
               break;
            }
            case CL_LOG_OFF: {
               break;
            }
         }
      }
      sge_mutex_unlock("general_communication_error_mutex", 
                       SGE_FUNC, __LINE__, &general_communication_error_mutex);  
   }
   DEXIT;
}


/****** sge_any_request/sge_get_com_error_flag() *******************************
*  NAME
*     sge_get_com_error_flag() -- return gdi error flag state
*
*  SYNOPSIS
*     bool sge_get_com_error_flag(sge_gdi_stored_com_error_t error_type) 
*
*  FUNCTION
*     This function returns the error flag for the specified error type
*
*  INPUTS
*     sge_gdi_stored_com_error_t error_type - error type value
*
*  RESULT
*     bool - true: error has occured, false: error never occured
*
*  NOTES
*     MT-NOTE: sge_get_com_error_flag() is MT safe 
*
*  SEE ALSO
*     sge_any_request/general_communication_error()
*******************************************************************************/
bool sge_get_com_error_flag(u_long32 progid, sge_gdi_stored_com_error_t error_type) {
   bool ret_val = false;
   DENTER(TOP_LAYER, "sge_get_com_error_flag");
   sge_mutex_lock("general_communication_error_mutex", 
                  SGE_FUNC, __LINE__, &general_communication_error_mutex);  

   /* 
    * never add a default case for that switch, because of compiler warnings
    * for un-"cased" values 
    */

   /* TODO: remove uti_state_get_mewho() cases for QMASTER and EXECD after
            BT: 6350264, IZ: 1893 is fixed */
   switch (error_type) {
      case SGE_COM_ACCESS_DENIED: {
         ret_val = sge_gdi_communication_error.com_access_denied;
         break;
      }
      case SGE_COM_ENDPOINT_NOT_UNIQUE: {
         if ( progid == QMASTER || progid == EXECD ) {
            ret_val = false;
         } else { 
            ret_val = sge_gdi_communication_error.com_endpoint_not_unique;
         }
         break;
      }
      case SGE_COM_WAS_COMMUNICATION_ERROR: {
         ret_val = sge_gdi_communication_error.com_was_error;
         sge_gdi_communication_error.com_was_error = false;  /* reset error flag */
      }
   }
   sge_mutex_unlock("general_communication_error_mutex",
                    SGE_FUNC, __LINE__, &general_communication_error_mutex);  

   DRETURN(ret_val);
}

