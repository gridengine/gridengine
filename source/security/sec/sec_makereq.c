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
#include <stdlib.h>
#include <string.h>

#include "sec_makereq.h"

/*
** NAME
**      sec_makereq
**
** SYNOPSIS
**      #include "sec_makereq.h"
**
**	X509 *sec_makereq(rsa,textfile,reqfile)
**	RSA *rsa       - key of the request
**	char *textfile - textfile which contains the text for the certificate
**	char *reqfile  - where to put the certificate request
**
** DESCRIPTION
**	This function generates a certificate request, which is a self
**	signed certificate, which the certification authority has to sign.
**
** RETURN VALUES
**      *X509   this structure contains the certificate request
**      NULL    on failure
*/

X509 *sec_makereq(
RSA *rsa,
char *textfile,
char *reqfile 
) {
	int	i;
	X509	*x509;
	FILE	*fp;
	
	x509 = sec_read_certtext(textfile);
	if (x509 == NULL) return(NULL);

	i = sec_sign(x509,rsa);
	if (i) return(NULL);

	if (reqfile == NULL) return(x509);

	fp = fopen(reqfile,"w");
	if(fp == NULL){
		fprintf(stderr,"Can't open certificate request '%s'\n",reqfile);
		return(NULL);
	}

	i = PEM_write_X509(fp,x509);
	if(!i){
		fprintf(stderr,"Can't write certificate request\n");
                ERR_print_errors(stderr);
		if(fp) fclose(fp);
                return(NULL);
        }

	if(fp) fclose(fp);
	return(x509);
	
}

/*
** NAME
**      sec_sec_read_certtex
**
** SYNOPSIS
**      #include "sec_makereq.h"
**
**	X509 *sec_read_certtext(textfile)
**      char *textfile - textfile which contains the text for the certificate
**
** DESCRIPTION
**      This function reads the test for this certificate from file.
**
** RETURN VALUES
**      *X509   this structure contains the certificate request
**      NULL    on failure
*/


X509 *sec_read_certtext(
char *textfile 
) {
	int	i;
	FILE	*fp;
	X509    *x509;

	fp = fopen(textfile,"r");
	if(fp == NULL){
		fprintf(stderr,"Can't open certificate text '%s'\n",textfile);
		return(NULL);
	}

	x509 = X509_new();
	if(x509 == NULL){
		ERR_print_errors(stderr);
		return(NULL);
	}

	i = f2i_X509(fp,x509);
	if(!i){
		fprintf(stderr,"Can't load certificate\n");
		ERR_print_errors(stderr);
                return(NULL);
        }

	if(fp) fclose(fp);
	return(x509);
}

/*
** NAME
**      sec_sign
**
** SYNOPSIS
**      #include "sec_makereq.h"
**
**	int sec_sign(x509,rsa)
**	X509 *x509 - Certificate to selfsign
**	RSA *rsa   - key for selfsigning
**
** DESCRIPTION
**      This function selfsigns a certificate to proof, that the request
**	is from this person.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
*/

int sec_sign(
X509 *x509,
RSA *rsa 
) {
	int		j;
	unsigned char 	*p;

        x509->cert_info->issuer = x509->cert_info->subject;
        X509_gmtime_adj(x509->cert_info->validity->notBefore,0);
        X509_gmtime_adj(x509->cert_info->validity->notAfter,60*60*24*28); /* 28 days */
        j = i2D_RSAPublicKey(rsa,NULL);

        p = x509->cert_info->key->public_key->data
          = (unsigned char *)malloc((unsigned int)j+10);
        if (p == NULL){ 
		fprintf(stderr,"out of memory\n"); 	
		return(-1); 
	}

        x509->cert_info->key->public_key->length = j;
        if (!i2D_RSAPublicKey(rsa,&p)){
                ERR_print_errors(stderr);
                return(-1);
        }

        if (!X509_sign(x509,rsa,NID_md5withRSAEncryption)){
                ERR_print_errors(stderr);
                return(-1);
        }

        return(0);
}

