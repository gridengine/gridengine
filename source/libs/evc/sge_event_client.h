#ifndef __SGE_C_EVENT_H
#define __SGE_C_EVENT_H
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

#ifndef TEST_GDI2

#include "sge_gdi.h"

#include "sge_eventL.h"
#include "uti/sge_monitor.h"

#define DEFAULT_EVENT_DELIVERY_INTERVAL (10)



lListElem *ec_get_event_client(void);
void ec_set_event_client(lListElem *ec);

bool ec_prepare_registration(ev_registration_id id, const char *name);
bool ec_register(lListElem *event_client, bool exit_on_qmaster_down, lList **alpp);
bool ec_deregister(lListElem **event_client);
bool ec_is_initialized(lListElem *event_client);


bool ec_subscribe(lListElem *event_client, ev_event event);
bool ec_subscribe_all(lListElem *event_client);
bool ec_subscribe_flush(lListElem *event_client, ev_event event, int flush);

bool ec_unsubscribe(lListElem *event_client, ev_event event);
bool ec_unsubscribe_all(lListElem *event_client);

int ec_get_flush(lListElem *event_client, ev_event event);
bool ec_set_flush(lListElem *event_client, ev_event event, bool flush, int interval);
bool ec_unset_flush(lListElem *event_client, ev_event event);

bool ec_mod_subscription_where(lListElem *event_client, ev_event event, const lListElem *what, const lListElem *where);



int ec_set_edtime(lListElem *event_client, int intval);
int ec_get_edtime(lListElem *event_client);

bool ec_set_busy_handling(lListElem *event_client, ev_busy_handling handling);
ev_busy_handling ec_get_busy_handling(lListElem *event_client);

bool ec_set_flush_delay(lListElem *event_client, int flush_delay);
int ec_get_flush_delay(lListElem *event_client);

bool ec_set_busy(lListElem *event_client, int busy);
bool ec_get_busy(lListElem *event_client);

bool ec_set_session(lListElem *event_client, const char *session);
const char *ec_get_session(lListElem *event_client);

ev_registration_id ec_get_id(lListElem *event_client);

bool ec_commit(lListElem *event_client);
bool ec_commit_multi(lListElem *event_client, lList **malp, state_gdi_multi *state);


bool ec_get(lListElem *event_client, lList **event_list, bool exit_on_qmaster_down);

void ec_mark4registration(lListElem *event_client);
bool ec_need_new_registration(void);




void ec_init_local(evm_mod_func_t mod_func, evm_add_func_t add_func,
                   evm_remove_func_t remove_func, evm_ack_func_t ack_func);

bool ec_commit_local(lListElem *event_client, event_client_update_func_t update_func); 
bool ec_deregister_local(lListElem **event_client);
bool ec_register_local(lListElem *event_client, lList **alpp, event_client_update_func_t update_func, monitoring_t *monitor);
bool ec_ack_local(lListElem *event_client);

#endif /* TEST_GDI2 */


#endif /* __SGE_C_EVENT_H */

