#ifndef __SGE_SUPPORT_H
#define __SGE_SUPPORT_H
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

typedef struct {
   int depth;
   lListElem **nodes;
} ancestors_t;

typedef int (*sge_node_func_t) ( lListElem *node, void *ptr );

void decay_userprj_usage ( lListElem *userprj, lList *decay_list, u_long seqno, u_long curr_time );
void calculate_default_decay_constant ( int halftime );
void calculate_decay_constant ( double halftime, double *decay_rate, double *decay_constant );
int sge_for_each_share_tree_node ( lListElem *node, sge_node_func_t func, void *ptr );
int sge_zero_node_fields ( lListElem *node, void *ptr );
int sge_init_node_fields ( lListElem *root );
void sge_calc_node_proportion(lListElem *node, double total_usage);
double sge_calc_node_usage ( lListElem *node, lList *user_list, lList *project_list, lList *config_list, lList *decay_list, u_long curr_time, char *projname, u_long seqno );
void _sge_calc_share_tree_proportions ( lList *share_tree, lList *user_list, lList *project_list, lList *config_list, lList *decay_list, u_long curr_time );
void sge_calc_share_tree_proportions ( lList *share_tree, lList *user_list, lList *project_list, lList *config_list, lList *decay_list );
lListElem *search_userprj_node ( lListElem *ep, char *username, char *projname, lListElem **pep );
lListElem *search_named_node ( lListElem *ep, char *name );
lListElem *search_named_node_path ( lListElem *ep, char *path, ancestors_t *ancestors );
void free_ancestors( ancestors_t *ancestors);
#ifdef notdef
lListElem *search_ancestor_list ( lListElem *ep, char *name, ancestors_t *ancestors );
#endif
void sgeee_sort_jobs( lList **job_list );

#endif /* __SGE_SUPPORT_H */
