#ifndef QMON_RMON_H
#define QMON_RMON_H
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


#ifndef NO_SGE_COMPILE_DEBUG

#include "sgermon.h"


/*-----------------------------------------------------------------------

#define DENTER_MAIN( layer, program ) \
   static char SGE_FUNC[] = "main"; \
   int old_layer; \
   \
   old_layer = 0; \
   rmon_mopen(&argc,argv,program); \
   LAYER = layer; \
   if ( __CONDITION(TRACE) ) \
      rmon_menter (SGE_FUNC)

#define DENTER( layer, function) \
   int old_layer; \
   static char SGE_FUNC[] = function; \
   \
   old_layer = LAYER; \
   LAYER = layer; \
   if ( __CONDITION(TRACE) ) \
      rmon_menter (SGE_FUNC)

#define DEXIT  __CONDITION(TRACE)  ? \
	rmon_mexit(SGE_FUNC,__FILE__,__LINE__),1   : 0, \
LAYER=old_layer
#define DTRACE          __CONDITION(TRACE)      ?  \
        rmon_mtrace(SGE_FUNC,__FILE__,__LINE__),1 : 0;

#define DPRINTF(x)      __CONDITION(INFOPRINT)  ?  rmon_mprintf x ,1 : 0 

#define DTRACEID(x)     __CONDITION(TRACE)      ?  DEBUG_TRACEID=x,1 : 0;
#define DJOBTRACE(x)    __CONDITION(JOBTRACE)   ?  rmon_mjobtrace x,1 : 0
#define DCLOSE                                     rmon_mclose ()
#define DMAYCLOSE(x)                               rmon_mmayclose(x)
#define TRACEON         (MTYPE == RMON_LOCAL && !rmon_mliszero(&DEBUG_ON))
#define DEXECLP(x)                                 rmon_mexeclp x
#define DEXECVP(x)                                 rmon_mexecvp x
#define DFORK                                      rmon_mfork


#else

#define DENTER_MAIN( layer, program ) 
#define DENTER( layer, function) 
#define DEXIT  
#define DTRACE          
#define DPRINTF(x)
#define DTRACEID(x)
#define DJOBTRACE(x)    
#define DCLOSE         
#define DMAYCLOSE(x)  
#define TRACEON      
#define DEXECLP(x)  
#define DEXECVP(x) 
#define DFORK     

-------------------------------------------------------------------------*/

#endif /* SGE_COMPILE_DEBUG */

#endif /* QMON_RMON_H */
