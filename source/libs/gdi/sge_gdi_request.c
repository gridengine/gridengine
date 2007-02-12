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

#ifndef WIN32NATIVE
#	include <unistd.h>
#endif
#include <stdlib.h>

#include "basis_types.h"
#include "sge_stdlib.h"
#include "commlib.h"
#include "sge_gdiP.h"
#include "sge_gdi_request.h"
#include "sge_multiL.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sge_uidgid.h"
#include "sge_profiling.h"
#include "qm_name.h"
#include "sge_unistd.h"
#include "sge_hostname.h"
#include "sge_answer.h"
#ifdef KERBEROS
#  include "krb_lib.h"
#endif
#include "msg_common.h"
#include "msg_gdilib.h"
#include "gdi/version.h"

static bool
sge_pack_gdi_info(u_long32 command);

static bool 
gdi_request_map_pack_error(int pack_ret, lList **answer_list);

static const char *target2string(u_long32 target);


/****** gdi/request/sge_gdi_extract_answer() **********************************
*  NAME
*     sge_gdi_extract_answer() -- exctact answers of a multi request.
*
*  SYNOPSIS
*     lList* sge_gdi_extract_answer(u_long32 cmd, u_long32 target, 
*                                   int id, lList* mal, lList** olpp) 
*
*  FUNCTION
*     This function extracts the answer for each invidual request on 
*     previous sge_gdi_multi() calls. 
*
*  INPUTS
*     lList** alpp    - List for answers if an error occurs in
*                       sge_gdi_extract_answer
*                       This list gets allocated by GDI. The caller is responsible 
*                       for freeing. A response is a list element containing a field 
*                       with a status value (AN_status). The value STATUS_OK is used 
*                       in case of success. STATUS_OK and other values are defined in 
*                       sge_answerL.h. the second field (AN_text) in a response list 
*                       element is a string that describes the performed operation or 
*                       a description of an error.
*     u_long32 cmd    - bitmask which decribes the operation 
*        (see sge_gdi_multi)
*
*     u_long32 target - unique id to identify masters list
*        (see sge_gdi_multi) 
*
*     int id          - unique id returned by a previous
*        sge_gdi_multi() call. 
*
*     lList* mal      - List of answer/response lists returned from
*        sge_gdi_multi(mode=SGE_GDI_SEND)
*
*     lList** olpp    - This parameter is used to get a list in case 
*           of SGE_GDI_GET command. The caller is responsible for 
*           freeing by using lFreeList(). 
*
*  RESULT
*     true   in case of success
*     false  in case of error
*
*  NOTES
*     MT-NOTE: sge_gdi_extract_answer() is MT safe
******************************************************************************/
bool sge_gdi_extract_answer(lList **alpp, u_long32 cmd, u_long32 target, int id,
                              lList *mal, lList **olpp) 
{
   lListElem *map = NULL;
   int operation, sub_command;

   DENTER(GDI_LAYER, "sge_gdi_extract_answer");

   operation = SGE_GDI_GET_OPERATION(cmd); 
   sub_command = SGE_GDI_GET_SUBCOMMAND(cmd);

   if (!mal || id < 0) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      DRETURN(false);
   }
   
   map = lGetElemUlong(mal, MA_id, id);
   if (!map) {
      sprintf(SGE_EVENT, MSG_GDI_SGEGDIFAILED_S, target2string(target)); 
      SGE_ADD_MSG_ID(SGE_EVENT);
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      DRETURN(false);
   }

   if ((operation == SGE_GDI_GET) || (operation == SGE_GDI_PERMCHECK) ||
       (operation == SGE_GDI_ADD && sub_command == SGE_GDI_RETURN_NEW_VERSION )) {
      if (!olpp) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DRETURN(false);
      }
      lXchgList(map, MA_objects, olpp);
   }

   lXchgList(map, MA_answers, alpp);

   DRETURN(true);
}
  
