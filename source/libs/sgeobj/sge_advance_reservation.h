#ifndef __SGE_ADVANCE_RESERVATION_H
#define __SGE_ADVANCE_RESERVATION_H 
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

#include "sgeobj/sge_advance_reservationL.h"

lListElem *
ar_list_locate(lList *ar_list, u_long32 job_id);

bool 
ar_validate(lListElem *ar, lList **alpp, bool in_master, bool is_spool);

ar_state_event_t 
ar_get_event_from_string(const char *string);

const char *
ar_get_string_from_event(ar_state_event_t event);

void ar_state2dstring(ar_state_t state, dstring *state_as_string);

bool sge_ar_has_errors(lListElem *ar);

#endif /* __SGE_ADVANCE_RESERVATION_H */
