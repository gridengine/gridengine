#ifndef __SEC_LOCAL_H
#define __SEC_LOCAL_H
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

#include "sec_crypto.h"
#include "sge_all_listsL.h"

#define CHALL_LEN       16
#define ValidHours      12          /* expiry of connection    */
#define ClearHours      1           /* search and destroy      */
                                    /* expired connections     */
                                    /* after every ClearHours  */

#define KeyPath         "KEY"
#define CertPath        "CERT"
#define SGESecPath      "SGE_SECURE"

#define CaKey           "ca_key.pem"
#define CaText          "ca_cert.txt"
#define CaTextDefault   "ca_def.txt"
#define CaCert          "ca_cert.pem"

#define RsaKey          "rsakey.pem"
#define CertText        "cert.txt"
#define CertTextDefault "default.txt"
#define CertReq         "request.pem"
#define Cert            "cert.pem"

#define ReconnectFile   "reconnect.dat"

#define DES_192_EDE3_CBC_WITH_MD5       0
#define NULL_WITH_MD5                   1   /* not implemented jet   */
#define RC4_128_WITH_MD5                2   /* not implemented jet  */
#define IDEA_128_CBC_WITH_MD5           3   /* not implemented jet  */
#define DES_64_CBC_WITH_MD5             4   

#define CIPHER_MODE                     DES_192_EDE3_CBC_WITH_MD5

#define INC32(a)        (((a) == 0xffffffffL)? 0:(a)+1)

typedef struct path_str {
   char *ca_key_file;
   char *ca_text_file;
   char *ca_textdefault_file;
   char *ca_cert_file;
   char *key_file;
   char *text_file;
   char *textdefault_file;
   char *req_file;
   char *cert_file;
   char *reconnect_file;
} FILES;


typedef struct gsd_str {
   int crypt_space;
   int block_len;
   u_char *key_mat;
   int key_mat_len;
   char *keys;
   int keys_len;
   RSA *rsa;
   X509 *x509;
   int x509_len;
   void (*crypt_init)(u_char *key_material);
   u_long32 (*crypt)(u_long32 len, u_char *from, u_char *to, int encrypt);
   u_long32 connid;
   u_long32 connid_counter;
   int issgesys;
   int connect;
   lList *conn_list;
   u_long32 seq_send;
   u_long32 seq_receive;
   char *refresh_time;
   FILES *files;
} GlobalSecureData;

typedef struct des_ede3_cbc_state_st {
   des_key_schedule        k1;
   des_key_schedule        k2;
   des_key_schedule        k3;
   des_cblock              iv;
} DES_EDE3_CBC_STATE;

typedef struct des_cbc_state_st {
   des_key_schedule        k1;
   des_cblock              iv;
} DES_CBC_STATE;

#define c4TOl(c,l)      (l =((unsigned long)(*((c)++)))<<24, \
                         l|=((unsigned long)(*((c)++)))<<16, \
                         l|=((unsigned long)(*((c)++)))<< 8, \
                         l|=((unsigned long)(*((c)++))))

#define lTO4c(l,c)      (*((c)++)=(unsigned char)(((l)>>24)&0xff), \
                         *((c)++)=(unsigned char)(((l)>>16)&0xff), \
                         *((c)++)=(unsigned char)(((l)>> 8)&0xff), \
                         *((c)++)=(unsigned char)(((l)    )&0xff))


#define GSD_BLOCK_LEN      8
#define GSD_KEY_MAT_32     32
#define GSD_KEY_MAT_16     16

#ifdef SEC_MAIN
GlobalSecureData   gsd;
#else
extern GlobalSecureData        gsd;
#endif /* SEC_MAIN   */

#endif /* __SEC_LOCAL_H */
