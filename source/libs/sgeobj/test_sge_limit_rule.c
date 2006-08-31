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

#include "sge_all_listsL.h"
#include "sgermon.h"
#include "sge_bootstrap.h"
#include "sge_hgroup.h"
#include "sge_href.h"
#include "sge_userset.h"
#include "sge_limit_rule.h"

typedef struct {
   const char* users;
   const char* projects;
   const char* pes;
   const char* hosts;
   const char* queues;
} filter_t;

typedef struct {
   filter_t rule;
   filter_t query;
   bool last;
} filter_test_t;

int main(int argc, char *argv[])
{
   int pos_tests_failed = 0;
   int neg_tests_failed = 0;
   int i = 0;
   lListElem *filter;

   filter_test_t positiv_test[] = {
   /* simple search */
      {{"user1,user2,user3", NULL, NULL, NULL, NULL}, {"user3", "*", "*", "*", "*"}, false},
      {{NULL, "project1,project2,project3", NULL, NULL, NULL}, {"*", "project2", "*", "*", "*"}, false},
      {{NULL, NULL, "pe1,pe2,pe3", NULL, NULL}, {"*", "*", "pe3", "*", "*"}, false},
      {{NULL, NULL, NULL, "h1,h2,h3", NULL}, {"*", "*", "*", "h3", "*"}, false},
      {{NULL, NULL, NULL, NULL, "queue1,queue2,queue3"}, {"*", "*", "*", "*", "queue1"}, false},
   /* wildcard search */
      {{"user1,user2,user3", NULL, NULL, NULL, NULL}, {"user*", "*", "*", "*", "*"}, false},
      {{NULL, "project1,project2,project3", NULL, NULL, NULL}, {"*", "project*", "*", "*", "*"}, false},
      {{NULL, NULL, "pe1,pe2,pe3", NULL, NULL}, {"*", "*", "pe*", "*", "*"}, false},
      {{NULL, NULL, NULL, "h1,h2,h3", NULL}, {"*", "*", "*", "h*", "*"}, false},
      {{NULL, NULL, NULL, NULL, "queue1,queue2,queue3"}, {"*", "*", "*", "*", "que*"}, false},
   /* wildcard definition */
      {{"user*", NULL, NULL, NULL, NULL}, {"user3", "*", "*", "*", "*"}, false},
      {{NULL, "project*", NULL, NULL, NULL}, {"*", "project2", "*", "*", "*"}, false},
      {{NULL, NULL, "pe*", NULL, NULL}, {"*", "*", "pe3", "*", "*"}, false},
      {{NULL, NULL, NULL, "h*", NULL}, {"*", "*", "*", "h1", "*"}, false},
      {{NULL, NULL, NULL, NULL, "queue*"}, {"*", "*", "*", "*", "queue1"}, false},
   /* wildcard definition, wildcard search */
      {{"user*", NULL, NULL, NULL, NULL}, {"u*", "*", "*", "*", "*"}, false},
      {{NULL, "project*", NULL, NULL, NULL}, {"*", "pro*", "*", "*", "*"}, false},
      {{NULL, NULL, "pe*", NULL, NULL}, {"*", "*", "p*", "*", "*"}, false},
      {{NULL, NULL, NULL, "host*", NULL}, {"*", "*", "*", "h*", "*"}, false},
      {{NULL, NULL, NULL, NULL, "queue*"}, {"*", "*", "*", "*", "qu*"}, false},
   /* hostgroup definition*/
      {{NULL, NULL, NULL, "@hgrp1", NULL}, {"*", "*", "*", "host1", "*"}, false},
      {{NULL, NULL, NULL, "@hgrp1", NULL}, {"*", "*", "*", "ho*", "*"}, false},
      {{NULL, NULL, NULL, "@hgr*", NULL}, {"*", "*", "*", "host1", "*"}, false},
      {{NULL, NULL, NULL, "@hgr*", NULL}, {"*", "*", "*", "hos*", "*"}, false},
      {{NULL, NULL, NULL, "@hgrp1", NULL}, {"*", "*", "*", "@hgrp1", "*"}, false},
      {{NULL, NULL, NULL, "host1", NULL}, {"*", "*", "*", "@hgrp1", "*"}, false},
      {{NULL, NULL, NULL, "ho*", NULL}, {"*", "*", "*", "@hgrp1", "*"}, false},
      {{NULL, NULL, NULL, "host1", NULL}, {"*", "*", "*", "@hgrp*", "*"}, false},
      {{NULL, NULL, NULL, "ho*", NULL}, {"*", "*", "*", "@hgrp*", "*"}, false},
      {{NULL, NULL, NULL, "ho*", NULL}, {"*", "*", "*", "*", "*"}, false},
   /* userset definition */
      {{"@userset1", NULL, NULL, NULL, NULL}, {"user1", "*", "*", "*", "*"}, false},
      {{"@userset1", NULL, NULL, NULL, NULL}, {"use*", "*", "*", "*", "*"}, false},
      {{"@users*", NULL, NULL, NULL, NULL}, {"user1", "*", "*", "*", "*"}, false},
      {{"@users*", NULL, NULL, NULL, NULL}, {"user*", "*", "*", "*", "*"}, false},
      {{"user1", NULL, NULL, NULL, NULL}, {"@userset1", "*", "*", "*", "*"}, false},
      {{"us*", NULL, NULL, NULL, NULL}, {"@userset1", "*", "*", "*", "*"}, false},
      {{"user1", NULL, NULL, NULL, NULL}, {"@use*", "*", "*", "*", "*"}, false},
      {{"use*", NULL, NULL, NULL, NULL}, {"@use*", "*", "*", "*", "*"}, false},
   /* project definition */
      {{NULL, "!*", NULL, NULL, NULL}, {"*", NULL, "*", "*", "*"}, false},
   /* end test */
      {{NULL, NULL, NULL, NULL, NULL}, {"*", "*", "*", "*", "*"}, true},
   };

   filter_test_t negativ_test[] = {
   /* simple search */
      {{"*,!user3", NULL, NULL, NULL, NULL}, {"user3", "*", "*", "*", "*"}, false},
      {{"user1,user2", NULL, NULL, NULL, NULL}, {"user3", "*", "*", "*", "*"}, false},
      {{NULL, "*,!project2", NULL, NULL, NULL}, {"*", "project2", "*", "*", "*"}, false},
      {{NULL, "project1,project3", NULL, NULL, NULL}, {"*", "project2", "*", "*", "*"}, false},
      {{NULL, NULL, "*,!pe3", NULL, NULL}, {"*", "*", "pe3", "*", "*"}, false},
      {{NULL, NULL, "pe1,pe2", NULL, NULL}, {"*", "*", "pe3", "*", "*"}, false},
      {{NULL, NULL, NULL, "*,!h3", NULL}, {"*", "*", "*", "h3", "*"}, false},
      {{NULL, NULL, NULL, "h1,h2", NULL}, {"*", "*", "*", "h3", "*"}, false},
      {{NULL, NULL, NULL, NULL, "*,!queue1"}, {"*", "*", "*", "*", "queue1"}, false},
      {{NULL, NULL, NULL, NULL, "queue2,queue3"}, {"*", "*", "*", "*", "queue1"}, false},
   /* wildcard definition, wildcard search */
      {{"!us*", NULL, NULL, NULL, NULL}, {"user*", "*", "*", "*", "*"}, false},
      {{NULL, "!pro*", NULL, NULL, NULL}, {"*", "project*", "*", "*", "*"}, false},
      {{NULL, NULL, "!p*", NULL, NULL}, {"*", "*", "pe*", "*", "*"}, false},
      {{NULL, NULL, NULL, "!h*", NULL}, {"*", "*", "*", "hos*", "*"}, false},
      {{NULL, NULL, NULL, NULL, "!qu*"}, {"*", "*", "*", "*", "que*"}, false},
   /* hostgroup definition*/
      {{NULL, NULL, NULL, "!@hgrp1", NULL}, {"*", "*", "*", "host1", "*"}, false},
      {{NULL, NULL, NULL, "!@hgrp1", NULL}, {"*", "*", "*", "ho*", "*"}, false},
      {{NULL, NULL, NULL, "!@hgr*", NULL}, {"*", "*", "*", "host1", "*"}, false},
      {{NULL, NULL, NULL, "!@hgr*", NULL}, {"*", "*", "*", "hos*", "*"}, false},
   /* userset definition */
      {{"!@userset1", NULL, NULL, NULL, NULL}, {"user1", "*", "*", "*", "*"}, false},
      {{"!@userset1", NULL, NULL, NULL, NULL}, {"use*", "*", "*", "*", "*"}, false},
      {{"!@users*", NULL, NULL, NULL, NULL}, {"user1", "*", "*", "*", "*"}, false},
      {{"!@users*", NULL, NULL, NULL, NULL}, {"user*", "*", "*", "*", "*"}, false},
   /* project definition */
      {{NULL, "*", NULL, NULL, NULL}, {"*", NULL, "*", "*", "*"}, false},
      {{NULL, "!*", NULL, NULL, NULL}, {"*", "project1", "*", "*", "*"}, false},
   /* end test */
      {{"!*", NULL, NULL, NULL, NULL}, {"*", "*", "*", "*", "*"}, true},
   };

   lList *hgroup_list;
   lListElem *hgroup;
   lList *userset_list;
   lListElem *userset;

   DENTER_MAIN(TOP_LAYER, "test_sge_limit_rule");

   bootstrap_mt_init();

   lInit(nmv);

   hgroup_list = lCreateList("" , HGRP_Type);
   hgroup = lCreateElem(HGRP_Type);
   lSetHost(hgroup, HGRP_name, "@hgrp1");
   lAddSubHost(hgroup, HR_name, "host1", HGRP_host_list, HR_Type);
   lAppendElem(hgroup_list, hgroup);

   userset_list = lCreateList("", US_Type);
   userset = lCreateElem(US_Type);
   lSetString(userset, US_name, "userset1");
   lAddSubStr(userset, UE_name, "user1", US_entries, UE_Type);
   lAppendElem(userset_list, userset);

   for (i=0; ; i++){
      lListElem *rule = lCreateElem(LIR_Type);
      filter_t rule_filter = positiv_test[i].rule;
      filter_t query_filter = positiv_test[i].query;

      if (LIRF_object_parse_from_string(&filter, rule_filter.users, NULL)) {
         lSetObject(rule, LIR_filter_users, filter);
      }
      if (LIRF_object_parse_from_string(&filter, rule_filter.projects, NULL)) {
         lSetObject(rule, LIR_filter_projects, filter);
      }
      if (LIRF_object_parse_from_string(&filter, rule_filter.pes, NULL)) {
         lSetObject(rule, LIR_filter_pes, filter);
      }
      if (LIRF_object_parse_from_string(&filter, rule_filter.hosts, NULL)) {
         lSetObject(rule, LIR_filter_hosts, filter);
      }
      if (LIRF_object_parse_from_string(&filter, rule_filter.queues, NULL)) {
         lSetObject(rule, LIR_filter_queues, filter);
      }
      if(!lirs_is_matching_rule(rule, query_filter.users, query_filter.projects, 
                                query_filter.pes, query_filter.hosts, query_filter.queues,
                                userset_list, hgroup_list)) {
         printf("positiv filter matching failed (test %d)\n", i+1);
         pos_tests_failed++;
      }
      if (positiv_test[i].last == true) {
         break;
      }
      lFreeElem(&rule);
   }
   printf("%d positiv test(s) failed\n", pos_tests_failed);

   for (i=0; ; i++){
      lListElem *rule = lCreateElem(LIR_Type);
      filter_t rule_filter = negativ_test[i].rule;
      filter_t query_filter = negativ_test[i].query;

      if (LIRF_object_parse_from_string(&filter, rule_filter.users, NULL)) {
         lSetObject(rule, LIR_filter_users, filter);
      }
      if (LIRF_object_parse_from_string(&filter, rule_filter.projects, NULL)) {
         lSetObject(rule, LIR_filter_projects, filter);
      }
      if (LIRF_object_parse_from_string(&filter, rule_filter.pes, NULL)) {
         lSetObject(rule, LIR_filter_pes, filter);
      }
      if (LIRF_object_parse_from_string(&filter, rule_filter.hosts, NULL)) {
         lSetObject(rule, LIR_filter_hosts, filter);
      }
      if (LIRF_object_parse_from_string(&filter, rule_filter.queues, NULL)) {
         lSetObject(rule, LIR_filter_queues, filter);
      }
      if(lirs_is_matching_rule(rule, query_filter.users, query_filter.projects, 
                                query_filter.pes, query_filter.hosts, query_filter.queues,
                                userset_list, hgroup_list)) {
         printf("negativ filter matching failed (test %d)\n", i+1);
         
         neg_tests_failed++;
      }
      if (negativ_test[i].last == true) {
         break;
      }
      lFreeElem(&rule);
   }
  
  lFreeList(&hgroup_list);
  lFreeList(&userset_list);

  printf("%d negativ test(s) failed\n", neg_tests_failed);
  DRETURN(pos_tests_failed + neg_tests_failed);
}