/****** gdi/request/sge_unpack_gdi_request() **********************************
*  NAME
*     sge_unpack_gdi_request() -- unpacks an gdi_request structure 
*
*  SYNOPSIS
*     int sge_unpack_gdi_request(sge_pack_buffer *pb, 
*                                sge_gdi_request **arp) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     sge_pack_buffer *pb   - ??? 
*     sge_gdi_request **arp - ??? 
*
*  RESULT
*     int - 
*         0 on success
*        -1 not enough memory
*        -2 format error 
*
*  NOTES
*     MT-NOTE: sge_unpack_gdi_request() is MT safe
******************************************************************************/
int sge_unpack_gdi_request(sge_pack_buffer *pb, sge_gdi_request **arp) 
{
   int ret = PACK_SUCCESS;
   sge_gdi_request *ar = NULL;
   sge_gdi_request *prev_ar = NULL;
   u_long32 next;

   DENTER(GDI_LAYER, "sge_unpack_gdi_request");

   do {
      if (!(ar = new_gdi_request())) {
         ret = PACK_ENOMEM;
         goto error;
      }
      if ((ret=unpackint(pb, &(ar->op)) )) goto error;
      if ((ret=unpackint(pb, &(ar->target)) )) goto error;

      if ((ret=unpackint(pb, &(ar->version)) )) goto error;
      /* JG: TODO (322): At this point we should check the version! 
      **                 The existent check function verify_request_version
      **                 cannot be called as neccesary data structures are 
      **                 available here (e.g. answer list).
      **                 Better do these changes at a more general place 
      **                 together with (hopefully coming) further communication
      **                 redesign.
      */
      if ((ret=cull_unpack_list(pb, &(ar->lp)) )) goto error;
      if ((ret=cull_unpack_list(pb, &(ar->alp)) )) goto error;
      if ((ret=cull_unpack_cond(pb, &(ar->cp)) )) goto error;
      if ((ret=cull_unpack_enum(pb, &(ar->enp)) )) goto error;
      
      if ((ret=unpackstr(pb, &(ar->auth_info)) )) goto error;
      if ((ret=unpackint(pb, &(ar->sequence_id)) )) goto error;
      if ((ret=unpackint(pb, &(ar->request_id)) )) goto error;
      if ((ret=unpackint(pb, &next) )) goto error;

      switch (ar->op) {
      case SGE_GDI_GET:
         DPRINTF(("unpacking SGE_GDI_GET request\n"));
         break;
      case SGE_GDI_ADD:
      case SGE_GDI_ADD | SGE_GDI_RETURN_NEW_VERSION:
      case SGE_GDI_ADD | SGE_GDI_SET_ALL:
         DPRINTF(("unpacking SGE_GDI_ADD request\n"));
         break;
      case SGE_GDI_DEL:
      case SGE_GDI_DEL | SGE_GDI_ALL_JOBS:
      case SGE_GDI_DEL | SGE_GDI_ALL_USERS:
      case SGE_GDI_DEL | SGE_GDI_ALL_JOBS | SGE_GDI_ALL_USERS:
         DPRINTF(("unpacking SGE_GDI_DEL request\n"));
         break;
      case SGE_GDI_MOD:
      case SGE_GDI_MOD | SGE_GDI_ALL_JOBS:
      case SGE_GDI_MOD | SGE_GDI_ALL_USERS:
      case SGE_GDI_MOD | SGE_GDI_ALL_JOBS | SGE_GDI_ALL_USERS:
      case SGE_GDI_MOD | SGE_GDI_APPEND:
      case SGE_GDI_MOD | SGE_GDI_REMOVE:
      case SGE_GDI_MOD | SGE_GDI_CHANGE:
      case SGE_GDI_MOD | SGE_GDI_SET_ALL:
         DPRINTF(("unpacking SGE_GDI_MOD request\n"));
         break;
      case SGE_GDI_TRIGGER:
         DPRINTF(("unpacking SGE_GDI_TRIGGER request\n"));
         break;
      case SGE_GDI_PERMCHECK:
         DPRINTF(("unpacking SGE_GDI_PERMCHECK request\n"));
         break;
      case SGE_GDI_SPECIAL:
         DPRINTF(("unpacking special things\n"));
         break;
      case SGE_GDI_COPY:
         DPRINTF(("unpacking copy request\n"));
         break;
      case SGE_GDI_REPLACE:
      case SGE_GDI_REPLACE | SGE_GDI_SET_ALL:
         DPRINTF(("unpacking SGE_GDI_REPLACE request\n"));
         break;
      default:
         ERROR((SGE_EVENT, MSG_GDI_ERROR_INVALIDVALUEXFORARTOOP_D, sge_u32c(ar->op)));
         ret = PACK_BADARG;
         goto error;
      }

      if (!prev_ar) {
         *arp = prev_ar = ar;
      }
      else {
         prev_ar->next = ar;
         prev_ar = ar;
      }
   } while (next);
   
   DRETURN(ret);

error:
   ERROR((SGE_EVENT, MSG_GDI_CANTUNPACKGDIREQUEST ));
   *arp = free_gdi_request(*arp);
   ar = free_gdi_request(ar);

   DRETURN(ret);
}

