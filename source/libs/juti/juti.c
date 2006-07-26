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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <termios.h>

#include <pwd.h>

#if defined(IRIX65) || defined(AIX43) || defined(HP1164) \
    || defined(DARWIN_PPC) || defined(INTERIX)
#define JUTI_NO_PAM
#else
#include <security/pam_appl.h>
#endif

#if defined(DARWIN_PPC) || defined(AIX51) || defined(AIX43) || defined(INTERIX)
#define JUTI_NO_SHADOW
#else
#include <shadow.h>
#endif

#if defined(AIX51) || defined(AIX43)
#include <userpw.h>
#endif

#include <unistd.h>
#include <dlfcn.h>
#include <sys/param.h>
#include <grp.h>

#include "juti.h"
#include "msg_juti.h"

#if 0
#define DEBUG
#endif

#define BUFSIZE 1024


#ifndef JUTI_NO_PAM
/*
 * For application data, like password. Passes through the
 * pam framework to the conversation function.
 */
struct app_pam_data {
	const char *password;
};

/* pam conversation function */
#if defined(SOLARIS) || defined(AIX) || defined(HP11) || defined(HP1164)
static int login_conv(int num_msg, struct pam_message **msgm,
                      struct pam_response **response, void *appdata_ptr);
#else
static int login_conv(int num_msg, const struct pam_message **msgm,
                      struct pam_response **response, void *appdata_ptr);
#endif

#endif 
static auth_result_t get_crypted_password(const char* username, char* buffer, size_t size, 
                                error_handler_t *error_handler);


                      
auth_result_t do_pam_authenticate(const char* service, const char *username, 
                                  const char *password,
                                  error_handler_t *error_handler)
{
#ifdef JUTI_NO_PAM
   error_handler->error(MSG_JUTI_PAM_NOT_AVAILABLE);
   return JUTI_AUTH_ERROR;
#else
   auth_result_t ret;
	int status;
	pam_handle_t *pamh;		/* pam handle */
	struct pam_conv pamconv;	/* pam init structure */
	struct app_pam_data app_data;	/* pam application data */

	pamh = NULL;
	/*
	 * app_data gets passed through the framework
	 * to "login_conv". Set the password for use in login_conv
	 * and set up the pam_conv data with the conversation
	 * function and the application data pointer.
	 */
	app_data.password = password;
	pamconv.conv = login_conv;
	pamconv.appdata_ptr = &app_data;

	/* pam start session */
	status = pam_start(service, username, &pamconv, &pamh);
   if(status != PAM_SUCCESS) {
      ret = JUTI_AUTH_ERROR;
      goto error;
   }
   status = pam_authenticate(pamh, PAM_SILENT);
   if(status != PAM_SUCCESS) {
      ret = JUTI_AUTH_FAILED;
      goto error;
   }

	/* check if the authenicated user is allowed to use machine */
   status = pam_acct_mgmt(pamh, PAM_SILENT);
	if (status != PAM_SUCCESS) {
      ret = JUTI_AUTH_FAILED;
      goto error;
	}
   ret = JUTI_AUTH_SUCCESS; 

error:
	if (status != PAM_SUCCESS) {
		const char *pam_err_msg = pam_strerror(pamh, status);
		error_handler->error(MSG_JUTI_PAM_ERROR_S, pam_err_msg);
	}

	/* end pam session */
	if (pamh != NULL) {
	    pam_end(pamh, status == PAM_SUCCESS ? PAM_SUCCESS : PAM_ABORT);
	}
	return ret;
#endif
}

/*
 * login_conv:
 * this is the conversation function called from a PAM authentication module
 * to print erro messagae or get user information
 */
#ifndef JUTI_NO_PAM
#if defined(SOLARIS) || defined(AIX) || defined(HP11) || defined(HP1164)
static int login_conv(int num_msg, struct pam_message **msgm,
                      struct pam_response **response, void *appdata_ptr)
#else
static int login_conv(int num_msg, const struct pam_message **msgm,
                      struct pam_response **response, void *appdata_ptr)
