#ifndef __SGE_M_EVENT_H
#define __SGE_M_EVENT_H
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

#include "basis_types.h"
#include "sge_eventL.h"


extern int    sge_add_event_client(lListElem*, lList**, lList**, char*, char*);
extern int    sge_mod_event_client(lListElem*, lList**, lList**, char*, char*);
extern bool   sge_event_client_registered(u_long32);
extern void   sge_remove_event_client(u_long32);
extern lList* sge_select_event_clients(const char*, const lCondition*, const lEnumeration*);
extern int    sge_shutdown_event_client(u_long32, const char*, uid_t, lList **alpp);
extern int    sge_shutdown_dynamic_event_clients(const char*, lList **alpp);

extern u_long32 sge_get_event_client_data(u_long32);
extern int      sge_set_event_client_data(u_long32, u_long32);

extern bool sge_add_event( u_long32, ev_event, u_long32, u_long32, 
                          const char*, const char*, const char*, lListElem*);
                          
extern bool sge_add_event_for_client(u_long32, u_long32, ev_event, u_long32, u_long32, 
                                    const char*, const char*, const char*, lListElem*);
                                    
extern bool sge_add_list_event( u_long32, ev_event, u_long32, u_long32, 
                               const char*, const char*, const char*, lList*); 

extern void sge_handle_event_ack(u_long32, ev_event);
extern void sge_deliver_events_immediately(u_long32);

extern u_long32 sge_get_next_event_number(u_long32);
extern int      sge_resync_schedd(void);

extern u_long32 sge_set_max_dynamic_event_clients(u_long32 max);
extern u_long32 sge_get_max_dynamic_event_clients(void);

extern void sge_event_shutdown(void);
extern bool sge_commit(void);
extern void sge_set_commit_required(void);
extern bool sge_is_commit_required(void);

#endif /* __SGE_M_EVENT_H */
