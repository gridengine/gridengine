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
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "sec_genrsa.h"

#define BUFSIZE 1024

/*
** NAME
**      cb
**
** SYNOPSIS
**      #include "sec_genrsa.h"
**
**      void cb(p, ni, void*)
**
** DESCRIPTION
**      This function makes an output while generating the key.
**
*/
static void cb(
int p,
int n,
void *arg 
) {
	int c='*';

	if (p == 0) c='.';
	if (p == 1) c='+';
	if (p == 2) c='\n';
	fputc(c,stderr);
	fflush(stderr);
#ifdef LINT
	p=n;
#endif
}

/*
** NAME
**      load_rand
**
** SYNOPSIS
**      #include "sec_genrsa.h"
**
**      long load_rand(name)
**	char *name - the name of the file from where to load the random bytes
**
** DESCRIPTION
**	This function loads from a file "random" bytes.
**      This function initialices the security related data in the global
**      gsd-struct. This must be done only once, typically before the
**      enroll to the commd.
**
** RETURN VALUES
**	long	number of loaded bytes
*/
static long load_rand(
char *name 
) {
	struct stat sb;
	char *p,*n;
	int last,i;
	long tot=0;
	FILE *in;
	char buf[1513];

	for (;;){
		last=0;
		for (p=name; ((*p != '\0') && (*p != ':')); p++);
		if (*p == '\0') last=1;
		*p='\0';
		n=name;
		name=p+1;
		if (*n == '\0') break;

		/* we now have either a file or directory name */
		i=stat(n,&sb);
		RAND_seed((unsigned char *)&sb, (int) sizeof(sb));
		tot+=sizeof(sb);
		if (i < 0) continue;

		in=fopen(n,"r");
		if (in == NULL){
			fprintf(stderr,"warning, can not open random files\n");
			return(tot);
		}

		for (;;){
			i=fread(buf,1,1513,in);
			if (i <= 0) break;
			RAND_seed((unsigned char *)buf, i);
			tot+=i;
		}
		fclose(in);
		if (last) break;
	}
	return(tot);
}

/*
** NAME
**      sec_genrsa
**
** SYNOPSIS
**      #include "sec_genrsa.h"
**
**      RSA *sec_genrsa(keyfile,num)
**	char *keyfile - name of the keyfile where to put the generated key
**	int num       - length of the key
**
** DESCRIPTION
**	This function generates a RSA-key.
**
** RETURN VALUES
**      *RSA    this structure contains the generated RSA-key
**      NULL	on failure
*/

RSA *sec_genrsa(
char *keyfile,
int num 
) {
	int 	enc=PEM_DEK_DES_CBC;
	int 	i;
	u_long 	f4=0x10001;
	char 	buf[BUFSIZE],input[256];
	FILE 	*out;
	RSA	*rsa;
	struct stat st;
	
        if (!stat(keyfile,&st)){
                fprintf(stderr,"Key file '%s' already exists!\n - overwrite? [y/n] ",
                                keyfile);
                fflush(stderr);
                fgets(input,256,stdin);
                if(input[0] != 'y')
                        goto err;
        }
	
	out=fopen(keyfile,"w");
	if (out == NULL){
		fprintf(stderr,"error: can`t open keyfile %s\n",keyfile);
		return(NULL);
	}

	i = des_read_pw_string(buf,BUFSIZE,"Enter random files, seperated with ':' :",0);
	if(i){
		memset(buf,0,BUFSIZE);
		fprintf(stderr,"error: reading random file) names\n");
		goto err;
	}

	if (buf[0] == '\0')
		fprintf(stderr,"warning, not much random data\n");
	else{
		fprintf(stderr,"%ld semi-random bytes loaded\n",
			load_rand(buf));
			memset(buf,0,BUFSIZE);
	}

	fprintf(stderr,"Generating RSA private key, %d bit long modulus\n",
		num);
/* 	RSA_set_generate_prime_callback(cb); */
	rsa=RSA_generate_key(num, RSA_F4, cb, NULL);
		
	if (rsa == NULL) goto err;
	fprintf(stderr,"e is %ld (0x%lX)\n",(unsigned long)rsa->e->d[0],
		(unsigned long)rsa->e->d[0]);

	if (!PEM_write_RSA(out,rsa,enc,0,NULL)) 
		goto err;

	if (out) fclose(out);
	return(rsa);
err:
	if (out) fclose(out);
	ERR_print_errors_fp(stderr);
	return(NULL);
}