static bool
sge_pack_gdi_info(u_long32 command) 
{
   bool ret = true;

   DENTER(GDI_LAYER, "sge_pack_gdi_info");
   switch (command) {
   case SGE_GDI_GET:
      DPRINTF(("packing SGE_GDI_GET request\n"));
      break;
   case SGE_GDI_ADD:
   case SGE_GDI_ADD | SGE_GDI_RETURN_NEW_VERSION:
   case SGE_GDI_ADD | SGE_GDI_SET_ALL:
      DPRINTF(("packing SGE_GDI_ADD request\n"));
      break;
   case SGE_GDI_DEL:
   case SGE_GDI_DEL | SGE_GDI_ALL_JOBS:
   case SGE_GDI_DEL | SGE_GDI_ALL_USERS:
   case SGE_GDI_DEL | SGE_GDI_ALL_JOBS | SGE_GDI_ALL_USERS:
      DPRINTF(("packing SGE_GDI_DEL request\n"));
      break;
   case SGE_GDI_MOD:

   case SGE_GDI_MOD | SGE_GDI_ALL_JOBS:
   case SGE_GDI_MOD | SGE_GDI_ALL_USERS:
   case SGE_GDI_MOD | SGE_GDI_ALL_JOBS | SGE_GDI_ALL_USERS:

   case SGE_GDI_MOD | SGE_GDI_APPEND:
   case SGE_GDI_MOD | SGE_GDI_REMOVE:
   case SGE_GDI_MOD | SGE_GDI_CHANGE:
   case SGE_GDI_MOD | SGE_GDI_SET_ALL:

      DPRINTF(("packing SGE_GDI_MOD request\n"));
      break;
   case SGE_GDI_TRIGGER:
      DPRINTF(("packing SGE_GDI_TRIGGER request\n"));
      break;
   case SGE_GDI_PERMCHECK:
      DPRINTF(("packing SGE_GDI_PERMCHECK request\n"));
      break;
   case SGE_GDI_SPECIAL:
      DPRINTF(("packing special things\n"));
      break;
   case SGE_GDI_COPY:
      DPRINTF(("request denied\n"));
      break;
   case SGE_GDI_REPLACE:
   case SGE_GDI_REPLACE | SGE_GDI_SET_ALL:
      DPRINTF(("packing SGE_GDI_REPLACE request\n"));
      break;
   default:
      ERROR((SGE_EVENT, MSG_GDI_ERROR_INVALIDVALUEXFORARTOOP_D, 
             sge_u32c(command)));
      ret = false;
   }

   DRETURN(ret);
}

static bool 
gdi_request_map_pack_error(int pack_ret, lList **answer_list)
{
   bool ret = true;

   DENTER(GDI_LAYER, "gdi_request_map_pack_error");
   switch (pack_ret) {
   case PACK_SUCCESS:
      break;
   case PACK_ENOMEM:
      answer_list_add_sprintf(answer_list, STATUS_ERROR2,
                              ANSWER_QUALITY_ERROR,
                   MSG_GDI_MEMORY_NOTENOUGHMEMORYFORPACKINGGDIREQUEST);
      break;
   case PACK_FORMAT:
      answer_list_add_sprintf(answer_list, STATUS_ERROR3,
                              ANSWER_QUALITY_ERROR,
                              MSG_GDI_REQUESTFORMATERROR);
      break;
   default:
      answer_list_add_sprintf(answer_list, STATUS_ERROR1,
                              ANSWER_QUALITY_ERROR,
                     MSG_GDI_UNEXPECTEDERRORWHILEPACKINGGDIREQUEST);
      break;
   }
   ret = (pack_ret == PACK_SUCCESS) ? true : false;

   DRETURN(ret);
}

