#ifndef __SGE_LOG_H
#define __SGE_LOG_H
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

#ifndef WIN32NATIVE
#	include <syslog.h>
#else 
#	include "win32nativetypes.h"
#endif 



#ifndef __BASIS_TYPES_H
#   include "basis_types.h"
#endif

extern stringTlong SGE_EVENT;
extern char *error_file;

/* hook for commd trace() */
typedef void (*trace_func_type)(char *);
extern trace_func_type trace_func;


void sge_log_as_admin_user(void);
void sge_log_verbose(int i);
int sge_is_verbose(void);
void sge_qmon_log(int i);


#define SGE_LOG(level,msg) sge_log(level, msg, __FILE__, SGE_FUNC, __LINE__ );

#ifdef NO_SGE_COMPILE_DEBUG
#   define SGE_FUNC ""
#endif

#ifdef WIN32NATIVE
#undef ERROR
/* One windows header file defines ERROR as 0. This haeder is included by
   winsock2.h. Until we do not need this constant it is save to make an undef 
   here. */
#endif 

#if defined(__INSURE__)
#   define CRITICAL(x) (sprintf x,sge_log(LOG_CRIT,   SGE_EVENT,__FILE__,SGE_FUNC,__LINE__)) ? 1 : 0
#   define ERROR(x)    (sprintf x,sge_log(LOG_ERR,    SGE_EVENT,__FILE__,SGE_FUNC,__LINE__)) ? 1 : 0
#   define WARNING(x)  (sprintf x,sge_log(LOG_WARNING,SGE_EVENT,__FILE__,SGE_FUNC,__LINE__)) ? 1 : 0
#   define NOTICE(x)   (sprintf x,sge_log(LOG_NOTICE, SGE_EVENT,__FILE__,SGE_FUNC,__LINE__)) ? 1 : 0
#   define INFO(x)     (sprintf x,sge_log(LOG_INFO,   SGE_EVENT,__FILE__,SGE_FUNC,__LINE__)) ? 1 : 0
#   define DEBUG(x)    (sprintf x,sge_log(LOG_DEBUG,  SGE_EVENT,__FILE__,SGE_FUNC,__LINE__)) ? 1 : 0
#else
#   define CRITICAL(x) (sprintf x,sge_log(LOG_CRIT,   SGE_EVENT,__FILE__,SGE_FUNC,__LINE__) ,1) ? 1 : 0
#   define ERROR(x)    (sprintf x,sge_log(LOG_ERR,    SGE_EVENT,__FILE__,SGE_FUNC,__LINE__) ,1) ? 1 : 0
#   define WARNING(x)  (sprintf x,sge_log(LOG_WARNING,SGE_EVENT,__FILE__,SGE_FUNC,__LINE__) ,1) ? 1 : 0
#   define NOTICE(x)   (sprintf x,sge_log(LOG_NOTICE, SGE_EVENT,__FILE__,SGE_FUNC,__LINE__) ,1) ? 1 : 0
#   define INFO(x)     (sprintf x,sge_log(LOG_INFO,   SGE_EVENT,__FILE__,SGE_FUNC,__LINE__) ,1) ? 1 : 0
#   define DEBUG(x)    (sprintf x,sge_log(LOG_DEBUG,  SGE_EVENT,__FILE__,SGE_FUNC,__LINE__) ,1) ? 1 : 0
#endif
int sge_log(int log_level, char *mesg, char *file__, char *func__, int line__);

#endif /* __SGE_LOG_H */

