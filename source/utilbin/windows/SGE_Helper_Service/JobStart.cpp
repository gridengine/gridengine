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
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>


#include "Job.h"
#include "JobList.h"
#include "JobStart.h"
#include "SGE_Helper_Service.h"
#include "Communication.h"

#define DESKTOP_ALL (DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | \
DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | \
DESKTOP_JOURNALPLAYBACK | DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | \
DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ALL (WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | \
WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | WINSTA_WRITEATTRIBUTES | \
WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS | WINSTA_ENUMERATE | \
WINSTA_READSCREEN | STANDARD_RIGHTS_REQUIRED)

#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | \
GENERIC_ALL)

// function forward declarations
static BOOL GetLogonSID(HANDLE hToken, PSID &pSid);
static void FreeLogonSID(PSID &pSid);
static BOOL AddAceToWindowStation(HWINSTA hWinsta, PSID pSid);
static BOOL AddAceToDesktop(HDESK hDesk, PSID pSid);
static BOOL RemoveAceFromWindowStation(HWINSTA hWinsta, PSID pSid);
static BOOL RemoveAceFromDesktop(HDESK hDesk, PSID pSid);

static BOOL GetJobStartModeFromConf(char **conf, int nconf);
static BOOL GetModeFromEnv(const char *mode, char **env, int nenv);
static void WriteEnvToFile(char *pszEnv, char *pszFile);
static DWORD RedirectStdHandles(const C_Job& Job, HANDLE &hStdout, HANDLE &hStderr);

// external variables
extern C_JobList       g_JobList;
extern C_Communication g_Communication;
extern HANDLE          g_hWinstaACLMutex;
extern HANDLE          g_hDeskACLMutex;

/****** JobStarterThread() ****************************************************
*  NAME
*     JobStarterThread() -- starting point of job starter thread
*
*  SYNOPSIS
*     DWORD WINAPI JobStarterThread(LPVOID lpParameter)
*
*  FUNCTION
*     This is the starting point for the job starter thread
*     It searches a job with js_Received from the job list and tries to
*     start it. It sends a response to the sge_shepherd if job was
*     successfully or unsuccessfully started.
*
*  INPUTS
*     LPVOID lpParameter - unused
*
*  RESULT
*     DWORD - exit status of the job starter thread
*     0:  no errors
*     >0: GetLastError() value
*
*  NOTES
*******************************************************************************/
DWORD WINAPI JobStarterThread(LPVOID lpParameter)
{
   C_Job    *pJob = NULL;
   DWORD    ret = 1;

   // lock access to job list
   CSingleLock singleLock(&g_JobList.m_JobListMutex);
   singleLock.Lock();

   // get job from job list that still is to be executed
   pJob = g_JobList.GetFirstJobInReceivedState();
   if(pJob != NULL) {
      pJob->m_JobStatus = js_ToBeStarted;
   }

   // unlock access to job list
   singleLock.Unlock();

   if(pJob != NULL) {
      // start job
      ret = StartJob(*pJob);
      
      // send exit code to sge_shepherd
      g_Communication.SendExitStatus(*pJob);
      g_Communication.ShutdownSocket(&(pJob->m_comm_sock));
   }
   return ret;
}

