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
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "sge.h"
#include "sge_arch.h"
#include "sec_local.h"
#include "sec_setup_path.h"

static char *sec_make_certdir(char *rootdir);
static char *sec_make_keydir(char *rootdir);
static char *sec_make_cadir(char *cell);
static char *sec_make_userdir(char *cell);
static FILES *sec_set_path(char *ca_keydir, char *ca_certdir, char *user_keydir, char *user_certdir);

/*
** DESCRIPTION
**	These functions generate the pathnames for key and certificate files.
*/
FILES *sec_setup_path(
char *cell,
int issgesys 
) {
	FILES	*files;
	char	*userdir,*cadir,*user_certdir,*user_keydir,*ca_certdir,*ca_keydir;

	cadir = sec_make_cadir(cell);
	if (cadir == NULL){
		fprintf(stderr,"error: failed sec_make_cadir\n");
		return(NULL);
	}

	ca_certdir = sec_make_certdir(cadir);
   if (ca_certdir == NULL){
      fprintf(stderr,"error: failed sec_make_certdir\n");
      return(NULL);
   }

	ca_keydir = sec_make_keydir(cadir);
   if (ca_keydir == NULL){
      fprintf(stderr,"error: failed sec_make_keydir\n");
      return(NULL);
   }

	if (issgesys){
		userdir = cadir;
		user_certdir = ca_certdir;
		user_keydir = ca_keydir;
	}
	else{
		userdir = sec_make_userdir(cell);
		if (userdir == NULL){
			fprintf(stderr,"error: failed sec_make_userr\n");
			return(NULL);
		}

		user_certdir = sec_make_certdir(userdir);
		if (user_certdir == NULL){
			fprintf(stderr,"error: failed sec_make_certdir\n");
			return(NULL);
		}

		user_keydir = sec_make_keydir(userdir);
		if (user_keydir == NULL){
			fprintf(stderr,"error: failed sec_make_keydir\n");
			return(NULL);
		}
	}

	files = sec_set_path(ca_keydir,ca_certdir,user_keydir,user_certdir);
   if (files == NULL){
      fprintf(stderr,"error: failed sec_set_path\n");
      return(NULL);
   }

	return(files);
}

FILES *sec_set_path(
char *ca_keydir,
char *ca_certdir,
char *user_keydir,
char *user_certdir 
) {
	FILES	*files;

   files = (FILES *) malloc(sizeof(FILES));
   if (files == NULL) 
      goto error;

	files->ca_key_file = (char *) malloc(strlen(ca_keydir) + strlen(CaKey) + 10);
	if (files->ca_key_file == NULL) 
      goto error;
	sprintf(files->ca_key_file,"%s/%s",ca_keydir,CaKey);

	files->ca_text_file = (char *) malloc(strlen(ca_certdir) + strlen(CaText) + 10);
	if (files->ca_text_file == NULL) 
      goto error;
	sprintf(files->ca_text_file,"%s/%s",ca_certdir,CaText);

	files->ca_textdefault_file = (char *) malloc(strlen(ca_certdir) + 
					strlen(CaTextDefault) + 10);
	if (files->ca_textdefault_file == NULL) 
      goto error;
	sprintf(files->ca_textdefault_file,"%s/%s",ca_certdir,CaTextDefault);

	files->ca_cert_file = (char *) malloc(strlen(ca_certdir) + strlen(CaCert) + 10);
	if (files->ca_cert_file == NULL) 
      goto error;
	sprintf(files->ca_cert_file,"%s/%s",ca_certdir,CaCert);

   files->key_file = (char *) malloc(strlen(user_keydir) + strlen(RsaKey) + 10);
   if (files->key_file == NULL) 
      goto error;
   sprintf(files->key_file,"%s/%s",user_keydir,RsaKey);

   files->text_file = (char *) malloc(strlen(user_certdir) + strlen(CertText) + 10);
   if (files->text_file == NULL) 
      goto error;
   sprintf(files->text_file,"%s/%s",user_certdir,CertText);

   files->textdefault_file = (char *) malloc(strlen(user_certdir) + 
                                        strlen(CertTextDefault) + 10);
   if (files->textdefault_file == NULL) 
      goto error;
   sprintf(files->textdefault_file,"%s/%s",user_certdir,CertTextDefault);

   files->req_file = (char *) malloc(strlen(user_certdir) + strlen(CertReq) + 10);
   if (files->req_file == NULL) 
      goto error;
   sprintf(files->req_file,"%s/%s",user_certdir,CertReq);

   files->cert_file = (char *) malloc(strlen(user_certdir) + strlen(Cert) + 10);
   if (files->cert_file == NULL) 
      goto error;
   sprintf(files->cert_file,"%s/%s",user_certdir,Cert);

	files->reconnect_file = (char *) malloc(strlen(user_keydir) + 
					strlen(ReconnectFile) + 10);
	if (files->reconnect_file == NULL) 
      goto error;
   sprintf(files->reconnect_file,"%s/%s",user_keydir,ReconnectFile);

	return(files);

error:
	fprintf(stderr,"error: failed malloc memory\n");
	return(NULL);	
}

