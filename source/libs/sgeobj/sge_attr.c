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

#include <math.h>

#include "basis_types.h"
#include "sgermon.h" 
#include "sge_string.h"
#include "sge_stringL.h"
#include "sge_log.h"
#include "sge_answer.h"
#include "sge_hostname.h"
#include "sge_attr.h"
#include "sge_href.h"
#include "sge_hgroup.h"
#include "sge_object.h"
#include "sge_stdlib.h"
#include "commlib.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#define HOSTATTR_LAYER TOP_LAYER

#define TEMPLATE_ATTR_IMPL(PREFIX, TYPE, DESCRIPTOR, HREF_NM, VALUE_NM)       \
                                                                              \
lListElem *                                                                   \
PREFIX##_create(lList **answer_list, const char *href, TYPE value)            \
{                                                                             \
   return attr_create(answer_list, href, &value,                              \
                      DESCRIPTOR, HREF_NM, VALUE_NM);                         \
}                                                                             \
                                                                              \
lListElem *                                                                   \
PREFIX##_list_find(const lList *this_list, const char *href)                  \
{                                                                             \
   return lGetElemHost(this_list, HREF_NM, href);                             \
}                                                                             \
                                                                              \
bool                                                                          \
PREFIX##_list_add(lList **this_list, lList **answer_list, lListElem **attr,   \
                  int flags, lList **ambiguous_href_list)                     \
{                                                                             \
   return attr_list_add(this_list, answer_list, attr, flags,                  \
                        ambiguous_href_list,                                  \
                        DESCRIPTOR, HREF_NM, VALUE_NM);                       \
}                                                                             \
                                                                              \
bool                                                                          \
PREFIX##_list_find_value(const lList *this_list, lList **answer_list,         \
                         const char *hostname, TYPE *value)                   \
{                                                                             \
   return attr_list_find_value(this_list, answer_list, hostname,              \
                               value, DESCRIPTOR, HREF_NM, VALUE_NM);         \
}                                                                             \
                                                                              \
bool                                                                          \
PREFIX##_list_append_to_dstring(const lList *this_list, dstring *string)      \
{                                                                             \
   return attr_list_append_to_dstring(this_list, string,                      \
                                      DESCRIPTOR, HREF_NM, VALUE_NM);         \
}                                                                             \
                                                                              \
bool                                                                          \
PREFIX##_list_parse_from_string(lList **this_list, lList **answer_list,       \
                                const char *string, int flags)                \
{                                                                             \
   return attr_list_parse_from_string(this_list, answer_list, string,         \
                                      flags, DESCRIPTOR, HREF_NM, VALUE_NM);  \
}                                                                             \
                                                                              \
lListElem *                                                                   \
PREFIX##_list_locate(const lList *this_list, const char *host_or_group)       \
{                                                                             \
   return attr_list_locate(this_list, host_or_group, HREF_NM);                \
}                                                                             \
                                                                              \
bool                                                                          \
PREFIX##_list_verify(const lList *this_list, lList **answer_list,             \
                     bool *is_ambiguous)                                      \
{                                                                             \
   return attr_list_verify(this_list, answer_list, is_ambiguous, HREF_NM);    \
}                                                                             

static lListElem *
attr_create(lList **answer_list, const char *href, void *value,
            const lDescr *descriptor, int href_nm, int value_nm);

static bool 
attr_list_add(lList **this_list, lList **answer_list, lListElem **attr, 
              int flags, lList **ambiguous_href_list,
              const lDescr *descriptor, int href_nm, int value_nm);

static bool
attr_list_find_value(const lList *this_list, lList **answer_list, 
                     const char *hostname, void *value_buffer,
                     const lDescr *descriptor, int href_nm, int value_nm);

static bool
attr_list_append_to_dstring(const lList *this_list, dstring *string,
                            const lDescr *descriptor, int href_nm, 
                            int value_nm);

static bool
attr_list_parse_from_string(lList **this_list, lList **answer_list,
                            const char *string, int flags,
                            const lDescr *descriptor, int href_nm, 
                            int value_nm);

static lListElem *
attr_list_locate(const lList *this_list, const char *host_or_group, 
                 int href_nm);

