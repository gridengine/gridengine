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
#include <stdlib.h>
#include <errno.h>

#include "sge_all_listsL.h"
#include "cull.h"
#include "sge.h"
#include "sge_order.h"
#include "sge_follow.h"
#include "sge_gdi_request.h"
#include "sge_c_gdi.h"
#include "sge_host.h"
#include "sge_host_qmaster.h"
#include "sge_job_qmaster.h"
#include "sge_userset_qmaster.h"
#include "sge_calendar_qmaster.h"
#include "sge_manop_qmaster.h"
#include "sge_centry_qmaster.h"
#include "sge_cqueue_qmaster.h"
#include "sge_pe_qmaster.h"
#include "sge_conf.h"
#include "configuration_qmaster.h"
#include "sge_event_master.h"
#include "sched_conf_qmaster.h"
#include "sge_userprj_qmaster.h"
#include "sge_ckpt_qmaster.h"
#include "sge_hgroup_qmaster.h"
#include "sge_sharetree_qmaster.h"
#include "sge_cuser_qmaster.h"
#include "sge_feature.h"
#include "sge_qmod_qmaster.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_qmaster_threads.h"
#include "sge_time.h"  
#include "version.h"  
#include "sge_security.h"  
#include "sge_answer.h"
#include "sge_pe.h"
#include "sge_ckpt.h"
#include "sge_qinstance.h"
#include "sge_userprj.h"
#include "sge_job.h"
#include "sge_userset.h"
#include "sge_manop.h"
#include "sge_calendar.h"
#include "sge_sharetree.h"
#include "sge_hgroup.h"
#include "sge_cuser.h"
#include "sge_centry.h"
#include "sge_cqueue.h"
#include "sge_lock.h"
#include "msg_common.h"
#include "msg_qmaster.h"

static void sge_c_gdi_get(gdi_object_t *ao, char *host, sge_gdi_request *request, 
                          sge_gdi_request *answer, sge_pack_buffer *pb, monitoring_t *monitor);

static void sge_c_gdi_add(gdi_object_t *ao, char *host, sge_gdi_request *request, 
                          sge_gdi_request *answer, int return_list_flag, uid_t uid, 
                          gid_t gid, char *user, char *group, monitoring_t *monitor);

static void sge_c_gdi_del(char *host, sge_gdi_request *request, sge_gdi_request *answer, 
                          int sub_command, uid_t uid, gid_t gid, char *user, char *group, 
                          monitoring_t *monitor);

static void sge_c_gdi_mod(gdi_object_t *ao, char *host, sge_gdi_request *request, 
                          sge_gdi_request *answer, int sub_command, monitoring_t *monitor);

static void sge_c_gdi_copy(gdi_object_t *ao, char *host, sge_gdi_request *request, 
                           sge_gdi_request *answer, int sub_command, uid_t uid, 
                           gid_t gid, char *user, char *group, monitoring_t *monitor);

static void sge_c_gdi_permcheck(char *host, sge_gdi_request *request, sge_gdi_request *answer, 
                                monitoring_t *monitor);

static void sge_gdi_do_permcheck(char *host, sge_gdi_request *request, sge_gdi_request *answer);

static void sge_c_gdi_trigger(char *host, sge_gdi_request *request, sge_gdi_request *answer, 
                              monitoring_t *monitor, object_description *object_base);

static void sge_gdi_shutdown_event_client(const char*, sge_gdi_request*, sge_gdi_request*, 
                                          monitoring_t *monitor, object_description *object_base);

static int  get_client_id(lListElem*, int*);

static void trigger_scheduler_monitoring(char*, sge_gdi_request*, sge_gdi_request*, monitoring_t*); 

static int sge_chck_mod_perm_user(lList **alpp, u_long32 target, char *user, monitoring_t *monitor);
static int sge_chck_mod_perm_host(lList **alpp, u_long32 target, char *host, 
                                  char *commproc, int mod, lListElem *ep, 
                                  bool is_locked, monitoring_t *monitor,
                                  object_description *object_base);
static int sge_chck_get_perm_host(lList **alpp, sge_gdi_request *request, monitoring_t *monitor,
                                  object_description *object_base);


static int schedd_mod(lList **alpp, lListElem *modp, lListElem *ep, int add, 
                      const char *ruser, const char *rhost, gdi_object_t *object, 
                      int sub_command, monitoring_t *monitor );
#if 0
static int do_gdi_get_config_list(sge_gdi_request *aReq, sge_gdi_request *aRes, int *aBeforeCnt, int *anAfterCnt);

static int do_gdi_get_sc_config_list(sge_gdi_request *aReq, sge_gdi_request *aRes, int *aBeforeCnt, int *anAfterCnt);
#endif

/* ------------------------------ generic gdi objects --------------------- */
/* *INDENT-OFF* */
static gdi_object_t gdi_object[] = {
   { SGE_CALENDAR_LIST,     CAL_name,  CAL_Type,  "calendar",                SGE_TYPE_CALENDAR,        calendar_mod, calendar_spool, calendar_update_queue_states },
   { SGE_EVENT_LIST,        0,         NULL,      "event",                   SGE_TYPE_NONE,            NULL,         NULL,           NULL },
   { SGE_ADMINHOST_LIST,    AH_name,   AH_Type,   "adminhost",               SGE_TYPE_ADMINHOST,       host_mod,     host_spool,     host_success },
   { SGE_SUBMITHOST_LIST,   SH_name,   SH_Type,   "submithost",              SGE_TYPE_SUBMITHOST,      host_mod,     host_spool,     host_success },
   { SGE_EXECHOST_LIST,     EH_name,   EH_Type,   "exechost",                SGE_TYPE_EXECHOST,        host_mod,     host_spool,     host_success },
   { SGE_CQUEUE_LIST,       CQ_name,   CQ_Type,   "cluster queue",           SGE_TYPE_CQUEUE,          cqueue_mod,   cqueue_spool,   cqueue_success },
   { SGE_JOB_LIST,          0,         NULL,      "job",                     SGE_TYPE_JOB,             NULL,         NULL,           NULL },
   { SGE_CENTRY_LIST,       CE_name,   CE_Type,   "complex entry",           SGE_TYPE_CENTRY,          centry_mod,   centry_spool,   centry_success },
   { SGE_ORDER_LIST,        0,         NULL,      "order",                   SGE_TYPE_NONE,            NULL,         NULL,           NULL },
   { SGE_MASTER_EVENT,      0,         NULL,      "master event",            SGE_TYPE_NONE,            NULL,         NULL,           NULL },
   { SGE_MANAGER_LIST,      0,         NULL,      "manager",                 SGE_TYPE_MANAGER,         NULL,         NULL,           NULL },
   { SGE_OPERATOR_LIST,     0,         NULL,      "operator",                SGE_TYPE_OPERATOR,        NULL,         NULL,           NULL },
   { SGE_PE_LIST,           PE_name,   PE_Type,   "parallel environment",    SGE_TYPE_PE,              pe_mod,       pe_spool,       pe_success },
   { SGE_CONFIG_LIST,       0,         NULL,      "configuration",           SGE_TYPE_NONE,            NULL,         NULL,           NULL },
   { SGE_SC_LIST,           0,         NULL,      "scheduler configuration", SGE_TYPE_NONE,            schedd_mod,   NULL,           NULL },
   { SGE_USER_LIST,         UP_name,   UP_Type,   "user",                    SGE_TYPE_USER,            userprj_mod,  userprj_spool,  userprj_success },
   { SGE_USERSET_LIST,      0,         NULL,      "userset",                 SGE_TYPE_USERSET,         NULL,         NULL,           NULL },
   { SGE_PROJECT_LIST,      UP_name,   UP_Type,   "project",                 SGE_TYPE_PROJECT,         userprj_mod,  userprj_spool,  userprj_success },
   { SGE_SHARETREE_LIST,    0,         NULL,      "sharetree",               SGE_TYPE_SHARETREE,       NULL,         NULL,           NULL },
   { SGE_CKPT_LIST,         CK_name,   CK_Type,   "checkpoint interface",    SGE_TYPE_CKPT,            ckpt_mod,     ckpt_spool,     ckpt_success },
   { SGE_JOB_SCHEDD_INFO,   0,         NULL,      "schedd info",             SGE_TYPE_JOB_SCHEDD_INFO, NULL,         NULL,           NULL },
   { SGE_ZOMBIE_LIST,       0,         NULL,      "job zombie list",         SGE_TYPE_ZOMBIE,          NULL,         NULL,           NULL },
#ifndef __SGE_NO_USERMAPPING__
   { SGE_USER_MAPPING_LIST, CU_name,   CU_Type,   "user mapping entry",      SGE_TYPE_CUSER,           cuser_mod,    cuser_spool,    cuser_success },
#endif
   { SGE_HGROUP_LIST,       HGRP_name, HGRP_Type, "host group",              SGE_TYPE_HGROUP,          hgroup_mod,   hgroup_spool,   hgroup_success },
   { SGE_DUMMY_LIST,        0,         NULL,      "general request",         SGE_TYPE_NONE,            NULL,         NULL,           NULL },
   { 0,                     0,         NULL,      NULL,                      SGE_TYPE_NONE,            NULL,         NULL,           NULL }
};
/* *INDENT-ON* */

