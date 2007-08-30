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


/*
 * This is the list type we use to hold the 
 * information for project. This objects are targets of throwing
 * tickets to them and as usage accumulators.
 */
enum {
   PR_name = PR_LOWERBOUND,
   PR_oticket,
   PR_fshare,
   PR_job_cnt,
   PR_pending_job_cnt,
   PR_usage,
   PR_usage_time_stamp,
   PR_usage_seqno,
   PR_long_term_usage,
   PR_project,
   PR_acl,
   PR_xacl,
   PR_debited_job_usage,
   PR_version,
   PR_consider_with_categories
};

enum {
   PR_name_POS = 0,
   PR_oticket_POS,
   PR_fshare_POS,
   PR_job_cnt_POS,
   PR_pending_job_cnt_POS,
   PR_usage_POS,
   PR_usage_time_stamp_POS,
   PR_usage_seqno_POS,
   PR_long_term_usage_POS,
   PR_project_POS,
   PR_acl_POS,
   PR_xacl_POS,
   PR_debited_job_usage_POS,
   PR_version_POS,
   PR_consider_with_categories_POS
};

LISTDEF(PR_Type)
   JGDI_ROOT_OBJ(Project, SGE_PROJECT_LIST, ADD | MODIFY | DELETE | GET | GET_LIST)
   JGDI_EVENT_OBJ(ADD(sgeE_PROJECT_ADD) | MODIFY(sgeE_PROJECT_MOD) | DELETE(sgeE_PROJECT_DEL) | GET_LIST(sgeE_PROJECT_LIST))

   /* configured project name spooled */
   SGE_STRING_D(PR_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SPOOL | CULL_SUBLIST | CULL_JGDI_CONF, "template")

   /* configured override tickets (set by Qmon, used by SGEEE schedd) spooled */
   SGE_ULONG(PR_oticket, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_CONF)

   /* configured functional shares (set by Qmon, used by SGEEE schedd) spooled */
   SGE_ULONG(PR_fshare, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_CONF)

   /* job count (set and used by SGEEE schedd, not spooled) schedd local, not stored to qmaster */
   SGE_ULONG(PR_job_cnt, CULL_DEFAULT | CULL_JGDI_HIDDEN)

   /* job count (set and used by SGEEE schedd, not spooled) schedd local, not stored to qmaster */
   SGE_ULONG(PR_pending_job_cnt, CULL_DEFAULT | CULL_JGDI_HIDDEN)

   /* UA_Type; decayed usage set and used by SGEEE schedd stored to qmaster; spooled */
   SGE_MAP(PR_usage, UA_Type, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_RO)

   /* time stamp of last decay set when * PR_usage changes; set and used by SGEEE schedd stored to qmaster; spooled */
   SGE_ULONG(PR_usage_time_stamp, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_RO)

   /* usage sequence number set and used by SGE schedd, not stored to qmaster; not spooled */
   SGE_ULONG(PR_usage_seqno, CULL_DEFAULT | CULL_JGDI_HIDDEN)

   /* UA_Type; long term accumulated non-decayed i usage; set by SGEEE schedd stored to qmaster; spooled */
   SGE_MAP(PR_long_term_usage, UA_Type, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_RO)

   /* UPP_Type; usage on a project basis set and used by SGEEE schedd stored to qmaster; spooled Only used by projects */
   SGE_LIST(PR_project, UPP_Type, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_RO)

   /* US_Type but only names are filled configured excluded user access list used by SGEEE schedd; spooled */
   SGE_LIST(PR_acl, US_Type, CULL_DEFAULT | CULL_SPOOL_PROJECT | CULL_JGDI_CONF)

   /* US_Type but only names are filled configured excluded user access list used by SGEEE schedd; spooled */
   SGE_LIST(PR_xacl, US_Type, CULL_DEFAULT | CULL_SPOOL_PROJECT | CULL_JGDI_CONF)

   /* UPU_Type (see below) still debited usage per job (set and used by SGEEE schedd) */
   SGE_LIST(PR_debited_job_usage, UPU_Type, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_RO)

   /* user/project version, increments when usage is updated, stored to qmaster, not spooled */
   SGE_ULONG(PR_version, CULL_DEFAULT | CULL_JGDI_RO)

   /* true, if project plays role with categories */
   SGE_BOOL(PR_consider_with_categories, CULL_DEFAULT | CULL_JGDI_HIDDEN)
LISTEND 

NAMEDEF(PRN)
   NAME("PR_name")
   NAME("PR_oticket")
   NAME("PR_fshare")
   NAME("PR_job_cnt")
   NAME("PR_pending_job_cnt")
   NAME("PR_usage")
   NAME("PR_usage_time_stamp")
   NAME("PR_usage_seqno")
   NAME("PR_long_term_usage")
   NAME("PR_project")
   NAME("PR_acl")
   NAME("PR_xacl")
   NAME("PR_debited_job_usage")
   NAME("PR_version")
   NAME("PR_consider_with_categories")
NAMEEND


#define PRS sizeof(PRN)/sizeof(char*)

/*
 * This is the list type we use to hold the 
 * information for user. This objects are targets of throwing
 * tickets to them and as usage accumulators.
 */
