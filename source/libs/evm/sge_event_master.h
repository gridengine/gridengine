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

#include "sge_gdiP.h"
#include "sge_eventL.h"

extern lList *EV_Clients;
#ifdef QIDL
extern u_long32 qidl_event_count;
#endif

int reinit_event_client(ev_registration_id id);

int sge_add_event_client(lListElem *clio, lList **alpp, lList **eclpp, char *ruser, char *rhost);
int sge_mod_event_client(lListElem *clio, lList **alpp, lList **eclpp, char *ruser, char *rhost);

void sge_event_client_exit(const char *host, const char *commproc, sge_pack_buffer *pb);
void sge_gdi_kill_eventclient(const char *host, sge_gdi_request *request, sge_gdi_request *answer);

int sge_eventclient_subscribed(const lListElem *event_client, ev_event event);

int sge_ack_event(lListElem *er, ev_event event_number);
void ck_4_deliver_events(u_long32 now);

void sge_flush_events(lListElem *event_client, int interval);
int sge_next_flush(int);

void sge_add_list_event(lListElem *event_client, u_long32 timestamp,
                        ev_event type, u_long32 intkey, u_long32 intkey2, 
                        const char *strkey, lList *list); 

void sge_add_event(lListElem *event_client, u_long32 timestamp,
                   ev_event type, u_long32 intkey, u_long32 intkey2, 
                   const char *strkey, lListElem *element);

u_long32 sge_get_next_event_number(u_long32 client_id);

void sge_gdi_tsm(char *host, sge_gdi_request *request, sge_gdi_request *answer);

lListElem* eventclient_list_locate(ev_registration_id id);

void set_event_client_busy(lListElem *event_client, int busy);

#endif /* __SGE_M_EVENT_H */
