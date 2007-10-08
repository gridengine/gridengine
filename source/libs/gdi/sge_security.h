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
#include "sge_gdiP.h"
#include "sge_qmaster_timed_event.h"


#ifdef KERBEROS
#   include "krb_lib.h"
#endif



void sge_security_exit(int i);

#ifdef SECURE
/* int 0 on success, -1 on failure */
int sge_ssl_setup_security_path(const char *progname, const char *username);
#endif


#ifdef KERBEROS
int kerb_job(lListElem *jelem, struct dispatch_entry *de);
#endif

void tgt2cc(lListElem *jep, const char *rhost, const char* target);
void tgtcclr(lListElem *jep, const char *rhost, const char* target);
int set_sec_cred(const char *sge_root, const char *mastername, lListElem *job, lList **alpp);
void delete_credentials(const char *sge_root, lListElem *jep);
bool cache_sec_cred(const char *sge_root, lListElem *jep, const char *rhost);
int store_sec_cred(const char *sge_root, sge_gdi_request *request, lListElem *jep, int do_authentication, lList **alpp);
int store_sec_cred2(const char* sge_root, 
                    const char* unqualified_hostname, 
                    lListElem *jelem, 
                    int do_authentication, 
                    int *general, 
                    dstring *err_str);

int sge_set_auth_info(sge_gdi_request *request, 
                      uid_t uid, const char *user, 
                      gid_t gid, const char *group);

int sge_get_auth_info(sge_gdi_request *request, 
                      uid_t *uid, char *user, size_t user_len,
                      gid_t *gid, char *group, size_t group_len);

int sge_security_verify_user(const char *host, const char *commproc, u_long32 id,
                             const char *admin_user, const char *user, const char *progname); 

bool sge_security_verify_unique_identifier(bool check_admin_user, 
                                           const char* user, 
                                           const char* progname,
                                           unsigned long progid, 
                                           const char* hostname, 
                                           const char* commproc, 
                                           unsigned long commid);

void sge_security_event_handler(sge_gdi_ctx_class_t *ctx, te_event_t anEvent, monitoring_t *monitor);


#endif /* __SGE_SECURITY_H */