static bool
attr_list_verify(const lList *this_list, lList **answer_list,
                 bool *is_ambiguous, int href_nm);

/*
descriptor        ASTR_Type
href_nm           ASTR_href
value_nm          ASTR_value
*/
static lListElem *
attr_create(lList **answer_list, const char *href, void *value,
            const lDescr *descriptor, int href_nm, int value_nm)
{
   lListElem *ret = NULL;

   DENTER(HOSTATTR_LAYER, "attr_create");
   if (href != NULL) {
      lListElem *new_attr = lCreateElem(descriptor);

      if (new_attr != NULL) {
         lSetHost(new_attr, href_nm, href);
         if (value != NULL) {
            object_set_any_type(new_attr, value_nm, value);
         }
         ret = new_attr;
      } else {
         /* EB: TODO: error handling */
      }
   } else {
      /* EB: TODO: error handling */
   }
   DEXIT;
   return ret;
}

/*
descriptor        ASTR_Type
href_nm           ASTR_href
value_nm          ASTR_value
*/
static bool 
attr_list_add(lList **this_list, lList **answer_list, lListElem **attr, 
              int flags, lList **ambiguous_href_list,
              const lDescr *descriptor, int href_nm, int value_nm)
{
   bool ret = false;

   DENTER(HOSTATTR_LAYER, "attr_list_add");
   if (this_list != NULL && attr != NULL && *attr != NULL) {
      lListElem *attr_elem = NULL; 
      const char *href = NULL;
      bool is_hgroup = false; 
      bool created_list = false;

      href = lGetHost(*attr, href_nm);
      is_hgroup = sge_is_hgroup_ref(href);
      attr_elem = attr_list_locate(*this_list, href, href_nm);

      if (*this_list == NULL) {
         *this_list = lCreateList("", descriptor);
         created_list = true;
      }

      /*
       * HOSTREF_DEFAULT and host reference values can be added/changed
       * Hostgroup entries already contained in the list might be changed.
`      *
       * New hostgroup references might cause a conflict. We have to
       * make additional checks.
       */
      if (!strcmp(href, HOSTREF_DEFAULT) || 
          !is_hgroup ||
          (is_hgroup && attr_elem != NULL)) {
         const char *value = NULL; 

         object_get_any_type(*attr, value_nm, &value);
         if (attr_elem != NULL) {
            if (flags & HOSTATTR_OVERWRITE) {
               object_set_any_type(attr_elem, value_nm, &value);
               lFreeElem(*attr);
               *attr = attr_elem;
               ret = true;
            } else {
               /* EB: TODO: move to msg file */
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, "Attribute for "SFQ" is already defined\n", href));
               answer_list_add(answer_list, SGE_EVENT,
                               STATUS_ERROR1, ANSWER_QUALITY_ERROR);
            }
         } else {
            lAppendElem(*this_list, *attr);
            ret = true;
         }
      } else {
         lList *href_list = NULL;
         lList *host_list = NULL;
         lList *new_host_list = NULL;
         bool lret = true;
       
         /*
          * Create list of ambiguous hosts only 
          *  - if ambiguious entries are not allowed or
          *  - if the the caller of this functions expects this
          *    function to return it
          */ 
         if (!(flags & HOSTATTR_ALLOW_AMBIGUITY) ||
             ambiguous_href_list != NULL) {

            /*
             * Create host reference list of all used hostgroups
             * (except HOSTREF_DEFAULT, and host entries)
             */
            if (lret) {
               for_each(attr_elem, *this_list) {
                  const char *href = lGetHost(attr_elem, ASTR_href); 

                  if (strcmp(href, HOSTREF_DEFAULT) && 
                      sge_is_hgroup_ref(href)) {
                     lret &= href_list_add(&href_list, NULL, href);
                  }
               }
            }

            /*
             * Find all directly or indirectly referenced hosts for all 
             * hostgroups
             */
            if (lret && href_list != NULL) {
               lList *master_list = *(hgroup_list_get_master_list());
               lList *tmp_href_list = NULL; 

               lret &= href_list_find_all_references(href_list, NULL, 
                                                     master_list, &host_list, 
                                                     NULL); 
               lret &= href_list_add(&tmp_href_list, NULL, href);
               lret &= href_list_find_all_references(tmp_href_list, NULL,
                                                     master_list, 
                                                     &new_host_list, NULL);
               lFreeList(tmp_href_list);
            }

            /*
             * Find all host references which already have a value attached.
             * For all these hosts the new value in ambiguious.
             */
            if (lret && ambiguous_href_list != NULL && host_list != NULL &&
                new_host_list != NULL) { 
               lret = href_list_compare(new_host_list, NULL, host_list,
                                           NULL, NULL, ambiguous_href_list,
                                           NULL);
            }
         } 

         if (ambiguous_href_list != NULL &&
               lGetNumberOfElem(*ambiguous_href_list) >= 1 &&
               !(flags & HOSTATTR_ALLOW_AMBIGUITY)) {

            /* EB: TODO: move to msg file */
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, "Modification would result in ambiguous configuration\n"));
            answer_list_add(answer_list, SGE_EVENT,
                            STATUS_ERROR1, ANSWER_QUALITY_ERROR);
         } else {
            lAppendElem(*this_list, *attr);
            ret = true;
         }
      }

      if (created_list == true && ret == false) {
         lFreeList(*this_list);
      }
   } 
   DEXIT;
   return ret;
}

