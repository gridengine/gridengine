#ifndef __SGE_QINSTANCE_STATE_H
#define __SGE_QINSTANCE_STATE_H

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

#include "sge_dstring.h"
#include "sge_queueL.h"

const char *
qinstance_state_as_string(u_long32 bit);

/* */

void 
qinstance_state_set_alarm(lListElem *this_elem, bool set_state);

void 
qinstance_state_set_suspend_alarm(lListElem *this_elem, bool set_state);

void 
qinstance_state_set_manual_disabled(lListElem *this_elem, bool set_state);

void 
qinstance_state_set_manual_suspended(lListElem *this_elem, bool set_state);

void 
qinstance_state_set_unknown(lListElem *this_elem, bool set_state);

void 
qinstance_state_set_error(lListElem *this_elem, bool set_state);

void 
qinstance_state_set_susp_on_sub(lListElem *this_elem, bool set_state);

void 
qinstance_state_set_cal_disabled(lListElem *this_elem, bool set_state);

void 
qinstance_state_set_cal_suspended(lListElem *this_elem, bool set_state);


bool 
qinstance_state_is_alarm(const lListElem *this_elem);

bool 
qinstance_state_is_suspend_alarm(const lListElem *this_elem);

bool 
qinstance_state_is_manual_disabled(const lListElem *this_elem);

bool 
qinstance_state_is_manual_suspended(const lListElem *this_elem);

bool 
qinstance_state_is_unknown(const lListElem *this_elem);

bool 
qinstance_state_is_error(const lListElem *this_elem);

bool 
qinstance_state_is_susp_on_sub(const lListElem *this_elem);

bool 
qinstance_state_is_cal_disabled(const lListElem *this_elem);

bool 
qinstance_state_is_cal_suspended(const lListElem *this_elem);

bool 
qinstance_state_append_to_dstring(const lListElem *this_elem, dstring *string);

#endif /* __SGE_QINSTANCE_STATE_H */
