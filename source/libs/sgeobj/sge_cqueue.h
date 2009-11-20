#ifndef __SGE_CQUEUE_H
#define __SGE_CQUEUE_H

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

#include "sge_cqueue_CQ_L.h"

enum {
   SGE_QI_TAG_DEFAULT = 0,

   /*
    * send delete event and remove from spool area
    */
   SGE_QI_TAG_DEL = 1,

   /*
    * send add events and make persistent
    */
   SGE_QI_TAG_ADD = 2,

   /*
    * send mod event and make persistent
    */
   SGE_QI_TAG_MOD = 4,

   /*
    * send mod event but skip spooling (no state value changed!)
    */
   SGE_QI_TAG_MOD_ONLY_CONFIG = 8
};

typedef struct _list_attribute_struct {
   int cqueue_attr;
   int qinstance_attr;
   int href_attr;
   int value_attr;
   int primary_key_attr;
   const char *name;
   bool is_sgeee_attribute;
   bool verify_client;
   bool (*verify_function)(lListElem *attr_elem, lList **answer_list, lListElem *cqueue);
} list_attribute_struct;

extern list_attribute_struct cqueue_attribute_array[];

lEnumeration *
enumeration_create_reduced_cq(bool fetch_all_qi, bool fetch_all_nqi);

bool
cqueue_name_split(const char *name, dstring *cqueue_name, dstring *host_domain,
                  bool *has_hostname, bool *has_domain);

char* cqueue_get_name_from_qinstance(const char *queue_instance);

lListElem *
cqueue_create(lList **answer_list, const char *name);

bool 
cqueue_is_href_referenced(const lListElem *this_elem, 
                          const lListElem *href, bool only_hostlist);

bool
cqueue_is_a_href_referenced(const lListElem *this_elem, 
                            const lList *href_list, bool only_hostlist);

bool
cqueue_list_add_cqueue(lList *this_list, lListElem *queue);

bool
cqueue_set_template_attributes(lListElem *this_elem, lList **answer_list);

lListElem *
cqueue_list_locate(const lList *this_list, const char *name);

lListElem *
cqueue_locate_qinstance(const lListElem *this_elem, const char *hostname);

lListElem *
cqueue_list_locate_qinstance_msg(lList *cqueue_list, const char *full_name, bool raise_error);

bool
cqueue_list_find_all_matching_references(const lList *this_list,
                                         lList **answer_list,
                                         const char *cqueue_pattern,
                                         lList **qref_list);

bool
cqueue_list_find_hgroup_references(const lList *this_list, 
                                   lList **answer_list,
                                   const lListElem *hgroup, 
                                   lList **string_list);

bool
cqueue_xattr_pre_gdi(lList *this_list, lList **answer_list);

bool
cqueue_verify_attributes(lListElem *cqueue, lList **answer_list,
                         lListElem *reduced_elem, bool in_master);

bool
cqueue_is_used_in_subordinate(const char *cqueue_name, const lListElem *cqueue);

void
cqueue_list_set_tag(lList *this_list, u_long32 tag_value, bool tag_qinstances);

lListElem *
cqueue_list_locate_qinstance(lList *cqueue_list, const char *full_name);

bool
cqueue_find_used_href(lListElem *this_elem, lList **answer_list, 
                      lList **href_list);

bool  
cqueue_trash_used_href_setting(lListElem *this_elem, lList **answer_list,
                               const char *hgroup_or_hostname);

bool
cqueue_purge_host(lListElem *this_elem, lList **answer_list, lList *attr_list, const char *hgroup_or_hostname);

bool
cqueue_sick(lListElem *cqueue, lList **answer_list, lList *hgroup_list, dstring *ds);

#endif /* __SGE_CQUEUE_H */