/*
descriptor        ASTR_Type
href_nm           ASTR_href
value_nm          ASTR_value
*/
static bool
attr_list_find_value(const lList *this_list, lList **answer_list, 
                     const char *hostname, void *value_buffer,
                     const lDescr *descriptor, int href_nm, int value_nm)
{
   bool ret = false;

   DENTER(HOSTATTR_LAYER, "attr_list_find_value");
   if (this_list != NULL && hostname != NULL) {
      lListElem *href = NULL;
   
      /*
       * Try to find a value for the concerned host
       */ 
      href = attr_list_locate(this_list, hostname, href_nm);
      if (href != NULL) {  
         object_get_any_type(href, value_nm, value_buffer);
         ret = true;
      } else {
         bool is_ambiguous = false;
         bool already_found = false;

         /*
          * Try to find a value for all hgroup definitions
          * if there was no host related value
          *
          * Exit the loop as soon as possible! This will be the case if
          * an ambiguous value for the concerned host will be found.
          */
         for_each(href, this_list) {
            const char *href_name = lGetHost(href, href_nm);
            lList *master_list = *(hgroup_list_get_master_list());
            bool lret = true;

            if (strcmp(href_name, HOSTREF_DEFAULT) && 
                sge_is_hgroup_ref(href_name)) {
               lList *tmp_href_list = NULL;
               lListElem *tmp_href = NULL;
               lList *host_list = NULL;

               href_list_add(&tmp_href_list, NULL, href_name);
               lret &= href_list_find_all_references(tmp_href_list, NULL,
                                                     master_list, &host_list,
                                                     NULL); 
               tmp_href = href_list_locate(host_list, hostname);
               if (tmp_href != NULL) {
                  if (already_found == false) {
                     already_found = true;
                     object_get_any_type(href, value_nm, value_buffer);
                  } else {
                     is_ambiguous = true;
                     break;
                  }
               }
               host_list = lFreeList(host_list);
               tmp_href_list = lFreeList(tmp_href_list);
            }
         }
         if (!is_ambiguous) {
            ret = true;
         } else {
            lListElem *tmp_href = NULL;

            /*
             * Use the default value
             */
            tmp_href = attr_list_locate(this_list, HOSTREF_DEFAULT, href_nm);
            if (tmp_href != NULL) {
               object_get_any_type(tmp_href, value_nm, value_buffer);
               ret = true;
            } else {
               /*
                * Should never happen.
                */
               /* EB: TODO: move to msg file */
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, "No default/hostgroup/host value found\n"));
               answer_list_add(answer_list, SGE_EVENT,
                               STATUS_ERROR1, ANSWER_QUALITY_ERROR);            
            }
         }
      }
      if (ret) {
         DTRACE;
      }
   }
   DEXIT;
   return ret;
}

