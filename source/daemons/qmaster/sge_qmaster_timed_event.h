#ifndef __SGE_QMASTER_TIMED_EVENT_H__
#define __SGE_QMASTER_TIMED_EVENT_H__
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

#include <time.h>
#include <sys/types.h>

#include "basis_types.h"


typedef enum {
   TYPE_CALENDAR_EVENT = 4,
   TYPE_SIGNAL_RESEND_EVENT,
   TYPE_JOB_RESEND_EVENT,
   TYPE_RESCHEDULE_UNKNOWN_EVENT,
   TYPE_SPOOLING_TRIGGER,
   TYPE_REPORTING_TRIGGER,
   TYPE_SHARELOG_TRIGGER
} te_type_t;

typedef enum {
   ONE_TIME_EVENT = 1,
   RECURRING_EVENT
} te_mode_t;

typedef struct te_event* te_event_t;

typedef void (*te_handler_t)(te_event_t);


extern void       te_register_event_handler(te_handler_t, te_type_t);
extern te_event_t te_new_event(time_t, te_type_t, te_mode_t, u_long32, u_long32, const char*);
extern void       te_free_event(te_event_t);
extern void       te_add_event(te_event_t);
extern int        te_delete_one_time_event(te_type_t, u_long32, u_long32, const char*);     
extern void       te_shutdown(void);

extern time_t      te_get_when(te_event_t);
extern te_type_t   te_get_type(te_event_t);
extern te_mode_t   te_get_mode(te_event_t);
extern u_long32    te_get_first_numeric_key(te_event_t);
extern u_long32    te_get_second_numeric_key(te_event_t);
extern const char* te_get_alphanumeric_key(te_event_t);
extern u_long32    te_get_sequence_number(te_event_t);


#endif /* __SGE_QMASTER_TIMED_EVENT_H__ */

