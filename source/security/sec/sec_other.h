#ifndef __SEC_OTHER_H
#define __SEC_OTHER_H
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

#include "sec_crypto.h"
#include "cull.h"

void sec_error(void);
int sec_send_err(char *commproc, int id, char *host, sge_pack_buffer *pb, char *err_msg);
int sec_set_connid(char **buffer, u_long32 *buflen);
int sec_get_connid(char **buffer, u_long32 *buflen);
int sec_update_connlist(char *host, char *commproc, int id);
int sec_set_secdata(char *host, char *commproc, int id);
int sec_insert_conn2list(char *host, char *commproc, int id);
void sec_clearup_list(void);
int sec_time_cmp(char *str);
void sec_keymat2list(lListElem *element);
void sec_list2keymat(lListElem *element);
void sec_print_bytes(FILE *f, int n, char *b);
int sec_set_verify_locations(char *file_env);
/* int sec_verify_callback(int ok, X509 *xs, X509 *xi, int depth, int error); */
void sec_des_ede3_cbc_init(u_char *key_material);
u_long32 sec_des_ede3_cbc(u_long32 len, u_char *from, u_char *to, int encrypt);
void sec_des_cbc_init(u_char *key_material);
u_long32 sec_des_cbc(u_long32 len, u_char *from, u_char *to, int encrypt);
void sec_md5_mac(u_long32 len, u_char *data, u_char *md);

#endif /* __SEC_OTHER_H */
