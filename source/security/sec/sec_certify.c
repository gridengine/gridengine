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
#include <stdlib.h>
#include <string.h>

#include "sec_certify.h"

#define	POSTFIX ".srl"

/*
** NAME
**      sec_certify
**
** SYNOPSIS
**      #include "sec_certify.h"
**
**      int sec_certify()
**
** DESCRIPTION
**	This function certifies a given certificate with the key of
**	the certification authority.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
*/

int sec_certify(
char *cafile,
X509 *x,
X509 *xca,
RSA *rsa,
int create,
int days 
) {
        FILE *io;
        char *buf,buf2[1024];
        BIGNUM *serial;
        DER_BIT_STRING *bs,bs2;

        buf = (char *)malloc(RSA_size(rsa) * 2 + strlen(cafile) + strlen(POSTFIX));
        if (buf == NULL){ 
		fprintf(stderr,"out of mem\n"); 
		return(-1); 
	}

        strcpy(buf,cafile);
        strcat(buf,POSTFIX);

        serial=bn_new();
        bs=DER_BIT_STRING_new();
        if ((serial == NULL) || (bs == NULL)){
                ERR_print_errors(stderr);
                return(-1);
        }

        io=fopen(buf,"r");
        if (io == NULL){
                if (!create){
                        perror(buf);
                        return(-1);
                }
                else
                        bn_zero(serial);
        }
        else{
                if (!f2i_DER_BIT_STRING(io,bs,1024,buf2)){
                        fprintf(stderr,"unable to load serial number from %s\n",buf);
                        ERR_print_errors(stderr);
                        fclose(io);
                        return(-1);
                }
                else{
                        serial=bn_bin2bn(bs->length,bs->data,serial);
                        if (serial == NULL){
                                fprintf(stderr,"error converting bin 2 bn");
                                return(-1);
                        }
                }
                fclose(io);
        }

        if (!bn_add_word(serial,1)){
        	fprintf(stderr,"add_word failure\n"); 
		return(-1); 
	}

        bs2.data=(unsigned char *)buf2;
        bs2.length=bn_bn2bin(serial,bs2.data);

        io=fopen(buf,"w");
        if (io == NULL){
                fprintf(stderr,"error attempting to write serial number file\n");
                perror(buf);
                return(-1);
        }

        i2f_DER_BIT_STRING(io,&bs2);
        fclose(io);

        if (!X509_add_cert(x)) return(-1);

        /* NOTE: this certificate can/should be self signed */
        if (!X509_verify(x,sec_callb)) return(-1);

        /* don't free this X509 struct or bad things will happen
         * unless you put issuer first :-) */
        x->cert_info->issuer=xca->cert_info->subject;
        if (x->cert_info->validity->notBefore == NULL)
                free(x->cert_info->validity->notBefore);
        x->cert_info->validity->notBefore=(char *)malloc(100);
        x->cert_info->serialNumber=bs;

        if (x->cert_info->validity->notBefore == NULL){
                fprintf(stderr,"out of mem\n");
                return(-1);
        }

        X509_gmtime_adj(x->cert_info->validity->notBefore,0);
        /* hardwired expired */
        X509_gmtime_adj(x->cert_info->validity->notAfter,60*60*24*days);
        if (!X509_sign(x,rsa,NID_md5withRSAEncryption)){
                ERR_print_errors(stderr);
                return(-1);
        }

        return(0);
}

/*
** NAME
**      sec_callb
**
** SYNOPSIS
**      #include "sec_certify.h"
**
**      int sec_callb()
**
** DESCRIPTION
**	This function is the callback-function to verify a certificate.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
*/

int sec_callb(
int ok,
X509 *xs,
X509 *xi,
int depth,
int error 
) {
        /* it is ok to use a self signed certificate */
        if ((!ok) && (error == VERIFY_ERR_DEPTH_ZERO_SELF_SIGNED_CERT))
                return(1);

        /* BAD we should have gotten an error :-) */
        if (ok)
                printf("error with certificate to be certified - should be selfsigned\n");
        else{
                char *s;

                s=X509_oneline_X509_NAME(X509_get_subject_name(xs));
                printf("%s\n",s);
                free(s);
                printf("error with certificate - error %d at depth %d\n%s\n",
                        error,depth,X509_verify_error_string(error));
        }
#ifdef LINT
        xi=xs; xs=xi;
#endif
        return(0);
}

