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
#define MSG_SGETEXT_COMMPROC_ALREADY_STARTED_S  _MESSAGE(49000, _(""SFN" is already running\n"))
#define MSG_SGETEXT_NOCOMMD                     _MESSAGE(49001, _("can't connect commd.\n"))
#define MSG_SGETEXT_NOMEM                       _MESSAGE(49002, _("out of memory\n"))
#define MSG_SGETEXT_CANT_OPEN_SS                      _MESSAGE(49003, _("can't open "SFQ" ("SFN")\n"))
#define MSG_SGETEXT_NULLPTRPASSED_S             _MESSAGE(49004, _("NULL ptr passed to "SFN"()\n"))
#define MSG_SGETEXT_MISSINGCULLFIELD_SS         _MESSAGE(49005, _("missing cull field "SFQ" in "SFN"()\n"))

/* 
** utilib/sge_afsutil.c
*/ 
#define MSG_TOKEN_NOSTART_S         _MESSAGE(49006, _("can't start set_token_command \"%s\""))
#define MSG_TOKEN_NOWRITEAFS_S      _MESSAGE(49007, _("can't write AFS token to set_token_command \"%s\""))
#define MSG_TOKEN_NOSETAFS_SI       _MESSAGE(49008, _("failed to set AFS token - set_token_command \"%s\" returned with exit status %d"))
#define MSG_COMMAND_NOPATHFORTOKEN  _MESSAGE(49009, _("can't get path for command to get AFS token\n"))
#define MSG_COMMAND_NOFILESTATUS_S  _MESSAGE(49010, _("can't determine file status of command \"%s\"\n"))
#define MSG_COMMAND_NOTEXECUTABLE_S _MESSAGE(49011, _("command \"%s\" is not executable\n"))

/*
** utilib/sge_arch.c
*/       
#define MSG_SGEROOTNOTSET           _MESSAGE(49012, _("Please set the environment variable SGE_ROOT.\n"))
#define MSG_SGEGRDROOTNOTEQUIV      _MESSAGE(49013, _("SGE_ROOT and GRD_ROOT are not equivalent\n"))
#define MSG_SGECODINEROOTNOTEQUIV   _MESSAGE(49014, _("SGE_ROOT and CODINE_ROOT are not equivalent\n"))
#define MSG_GRDCODINEROOTNOTEQUIV   _MESSAGE(49015, _("GRD_ROOT and CODINE_ROOT are not equivalent\n"))
#define MSG_UNKNOWNERRORINSGEROOT   _MESSAGE(49016, _("Unknown error in function sge_sge_root()\n"))


/* 
** utilib/sge_nprocs.c
*/ 
#define MSG_PERROR_PSTATDYNAMIC     _MESSAGE(49017, _("Pstat: PSTAT_DYNAMIC"))
#define MSG_INFO_NUMBOFPROCESSORS_I _MESSAGE(49018, _("Number of Processors '%d'\n"))
/* 
** utilib/sge_bitop.c
*/ 
#define MSG_MEMORY_NOMEMORYFORBYTEARRAY_S    _MESSAGE(49019, _("%s: can't malloc memory for byte array\n"))
#define MSG_FILE_NOOPENFORWRITEING_SS        _MESSAGE(49020, _("%s: unable to open file %s for writing!\n"))


/* 
** utilib/sge_chdir.c
*/ 
#define MSG_FILE_NOCDTODIRECTORY_S           _MESSAGE(49021, _("can't change to directory \"%s\""))


/* 
** utilib/sge_daemonize.c
*/ 
#define MSG_PROC_FIRSTFORKFAILED_S           _MESSAGE(49022, _("1st fork() failed while daemonizing: %s\n"))
#define MSG_PROC_SECONDFORKFAILED_S          _MESSAGE(49023, _("2nd fork() failed while daemonizing: %s\n"))

