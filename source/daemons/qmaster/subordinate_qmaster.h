#ifndef __SUBORDINATE_QMASTER_H
#define __SUBORDINATE_QMASTER_H
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



int sos_using_gdil(lList *gdil, u_long32 jobid);
int usos_using_gdil(lList *gdil, u_long32 jobid);

int count_suspended_on_subordinate(lListElem *queueep); 

/* parameters for check_subordinate_list(how) */
enum { CHECK4ADD, CHECK4MOD, CHECK4SETUP }; 
int check_subordinate_list(lList **alpp, const char *qname, const char *host, 
                           u_long32 slots, lList *sol, int how);

int copy_suspended(lList **sol_out, lList *sol_in, int unused, int total, int suspended_on_subordinate);

int suspend_all(lList *sl, int recompute_cache); 
int unsuspend_all(lList *sl, int recompute_cache); 

#endif /* __SUBORDINATE_QMASTER_H */

