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
#include "sge_qmaster_main.h"
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



static void sge_c_gdi_get(gdi_object_t *ao, char *host, sge_gdi_request *request, sge_gdi_request *answer, int *before, int *after);
static void sge_c_gdi_add(gdi_object_t *ao, char *host, sge_gdi_request *request, sge_gdi_request *answer, int return_list_flag);
static void sge_c_gdi_del(char *host, sge_gdi_request *request, sge_gdi_request *answer, int sub_command);
static void sge_c_gdi_mod(gdi_object_t *ao, char *host, sge_gdi_request *request, sge_gdi_request *answer, int sub_command);
static void sge_c_gdi_copy(gdi_object_t *ao, char *host, sge_gdi_request *request, sge_gdi_request *answer, int sub_command);

static void sge_c_gdi_permcheck(char *host, sge_gdi_request *request, sge_gdi_request *answer);
static void sge_gdi_do_permcheck(char *host, sge_gdi_request *request, sge_gdi_request *answer);
static void sge_c_gdi_trigger(char *host, sge_gdi_request *request, sge_gdi_request *answer);

static void sge_gdi_shutdown_event_client(const char*, sge_gdi_request*, sge_gdi_request*, lList **alpp);
static int  get_client_id(lListElem*, int*);
static void trigger_scheduler_monitoring(char*, sge_gdi_request*, sge_gdi_request*); 

static int sge_chck_mod_perm_user(lList **alpp, u_long32 target, char *user);
static int sge_chck_mod_perm_host(lList **alpp, u_long32 target, char *host, char *commproc, int mod, lListElem *ep);
static int sge_chck_get_perm_host(lList **alpp, sge_gdi_request *request);


static int schedd_mod( lList **alpp, lListElem *modp, lListElem *ep, int add, const char *ruser, const char *rhost, gdi_object_t *object, int sub_command );
static int do_gdi_get_config_list(sge_gdi_request *aReq, sge_gdi_request *aRes, int *aBeforeCnt, int *anAfterCnt);

