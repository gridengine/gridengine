#ifndef __SGE_SHARE_TREE_NODEL_H
#define __SGE_SHARE_TREE_NODEL_H

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
 * nodes of a share tree.
 */
#define STT_USER    0
#define STT_PROJECT 1

enum {
   STN_name = STN_LOWERBOUND,        /* Node name (user or projectname in
                                      * the leafs) spooled */
   STN_type,                 /* Kind of tree: STT_USER | STT_PROJECT All
                              * nodes in a tree have the same type spooled 
                              */

   STN_id,                   /* Only for editing and save/restore. May be
                              * different each time stored not spooled */

   STN_shares,               /* Configured shares for a subtree spooled */

   STN_children,             /* Configured childrens of this node.
                              * STN_Type list spooled */

   STN_job_ref_count,        /* number of active and pending jobs used in
                              * schedd, not stored to qmaster */
   STN_active_job_ref_count, /* number of active jobs used in schedd, not
                              * stored to qmaster */
   STN_project,              /* set to 1 if this node is a project used in 
                              * schedd, not stored to qmaster */
   STN_proportion,           /* share proportion used in schedd, not
                              * stored to qmaster */
   STN_adjusted_proportion,  /* share adjusted proportion used in schedd,
                              * not stored to qmaster */
   STN_target_proportion,    /* targetted proportion used in schedd, not
                              * stored to qmaster */
   STN_current_proportion,   /* current proportion used in schedd, not
                              * stored to qmaster */
   STN_adjusted_usage,       /* adjusted usage used in schedd, not stored
                              * to qmaster */
   STN_combined_usage,       /* combined usage used in schedd, not stored
                              * to qmaster */
   STN_pass0_seqno,          /* seqno for pass 0 used in schedd, not
                              * stored to qmaster */
   STN_pass1_seqno,          /* seqno for pass 1 used in schedd, not
                              * stored to qmaster */
   STN_pass2_seqno,          /* seqno for pass 2 used in schedd, not
                              * stored to qmaster */
   STN_sum_priority,         /* sum of job priorities used in schedd, not
                              * stored to qmaster */
   STN_actual_proportion,    /* long term actual proportion, calculated in 
                              * qmon, not stored to qmaster */
   STN_m_share,              /* dynamic long term targetted proportion,
                              * set in schedd, stored to qmaster, not
                              * spooled */
   STN_last_actual_proportion,       /* short term actual proportion, set
                                      * in schedd, stored to qmaster, not
                                      * spooled */
   STN_adjusted_current_proportion,  /* short term targetted proportion, 
                                      * set in schedd, stored to qmaster,
                                      * not spooled */
   STN_temp,                 /* this node is a temporary node created in
                              * schedd and will not be sent to qmaster,
                              * set in schedd, not stored to qmaster, not
                              * spooled */
   STN_stt,                  /* short term targeted proportion of node 
                              * compared to sibling nodes,
                              * calculated during scheduling of pending
                              * jobs, set in schedd, not stored to qmaster,
                              * not spooled */
   STN_ostt,                 /* overall short term targeted proportion of
                              * node compared to all nodes,
                              * calculated during scheduling of pending
                              * jobs, set in schedd, not stored to qmaster,
                              * not spooled */
   STN_ltt,                  /* long term targeted proportion of node 
                              * compared to all nodes,
                              * calculated during scheduling of pending
                              * jobs, set in schedd, not stored to qmaster,
                              * not spooled */
   STN_oltt,                 /* overall long term targeted proportion of
                              * node compared to all nodes,
                              * calculated during scheduling of pending
                              * jobs, set in schedd, not stored to qmaster,
                              * not spooled */
   STN_shr,                  /* hierarchical calculated "share" node,
                              * calculated during scheduling of pending
                              * jobs, set in schedd, not stored to qmaster,
                              * not spooled */
   STN_ref,                  /* Temporary index reference back into the
                              * array of pending jobs, used during
                              * scheduling of pending jobs, set in schedd,
                              * not stored to qmaster, not spooled */
   STN_jobid,                /* Job number of a temporary job node,
                              * used during scheduling of pending jobs,
                              * set in schedd, not stored to qmaster,
                              * not spooled */
   STN_taskid,               /* Task number of a temporary job node,
                              * used during scheduling of pending jobs,
                              * set in schedd, not stored to qmaster,
                              * not spooled */
   STN_usage_list,           /* Node usage list used during scheduling
                              * of pending jobs, set in schedd,
                              * not stored to qmaster, not spooled */
   STN_version               /* version of share tree set in qmaster when
                              * sharetree changes spooled */
};