bool
gdi_request_pack_prefix(sge_gdi_request *ar, lList **answer_list,
                        sge_pack_buffer *pb)
{
   bool ret = true;
   
   DENTER(GDI_LAYER, "gdi_request_pack_prefix");
   if (ar != NULL) {
      int pack_ret = PACK_SUCCESS;

      sge_pack_gdi_info(ar->op);
      
      pack_ret = packint(pb, ar->op);
      if (pack_ret != PACK_SUCCESS) {
         goto handle_error; 
      }
      pack_ret = packint(pb, ar->target);
      if (pack_ret != PACK_SUCCESS) {
         goto handle_error; 
      }
      pack_ret = packint(pb, ar->version);
      if (pack_ret != PACK_SUCCESS) {
         goto handle_error; 
      }
 handle_error:
      ret = gdi_request_map_pack_error(pack_ret, answer_list);
   }

   DRETURN(ret);
}

bool
gdi_request_pack_suffix(sge_gdi_request *ar, lList **answer_list,
                        sge_pack_buffer *pb)
{
   bool ret = true;
   
   DENTER(GDI_LAYER, "gdi_request_pack_suffix");
   if (ar != NULL) {
      int pack_ret = PACK_SUCCESS;

      pack_ret = cull_pack_list(pb, ar->alp);
      if (pack_ret != PACK_SUCCESS) {
         goto handle_error;
      }
      pack_ret = cull_pack_cond(pb, ar->cp);
      if (pack_ret != PACK_SUCCESS) {
         goto handle_error;
      }
      pack_ret = cull_pack_enum(pb, ar->enp);
      if (pack_ret != PACK_SUCCESS) {
         goto handle_error;
      }
      pack_ret = packstr(pb, ar->auth_info);
      if (pack_ret != PACK_SUCCESS) {
         goto handle_error;
      }
      pack_ret = packint(pb, ar->sequence_id);
      if (pack_ret != PACK_SUCCESS) {
         goto handle_error;
      }
      pack_ret = packint(pb, ar->request_id);
      if (pack_ret != PACK_SUCCESS) {
         goto handle_error;
      }
      pack_ret = packint(pb, ar->next ? 1 : 0);
      if (pack_ret != PACK_SUCCESS) {
         goto handle_error;
      }
 handle_error:
      ret = gdi_request_map_pack_error(pack_ret, answer_list);
   }

   DRETURN(ret);
}

/*
 * NOTES
 *    MT-NOTE: gdi_request_pack_result() is MT safe
 */
bool
gdi_request_pack_result(sge_gdi_request *ar, lList **answer_list,
                        sge_pack_buffer *pb)
{
   bool ret = true;

   DENTER(GDI_LAYER, "gdi_request_pack_result");
   if (ar != NULL && pb != NULL) {
      int pack_ret = PACK_SUCCESS;

      ret &= gdi_request_pack_prefix(ar, answer_list, pb);
      if (!ret) {
         goto exit_this_function;
      }

      pack_ret = cull_pack_list(pb, ar->lp);
      if (pack_ret != PACK_SUCCESS) {
         goto handle_error; 
      }

      ret &= gdi_request_pack_suffix(ar, answer_list, pb);
      if (!ret) {
         goto exit_this_function;
      }
 handle_error:
      ret = gdi_request_map_pack_error(pack_ret, answer_list);
   }

exit_this_function:

   DRETURN(ret);
}



/*--------------------------------------------------------------
 * request_list_pack_results
 *
 * NOTES
 *    MT-NOTE: request_list_pack_results() is MT safe
 *--------------------------------------------------------------*/
bool
request_list_pack_results(sge_gdi_request *ar, lList **answer_list,
                          sge_pack_buffer *pb)
{
   bool ret = true;

   DENTER(GDI_LAYER, "request_list_pack_results");

   while (ret && ar != NULL) {
      ret = gdi_request_pack_result(ar, answer_list, pb);

      ar = ar->next;
   }

   DRETURN(ret);
}


/****** sge_gdi_request/new_gdi_request() **************************************
*  NAME
*     new_gdi_request() -- allocates a gdi request structure
*
*  SYNOPSIS
*     sge_gdi_request* new_gdi_request(void) 
*
*  NOTES
*     MT-NOTE: new_gdi_request() is MT safe
*******************************************************************************/
sge_gdi_request *new_gdi_request(void)
{
   sge_gdi_request *ar;

   DENTER(GDI_LAYER, "new_gdi_request");

   ar = (sge_gdi_request *) sge_malloc(sizeof(sge_gdi_request));
   
   memset(ar, 0, sizeof(sge_gdi_request));

   DRETURN(ar);
}


