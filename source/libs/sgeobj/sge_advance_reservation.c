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

#include "sge_advance_reservation.h"

#include "rmon/sgermon.h"
#include "sgeobj/sge_answer.h"
#include "uti/sge_time.h"
#include "sgeobj/msg_sgeobjlib.h"

/****** sge_advance_reservation/ar_list_locate() *******************************
*  NAME
*     ar_list_locate() -- locate a advance reservation by id
*
*  SYNOPSIS
*     lListElem* ar_list_locate(lList *ar_list, u_long32 ar_id) 
*
*  FUNCTION
*     This function returns a ar object with the selected id from the
*     given list.
*
*  INPUTS
*     lList *ar_list - list to be searched in
*     u_long32 ar_id - id of interest
*
*  RESULT
*     lListElem* - if found the reference to the ar object, else NULL
*
*  NOTES
*     MT-NOTE: ar_list_locate() is MT safe 
*******************************************************************************/
lListElem *ar_list_locate(lList *ar_list, u_long32 ar_id)
{
   lListElem *ep = NULL;

   DENTER(TOP_LAYER, "ar_list_locate");

   ep = lGetElemUlong(ar_list, AR_id, ar_id);

   DRETURN(ep);
}

/****** sge_advance_reservation/ar_validate() **********************************
*  NAME
*     ar_validate() -- validate a advance reservation
*
*  SYNOPSIS
*     bool ar_validate(lListElem *object, lList **answer_list, bool in_master) 
*
*  FUNCTION
*     Ensures a new ar has valid start and end times
*
*  INPUTS
*     lListElem *object   - the ar to check
*     lList **answer_list - answer list pointer
*     bool in_master      - are we in qmaster?
*
*  RESULT
*     bool - true if OK, else false
*
*  NOTES
*     MT-NOTE: ar_validate() is MT safe 
*******************************************************************************/
bool ar_validate(lListElem *object, lList **answer_list, bool in_master) {
   u_long32 start_time;
   u_long32 end_time;
   u_long32 duration;
   u_long32 now = sge_get_gmt();

   DENTER(TOP_LAYER, "ar_validate");

   if ((start_time = lGetUlong(object, AR_start_time)) == 0) {
      answer_list_add_sprintf(answer_list, STATUS_EEXIST, ANSWER_QUALITY_ERROR ,
                              MSG_AR_MISSING_VALUE_S, "start time");
      goto ERROR;
   }

   if ((end_time = lGetUlong(object, AR_end_time)) == 0) {
      answer_list_add_sprintf(answer_list, STATUS_EEXIST, ANSWER_QUALITY_ERROR,
                              MSG_AR_MISSING_VALUE_S, "end time");
      goto ERROR;
   }

   if ((duration =  lGetUlong(object, AR_duration)) == 0) {
      answer_list_add_sprintf(answer_list, STATUS_EEXIST, ANSWER_QUALITY_ERROR,
                              MSG_AR_MISSING_VALUE_S, "duration");
      goto ERROR;
   }

   if ((end_time - start_time) != duration) {
      answer_list_add_sprintf(answer_list, STATUS_EEXIST, ANSWER_QUALITY_ERROR, 
                              MSG_AR_START_END_DURATION_INVALID);
      goto ERROR;
   }

   if (start_time > end_time) {
      answer_list_add_sprintf(answer_list, STATUS_EEXIST, ANSWER_QUALITY_ERROR,
                              MSG_AR_START_LATER_THAN_END);
      goto ERROR;
   }
   if (start_time < now) {
      answer_list_add_sprintf(answer_list, STATUS_EEXIST, ANSWER_QUALITY_ERROR,
                              MSG_AR_START_IN_PAST);
      goto ERROR;
   }

   if (in_master) {
      /* AR TBD */
   }

   DRETURN(true);

ERROR:
   DRETURN(false);
}
