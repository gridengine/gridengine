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

#include "sgermon.h"
#include "sge_string.h"
#include "sge_log.h"
#include "cull_list.h"
#include "sge.h"

#include "sge_queue.h"
#include "sge_qinstance_state.h"
#include "msg_sgeobjlib.h"

/****** sgelib/qinstance/--State_Chart() **************************************
*
*         /---------------------------------------------------\
*         |                     exists                        |
*         |                                                   |
* o-----> |         /--------\                    /-------\   |------->X
*         | o-----> |        |------------------> |       |   |
*         |         |   !u   |                    |   u   |   |
*         |         |        | <------------------|       |   |
*         |         \--------/                    \-------/   | 
*         |- - - - - - - - - - - - - - - - - - - - - - - - - -|
*         |         /--------\                    /-------\   |   
*         | o-----> |        |------------------> |       |   |
*         |         |   !a   |                    |   a   |   |
*         |         |        | <------------------|       |   |
*         |         \--------/                    \-------/   | 
*         |- - - - - - - - - - - - - - - - - - - - - - - - - -|
*         |                               /-----------------\ |
*         |                               |    suspended    | |
*         |                               |                 | |
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !s   |            |    |   s   |    | |
*         |         |        | <---------------|       |    | | 
*         |         \--------/            |    \-------/    | |
*         |- - - - - - - - - - - - - - - -|- - - - - - - - -|-|
*         |                               |                 | |
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !S   |            |    |   S   |    | |
*         |         |        | <---------------|       |    | | 
*         |         \--------/            |    \-------/    | |
*         |- - - - - - - - - - - - - - - -|- - - - - - - - -|-|
*         |                               |                 | |
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !A   |            |    |   A   |    | |
*         |         |        | <---------------|       |    | | 
*         |         \--------/            |    \-------/    | |
*         |- - - - - - - - - - - - - - - -|- - - - - - - - -|-|
*         |                               |                 | |
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !C   |            |    |   C   |    | |
*         |         |        | <---------------|       |    | | 
*         |         \--------/            |    \-------/    | |
*         |                               \-----------------/ |
*         |- - - - - - - - - - - - - - - - - - - - - - - - - -|
*         |                               /-----------------\ |
*         |                               |    disabled     | |
*         |                               |                 | |
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !d   |            |    |   d   |    | |
*         |         |        | <---------------|       |    | | 
*         |         \--------/            |    \-------/    | |
*         |- - - - - - - - - - - - - - - -|- - - - - - - - -|-|
*         |                               |                 | |
*         |         /--------\            |    /-------\    | |   
*         | o-----> |        |---------------> |       |    | |
*         |         |   !D   |            |    |   D   |    | |
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

const char *
qinstance_state_as_string(u_long32 bit) 
{
   static const u_long32 states[] = {
      QALARM,
      QSUSPEND_ALARM,
      QDISABLED,
      QSUSPENDED,
      QUNKNOWN,
      QERROR,
      QSUSPENDED_ON_SUBORDINATE,
      QCAL_DISABLED,
      QCAL_SUSPENDED,

      ~QALARM,
      ~QSUSPEND_ALARM,
      ~QDISABLED,
      ~QSUSPENDED,
      ~QUNKNOWN,
      ~QERROR,
      ~QSUSPENDED_ON_SUBORDINATE,
      ~QCAL_DISABLED,
      ~QCAL_SUSPENDED,

      0 
   };
   static const char *names[] = {
      "ALARM",
      "SUSPEND ALARM",
      "DISABLED",
      "SUSPENDED",
      "UNKNOWN",
      "ERROR",
      "SUSPENDED ON SUBORDINATE",
      "CALENDAR DISABLED",
      "CALENDAR SUSPENDED",

      "NO ALARM",
      "NO SUSPEND ALARM",
      "ENABLED",
      "UNSUSPENDED",
      "NOT UNKNOWN",
      "NO ERROR",
      "NO SUSPENDED ON SUBORDINATE",
      "CALENDAR ENABLED",
      "CALENDAR UNSUSPENDED",

      NULL
   };
   const char *ret = NULL;
   int i = 0;

   while (states[i] != 0) {
      if (states[i] == bit) {
         ret = names[i];
         break;
      }
      i++;
   }
   return ret;
}

bool 
qinstance_state_append_to_dstring(const lListElem *this_elem, dstring *string)
{
   static const u_long32 states[] = {
      QALARM,
      QSUSPEND_ALARM,
      QCAL_SUSPENDED,
      QCAL_DISABLED,
      QDISABLED,
      QUNKNOWN,
      QERROR,
      QSUSPENDED_ON_SUBORDINATE,
      QSUSPENDED,
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
qinstance_state_set_alarm(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QALARM);
}

bool 
qinstance_state_is_alarm(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QALARM);
}

void 
qinstance_state_set_suspend_alarm(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QSUSPEND_ALARM);
}

bool 
qinstance_state_is_suspend_alarm(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QSUSPEND_ALARM);
}

void 
qinstance_state_set_manual_disabled(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QDISABLED);
}

bool 
qinstance_state_is_manual_disabled(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QDISABLED);
}

void 
qinstance_state_set_manual_suspended(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QSUSPENDED);
}

bool 
qinstance_state_is_manual_suspended(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QSUSPENDED);
}

void 
qinstance_state_set_unknown(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QUNKNOWN);
}

bool 
qinstance_state_is_unknown(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QUNKNOWN);
}

void 
qinstance_state_set_error(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QERROR);
}

bool 
qinstance_state_is_error(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QERROR);
}

void 
qinstance_state_set_susp_on_sub(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QSUSPENDED_ON_SUBORDINATE);
}

bool 
qinstance_state_is_susp_on_sub(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QSUSPENDED_ON_SUBORDINATE);
}

void 
qinstance_state_set_cal_disabled(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QCAL_DISABLED);
}

bool 
qinstance_state_is_cal_disabled(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QCAL_DISABLED);
}

void 
qinstance_state_set_cal_suspended(lListElem *this_elem, bool set_state)
{
   qinstance_set_state(this_elem, set_state, QCAL_SUSPENDED);
}

bool 
qinstance_state_is_cal_suspended(const lListElem *this_elem)
{
   return qinstance_has_state(this_elem, QCAL_SUSPENDED);
}