/* ------------------------------ generic gdi objects --------------------- */
/* *INDENT-OFF* */
static gdi_object_t gdi_object[] = {
   { SGE_CALENDAR_LIST,     CAL_name,  CAL_Type,  "calendar",                &Master_Calendar_List,        NULL,                  calendar_mod, calendar_spool, calendar_update_queue_states },
   { SGE_EVENT_LIST,        0,         NULL,      "event",                   NULL,                         NULL,                  NULL,         NULL,           NULL },
   { SGE_ADMINHOST_LIST,    AH_name,   AH_Type,   "adminhost",               &Master_Adminhost_List,       NULL,                  host_mod,     host_spool,     host_success },
   { SGE_SUBMITHOST_LIST,   SH_name,   SH_Type,   "submithost",              &Master_Submithost_List,      NULL,                  host_mod,     host_spool,     host_success },
   { SGE_EXECHOST_LIST,     EH_name,   EH_Type,   "exechost",                &Master_Exechost_List,        NULL,                  host_mod,     host_spool,     host_success },
   { SGE_CQUEUE_LIST,       CQ_name,   CQ_Type,   "cluster queue",           &Master_CQueue_List,          NULL,                  cqueue_mod,   cqueue_spool,   cqueue_success },
   { SGE_JOB_LIST,          0,         NULL,      "job",                     &Master_Job_List,             NULL,                  NULL,         NULL,           NULL },
   { SGE_CENTRY_LIST,       CE_name,   CE_Type,   "complex entry",           &Master_CEntry_List,          NULL,                  centry_mod,   centry_spool,   centry_success },
   { SGE_ORDER_LIST,        0,         NULL,      "order",                   NULL,                         NULL,                  NULL,         NULL,           NULL },
   { SGE_MASTER_EVENT,      0,         NULL,      "master event",            NULL,                         NULL,                  NULL,         NULL,           NULL },
   { SGE_MANAGER_LIST,      0,         NULL,      "manager",                 &Master_Manager_List,         NULL,                  NULL,         NULL,           NULL },
   { SGE_OPERATOR_LIST,     0,         NULL,      "operator",                &Master_Operator_List,        NULL,                  NULL,         NULL,           NULL },
   { SGE_PE_LIST,           PE_name,   PE_Type,   "parallel environment",    &Master_Pe_List,              NULL,                  pe_mod,       pe_spool,       pe_success },
   { SGE_CONFIG_LIST,       0,         NULL,      "configuration",           NULL,                         NULL,                  NULL,         NULL,           NULL },
   { SGE_SC_LIST,           0,         NULL,      "scheduler configuration", NULL,                         sconf_get_config_list, schedd_mod,   NULL,           NULL },
   { SGE_USER_LIST,         UP_name,   UP_Type,   "user",                    &Master_User_List,            NULL,                  userprj_mod,  userprj_spool,  userprj_success },
   { SGE_USERSET_LIST,      0,         NULL,      "userset",                 &Master_Userset_List,         NULL,                  NULL,         NULL,           NULL },
   { SGE_PROJECT_LIST,      UP_name,   UP_Type,   "project",                 &Master_Project_List,         NULL,                  userprj_mod,  userprj_spool,  userprj_success },
   { SGE_SHARETREE_LIST,    0,         NULL,      "sharetree",               &Master_Sharetree_List,       NULL,                  NULL,         NULL,           NULL },
   { SGE_CKPT_LIST,         CK_name,   CK_Type,   "checkpoint interface",    &Master_Ckpt_List,            NULL,                  ckpt_mod,     ckpt_spool,     ckpt_success },
   { SGE_JOB_SCHEDD_INFO,   0,         NULL,      "schedd info",             &Master_Job_Schedd_Info_List, NULL,                  NULL,         NULL,           NULL },
   { SGE_ZOMBIE_LIST,       0,         NULL,      "job zombie list",         &Master_Zombie_List,          NULL,                  NULL,         NULL,           NULL },
#ifndef __SGE_NO_USERMAPPING__
   { SGE_USER_MAPPING_LIST, CU_name,   CU_Type,   "user mapping entry",      &Master_Cuser_List,           NULL,                  cuser_mod,    cuser_spool,    cuser_success },
#endif
   { SGE_HGROUP_LIST,       HGRP_name, HGRP_Type, "host group",              &Master_HGroup_List,          NULL,                  hgroup_mod,   hgroup_spool,   hgroup_success },
   { SGE_DUMMY_LIST,        0,         NULL,      "general request",         NULL,                         NULL,                  NULL,         NULL,           NULL },
   { 0,                     0,         NULL,      NULL,                      NULL,                         NULL,                  NULL,         NULL,           NULL }
};
/* *INDENT-ON* */

