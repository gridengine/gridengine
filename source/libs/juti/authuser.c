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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "msg_juti.h"
#include "juti.h"
#if defined(DARWIN_PPC) || defined(ALPHA5)
#  include <unistd.h>
#endif
#ifndef WINDOWS
#  include <pwd.h>
#  ifndef DARWIN_PPC
#     include <crypt.h>
#  endif
#endif
#ifdef WINDOWS
#  define _WIN32_WINNT 0x0500
#  include <windows.h>
#  include <Sddl.h>
#endif

#if 0
#define DEBUG 1
#endif

#define BUF_SIZE 1024

static void usage(void);
static void print_error(const char* format, ...);
static int  mygetline(char s[], int lim);
static auth_result_t do_system_authentication(const char *username, 
                                              const char *password, 
                                              int *uid, 
                                              int *gid,
                                              error_handler_t *error_handler);
#ifdef WINDOWS
static auth_result_t do_windows_system_authentication(const char *username, 
                                              const char *password, 
                                              char ***pppszUID, 
                                              char ***pppszGID,
                                              int  *nGIDs,
                                              error_handler_t *error_handler);

void setEcho(int flag) 
{
   HANDLE hConsole;
   DWORD  dwMode;

   hConsole = CreateFile("CONIN$", 
              GENERIC_READ|GENERIC_WRITE, 
              FILE_SHARE_READ|FILE_SHARE_WRITE,
              NULL,
              OPEN_EXISTING,
              0,
              NULL);

   if(hConsole != INVALID_HANDLE_VALUE) {
      GetConsoleMode(hConsole, &dwMode);
      if(flag) {
         dwMode |= ENABLE_ECHO_INPUT;
      } else {
         dwMode &= ~ENABLE_ECHO_INPUT;
      }
      SetConsoleMode(hConsole, dwMode);
      CloseHandle(hConsole);
   }
}
#endif

static void usage(void) {
   fprintf(stderr, "userverifier <method> <params>\n");
   fprintf(stderr, "\n");
   fprintf(stderr, "   <method>    \"pam\" or \"system\"\n");
   fprintf(stderr, "\n");
   fprintf(stderr, "Params for PAM authentification\n");
   fprintf(stderr, "  -s <servicename>   name of the pam service\n");
   fprintf(stderr, "\n");
   fprintf(stderr, "<username> and <password> are read from stdin\n");
   exit(1);
}

static void print_error(const char* format, ...) {
   va_list ap;
   va_start(ap, format);
   fprintf(stderr, MSG_AUTHUSER_ERROR);
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
}


int main(int argc, char** argv) {
   const char *auth_method = NULL;
   const char *service = NULL;
   char username[BUF_SIZE];
   char password[BUF_SIZE];
   int i = 0;
   int ret = 0;
   error_handler_t error_handler;
   int uid = 0;
   int gid = 0;
#ifdef WINDOWS
   char **ppszUID = NULL;
   char **ppszGID = NULL;
   int  nGIDs;
#endif

   error_handler.error = print_error;
   
   if(argc < 2 ) {
      print_error(MSG_AUTUSER_INVAILD_ARG_COUNT);
      usage();
   }
   auth_method = argv[1];

   if(strcmp(auth_method, "pam") == 0 ) {
      for(i=2; i < argc; i++) {
         if( strcmp(argv[i], "-s") == 0 ) {
            i++;
            if(i >= argc) {
               print_error(MSG_AUTUSER_MISSING_PAM_SERVICE);
               usage();
            }
            service = argv[i];
         } else {
            print_error(MSG_AUTUSER_UNKNOWN_PARAM_S, argv[i]);
            usage();
         }
      }
   } else if (strcmp(auth_method, "system" ) == 0 ) {
   } else {
      print_error(MSG_AUTUSER_UNKNOWN_AUTH_METHOD_S, auth_method);
      usage();
   }

   fprintf(stdout, "username: ");
   fflush(stdout);
   if(mygetline(username, BUF_SIZE-1) <= 0) {
      usage();
      return 1;
   }
   
   fprintf(stdout, "password: ");
   fflush(stdout);
   setEcho(0);
   if(mygetline(password, BUF_SIZE-1) < 0) {
      setEcho(1);
      usage();
      return 1;
   }
   fprintf(stdout,"\n");
   setEcho(1);
   
   if (strcmp(auth_method, "pam") == 0 ) {      
#ifndef WINDOWS
      ret = do_pam_authenticate(service, username, password, &error_handler);
      if(ret == JUTI_AUTH_SUCCESS) {
         struct passwd *pw = getpwnam(username);
         if(pw == NULL) {
            print_error(MSG_AUTHUSER_NO_PW_ENTRY_SS,
                        username, strerror(errno));
            return -1;
         }
         uid = pw->pw_uid;
         gid = pw->pw_gid;
      }
#else
      ret = JUTI_AUTH_ERROR;
      print_error(MSG_JUTI_PAM_NOT_AVAILABLE);
#endif
   } else if(strcmp(auth_method, "system") == 0 ) {
#ifndef WINDOWS
      ret = do_system_authentication(username, password, &uid, &gid, &error_handler);
#else
      ret = do_windows_system_authentication(username, password, 
                                             &ppszUID, &ppszGID, &nGIDs,
                                             &error_handler); 
#endif
   } else {
      ret = -1;
   }
   if (ret==JUTI_AUTH_SUCCESS) {
      int group_count = 0;
      gid_t *groups = NULL;
#ifndef WINDOWS      
      fprintf(stdout, "uid %d\n", uid);
      fprintf(stdout, "gid ");

      if(juti_getgrouplist(username, gid, &groups, &group_count) == 0) {            
         for(i = 0; i < group_count; i++) {
            if(i>0) {
               fprintf(stdout, ","gid_t_fmt, groups[i]);
            } else {
               fprintf(stdout, gid_t_fmt, groups[i]);
            }
         }
         free(groups);
      }
#else
      fprintf(stdout, "uid %s\n", *ppszUID);
      LocalFree(ppszUID[0]);
      free(ppszUID);

      fprintf(stdout, "gid ");
      
      for(i=0; i<nGIDs; i++) {
         if(i>0) {
            fprintf(stdout, ",%s", ppszGID[i]);
         } else {
            fprintf(stdout, "%s", ppszGID[i]);
         }
         LocalFree(ppszGID[i]);
      }
      free(ppszGID);
#endif
      fprintf(stdout, "\n");
   }
   return ret;
}

