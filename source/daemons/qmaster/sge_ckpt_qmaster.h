#ifndef __SGE_CKPT_QMASTER_H
#define __SGE_CKPT_QMASTER_H
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



#include "sge_c_gdi.h" 

int ckpt_mod(lList **alpp, lListElem *new_ckpt, lListElem *ckpt, int add, char *ruser, char *rhost, gdi_object_t *object, int sub_command);

int ckpt_spool(lList **alpp, lListElem *ep, gdi_object_t *object);

int ckpt_success(lListElem *ep, lListElem *old_ep, gdi_object_t *object);

/* funtions called via gdi and inside the qmaster */
int sge_del_ckpt(lListElem *, lList **, char *, char *);

lListElem *sge_locate_ckpt(char *ckpt_name);

void sge_change_queue_version_qr_list(lList *nq, lList *oq, 
                                      const char *obj_name, 
                                      const char *ckpt_name);

int validate_ckpt(lListElem *ep, lList **alpp);

int check_ckpt_lic(int licensed_ckpts, int verbose);     

#endif /* __SGE_CKPT_QMASTER_H */

