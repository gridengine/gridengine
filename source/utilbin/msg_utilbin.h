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
#define MSG_UTILBIN_USAGE                    _("usage:") 
#define MSG_COMMAND_RUNCOMMANDASUSERNAME_S     _("run commandline under %s of given user\n")
#define MSG_COMMAND_EXECUTEFAILED_S          _("can't execute command \"%s\"\n")
#define MSG_SYSTEM_RESOLVEUSERFAILED_S       _("can't resolve username \"%s\"\n")

/* 
** utilbin/gdi_request.c
*/ 
#define MSG_SYSTEM_QMASTERNOTALIVE           _("qmaster not alive\n")
#define MSG_SYSTEM_GDISETUPERROR             _("sge_gdi_setup error\n")
#define MSG_SYSTEM_MULTIREQUEST_I            _("%d. ----- Multirequest")
#define MSG_SYSTEM_6SINGLEREQUEST_I          _("%d. ----- 6 single requests")


/* 
** utilbin/checkprog.c
*/ 
#define MSG_COMMAND_USAGECHECKPROG           _("\ncheckprog pid processname\ncheck the first 8 letters of process basename\n\nexit status: 0 if process was found\n             1 if process was not found\n             2 if ps program couldn't be spawned\n")
#define MSG_COMMAND_USAGEGETPROGS            _("\ngetprogs processname\ncheck and list pids of \"processname\"\n\nexit status: 0 if process(es) were found\n             1 if process(es) was not found\n             2 if ps program couldn't be spawned\n")
#define MSG_COMMAND_CALLCHECKPROGORGETPROGS  _("program must be called as \"checkprog\" or \"getprogs\"\n")
#define MSG_PROC_PIDNOTRUNNINGORWRONGNAME_IS _("pid \"%d\" is not running or has another program name than \"%s\"\n")
#define MSG_PROC_PIDISRUNNINGWITHNAME_IS     _("pid \"%d\" with process name \"%s\" is running\n")
#define MSG_COMMAND_SPANPSFAILED             _("could not spawn ps command\n")
#define MSG_COMMAND_RUNPSCMDFAILED_S         _("could not run \"%s\" to get pids of processes\n")
#define MSG_PROC_FOUNDNOPROCESS_S            _("found no running processes with name \"%s\"\n")
#define MSG_PROC_FOUNDPIDSWITHNAME_S         _("found the following pids which have process name \"%s\"\n")



/* 
** utilbin/filestat.c
*/ 
#define MSG_COMMAND_STATFAILED               _("stat failed")
#define MSG_SYSTEM_UIDRESOLVEFAILED          _("can't resolve userid")



/* 
** utilbin/gethostbyaddr.c
*/ 
#define MSG_SYSTEM_GETHOSTBYADDRFAILED       _("gethostbyaddr() failed")
#define MSG_SYSTEM_HOSTNAMEIS_S              _("Hostname: %s\n")
#define MSG_SYSTEM_ALIASES                   _("Aliases:  ")
#define MSG_SYSTEM_ADDRESSES                 _("Host Address(es): ")



/* 
** utilbin/gethostname.c
*/ 
#define MSG_COMMAND_USAGE_GETHOSTNAME        _("get resolved hostname of this host\n")
#define MSG_SYSTEM_GETHOSTNAMEFAILED         _("gethostname() failed")
#define MSG_SYSTEM_GETHOSTBYNAMEFAILED       _("gethostbyname() failed")


/* 
** utilbin/getservbyname.c
*/ 
#define MSG_COMMAND_USAGE_GETSERVBYNAME      _("get number of a tcp service\n")
#define MSG_SYSTEM_SERVICENOTFOUND_S         _("service %s not found\n")


/* 
** utilbin/loadcheck.c
*/ 
#define MSG_SYSTEM_RETMEMORYINDICESFAILED    _("failed retrieving memory indices\n")


/* 
** utilbin/permutation.c
*/ 
#define MSG_UTILBIN_PERMUSAGE1   _("options are:\n")
#define MSG_UTILBIN_PERMUSAGE2   _("silent mode\n")
#define MSG_UTILBIN_PERMUSAGE3   _("sleep 'n' seconds after 1 second work\n")
#define MSG_UTILBIN_PERMUSAGE4   _("do work 'n' times using 'n' processes\n")
#define MSG_UTILBIN_PERMUSAGE5   _("with range s.th. like a-k\n")


/*
**  utilbin/sge_printf.c
*/
#define SGE_PRINTF_TESTSTRING_S "Welcome, %s\nhave a nice day!\n"
#define SGE_PRINTF_UNDERLINE  "-"
#define SGE_PRINTF_TESTSTRING_S_L10N _(SGE_PRINTF_TESTSTRING_S)
#define SGE_PRINTF_UNDERLINE_L10N    _(SGE_PRINTF_UNDERLINE)

/* 
** utilbin/range.c
*/ 
/* #define MSG_COMMAND_USAGE_RANGE              _("usage: range lower upper\n") */

#endif /* __MSG_UTILBIN_H */
