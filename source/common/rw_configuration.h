#ifndef __RW_CONFIGURATION_H
#define __RW_CONFIGURATION_H
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

#define FLG_CONF_SPOOL  0x00000001L
#define FLG_CONF_STDOUT 0x00000002L

#define GID_RANGE_NOT_ALLOWED_ID 100

int write_configuration(int write_configuration, lList **alpp, char *fname, lListElem *conf_list, FILE *fpout, u_long32 flags);

lListElem *read_configuration(const char *fname, const char *conf_name, u_long32 flags);

const char *read_adminuser_from_configuration(const lListElem *el, const char *fname, const char *conf_name, u_long32 flags);

#endif /* __RW_CONFIGURATION_H */
