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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "sge_peL.h"
#include "sge_jobL.h"
#include "sge_hostL.h"
#include "sgermon.h"
#include "sge_log.h"
#include "cull.h"
#include "cull_list.h"

#include "sge_pe_schedd.h"
#include "valid_queue_user.h"
#include "sge_range_schedd.h"
#include "schedd_monitor.h"
#include "sge_schedd_text.h"
#include "schedd_message.h"
#include "msg_schedd.h"
#ifdef WIN32NATIVE
#	define strcasecmp( a, b) stricmp( a, b)
#	define strncasecmp( a, b, n) strnicmp( a, b, n)
#endif

 /* -------------------------------------------------
   
   get number of slots per host from alloc rule 

   a return value of >0 allocate exactly this number at each host 
                      0 indicates an unknown allocation rule
                     -1 = ALLOC_RULE_FILLUP indicates that simply hosts should 
                         be filled up sequentially 
                     -2 = ALLOC_RULE_ROUNDROBIN indicates that a round robin 
                        algorithm with all available host is used 
*/
int sge_pe_slots_per_host(
lListElem *pep,
int slots 
) {
   const char *alloc_rule;
   int ret = 0;

   DENTER(TOP_LAYER, "sge_pe_slots_per_host");

   if (!pep) { /* seq jobs */
      DEXIT;
      return 1;
   }

   alloc_rule = lGetString(pep, PE_allocation_rule);

   if (isdigit((int)alloc_rule[0])) {
      ret = atoi(alloc_rule);
      if (ret==0) {
         ERROR((SGE_EVENT, MSG_PE_XFAILEDPARSINGALLOCATIONRULEY_SS , 
            lGetString(pep, PE_name), alloc_rule));
      }
   
      /* can we divide */
      if ( (slots % ret)!=0 ) {
         DPRINTF(("pe >%s<: cant distribute %d slots "
         "using \"%s\" as alloc rule\n", 
         lGetString(pep, PE_name), slots, alloc_rule)); 
         ret = 0; 
      }

      DEXIT;
      return ret;
   }

   if  (!strcasecmp(alloc_rule, "$pe_slots")) {
      DEXIT;
      return slots;
   }

   if  (!strcasecmp(alloc_rule, "$fill_up")) {
      DEXIT;
      return ALLOC_RULE_FILLUP;
   }
      
   if  (!strcasecmp(alloc_rule, "$round_robin")) {
      DEXIT;
      return ALLOC_RULE_ROUNDROBIN;
   }

   ERROR((SGE_EVENT, MSG_PE_XFAILEDPARSINGALLOCATIONRULEY_SS , 
      lGetString(pep, PE_name), alloc_rule));

   DEXIT;
   return 0;
}


/* ------------------------------------------------------------
   get number of slots per host from alloc rule
   a return value of 0 indicates an unknown allocation rule
   -------------------------------------------------------------*/

int or_sge_pe_slots_per_host(
lListElem *pep,
lList *hosts,
lListElem *h_elem,
int *sm 

) {
   lListElem *hep;
   const char *alloc_rule;
   int ret = 0;
   int available;
   DENTER(TOP_LAYER, "or_sge_pe_slots_per_host");

   alloc_rule = lGetString(pep, PE_allocation_rule);

   if (isdigit((int)alloc_rule[0])) {
      ret = atoi(alloc_rule);
      if (ret==0) {
         ERROR((SGE_EVENT, MSG_PE_XFAILEDPARSINGALLOCATIONRULEY_SS ,
            lGetString(pep, PE_name), alloc_rule));
      }
      DEXIT;
      return ret;
   }

   if (!strcasecmp(alloc_rule, "$pe_slots")) {
      if (!*sm) {
         for_each (hep, hosts) {
            available = lGetUlong (hep, EH_job_slots_free);
				    ret = (ret > available) ? ret : available;
         }
         *sm = 1;
      }
      else 
         ret = lGetUlong(h_elem, EH_job_slots_free);
      DEXIT;
      return ret;
   }

   ERROR((SGE_EVENT, MSG_PE_XFAILEDPARSINGALLOCATIONRULEY_SS ,
      lGetString(pep, PE_name), alloc_rule));

   DEXIT;
   return 0;
}



int sge_debit_job_from_pe(lListElem *pep, lListElem *jep, int slots)
{
   u_long n; 
  
   n = lGetUlong(pep, PE_used_slots);
   n += slots;
   lSetUlong(pep, PE_used_slots, n);

   return 0;
}


/* pe_restricted returns
      0 no restrictions to pe for this job 
      1 not enough slots in pe  
      2 no access permissions
*/
int pe_restricted(
lListElem *job,
lListElem *pe,
lList *acl_list 
) {
   int free_slots, total_slots;
   DENTER(TOP_LAYER, "pe_restricted");

   total_slots = (int)lGetUlong(pe, PE_slots);
   if (total_slots == 0 ||
      !num_in_range(total_slots, lGetList(job, JB_pe_range))) {
      /* because there are not enough PE slots in total */
      DPRINTF(("total slots of PE \"%s\" not in range of job %d\n",
            lGetString(pe, PE_name), (int)lGetUlong(job, JB_job_number)));
         schedd_add_message((lGetUlong(job, JB_job_number)), SCHEDD_INFO_TOTALPESLOTSNOTINRANGE_S, 
            lGetString(pe, PE_name));
      DEXIT;
      return 1;
   }

   free_slots = (int)lGetUlong(pe, PE_slots) - 
      (int)lGetUlong(pe, PE_used_slots);
   if (!lGetUlong(pe, PE_slots)
       || free_slots <= 0) {
      /* because there are not enough free PE slots */
      DPRINTF(("no free slots in PE \"%s\" for job %d\n",
            lGetString(pe, PE_name), (int)lGetUlong(job, JB_job_number)));
         schedd_add_message((lGetUlong(job, JB_job_number)), SCHEDD_INFO_PESLOTSNOTINRANGE_S,
            lGetString(pe, PE_name));
      DEXIT;
      return 1; 
   }

   if (lGetUlong(pe, PE_slots) && 
      !num_in_range(free_slots, lGetList(job, JB_pe_range))) {
      DPRINTF(("free slots of PE \"%s\" not in range of job %d\n",
            lGetString(pe, PE_name), (int)lGetUlong(job, JB_job_number)));
         schedd_add_message((lGetUlong(job, JB_job_number)), SCHEDD_INFO_PESLOTSNOTINRANGE_S, 
            lGetString(pe, PE_name));
      DEXIT;
      return 1;
   }

   if (!sge_has_access_(lGetString(job, JB_owner), lGetString(job, JB_group),
         lGetList(pe, PE_user_list), lGetList(pe, PE_xuser_list), acl_list)) {
      DPRINTF(("job %d has no access to parallel environment \"%s\"\n",
            (int)lGetUlong(job, JB_job_number), lGetString(pe, PE_name)));
      schedd_add_message(lGetUlong(job, JB_job_number), SCHEDD_INFO_NOACCESSTOPE_S, 
            lGetString(pe, PE_name));
      DEXIT;
      return 2;
   }

   DEXIT;
   return 0;
}