/*
descriptor        ASTR_Type
href_nm           ASTR_href
value_nm          ASTR_value
*/
static bool
attr_list_append_to_dstring(const lList *this_list, dstring *string,
                            const lDescr *descriptor, int href_nm, int value_nm)
{
   bool ret = true;
   bool found_default = false;
   bool found_group = false;
   bool found_host = false;
   lListElem *attr = NULL;
   dstring default_string = DSTRING_INIT;
   dstring group_string = DSTRING_INIT;
   dstring host_string = DSTRING_INIT;
   const char *const comma = ",";

   DENTER(HOSTATTR_LAYER, "attr_list_append_to_dstring");

   for_each(attr, this_list) {
      const char *href;

      href = lGetHost(attr, href_nm);

      if (href != NULL && !strcmp(href, HOSTREF_DEFAULT)) {
         object_append_field_to_dstring(attr, NULL, &default_string, value_nm,
                                        '\0');
         found_default = true;
      } else if (sge_is_hgroup_ref(href)) {
         if (found_group) {
            sge_dstring_sprintf_append(&group_string, "%s", comma);
         }
         sge_dstring_sprintf_append(&group_string, "[%s=", href);
         object_append_field_to_dstring(attr, NULL, &group_string, value_nm, 
                                        '\0');
         sge_dstring_sprintf_append(&group_string, "]");
         found_group = true;
      } else {
         if (found_host) {
            sge_dstring_sprintf_append(&host_string, "%s", comma);
         }
         sge_dstring_sprintf_append(&host_string, "[%s=", href);
         object_append_field_to_dstring(attr, NULL, &host_string, value_nm,
                                        '\0');
         sge_dstring_sprintf_append(&host_string, "]");
         found_host = true;
      }
   }
   if (found_default) {
      const char *s = sge_dstring_get_string(&default_string);

      sge_dstring_sprintf_append(string, "%s", s);
   }
   if (found_group) {
      const char *s = sge_dstring_get_string(&group_string);

      if (found_default) {
         sge_dstring_sprintf_append(string, "%s", comma);
      }
      sge_dstring_sprintf_append(string, "%s", s);
   }
   if (found_host) {
      const char *s = sge_dstring_get_string(&host_string);

      if (found_group || found_default) {
         sge_dstring_sprintf_append(string, "%s", comma);
      }
      sge_dstring_sprintf_append(string, "%s", s);
   }
   if (!found_default && !found_group && !found_host) {
      sge_dstring_sprintf_append(string, "NONE");
   }
   sge_dstring_free(&default_string);
   sge_dstring_free(&group_string);
   sge_dstring_free(&host_string);
   DEXIT;
   return ret;
}

