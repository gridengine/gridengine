#ifndef __SGE_HOSTATTR_H__
#define __SGE_HOSTATTR_H__
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

#include "sge_attrL.h"

#define HOSTATTR_DEFAULT            0x0000
#define HOSTATTR_ALLOW_AMBIGUITY    0x0001
#define HOSTATTR_OWRITE_DEF_HOST    0x0002

/*
 * Internally this name should be handled as hostgroup name. Therefore it 
 * begins with a @ sign. The name does not cause a conflict with
 * user defined hostgroup names because in these names slashes are not
 * allowed.
 */ 
#define HOSTREF_DEFAULT             "@/default_name"

lListElem *
attr_str_create(lList **answer_list, const char *href, const char *value);

bool
attr_str_list_add(lList **this_list, lList **answer_list,
                  lListElem **attr, int flags, lList **href_list);

lListElem *
attr_str_list_find(const lList *this_list, const char *href);

bool
attr_str_list_find_value(const lList *this_list, lList **answer_list,
                         const char *hostname, const char **value);

bool
attr_str_list_verify(const lList *this_list, lList **answer_list,
                     bool *is_ambiguous);

bool
attr_str_list_append_to_dstring(const lList *this_list, dstring *string);

bool
attr_str_list_parse_from_string(lList **this_list, lList **answer_list,
                                const char *string, int flags);

bool
attr_str_has_hgroup_reference(const lList *this_list,
                              const char *host_or_group);

lListElem *
attr_str_list_locate(const lList *this_list, const char *host_or_group);

#endif /* __SGE_HOSTATTR_H__ */


