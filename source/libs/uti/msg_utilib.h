#ifndef __MSG_UTILIB_H
#define __MSG_UTILIB_H
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

#include "basis_types.h"


/* 
** utilib/sge_afsutil.c
*/ 
#define MSG_TOKEN_NOSTART_S         _("can't start set_token_command "SFQ)
#define MSG_TOKEN_NOWRITEAFS_S      _("can't write AFS token to set_token_command "SFQ)
#define MSG_TOKEN_NOSETAFS_SI       _("failed to set AFS token - set_token_command "SFQ" returned with exit status %d")
#define MSG_COMMAND_NOPATHFORTOKEN  _("can't get path for command to get AFS token\n")
#define MSG_COMMAND_NOFILESTATUS_S  _("can't determine file status of command "SFQ"\n")
#define MSG_COMMAND_NOTEXECUTABLE_S _("command "SFQ" is not executable\n")

/*
** utilib/sge_arch.c
*/       
#define MSG_SGEROOTNOTSET           _("Please set the environment variable SGE_ROOT.\n")
#define MSG_SGEGRDROOTNOTEQUIV      _("SGE_ROOT and GRD_ROOT are not equivalent\n")
#define MSG_SGECODINEROOTNOTEQUIV   _("SGE_ROOT and CODINE_ROOT are not equivalent\n")
#define MSG_GRDCODINEROOTNOTEQUIV   _("GRD_ROOT and CODINE_ROOT are not equivalent\n")
#define MSG_UNKNOWNERRORINSGEROOT   _("Unknown error in function sge_sge_root()\n")
#define MSG_MEMORY_MALLOCFAILEDFORPATHTOHOSTALIASFILE _("can't malloc() for path to host alias file")

/* 
** utilib/sge_nprocs.c
*/ 
#define MSG_PERROR_PSTATDYNAMIC     _("Pstat: PSTAT_DYNAMIC")
#define MSG_INFO_NUMBOFPROCESSORS_I _("Number of Processors '%d'\n")
/* 
** utilib/sge_bitop.c
*/ 
#define MSG_MEMORY_NOMEMORYFORBYTEARRAY_S    _(SFN": can't malloc memory for byte array\n")
#define MSG_FILE_NOOPENFORWRITEING_SS        _(SFN": unable to open file "SFN" for writing!\n")


/* 
** utilib/sge_unistd.c
*/ 
#define MSG_FILE_NOCDTODIRECTORY_S           _("can't change to directory "SFQ)


/* 
** utilib/sge_daemonize.c
*/ 
#define MSG_PROC_FIRSTFORKFAILED_S           _("1st fork() failed while daemonizing: "SFN"\n")
#define MSG_PROC_SECONDFORKFAILED_S          _("2nd fork() failed while daemonizing: "SFN"\n")

/* 
** utilib/sge_dir.c
*/ 
#define MSG_POINTER_NULLPARAMETER            _("NULL parameter")
#define MSG_FILE_OPENDIRFAILED_SS            _("opendir("SFN") failed: "SFN"\n")
#define MSG_FILE_STATFAILED_SS               _("stat("SFN") failed: "SFN"\n")
#define MSG_FILE_RECURSIVERMDIRFAILED        _("==================== recursive_rmdir() failed\n")
#define MSG_FILE_UNLINKFAILED_SS             _("unlink("SFN") failed: "SFN"\n")
#define MSG_FILE_RMDIRFAILED_SS              _("rmdir("SFN") failed: "SFN"\n")


/* 
** utilib/sge_getloadavg.c
*/ 
#define MSG_PROCESSOR_SETNOTFOUND_I          _("processor set %d not found.\n")
#define MSG_PROCESSOR_TOTALLOADAVG_F         _("total load_avg %2.2f\n")  
#define MSG_PROCESSOR_KNLISTFAILED           _("Can't knlist()\n")
#define MSG_PROCESSOR_KMEMFAILED             _("Can't read kmem\n")
#define MSG_PROCESSOR_NLISTFAILED            _("Can't nlist()\n")


/* 
** utilib/sge_loadmem.c
*/ 
#define MSG_SYSTEM_NOPAGESIZEASSUME8192      _("can't determine system page size - assuming 8192")
#define MSG_SYSTEM_TABINFO_FAILED_SS         _("tabinfo("SFQ", ...) failed, "SFN"\n")            
#define MSG_MEMORY_MALLOCFAILED_D            _("malloc("U32CFormat") failed" )


/* 
** utilib/sge_log.c
*/ 
#define MSG_LOG_CRITICALERROR                _("critical error: ")
#define MSG_LOG_ERROR                        _("error: ")
#define MSG_LOG_CALLEDLOGGINGSTRING_S        _("logging called with "SFN" logging string")
#define MSG_LOG_ZEROLENGTH                   _("zero length")
#define MSG_POINTER_NULL                     _("NULL")

/* 
** utilib/sge_peopen.c
*/ 
#define MSG_SYSTEM_EXECBINSHFAILED              _("can't exec /bin/sh\n")
#define MSG_SYSTEM_NOROOTRIGHTSTOSWITCHUSER     _("you have to be root to become another user\n" )
#define MSG_SYSTEM_NOUSERFOUND_SS               _("can't get user "SFN": "SFN"\n")
#define MSG_SYSTEM_INITGROUPSFORUSERFAILED_ISS  _("res = %d, can't initialize groups for user "SFN": "SFN"\n")
#define MSG_SYSTEM_SWITCHTOUSERFAILED_SS        _("can't change to user "SFN": "SFN"\n")
#define MSG_SYSTEM_FAILOPENPIPES_SS             _("failed opening pipes for "SFN": "SFN"\n")