/*
descriptor        ASTR_Type
href_nm           ASTR_href
value_nm          ASTR_value
*/
static bool
attr_list_parse_from_string(lList **this_list, lList **answer_list,
                            const char *string, int flags,
                            const lDescr *descriptor, int href_nm, 
                            int value_nm)
{
   bool ret = true;
   DENTER(HOSTATTR_LAYER, "attr_list_parse_from_string");
  
   if (this_list != NULL && string != NULL) { 

      /* 
       * start         := value {',' group_value} .
       * group_value   := '[' group_or_host '=' value ']' . 
       * group_or_host := ['@'] name .
       * value         := <depends on listtype>
       * 
       * example: lic=5,fast=1,[@group=lic=4,fast=0],[b=lic=0] 
       *
       * lic=5,fast=1, 
       * @group=lic=4,fast=0], 
       * b=lic=0]
       * 
       * lic=5,fast=1
       * @group=lic=4,fast=0
       * b=lic=0
       *
       * default  lic=5,fast=1
       * @group   lic=4,fast=0
       * b        lic=0
       * 
       */

      if (!strcasecmp(string, "NONE")) {
         /* EB: TODO: Allow NONE only if it is a regular value */
         ;
      } else {
         struct saved_vars_s *strtok_context = NULL;
         char *token = NULL;
         char *next_token = NULL;
         bool is_first_token = true;
         bool is_last_token = false;

         next_token = sge_strtok_r(string, "[", &strtok_context);
         while (ret && (token = next_token)) {
            size_t length; 

            next_token = sge_strtok_r(NULL, "[", &strtok_context);
            if (next_token == NULL) {
               is_last_token = true;
            }

            /* remove all blanks from token */
#if 0 /* EB: space separated list will not be possible then */
            sge_strip_blanks(token);
#endif
            length = strlen(token);
            if (length >= 1) {
               char *href_name = NULL;
               char *value = NULL;
               bool first_is_default = true;
      
               /* 
                * All except the last token has to conatin a ',' as last
                * character in the string. This ',' has to be removed.
                */
               if (ret && !is_last_token) {
                  if (token[length - 1] == ',') {
                     token[length - 1] = '\0';
                     length--;
                  } else {
                     /* EB: TODO: error handling */
                     SGE_ADD_MSG_ID(sprintf(SGE_EVENT, "missing \',\' character\n"));
                     answer_list_add(answer_list, SGE_EVENT,
                                     STATUS_ERROR1, ANSWER_QUALITY_ERROR);
                     ret = false;
                  }
               }

               /* 
                * All except the first token has to end with a ']'. Also
                * this charcter has to be removed.
                */
               if (ret && !is_first_token) {
                  if (token[length - 1] == ']') {
                     token[length - 1] = '\0';
                     length--;
                  } else {
                     /* EB: TODO: error handling */
                     SGE_ADD_MSG_ID(sprintf(SGE_EVENT, "missing \']\' character in token %s\n", token));
                     answer_list_add(answer_list, SGE_EVENT,
                                     STATUS_ERROR1, ANSWER_QUALITY_ERROR);
                     ret = false;
                  }
               }
  
               /*
                * If the first token containes a ']' as last charcter than
                * a default value is missing! This is not a error but
                * we have to parse a group or host additionally
                */ 
               if (ret && is_first_token) {
                  if (token[length - 1] == ']') {
                     token[length - 1] = '\0';
                     length--;
                     first_is_default = false;
                  } 
               }

               /*
                * All but the first token has to contain a hostgroup
                * or a host reference in the beginning of the string 
                */
               if (ret) {
                  if (!is_first_token || !first_is_default) {
                     value = strchr(token, '=');
                     href_name = token;

                     if (value != NULL) {
                        value[0] = '\0';
                        value++;

                        /* 
                         * EB: TODO: additional checks might be done
                         *    - is valid hostname/ hostgroupname
                         *    - depending on type 'value' might be tested
                         */
                        if (1) {
                           ;
                        } else {
                           /* EB: TODO: error handling */
                           SGE_ADD_MSG_ID(sprintf(SGE_EVENT, "Invalid name\n"));
                           answer_list_add(answer_list, SGE_EVENT,
                                           STATUS_ERROR1, ANSWER_QUALITY_ERROR);
                           ret = false;
                        }
                     } else {
                        /* EB: TODO: error handling */
                        SGE_ADD_MSG_ID(sprintf(SGE_EVENT, "\'=\' has to sepatate host or group from value\n"));
                        answer_list_add(answer_list, SGE_EVENT,
                                        STATUS_ERROR1, ANSWER_QUALITY_ERROR);
                        ret = false;
                     }
                  } else {
                     href_name = HOSTREF_DEFAULT;
                     value = token;
                  }
               }

               /*
                * Parsing the token was successfull. We can create a new 
                * element.
                */
               if (ret) {
                  lListElem *attr_elem = NULL;
        
                  attr_elem = attr_create(answer_list, href_name, NULL,
                                          descriptor, href_nm, value_nm);
                  if (attr_elem != NULL) {
                     ret &= object_parse_field_from_string(attr_elem, 
                                                           answer_list,
                                                           value_nm, value);
                     if (ret) {
                        ret &= attr_list_add(this_list, answer_list,
                                             &attr_elem, flags, NULL,
                                             descriptor, href_nm, value_nm);
                     } else {
                        /* EB: TODO: error handling */
                        SGE_ADD_MSG_ID(sprintf(SGE_EVENT, "Error during parsing\n"));
                        answer_list_add(answer_list, SGE_EVENT,
                                        STATUS_ERROR1, ANSWER_QUALITY_ERROR);
                     }
                     if (!ret) {
                        attr_elem = lFreeElem(attr_elem);
                     }
                  } else {
                     ret = false;
                  }
               }
            } else {
               /* EB: TODO: error handling */
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, "Token is less than one character\n"));
               answer_list_add(answer_list, SGE_EVENT,
                               STATUS_ERROR1, ANSWER_QUALITY_ERROR);
               ret = false;
            }
            is_first_token = false;
         } 
         sge_free_saved_vars(strtok_context);
         strtok_context = NULL;
      }
   } else {
      /* EB: TODOD: error handling */
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, "Invalid parameter\n"));
      answer_list_add(answer_list, SGE_EVENT,
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      ret = false;
   } 

   DEXIT;
   return ret;
}

