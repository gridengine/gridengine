#ifndef __TIME_EVENT_H
#define __TIME_EVENT_H
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
 *  License at http://www.gridengine.sunsource.net/license.html
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



#include "sge_time_eventL.h"

typedef void (*te_deliver_func_t)(u_long32 type, u_long32 when, u_long32 uval0, u_long32 uval1, char *sval);

typedef struct {
   u_long32 type;
   te_deliver_func_t func;
} te_tab_t;

void te_deliver(u_long32 now, te_tab_t *tab);
void te_add(u_long32 type, u_long32 when, u_long32 uval0, u_long32 uval1, char *sval);

/* removes all pending events with 'str' as key */
int te_delete(u_long32 type, char *str, u_long32 uval0, u_long32 uval1);


#endif /* __TIME_EVENT_H */

