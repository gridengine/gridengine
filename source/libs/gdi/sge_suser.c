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

#include "sge.h"
#include "utility.h"
#include "def.h"
#include "sgermon.h"
#include "basis_types.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_answerL.h"
#include "sge_log.h"
#include "msg_qmaster.h"   
#include "sge_suserL.h"
#include "sge_suser.h"
#include "msg_qmaster.h"

lList *Master_SUser_List;

/****** gdi/suser/suser_list_add() ********************************************
*  NAME
*     suser_list_add() -- Add a new entry (uniq) entry into a list 
*
*  SYNOPSIS
*     lListElem* suser_list_add(lList **suser_list, lList **answer_list, 
*                               const char *suser_name) 
*
*  FUNCTION
*     This function creates a new CULL element for the user "susername" 
*     into the "suser_list". The newly created element will be returned.
*     If an element for this user already exists than this element will
*     be returned.
*
*  INPUTS
*     lList **suser_list     - SU_Type list 
*     lList **answer_list    - AN_Type list 
*     const char *suser_name - username 
*
*  RESULT
*     lListElem* - SU_Type element or NULL
******************************************************************************/
lListElem *suser_list_add(lList **suser_list, lList **answer_list,
                          const char *suser_name)
{
   lListElem *ret = NULL;

   if (suser_list != NULL) {
      ret = suser_list_find(*suser_list, suser_name);
      if (ret == NULL) {
         ret = lAddElemStr(suser_list, SU_name, suser_name, SU_Type);
      }
   }
   return ret;
}

/****** gdi/suser/suser_list_find() *******************************************
*  NAME
*     suser_list_find() -- find a user entry in a list 
*
*  SYNOPSIS
*     lListElem* suser_list_find(lList *suser_list, const char *suser_name) 
*
*  FUNCTION
*     This function tries to find the first entry for user "suser_name" in
*     the list "suser_list".
*
*  INPUTS
*     lList *suser_list      - SU_Type list 
*     const char *suser_name - username 
*
*  RESULT
*     lListElem* - SU_Type element pointer or NULL
******************************************************************************/
lListElem *suser_list_find(lList *suser_list, const char *suser_name)
{
   lListElem *ret = NULL;

   if (suser_list != NULL && suser_name != NULL) {
      ret = lGetElemStr(suser_list, SU_name, suser_name);
   }
   return ret;
}

/****** gdi/suser/suser_increase_job_counter() ********************************
*  NAME
*     suser_increase_job_counter() -- increase the users job counter 
*
*  SYNOPSIS
*     void suser_increase_job_counter(lListElem *suser) 
*
*  FUNCTION
*     The job counter within "suser" will be increased by one 
*
*  INPUTS
*     lListElem *suser - SU_Type list 
*
*  RESULT
*     void - NONE
******************************************************************************/
void suser_increase_job_counter(lListElem *suser)
{
   if (suser != NULL) {
      u_long32 jobs = lGetUlong(suser, SU_jobs) + 1;
      lSetUlong(suser, SU_jobs, jobs);
   }
}

/****** gdi/suser/suser_decrease_job_counter() ********************************
*  NAME
*     suser_decrease_job_counter() -- decrease the users job counter 
*
*  SYNOPSIS
*     void suser_decrease_job_counter(lListElem *suser) 
*
*  FUNCTION
*     The job counter within "suser" will be decreased by one 
*
*  INPUTS
*     lListElem *suser - SU_Type list 
*
*  RESULT
*     0, if the job counter was decreased
*     1, in case of an error (meaning the job counter was allready 0) 
******************************************************************************/
int suser_decrease_job_counter(lListElem *suser)
{
   if (suser != NULL) {
      u_long32 jobs = lGetUlong(suser, SU_jobs);
      if(jobs>0){
         jobs -= 1;
         lSetUlong(suser, SU_jobs, jobs);
         return 0;
      }
   }
   return 1;
}

/****** gdi/suser/suser_get_job_counter() *************************************
*  NAME
*     suser_get_job_counter() -- return the users job counter 
*
*  SYNOPSIS
*     u_long32 suser_get_job_counter(lListElem *suser) 
*
*  FUNCTION
*     Returns the current number of jobs registed for "suser" 
*
*  INPUTS
*     lListElem *suser - SU_Type element 
*
*  RESULT
*     u_long32 - number of jobs 
******************************************************************************/
u_long32 suser_get_job_counter(lListElem *suser)
{
   u_long32 ret = 0;

   if (suser != NULL) {
      ret = lGetUlong(suser, SU_jobs);
   }
   return ret;
}

