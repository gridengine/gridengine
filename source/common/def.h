#ifndef __DEF_H
#define __DEF_H
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

#ifndef FALSE
#   define FALSE                (0)
#endif

#ifndef TRUE
#   define TRUE                 (1)
#endif

#define FREE(x) if(x){free( (char *)x); x=NULL;}

#if defined(_AIX) || defined(SVR3)
#define SGE_ASSERT(x)  if x ; else {sge_log(LOG_CRIT, # x ,__FILE__,SGE_FUNC,__LINE__);sge_log(LOG_CRIT,"unrecoverable error - contact systems manager",__FILE__,SGE_FUNC,__LINE__);abort();}
#else
#define SGE_ASSERT(x)  if x ; else {sge_log(LOG_CRIT,"unrecoverable error - contact systems manager",__FILE__,SGE_FUNC,__LINE__);abort();}
#endif

#define ISSET(a,b) ((a&b)==b)
#define VALID(a,b) ((a|b)==b)
#define SETBIT(a,b) (b=(a)|b);
#define CLEARBIT(a,b) (b &= (~(a)));

/* CONFIG_TAG constants are used to return conditions from
   functions such as read_in_qconf etc.
      CONFIG_TAG_OBSOLETE_VALUE:
         While reading an config file, there was found an 
         obsolete config-value. The caller should re-spool the
         config file to update it on disk.
      CONFIG_TAG_NEW_VALUE:
         While reading an config file, a newly added value
         was not found. A default has been set. The caller
         should re-spool the config file to update it on disk.
*/

#define CONFIG_TAG_OBSOLETE_VALUE         0x0001
#define CONFIG_TAG_NEW_VALUE              0x0002

#endif /* __DEF_H */
