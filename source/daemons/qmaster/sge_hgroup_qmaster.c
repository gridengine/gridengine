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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "sge.h"
#include "sgermon.h"
#include "sge_conf.h"
#include "sge_log.h"
#include "sge_c_gdi.h"
#include "sge_stringL.h"
#include "sge_string.h"
#include "sge_utility.h"
#include "sge_cuser.h"
#include "sge_hgroup_qmaster.h"
#include "sge_answer.h"
#include "sge_unistd.h"
#include "sge_hostname.h"
#include "sge_hgroup.h"
#include "sge_href.h"
#include "sge_cstring.h"
#include "sge_event_master.h"
#include "spool/sge_spooling.h"

#include "msg_common.h"
#include "msg_qmaster.h"

static 
bool hgroup_verify_hostlist_modification(lListElem *hgroup, lList **answer_list,
                                         lList *old_href_list, 
                                         lList **add_hosts, lList **rem_hosts, 
                                         lList **add_groups, lList **rem_groups) 
{
   bool ret = true;

   DENTER(TOP_LAYER, "hgroup_verify_hostlist_modification");
   if (hgroup != NULL) {
      lList *href_list = NULL;

      href_list = lGetList(hgroup, HGRP_host_list);
      if (href_list != NULL) {
         /*
          * Find all modifications
          */
         ret &= href_list_find_diff(href_list, answer_list, 
                                    old_href_list, add_hosts, 
                                    rem_hosts, add_groups, 
                                    rem_groups);

         if (ret) {
            lList *master_list = *(hgroup_list_get_master_list());

            /*
             * If there are additional hosts -> try to resolve them
             */
            if (*add_hosts != NULL) {
               ret &= href_list_resolve_hostnames(*add_hosts, answer_list);
            } 

            /*
             * Make sure all hgroup references are valid.
             */
            if (*add_groups != NULL) {
               ret &= hgroup_list_exists(master_list, answer_list, *add_groups);
            }

            /*
             * Try to find cycles in the definition 
             */
            if (ret) {
               lList *occupant_groups = NULL;
               lList *used_groups = NULL;

               ret &= hgroup_find_all_referencees(hgroup, answer_list,
                                                  master_list, 
                                                  &occupant_groups);
               ret &= hgroup_find_all_references(hgroup, answer_list,
                                                 master_list, NULL, 
                                                 &used_groups);
               ret &= href_list_add(&occupant_groups, answer_list,
                                    lGetHost(hgroup, HGRP_name));
               if (ret) {
                  if (occupant_groups != NULL && add_groups != NULL) {
                     lListElem *add_group = NULL;

                     for_each(add_group, *add_groups) {
                        const char *name = lGetHost(add_group, HR_name);

                        if (href_list_has_member(occupant_groups, name)) {
                           break;
                        }
                     }
                     if (add_group == NULL) {
                        /* 
                         * No cycle found => success
                         */
                        ;
                     } else {
                        ERROR((SGE_EVENT, "Hostgroup "SFQ" in specification "
                               "of "SFQ" would create a cycle\n", 
                               lGetHost(add_group, HR_name), 
                               lGetHost(hgroup, HGRP_name)));
                        answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX,
                                        ANSWER_QUALITY_ERROR);
                        ret = false;
                     }
                  } 
               } else {
                  ret = false;
               }
               occupant_groups = lFreeList(occupant_groups);
               used_groups = lFreeList(used_groups);
            } else {
               ret = false;
            }
         } else {
            ret = false;
         }  
      }
   } else {
      ret = false; 
   }
   DEXIT;
   return ret;
}

