#ifndef __GDI_CHECKPERMISSIONS_H
#define __GDI_CHECKPERMISSIONS_H   
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

#define MANAGER_CHECK     (1<<0)
#define OPERATOR_CHECK    (1<<1)
/*#define USER_CHECK        (1<<2)
#define SGE_USER_CHECK (1<<3)
*/
int sge_gdi_get_mapping_name(const char *requestedHost, char *buf, int buflen);
                                            /* requestedHost is for getting information for this host */
int sge_gdi_check_permission(int option); 
                                            /* option is MANAGER_CHECK, OPERATOR_CHECK, USER_CHECK ... */
                                            
                                            /* returns TRUE on success, FALSE if user has not the rights */

#endif /* __GDI_CHECKPERMISSIONS_H */