static int mygetline(char s[], int lim) { /* get a line into s, return length */

	/*
	User data is sent one field to a line, with the data
	set terminated when a line containing only a semi-colon is received.
	 */

	int c, i;

	for (i = 0; i < lim-1 && ((c = getchar()) != EOF) && c != '\n'; i++) {
	    s[i] = c;
	}

	s[i] = '\0';
	return (i);
}


#ifndef WINDOWS
static auth_result_t do_system_authentication(const char *username, 
                                              const char *password, 
                                              int *uid, 
                                              int *gid,
                                              error_handler_t *error_handler) 
{
   auth_result_t ret = JUTI_AUTH_SUCCESS;
   char crypted_password[BUF_SIZE];
   char *new_crypted_password = NULL;
   struct passwd *pwres = getpwnam(username);
      
   if(pwres == NULL) {
      ret = JUTI_AUTH_FAILED;
      error_handler->error(MSG_JUTI_USER_UNKNOWN_S, username);
      goto error;
   }
#ifdef DEBUG
   printf("user found in passwd\n");
#endif
   ret = get_crypted_password(username, crypted_password, BUF_SIZE, error_handler);
   if(ret != JUTI_AUTH_SUCCESS) {
      goto error;
   }
#ifdef DEBUG
   printf("    crypted password: %s\n", crypted_password);
#endif
#if !defined(INTERIX)
   new_crypted_password = crypt(password, crypted_password);
#endif
   if (new_crypted_password == NULL) {
      error_handler->error(MSG_JUTI_CRYPT_FAILED_S, strerror(errno));
      ret = JUTI_AUTH_ERROR;
   } else {
#ifdef DEBUG
      printf("new crypted password: %s\n", new_crypted_password);
#endif
      if(strcmp(crypted_password, new_crypted_password) == 0) {
         ret = JUTI_AUTH_SUCCESS;
      } else {
         error_handler->error(MSG_JUTI_INVALID_PASSWORD);
         ret = JUTI_AUTH_FAILED;
      }
   }
   
   *uid = pwres->pw_uid;
   *gid = pwres->pw_gid;
error:
   return ret;
}

#else
/* WINDOWS is defined */

static int GetSidStrings(HANDLE hToken,
                         TOKEN_INFORMATION_CLASS TokenInfo,
                         char ***pppszSid, 
                         int *nStrings)
{
   LPVOID pTokenInfo = NULL;
   DWORD  dwLength;
   DWORD  i;
   int    ret = 0;

   GetTokenInformation(hToken, TokenInfo,
                      (LPVOID)pTokenInfo, 0, &dwLength);
   if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
      ret = 1;
      goto error;
   }

   pTokenInfo = (PTOKEN_OWNER)HeapAlloc(GetProcessHeap(),
                         HEAP_ZERO_MEMORY, dwLength);
   if(pTokenInfo == NULL) {
      ret = 2;
      goto error;
   }
   if(!GetTokenInformation(hToken, TokenInfo,
                           (LPVOID)pTokenInfo, dwLength, &dwLength)) {
      ret = 3;
      goto error;
   }

   if(TokenInfo == TokenOwner) {
      TOKEN_OWNER *pTokenOwner = (TOKEN_OWNER*)pTokenInfo;
      
      *pppszSid = (char**)malloc(sizeof(char*));
      ConvertSidToStringSid(pTokenOwner->Owner, *pppszSid);
   } else if(TokenInfo == TokenGroups) {
      TOKEN_GROUPS *pTokenGroups = (TOKEN_GROUPS*)pTokenInfo;
   
      *pppszSid = (char**)malloc(sizeof(char*) * pTokenGroups->GroupCount);
      *nStrings = pTokenGroups->GroupCount;
      for(i=0; i<pTokenGroups->GroupCount; i++) {
         ConvertSidToStringSid(pTokenGroups->Groups[i].Sid, &((*pppszSid)[i]));
      }
   }
error:
   HeapFree(GetProcessHeap(), 0, pTokenInfo);
   return ret;
}

static auth_result_t do_windows_system_authentication(const char *username, 
                                              const char *password, 
                                              char ***pppszUID, 
                                              char ***pppszGID,
                                              int  *nGIDs,
                                              error_handler_t *error_handler) 
{
   auth_result_t ret    = JUTI_AUTH_SUCCESS;
   HANDLE        hToken = INVALID_HANDLE_VALUE;
   int           nUIDs  = 0;

   if(!LogonUser(
      username,
      NULL, //".",
      password,
      LOGON32_LOGON_INTERACTIVE,
      LOGON32_PROVIDER_DEFAULT,
      &hToken)) {
         ret = JUTI_AUTH_ERROR;
         error_handler->error(MSG_AUTHUSER_WRONG_USER_OR_PASSWORD);
         goto error;
   }
   GetSidStrings(hToken, TokenOwner,  pppszUID, &nUIDs);
   GetSidStrings(hToken, TokenGroups, pppszGID, nGIDs);

   CloseHandle(hToken);

error:
   return ret;
}
#endif