/****** StartJob() ************************************************************
*  NAME
*     StartJob() -- starts the job
*
*  SYNOPSIS
*     DWORD StartJob(C_Job &Job)
*
*  FUNCTION
*     Gives job user full access to the visible desktop, starts the job
*     waits for job end, reads usage data, withdraws access to visible
*     desktop from job user.
*
*  INPUTS
*     C_Job &Job - all informations about the job that is to be started
*
*  RESULT
*     DWORD - exit status of the job starter thread
*     0:  no errors
*     >0: GetLastError() value
*
*  NOTES
*******************************************************************************/
DWORD StartJob(C_Job &Job)
{
   STARTUPINFO         si; 
   PROCESS_INFORMATION pi; 
   DWORD               dwWait;
   HANDLE              hToken  = INVALID_HANDLE_VALUE;
   HANDLE              hStdout = INVALID_HANDLE_VALUE;
   HANDLE              hStderr = INVALID_HANDLE_VALUE;
   char                *pszEnv = NULL;
   char                *pszCmdLine = NULL;
   char                szError[4096];
   char                szErrorPart[1024];
   BOOL                bError      = TRUE;
   DWORD               dwError     = ERROR_SUCCESS;
   HWINSTA             hWinstaSave = NULL;
   HWINSTA             hWinsta     = NULL;
   HDESK               hDesk       = NULL;
   PSID                pSid        = NULL;
   BOOL                bResult     = FALSE;
   char                *pFileName  = NULL;
   const char          *pszCurDir  = NULL;
   DWORD               BytesRead   = 0;
   BOOL                bBackgndMode = FALSE;
   CSingleLock         singleLock(&g_JobList.m_JobListMutex);

   if(GetJobStartModeFromConf(Job.conf, Job.nconf) == FALSE) {
      Job.m_JobStatus = js_Failed;
      return 1;
   }

   // Build environment as local Administrator
   Job.BuildEnvironment(pszEnv);
   Job.BuildCommandLine(pszCmdLine);
   pszCurDir = Job.GetConfValue("cwd");

   ZeroMemory(&si, sizeof(si));
   ZeroMemory(&pi, sizeof(pi));
   ZeroMemory(szErrorPart, sizeof(szErrorPart));

   if(!LogonUser(
         Job.user,
         Job.domain,
         Job.pass,
         LOGON32_LOGON_INTERACTIVE,
         LOGON32_PROVIDER_DEFAULT, 
         &hToken)) {
      sprintf(szErrorPart, "LogonUser failed:");
      goto Cleanup;
   }

   bBackgndMode = GetModeFromEnv("SGE_BACKGND_MODE", Job.env, Job.nenv);
   if(bBackgndMode == FALSE) {
      // Save a handle to the caller's current window station.
      if((hWinstaSave = GetProcessWindowStation()) == NULL) {
         sprintf(szErrorPart, "GetProcessWindowStation failed:");
         goto Cleanup;
      }

      // Get a handle to the interactive window station.
      hWinsta = OpenWindowStation(
         "winsta0",                   // the interactive window station 
         FALSE,                       // handle is not inheritable
         READ_CONTROL | WRITE_DAC);   // rights to read/write the DACL

      if(hWinsta == NULL) { 
         sprintf(szErrorPart, "OpenWindowStation failed:");
         goto Cleanup;
      }

      // To get the correct default desktop, set the caller's 
      // window station to the interactive window station.
      if(!SetProcessWindowStation(hWinsta)) {
         sprintf(szErrorPart, "SetProcessWindowStation(hWinsta) failed:");
         goto Cleanup;
      }

      // Get a handle to the interactive desktop.
      hDesk = OpenDesktop(
         "default",     // the interactive window station 
         0,             // no interaction with other desktop processes
         FALSE,         // handle is not inheritable
         READ_CONTROL | // request the rights to read and write the DACL
         WRITE_DAC | 
         DESKTOP_WRITEOBJECTS | 
         DESKTOP_READOBJECTS);

      // Restore the caller's window station.
      if(!SetProcessWindowStation(hWinstaSave)) {
         sprintf(szErrorPart, "SetProcessWindowStation(hWinstaSave) failed:");
         goto Cleanup;
      }

      if(hDesk == NULL) {
         sprintf(szErrorPart, "OpenDesktop failed:");
         goto Cleanup;
      }

      // Get the SID for the client's logon session.
      if(!GetLogonSID(hToken, pSid)) {
         sprintf(szErrorPart, "GetLogonSID failed:");
         goto Cleanup;
      }

      // Allow logon SID full access to interactive window station.
      if(!AddAceToWindowStation(hWinsta, pSid))  {
         sprintf(szErrorPart, "AddAceToWindowStation failed:");
         goto Cleanup;
      }

      // Allow logon SID full access to interactive desktop.
      if(!AddAceToDesktop(hDesk, pSid)) {
         sprintf(szErrorPart, "AddAceToDesktop failed:");
         goto Cleanup;
      }
   }

   // Impersonate client to ensure access to executable file.
   if(!ImpersonateLoggedOnUser(hToken))  {
      sprintf(szErrorPart, "ImpersonateLoggedOnUser failed:");
      goto Cleanup;
   }

   // Redirect stdout and stderr
   if(RedirectStdHandles(Job, hStdout, hStderr)!=0) {
      sprintf(szErrorPart, "Redirecting File Handles failed:");
      goto Cleanup;
   }

   si.cb          = sizeof(STARTUPINFO); 
   si.dwFlags     |= STARTF_USESTDHANDLES;
   si.hStdOutput  = hStdout;
   si.hStdError   = hStderr;
   si.wShowWindow = SW_SHOW; 

   if(bBackgndMode == TRUE) {
      si.lpDesktop = "";
   } else {
      si.lpDesktop   = "WinSta0\\Default"; 
   }

   // To avoid a race condition with a signal here, lock Job list, check if
   // this job has been killed in the meanwhile. If job is not locked, start
   // it (job may not get killed in the meanwhile, because list is locked),
   // but unlock before the blocking wait. After the job has been started,
   // killing it will not lead to unexpected resultst.
   singleLock.Lock();
   
   if(Job.m_JobStatus == js_Deleted) {
      singleLock.Unlock();
      goto Cleanup;
   }

   Job.m_hJobObject = CreateJobObject(NULL, NULL);

   // Launch the process in the client's logon session.
   bResult = CreateProcessAsUser(hToken,
      NULL,
      pszCmdLine,
      NULL,
      NULL,
      TRUE,
      NORMAL_PRIORITY_CLASS,//|CREATE_NO_WINDOW,//|CREATE_NEW_CONSOLE,
      pszEnv,
      pszCurDir,
      &si,
      &pi);

   if(!bResult) {
      dwError = GetLastError();

      RevertToSelf();
      sprintf(szErrorPart, "CreateProcessAsUser failed:");
      singleLock.Unlock();

      SetLastError(dwError);
      goto Cleanup;
   }

   AssignProcessToJobObject(Job.m_hJobObject, pi.hProcess);

   // End impersonation of client.
   RevertToSelf();

   Job.m_JobStatus = js_Started;
   Job.m_hProcess  = pi.hProcess;

   // unlock access to job list
   singleLock.Unlock();

   // Wait blocking for job end
   if(bResult && pi.hProcess != INVALID_HANDLE_VALUE) {
      WriteToLogFile("Waiting for job end.");
      dwWait = WaitForSingleObjectEx(pi.hProcess, INFINITE, FALSE);
      if(dwWait==WAIT_OBJECT_0) {
         Job.StoreUsage(pi.hProcess);
      }
      WriteToLogFile("Job ended.");
      CloseHandle(pi.hProcess); 
      CloseHandle(Job.m_hJobObject);
      Job.m_hProcess = INVALID_HANDLE_VALUE;
      Job.m_hJobObject = INVALID_HANDLE_VALUE;
      Job.m_JobStatus = js_Finished;
   }
   if(pi.hThread != INVALID_HANDLE_VALUE) {
      CloseHandle(pi.hThread); 
   }

   if(bBackgndMode == FALSE) {
      // Disallow logon SID full access to interactive desktop.
      if(!RemoveAceFromDesktop(hDesk, pSid)) {
         sprintf(szErrorPart, "RemoveAceFromDesktop failed:");
         goto Cleanup;
      }
      // Disallow logon SID full access to interactive window station.
      if(!RemoveAceFromWindowStation(hWinsta, pSid))  {
         sprintf(szErrorPart, "RemoveAceFromWindowStation failed:");
         goto Cleanup;
      }
   }
   if(bResult) {
      bError = FALSE;
   }

Cleanup: 
   if(bError == TRUE) {   
      char szLastError[501];

      dwError = GetLastError();

      if(dwError == 193) {
         // FormatMessage doesn't provide an error message for errno=193
         // The table of System Errors from the MSDN Libary tells us:
         // 193 Is not a valid application.  ERROR_BAD_EXE_FORMAT 
         strcpy(szLastError, "Is not a valid application.");
      } else {
         FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 
            NULL, dwError, 0, szLastError, 500, NULL);
      }

      if(strlen(szLastError) > 0) {
         if(szLastError[strlen(szLastError)-2] == '\r') {
            szLastError[strlen(szLastError)-2] = '\0';
         }
      } else {
         strcpy(szLastError, "(no error message available from system)");
      }

      sprintf(szError, "%s %s (errno=%d)",
         szErrorPart,
         szLastError,
         dwError);
      WriteToLogFile(szError);

      Job.szError     = strdup(szError);
      Job.m_JobStatus = js_Failed;
   }

   if(hStdout != INVALID_HANDLE_VALUE) {
      CloseHandle(hStdout);
   }
   if(hStderr != INVALID_HANDLE_VALUE) {
      CloseHandle(hStderr);
   }

   if(hWinstaSave != NULL) {
      SetProcessWindowStation(hWinstaSave);
   }

   // Free the buffer for the logon SID.
   if(pSid) {
      FreeLogonSID(pSid);
   }

   // Close the handles to the interactive window station and desktop.
   if(hWinsta != NULL) {
      CloseWindowStation(hWinsta);
   }
   if(hDesk != NULL) {
      CloseDesktop(hDesk);
   }
   // Close the handle to the client's access token.
   if(hToken != INVALID_HANDLE_VALUE) {
      CloseHandle(hToken);  
   }

   free(pszEnv);
   free(pszCmdLine);
   return dwError;
}

