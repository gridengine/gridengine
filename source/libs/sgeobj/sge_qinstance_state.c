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

#include "sgermon.h"
#include "sge_string.h"
#include "sge_log.h"
#include "cull_list.h"
#include "sge.h"

#include "sge_answer.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "msg_sgeobjlib.h"

/****** sgelib/qinstance/--State_Chart() **************************************
*
*         /---------------------------------------------------\
*         |                     exists                        |
*         |                                                   |
* o-----> |                                                   |------->X
*         |                               /-----------------\ |
*         |                               |   (suspended)   | |
*         |                               |                 | |
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !s   |            |    |   s   |    | |
*         |         |        | <---------------|       |    | | 
*         |         \--------/            |    \-------/    | |
*         |- - - - - - - - - - - - - - - -|- - - - - - - - -|-|
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !S   |            |    |   S   |    | |
*         |         |        | <---------------|       |    | | 
*         |         \--------/            |    \-------/    | |
*         |- - - - - - - - - - - - - - - -|- - - - - - - - -|-|
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !A   |            |    |   A   |    | |
*         |         |        | <---------------|       |    | | 
*         |         \--------/            |    \-------/    | |
*         |- - - - - - - - - - - - - - - -|- - - - - - - - -|-|
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !C   |            |    |   C   |    | |
*         |         |        | <---------------|       |    | | 
*         |         \--------/            |    \-------/    | |
*         |                               \-----------------/ |
*         |- - - - - - - - - - - - - - - - - - - - - - - - - -|
*         |                               /-----------------\ |
*         |                               |   (disabled)    | |
*         |                               |                 | |
*         |         /--------\            |    /-------\    | |
*         | o-----> |        |---------------> |       |    | |
*         |         |   !u   |            |    |   u   |    | |
*         |         |        | <---------------|       |    | |
*         |         \--------/            |    \-------/    | | 
*         |- - - - - - - - - - - - - - - -|- - - - - - - - - -|
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !a   |            |    |   a   |    | |
*         |         |        | <---------------|       |    | |
*         |         \--------/            |    \-------/    | | 
*         |- - - - - - - - - - - - - - - -|- - - - - - - - - -|
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !d   |            |    |   d   |    | |
*         |         |        | <---------------|       |    | | 
*         |         \--------/            |    \-------/    | |
*         |- - - - - - - - - - - - - - - -|- - - - - - - - -|-|
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !D   |            |    |   D   |    | |
*         |         |        | <---------------|       |    | | 
*         |         \--------/            |    \-------/    | |
*         |- - - - - - - - - - - - - - - -|- - - - - - - - -|-|
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !E   |            |    |   E   |    | |
*         |         |        | <---------------|       |    | | 
*         |         \--------/            |    \-------/    | |
*         |- - - - - - - - - - - - - - - -|- - - - - - - - -|-|
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !c   |            |    |   c   |    | |
*         |         |        | <---------------|       |    | | 
*         |         \--------/            |    \-------/    | |
*         |- - - - - - - - - - - - - - - -|- - - - - - - - -|-|
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !o   |            |    |   o   |    | |
*         |         |        | <---------------|       |    | | 
*         |         \--------/            |    \-------/    | |
*         |                               \-----------------/ |
*         \---------------------------------------------------/
*
*         u := qinstance-host is unknown
*         a := load alarm
*         s := manual suspended
*         A := suspended due to suspend_threshold
*         S := suspended due to subordinate
*         C := suspended due to calendar
*         d := manual disabled
*         D := disabled due to calendar
*         c := configuration ambiguous
*         o := orphaned
*
*******************************************************************************/


static bool
qinstance_has_state(const lListElem *this_elem, u_long32 bit);

static void 
qinstance_set_state(lListElem *this_elem, bool set_state, u_long32 bit);

static void 
qinstance_set_state(lListElem *this_elem, bool set_state, u_long32 bit)
{
   u_long32 state = lGetUlong(this_elem, QU_state);

   if (set_state) {   
      state |= bit;
   } else {
      state &= ~bit;
   }
   lSetUlong(this_elem, QU_state, state);
}

static bool 
qinstance_has_state(const lListElem *this_elem, u_long32 bit) 
{
   return (lGetUlong(this_elem, QU_state) & bit) ? true : false;
}

