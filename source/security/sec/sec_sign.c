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
#include <stdio.h>
#include <string.h>			/* for strcmp			*/
#include <stdlib.h>			/* for atoi			*/
#include <sys/stat.h>
#include <sys/types.h>

#include "sec_crypto.h"
#include "sec_setup_path.h"     	/* for sec_setup_path           */
#include "sec_certify.h"		/* for sec_certify		*/
#include "sec_local.h"                  /* for paths                    */

#define DAYS 180

int main(int argc, char **argv);

static char *usage[]={
"This program certifies an input certificate using the key of the certificate\n",
"authority (CA)\n\n",
"usage:\n\n",
" -days arg	- how many days till the certificate will expire (default: 180)\n",
" -cell arg	- specifies other cell than default cell\n",
" -in arg	- input certificate\n\n",
NULL
};

/*
** DESCRIPTION
**      This program signes a certificate request with the key of the CA.
*/

int main(
int argc,
char **argv 
) {
        int     i,days=DAYS;
	char	**ptr,*infile=NULL,*outfile,*cell=NULL,input[256];
        RSA     *carsa;
        X509    *x509,*cax509;
        FILE    *fp=NULL;
	FILES	*files;
        struct stat st;

	argc--;
	argv++;
	while (argc >=1){
		if	(strcmp(*argv,"-in") == 0){
				if (--argc < 1) goto bad;
				infile= *(++argv);
			}
                else if (strcmp(*argv,"-cell") == 0){
                                if (--argc < 1) goto bad;
                                cell = *(++argv);
                        }
		else if	(strcmp(*argv,"-days") == 0){
				if (--argc < 1) goto bad;
				days = atoi(*(++argv));
				if (days == 0) goto bad;
			}
		else	{
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

	if(infile == NULL){
		fprintf(stderr,"error: you must specify input file!\n");
		exit(1);
	}
	
        ERR_clear_error();
        ERR_load_crypto_strings();

        files = sec_setup_path(cell,0);
        if(files == NULL){
                fprintf(stderr,"error: failed sec_setup_path\n");
                exit(1);
        }

        if (!stat(files->cert_file,&st)){
                fprintf(stderr,"Certificate file '%s' already exists!\n - overwrite? [y/n] ",files->cert_file);
                fflush(stderr);
                fgets(input,256,stdin);
                if(input[0] != 'y')
                        exit(1);
        }

	/* read certificate to sign					*/
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
                fprintf(stderr,"Can't read certificate file '%s'\n",infile);
                ERR_print_errors(stderr);
                if(fp) fclose(fp);
                exit(1);
        }

	if(fp) fclose(fp);

        /* make outfile                                                 */
        i = strlen(infile);
        outfile = (char *) malloc(i + strlen(Cert) + 10);
        if(outfile == NULL){
                fprintf(stderr,"error: failed malloc memory!\n");
                exit(1);
        }
        while(infile[i] != '/' && i > 0)
                i--;
        if(infile[i] == '/')
                infile[i+1] = '\0';
        else
                infile[i] = '\0';
        sprintf(outfile,"%s%s",infile,Cert);

	/* read certificate of CA					*/
        fp = fopen(files->ca_cert_file,"r");
        if(fp == NULL){
                fprintf(stderr,"Can't open certificate file '%s'\n",files->ca_cert_file);
                exit(1);
        }

        cax509 = X509_new();
        if(cax509 == NULL){
                ERR_print_errors(stderr);
                if(fp) fclose(fp);
                exit(1);
        }

        i = PEM_read_X509(fp,cax509);
        if(!i){
                fprintf(stderr,"Can't read certificate file\n");
                ERR_print_errors(stderr);
                if(fp) fclose(fp);
                exit(1);
        }

	if(fp) fclose(fp);

        /* read key of CA						*/
        fp = fopen(files->ca_key_file,"r");
        if(fp == NULL){
                fprintf(stderr,"Can't open key file '%s'\n",files->ca_key_file);
                exit(1);
        }

        carsa = RSA_new();
        if(carsa == NULL){
                ERR_print_errors(stderr);
                if(fp) fclose(fp);
                exit(1);
        }

        i = PEM_read_RSA(fp,carsa);
        if(!i){
                fprintf(stderr,"Can't read key file\n");
                ERR_print_errors(stderr);
                if(fp) fclose(fp);
                exit(1);
        }

	if(fp) fclose(fp);

	/* certify input certificate					*/
        i = sec_certify(files->ca_cert_file,x509,cax509,carsa,0,days);
        if(i){
                fprintf(stderr,"error: certify certificate\n");
                exit(1);
        }

	/* write certified certificate to disk				*/
        fp = fopen(outfile,"w");
        if(fp == NULL){
                fprintf(stderr,"Can't open certificate file '%s'\n",outfile);
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