/****** gdi/suser/suser_check_new_job() ************************************
*  NAME
*     suser_check_new_job() -- checks, if a job can be registered
*
*  SYNOPSIS
*     int suser_check_new_job(const lListElem *job, u_long32 max_u_jobs, 
*                                int force_registration) 
*
*  FUNCTION
*     This function checks whether a new "job" would exceed the maxium
*     number of allowed jobs per user ("max_u_jobs"). JB_owner of "job" 
*     is the username which will be used by this function to compare
*     the current number of registered jobs with "max_u_jobs". If the
*     limit would be exceeded than the function will return 1 otherwise 0.
*
*  INPUTS
*     const lListElem *job   - JB_Type element 
*     u_long32 max_u_jobs    - maximum number of allowed jobs per user 
*     int force_registration - force job registration 
*
*  RESULT
*     int - 1 => limit would be exceeded
*           0 => otherwise
******************************************************************************/
int suser_check_new_job(const lListElem *job, u_long32 max_u_jobs,
                           int force_registration)
{
   const char *submit_user = NULL;
   lListElem *suser = NULL;
   int ret = 1;

   DENTER(TOP_LAYER, "suser_check_new_job");
   submit_user = lGetString(job, JB_owner);
   suser = suser_list_add(&Master_SUser_List, NULL, submit_user);
   if (suser != NULL) {
      if(max_u_jobs == 0 || force_registration || 
         max_u_jobs > suser_get_job_counter(suser))
         ret = 0;
      else
         ret = 1;
/* 
      if (max_u_jobs > 0 && !force_registration &&
          max_u_jobs <= suser_get_job_counter(suser)) {
         ret = 1;
      } else {
         ret = 0;
      }
*/
   }      
   DEXIT;
   return ret;
}


/****** gdi/suser/suser_register_new_job() ************************************
*  NAME
*     suser_register_new_job() -- try to register a new job 
*
*  SYNOPSIS
*     int suser_register_new_job(const lListElem *job, u_long32 max_u_jobs, 
*                                int force_registration) 
*
*  FUNCTION
*     This function checks whether a new "job" would exceed the maxium
*     number of allowed jobs per user ("max_u_jobs"). JB_owner of "job" 
*     is the username which will be used by this function to compare
*     the current number of registered jobs with "max_u_jobs". If the
*     limit would be exceeded than the function will return 1 otherwise
*     it will increase the jobcounter of the job owner and return 0.
*     In some situation it may be necessary to force the incrementation
*     of the jobcounter (reading jobs from spool area). This may be done
*     with "force_registration".
*
*  INPUTS
*     const lListElem *job   - JB_Type element 
*     u_long32 max_u_jobs    - maximum number of allowed jobs per user 
*     int force_registration - force job registration 
*
*  RESULT
*     int - 1 => limit would be exceeded
*           0 => otherwise
******************************************************************************/
int suser_register_new_job(const lListElem *job, u_long32 max_u_jobs,
                           int force_registration)
{   
   int ret;
   lListElem *suser = NULL;
   const char *submit_user = NULL;

   DENTER(TOP_LAYER, "suser_register_new_job");
   submit_user = lGetString(job, JB_owner);
   suser = suser_list_find(Master_SUser_List, submit_user);

   ret = suser_check_new_job(job, max_u_jobs, force_registration);
   if( ret == 0){ 
      suser_increase_job_counter(suser);
   }

   DEXIT;
   return ret;
}

/****** gdi/suser/suser_get_job_count(const lListElem *job) **************************************
*  NAME
*     suser_job_count() - number of jobs for a given user
*
*  SYNOPSIS
*     void suser_job_count(const lListElem *job) 
*
*  FUNCTION
*
*  INPUTS
*     const lListElem *job - JB_Type element 
*
*  RESULT
*     number of jobs in the system
******************************************************************************/
int suser_job_count(const lListElem *job)
{
   const char *submit_user = NULL;
   lListElem *suser = NULL;
   int ret = 0;    

   DENTER(TOP_LAYER, "suser_unregister_job");
   submit_user = lGetString(job, JB_owner);  
   suser = suser_list_find(Master_SUser_List, submit_user);
   if (suser != NULL) {
      ret = suser_get_job_counter(suser);
   }
   DEXIT;
   return ret;
}


/****** gdi/suser/suser_unregister_job() **************************************
*  NAME
*     suser_unregister_job() -- unregister a job 
*
*  SYNOPSIS
*     void suser_unregister_job(const lListElem *job) 
*
*  FUNCTION
*     Decrease the jobcounter for the job owner of "job".
*
*  INPUTS
*     const lListElem *job - JB_Type element 
*
*  RESULT
*     0, everything went fine
*     1, the user was found, but no jobs were registered for the user
*     2, user not found 
******************************************************************************/
int suser_unregister_job(const lListElem *job)
{
   const char *submit_user = NULL;
   lListElem *suser = NULL;
   int ret = 2;    

   DENTER(TOP_LAYER, "suser_unregister_job");
   submit_user = lGetString(job, JB_owner);  
   suser = suser_list_find(Master_SUser_List, submit_user);
   if (suser != NULL) {
      ret = suser_decrease_job_counter(suser);
   }
   DEXIT;
   return ret;
}