/* 
** utilib/sge_dir.c
*/ 
#define MSG_POINTER_NULLPARAMETER            _MESSAGE(49024, _("NULL parameter"))
#define MSG_FILE_OPENDIRFAILED_SS            _MESSAGE(49025, _("opendir(%s) failed: %s\n"))
#define MSG_FILE_STATFAILED_SS               _MESSAGE(49026, _("stat(%s) failed: %s\n"))
#define MSG_FILE_RECURSIVERMDIRFAILED        _MESSAGE(49027, _("==================== recursive_rmdir() failed\n"))
#define MSG_FILE_UNLINKFAILED_SS             _MESSAGE(49028, _("unlink(%s) failed: %s\n"))
#define MSG_FILE_RMDIRFAILED_SS              _MESSAGE(49029, _("rmdir(%s) failed: %s\n"))


/* 
** utilib/sge_getloadavg.c
*/ 
#define MSG_PROCESSOR_SETNOTFOUND_I          _MESSAGE(49030, _("processor set %d not found.\n"))
#define MSG_PROCESSOR_TOTALLOADAVG_F         _MESSAGE(49031, _("total load_avg %2.2f\n") )  
#define MSG_PROCESSOR_KNLISTFAILED           _MESSAGE(49032, _("Can't knlist()\n"))
#define MSG_PROCESSOR_KMEMFAILED             _MESSAGE(49033, _("Can't read kmem\n"))
#define MSG_PROCESSOR_NLISTFAILED            _MESSAGE(49034, _("Can't nlist()\n"))


/* 
** utilib/sge_loadmem.c
*/ 
#define MSG_SYSTEM_NOPAGESIZEASSUME8192      _MESSAGE(49035, _("can't determine system page size - assuming 8192"))
#define MSG_SYSTEM_TABINFO_FAILED_SS         _MESSAGE(49036, _("tabinfo(\"%s\", ...) failed, %s\n") )            
#define MSG_MEMORY_MALLOCFAILED_D            _MESSAGE(49037, _("malloc("U32CFormat") failed" ))


/* 
** utilib/sge_log.c
*/ 
#define MSG_LOG_CRITICALERROR                _MESSAGE(49038, _("critical error: "))
#define MSG_LOG_ERROR                        _MESSAGE(49039, _("error: "))
#define MSG_LOG_CALLEDLOGGINGSTRING_S        _MESSAGE(49040, _("logging called with %s logging string"))
#define MSG_LOG_ZEROLENGTH                   _MESSAGE(49041, _("zero length"))
#define MSG_POINTER_NULL                     _MESSAGE(49042, _("NULL"))


/* 
** utilib/sge_mkdir.c
*/ 
#define MSG_VAR_PATHISNULLINSGEMKDIR            _MESSAGE(49043, _("path == NULL in sge_mkdir()"))
#define MSG_FILE_CREATEDIRFAILED_SS             _MESSAGE(49044, _("can't create directory \"%s\": %s\n"))


/* 
** utilib/sge_peopen.c
*/ 
#define MSG_SYSTEM_EXECBINSHFAILED              _MESSAGE(49045, _("can't exec /bin/sh\n"))
#define MSG_SYSTEM_NOROOTRIGHTSTOSWITCHUSER     _MESSAGE(49046, _("you have to be root to become another user\n" ))
#define MSG_SYSTEM_NOUSERFOUND_SS               _MESSAGE(49047, _("can't get user %s: %s\n"))
#define MSG_SYSTEM_INITGROUPSFORUSERFAILED_ISS  _MESSAGE(49048, _("res = %d, can't initialize groups for user %s: %s\n"))
#define MSG_SYSTEM_SWITCHTOUSERFAILED_SS        _MESSAGE(49049, _("can't change to user %s: %s\n"))
#define MSG_SYSTEM_FAILOPENPIPES_SS             _MESSAGE(49050, _("failed opening pipes for %s: %s\n"))


