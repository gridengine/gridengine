#ifndef __SGE_PGRP_H
#define __SGE_PGRP_H
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
 *  License at http://www.gridengine.sunsource.net/license.html
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

/* SETPGRP - set process group */
#if defined(_UNICOS) || defined(SOLARIS) || defined(__hpux) || defined(LINUX) || defined(AIX4) || defined(SINIX) || defined(NECSX4) || defined(NECSX5)
#   define SETPGRP setpgrp()
#elif defined(__sgi)
#   define SETPGRP BSDsetpgrp(getpid(),getpid())
#elif defined(SUN4) || defined(WIN32)
#   define SETPGRP setsid()
#else
#   define SETPGRP setpgrp(getpid(),getpid())
#endif

/* GETPGRP - get process group */
#ifdef SUN4
#   define GETPGRP getpgrp(0)
#else
#   define GETPGRP getpgrp()
#endif

#endif /* __SGE_PGRP_H */