static char *sec_make_certdir(
char *rootdir 
) {
   char *certdir,*cert_dir_root;
   struct stat     sbuf;

   certdir = (char *) malloc(strlen(CertPath) + 10);
	strcpy(certdir,CertPath);

   /* get rid of surrounding slashs                                */
   if (certdir[0] == '/')
      strcpy(certdir,certdir+1);
   if (certdir[strlen(certdir)-1] == '/')
      certdir[strlen(certdir)-1] = '\0';

   cert_dir_root = (char *) malloc(strlen(certdir) + strlen(rootdir) + 10);
   if (cert_dir_root == NULL){
      fprintf(stderr,"error: failed malloc memory\n");
      return(NULL);
   }
   sprintf(cert_dir_root,"%s/%s",rootdir,certdir);

   /* is cert_dir_root already there?                              */
   if (stat(cert_dir_root,&sbuf)){
      fprintf(stderr, "cert directory %s doesn't exist - making\n",
                        cert_dir_root);
      if (mkdir(cert_dir_root,(mode_t) 0755)) {
         fprintf(stderr,"error: could not mkdir %s\n",cert_dir_root);
         return(NULL);
      }
   }

	free(certdir);
   return(cert_dir_root);
}


static char *sec_make_keydir(
char *rootdir 
) {
	char		*keydir,*key_dir_root;
	struct stat     sbuf;

	keydir = (char *) malloc(strlen(KeyPath) + 10);
	if (keydir== NULL){
      fprintf(stderr,"error: failed malloc memory\n");
      return(NULL);
   }

	strcpy(keydir,KeyPath);

        /* get rid of surrounding slashs                                */
        if(keydir[0] == '/')
                strcpy(keydir,keydir+1);
        if (keydir[strlen(keydir)-1] == '/')
                keydir[strlen(keydir)-1] = '\0';

        key_dir_root = (char *) malloc(strlen(keydir) + strlen(rootdir) + 10); 
        if(key_dir_root == NULL){
                fprintf(stderr,"error: failed malloc memory\n");
                return(NULL);
        }
	sprintf(key_dir_root,"%s/%s",rootdir,keydir);

        /* is key_dir_root already there?				*/
        if (stat(key_dir_root,&sbuf)){
                fprintf(stderr, "key directory %s doesn't exist - making\n",
                        key_dir_root);
                if(mkdir(key_dir_root,(mode_t) 0700)){
                        fprintf(stderr,"error: could not mkdir %s\n",key_dir_root);
                        return(NULL);
                }
        }
	free(keydir);
	return(key_dir_root);
}
	
	
static char *sec_make_cadir(
char *cell 
) {
	char		*ca_root,*ca_cell,*cell_root;
	struct stat 	sbuf;

	ca_root = sge_sge_root();

	/* is ca_root already there?					*/
	if (stat(ca_root,&sbuf)) { 
		fprintf(stderr,"error: could not stat %s\n",ca_root);
		return(NULL);	
	}

	if(cell) 
		ca_cell = cell;
	else{
		ca_cell = getenv("SGE_CELL");
        	if (!ca_cell || strlen(ca_cell) == 0)
                ca_cell = DEFAULT_CELL;
	}
	
	/* get rid of surrounding slashs				*/
	if(ca_cell[0] == '/')
		strcpy(ca_cell,ca_cell+1);
        if (ca_cell[strlen(ca_cell)-1] == '/')
                ca_cell[strlen(ca_cell)-1] = '\0';

	cell_root = (char *) malloc(strlen(ca_cell) + strlen(ca_root) + 10);
	if(cell_root == NULL){
		fprintf(stderr,"error: failed malloc memory\n");
		return(NULL);
	}
	sprintf(cell_root,"%s/%s",ca_root,ca_cell);

	/* is cell_root already there?                                	*/
        if (stat(cell_root,&sbuf)){
                fprintf(stderr, "cell_root directory %s doesn't exist - making\n",
                        cell_root);
                if(mkdir(cell_root,(mode_t) 0755)){
                        fprintf(stderr,"error: could not mkdir %s\n",cell_root);
                        return(NULL);
                }
        }

	return(cell_root);
}

