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
#include <string.h>

#include "sec.h"
#include "sge_job.h"
#include "sge_host.h"
#include "sge_manop.h"
#include "sgermon.h"

#include "sge_jobL.h"
#include "sge_usersetL.h"

/***********************************************************************/
int sge_manager(
const char *cp 
) {

   DENTER(TOP_LAYER, "sge_manager");

   if (!cp) {
      DEXIT;
      return -1;
   }

   if (sge_locate_manager(cp)) {
      DEXIT;
      return 0;
   }

   DEXIT;
   return -1;

}

/***********************************************************************/
int sge_operator(
const char *cp 

/*
   Note: a manager is implicitly an "operator".
 */

) {

   DENTER(TOP_LAYER, "sge_operator");

   if (!cp) {
      DEXIT;
      return -1;
   }

   if (sge_locate_operator(cp)) {
      DEXIT;
      return 0;
   }

   if (sge_locate_manager(cp)) {
      DEXIT;
      return 0;
   }

   DEXIT;
   return -1;

}

/***********************************************************************/
int sge_owner(
const char *cp,
const lList *lp 

/*
   Note: a manager/operator is implicitly an "owner".
 */
) {
   lListElem *ep;

   DENTER(TOP_LAYER, "sge_owner");

   if (!cp) {
      DEXIT;
      return -1;
   }

   if (!sge_operator(cp)) {
      DEXIT;
      return 0;
   }

   for_each(ep, lp) {
      DPRINTF(("comparing user >>%s<< vs. owner_list entry >>%s<<\n", cp, 
         lGetString(ep, US_name)));
      if (!strcmp(cp, lGetString(ep, US_name))) {
         DEXIT;
         return 0;
      }
   }

   DEXIT;
   return -1;
}

/***********************************************************************/
int sge_job_owner(
const char *user_name,
u_long32 job_number 

/*
   Determines if the user is an owner of a job.

   Note: a manager/operator is implicitly an "owner".

   returns : -1 if unable to locate the job.
   0 if a valid owner
   1 not a valid owner

 */

) {
   lListElem *jep;

   DENTER(TOP_LAYER, "sge_job_owner");

   if (!user_name) {
      DEXIT;
      return -1;
   }

   if (sge_operator(user_name) == 0) {
      DEXIT;
      return 0;
   }

   jep = sge_locate_job(job_number);
   if (!jep) {
      DEXIT;
      return -1;
   }

   if (strcmp(user_name, lGetString(jep, JB_owner))) {
      DEXIT;
      return 1;
   }

   DEXIT;
   return 0;
}
