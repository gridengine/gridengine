#ifndef __SGERMON_H
#define __SGERMON_H
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

#ifndef NO_SGE_COMPILE_DEBUG

#ifndef WIN32NATIVE
#	include "rmon.h"
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef WIN32NATIVE
#ifdef REENTRANCY_CHECK
#define DENTER_MAIN( layer, program ) \
   static char SGE_FUNC[] = "main";  \
   int LAYER = 0; \
   static int entered = 0; \
   entered = 1; \
   \
   rmon_mopen(&argc,argv,program); \
   LAYER = layer; \
   if ( __CONDITION(TRACE) ) \
      rmon_menter (SGE_FUNC)
#else
#define DENTER_MAIN( layer, program ) \
   static char SGE_FUNC[] = "main";  \
   int LAYER = 0; \
   \
   rmon_mopen(&argc,argv,program); \
   LAYER = layer; \
   if ( __CONDITION(TRACE) ) \
      rmon_menter (SGE_FUNC)
#endif  /* REENTRANCY_CHECK */

#ifdef REENTRANCY_CHECK
#define DENTER( layer, function) \
   int LAYER = layer; \
   static char SGE_FUNC[] = function; \
   static int entered = 0; \
   \
   if ( __CONDITION(TRACE) ) \
      rmon_menter (SGE_FUNC); \
   if ( entered && __CONDITION(INFOPRINT) ) \
      rmon_mprintf("Reentering function %s!!!!!!\n", SGE_FUNC); \
   else \
      entered = 1
#else
#define DENTER( layer, function) \
   int LAYER = layer; \
   static char SGE_FUNC[] = function; \
   \
   if ( __CONDITION(TRACE) ) \
      rmon_menter (SGE_FUNC)
#endif  /* REENTRANCY_CHECK*/

#define DENTERBLOCK( layer, save ) \
   save = LAYER; \
   LAYER = layer

#define DEXITBLOCK(save) \
   LAYER = save

#ifdef REENTRANCY_CHECK
#define DEXIT  entered = 0; __CONDITION(TRACE)  ? \
    rmon_mexit(SGE_FUNC,__FILE__,__LINE__),1   : 0
#else
#define DEXIT  __CONDITION(TRACE)  ? \
    rmon_mexit(SGE_FUNC,__FILE__,__LINE__),1   : 0
#endif  /* REENTRANCY_CHECK */

#define DTRACE          __CONDITION(TRACE)      ?  \
        rmon_mtrace(SGE_FUNC,__FILE__,__LINE__),1 : 0

#define DPRINTF(x)      __CONDITION(INFOPRINT)  ?  rmon_mprintf x ,1 : 0
#define DTIMEPRINTF(x)  __CONDITION(TIMING)  ?  rmon_mprintf x ,1 : 0
#define DSPECIALPRINTF(x) __CONDITION(SPECIAL)  ?  rmon_mprintf x ,1 : 0
#define DTRACEID(x)     __CONDITION(TRACE)      ?  DEBUG_TRACEID=x,1 : 0;
#define DJOBTRACE(x)    __CONDITION(JOBTRACE)   ?  rmon_mjobtrace x,1 : 0
#define DCLOSE                                     rmon_mclose ()
#define DMAYCLOSE(x)                               rmon_mmayclose(x)
#define TRACEON         (MTYPE == RMON_LOCAL && !rmon_mliszero(&DEBUG_ON))
#define DEXECLP(x)                                 rmon_mexeclp x
#define DEXECVP(x)                                 rmon_mexecvp x
#define DFORK                                      rmon_mfork
#define ISTRACE         __CONDITION(TRACE)

#ifndef __INSURE__
#   define SGE_EXIT(x)     (DTRACE, sge_exit(x),0)?0:1
#else
#   define SGE_EXIT(x) sge_exit(x)
#endif

#else

#include "DebugC.h"

#define DENTER( layer, function ) \
	int LAYER = layer; \
	static char SGE_FUNC[] = function; \
	DebugObj_Enter((LAYER), (SGE_FUNC))

#define DEXIT \
	DebugObj_Exit((LAYER), (SGE_FUNC))

#define DPRINTF(x) \
	DebugObj_PrintInfoMsg x 

#define SGE_EXIT(x) \
	sge_exit(x)

#define DCLOSE \
	DebugObj_PrintInfoMsg("DCLOSE")

//	(DTRACE, sge_exit(x),0)?0:1

#endif /* WIN32NATIVE */

#else

#define DENTER_MAIN( layer, program )
#define DENTER( layer, function)
#define DEXIT
#define DTRACE
#define DPRINTF(x)
#define DTIMEPRINTF(x)
#define DSPECIALPRINTF(x)
#define DTRACEID(x)
#define DJOBTRACE(x)
#define DCLOSE
#define DMAYCLOSE(x)
#define TRACEON
#define DEXECLP(x)
#define DEXECVP(x)
#define DFORK
#define ISTRACE
#define SGE_EXIT(x)     sge_exit(x)

#endif /* NO_SGE_COMPILE_DEBUG */

#ifdef  __cplusplus
}
#endif

#endif /* __SGERMON_H */
