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
#define MSG_TOKEN_NOSTART_S         _MESSAGE(49000, _("can't start set_token_command "SFQ))
#define MSG_TOKEN_NOWRITEAFS_S      _MESSAGE(49001, _("can't write AFS token to set_token_command "SFQ))
#define MSG_TOKEN_NOSETAFS_SI       _MESSAGE(49002, _("failed to set AFS token - set_token_command "SFQ" returned with exit status %d"))
#define MSG_COMMAND_NOPATHFORTOKEN  _MESSAGE(49003, _("can't get path for command to get AFS token\n"))
#define MSG_COMMAND_NOFILESTATUS_S  _MESSAGE(49004, _("can't determine file status of command "SFQ"\n"))
#define MSG_COMMAND_NOTEXECUTABLE_S _MESSAGE(49005, _("command "SFQ" is not executable\n"))

/*
** utilib/sge_arch.c
*/       
#define MSG_SGEROOTNOTSET           _MESSAGE(49006, _("Please set the environment variable SGE_ROOT.\n"))
#define MSG_SGEGRDROOTNOTEQUIV      _MESSAGE(49007, _("SGE_ROOT and GRD_ROOT are not equivalent\n"))
#define MSG_SGECODINEROOTNOTEQUIV   _MESSAGE(49008, _("SGE_ROOT and CODINE_ROOT are not equivalent\n"))
#define MSG_GRDCODINEROOTNOTEQUIV   _MESSAGE(49009, _("GRD_ROOT and CODINE_ROOT are not equivalent\n"))
#define MSG_UNKNOWNERRORINSGEROOT   _MESSAGE(49010, _("Unknown error in function sge_sge_root()\n"))
#define MSG_MEMORY_MALLOCFAILEDFORPATHTOHOSTALIASFILE _MESSAGE(49011, _("can't malloc() for path to host alias file"))

/* 
** utilib/sge_nprocs.c
*/ 
#define MSG_PERROR_PSTATDYNAMIC     _MESSAGE(49012, _("Pstat: PSTAT_DYNAMIC"))
#define MSG_INFO_NUMBOFPROCESSORS_I _MESSAGE(49013, _("Number of Processors '%d'\n"))
/* 
** utilib/sge_bitop.c
*/ 
#define MSG_MEMORY_NOMEMORYFORBYTEARRAY_S    _MESSAGE(49014, _(SFN": can't malloc memory for byte array\n"))
#define MSG_FILE_NOOPENFORWRITEING_SS        _MESSAGE(49015, _(SFN": unable to open file "SFN" for writing!\n"))


/* 
** utilib/sge_unistd.c
*/ 
#define MSG_FILE_NOCDTODIRECTORY_S           _MESSAGE(49016, _("can't change to directory "SFQ))


/* 
** utilib/sge_daemonize.c
*/ 
#define MSG_PROC_FIRSTFORKFAILED_S           _MESSAGE(49017, _("1st fork() failed while daemonizing: "SFN"\n"))
#define MSG_PROC_SECONDFORKFAILED_S          _MESSAGE(49018, _("2nd fork() failed while daemonizing: "SFN"\n"))

/* 
** utilib/sge_dir.c
*/ 
#define MSG_POINTER_NULLPARAMETER            _MESSAGE(49019, _("NULL parameter"))
#define MSG_FILE_OPENDIRFAILED_SS            _MESSAGE(49020, _("opendir("SFN") failed: "SFN"\n"))
#define MSG_FILE_STATFAILED_SS               _MESSAGE(49021, _("stat("SFN") failed: "SFN"\n"))
#define MSG_FILE_RECURSIVERMDIRFAILED        _MESSAGE(49022, _("==================== recursive_rmdir() failed\n"))
#define MSG_FILE_UNLINKFAILED_SS             _MESSAGE(49023, _("unlink("SFN") failed: "SFN"\n"))
#define MSG_FILE_RMDIRFAILED_SS              _MESSAGE(49024, _("rmdir("SFN") failed: "SFN"\n"))


/* 
** utilib/sge_getloadavg.c
*/ 
#define MSG_PROCESSOR_SETNOTFOUND_I          _MESSAGE(49025, _("processor set %d not found.\n"))
#define MSG_PROCESSOR_TOTALLOADAVG_F         _MESSAGE(49026, _("total load_avg %2.2f\n") )  
#define MSG_PROCESSOR_KNLISTFAILED           _MESSAGE(49027, _("Can't knlist()\n"))
#define MSG_PROCESSOR_KMEMFAILED             _MESSAGE(49028, _("Can't read kmem\n"))
#define MSG_PROCESSOR_NLISTFAILED            _MESSAGE(49029, _("Can't nlist()\n"))


