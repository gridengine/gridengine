#ifndef __RMON_RMON_H
#define __RMON_RMON_H
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

#ifndef DEBUG

#define DOPEN(x)
#define DENTER
#define DTRACE
#define DPRINTF(x)
#define DEXIT
#define DTRACEID(x)
#define DJOBTRACE(x)
#define DCLOSE
#define DMAYCLOSE(x)    (1)
#define TRACEON                 (0)
#define DEXECLP(x)              execlp x
#define DEXECVP(x)              execvp x
#define DFORK                   fork
#define DPUSH_LAYER(x)
#define DPOP_LAYER

#else

/*
   the following macro definitions
   operate on global variables
   defined in rmon.h and call functions
   from librmon.a

   the macro __CONDITION(x) is defined
   in rmon.h
 */

#include "rmon.h"

#define DOPEN(x)                                                                                                rmon_mopen(&argc,argv,x)
#define DENTER          __CONDITION(TRACE)              ?  rmon_menter (FUNC),1 : 0
#define DTRACE          __CONDITION(TRACE)              ?  rmon_mtrace (FUNC,__FILE__,__LINE__),1 : 0
#define DPRINTF(x)      __CONDITION(INFOPRINT)  ?  rmon_mprintf x,1 : 0
#define DEXIT           __CONDITION(TRACE)              ?  rmon_mexit (FUNC,__FILE__,__LINE__),1 : 0
#define DEXITE          __CONDITION(TRACE)              ?  rmon_mexite (FUNC,__FILE__,__LINE__),1 : 0
#define DTRACEID(x)     __CONDITION(TRACE)              ?       DEBUG_TRACEID=x,1 : 0
#define DJOBTRACE(x)    __CONDITION(JOBTRACE)   ?  rmon_mjobtrace x,1 : 0
#define DCLOSE                                                                                           rmon_mclose ()
#define DMAYCLOSE(x)                                                                                    rmon_mmayclose(x)
#define TRACEON                 ((MTYPE==RMON_LOCAL) && (!rmon_mliszero(&DEBUG_ON)))
#define DEXECLP(x)                                                                               rmon_mexeclp x
#define DEXECVP(x)                                                                               rmon_mexecvp x
#define DFORK                                                                                                   rmon_mfork
#define DPUSH_LAYER(x)  MTYPE==RMON_LOCAL?(rmon_mpush_layer(x),1):1
#define DPOP_LAYER              MTYPE==RMON_LOCAL?rmon_mpop_layer():0

#endif /* DEBUG */

#endif /* __RMON_RMON_H */
