#ifndef __SGE_REPORTL_H
#define __SGE_REPORTL_H

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

#include "sge_boundaries.h"
#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* 
** valid values for REP_type 
*/

/* REP_list is LR_Type */
#define NUM_REP_REPORT_LOAD    1

/* REP_list is ET_Type */
#define NUM_REP_REPORT_EVENTS  2

/* REP_list is CONF_Type */
#define NUM_REP_REPORT_CONF    3

/* REP_list is LIC_Type */
#define NUM_REP_REPORT_PROCESSORS 4

/* REP_list is JR_Type */
#define NUM_REP_REPORT_JOB     5

/* *INDENT-OFF* */ 

/*
 * definition for REP_Type, sge report type
 */
enum {
   REP_type = REP_LOWERBOUND,
   REP_host,
   REP_queue,
   REP_list,
   REP_version,
   REP_seqno
};

LISTDEF(REP_Type)
   SGE_ULONG(REP_type)        /* type of report, e.g. load report */
   SGE_STRING(REP_host)       /* hostname as it is seen by sender of report */
   SGE_STRING(REP_queue)      /* neccesary for qstd, gives queue this 
                               * report is directed to */
   SGE_LIST(REP_list)         /* list type depends on REP_type */
   SGE_ULONG(REP_version)     /* used to report software version of execd */
   SGE_ULONG(REP_seqno)       /* used to recognize old reports sent by execd */
LISTEND 

NAMEDEF(REPN)
   NAME("REP_type")
   NAME("REP_host")
   NAME("REP_queue")
   NAME("REP_list")
   NAME("REP_version")
   NAME("REP_seqno")
NAMEEND

#define REPS sizeof(REPN)/sizeof(char*)

/*
 * definition for license report, still to be enhanced
 */
enum {
   LIC_processors = LIC_LOWERBOUND,
   LIC_arch
};

LISTDEF(LIC_Type)
   SGE_ULONG(LIC_processors)
   SGE_STRING(LIC_arch)
LISTEND 

NAMEDEF(LICN)
   NAME("LIC_processors")
   NAME("LIC_arch")
NAMEEND

/* *INDENT-ON* */ 

#define LICS sizeof(LICN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_REPORTL_H */