/*
href_nm           ASTR_href
*/
static lListElem *
attr_list_locate(const lList *this_list, const char *host_or_group, int href_nm)
{
   lListElem *ret = NULL;

   DENTER(HOSTATTR_LAYER, "attr_list_locate");
   if (this_list != NULL && host_or_group != NULL) {
      ret = lGetElemHost(this_list, href_nm, host_or_group);
   }
   DEXIT;
   return ret; 
}

/*
href_nm           ASTR_href
*/
static bool
attr_list_verify(const lList *this_list, lList **answer_list,
                 bool *is_ambiguous, int href_nm)
{
   bool ret = true;

   DENTER(HOSTATTR_LAYER, "attr_list_verify");
   if (this_list != NULL) {
      lList *href_list = NULL;
      lListElem *attr;
      size_t default_elements = 0;
      bool do_section_c = (is_ambiguous != NULL) ? true : false;

      /*
       * Check all entries if they are correct
       */
      for_each(attr, this_list) {
         const char *href = lGetHost(attr, href_nm);
         
         if (!strcmp(HOSTREF_DEFAULT, href)) {
            default_elements++;
         }         

         /*
          * a)
          *
          * NULL entries are not allowed
          */
         if (href == NULL) {
            ret = false; 

            /* 
             * For section c) we need a list of all hostgroup references 
             * No break in this case!
             */
            if (!do_section_c) {
               break;
            }
         }

         /* Needed for section c) */
         if (do_section_c && href != NULL && 
             sge_is_hgroup_ref(href) && 
             strcmp(HOSTREF_DEFAULT, href)) {
            ret &= href_list_add(&href_list, answer_list, href);
         }   
      }

      if (ret) {
         /*
          * b)
          *
          * Either the list has to be empty 
          * or it has to contain only one default entry
          */
         if (lGetNumberOfElem(this_list) != 0 && default_elements != 1) {
            ret = false;
         }
      }

      if (ret && do_section_c) {
         /*
          * c)
          *
          * We have to check if the attribute list containes ambiguous 
          * configurations for any host (hostgroup) it uses.
          */ 
         
         /* 
          * EB: TODO: implement 
          * href_list has to be used
          */   
      }
   } else {
      /*
       * d)
       *
       * NULL lists are valid - they represent NONE
       */
      ;
   }
   DEXIT;
   return ret;
}

TEMPLATE_ATTR_IMPL(str_attr, const char *, ASTR_Type, ASTR_href, ASTR_value) 

TEMPLATE_ATTR_IMPL(ulng_attr, u_long32, AULNG_Type, AULNG_href, AULNG_value) 

TEMPLATE_ATTR_IMPL(bool_attr, bool, ABOOL_Type, ABOOL_href, ABOOL_value) 

TEMPLATE_ATTR_IMPL(time_attr, const char *, ATIME_Type, ATIME_href, ATIME_value) 
TEMPLATE_ATTR_IMPL(mem_attr, const char *, AMEM_Type, AMEM_href, AMEM_value) 

TEMPLATE_ATTR_IMPL(inter_attr, const char *, AINTER_Type, AINTER_href, AINTER_value) 

TEMPLATE_ATTR_IMPL(strlist_attr, const char *, ASTRLIST_Type, ASTRLIST_href, ASTRLIST_value) 

TEMPLATE_ATTR_IMPL(usrlist_attr, const char *, AUSRLIST_Type, AUSRLIST_href, AUSRLIST_value) 

TEMPLATE_ATTR_IMPL(prjlist_attr, const char *, APRJLIST_Type, APRJLIST_href, APRJLIST_value) 