/****** GetLogonSID() *********************************************************
*  NAME
*     GetLogonSID() -- retrieve SID of logged on user
*
*  SYNOPSIS
*     static BOOL GetLogonSID(HANDLE hToken, PSID &pSid)
*
*  FUNCTION
*     Retrieves the SID of the logged on user represented by the logon
*     token.
*
*  INPUTS
*     HANDLE hToken - token of the logged on user
*     
*  OUTPUTS
*     PSID &pSid - SID of the logged on user
*
*  RESULT
*     BOOL - true if SID could be retrieved, false if not
*
*  NOTES
*******************************************************************************/
static BOOL GetLogonSID(HANDLE hToken, PSID &pSid) 
{
   BOOL          bRet = FALSE;
   DWORD         dwIndex;
   DWORD         dwLength = 0;
   PTOKEN_GROUPS ptg      = NULL;

   // Get required buffer size and allocate the TOKEN_GROUPS buffer.
   if(!GetTokenInformation(hToken, TokenGroups,
                           (LPVOID)ptg, 0, &dwLength)) {
      if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
         goto Cleanup;
      }
      ptg = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(), 
                                     HEAP_ZERO_MEMORY, dwLength);
      if(ptg == NULL) {
         goto Cleanup;
      }
   }

   // Get the token group information from the access token.
   if(!GetTokenInformation(hToken, TokenGroups,
                           (LPVOID)ptg, dwLength, &dwLength)) {
      goto Cleanup;
   }

   // Loop through the groups to find the logon SID.
   for(dwIndex = 0; dwIndex < ptg->GroupCount; dwIndex++) {
      if((ptg->Groups[dwIndex].Attributes&SE_GROUP_LOGON_ID) == SE_GROUP_LOGON_ID) {
         // Found the logon SID; make a copy of it.
         dwLength = GetLengthSid(ptg->Groups[dwIndex].Sid);
         pSid = (PSID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);

         if(pSid == NULL) {
             goto Cleanup;
         }
         if(!CopySid(dwLength, pSid, ptg->Groups[dwIndex].Sid)) {
             HeapFree(GetProcessHeap(), 0, (LPVOID)pSid);
             goto Cleanup;
         }
         break;
      }
   }
   bRet = TRUE;

Cleanup: 
   // Free the buffer for the token groups.
   if(ptg != NULL) {
      HeapFree(GetProcessHeap(), 0, (LPVOID)ptg);
   }
   return bRet;
}