bool
transition_is_valid_for_qinstance(u_long32 transition, lList **answer_list)
{
   bool ret = false;
   
   if (transition == QI_DO_NOTHING ||
       transition == QI_DO_DISABLE ||
       transition == QI_DO_ENABLE ||
       transition == QI_DO_SUSPEND ||
       transition == QI_DO_UNSUSPEND ||
       transition == QI_DO_CLEARERROR ||
       transition == QI_DO_RESCHEDULE
#ifdef __SGE_QINSTANCE_STATE_DEBUG__
       || transition == QI_DO_SETERROR ||
       transition == QI_DO_SETORPHANED ||
       transition == QI_DO_CLEARORPHANED ||
       transition == QI_DO_SETUNKNOWN ||
       transition == QI_DO_CLEARUNKNOWN ||
       transition == QI_DO_SETAMBIGUOUS ||
       transition == QI_DO_CLEARAMBIGUOUS 
#endif
      ) {
      ret = true;
   } else {
      answer_list_add(answer_list, MSG_QINSTANCE_INVALIDACTION, 
                      STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
   }

   return ret;
}

bool
transition_option_is_valid_for_qinstance(u_long32 option, lList **answer_list)
{
   bool ret = false;

   DENTER(TOP_LAYER, "transition_option_is_valid_for_qinstance");
   if (option == QI_TRANSITION_NOTHING ||
       option == QI_TRANSITION_OPTION) {
      ret = true;
   } else {
      answer_list_add(answer_list, MSG_QINSTANCE_INVALIDOPTION, 
                     STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
   }
   DEXIT;
   return ret;
}

const char *
qinstance_state_as_string(u_long32 bit) 
{
   static const u_long32 states[] = { 
      QI_ALARM,
      QI_SUSPEND_ALARM,
      QI_DISABLED,
      QI_SUSPENDED,
      QI_UNKNOWN,
      QI_ERROR,
      QI_SUSPENDED_ON_SUBORDINATE,
      QI_CAL_DISABLED,
      QI_CAL_SUSPENDED,
      QI_AMBIGUOUS,
      QI_ORPHANED,

      (u_long32)~QI_ALARM,
      (u_long32)~QI_SUSPEND_ALARM,
      (u_long32)~QI_DISABLED,
      (u_long32)~QI_SUSPENDED,
      (u_long32)~QI_UNKNOWN,
      (u_long32)~QI_ERROR,
      (u_long32)~QI_SUSPENDED_ON_SUBORDINATE,
      (u_long32)~QI_CAL_DISABLED,
      (u_long32)~QI_CAL_SUSPENDED,
      (u_long32)~QI_AMBIGUOUS,
      (u_long32)~QI_ORPHANED,

      /*
       * Don't forget to change the names-array, too
       * if something is changed here
       */

      /*
       * Don't forget to change the names-array, too
       * if something is changed here
       */

      0 
   };
   static const char *names[23] = { NULL }; 
   const char *ret = NULL;
   int i = 0;

   DENTER(TOP_LAYER, "qinstance_state_as_string");
   if (names[0] == NULL) {
      names[0] = MSG_QINSTANCE_ALARM;
      names[1] = MSG_QINSTANCE_SUSPALARM;
      names[2] = MSG_QINSTANCE_DISABLED;
      names[3] = MSG_QINSTANCE_SUSPENDED;
      names[4] = MSG_QINSTANCE_UNKNOWN;
      names[5] = MSG_QINSTANCE_ERROR;
      names[6] = MSG_QINSTANCE_SUSPOSUB;
      names[7] = MSG_QINSTANCE_CALDIS;
      names[8] = MSG_QINSTANCE_CALSUSP;
      names[9] = MSG_QINSTANCE_CONFAMB;
      names[10] = MSG_QINSTANCE_ORPHANED;
      names[11] = MSG_QINSTANCE_NALARM;
      names[12] = MSG_QINSTANCE_NSUSPALARM;
      names[13] = MSG_QINSTANCE_NDISABLED;
      names[14] = MSG_QINSTANCE_NSUSPENDED;
      names[15] = MSG_QINSTANCE_NUNKNOWN;
      names[16] = MSG_QINSTANCE_NERROR;
      names[17] = MSG_QINSTANCE_NSUSPOSUB;
      names[18] = MSG_QINSTANCE_NCALDIS;
      names[19] = MSG_QINSTANCE_NCALSUSP;
      names[20] = MSG_QINSTANCE_NCONFAMB;
      names[21] = MSG_QINSTANCE_NORPHANED;
      names[22] = NULL;
   }

   while (states[i] != 0) {
      if (states[i] == bit) {
         ret = names[i];
         DTRACE;
         break;
      }
      i++;
   }
   DEXIT;
   return ret;
}

bool 
qinstance_state_append_to_dstring(const lListElem *this_elem, dstring *string)
{
   static const u_long32 states[] = {
      QI_ALARM,
      QI_SUSPEND_ALARM,
      QI_CAL_SUSPENDED,
      QI_CAL_DISABLED,
      QI_DISABLED,
      QI_UNKNOWN,
      QI_ERROR,
      QI_SUSPENDED_ON_SUBORDINATE,
      QI_SUSPENDED,
      QI_AMBIGUOUS,
      QI_ORPHANED, 
      0
   };
   static const char letters[] = {
      'a',
      'A',
      'C',
      'D',
      'd',
      'u',
      'E',
      'S',
      's',
      'c',
      'o',
      '\0'
   };
   bool ret = true;
   int i = 0;

   while (states[i] != 0) {
      if (qinstance_has_state(this_elem, states[i])) {
         sge_dstring_sprintf_append(string, "%c", letters[i]);
      }
      i++;
   }
   sge_dstring_sprintf_append(string, "%c", '\0');
   return ret;
}

void 
qinstance_state_set_orphaned(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QI_ORPHANED);
}

bool 
qinstance_state_is_orphaned(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QI_ORPHANED);
}