void sge_clean_lists(void) 
{
   int i = 0;

   for(;gdi_object[i].target != 0 ; i++) {
      if (gdi_object[i].list_type != SGE_TYPE_NONE) {
         lList **master_list = object_type_get_master_list(gdi_object[i].list_type); 
   /*          fprintf(stderr, "---> freeing list %s, it has %d elems\n", gdi_object[i].object_name, lGetNumberOfElem(*master_list)); */
         lFreeList(master_list);
      }   
   }
   
}

/*
 * MT-NOTE: verify_request_version() is MT safe
 */
int verify_request_version(
lList **alpp,
u_long32 version,
char *host,
char *commproc,
int id 
) {
   char *client_version = NULL;
   dstring ds;
   char buffer[256];

   /* actual GDI version is defined in
    *   libs/gdi/sge_gdiP.h
    */   
   const struct vdict_t {
      u_long32 version;
      char *release;
   } *vp,vdict[] = {
      { 0x10000000, "5.0"  },
      { 0x10000001, "5.1"  },
      { 0x10000002, "5.2"  },
      { 0x10000003, "5.2.3"  },
      { 0x100000F0, "5.3alpha1" },
      { 0x100000F1, "5.3beta1 without hashing" },
      { 0x100000F2, "5.3beta1" },
      { 0x100000F3, "5.3beta2" },
      { 0x100000F4, "5.3" },
      { 0x10000FFF, "6.0"   },
      { 0x10001000, "6.0u3" },
      { 0x10001001, "6.0u4" }, /* not used for 6.0u4 version check which must be < 6.0u4 */
      { 0,          NULL   }
   };

   DENTER(TOP_LAYER, "verify_request_version");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if (version == GRM_GDI_VERSION) {
      DEXIT;
      return 0;
   }

   for (vp = &vdict[0]; vp->version; vp++) {
      if (version == vp->version) {
         client_version = vp->release;
      }
   }

   if (client_version) {
      WARNING((SGE_EVENT, MSG_GDI_WRONG_GDI_SSISS,
         host, commproc, id, client_version, feature_get_product_name(FS_VERSION, &ds)));
   }
   else {
      WARNING((SGE_EVENT, MSG_GDI_WRONG_GDI_SSIUS,
         host, commproc, id, sge_u32c(version), feature_get_product_name(FS_VERSION, &ds)));
   }
   answer_list_add(alpp, SGE_EVENT, STATUS_EVERSION, ANSWER_QUALITY_ERROR);

   DEXIT;
   return 1;
}

/* ------------------------------------------------------------ */

