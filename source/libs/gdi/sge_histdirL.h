#ifndef __SGE_HISTDIRL_H
#define __SGE_HISTDIRL_H

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

#include "sge_boundaries.h"
#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * this list is used to cache the names of the complexes' history
 * directory in memory
 * the list should be identical to the complex list
 */

/* *INDENT-OFF* */ 

enum {
   HD_name = HD_LOWERBOUND,
   HD_files,                 /* HF_Type */
   HD_oldest,                /* date of oldest file in dir as ulong */
   HD_latest
};

LISTDEF(HD_Type)
   SGE_STRING(HD_name)
   SGE_LIST(HD_files)
   SGE_ULONG(HD_oldest)
   SGE_ULONG(HD_latest)
LISTEND 

NAMEDEF(HDN)
   NAME("HD_name")
   NAME("HD_files")
   NAME("HD_oldest")
   NAME("HD_latest")
NAMEEND

#define HDS sizeof(HDN)/sizeof(char*)

/*
** list for history files, each describing one version of a complex
*/
enum {
   HF_name = HF_LOWERBOUND,
   HF_entries                /* CE_Type */
};

LISTDEF(HF_Type)
   SGE_STRING(HF_name)
   SGE_LIST(HF_entries)
LISTEND 

NAMEDEF(HFN)
   NAME("HF_name")
   NAME("HF_entries")
NAMEEND

#define HFS sizeof(HFN)/sizeof(char*)

/*
** list for sorted output of job sums for QACCT
*/
enum {
   QAJ_host = QAJ_LOWERBOUND,
   QAJ_queue,                /* qname */
   QAJ_group,
   QAJ_owner,
   QAJ_project,              /* SGEEE project */
   QAJ_department,           /* SGEEE department */
   QAJ_ru_wallclock,
   QAJ_ru_utime,
   QAJ_ru_stime,
   QAJ_ru_maxrss,
   QAJ_ru_inblock,
   QAJ_granted_pe,
   QAJ_slots,
   QAJ_cpu,
   QAJ_mem,
   QAJ_io,
   QAJ_iow
};

LISTDEF(QAJ_Type)
   SGE_HOST(QAJ_host)               /* CR - hostname change */
   SGE_STRING(QAJ_queue)
   SGE_STRING(QAJ_group)
   SGE_STRING(QAJ_owner)
   SGE_STRING(QAJ_project)
   SGE_STRING(QAJ_department)
   SGE_DOUBLE(QAJ_ru_wallclock)
   SGE_DOUBLE(QAJ_ru_utime)
   SGE_DOUBLE(QAJ_ru_stime)
   SGE_DOUBLE(QAJ_ru_maxrss)
   SGE_DOUBLE(QAJ_ru_inblock)
   SGE_STRING(QAJ_granted_pe)
   SGE_ULONG(QAJ_slots)
   SGE_DOUBLE(QAJ_cpu)
   SGE_DOUBLE(QAJ_mem)
   SGE_DOUBLE(QAJ_io)
   SGE_DOUBLE(QAJ_iow)
LISTEND 

NAMEDEF(QAJN)
   NAME("QAJ_host")
   NAME("QAJ_queue")
   NAME("QAJ_group")
   NAME("QAJ_owner")
   NAME("QAJ_project")
   NAME("QAJ_department")
   NAME("QAJ_ru_wallclock")
   NAME("QAJ_ru_utime")
   NAME("QAJ_ru_stime")
   NAME("QAJ_ru_maxrss")
   NAME("QAJ_ru_inblock")
   NAME("QAJ_granted_pe")
   NAME("QAJ_slots")
   NAME("QAJ_cpu")
   NAME("QAJ_mem")
   NAME("QAJ_io")
   NAME("QAJ_iow")
NAMEEND

/* *INDENT-ON* */

#define QAJS sizeof(QAJN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_HISTDIRL_H */