static char *sec_make_userdir(
char *cell 
) {
        char            *user_root,*user_cell,*cell_root,*home;
        struct stat     sbuf;

        home = getenv("HOME");
        if(!home){
        	fprintf(stderr,"error: failed get HOME variable\n");
        	return(NULL);
        }

        /* get rid of trailing slash                            */
        if (home[strlen(home)-1] == '/')
        	home[strlen(home)-1] = '\0';
	user_root = (char *) malloc(strlen(home) + strlen(SGESecPath) + 10);
        if(user_root == NULL){
        	fprintf(stderr,"error: failed malloc memory\n");
        	return(NULL);
        }

        sprintf(user_root, "%s/%s",home,SGESecPath);

        /* get rid of trailing slash                                    */
        if (user_root[strlen(user_root)-1] == '/')
                user_root[strlen(user_root)-1] = '\0';

        /* is user_root already there?                                */
        if (stat(user_root,&sbuf)){
                fprintf(stderr, "root directory %s doesn't exist - making\n",
                                user_root);
                if(mkdir(user_root,(mode_t) 0755)){
                        fprintf(stderr,"error: could not mkdir %s\n",user_root);
                        return(NULL);
                }
        }

        if(cell) 
		user_cell = cell;
        else{
                user_cell = getenv("SGE_CELL");
                if (!user_cell || strlen(user_cell) == 0)
                user_cell = DEFAULT_CELL;
        }

        /* get rid of surrounding slashs                                */
        if(user_cell[0] == '/')
                strcpy(user_cell,user_cell+1);
        if (user_cell[strlen(user_cell)-1] == '/')
                user_cell[strlen(user_cell)-1] = '\0';

        cell_root = (char *) malloc(strlen(user_cell) + strlen(user_root) + 10);
        if(cell_root == NULL){
                fprintf(stderr,"error: failed malloc memory\n");
                return(NULL);
        }
        sprintf(cell_root,"%s/%s",user_root,user_cell);

        /* is cell_root already there?                                  */
        if (stat(cell_root,&sbuf)){
                fprintf(stderr, "cell_root directory %s doesn't exist - making\n",
                        cell_root);
                if(mkdir(cell_root,(mode_t) 0755)){
                        fprintf(stderr,"error: could not mkdir %s\n",cell_root);
                        return(NULL);
                }
        }

        return(cell_root);
}

