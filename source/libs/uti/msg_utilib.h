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
** sge_text() messages (more than once used)
*/
#define MSG_SGETEXT_COMMPROC_ALREADY_STARTED_S  _(""SFN" is already running\n")
#define MSG_SGETEXT_NOCOMMD                     _("can't connect commd.\n")
#define MSG_SGETEXT_NOMEM                       _("out of memory\n")
#define MSG_SGETEXT_CANT_OPEN_SS                      _("can't open "SFQ" ("SFN")\n")
#define MSG_SGETEXT_NULLPTRPASSED_S             _("NULL ptr passed to "SFN"()\n")
#define MSG_SGETEXT_MISSINGCULLFIELD_SS         _("missing cull field "SFQ" in "SFN"()\n")

/* 
** utilib/sge_afsutil.c
*/ 
#define MSG_TOKEN_NOSTART_S         _("can't start set_token_command \"%s\"")
#define MSG_TOKEN_NOWRITEAFS_S      _("can't write AFS token to set_token_command \"%s\"")
#define MSG_TOKEN_NOSETAFS_SI       _("failed to set AFS token - set_token_command \"%s\" returned with exit status %d")
#define MSG_COMMAND_NOPATHFORTOKEN  _("can't get path for command to get AFS token\n")
#define MSG_COMMAND_NOFILESTATUS_S  _("can't determine file status of command \"%s\"\n")
#define MSG_COMMAND_NOTEXECUTABLE_S _("command \"%s\" is not executable\n")

/*
** utilib/sge_arch.c
*/       
#define MSG_SGEROOTNOTSET           _("Please set the environment variable SGE_ROOT.\n")
#define MSG_SGEGRDROOTNOTEQUIV      _("SGE_ROOT and GRD_ROOT are not equivalent\n")
#define MSG_SGECODINEROOTNOTEQUIV   _("SGE_ROOT and CODINE_ROOT are not equivalent\n")
#define MSG_GRDCODINEROOTNOTEQUIV   _("GRD_ROOT and CODINE_ROOT are not equivalent\n")
#define MSG_UNKNOWNERRORINSGEROOT   _("Unknown error in function sge_sge_root()\n")


/* 
** utilib/sge_nprocs.c
*/ 
#define MSG_PERROR_PSTATDYNAMIC     _("Pstat: PSTAT_DYNAMIC")
#define MSG_INFO_NUMBOFPROCESSORS_I _("Number of Processors '%d'\n")
/* 
** utilib/sge_bitop.c
*/ 
#define MSG_MEMORY_NOMEMORYFORBYTEARRAY_S    _("%s: can't malloc memory for byte array\n")
#define MSG_FILE_NOOPENFORWRITEING_SS        _("%s: unable to open file %s for writing!\n")


/* 
** utilib/sge_chdir.c
*/ 
#define MSG_FILE_NOCDTODIRECTORY_S           _("can't change to directory \"%s\"")


/* 
** utilib/sge_daemonize.c
*/ 
#define MSG_PROC_FIRSTFORKFAILED_S           _("1st fork() failed while daemonizing: %s\n")
#define MSG_PROC_SECONDFORKFAILED_S          _("2nd fork() failed while daemonizing: %s\n")

/* 
** utilib/sge_dir.c
*/ 
#define MSG_POINTER_NULLPARAMETER            _("NULL parameter")
#define MSG_FILE_OPENDIRFAILED_SS            _("opendir(%s) failed: %s\n")
#define MSG_FILE_STATFAILED_SS               _("stat(%s) failed: %s\n")
#define MSG_FILE_RECURSIVERMDIRFAILED        _("==================== recursive_rmdir() failed\n")
#define MSG_FILE_UNLINKFAILED_SS             _("unlink(%s) failed: %s\n")
#define MSG_FILE_RMDIRFAILED_SS              _("rmdir(%s) failed: %s\n")


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
#define MSG_SYSTEM_TABINFO_FAILED_SS         _("tabinfo(\"%s\", ...) failed, %s\n")            
#define MSG_MEMORY_MALLOCFAILED_D            _("malloc("U32CFormat") failed" )


/* 
** utilib/sge_log.c
*/ 
#define MSG_LOG_CRITICALERROR                _("critical error: ")
#define MSG_LOG_ERROR                        _("error: ")
#define MSG_LOG_CALLEDLOGGINGSTRING_S        _("logging called with %s logging string")
#define MSG_LOG_ZEROLENGTH                   _("zero length")
#define MSG_POINTER_NULL                     _("NULL")


/* 
** utilib/sge_mkdir.c
*/ 
#define MSG_VAR_PATHISNULLINSGEMKDIR            _("path == NULL in sge_mkdir()")
#define MSG_FILE_CREATEDIRFAILED_SS             _("can't create directory \"%s\": %s\n")


/* 
** utilib/sge_peopen.c
*/ 
#define MSG_SYSTEM_EXECBINSHFAILED              _("can't exec /bin/sh\n")
#define MSG_SYSTEM_NOROOTRIGHTSTOSWITCHUSER     _("you have to be root to become another user\n" )
#define MSG_SYSTEM_NOUSERFOUND_SS               _("can't get user %s: %s\n")
#define MSG_SYSTEM_INITGROUPSFORUSERFAILED_ISS  _("res = %d, can't initialize groups for user %s: %s\n")
#define MSG_SYSTEM_SWITCHTOUSERFAILED_SS        _("can't change to user %s: %s\n")
#define MSG_SYSTEM_FAILOPENPIPES_SS             _("failed opening pipes for %s: %s\n")


