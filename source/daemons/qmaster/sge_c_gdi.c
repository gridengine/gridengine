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
#include "sge_all_listsL.h"
#include "cull.h"
#include "sge.h"
#include "sge_follow.h"
#include "sge_c_gdi.h"
#include "sge_host.h"
#include "sge_host_qmaster.h"
#include "sge_job.h"
#include "sge_queue_qmaster.h"
#include "sge_userset_qmaster.h"
#include "sge_calendar_qmaster.h"
#include "sge_manop.h"
#include "complex_qmaster.h"
#include "sge_pe_qmaster.h"
#include "sge_conf.h"
#include "configuration_qmaster.h"
#include "sge_m_event.h"
#include "sched_conf_qmaster.h"
#include "sge_userprj_qmaster.h"
#include "sge_ckpt_qmaster.h"
#include "sge_usermap_qmaster.h"
#include "sge_hostgroup_qmaster.h"
#include "sge_sharetree_qmaster.h"
#include "sge_feature.h"
#include "sge_qmod_qmaster.h"
#include "read_write_queue.h"
#include "sec.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "qmaster.h"
#include "resolve_host.h"
#include "sge_user_mapping.h"
#include "msg_utilib.h"
#include "msg_common.h"
#include "msg_qmaster.h"
#include "msg_gdilib.h"
#include "sge_time.h"  
#include "version.h"  
#include "sge_security.h"  
#include "sge_answer.h"

#ifdef QIDL
#include "qidl_c_gdi.h"
#endif

extern lList *Master_User_List;
extern lList *Master_Userset_List;
extern lList *Master_Project_List;
extern lList *Master_Sharetree_List;
extern lList *Master_Config_List;
extern lList *Master_Sched_Config_List;
extern lList *Master_Complex_List;
extern lList *Master_Job_List;
extern lList *Master_Queue_List;
extern lList *Master_Submithost_List;
extern lList *Master_Adminhost_List;
extern lList *Master_Exechost_List;
extern lList *Master_Ckpt_List;
extern lList *Master_Pe_List;
extern lList *Master_Manager_List;
extern lList *Master_Operator_List;
extern lList *Master_Calendar_List;
extern lList *Master_Job_Schedd_Info_List;
extern lList *Master_Zombie_List;

#ifndef __SGE_NO_USERMAPPING__
extern lList *Master_Usermapping_Entry_List;
extern lList *Master_Host_Group_List;
#endif

static void sge_c_gdi_get(gdi_object_t *ao, char *host, sge_gdi_request *request, sge_gdi_request *answer, int *before, int *after);
static void sge_c_gdi_add(gdi_object_t *ao, char *host, sge_gdi_request *request, sge_gdi_request *answer, int return_list_flag);
static void sge_c_gdi_del(char *host, sge_gdi_request *request, sge_gdi_request *answer, int sub_command);
static void sge_c_gdi_mod(gdi_object_t *ao, char *host, sge_gdi_request *request, sge_gdi_request *answer, int sub_command);
static void sge_c_gdi_copy(gdi_object_t *ao, char *host, sge_gdi_request *request, sge_gdi_request *answer, int sub_command);

static void sge_c_gdi_permcheck(char *host, sge_gdi_request *request, sge_gdi_request *answer);
static void sge_gdi_do_permcheck(char *host, sge_gdi_request *request, sge_gdi_request *answer);
static void sge_c_gdi_trigger(char *host, sge_gdi_request *request, sge_gdi_request *answer);

static int sge_chck_mod_perm_user(lList **alpp, u_long32 target, char *user);
static int sge_chck_mod_perm_host(lList **alpp, u_long32 target, char *host, char *commproc, int mod, lListElem *ep);
static int sge_chck_get_perm_host(lList **alpp, sge_gdi_request *request);


