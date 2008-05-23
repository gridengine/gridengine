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

#include <afxtempl.h>
#include <afxmt.h>
#include <winsock2.h>
#include "Job.h"
#include "JobList.h"

/****** C_JobList::C_JobList() ************************************************
*  NAME
*     C_JobList::C_JobList() -- constructor
*
*  SYNOPSIS
*     C_JobList::C_JobList()
*
*  FUNCTION
*     Initializes C_JobList object.
*
*  NOTES
*******************************************************************************/
C_JobList::C_JobList() : m_JobListMutex(FALSE, "ListMutex", NULL)
{
}

/****** C_JobList::AddJobToList() *********************************************
*  NAME
*     C_JobList::AddJobToList() -- adds new job to the global job list
*
*  SYNOPSIS
*     POSITION C_JobList::AddJobToList(C_Job *pJob)
*
*  FUNCTION
*     Adds the new job to the global job list. If the job (identified by
*     job_id, ja_task_id, pe_task_id) already exists in the job list,
*     function fails.
*
*  INPUTS
*     C_Job *pJob - the new job object that is to be added to the list
*
*  RESULT
*     POSITION - the position of the new job object in the job list. NULL if
*                the job already existed in the job list.
*
*  NOTES
*******************************************************************************/
POSITION C_JobList::AddJobToList(C_Job *pJob)
{
   C_Job    *pExistingJob = NULL;
   POSITION Pos = NULL;

   // lock access to job list
   CSingleLock singleLock(&m_JobListMutex);
   singleLock.Lock();

   pExistingJob = FindJobInList(*pJob);
   if(pExistingJob == NULL) {
      Pos = AddTail(pJob);
   }

   // unlock access to job list
   singleLock.Unlock();

   return Pos;
}

/****** C_JobList::RemoveJobFromList() *****************************************
*  NAME
*     C_JobList::RemoveJobFromList() -- removes job from the job list
*
*  SYNOPSIS
*     C_Job *C_JobList::RemoveJobFromList(C_Job &Job)
*
*  FUNCTION
*     Removes a job object from the job list. 
*
*  INPUTS
*     C_Job &pJob - a temporary job object that contains the job_id, ja_task_id
*                   and pe_task_id of the job object that is to be removed from
*                   the list.
*
*  RESULT
*     C_Job* - pointer to the job object that was removed from the list.
*              NULL if the job object was not found in the list.
*
*  NOTES
*******************************************************************************/
C_Job *C_JobList::RemoveJobFromList(C_Job &Job)
{
   POSITION Pos;
   POSITION PosOld;
   C_Job    *pJob = NULL;

   // lock access to job list
   CSingleLock singleLock(&m_JobListMutex);
   singleLock.Lock();

   // get job from job list
   Pos = GetHeadPosition();
   while(Pos != NULL) {
      PosOld = Pos;
      pJob = GetNext(Pos);
      if(pJob != NULL) {
         if(pJob->m_job_id == Job.m_job_id
            && pJob->m_ja_task_id == Job.m_ja_task_id
            && ((pJob->m_pe_task_id==NULL && Job.m_pe_task_id==NULL)
               || strcmp(pJob->m_pe_task_id, Job.m_pe_task_id)==0)
            && (pJob->m_JobStatus == js_Finished 
               || pJob->m_JobStatus == js_Failed
               || pJob->m_JobStatus == js_Deleted)) {
            RemoveAt(PosOld);
            Pos = NULL;
         } else {
            pJob = NULL;
         }
      }
   }

   // unlock access to job list
   singleLock.Unlock();

   return pJob;
}

/****** C_JobList::FindJobInList() ********************************************
*  NAME
*     C_JobList::FindJobInList() -- searches job in job list
*
*  SYNOPSIS
*     C_Job *C_JobList::FindJobInList(C_Job &Job)
*
*  FUNCTION
*     Searches a job object in the list
*
*  INPUTS
*     C_Job &pJob - a temporary job object that contains the job_id, ja_task_id
*                   and pe_task_id of the job object that is to be searched in 
*                   the list.
*
*  RESULT
*     C_Job* - pointer to a copy of the job object that was found in the list.
*              Delete this copy if you finished using it!
*              NULL if the job object was not found in the list.
*
*  NOTES
*    MT-Note: Lock list before calling this function!
*******************************************************************************/
C_Job *C_JobList::FindJobInList(C_Job &Job)
{
   POSITION Pos;
   C_Job    *pJob = NULL;

   // get job from job list that is not already executed
   Pos = GetHeadPosition();
   while(Pos) {
      pJob = GetNext(Pos);
      if(pJob) {
         if(pJob->m_job_id == Job.m_job_id
            && pJob->m_ja_task_id == Job.m_ja_task_id
            && ((pJob->m_pe_task_id==NULL && Job.m_pe_task_id==NULL)
               || strcmp(pJob->m_pe_task_id, Job.m_pe_task_id)==0)) {
            Pos = NULL;
         } else {
            pJob = NULL;
         }
      }
   }
   return pJob;
}

/****** C_JobList::GetFirstJobInReceivedState() ********************************
*  NAME
*     C_JobList::GetFirstJobInReceivedState() -- searches first job in 
*                                                js_Revceived state
*
*  SYNOPSIS
*     C_Job *C_JobList::GetFirstJobInReceivedState(C_Job &Job)
*
*  FUNCTION
*     Searches the first job in js_Received state in the list
*
*  RESULT
*     C_Job* - pointer to the first job object in js_Received state in the list.
*              NULL if no object in js_Received state was found in the list.
*
*  NOTES
*    MT-Note: Lock list before calling this function!
*******************************************************************************/
C_Job* C_JobList::GetFirstJobInReceivedState()
{
   POSITION Pos;
   C_Job    *pJob = NULL;

   Pos = GetHeadPosition();
   while(Pos) {
      pJob = GetNext(Pos);
      if(pJob) {
         if(pJob->m_JobStatus == js_Received) {
            Pos = NULL;
         } else {
            pJob = NULL;
         }
      }
   }
   return pJob;
}