/* 
** utilib/sge_loadmem.c
*/ 
#define MSG_SYSTEM_NOPAGESIZEASSUME8192      _MESSAGE(49030, _("can't determine system page size - assuming 8192"))
#define MSG_SYSTEM_TABINFO_FAILED_SS         _MESSAGE(49031, _("tabinfo("SFQ", ...) failed, "SFN"\n") )            
#define MSG_MEMORY_MALLOCFAILED_D            _MESSAGE(49032, _("malloc("U32CFormat") failed" ))


/* 
** utilib/sge_log.c
*/ 
#define MSG_LOG_CRITICALERROR                _MESSAGE(49033, _("critical error: "))
#define MSG_LOG_ERROR                        _MESSAGE(49034, _("error: "))
#define MSG_LOG_CALLEDLOGGINGSTRING_S        _MESSAGE(49035, _("logging called with "SFN" logging string"))
#define MSG_LOG_ZEROLENGTH                   _MESSAGE(49036, _("zero length"))
#define MSG_POINTER_NULL                     _MESSAGE(49037, _("NULL"))

/* 
** utilib/sge_peopen.c
*/ 
#define MSG_SYSTEM_EXECBINSHFAILED              _MESSAGE(49038, _("can't exec /bin/sh\n"))
#define MSG_SYSTEM_NOROOTRIGHTSTOSWITCHUSER     _MESSAGE(49039, _("you have to be root to become another user\n" ))
#define MSG_SYSTEM_NOUSERFOUND_SS               _MESSAGE(49040, _("can't get user "SFN": "SFN"\n"))
#define MSG_SYSTEM_INITGROUPSFORUSERFAILED_ISS  _MESSAGE(49041, _("res = %d, can't initialize groups for user "SFN": "SFN"\n"))
#define MSG_SYSTEM_SWITCHTOUSERFAILED_SS        _MESSAGE(49042, _("can't change to user "SFN": "SFN"\n"))
#define MSG_SYSTEM_FAILOPENPIPES_SS             _MESSAGE(49043, _("failed opening pipes for "SFN": "SFN"\n"))


/* 
** utilib/sge_put_get_file.c
*/ 
#define MSG_FILE_WRITEOPENFAILED_SS             _MESSAGE(49044, _("couldn't open "SFN" for writing: "SFN"\n"))
#define MSG_FILE_READOPENFAILED_SS              _MESSAGE(49045, _("couldn't open "SFN" for reading: "SFN""))


/* 
** utilib/sge_signal.c
*/ 
#define MSG_PROC_UNKNOWNSIGNAL                  _MESSAGE(49046, _("unknown signal"))
#define MSG_PROC_SIGACTIONFAILED_IS             _MESSAGE(49047, _("sigaction for signal %d failed: "SFN""))


/* 
** utilib/sge_str_from_file.c
*/ 
#define MSG_FILE_FOPENFAILED_SS                 _MESSAGE(49048, _("fopen("SFQ") failed: "SFN"\n"))
#define MSG_FILE_FREADFAILED_SS                 _MESSAGE(49049, _("fread("SFQ") failed: "SFN"\n"))
#define MSG_FILE_OPENFAILED_S                   _MESSAGE(49050, _("cant open file "SFQ"\n"))
#define MSG_FILE_WRITEBYTESFAILED_IS            _MESSAGE(49051, _("cant write %d bytes into file "SFQ"\n"))

/* 
** utilib/sge_string.c
*/ 
#define MSG_POINTER_INVALIDSTRTOKCALL           _MESSAGE(49052, _("Invalid sge_strtok_r call, last is not NULL\n"))

/* 
** utilib/sge_switch_user.c
*/ 
#define MSG_POINTER_SETADMINUSERNAMEFAILED      _MESSAGE(49053, _("set_admin_username() with zero length username"))
#define MSG_SYSTEM_ADMINUSERNOTEXIST_S          _MESSAGE(49054, _("admin_user "SFQ" does not exist"))
#define MSG_SWITCH_USER_NOT_INITIALIZED         _MESSAGE(49055, _("Module 'sge_switch_user' not initialized"))
#define MSG_SWITCH_USER_NOT_ROOT                _MESSAGE(49056, _("User 'root' did not start the application\n"))