void 
sge_c_gdi(char *host, sge_gdi_request *request, sge_gdi_request *response,
          sge_pack_buffer *pb, monitoring_t *monitor) 
{
   const char *target_name = NULL;
   char *operation_name = NULL;
   int sub_command = 0;
   gdi_object_t *ao;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];
   lList *local_answer_list = NULL;
   object_description *object_base = object_type_get_object_description();

   DENTER(TOP_LAYER, "sge_c_gdi");

   response->op = request->op;
   response->target = request->target;
   response->sequence_id = request->sequence_id;
   response->request_id = request->request_id;

   if (verify_request_version(&(response->alp), request->version, request->host, 
                              request->commproc, request->id)) {
      DEXIT;
      return;
   }

   if (sge_get_auth_info(request, &uid, user, sizeof(user), &gid, group, sizeof(group)) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(response->alp), SGE_EVENT, STATUS_ENOMGR, 
                      ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (!user || !group) {
      CRITICAL((SGE_EVENT, MSG_GDI_NULL_IN_GDI_SSS,  
               (!user)?MSG_OBJ_USER:"", 
               (!group)?MSG_OBJ_GROUP:"", host));
      answer_list_add(&(response->alp), SGE_EVENT, 
                      STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (!sge_security_verify_user(request->host, request->commproc, 
                                 request->id, user)) {
      CRITICAL((SGE_EVENT, MSG_SEC_CRED_SSSI, user, request->host, 
                request->commproc, request->id));
      answer_list_add(&(response->alp), SGE_EVENT, 
                      STATUS_ENOSUCHUSER, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if ((ao = get_gdi_object(request->target))) {
     target_name = ao->object_name;
   }

   if (!ao || !target_name) {
      target_name = MSG_UNKNOWN_OBJECT;
   }
   /*
   ** we take request->op % SGE_GDI_RETURN_NEW_VERSION to get the
   ** real operation and request->op / SGE_GDI_RETURN_NEW_VERSION 
   ** to get the changed list back in the answer sge_gdi_request
   ** struct for add/modify operations
   ** If request->op / SGE_GDI_RETURN_NEW_VERSION is 1 we create
   ** a list response->lp this list is handed over to the corresponding
   ** add/modify routine.
   ** Now only for job add available.
   */
#if 0
   all_users_flag = request->op / SGE_GDI_ALL_USERS;
   request->op %= SGE_GDI_ALL_USERS;

   all_jobs_flag = request->op / SGE_GDI_ALL_JOBS;
   request->op %= SGE_GDI_ALL_JOBS;

   
   request->op %= SGE_GDI_RETURN_NEW_VERSION;
#endif

   sub_command = SGE_GDI_GET_SUBCOMMAND(request->op);
   request->op = SGE_GDI_GET_OPERATION(request->op);

   switch (request->op) {
   case SGE_GDI_GET:
      operation_name = "GET";
      MONITOR_GDI_GET(monitor);
      break;
   case SGE_GDI_ADD:
      operation_name = "ADD";
      MONITOR_GDI_ADD(monitor);
      break;
   case SGE_GDI_DEL:
      operation_name = "DEL";
      MONITOR_GDI_DEL(monitor);
      break;
   case SGE_GDI_MOD:
      operation_name = "MOD";
      MONITOR_GDI_MOD(monitor);
      break;
   case SGE_GDI_COPY:
      operation_name = "COPY";
      MONITOR_GDI_CP(monitor);
      break;
   case SGE_GDI_TRIGGER:
      operation_name = "TRIGGER";
      MONITOR_GDI_TRIG(monitor);
      break;
   case SGE_GDI_PERMCHECK:
      operation_name = "PERMCHECK";
      MONITOR_GDI_PERM(monitor);
      break; 
   default:
      operation_name = "???";
      break;
   }

   /* different report types */
   switch (request->op) {
   case SGE_GDI_GET:
      break;
   case SGE_GDI_ADD:
   case SGE_GDI_DEL:
   case SGE_GDI_MOD:
   case SGE_GDI_COPY:
   case SGE_GDI_TRIGGER:
   default:
   	DPRINTF(("GDI %s %s (%s/%s/%d) (%s/%d/%s/%d)\n",
			      operation_name, target_name, 
			      request->host, request->commproc, (int)request->id,
			      user, (int)uid, group, (int)gid));
     break;
   }
   switch (request->op) {
   case SGE_GDI_GET:
      sge_c_gdi_get(ao, host, request, response, pb, monitor);
      break;

   case SGE_GDI_ADD:
      sge_c_gdi_add(ao, host, request, response, sub_command, uid, gid, user, group, monitor);
      break;

   case SGE_GDI_DEL:
      sge_c_gdi_del(host, request, response, sub_command, uid, gid, user, group, monitor);
      break;

   case SGE_GDI_MOD:
      sge_c_gdi_mod(ao, host, request, response, sub_command, monitor);
      break;

   case SGE_GDI_COPY:
      sge_c_gdi_copy(ao, host, request, response, sub_command, uid, gid, user, group, monitor);
      break;

   case SGE_GDI_TRIGGER:
      sge_c_gdi_trigger(host, request, response, monitor, object_base);
      break;

   case SGE_GDI_PERMCHECK:
      sge_c_gdi_permcheck(host, request, response, monitor);
      break;

   default:
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SGETEXT_UNKNOWNOP));
      answer_list_add(&(response->alp), SGE_EVENT, STATUS_ENOIMP, 
                      ANSWER_QUALITY_ERROR);
      break;
   }

   /* GDI_GET fills the pack-buffer by itself */
   if (request->op != SGE_GDI_GET) {
      gdi_request_pack_result(response, &local_answer_list, pb);
   }
   /* different report types */
   switch (request->op) {

   case SGE_GDI_GET:
   	DPRINTF(("GDI %s %s (%s/%s/%d) (%s/%d/%s/%d)\n",
	      		operation_name, target_name, 
			      request->host, request->commproc, (int)request->id,
			      user, (int)uid, group, (int)gid));
       break;

   case SGE_GDI_ADD:
   case SGE_GDI_DEL:
   case SGE_GDI_MOD:
   case SGE_GDI_COPY:
   case SGE_GDI_TRIGGER:
   default:
     break;
   }

   DEXIT;
   return;
}


/*
 * MT-NOTE: sge_c_gdi_get() is MT safe
 */
static void 
sge_c_gdi_get(gdi_object_t *ao, char *host, sge_gdi_request *request, 
              sge_gdi_request *answer, sge_pack_buffer *pb, monitoring_t *monitor) 
{
   lList *local_answer_list = NULL;
#define USE_OLD_IMPL 0
#if !USE_OLD_IMPL
   bool local_ret = true;
#endif
   lList *lp = NULL;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];
   dstring ds;
   char buffer[256];
   object_description *object_base = object_type_get_object_description();

   DENTER(TOP_LAYER, "sge_c_gdi_get");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if (sge_chck_get_perm_host(&(answer->alp), request, monitor, object_base)) {
      gdi_request_pack_result(answer, &local_answer_list, pb);
      DEXIT;
      return;
   }

   if (sge_get_auth_info(request, &uid, user, sizeof(user), &gid, group, sizeof(group)) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, 
                      ANSWER_QUALITY_ERROR);
      gdi_request_pack_result(answer, &local_answer_list, pb);
      DEXIT;
      return;
   }

   switch (request->target) {
#ifdef QHOST_TEST
      case SGE_QHOST:
         sprintf(SGE_EVENT, "SGE_QHOST\n");
         answer_list_add(&(answer->alp), SGE_EVENT, 
                         STATUS_OK, ANSWER_QUALITY_INFO);
         gdi_request_pack_result(answer, &local_answer_list, pb);
         DEXIT;
         return;
#endif
      case SGE_EVENT_LIST:
         answer->lp = sge_select_event_clients("qmaster_response", 
                                               request->cp, request->enp);
         sprintf(SGE_EVENT, MSG_GDI_OKNL);
         answer_list_add(&(answer->alp), SGE_EVENT, 
                         STATUS_OK, ANSWER_QUALITY_INFO);
         gdi_request_pack_result(answer, &local_answer_list, pb);
         DEXIT;
         return;
      case SGE_CONFIG_LIST: {

      /* TODO EB: move this into the master configuration, 
                                    and pack the list right away */
#if 0 /* EB: TODO PACKING */
         do_gdi_get_config_list(request, answer, before, after);
#else
         lList *conf = NULL;

         conf = sge_get_configuration();

         answer->lp = lSelectHashPack("qmaster_response", conf, request->cp,
                                      request->enp, false, NULL);

         sprintf(SGE_EVENT, MSG_GDI_OKNL);
         answer_list_add(&(answer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
         lFreeList(&conf);
      }
      gdi_request_pack_result(answer, &local_answer_list, pb);

#endif
         DEXIT;
         return;
      case SGE_SC_LIST: /* TODO EB: move this into the scheduler configuration, 
                                    and pack the list right away */
      {
         lList *conf = NULL;

         conf = sconf_get_config_list();

         answer->lp = lSelectHashPack("qmaster_response", conf, request->cp, 
                                      request->enp, false, NULL);
         sprintf(SGE_EVENT, MSG_GDI_OKNL);
         answer_list_add(&(answer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
         lFreeList(&conf);
      }
      gdi_request_pack_result(answer, &local_answer_list, pb);
         DEXIT;
         return;
      default:
/* EB: TODO PACKING */
         MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_READ), monitor);

         /* 
          * Issue 1365
          * If the scheduler is not available the information in the job info
          * messages are outdated. In this case we have to reject the request.
          */
         if (request->target == SGE_JOB_SCHEDD_INFO &&
             !sge_has_event_client(EV_ID_SCHEDD) ) {
            answer_list_add(&(answer->alp),MSG_SGETEXT_JOBINFOMESSAGESOUTDATED,
                            STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
         } else if (ao == NULL || ao->list_type == SGE_TYPE_NONE) {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
            answer_list_add(&(answer->alp), SGE_EVENT, 
                            STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
         } else {
            lp = *object_type_get_master_list(ao->list_type);
#if !USE_OLD_IMPL
            /*
             * start with first part of packing
             */
#if 0
fprintf(stderr, "### before gdi_request_pack_prefix {\n");
pb_print_to(pb, false, stderr);
fprintf(stderr, "\n");
#endif

            local_ret &= gdi_request_pack_prefix(answer, 
                                                 &local_answer_list, pb);

#if 0
fprintf(stderr, "### after gdi_request_pack_prefix {\n");
pb_print_to(pb, false, stderr);
fprintf(stderr, "\n");
#endif

#endif

#if !USE_OLD_IMPL
            lSelectHashPack("qmaster_response", lp, request->cp, 
                            request->enp, false, pb);

#if 0
            {
               sge_pack_buffer pb2;
               lList *lpr = NULL;

               init_packbuffer(&pb2, 0, 0);
               lpr = lSelectHashPack("qmaster_response", lp, request->cp,
                               request->enp, false, NULL);
               cull_pack_list(&pb2, lpr);
               lFreeList(lpr);

               fprintf(stderr, "************* lSelectHashPack with pb\n");
               pb_print_to(pb, false, stderr);
               fprintf(stderr, "************* lSelectHashPack without pb\n");
               pb_print_to(&pb2, false, stderr);

               clear_packbuffer(&pb2);
            }
#endif
#else
            answer->lp = lSelectHashPack("qmaster_response", lp, request->cp,
                                         request->enp, false, NULL);
#endif

            sprintf(SGE_EVENT, MSG_GDI_OKNL);
            answer_list_add(&(answer->alp), SGE_EVENT, 
                            STATUS_OK, ANSWER_QUALITY_INFO);

#if !USE_OLD_IMPL
            /*
             * finish packing
             */
            local_ret &= gdi_request_pack_suffix(answer, 
                                                 &local_answer_list, pb);
#else

            gdi_request_pack_result(answer, &local_answer_list, pb);
#endif
#if 0
B
            fprintf(stderr, "*** pb\n");
            pb_print_to(pb, false, stderr);
#endif
            lFreeList(&local_answer_list);

         }

         SGE_UNLOCK(LOCK_GLOBAL, LOCK_READ);
   }
   DEXIT;
   return;
}


#if 0  /* EB: TODO PACKING */

/*
 * Implement 'SGE_GDI_GET' for request target 'SGE_CONFIG_LIST'.
 *
 * MT-NOTE: do_gdi_get_config() is MT safe
 */
static int do_gdi_get_config_list(sge_gdi_request *aReq, sge_gdi_request *aRes, int *aBeforeCnt, int *anAfterCnt)
{
   lList *conf = NULL;

   DENTER(TOP_LAYER, "do_gdi_get_config_list");

   conf = sge_get_configuration();

   *aBeforeCnt = lGetNumberOfElem(conf);

   aRes->lp = lSelectHashPack("qmaster_response", conf, aReq->cp, 
                              aReq->enp, false, NULL);

   conf = lFreeList(conf);

   *anAfterCnt = lGetNumberOfElem(aRes->lp);

   sprintf(SGE_EVENT, MSG_GDI_OKNL);

   answer_list_add(&(aRes->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);

   DEXIT;
   return 0;
}

#endif

#if 0

/*
 * Implement 'SGE_GDI_GET' for request target 'SGE_CONFIG_LIST'.
 *
 * MT-NOTE: do_gdi_get_config() is MT safe
 */ 
static int do_gdi_get_config_list(sge_gdi_request *aReq, sge_gdi_request *aRes, int *aBeforeCnt, int *anAfterCnt)
{
   lList *conf = NULL;

   DENTER(TOP_LAYER, "do_gdi_get_config_list");
   
   conf = sge_get_configuration();

   *aBeforeCnt = lGetNumberOfElem(conf);

   aRes->lp = lSelectHashPack("qmaster_response", conf, aReq->cp, 
                              aReq->enp, false, NULL);

   lFreeList(&conf);

   *anAfterCnt = lGetNumberOfElem(aRes->lp);

   sprintf(SGE_EVENT, MSG_GDI_OKNL);

   answer_list_add(&(aRes->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);

   DEXIT;
   return 0;
}

static int do_gdi_get_sc_config_list(sge_gdi_request *aReq, sge_gdi_request *aRes, 
                                     int *aBeforeCnt, int *anAfterCnt)
{
   lList *conf = NULL;

   DENTER(TOP_LAYER, "do_gdi_get_sc_config_list");
   
   conf = sconf_get_config_list();

   *aBeforeCnt = lGetNumberOfElem(conf);
   aRes->lp = lSelectHashPack("qmaster_response", conf, aReq->cp, aReq->enp, false, NULL);
   *anAfterCnt = lGetNumberOfElem(aRes->lp);

   sprintf(SGE_EVENT, MSG_GDI_OKNL);
   answer_list_add(&(aRes->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);

   lFreeList(&conf);
   
   DEXIT;
   return 0;
}

#endif

/*
 * MT-NOTE: sge_c_gdi_add() is MT safe
 */
static void 
sge_c_gdi_add(gdi_object_t *ao, char *host, sge_gdi_request *request, 
              sge_gdi_request *answer, int sub_command, 
              uid_t uid, gid_t gid, char *user, char *group, 
              monitoring_t *monitor) 
{
   lListElem *ep;
   lList *ticket_orders = NULL;
   dstring ds;
   char buffer[256];
   object_description *object_base = object_type_get_object_description();

   DENTER(TOP_LAYER, "sge_c_gdi_add");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if (!request->host || !user || !request->commproc) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   /* check permissions of host and user */
   if ((!sge_chck_mod_perm_user(&(answer->alp), request->target, user, monitor)) &&
       (!sge_chck_mod_perm_host(&(answer->alp), request->target, request->host, 
                                request->commproc, 0, NULL, false, monitor, object_base))) {

      if (request->target == SGE_EVENT_LIST) {
         for_each (ep, request->lp) {/* is thread save. the global lock is used, when needed */
                                     /* fill address infos from request into event client that must be added */
            lSetHost(ep, EV_host, request->host);
            lSetString(ep, EV_commproc, request->commproc);
            lSetUlong(ep, EV_commid, request->id);

            /* fill in authentication infos from request */
            lSetUlong(ep, EV_uid, uid);

            mconf_set_max_dynamic_event_clients(sge_set_max_dynamic_event_clients(mconf_get_max_dynamic_event_clients()));
            
            sge_add_event_client(ep,&(answer->alp), (sub_command & SGE_GDI_RETURN_NEW_VERSION) ? &(answer->lp) : NULL, user, host, monitor);
         }
      } else if (request->target == SGE_JOB_LIST) {
         for_each(ep, request->lp) { /* is thread save. the global lock is used, when needed */
                                                   /* fill address infos from request into event client that must be added */
            if (!job_verify_submitted_job(ep, &(answer->alp))) {
               ERROR((SGE_EVENT, MSG_QMASTER_INVALIDJOBSUBMISSION_SSS,
                      user, request->commproc, request->host));
            } else {
               if(mconf_get_simulate_hosts()) {

                  int multi_job = 1;
                  int i;
                  lList *context = lGetList(ep, JB_context);
                  if(context != NULL) {
                     lListElem *multi = lGetElemStr(context, VA_variable, "SGE_MULTI_SUBMIT");
                     if(multi != NULL) {
                        multi_job = atoi(lGetString(multi, VA_value));
                        DPRINTF(("Cloning job %d times in simulation mode\n", multi_job));
                     }
                  }
                  
                  for(i = 0; i < multi_job; i++) {
                     lListElem *clone = lCopyElem(ep);
                     sge_gdi_add_job(clone, &(answer->alp), 
                                     (sub_command & SGE_GDI_RETURN_NEW_VERSION) ? 
                                     &(answer->lp) : NULL, 
                                     user, host, uid, gid, group, request, monitor);
                        lFreeElem(&clone);
                  }
                  
               } else {
                  /* submit needs to know user and group */
                  sge_gdi_add_job(ep, &(answer->alp), 
                                  (sub_command & SGE_GDI_RETURN_NEW_VERSION) ? 
                                  &(answer->lp) : NULL, 
                                  user, host, uid, gid, group, request, monitor);
               }
            }
         }
      } else if (request->target == SGE_SC_LIST ) {
         for_each (ep, request->lp) {
            sge_mod_sched_configuration(ep, &(answer->alp), user, host);
         }
      } 
      else {
         bool is_scheduler_resync = false;
         lList *ppList = NULL;

         MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE), monitor); 

         if (request->target == SGE_ORDER_LIST) {
             sge_set_commit_required(); 
         }

         for_each (ep, request->lp) {

            /* add each element */
            switch (request->target) {

               case SGE_ORDER_LIST:
                 switch (sge_follow_order(ep, &(answer->alp), user, host, &ticket_orders, monitor, object_base)) {
                    case STATUS_OK :
                    case  0 : /* everything went fine */
                       break;
                    case -2 : is_scheduler_resync = true;
                    case -1 :
                    case -3 :
                              /* stop the order processing */
                              DPRINTF(("Failed to follow order . Remaining %d orders unprocessed.\n", 
                                        lGetNumberOfRemainingElem(ep)));
                              ep = lLast(request->lp); 
                       break;
                       
                    default :  DPRINTF(("--> FAILED: unexpected state from in the order processing <--\n"));
                       break;        
                  }
                  break;
               
               case SGE_MANAGER_LIST:
               case SGE_OPERATOR_LIST:
                  sge_add_manop(ep, &(answer->alp), user, host, request->target);
                  break;

               case SGE_USERSET_LIST:
                  sge_add_userset(ep, &(answer->alp), object_base[SGE_TYPE_USERSET].list, user, host);
                  break;

               case SGE_SHARETREE_LIST:
                  sge_add_sharetree(ep, object_base[SGE_TYPE_SHARETREE].list, &(answer->alp), user, host);
                  break;

               default:
                  if (!ao) {
                     SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
                     answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
                     break;
                  } 
                  
                  if (request->target==SGE_EXECHOST_LIST && !strcmp(prognames[EXECD], request->commproc)) {
                     sge_execd_startedup(ep, &(answer->alp), user, host, request->target, monitor);
                  } 
                  else {
                     sge_gdi_add_mod_generic(&(answer->alp), ep, 1, ao, user, host, sub_command, &ppList, monitor);
                  }
                  break;
            }
         } /* for_each request */
         if (request->target == SGE_ORDER_LIST) {
            sge_commit();
            sge_set_next_spooling_time();
            answer_list_add(&(answer->alp), "OK\n", STATUS_OK, ANSWER_QUALITY_INFO);
         }

         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);

         if (is_scheduler_resync) {
             sge_resync_schedd(monitor); /* ask for a total update */
         }

         /* we could do postprocessing based on ppList here */
         lFreeList(&ppList);
      }
   }
   
   if (ticket_orders != NULL) {
      if (sge_conf_is_reprioritize()) {

         MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE), monitor);
         distribute_ticket_orders(ticket_orders, monitor, object_base);
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);

      }
      else {
         /* tickets not needed at execd's if no repriorization is done */
         DPRINTF(("NO TICKET DELIVERY\n"));
      } 

      lFreeList(&ticket_orders);
   }

   DEXIT;
   return;
}

/*
 * MT-NOTE: sge_c_gdi-del() is MT safe
 */
static void sge_c_gdi_del(char *host, sge_gdi_request *request, sge_gdi_request *answer, 
                          int sub_command, uid_t uid, gid_t gid, char *user, char *group, 
                          monitoring_t *monitor)
{
   lListElem *ep;
   dstring ds;
   char buffer[256];
   object_description *object_base = object_type_get_object_description();

   DENTER(GDI_LAYER, "sge_c_gdi_del");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if (!request->lp) /* delete whole list */
   { 
      if (sge_chck_mod_perm_user(&(answer->alp), request->target, user, monitor)) {
         DEXIT;
         return;
      }
      
      if (sge_chck_mod_perm_host(&(answer->alp), request->target, request->host, 
                                 request->commproc, 0, NULL, false, monitor, object_base)) {
         DEXIT;
         return;
      }
      
      switch (request->target)
      {
         case SGE_SHARETREE_LIST:
            MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE), monitor);
            sge_del_sharetree(object_base[SGE_TYPE_SHARETREE].list, &(answer->alp), user,host);
            SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
            break;
            
         default:
            SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
            answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
            break;
      }
   }
   else {

      if (sge_chck_mod_perm_user(&(answer->alp), request->target, user, monitor)) {
         DEXIT;
         return;
      }  

      if (sge_chck_mod_perm_host(&(answer->alp), request->target, request->host, 
                                 request->commproc, 0, NULL, false, monitor, object_base)) {
         DEXIT;
         return;
      }

      for_each (ep, request->lp) {

         MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE), monitor);

         /* try to remove the element */
         switch (request->target)
         {
            case SGE_ADMINHOST_LIST:
            case SGE_SUBMITHOST_LIST:
            case SGE_EXECHOST_LIST:
               sge_del_host(ep, &(answer->alp), user, host, request->target, *object_base[SGE_TYPE_HGROUP].list);
               break;

            case SGE_CQUEUE_LIST:
               cqueue_del(ep, &(answer->alp), user, host);
               break;

            case SGE_JOB_LIST:
               sge_set_commit_required();
               sge_gdi_del_job(ep, &(answer->alp), user, host, sub_command, monitor);
               sge_commit();
               break;

            case SGE_CENTRY_LIST:
               sge_del_centry(ep, &(answer->alp), user, host);
               break;

            case SGE_PE_LIST:
               sge_del_pe(ep, &(answer->alp), user, host); 
               break;

            case SGE_MANAGER_LIST:
            case SGE_OPERATOR_LIST:
               sge_del_manop(ep, &(answer->alp), user, host, request->target);
               break;

            case SGE_CONFIG_LIST:
               sge_del_configuration(ep, &(answer->alp), user, host);
               break;

            case SGE_USER_LIST:
               sge_del_userprj(ep, &(answer->alp), object_base[SGE_TYPE_USER].list, user, host, 1);
               break;

            case SGE_USERSET_LIST:
               sge_del_userset(ep, &(answer->alp), object_base[SGE_TYPE_USERSET].list, user, host);
               break;

            case SGE_PROJECT_LIST:
               sge_del_userprj(ep, &(answer->alp), object_base[SGE_TYPE_PROJECT].list, user, host, 0);
               break;

            case SGE_CKPT_LIST:
               sge_del_ckpt(ep, &(answer->alp), user, host); 
               break;

            case SGE_CALENDAR_LIST:
               sge_del_calendar(ep, &(answer->alp), user, host);
               break;
   #ifndef __SGE_NO_USERMAPPING__
            case SGE_USER_MAPPING_LIST:
               cuser_del(ep, &(answer->alp), user, host);
               break;
   #endif
            case SGE_HGROUP_LIST:
               hgroup_del(ep, &(answer->alp), user, host);
               break;
            default:
               SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
               answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
               break;
         } /* switch target */
           
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
      } /* for_each element */
   }

   DEXIT;
   return;
}