ILISTDEF(STN_Type, ShareTreeNode, SGE_SHARETREE_LIST)
   SGE_KSTRINGHU(STN_name)
   SGE_ULONG(STN_type)        /* 960624 svd - changed to STN_type */
   SGE_XULONG(STN_id)         /* Unique node id for storing to disk */
   SGE_ULONG(STN_shares)
   SGE_RLIST(STN_children, STN_Type)
   SGE_XULONG(STN_job_ref_count)
   SGE_XULONG(STN_active_job_ref_count)
   SGE_XULONG(STN_project)
   SGE_XDOUBLE(STN_proportion)
   SGE_XDOUBLE(STN_adjusted_proportion)
   SGE_XDOUBLE(STN_target_proportion)
   SGE_XDOUBLE(STN_current_proportion)
   SGE_XDOUBLE(STN_adjusted_usage)
   SGE_XDOUBLE(STN_combined_usage)
   SGE_XULONG(STN_pass0_seqno)
   SGE_XULONG(STN_pass1_seqno)
   SGE_XULONG(STN_pass2_seqno)
   SGE_XULONG(STN_sum_priority)
   SGE_XDOUBLE(STN_actual_proportion)
   SGE_DOUBLE(STN_m_share)
   SGE_DOUBLE(STN_last_actual_proportion)
   SGE_DOUBLE(STN_adjusted_current_proportion)
   SGE_XULONG(STN_temp)
   SGE_DOUBLE(STN_stt)
   SGE_DOUBLE(STN_ostt)                                                   
   SGE_DOUBLE(STN_ltt)                                                    
   SGE_DOUBLE(STN_oltt)                                                   
   SGE_DOUBLE(STN_shr)                                                    
   SGE_XULONG(STN_ref)                                                    
   SGE_XULONG(STN_jobid)                                                  
   SGE_XULONG(STN_taskid)                                                 
   SGE_RLIST(STN_usage_list,UA_Type)                                     
   SGE_XULONG(STN_version)
   /* IDL 
     ShareTreeNode newNode(in string name, in unsigned long shares)
     raises(ObjDestroyed, Authentication, Error) context("sge_auth");
     ShareTreeNode newLeaf(in string name, in unsigned long shares)
     raises(ObjDestroyed, Authentication, Error) context("sge_auth");
   XIDL */
LISTEND 

NAMEDEF(STNN)
   NAME("STN_name")
   NAME("STN_type")
   NAME("STN_id")
   NAME("STN_shares")
   NAME("STN_children")
   NAME("STN_job_ref_count")
   NAME("STN_active_job_ref_count")
   NAME("STN_project")
   NAME("STN_proportion")
   NAME("STN_adjusted_proportion")
   NAME("STN_target_proportion")
   NAME("STN_current_proportion")
   NAME("STN_adjusted_usage")
   NAME("STN_combined_usage")
   NAME("STN_pass0_seqno")
   NAME("STN_pass1_seqno")
   NAME("STN_pass2_seqno")
   NAME("STN_sum_priority")
   NAME("STN_actual_proportion")
   NAME("STN_m_share")
   NAME("STN_last_actual_proportion")
   NAME("STN_adjusted_current_proportion")
   NAME("STN_temp")
   NAME("STN_stt")
   NAME("STN_ostt")
   NAME("STN_ltt")
   NAME("STN_oltt")
   NAME("STN_shr")
   NAME("STN_ref")
   NAME("STN_jobid")
   NAME("STN_taskid")
   NAME("STN_usage_list")
   NAME("STN_version")
NAMEEND

/* *INDENT-ON* */

#define STNS sizeof(STNN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_SHARE_TREE_NODEL_H */