/* ------------------------------ generic gdi objects --------------------- */
/* *INDENT-OFF* */
static gdi_object_t gdi_object[] = {
   { SGE_CALENDAR_LIST,     CAL_name,         CAL_Type, "calendar",                &Master_Calendar_List,          calendar_mod, calendar_spool, calendar_update_queue_states },
   { SGE_EVENT_LIST,        0,                NULL,     "event",                   &EV_Clients,                    NULL,         NULL,           NULL },
   { SGE_ADMINHOST_LIST,    AH_name,          AH_Type,  "adminhost",               &Master_Adminhost_List,         host_mod,     host_spool,     host_success },
   { SGE_SUBMITHOST_LIST,   SH_name,          SH_Type,  "submithost",              &Master_Submithost_List,        host_mod,     host_spool,     host_success },
   { SGE_EXECHOST_LIST,     EH_name,          EH_Type,  "exechost",                &Master_Exechost_List,          host_mod,     host_spool,     host_success },
   { SGE_QUEUE_LIST,        0,                NULL,     "queue",                   &Master_Queue_List,             NULL,         NULL,           NULL },
   { SGE_JOB_LIST,          0,                NULL,     "job",                     &Master_Job_List,               NULL,         NULL,           NULL },
   { SGE_COMPLEX_LIST,      CX_name,          CX_Type,  "complex",                 &Master_Complex_List,           complex_mod,  complex_spool,  complex_success },
   { SGE_ORDER_LIST,        0,                NULL,     "order",                   NULL,                           NULL,         NULL,           NULL },
   { SGE_MASTER_EVENT,      0,                NULL,     "master event",            NULL,                           NULL,         NULL,           NULL },
   { SGE_MANAGER_LIST,      0,                NULL,     "manager",                 &Master_Manager_List,           NULL,         NULL,           NULL },
   { SGE_OPERATOR_LIST,     0,                NULL,     "operator",                &Master_Operator_List,          NULL,         NULL,           NULL },
   { SGE_PE_LIST,           PE_name,          PE_Type,  "parallel environment",    &Master_Pe_List,                pe_mod,       pe_spool,       pe_success },
   { SGE_CONFIG_LIST,       0,                NULL,     "configuration",           &Master_Config_List,            NULL,         NULL,           NULL },
   { SGE_SC_LIST,           0,                NULL,     "scheduler configuration", &Master_Sched_Config_List,      NULL,         NULL,           NULL },
   { SGE_USER_LIST,         UP_name,          UP_Type,  "user",                    &Master_User_List,              userprj_mod,  userprj_spool,  userprj_success },
   { SGE_USERSET_LIST,      0,                NULL,     "userset",                 &Master_Userset_List,           NULL,         NULL,           NULL },
   { SGE_PROJECT_LIST,      UP_name,          UP_Type,  "project",                 &Master_Project_List,           userprj_mod,  userprj_spool,  userprj_success },
   { SGE_SHARETREE_LIST,    0,                NULL,     "sharetree",               &Master_Sharetree_List,         NULL,         NULL,           NULL },
   { SGE_CKPT_LIST,         CK_name,          CK_Type,  "checkpoint interface",    &Master_Ckpt_List,              ckpt_mod,     ckpt_spool,     ckpt_success },
   { SGE_JOB_SCHEDD_INFO,   0,                NULL,     "schedd info",             &Master_Job_Schedd_Info_List,   NULL,         NULL,           NULL },
   { SGE_ZOMBIE_LIST,       0,                NULL,     "job zombie list",         &Master_Zombie_List,            NULL,         NULL,           NULL },
#ifndef __SGE_NO_USERMAPPING__
   { SGE_USER_MAPPING_LIST, UME_cluster_user, UME_Type, "user mapping entry",      &Master_Usermapping_Entry_List, usermap_mod,  usermap_spool,  usermap_success },
   { SGE_HOST_GROUP_LIST,   GRP_group_name,   GRP_Type, "host group",              &Master_Host_Group_List,        hostgrp_mod,  hostgrp_spool,  hostgrp_success },
#endif
   { SGE_DUMMY_LIST,        0,                NULL,     "general request",         NULL,                           NULL,         NULL,           NULL },
   { 0,                     0,                NULL,     NULL,                      NULL,                           NULL,         NULL,           NULL }
};
/* *INDENT-ON* */

