#ifndef __SGE_BOOTSTRAP_H
#define __SGE_BOOTSTRAP_H
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

#include "basis_types.h"
#include "sge.h"

#include "sge_dstring.h"

void bootstrap_mt_init(void);

const char *bootstrap_get_admin_user(void);
const char *bootstrap_get_default_domain(void);
bool        bootstrap_get_ignore_fqdn(void);
const char *bootstrap_get_spooling_method(void);
const char *bootstrap_get_spooling_lib(void);
const char *bootstrap_get_spooling_params(void);
const char *bootstrap_get_binary_path(void);
const char *bootstrap_get_qmaster_spool_dir(void);
const char *bootstrap_get_product_mode(void);

void bootstrap_set_admin_user(const char *value);
void bootstrap_set_default_domain(const char *value);
void bootstrap_set_ignore_fqdn(bool value);
void bootstrap_set_spooling_method(const char *value);
void bootstrap_set_spooling_lib(const char *value);
void bootstrap_set_spooling_params(const char *value);
void bootstrap_set_binary_path(const char *value);
void bootstrap_set_qmaster_spool_dir(const char *value);
void bootstrap_set_product_mode(const char *value);

bool sge_bootstrap(dstring *error_dstring);

#endif /* __SGE_BOOTSTRAP_H */
