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
 *  License at http://www.gridengine.sunsource.net/license.html
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
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "sec_genrsa.h"
#include "sec_certtext.h"
#include "sec_makereq.h"
#include "sec_certify.h"
#include "sec_setup_path.h"		/* for sec_setup_path		*/
#include "sec_local.h"                  /* for FILES                    */

#define DAYS 356


int main(int argc, char **argv);


static char *usage[]={
"This program creates a key and certificate of a certificate authority (CA).\n",
"The CA can then sign other certificates.\n\n",
"usage:\n\n",
" -keylen arg   - default key length is 512 Bits and must not be lower\n",
" -days arg     - how many days till the certificate will expire (default: 356)\n",
" -cell arg	- specifies other cell than default cell\n\n",
NULL
};

/*
** DESCRIPTION
**	This program generates a RSA-key and a certificate for a 
**	certification authority.
*/
int main(
int argc,
char **argv 
) {
        int     i,keylen=DEFBITS,days=DAYS;
	char	*cell=NULL,**ptr,input[256];
        RSA     *rsa;
        X509    *x509;
	FILE	*fp;
	FILES	*files;
	struct stat st;

        argc--;
        argv++;
        while (argc >=1){
                if      (strcmp(*argv,"-cell") == 0){
                                if (--argc < 1) goto bad;
                                cell = *(++argv);
                        }
                else if (strcmp(*argv,"-keylen") == 0){
                                if (--argc < 1) goto bad; 
                                keylen = atoi(*(++argv));
                                if (keylen == 0 || keylen < DEFBITS) goto bad;
                        }
                else if (strcmp(*argv,"-days") == 0){
                                if (--argc < 1) goto bad;
                                days = atoi(*(++argv));
                                if (days == 0) goto bad;
                        }
                else    {
                                fprintf(stderr,"unknown option %s\n",*argv);
                                goto bad;
                        }
                argc--;
                argv++;
        }

        if(0){
bad:
                for(ptr=usage;*ptr !=NULL;ptr++)
                        fprintf(stderr,*ptr);
                        exit(1);
        }

        ERR_clear_error();
        ERR_load_crypto_strings();

	files = sec_setup_path(cell,1);
	if(files == NULL){
		fprintf(stderr,"error: failed sec_setup_path\n");
		exit(1);
	}

        if (!stat(files->ca_cert_file,&st)){
                fprintf(stderr,"CA certificate file '%s' already exists!\n - overwrite? [y/n] ",files->ca_cert_file);
                fflush(stderr);
                fgets(input,256,stdin);
                if(input[0] != 'y')
                        exit(1);
        }

        rsa = sec_genrsa(files->ca_key_file,keylen);
        if(rsa == NULL){
                fprintf(stderr,"error: error generating key\n");
                exit(1);
        }

        i = sec_certtext(files->ca_text_file,files->ca_textdefault_file);
        if(i<0){
                fprintf(stderr,"error: error making certificate text\n");
                exit(1);
        }

        x509 = sec_makereq(rsa,files->ca_text_file,NULL);
        if(x509 == NULL){
                fprintf(stderr,"error: error making certificate request\n");
                exit(1);
        }

	i = sec_certify(files->ca_cert_file,x509,x509,rsa,1,days);
	if(i){
		fprintf(stderr,"error: certify certificate\n");
		exit(1);
	}

        fp = fopen(files->ca_cert_file,"w");
        if(fp == NULL){
                fprintf(stderr,"Can't open certificate file '%s'\n",files->ca_cert_file);
                exit(1);
        }

        i = PEM_write_X509(fp,x509);
        if(!i){
                fprintf(stderr,"Can't write certificate file\n");
                ERR_print_errors(stderr);
                if(fp) fclose(fp);
                exit(1);
        }

        if(fp) fclose(fp);

        exit(0);
}