int verify_request_version(
lList **alpp,
u_long32 version,
char *host,
char *commproc,
int id 
) {
   char *client_version = NULL;
   
   /* actual GDI version is defined in
    *   libs/gdi/sge_gdi_intern.h
    */   
   struct vdict_t {
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

   if (version == GRM_GDI_VERSION) {
      DEXIT;
      return 0;
   }

   for (vp = &vdict[0]; vp->version; vp++)
      if (version == vp->version)
         client_version = vp->release;

   if (client_version)
      WARNING((SGE_EVENT, MSG_GDI_WRONG_GDI_SSISS,
         host, commproc, id, client_version, feature_get_product_name(FS_VERSION)));
   else
      WARNING((SGE_EVENT, MSG_GDI_WRONG_GDI_SSIUS,
         host, commproc, id, u32c(version), feature_get_product_name(FS_VERSION)));
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
      sprintf(SGE_EVENT, MSG_SGETEXT_UNKNOWNOP);
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

   DEXIT;
   return;
}


/* ------------------------------------------------------------ */

static void sge_c_gdi_get(
gdi_object_t *ao,
char *host,
sge_gdi_request *request,
sge_gdi_request *answer,
int *before,
int *after 
) {
   lList *lp = NULL;
   lListElem *qep = NULL;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];

   DENTER(GDI_LAYER, "sge_c_gdi_get");

   if (sge_chck_get_perm_host(&(answer->alp), request )) {
      DEXIT;
      return;
   }

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   /* ensure sge objects are licensed */

   switch (request->target) {
   case SGE_USER_LIST:
   case SGE_PROJECT_LIST:
   case SGE_SHARETREE_LIST:
      if (!feature_is_enabled(FEATURE_SGEEE)) {
         sprintf(SGE_EVENT,MSG_SGETEXT_FEATURE_NOT_AVAILABLE_FORX_S , feature_get_product_name(FS_SHORT));
         answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
         DEXIT;
         return;
      }
   }

   switch (request->target) {
   case SGE_QUEUE_LIST:
      qep = cull_read_in_qconf(NULL, NULL, 0, 0, NULL, NULL);
      if (!qep) {
         /* could not get queue template */
         ERROR((SGE_EVENT, MSG_OBJ_GENQ));
         answer_list_add(&(answer->alp), SGE_EVENT, STATUS_EDISK, ANSWER_QUALITY_ERROR);
         DEXIT;
         return;
      }

      lp = Master_Queue_List;

      if (!lp)
         lp = lCreateList("QueueList", QU_Type);

      if (lp && qep)
         lAppendElem(lp, qep);
      else {
         ERROR((SGE_EVENT, MSG_SGETEXT_CANTGET_SS, user, MSG_OBJ_QLIST));
         answer_list_add(&(answer->alp), SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      }         
      break;
#ifdef QHOST_TEST
   case SGE_QHOST:
/* lWriteListTo(request->lp, stdout); */
      sprintf(SGE_EVENT, "SGE_QHOST\n");
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
      DEXIT;
      return;
#endif

   default:
      if (!ao) {
         sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET);
         answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
         DEXIT;
         return;
      }
      lp = *(ao->master_list);
   }

   *before = lGetNumberOfElem(lp);

   answer->lp = lSelect("sge_c_gdi_get(answer)", 
            lp, request->cp, request->enp);

   *after = lGetNumberOfElem(answer->lp);

   /* remove template queue */
   if (request->target == SGE_QUEUE_LIST) {
      lDelElemStr(&lp, QU_qname, SGE_TEMPLATE_NAME);
      Master_Queue_List = lp;
   }   
         
   sprintf(SGE_EVENT, MSG_GDI_OKNL);
   answer_list_add(&(answer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   
   DEXIT;
   return;
}

/* ------------------------------------------------------------ */

static void sge_c_gdi_add(
gdi_object_t *ao,
char *host,
sge_gdi_request *request,
sge_gdi_request *answer,
int sub_command 
) {
   int ret;
   lListElem *ep;
   lList *ticket_orders = NULL;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];
   extern int deactivate_ptf;

   DENTER(TOP_LAYER, "sge_c_gdi_add");

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (!request->host || !user || !request->commproc) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   for_each (ep, request->lp) {

      /* check permissions of host and user */
      if (sge_chck_mod_perm_user(&(answer->alp), request->target, user))
         continue;
      if (sge_chck_mod_perm_host(&(answer->alp), request->target, request->host, request->commproc, 0, NULL))
         continue;

      /* ensure sge objects are licensed */
      switch (request->target) {
      case SGE_USER_LIST:
      case SGE_PROJECT_LIST:
      case SGE_SHARETREE_LIST:
         if (!feature_is_enabled(FEATURE_SGEEE)) {
            sprintf(SGE_EVENT,MSG_SGETEXT_FEATURE_NOT_AVAILABLE_FORX_S , feature_get_product_name(FS_SHORT) );
            answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
            DEXIT;
            return;
         }
      }

      /* add each element */
      switch (request->target) {
      case SGE_EVENT_LIST:

         /* fill address infos from request */
         /* into event client that must be added */
         lSetHost(ep, EV_host, request->host);
         lSetString(ep, EV_commproc, request->commproc);
         lSetUlong(ep, EV_commid, request->id);

         /* fill in authentication infos from request */
         lSetUlong(ep, EV_uid, uid);

         sge_add_event_client(ep,&(answer->alp), 
                              (sub_command & SGE_GDI_RETURN_NEW_VERSION) ? &(answer->lp) : NULL,
                              user,host);
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

               if (ret==-2)
                  reinit_event_client(EV_ID_SCHEDD);
            }
         }
         break;
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

      case SGE_QUEUE_LIST:
         sge_gdi_add_mod_queue(ep, &(answer->alp), user, host, 1, sub_command);
         break;

      case SGE_MANAGER_LIST:
      case SGE_OPERATOR_LIST:
         sge_add_manop(ep, &(answer->alp), user, host, 
            request->target);
         break;

      case SGE_USERSET_LIST:
         sge_add_userset(ep, &(answer->alp), &Master_Userset_List, 
            user, host);
         break;

      case SGE_SHARETREE_LIST:
         sge_add_sharetree(ep, &Master_Sharetree_List, &(answer->alp), 
            user, host);
         break;

      default:
         if (!ao) {
            sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET);
            answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
            break;
         } 
         if (request->target==SGE_EXECHOST_LIST && 
               !strcmp(prognames[EXECD], request->commproc)) {
               sge_execd_startedup(ep, &(answer->alp), user, host, request->target);
         } else {
            sge_gdi_add_mod_generic(&(answer->alp), ep, 1, ao, 
               user, host, sub_command);
         }
         break;
      }
   }

   if (ticket_orders) {

      if (!deactivate_ptf) {
         /* send all ticket orders to the exec hosts */
         distribute_ticket_orders(ticket_orders);
      } else {
         /* tickets not needed at execd's if no repriorization is done */
         DPRINTF(("NO TICKET DELIVERY\n"));
      }
      ticket_orders = lFreeList(ticket_orders);
   }

   DEXIT;
   return;
}