#endif
{
	int count = 0;
	int reply_used = 0;
	struct pam_response *reply;

	if (num_msg <= 0) {
#ifdef DEBUG
		printf("no message returned from pam module\n");
#endif
		return (PAM_CONV_ERR);
	}

	reply = (struct pam_response *)calloc(num_msg,
		sizeof (struct pam_response));
	if (reply == NULL) {
#ifdef DEBUG
		printf("calloc memory error\n");
#endif
		return (PAM_BUF_ERR);
	}

	for (count = 0; count < num_msg; ++count) {
#ifdef DEBUG
            printf("\n login_conv: prompt_style:  %d" , msgm[count]->msg_style);
            printf("\n login_conv: prompt_message:  %s" , msgm[count]->msg);
            printf("\n PAM_PROMPT_ECHO_OFF  %d" , PAM_PROMPT_ECHO_OFF);
            printf("\n PAM_PROMPT_ECHO_ON  %d" , PAM_PROMPT_ECHO_ON);
            printf("\n PAM_ERROR_MSG  %d" , PAM_ERROR_MSG);
            printf("\n PAM_TEXT_INFO  %d" , PAM_TEXT_INFO);
#endif

	    switch (msgm[count]->msg_style) {
		case PAM_PROMPT_ECHO_OFF:
		if (((struct app_pam_data *)appdata_ptr)->password != NULL) {
			(reply+count)->resp = strdup(((struct app_pam_data *)
				appdata_ptr)->password);
                    if (reply[count].resp == NULL) {
                        /*
                         * It may be the case that some modules won't free
                         * the pam_response memory if the return is not
                         * PAM_SUCCESS. We should not have had
                         * multiple PAM_PROMPT_ECHO_OFF in a single
                         * login_conv call but just in case see if
                         * reply was modified anyway.
                         */
                        if (reply_used) {
                            int i;
                            for (i = 0; i < num_msg; ++i) {
                                if (reply[i].resp != NULL) {
                                    free(reply[i].resp);
                                }
                            }
                        }
                        free(reply);
                        *response = NULL;
                        return (PAM_BUF_ERR);
                    }
		}
                /*
                 * Always set reply_used even if the password is NULL
                 * A NULL response cannot be returned with PAM_SUCCESS.
                 * Could return PAN_CONV_ERR but let later code determine
                 * the effect of a NULL password.
                 */
                reply_used = 1;
		break;

            /* For empty user name, shouldn't happen */
		case PAM_PROMPT_ECHO_ON:
                /*
                 * It may be the case that some modules won't free
                 * the pam_response memory if the return is not
                 * PAM_SUCCESS. We should not have had
                 * multiple PAM_PROMPT_ECHO_OFF in a single
                 * login_conv call but just in case see if
                 * reply was modified anyway.
                 */
                if (reply_used) {
                    int i;
                    for (i = 0; i < num_msg; ++i) {
                        if (reply[i].resp != NULL) {
                            free(reply[i].resp);
                        }
                    }
                }
                free(reply);
                *response = NULL;
                /*
                 * Have to return error here since PAM can loop
                 * when no username is provided
                 */
		    break;
		case PAM_ERROR_MSG:
		case PAM_TEXT_INFO:
			break;
		default:
			break;
	    }
	}

        /*
         * The response may not always be freed by
         * modules for PAM_ERROR_MSG, and PAM_TEXT_INFO
         * So make sure it is freed if we didn't use it.
         */
        if (!reply_used) {
            free(reply);
            reply = NULL;
        }
	*response = reply;

	return (PAM_SUCCESS);
}
#endif

static auth_result_t get_crypted_password(const char* username, char* buffer, size_t size,

                                error_handler_t *error_handler) {
                     
#if defined(AIX43) || defined(AIX51)
   char buf[BUFSIZE];
   struct userpw *pw = NULL;
   
   strncpy(buf, username, BUFSIZE);
   pw = getuserpw(buf);
   if(pw == NULL) {
      error_handler->error(MSG_JUTI_USER_UNKNOWN_S, username);
      return JUTI_AUTH_FAILED;
   } else {
      strncpy(pw->upw_passwd, buffer, size);
      return JUTI_AUTH_SUCCESS;
   }
#elif defined(JUTI_NO_SHADOW)
   struct passwd *pw = getpwnam(username);
   if(pw == NULL) {
      error_handler->error(MSG_JUTI_USER_UNKNOWN_S, username);
      return JUTI_AUTH_FAILED;
   } else {
      strncpy(pw->pw_passwd, buffer, size);
      return JUTI_AUTH_SUCCESS;
   }
#else
   struct spwd *pres = getspnam(username);
   if(pres == NULL) {
      error_handler->error(MSG_JUTI_NO_SHADOW_ENTRY_S, username);
      return JUTI_AUTH_FAILED;
   }
   strncpy(buffer, pres->sp_pwdp, size);
   return JUTI_AUTH_SUCCESS;
#endif

}



auth_result_t do_shadow_authentication(const char* username, const char* password, 
                             int* uid, int *gid,
                             error_handler_t *error_handler) {
   
   auth_result_t ret = JUTI_AUTH_SUCCESS;
   char crypted_password[BUFSIZE];
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
   ret = get_crypted_password(username, crypted_password, BUFSIZE, error_handler);
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


static struct termios init_set;
static int initialized = 0;

void setEcho(int flag) {
   
   struct termios new_set;
   
   if(initialized == 0) {
      tcgetattr(fileno(stdin), &init_set);
      initialized = 1;
   }

   new_set = init_set;
   if(flag) {   
      tcsetattr(fileno(stdin), TCSAFLUSH, &init_set);
   } else {
      new_set.c_lflag &= ~ECHO;
      tcsetattr(fileno(stdin), TCSAFLUSH, &new_set);
   }
   
}



int juti_getgrouplist(const char *uname, gid_t agroup, gid_t **groups_res, int *grpcnt)
{
	struct group *grp;
	int bail;
   
   int maxgroups = 10;
   gid_t *groups = (gid_t*)malloc(sizeof(groups)*maxgroups);
   int ngroups = 0;
   int i = 0;
   
   if(groups == NULL) {
      return -1;
   }
   
   groups[ngroups++]=agroup;

	setgrent();
	while ((grp = getgrent())) {
		if (grp->gr_gid == agroup) {
			continue;
      }
      for (bail = 0, i = 0; bail == 0 && i < ngroups; i++) {
			if (groups[i] == grp->gr_gid) {
				bail = 1;
         }
      }
		if (bail) {
			continue;
      }
		for (i = 0; grp->gr_mem[i]; i++) {
			if (!strcmp(grp->gr_mem[i], uname)) {
            if (ngroups >= maxgroups) {
               gid_t *tmp = groups;
               maxgroups +=10;
               groups = (gid_t*)malloc(sizeof(groups)*maxgroups);
               if(groups == NULL) {
                  free(tmp);
                  return -1;
               }
               memcpy(groups, tmp, sizeof(gid_t) * ngroups);
               free(tmp);
            }
            groups[ngroups++] = grp->gr_gid;
			}
		}
	}

	endgrent();
   *groups_res = groups;   
	*grpcnt = ngroups;
	return 0;
}