/* 
** utilib/sge_processes_irix.c
*/ 
#define MSG_FILE_OPENFAILED_SS                  _("failed opening "SFN": "SFN"")
#define MSG_SYSTEM_GETPIDSFAILED_S              _("getpidsOfJob: ioctl("SFN", PIOCSTATUS) failed\n")
#define MSG_PROC_KILL_IIS                       _("kill(%d, %d): "SFN"")
#define MSG_PROC_KILLISSUED_II                  _("kill(%d, %d) issued")


/* 
** utilib/sge_put_get_file.c
*/ 
#define MSG_FILE_WRITEOPENFAILED_SS             _("couldn't open "SFN" for writing: "SFN"\n")
#define MSG_FILE_READOPENFAILED_SS              _("couldn't open "SFN" for reading: "SFN"")


/* 
** utilib/sge_signal.c
*/ 
#define MSG_PROC_UNKNOWNSIGNAL                  _("unknown signal")
#define MSG_PROC_SIGACTIONFAILED_IS             _("sigaction for signal %d failed: "SFN"")


/* 
** utilib/sge_str_from_file.c
*/ 
#define MSG_FILE_FOPENFAILED_SS                 _("fopen("SFQ") failed: "SFN"\n")
#define MSG_FILE_FREADFAILED_SS                 _("fread("SFQ") failed: "SFN"\n")
#define MSG_FILE_OPENFAILED_S                   _("cant open file "SFQ"\n")
#define MSG_FILE_WRITEBYTESFAILED_IS            _("cant write %d bytes into file "SFQ"\n")

/* 
** utilib/sge_string.c
*/ 
#define MSG_POINTER_INVALIDSTRTOKCALL           _("Invalid sge_strtok_r call, last is not NULL\n")

/* 
** utilib/sge_switch_user.c
*/ 
#define MSG_POINTER_SETADMINUSERNAMEFAILED      _("set_admin_username() with zero length username")
#define MSG_SYSTEM_ADMINUSERNOTEXIST_S          _("admin_user "SFQ" does not exist")
#define MSG_SWITCH_USER_NOT_INITIALIZED         _("Module 'sge_switch_user' not initialized")
#define MSG_SWITCH_USER_NOT_ROOT                _("User 'root' did not start the application\n")


/* 
** utilib/sge_sysconf.c
*/
#define MSG_SYSCONF_UNABLETORETRIEVE_I                _("unable to retrieve value for system limit (%d)\n")   

/*
**  former SGE_TEXT:
*/
#define MSG_SGETEXT_SHOULD_BE_ROOT_S            _("should be root to start "SFN"\n")

/* 
** utilib/sge_uidgid.c
*/ 
#define MSG_SYSTEM_GETPWUIDFAILED_US               _("getpwuid("pid_t_fmt") failed: "SFN"\n")
#define MSG_SYSTEM_GETGRGIDFAILED_US               _("getgrgid("pid_t_fmt") failed: "SFN"\n")
#define MSG_SYSTEM_CHANGEUIDORGIDFAILED         _("tried to change uid/gid without being root")
#define MSG_SYSTEM_GIDLESSTHANMINIMUM_SUI       _("gid of user "SFN" ("gid_t_fmt") less than minimum allowed in conf (%d)")
#define MSG_SYSTEM_UIDLESSTHANMINIMUM_SUI       _("uid of user "SFN" ("uid_t_fmt") less than minimum allowed in conf (%d)")
#define MSG_SYSTEM_SETGIDFAILED_U               _("setgid("gid_t_fmt") failed")
#define MSG_SYSTEM_SETUIDFAILED_U               _("setuid("uid_t_fmt") failed")
#define MSG_SYSTEM_SETEGIDFAILED_U              _("setegid("gid_t_fmt") failed")
#define MSG_SYSTEM_SETEUIDFAILED_U              _("seteuid("uid_t_fmt") failed")
#define MSG_SYSTEM_INITGROUPSFAILED_I           _("initgroups() failed with errno %d\n")
#define MSG_SYSTEM_ADDGROUPIDFORSGEFAILED_UUS   _("can't set additional group id (uid="uid_t_fmt", euid="uid_t_fmt"): "SFN"\n")
#define MSG_SYSTEM_INVALID_NGROUPS_MAX          _("invalid value for NGROUPS_MAX")
#define MSG_SYSTEM_USER_HAS_TOO_MANY_GIDS       _("the user already has too many group ids")

/* 
** utilib/sge_stdlib.c
*/ 
#define MSG_MEMORY_MALLOCFAILED                    _("malloc() failure\n")
#define MSG_MEMORY_REALLOCFAILED                   _("realloc() failure\n") 
#define MSG_GDI_STRING_LENGTHEXCEEDED_SI           _("strlen($"SFN") exceeds MAX_STRING_SIZE %d\n") 

/* 
** utilib/sge_unistd.c
*/ 
#define MSG_POINTER_SUFFIXISNULLINSGEUNLINK        _("suffix == NULL in sge_unlink()\n") 
#define MSG_VAR_PATHISNULLINSGEMKDIR            _("path == NULL in sge_mkdir()")
#define MSG_FILE_CREATEDIRFAILED_SS             _("can't create directory "SFQ": "SFN"\n")

/*
** utilib/sge_hostname.c
*/
#define MSG_NET_GETHOSTNAMEFAILED                  _("gethostname failed")
#define MSG_NET_RESOLVINGLOCALHOSTFAILED           _("failed resolving local host")

#endif /* __MSG_UTILIB_H */

