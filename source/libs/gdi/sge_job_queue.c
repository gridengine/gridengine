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

#include "sge_job_queue.h"
#include "symbols.h"
#include "def.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_jobL.h"
#include "sge_queueL.h" 
#include "msg_utilib.h" 

static void sge_get_states(int nm, char *str, u_long32 op); 

static void sge_get_states(int nm, char *str, u_long32 op) 
{
   int count = 0;
 
   DENTER(TOP_LAYER, "sge_get_states");
 
   if (nm==QU_qname) {
      if (VALID(QALARM, op))
         str[count++] = ALARM_SYM;
      if (VALID(QSUSPEND_ALARM, op))
         str[count++] = SUSPEND_ALARM_SYM;
      if (VALID(QCAL_SUSPENDED, op))
         str[count++] = SUSPENDED_ON_CALENDAR_SYM;
      if (VALID(QCAL_DISABLED, op))
         str[count++] = DISABLED_ON_CALENDAR_SYM;
      if (VALID(QDISABLED, op))
         str[count++] = DISABLED_SYM;
      if (!VALID(!QDISABLED, op))
         str[count++] = ENABLED_SYM;
      if (VALID(QUNKNOWN, op))
         str[count++] = UNKNOWN_SYM;
      if (VALID(QERROR, op))
         str[count++] = ERROR_SYM;
      if (VALID(QSUSPENDED_ON_SUBORDINATE, op))
         str[count++] = SUSPENDED_ON_SUBORDINATE_SYM;
   }
 
   if (nm==JB_job_number) {
      if (VALID(JDELETED, op))
         str[count++] = DISABLED_SYM;
      if (VALID(JERROR, op))
         str[count++] = ERROR_SYM;
      if (VALID(JSUSPENDED_ON_SUBORDINATE, op))
         str[count++] = SUSPENDED_ON_SUBORDINATE_SYM;
   }
 
   if (VALID(JSUSPENDED_ON_THRESHOLD, op)) {
      str[count++] = SUSPENDED_ON_THRESHOLD_SYM;
   }
 
   if (VALID(JHELD, op)) {
      str[count++] = HELD_SYM;
   }
 
   if (VALID(JMIGRATING, op)) {
      str[count++] = RESTARTING_SYM;
   }
 
   if (VALID(JQUEUED, op)) {
      str[count++] = QUEUED_SYM;
   }                      

   if (VALID(JRUNNING, op)) {
      str[count++] = RUNNING_SYM;
   }
 
   if (VALID(JSUSPENDED, op)) {
      str[count++] = SUSPENDED_SYM;
   }
 
   if (VALID(JTRANSFERING, op)) {
      str[count++] = TRANSISTING_SYM;
   }
 
   if (VALID(JWAITING, op)) {
      str[count++] = WAITING_SYM;
   }
 
   if (VALID(JEXITING, op)) {
      str[count++] = EXITING_SYM;
   }
 
   str[count++] = '\0';
 
   DEXIT;
   return;
}     

/****** gdi/job_jatask/job_get_state_string() *********************************
*  NAME
*     job_get_state_string() -- write job state flags into a string 
*
*  SYNOPSIS
*     void job_get_state_string(char *str, u_long32 op) 
*
*  FUNCTION
*     This function writes the state flags given by 'op' into the 
*     string 'str'
*
*  INPUTS
*     char *str   - containes the state flags for 'qstat'/'qhost' 
*     u_long32 op - job state bitmask 
******************************************************************************/
void job_get_state_string(char *str, u_long32 op) 
{
   sge_get_states(JB_job_number, str, op);
}

/****** gdi/queue/queue_get_state_string() ************************************
*  NAME
*     queue_get_state_string() -- write queue state flags into a string 
*
*  SYNOPSIS
*     void queue_get_state_string(char *str, u_long32 op) 
*
*  FUNCTION
*     This function writes the state flags given by 'op' into the
*     string 'str'                                     
*
*  INPUTS
*     char *str   - containes the state flags for 'qstat'/'qhost' 
*     u_long32 op - queue state bitmask 
******************************************************************************/
void queue_get_state_string(char *str, u_long32 op) 
{
   sge_get_states(QU_qname, str, op); 
}

