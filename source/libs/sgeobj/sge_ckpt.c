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

#include <strings.h>
#include <ctype.h>

#include "sgermon.h"
#include "sge_log.h"
#include "cull_list.h"

#include "config_file.h"
#include "sge_signal.h"

#include "sge_answer.h"
#include "sge_job.h"
#include "sge_queue.h"
#include "sge_utility.h"
#include "sge_ckpt.h"
#include "symbols.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

lList *Master_Ckpt_List = NULL;

/****** sgeobj/ckpt/ckpt_is_referenced() **************************************
*  NAME
*     ckpt_is_referenced() -- Is a given CKPT referenced in other objects? 
*
*  SYNOPSIS
*     bool ckpt_is_referenced(const lListElem *ckpt, lList **answer_list, 
*                             const lList *job_list) 
*
*  FUNCTION
*     This function returns true if the given "ckpt" is referenced
*     in a job contained in "job_list". If this is the case than
*     a corresponding message will be added to the "answer_list". 
*
*  INPUTS
*     const lListElem *ckpt - CK_Type object 
*     lList **answer_list   - AN_Type list 
*     const lList *job_list - JB_Type list 
*
*  RESULT
*     bool - true or false  
******************************************************************************/
bool ckpt_is_referenced(const lListElem *ckpt, lList **answer_list,
                        const lList *job_list)
{
   lListElem *job = NULL;
   bool ret = false;

   for_each(job, job_list) {
      if (job_is_ckpt_referenced(job, ckpt)) {
         const char *ckpt_name = lGetString(ckpt, CK_name);
         u_long32 job_id = lGetUlong(job, JB_job_number);

         sprintf(SGE_EVENT, MSG_CKPTREFINJOB_SU, ckpt_name, u32c(job_id));
         answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN,
                         ANSWER_QUALITY_INFO);
         ret = true;
      }
   } 
   return ret;
}

/****** sgeobj/ckpt/ckpt_list_locate() ***************************************
*  NAME
*     ckpt_list_locate -- find a ckpt object in a list 
*
*  SYNOPSIS
*     lListElem *ckpt_list_locate(lList *ckpt_list, const char *ckpt_name)
*
*  FUNCTION
*     This function will return a ckpt object by name if it exists.
*
*
*  INPUTS
*     lList *ckpt_list      - CK_Type object
*     const char *ckpt_name - name of the ckpt object. 
*
*  RESULT
*     NULL - ckpt object with name "ckpt_name" does not exist
*     !NULL - pointer to the cull element (CK_Type) 
******************************************************************************/
lListElem *ckpt_list_locate(lList *ckpt_list, const char *ckpt_name)
{
   return lGetElemStr(ckpt_list, CK_name, ckpt_name);
}

/*-----------------------------------------------------------
 * sge_parse_checkpoint_attr
 *    parse checkpoint "when" string
 * return:
 *    bitmask of checkpoint specifers
 *    0 if attr_str == NULL or nothing set or value may be a time value
 *
 * NOTES
 *    MT-NOTE: sge_parse_checkpoint_attr() is MT safe
 *-----------------------------------------------------------*/
int sge_parse_checkpoint_attr(const char *attr_str)
{
   int opr;

   if (attr_str == NULL) {
      return 0;
   }

   /* May be it's a time value */
   if (isdigit((int) *attr_str) || (*attr_str == ':')) {
      return 0;
   }

   opr = 0;
   while (*attr_str) {
      if (*attr_str == CHECKPOINT_AT_MINIMUM_INTERVAL_SYM)
         opr = opr | CHECKPOINT_AT_MINIMUM_INTERVAL;
      else if (*attr_str == CHECKPOINT_AT_SHUTDOWN_SYM)
         opr = opr | CHECKPOINT_AT_SHUTDOWN;
      else if (*attr_str == CHECKPOINT_SUSPEND_SYM)
         opr = opr | CHECKPOINT_SUSPEND;
      else if (*attr_str == NO_CHECKPOINT_SYM)
         opr = opr | NO_CHECKPOINT;
      else if (*attr_str == CHECKPOINT_AT_AUTO_RES_SYM)
         opr = opr | CHECKPOINT_AT_AUTO_RES;
      else {
         opr = -1;
         break;
      }
      attr_str++;
   }

   return opr;
}

/****** gdi/ckpt/ckpt_validate() ******************************************
*  NAME
*     ckpt_validate -- validate all ckpt interface parameters
*
*  SYNOPSIS
*     int ckpt_validate(lListElem *ep, lList **alpp);
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
*
*  NOTES
*     MT-NOTE: ckpt_validate() is not MT safe
******************************************************************************/
int ckpt_validate(lListElem *this_elem, lList **alpp)
{
   static const char* ckpt_interfaces[] = {
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

   DENTER(TOP_LAYER, "ckpt_validate");

   if (!this_elem) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* -------- CK_name */
   if (verify_str_key(alpp, lGetString(this_elem, CK_name), 
                      "checkpoint interface")) {
      DEXIT;
      return STATUS_EUNKNOWN;
   }



   /*
   ** check if ckpt obj can be added
   ** check allowed interfaces and license
   */
   if ((interface = lGetString(this_elem, CK_interface))) {
      found = 0;
      for (i=0; i < (sizeof(ckpt_interfaces)/sizeof(char*)); i++) {
         if (!strcasecmp(interface, ckpt_interfaces[i])) {
            found = 1;
            break;
         }
      }

      if (!found) {
         ERROR((SGE_EVENT, MSG_SGETEXT_NO_INTERFACE_S, interface));
         answer_list_add(alpp, SGE_EVENT, 
                         STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
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
            answer_list_add(alpp, SGE_EVENT, 
                            STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         }
      }
#endif
   }

   for (i=0; ckpt_commands[i].nm!=NoName; i++) {
      if (replace_params(lGetString(this_elem, ckpt_commands[i].nm),
               NULL, 0, ckpt_variables)) {
         ERROR((SGE_EVENT, MSG_OBJ_CKPTENV_SSS,
               ckpt_commands[i].text, lGetString(this_elem, CK_name), err_msg));
         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EEXIST;
      }
   }

   /* -------- CK_queue_list */
   if (queue_reference_list_validate(alpp, lGetList(this_elem, CK_queue_list), 
                                     MSG_OBJ_QLIST, MSG_OBJ_CKPTI, 
                                   lGetString(this_elem, CK_name))!=STATUS_OK) {
      DEXIT;
      return STATUS_EEXIST;
   }

   /* -------- CK_signal */
   if ((s=lGetString(this_elem, CK_signal)) &&
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