/* 
** utilib/sge_sysconf.c
*/
#define MSG_SYSCONF_UNABLETORETRIEVE_I                _MESSAGE(49057, _("unable to retrieve value for system limit (%d)\n") )   

/*
**  former SGE_TEXT:
*/
#define MSG_SGETEXT_SHOULD_BE_ROOT_S            _MESSAGE(49058, _("should be root to start "SFN"\n"))

/* 
** utilib/sge_uidgid.c
*/ 
#define MSG_SYSTEM_GETPWUIDFAILED_US               _MESSAGE(49059, _("getpwuid("U32CFormat") failed: "SFN"\n"))
#define MSG_SYSTEM_GETGRGIDFAILED_US               _MESSAGE(49060, _("getgrgid("U32CFormat") failed: "SFN"\n"))
#define MSG_SYSTEM_CHANGEUIDORGIDFAILED         _MESSAGE(49061, _("tried to change uid/gid without being root"))
#define MSG_SYSTEM_GIDLESSTHANMINIMUM_SUI       _MESSAGE(49062, _("gid of user "SFN" ("U32CFormat") less than minimum allowed in conf (%d)"))
#define MSG_SYSTEM_UIDLESSTHANMINIMUM_SUI       _MESSAGE(49063, _("uid of user "SFN" ("U32CFormat") less than minimum allowed in conf (%d)"))
#define MSG_SYSTEM_SETGIDFAILED_U               _MESSAGE(49064, _("setgid("U32CFormat") failed"))
#define MSG_SYSTEM_SETUIDFAILED_U               _MESSAGE(49065, _("setuid("U32CFormat") failed"))
#define MSG_SYSTEM_SETEGIDFAILED_U              _MESSAGE(49066, _("setegid("U32CFormat") failed"))
#define MSG_SYSTEM_SETEUIDFAILED_U              _MESSAGE(49067, _("seteuid("U32CFormat") failed"))
#define MSG_SYSTEM_INITGROUPSFAILED_I           _MESSAGE(49068, _("initgroups() failed with errno %d\n"))
#define MSG_SYSTEM_ADDGROUPIDFORSGEFAILED_UUS   _MESSAGE(49069, _("can't set additional group id (uid="U32CFormat", euid="U32CFormat"): "SFN"\n"))
#define MSG_SYSTEM_INVALID_NGROUPS_MAX          _MESSAGE(49070, _("invalid value for NGROUPS_MAX"))
#define MSG_SYSTEM_USER_HAS_TOO_MANY_GIDS       _MESSAGE(49071, _("the user already has too many group ids"))

/* 
** utilib/sge_stdlib.c
*/ 
#define MSG_MEMORY_MALLOCFAILED                    _MESSAGE(49072, _("malloc() failure\n"))
#define MSG_MEMORY_REALLOCFAILED                   _MESSAGE(49073, _("realloc() failure\n") ) 
#define MSG_GDI_STRING_LENGTHEXCEEDED_SI           _MESSAGE(49074, _("strlen($"SFN") exceeds MAX_STRING_SIZE %d\n") ) 

/* 
** utilib/sge_unistd.c
*/ 
#define MSG_POINTER_SUFFIXISNULLINSGEUNLINK        _MESSAGE(49075, _("suffix == NULL in sge_unlink()\n") ) 
#define MSG_VAR_PATHISNULLINSGEMKDIR            _MESSAGE(49076, _("path == NULL in sge_mkdir()"))
#define MSG_FILE_CREATEDIRFAILED_SS             _MESSAGE(49077, _("can't create directory "SFQ": "SFN"\n"))

/*
** utilib/sge_hostname.c
*/
#define MSG_NET_GETHOSTNAMEFAILED                  _MESSAGE(49078, _("gethostname failed"))
#define MSG_NET_RESOLVINGLOCALHOSTFAILED           _MESSAGE(49079, _("failed resolving local host"))

/*
** libs/uti/host.c
*/
#define MSG_SYSTEM_BADMAINNAME_SS           _MESSAGE(49080, _("unresolvable mainname "SFQ" in alias file "SFN"\n"))
#define MSG_SYSTEM_BADALIASNAME_SS          _MESSAGE(49081, _("unresolvable aliasname "SFQ" in alias file "SFN"\n"))


/*
 * 
 */
#define MSG_UNREC_ERROR                            _MESSAGE(49082, _("unrecoverable error - contact systems manager"))

/*
 * libs/uti/sge_dirent.c
 */

#endif /* __MSG_UTILIB_H */