/****** sge_gdi_request/free_gdi_request() **************************************
*  NAME
*     free_gdi_request() -- free's a gdi request structure
*
*  SYNOPSIS
*     sge_gdi_request *free_gdi_request(sge_gdi_request *ar)
*
*  NOTES
*     MT-NOTE: free_gdi_request() is MT safe
*******************************************************************************/
sge_gdi_request *free_gdi_request(sge_gdi_request *ar) {
   sge_gdi_request *next;

   DENTER(GDI_LAYER, "free_gdi_request");

   /*
   ** free the list from first to last
   */
   while (ar) {
      /* save next pointer */
      next = ar->next;

      FREE(ar->host);
      FREE(ar->commproc);
      FREE(ar->auth_info);

      lFreeList(&(ar->lp));
      lFreeList(&(ar->alp));
      lFreeWhere(&(ar->cp));
      lFreeWhat(&(ar->enp));

      FREE(ar);

      ar = next;
   }

   DRETURN(NULL);
}


/****** sge_gdi_request/target2string() **************************************
*  NAME
*     target2string() -- return the target list name
*
*  SYNOPSIS
*     const char *target2string(u_long32 target)
*
*  NOTES
*     MT-NOTE: target2string() is MT safe
*******************************************************************************/
static const char *target2string(u_long32 target) 
{
   const char *ret = NULL;
  
   switch (target) {
      case SGE_ADMINHOST_LIST:
         ret = "SGE_ADMINHOST_LIST";
         break;
      case SGE_SUBMITHOST_LIST:
         ret = "SGE_SUBMITHOST_LIST";
         break;
      case SGE_EXECHOST_LIST:
         ret = "SGE_EXECHOST_LIST";
         break;
      case SGE_CQUEUE_LIST:
         ret = "SGE_CQUEUE_LIST";
         break;
      case SGE_JOB_LIST:
         ret = "SGE_JOB_LIST";
         break;
      case SGE_EVENT_LIST:
         ret = "SGE_EVENT_LIST";
         break;
      case SGE_CENTRY_LIST:
         ret = "SGE_CENTRY_LIST";
         break;
      case SGE_ORDER_LIST:
         ret = "SGE_ORDER_LIST";
         break;
      case SGE_MASTER_EVENT:
         ret = "SGE_MASTER_EVENT";
         break;
      case SGE_CONFIG_LIST:
         ret = "SGE_CONFIG_LIST";
         break;
      case SGE_MANAGER_LIST:
         ret = "SGE_MANAGER_LIST";
         break;
      case SGE_OPERATOR_LIST:
         ret = "SGE_OPERATOR_LIST";
         break;
      case SGE_PE_LIST:
         ret = "SGE_PE_LIST";
         break;
      case SGE_SC_LIST:
         ret = "SGE_SC_LIST";
         break;
      case SGE_USER_LIST:
         ret = "SGE_USER_LIST";
         break;
      case SGE_USERSET_LIST:
         ret = "SGE_USERSET_LIST";
         break;
      case SGE_PROJECT_LIST:
         ret = "SGE_PROJECT_LIST";
         break;
      case SGE_SHARETREE_LIST:
         ret = "SGE_SHARETREE_LIST";
         break;
      case SGE_CKPT_LIST:
         ret = "SGE_CKPT_LIST";
         break;
      case SGE_CALENDAR_LIST:
         ret = "SGE_CALENDAR_LIST";
         break;
      case SGE_JOB_SCHEDD_INFO_LIST:
         ret = "SGE_JOB_SCHEDD_INFO_LIST";
         break;
      case SGE_ZOMBIE_LIST:
         ret = "SGE_ZOMBIE_LIST";
         break;
      case SGE_USER_MAPPING_LIST:
         ret = "SGE_USER_MAPPING_LIST";
         break;
      case SGE_HGROUP_LIST:
         ret = "SGE_HGROUP_LIST";
         break;
      case SGE_RQS_LIST:
         ret = "SGE_RQS_LIST";
         break;
      case SGE_AR_LIST:
         ret = "SGE_AR_LIST";
         break;
      default:
         ret = "unknown list";
   }
   return ret;
}

