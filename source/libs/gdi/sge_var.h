#ifndef __SGE_VAR_H
#define __SGE_VAR_H
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

#include "sge_jobL.h"

#define VAR_PREFIX "__SGE_PREFIX__"
#define VAR_COMPLEX_PREFIX "SGE_COMPLEX_"

void var_list_dump_to_file(const lList *varl, FILE *file);

void var_list_copy_complex_vars_and_value(lList *varl,
                                          const lList* src_varl,
                                          const lList* cplx_list);

void var_list_copy_prefix_vars(lList *varl, 
                               const lList *src_varl,
                               const char *prefix, 
                               const char *new_prefix);

const char* var_list_get_string(lList *varl, const char *name);

void var_list_set_string(lList **varl, const char *name, const char *value);

void var_list_set_int(lList **varl, const char *name, int value);

void var_list_set_u32(lList **varl, const char *name, u_long32 value);

void var_list_set_sharedlib_path(lList **varl);

void var_list_remove_prefix_vars(lList *varl, const char *prefix);

#endif /* __SGE_VAR_H */
