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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

#include "sge.h"
#include "sge_log.h"
#include "sgermon.h"
#include "sge_m_event.h"
#include "time_event.h"
#include "read_write_cal.h"
#include "sge_c_gdi.h"
#include "sge_calendar_qmaster.h"
#include "sge_qmod_qmaster.h"
#include "gdi_utility.h"
#include "sge_time.h"
#include "sge_unistd.h"
#include "sge_answer.h"
#include "sge_queue.h"
#include "sge_calendar.h"
#include "sge_complex.h"
#include "sge_utility.h"

#include "msg_common.h"
#include "msg_qmaster.h"

#ifdef QIDL
   #include "qidl_c_gdi.h"
#endif

int calendar_mod(
lList **alpp,
lListElem *new_cal,
lListElem *cep,
int add,
const char *ruser,
const char *rhost,
gdi_object_t *object,
int sub_command 
) {
   const char *cal_name;

   DENTER(TOP_LAYER, "calendar_mod");

   /* ---- CAL_name cannot get changed - we just ignore it */
   if (add) {
      cal_name = lGetString(cep, CAL_name);
      if (verify_str_key(alpp, cal_name, "calendar"))
         goto ERROR;
      lSetString(new_cal, CAL_name, cal_name);
   } else
      cal_name = lGetString(new_cal, CAL_name);

   /* ---- CAL_year_calendar */
   attr_mod_zerostr(cep, new_cal, CAL_year_calendar, "year calendar");
   if (lGetPosViaElem(cep, CAL_year_calendar)>=0) {
      if (parse_year(alpp, new_cal)) 
         goto ERROR;
   }

   /* ---- CAL_week_calendar */
   attr_mod_zerostr(cep, new_cal, CAL_week_calendar, "week calendar");
   if (lGetPosViaElem(cep, CAL_week_calendar)>=0) {
      if (parse_week(alpp, new_cal))
         goto ERROR;
   }

   sge_add_event( NULL, 0, add ? sgeE_CALENDAR_ADD : sgeE_CALENDAR_MOD, 
         0, 0, cal_name, new_cal);

   DEXIT;
   return 0;

ERROR:
   DEXIT;
   return STATUS_EUNKNOWN;
}

int calendar_spool(
lList **alpp,
lListElem *cep,
gdi_object_t *object 
) {
   DENTER(TOP_LAYER, "calendar_spool");

   if (write_cal(1, 2, cep)==NULL) {
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return 1;
   }

   DEXIT;
   return 0;
}

int sge_del_calendar(
lListElem *cep,
lList **alpp,
char *ruser,
char *rhost 
) {
   lListElem *qep;
   const char *cal_name;

   DENTER(TOP_LAYER, "sge_del_calendar");

   if ( !cep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* ep is no calendar element, if cep has no CAL_name */
   if (lGetPosViaElem(cep, CAL_name)<0) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(QU_qname), SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }
   cal_name = lGetString(cep, CAL_name);

   if (!calendar_list_locate(Master_Calendar_List, cal_name)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, MSG_OBJ_CALENDAR, cal_name));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* prevent deletion of a still referenced calendar */
   for_each (qep, Master_Queue_List) {
      const char *s;

      if ((s=lGetString(qep, QU_calendar)) && !strcmp(cal_name, s)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_CALENDARSTILLREFERENCEDINQUEUE_SS, 
               cal_name, lGetString(qep, QU_qname)));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_ESEMANTIC;
      }
   }

   /* remove timer for this calendar */
   te_delete(TYPE_CALENDAR_EVENT, cal_name, 0, 0);

   sge_unlink(CAL_DIR, cal_name);
   lDelElemStr(&Master_Calendar_List, CAL_name, cal_name);

#ifdef QIDL
   deleteObjectByName(SGE_CALENDAR_LIST,cal_name);
#endif
   
   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS,
         ruser, rhost, cal_name, MSG_OBJ_CALENDAR));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   sge_add_event(NULL, 0, sgeE_CALENDAR_DEL, 0, 0, cal_name, NULL);
   DEXIT;
   return STATUS_OK;
}

void calendar_event(
u_long32 type,
u_long32 when,
u_long32 uval0,
u_long32 uval1,
const char *cal_name 
) {
   lListElem *cep;

   DENTER(TOP_LAYER, "calendar_event");

   if (!(cep=calendar_list_locate(Master_Calendar_List, cal_name))) {
      ERROR((SGE_EVENT, MSG_EVE_TE4CAL_S, cal_name));
      DEXIT;
      return;
   }
      
   calendar_update_queue_states(cep, 0, NULL);

   DEXIT;
}




int calendar_update_queue_states(
lListElem *cep,
lListElem *old_cep,
gdi_object_t *object 
) {
   const char *cal_name, *s;
   time_t next_event;
   u_long32 new_state, old_state;
   lListElem *qep;

   DENTER(TOP_LAYER, "calendar_update_queue_states");

   cal_name = lGetString(cep, CAL_name);
   DPRINTF(("CALENDAR: %s\n", cal_name));
   new_state = act_cal_state(cep, &next_event);

   for_each (qep, Master_Queue_List) {
      if (!(s=lGetString(qep, QU_calendar)) || strcmp(s, cal_name))
         continue;

      old_state = lGetUlong(qep, QU_state) & (QCAL_SUSPENDED|QCAL_DISABLED);
      signal_on_calendar(qep, old_state, new_state);
   }

   if (next_event) {
      te_add(TYPE_CALENDAR_EVENT, next_event, 0, 0, cal_name);
   }

   DEXIT;
   return 0;
}