/****** FreeLogonSID() ****************************************************++++
*  NAME
*     FreeLogonSID() -- frees the buffer of the SID 
*
*  SYNOPSIS
*     static void FreeLogonSID(PSID &pSid)
*
*  FUNCTION
*     Frees the buffer of the SID allocated by GetLogonSID()
*
*  INPUTS
*     PSID &pSid - the SID to be freed.
*     
*  RESULT
*     void - no result
*
*  NOTES
*******************************************************************************/
static void FreeLogonSID(PSID &pSid)
{
    HeapFree(GetProcessHeap(), 0, (LPVOID)pSid);
}

/****** AddAceToWindowStation() ************************************************
*  NAME
*     AddAceToWindowStation() -- adds the ACE of the job user to the ACL of the 
*                                visible window station.
*
*  SYNOPSIS
*     static BOOL AddAceToWindowStation(HWINSTA hWinsta, PSID pSid)
*
*  FUNCTION
*    Adds the ACE (Access Control Entry) of the job user to the ACL
*    (Access Control List) of the visible window station.
*
*  INPUTS
*     HWINSTA hWinsta - Handle of the visible window station
*     PSID    pSid    - SID (Security Identifier) of the job user
*     
*  RESULT
*     BOOL - true if adding succeeded, false if it failed
*
*  NOTES
*******************************************************************************/
static BOOL AddAceToWindowStation(HWINSTA hWinsta, PSID pSid)
{
   ACCESS_ALLOWED_ACE   *pAce;
   ACL_SIZE_INFORMATION aclSizeInfo;
   BOOL                 bDaclExist;
   BOOL                 bDaclPresent;
   BOOL                 bRet = FALSE;
   DWORD                dwNewAclSize;
   DWORD                dwSidSize = 0;
   DWORD                dwSdSizeNeeded;
   PACL                 pAcl;
   PACL                 pNewAcl;
   PSECURITY_DESCRIPTOR pSd = NULL;
   PSECURITY_DESCRIPTOR pSdNew = NULL;
   PVOID                pTempAce;
   SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
   unsigned int         i;

   if(WaitForSingleObject(g_hWinstaACLMutex, INFINITE) == WAIT_OBJECT_0) {
      __try
      {
         // Obtain the DACL for the window station.
         if(!GetUserObjectSecurity(hWinsta, &si, pSd, dwSidSize, &dwSdSizeNeeded)) {
            if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
               pSd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(),
                                       HEAP_ZERO_MEMORY, dwSdSizeNeeded);
            }

            if (pSd == NULL) {
               __leave;
            }

            pSdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(),
                                       HEAP_ZERO_MEMORY, dwSdSizeNeeded);

            if(pSdNew == NULL) {
               __leave;
            }

            dwSidSize = dwSdSizeNeeded;
            if(!GetUserObjectSecurity(hWinsta, &si, pSd, dwSidSize, &dwSdSizeNeeded)) {
               __leave;
            }
         } else {
            __leave;
         }

         // Create a new DACL.
         if(!InitializeSecurityDescriptor(pSdNew, SECURITY_DESCRIPTOR_REVISION)) {
            __leave;
         }

         // Get the DACL from the security descriptor.
         if(!GetSecurityDescriptorDacl(pSd, &bDaclPresent, &pAcl, &bDaclExist)) {
            __leave;
         }

         // Initialize the ACL.
         ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
         aclSizeInfo.AclBytesInUse = sizeof(ACL);

         // Call only if the DACL is not NULL.
         if (pAcl != NULL) {
            // get the file ACL size info
            if(!GetAclInformation(pAcl, (LPVOID)&aclSizeInfo,
                  sizeof(ACL_SIZE_INFORMATION), AclSizeInformation)) {
               __leave;
            }
         }

         // Compute the size of the new ACL.
         dwNewAclSize = aclSizeInfo.AclBytesInUse 
                        + (2*sizeof(ACCESS_ALLOWED_ACE)) 
                        + (2*GetLengthSid(pSid)) - (2*sizeof(DWORD));

         // Allocate memory for the new ACL.
         pNewAcl = (PACL)HeapAlloc(GetProcessHeap(),  
                           HEAP_ZERO_MEMORY, dwNewAclSize);

         if(pNewAcl == NULL) {
            __leave;
         }

         // Initialize the new DACL.
         if(!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION)) {
            __leave;
         }

         // If DACL is present, copy it to a new DACL.
         if(bDaclPresent) {
            // Copy the ACEs to the new ACL.
            if(aclSizeInfo.AceCount) {
               for(i=0; i < aclSizeInfo.AceCount; i++) {
                  if(!GetAce(pAcl, i, &pTempAce)) {
                     __leave;
                  }

                  // Add the ACE to the new ACL.
                  if(!AddAce(pNewAcl, ACL_REVISION, MAXDWORD,
                        pTempAce, ((PACE_HEADER)pTempAce)->AceSize)) {
                     __leave;
                  }
               }
            }
         }

         // Add the first ACE to the window station.
         pAce = (ACCESS_ALLOWED_ACE *)HeapAlloc(
               GetProcessHeap(),
               HEAP_ZERO_MEMORY,
               sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pSid) -
                     sizeof(DWORD));

         if (pAce == NULL)
            __leave;

         pAce->Header.AceType  = ACCESS_ALLOWED_ACE_TYPE;
         pAce->Header.AceFlags = CONTAINER_INHERIT_ACE |
                                    INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
         pAce->Header.AceSize  = (WORD)(sizeof(ACCESS_ALLOWED_ACE) +
                                    GetLengthSid(pSid) - sizeof(DWORD));
         pAce->Mask            = GENERIC_ACCESS;

         if (!CopySid(GetLengthSid(pSid), &pAce->SidStart, pSid))
            __leave;

         if (!AddAce(
               pNewAcl,
               ACL_REVISION,
               MAXDWORD,
               (LPVOID)pAce,
               pAce->Header.AceSize)
         )
            __leave;

         // Add the second ACE to the window station.
         pAce->Header.AceFlags = NO_PROPAGATE_INHERIT_ACE;
         pAce->Mask            = WINSTA_ALL;

         if (!AddAce(
               pNewAcl,
               ACL_REVISION,
               MAXDWORD,
               (LPVOID)pAce,
               pAce->Header.AceSize)
         )
            __leave;

         // Set a new DACL for the security descriptor.
         if (!SetSecurityDescriptorDacl(
               pSdNew,
               TRUE,
               pNewAcl,
               FALSE)
         )
            __leave;

         // Set the new security descriptor for the window station.
         if(!SetUserObjectSecurity(hWinsta, &si, pSdNew)) {
            DWORD dwError;
            char  szLastError[501];

            dwError = GetLastError();
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0, szLastError, 500, NULL);
            __leave;
         }
         // Indicate success.
         bRet = TRUE;
      }
      __finally
      {
         // Free the allocated buffers.

         if (pAce != NULL)
            HeapFree(GetProcessHeap(), 0, (LPVOID)pAce);

         if (pNewAcl != NULL)
            HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);

         if (pSd != NULL)
            HeapFree(GetProcessHeap(), 0, (LPVOID)pSd);

         if (pSdNew != NULL)
            HeapFree(GetProcessHeap(), 0, (LPVOID)pSdNew);
      }
      ReleaseMutex(g_hWinstaACLMutex);
   }
   return bRet;
}

