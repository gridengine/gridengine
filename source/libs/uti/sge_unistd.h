#ifndef __SGE_UNISTD_H
#define __SGE_UNISTD_H
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

#include <unistd.h>
#include <dirent.h>      
#include <sys/stat.h> 

#include "basis_types.h"  

#include "sge_dstring.h"

#if defined(INTERIX)
#  include "../wingrid/wingrid.h"
#endif

#if defined(SOLARIS) || defined(LINUX) || defined(IRIX)
#  define SGE_OPEN2(filename, oflag)       open64(filename, oflag)
#  define SGE_OPEN3(filename, oflag, mode) open64(filename, oflag, mode)
#else
#  define SGE_OPEN2(filename, oflag)       open(filename, oflag)
#  define SGE_OPEN3(filename, oflag, mode) open(filename, oflag, mode)
#endif

#if defined (IRIX)
#  define SGE_CLOSE(fd) fsync(fd); close(fd)
#else
#  define SGE_CLOSE(fd) close(fd);
#endif


#ifdef IRIX
#  define SGE_STAT(filename, buffer) stat64(filename, buffer)
#  define SGE_LSTAT(filename, buffer) lstat64(filename, buffer)
#  define SGE_FSTAT(filename, buffer) fstat64(filename, buffer)
#  define SGE_STRUCT_STAT struct stat64
#  define SGE_INO_T ino64_t
#  define SGE_OFF_T off64_t
#elif defined(SOLARIS)
#  define SGE_STAT(filename, buffer) stat64(filename, buffer)
#  define SGE_LSTAT(filename, buffer) lstat64(filename, buffer)
#  define SGE_FSTAT(filename, buffer) fstat64(filename, buffer)
#  define SGE_STRUCT_STAT struct stat64
#  define SGE_INO_T ino64_t
#  define SGE_OFF_T off64_t
#elif defined(INTERIX)
#  define SGE_STAT(filename, buffer) wl_stat(filename, buffer)
#  define SGE_LSTAT(filename, buffer) lstat(filename, buffer)
#  define SGE_FSTAT(filedes, buffer) fstat(filedes, buffer)
#  define SGE_STRUCT_STAT struct stat
#  define SGE_INO_T ino_t
#  define SGE_OFF_T off_t
#else
#  define SGE_STAT(filename, buffer) stat(filename, buffer)
#  define SGE_LSTAT(filename, buffer) lstat(filename, buffer)
#  define SGE_FSTAT(filename, buffer) fstat(filename, buffer)
#  define SGE_STRUCT_STAT struct stat
#  define SGE_INO_T ino_t
#  define SGE_OFF_T off_t
#endif                

#if defined(IRIX) || defined(SOLARIS) || defined(LINUX)
#  define SGE_READDIR(directory) readdir64(directory)
#  define SGE_READDIR_R(directory, entry, result) readdir64_r(directory, entry, result)
#  define SGE_TELLDIR(directory) telldir64(directory)
#  define SGE_SEEKDIR(directory, offset) seekdir64(directory, offset)
#  define SGE_STRUCT_DIRENT struct dirent64
#else
#  define SGE_READDIR(directory) readdir(directory)
#  define SGE_READDIR_R(directory, entry, result) readdir_r(directory, entry, result)
#  define SGE_TELLDIR(directory) telldir(directory)
#  define SGE_SEEKDIR(directory, offset) seekdir(directory, offset)
#  define SGE_STRUCT_DIRENT struct dirent
#endif       

#if defined(_UNICOS) || defined(SOLARIS) || defined(__hpux) || defined(LINUX) || defined(AIX) || defined(SINIX) || defined(NECSX4) || defined(NECSX5) || defined(DARWIN9) || defined(DARWIN10)
#   define SETPGRP setpgrp()
#elif defined(__sgi)
#   define SETPGRP BSDsetpgrp(getpid(),getpid())
#elif defined(WIN32) || defined(INTERIX)
#   define SETPGRP setsid()
#else
#   define SETPGRP setpgrp(getpid(),getpid())
#endif

#define GETPGRP getpgrp()

void sge_exit(void **ctx_ref, int i);

int sge_chdir_exit(const char *path, int exit_on_error);  

int sge_chdir(const char *dir);

int sge_mkdir(const char *path, int fmode, int exit_on_error, int may_not_exist);    
int sge_mkdir2(const char *base_dir, const char *name, int fmode, 
               int exit_on_error);    

int sge_rmdir(const char *cp, dstring *err_str);

bool sge_unlink(const char *prefix, const char *suffix); 
 
int sge_is_directory(const char *name);
 
int sge_is_file(const char *name);

int sge_is_executable(const char *name); 


void sge_sleep(int sec, int usec);

/****** uti/unistd/sge_sysconf_t **********************************************
*  NAME
*     sge_sysconf_t -- Constants for sge_sysconf() 
*
*  SYNOPSIS
*     typedef enum {
*        SGE_SYSCONF_NGROUPS_MAX
*     } sge_sysconf_t;  
*
*  FUNCTION
*     SGE_SYSCONF_NGROUPS_MAX - Maximum number of additional group ids
*                               which are allowed per user  
*
*  SEE ALSO
*     uti/unistd/sge_sysconf()
******************************************************************************/
typedef enum {
   SGE_SYSCONF_NGROUPS_MAX
} sge_sysconf_t;
 
u_long32 sge_sysconf(sge_sysconf_t id); 

#endif /* __SGE_UNISTD_H */