/* 
 * MT-NOTE: sge_c_gdi_copy() is MT safe
 */
static void sge_c_gdi_copy(gdi_object_t *ao, char *host, sge_gdi_request *request, 
                           sge_gdi_request *answer, int sub_command, uid_t uid, 
                           gid_t gid, char *user, char *group, monitoring_t *monitor)
{
   lListElem *ep = NULL;
   object_description *object_base = object_type_get_object_description();

   DENTER(TOP_LAYER, "sge_c_gdi_copy");

   if (!request->host || !user || !request->commproc)
   {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (sge_chck_mod_perm_user(&(answer->alp), request->target, user, monitor)) {
      DEXIT;
      return;
   }

   if (sge_chck_mod_perm_host(&(answer->alp), request->target, request->host, 
                              request->commproc, 0, NULL, false, monitor, object_base)) {
      DEXIT;
      return;
   }
   
   for_each (ep, request->lp) {
      switch (request->target)
      {
         case SGE_JOB_LIST:
            /* gdi_copy_job uses the global lock internal */
            sge_gdi_copy_job(ep, &(answer->alp), (sub_command & SGE_GDI_RETURN_NEW_VERSION) ? &(answer->lp) : NULL, 
                             user, host, uid, gid, group, request, monitor);
            break;

         default:
            SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
            answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
            break;
      }
   }

   DEXIT;
   return;
}

/* ------------------------------------------------------------ */

static void sge_gdi_do_permcheck(char *host, sge_gdi_request *request, sge_gdi_request *answer)
{ 
   lList *lp = NULL;
   lListElem *ep = NULL;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];

   DENTER(GDI_LAYER, "sge_gdi_do_permcheck");

   if (sge_get_auth_info(request, &uid, user, sizeof(user), &gid, group, sizeof(group)) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   DPRINTF(("User: %s\n", user ));
 
   if (answer->lp == NULL)
   { 
      const char *mapped_user = NULL;
      const char* requested_host = NULL;
      bool did_mapping = false;

      lUlong value;
      /* create PERM_Type list for answer structure*/
      lp = lCreateList("permissions", PERM_Type);
      ep = lCreateElem(PERM_Type);
      lAppendElem(lp,ep);

      /* set sge username */ 
      lSetString(ep, PERM_sge_username, user );

      /* set requested host name */
      if (request->lp == NULL) {
         requested_host = host;
      } else {
         lList*     tmp_lp = NULL;
         lListElem* tmp_ep = NULL;
     
         tmp_lp = request->lp;
         tmp_ep = tmp_lp->first;
         requested_host = lGetHost(tmp_ep, PERM_req_host);
#ifndef __SGE_NO_USERMAPPING__
         cuser_list_map_user(*(cuser_list_get_master_list()), NULL,
                             user, requested_host, &mapped_user); 
         did_mapping = true;
#endif
      }

      if (requested_host != NULL) {
         lSetHost(ep, PERM_req_host, requested_host);  
      }   

      if (did_mapping && strcmp(mapped_user, user)) {
         DPRINTF(("execution mapping: user %s mapped to %s on host %s\n",
                  user, mapped_user, requested_host));

         lSetString(ep, PERM_req_username, mapped_user);
      } else { 
         lSetString(ep, PERM_req_username, "");
      }
    

      /* check for manager permission */
      value = 0;
      if (manop_is_manager(user)) {
         value = 1; 
      }   
      lSetUlong(ep, PERM_manager, value);

      /* check for operator permission */
      value = 0;
      if (manop_is_operator(user)) {
         value = 1; 
      }   
      lSetUlong(ep, PERM_operator, value);
      if ((request->cp != NULL) && (request->enp != NULL)) {
         answer->lp = lSelect("permissions", lp, request->cp, request->enp);
         lFreeList(&lp); 
      } else {
         answer->lp = lp;
      }
   }

  sprintf(SGE_EVENT, MSG_GDI_OKNL);
  answer_list_add(&(answer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO); 
  DEXIT;
  return;
}

/*
 * MT-NOTE: sge_c_gdi_permcheck() is MT safe
 */
static void sge_c_gdi_permcheck(char *host, sge_gdi_request *request, sge_gdi_request *answer, 
                               monitoring_t *monitor)
{
  DENTER(GDI_LAYER, "sge_c_gdi_permcheck");

  MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE), monitor);
  
  switch (request->target)
  {
     case SGE_DUMMY_LIST:
       sge_gdi_do_permcheck(host, request, answer);
       break;
     default:
       WARNING((SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
       answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR); 
       break;
  }

  SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);

  DEXIT;
  return;
}