/****** AddAceToDesktop() ******************************************************
*  NAME
*     AddAceToDesktop() -- adds the ACE of the job user to the ACL of the 
*                          visible desktop.
*
*  SYNOPSIS
*     static BOOL AddAceToDesktop(HDESK hDesk, PSID pSid)
*
*  FUNCTION
*    Adds the ACE (Access Control Entry) of the job user to the ACL
*    (Access Control List) of the visible desktop.
*
*  INPUTS
*     HDESK hDesk - Handle of the visible desktop
*     PSID  pSid  - SID (Security Identifier) of the job user
*     
*  RESULT
*     BOOL - true if adding succeeded, false if it failed
*
*  NOTES
*******************************************************************************/
static BOOL AddAceToDesktop(HDESK hDesk, PSID pSid)
{
   ACL_SIZE_INFORMATION aclSizeInfo;
   BOOL                 bDaclExist;
   BOOL                 bDaclPresent;
   BOOL                 bRet      = FALSE;
   DWORD                dwSidSize = 0;
   DWORD                dwNewAclSize;
   DWORD                dwSdSizeNeeded;
   PVOID                pTempAce;
   PACL                 pAcl;
   PACL                 pNewAcl;
   PSECURITY_DESCRIPTOR pSd    = NULL;
   PSECURITY_DESCRIPTOR pSdNew = NULL;
   SECURITY_INFORMATION si     = DACL_SECURITY_INFORMATION;
   unsigned int         i;

   if(WaitForSingleObject(g_hDeskACLMutex, INFINITE) == WAIT_OBJECT_0) {
      __try
      {
         // Obtain the security descriptor for the desktop object.
         if(!GetUserObjectSecurity(hDesk, &si, pSd, dwSidSize, &dwSdSizeNeeded)) {
            if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
               pSd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(),
                                       HEAP_ZERO_MEMORY, dwSdSizeNeeded);
               if(pSd == NULL) {
                  __leave;
               }

               pSdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(),
                                          HEAP_ZERO_MEMORY, dwSdSizeNeeded);
               if(pSdNew == NULL) {
                  __leave;
               }

               dwSidSize = dwSdSizeNeeded;
               if(!GetUserObjectSecurity(hDesk, &si, pSd,
                                       dwSidSize, &dwSdSizeNeeded)) {
                  __leave;
               }
            } else {
               __leave;
            }
         }

         // Create a new security descriptor.
         if(!InitializeSecurityDescriptor(pSdNew, SECURITY_DESCRIPTOR_REVISION)) {
            __leave;
         }

         // Obtain the DACL from the security descriptor.
         if (!GetSecurityDescriptorDacl(pSd, &bDaclPresent, &pAcl, &bDaclExist)) {
            __leave;
         }

         // Initialize.
         ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
         aclSizeInfo.AclBytesInUse = sizeof(ACL);

         if(pAcl != NULL) {
            // Determine the size of the ACL information.
            if (!GetAclInformation(pAcl, (LPVOID)&aclSizeInfo,
                     sizeof(ACL_SIZE_INFORMATION), AclSizeInformation)) {
               __leave;
            }
         }

         // Compute the size of the new ACL and allocate buffer
         dwNewAclSize = aclSizeInfo.AclBytesInUse
                        + sizeof(ACCESS_ALLOWED_ACE)
                        + GetLengthSid(pSid) - sizeof(DWORD);

         pNewAcl = (PACL)HeapAlloc(GetProcessHeap(), 
                        HEAP_ZERO_MEMORY, dwNewAclSize);

         if(pNewAcl == NULL) {
            __leave;
         }

         if(!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION)) {
            __leave;
         }

         // If DACL is present, copy it to a new DACL.
         if(bDaclPresent) {
            // Copy the ACEs to the new ACL.
            if(aclSizeInfo.AceCount) {
               for(i=0; i < aclSizeInfo.AceCount; i++) {
                  // Get an ACE.
                  if(!GetAce(pAcl, i, &pTempAce)) {
                     __leave;
                  }

                  // Add the ACE to the new ACL.
                  if(!AddAce(pNewAcl, ACL_REVISION, MAXDWORD, pTempAce,
                                          ((PACE_HEADER)pTempAce)->AceSize)) {
                     __leave;
                  }
               }
            }
         }

         // Add ACE to the DACL, set new DACL to the new security descriptor,
         // set new security descriptor for the desktop object.
         if(!AddAccessAllowedAce(pNewAcl, ACL_REVISION, DESKTOP_ALL, pSid)) {
            __leave;
         }
         if(!SetSecurityDescriptorDacl(pSdNew, TRUE, pNewAcl, FALSE)) {
            __leave;
         }
         if(!SetUserObjectSecurity(hDesk, &si, pSdNew)) {
            __leave;
         }
         bRet = TRUE;
      }
      __finally
      {
         // Free buffers.
         if(pNewAcl != NULL) {
            HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);
         }

         if(pSd != NULL) {
            HeapFree(GetProcessHeap(), 0, (LPVOID)pSd);
         }

         if(pSdNew != NULL) {
            HeapFree(GetProcessHeap(), 0, (LPVOID)pSdNew);
         }
      }
      ReleaseMutex(g_hDeskACLMutex);
   }
   return bRet;
}