int 
hgroup_mod(lList **answer_list, lListElem *hgroup, lListElem *reduced_elem,
           int add, const char *remote_user, const char *remote_host, 
           gdi_object_t *object, int sub_command) 
{
   bool ret = true;
   int pos;

   DENTER(TOP_LAYER, "hgroup_mod");

   /*
    * Did we get a hostgroupname? 
    */
   if (ret) {
      pos = lGetPosViaElem(reduced_elem, HGRP_name);
      if (pos >= 0) {
         const char *name = lGetPosHost(reduced_elem, pos);

         if (add) {
            /* Check groupname for new hostgroups */
            if (!verify_str_key(answer_list, &name[1], "hostgroup")) {
               lSetHost(hgroup, HGRP_name, name); 
            } else {
               ERROR((SGE_EVENT, MSG_HGRP_GROUPXNOTGUILTY_S, name));
               answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, 
                               ANSWER_QUALITY_ERROR);
               ret = false;
            }
         } else {
            const char *old_name = lGetHost(hgroup, HGRP_name);

            /* Reject modify requests which try to change the groupname */
            if (sge_hostcmp(old_name, name)) {
               /* EB: move to message file */
               ERROR((SGE_EVENT, MSG_HGRP_NONAMECHANGE));
               answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX,
                               ANSWER_QUALITY_ERROR);
               ret = false;
            }
         }
      } else {
         ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS, 
                lNm2Str(HGRP_name), SGE_FUNC));
         answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN, 
                         ANSWER_QUALITY_ERROR);
         ret = false;
      }
   }

   /*
    * Is there a list of host references
    */
   if (ret) {
      pos = lGetPosViaElem(reduced_elem, HGRP_host_list);
      if (pos >= 0) {
         lList *old_href_list = NULL;
         lList *href_list = NULL;
         lList *add_hosts = NULL;
         lList *rem_hosts = NULL;
         lList *add_groups = NULL;
         lList *rem_groups = NULL;
   
         DPRINTF(("got new HGRP_host_list\n")); 

         /*
          * Save the old reference list
          * Modify the reference list according to the instructions
          * Identify all differences which have been applied
          */
         old_href_list = lCopyList("", lGetList(hgroup, HGRP_host_list));
         attr_mod_sub_list(answer_list, hgroup, HGRP_host_list, HR_name,
                           reduced_elem, sub_command, SGE_ATTR_HOSTNAME,
                           SGE_OBJ_HGROUP, 0); 
         href_list = lGetList(hgroup, HGRP_host_list);

         ret &= hgroup_verify_hostlist_modification(hgroup, answer_list,
                                                    old_href_list, 
                                                    &add_hosts, &rem_hosts, 
                                                    &add_groups, 
                                                    &rem_groups);
         old_href_list = lFreeList(old_href_list);

         if (ret) {
            /* 
             * EB: TODO Cluster Queue
             *
             * Create/Remove queue instances ...
             *
             *    for each CQ which uses a hostgroup of 'occupant_groups' {
             *       for each 'add_hosts' 
             *          add instance
             *       for each 'rem_hosts'
             *          delete instance
             *       for each 'add_groups' {
             *          find used hosts
             *          for each of these hosts
             *             add instance
             *       }
             *       for each 'rem_groups' {
             *          find used hosts
             *          for each of these hosts
             *             delete instance
             *       }
             *    }
             */
         } else {
            ret = false;
         }

         add_hosts = lFreeList(add_hosts);
         rem_hosts = lFreeList(rem_hosts);
         add_groups = lFreeList(add_groups);
         rem_groups = lFreeList(rem_groups);
      }  
   } 

   DEXIT;
   if (ret) {
      return 0;
   } else {
      return STATUS_EUNKNOWN;
   }
}

