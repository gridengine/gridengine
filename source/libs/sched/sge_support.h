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

void decay_userprj_usage( lListElem *userprj, u_long seqno, u_long curr_time );
void calculate_decay_constant( int halftime );
int sge_for_each_share_tree_node( lListElem *node, sge_node_func_t func, void *ptr);
int sge_zero_node_fields( lListElem *node, void *ptr );
int sge_init_node_fields( lListElem *root );
double sge_calc_node_usage(lListElem *node, lList *user_list, lList *project_list, lList *config_list, u_long curr_time, char *projname);
void sge_calc_node_proportion(lListElem *node, double total_usage);
void _sge_calc_share_tree_proportions(lList *share_tree, lList *user_list, lList *project_list, lList *config_list, u_long curr_time);
void sge_calc_share_tree_proportions(lList *share_tree, lList *user_list, lList *project_list, lList *config_list);
lListElem *search_userprj_node( lListElem *ep, char *username, char *projname, lListElem **pep);
lListElem *search_named_node( lListElem *ep, char *name ); 
void free_ancestors( ancestors_t *ancestors);
lListElem *search_named_node_path( lListElem *ep, char *path, ancestors_t *ancestors );

#endif /* __SGE_SUPPORT_H */