/*
 * MT-NOTE: sge_c_gdi_trigger() is MT safe
 */
static void sge_c_gdi_trigger(char *host, sge_gdi_request *request, sge_gdi_request *answer, 
                              monitoring_t *monitor, object_description *object_base)
{
   u_long32 target = request->target;
   
   DENTER(GDI_LAYER, "sge_c_gdi_trigger");

   switch (target) {

      case SGE_EXECHOST_LIST: /* kill execd */
      case SGE_MASTER_EVENT:  /* kill master */
      case SGE_SC_LIST:       /* trigger scheduler monitoring */

            MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE), monitor);
            
            if (!host_list_locate(*object_base[SGE_TYPE_ADMINHOST].list, host)) {
               ERROR((SGE_EVENT, MSG_SGETEXT_NOADMINHOST_S, host));
               answer_list_add(&(answer->alp), SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
               SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
               DEXIT;
               return;
            }
      
            if (SGE_EXECHOST_LIST == target) {
               sge_gdi_kill_exechost(host, request, answer);
            }
            
            SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
            
            if (SGE_SC_LIST == target) {
               trigger_scheduler_monitoring(host, request, answer, monitor);
            }
            else if (target == SGE_MASTER_EVENT) {
               /* shutdown qmaster. Do NOT hold the global lock, while doing this !! */
               sge_gdi_kill_master(host, request, answer);

            }
         break;

       case SGE_CQUEUE_LIST:
       case SGE_JOB_LIST:
            MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE), monitor);
            sge_gdi_qmod(host, request, answer, monitor);
            SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
         break; 

       case SGE_EVENT_LIST:
            /* kill scheduler or event client */
            sge_gdi_shutdown_event_client(host, request, answer, monitor, object_base);
            answer_list_log(&answer->alp, false);
         break;
            
       default:
            /* permissions should be checked in the functions. Here we don't
               know what is to do, so we don't know what permissions we need */
            WARNING((SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
            answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
         break;
   }

   DEXIT;
   return;
}