/* ------------------------------------------------------------ */

static void sge_c_gdi_del(
char *host,
sge_gdi_request *request,
sge_gdi_request *answer,
int sub_command 
) {
   lListElem *ep;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];

   DENTER(GDI_LAYER, "sge_c_gdi_del");

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (!request->lp) {  /* deletion of a whole list */
      /* check permissions of host and user */
      if (sge_chck_mod_perm_user(&(answer->alp), request->target, 
                                 user)) {
         DEXIT;
         return;
      }
      if (sge_chck_mod_perm_host(&(answer->alp), request->target, 
          request->host, request->commproc, 0, NULL)) {
         DEXIT;
         return;
      }
      switch (request->target) {
      case SGE_SHARETREE_LIST:
         sge_del_sharetree(&Master_Sharetree_List, &(answer->alp), user,host);
         break;
      default:
         sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET);
         answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
         break;
      }
   }
   else {
      for_each (ep, request->lp) {
         /* check permissions of host and user */
         if (sge_chck_mod_perm_user(&(answer->alp), request->target, 
                                    user))
            continue;
         if (sge_chck_mod_perm_host(&(answer->alp), request->target, 
                                    request->host, request->commproc, 0, NULL))
            continue;

         /* ensure sge objects are licensed */
         switch (request->target) {
         case SGE_USER_LIST:
         case SGE_PROJECT_LIST:
         case SGE_SHARETREE_LIST:
            if (!feature_is_enabled(FEATURE_SGEEE)) {
               sprintf(SGE_EVENT, MSG_SGETEXT_FEATURE_NOT_AVAILABLE_FORX_S , feature_get_product_name(FS_SHORT));
               answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
               DEXIT;
               return;
            }
         }

         /* try to remove the element */
         switch (request->target) {
         case SGE_ADMINHOST_LIST:
         case SGE_SUBMITHOST_LIST:
         case SGE_EXECHOST_LIST:
            sge_del_host(ep, &(answer->alp), user, host, request->target);
            break;

         case SGE_QUEUE_LIST:
            sge_gdi_delete_queue(ep, &(answer->alp), user, host);
            break;

         case SGE_JOB_LIST:
            sge_gdi_del_job(ep, &(answer->alp), user, host, sub_command);
            break;

         case SGE_COMPLEX_LIST:
            sge_del_cmplx(ep, &(answer->alp), user, host); 
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
            sge_del_userprj(ep, &(answer->alp), &Master_User_List, user, host, 
                            1);
            break;

         case SGE_USERSET_LIST:
            sge_del_userset(ep, &(answer->alp), &Master_Userset_List, user, 
                            host);
            break;

         case SGE_PROJECT_LIST:
            sge_del_userprj(ep, &(answer->alp), &Master_Project_List, user, 
                            host, 0);
            break;

         case SGE_CKPT_LIST:
            sge_del_ckpt(ep, &(answer->alp), user, host); 
            break;

         case SGE_CALENDAR_LIST:
            sge_del_calendar(ep, &(answer->alp), user, host);
            break;
#ifndef __SGE_NO_USERMAPPING__
         case SGE_USER_MAPPING_LIST:
            sge_del_usermap(ep, &(answer->alp), user, host);
            break;

         case SGE_HOST_GROUP_LIST:
            sge_del_hostgrp(ep, &(answer->alp), user, host);
            break;
#endif /* __SGE_NO_USERMAPPING__ */                           
         default:
            sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET);
            answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
            break;
         }
      }
   }

   DEXIT;
   return;
}