void sge_clean_lists(void) {
   int i = 0;

   for(;gdi_object[i].target != 0 ; i++) {
      if (gdi_object[i].master_list != NULL) {
/*          fprintf(stderr, "---> freeing list %s, it has %d elems\n", gdi_object[i].object_name, lGetNumberOfElem(*gdi_object[i].master_list)); */
         *gdi_object[i].master_list = lFreeList(*gdi_object[i].master_list);
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
      { 0,          NULL   }
   };

   DENTER(TOP_LAYER, "verify_request_version");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if (version == GRM_GDI_VERSION) {
      DEXIT;
      return 0;
   }

   for (vp = &vdict[0]; vp->version; vp++)
      if (version == vp->version)
         client_version = vp->release;

   if (client_version)
      WARNING((SGE_EVENT, MSG_GDI_WRONG_GDI_SSISS,
         host, commproc, id, client_version, feature_get_product_name(FS_VERSION, &ds)));
   else
      WARNING((SGE_EVENT, MSG_GDI_WRONG_GDI_SSIUS,
         host, commproc, id, u32c(version), feature_get_product_name(FS_VERSION, &ds)));
   answer_list_add(alpp, SGE_EVENT, STATUS_EVERSION, ANSWER_QUALITY_ERROR);

   DEXIT;
   return 1;
}

/* ------------------------------------------------------------ */

void sge_c_gdi(
char *host,
sge_gdi_request *request,
sge_gdi_request *answer 
) {
   const char *target_name = NULL;
   char *operation_name = NULL;
   int before = 0, after = 0;
#if 0
   int return_list_flag = 0;
   int all_jobs_flag = 0;
   int all_users_flag = 0;
#endif
   int sub_command = 0;
   gdi_object_t *ao;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];

   DENTER(TOP_LAYER, "sge_c_gdi");

   answer->op = request->op;
   answer->target = request->target;
   answer->sequence_id = request->sequence_id;
   answer->request_id = request->request_id;

   if (verify_request_version(&(answer->alp), request->version, request->host, 
            request->commproc, request->id)) {
      DEXIT;
      return;
   }

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (!user || !group) {
      CRITICAL((SGE_EVENT, MSG_GDI_NULL_IN_GDI_SSS,  
               (!user)?MSG_OBJ_USER:"", 
               (!group)?MSG_OBJ_GROUP:"", host));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (!sge_security_verify_user(request->host, request->commproc, request->id, user)) {
      CRITICAL((SGE_EVENT, MSG_SEC_CRED_SSSI, user, request->host, 
                  request->commproc, request->id));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOSUCHUSER, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   sge_set_commit_required(true);

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
   ** a list answer->lp this list is handed over to the corresponding
   ** add/modify routine.
   ** Now only for job add available.
   */
#if 0
   all_users_flag = request->op / SGE_GDI_ALL_USERS;
   request->op %= SGE_GDI_ALL_USERS;

   all_jobs_flag = request->op / SGE_GDI_ALL_JOBS;
   request->op %= SGE_GDI_ALL_JOBS;

   return_list_flag = request->op / SGE_GDI_RETURN_NEW_VERSION;
   request->op %= SGE_GDI_RETURN_NEW_VERSION;
#endif

   sub_command = SGE_GDI_GET_SUBCOMMAND(request->op);
   request->op = SGE_GDI_GET_OPERATION(request->op);

   switch (request->op) {
   case SGE_GDI_GET:
      operation_name = "GET";
      break;
   case SGE_GDI_ADD:
      operation_name = "ADD";
      break;
   case SGE_GDI_DEL:
      operation_name = "DEL";
      break;
   case SGE_GDI_MOD:
      operation_name = "MOD";
      break;
   case SGE_GDI_COPY:
      operation_name = "COPY";
      break;
   case SGE_GDI_TRIGGER:
      operation_name = "TRIGGER";
      break;
   case SGE_GDI_PERMCHECK:
      operation_name = "PERMCHECK";
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
      sge_c_gdi_get(ao, host, request, answer, &before, &after);
      break;

   case SGE_GDI_ADD:
      sge_c_gdi_add(ao, host, request, answer, sub_command);
      break;

   case SGE_GDI_DEL:
      sge_c_gdi_del(host, request, answer, sub_command);
      break;

   case SGE_GDI_MOD:
      sge_c_gdi_mod(ao, host, request, answer, sub_command);
      break;

   case SGE_GDI_COPY:
      sge_c_gdi_copy(ao, host, request, answer, sub_command);
      break;

   case SGE_GDI_TRIGGER:
      sge_c_gdi_trigger(host, request,answer);
      break;

   case SGE_GDI_PERMCHECK:
      sge_c_gdi_permcheck(host, request, answer);
      break;

   default:
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SGETEXT_UNKNOWNOP));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
      break;
   }

   /* different report types */
   switch (request->op) {

   case SGE_GDI_GET:
 	DPRINTF(("GDI %s %s (%s/%s/%d) (%s/%d/%s/%d) %d -> %d\n",
			operation_name, target_name, 
			request->host, request->commproc, (int)request->id,
			user, (int)uid, group, (int)gid,
         before, after));
       break;

   case SGE_GDI_ADD:
   case SGE_GDI_DEL:
   case SGE_GDI_MOD:
   case SGE_GDI_COPY:
   case SGE_GDI_TRIGGER:
   default:
     break;
   }

   sge_commit();
   /* enable event transaction handling */
   sge_set_commit_required(false);

   DEXIT;
   return;
}


/*
 * MT-NOTE: sge_c_gdi_get() is MT safe
 */