/****** qmaster/sge_c_gdi/sge_gdi_shutdown_event_client() **********************
*  NAME
*     sge_gdi_shutdown_event_client() -- shutdown event client 
*
*  SYNOPSIS
*     static void sge_gdi_shutdown_event_client(const char *aHost, 
*     sge_gdi_request *aRequest, sge_gdi_request *anAnswer) 
*
*  FUNCTION
*     Shutdown event clients by client id. 'aRequest' does contain a list of 
*     client id's. This is a list of 'ID_Type' elements.
*
*  INPUTS
*     const char *aHost         - sender 
*     sge_gdi_request *aRequest - request 
*     sge_gdi_request *anAnswer - answer
*     monitoring_t    *monitor  - the monitoring structure 
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: sge_gdi_shutdown_event_client() is NOT MT safe. 
*
*******************************************************************************/
static void sge_gdi_shutdown_event_client(const char *aHost,
                                          sge_gdi_request *aRequest,
                                          sge_gdi_request *anAnswer,
                                          monitoring_t *monitor,
                                          object_description *object_base)
{
   uid_t uid = 0;
   gid_t gid = 0;
   char user[128]  = { '\0' };
   char group[128] = { '\0' };
   lListElem *elem = NULL; /* ID_Type */

   DENTER(TOP_LAYER, "sge_gdi_shutdown_event_client");

   if (sge_get_auth_info(aRequest, &uid, user, sizeof(user), &gid, group, sizeof(group)) == -1)
   {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(anAnswer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   for_each (elem, aRequest->lp) {
      lList *local_alp = NULL;
      int client_id = EV_ID_ANY;
      int res = -1;


      if (get_client_id(elem, &client_id) != 0) {
         answer_list_add(&(anAnswer->alp), SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         continue;
      }

      if (client_id == EV_ID_SCHEDD && !host_list_locate(*object_base[SGE_TYPE_ADMINHOST].list, aHost)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_NOADMINHOST_S, aHost));
         answer_list_add(&(anAnswer->alp), SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
         continue;
      } else if (!host_list_locate(*object_base[SGE_TYPE_SUBMITHOST].list, aHost) 
              && !host_list_locate(*object_base[SGE_TYPE_ADMINHOST].list, aHost)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_NOSUBMITORADMINHOST_S, aHost));
         answer_list_add(&(anAnswer->alp), SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
         continue;
      }

      if (client_id == EV_ID_ANY) {
         res = sge_shutdown_dynamic_event_clients(user, &(local_alp), monitor);
      } else {
         res = sge_shutdown_event_client(client_id, user, uid, &(local_alp), monitor);
      }

      if ((res == EINVAL) && (client_id == EV_ID_SCHEDD)) {
         lFreeList(&local_alp); 
         answer_list_add(&(anAnswer->alp), MSG_COM_NOSCHEDDREGMASTER, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
      }
      else {
         answer_list_append_list(&(anAnswer->alp), &local_alp);
      }
   }

   DEXIT;
   return;
} /* sge_gdi_shutdown_event_client() */

/****** qmaster/sge_c_gdi/get_client_id() **************************************
*  NAME
*     get_client_id() -- get client id from ID_Type element. 
*
*  SYNOPSIS
*     static int get_client_id(lListElem *anElem, int *anID) 
*
*  FUNCTION
*     Get client id from ID_Type element. The client id is converted to an
*     integer and stored in 'anID'.
*
*  INPUTS
*     lListElem *anElem - ID_Type element 
*     int *anID         - will contain client id on return
*
*  RESULT
*     EINVAL - failed to extract client id. 
*     0      - otherwise
*
*  NOTES
*     MT-NOTE: get_client_id() is MT safe. 
*
*     Using 'errno' to check for 'strtol' error situations is recommended
*     by POSIX.
*
*******************************************************************************/
static int get_client_id(lListElem *anElem, int *anID)
{
   const char *id = NULL;

   DENTER(TOP_LAYER, "get_client_id");

   if ((id = lGetString(anElem, ID_str)) == NULL)
   {
      DEXIT;
      return EINVAL;
   }

   errno = 0; /* errno is thread local */

   *anID = strtol(id, NULL, 0);

   if (errno != 0)
   {
      ERROR((SGE_EVENT, MSG_GDI_EVENTCLIENTIDFORMAT_S, id));
      DEXIT;
      return EINVAL;
   }

   DEXIT;
   return 0;
} /* get_client_id() */

/****** qmaster/sge_c_gdi/trigger_scheduler_monitoring() ***********************
*  NAME
*     trigger_scheduler_monitoring() -- trigger scheduler monitoring 
*
*  SYNOPSIS
*     void trigger_scheduler_monitoring(char *aHost, sge_gdi_request *aRequest, 
*     sge_gdi_request *anAnswer) 
*
*  FUNCTION
*     Trigger scheduler monitoring.
*
*  INPUTS
*     char *aHost               - sender 
*     sge_gdi_request *aRequest - request 
*     sge_gdi_request *anAnswer - response 
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: trigger_scheduler_monitoring() is MT safe, using global lock 
*
*  SEE ALSO
*     qconf -tsm
*
*******************************************************************************/
static void 
trigger_scheduler_monitoring(char *aHost, sge_gdi_request *aRequest, sge_gdi_request *anAnswer,
                             monitoring_t *monitor) 
{
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];

   DENTER(GDI_LAYER, "trigger_scheduler_monitoring");

   if (sge_get_auth_info(aRequest, &uid, user, sizeof(user), &gid, group, sizeof(group)) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(anAnswer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_READ), monitor);
   if (!manop_is_manager(user)) {
      SGE_UNLOCK(LOCK_GLOBAL, LOCK_READ);
      WARNING((SGE_EVENT, MSG_COM_NOSCHEDMONPERMS));
      answer_list_add(&(anAnswer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_WARNING);
      DEXIT;
      return;
   }
   SGE_UNLOCK(LOCK_GLOBAL, LOCK_READ);
     
   if (!sge_add_event_for_client(EV_ID_SCHEDD, 0, sgeE_SCHEDDMONITOR, 0, 0, NULL, NULL, NULL, NULL)) {
      WARNING((SGE_EVENT, MSG_COM_NOSCHEDDREGMASTER));
      answer_list_add(&(anAnswer->alp), SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
      DEXIT;
      return;
   }

   INFO((SGE_EVENT, MSG_COM_SCHEDMON_SS, user, aHost));
   answer_list_add(&(anAnswer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);

   DEXIT;
   return;
} /* trigger_scheduler_monitoring() */

/*
 * MT-NOTE: sge_c_gdi_mod() is MT safe
 */
static void sge_c_gdi_mod(gdi_object_t *ao, char *host, sge_gdi_request *request, 
                          sge_gdi_request *answer, int sub_command, monitoring_t *monitor)
{
   lListElem *ep;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];
   dstring ds;
   char buffer[256];
   lList *ppList = NULL; /* for postprocessing, after the lists of requests has been processed */
   bool is_locked = false;
   object_description *object_base = object_type_get_object_description();
      
   DENTER(TOP_LAYER, "sge_c_gdi_mod");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if (sge_get_auth_info(request, &uid, user, sizeof(user), &gid, group, sizeof(group)) == -1)
   {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (sge_chck_mod_perm_user(&(answer->alp), request->target, user, monitor)) {
      DEXIT;
      return;
   }

   for_each (ep, request->lp)
   {
      if (sge_chck_mod_perm_host(&(answer->alp), request->target, request->host, 
                                 request->commproc, 1, ep, is_locked, monitor, object_base)) {
         continue;
      }

      if (request->target == SGE_CONFIG_LIST) {
         sge_mod_configuration(ep, &(answer->alp), user, host);      
      }
      else if (request->target == SGE_EVENT_LIST) {
         /* fill address infos from request into event client that must be added */
         lSetHost(ep, EV_host, request->host);
         lSetString(ep, EV_commproc, request->commproc);
         lSetUlong(ep, EV_commid, request->id);
 
         /* fill in authentication infos from request */
         lSetUlong(ep, EV_uid, uid);
 
         sge_mod_event_client(ep, &(answer->alp), user, host);      
      }
      else if (request->target == SGE_SC_LIST) {
         sge_mod_sched_configuration(ep, &(answer->alp), user, host);
      }
      else {
         if (!is_locked) {
            MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE), monitor);
            sge_set_commit_required();
            is_locked = true; 
         }
               
         switch (request->target)
         {
            case SGE_JOB_LIST:
               sge_gdi_mod_job(ep, &(answer->alp), user, host, sub_command);
               break;

            case SGE_USERSET_LIST:
               sge_mod_userset(ep, &(answer->alp), object_base[SGE_TYPE_USERSET].list,
                               user, host);
               break;

            case SGE_SHARETREE_LIST:
               sge_mod_sharetree(ep, object_base[SGE_TYPE_SHARETREE].list,
                                 &(answer->alp), user, host);
               break;
            default:
               if (ao == NULL) {
                  SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
                  answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
                  break;
               }
               sge_gdi_add_mod_generic(&(answer->alp), ep, 0, ao, user, host, sub_command, &ppList, monitor);
               break;
         }
      }
   } /* for_each */

   if (is_locked) {
      sge_commit();
      SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
   }
   
   /* postprocessing for the list of requests */
   if (lGetNumberOfElem(ppList) != 0) {
      switch (request->target) {
         case SGE_CENTRY_LIST:
            DPRINTF(("rebuilding consumable debitation\n"));
            centry_redebit_consumables(ppList);
            break;
      }
   }

   lFreeList(&ppList);

   DEXIT;
   return;
}

/*
 * MT-NOTE: sge_chck_mod_perm_user() is MT safe
 */
static int sge_chck_mod_perm_user(lList **alpp, u_long32 target, char *user, monitoring_t *monitor)
{

   DENTER(TOP_LAYER, "sge_chck_mod_perm_user");

   MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE), monitor);

   /* check permissions of user */
   switch (target) {

   case SGE_ORDER_LIST:
   case SGE_ADMINHOST_LIST:
   case SGE_SUBMITHOST_LIST:
   case SGE_EXECHOST_LIST:
   case SGE_CQUEUE_LIST:
   case SGE_CENTRY_LIST:
   case SGE_OPERATOR_LIST:
   case SGE_MANAGER_LIST:
   case SGE_PE_LIST:
   case SGE_CONFIG_LIST:
   case SGE_SC_LIST:
   case SGE_USER_LIST:
   case SGE_PROJECT_LIST:
   case SGE_SHARETREE_LIST:
   case SGE_CKPT_LIST:
   case SGE_CALENDAR_LIST:
   case SGE_USER_MAPPING_LIST:
   case SGE_HGROUP_LIST:
      /* user must be a manager */
      if (!manop_is_manager(user)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_MUSTBEMANAGER_S, user));
         answer_list_add(alpp, SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
         DEXIT;
         return 1;
      }
      break;

   case SGE_USERSET_LIST:
      /* user must be a operator */
      if (!manop_is_operator(user)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_MUSTBEOPERATOR_S, user));
         answer_list_add(alpp, SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
         DEXIT;
         return 1;
      }
      break;

   case SGE_JOB_LIST:

      /*
         what checking could we do here ? 

         we had to check if there is a queue configured for scheduling
         of jobs of this group/user. If there is no such queue we
         had to deny submitting.

         Other checkings need to be done in stub functions.

      */
      break;

   case SGE_EVENT_LIST:
      /* 
         an event client can be started by any user - it can only
         read objects like SGE_GDI_GET
         delete requires more info - is done in sge_gdi_kill_eventclient
      */  
      break;
   default:
      SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
      answer_list_add(alpp, SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
      SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
      DEXIT;
      return 1;
   }

   SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
   DEXIT;
   return 0;
}