/* ------------------------------------------------------------ */

static void sge_c_gdi_copy(
gdi_object_t *ao,
char *host,
sge_gdi_request *request,
sge_gdi_request *answer,
int sub_command 
) {
   lListElem *ep;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];


   DENTER(TOP_LAYER, "sge_c_gdi_copy");

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (!request->host || !user || !request->commproc) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   for_each (ep, request->lp) {

      /* check permissions of host and user */
      if (sge_chck_mod_perm_user(&(answer->alp), request->target, user))
         continue;
      if (sge_chck_mod_perm_host(&(answer->alp), request->target, request->host, request->commproc, 0, NULL))
         continue;

      /* add each element */
      switch (request->target) {
      case SGE_JOB_LIST:
            sge_gdi_copy_job(ep, &(answer->alp), 
                             (sub_command & SGE_GDI_RETURN_NEW_VERSION) ? &(answer->lp) : NULL, 
                             user, host, request);
         break;

      default:
         sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET);
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

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   DPRINTF(("User: %s\n", user ));
 
   if (answer->lp == NULL)
   { 
      char* mappingName = NULL;
      const char* requestedHost = NULL;

      lUlong value;
      /* create PERM_Type list for answer structure*/
      lp = lCreateList("permissions", PERM_Type);
      ep = lCreateElem(PERM_Type);
      lAppendElem(lp,ep);

      /* set sge username */ 
      lSetString(ep, PERM_sge_username, user );
 
      /* set requested host name */
      if (request->lp == NULL) {
         requestedHost = host;
      } else {
         lList*     tmp_lp = NULL;
         lListElem* tmp_ep = NULL;
     
         tmp_lp = request->lp;
         tmp_ep = tmp_lp->first;
         requestedHost = lGetHost(tmp_ep, PERM_req_host);
#ifndef __SGE_NO_USERMAPPING__
         mappingName =  sge_malloc_map_out_going_username( Master_Host_Group_List,
                                                         Master_Usermapping_Entry_List,
                                                         user , 
                                                         requestedHost );
#endif
    }

    if (requestedHost != NULL) {
       lSetHost(ep, PERM_req_host, requestedHost );  
    }   

    if (mappingName != NULL) {
      DPRINTF(("execution mapping: user %s mapped to %s on host %s\n",user ,mappingName, requestedHost));
      lSetString(ep, PERM_req_username, mappingName);
      free(mappingName);
      mappingName = NULL; 
    } else { 
      lSetString(ep, PERM_req_username, "");
    }
    

    /* check for manager permission */
    value = 0;
    if (lGetElemStr(Master_Manager_List, MO_name, user ) != NULL ) {
       value = 1; 
    }   
    lSetUlong(ep, PERM_manager, value);

    /* check for operator permission */
    value = 0;
    if (lGetElemStr(Master_Operator_List, MO_name, user ) != NULL ) {
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

/* ------------------------------------------------------------ */
static void sge_c_gdi_permcheck(
char *host,
sge_gdi_request *request,
sge_gdi_request *answer 
) {
  DENTER(GDI_LAYER, "sge_c_gdi_permcheck");
  switch (request->target) {
  case SGE_DUMMY_LIST:
    sge_gdi_do_permcheck(host,request,answer);
    break;
  default:
    WARNING((SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
    answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR); 
    break;
  }
  DEXIT;
  return;
}


/* ------------------------------------------------------------ */

static void sge_c_gdi_trigger(
char *host,
sge_gdi_request *request,
sge_gdi_request *answer 
) {
   DENTER(GDI_LAYER, "sge_c_gdi_trigger");

   switch (request->target) {
   case SGE_EXECHOST_LIST: /* kill execd */
   case SGE_MASTER_EVENT: /* kill master */
   case SGE_EVENT_LIST: /* kill scheduler */
   case SGE_SC_LIST: /* trigger scheduler monitoring */
      if ( !sge_locate_host(host, SGE_ADMINHOST_LIST)) {
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

   switch (request->target) {
   case SGE_QUEUE_LIST: /* do no break here! */
   case SGE_JOB_LIST:  /* en/dis/able un/suspend clean/clear job/queue */
      sge_gdi_qmod(host, request, answer);
      break;
   case SGE_EXECHOST_LIST: /* kill execd */
      sge_gdi_kill_exechost(host, request, answer);
      break;
   case SGE_MASTER_EVENT: /* kill master */
      sge_gdi_kill_master(host, request, answer);
      break;
   case SGE_EVENT_LIST: /* kill schedler */
      sge_gdi_kill_eventclient(host, request, answer);
      break;
   case SGE_SC_LIST: /* trigger scheduler monitoring */
      sge_gdi_tsm(host, request, answer);
      break;
   default:
      WARNING((SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
      break;
   }
   DEXIT;
   return;
}

static void sge_c_gdi_mod(
gdi_object_t *ao,
char *host,
sge_gdi_request *request,
sge_gdi_request *answer,
int sub_command 
) {
   lListElem *ep;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];

   DENTER(TOP_LAYER, "sge_c_gdi_mod");

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   for_each (ep, request->lp) {

      if (sge_chck_mod_perm_user(&(answer->alp), request->target, 
            user)) {
         DTRACE;
         continue;
      }
      if (sge_chck_mod_perm_host(&(answer->alp), request->target, 
            request->host, request->commproc, 1, ep)) {
         DTRACE;
         continue;
      }

      /* ensure sge objects are licensed */
      switch (request->target) {
      case SGE_USER_LIST:
      case SGE_PROJECT_LIST:
      case SGE_SHARETREE_LIST:
         if (!feature_is_enabled(FEATURE_SGEEE)) {
            sprintf(SGE_EVENT, MSG_SGETEXT_FEATURE_NOT_AVAILABLE_FORX_S , feature_get_product_name(FS_SHORT) );
            answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
            DEXIT;
            return;
         }
      }

      /* try to mod the element */
      switch (request->target) {

      case SGE_QUEUE_LIST:
         sge_gdi_add_mod_queue(ep, &(answer->alp), user, host, 0, 
            sub_command);
         break;

      case SGE_JOB_LIST:
         sge_gdi_mod_job(ep, &(answer->alp), user, host, sub_command);
         break;

      case SGE_CONFIG_LIST:
         sge_mod_configuration(ep, &(answer->alp), user, host);
         break;

      case SGE_SC_LIST:
         sge_mod_sched_configuration(ep, &Master_Sched_Config_List, 
            &(answer->alp), user, host);
         break;

      case SGE_USERSET_LIST:
         sge_mod_userset(ep, &(answer->alp), &Master_Userset_List, 
            user, host);
         break;

      case SGE_SHARETREE_LIST:
         sge_mod_sharetree(ep, &Master_Sharetree_List, &(answer->alp), 
            user, host);
         break;

      case SGE_EVENT_LIST:
 
         /* fill address infos from request */
         /* into event client that must be added */
         lSetHost(ep, EV_host, request->host);
         lSetString(ep, EV_commproc, request->commproc);
         lSetUlong(ep, EV_commid, request->id);
 
         /* fill in authentication infos from request */
         lSetUlong(ep, EV_uid, uid);
 
         sge_mod_event_client(ep,&(answer->alp), NULL, user,host);
         break;

#if 0
      case SGE_CKPT_LIST:
         sge_mod_ckpt(ep, &(answer->alp), user, host);
         break;
#endif

      default:
         if (!ao) {
            sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET);
            answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
            break;
         } 
         sge_gdi_add_mod_generic(&(answer->alp), ep, 0, ao, user, host,
            sub_command);
         break;
      }
   }

   

   DEXIT;
   return;
}

static int sge_chck_mod_perm_user(
lList **alpp,
u_long32 target,
char *user 
) {

   DENTER(TOP_LAYER, "sge_chck_mod_perm_user");

   /* check permissions of user */
   switch (target) {

   case SGE_ORDER_LIST:
   case SGE_ADMINHOST_LIST:
   case SGE_SUBMITHOST_LIST:
   case SGE_EXECHOST_LIST:
   case SGE_QUEUE_LIST:
   case SGE_COMPLEX_LIST:
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
   case SGE_HOST_GROUP_LIST:
   case SGE_FEATURESET_LIST:
      /* user must be a manager */
      if (sge_manager(user)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_MUSTBEMANAGER_S, user));
         answer_list_add(alpp, SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
         DEXIT;
         return 1;
      }
      break;

   case SGE_USERSET_LIST:
      /* user must be a operator */
      if (sge_operator(user)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_MUSTBEOPERATOR_S, user));
         answer_list_add(alpp, SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
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
      sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET);
      answer_list_add(alpp, SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
      DEXIT;
      return 1;
   }

   DEXIT;
   return 0;
}


static int sge_chck_mod_perm_host(
lList **alpp,
u_long32 target,
char *host,
char *commproc,
int mod,
lListElem *ep 
) {

   DENTER(TOP_LAYER, "sge_chck_mod_perm_host");

   /* check permissions of host */
   switch (target) {

   case SGE_ORDER_LIST:
   case SGE_ADMINHOST_LIST:
   case SGE_OPERATOR_LIST:
   case SGE_MANAGER_LIST:
   case SGE_SUBMITHOST_LIST:
   case SGE_QUEUE_LIST:
   case SGE_COMPLEX_LIST:
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
   case SGE_HOST_GROUP_LIST:
   case SGE_FEATURESET_LIST:
      
      /* host must be SGE_ADMINHOST_LIST */
      if ( !sge_locate_host(host, SGE_ADMINHOST_LIST)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_NOADMINHOST_S, host));
         answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
         DEXIT;
         return 1;
      }
      break;

   case SGE_EXECHOST_LIST:
      
      /* host must be either admin host or exec host and execd */

      if (!(sge_locate_host(host, SGE_ADMINHOST_LIST) ||
         (sge_locate_host(host, SGE_EXECHOST_LIST) && !strcmp(commproc, prognames[EXECD])))) {
         ERROR((SGE_EVENT, MSG_SGETEXT_NOADMINHOST_S, host));
         answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
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
         if ( !sge_locate_host(host, SGE_ADMINHOST_LIST)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_NOADMINHOST_S, host));
            answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return 1;
         }
         break;
      }    
#endif      
      /* host must be SGE_SUBMITHOST_LIST */
      if ( !sge_locate_host(host, SGE_SUBMITHOST_LIST)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_NOSUBMITHOST_S, host));
         answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
         DEXIT;
         return 1;
      }
      break;

   case SGE_EVENT_LIST:
      /* to start an event client or if an event client
         performs modify requests on itself
         it must be on a submit or an admin host 
       */
      if ( (!sge_locate_host(host, SGE_SUBMITHOST_LIST)) 
        && (!sge_locate_host(host, SGE_ADMINHOST_LIST))) {
        ERROR((SGE_EVENT, MSG_SGETEXT_NOSUBMITORADMINHOST_S, host));
        answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
        DEXIT;
        return 1;
      }
      break;
   default:
      sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET);
      answer_list_add(alpp, SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
      DEXIT;
      return 1;
   }

   DEXIT;
   return 0;
}

