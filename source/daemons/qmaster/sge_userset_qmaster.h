#ifndef __SGE_USERSET_QMASTER_H
#define __SGE_USERSET_QMASTER_H
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

int sge_add_userset(lListElem *ep, lList **alpp, lList **userset_list, 
                    char *ruser, char *rhost);

int sge_del_userset(lListElem *ep, lList **alpp, lList **userset_list, 
                    char *ruser, char *rhost);

int sge_mod_userset(lListElem *ep, lList **alpp, lList **userset_list, 
                    char *ruser, char *rhost);

int sge_verify_userset_entries(lList *u, lList **alpp, int start_up);

int sge_verify_department_entries(lList *userset_list, lListElem *new_userset, 
                                  lList **alpp);

int set_department(lList **alpp, lListElem *job, lList *userset_list);

int verify_acl_list(lList **alpp, lList *acl_list, const char *attr_name, 
                    const char *obj_descr, const char *obj_name);

#endif
