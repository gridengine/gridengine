#ifndef __SGE_QMOD_QMASTER_H
#define __SGE_QMOD_QMASTER_H
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



#ifndef __SGE_GDI__INTERN_H
#   include "sge_gdi_intern.h"
#endif

void resend_signal_event(u_long32 type, u_long32 when, u_long32 jobid, u_long32 jataskid, char *queue);
void rebuild_signal_events(void);
void sge_gdi_qmod(char *host, sge_gdi_request *request, sge_gdi_request *answer);

int sge_signal_queue(int how, lListElem *qep, lListElem *jep, lListElem *jatep);
void signal_on_calendar(lListElem *qep, u_long32 old_state, u_long32 new_state);
int queue_initial_state(lListElem *qep, char *rhost);


#endif /* __SGE_QMOD_QMASTER_H */