static int sge_chck_get_perm_host(
lList **alpp,
sge_gdi_request *request
) {
   u_long32 target;
   char *host     = NULL;
   char *commproc = NULL;
   static int last_id = -1; 
   
   DENTER(TOP_LAYER, "sge_chck_get_perm_host");

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
   case SGE_QUEUE_LIST:
   case SGE_COMPLEX_LIST:
   case SGE_PE_LIST:
   case SGE_SC_LIST:
   case SGE_USER_LIST:
   case SGE_USERSET_LIST:
   case SGE_PROJECT_LIST:
   case SGE_SHARETREE_LIST:
   case SGE_CKPT_LIST:
   case SGE_CALENDAR_LIST:
   case SGE_USER_MAPPING_LIST:
   case SGE_HOST_GROUP_LIST:
   case SGE_FEATURESET_LIST:
   case SGE_EXECHOST_LIST:
   case SGE_JOB_LIST:
   case SGE_ZOMBIE_LIST:
   case SGE_JOB_SCHEDD_INFO:
      
      /* host must be admin or submit host */
      if ( !sge_locate_host(host, SGE_ADMINHOST_LIST) &&
           !sge_locate_host(host, SGE_SUBMITHOST_LIST)) {

         if (last_id != request->id) {     /* only log the first error
                                              in an api multi request */
            ERROR((SGE_EVENT, MSG_SGETEXT_NOSUBMITORADMINHOST_S, host));
         } else {    
            sprintf(SGE_EVENT, MSG_SGETEXT_NOSUBMITORADMINHOST_S, host);
         }
         answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
         last_id = request->id;       /* this indicates that the error
                                         is allready locked */
         DEXIT;
         return 1;
      }
      break;

   case SGE_CONFIG_LIST:
      /* host must be admin or submit host or exec host */
      if ( !sge_locate_host(host, SGE_ADMINHOST_LIST) &&
           !sge_locate_host(host, SGE_SUBMITHOST_LIST) &&
           !sge_locate_host(host, SGE_EXECHOST_LIST)) {
         if (last_id != request->id) {  /* only log the first error
                                              in an api multi request */
            ERROR((SGE_EVENT, MSG_SGETEXT_NOSUBMITORADMINHOST_S, host));
         } else {
            sprintf(SGE_EVENT, MSG_SGETEXT_NOSUBMITORADMINHOST_S, host);
         }
         answer_list_add(alpp, SGE_EVENT, STATUS_EDENIED2HOST, ANSWER_QUALITY_ERROR);
         last_id = request->id;       /* this indicates that the error
                                         is allready locked */
         DEXIT;
         return 1;
      }
      break;

   default:
      sprintf(SGE_EVENT, MSG_SGETEXT_OPNOIMPFORTARGET);
      answer_list_add(alpp, SGE_EVENT, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
      DEXIT;
      return 1;
   }

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
char *ruser,
char *rhost,
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
   if (lGetPosViaElem(instructions, object->key_nm)<0) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS ,
            lNm2Str(object->key_nm), SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* resolve host name in case of objects with hostnames as key 
      before searching for the objects */
   if ((object->key_nm == EH_name||
          object->key_nm == AH_name||
          object->key_nm == SH_name) && 
          sge_resolve_host(instructions, object->key_nm)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, 
            lGetHost(instructions, object->key_nm)));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   pos = lGetPosViaElem(instructions,  object->key_nm );
   dataType = lGetPosType(lGetElemDescr(instructions),pos);
   if (dataType == lHostT) {
      name = lGetHost(instructions, object->key_nm); 
      old_obj = lGetElemHost(*(object->master_list), object->key_nm, name); 
   } else {
      name = lGetString(instructions, object->key_nm); 
      old_obj = lGetElemStr(*(object->master_list), object->key_nm, name);
   }

   

   /* special search for host types */
/*   if (object->key_nm == EH_name ||
       object->key_nm == AH_name||
       object->key_nm == SH_name) {
       old_obj = lGetElemHost(*(object->master_list), object->key_nm, name); 
   } else { 
      old_obj = lGetElemStr(*(object->master_list), object->key_nm, name);
   }

   This code was removed because the information is here via lHostT data type
*/

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

   /* chain out the old object */
   if (old_obj) {
      lDechainElem(*(object->master_list), old_obj);
   }

   /* ensure our global list exists */ 
   if (!*(object->master_list)) {
      *(object->master_list) = lCreateList(object->object_name, object->type);
   }

   /* chain in new object */
   lAppendElem(*(object->master_list), new_obj);

#ifdef QIDL
   if (add) /* this assumes that all generic object are identified by name */
      addObjectByName(object->target,lGetString(new_obj,object->key_nm));
#endif
   
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

gdi_object_t *get_gdi_object(
u_long32 target 
) {
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