/****** RemoveAceFromWindowStation() ******************************************
*  NAME
*     RemoveAceFromWindowStation() -- removes the ACE of the job user from the
*                                     ACL of the visible window station.
*
*  SYNOPSIS
*     static BOOL RemoveAceFromWindowStation(HWINSTA hWinsta, PSID pSid)
*
*  FUNCTION
*    Removes the ACE (Access Control Entry) of the job user from the ACL
*    (Access Control List) of the visible window station.
*
*  INPUTS
*     HWINSTA hWinsta - Handle of the visible window station
*     PSID    pSid    - SID (Security Identifier) of the job user
*     
*  RESULT
*     BOOL - true if removing succeeded, false if it failed
*
*  NOTES
*******************************************************************************/
static BOOL RemoveAceFromWindowStation(HWINSTA hWinsta, PSID pSid)
{
   SECURITY_DESCRIPTOR  *pSD = NULL;
   BOOL                 bSecRet;
   BOOL                 bDaclPresent = TRUE;
   BOOL                 bDaclDefaulted = FALSE;
   DWORD                SDLength = 0;
   DWORD                SDLengthNeeded = 0;
   PACL			    		pWinstaDacl;    
   LPVOID               pWinstaAce;
   SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
   BOOL                 bRet = TRUE;
   BOOL                 bEqual;
   PSID                 pListSid;
   int                  nDeleted = 0;

   if(WaitForSingleObject(g_hWinstaACLMutex, INFINITE) == WAIT_OBJECT_0) {
      __try
      {
         // Obtain DACL from Windows station, search for ACE, remove ACE from DACL
         bSecRet = GetUserObjectSecurity(hWinsta, &si, pSD, SDLength, &SDLengthNeeded);
         if(!bSecRet) {
            pSD = (SECURITY_DESCRIPTOR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, SDLengthNeeded);
         }
         bSecRet = GetUserObjectSecurity(hWinsta, &si, pSD, SDLengthNeeded, &SDLengthNeeded);
         bSecRet = GetSecurityDescriptorDacl(pSD, &bDaclPresent, &pWinstaDacl, &bDaclDefaulted);

         for(int i=pWinstaDacl->AceCount-1; i>=0; i--) {
            bSecRet = GetAce(pWinstaDacl, i, &pWinstaAce);
            if(((ACCESS_ALLOWED_ACE*)pWinstaAce)->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) {
               pListSid = (PSID)&(((ACCESS_ALLOWED_ACE*)pWinstaAce)->SidStart);
               bEqual = TRUE;
               bSecRet = IsValidSid(pSid);
               bSecRet = IsValidSid(pListSid);
               DWORD dwSidLength = GetLengthSid(pSid);
               DWORD dwListSidLength = GetLengthSid(pListSid);

               for(DWORD j=0; j<dwSidLength && j<dwListSidLength; j++) {
                  if(*((BYTE*)pListSid+j) != *((BYTE*)pSid+j)) {
                     bEqual = FALSE;
                     break;
                  }
               }
               if(bEqual) {
                  DeleteAce(pWinstaDacl, i);
                  nDeleted++;
                  if(nDeleted == 2) {
                     break;
                  }
               }
            }
         }
         SetUserObjectSecurity(hWinsta, &si, pSD);
      }

      __finally
      {
         if(pSD != NULL) {
            HeapFree(GetProcessHeap(), 0, (LPVOID)pSD);
         }
      }

      ReleaseMutex(g_hWinstaACLMutex);
   }         
   return bRet;
}

