#ifndef __MSG_UTILBIN_H
#define __MSG_UTILBIN_H
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
** utilbin/adminrun.c
*/
#define MSG_UTILBIN_USAGE                    _MESSAGE(57000, _("usage:") ) 
#define MSG_COMMAND_RUNCOMMANDASUSERNAME_S     _MESSAGE(57001, _("run commandline under "SFN" of given user\n"))
#define MSG_COMMAND_EXECUTEFAILED_S          _MESSAGE(57002, _("can't execute command "SFQ"\n"))
#define MSG_SYSTEM_RESOLVEUSERFAILED_S       _MESSAGE(57003, _("can't resolve username "SFQ"\n"))

/* 
** utilbin/gdi_request.c
*/ 
/* #define MSG_SYSTEM_QMASTERNOTALIVE           _message(57004, _("qmaster not alive\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SYSTEM_GDISETUPERROR             _message(57005, _("sge_gdi_setup error\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SYSTEM_MULTIREQUEST_I            _message(57006, _("%d. ----- Multirequest")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SYSTEM_6SINGLEREQUEST_I          _message(57007, _("%d. ----- 6 single requests")) __TS Removed automatically from testsuite!! TS__*/


/* 
** utilbin/checkprog.c
*/ 
#define MSG_COMMAND_USAGECHECKPROG           _MESSAGE(57008, _("\ncheckprog pid processname\ncheck the first 8 letters of process basename\n\nexit status: 0 if process was found\n             1 if process was not found\n             2 if ps program couldn't be spawned\n"))
#define MSG_COMMAND_USAGEGETPROGS            _MESSAGE(57009, _("\ngetprogs processname\ncheck and list pids of \"processname\"\n\nexit status: 0 if process(es) were found\n             1 if process(es) was not found\n             2 if ps program couldn't be spawned\n"))
#define MSG_COMMAND_CALLCHECKPROGORGETPROGS  _MESSAGE(57010, _("program must be called as \"checkprog\" or \"getprogs\"\n"))
#define MSG_PROC_PIDNOTRUNNINGORWRONGNAME_IS _MESSAGE(57011, _("pid \"%d\" is not running or has another program name than "SFQ"\n"))
#define MSG_PROC_PIDISRUNNINGWITHNAME_IS     _MESSAGE(57012, _("pid \"%d\" with process name "SFQ" is running\n"))
#define MSG_COMMAND_SPANPSFAILED             _MESSAGE(57013, _("could not spawn ps command\n"))
#define MSG_COMMAND_RUNPSCMDFAILED_S         _MESSAGE(57014, _("could not run "SFQ" to get pids of processes\n"))
#define MSG_PROC_FOUNDNOPROCESS_S            _MESSAGE(57015, _("found no running processes with name "SFQ"\n"))
#define MSG_PROC_FOUNDPIDSWITHNAME_S         _MESSAGE(57016, _("found the following pids which have process name "SFQ"\n"))



/* 
** utilbin/filestat.c
*/ 
#define MSG_COMMAND_STATFAILED               _MESSAGE(57017, _("stat failed"))
#define MSG_SYSTEM_UIDRESOLVEFAILED          _MESSAGE(57018, _("can't resolve userid"))



/* 
** utilbin/gethostbyaddr.c
*/ 
#define MSG_SYSTEM_HOSTNAMEIS_S              _MESSAGE(57020, _("Hostname: "SFN"\n"))
#define MSG_SYSTEM_ALIASES                   _MESSAGE(57021, _("Aliases:  "))
#define MSG_SYSTEM_ADDRESSES                 _MESSAGE(57022, _("Host Address(es): "))



/* 
** utilbin/gethostname.c
*/ 
#define MSG_COMMAND_USAGE_GETHOSTNAME        _MESSAGE(57023, _("get resolved hostname of this host\n"))


/* 
** utilbin/getservbyname.c
*/ 
#define MSG_COMMAND_USAGE_GETSERVBYNAME      _MESSAGE(57026, _("get number of a tcp service\n"))
#define MSG_SYSTEM_SERVICENOTFOUND_S         _MESSAGE(57027, _("service "SFN" not found\n"))


/* 
** utilbin/loadcheck.c
*/ 
#define MSG_SYSTEM_RETMEMORYINDICESFAILED    _MESSAGE(57028, _("failed retrieving memory indices\n"))


/* 
** utilbin/permutation.c
*/ 
#define MSG_UTILBIN_PERMUSAGE1   _MESSAGE(57029, _("options are:\n"))
#define MSG_UTILBIN_PERMUSAGE2   _MESSAGE(57030, _("silent mode\n"))
#define MSG_UTILBIN_PERMUSAGE3   _MESSAGE(57031, _("sleep 'n' seconds after 1 second work\n"))
#define MSG_UTILBIN_PERMUSAGE4   _MESSAGE(57032, _("do work 'n' times using 'n' processes\n"))
#define MSG_UTILBIN_PERMUSAGE5   _MESSAGE(57033, _("with range s.th. like a-k\n"))


/*
**  utilbin/infotext.c
*/
#define SGE_INFOTEXT_TESTSTRING_S "Welcome, "SFN"\nhave a nice day!\n"
#define SGE_INFOTEXT_UNDERLINE  "-"
/* #define SGE_INFOTEXT_TESTSTRING_S_L10N _message(57034, _(SGE_INFOTEXT_TESTSTRING_S)) __TS Removed automatically from testsuite!! TS__*/
/* #define SGE_INFOTEXT_UNDERLINE_L10N    _message(57035, _(SGE_INFOTEXT_UNDERLINE)) __TS Removed automatically from testsuite!! TS__*/
#define SGE_INFOTEXT_ONLY_ALLOWED_SS _MESSAGE(57036, _("\nThere are only two answers allowed: "SFQ" or "SFQ"!\n\n"))