/* 
** utilib/sge_processes_irix.c
*/ 
#define MSG_FILE_OPENFAILED_SS                  _MESSAGE(49051, _("failed opening %s: %s"))
#define MSG_SYSTEM_GETPIDSFAILED_S              _MESSAGE(49052, _("getpidsOfJob: ioctl(%s, PIOCSTATUS) failed\n"))
#define MSG_PROC_KILL_IIS                       _MESSAGE(49053, _("kill(%d, %d): %s"))
#define MSG_PROC_KILLISSUED_II                  _MESSAGE(49054, _("kill(%d, %d) issued"))


/* 
** utilib/sge_put_get_file.c
*/ 
#define MSG_FILE_WRITEOPENFAILED_SS             _MESSAGE(49055, _("couldn't open %s for writing: %s\n"))
#define MSG_FILE_READOPENFAILED_SS              _MESSAGE(49056, _("couldn't open %s for reading: %s"))

/* 
** utilib/sge_set_def_sig_mask.c
*/ 
#define MSG_PROC_SIGACTIONFAILED_IS             _MESSAGE(49057, _("sigaction for signal %d failed: %s"))


/* 
** utilib/sge_set_uid_gid.c
*/ 
#define MSG_SYSTEM_CHANGEUIDORGIDFAILED         _MESSAGE(49058, _("tried to change uid/gid without being root"))
#define MSG_SYSTEM_GIDLESSTHANMINIMUM_SUI       _MESSAGE(49059, _("gid of user %s ("U32CFormat") less than minimum allowed in conf (%d)"))
#define MSG_SYSTEM_UIDLESSTHANMINIMUM_SUI       _MESSAGE(49060, _("uid of user %s ("U32CFormat") less than minimum allowed in conf (%d)"))
#define MSG_SYSTEM_SETGIDFAILED_U               _MESSAGE(49061, _("setgid("U32CFormat") failed"))
#define MSG_SYSTEM_SETUIDFAILED_U               _MESSAGE(49062, _("setuid("U32CFormat") failed"))
#define MSG_SYSTEM_SETEGIDFAILED_U              _MESSAGE(49063, _("setegid("U32CFormat") failed"))
#define MSG_SYSTEM_SETEUIDFAILED_U              _MESSAGE(49064, _("seteuid("U32CFormat") failed"))
#define MSG_SYSTEM_INITGROUPSFAILED_I           _MESSAGE(49065, _("initgroups() failed with errno %d\n"))
#define MSG_SYSTEM_ADDGROUPIDFORSGEFAILED_UUS   _MESSAGE(49066, _("can't set additional group id (uid="U32CFormat", euid="U32CFormat"): "SFN"\n"))
#define MSG_SYSTEM_INVALID_NGROUPS_MAX          _MESSAGE(49067, _("invalid value for NGROUPS_MAX"))
#define MSG_SYSTEM_USER_HAS_TOO_MANY_GIDS       _MESSAGE(49068, _("the user already has too many group ids"))

/* 
** utilib/sge_signal.c
*/ 
#define MSG_PROC_UNKNOWNSIGNAL                  _MESSAGE(49069, _("unknown signal"))


/* 
** utilib/sge_str_from_file.c
*/ 
#define MSG_FILE_FOPENFAILED_SS                 _MESSAGE(49070, _("fopen("SFQ") failed: %s\n"))
#define MSG_FILE_FREADFAILED_SS                 _MESSAGE(49071, _("fread("SFQ") failed: %s\n"))
#define MSG_FILE_OPENFAILED_S                   _MESSAGE(49072, _("cant open file "SFQ"\n"))
#define MSG_FILE_WRITEBYTESFAILED_IS            _MESSAGE(49073, _("cant write %d bytes into file "SFQ"\n"))

/* 
** utilib/sge_string.c
*/ 
#define MSG_POINTER_INVALIDSTRTOKCALL           _MESSAGE(49074, _("Invalid sge_strtok_r call, last is not NULL\n"))

