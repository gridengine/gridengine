#ifndef __SGE_USERPRJL_H
#define __SGE_USERPRJL_H

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

/* *INDENT-OFF* */ 

/*
 * This is the list type we use to hold the 
 * information for user/project. This objects are targets of throwing
 * tickets to them and as usage accumulators. There are no real differences 
 * at the moment, so putting them together is convenient.
 */
enum {
   UP_name = UP_LOWERBOUND,
   UP_oticket,
   UP_fshare,
   UP_job_cnt,
   UP_pending_job_cnt,
   UP_usage,
   UP_usage_time_stamp,
   UP_usage_seqno,
   UP_long_term_usage,
   UP_project,
   UP_acl,
   UP_xacl,
   UP_debited_job_usage,
   UP_default_project
};

ILISTDEF(UP_Type, UserProject, SGE_PROJECT_LIST)
   SGE_KSTRINGHU(UP_name)       /* configured user/project name spooled */
   SGE_ULONG(UP_oticket)      /* configured override tickets (set by Qmon,
                               * used by SGEEE schedd) spooled */
   SGE_ULONG(UP_fshare)       /* configured functional shares (set by Qmon, 
                               * used by SGEEE schedd) spooled */
   SGE_XULONG(UP_job_cnt)     /* job count (set and used by SGEEE schedd, not 
                               * spooled) schedd local, not stored to 
                               * qmaster */
   SGE_XULONG(UP_pending_job_cnt)  /* job count (set and used by SGEEE schedd, not 
                               * spooled) schedd local, not stored to 
                               * qmaster */
   SGE_LIST(UP_usage)         /* UA_Type; decayed usage set and used by SGEEE 
                               * schedd stored to qmaster; spooled */
   SGE_XULONG(UP_usage_time_stamp)    /* time stamp of last decay set when
                                       * UP_usage changes; set and used
                                       * by * SGEEE schedd stored to qmaster;
                                       * spooled */
   SGE_XULONG(UP_usage_seqno) /* usage sequence number set and used by SGE
                               * schedd, not stored to qmaster; not
                               * spooled */
   SGE_LIST(UP_long_term_usage)       /* UA_Type; long term accumulated 
                                       * non-decayed i usage; set by SGEEE 
                                       * schedd stored to qmaster; spooled */
   SGE_LIST(UP_project)       /* UPP_Type; usage on a project basis set and used 
                               * by SGEEE schedd stored to qmaster; spooled
                               * Only used by projects */
   SGE_TLIST(UP_acl, US_Type) /* US_Type but only names are filled 
                               * configured excluded user access list used
                               * by SGEEE schedd; spooled */
   SGE_TLIST(UP_xacl, US_Type)        /* US_Type but only names are filled configured 
                                       * excluded user access list used by SGEEE schedd; 
                                       * spooled */
   SGE_LIST(UP_debited_job_usage)     /* UPU_Type (see below) still *
                                       * debited usage per job (set and *
                                       * used by SGEEE schedd) */
   SGE_STRING(UP_default_project)     /* default project for user */
LISTEND 

NAMEDEF(UPN)
   NAME("UP_name")
   NAME("UP_oticket")
   NAME("UP_fshare")
   NAME("UP_job_cnt")
   NAME("UP_pending_job_cnt")
   NAME("UP_usage")
   NAME("UP_usage_time_stamp")
   NAME("UP_usage_seqno")
   NAME("UP_long_term_usage")
   NAME("UP_project")
   NAME("UP_acl")
   NAME("UP_xacl")
   NAME("UP_debited_job_usage")
   NAME("UP_default_project")
NAMEEND


#define UPS sizeof(UPN)/sizeof(char*)


/*
 * This is the list type we use to hold the 
 * information for user/project. This objects are targets of throwing
 * tickets to them and as usage accumulators. There are no real differences 
 * at the moment, so putting them together is convenient.
 */

enum {
   UPU_job_number = UPU_LOWERBOUND,  /* job number */
   UPU_old_usage_list        /* UA_Type still debited usage set and used
                              * via orders by SGEEE ted_job_usageschedd by
                              * qmaster */
};

LISTDEF(UPU_Type)
   SGE_ULONGHU(UPU_job_number)
   SGE_LIST(UPU_old_usage_list)
LISTEND 

NAMEDEF(UPUN)
   NAME("UPU_job_number")
   NAME("UPU_old_usage_list")
NAMEEND

#define UPUS sizeof(UPUN)/sizeof(char*)

/* 
 *
 * This is the project usage list type we use to hold the usage for
 * a user on a project basis. Each entry contains a project name and
 * a usage list.
 */
enum {
   UPP_name = UPP_LOWERBOUND,
   UPP_usage,
   UPP_long_term_usage
};

LISTDEF(UPP_Type)
   SGE_STRINGHU(UPP_name)    /* project name */
   SGE_LIST(UPP_usage)     /* 
                            * UA_Type; decayed usage set and used by SGEEE 
                            * schedd stored to qmaster; spooled 
                            */
   SGE_LIST(UPP_long_term_usage) /* UA_Type; long term accumulated 
                                  * non-decayed usage; set by SGEEE 
                                  * schedd stored to qmaster; spooled */
LISTEND 

NAMEDEF(UPPN)
   NAME("UPP_name")
   NAME("UPP_usage")
   NAME("UPP_long_term_usage")
NAMEEND

/* *INDENT-ON* */ 

#define UPPS sizeof(UPPN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_USERPRJL_H */
