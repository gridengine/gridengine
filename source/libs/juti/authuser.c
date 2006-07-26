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
#include <pwd.h>
#include <errno.h>
#include "juti.h"
#include "msg_juti.h"

#if 0
#define DEBUG 1
#endif

static void usage(void);
static void print_error(const char* format, ...);

static void usage(void) {
   fprintf(stderr, "userverifier <method> <params>\n");
   fprintf(stderr, "\n");
   fprintf(stderr, "   <method>    \"pam\" or \"shadow\"\n");
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

static int mygetline(char s[], int lim);

#define BUF_SIZE 1024

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
   } else if (strcmp(auth_method, "shadow" ) == 0 ) {
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
   } else if(strcmp(auth_method, "shadow") == 0 ) {
       ret = do_shadow_authentication(username, password, &uid, &gid, &error_handler);
   } else {
      ret = -1;
   }
   if (ret==JUTI_AUTH_SUCCESS) {
      int group_count = 0;
      gid_t *groups = NULL;
      
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




