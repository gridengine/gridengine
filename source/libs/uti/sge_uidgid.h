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

#include <stdio.h>
#include <sys/types.h> 
#include <grp.h>
#include <pwd.h>

#include "sge_dstring.h"
#include "sge_unistd.h"

#if defined(HPUX)
#  define seteuid(euid) setresuid(-1, euid, -1)
#  define setegid(egid) setresgid(-1, egid, -1)
#endif 

#if defined(INTERIX) && !defined(INTERIX52)
#  define seteuid(euid) setreuid(-1, euid)
#  define setegid(egid) setregid(-1, egid)
#  define SGE_SUPERUSER_UID wl_get_superuser_id()
#  define SGE_SUPERUSER_GID wl_get_superuser_gid() 
#else
#  define SGE_SUPERUSER_UID 0
#  define SGE_SUPERUSER_GID 0
#endif

#ifndef MAX_NIS_RETRIES
#  define MAX_NIS_RETRIES 10
#endif    

void uidgid_mt_init(void);

bool sge_is_start_user_superuser(void);
int password_read_file(char **users[], char **encryped_pwds[], const char *filename);
const char* sge_get_file_passwd(void);

int  sge_set_admin_username(const char *username, char *err_str);
bool sge_is_admin_user(const char *username);
const char *get_admin_user_name(void); 
int sge_switch2admin_user(void);
int sge_switch2start_user(void);
bool sge_has_admin_user(void);
int sge_run_as_user(void);
int sge_user2uid(const char *user, uid_t *puid, gid_t *pgid, int retries);  
int sge_group2gid(const char *gname, gid_t *gidp, int retries);
int sge_uid2user(uid_t uid, char *dst, size_t sz, int retries); 
int sge_gid2group(gid_t gid, char *dst, size_t sz, int retries);
int _sge_gid2group(gid_t gid, gid_t *last_gid, char **grpnamep, int retries);
int sge_add_group(gid_t newgid, char *err_str); 
int sge_set_uid_gid_addgrp(const char *user, const char *intermediate_user,
                           int min_gid, int min_uid, int add_grp, 
                           char *err_str, int use_qsub_gid, gid_t qsub_gid);

struct passwd *sge_getpwnam_r(const char *name, struct passwd *pw, 
                              char *buffer, size_t bufsize);
struct group *sge_getgrgid_r(gid_t gid, struct group *pg, 
                             char *buffer, size_t bufsize, int retries);

bool sge_is_user_superuser(const char *name); 

/* getting buffer sizes for getpwnam_r etc. */
int get_group_buffer_size(void);
int get_pw_buffer_size(void);

/*
 * Deprecated functions. Do not use anymore!
 */
int sge_set_uid_gid_addgrp(const char *user, const char *intermediate_user,
                           int min_gid, int min_uid, int add_grp, 
                           char *err_str, int use_qsub_gid, gid_t qsub_gid);
#if defined(INTERIX)
int uidgid_read_passwd(const char *user, char **pass, char *err_str);
#endif

#ifdef SGE_THREADSAFE_UTIL

#include <grp.h>

int getpwnam_r(const char *, struct passwd *, char *, size_t, struct passwd **);
int getgrnam_r(const char *, struct group *,  char *, size_t, struct group **);
int getpwuid_r(uid_t,  struct passwd *, char *, size_t, struct passwd **);
int getgrgid_r(gid_t , struct group *,  char *, size_t, struct group **);

#endif


#endif /* __SGE_UIDGID_H */