/* 
** utilib/sge_switch_user.c
*/ 
#define MSG_POINTER_SETADMINUSERNAMEFAILED      _MESSAGE(49075, _("set_admin_username() with zero length username"))
#define MSG_SYSTEM_ADMINUSERNOTEXIST_S          _MESSAGE(49076, _("admin_user \"%s\" does not exist"))
#define MSG_SYSTEM_CANTRUNASCALLINGUSER         _MESSAGE(49077, _("cannot run as calling user"))


/* 
** utilib/sge_sysconf.c
*/
#define MSG_SYSCONF_UNABLETORETRIEVE_I                _MESSAGE(49078, _("unable to retrieve value for system limit (%d)\n") )   

/*
**  former SGE_TEXT:
*/
#define MSG_SGETEXT_SHOULD_BE_ROOT_S            _MESSAGE(49079, _("should be root to start "SFN"\n"))
/* 
#define MSG_SGETEXT_INVALIDHOSTINQUEUE_S        _MESSAGE(49080, _("invalid hostname "SFQ" associated with queue\n"))
#define MSG_SGETEXT_MODIFIEDINLIST_SSUS         _MESSAGE(49081, _(""SFN"@"SFN" modified \"" U32CFormat "\" in "SFN" list\n"))
#define MSG_SGETEXT_CANTRESOLVEUSER_S           _MESSAGE(49082, _("unknown user name "SFQ"\n") )   
#define MSG_SGETEXT_CANTRESOLVEGROUP_S          _MESSAGE(49083, _("unknown group name "SFQ"\n") )  
#define MSG_SGETEXT_NOCOMMD_SS                  _MESSAGE(49084, _("unable to contact commd at host "SFN" using service "SFN"\n"))
#define MSG_SGETEXT_NOPERM                      _MESSAGE(49085, _("no permissions for this operation\n"))
#define MSG_SGETEXT_CANTFINDACL_S               _MESSAGE(49086, _("unable to find referenced access list "SFQ"\n"))
#define MSG_SGETEXT_SHOULD_BE_ROOT_S            _MESSAGE(49087, _("should be root to start "SFN"\n"))
#define MSG_SGETEXT_STILL_REFERENCED_SS         _MESSAGE(49088, _("remove reference to "SFQ" in subordinates of queue "SFQ" before deletion\n") ) 
#define MSG_SGETEXT_NO_SECURITY_LEVEL_FOR_S           _MESSAGE(49089, _("denied: missing security level for "SFN"\n"))
#define MSG_SGETEXT_MAY_NOT_CHG_QHOST_S               _MESSAGE(49090, _("may not change host of queue "SFQ"\n"))
#define MSG_SGETEXT_UP_REFERENCED_TWICE_SS            _MESSAGE(49091, _("denied: share tree contains reference to unknown "SFN" "SFQ"\n") )   
#define MSG_SGETEXT_NO_PROJECT                        _MESSAGE(49092, _("job rejected: no project assigned to job\n") )     
#define MSG_SGETEXT_UNABLETORETRIEVE_I                _MESSAGE(49093, _("unable to retrieve value for system limit (%d)\n") )     

#define MSG_SYSTEM_INVALIDERRORNUMBER                 _MESSAGE(49094, _("invalid error number"))
#define MSG_SYSTEM_GOTNULLASERRORTEXT                 _MESSAGE(49095, _("no error text available"))
*/


/* 
** utilib/sge_uidgid.c
*/ 
#define MSG_SYSTEM_GETPWUIDFAILED_US               _MESSAGE(49096, _("getpwuid("U32CFormat") failed: %s\n"))
#define MSG_SYSTEM_GETGRGIDFAILED_US               _MESSAGE(49097, _("getgrgid("U32CFormat") failed: %s\n"))

#endif /* __MSG_UTILIB_H */

