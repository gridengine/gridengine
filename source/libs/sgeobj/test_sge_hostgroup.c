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

#include "sgermon.h"
#include "cull.h"
#include "sge_all_listsL.h"
#include "sge_hostref.h"
#include "sge_hostgroup.h"

bool check_hostref_list(void) 
{
   bool ret = false;
   lList *initial = NULL;
   lList *modified = NULL;
   lList *answer_list = NULL;
   lList *add_hosts = NULL;
   lList *add_group = NULL;
   lList *rem_hosts = NULL;
   lList *rem_group = NULL;

   hostref_list_add(&initial, &answer_list, "hostA");
   hostref_list_add(&initial, &answer_list, "@A");
   hostref_list_add(&initial, &answer_list, "hostB");
   hostref_list_add(&initial, &answer_list, "@B");
   hostref_list_add(&initial, &answer_list, "hostC");
   hostref_list_add(&initial, &answer_list, "@C");

   hostref_list_add(&modified, &answer_list, "hostA");
   hostref_list_add(&modified, &answer_list, "@A");
   hostref_list_add(&modified, &answer_list, "hostC");
   hostref_list_add(&modified, &answer_list, "@C");
   hostref_list_add(&modified, &answer_list, "hostE");
   hostref_list_add(&modified, &answer_list, "@E");

   hostref_list_find_diff(modified, &answer_list, initial, 
                          &add_hosts, &rem_hosts, &add_group, &rem_group);

   if (answer_list == NULL &&
       lGetNumberOfElem(add_hosts) == 1 && 
       hostref_list_has_member(add_hosts, "hostE") &&
       lGetNumberOfElem(rem_hosts) == 1 &&
       hostref_list_has_member(add_hosts, "hostB") &&
       lGetNumberOfElem(add_group) == 1 &&
       hostref_list_has_member(add_hosts, "@E") &&
       lGetNumberOfElem(rem_group) == 1 &&
       hostref_list_has_member(add_hosts, "@B")) {
      ret = true;
   }

   add_hosts = lFreeList(add_hosts);
   rem_hosts = lFreeList(rem_hosts);
   add_group = lFreeList(add_group);
   rem_group = lFreeList(rem_group);
   return ret;
}

bool check_hostgroup_list(void) 
{
   bool ret = false;
   lList *hostgroup_list = NULL;
   lList *answer_list = NULL;

   {
      lListElem *a = NULL;
      lListElem *b = NULL;
      lListElem *c = NULL;
      lListElem *d = NULL;
      lListElem *e = NULL;
      lListElem *f = NULL;
      lListElem *g = NULL;
      lListElem *h = NULL;
      lListElem *i = NULL;
      lList *hostref = NULL;

      i = hostgroup_create(&answer_list, "@i", NULL);
      h = hostgroup_create(&answer_list, "@h", NULL);
      g = hostgroup_create(&answer_list, "@g", NULL);

      hostref_list_add(&hostref, &answer_list, "@i"); 
      hostref_list_add(&hostref, &answer_list, "@h"); 
      hostref_list_add(&hostref, &answer_list, "f"); 
      f = hostgroup_create(&answer_list, "@f", hostref);

      hostref = NULL;
      hostref_list_add(&hostref, &answer_list, "@f"); 
      hostref_list_add(&hostref, &answer_list, "@g"); 
      hostref_list_add(&hostref, &answer_list, "e"); 
      e = hostgroup_create(&answer_list, "@e", hostref);

      hostref = NULL;
      hostref_list_add(&hostref, &answer_list, "@e"); 
      hostref_list_add(&hostref, &answer_list, "d"); 
      d = hostgroup_create(&answer_list, "@d", hostref);

      hostref = NULL;
      hostref_list_add(&hostref, &answer_list, "@e"); 
      hostref_list_add(&hostref, &answer_list, "c"); 
      c = hostgroup_create(&answer_list, "@c", hostref);

      hostref = NULL;
      hostref_list_add(&hostref, &answer_list, "@c"); 
      hostref_list_add(&hostref, &answer_list, "b"); 
      b = hostgroup_create(&answer_list, "@b", hostref);

      hostref = NULL;
      hostref_list_add(&hostref, &answer_list, "@c"); 
      hostref_list_add(&hostref, &answer_list, "a"); 
      a = hostgroup_create(&answer_list, "@a", hostref);

      hostgroup_list = lCreateList("", HGRP_Type);
      lAppendElem(hostgroup_list, a);
      lAppendElem(hostgroup_list, b);
      lAppendElem(hostgroup_list, c);
      lAppendElem(hostgroup_list, d);
      lAppendElem(hostgroup_list, e);
      lAppendElem(hostgroup_list, f);
      lAppendElem(hostgroup_list, g);
      lAppendElem(hostgroup_list, h);
      lAppendElem(hostgroup_list, i);
   }

   {
      lList *used_hosts = NULL;
      lList *used_groups = NULL;
      lList *occupant_groups = NULL;
      lListElem *e;
      lList *hostref_list;

      e = hostgroup_list_locate(hostgroup_list, "@c");
      hostref_list = lGetList(e, HGRP_host_list);

fprintf(stderr, "hostref_list_find_used\n");
      hostref_list_find_used(hostref_list, &answer_list, hostgroup_list, 
                             &used_hosts, &used_groups);
      lWriteListTo(used_groups, stderr);
      used_groups = lFreeList(used_groups);

fprintf(stderr, "hostref_list_find_all_used\n");
      hostref_list_find_all_used(hostref_list, &answer_list, hostgroup_list, 
                                 &used_hosts, &used_groups);
      lWriteListTo(used_groups, stderr);
      used_groups = lFreeList(used_groups);

fprintf(stderr, "hostref_list_find_occupants\n");
      hostref_list_find_occupants(hostref_list, &answer_list, 
                                      hostgroup_list,&occupant_groups);
      lWriteListTo(occupant_groups, stderr);
      occupant_groups = lFreeList(occupant_groups);

fprintf(stderr, "hostref_list_find_all_occupants\n");
      hostref_list_find_all_occupants(hostref_list, &answer_list, 
                                      hostgroup_list,&occupant_groups);
      lWriteListTo(occupant_groups, stderr);
      occupant_groups = lFreeList(occupant_groups);
   }
   



   return ret;
}

int main(int argc, char *argv[])
{
   bool failed = false;

   DENTER_MAIN(TOP_LAYER, "execd");

   lInit(nmv);

   if (!failed) {
      failed = check_hostref_list();
   }
   if (!failed) {
      failed = check_hostgroup_list();
   }

   return failed ? 1 : 0;
}