/****** RemoveAceFromDesktop() ************************************************
*  NAME
*     RemoveAceFromDesktop() -- removes the ACE of the job user from the
*                               ACL of the visible desktop
*
*  SYNOPSIS
*     static BOOL RemoveAceFromDesktop(HDESK hDesk, PSID pSid)
*
*  FUNCTION
*    Removes the ACE (Access Control Entry) of the job user from the ACL
*    (Access Control List) of the visible desktop
*
*  INPUTS
*     HDESK hDesk - Handle of the visible desktop
*     PSID  pSid  - SID (Security Identifier) of the job user
*     
*  RESULT
*     BOOL - true if removing succeeded, false if it failed
*
*  NOTES
*******************************************************************************/
static BOOL RemoveAceFromDesktop(HDESK hDesk, PSID pSid)
{
   SECURITY_DESCRIPTOR  *pSD = NULL;
   BOOL                 bSecRet;
   BOOL                 bDaclPresent = TRUE;
   BOOL                 bDaclDefaulted = FALSE;
   DWORD                SDLength = 0;
   DWORD                SDLengthNeeded = 0;
   PACL			    		pDeskDacl;    
   LPVOID               pDeskAce;
   SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
   BOOL                 bRet = TRUE;
   BOOL                 bEqual;
   PSID                 pListSid;

   if(WaitForSingleObject(g_hDeskACLMutex, INFINITE) == WAIT_OBJECT_0) {
      __try
      {
         // Obtain DACL from Windows station, search for ACE, remove ACE from DACL
         bSecRet = GetUserObjectSecurity(hDesk, &si, pSD, SDLength, &SDLengthNeeded);
         if(!bSecRet) {
            pSD = (SECURITY_DESCRIPTOR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, SDLengthNeeded);
         }
         bSecRet = GetUserObjectSecurity(hDesk, &si, pSD, SDLengthNeeded, &SDLengthNeeded);
         bSecRet = GetSecurityDescriptorDacl(pSD, &bDaclPresent, &pDeskDacl, &bDaclDefaulted);

         for(DWORD i=0; i<pDeskDacl->AceCount; i++) {
            bSecRet = GetAce(pDeskDacl, i, &pDeskAce);
            if(((ACCESS_ALLOWED_ACE*)pDeskAce)->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) {
               pListSid = (PSID)&(((ACCESS_ALLOWED_ACE*)pDeskAce)->SidStart);
               bEqual = TRUE;
               bSecRet = IsValidSid(pSid);
               bSecRet = IsValidSid(pListSid);
               DWORD dwSidLength = GetLengthSid(pSid);
               DWORD dwListSidLength = GetLengthSid(pListSid);

               for(DWORD j=0; j<dwSidLength && j<dwListSidLength; j++) {
                  if(*((BYTE*)pListSid+j) != *((BYTE*)pSid+j)) {
                     bEqual = FALSE;
                     break;
                  }
               }
               if(bEqual) {
                  DeleteAce(pDeskDacl, i);
                  break;
               }
            }
         }
         SetUserObjectSecurity(hDesk, &si, pSD);
      }
      __finally
      {
         if(pSD != NULL) {
            HeapFree(GetProcessHeap(), 0, (LPVOID)pSD);
         }
      }
      ReleaseMutex(g_hDeskACLMutex);
   }
   return bRet;
}

/****** GetJobStartModeFromConf() *********************************************
*  NAME
*     GetJobStartModeFromConf() -- searches conf for display_win_gui and
*                                   retrieves the corresponding value.
*
*  SYNOPSIS
*     static BOOL GetJobStartModeFromConf(char **conf, int nconf)
*
*  FUNCTION
*    Searchs conf for the complex variable display_win_gui and retrieves
*    it's value.
*
*  INPUTS
*     char **conf - the configuration that is to be searched
*     int  nconf  - number of entries in configuration
*     
*  RESULT
*     BOOL - TRUE if the value of the complex variable display_win_gui is "1",
*            else FALSE.
*
*  NOTES
*******************************************************************************/
static BOOL GetJobStartModeFromConf(char **conf, int nconf)
{
   int             i;
   char            *ptr;
   char            *tmp;
   BOOL            bRet = FALSE;
   BOOL            bFound = FALSE;

   for(i=0; i<nconf && bFound==FALSE; i++) {
      tmp = strdup(conf[i]);
      ptr = strtok(tmp, "=");
      if(ptr && stricmp(ptr, "display_win_gui")==0) {
         ptr=strtok(NULL, "=");
         if(ptr && stricmp(ptr, "1")==0) {
            bRet = TRUE;
            bFound = TRUE;
         } 
      }
      free(tmp);
   }
   return bRet;
}

/****** GetModeFromEnv() ******************************************************
*  NAME
*     GetModeFromEnv() -- searches env for given variable and retrieves
*                         the corresponding value.
*
*  SYNOPSIS
*     static BOOL GetModeFromEnv(const char *mode, char **env, int nenv)
*
*  FUNCTION
*    Searchs env for the given variable and retrieves it's value.
*    
*
*  INPUTS
*     char **env - the environment that is to be searched
*     int  nenv  - number of entries in environment
*     
*  RESULT
*     BOOL - TRUE if the value of the environment variable is "1"
*            or "TRUE" (not case sensitive), else FALSE.
*
*  NOTES
*******************************************************************************/
static BOOL GetModeFromEnv(const char *mode, char **env, int nenv)
{
   int             i;
   char            *ptr;
   char            *tmp;
   BOOL            bRet = FALSE;
   BOOL            bFound = FALSE;

   for(i=0; i<nenv && bFound==FALSE; i++) {
      tmp = strdup(env[i]);
      ptr = strtok(tmp, "=");
      if(ptr && stricmp(ptr, mode)==0) {
         ptr=strtok(NULL, "=");
         if(ptr && (stricmp(ptr, "TRUE")==0 || stricmp(ptr, "1")==0)) {
            bRet = TRUE;
            bFound = TRUE;
         } 
      }
      free(tmp);
   }
   return bRet;
}