/* 
** utilib/sge_processes_irix.c
*/ 
#define MSG_FILE_OPENFAILED_SS                  _("failed opening %s: %s")
#define MSG_SYSTEM_GETPIDSFAILED_S              _("getpidsOfJob: ioctl(%s, PIOCSTATUS) failed\n")
#define MSG_PROC_KILL_IIS                       _("kill(%d, %d): %s")
#define MSG_PROC_KILLISSUED_II                  _("kill(%d, %d) issued")


/* 
** utilib/sge_put_get_file.c
*/ 
#define MSG_FILE_WRITEOPENFAILED_SS             _("couldn't open %s for writing: %s\n")
#define MSG_FILE_READOPENFAILED_SS              _("couldn't open %s for reading: %s")

/* 
** utilib/sge_set_def_sig_mask.c
*/ 
#define MSG_PROC_SIGACTIONFAILED_IS             _("sigaction for signal %d failed: %s")


/* 
** utilib/sge_set_uid_gid.c
*/ 
#define MSG_SYSTEM_CHANGEUIDORGIDFAILED         _("tried to change uid/gid without being root")
#define MSG_SYSTEM_GIDLESSTHANMINIMUM_SUI       _("gid of user %s ("gid_t_fmt") less than minimum allowed in conf (%d)")
#define MSG_SYSTEM_UIDLESSTHANMINIMUM_SUI       _("uid of user %s ("uid_t_fmt") less than minimum allowed in conf (%d)")
#define MSG_SYSTEM_SETGIDFAILED_U               _("setgid("gid_t_fmt") failed")
#define MSG_SYSTEM_SETUIDFAILED_U               _("setuid("uid_t_fmt") failed")
#define MSG_SYSTEM_SETEGIDFAILED_U              _("setegid("gid_t_fmt") failed")
#define MSG_SYSTEM_SETEUIDFAILED_U              _("seteuid("uid_t_fmt") failed")
#define MSG_SYSTEM_INITGROUPSFAILED_I           _("initgroups() failed with errno %d\n")
#define MSG_SYSTEM_ADDGROUPIDFORSGEFAILED_UUS   _("can't set additional group id (uid="uid_t_fmt", euid="uid_t_fmt"): "SFN"\n")
#define MSG_SYSTEM_INVALID_NGROUPS_MAX          _("invalid value for NGROUPS_MAX")
#define MSG_SYSTEM_USER_HAS_TOO_MANY_GIDS       _("the user already has too many group ids")

/* 
** utilib/sge_signal.c
*/ 
#define MSG_PROC_UNKNOWNSIGNAL                  _("unknown signal")


/* 
** utilib/sge_str_from_file.c
*/ 
#define MSG_FILE_FOPENFAILED_SS                 _("fopen("SFQ") failed: %s\n")
#define MSG_FILE_FREADFAILED_SS                 _("fread("SFQ") failed: %s\n")
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
#define MSG_SYSTEM_ADMINUSERNOTEXIST_S          _("admin_user \"%s\" does not exist")
#define MSG_SYSTEM_CANTRUNASCALLINGUSER         _("cannot run as calling user")


/* 
** utilib/sge_sysconf.c
*/
#define MSG_SYSCONF_UNABLETORETRIEVE_I                _("unable to retrieve value for system limit (%d)\n")   

/*
**  former SGE_TEXT:
*/
#define MSG_SGETEXT_SHOULD_BE_ROOT_S            _("should be root to start "SFN"\n")
/* 
#define MSG_SGETEXT_INVALIDHOSTINQUEUE_S        _("invalid hostname "SFQ" associated with queue\n")
#define MSG_SGETEXT_MODIFIEDINLIST_SSUS         _(""SFN"@"SFN" modified \"" U32CFormat "\" in "SFN" list\n")
#define MSG_SGETEXT_CANTRESOLVEUSER_S           _("unknown user name "SFQ"\n")   
#define MSG_SGETEXT_CANTRESOLVEGROUP_S          _("unknown group name "SFQ"\n")  
#define MSG_SGETEXT_NOCOMMD_SS                  _("unable to contact commd at host "SFN" using service "SFN"\n")
#define MSG_SGETEXT_NOPERM                      _("no permissions for this operation\n")
#define MSG_SGETEXT_CANTFINDACL_S               _("unable to find referenced access list "SFQ"\n")
#define MSG_SGETEXT_SHOULD_BE_ROOT_S            _("should be root to start "SFN"\n")
#define MSG_SGETEXT_STILL_REFERENCED_SS         _("remove reference to "SFQ" in subordinates of queue "SFQ" before deletion\n") 
#define MSG_SGETEXT_NO_SECURITY_LEVEL_FOR_S           _("denied: missing security level for "SFN"\n")
#define MSG_SGETEXT_MAY_NOT_CHG_QHOST_S               _("may not change host of queue "SFQ"\n")
#define MSG_SGETEXT_UP_REFERENCED_TWICE_SS            _("denied: share tree contains reference to unknown "SFN" "SFQ"\n")   
#define MSG_SGETEXT_NO_PROJECT                        _("job rejected: no project assigned to job\n")     
#define MSG_SGETEXT_UNABLETORETRIEVE_I                _("unable to retrieve value for system limit (%d)\n")     

#define MSG_SYSTEM_INVALIDERRORNUMBER                 _("invalid error number")
#define MSG_SYSTEM_GOTNULLASERRORTEXT                 _("no error text available")
*/


/* 
** utilib/sge_uidgid.c
*/ 
#define MSG_SYSTEM_GETPWUIDFAILED_US               _("getpwuid("pid_t_fmt") failed: %s\n")
#define MSG_SYSTEM_GETGRGIDFAILED_US               _("getgrgid("pid_t_fmt") failed: %s\n")

#endif /* __MSG_UTILIB_H */

