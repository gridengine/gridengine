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

#include "sge.h"
#include "sgermon.h"
#include "basis_types.h"
#include "sge_ja_task.h"
#include "sge_answer.h"
#include "sge_log.h"
#include "sge_suserL.h"
#include "sge_job.h"
#include "sge_suser.h"
#include "sge_queue.h"
#include "sge_pe.h"
#include "sge_ckpt.h"
#include "sge_todo.h"
#include "sge_utility.h"
#include "sge_string.h"
#include "sge_userset.h"
#include "config_file.h"
#include "sge_event_master.h"
#include "gdi_utility.h"
#include "sge_signal.h"
#include "sge_userprj.h"
#include "sge_pe_task.h"
#include "sge_manop.h"
#include "sge_host.h"
#include "sge_complex.h"
#include "sge_sharetree.h"
#include "sge_answer.h"
#include "version.h"
#include "sge_schedd_conf.h"
#include "sge_conf.h"
#include "sge_calendar.h"
#include "sge_report.h"
#include "sge_queue_event_master.h"

#include "msg_common.h"
#include "msg_qmaster.h"
#include "msg_gdilib.h"
#include "msg_sgeobjlib.h"

/****** gdi/ckpt/validate_ckpt() ******************************************
*  NAME
*     validate_ckpt -- validate all ckpt interface parameters
*
*  SYNOPSIS
*     int validate_ckpt(lListElem *ep, lList **alpp);
*
*  FUNCTION
*     This function will test all ckpt interface parameters.
*     If all are valid then it will return successfull.
*
*
*  INPUTS
*     ep     - element which sould be verified.
*     answer - answer list where the function stored error messages
*
*
*  RESULT
*     [answer] - error messages will be added to this list
*     STATUS_OK - success
*     STATUS_EUNKNOWN or STATUS_EEXIST - error
******************************************************************************/
int validate_ckpt(lListElem *ep, lList **alpp)
{
   static char* ckpt_interfaces[] = {
      "USERDEFINED",
      "HIBERNATOR",
      "TRANSPARENT",
      "APPLICATION-LEVEL",
      "CPR",
      "CRAY-CKPT"
   };
   static struct attr {
      int nm;
      char *text;
   } ckpt_commands[] = {
      { CK_ckpt_command, "ckpt_command" },
      { CK_migr_command, "migr_command" },
      { CK_rest_command, "restart_command"},
      { CK_clean_command, "clean_command"},
      { NoName,           NULL} };

   int i;
   int found = 0;
   const char *s, *interface;

   DENTER(TOP_LAYER, "validate_ckpt_obj");

   if (!ep) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* -------- CK_name */
   if (verify_str_key(alpp, lGetString(ep, CK_name), "checkpoint interface")) {
      DEXIT;
      return STATUS_EUNKNOWN;
   }



   /*
   ** check if ckpt obj can be added
   ** check allowed interfaces and license
   */
   if ((interface = lGetString(ep, CK_interface))) {
      found = 0;
      for (i=0; i < (sizeof(ckpt_interfaces)/sizeof(char*)); i++) {
         if (!strcasecmp(interface, ckpt_interfaces[i])) {
            found = 1;
            break;
         }
      }

      if (!found) {
         ERROR((SGE_EVENT, MSG_SGETEXT_NO_INTERFACE_S, interface));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EEXIST;
      }

#ifdef PW
      /* license check */
      if (!set_licensed_feature("ckpt")) {
         if (!strcasecmp(interface, "HIBERNATOR") ||
             !strcasecmp(interface, "CPR") ||
             !strcasecmp(interface, "APPLICATION-LEVEL") ||
             !strcasecmp(interface, "CRAY-CKPT")) {
            ERROR((SGE_EVENT, MSG_SGETEXT_NO_CKPT_LIC));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         }
      }
#endif
   }

   for (i=0; ckpt_commands[i].nm!=NoName; i++) {
      if (replace_params(lGetString(ep, ckpt_commands[i].nm),
               NULL, 0, ckpt_variables)) {
         ERROR((SGE_EVENT, MSG_OBJ_CKPTENV_SSS,
               ckpt_commands[i].text, lGetString(ep, CK_name), err_msg));
         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EEXIST;
      }
   }

   /* -------- CK_queue_list */
   if (queue_reference_list_validate(alpp, lGetList(ep, CK_queue_list), MSG_OBJ_QLIST,
               MSG_OBJ_CKPTI, lGetString(ep, CK_name))!=STATUS_OK) {
      DEXIT;
      return STATUS_EEXIST;
   }

   /* -------- CK_signal */
   if ((s=lGetString(ep, CK_signal)) &&
         strcasecmp(s, "none") &&
         sge_sys_str2signal(s)==-1) {
      ERROR((SGE_EVENT, MSG_CKPT_XISNOTASIGNALSTRING_S , s));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   DEXIT;
   return STATUS_OK;
}


/****** gdi/pe/pe_validate() ***************************************************
*  NAME
*     pe_validate() -- validate a parallel environment
*
*  SYNOPSIS
*     int pe_validate(int startup, lListElem *pep, lList **alpp)
*
*  FUNCTION
*     Ensures that a new pe is not a duplicate of an already existing one
*     and checks consistency of the parallel environment:
*        - pseudo parameters in start and stop proc
*        - validity of the allocation rule
*        - correctness of the queue list, the user list and the xuser list
*
*
*  INPUTS
*     int startup    - are we in qmaster startup phase?
*     lListElem *pep - the pe to check
*     lList **alpp   - answer list pointer, if an answer shall be created, else
*                      NULL - errors will in any case be output using the
*                      Grid Engine error logging macros.
*
*  RESULT
*     int - STATUS_OK, if everything is ok, else other status values,
*           see libs/gdi/sge_answer.h
*******************************************************************************/
int pe_validate(int startup, lListElem *pep, lList **alpp)
{
   const char *s;
   const char *pe_name;
   int ret;

   DENTER(TOP_LAYER, "pe_validate");

   pe_name = lGetString(pep, PE_name);
   if (pe_name && verify_str_key(alpp, pe_name, MSG_OBJ_PE)) {
      ERROR((SGE_EVENT, "Invalid character in pe name of pe "SFQ, pe_name));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* register our error function for use in replace_params() */
   config_errfunc = error;

   /* -------- start_proc_args */
   s = lGetString(pep, PE_start_proc_args);
   if (s && replace_params(s, NULL, 0, pe_variables )) {
      ERROR((SGE_EVENT, MSG_PE_STARTPROCARGS_SS, pe_name, err_msg));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }


   /* -------- stop_proc_args */
   s = lGetString(pep, PE_stop_proc_args);
   if (s && replace_params(s, NULL, 0, pe_variables )) {
      ERROR((SGE_EVENT, MSG_PE_STOPPROCARGS_SS, pe_name, err_msg));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* -------- allocation_rule */
   s = lGetString(pep, PE_allocation_rule);
   if (!s)  {
      ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(PE_allocation_rule), "validate_pe"));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   if (replace_params(s, NULL, 0, pe_alloc_rule_variables )) {
      ERROR((SGE_EVENT, MSG_PE_ALLOCRULE_SS, pe_name, err_msg));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* -------- PE_queue_list */
   if ((ret=queue_reference_list_validate(alpp, lGetList(pep, PE_queue_list), MSG_OBJ_QLIST,
               MSG_OBJ_PE, pe_name))!=STATUS_OK && !startup) {
      DEXIT;
      return ret;
   }

   /* -------- PE_user_list */
   if ((ret=userset_list_validate_acl_list(alpp, lGetList(pep, PE_user_list), MSG_OBJ_USERLIST,
               MSG_OBJ_PE, pe_name))!=STATUS_OK) {
      DEXIT;
      return ret;
   }

   /* -------- PE_xuser_list */
   if ((ret=userset_list_validate_acl_list(alpp, lGetList(pep, PE_xuser_list), MSG_OBJ_XUSERLIST,
               MSG_OBJ_PE, pe_name))!=STATUS_OK) {
      DEXIT;
      return ret;
   }

   DEXIT;
   return STATUS_OK;
}

/****** gdi/report/report_list_send() ******************************************
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
*******************************************************************************/
int report_list_send(const lList *rlp, const char *rhost,
                     const char *commproc, int id,
                     int synchron, u_long32 *mid)
{
   sge_pack_buffer pb;
   int ret, size;

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

   ret = sge_send_any_request(synchron, mid, rhost, commproc, id, &pb,
                              TAG_REPORT_REQUEST);
   clear_packbuffer(&pb);

   DEXIT;
   return ret;
}