void 
qinstance_state_set_ambiguous(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QI_AMBIGUOUS);
}

bool 
qinstance_state_is_ambiguous(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QI_AMBIGUOUS);
}

void 
qinstance_state_set_alarm(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QI_ALARM);
}

bool 
qinstance_state_is_alarm(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QI_ALARM);
}

void 
qinstance_state_set_suspend_alarm(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QI_SUSPEND_ALARM);
}

bool 
qinstance_state_is_suspend_alarm(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QI_SUSPEND_ALARM);
}

void 
qinstance_state_set_manual_disabled(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QI_DISABLED);
}

bool 
qinstance_state_is_manual_disabled(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QI_DISABLED);
}

void 
qinstance_state_set_manual_suspended(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QI_SUSPENDED);
}

bool 
qinstance_state_is_manual_suspended(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QI_SUSPENDED);
}

void 
qinstance_state_set_unknown(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QI_UNKNOWN);
}

bool 
qinstance_state_is_unknown(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QI_UNKNOWN);
}

void 
qinstance_state_set_error(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QI_ERROR);
}

bool 
qinstance_state_is_error(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QI_ERROR);
}

void 
qinstance_state_set_susp_on_sub(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QI_SUSPENDED_ON_SUBORDINATE);
}

bool 
qinstance_state_is_susp_on_sub(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QI_SUSPENDED_ON_SUBORDINATE);
}

void 
qinstance_state_set_cal_disabled(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QI_CAL_DISABLED);
}

bool 
qinstance_state_is_cal_disabled(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QI_CAL_DISABLED);
}

void 
qinstance_state_set_cal_suspended(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QI_CAL_SUSPENDED);
}

bool 
qinstance_state_is_cal_suspended(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QI_CAL_SUSPENDED);
}

/* ret: did the state change */
bool
qinstance_set_initial_state(lListElem *this_elem)
{
   bool ret = false;
   const char *state_string = lGetString(this_elem, QU_initial_state);

   if (state_string != NULL) {
      bool do_disable = !strcmp(state_string, "disabled");
      bool is_disabled = qinstance_state_is_manual_disabled(this_elem);

      if ((do_disable && !is_disabled) || (!do_disable && is_disabled)) {
         ret = true;
         qinstance_state_set_manual_disabled(this_elem, do_disable);
      }
   }
   return ret;
}