/****** RedirectStdHandles() **************************************************
*  NAME
*     RedirectStdHandles() -- Redirects stdout and stderr
*
*  SYNOPSIS
*    static DWORD RedirectStdHandles(const C_Job &Job, 
*                                    HANDLE &hStdout, HANDLE &hStderr)
*
*  FUNCTION
*    Redirects stdout and stderr of the job to files. The file names are
*    retrieved from the job's environment.
*
*  INPUTS
*     C_Job &Job - The job object of the job for whom the standard handles
*                  are to be redirected.
*
*  OUTPUTS
*     HANDLE &hStdout - The redirected stdout handle
*     HANDLE &hStderr - The redirected stderr handle
*     
*  RESULT
*     DWORD - 0: OK
*             1: Can't create stdout file
*             2: Can't create stderr file
*
*  NOTES
*******************************************************************************/
static DWORD RedirectStdHandles(const C_Job &Job, HANDLE &hStdout, HANDLE &hStderr)
{
   const char *pszStdout, *pszStderr, *pszMerge;
   int        iMerge = 0;
   int        ret = 0;

   try {
      pszStdout = Job.GetConfValue("stdout_path");
      pszStderr = Job.GetConfValue("stderr_path");
      pszMerge  = Job.GetConfValue("merge_stderr");
      if(pszMerge != NULL) {
         sscanf(pszMerge, "%d", &iMerge);
      }

      hStdout = CreateFile(
                  pszStdout,
                  GENERIC_WRITE,
                  FILE_SHARE_READ|FILE_SHARE_WRITE,
                  NULL,
                  OPEN_ALWAYS,
                  FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH,
                  NULL);
      if(hStdout == INVALID_HANDLE_VALUE) {
         throw 1;
      }
      SetFilePointer(hStdout, 1, NULL, FILE_END);
      SetHandleInformation(hStdout, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

      if(iMerge == 0) {
         hStderr = CreateFile(
                     pszStderr,
                     GENERIC_WRITE,
                     FILE_SHARE_READ|FILE_SHARE_WRITE,
                     NULL,
                     OPEN_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH,
                     NULL);
      } else {
         DuplicateHandle(GetCurrentProcess(), hStdout, 
                        GetCurrentProcess(), &hStderr,
                        0, TRUE, DUPLICATE_SAME_ACCESS);
      }
      if(hStderr == INVALID_HANDLE_VALUE) {
         throw 2;
      }
      SetFilePointer(hStderr, 1, NULL, FILE_END);
      SetHandleInformation(hStderr, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
   }
   catch(int ret_val) {
      ret = ret_val;
   }
   return ret;
}

/******************************************************************************
* Test/Debugging functions
******************************************************************************/
static void WriteEnvToFile(char *pszEnv, char *pszFile) 
{
   FILE *fp;
   char *ptr = pszEnv;

   fp = fopen(pszFile, "w+");
   if(fp) {
      while(*ptr != '\0' || *(ptr+1) != '\0') {
         if(*ptr=='\0') {
            fwrite("\n", 1, 1, fp);
         } else {
            fwrite(ptr, 1, 1, fp);
         }
         ptr++;
      }
      fclose(fp);
   }
}

void OpenPipe()
{
/*      
      // Copy current Job object to pipe
      HANDLE hReadParent, hWriteChild;
      HANDLE hWriteParent, hReadChild;
      HANDLE hDuplReadParent, hDuplWriteParent;
      SECURITY_ATTRIBUTES secAttr;

      ZeroMemory(&secAttr, sizeof(secAttr));
      secAttr.nLength        = sizeof(secAttr);
      secAttr.bInheritHandle = TRUE;

      // Create inheritable pipe
      if(!CreatePipe(&hReadParent, &hWriteChild, &secAttr, 10000)) {
         goto Cleanup;
      }
      // Create non-inheritable handle of one side of pipe
      DuplicateHandle(GetCurrentProcess(),
                        hReadParent,
                        GetCurrentProcess(),
                        &hDuplReadParent,
                        DUPLICATE_SAME_ACCESS,
                        FALSE,
                        DUPLICATE_SAME_ACCESS);
      // Close inheritable duplicate of handle of one side of pipe
      CloseHandle(hReadParent);

      // Create inheritable pipe
      if(!CreatePipe(&hReadChild, &hWriteParent, &secAttr, 10000)) {
         goto Cleanup;
      }
      // Create non-inheritable handle of one side of pipe
      DuplicateHandle(GetCurrentProcess(),
                        hWriteParent,
                        GetCurrentProcess(),
                        &hDuplWriteParent,
                        DUPLICATE_SAME_ACCESS,
                        FALSE,
                        DUPLICATE_SAME_ACCESS);
      // Close inheritable duplicate of handle of one side of pipe
      CloseHandle(hWriteParent);
*/
}

//ClosePipe();
void ClosePipe()
{
/*
   BOOL bRet;
   int iRet;
         // First wait for data on the pipe
         WriteToLogFile("Now reading data from the pipe.");
         iRet = Job.Unserialize(hDuplReadParent);
         WriteToLogFile("Read data from the pipe: %d.", iRet);

         FlushFileBuffers(hDuplWriteParent);
         bRet = CloseHandle(hDuplWriteParent);
         WriteToLogFile("CloseHandle(hDuplWriteParent) returned %d", bRet);

         bRet = CloseHandle(hDuplReadParent);
         WriteToLogFile("CloseHandle(hDuplReadParent) returned %d.", bRet);
*/
}
