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
#include <stdlib.h>                     /* for atoi                     */
#include <stdio.h>

#include "sec_genrsa.h"
#include "sec_certtext.h"
#include "sec_makereq.h"
#include "sec_setup_path.h"             /* for sec_setup_path           */
#include "sec_local.h"                  /* for paths                    */


int main(int argc, char **argv);


static char *usage[]={
"This program makes a key and certificate request, which is to sign from the\n",
"certificate authority to get a valid certificate\n\n",
"usage:\n\n",
" -keylen arg	- default key length is 512 Bits and must not be lower\n",
" -cell arg     - specifies other cell than default cell\n",
" -sgesys	- the request is for components of sge (i.e. sge_qmaster)\n\n",
NULL
};

/*
** DESCRIPTION
**	This program makes a certificate request for a user. It makes a
**	RSA-key and a selfsigned certificate.
*/

int main(
int argc,
char **argv 
) {
	int	i,keylen=DEFBITS,issgesys=0;
	char	**ptr,*cell=NULL;
	RSA	*rsa;
	X509	*x509;
	FILES	*files;

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
		else if (strcmp(*argv,"-sgesys") == 0)
			issgesys = 1;	
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

        files = sec_setup_path(cell,issgesys);
        if(files == NULL){
                fprintf(stderr,"error: failed sec_setup_path\n");
                exit(1);
        }

	rsa = sec_genrsa(files->key_file,keylen);
	if(rsa == NULL){
		fprintf(stderr,"error: error generating key\n");
		exit(1);
	}

	i = sec_certtext(files->text_file,files->textdefault_file);
	if(i<0){
		fprintf(stderr,"error: error making certificate text\n");
		exit(1);
	}
	
	x509 = sec_makereq(rsa,files->text_file,files->req_file);
	if(x509 == NULL){
		fprintf(stderr,"error: error making certificate request\n");
		exit(1);
	}
	exit(0);
}
