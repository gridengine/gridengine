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
#include <stdlib.h>
#include <sys/stat.h>                           /* stat                 */

#include "sec_verify.h"
#include "sec_setup_path.h"	/* for sec_setup_path			*/
#include "sec_other.h"		/* for sec_set_verify			*/
#include "sec_local.h"		/* for FILES				*/

char *usage[]={
"This program verifies an input certificate. A certificate is o.k., if it is\n",
"signed from the certificate authority (CA)\n\n",
"usage:\n\n",
" -ca		- verifies CA's certificate\n",
" -cell arg     - specifies other cell than default cell\n",
" -in arg       - input certificate\n\n",
NULL
};

int ca=0;

/*
** DESCRIPTION
**      This program verifies a certificate, it checks if the certificate
**	is valid and if it is signed from the CA.
*/

int main(
int argc,
char **argv 
) {
        int     i,year,month,day,hour,min,sec;
	char	*infile=NULL,*cell=NULL,*str,**ptr;
	X509	*x509;
	FILE	*fp;
	FILES	*files;

        argc--;
        argv++;
        while (argc >=1){
                if      (strcmp(*argv,"-in") == 0){
                                if (--argc < 1) goto bad;
                                infile= *(++argv);
                        }
		else if (strcmp(*argv,"-ca") == 0){
				ca = 1;
			}
                else if (strcmp(*argv,"-cell") == 0){
                                if (--argc < 1) goto bad;
                                cell = *(++argv);
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
                for(ptr=usage;*ptr != NULL;ptr++)
                        fprintf(stderr,*ptr);
                        exit(1);
	}

        ERR_clear_error();
        ERR_load_crypto_strings();

        files = sec_setup_path(cell,0);
        if(files == NULL){
                fprintf(stderr,"error: failed sec_setup_path\n");
                exit(1);
        } 

	if(infile == NULL && !ca) 
		infile = files->cert_file;
	if(infile == NULL && ca)
		infile = files->ca_cert_file;

        /* read certificate to verfiy					*/
        fp = fopen(infile,"r");
        if(fp == NULL){
                fprintf(stderr,"Can't open certificate file '%s'\n",infile);
                exit(1);
        }

        x509 = X509_new();
        if(x509 == NULL){
                ERR_print_errors(stderr);
                if(fp) fclose(fp);
                exit(1);
        }

        i = PEM_read_X509(fp,x509);
        if(!i){
                fprintf(stderr,"Can't read certificate file\n");
                ERR_print_errors(stderr);
                if(fp) fclose(fp);
                exit(1);
        }

        if(fp) fclose(fp);

	str=X509_oneline_X509_NAME(X509_get_subject_name(x509));
	printf("subject=%s\n",str);
	free(str);
	str=X509_oneline_X509_NAME(X509_get_issuer_name(x509));
	printf("issuer=%s\n",str);
	free(str);

	sscanf(x509->cert_info->validity->notBefore,"%02d%02d%02d%02d%02d%02dZ",
		&year,&month,&day,&hour,&min,&sec);
	printf("not valid before %02d.%02d.%02d %02d:%02d:%02d\n",day,month,year,
		hour,min,sec);
        sscanf(x509->cert_info->validity->notAfter,"%02d%02d%02d%02d%02d%02dZ",
                &year,&month,&day,&hour,&min,&sec);
        printf("not valid after  %02d.%02d.%02d %02d:%02d:%02d\n",day,month,year,
                hour,min,sec);


        /* set location of CA                                           */
        i = sec_set_verify_loc(files->ca_cert_file,files->ca_key_file);
        if (i){
                fprintf(stderr,"sec_set_verify_locations failed!\n");
                if(i == -1)
                        ERR_print_errors(stderr);
                exit(1);
        }

	/* verify certificate						*/
        i=X509_verify(x509,sec_verify_callb);
        if (i) 
		fprintf(stdout,"%s is ok.\n",infile);
        else{
    		ERR_print_errors(stderr);
		fprintf(stderr,"%s is NOT ok.\n",infile);
	}

	exit(0);
	
}

int sec_set_verify_loc(
char *file_env,
char *dir_env 
) {
        int i,success1=0,success2=0;
        struct stat st;
        char *str;

        str=file_env;
        if ((str != NULL) && (stat(str,&st) == 0))
        {
                i=X509_add_cert_file(str,X509_FILETYPE_PEM);
                if (!i) return(-1);
                success1=1;
        }
        str=dir_env;
        if (str != NULL)
        {
                i=X509_add_cert_dir(str,X509_FILETYPE_PEM);
                if (!i) return(-1);
                success2=1;
        }
        if(success1 && success2) return(0);
        return(-2);
}

int sec_verify_callb(
int ok,
X509 *xs,
X509 *xi,
int depth,
int error 
) {
	if(!ok){
		/* CA should be selfsigned!				*/
		if (error == VERIFY_ERR_DEPTH_ZERO_SELF_SIGNED_CERT && ca){
			ERR_clear_error();
			return(1);
		}
         	fprintf(stderr,"verify error:num=%d:%s\n",error,
                        X509_verify_error_string(error));
	}
        return(ok);
}

