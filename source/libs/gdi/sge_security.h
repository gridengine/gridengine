#ifndef __SGE_SECURITY_H
#define __SGE_SECURITY_H
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

#include "cull.h"
#include "sge_gdi_intern.h"
#include "dispatcher.h"

#ifdef SECURE
#   include "sec_subst.h"
#   include "sec_lib.h"
#endif

#ifdef KERBEROS
#   include "krb_subst.h"
#   include "krb_lib.h"
#endif

int sge_security_initialize(char *name);
int set_sec_cred(lListElem *job);
void delete_credentials(lListElem *jep);
void cache_sec_cred(lListElem *jep, char *rhost);
int store_sec_cred(sge_gdi_request *request, lListElem *jep, int do_authentication, lList **alpp);
int store_sec_cred2(lListElem *jelem, int do_authentication, int *general, char *err_str);

#ifdef KERBEROS
int kerb_job(lListElem *jelem, struct dispatch_entry *de);
#endif

void tgt2cc(lListElem *jep, char *rhost, char* target);
void tgtcclr(lListElem *jep, char *rhost, char* target);


int sge_set_auth_info(sge_gdi_request *request, uid_t uid, char *user, 
                        gid_t gid, char *group);
int sge_get_auth_info(sge_gdi_request *request, uid_t *uid, char *user, 
                        gid_t *gid, char *group);

#endif /* __SGE_SECURITY_H */

