#ifndef __SGE_CATEGORY_H
#define __SGE_CATEGORY_H
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

/*
** ------------ to be called from within the data model layer 
*/

int sge_add_job_category(lListElem *job, lList *acl_list);
int sge_delete_job_category(lListElem *job);
int sge_rebuild_job_category(lList *job_list, lList *acl_list);
void sge_free_job_category(void);

/*
** ------------ to be called from within the decision-making layer 
*/

void sge_reject_category(lRef cat);
int sge_is_job_category_rejected_(lRef cat);
int sge_is_job_category_rejected(lListElem *job);
int sge_reset_job_category(void);

#endif /* __SGE_CATEGORY_H */