/*
 * MT-NOTE: sge_chck_mod_perm_host() is MT safe
 */
static int sge_chck_mod_perm_host(lList **alpp, u_long32 target, char *host, 
                                  char *commproc, int mod, lListElem *ep, 
                                  bool is_locked, monitoring_t *monitor,
                                  object_description *object_base)
{
   DENTER(TOP_LAYER, "sge_chck_mod_perm_host");

   if (!is_locked) {
      MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE), monitor);
   }

   /* check permissions of host */
   switch (target) {

   case SGE_ORDER_LIST:
   case SGE_ADMINHOST_LIST:
   case SGE_OPERATOR_LIST:
   case SGE_MANAGER_LIST:
   case SGE_SUBMITHOST_LIST:
   case SGE_CQUEUE_LIST:
   case SGE_CENTRY_LIST:
   case SGE_PE_LIST:
   case SGE_CONFIG_LIST:
   case SGE_SC_LIST:
   case SGE_USER_LIST:
   case SGE_USERSET_LIST:
   case SGE_PROJECT_LIST:
   case SGE_SHARETREE_LIST:
   case SGE_CKPT_LIST:
   case SGE_CALENDAR_LIST:
   case SGE_USER_MAPPING_LIST:
   case SGE_HGROUP_LIST:
      
      /* host must be SGE_ADMINHOST_LIST */
      if (!host_list_locate(*object_base[SGE_TYPE_ADMINHOST].list, host)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_NOADMINHOST_S, host));
         answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
         DEXIT;
         return 1;
      }
      break;

   case SGE_EXECHOST_LIST:
      
      /* host must be either admin host or exec host and execd */

      if (!(host_list_locate(*object_base[SGE_TYPE_ADMINHOST].list, host) ||
         (host_list_locate(*object_base[SGE_TYPE_EXECHOST].list, host) && !strcmp(commproc, prognames[EXECD])))) {
         ERROR((SGE_EVENT, MSG_SGETEXT_NOADMINHOST_S, host));
         answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
         DEXIT;
         return 1;
      }
      break;

   case SGE_JOB_LIST:
#if 1
      /* 
      ** check if override ticket changei request, if yes we need
      ** to be on an admin host and must not be on a submit host
      */
      if ( mod && (lGetPosViaElem(ep, JB_override_tickets, SGE_NO_ABORT) >= 0)) {
         /* host must be SGE_ADMINHOST_LIST */
         if (!host_list_locate(*object_base[SGE_TYPE_ADMINHOST].list, host)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_NOADMINHOST_S, host));
            answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
            SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
            DEXIT;
            return 1;
         }
         break;
      }    
#endif      
      /* host must be SGE_SUBMITHOST_LIST */
      if (!host_list_locate(*object_base[SGE_TYPE_SUBMITHOST].list, host)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_NOSUBMITHOST_S, host));
         answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
         DEXIT;
         return 1;
      }
      break;

   case SGE_EVENT_LIST:
      /* to start an event client or if an event client
         performs modify requests on itself
         it must be on a submit or an admin host 
       */
      if ( (!host_list_locate(*object_base[SGE_TYPE_SUBMITHOST].list, host)) 
        && (!host_list_locate(*object_base[SGE_TYPE_ADMINHOST].list, host))) {
        ERROR((SGE_EVENT, MSG_SGETEXT_NOSUBMITORADMINHOST_S, host));
        answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
        SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
        DEXIT;
        return 1;
      }
      break;
   default:
      SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
      answer_list_add(alpp, SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
      SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
      DEXIT;
      return 1;
   }
   
   if (!is_locked) {
      SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
   }

   DEXIT;
   return 0;
}

/*
 * MT-NOTE: sge_chck_get_perm_host() is MT safe
 */