/* 
** utilbin/range.c
*/ 
/* #define MSG_COMMAND_USAGE_RANGE              _message(57037, _("usage: range lower upper\n") )  __TS Removed automatically from testsuite!! TS__*/

/*
 * utilbin/spooldefaults.c
 */

#define MSG_SPOOLDEFAULTS_COMMANDINTRO1   _MESSAGE(57100,         _("create default entries during installation process\n"))
#define MSG_SPOOLDEFAULTS_COMMANDINTRO2   _MESSAGE(57101,         _("following are the valid commands:\n"))
#define MSG_SPOOLDEFAULTS_TEST            _MESSAGE(57102,         _("test                          test the spooling framework\n"))
#define MSG_SPOOLDEFAULTS_MANAGERS        _MESSAGE(57103,         _("managers <mgr1> [<mgr2> ...]  create managers\n"))
#define MSG_SPOOLDEFAULTS_OPERATORS       _MESSAGE(57104,         _("operators <op1> [<op2> ...]   create operators\n"))
#define MSG_SPOOLDEFAULTS_PES             _MESSAGE(57105,         _("pes <template_dir>            create parallel environments\n"))
#define MSG_SPOOLDEFAULTS_CONFIGURATION          _MESSAGE(57106,  _("configuration <template>      create the global configuration\n"))
#define MSG_SPOOLDEFAULTS_LOCAL_CONF          _MESSAGE(57107,     _("local_conf <template> <name>  create a local configuration\n"))
#define MSG_SPOOLDEFAULTS_USERSETS            _MESSAGE(57108,     _("usersets <template_dir>       create usersets\n"))
#define MSG_SPOOLDEFAULTS_CANNOTCREATECONTEXT   _MESSAGE(57109, _("cannot create spooling context\n"))
#define MSG_SPOOLDEFAULTS_CANNOTSTARTUPCONTEXT  _MESSAGE(57110, _("cannot startup spooling context\n"))
#define MSG_SPOOLDEFAULTS_COMPLEXES             _MESSAGE(57111,         _("complexes <template_dir>      create complexes\n"))
#define MSG_SPOOLDEFAULTS_ADMINHOSTS            _MESSAGE(57112,         _("adminhosts <template_dir>     create admin hosts\n"))
#define MSG_SPOOLDEFAULTS_SUBMITHOSTS           _MESSAGE(57113,         _("submithosts <template_dir>    create submit hosts\n"))
#define MSG_SPOOLDEFAULTS_CALENDARS             _MESSAGE(57114,         _("calendars <template_dir>      create calendars\n"))
#define MSG_SPOOLDEFAULTS_CKPTS                 _MESSAGE(57115,         _("ckpts <template_dir>          create checkpoint environments\n"))
#define MSG_SPOOLDEFAULTS_EXECHOSTS             _MESSAGE(57116,         _("exechosts <template_dir>      create execution hosts\n"))
#define MSG_SPOOLDEFAULTS_PROJECTS              _MESSAGE(57117,         _("projects <template_dir>       create projects\n"))
#define MSG_SPOOLDEFAULTS_CQUEUES               _MESSAGE(57118,         _("cqueues <template_dir>        create cluster queues\n"))
#define MSG_SPOOLDEFAULTS_USERS                 _MESSAGE(57119,         _("users <template_dir>          create users\n"))

/*
 * utilbin/spoolinit.c
 */

#define MSG_SPOOLINIT_COMMANDINTRO0   _MESSAGE(57200, _("method shlib libargs command [args]"))
#define MSG_SPOOLINIT_COMMANDINTRO1   _MESSAGE(57201, _("database maintenance\n"))
#define MSG_SPOOLINIT_COMMANDINTRO2   _MESSAGE(57202, _("following are the valid commands\n"))
#define MSG_SPOOLINIT_COMMANDINTRO3   _MESSAGE(57203, _("init [history]    initialize the database [with history enabled]\n"))
#define MSG_SPOOLINIT_COMMANDINTRO4   _MESSAGE(57204, _("history on|off    switch spooling with history on or off\n"))
#define MSG_SPOOLINIT_COMMANDINTRO5   _MESSAGE(57205, _("backup path       backup the database to path\n"))
#define MSG_SPOOLINIT_COMMANDINTRO6   _MESSAGE(57206, _("purge days        remove historical data older than days\n"))
#define MSG_SPOOLINIT_COMMANDINTRO7   _MESSAGE(57207, _("vacuum            compress database, update statistics\n"))
#define MSG_SPOOLINIT_COMMANDINTRO8   _MESSAGE(57208, _("info              output information about the database\n"))
#define MSG_SPOOLINIT_COMMANDINTRO9   _MESSAGE(57209, _("method            output the compiled in spooling method\n"))

/*
 * utilbin/dbstat
 */
#define MSG_DBSTAT_COMMANDINTRO1   _MESSAGE(57300,         _("database query and maintenance\n"))
#define MSG_DBSTAT_COMMANDINTRO2   _MESSAGE(57301,         _("following are the valid commands:\n"))
#define MSG_DBSTAT_LIST            _MESSAGE(57302,         _("list [object type]  list all objects [matching object type]\n"))
#define MSG_DBSTAT_DUMP            _MESSAGE(57303,         _("dump key            dump the objects matching key\n"))
#define MSG_DBSTAT_DELETE          _MESSAGE(57304,         _("delete key          delete the objects matching key\n"))


#endif /* __MSG_UTILBIN_H */
