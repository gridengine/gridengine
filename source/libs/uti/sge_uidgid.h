#ifndef __SGE_UIDGID_H
#define __SGE_UIDGID_H
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

int sge_user2uid(const char *user, uid_t *uidp, int retries);  

int sge_group2gid(const char *gname, gid_t *gidp, int retries);

int sge_uid2user(uid_t uid, char *dst, size_t sz, int retries); 

int sge_gid2group(gid_t gid, char *dst, size_t sz, int retries);

int sge_set_uid_gid_addgrp(const char *user, const char *intermediate_user,
                           int min_gid, int min_uid, int add_grp, 
                           char *err_str, int use_qsub_gid, gid_t qsub_gid);
 
int sge_add_group(gid_t newgid); 
 
#endif /* __SGE_UIDGID_H */