static int 
sge_chck_get_perm_host(lList **alpp, sge_gdi_request *request, monitoring_t *monitor,
                       object_description *object_base)
{
   u_long32 target;
   char *host     = NULL;
   static int last_id = -1; 
   
   DENTER(TOP_LAYER, "sge_chck_get_perm_host");

   MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE), monitor);

   /* reset the last_id counter on first sequence number we won't
      log the same error message twice in an api multi request */
   if (request->sequence_id == 1) {
      last_id = -1;
   }

   target = request->target;
   host = request->host;

   /* check permissions of host */
   switch (target) {

   case SGE_ORDER_LIST:
   case SGE_EVENT_LIST:
   case SGE_ADMINHOST_LIST:
   case SGE_OPERATOR_LIST:
   case SGE_MANAGER_LIST:
   case SGE_SUBMITHOST_LIST:
   case SGE_CQUEUE_LIST:
   case SGE_CENTRY_LIST:
   case SGE_PE_LIST:
   case SGE_SC_LIST:
   case SGE_USER_LIST:
   case SGE_USERSET_LIST:
   case SGE_PROJECT_LIST:
   case SGE_SHARETREE_LIST:
   case SGE_CKPT_LIST:
   case SGE_CALENDAR_LIST:
   case SGE_USER_MAPPING_LIST:
   case SGE_HGROUP_LIST:
   case SGE_EXECHOST_LIST:
   case SGE_JOB_LIST:
   case SGE_ZOMBIE_LIST:
   case SGE_JOB_SCHEDD_INFO:
      
      /* host must be admin or submit host */
      if ( !host_list_locate(*object_base[SGE_TYPE_ADMINHOST].list, host) &&
           !host_list_locate(*object_base[SGE_TYPE_SUBMITHOST].list, host)) {

         if (last_id != request->id) {     /* only log the first error
                                              in an api multi request */
            ERROR((SGE_EVENT, MSG_SGETEXT_NOSUBMITORADMINHOST_S, host));
         } else {    
            SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_SGETEXT_NOSUBMITORADMINHOST_S, host));
         }
         answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
         last_id = request->id;       /* this indicates that the error is already locked */
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
         DEXIT;
         return 1;
      }
      break;

   case SGE_CONFIG_LIST:
      /* host must be admin or submit host or exec host */
      if ( !host_list_locate(*object_base[SGE_TYPE_ADMINHOST].list, host) &&
           !host_list_locate(*object_base[SGE_TYPE_SUBMITHOST].list, host) &&
           !host_list_locate(*object_base[SGE_TYPE_EXECHOST].list, host)) {
         if (last_id != request->id) {  /* only log the first error
                                              in an api multi request */
            ERROR((SGE_EVENT, MSG_SGETEXT_NOSUBMITORADMINHOST_S, host));
         } else {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SGETEXT_NOSUBMITORADMINHOST_S, host));
         }
         answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
         last_id = request->id;       /* this indicates that the error is already locked */
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
         DEXIT;
         return 1;
      }
      break;

   default:
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
      answer_list_add(alpp, SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
      SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
      DEXIT;
      return 1;
   }

   SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);

   DEXIT;
   return 0;
}


/*
   this is our strategy:

   do common checks and search old object
   make a copy of the old object (this will become the new object)
   modify new object using reduced object as instruction
      on error: dispose new object
   store new object to disc
      on error: dispose new object
   on success create events
   replace old object by new queue
*/
int sge_gdi_add_mod_generic(
lList **alpp,
lListElem *instructions, /* our instructions - a reduced object */
int add,                 /* true in case of add */
gdi_object_t *object, 
const char *ruser,
const char *rhost,
int sub_command,
lList **ppList,
monitoring_t *monitor
) {
   int pos;
   int dataType;
   const char *name;
   lList *tmp_alp = NULL;
   lListElem *new_obj = NULL,
             *old_obj;

   lListElem *tmp_ep = NULL;

   DENTER(TOP_LAYER, "sge_gdi_add_mod_generic");

   /* DO COMMON CHECKS AND SEARCH OLD OBJECT */
   if (!instructions || !object) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* ep is no element of this type, if ep has no QU_qname */
   if (lGetPosViaElem(instructions, object->key_nm, SGE_NO_ABORT) < 0)
   {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS, lNm2Str(object->key_nm), SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* resolve host name in case of objects with hostnames as key 
      before searching for the objects */
   
   if ( object->key_nm == EH_name || 
        object->key_nm == AH_name || 
        object->key_nm == SH_name ) {
      if ( sge_resolve_host(instructions, object->key_nm) != CL_RETVAL_OK )
      {
         const char *host = lGetHost(instructions, object->key_nm);    
         ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, host ? host : "NULL"));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }

   pos = lGetPosViaElem(instructions,  object->key_nm, SGE_NO_ABORT);
   dataType = lGetPosType(lGetElemDescr(instructions),pos);
   if (dataType == lHostT) {
      name = lGetHost(instructions, object->key_nm); 
      old_obj = lGetElemHost(*object_type_get_master_list(object->list_type), object->key_nm, name);
      
   } else {
      name = lGetString(instructions, object->key_nm); 
      old_obj = lGetElemStr(*object_type_get_master_list(object->list_type), object->key_nm, name);
   }

   if ((old_obj && add) ||
      (!old_obj && !add)) {
      ERROR((SGE_EVENT, add?
            MSG_SGETEXT_ALREADYEXISTS_SS:MSG_SGETEXT_DOESNOTEXIST_SS, 
            object->object_name, name));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* MAKE A COPY OF THE OLD QUEUE (THIS WILL BECOME THE NEW QUEUE) */
   if (!(new_obj = (add 
         ? lCreateElem(object->type) 
         : lCopyElem(old_obj)))) {
      ERROR((SGE_EVENT, MSG_MEM_MALLOC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* MODIFY NEW QUEUE USING REDUCED QUEUE AS INSTRUCTION */
   if (object->modifier(&tmp_alp, new_obj, instructions, add, ruser, rhost, 
         object, sub_command, monitor)) {
      
      if (alpp) {
         /* ON ERROR: DISPOSE NEW QUEUE */
         /* failure: just append last elem in tmp_alp
            elements before may contain invalid success messages */
         if (tmp_alp) {
            lListElem *failure;
            failure = lLast(tmp_alp);
            lDechainElem(tmp_alp, failure);
            if (!*alpp)
               *alpp = lCreateList("answer", AN_Type);
            lAppendElem(*alpp, failure);
         }
      }
      lFreeList(&tmp_alp);
      lFreeElem(&new_obj);
      DEXIT;
      return STATUS_EUNKNOWN;
   }  


   /* write on file */
   if (object->writer(alpp, new_obj, object)) {
      lFreeElem(&new_obj);
      lFreeList(&tmp_alp);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if (alpp != NULL) {
      if (*alpp == NULL) {
         *alpp = lCreateList("answer", AN_Type);
      }
   
      /* copy every entrie from tmp_alp into alpp */
      for_each (tmp_ep, tmp_alp) {
         lListElem* copy = NULL;
      
         copy = lCopyElem(tmp_ep);
         if (copy != NULL) {
            lAppendElem(*alpp,copy);
         } 
      }
   }
   lFreeList(&tmp_alp);
   {
      lList **master_list = NULL;

      master_list = object_type_get_master_list(object->list_type);
         
   /* chain out the old object */
      if (old_obj) {
         lDechainElem(*master_list, old_obj);
      }

      /* ensure our global list exists */ 
      if (*master_list == NULL ) {
         *master_list = lCreateList(object->object_name, object->type);
      }

      /* chain in new object */
      lAppendElem(*master_list, new_obj);
   }
   if (object->on_success) {
      object->on_success(new_obj, old_obj, object, ppList, monitor);
   }

   lFreeElem(&old_obj);

   INFO((SGE_EVENT, 
      add?MSG_SGETEXT_ADDEDTOLIST_SSSS:
          MSG_SGETEXT_MODIFIEDINLIST_SSSS, ruser, rhost, name, object->object_name));

   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
    
   DEXIT;
   return STATUS_OK;
}

/*
 * MT-NOTE: get_gdi_object() is MT safe
 */
gdi_object_t *get_gdi_object(u_long32 target)
{
   int i;

   DENTER(TOP_LAYER, "get_gdi_object");

   for (i=0; gdi_object[i].target; i++)
      if (target == gdi_object[i].target) {
         DEXIT;
         return &gdi_object[i];
      }

   DEXIT;
   return NULL;
}

static int schedd_mod(
lList **alpp,
lListElem *modp,
lListElem *ep,
int add,
const char *ruser,
const char *rhost,
gdi_object_t *object,
int sub_command, monitoring_t *monitor
) {
   int ret;
   DENTER(TOP_LAYER, "schedd_mod");

   ret = sconf_validate_config_(alpp) ? 0 : 1;
   
   DEXIT;
   return ret;
}

