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
#include "dispatcher.h"
#include "sec_lib.h"
#include "sge_qmaster_timed_event.h"


#ifdef KERBEROS
#   include "krb_lib.h"
#endif

int sge_security_initialize(const char *name);
void sge_security_exit(int i);

int gdi_receive_message(
char *fromcommproc,
u_short *fromid,
char *fromhost,
int *tag,
char **buffer,
u_long32 *buflen,
int synchron,
u_short *compressed 
);

int gdi_send_message(
int synchron,
const char *tocomproc,
int toid,
const char *tohost,
int tag,
char *buffer,
int buflen,
u_long32 *mid,
int compressed 
);

int set_sec_cred(lListElem *job);

void delete_credentials(lListElem *jep);

void cache_sec_cred(lListElem *jep, const char *rhost);

int store_sec_cred(sge_gdi_request *request, lListElem *jep, int do_authentication, lList **alpp);

int store_sec_cred2(lListElem *jelem, int do_authentication, int *general, char *err_str);

#ifdef KERBEROS
int kerb_job(lListElem *jelem, struct dispatch_entry *de);
#endif

void tgt2cc(lListElem *jep, const char *rhost, const char* target);

void tgtcclr(lListElem *jep, const char *rhost, const char* target);

int sge_set_auth_info(sge_gdi_request *request, uid_t uid, char *user, 
                        gid_t gid, char *group);

int sge_get_auth_info(sge_gdi_request *request, uid_t *uid, char *user, 
                        gid_t *gid, char *group);

int sge_security_verify_user(const char *host, const char *commproc, u_short id, const char *user); 

void sge_security_event_handler(te_event_t anEvent);

#endif /* __SGE_SECURITY_H */