int 
sge_del_hgroup(lListElem *this_elem, lList **answer_list, 
               char *remote_user, char *remote_host) 
{
   int ret = true;

   DENTER(TOP_LAYER, "sge_del_hgroup");
   /*
    * Check all incoming parameter
    */
   if (this_elem != NULL && remote_user != NULL && remote_host != NULL) {
      const char* name = lGetHost(this_elem, HGRP_name);

      /*
       * What is the name ob the hostgroup which should be removed?
       */
      if (name != NULL) {
         lList *master_hgroup_list = *(hgroup_list_get_master_list());
#ifndef __SGE_NO_USERMAPPING__
         lList *master_cuser_list = *(cuser_list_get_master_list());
#endif
         lListElem *hgroup;

         /*
          * Does this hostgroup exist?
          */
         hgroup = hgroup_list_locate(master_hgroup_list, name);
         if (hgroup != NULL) {
            lList *href_list = NULL;
#ifndef __SGE_NO_USERMAPPING__
            lList *string_list = NULL;
#endif

            /*
             * Is it still referenced in another hostgroup?
             */
            ret &= hgroup_find_referencees(hgroup, answer_list, 
                                           master_hgroup_list, 
                                           &href_list);
            if (ret) {
               if (href_list != NULL) {
                  dstring string = DSTRING_INIT;

                  href_list_append_to_dstring(href_list, &string);
                  /* EB: TODO: move to msg file */
                  ERROR((SGE_EVENT, "denied: following hostgroups "
                         "still reference "SFQ": "SFN"\n", name,
                         sge_dstring_get_string(&string)));
                  answer_list_add(answer_list, SGE_EVENT, STATUS_EEXIST,
                                  ANSWER_QUALITY_ERROR);
                  sge_dstring_free(&string);
                  ret = false;
               }
            }
            href_list = lFreeList(href_list);


#ifndef __SGE_NO_USERMAPPING__
            /*
             * Is it still referenced in a cluster user object (user mapping)
             */ 
            ret &= cuser_list_find_hgroup_references(master_cuser_list,
                                                    answer_list, hgroup,
                                                    &string_list);
            if (ret) {
               if (string_list != NULL) {
                  dstring string = DSTRING_INIT;

                  cstring_list_append_to_string(string_list, &string); 
                  ERROR((SGE_EVENT, "denied: following user mapping entries "
                         "still reference "SFQ": "SFN"\n", name,
                         sge_dstring_get_string(&string)));
                  answer_list_add(answer_list, SGE_EVENT, STATUS_EEXIST,
                                  ANSWER_QUALITY_ERROR);
                  sge_dstring_free(&string);
                  ret = false;
               }
            }
            string_list = lFreeList(string_list);
#endif
                                                        

            /* 
             * EB: TODO: reject removal of still referenced hostgroups
             *    - cluster queue
             */
            if (ret) {
               ;
            }
            
            
            /*
             * Try to unlink the concerned spoolfile
             */
            if (ret) {
               if (!sge_unlink(HGROUP_DIR, name)) {
      
                  /*
                   * Let's remove the object => Success!
                   */

                  sge_add_event(NULL, 0, sgeE_HGROUP_DEL, 0, 0, name, NULL);
                  lRemoveElem(Master_HGroup_List, hgroup);

                  INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS, 
                        remote_user, remote_host, name , "host group entry"));
                  answer_list_add(answer_list, SGE_EVENT, STATUS_OK, 
                                  ANSWER_QUALITY_INFO);
               } else {
                  ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS,"host group entry",
                         name ));
                  answer_list_add(answer_list, SGE_EVENT, STATUS_EEXIST, 
                                  ANSWER_QUALITY_ERROR);
                  ret = false;
               }
            }
         } else {
            ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, 
                   "host group", name));
            answer_list_add(answer_list, SGE_EVENT, STATUS_EEXIST, 
                            ANSWER_QUALITY_ERROR);
            ret = false;
         }
      } else {
         ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS, 
                lNm2Str(HGRP_name), SGE_FUNC));
         answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN, 
                         ANSWER_QUALITY_ERROR);
         ret = false;
      } 
   } else {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN, 
                      ANSWER_QUALITY_ERROR);
      ret = false;        
   }

   DEXIT;
   if (ret) {
      return STATUS_OK;
   } else {
      return STATUS_EUNKNOWN;
   }
}

int 
hgroup_success(lListElem *hgroup, lListElem *old_hgroup, gdi_object_t *object) 
{
   const char *name = lGetHost(hgroup, HGRP_name);

   DENTER(TOP_LAYER, "hgroup_success");
   sge_add_event(NULL, 0, old_hgroup?sgeE_HGROUP_MOD:sgeE_HGROUP_ADD, 0, 
                 0, name, hgroup);
   DEXIT;
   return 0;
}

int 
hgroup_spool(lList **answer_list, lListElem *this_elem, gdi_object_t *object) 
{
   int ret = 0; 
   const char *name = lGetHost(this_elem, HGRP_name);

   DENTER(TOP_LAYER, "hgroup_spool");
   if (!spool_write_object(answer_list, spool_get_default_context(), this_elem, name,
                           SGE_TYPE_HGROUP)) {
      ERROR((SGE_EVENT, MSG_HGRP_ERRORWRITESPOOLFORGROUP_S, name));
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, 
                      ANSWER_QUALITY_ERROR);
      ret = 1;
   }
   DEXIT;
   return ret;
}

