#ifndef __SGE_USERSETL_H
#define __SGE_USERSETL_H

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

#define US_ACL       (1<<0)
#define US_DEPT      (1<<1)

/* 
 * This is the list type we use to hold the 
 * user set lists in the qmaster. These are also used as 
 * (x)access lists.
 */

/* special list element */
#define DEADLINE_USERS "deadlineusers"
#define DEFAULT_DEPARTMENT "defaultdepartment"

/* *INDENT-OFF* */

enum {
   US_name = US_LOWERBOUND,
   US_type,                  /* type of USERSET encoded as bitfield */
   US_fshare,                /* 960703 svd - SGEEE functional share */
   US_oticket,               /* SGEEE override tickets */
   US_job_cnt,               /* SGEEE job count (internal to schedd) */
   US_pending_job_cnt,       /* SGEEE job count (internal to schedd) */
   US_entries
};

ILISTDEF(US_Type, UserSet, SGE_USERSET_LIST)
   SGE_KSTRING(US_name)       /* configured name spooled */
   SGE_ULONG(US_type)         /* configured type spooled */
   SGE_ULONG(US_fshare)       /* configured share spooled */
   SGE_ULONG(US_oticket)      /* configured override tickets spooled */
   SGE_XULONG(US_job_cnt)     /* local to schedd */
   SGE_XULONG(US_pending_job_cnt) /* local to schedd */
   SGE_TLIST(US_entries, UE_Type)     /* UE_Type */
LISTEND 

NAMEDEF(USEN)
   NAME("US_name")
   NAME("US_type")
   NAME("US_fshare")
   NAME("US_oticket")
   NAME("US_job_cnt")
   NAME("US_pending_job_cnt")
   NAME("US_entries")
NAMEEND

#define USES sizeof(USEN)/sizeof(char*)

/* 
 * an USERSET (US) is used to store
 * a user or a group in an user set list (US)
 * the flag field indicates whether name
 * and id specifiy an user or a group
 */
enum { 
   USER_ENTRY, 
   GROUP_ENTRY 
};

enum {
   UE_name = UE_LOWERBOUND   /* user or @group name */
};

SLISTDEF(UE_Type, UserEntry)
   SGE_STRING(UE_name)
LISTEND 

NAMEDEF(UEN)
   NAME("UE_name")
NAMEEND

#define UES sizeof(UEN)/sizeof(char*)

/*
 * this list is used by schedd to keep the number 
 * of running jobs per user/group efficiently
 */
enum {
   JC_name = JC_LOWERBOUND,  /* user or group name */
   JC_jobs                   /* number of running jobs */
};

LISTDEF(JC_Type)
   SGE_STRING(JC_name)
   SGE_ULONG(JC_jobs)
LISTEND 

NAMEDEF(JCN)
   NAME("JC_name")
   NAME("JC_jobs")
NAMEEND

/* *INDENT-ON* */

#define JCS sizeof(JCN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_USERSETL_H */
