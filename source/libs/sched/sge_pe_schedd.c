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

#include "sge_pe.h"
#include "sge_job.h"
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
#include "sge_host.h"

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
const lListElem *pep,
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




int sge_debit_job_from_pe(lListElem *pep, lListElem *jep, int slots)
{
   u_long n; 
  
   n = pe_get_slots_used(pep);
   n += slots;
   pe_set_slots_used(pep, n);

   return 0;
}


/****** sge_pe_schedd/pe_restricted() ******************************************
*  NAME
*     pe_match_static() -- Why not job to PE?
*
*  SYNOPSIS
*     int pe_match_static(lListElem *job, lListElem *pe, lList *acl_list, bool 
*     only_static_checks) 
*
*  FUNCTION
*     Checks if PE is suited for the job.
*
*  INPUTS
*     lListElem *job          - ??? 
*     lListElem *pe           - ??? 
*     lList *acl_list         - ??? 
*     bool only_static_checks - ??? 
*
*  RESULT
*     int - 0 ok 
*          -1 assignment will never be possible for all jobs of that category
*
*  NOTES
*     MT-NOTE: pe_restricted() is not MT safe 
*******************************************************************************/
int pe_match_static(
lListElem *job,
lListElem *pe,
lList *acl_list
) {
   int total_slots;

   DENTER(TOP_LAYER, "pe_match_static");

   total_slots = (int)lGetUlong(pe, PE_slots);
   if (total_slots == 0 /* || !num_in_range(total_slots, lGetList(job, JB_pe_range)) this is not relevant */  ) { 
      /* because there are not enough PE slots in total */
      DPRINTF(("total slots %d of PE \"%s\" not in range of job "u32"\n",
            total_slots, lGetString(pe, PE_name), lGetUlong(job, JB_job_number)));
         schedd_mes_add((lGetUlong(job, JB_job_number)), SCHEDD_INFO_TOTALPESLOTSNOTINRANGE_S,
            lGetString(pe, PE_name));
      DEXIT;
      return -1;
   }

   if (!sge_has_access_(lGetString(job, JB_owner), lGetString(job, JB_group),
         lGetList(pe, PE_user_list), lGetList(pe, PE_xuser_list), acl_list)) {
      DPRINTF(("job %d has no access to parallel environment \"%s\"\n",
            (int)lGetUlong(job, JB_job_number), lGetString(pe, PE_name)));
      schedd_mes_add(lGetUlong(job, JB_job_number), SCHEDD_INFO_NOACCESSTOPE_S, 
            lGetString(pe, PE_name));
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}