static void sge_c_gdi_get(
gdi_object_t *ao,
char *host,
sge_gdi_request *request,
sge_gdi_request *answer,
int *before,
int *after 
) {
   lList *lp = NULL;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];
   dstring ds;
   char buffer[256];

   DENTER(GDI_LAYER, "sge_c_gdi_get");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if (sge_chck_get_perm_host(&(answer->alp), request ))
   {
      DEXIT;
      return;
   }

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1)
   {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   switch (request->target)
   {
#ifdef QHOST_TEST
      case SGE_QHOST:
         sprintf(SGE_EVENT, "SGE_QHOST\n");
         answer_list_add(&(answer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
         DEXIT;
         return;
#endif
      case SGE_EVENT_LIST:
         answer->lp = sge_select_event_clients("qmaster_response", request->cp, request->enp);
         sprintf(SGE_EVENT, MSG_GDI_OKNL);
         answer_list_add(&(answer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
         DEXIT;
         return;
         
      case SGE_CONFIG_LIST:
         do_gdi_get_config_list(request, answer, before, after);
         DEXIT;
         return;
         
      default:
         if (ao == NULL || (ao->master_list == NULL && ao->getMasterList == NULL))
         {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
            answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
            DEXIT;
            return;
         }
   }

   SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE);

   if (ao->master_list)
      lp = *(ao->master_list);
   else
      lp = *(ao->getMasterList());

   *before = lGetNumberOfElem(lp);
   answer->lp = lSelectHash("qmaster_response", lp, request->cp, request->enp, false);

   *after = lGetNumberOfElem(answer->lp);

   sprintf(SGE_EVENT, MSG_GDI_OKNL);
   answer_list_add(&(answer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
 
   SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
     
   DEXIT;
   return;
}

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

   aRes->lp = lSelectHash("qmaster_response", conf, aReq->cp, aReq->enp, false);

   conf = lFreeList(conf);

   *anAfterCnt = lGetNumberOfElem(aRes->lp);

   sprintf(SGE_EVENT, MSG_GDI_OKNL);

   answer_list_add(&(aRes->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);

   DEXIT;
   return 0;
}

/*
 * MT-NOTE: sge_c_gdi_add() is MT safe
 */
static void sge_c_gdi_add(gdi_object_t *ao, char *host, sge_gdi_request *request, sge_gdi_request *answer, int sub_command)
{
   int ret;
   lListElem *ep;
   lList *ticket_orders = NULL;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "sge_c_gdi_add");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1)
   {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (!request->host || !user || !request->commproc)
   {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   for_each (ep, request->lp)
   {
      /* check permissions of host and user */
      if (sge_chck_mod_perm_user(&(answer->alp), request->target, user))
         continue;
      if (sge_chck_mod_perm_host(&(answer->alp), request->target, request->host, request->commproc, 0, NULL))
         continue;

      SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE);

      /* add each element */
      switch (request->target)
      {
         case SGE_EVENT_LIST:
            /* fill address infos from request into event client that must be added */
            lSetHost(ep, EV_host, request->host);
            lSetString(ep, EV_commproc, request->commproc);
            lSetUlong(ep, EV_commid, request->id);

            /* fill in authentication infos from request */
            lSetUlong(ep, EV_uid, uid);

            max_dynamic_event_clients = sge_set_max_dynamic_event_clients(max_dynamic_event_clients);
            
            sge_add_event_client(ep,&(answer->alp), (sub_command & SGE_GDI_RETURN_NEW_VERSION) ? &(answer->lp) : NULL, user, host);
            break;

         case SGE_ORDER_LIST:
         {
            u_long32 event_number, now;
            static u_long32 last_order_arrived = 0;
            
            /* statistics */
            if (lFirst(request->lp) == ep) {
               now = sge_get_gmt();
               if (last_order_arrived) {
                  DPRINTF(("TIME SINCE LAST ORDER: %d seconds\n", 
                     now - last_order_arrived));
               }
               last_order_arrived = now;
            } 

            event_number = sge_get_next_event_number(EV_ID_SCHEDD); 
            if ((ret=sge_follow_order(ep, &(answer->alp), user, 
                  host, &ticket_orders))!=STATUS_OK) {
               DPRINTF(("Failed to follow order #%d. Remaining %d orders unprocessed.\n", 
                     lGetUlong(ep, OR_seq_no), lGetNumberOfRemainingElem(ep)));
               ep = lLast(request->lp); /* this will exit the loop */

               if (ret==-2) {
                  sge_resync_schedd();
               }
            }
            break;
         }
         
         case SGE_JOB_LIST:
            if(simulate_hosts == 1) {
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
                                  user, host, request);
                  lFreeElem(clone);                
               }
               
            } else {
               /* submit needs to know user and group */
               sge_gdi_add_job(ep, &(answer->alp), 
                               (sub_command & SGE_GDI_RETURN_NEW_VERSION) ? 
                               &(answer->lp) : NULL, 
                               user, host, request);
            }
            break;

         case SGE_MANAGER_LIST:
         case SGE_OPERATOR_LIST:
            sge_add_manop(ep, &(answer->alp), user, host, request->target);
            break;

         case SGE_USERSET_LIST:
            sge_add_userset(ep, &(answer->alp), &Master_Userset_List, user, host);
            break;

         case SGE_SHARETREE_LIST:
            sge_add_sharetree(ep, &Master_Sharetree_List, &(answer->alp), user, host);
            break;

         case SGE_SC_LIST:
            sge_mod_sched_configuration(ep, &(answer->alp), user, host);
            break;

         default:
            if (!ao)
            {
               SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
               answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
               break;
            } 
            
            if (request->target==SGE_EXECHOST_LIST && !strcmp(prognames[EXECD], request->commproc)) {
               sge_execd_startedup(ep, &(answer->alp), user, host, request->target);
            } else {
               sge_gdi_add_mod_generic(&(answer->alp), ep, 1, ao, user, host, sub_command);
            }
            break;
      }

      SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
   }

   if (ticket_orders)
   {
      if (sge_conf_is_reprioritize())
      {
         SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE);
         distribute_ticket_orders(ticket_orders);
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
      }
      else
      {
         /* tickets not needed at execd's if no repriorization is done */
         DPRINTF(("NO TICKET DELIVERY\n"));
      }
      ticket_orders = lFreeList(ticket_orders);
   }

   DEXIT;
   return;
}

/*
 * MT-NOTE: sge_c_gdi-del() is MT safe
 */
static void sge_c_gdi_del(char *host, sge_gdi_request *request, sge_gdi_request *answer, int sub_command)
{
   lListElem *ep;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];
   dstring ds;
   char buffer[256];

   DENTER(GDI_LAYER, "sge_c_gdi_del");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1)
   {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (!request->lp) /* delete whole list */
   { 
      if (sge_chck_mod_perm_user(&(answer->alp), request->target, user))
      {
         DEXIT;
         return;
      }
      
      if (sge_chck_mod_perm_host(&(answer->alp), request->target, request->host, request->commproc, 0, NULL))
      {
         DEXIT;
         return;
      }
      
      switch (request->target)
      {
         case SGE_SHARETREE_LIST:
            SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE);
            sge_del_sharetree(&Master_Sharetree_List, &(answer->alp), user,host);
            SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
            break;
            
         default:
            SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
            answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
            break;
      }
   }
   else 
   {
      for_each (ep, request->lp)
      {
         if (sge_chck_mod_perm_user(&(answer->alp), request->target, user))
            continue;
         if (sge_chck_mod_perm_host(&(answer->alp), request->target, request->host, request->commproc, 0, NULL))
            continue;

         SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE);

         /* try to remove the element */
         switch (request->target)
         {
            case SGE_ADMINHOST_LIST:
            case SGE_SUBMITHOST_LIST:
            case SGE_EXECHOST_LIST:
               sge_del_host(ep, &(answer->alp), user, host, request->target);
               break;

            case SGE_CQUEUE_LIST:
               cqueue_del(ep, &(answer->alp), user, host);
               break;

            case SGE_JOB_LIST:
               sge_gdi_del_job(ep, &(answer->alp), user, host, sub_command);
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
               sge_del_userprj(ep, &(answer->alp), &Master_User_List, user, host, 1);
               break;

            case SGE_USERSET_LIST:
               sge_del_userset(ep, &(answer->alp), &Master_Userset_List, user, host);
               break;

            case SGE_PROJECT_LIST:
               sge_del_userprj(ep, &(answer->alp), &Master_Project_List, user, host, 0);
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
static void sge_c_gdi_copy(gdi_object_t *ao, char *host, sge_gdi_request *request, sge_gdi_request *answer, int sub_command)
{
   lListElem *ep;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];


   DENTER(TOP_LAYER, "sge_c_gdi_copy");

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1)
   {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (!request->host || !user || !request->commproc)
   {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   for_each (ep, request->lp)
   {
      if (sge_chck_mod_perm_user(&(answer->alp), request->target, user))
         continue;
      if (sge_chck_mod_perm_host(&(answer->alp), request->target, request->host, request->commproc, 0, NULL))
         continue;

      SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE);

      switch (request->target)
      {
         case SGE_JOB_LIST:
            sge_gdi_copy_job(ep, &(answer->alp), (sub_command & SGE_GDI_RETURN_NEW_VERSION) ? &(answer->lp) : NULL, user, host, request);
            break;

         default:
            SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
            answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
            break;
      }
      
      SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
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

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1) {
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
         lFreeList(lp); 
         lp = NULL;
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
static void sge_c_gdi_permcheck(char *host, sge_gdi_request *request, sge_gdi_request *answer)
{
  DENTER(GDI_LAYER, "sge_c_gdi_permcheck");

  SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE);
  
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
static void sge_c_gdi_trigger(char *host, sge_gdi_request *request, sge_gdi_request *answer)
{
   lList *alp = NULL;
   
   
   DENTER(GDI_LAYER, "sge_c_gdi_trigger");

   SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE);

   switch (request->target)
   {
      case SGE_EXECHOST_LIST: /* kill execd */
      case SGE_MASTER_EVENT:  /* kill master */
      case SGE_EVENT_LIST:    /* kill scheduler */
      case SGE_SC_LIST:       /* trigger scheduler monitoring */
         if (!host_list_locate(Master_Adminhost_List, host))
         {
            ERROR((SGE_EVENT, MSG_SGETEXT_NOADMINHOST_S, host));
            answer_list_add(&(answer->alp), SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return;
         }
         break;
      default:
         /* permissions should be checked in the functions. Here we don't
            know what is to do, so we don't know what permissions we need */
         break;
   }

   switch (request->target)
   {
      case SGE_CQUEUE_LIST: /* do no break here! */
      case SGE_JOB_LIST:    /* en/dis/able un/suspend clean/clear job/queue */
         sge_gdi_qmod(host, request, answer);
         break;

      case SGE_EXECHOST_LIST: /* kill execd */
         sge_gdi_kill_exechost(host, request, answer);
         break;

     case SGE_SC_LIST: /* trigger scheduler monitoring */
         trigger_scheduler_monitoring(host, request, answer);
         break;

      case SGE_EVENT_LIST:
         /* JG: TODO: why does sge_gdi_shutdown_event_client use two different answer lists? */
         sge_gdi_shutdown_event_client(host, request, answer, &alp);
         answer_list_output(&alp);
         break;

      default:
         WARNING((SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
         answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
         break;
   }

   SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
      
   if (SGE_MASTER_EVENT == (request->target))
   {
      /* shutdown qmaster. Do NOT hold the global lock, while doing this !! */
      sge_gdi_kill_master(host, request, answer);
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
*     lList **alpp              - answer list for info & errors
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
                                          lList **alpp)
{
   uid_t uid = 0;
   gid_t gid = 0;
   char user[128]  = { '\0' };
   char group[128] = { '\0' };
   lListElem *elem = NULL; /* ID_Type */
   lListElem *answer = NULL;

   DENTER(TOP_LAYER, "sge_gdi_shutdown_event_client");

   if (sge_get_auth_info(aRequest, &uid, user, &gid, group) == -1)
   {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(anAnswer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   for_each (elem, aRequest->lp)
   {
      int client_id = EV_ID_ANY;
      int res = -1;

      if (get_client_id(elem, &client_id) != 0) {
         answer_list_add(&(anAnswer->alp), SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         continue;
      }

      if (client_id == EV_ID_ANY) {
         res = sge_shutdown_dynamic_event_clients(user, alpp);
      } else {
         res = sge_shutdown_event_client(client_id, user, uid, alpp);
      }

      
      /* Process answer */
      if (anAnswer->alp == NULL) {
         anAnswer->alp = lCopyList ("Answer", *alpp);
      }
      else {
         for_each (answer, *alpp) {
            answer = lDechainElem (*alpp, answer);
            lAppendElem (anAnswer->alp, answer);
         }
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
*     MT-NOTE: trigger_scheduler_monitoring() is not MT safe 
*
*  SEE ALSO
*     qconf -tsm
*
*******************************************************************************/
void trigger_scheduler_monitoring(char *aHost, sge_gdi_request *aRequest, sge_gdi_request *anAnswer) 
{
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];

   DENTER(GDI_LAYER, "trigger_scheduler_monitoring");

   if (sge_get_auth_info(aRequest, &uid, user, &gid, group) == -1)
   {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(anAnswer->alp), SGE_EVENT, STATUS_ENOMGR, 0);
      DEXIT;
      return;
   }

   if (!manop_is_manager(user))
   {
      WARNING((SGE_EVENT, MSG_COM_NOSCHEDMONPERMS));
      answer_list_add(&(anAnswer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_WARNING);
      DEXIT;
      return;
   }
     
   if (!sge_add_event_for_client(EV_ID_SCHEDD, 0, sgeE_SCHEDDMONITOR, 0, 0, NULL, NULL, NULL, NULL))
   {
      WARNING((SGE_EVENT, MSG_COM_NOSCHEDDREGMASTER));
      answer_list_add(&(anAnswer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_WARNING);
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
static void sge_c_gdi_mod(gdi_object_t *ao, char *host, sge_gdi_request *request, sge_gdi_request *answer, int sub_command)
{
   lListElem *ep;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "sge_c_gdi_mod");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1)
   {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   for_each (ep, request->lp)
   {
      if (sge_chck_mod_perm_user(&(answer->alp), request->target, user)) {
         continue;
      }
      if (sge_chck_mod_perm_host(&(answer->alp), request->target, request->host, request->commproc, 1, ep)) {
         continue;
      }

      if (SGE_CONFIG_LIST == (request->target))
      {
         sge_mod_configuration(ep, &(answer->alp), user, host);      
      }
      else if (SGE_EVENT_LIST == (request->target))
      {
         /* fill address infos from request into event client that must be added */
         lSetHost(ep, EV_host, request->host);
         lSetString(ep, EV_commproc, request->commproc);
         lSetUlong(ep, EV_commid, request->id);
 
         /* fill in authentication infos from request */
         lSetUlong(ep, EV_uid, uid);
 
         sge_mod_event_client(ep, &(answer->alp), NULL, user, host);      
      }
      else 
      {
         SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE);
               
         switch (request->target)
         {
            case SGE_JOB_LIST:
               sge_gdi_mod_job(ep, &(answer->alp), user, host, sub_command);
               break;

            case SGE_SC_LIST:
               sge_mod_sched_configuration(ep, &(answer->alp), user, host);
               break;

            case SGE_USERSET_LIST:
               sge_mod_userset(ep, &(answer->alp), &Master_Userset_List, user, host);
               break;

            case SGE_SHARETREE_LIST:
               sge_mod_sharetree(ep, &Master_Sharetree_List, &(answer->alp), user, host);
               break;
#if 0
            case SGE_CKPT_LIST:
               sge_mod_ckpt(ep, &(answer->alp), user, host);
               break;
#endif
            default:
               if (!ao)
               {
                  SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
                  answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
                  break;
               }
                
               sge_gdi_add_mod_generic(&(answer->alp), ep, 0, ao, user, host, sub_command);
               break;
         }
         
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
      }
   } /* for_each */

   DEXIT;
   return;
}

/*
 * MT-NOTE: sge_chck_mod_perm_user() is MT safe
 */
static int sge_chck_mod_perm_user(lList **alpp, u_long32 target, char *user)
{

   DENTER(TOP_LAYER, "sge_chck_mod_perm_user");

   SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE);

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
static int sge_chck_mod_perm_host(lList **alpp, u_long32 target, char *host, char *commproc, int mod, lListElem *ep)
{
   DENTER(TOP_LAYER, "sge_chck_mod_perm_host");

   SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE);

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
      if (!host_list_locate(Master_Adminhost_List, host)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_NOADMINHOST_S, host));
         answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
         DEXIT;
         return 1;
      }
      break;

   case SGE_EXECHOST_LIST:
      
      /* host must be either admin host or exec host and execd */

      if (!(host_list_locate(Master_Adminhost_List, host) ||
         (host_list_locate(Master_Exechost_List, host) && !strcmp(commproc, prognames[EXECD])))) {
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
      if ( mod && (lGetPosViaElem(ep, JB_override_tickets) >= 0)) {
         /* host must be SGE_ADMINHOST_LIST */
         if (!host_list_locate(Master_Adminhost_List, host)) {
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
      if (!host_list_locate(Master_Submithost_List, host)) {
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
      if ( (!host_list_locate(Master_Submithost_List, host)) 
        && (!host_list_locate(Master_Adminhost_List, host))) {
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

   SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
   DEXIT;
   return 0;
}

/*
 * MT-NOTE: sge_chck_get_perm_host() is MT safe
 */
static int sge_chck_get_perm_host(lList **alpp, sge_gdi_request *request)
{
   u_long32 target;
   char *host     = NULL;
   char *commproc = NULL;
   static int last_id = -1; 
   
   DENTER(TOP_LAYER, "sge_chck_get_perm_host");

   SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE);

   /* reset the last_id counter on first sequence number we won't
      log the same error message twice in an api multi request */
   if (request->sequence_id == 1) {
      last_id = -1;
   }

   target = request->target;
   host = request->host;
   commproc = request->commproc;

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
      if ( !host_list_locate(Master_Adminhost_List, host) &&
           !host_list_locate(Master_Submithost_List, host)) {

         if (last_id != request->id) {     /* only log the first error
                                              in an api multi request */
            ERROR((SGE_EVENT, MSG_SGETEXT_NOSUBMITORADMINHOST_S, host));
         } else {    
            SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_SGETEXT_NOSUBMITORADMINHOST_S, host));
         }
         answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
         last_id = request->id;       /* this indicates that the error is allready locked */
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
         DEXIT;
         return 1;
      }
      break;

   case SGE_CONFIG_LIST:
      /* host must be admin or submit host or exec host */
      if ( !host_list_locate(Master_Adminhost_List, host) &&
           !host_list_locate(Master_Submithost_List, host) &&
           !host_list_locate(Master_Exechost_List, host)) {
         if (last_id != request->id) {  /* only log the first error
                                              in an api multi request */
            ERROR((SGE_EVENT, MSG_SGETEXT_NOSUBMITORADMINHOST_S, host));
         } else {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SGETEXT_NOSUBMITORADMINHOST_S, host));
         }
         answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
         last_id = request->id;       /* this indicates that the error is allready locked */
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
int sub_command 
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
   if (lGetPosViaElem(instructions, object->key_nm) < 0)
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

   pos = lGetPosViaElem(instructions,  object->key_nm );
   dataType = lGetPosType(lGetElemDescr(instructions),pos);
   if (dataType == lHostT) {
      name = lGetHost(instructions, object->key_nm); 
      if (object->getMasterList)
         old_obj = lGetElemHost(*(object->getMasterList()), object->key_nm, name);
      else
         old_obj = lGetElemHost(*(object->master_list), object->key_nm, name);
      
   } else {
      name = lGetString(instructions, object->key_nm); 
      if (object->getMasterList)
         old_obj = lGetElemStr(*(object->getMasterList()), object->key_nm, name);
      else
         old_obj = lGetElemStr(*(object->master_list), object->key_nm, name);
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
         object, sub_command)) {
      
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
      lFreeList(tmp_alp);
      lFreeElem(new_obj);
      DEXIT;
      return STATUS_EUNKNOWN;
   }  


   /* write on file */
   if (object->writer(alpp, new_obj, object)) {
      lFreeElem(new_obj);
      lFreeList(tmp_alp);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if (alpp) {
      if (!*alpp) {
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
   tmp_alp = lFreeList(tmp_alp);
   {
      lList ** master_list;
      if (object->master_list)
         master_list = object->master_list;
      else
         master_list = object->getMasterList();
         
   /* chain out the old object */
      if (old_obj) {
         lDechainElem(*master_list, old_obj);
      }

      /* ensure our global list exists */ 
      if (!*(master_list)) {
         *(master_list) = lCreateList(object->object_name, object->type);
      }

      /* chain in new object */
      lAppendElem(*(master_list), new_obj);
   }
   if (object->on_success)
      object->on_success(new_obj, old_obj, object);

   lFreeElem(old_obj);

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
int sub_command 
) {
   int ret;
   DENTER(TOP_LAYER, "schedd_mod");

   ret = sconf_validate_config_(alpp)?0:1;
   
   DEXIT;
   return 0;
}