enum {
   UU_name = UU_LOWERBOUND,
   UU_oticket,
   UU_fshare,
   UU_delete_time,
   UU_job_cnt,
   UU_pending_job_cnt,
   UU_usage,
   UU_usage_time_stamp,
   UU_usage_seqno,
   UU_long_term_usage,
   UU_project,
   UU_debited_job_usage,
   UU_default_project,
   UU_version,
   UU_consider_with_categories
};

enum {
   UU_name_POS = 0,
   UU_oticket_POS,
   UU_fshare_POS,
   UU_delete_time_POS,
   UU_job_cnt_POS,
   UU_pending_job_cnt_POS,
   UU_usage_POS,
   UU_usage_time_stamp_POS,
   UU_usage_seqno_POS,
   UU_long_term_usage_POS,
   UU_project_POS,
   UU_debited_job_usage_POS,
   UU_default_project_POS,
   UU_version_POS,
   UU_consider_with_categories_POS
};

LISTDEF(UU_Type)
   JGDI_ROOT_OBJ(User, SGE_USER_LIST, ADD | MODIFY | DELETE | GET | GET_LIST)
   JGDI_EVENT_OBJ(ADD(sgeE_USER_ADD) | MODIFY(sgeE_USER_MOD) | DELETE(sgeE_USER_DEL) | GET_LIST(sgeE_USER_LIST))

   /* configured user name spooled */
   SGE_STRING_D(UU_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SPOOL | CULL_SUBLIST | CULL_JGDI_CONF, "template")

   /* configured override tickets (set by Qmon, used by SGEEE schedd) spooled */
   SGE_ULONG(UU_oticket, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_CONF)
   
   /* configured functional shares (set by Qmon, used by SGEEE schedd) spooled */
   SGE_ULONG(UU_fshare, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_CONF)

   /* delete time for automatic users, (set by qmaster, * used by SGEEE qmaster) spooled */
   SGE_ULONG(UU_delete_time, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_CONF)

   /* job count (set and used by SGEEE schedd, not spooled) schedd local, not stored to qmaster */
   SGE_ULONG(UU_job_cnt, CULL_DEFAULT | CULL_JGDI_HIDDEN)

   /* job count (set and used by SGEEE schedd, not spooled) schedd local, not stored to qmaster */
   SGE_ULONG(UU_pending_job_cnt, CULL_DEFAULT | CULL_JGDI_HIDDEN)

   /* UA_Type; decayed usage set and used by SGEEE schedd stored to qmaster; spooled */
   SGE_MAP(UU_usage, UA_Type, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_RO)

   /* time stamp of last decay set when UU_usage changes; set and used by SGEEE schedd stored to qmaster; spooled */
   SGE_ULONG(UU_usage_time_stamp, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_RO)

   /* usage sequence number set and used by SGE schedd, not stored to qmaster; not spooled */
   SGE_ULONG(UU_usage_seqno, CULL_DEFAULT | CULL_JGDI_HIDDEN)

   /* UA_Type; long term accumulated * non-decayed i usage; set by SGEEE schedd stored to qmaster; spooled */
   SGE_MAP(UU_long_term_usage, UA_Type, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_RO)

   /* UPP_Type; usage on a project basis set and used by SGEEE schedd stored to qmaster; spooled Only used by projects */
   SGE_LIST(UU_project, UPP_Type, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_RO)

   /* UPU_Type (see below) still debited usage per job (set and used by SGEEE schedd) */
   SGE_LIST(UU_debited_job_usage, UPU_Type, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_RO)

   /* default project for user */
   SGE_STRING(UU_default_project, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_CONF)

   /* user/project version, increments when usage is updated, stored to qmaster, not spooled */
   SGE_ULONG(UU_version, CULL_DEFAULT | CULL_JGDI_RO)

   /* true, if project plays role with categories */
   SGE_BOOL(UU_consider_with_categories, CULL_DEFAULT | CULL_JGDI_HIDDEN)
LISTEND 

NAMEDEF(UUN)
   NAME("UU_name")
   NAME("UU_oticket")
   NAME("UU_fshare")
   NAME("UU_delete_time")
   NAME("UU_job_cnt")
   NAME("UU_pending_job_cnt")
   NAME("UU_usage")
   NAME("UU_usage_time_stamp")
   NAME("UU_usage_seqno")
   NAME("UU_long_term_usage")
   NAME("UU_project")
   NAME("UU_debited_job_usage")
   NAME("UU_default_project")
   NAME("UU_version")
   NAME("UU_consider_with_categories")
NAMEEND

#define UUS sizeof(UUN)/sizeof(char*)


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
   JGDI_OBJ(JobUsage)
   SGE_ULONG(UPU_job_number, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SUBLIST)
   SGE_MAP(UPU_old_usage_list, UA_Type, CULL_DEFAULT | CULL_SUBLIST)
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

enum {
   UPP_name_POS = 0,
   UPP_usage_POS,
   UPP_long_term_usage_POS
};


LISTDEF(UPP_Type)
   JGDI_OBJ(ProjectUsage)
   SGE_STRING(UPP_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SUBLIST)    /* project name */
   SGE_MAP(UPP_usage, UA_Type, CULL_DEFAULT | CULL_SUBLIST)     /* 
                            * UA_Type; decayed usage set and used by SGEEE 
                            * schedd stored to qmaster; spooled 
                            */
   SGE_MAP(UPP_long_term_usage, UA_Type, CULL_DEFAULT | CULL_SUBLIST) /* UA_Type; long term accumulated 
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
