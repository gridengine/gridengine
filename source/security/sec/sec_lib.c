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
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <pthread.h>

#ifdef LOAD_OPENSSL
#ifdef LINUX
#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */
#endif /* LINUX */

#include <dlfcn.h>

#ifdef LINUX
#ifndef __USE_GNU
#undef __USE_GNU
#endif /* __USE_GNU */
#endif /* LINUX */

#ifdef SOLARIS
#include <link.h>
#endif /* SOLARIS */

/* This is the handle to the crypto library used to dyanmically load the
 * OpenSSL symbols. */
static void *handle = NULL;
#endif /* LOAD_OPENSSL */

#include "commlib.h"
#include "sge.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_prog.h" 
#include "basis_types.h"
#include "sge_any_request.h"
#include "sge_secL.h"
#include "sge_stdlib.h"
#include "sge_uidgid.h"
#include "sge_unistd.h"

#include "msg_sec.h"
#include "msg_utilib.h"

#include "sec_crypto.h"          /* lib protos      */
#include "sec_lib.h"             /* lib protos      */
#include "sge_mtutil.h"
#include "sgeobj/sge_answer.h"

#if (OPENSSL_VERSION_NUMBER < 0x0090700fL) 
#define OPENSSL_CONST
#define NID_userId NID_uniqueIdentifier
#else
#define OPENSSL_CONST const
#endif

#define CHALL_LEN       16
#define VALID_MINUTES    10          /* expiry of connection        */
#define SGESecPath      ".sge"
#define CaKey           "cakey.pem"
#define CaCert          "cacert.pem"
#define CA_DIR          "common/sgeCA"
#define CA_LOCAL_DIR    "/var/sgeCA"
#define USER_CA_LOCAL_DIR "/tmp/sgeCA"
#define UserKey         "key.pem"
#define UserCert        "cert.pem"
#define RandFile        "rand.seed"
#define ReconnectFile   "private/reconnect.dat"


#define INC32(a)        (((a) == 0xffffffff)? 0:(a)+1)
struct sec_state_t {
   u_char *key_mat;                  /* secret key used for connection per thread */
   u_long32 key_mat_len;             /* length of per connection secret key */
   u_long32 connid;                  /* per thread sec connection id */
   int connect;                      /* per thread connected state */
   u_long32 seq_send;                /* per thread send sequence number */
   u_long32 seq_receive;             /* per thread receive sequence number */
   ASN1_UTCTIME *refresh_time;       /* per thread refresh time in clients */
   char unique_identifier[BUFSIZ];   /* per thread unique identifier */
};

typedef struct gsd_str {
   X509 *x509;                       /* per process certificate of daemon or client 
                                        MT-NOTE: no mutex needed as long as write acess is done only by sec_init() */
   EVP_PKEY *private_key;            /* per process private key
                                        MT-NOTE: no mutex needed as long as write acess is done only by sec_init() */ 
   int is_daemon;                    /* per process information whether we are a deamon 
                                        MT-NOTE: no mutex needed as long as write acess is done only by sec_init() */
   OPENSSL_CONST EVP_CIPHER *cipher; /* per process cipher 
                                        MT-NOTE: no mutex needed as long as write acess is done only by sec_init() */
   OPENSSL_CONST EVP_MD *digest;     /* per process digest
                                        MT-NOTE: no mutex needed as long as write acess is done only by sec_init() */
   /* ASN1_UTCTIME *refresh_time;       per process in qmaster (constant) */
   struct sec_state_t sec_state;
} GlobalSecureData;


static pthread_key_t   sec_state_key;

static void sec_state_init(struct sec_state_t* state) {
   state->key_mat              = NULL;
   state->key_mat_len          = 0;
   state->connid               = 0;
   state->connect              = 0;
   state->seq_send             = 0;
   state->seq_receive          = 0;
   state->refresh_time         = NULL;
   state->unique_identifier[0] = '\0';
}

static void sec_state_destroy(void* state) {
   if (((struct sec_state_t *)state)->key_mat)
      free(((struct sec_state_t *)state)->key_mat);
   free(state);
}

static pthread_once_t sec_once_control = PTHREAD_ONCE_INIT;
void sec_once_init(void) {
   pthread_key_create(&sec_state_key, &sec_state_destroy);
}
void sec_mt_init(void) {
   pthread_once(&sec_once_control, sec_once_init);
}

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

/* Symbol table */
/* In order to reduce SGE's dependecy on OpenSSL, we no longer use OpenSSL
 * functions directly.  Instead, we use the function pointers below.  On
 * platforms with dlopen, i.e. Solaris and Linux, the functions will be
 * dynamically loaded from the OpenSSL library.  All such function pointers
 * start with "sec_" and have the same parameters as the OpenSSL functions.
 * Some of the functions in OpenSSL aren't really functions, but rather
 * #define's.  In order to maintain consistency, #define's that are used like
 * functions also begin with "sec_".  The prefix is then removed via a #define
 * so that OpenSSL's #define's can find them.
 * In some cases, a function is #define'd to another function.  In those cases
 * we simply load the new function instead of the #define'd one.
 * In some cases, a function is #define'd to another function with a different
 * set of parameters.  In those cases, we #define the "sec_" version with the
 * original parameters to a "_sec_" prefixed function pointer with the new
 * parameters.  The function pointer is then loaded with the new function.
 * Data types were left as they were.  Because of this the OpenSSL header files
 * are still needed for compiling.  The difficulty involved both technically and
 * legally in copying the data types out of the OpenSSL header files outweighed
 * the gain.
 * No new OpenSSL calls should be added to this file.  Instead, a new function
 * pointer should be created here and initialized in the sec_build_symbol_table()
 * method.
 */
static void (*sec_ASN1_UTCTIME_free)(ASN1_UTCTIME *a);
static ASN1_UTCTIME *(*sec_ASN1_UTCTIME_new)(void);
static int (*sec_ASN1_UTCTIME_print)(BIO *fp, ASN1_UTCTIME *a);
static int (*sec_BIO_free)(BIO *a);
static BIO *(*sec_BIO_new_file)(const char *filename, const char *mode);
static BIO *(*sec_BIO_new_fp)(FILE *stream, int close_flag);
static int (*sec_BIO_read)(BIO *b, void *data, int len);
static int (*sec_BIO_write)(BIO *b, const void *data, int len);
static const char *(*sec_CRYPTO_get_lock_name)(int type);
static int (*sec_CRYPTO_num_locks)(void);
static void (*sec_CRYPTO_set_id_callback)(unsigned long (*func)(void));
static void (*sec_CRYPTO_set_locking_callback)(void (*func)(int mode, int type,
					      const char *file, int line));
static ASN1_UTCTIME *(*sec_d2i_ASN1_UTCTIME)(ASN1_UTCTIME **a, unsigned char **in,
                     long len);
static X509 *(*sec_d2i_X509)(X509 **a, unsigned char **in, long len);
static void (*sec_ERR_clear_error)(void);
static char *(*sec_ERR_error_string)(unsigned long e, char *buf);
static unsigned long (*sec_ERR_get_error)(void);
static void (*sec_ERR_load_crypto_strings)(void);
static int (*sec_EVP_add_cipher)(const EVP_CIPHER *cipher);
static int (*sec_EVP_add_digest)(const EVP_MD *digest);
/* EVP_add_digest_alias is #define'd to OBJ_NAME_add */
#define sec_EVP_add_digest_alias(n, alias) \
	_sec_EVP_add_digest_alias((alias), OBJ_NAME_TYPE_MD_METH|OBJ_NAME_ALIAS, (n))
static int (*_sec_EVP_add_digest_alias)(const char *name, int type, const char *data);
/* EVP_CIPHER_block_size is a #define for structure member access */
#define sec_EVP_CIPHER_block_size EVP_CIPHER_block_size
static int (*sec_EVP_CIPHER_CTX_cleanup)(EVP_CIPHER_CTX *a);
/* EVP_CIPHER_CTX_iv_length is a #define for structure member access */
#define sec_EVP_CIPHER_CTX_iv_length EVP_CIPHER_CTX_iv_length
static int (*sec_EVP_DecryptFinal)(EVP_CIPHER_CTX *ctx, unsigned char *outm, int *outl);
static int (*sec_EVP_DecryptInit)(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher,
		const unsigned char *key, const unsigned char *iv);
static int (*sec_EVP_DecryptUpdate)(EVP_CIPHER_CTX *ctx, unsigned char *out,
		int *outl, const unsigned char *in, int inl);
static int (*sec_EVP_DigestInit)(EVP_MD_CTX *ctx, const EVP_MD *type);
static int (*sec_EVP_DigestFinal)(EVP_MD_CTX *ctx, unsigned char *md, unsigned int *s);
static int (*sec_EVP_DigestUpdate)(EVP_MD_CTX *ctx, const void *d, unsigned int cnt);
static int (*sec_EVP_EncryptInit)(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher,
		const unsigned char *key, const unsigned char *iv);
static int (*sec_EVP_EncryptUpdate)(EVP_CIPHER_CTX *ctx, unsigned char *out,
		int *outl, const unsigned char *in, int inl);
static int (*sec_EVP_EncryptFinal)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);
static const EVP_MD *(*sec_EVP_md5)(void);
static int (*sec_EVP_OpenInit)(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type,
		unsigned char *ek, int ekl, unsigned char *iv, EVP_PKEY *priv);
static int (*sec_EVP_OpenFinal)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);
/* EVP_OpenUpdate is #define'd to EVP_DecryptUpdate */
static int (*sec_EVP_OpenUpdate)(EVP_CIPHER_CTX *ctx, unsigned char *out,
		int *outl, const unsigned char *in, int inl);
static void (*sec_EVP_PKEY_free)(EVP_PKEY *pkey);
static int (*sec_EVP_PKEY_size)(EVP_PKEY *pkey);
static const EVP_CIPHER *(*sec_EVP_rc4)(void);
static int (*sec_EVP_SealInit)(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type, unsigned char **ek,
		int *ekl, unsigned char *iv, EVP_PKEY **pubk, int npubk);
static int (*sec_EVP_SealFinal)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);
static int (*sec_EVP_SignFinal)(EVP_MD_CTX *ctx, unsigned char *md, unsigned int *s,
		EVP_PKEY *pkey);
/* EVP_SignInit is #define'd to EVP_DigestInit */
static int (*sec_EVP_SignInit)(EVP_MD_CTX *ctx, const EVP_MD *type);
/* EVP_SignUpdate is #define'd to EVP_DigestUpdate */
static int (*sec_EVP_SignUpdate)(EVP_MD_CTX *ctx, const void *d, unsigned int cnt);
static int (*sec_EVP_VerifyFinal)(EVP_MD_CTX *ctx, unsigned char *sigbuf,
		unsigned int siglen, EVP_PKEY *pkey);
/* EVP_VerifyInit is #define'd to EVP_DigestInit */
static int (*sec_EVP_VerifyInit)(EVP_MD_CTX *ctx, const EVP_MD *type);
/* EVP_VerifyUpdate is #define'd to EVP_DigestUpdate */
static int (*sec_EVP_VerifyUpdate)(EVP_MD_CTX *ctx, const void *d, unsigned int cnt);
static int (*sec_i2d_ASN1_UTCTIME)(ASN1_UTCTIME *a, unsigned char **out);
static int (*sec_i2d_X509)(X509 *a, unsigned char** out);
static ASN1_OBJECT *(*sec_OBJ_nid2obj)(int n);
/* OPENSSL_free is #define'd to CRYPTO_free */
static void (*sec_OPENSSL_free)(void *a);
/* OPENSSL_malloc is #define'd to CRYPTO_malloc */
#define sec_OPENSSL_malloc(num)	_sec_OPENSSL_malloc((int)num,__FILE__,__LINE__)
static void *(*_sec_OPENSSL_malloc)(int num, const char *file, int line);
#ifdef SSLEAY_MACROS
/* PEM_read_PrivateKey is #define'd to PEM_ASN1_read when SSLEAY_MACROS is defined */
#define sec_PEM_read_PrivateKey(fp,x,cb,u) (EVP_PKEY *)_sec_PEM_read_PrivateKey( \
	(char *(*)())sec_d2i_PrivateKey,PEM_STRING_EVP_PKEY,fp,(char **)x,cb,u)
static char *(*_sec_PEM_read_PrivateKey)(char *(*d2i)(), const char *name, 
   FILE *fp, char **x, pem_password_cb *cb, void *u);
/* We also now need sec_d2i_PrivateKey */
static EVP_PKEY *(*sec_d2i_PrivateKey)(int type, EVP_PKEY **a, unsigned char **pp, long length);
/* PEM_read_X509 is #define'd to PEM_ASN1_read when SSLEAY_MACROS is defined */
#define sec_PEM_read_X509(fp,x,cb,u) (X509 *)_sec_PEM_read_X509( \
	(char *(*)())sec_d2i_X509,PEM_STRING_X509,fp,(char **)x,cb,u)
static char *(*_sec_PEM_read_X509)(char *(*d2i)(), const char *name, FILE *fp,
   char **x, pem_password_cb *cb, void *u);
#else
static EVP_PKEY *(*sec_PEM_read_PrivateKey)(FILE *fp, EVP_PKEY **x, pem_password_cb *cb, void *u);
static X509 *(*sec_PEM_read_X509)(FILE *fp, X509 **x, pem_password_cb *cb, void *u);
#endif
static int (*sec_RAND_load_file)(const char *file,long max_bytes);
static int (*sec_RAND_pseudo_bytes)(unsigned char *buf, int num);
static int (*sec_RAND_status)(void);
static int	(*sec_RSA_private_decrypt)(int flen, const unsigned char *from, 
		unsigned char *to, RSA *rsa, int padding);
static int	(*sec_RSA_public_encrypt)(int flen, const unsigned char *from,
		unsigned char *to, RSA *rsa, int padding);
static X509 *(*sec_X509_STORE_CTX_get_current_cert)(X509_STORE_CTX *ctx);
static int (*sec_X509_cmp_current_time)(ASN1_TIME *s);
static void (*sec_X509_free)(X509 *a);
/* X509_get_notBefore is a #define for structure member access */
#define sec_X509_get_notBefore X509_get_notBefore
/* X509_get_notAfter is a #define for structure member access */
#define sec_X509_get_notAfter X509_get_notAfter
static EVP_PKEY *(*sec_X509_get_pubkey)(X509 *x);
static int	(*sec_X509_get_pubkey_parameters)(EVP_PKEY *pkey, STACK_OF(X509) *chain);
static X509_NAME *(*sec_X509_get_subject_name)(X509 *a);
static ASN1_TIME *(*sec_X509_gmtime_adj)(ASN1_TIME *s, long adj);
static int (*sec_X509_print_fp)(FILE *bp,X509 *x);
static int	(*sec_X509_NAME_get_text_by_OBJ)(X509_NAME *name, ASN1_OBJECT *obj,
			char *buf,int len);
static char *(*sec_X509_NAME_oneline)(X509_NAME *a, char *buf, int size);
static void (*sec_X509_STORE_CTX_cleanup)(X509_STORE_CTX *ctx);
static int	(*sec_X509_STORE_CTX_get_error)(X509_STORE_CTX *ctx);
static int (*sec_X509_STORE_CTX_init)(X509_STORE_CTX *ctx, X509_STORE *store,
			 X509 *x509, STACK_OF(X509) *chain);
static void (*sec_X509_STORE_free)(X509_STORE *v);
static int	(*sec_X509_STORE_load_locations)(X509_STORE *ctx,	const char *file, const char *dir);
static X509_STORE *(*sec_X509_STORE_new)(void);
/* X509_STORE_set_verify_cb_func is a #define a structure member access */
#define sec_X509_STORE_set_verify_cb_func X509_STORE_set_verify_cb_func
static int	(*sec_X509_verify_cert)(X509_STORE_CTX *ctx);

/*
** prototypes needed to make openssl library MT safe
*/
static unsigned long sec_crypto_thread_id(void);
static void sec_crypto_locking_callback(int mode, int type, char *file, int line);
static void sge_thread_setup(void);
static void sec_thread_cleanup(void);

static int sec_alloc_key_mat(void);

#ifdef SEC_RECONNECT
static void sec_dealloc_key_mat(void);
#endif

/* 
** prototypes for per thread data access functions
*/
static void sec_state_set_key_mat(u_char *);
static u_char *sec_state_get_key_mat(void);
static void sec_state_set_key_mat_len(u_long32);
static u_long32 sec_state_get_key_mat_len(void);

static void sec_state_set_connid(u_long32);
static u_long32 sec_state_get_connid(void);
static void sec_state_set_connect(int);
static int sec_state_get_connect(void);

static void sec_state_set_seq_receive(u_long32);
static u_long32 sec_state_get_seq_receive(void);
static void sec_state_set_seq_send(u_long32);
static u_long32 sec_state_get_seq_send(void);
static void sec_state_set_refresh_time(ASN1_UTCTIME *);
static ASN1_UTCTIME *sec_state_get_refresh_time(void);
static char *sec_state_get_unique_identifier(void);
static void sec_state_set_unique_identifier(const char *unique_identifier);

/*
** prototypes
*/
static int sec_set_encrypt(int tag);
static int sec_files(void);
static int sec_is_daemon(const char *progname);
static int sec_is_master(const char *progname);


static int sec_announce_connection(cl_com_handle_t* cl_handle, GlobalSecureData *gsd, const cl_com_endpoint_t *sender);
static int sec_encrypt(sge_pack_buffer *pb, const char *buf, const int buflen);

static int sec_handle_announce(cl_com_handle_t* cl_handle, const cl_com_endpoint_t *sender, char *buffer, u_long32 buflen, unsigned long response_id);

static int sec_decrypt(char **buffer, u_long32 *buflen, const cl_com_endpoint_t *sender);

static int sec_respond_announce(cl_com_handle_t* cl_handle, const cl_com_endpoint_t *sender, char *buffer, u_long32 buflen, u_long32 response_id);
static int sec_verify_certificate(X509 *cert);

/* #define SEC_RECONNECT */
#ifdef SEC_RECONNECT

static int sec_dump_connlist(void);
static int sec_undump_connlist(void);
static int sec_reconnect(void);
static int sec_write_data2hd(void);
static int sec_pack_reconnect(sge_pack_buffer *pb, 
                              u_long32 connid, 
                              u_long32 seq_send, 
                              u_long32 seq_receive, 
                              u_char *key_mat, 
                              u_long32 key_mat_len,
                              u_char *refreshtime, 
                              u_long32 refreshtime_len);
static int sec_unpack_reconnect(sge_pack_buffer *pb, 
                                u_long32 *connid, 
                                u_long32 *seq_send, 
                                u_long32 *seq_receive, 
                                u_char **key_mat, 
                                u_long32 *key_mat_len,
                                u_char **refreshtime, 
                                u_long32 *refreshtime_len);

#endif

static int sec_set_secdata(const cl_com_endpoint_t *sender);
static int sec_set_connid(char **buffer, int *buflen);
static int sec_get_connid(char **buffer, u_long32 *buflen);
static int sec_update_connlist(const cl_com_endpoint_t *sender);

static int sec_send_err(const cl_com_endpoint_t *sender, sge_pack_buffer *pb,
                        const char *err_msg, u_long32 request_mid, lList **alp);
static int sec_insert_conn2list(u_long32 connid, const cl_com_endpoint_t *sender, const char *uniqueIdentifier);


static void sec_keymat2list(lListElem *element);

static void sec_list2keymat(lListElem *element);
static int sec_verify_callback(int ok, X509_STORE_CTX *ctx);
static void sec_setup_path(int is_daemon, int is_master);

static int sec_pack_announce(u_long32 len, u_char *buf, u_char *chall, sge_pack_buffer *pb);
static int sec_unpack_announce(u_long32 *len, u_char **buf, u_char **chall, sge_pack_buffer *pb);
static int sec_pack_response(u_long32 len, u_char *buf, 
                      u_long32 enc_chall_len, u_char *enc_chall, 
                      u_long32 connid, 
                      u_long32 enc_key_len, u_char *enc_key, 
                      u_long32 iv_len, u_char *iv, 
                      u_long32 enc_key_mat_len, u_char *enc_key_mat, 
                      sge_pack_buffer *pb);
static int sec_unpack_response(u_long32 *len, u_char **buf, 
                        u_long32 *enc_chall_len, u_char **enc_chall, 
                        u_long32 *connid, 
                        u_long32 *enc_key_len, u_char **enc_key, 
                        u_long32 *iv_len, u_char **iv, 
                        u_long32 *enc_key_mat_len, u_char **enc_key_mat, 
                        sge_pack_buffer *pb);
static int sec_pack_message(sge_pack_buffer *pb, u_long32 connid, u_long32 enc_mac_len, u_char *enc_mac, u_char *enc_msg, u_long32 enc_msg_len);
static int sec_unpack_message(sge_pack_buffer *pb, u_long32 *connid, u_long32 *enc_mac_len, u_char **enc_mac, u_long32 *enc_msg_len, u_char **enc_msg);
static int sec_build_symbol_table(void);

/****** security/sec_lib/debug_print_ASN1_UTCTIME() *************************************
*  NAME
*     debug_print_ASN1_UTCTIME() -- print an ASN1_UTCTIME string prefixed by a label 
*
*  SYNOPSIS
*     static void debug_print_ASN1_UTCTIME(char *label, ASN1_UTCTIME *time) 
*
*  FUNCTION
*     An ASN1_UTCTIME is converted to char* and label and the time string are printed
*
*  INPUTS
*     char *label        - label string 
*     ASN1_UTCTIME *time - time in ASN1_UTCTIME format
*
*  RESULT
*     static void - 
*
*  EXAMPLE
*     debug_print_ASN1_UTCTIME("refresh_time: ", sec_state_get_refresh_time());
*     debug_print_ASN1_UTCTIME("Certificate not valid before: ", sec_X509_get_notBefore(cert));
*
*  SEE ALSO
*     security/sec_lib/debug_print_buffer()
*
*  NOTES
*     MT-NOTE: 
*******************************************************************************/
static void debug_print_ASN1_UTCTIME(char *label, ASN1_UTCTIME *time)
{
   BIO *out;
   printf("%s", label);
   out = sec_BIO_new_fp(stdout, BIO_NOCLOSE);
   sec_ASN1_UTCTIME_print(out, time);   
   sec_BIO_free(out);
   printf("\n");
}   
   
/* #define SEC_DEBUG    */
#ifdef SEC_DEBUG
/****** security/sec_lib/debug_print_buffer() ************************
*  NAME
*     debug_print_buffer() -- print a title, buflen and buf as a 
*                             sequence of hex values
*
*  SYNOPSIS
*     static void debug_print_buffer(char *title, unsigned char *buf,
*                                    int buflen) 
*
*  FUNCTION
*     Print a title, buflen and buf as a sequence of hex values.
*
*  INPUTS
*     char *title        - title
*     unsigned char *buf - buffer keeping a sequence of binary info 
*     int buflen         - length of buffer
*
*  RESULT
*     static void - 
*
*  EXAMPLE
*     DEBUG_PRINT_BUFFER("Encrypted incoming message", 
*                         (unsigned char*) (*buffer), (int)(*buflen));
*
*  NOTES
*     Used as a macro DEBUG_PRINT_BUFFER(x,y,z) which expands to the 
*     empty string if SEC_BEBUG is undefined and to debug_print_buffer() 
*     if SEC_BEBUG is defined.
*     MT-NOTE: 
*
*  SEE ALSO
*     security/sec_lib/debug_print_ASN1_UTCTIME()
*******************************************************************************/
static void debug_print_buffer(char *title, unsigned char *buf, int buflen)
{
   int j;
   
   printf("------------------------------------------------------\n");
   printf("--- %s\n", title);
   printf("Buffer size: %d\n", buflen);
   
   for (j=0; buf && j<buflen; j++) {
      printf("%02x", buf[j]);
   }
   printf("\n");
   printf("------------------------------------------------------\n");
}

/*
** NAME
**   sec_error
**
** SYNOPSIS
**   
**   static void sec_error()
**
** DESCRIPTION
**      This function prints error messages from crypto library
**
** NOTES
**      MT-NOTE: sec_error() is MT safe
*/
static void sec_error(void)
{
   long   l;

   DENTER(GDI_LAYER,"sec_error");
   while ((l=sec_ERR_get_error())){
      ERROR((SGE_EVENT, "%s\n", sec_ERR_error_string(l, NULL)));
   }
   DEXIT;
}


#  define DEBUG_PRINT(x, y)              printf(x,y)
#  define DEBUG_PRINT_BUFFER(x,y,z)      debug_print_buffer(x,y,z)
#  define sge_sec_error()                sec_error() 
#else
#  define sge_sec_error()
#  define DEBUG_PRINT(x,y)
#  define DEBUG_PRINT_BUFFER(x,y,z)
#endif /* SEC_DEBUG */

/*
** FIXME
*/
/* #define PREDEFINED_PW         "troet" */
#define PREDEFINED_PW         NULL


/*
** process global secure data
*/
static GlobalSecureData gsd;

/*
** MT-NOTE: see GlobalSecureData typedef for detailed MT notes on
** MT-NOTE: each gsd struct member 
*/

/* ---- sec_initialized --------------------------------- */

static int sec_initialized = 0;

/* 
** MT-NOTE: sec_initialized_mutex guards access to sec_initialized 
** MT-NOTE: that must be synchronized in case of any MT program using sec lib
*/

static pthread_mutex_t sec_initialized_mutex = PTHREAD_MUTEX_INITIALIZER;
#define SEC_LOCK_INITIALIZED()   sge_mutex_lock("sec_initialized_mutex", SGE_FUNC, __LINE__, &sec_initialized_mutex)
#define SEC_UNLOCK_INITIALIZED() sge_mutex_unlock("sec_initialized_mutex", SGE_FUNC, __LINE__, &sec_initialized_mutex)

static pthread_mutex_t sec_global_data_mutex = PTHREAD_MUTEX_INITIALIZER;
#define SEC_LOCK_GLOBAL_SD()   sge_mutex_lock("sec_global_data_mutex", SGE_FUNC, __LINE__, &sec_global_data_mutex)
#define SEC_UNLOCK_GLOBAL_SD() sge_mutex_unlock("sec_global_data_mutex", SGE_FUNC, __LINE__, &sec_global_data_mutex)
/* ---- sec_connid_counter -- a consecutive number for connections -- */

static u_long32 sec_connid_counter = 0;


/* 
** MT-NOTE: sec_connid_counter_mutex guards access to sec_connid_counter 
** MT-NOTE: that must be synchronized in case of a MT qmaster
*/
static pthread_mutex_t sec_connid_counter_mutex = PTHREAD_MUTEX_INITIALIZER;


#define SEC_LOCK_CONNID_COUNTER()   sge_mutex_lock("sec_connid_counter_mutex", SGE_FUNC, __LINE__, &sec_connid_counter_mutex)
#define SEC_UNLOCK_CONNID_COUNTER() sge_mutex_unlock("sec_connid_counter_mutex", SGE_FUNC, __LINE__, &sec_connid_counter_mutex)
/* ---- sec_conn_list --------------------------------- */

static lList *sec_conn_list = NULL;

/* 
** MT-NOTE: sec_conn_list_mutex guards access to sec_conn_list 
** MT-NOTE: that must be synchronized in case of a MT qmaster
*/
static pthread_mutex_t sec_conn_list_mutex = PTHREAD_MUTEX_INITIALIZER;

#define SEC_LOCK_CONN_LIST()      sge_mutex_lock("sec_conn_list_mutex", SGE_FUNC, __LINE__, &sec_conn_list_mutex)
#define SEC_UNLOCK_CONN_LIST()    sge_mutex_unlock("sec_conn_list_mutex", SGE_FUNC, __LINE__, &sec_conn_list_mutex)

/* ---- *_file --------------------------------- */

static char *ca_key_file;
static char *reconnect_file;
static char *ca_cert_file;
static char *key_file;
static char *cert_file;
static char *rand_file;

/* 
** MT-NOTE: no mutex needed for all *_file global variables as long as write access 
** MT-NOTE: is done only in MT safe sec_init() 
*/

/* 
** Two callback functions are needed for making -lcrypto MT safe 
*/ 

/* this looks like a global variable, but actually it's an array of mutexes 
   one must be sure that 'lock_cs' is initialized only once */
static pthread_mutex_t *lock_cs = NULL;
static long *lock_count = NULL;

/****** sec_lib/sec_crypto_thread_id() *****************************************
*  NAME
*     sec_crypto_thread_id() -- Callback function returning thread id
*
*  SYNOPSIS
*     static unsigned long sec_crypto_thread_id(void) 
*
*  FUNCTION
*     Callback function returning a threads id. This is needed to
*     make -lcrypto MT safe.
*
*  RESULT
*     static unsigned long - The thread ID
*
*  NOTES
*     MT-NOTE: sec_crypto_thread_id() is MT safe
*
*  SEE ALSO
*     OpenSSL threads(3) man page 
*******************************************************************************/
static unsigned long sec_crypto_thread_id(void)
{
   return (unsigned long)pthread_self();
}

/****** sec_lib/sec_crypto_locking_callback() **********************************
*  NAME
*     sec_crypto_locking_callback() -- Callback function for mutex (un)locking 
*
*  SYNOPSIS
*     static void sec_crypto_locking_callback(int mode, int type, char *file, 
*     int line) 
*
*  FUNCTION
*     Callback function that (un)locks a mutex. This is needed to
*     make -lcrypto MT safe.
*
*  INPUTS
*     int mode   - Specifies lock/unlock case.
*     int type   - Unclear. Simply taken from mttest.c referenced in OpenSSL threads(3) 
*     char *file - OpenSSL source file from where we are called.
*     int line   - Line number in OpenSSL source file from where we are called.
*
*  NOTES
*     MT-NOTES: sec_crypto_locking_callback() is MT safe
*
*  SEE ALSO
*     OpenSSL threads(3) man page 
*******************************************************************************/
static void sec_crypto_locking_callback(int mode, int type, char *file, int line)
{
   char mutex_name[255];

   sprintf(mutex_name, "crypto-lock[ mode=%s lock=%s %s:%d ]", 
                (mode&CRYPTO_LOCK)?"l":"u",
                (type&CRYPTO_READ)?"r":"w", file, line);
   if (mode & CRYPTO_LOCK) {
      sge_mutex_lock(mutex_name, "sec_crypto_locking_callback", __LINE__, &(lock_cs[type]));
   } else {
      sge_mutex_unlock(mutex_name, "sec_crypto_locking_callback", __LINE__, &(lock_cs[type]));
   }
   return;
}

/****** sec_lib/sge_thread_setup() *********************************************
*  NAME
*     sge_thread_setup() -- Make -lcrypto MT safe
*
*  SYNOPSIS
*     static void sge_thread_setup(void) 
*
*  FUNCTION
*     We use -lcrypto threads(3) interface to allow OpenSSL safely be used by 
*     multiple threads. No -lcrypto function may be called prior this function
*     was called.
*
*  NOTES
*     MT-NOTES: sge_thread_setup() is not MT safe 
*
*  SEE ALSO
*     OpenSSL threads(3) man page
*******************************************************************************/
static void sge_thread_setup(void)
{
   int i;

   DENTER(TOP_LAYER, "sge_thread_setup");

   lock_cs=sec_OPENSSL_malloc(sec_CRYPTO_num_locks() * sizeof(pthread_mutex_t));
   lock_count=sec_OPENSSL_malloc(sec_CRYPTO_num_locks() * sizeof(long));
   for (i=0; i<sec_CRYPTO_num_locks(); i++) {
      lock_count[i]=0;
      pthread_mutex_init(&(lock_cs[i]),NULL);
   }

   sec_CRYPTO_set_id_callback((unsigned long (*)())sec_crypto_thread_id);
   sec_CRYPTO_set_locking_callback((void (*)())sec_crypto_locking_callback);

   DEXIT;
   return;
}

/****** sec_lib/sec_thread_cleanup() *******************************************
*  NAME
*     sec_thread_cleanup() --  Make -lcrypto MT unsafe
*
*  SYNOPSIS
*     static void sec_thread_cleanup(void) 
*
*  FUNCTION
*     As part of the libsec library shutdown call back functions are 
*     unregistered and mutexes that were used for making -lcrypto MT 
*     safe are freed.
*
*  NOTES
*     MT-NOTES: sec_thread_cleanup() is not MT safe 
*
*  SEE ALSO
*     OpenSSL threads(3) man page
*******************************************************************************/
static void sec_thread_cleanup(void)
{
   int i;

   DENTER(TOP_LAYER, "sec_thread_cleanup");

   sec_CRYPTO_set_locking_callback(NULL);

   for (i=0; i<sec_CRYPTO_num_locks(); i++) {
      pthread_mutex_destroy(&(lock_cs[i]));
      DPRINTF(("%8ld:%s\n", lock_count[i], sec_CRYPTO_get_lock_name(i)));
   }
   sec_OPENSSL_free(lock_cs);
   sec_OPENSSL_free(lock_count);

   DEXIT;
   return;
}


/****** sec_lib/sec_alloc_key_mat() ********************************************
*  NAME
*     sec_alloc_key_mat() -- Convenience function to alloc/init per thread 
*                            key_mat* variables.
*
*  SYNOPSIS
*     static int sec_alloc_key_mat(void) 
*
*  RESULT
*     static int - 0 on success, -1 on failure
*
*  NOTES
*     MT-NOTE: sec_alloc_key_mat() is MT safe
*******************************************************************************/
static int sec_alloc_key_mat(void)
{
   /* must be done per thread on demand before accessing key_mat */
   sec_state_set_key_mat_len(GSD_KEY_MAT_32);
   if (!sec_state_get_key_mat())
      sec_state_set_key_mat((u_char *) malloc(GSD_KEY_MAT_32));
   if (!sec_state_get_key_mat())
      return -1;
      
   return 0;
}

#ifdef SEC_RECONNECT
/****** sec_lib/sec_dealloc_key_mat() ******************************************
*  NAME
*     sec_dealloc_key_mat() -- Convenience function to dealloc/init per thread
*                              key_mat* variables.
*
*  SYNOPSIS
*     static void sec_dealloc_key_mat(void) 
*
*  NOTES
*     MT-NOTE: sec_dealloc_key_mat() is MT safe
*******************************************************************************/
static void sec_dealloc_key_mat(void)
{
   u_char *buf;
   if ((buf=sec_state_get_key_mat())) {
      free(buf);
      sec_state_set_key_mat(NULL);
   }

   return;
}
#endif

/****** sec_lib/sec_state_set_key_mat() ****************************************
*  NAME
*     sec_state_{s|g}et_*() -- Per thread global variables setter/getter funcs
*
*  FUNCTION
*     Used to set and get values of sec lib per thread global variables.
*
*  NOTES
*     MT-NOTE: sec_state_{s|g}et_*() functions are MT safe
*
*  SEE ALSO
*     See definition of 'struct sec_state_t' for information about purpose
*     of these variables.
*******************************************************************************/
static void sec_state_set_key_mat(u_char *key_mat)
{
   DENTER(GDI_LAYER, "sec_state_set_key_mat");
   if (uti_state_get_mewho() != QMASTER) {
      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_set_key_mat");
      sec_state->key_mat = key_mat;
   } else { 
      SEC_LOCK_GLOBAL_SD();
      gsd.sec_state.key_mat = key_mat;
      SEC_UNLOCK_GLOBAL_SD();
   }
   DEXIT;
}

static u_char *sec_state_get_key_mat(void)
{
   u_char * ret_val = NULL;
   
   DENTER(GDI_LAYER, "sec_state_get_key_mat");

   if (uti_state_get_mewho() != QMASTER) {
      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, 
                     sec_state_key, "sec_state_get_key_mat");
      ret_val = sec_state->key_mat;
   } else { 
      /* copy global data to thread specific data */
      SEC_LOCK_GLOBAL_SD();
      {
         GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_get_key_mat");
         sec_state->key_mat = gsd.sec_state.key_mat;
         ret_val = sec_state->key_mat;
      }
      SEC_UNLOCK_GLOBAL_SD();
   }
   DEXIT;
   return ret_val;
}

static void sec_state_set_key_mat_len(u_long32 key_mat_len)
{
   DENTER(GDI_LAYER, "sec_state_set_key_mat_len");

   if (uti_state_get_mewho() != QMASTER) {
      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_set_key_mat_len");
      sec_state->key_mat_len = key_mat_len;
   } else { 
      SEC_LOCK_GLOBAL_SD();
      gsd.sec_state.key_mat_len = key_mat_len;
      SEC_UNLOCK_GLOBAL_SD();
   }
   DEXIT;

}

static u_long32 sec_state_get_key_mat_len(void)
{
   u_long32 ret_val = 0;
   DENTER(GDI_LAYER, "sec_state_get_key_mat_len");

   if (uti_state_get_mewho() != QMASTER) {
      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_get_key_mat_len");
      ret_val = sec_state->key_mat_len;
   } else { 
      /* copy global data to thread specific data */
      SEC_LOCK_GLOBAL_SD();
      {
         GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_get_key_mat_len");
         sec_state->key_mat_len = gsd.sec_state.key_mat_len;
         ret_val = sec_state->key_mat_len;
      }
      SEC_UNLOCK_GLOBAL_SD();
   }
   DEXIT;
   return ret_val;
}

static void sec_state_set_connid(u_long32 connid)
{
   DENTER(GDI_LAYER, "sec_state_set_connid");

   if (uti_state_get_mewho() != QMASTER) {
      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_set_connid");
      sec_state->connid = connid;
   } else { 
      SEC_LOCK_GLOBAL_SD();
      gsd.sec_state.connid = connid;
      SEC_UNLOCK_GLOBAL_SD();
   }
   DEXIT;

}

static u_long32 sec_state_get_connid(void)
{ 
   u_long32 ret_val = 0;

   DENTER(GDI_LAYER, "sec_state_get_connid");

   if (uti_state_get_mewho() != QMASTER) {
      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_get_connid");
      ret_val = sec_state->connid;
   } else {
      /* copy global data to thread specific data */
      SEC_LOCK_GLOBAL_SD();
      {
         GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_get_connid");
         sec_state->connid = gsd.sec_state.connid;
         ret_val = sec_state->connid;
      }
      SEC_UNLOCK_GLOBAL_SD();
   }
   DEXIT;
   return ret_val;
}

static void sec_state_set_connect(int connect)
{ 
   DENTER(GDI_LAYER, "sec_state_set_connect");

   if (uti_state_get_mewho() != QMASTER) {
      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_set_connect");
      sec_state->connect = connect;
   } else {
      SEC_LOCK_GLOBAL_SD();
      gsd.sec_state.connect = connect;
      SEC_UNLOCK_GLOBAL_SD();
   }
   DEXIT;
}

static int sec_state_get_connect(void)
{ 
   int ret_val = 0;

   DENTER(GDI_LAYER, "sec_state_get_connect");

   if (uti_state_get_mewho() != QMASTER) {
      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_get_connect");
      ret_val = sec_state->connect;
   } else {
      /* copy global data to thread specific data */
      SEC_LOCK_GLOBAL_SD();
      {
         GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_get_connect");
         sec_state->connect = gsd.sec_state.connect;   
         ret_val = sec_state->connect;
      }
      SEC_UNLOCK_GLOBAL_SD();
   }
   DEXIT;
   return ret_val;
}

static void sec_state_set_seq_receive(u_long32 seq_receive)
{ 
   DENTER(GDI_LAYER, "sec_state_set_seq_receive");

   if (uti_state_get_mewho() != QMASTER) {
      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_set_seq_receive");
      sec_state->seq_receive = seq_receive;
   } else {
      SEC_LOCK_GLOBAL_SD();
      gsd.sec_state.seq_receive = seq_receive;
      SEC_UNLOCK_GLOBAL_SD();
   }
   DEXIT;

}

static u_long32 sec_state_get_seq_receive(void)
{ 
   u_long32 ret_val = 0;

   DENTER(GDI_LAYER, "sec_state_get_seq_receive");

   if (uti_state_get_mewho() != QMASTER) {
      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_get_seq_receive");
      ret_val = sec_state->seq_receive;
   } else {
      /* copy global data to thread specific data */
      SEC_LOCK_GLOBAL_SD();
      {
         GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_get_seq_receive");
         sec_state->seq_receive = gsd.sec_state.seq_receive;
         ret_val = sec_state->seq_receive;
      }
      SEC_UNLOCK_GLOBAL_SD();
   }
   DEXIT;
   return ret_val;
}

static void sec_state_set_seq_send(u_long32 seq_send)
{
   DENTER(GDI_LAYER, "sec_state_set_seq_send");

   if (uti_state_get_mewho() != QMASTER) {
      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_set_seq_send");
      sec_state->seq_send = seq_send;
   } else {
      SEC_LOCK_GLOBAL_SD();
      gsd.sec_state.seq_send = seq_send;
      SEC_UNLOCK_GLOBAL_SD();
   }
   DEXIT;
}

static u_long32 sec_state_get_seq_send(void)
{
   u_long32 ret_val = 0;

   DENTER(GDI_LAYER, "sec_state_get_seq_send");

   if (uti_state_get_mewho() != QMASTER) {
      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_get_seq_send");
      ret_val = sec_state->seq_send;
   } else {
      /* copy global data to thread specific data */
      SEC_LOCK_GLOBAL_SD();
      {
         GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_get_seq_send");
         sec_state->seq_send = gsd.sec_state.seq_send;  
         ret_val = sec_state->seq_send;
      }
      SEC_UNLOCK_GLOBAL_SD();
   }
   DEXIT;
   return ret_val;
}

static void sec_state_set_refresh_time(ASN1_UTCTIME *refresh_time)
{
   DENTER(GDI_LAYER, "sec_state_set_refresh_time");

   if (uti_state_get_mewho() != QMASTER) {

      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_set_refresh_time");
      sec_state->refresh_time = refresh_time;
   } else {
      SEC_LOCK_GLOBAL_SD();
      gsd.sec_state.refresh_time = refresh_time;
      SEC_UNLOCK_GLOBAL_SD();
   }
   DEXIT;
}

static ASN1_UTCTIME *sec_state_get_refresh_time(void)
{
   DENTER(GDI_LAYER, "sec_state_get_refresh_time");

   if (uti_state_get_mewho() != QMASTER) {
      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_get_refresh_time");
      DEXIT;
      return sec_state->refresh_time;
   } else {
      ASN1_UTCTIME * ret_val;
      /* copy global data to thread specific data */
      SEC_LOCK_GLOBAL_SD();
      {
         GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_get_refresh_time");
         sec_state->refresh_time = gsd.sec_state.refresh_time;
         ret_val = sec_state->refresh_time;
      }
      SEC_UNLOCK_GLOBAL_SD();
      DEXIT;
      return ret_val;
   }
}

static char *sec_state_get_unique_identifier(void)
{
   char *ret_val = NULL;

   DENTER(GDI_LAYER, "sec_state_get_unique_identifier");

   if (uti_state_get_mewho() != QMASTER) {
      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_get_unique_identifier");
      DEXIT;
      ret_val = sec_state->unique_identifier;
   } else {
      /* copy global data to thread specific data */
      SEC_LOCK_GLOBAL_SD();
      {
         GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_get_unique_identifier");
         strncpy(sec_state->unique_identifier, gsd.sec_state.unique_identifier , BUFSIZ);
         ret_val = sec_state->unique_identifier;
      }
      SEC_UNLOCK_GLOBAL_SD();
   }
   DEXIT;
   return ret_val;
}

static void sec_state_set_unique_identifier(const char *unique_identifier)
{
   DENTER(GDI_LAYER, "sec_state_set_unique_identifier");
   if (uti_state_get_mewho() != QMASTER) {
      GET_SPECIFIC(struct sec_state_t, sec_state, sec_state_init, sec_state_key, "sec_state_set_unique_identifier");
      strncpy(sec_state->unique_identifier, unique_identifier, BUFSIZ);
   } else {
      SEC_LOCK_GLOBAL_SD();
      strncpy(gsd.sec_state.unique_identifier, unique_identifier, BUFSIZ );
      SEC_UNLOCK_GLOBAL_SD();
   }
   DEXIT;
}

/****** security/sec_lib/sec_init() ******************************************
*  NAME
*     sec_init() -- initialize CSP security 
*
*  SYNOPSIS
*     int sec_init(const char *progname) 
*
*  FUNCTION
*     This function initialices all security related data for the process
*     This must be done only once, typically before the enroll to the commd.
*
*  INPUTS
*     const char *progname - commproc name to distinguish daemons and non-daemons 
*
*  RESULT
*     int 0 on success, -1 on failure
*
*  NOTES
*     MT-NOTE: sec_init() is MT safe
*******************************************************************************/
int sec_init(const char *progname) 
{
   char *randfile = NULL;
   SGE_STRUCT_STAT file_info;
   int is_master;
   int ec = 0;

   DENTER(GDI_LAYER, "sec_init");

   SEC_LOCK_INITIALIZED();

   if (sec_initialized) {
      SEC_UNLOCK_INITIALIZED();
      DEXIT;
      return 0;
   }   
   
   /* Build dynamic symbol table */
   if ((ec = sec_build_symbol_table ()) != 0) {
         SEC_UNLOCK_INITIALIZED();
         if (ec == 1)
            ERROR((SGE_EVENT, MSG_SEC_DLLOADFAILED));
         else
            ERROR((SGE_EVENT, MSG_SEC_DLLOADSYMFAILED));
         DEXIT;
         return -1;
   }


   /* use -lcrypto threads(3) interface here to allow OpenSSL 
      safely be used by multiple threads */
   sge_thread_setup();

   /*
   ** set everything to zero
   */
   memset(&gsd, 0, sizeof(gsd));

   /*
   ** initialize error strings
   */
   sec_ERR_clear_error();
   sec_ERR_load_crypto_strings();

   /*
   ** load all algorithms and digests
   */
/*    OpenSSL_add_all_algorithms(); */
#ifdef CITIGROUP
   OpenSSL_add_all_algorithms();
/*    sec_EVP_add_cipher(sec_EVP_rc4()); */
/*    sec_EVP_add_digest(sec_EVP_sha1()); */
/*    sec_EVP_add_digest_alias(SN_sha1,"ssl3-sha1"); */
/*    sec_EVP_add_digest_alias(SN_sha1WithRSAEncryption,SN_sha1WithRSA); */
#else
   sec_EVP_add_cipher(sec_EVP_rc4());
   sec_EVP_add_digest(sec_EVP_md5());
   sec_EVP_add_digest_alias(SN_md5,"ssl2-md5");
   sec_EVP_add_digest_alias(SN_md5,"ssl3-md5");
#endif

   /* 
   ** FIXME: am I a sge daemon? -> is_daemon() should be implemented
   */
   gsd.is_daemon = sec_is_daemon(progname);
   is_master     = sec_is_master(progname);

   /* 
   ** setup the filenames of randfile, certificates, keys, etc.
   */
   sec_setup_path(gsd.is_daemon, is_master);

   /* 
   ** seed PRNG, /dev/random is used if possible
   ** if RANDFILE is set it is used
   ** otherwise rand_file is used
   */
   if (!sec_RAND_status()) {
      randfile = getenv("RANDFILE");
      if (!SGE_STAT(randfile, &file_info)) {
         sec_RAND_load_file(randfile, 2048);
      } else if (rand_file) {   
         sec_RAND_load_file(rand_file, 2048); 
      } else {
         SEC_UNLOCK_INITIALIZED();
         ERROR((SGE_EVENT, MSG_SEC_RANDFILENOTSET));
         DEXIT;
         return -1;
      }
   }

   /* 
   ** FIXME: init all the other stuff
   */
#ifdef CITIGROUP   
   gsd.digest = sec_EVP_sha1();
   gsd.cipher = sec_EVP_rc4();
#else   
   gsd.digest = sec_EVP_md5();
   gsd.cipher = sec_EVP_rc4();
/*    gsd.cipher = sec_EVP_des_cbc(); */
/*    gsd.cipher = sec_EVP_enc_null(); */
#endif   

   if(strcmp(progname, prognames[QMASTER])) {
      /* 
      ** time after client makes a new announce to master   
      */
      sec_state_set_refresh_time(sec_X509_gmtime_adj(sec_state_get_refresh_time(), (long) 60*VALID_MINUTES));
   }   

   sec_state_set_connid(0);
   sec_state_set_seq_send(0);
   sec_state_set_connect(0);
   sec_state_set_seq_receive(0);
   if (is_master) {
      SEC_LOCK_CONN_LIST();
      sec_conn_list = lCreateList("sec_conn_list", SecurityT);
      SEC_UNLOCK_CONN_LIST();
   }   

   /* 
   ** read security related files
   */
   if (sec_files()){
      SEC_UNLOCK_INITIALIZED();
      DEXIT;
      return -1;
   }

#ifdef SEC_RECONNECT   
   /*
   ** read reconnect data
   ** FIXME
   */
   if (!gsd.is_daemon)
      sec_reconnect();
#endif      

   
   DPRINTF(("====================[  CSP SECURITY  ]===========================\n"));

   sec_initialized = 1;
   SEC_UNLOCK_INITIALIZED();

   DEXIT;
   return 0;
}

/****** sec_lib/sec_exit() *****************************************************
*  NAME
*     sec_exit() -- Shutown CSP security 
*
*  SYNOPSIS
*     int sec_exit() 
*
*  FUNCTION
*     All actions that are required for shutting down CSP security are performed.
*
*  RESULT
*     int 0 on success, -1 on failure
*
*  NOTES
*     MT-NOTES: sec_exit() is MT safe
*******************************************************************************/
int sec_exit(void) 
{
   DENTER(GDI_LAYER, "sec_exit");

   SEC_LOCK_INITIALIZED();
   if (!sec_initialized) {
      SEC_UNLOCK_INITIALIZED();
      DEXIT;
      return -1;
   }   

   /* dump client reconnect data to file */
#ifdef SEC_RECONNECT   
   if(!gsd.is_daemon)
      sec_write_data2hd();
#endif

   sec_thread_cleanup();
   
#ifdef LOAD_OPENSSL
   dlclose (handle);
#endif
   
   sec_initialized = 0;   
   SEC_UNLOCK_INITIALIZED();

   DEXIT;
   return 0;
}


/****** security/sec_lib/sec_send_message() *********************************************
*  NAME
*     sec_send_message() -- CSP secured version of send_message()
*
*  SYNOPSIS
*     int sec_send_message(int synchron, const char *tocomproc, int toid, const 
*     char *tohost, int tag, char *buffer, int buflen, u_long32 *mid, int 
*     compressed) 
*
*  FUNCTION
*     This function is used instead of the normal send_message call from
*     the commlib. It checks if a client has to announce to the master and 
*     if the message should be encrypted and then it sends the message.
*
*  INPUTS
*     int synchron          - transfer modus,
*     const char *tocomproc - name destination program,
*     int toid              - id of communication,
*     const char *tohost    - destination host,
*     int tag               - tag of message,
*     char *buffer          - buffer which contains the message to send
*     int buflen            - lenght of message,
*     u_long32  *mid        - id for asynchronous messages;
*     int compressed        - use zlib compression 
*
*  RESULT
*     int - 0 on success, CL_ or SEC_ errors otherwise
*
*  NOTES
*     MT-NOTE: sec_send_message() is not MT safe
*******************************************************************************/
#ifdef ENABLE_NGC
int sec_send_message(cl_com_handle_t* cl_handle,
                     char* un_resolved_hostname, char* component_name, unsigned long component_id, 
                     cl_xml_ack_type_t ack_type, 
                     cl_byte_t* data, unsigned long size , 
                     unsigned long* mid, unsigned long response_mid, unsigned long tag ,
                     int copy_data,
                     int wait_for_ack
) {
   int i = 0;
   sge_pack_buffer pb;
   char* buffer;
   int buflen;
   cl_com_endpoint_t *destination = NULL;

   DENTER(GDI_LAYER, "sec_send_message");

   destination = cl_com_create_endpoint(un_resolved_hostname, component_name, component_id);

   buffer = (char*) data;
   buflen = size;
   /*
   ** every component has to negotiate its connection with qmaster
   */
   if (uti_state_get_mewho() != QMASTER) {
      if (sec_announce_connection(cl_handle,&gsd, destination)) {
/*          ERROR((SGE_EVENT, MSG_SEC_ANNOUNCEFAILED)); */
         DPRINTF((MSG_SEC_ANNOUNCEFAILED));
         cl_com_free_endpoint(&destination);
         DEXIT;
         return CL_RETVAL_SECURITY_ANNOUNCE_FAILED;
      }
   }   
      
   if (sec_set_encrypt(tag)) {
      if (uti_state_get_mewho() == QMASTER) {
         SEC_LOCK_CONN_LIST();
         if (sec_conn_list) {
            /*
            ** set the corresponding key and connection information
            ** in the connection list for commd triple 
            ** (tohost, tocomproc, toid)
            */
            if (sec_set_secdata(destination)) {
               SEC_UNLOCK_CONN_LIST();
               ERROR((SGE_EVENT, MSG_SEC_SETSECDATAFAILED));
               cl_com_free_endpoint(&destination);
               DEXIT;
               return CL_RETVAL_SECURITY_SEND_FAILED;
            }
         }
         SEC_UNLOCK_CONN_LIST();
      }
      DEBUG_PRINT_BUFFER("Message before encryption", (unsigned char*) buffer, buflen);

      if (sec_encrypt(&pb, buffer, buflen)) {
         ERROR((SGE_EVENT, MSG_SEC_MSGENCFAILED));
         cl_com_free_endpoint(&destination);
         DEXIT;
         return CL_RETVAL_SECURITY_SEND_FAILED;
      }

      DEBUG_PRINT_BUFFER("Message after encryption", (unsigned char*)pb.head_ptr, pb.bytes_used);
   }
   else if (uti_state_get_mewho() != QMASTER) {
      if (sec_set_connid(& buffer, &buflen)) {
         cl_com_free_endpoint(&destination);
         ERROR((SGE_EVENT, MSG_SEC_CONNIDSETFAILED));
         DEXIT;
         return CL_RETVAL_SECURITY_SEND_FAILED;
      }
   }
   cl_com_free_endpoint(&destination);

   i = cl_commlib_send_message(cl_handle, un_resolved_hostname,  component_name,  component_id, 
                                  ack_type, (cl_byte_t *)pb.head_ptr, pb.bytes_used ,
                                  mid,  response_mid,  tag , copy_data, wait_for_ack);
   clear_packbuffer(&pb);
   DEXIT;
   return i;
}
#else
int sec_send_message(
int synchron,
const char *tocomproc,
int toid,
const char *tohost,
int tag,
char *buffer,
int buflen,
u_long32 *mid,
int compressed 
) {
   int i = 0;
   sge_pack_buffer pb;

   DENTER(GDI_LAYER, "sec_send_message");

   /*
   ** every component has to negotiate its connection with qmaster
   */
   if (uti_state_get_mewho() != QMASTER) {
      if (sec_announce_connection(&gsd, tocomproc,tohost)) {
/*          ERROR((SGE_EVENT, MSG_SEC_ANNOUNCEFAILED)); */
         DPRINTF((MSG_SEC_ANNOUNCEFAILED));
         DEXIT;
         return SEC_ANNOUNCE_FAILED;
      }
   }   
      
   if (sec_set_encrypt(tag)) {
      if (uti_state_get_mewho() == QMASTER) {
         SEC_LOCK_CONN_LIST();
         if (sec_conn_list) {
            /*
            ** set the corresponding key and connection information
            ** in the connection list for commd triple 
            ** (tohost, tocomproc, toid)
            */
            if (sec_set_secdata(tohost,tocomproc,toid)) {
               SEC_UNLOCK_CONN_LIST();
               ERROR((SGE_EVENT, MSG_SEC_SETSECDATAFAILED));
               DEXIT;
               return SEC_SEND_FAILED;
            }
            SEC_UNLOCK_CONN_LIST();
         }
      }
      DEBUG_PRINT_BUFFER("Message before encryption", (unsigned char*) buffer, buflen);

      if (sec_encrypt(&pb, buffer, buflen)) {
         ERROR((SGE_EVENT, MSG_SEC_MSGENCFAILED));
         DEXIT;
         return SEC_SEND_FAILED;
      }

      DEBUG_PRINT_BUFFER("Message after encryption",
                               pb.head_ptr, pb.bytes_used);
   }
   else if (uti_state_get_mewho() != QMASTER) {
      if (sec_set_connid(&buffer, &buflen)) {
         ERROR((SGE_EVENT, MSG_SEC_CONNIDSETFAILED));
         DEXIT;
         return SEC_SEND_FAILED;
      }
   }

   i = send_message(synchron, tocomproc, toid, tohost, tag, 
                    pb.head_ptr, pb.bytes_used, mid, compressed);
   
   clear_packbuffer(&pb);
   
   DEXIT;
   return i;
}
#endif

/****** security/sec_lib/sec_receive_message() *******************************
*  NAME
*     sec_receive_message() -- CSP secured version of receive_message() 
*
*  SYNOPSIS
*     int sec_receive_message(char *fromcommproc, u_short *fromid, char 
*     *fromhost, int *tag, char **buffer, u_long32 *buflen, int synchron, 
*     u_short* compressed) 
*
*  FUNCTION
*     This function is used instead of the normal receive message of the
*     commlib. It handles an announce and decrypts the message if necessary.
*     The qmaster writes this connection to his connection list.
*
*  INPUTS
*     char *fromcommproc  - sender's commproc name 
*     u_short *fromid     - id of message
*     char *fromhost      - sender's hostname
*     int *tag            - tag of received message
*     char **buffer       - buffer contains the received message
*     u_long32 *buflen    - length of message 
*     int synchron        - transfer modus
*     u_short* compressed - use zlib compression
*
*  RESULT
*     int - 0 on success, -1 on failure
*
*  NOTES
*     MT-NOTE: sec_receive_message() is MT safe
*******************************************************************************/
#ifdef ENABLE_NGC
int sec_receive_message(cl_com_handle_t* cl_handle,char* un_resolved_hostname, char* component_name, unsigned long component_id, int synchron, unsigned long response_mid, cl_com_message_t** message, cl_com_endpoint_t** sender)
{
   int i;
   int tag = -1;
   DENTER(GDI_LAYER, "sec_receive_message");
   
   if ((i=cl_commlib_receive_message(cl_handle, un_resolved_hostname,  component_name,  component_id,  
                                     synchron, response_mid,  message,  sender) ) != CL_RETVAL_OK) {

      /*
      ** reset connect state to force reannouncement
      */
#if 0      
      if (message != NULL && *message != NULL /* && 
               (*message)->message_tag == TAG_GDI_REQUEST) { */
         DPRINTF(("<<<<< resetting connect state >>>>>\n"));
         sec_state_set_connect(0);
      }
#endif

#if 1
/* printf("\n\n===============> cl_commlib_receive_message: %d\n\n", i); */
      if (i == CL_RETVAL_SYNC_RECEIVE_TIMEOUT) {
         DPRINTF(("<<<<< resetting connect state >>>>>\n"));
         sec_state_set_connect(0);
      }
#endif      
      DEXIT;
      return i;
   }

   if (message != NULL && *message != NULL) {
      tag = (*message)->message_tag;
   } else {
      DEXIT;
      return CL_RETVAL_SECURITY_ANNOUNCE_FAILED;
   }
   if (sender == NULL || *sender == NULL) {
      DEXIT;
      return CL_RETVAL_SECURITY_ANNOUNCE_FAILED;
   }

   if (tag == TAG_SEC_ANNOUNCE) {

      DEBUG_PRINT_BUFFER("Received announce message", 
                         (unsigned char*) (*message)->message, (int)(*message)->message_length);

      if ( sec_handle_announce(cl_handle, *sender, 
                           (char*) (*message)->message, (*message)->message_length, (*message)->message_id) ) {

         ERROR((SGE_EVENT, MSG_SEC_HANDLEANNOUNCEFAILED_SSU, (*sender)->comp_host, (*sender)->comp_name, (*sender)->comp_id));
         DEXIT;
         return CL_RETVAL_SECURITY_ANNOUNCE_FAILED;
      }
   }
   else if (sec_set_encrypt(tag)) {
      u_long32 tmp_buf_len = (*message)->message_length;
      char* tmp_buf_pp = (char*) ((*message)->message);

      DEBUG_PRINT_BUFFER("Encrypted incoming message", 
                         (unsigned char*) (*message)->message, (int)(*message)->message_length);
      if (sec_decrypt(&tmp_buf_pp, &tmp_buf_len, *sender)) {
         (*message)->message = (cl_byte_t*)tmp_buf_pp;
         (*message)->message_length = tmp_buf_len;

         ERROR((SGE_EVENT, MSG_SEC_MSGDECFAILED_SSU, (*sender)->comp_host, (*sender)->comp_name, (*sender)->comp_id));
         if (tag != TAG_GDI_REQUEST) {
            if (sec_handle_announce(cl_handle, *sender,NULL,0, (*message)->message_id)) {
               ERROR((SGE_EVENT, MSG_SEC_HANDLEDECERRFAILED));
            }   
         }
         DEXIT;
         return CL_RETVAL_SECURITY_RECEIVE_FAILED;
      }
      (*message)->message = (cl_byte_t*)tmp_buf_pp;
      (*message)->message_length = tmp_buf_len;

      DEBUG_PRINT_BUFFER("Decrypted incoming message", (unsigned char*) (*message)->message, (int)(*message)->message_length);
   }
   else if (uti_state_get_mewho() == QMASTER) {
      u_long32 tmp_buf_len = (*message)->message_length;
      char* tmp_buf_pp = (char*) ((*message)->message);

      if (sec_get_connid(&tmp_buf_pp, &tmp_buf_len) ||
               sec_update_connlist(*sender)) {
         ERROR((SGE_EVENT, MSG_SEC_CONNIDGETFAILED));
         DEXIT;
         return CL_RETVAL_SECURITY_RECEIVE_FAILED;
      }
      (*message)->message = (cl_byte_t*)tmp_buf_pp;
      (*message)->message_length = tmp_buf_len;
   }

   DEXIT;
   return CL_RETVAL_OK;
}
#else
int sec_receive_message(char *fromcommproc, u_short *fromid, char *fromhost, 
                        int *tag, char **buffer, u_long32 *buflen, 
                        int synchron, u_short* compressed)
{
   int i;

   DENTER(GDI_LAYER, "sec_receive_message");

   if ((i=receive_message(fromcommproc, fromid, fromhost, tag, buffer, buflen,
                       synchron, compressed))) {
      DEXIT;
      return i;
   }

   if (*tag == TAG_SEC_ANNOUNCE) {
      if (sec_handle_announce(fromcommproc,*fromid,fromhost,*buffer,*buflen)) {
         ERROR((SGE_EVENT, MSG_SEC_HANDLEANNOUNCEFAILED_SSI,
                  fromhost, fromcommproc, *fromid));
         DEXIT;
         return SEC_ANNOUNCE_FAILED;
      }
   }
   else if (sec_set_encrypt(*tag)) {
      DEBUG_PRINT_BUFFER("Encrypted incoming message", 
                         (unsigned char*) (*buffer), (int)(*buflen));

      if (sec_decrypt(buffer,buflen,fromhost,fromcommproc,*fromid)) {
         ERROR((SGE_EVENT, MSG_SEC_MSGDECFAILED_SSI,
                  fromhost,fromcommproc,*fromid));
         if (sec_handle_announce(fromcommproc,*fromid,fromhost,NULL,0))
            ERROR((SGE_EVENT, MSG_SEC_HANDLEDECERRFAILED));
         DEXIT;
         return SEC_RECEIVE_FAILED;
      }

      DEBUG_PRINT_BUFFER("Decrypted incoming message", 
                         (unsigned char*) (*buffer), (int)(*buflen));
   }
   else if (uti_state_get_mewho() == QMASTER) {
      if (sec_get_connid(buffer,buflen) ||
               sec_update_connlist(fromhost,fromcommproc,*fromid)) {
         ERROR((SGE_EVENT, MSG_SEC_CONNIDGETFAILED));
         DEXIT;
         return SEC_RECEIVE_FAILED;
      }
   }

   DEXIT;
   return 0;
}
#endif


/****** security/sec_lib/sec_set_encrypt() ***********************************
*  NAME
*     sec_set_encrypt() -- return encryption mode for tag
*
*  SYNOPSIS
*     static int sec_set_encrypt(int tag) 
*
*  FUNCTION
*     This function decides within a single switch by means of the
*     message tag if a message is/should be encrypted. ATTENTION:
*     every new tag, which message is not to be encrypted, must
*     occur within this switch. Tags who are not there are encrypted
*     by default. 
*     (definition of tags <gridengine>/source/libs/gdi/sge_any_request.h)
*
*  INPUTS
*     int tag - message tag
*
*  RESULT
*     static int - 0 if the message is not encrypted, 
*                  1 if the message is encrypted 
*
*  NOTES
*     MT-NOTE: sec_set_encrypt() is MT safe
*******************************************************************************/
static int sec_set_encrypt(
int tag 
) {
   switch (tag) {
      case TAG_SEC_ANNOUNCE:
      case TAG_SEC_RESPOND:
      case TAG_SEC_ERROR: 
         return 0;

      default: 
         return 1; 
   }
}

/****** sec_lib/sec_files() ****************************************************
*  NAME
*     sec_files() -- read CSP related files  
*
*  SYNOPSIS
*     static int sec_files() 
*
*  FUNCTION
*     This function reads security related files from hard disk
*     (key- and certificate-file) and verifies the own certificate.
*     It's normally called from the following sec_init function.
*
*  INPUTS
*
*  RESULT
*     static int - 0 on success, -1 on failure
*
*  NOTES
*     MT-NOTE: sec_files() is not MT safe
*******************************************************************************/
static int sec_files()
{
   int     i = 0;
   FILE    *fp = NULL;

   DENTER(GDI_LAYER,"sec_files");

   /* 
   ** read Certificate from file
   */
   fp = fopen(cert_file, "r");
   if (fp == NULL) {
       ERROR((SGE_EVENT, MSG_SEC_CANTOPENCERTFILE_SS,
              cert_file, strerror(errno)));
       i = -1;
       goto error;
   }

   gsd.x509 = sec_PEM_read_X509(fp, NULL, NULL, PREDEFINED_PW);
   if (gsd.x509 == NULL) {
      sge_sec_error();
      i = -1;
      goto error;
   }
   fclose(fp);

   if (!sec_verify_certificate(gsd.x509)) {
      ERROR((SGE_EVENT, MSG_SEC_FAILEDVERIFYOWNCERT));
      sge_sec_error();
      i = -1;
      goto error;
   }

   /* 
   ** read private key from file
   */
   fp = fopen(key_file, "r");
   if (fp == NULL){
      ERROR((SGE_EVENT, MSG_SEC_CANTOPENKEYFILE_SS, key_file, strerror(errno)));
      i = -1;
      goto error;
   }
   gsd.private_key = sec_PEM_read_PrivateKey(fp, NULL, NULL, PREDEFINED_PW);
   fclose(fp);

   if (!gsd.private_key) {
      sge_sec_error();
      i = -1;
      goto error;
   }   

   DEXIT;
   return 0;
    
   error:
      if (fp) 
         fclose(fp);
      DEXIT;
      return i;
}

/****** sec_lib/sec_verify_certificate() ***************************************
*  NAME
*     sec_verify_certificate() -- verify a user certificates validity
*
*  SYNOPSIS
*     static int sec_verify_certificate(X509 *cert) 
*
*  FUNCTION
*     Verify the user's X509 certificate. The certificate must be valid for the 
*     time when it is used and must be signed from the corresponding CA.
*
*  INPUTS
*     X509 *cert - a X509 user certificate
*
*  RESULT
*     static int - 1 if certificate is valid, 0 if certificate is not valid. 
*
*  BUGS
*     Revocation of certificates is not yet supported.
*
*  NOTES
*     MT-NOTE: sec_verify_certificate() is MT safe
*******************************************************************************/
static int sec_verify_certificate(X509 *cert)
{
   X509_STORE *ctx = NULL;
   X509_STORE_CTX csc;
   int ret;
   int err;
   char *x509_name = NULL;

   DENTER(GDI_LAYER, "sec_verify_certificate");

   x509_name = sec_X509_NAME_oneline(sec_X509_get_subject_name(cert), NULL, 0);
   DPRINTF(("subject: %s\n", x509_name));
   free(x509_name);
   DPRINTF(("ca_cert_file: %s\n", ca_cert_file));
   ctx = sec_X509_STORE_new();
   sec_X509_STORE_load_locations(ctx, ca_cert_file, NULL);
   sec_X509_STORE_CTX_init(&csc, ctx, cert, NULL);
   if (getenv("SGE_CERT_DEBUG")) {
      sec_X509_STORE_set_verify_cb_func(&csc, sec_verify_callback);
   }   
   ret = sec_X509_verify_cert(&csc);
   err = sec_X509_STORE_CTX_get_error(&csc);
   sec_X509_STORE_free(ctx);
   sec_X509_STORE_CTX_cleanup(&csc);
   
   switch (err) {
      case X509_V_ERR_CERT_NOT_YET_VALID:
         ERROR((SGE_EVENT, MSG_SEC_CERTNOTYETVALID));
         break;

      case X509_V_ERR_CERT_HAS_EXPIRED:
         ERROR((SGE_EVENT, MSG_SEC_CERTEXPIRED));
         break;
   }      
   
   DEXIT;
   return ret;
}


#ifdef SEC_RECONNECT
/*
** NAME
**   sec_dump_connlist
**
** SYNOPSIS
**
**   static int sec_dump_connlist()
**
** DESCRIPTION
**   This function writes the connection list of qmaster to disk in
**   his spool directory when the master is going down. Since these 
**   secret data isn't encrypted the spool directory should be on
**   a local hard disk with only root permission.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
**
** NOTES
**    MT-NOTES: sec_dump_connlist() is MT safe
*/
static int sec_dump_connlist()
{
   FILE    *fp;

   if (uti_state_get_mewho() == QMASTER) {
      fp = fopen("connlist.dat", "w");
      if (fp) {
         SEC_LOCK_CONN_LIST();
         lDumpList(fp, sec_conn_list, 0);
         SEC_UNLOCK_CONN_LIST();
         fclose(fp);
      }
      else 
         return -1;

      fp = fopen("connid.dat","w");
      if (fp) {
         SEC_LOCK_CONNID_COUNTER();
         fprintf(fp, u32, sec_connid_counter);
         SEC_UNLOCK_CONNID_COUNTER();
         fclose(fp);
      }
      else {
         return -1;
      }
   }
   return 0;
}

/*
** NAME
**   sec_undump_connlist
**
** SYNOPSIS
**
**   static int sec_undump_connlist()
**
** DESCRIPTION
**   This function reads the connection list from disk when master
**   is starting up.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
**
** NOTES
**    MT-NOTES: sec_undump_connlist() is MT safe
*/
static int sec_undump_connlist()
{
   FILE    *fp;

   if (uti_state_get_mewho() == QMASTER) {
      fp = fopen("connlist.dat","r");
      if (fp){
         SEC_LOCK_CONN_LIST();
         sec_conn_list = lUndumpList(fp,NULL,NULL);
         SEC_UNLOCK_CONN_LIST();
         fclose(fp);
      } 
      else
         return -1;
         
      fp = fopen("connid.dat","r");
      if (fp){
         SEC_LOCK_CONNID_COUNTER();
         fscanf(fp, u32, &sec_connid_counter);
         SEC_UNLOCK_CONNID_COUNTER();
         fclose(fp);
      }
      else {
         return -1;
      }
   } 
   return 0;
}

#endif

/****** security/sec_lib/sec_announce_connection() **************************
*  NAME
*     sec_announce_connection() -- announce a new CSP connection
*
*  SYNOPSIS
*     static int sec_announce_connection(GlobalSecureData *gsd, const char 
*     *tocomproc, const char *tohost) 
*
*  FUNCTION
*     This function is used from clients to announce to the master and to
*     receive his responce. It should be called before the first 
*     communication and its aim is to negotiate a secret key which is
*     used to encrypt the further communication.
*
*  INPUTS
*     GlobalSecureData *gsd - security data structure
*     const char *tocomproc - commproc name of destination
*     const char *tohost    - destination host 
*
*  RESULT
*     static int - 0 on success, -1 on failure
*
*  NOTES
*     MT-NOTE: sec_announce_connection() is MT safe
*******************************************************************************/
static int sec_announce_connection(
cl_com_handle_t* cl_handle,
GlobalSecureData *gsd,
const cl_com_endpoint_t *sender

) {
   unsigned long dummymid;
   EVP_PKEY *master_key = NULL;
   int i;
   int ngc_error;
   X509 *x509_master=NULL;
   u_long32 x509_len, x509_master_len, chall_enc_len, enc_key_len, enc_key_mat_len,
            iv_len = 0, connid;
   unsigned char challenge[CHALL_LEN];
   unsigned char *x509_master_buf=NULL, 
                 *enc_challenge=NULL,
                 *enc_key = NULL, 
                 *enc_key_mat=NULL,
                 *iv = NULL,
                 *tmp = NULL,
                 *x509_buf = NULL;
   sge_pack_buffer pb;
   u_short compressed = 0;
   EVP_MD_CTX ctx;
   EVP_CIPHER_CTX ectx;
   u_long32 tmp_key_mat_len;
   cl_com_message_t* message = NULL; 
   cl_com_endpoint_t* local_sender = NULL; 
   DENTER(GDI_LAYER, "sec_announce_connection");

   /*
   ** try to reconnect
   */
   if (sec_state_get_connect()) {
/* debug_print_ASN1_UTCTIME("refresh_time: ", sec_state_get_refresh_time()); */
      if (sec_state_get_refresh_time() && (sec_X509_cmp_current_time(sec_state_get_refresh_time()) > 0)) {
         DPRINTF(("++++ Connection %d still valid\n", (int) sec_state_get_connid()));      
         DEXIT;      
         return 0;
      }   
      DPRINTF(("---- Connection needs sec_announce_connection\n"));
   }

   /* 
   ** send sec_announce
   */
   sec_state_set_seq_send(0);
   sec_state_set_seq_receive(0);

   /* 
   ** write x509 to buffer
   */
   x509_len = sec_i2d_X509(gsd->x509, NULL);
   tmp = x509_buf = (unsigned char *) malloc(x509_len);
   x509_len = sec_i2d_X509(gsd->x509, &tmp);
   
   if (!x509_len) {
      ERROR((SGE_EVENT, MSG_SEC_I2DX509FAILED));
      i=-1;
      goto error;
   }

   /* 
   ** build challenge and challenge mac
   */
   sec_RAND_pseudo_bytes(challenge, CHALL_LEN);

   /* 
   ** prepare packing buffer
   */
   if ((i=init_packbuffer(&pb, 1024, 0)) != PACK_SUCCESS) {
      ERROR((SGE_EVENT, MSG_SEC_INITPACKBUFFERFAILED_S, cull_pack_strerror(i)));
      goto error;
   }

   /* 
   ** write data to packing buffer
   */
   if ((i=sec_pack_announce(x509_len, x509_buf, challenge, &pb))) {
      ERROR((SGE_EVENT, MSG_SEC_PACKANNOUNCEFAILED));
      goto error;
   }

   /* 
   ** send buffer
   */
   DPRINTF(("send announce to=(%s:%s:%d)\n", sender->comp_host, 
               sender->comp_name, sender->comp_id));
 
   ngc_error = cl_commlib_send_message(cl_handle, (char*)sender->comp_host, (char*)sender->comp_name, sender->comp_id, 
                                       CL_MIH_MAT_ACK, (unsigned char*)pb.head_ptr, pb.bytes_used, 
                                       &dummymid, 0 ,TAG_SEC_ANNOUNCE,1,1);
   if (ngc_error != CL_RETVAL_OK) {
      i = -1;
      
      goto error;
   }

   /*
   ** free x509_buf and clear pack buffer
   */
   free(x509_buf);
   x509_buf = NULL;
   clear_packbuffer(&pb);
   
   /* 
   ** receive sec_response
   */
   ngc_error = cl_commlib_receive_message(cl_handle, (char*)sender->comp_host, (char*)sender->comp_name, sender->comp_id, 
                                          1, dummymid, &message, &local_sender);
   if (ngc_error != CL_RETVAL_OK) {
      ERROR((SGE_EVENT, MSG_SEC_RESPONSEFAILED_SUSUS,(char*)sender->comp_name, sender->comp_id,(char*)sender->comp_host ,(long int) 0, cl_get_error_text(ngc_error)));
      i = -1;
      goto error;
   }

   if((i = init_packbuffer_from_buffer(&pb, (char*) message->message, message->message_length , compressed)) != PACK_SUCCESS) {
      ERROR((SGE_EVENT, MSG_SEC_INITPACKBUFFERFAILED_S, cull_pack_strerror(i)));
      i = -1;
      goto error;
   }

   DPRINTF(("received announcement response from=(%s:%s:%d) tag=%d buflen=%d\n",
            local_sender->comp_host,local_sender->comp_name , local_sender->comp_id, 
            message->message_tag, message->message_length));

   /* 
   ** manage error or wrong tags
   */
   if (message->message_tag == TAG_SEC_ERROR){
      ERROR((SGE_EVENT, MSG_SEC_MASTERERROR));
      ERROR((SGE_EVENT, pb.cur_ptr));
      i=-1;
      goto error;
   }
   if (message->message_tag != TAG_SEC_RESPOND) {
      ERROR((SGE_EVENT, MSG_SEC_UNEXPECTEDTAG));
      i=-1;
      goto error;
   }

   message->message = NULL;

   /* 
   ** unpack data from packing buffer
   */
   i = sec_unpack_response(&x509_master_len, &x509_master_buf, 
                           &chall_enc_len, &enc_challenge,
                           &connid, 
                           &enc_key_len, &enc_key, 
                           &iv_len, &iv, 
                           &enc_key_mat_len, &enc_key_mat, &pb);
   if (i) {
      ERROR((SGE_EVENT, MSG_SEC_UNPACKRESPONSEFAILED));
      goto error;
   }

   DEBUG_PRINT_BUFFER("enc_key", enc_key, enc_key_len);
   DEBUG_PRINT_BUFFER("enc_key_mat", enc_key_mat, enc_key_mat_len);

   DPRINTF(("received assigned connid = %d\n", (int) connid));
   
   /* 
   ** read x509 certificate from buffer
   */
   tmp = x509_master_buf;
   x509_master = sec_d2i_X509(NULL, &tmp, x509_master_len);
   if (!x509_master) {
      ERROR((SGE_EVENT, MSG_SEC_MASTERCERTREADFAILED));
      sge_sec_error();
      i = -1;
      goto error;
   }
   /*
   ** free x509_master_buf
   */
   free(x509_master_buf);
   x509_master_buf = NULL;


   if (!sec_verify_certificate(x509_master)) {
      sge_sec_error();
      i = -1;
      goto error;
   }
   DPRINTF(("Master certificate is ok\n"));

   /*
   ** extract public key 
   */
   master_key = sec_X509_get_pubkey(x509_master);
   
   if(!master_key){
      ERROR((SGE_EVENT, MSG_SEC_MASTERGETPUBKEYFAILED));
      sge_sec_error();
      i=-1;
      goto error;
   }

   /* 
   ** decrypt challenge with master public key
   */
   sec_EVP_VerifyInit(&ctx, gsd->digest);
   sec_EVP_VerifyUpdate(&ctx, challenge, CHALL_LEN);
   if (!sec_EVP_VerifyFinal(&ctx, enc_challenge, chall_enc_len, master_key)) {
      ERROR((SGE_EVENT, MSG_SEC_MASTERBADCHALLENGE));
      i = -1;
      goto error;
   }   
  
   /* ensure key mat buffer is allocated */  
   if (sec_alloc_key_mat()) {
      ERROR((SGE_EVENT, MSG_MEMORY_MALLOCFAILED));
      i = -1;
      goto error;
   }

   /*
   ** decrypt secret key
   */
   if (!sec_EVP_OpenInit(&ectx, gsd->cipher, enc_key, enc_key_len, iv, gsd->private_key)) {
      ERROR((SGE_EVENT, MSG_SEC_EVPOPENINITFAILED));
      sec_EVP_CIPHER_CTX_cleanup(&ectx);
      sge_sec_error();
      i=-1;
      goto error;
   }
   if (!sec_EVP_OpenUpdate(&ectx, sec_state_get_key_mat(), (int*) &tmp_key_mat_len, enc_key_mat, (int) enc_key_mat_len)) {
      ERROR((SGE_EVENT, MSG_SEC_EVPOPENUPDATEFAILED));
      sec_EVP_CIPHER_CTX_cleanup(&ectx);
      sge_sec_error();
      i = -1;
      goto error;
   }
   sec_state_set_key_mat_len(tmp_key_mat_len);

   sec_EVP_OpenFinal(&ectx, sec_state_get_key_mat(), (int*) &tmp_key_mat_len);
   sec_state_set_key_mat_len(tmp_key_mat_len);
   sec_EVP_CIPHER_CTX_cleanup(&ectx);
   /* FIXME: problem in EVP_* lib for stream ciphers */
   sec_state_set_key_mat_len(enc_key_mat_len);

   DEBUG_PRINT_BUFFER("Received key material", sec_state_get_key_mat(), sec_state_get_key_mat_len());
   
   /*
   ** set own connid to assigned connid
   ** set refresh_time to current time + 60 * VALID_MINUTES
   ** set connect flag
   */
   sec_state_set_connect(1);
   sec_state_set_connid(connid);
   sec_state_set_refresh_time(sec_X509_gmtime_adj(sec_state_get_refresh_time(), 
                                       (long) 60*VALID_MINUTES));

   i=0;

   error:
   clear_packbuffer(&pb);
   cl_com_free_message(&message);
   cl_com_free_endpoint(&local_sender);
   if (x509_buf)
      free(x509_buf);
   if (x509_master_buf)
      free(x509_master_buf);
   if (x509_master)
      sec_X509_free(x509_master);
   if (enc_challenge) 
      free(enc_challenge);
   if (enc_key) 
      free(enc_key);
   if (iv) 
      free(iv);
   if (enc_key_mat) 
      free(enc_key_mat);
   if (master_key)
      sec_EVP_PKEY_free(master_key);

   DEXIT;
   return i;
}


/****** sec_lib/sec_respond_announce() *****************************************
*  NAME
*     sec_respond_announce() -- respond to CSP announce of client 
*
*  SYNOPSIS
*     static int sec_respond_announce(char *commproc, u_short id, char *host, 
*     char *buffer, u_long32 buflen) 
*
*  FUNCTION
*     This function handles the announce of a client and sends a responce
*     to it which includes the encrypted secret key.
*
*  INPUTS
*     char *commproc  - the program name of the source of the message
*     u_short id      - the id of the communication
*     char *host      - the host name of the sender
*     char *buffer    - this buffer contains the incoming message
*     u_long32 buflen - the length of the message
*
*  RESULT
*     static int - 0 on success, -1 on failure 
*
*  NOTES
*     MT-NOTE: sec_respond_announce() is MT safe
*******************************************************************************/
static int sec_respond_announce(
cl_com_handle_t* cl_handle, 
const cl_com_endpoint_t *sender,
char *buffer, 
u_long32 buflen, 
u_long32 response_id)
{
   EVP_PKEY *public_key[1];
   u_long32 i = 0, x509_len = 0, enc_key_mat_len = 0;
   u_long32 chall_enc_len = 0;
   unsigned char *x509_buf = NULL, *tmp = NULL;
   u_char *enc_key_mat=NULL, *challenge=NULL, *enc_challenge=NULL;
   X509 *x509 = NULL;
   sge_pack_buffer pb, pb_respond;
   unsigned long dummymid;
   int public_key_size;
   unsigned char iv[EVP_MAX_IV_LENGTH];
   unsigned char *ekey[1];
   int ekeylen;
   EVP_CIPHER_CTX ectx;
   EVP_MD_CTX ctx;
   char uniqueIdentifier[BUFSIZ];
   int ngc_error;
   lList *alp = NULL;

   DENTER(GDI_LAYER, "sec_respond_announce");

   /* 
   ** prepare packbuffer pb for unpacking message
   */
   if ((i=init_packbuffer_from_buffer(&pb, buffer, buflen, 0)) != PACK_SUCCESS) {
      ERROR((SGE_EVENT, MSG_SEC_INITPACKBUFFERFAILED_S, cull_pack_strerror(i)));
      goto error;
   }

   /* 
   ** prepare packbuffer for sending message
   */
   if ((i=init_packbuffer(&pb_respond, 1024, 0)) != PACK_SUCCESS) {
      ERROR((SGE_EVENT, MSG_SEC_INITPACKBUFFERFAILED_S, cull_pack_strerror(i)));
      sec_send_err(sender, &pb_respond, SGE_EVENT, response_id, &alp);
      answer_list_output (&alp);
      goto error;
   }

   /* 
   ** read announce from packing buffer
   */
   if (sec_unpack_announce(&x509_len, &x509_buf, &challenge, &pb)) {
      ERROR((SGE_EVENT, MSG_SEC_UNPACKANNOUNCEFAILED));
      sec_send_err(sender, &pb_respond, MSG_SEC_UNPACKANNOUNCEFAILED,
                   response_id, &alp);
      answer_list_output (&alp);
      goto error;
   }

   DEBUG_PRINT_BUFFER("After sec_unpack_announce", x509_buf, x509_len);

   /* 
   ** read x509 certificate
   */
   tmp = x509_buf;
   x509 = sec_d2i_X509(NULL, &tmp, x509_len);
   free(x509_buf);
   x509_buf = NULL;

   if (!x509){
      ERROR((SGE_EVENT, MSG_SEC_CLIENTCERTREADFAILED));
      sge_sec_error();
      sec_send_err(sender, &pb_respond, MSG_SEC_CLIENTCERTREADFAILED,
                   response_id, &alp);
      i = -1;
      answer_list_output (&alp);
      goto error;
   }

   if (!sec_verify_certificate(x509)) {
      ERROR((SGE_EVENT, MSG_SEC_CLIENTCERTVERIFYFAILED));
      sge_sec_error();
      sec_send_err(sender, &pb_respond, MSG_SEC_CLIENTCERTVERIFYFAILED,
                   response_id, &alp);
      i = -1;
      answer_list_output (&alp);
      goto error;
   }
   DPRINTF(("Client certificate is ok\n"));
      
   /*
   ** reset uniqueIdentifier and get uniqueIdentifier from client cert
   */
   memset(uniqueIdentifier, '\0', BUFSIZ);
   if (sec_X509_NAME_get_text_by_OBJ(sec_X509_get_subject_name(x509), 
      sec_OBJ_nid2obj(NID_userId), uniqueIdentifier, 
                  sizeof(uniqueIdentifier))) {
      DPRINTF(("UID: %s\n", uniqueIdentifier));
   }   
   
   /*
   ** extract public key 
   */
   public_key[0] = sec_X509_get_pubkey(x509);

   sec_X509_free(x509);
   x509 = NULL;
   
   if(!public_key[0]){
      ERROR((SGE_EVENT, MSG_SEC_CLIENTGETPUBKEYFAILED));
      sge_sec_error();
      sec_send_err(sender,&pb_respond, MSG_SEC_CLIENTGETPUBKEYFAILED,
                   response_id, &alp);
      i=-1;
      answer_list_output (&alp);
      goto error;
   }
   public_key_size = sec_EVP_PKEY_size(public_key[0]);

   /* 
   ** malloc several buffers
   */
   ekey[0] = (u_char *) malloc(public_key_size);
   enc_key_mat = (u_char *) malloc(public_key_size);
   enc_challenge = (u_char *) malloc(public_key_size);

   if (!enc_key_mat ||  !enc_challenge || sec_alloc_key_mat()) {
      ERROR((SGE_EVENT, MSG_MEMORY_MALLOCFAILED));
      sec_send_err(sender,&pb_respond, MSG_MEMORY_MALLOCFAILED, response_id,
                   &alp);
      i=-1;
      answer_list_output (&alp);
      goto error;
   }
   
   /* 
   ** set connection ID
   */
   SEC_LOCK_CONNID_COUNTER();
   sec_state_set_connid(sec_connid_counter);
   SEC_UNLOCK_CONNID_COUNTER();

   /* 
   ** write x509 certificate to buffer
   */
   x509_len = sec_i2d_X509(gsd.x509, NULL);
   tmp = x509_buf = (u_char *) malloc(x509_len);
   x509_len = sec_i2d_X509(gsd.x509, &tmp);

   /*
   ** sign the challenge with master's private key
   */
   sec_EVP_SignInit(&ctx, gsd.digest);
   sec_EVP_SignUpdate(&ctx, challenge, CHALL_LEN);
   if (!sec_EVP_SignFinal(&ctx, enc_challenge, (unsigned int*) &chall_enc_len, gsd.private_key)) {
      ERROR((SGE_EVENT, MSG_SEC_ENCRYPTCHALLENGEFAILED));
      sge_sec_error();
      sec_send_err(sender, &pb_respond, MSG_SEC_ENCRYPTCHALLENGEFAILED,
                   response_id, &alp);
      i = -1;
      answer_list_output (&alp);
      goto error;
   }   

   /* 
   ** make key material
   */
   sec_RAND_pseudo_bytes(sec_state_get_key_mat(), sec_state_get_key_mat_len());
   DEBUG_PRINT_BUFFER("Sent key material", sec_state_get_key_mat(), sec_state_get_key_mat_len());
   DEBUG_PRINT_BUFFER("Sent enc_challenge", enc_challenge, chall_enc_len);
   
   /*
   ** seal secret key with client's public key
   */
   memset(iv, '\0', sizeof(iv));
   if (!sec_EVP_SealInit(&ectx, gsd.cipher, ekey, &ekeylen, iv, public_key, 1)) {
      ERROR((SGE_EVENT, MSG_SEC_SEALINITFAILED));;
      sec_EVP_CIPHER_CTX_cleanup(&ectx);
      sge_sec_error();
      sec_send_err(sender,&pb_respond, MSG_SEC_SEALINITFAILED, response_id,
                   &alp);
      i = -1;
      answer_list_output (&alp);
      goto error;
   }
   if (!sec_EVP_EncryptUpdate(&ectx, enc_key_mat, (int *)&enc_key_mat_len, sec_state_get_key_mat(), sec_state_get_key_mat_len())) {
      ERROR((SGE_EVENT, MSG_SEC_ENCRYPTKEYFAILED));
      sec_EVP_CIPHER_CTX_cleanup(&ectx);
      sge_sec_error();
      sec_send_err(sender,&pb_respond, MSG_SEC_ENCRYPTKEYFAILED, response_id,
                   &alp);
      i = -1;
      answer_list_output (&alp);
      goto error;
   }
   sec_EVP_SealFinal(&ectx, enc_key_mat, (int*) &enc_key_mat_len);

   enc_key_mat_len = sec_state_get_key_mat_len();

   DEBUG_PRINT_BUFFER("Sent ekey[0]", ekey[0], ekeylen);
   DEBUG_PRINT_BUFFER("Sent enc_key_mat", enc_key_mat, enc_key_mat_len);

   /* 
   ** write response data to packing buffer
   */
   i=sec_pack_response(x509_len, x509_buf, chall_enc_len, enc_challenge,
                       sec_state_get_connid(), 
                       ekeylen, ekey[0], 
                       sec_EVP_CIPHER_CTX_iv_length(&ectx), iv,
                       enc_key_mat_len, enc_key_mat,
                       &pb_respond);

   sec_EVP_CIPHER_CTX_cleanup(&ectx);

   if (i) {
      ERROR((SGE_EVENT, MSG_SEC_PACKRESPONSEFAILED));
      sec_send_err(sender, &pb_respond, MSG_SEC_PACKRESPONSEFAILED, response_id,
                   &alp);
      answer_list_output (&alp);
      goto error;   
   }

   /* 
   ** insert connection to Connection List
   */
   if (sec_insert_conn2list(sec_state_get_connid(), sender, uniqueIdentifier)) {
      ERROR((SGE_EVENT, MSG_SEC_INSERTCONNECTIONFAILED));
      sec_send_err(sender, &pb_respond, MSG_SEC_INSERTCONNECTIONFAILED,
                   response_id, &alp);
      answer_list_output (&alp);
      goto error;
   }

   /* 
   ** send response
   */
   DPRINTF(("send response to=(%s:%s:%d)\n", sender->comp_host, sender->comp_name, sender->comp_id));
  
   ngc_error =  cl_commlib_send_message(cl_handle, 
                                        sender->comp_host, sender->comp_name, sender->comp_id,
                                        CL_MIH_MAT_NAK,
                                        (unsigned char*)pb_respond.head_ptr, pb_respond.bytes_used,
                                        &dummymid,response_id,TAG_SEC_RESPOND,
                                        1,0 );


   if (ngc_error != CL_RETVAL_OK) {
      ERROR((SGE_EVENT, MSG_SEC_SENDRESPONSEFAILED_SUS, sender->comp_name, sender->comp_id, sender->comp_host));
      i = -1;
      goto error;
   }

   i = 0;

   error:
      clear_packbuffer(&pb_respond);
      if (x509_buf) 
         free(x509_buf);
      if (challenge) 
         free(challenge);
      if (enc_challenge) 
         free(enc_challenge);
      if (enc_key_mat) 
         free(enc_key_mat);
      if (ekey[0])
         free(ekey[0]);
      if (x509) 
         sec_X509_free(x509);
      if (public_key[0]) 
         sec_EVP_PKEY_free(public_key[0]);

      DEXIT;   
      return i;
}


/****** sec_lib/sec_encrypt() **************************************************
*  NAME
*     sec_encrypt() -- encrypt a message buffer 
*
*  SYNOPSIS
*     static int sec_encrypt(sge_pack_buffer *pb, const char *inbuf, int 
*     inbuflen) 
*
*  FUNCTION
*     This function encrypts a message, calculates the MAC.
*
*  INPUTS
*     sge_pack_buffer *pb - packbuffer that contains the encrypted message
*     const char *inbuf   - unencrypted message
*     int inbuflen        - length of unencrypted message 
*
*  RESULT
*     static int - 0 on success, -1 on failure
*
*  NOTES
*     MT-NOTE: sec_encrypt() is MT safe
*******************************************************************************/
static int sec_encrypt(
sge_pack_buffer *pb,
const char *inbuf,
const int inbuflen 
) {
   int i = 0;
   u_char seq_str[INTSIZE];
   u_long32 seq_no;
   EVP_CIPHER_CTX ctx;
   unsigned char iv[EVP_MAX_IV_LENGTH];
   EVP_MD_CTX mdctx;
   unsigned char md_value[EVP_MAX_MD_SIZE];
   unsigned int md_len;
   unsigned int enc_md_len;
   unsigned int outlen = 0;
   char *outbuf = NULL;
   
   DENTER(GDI_LAYER,"sec_encrypt");   

   seq_no = sec_state_get_seq_send();
   memcpy(seq_str, &seq_no, INTSIZE);

   /*
   ** create digest
   */
   sec_EVP_DigestInit(&mdctx, gsd.digest);
   sec_EVP_DigestUpdate(&mdctx, sec_state_get_key_mat(), sec_state_get_key_mat_len());
   sec_EVP_DigestUpdate(&mdctx, seq_str, INTSIZE);
   sec_EVP_DigestUpdate(&mdctx, inbuf, inbuflen);
   sec_EVP_DigestFinal(&mdctx, md_value, &md_len);
   
   /*
   ** malloc outbuf 
   */
   if (inbuflen > 0) {
      outbuf = malloc(inbuflen + sec_EVP_CIPHER_block_size(gsd.cipher));
      if (!outbuf) {
         ERROR((SGE_EVENT, MSG_MEMORY_MALLOCFAILED));
         i=-1;
         goto error;
      }
   }

   /*
   ** encrypt digest and message
   */
   
   memset(iv, '\0', sizeof(iv));
   sec_EVP_EncryptInit(&ctx, gsd.cipher, sec_state_get_key_mat(), iv);
   if (!sec_EVP_EncryptUpdate(&ctx, md_value, (int*) &enc_md_len, md_value, md_len)) {
      ERROR((SGE_EVENT, MSG_SEC_ENCRYPTMACFAILED));
      sge_sec_error();
      i=-1;
      goto error;
   }
   if (!sec_EVP_EncryptFinal(&ctx, md_value, (int*) &enc_md_len)) {
      ERROR((SGE_EVENT, MSG_SEC_ENCRYPTMACFAILED));
      sge_sec_error();
      i=-1;
      goto error;
   }
   sec_EVP_CIPHER_CTX_cleanup(&ctx);


   if (inbuflen > 0) {  
      memset(iv, '\0', sizeof(iv));
      sec_EVP_EncryptInit(&ctx, gsd.cipher, sec_state_get_key_mat(), iv);
      if (!sec_EVP_EncryptUpdate(&ctx, (unsigned char *) outbuf, (int*)&outlen,
                              (unsigned char*)inbuf, inbuflen)) {
         ERROR((SGE_EVENT, MSG_SEC_ENCRYPTMSGFAILED));
         sge_sec_error();
         i=-1;
         goto error;
      }
      if (!sec_EVP_EncryptFinal(&ctx, (unsigned char*)outbuf, (int*) &outlen)) {
         ERROR((SGE_EVENT, MSG_SEC_ENCRYPTMSGFAILED));
         sge_sec_error();
         i=-1;
         goto error;
      }
      sec_EVP_CIPHER_CTX_cleanup(&ctx);
      if (!outlen)
         outlen = inbuflen;
   }
   /*
   ** initialize packbuffer
   */
   if ((i=init_packbuffer(pb, md_len + outlen, 0)) != PACK_SUCCESS) {
      ERROR((SGE_EVENT, MSG_SEC_INITPACKBUFFERFAILED_S, cull_pack_strerror(i)));
      goto error;
   }

   /*
   ** pack the information into one buffer
   */
   i = sec_pack_message(pb, sec_state_get_connid(), md_len, md_value, 
                           (unsigned char *)outbuf, outlen);
   
   if (i) {
      ERROR((SGE_EVENT, MSG_SEC_PACKMSGFAILED));
      goto error;
   }

   sec_state_set_seq_send(INC32(sec_state_get_seq_send()));

   SEC_LOCK_CONN_LIST();
   if (sec_conn_list) {
      lListElem *element=NULL;
      
      element = lGetElemUlong(sec_conn_list, SEC_ConnectionID, sec_state_get_connid());
      if (!element) {
         SEC_UNLOCK_CONN_LIST();
         ERROR((SGE_EVENT, MSG_SEC_CONNECTIONNOENTRY));
         i = -1;
         goto error;
      }
      lSetUlong(element, SEC_SeqNoSend, sec_state_get_seq_send());
   }   
   SEC_UNLOCK_CONN_LIST();

   i=0;

   error:
      sec_EVP_CIPHER_CTX_cleanup(&ctx);
      if (outbuf)
         free(outbuf);
      DEXIT;
      return i;
}

/****** security/sec_lib/sec_handle_announce() *******************************
*  NAME
*     sec_handle_announce() -- handle the announcement of a new CSP connection
*
*  SYNOPSIS
*     static int sec_handle_announce(char *commproc, u_short id, char *host, 
*     char *buffer, u_long32 buflen) 
*
*  FUNCTION
*     This function handles an incoming message with tag TAG_SEC_ANNOUNCE.
*     If I am the master I send a responce to the client, if not I make
*     an announce to the master.
*
*  INPUTS
*     char *commproc  - program name of the destination of responce
*     u_short id      - id of communication
*     char *host      - destination host
*     char *buffer    - buffer contains announce
*     u_long32 buflen - lenght of announce
*
*  RESULT
*     static int - 0 on success, -1 on failure
*
*  NOTES
*     MT-NOTE: sec_handle_announce() is MT safe
*******************************************************************************/
static int sec_handle_announce(cl_com_handle_t* cl_handle, const cl_com_endpoint_t *sender, char *buffer, u_long32 buflen, unsigned long response_id)
{
   int i;
   unsigned long mid;
   SGE_STRUCT_STAT file_info;
   int ngc_error;

   DENTER(GDI_LAYER, "sec_handle_announce");

   if (uti_state_get_mewho() == QMASTER) {
      if (buffer && buflen) {
         /* 
         ** someone wants to announce to master
         */
         if (sec_respond_announce(cl_handle, sender, buffer,buflen,response_id )) {
            ERROR((SGE_EVENT, MSG_SEC_RESPONSEFAILED_SUS, sender->comp_name, sender->comp_id, sender->comp_host));
            i = -1;
            goto error;
         }
      } else {
         unsigned char dummy_buffer[] = "dummy buffer";
         int dummy_buffer_len = sizeof(dummy_buffer);
         /* 
         ** a sec_decrypt error has occured
         */
         ngc_error = cl_commlib_send_message(cl_handle, sender->comp_host, sender->comp_name, sender->comp_id,
                                             CL_MIH_MAT_NAK,
                                             dummy_buffer, dummy_buffer_len + 1,
                                             &mid, response_id, TAG_SEC_ANNOUNCE,
                                             1, 0);

         if (ngc_error != CL_RETVAL_OK) {
            i = -1;
            goto error;
         }
      }
   } else if (gsd.is_daemon) {
      sec_state_set_connect(0);
   } else {
/*       printf("You should reconnect - please try command again!\n"); */
      sec_state_set_connect(0);

      if (SGE_STAT(reconnect_file,&file_info) < 0) {
         i = 0;
         goto error;
      }
      if (remove(reconnect_file)) {
         ERROR((SGE_EVENT, MSG_SEC_RMRECONNECTFAILED_S,
                  reconnect_file));
         i = -1;
         goto error;
      }
   }
            
   i = 0;
   
   error:
      DEXIT;
      return i;
}

/****** security/sec_lib/sec_decrypt() **************************************
*  NAME
*     sec_decrypt() -- decrypt a message 
*
*  SYNOPSIS
*     static int sec_decrypt(char **buffer, u_long32 *buflen, char *host, char 
*     *commproc, int id) 
*
*  FUNCTION
*     This function decrypts a message from and to buffer and checks the MAC. 
*
*  INPUTS
*     char **buffer    - message to decrypt and message after decryption
*     u_long32 *buflen - message length before and after decryption
*     char *host       - name of host
*     char *commproc   - program name
*     int id           - id of connection
*
*  RESULT
*     static int - 0 on success, -1 on failure
*
*  NOTES 
*     MT-NOTE: sec_decrypt() is MT safe
*******************************************************************************/
static int sec_decrypt(char **buffer, u_long32 *buflen, const cl_com_endpoint_t *sender)
{
   int i;
   u_char *enc_msg = NULL;
   u_char *enc_mac = NULL;
   u_long32 connid, enc_msg_len, enc_mac_len;
   u_long32 in_buf_len;
   lListElem *element=NULL;
   sge_pack_buffer pb;
   EVP_CIPHER_CTX ctx;
   unsigned char iv[EVP_MAX_IV_LENGTH];
   unsigned char md_value[EVP_MAX_MD_SIZE];
   unsigned int md_len = 0;
   unsigned int outlen = 0;

   DENTER(GDI_LAYER, "sec_decrypt");

   /*
   ** initialize packbuffer
   */
   if ((i=init_packbuffer_from_buffer(&pb, *buffer, *buflen, 0))) {
      ERROR((SGE_EVENT, MSG_SEC_INITPBFAILED));
      goto error;
   }
   *buffer = NULL;
   in_buf_len = *buflen;
   *buflen = 0;
   

   /* 
   ** unpack data from packing buffer
   */
   i = sec_unpack_message(&pb, &connid, &enc_mac_len, &enc_mac, 
                          &enc_msg_len, &enc_msg);
   if (i) {
      ERROR((SGE_EVENT, MSG_SEC_UNPACKMSGFAILED));
      goto error;
   }

   SEC_LOCK_CONN_LIST();
   if (sec_conn_list) {
      /*
      ** for debugging
      */
      if (getenv("SGE_CONNLIST_DEBUG")) {
         lListElem *el;
         ASN1_UTCTIME *utctime = sec_ASN1_UTCTIME_new();
         for_each(el, sec_conn_list) {
            const char *dtime = lGetString(el, SEC_ExpiryDate);
            utctime = sec_d2i_ASN1_UTCTIME(&utctime, (unsigned char**) &dtime, 
                                             strlen(dtime));
            debug_print_ASN1_UTCTIME("Valid Time: ", utctime);
            printf("Connection: %d (%s, %s, %d)\n", 
                      (int) lGetUlong(el, SEC_ConnectionID), 
                      lGetHost(el, SEC_Host),
                      lGetString(el, SEC_Commproc),
                      lGetInt(el, SEC_Id));
         }
         sec_ASN1_UTCTIME_free(utctime);
      }
   
      /* 
      ** search list element for this connection
      */
      element = lGetElemUlong(sec_conn_list, SEC_ConnectionID, connid);
      if (!element) {
         SEC_UNLOCK_CONN_LIST();
         ERROR((SGE_EVENT, MSG_SEC_NOCONN_I, (int) connid));
         i = -1;
         goto error;
      }
   
      /* 
      ** get data from connection list
      */
      sec_list2keymat(element);
      sec_state_set_unique_identifier(lGetString(element, SEC_UniqueIdentifier));
      sec_state_set_seq_receive(lGetUlong(element, SEC_SeqNoReceive));
      sec_state_set_connid(connid);

      /* unfortunately we may not unlock sec_conn_list_mutex here 
         because 'element' is still in use and refers to a sec_conn_list entry */
   }
   else {
      if (sec_state_get_connid() != connid) {
         SEC_UNLOCK_CONN_LIST();
         ERROR((SGE_EVENT, MSG_SEC_CONNIDSHOULDBE_II,
                  (int) connid, (int) sec_state_get_connid()));
         i = -1;
         goto error;
      }
   }

   /*
   ** malloc *buffer 
   */
   *buffer = malloc(in_buf_len + sec_EVP_CIPHER_block_size(gsd.cipher));
   if (!*buffer) {
      SEC_UNLOCK_CONN_LIST();
      ERROR((SGE_EVENT, MSG_MEMORY_MALLOCFAILED));
      i = -1;
      goto error;
   }

   /*
   ** malloc key_mat buffer
   */
   if (sec_alloc_key_mat()) {
      SEC_UNLOCK_CONN_LIST();
      ERROR((SGE_EVENT, MSG_MEMORY_MALLOCFAILED));
      i = -1;
      goto error;
   }

   /*
   ** decrypt digest and message
   */
   memset(iv, '\0', sizeof(iv));
   if (!sec_EVP_DecryptInit(&ctx, gsd.cipher, sec_state_get_key_mat(), iv)) {
      SEC_UNLOCK_CONN_LIST();
      ERROR((SGE_EVENT, MSG_SEC_DECMACFAILED));
      sec_EVP_CIPHER_CTX_cleanup(&ctx);
      sge_sec_error();
      i = -1;
      goto error;
   }

   if (!sec_EVP_DecryptUpdate(&ctx, md_value, (int*) &md_len, 
                           enc_mac, enc_mac_len)) {
      SEC_UNLOCK_CONN_LIST();
      ERROR((SGE_EVENT, MSG_SEC_DECMACFAILED));
      sec_EVP_CIPHER_CTX_cleanup(&ctx);
      sge_sec_error();
      i = -1;
      goto error;
   }
   if (!sec_EVP_DecryptFinal(&ctx, md_value, (int*) &md_len)) {
      SEC_UNLOCK_CONN_LIST();
      ERROR((SGE_EVENT, MSG_SEC_DECMACFAILED));
      sec_EVP_CIPHER_CTX_cleanup(&ctx);
      sge_sec_error();
      i = -1;
      goto error;
   }
   sec_EVP_CIPHER_CTX_cleanup(&ctx);
   if (!md_len)
      md_len = enc_mac_len;   


   memset(iv, '\0', sizeof(iv));
   if (!sec_EVP_DecryptInit(&ctx, gsd.cipher, sec_state_get_key_mat(), iv)) {
      SEC_UNLOCK_CONN_LIST();
      ERROR((SGE_EVENT, MSG_SEC_DECMACFAILED));
      sec_EVP_CIPHER_CTX_cleanup(&ctx);
      sge_sec_error();
      i = -1;
      goto error;
   }
   if (!sec_EVP_DecryptUpdate(&ctx, (unsigned char*) *buffer, (int*) &outlen,
                           (unsigned char*)enc_msg, enc_msg_len)) {
      SEC_UNLOCK_CONN_LIST();
      ERROR((SGE_EVENT, MSG_SEC_MSGDECFAILED));
      sec_EVP_CIPHER_CTX_cleanup(&ctx);
      sge_sec_error();
      i = -1;
      goto error;
   }
   if (!sec_EVP_DecryptFinal(&ctx, (unsigned char*) *buffer, (int*)&outlen)) {
      SEC_UNLOCK_CONN_LIST();
      ERROR((SGE_EVENT, MSG_SEC_MSGDECFAILED));
      sec_EVP_CIPHER_CTX_cleanup(&ctx);
      sge_sec_error();
      i = -1;
      goto error;
   }
   sec_EVP_CIPHER_CTX_cleanup(&ctx);
   if (!outlen)
      outlen = enc_msg_len;

   /*
   ** FIXME: check of mac ????
   */

   /* 
   ** increment sequence number and write it to connection list   
   */
   sec_state_set_seq_receive(INC32(sec_state_get_seq_receive()));
   if(sec_conn_list) {
      lSetHost(element, SEC_Host, sender->comp_host);
      lSetString(element, SEC_Commproc, sender->comp_name);
      lSetInt(element, SEC_Id, sender->comp_id);
      lSetUlong(element, SEC_SeqNoReceive, sec_state_get_seq_receive());
   }

   /* uff .... now sec_conn_list_mutex can finally be released */
   SEC_UNLOCK_CONN_LIST();

   /* 
   ** set *buflen                  
   */
   *buflen = outlen;

   i = 0;

   error:
      clear_packbuffer(&pb);
      if (enc_mac)
         free(enc_mac);
      if (enc_msg)
         free(enc_msg);
      DEXIT;
      return i;
}

#ifdef SEC_RECONNECT
/****** security/sec_lib/sec_reconnect() *************************************
*  NAME
*     sec_reconnect() -- read CSP client reconnect data from file 
*
*  SYNOPSIS
*     static int sec_reconnect() 
*
*  FUNCTION
*     This function reads the security related data from disk and decrypts
*     it for clients.
*
*  INPUTS
*
*  RESULT
*     static int - 0 on success, -1 on failure
*
*  NOTES
*     MT-NOTE: sec_reconnect() is MT safe
*******************************************************************************/
static int sec_reconnect()
{
   FILE *fp=NULL;
   int i, c, nbytes;
   sge_pack_buffer pb;
   SGE_STRUCT_STAT file_info;
   char *ptr;
   u_char *time_str = NULL;
   u_long32 len, tmp_connid, tmp_seq_send, tmp_seq_receive;
   BIO *bio = NULL;

   DENTER(GDI_LAYER,"sec_reconnect");

   if (SGE_STAT(reconnect_file, &file_info) < 0) {
      i = -1;
      goto error;
   }

   /* prepare packing buffer                                       */
   if ((i=init_packbuffer(&pb, file_info.st_size, 0))) {
      ERROR((SGE_EVENT, MSG_SEC_INITPBFAILED));
      goto error;
   }
   /* 
   ** read encrypted buffer from file
   */
   bio = sec_BIO_new_file(reconnect_file, "rb");
   i = sec_BIO_read(bio, pb.head_ptr, file_info.st_size);
   if (!i) {
      ERROR((SGE_EVENT, MSG_SEC_CANTREAD));
      i = -1;
      goto error;
   }
   sec_BIO_free(bio);

   /* decrypt data with own rsa privat key            */
   nbytes = sec_RSA_private_decrypt((SGE_OFF_T)file_info.st_size, 
                                (u_char*) pb.cur_ptr, (u_char*) pb.cur_ptr, 
                                 gsd.private_key->pkey.rsa, RSA_PKCS1_PADDING);
   if (nbytes <= 0) {
      ERROR((SGE_EVENT, MSG_SEC_DECRECONNECTFAILED));
      sge_sec_error();
      i = -1;
      goto error;
   }

   i = sec_unpack_reconnect(&pb, &tmp_connid, &tmp_seq_send, &tmp_seq_receive,
                              &new_key_mat, &tmp_key_mat_len, &time_str, &len);
   if (i) {
      ERROR((SGE_EVENT, MSG_SEC_UNPACKRECONNECTFAILED));
      goto error;
   }

   /* free old key_mat and keep newly allocated instead */
   sec_dealloc_key_mat();
   sec_state_set_key_mat(new_key_mat);
   sec_state_set_key_mat_len(tmp_key_mat_len);
   sec_state_set_connid(tmp_connid);
   sec_state_set_seq_send(tmp_seq_send);

   ptr = time_str;
   {
      ASN1_UTCTIME *tmp_refresh_time;
      sec_d2i_ASN1_UTCTIME(&tmp_refresh_time, (unsigned char **)&time_str, len);
      sec_state_set_refresh_time(tmp_refresh_time);
   } 
   free((char*) ptr);

   /* debug_print_ASN1_UTCTIME("refresh_time: ", sec_state_get_refresh_time()); */

   sec_state_set_connect(1);
   i = 0;
   
   error:
      if (fp) 
         fclose(fp);
      remove(reconnect_file);
      clear_packbuffer(&pb);
      DEXIT;
      return i;
}

/****** security/sec_lib/sec_write_data2hd() ************************************
*  NAME
*     sec_write_data2hd() -- write client reconnect data to file 
*
*  SYNOPSIS
*     static int sec_write_data2hd() 
*
*  FUNCTION
*   This function writes the security related data to disk, encrypted
*   with RSA private key.
*
*  INPUTS
*
*  RESULT
*     static int - 0 on success, -1 on failure
*
*  NOTES
*     MT-NOTE: sec_write_data2hd() is MT safe
*******************************************************************************/
static int sec_write_data2hd()
{
   FILE *fp=NULL;
   int i, nbytes;
   sge_pack_buffer pb;
   EVP_PKEY *evp = NULL;
   BIO *bio = NULL;
   unsigned char time_str[50]; 
   unsigned char *tmp;
   int len;

   DENTER(GDI_LAYER,"sec_write_data2hd");

   /* 
   ** prepare packing buffer                                       
   */
   if ((i=init_packbuffer(&pb, 1024, 0))) {
      ERROR((SGE_EVENT, MSG_SEC_INITPBFAILED));
      i = -1;
      goto error;
   }
   
   tmp = time_str;
   len = sec_i2d_ASN1_UTCTIME(sec_state_get_refresh_time(), &tmp);
   time_str[len] = '\0';
  
   if (sec_alloc_key_mat()) {
      ERROR((SGE_EVENT, MSG_MEMORY_MALLOCFAILED));
      i = -1;
      goto error;
   }

   /* 
   ** write data to packing buffer
   */
   if (sec_pack_reconnect(&pb, sec_state_get_connid(), sec_state_get_seq_send(), sec_state_get_seq_receive(),
                           sec_state_get_key_mat(), sec_state_get_key_mat_len(), 
                           time_str, len)) {
      ERROR((SGE_EVENT, MSG_SEC_PACKRECONNECTFAILED));
      i = -1;
      goto error;
   }

   /* 
   ** encrypt data with own public rsa key
   */
   evp = sec_X509_get_pubkey(gsd.x509);
   nbytes = sec_RSA_public_encrypt(pb.bytes_used, (u_char*)pb.head_ptr, 
                               (u_char*)pb.head_ptr, evp->pkey.rsa, 
                               RSA_PKCS1_PADDING);
   if (nbytes <= 0) {
      ERROR((SGE_EVENT, MSG_SEC_ENCRECONNECTFAILED));
      sge_sec_error();
      i = -1;
      goto error;
   }

   /* 
   ** write encrypted buffer to file
   */
   bio = sec_BIO_new_file(reconnect_file, "w");
   if (!bio) {
      ERROR((SGE_EVENT, MSG_SEC_CANTWRITE_SS,
               reconnect_file, strerror(errno)));
      i = -1;
      goto error;
   }
   i = sec_BIO_write(bio, pb.head_ptr, nbytes);
   if (!i) {
      ERROR((SGE_EVENT, MSG_SEC_CANTWRITE_SS, reconnect_file, 
               strerror(errno)));
      i = -1;
      goto error;
   }


   i=0;

   error:
      sec_BIO_free(bio);
      clear_packbuffer(&pb);   
      DEXIT;
      return i;
}

#endif


/*===========================================================================*/

/*
** NAME
**      sec_send_err
**
** SYNOPSIS
**
**   static int sec_send_err(commproc,id,host,pb,err_msg)
**      char *commproc - the program name of the source of the message,
**      int id         - the id of the communication,
**      char *host     - the host name of the sender,
**   sge_pack_buffer *pb - packing buffer for error message,
**   char *err_msg  - error message to send;
**
** DESCRIPTION
**   This function sends an error message to a client.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
**
** NOTES
**    MT-NOTES: sec_send_err() is MT safe
*/

static int sec_send_err(
const cl_com_endpoint_t *sender,
sge_pack_buffer *pb,
const char *err_msg,
u_long32 request_id,
lList **alpp
) {
   int   i;

   DENTER(GDI_LAYER,"sec_send_error");
   
   if (((i=packstr(pb,err_msg)) != 0) ||
       ((i=sge_send_any_request(0, NULL, sender->comp_host, sender->comp_name,
                                sender->comp_id, pb, TAG_SEC_ERROR, request_id,
                                alpp)))) {
      DEXIT;
      return(-1);
   }
   else {
      DEXIT;
      return(0);
   }
}

/*
** NAME
**   sec_set_connid
**
** SYNOPSIS
**
**   static int sec_set_connid(buffer,buflen)
**   char **buffer - buffer where to pack the connection ID at the end,
**   int *buflen   - length of buffer;
**
** DESCRIPTION
**      This function writes the connection ID at the end of the buffer.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
**
** NOTES
**      MT-NOTE: sec_set_connid() is MT safe
*/
static int sec_set_connid(char **buffer, int *buflen)
{
   u_long32 i, new_buflen;
   sge_pack_buffer pb;

   DENTER(GDI_LAYER,"sec_set_connid");

   new_buflen = *buflen + INTSIZE;

   pb.head_ptr = *buffer;
   pb.bytes_used = *buflen;
   pb.mem_size = *buflen;

   if ((i = packint(&pb, sec_state_get_connid()))){
      ERROR((SGE_EVENT, MSG_SEC_PACKCONNIDFAILED));
      goto error;
   }

   *buflen = new_buflen;
   *buffer = pb.head_ptr;

   i = 0;
   error:
   DEXIT;
   return(i);
}

/*
** NAME
**   sec_get_connid
**
** SYNOPSIS
**
**   static int sec_get_connid(buffer,buflen)
**      char **buffer - buffer where to read the connection ID from the end,
**      u_long32 *buflen   - length of buffer;
**
** DESCRIPTION
**      This function reads the connection ID from the end of the buffer.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
**
** NOTES
**    MT-NOTE: sec_get_connid() is MT safe
*/

static int sec_get_connid(char **buffer, u_long32 *buflen)
{
   int i;
   u_long32 new_buflen, tmp_connid;
   sge_pack_buffer pb;

   DENTER(GDI_LAYER,"sec_get_connid");

   new_buflen = *buflen - INTSIZE;

   pb.head_ptr = *buffer;
   pb.cur_ptr = pb.head_ptr + new_buflen;
   pb.bytes_used = new_buflen;
   pb.mem_size = *buflen;

   if((i = unpackint(&pb, &tmp_connid))){
      ERROR((SGE_EVENT, MSG_SEC_UNPACKCONNIDFAILED));
      goto error;
   }
   sec_state_set_connid(tmp_connid);

   *buflen = new_buflen;
   
   i = 0;

   error:
     DEXIT;
     return(i);
}

/*
** NAME
**   sec_update_connlist
**
** SYNOPSIS
**
**   static int sec_update_connlist(host,commproc,id)
**   char *host     - hostname,
**   char *commproc - name of communication program,
**   int id         - commd ID of the connection;
**
** DESCRIPTION
**      This function searchs the connection list for the connection ID and
**   writes host name, commproc name and id to the list.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
**
** NOTES
**    MT-NOTE: sec_update_connlist() is MT safe
*/

static int sec_update_connlist(const cl_com_endpoint_t *sender)
{
   lListElem *element = NULL;

   DENTER(GDI_LAYER,"sec_update_connlist");

   SEC_LOCK_CONN_LIST();

   element = lGetElemUlong(sec_conn_list, SEC_ConnectionID, sec_state_get_connid());
   if (!element){
      SEC_UNLOCK_CONN_LIST();
      ERROR((SGE_EVENT, MSG_SEC_CONNECTIONNOENTRY));
      DEXIT;
      return -1;
   }
   lSetHost(element, SEC_Host, sender->comp_host);
   lSetString(element, SEC_Commproc, sender->comp_name);
   lSetInt(element, SEC_Id, sender->comp_id);

   SEC_UNLOCK_CONN_LIST();

   DEXIT;
   return 0;
}

/*
** NAME
**   sec_set_secdata
**
** SYNOPSIS
**
**   static int sec_set_secdata(host,commproc,id)
**   char *host     - hostname,
**      char *commproc - name of communication program,
**      int id         - commd ID of the connection;
**
** DESCRIPTION
**      This function searchs the connection list for name, commproc and
**   id and sets then the security data for this connection.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
**
** NOTES
**    MT-NOTE: Caller must own sec_conn_list_mutex
**    MT-NOTE: sec_set_secdata() is not MT safe
*/
static int sec_set_secdata(const cl_com_endpoint_t *sender)
{
   lListElem *element=NULL;
   lCondition *where=NULL;

   DENTER(GDI_LAYER,"sec_set_secdata");

   /* 
   ** get right element from connection list         
   */
   if (sender && sender->comp_id) {
      where = lWhere("%T(%I==%s && %I==%s && %I==%d)", SecurityT,
                       SEC_Host, sender->comp_host, SEC_Commproc, sender->comp_name, SEC_Id, sender->comp_id);
   }
   else {
      where = lWhere("%T(%I==%s && %I==%s)", SecurityT,
                     SEC_Host, sender->comp_host, SEC_Commproc, sender->comp_name);
   }

   element = lFindFirst(sec_conn_list, where);
   where = lFreeWhere(where);
   if (!element) {
      ERROR((SGE_EVENT, MSG_SEC_CONNECTIONNOENTRY_SSU, sender->comp_host, sender->comp_name, sender->comp_id));
      DEXIT;
      return -1;
   } 
   
   /* 
   ** set security data                  
   */
   sec_list2keymat(element);
   strcpy(sec_state_get_unique_identifier(), lGetString(element, SEC_UniqueIdentifier));
   sec_state_set_connid(lGetUlong(element, SEC_ConnectionID));
   sec_state_set_seq_send(lGetUlong(element, SEC_SeqNoSend));
   sec_state_set_seq_receive(lGetUlong(element, SEC_SeqNoReceive));
   
   DEXIT;
   return 0;
}

/*
** NAME
**   sec_insert_conn2list
**
** SYNOPSIS
**
**   static int sec_insert_conn2list(host,commproc,id)
**      char *host     - hostname,
**      char *commproc - name of communication program,
**      int id         - commd ID of the connection;
**
** DESCRIPTION
**      This function inserts a new connection to the connection list.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
**
** NOTES
**      MT-NOTE: sec_insert_conn2list() is MT safe
*/
static int sec_insert_conn2list(u_long32 connid, const cl_com_endpoint_t *sender, const char *uniqueIdentifier)
{
   lListElem *element;
   lCondition *where;

   DENTER(GDI_LAYER, "sec_insert_conn2list");
   
   /* 
   ** delete element if some with <host,commproc,id> exists   
   */
   where = lWhere("%T(%I==%s && %I==%s && %I==%d)", SecurityT,
                    SEC_Host, sender->comp_host,
                    SEC_Commproc, sender->comp_name,
                    SEC_Id, sender->comp_id);
   if (!where) {
      DEXIT;
      return -1;
   }

   SEC_LOCK_CONN_LIST();

   element = lFindFirst(sec_conn_list, where);
   where = lFreeWhere(where);

   if (!element && sec_is_daemon(sender->comp_name)) {
      where = lWhere("%T(%I==%s && %I==%s)", SecurityT,
                       SEC_Host, sender->comp_host,
                       SEC_Commproc, sender->comp_name);
      if (!where) {
         SEC_UNLOCK_CONN_LIST();
         DEXIT;
         return -1;
      }
      element = lFindFirst(sec_conn_list, where);
      where = lFreeWhere(where);
   }   

   /* 
   ** add new element if it does not exist                  
   */
   if (!element || !sec_conn_list) {
      element = lAddElemUlong(&sec_conn_list, SEC_ConnectionID, 
                                 connid, SecurityT);
   }
   if (!element) {
      SEC_UNLOCK_CONN_LIST();
      ERROR((SGE_EVENT, MSG_SEC_INSERTCONNECTIONFAILED));
      DEXIT;
      return -1;
   }

   /* 
   ** set values of element               
   */
   lSetUlong(element, SEC_ConnectionID, connid);
   lSetHost(element, SEC_Host, sender->comp_host);
   lSetString(element, SEC_Commproc, sender->comp_name); 
   lSetInt(element, SEC_Id, sender->comp_id); 
   lSetString(element, SEC_UniqueIdentifier, uniqueIdentifier); 
   lSetUlong(element, SEC_SeqNoSend, 0); 
   lSetUlong(element, SEC_SeqNoReceive, 0); 
   sec_keymat2list(element);
{
   int len;
   unsigned char *tmp;
   ASN1_UTCTIME *asn1_time_str = NULL;
   unsigned char time_str[50]; 

   asn1_time_str = sec_X509_gmtime_adj(asn1_time_str, 60*VALID_MINUTES);
   tmp = time_str;
   len = sec_i2d_ASN1_UTCTIME(asn1_time_str, &tmp);
   time_str[len] = '\0';
   sec_ASN1_UTCTIME_free(asn1_time_str);

   lSetString(element, SEC_ExpiryDate, (char*) time_str);
}   
   SEC_UNLOCK_CONN_LIST();

   DPRINTF(("++++ added %d (%s, %s, %d) of %s to sec_conn_list\n", 
               (int)connid, sender->comp_host, sender->comp_name, sender->comp_id, uniqueIdentifier));

   /* 
   ** increment connid                   
   */
   SEC_LOCK_CONNID_COUNTER();
   sec_connid_counter = INC32(sec_connid_counter);
   SEC_UNLOCK_CONNID_COUNTER();

   DEXIT;
   return 0;   
}


/*
** NAME
**   sec_keymat2list
**
** SYNOPSIS
**   
**   static void sec_keymat2list(element)
**   lListElem *element - the elment where to put the key material
**
** DESCRIPTION
**   This function converts the key material to unsigned long to store
**   it within the connection list, a cull list.
**
** NOTES
**    MT-NOTE: sec_keymat2list() is MT safe
*/

static void sec_keymat2list(lListElem *element)
{
   int i;
   u_long32 ul[8];
   u_char working_buf[32 + 33], *pos_ptr;

   memcpy(working_buf, sec_state_get_key_mat(), sec_state_get_key_mat_len());
   pos_ptr = working_buf;
   for(i=0;i<8;i++) c4TOl(pos_ptr,ul[i]);

   lSetUlong(element,SEC_KeyPart0,ul[0]);
   lSetUlong(element,SEC_KeyPart1,ul[1]);
   lSetUlong(element,SEC_KeyPart2,ul[2]);
   lSetUlong(element,SEC_KeyPart3,ul[3]);
   lSetUlong(element,SEC_KeyPart4,ul[4]);
   lSetUlong(element,SEC_KeyPart5,ul[5]);
   lSetUlong(element,SEC_KeyPart6,ul[6]);
   lSetUlong(element,SEC_KeyPart7,ul[7]);
}

/*
** NAME
**   sec_list2keymat
**
** SYNOPSIS
**
**   static void sec_list2keymat(element)
**   lListElem *element - the elment where to get the key material
**
** DESCRIPTION
**   This function converts the igned longs from the connection list
**   to key material.
**
** NOTES
**   MT-NOTE: sec_list2keymat() is MT safe
*/

static void sec_list2keymat(lListElem *element)
{
   int i;
   u_long32 ul[8];
   u_char working_buf[32 + 33],*pos_ptr;

   ul[0]=lGetUlong(element,SEC_KeyPart0);
   ul[1]=lGetUlong(element,SEC_KeyPart1);
   ul[2]=lGetUlong(element,SEC_KeyPart2);
   ul[3]=lGetUlong(element,SEC_KeyPart3);
   ul[4]=lGetUlong(element,SEC_KeyPart4);
   ul[5]=lGetUlong(element,SEC_KeyPart5);
   ul[6]=lGetUlong(element,SEC_KeyPart6);
   ul[7]=lGetUlong(element,SEC_KeyPart7);

   pos_ptr = working_buf;
   for(i=0;i<8;i++) lTO4c(ul[i],pos_ptr);
   memcpy(sec_state_get_key_mat(), working_buf, sec_state_get_key_mat_len());

}

/*==========================================================================*/
/*
** DESCRIPTION
**	These functions generate the pathnames for key and certificate files.
** and stores them in module static vars and forces an SGE_EXIT if something
** is wrong
**
** NOTES
**    MT-NOTE: sec_setup_path() is not MT safe
*/
static void sec_setup_path(
int is_daemon,
int is_master
) {
   SGE_STRUCT_STAT sbuf;
	char *userdir = NULL;
	char *user_local_dir = NULL;
	char *ca_root = NULL;
	char *ca_local_root = NULL;
   char *sge_cakeyfile = NULL;
   char *sge_keyfile = NULL;
   char *sge_certfile = NULL;
   int len;
   char *cp = NULL;

   DENTER(GDI_LAYER, "sec_setup_path");
 
   cp = getenv("SGE_QMASTER_PORT");
#define SGE_COMMD_SERVICE "sge_qmaster"
   
   /*
   ** malloc ca_root string and check if directory has been created during
   ** install otherwise exit
   */
   len = strlen(sge_get_root_dir(1, NULL, 0, 1)) + strlen(sge_get_default_cell()) +
         strlen(CA_DIR) + 3;
   ca_root = sge_malloc(len);
   sprintf(ca_root, "%s/%s/%s", sge_get_root_dir(1, NULL, 0, 1), 
                     sge_get_default_cell(), CA_DIR);
   if (SGE_STAT(ca_root, &sbuf)) { 
      CRITICAL((SGE_EVENT, MSG_SEC_CAROOTNOTFOUND_S, ca_root));
      SGE_EXIT(1);
   }

   /*
   ** malloc ca_local_root string and check if directory has been created during
   ** install otherwise exit
   */
   if ((sge_cakeyfile=getenv("SGE_CAKEYFILE"))) {
      ca_key_file = strdup(sge_cakeyfile);
   } else {
      if (getenv("SGE_NO_CA_LOCAL_ROOT")) {
         ca_local_root = ca_root;
      } else {
         char *ca_local_dir = NULL;
         /* If the user is root, use /var/sgeCA.  Otherwise, use /tmp/sgeCA */
#if 0
         if (geteuid () == 0) {
            ca_local_dir = CA_LOCAL_DIR;
         }
         else {
            ca_local_dir = USER_CA_LOCAL_DIR;
         }
#endif
         ca_local_dir = CA_LOCAL_DIR; 
         
         len = strlen(ca_local_dir) + 
               (cp ? strlen(cp)+4:strlen(SGE_COMMD_SERVICE)) +
               strlen(sge_get_default_cell()) + 3;
         ca_local_root = sge_malloc(len);
         if (cp)
            sprintf(ca_local_root, "%s/port%s/%s", ca_local_dir, cp, 
                     sge_get_default_cell());
         else
            sprintf(ca_local_root, "%s/%s/%s", ca_local_dir, SGE_COMMD_SERVICE, 
                     sge_get_default_cell());
      }   
      if (is_daemon && SGE_STAT(ca_local_root, &sbuf)) { 
         CRITICAL((SGE_EVENT, MSG_SEC_CALOCALROOTNOTFOUND_S, ca_local_root));
         SGE_EXIT(1);
      }
      ca_key_file = sge_malloc(strlen(ca_local_root) + (sizeof("private")-1) + strlen(CaKey) + 3);
      sprintf(ca_key_file, "%s/private/%s", ca_local_root, CaKey);
   }

   if (is_master && SGE_STAT(ca_key_file, &sbuf)) { 
      CRITICAL((SGE_EVENT, MSG_SEC_CAKEYFILENOTFOUND_S, ca_key_file));
      SGE_EXIT(1);
   }
   DPRINTF(("ca_key_file: %s\n", ca_key_file));

	ca_cert_file = sge_malloc(strlen(ca_root) + strlen(CaCert) + 2);
	sprintf(ca_cert_file, "%s/%s", ca_root, CaCert);

   if (SGE_STAT(ca_cert_file, &sbuf)) { 
      CRITICAL((SGE_EVENT, MSG_SEC_CACERTFILENOTFOUND_S, ca_cert_file));
      SGE_EXIT(1);
   }
   DPRINTF(("ca_cert_file: %s\n", ca_cert_file));

   /*
   ** determine userdir: 
   ** - ca_root, ca_local_root for daemons 
   ** - $HOME/.sge/{port$COMMD_PORT|SGE_COMMD_SERVICE}/$SGE_CELL
   **   and as fallback
   **   /var/sgeCA/{port$COMMD_PORT|SGE_COMMD_SERVICE}/$SGE_CELL/userkeys/$USER/{cert.pem,key.pem}
   */
   if (is_daemon){
      userdir = strdup(ca_root);
      user_local_dir = ca_local_root;
   } else {
      struct passwd *pw;
      pw = sge_getpwnam(uti_state_get_user_name());
      if (!pw) {   
         CRITICAL((SGE_EVENT, MSG_SEC_USERNOTFOUND_S, uti_state_get_user_name()));
         SGE_EXIT(1);
      }
      userdir = sge_malloc(strlen(pw->pw_dir) + strlen(SGESecPath) +
                          (cp ? strlen(cp) + 4 : strlen(SGE_COMMD_SERVICE)) +
                           strlen(sge_get_default_cell()) + 4);
      if (cp)                     
         sprintf(userdir, "%s/%s/port%s/%s", pw->pw_dir, SGESecPath, cp, 
               sge_get_default_cell());
      else         
         sprintf(userdir, "%s/%s/%s/%s", pw->pw_dir, SGESecPath, 
                  SGE_COMMD_SERVICE, sge_get_default_cell());
      user_local_dir = userdir;
   }

   if ((sge_keyfile = getenv("SGE_KEYFILE"))) {
      key_file = strdup(sge_keyfile);
   } else {   
      key_file = sge_malloc(strlen(user_local_dir) + (sizeof("private")-1) + strlen(UserKey) + 3);
      sprintf(key_file, "%s/private/%s", user_local_dir, UserKey);
   }   

   if (SGE_STAT(key_file, &sbuf)) { 
      free(key_file);
      key_file = sge_malloc(strlen(ca_local_root) + (sizeof("userkeys")-1) + 
                              strlen(uti_state_get_user_name()) + strlen(UserKey) + 4);
      sprintf(key_file, "%s/userkeys/%s/%s", ca_local_root, uti_state_get_user_name(), UserKey);
   }   

   if (!sec_RAND_status()) {
      rand_file = sge_malloc(strlen(user_local_dir) + (sizeof("private")-1) + strlen(RandFile) + 3);
      sprintf(rand_file, "%s/private/%s", user_local_dir, RandFile);

      if (SGE_STAT(rand_file, &sbuf)) { 
         free(rand_file);
         rand_file = sge_malloc(strlen(ca_local_root) + (sizeof("userkeys")-1) + 
                                 strlen(uti_state_get_user_name()) + strlen(RandFile) + 4);
         sprintf(rand_file, "%s/userkeys/%s/%s", ca_local_root, uti_state_get_user_name(), RandFile);
      }   
   }   
   if (SGE_STAT(key_file, &sbuf)) { 
      CRITICAL((SGE_EVENT, MSG_SEC_KEYFILENOTFOUND_S, key_file));
      SGE_EXIT(1);
   }
   DPRINTF(("key_file: %s\n", key_file));

   if (!sec_RAND_status()) {
      if (SGE_STAT(rand_file, &sbuf)) { 
         WARNING((SGE_EVENT, MSG_SEC_RANDFILENOTFOUND_S, rand_file));
      }
      else {
         DPRINTF(("rand_file: %s\n", rand_file));
      }   
   }    

   if ((sge_certfile = getenv("SGE_CERTFILE"))) {
      cert_file = strdup(sge_certfile);
   } else {   
      cert_file = sge_malloc(strlen(userdir) + (sizeof("certs")-1) + strlen(UserCert) + 3);
      sprintf(cert_file, "%s/certs/%s", userdir, UserCert);
   }

   if (SGE_STAT(cert_file, &sbuf)) {
      free(cert_file);
      cert_file = sge_malloc(strlen(ca_local_root) + (sizeof("userkeys")-1) + 
                              strlen(uti_state_get_user_name()) + strlen(UserCert) + 4);
      sprintf(cert_file, "%s/userkeys/%s/%s", ca_local_root, uti_state_get_user_name(), UserCert);
   }   

   if (SGE_STAT(cert_file, &sbuf)) { 
      CRITICAL((SGE_EVENT, MSG_SEC_CERTFILENOTFOUND_S, cert_file));
      SGE_EXIT(1);
   }
   DPRINTF(("cert_file: %s\n", cert_file));

	reconnect_file = sge_malloc(strlen(userdir) + strlen(ReconnectFile) + 2); 
   sprintf(reconnect_file, "%s/%s", userdir, ReconnectFile);
   DPRINTF(("reconnect_file: %s\n", reconnect_file));
    
   free(userdir);
   free(ca_root);
   if (!getenv("SGE_NO_CA_LOCAL_ROOT")) {
      free(ca_local_root);
   }   

   DEXIT;
   return;
}


/*
** NAME
**      sec_pack_* sec_unpack_*
**
** SYNOPSIS
**      #include "sec_pack.h"
**
**      int sec_pack_*  int sec_unpack_*
**
** DESCRIPTION
**      These functions pack or unpack data from a packing buffer.
**
** RETURN VALUES
**      0       on success
**      <>0     on failure
**
** NOTES 
**      MT-NOTE: sec_pack_announce() is MT safe 
**      MT-NOTE: sec_unpack_announce() is MT safe 
*/
static int sec_pack_announce(u_long32 certlen, u_char *x509_buf, u_char *challenge, sge_pack_buffer *pb)
{
   int   i;

   if((i=packint(pb, certlen))) 
      goto error;
   if((i=packbuf(pb,(char *) x509_buf, certlen))) 
      goto error;
   if((i=packbuf(pb,(char *) challenge, CHALL_LEN))) 
      goto error;

   error:
      return(i);
}

static int sec_unpack_announce(u_long32 *certlen, u_char **x509_buf, u_char **challenge, sge_pack_buffer *pb)
{
   int i;

   if((i=unpackint(pb, certlen))) 
      goto error;
   if((i=unpackbuf(pb, (char **) x509_buf, *certlen))) 
      goto error;
   if((i=unpackbuf(pb, (char **) challenge, CHALL_LEN))) 
      goto error;

   error:   
      return(i);
}

/* MT-NOTE: sec_pack_response() is MT safe */
static int sec_pack_response(u_long32 certlen, u_char *x509_buf, 
                      u_long32 chall_enc_len, u_char *enc_challenge, 
                      u_long32 connid, 
                      u_long32 enc_key_len, u_char *enc_key, 
                      u_long32 iv_len, u_char *iv, 
                      u_long32 enc_key_mat_len, u_char *enc_key_mat, 
                      sge_pack_buffer *pb)
{
   int i;

   if((i=packint(pb, certlen))) 
      goto error;
   if((i=packbuf(pb,(char *) x509_buf, certlen))) 
      goto error;
   if((i=packint(pb, chall_enc_len))) 
      goto error;
   if((i=packbuf(pb,(char *) enc_challenge, chall_enc_len)))
      goto error;
   if((i=packint(pb, connid))) 
      goto error;
   if((i=packint(pb, enc_key_len))) 
      goto error;
   if((i=packbuf(pb,(char *) enc_key, enc_key_len))) 
      goto error;
   if((i=packint(pb, iv_len))) 
      goto error;
   if((i=packbuf(pb, (char *) iv, iv_len))) 
      goto error;
   if((i=packint(pb, enc_key_mat_len))) 
      goto error;
   if((i=packbuf(pb, (char *) enc_key_mat, enc_key_mat_len))) 
      goto error;

   error:
      return(i);
}

/* MT-NOTE: sec_unpack_response() is MT safe */
static int sec_unpack_response(u_long32 *certlen, u_char **x509_buf, 
                        u_long32 *chall_enc_len, u_char **enc_challenge, 
                        u_long32 *connid, 
                        u_long32 *enc_key_len, u_char **enc_key, 
                        u_long32 *iv_len, u_char **iv, 
                        u_long32 *enc_key_mat_len, u_char **enc_key_mat, 
                        sge_pack_buffer *pb)
{
   int i;

   if((i=unpackint(pb, certlen))) 
      goto error;
   if((i=unpackbuf(pb, (char **) x509_buf, *certlen))) 
      goto error;
   if((i=unpackint(pb, chall_enc_len))) 
      goto error;
   if((i=unpackbuf(pb,(char **) enc_challenge, *chall_enc_len)))
      goto error;
   if((i=unpackint(pb, connid))) 
      goto error;
   if((i=unpackint(pb, enc_key_len))) 
      goto error;
   if((i=unpackbuf(pb,(char **) enc_key, *enc_key_len)))
      goto error;
   if((i=unpackint(pb, iv_len))) 
      goto error;
   if((i=unpackbuf(pb, (char **) iv, *iv_len)))
      goto error;
   if((i=unpackint(pb, enc_key_mat_len))) 
      goto error;
   if((i=unpackbuf(pb,(char **) enc_key_mat, *enc_key_mat_len)))
      goto error;


   error:
      return(i);
}

/* MT-NOTE: sec_pack_message() is MT safe */
static int sec_pack_message(sge_pack_buffer *pb, u_long32 connid, u_long32 enc_mac_len, u_char *enc_mac, u_char *enc_msg, u_long32 enc_msg_len)
{
   int i;

   if((i=packint(pb, connid))) 
      goto error;
   if((i=packint(pb, enc_mac_len))) 
      goto error;
   if((i=packbuf(pb, (char *) enc_mac, enc_mac_len))) 
      goto error;
   if((i=packint(pb, enc_msg_len))) 
      goto error;
   if((i=packbuf(pb, (char *) enc_msg, enc_msg_len))) 
      goto error;

   error:
      return(i);
}

/* MT-NOTE: sec_unpack_message() is MT safe */
static int sec_unpack_message(sge_pack_buffer *pb, u_long32 *connid, u_long32 *enc_mac_len, u_char **enc_mac, u_long32 *enc_msg_len, u_char **enc_msg)
{
   int i;

   if((i=unpackint(pb, connid))) 
      goto error;
   if((i=unpackint(pb, enc_mac_len))) 
      goto error;
   if((i=unpackbuf(pb, (char**) enc_mac, *enc_mac_len))) 
      goto error;
   if((i=unpackint(pb, enc_msg_len))) 
      goto error;
   if((i=unpackbuf(pb, (char**) enc_msg, *enc_msg_len))) 
      goto error;

   error:
      return(i);
}

#ifdef SEC_RECONNECT

/* MT-NOTES: sec_pack_reconnect() is MT safe */
static int sec_pack_reconnect(sge_pack_buffer *pb, u_long32 connid, u_long32 seq_send, u_long32 seq_receive, u_char *key_mat, u_long32 key_mat_len,
u_char *refreshtime, u_long32 refreshtime_len)
{
   int i;

   if ((i=packint(pb, connid))) 
      goto error;
   if ((i=packint(pb, seq_send))) 
      goto error;
   if ((i=packint(pb, seq_receive))) 
      goto error;
   if ((i=packint(pb, key_mat_len))) 
      goto error;
   if ((i=packbuf(pb, (char *) key_mat, key_mat_len)))
      goto error;
   if ((i=packint(pb, refreshtime_len))) 
      goto error;
   if ((i=packbuf(pb, (char *) refreshtime, refreshtime_len)))
      goto error;


   error:
        return(i);
}

/* MT-NOTES: sec_unpack_reconnect() is MT safe */
static int sec_unpack_reconnect(sge_pack_buffer *pb, u_long32 *connid, u_long32 *seq_send, u_long32 *seq_receive, u_char **key_mat, u_long32 *key_mat_len,
u_char **refreshtime, u_long32 *refreshtime_len)
{
   int i;

   if ((i=unpackint(pb, connid))) 
      goto error;
   if ((i=unpackint(pb, seq_send))) 
      goto error;
   if ((i=unpackint(pb, seq_receive))) 
      goto error;
   if ((i=unpackint(pb, key_mat_len))) 
      goto error;
   if ((i=unpackbuf(pb, (char **) key_mat, *key_mat_len)))
      goto error;
   if ((i=unpackint(pb, refreshtime_len))) 
      goto error;
   if ((i=unpackbuf(pb, (char **) refreshtime, *refreshtime_len)))
      goto error;

   error:
      return(i);
}
#endif

/*
** NAME
**   sec_verify_user
**
** SYNOPSIS
**   #include "sec_lib.h"
**
**   int sec_verify_user(user)
**   char *user - unix user name
**
** DESCRIPTION
**   This function checks if the unix user name that has been send corresponds 
**   to the certificate
**
** RETURN VALUES
**      1       on success
**      0       on failure
** NOTES
**    MT-NOTES: sec_verify_user() is MT safe
*/
int sec_verify_user(const char *user, const char *commproc) 
{
   DENTER(GDI_LAYER,"sec_verify_user");

   if (!sec_is_daemon(commproc)) {
      DPRINTF(("commproc = '%s' user = '%s', unique_identifier = '%s'\n", 
               commproc, user, sec_state_get_unique_identifier()));
      if (strcmp(user, sec_state_get_unique_identifier())) {
         DEXIT;
         return 0;
      }   
   }
      

   DEXIT;
   return 1;
}
 
/* MT-NOTE: sec_is_daemon() is MT safe */
static int sec_is_daemon(const char *progname)
{
   if (!strcmp(prognames[QMASTER],progname) ||
      !strcmp(prognames[EXECD],progname) ||
      !strcmp(prognames[SCHEDD],progname))
      return True;
   else
      return False;
}

/* MT-NOTE: sec_is_master() is MT safe */
static int sec_is_master(const char *progname)
{
   if (!strcmp(prognames[QMASTER],progname))
      return True;
   else
      return False;
}      
 
/* MT-NOTE: sec_verify_callback() is MT safe */
static int sec_verify_callback(int ok, X509_STORE_CTX *ctx)
{
   X509 *cert;
   
   printf("ok = %d\n", ok);
   cert = sec_X509_STORE_CTX_get_current_cert(ctx);
   if (cert) {
      sec_X509_print_fp(stdout, cert);
debug_print_ASN1_UTCTIME("not_before: ", sec_X509_get_notBefore(cert));
debug_print_ASN1_UTCTIME("not_after: ", sec_X509_get_notAfter(cert));
printf(" notBefore X509_cmp_current_time = %d\n",  sec_X509_cmp_current_time(sec_X509_get_notBefore(cert)));
printf(" notAfter X509_cmp_current_time = %d\n",  sec_X509_cmp_current_time(sec_X509_get_notAfter(cert)));
   }
   else
      printf("No cert\n");

   return ok;
}



/* MT-NOTE: sec_clear_connectionlist() is MT safe */
int sec_clear_connectionlist(void)
{
   lListElem *element = NULL;

   DENTER(GDI_LAYER, "sec_clear_connectionlist");
   
   /*
   ** remove any outdated entries first
   */
   SEC_LOCK_CONN_LIST();
   if (sec_conn_list) {
      ASN1_UTCTIME *utctime = sec_ASN1_UTCTIME_new();

      element = lFirst(sec_conn_list);
      while (element) {
         lListElem *del_el;
         const char *dtime = lGetString(element, SEC_ExpiryDate);
         utctime = sec_d2i_ASN1_UTCTIME(&utctime, (unsigned char**) &dtime, 
                                          strlen(dtime));
         /* debug_print_ASN1_UTCTIME("utctime: ", utctime); */
         del_el = element;
         element = lNext(element);
         if ((sec_X509_cmp_current_time(utctime) < 0)) { 
            DPRINTF(("---- removed %d (%s, %s, %d) from sec_conn_list\n", 
                      (int) lGetUlong(del_el, SEC_ConnectionID), 
                      lGetHost(del_el, SEC_Host),
                      lGetString(del_el, SEC_Commproc),
                      lGetInt(del_el, SEC_Id)));
            if (!sec_is_daemon(lGetString(del_el, SEC_Commproc))) {
               lRemoveElem(sec_conn_list, del_el);
            }
         }   
      }
      sec_ASN1_UTCTIME_free(utctime);
   }
   SEC_UNLOCK_CONN_LIST();

   DEXIT;
   return True;
}

static int sec_build_symbol_table (void)
{
   DENTER(GDI_LAYER, "sec_build_symbol_table");
   
#ifdef LOAD_OPENSSL

#if defined(DARWIN)
   handle = dlopen ("libcrypto.dylib", RTLD_NOW|RTLD_GLOBAL);
#elif defined(HP11)
   handle = dlopen ("libcrypto.sl", RTLD_LAZY);
#else   
   handle = dlopen ("libcrypto.so", RTLD_LAZY);
#endif


   if (handle == NULL) {
      DEXIT;
      return 1;
   }

   if ((sec_ASN1_UTCTIME_free = (void (*)(ASN1_UTCTIME *))dlsym (handle, "ASN1_UTCTIME_free")) == NULL) goto error;
   if ((sec_ASN1_UTCTIME_new = (ASN1_UTCTIME *(*)(void))dlsym (handle, "ASN1_UTCTIME_new")) == NULL) goto error;
   if ((sec_ASN1_UTCTIME_print = (int (*)(BIO *, ASN1_UTCTIME *))dlsym (handle, "ASN1_UTCTIME_print")) == NULL) goto error;
   if ((sec_BIO_free = (int (*)(BIO *))dlsym (handle, "BIO_free")) == NULL) goto error;
   if ((sec_BIO_new_file = (BIO *(*)(const char *, const char *))dlsym (handle, "BIO_new_file")) == NULL) goto error;
   if ((sec_BIO_new_fp = (BIO *(*)(FILE *, int))dlsym (handle, "BIO_new_fp")) == NULL) goto error;
   if ((sec_BIO_read = (int (*)(BIO *, void *, int))dlsym (handle, "BIO_read")) == NULL) goto error;
   if ((sec_BIO_write = (int (*)(BIO *b, const void *data, int len))dlsym (handle, "BIO_write")) == NULL) goto error;
   if ((sec_CRYPTO_get_lock_name = (const char *(*)(int))dlsym (handle, "CRYPTO_get_lock_name")) == NULL) goto error;
   if ((sec_CRYPTO_num_locks = (int (*)(void))dlsym (handle, "CRYPTO_num_locks")) == NULL) goto error;
   if ((sec_CRYPTO_set_id_callback = (void (*)(unsigned long (*)(void)))dlsym (handle, "CRYPTO_set_id_callback")) == NULL) goto error;
   if ((sec_CRYPTO_set_locking_callback = (void (*)(void (*)(int, int, const char *, int)))dlsym (handle, "CRYPTO_set_locking_callback")) == NULL) goto error;
   if ((sec_d2i_ASN1_UTCTIME = (ASN1_UTCTIME *(*)(ASN1_UTCTIME **, unsigned char **, long ))dlsym (handle, "d2i_ASN1_UTCTIME")) == NULL) goto error;
   if ((sec_d2i_X509 = (X509 *(*)(X509 **, unsigned char **, long))dlsym (handle, "d2i_X509")) == NULL) goto error;
   if ((sec_ERR_clear_error = (void (*)(void))dlsym (handle, "ERR_clear_error")) == NULL) goto error;
   if ((sec_ERR_error_string = (char *(*)(unsigned long, char *))dlsym (handle, "ERR_error_string")) == NULL) goto error;
   if ((sec_ERR_get_error = (unsigned long (*)(void))dlsym (handle, "ERR_get_error")) == NULL) goto error;
   if ((sec_ERR_load_crypto_strings = (void (*)(void))dlsym (handle, "ERR_load_crypto_strings")) == NULL) goto error;
   if ((sec_EVP_add_cipher = (int (*)(const EVP_CIPHER *))dlsym (handle, "EVP_add_cipher")) == NULL) goto error;
   if ((sec_EVP_add_digest = (int (*)(const EVP_MD *))dlsym (handle, "EVP_add_digest")) == NULL) goto error;
   if ((_sec_EVP_add_digest_alias = (int (*)(const char *, int, const char *))dlsym (handle, "OBJ_NAME_add")) == NULL) goto error;
   if ((sec_EVP_CIPHER_CTX_cleanup = (int (*)(EVP_CIPHER_CTX *))dlsym (handle, "EVP_CIPHER_CTX_cleanup")) == NULL) goto error;
   if ((sec_EVP_DecryptFinal = (int (*)(EVP_CIPHER_CTX *, unsigned char *, int *))dlsym (handle, "EVP_DecryptFinal")) == NULL) goto error;
   if ((sec_EVP_DecryptInit = (int (*)(EVP_CIPHER_CTX *, const EVP_CIPHER *, const unsigned char *, const unsigned char *))dlsym (handle, "EVP_DecryptInit")) == NULL) goto error;
   if ((sec_EVP_DecryptUpdate = (int (*)(EVP_CIPHER_CTX *, unsigned char *, int *, const unsigned char *, int))dlsym (handle, "EVP_DecryptUpdate")) == NULL) goto error;
   if ((sec_EVP_DigestInit = (int (*)(EVP_MD_CTX *, const EVP_MD *))dlsym (handle, "EVP_DigestInit")) == NULL) goto error;
   if ((sec_EVP_DigestFinal = (int (*)(EVP_MD_CTX *, unsigned char *, unsigned int *))dlsym (handle, "EVP_DigestFinal")) == NULL) goto error;
   if ((sec_EVP_DigestUpdate = (int (*)(EVP_MD_CTX *, const void *, unsigned int))dlsym (handle, "EVP_DigestUpdate")) == NULL) goto error;
   if ((sec_EVP_EncryptInit = (int (*)(EVP_CIPHER_CTX *, const EVP_CIPHER *, const unsigned char *, const unsigned char *))dlsym (handle, "EVP_EncryptInit")) == NULL) goto error;
   if ((sec_EVP_EncryptUpdate = (int (*)(EVP_CIPHER_CTX *, unsigned char *, int *, const unsigned char *, int))dlsym (handle, "EVP_EncryptUpdate")) == NULL) goto error;
   if ((sec_EVP_EncryptFinal = (int (*)(EVP_CIPHER_CTX *, unsigned char *, int *))dlsym (handle, "EVP_EncryptFinal")) == NULL) goto error;
   if ((sec_EVP_md5 = (const EVP_MD *(*)(void))dlsym (handle, "EVP_md5")) == NULL) goto error;
   if ((sec_EVP_OpenInit = (int (*)(EVP_CIPHER_CTX *, const EVP_CIPHER *, unsigned char *, int, unsigned char *, EVP_PKEY *))dlsym (handle, "EVP_OpenInit")) == NULL) goto error;
   if ((sec_EVP_OpenFinal = (int (*)(EVP_CIPHER_CTX *, unsigned char *, int *))dlsym (handle, "EVP_OpenFinal")) == NULL) goto error;
   sec_EVP_OpenUpdate = sec_EVP_DecryptUpdate;
   if ((sec_EVP_PKEY_free = (void (*)(EVP_PKEY *))dlsym (handle, "EVP_PKEY_free")) == NULL) goto error;
   if ((sec_EVP_PKEY_size = (int (*)(EVP_PKEY *))dlsym (handle, "EVP_PKEY_size")) == NULL) goto error;
   if ((sec_EVP_rc4 = (const EVP_CIPHER *(*)(void))dlsym (handle, "EVP_rc4")) == NULL) goto error;
   if ((sec_EVP_SealInit = (int (*)(EVP_CIPHER_CTX *, const EVP_CIPHER *, unsigned char **, int *, unsigned char *, EVP_PKEY **, int))dlsym (handle, "EVP_SealInit")) == NULL) goto error;
   if ((sec_EVP_SealFinal = (int (*)(EVP_CIPHER_CTX *, unsigned char *, int *))dlsym (handle, "EVP_SealFinal")) == NULL) goto error;
   if ((sec_EVP_SignFinal = (int (*)(EVP_MD_CTX *, unsigned char *, unsigned int *, EVP_PKEY *))dlsym (handle, "EVP_SignFinal")) == NULL) goto error;
   sec_EVP_SignInit = sec_EVP_DigestInit;
   sec_EVP_SignUpdate = sec_EVP_DigestUpdate;
   if ((sec_EVP_VerifyFinal = (int (*)(EVP_MD_CTX *, unsigned char *, unsigned int, EVP_PKEY *))dlsym (handle, "EVP_VerifyFinal")) == NULL) goto error;
   sec_EVP_VerifyInit = sec_EVP_DigestInit;
   sec_EVP_VerifyUpdate = sec_EVP_DigestUpdate;
   if ((sec_i2d_ASN1_UTCTIME = (int (*)(ASN1_UTCTIME *, unsigned char **))dlsym (handle, "i2d_ASN1_UTCTIME")) == NULL) goto error;
   if ((sec_i2d_X509 = (int (*)(X509 *, unsigned char**))dlsym (handle, "i2d_X509")) == NULL) goto error;
   if ((sec_OBJ_nid2obj = (ASN1_OBJECT *(*)(int))dlsym (handle, "OBJ_nid2obj")) == NULL) goto error;
   if ((sec_OPENSSL_free = (void (*)(void *))dlsym (handle, "CRYPTO_free")) == NULL) goto error;
   if ((_sec_OPENSSL_malloc = (void *(*)(int, const char *, int))dlsym (handle, "CRYPTO_malloc")) == NULL) goto error;
#ifdef SSLEAY_MACROS
   if ((_sec_PEM_read_PrivateKey = (char *(*)(char *(*)(), const char *, FILE *, char **, pem_password_cb *, void *))dlsym (handle, "PEM_ASN1_read")) == NULL) goto error;
   if ((sec_d2i_PrivateKey = (EVP_PKEY *(*)(int type, EVP_PKEY **a, unsigned char **pp, long length))dlsym (handle, "d2i_PrivateKey")) == NULL) goto error;
   _sec_PEM_read_X509 = _sec_PEM_read_PrivateKey;
#else
   if ((sec_PEM_read_PrivateKey = (EVP_PKEY *(*)(FILE *, EVP_PKEY **, pem_password_cb *, void *))dlsym (handle, "PEM_read_PrivateKey")) == NULL) goto error;
   if ((sec_PEM_read_X509 = (X509 *(*)(FILE *, X509 **, pem_password_cb *, void *))dlsym (handle, "PEM_read_X509")) == NULL) goto error;
#endif
   if ((sec_RAND_load_file = (int (*)(const char *, long))dlsym (handle, "RAND_load_file")) == NULL) goto error;
   if ((sec_RAND_pseudo_bytes = (int (*)(unsigned char *, int))dlsym (handle, "RAND_pseudo_bytes")) == NULL) goto error;
   if ((sec_RAND_status = (int (*)(void))dlsym (handle, "RAND_status")) == NULL) goto error;
   if ((sec_RSA_private_decrypt = (int (*)(int, const unsigned char *, unsigned char *, RSA *, int))dlsym (handle, "RSA_private_decrypt")) == NULL) goto error;
   if ((sec_RSA_public_encrypt = (int (*)(int, const unsigned char *, unsigned char *, RSA *, int))dlsym (handle, "RSA_public_encrypt")) == NULL) goto error;
   if ((sec_X509_STORE_CTX_get_current_cert = (X509 *(*)(X509_STORE_CTX *ctx))dlsym (handle, "X509_STORE_CTX_get_current_cert")) == NULL) goto error;
   if ((sec_X509_cmp_current_time = (int (*)(ASN1_TIME *))dlsym (handle, "X509_cmp_current_time")) == NULL) goto error;
   if ((sec_X509_free = (void (*)(X509 *))dlsym (handle, "X509_free")) == NULL) goto error;
   if ((sec_X509_get_pubkey = (EVP_PKEY *(*)(X509 *x))dlsym (handle, "X509_get_pubkey")) == NULL) goto error;
   if ((sec_X509_get_pubkey_parameters = (int (*)(EVP_PKEY *, STACK_OF(X509) *))dlsym (handle, "X509_get_pubkey_parameters")) == NULL) goto error;
   if ((sec_X509_get_subject_name = (X509_NAME *(*)(X509 *))dlsym (handle, "X509_get_subject_name")) == NULL) goto error;
   if ((sec_X509_gmtime_adj = (ASN1_TIME *(*)(ASN1_TIME *, long))dlsym (handle, "X509_gmtime_adj")) == NULL) goto error;
   if ((sec_X509_print_fp = (int (*)(FILE *, X509 *))dlsym (handle, "X509_print_fp")) == NULL) goto error;
   if ((sec_X509_NAME_get_text_by_OBJ = (int (*)(X509_NAME *, ASN1_OBJECT *, char *, int))dlsym (handle, "X509_NAME_get_text_by_OBJ")) == NULL) goto error;
   if ((sec_X509_NAME_oneline = (char *(*)(X509_NAME *, char *, int))dlsym (handle, "X509_NAME_oneline")) == NULL) goto error;
   if ((sec_X509_STORE_CTX_cleanup = (void (*)(X509_STORE_CTX *))dlsym (handle, "X509_STORE_CTX_cleanup")) == NULL) goto error;
   if ((sec_X509_STORE_CTX_get_error = (int (*)(X509_STORE_CTX *))dlsym (handle, "X509_STORE_CTX_get_error")) == NULL) goto error;
   if ((sec_X509_STORE_CTX_init = (int (*)(X509_STORE_CTX *, X509_STORE *, X509 *, STACK_OF(X509) *))dlsym (handle, "X509_STORE_CTX_init")) == NULL) goto error;
   if ((sec_X509_STORE_free = (void (*)(X509_STORE *))dlsym (handle, "X509_STORE_free")) == NULL) goto error;
   if ((sec_X509_STORE_load_locations = (int (*)(X509_STORE *, const char *, const char *))dlsym (handle, "X509_STORE_load_locations")) == NULL) goto error;
   if ((sec_X509_STORE_new = (X509_STORE *(*)(void))dlsym (handle, "X509_STORE_new")) == NULL) goto error;
   if ((sec_X509_verify_cert = (int (*)(X509_STORE_CTX *))dlsym (handle, "X509_verify_cert")) == NULL) goto error;
#else
   sec_ASN1_UTCTIME_free = ASN1_UTCTIME_free;
   sec_ASN1_UTCTIME_new = ASN1_UTCTIME_new;
   sec_ASN1_UTCTIME_print = ASN1_UTCTIME_print;
   sec_BIO_free = BIO_free;
   sec_BIO_new_file = BIO_new_file;
   sec_BIO_new_fp = BIO_new_fp;
   sec_BIO_read = BIO_read;
   sec_BIO_write = BIO_write;
   sec_CRYPTO_get_lock_name = CRYPTO_get_lock_name;
   sec_CRYPTO_num_locks = CRYPTO_num_locks;
   sec_CRYPTO_set_id_callback = CRYPTO_set_id_callback;
   sec_CRYPTO_set_locking_callback = CRYPTO_set_locking_callback;
   sec_d2i_ASN1_UTCTIME = d2i_ASN1_UTCTIME;
   sec_d2i_X509 = d2i_X509;
   sec_ERR_clear_error = ERR_clear_error;
   sec_ERR_error_string = ERR_error_string;
   sec_ERR_get_error = ERR_get_error;
   sec_ERR_load_crypto_strings = ERR_load_crypto_strings;
   sec_EVP_add_cipher = EVP_add_cipher;
   sec_EVP_add_digest = EVP_add_digest;
   _sec_EVP_add_digest_alias = OBJ_NAME_add;
   sec_EVP_CIPHER_CTX_cleanup = EVP_CIPHER_CTX_cleanup;
   sec_EVP_DecryptFinal = EVP_DecryptFinal;
   sec_EVP_DecryptInit = EVP_DecryptInit;
   sec_EVP_DecryptUpdate = EVP_DecryptUpdate;
   sec_EVP_DigestInit = EVP_DigestInit;
   sec_EVP_DigestFinal = EVP_DigestFinal;
   sec_EVP_DigestUpdate = EVP_DigestUpdate;
   sec_EVP_EncryptInit = EVP_EncryptInit;
   sec_EVP_EncryptUpdate = EVP_EncryptUpdate;
   sec_EVP_EncryptFinal = EVP_EncryptFinal;
   sec_EVP_md5 = EVP_md5;
   sec_EVP_OpenInit = EVP_OpenInit;
   sec_EVP_OpenFinal = EVP_OpenFinal;
   sec_EVP_OpenUpdate = EVP_DecryptUpdate;
   sec_EVP_PKEY_free = EVP_PKEY_free;
   sec_EVP_PKEY_size = EVP_PKEY_size;
   sec_EVP_rc4 = EVP_rc4;
   sec_EVP_SealInit = EVP_SealInit;
   sec_EVP_SealFinal = EVP_SealFinal;
   sec_EVP_SignFinal = EVP_SignFinal;
   sec_EVP_SignInit = EVP_DigestInit;
   sec_EVP_SignUpdate = EVP_DigestUpdate;
   sec_EVP_VerifyFinal = EVP_VerifyFinal;
   sec_EVP_VerifyInit = EVP_DigestInit;
   sec_EVP_VerifyUpdate = EVP_DigestUpdate;
   sec_i2d_ASN1_UTCTIME = i2d_ASN1_UTCTIME;
   sec_i2d_X509 = i2d_X509;
   sec_OBJ_nid2obj = OBJ_nid2obj;
   sec_OPENSSL_free = CRYPTO_free;
   _sec_OPENSSL_malloc = CRYPTO_malloc;
#ifdef SSLEAY_MACROS
   _sec_PEM_read_PrivateKey = PEM_ASN1_read;
   sec_d2i_PrivateKey = d2i_PrivateKey;
   _sec_PEM_read_X509 = PEM_ASN1_read;
#else
   sec_PEM_read_PrivateKey = PEM_read_PrivateKey;
   sec_PEM_read_X509 = PEM_read_X509;
#endif
   sec_RAND_load_file = RAND_load_file;
   sec_RAND_pseudo_bytes = RAND_pseudo_bytes;
   sec_RAND_status = RAND_status;
   sec_RSA_private_decrypt = RSA_private_decrypt;
   sec_RSA_public_encrypt = RSA_public_encrypt;
   sec_X509_STORE_CTX_get_current_cert = X509_STORE_CTX_get_current_cert;
   sec_X509_cmp_current_time = X509_cmp_current_time;
   sec_X509_free = X509_free;
   sec_X509_get_pubkey = X509_get_pubkey;
   sec_X509_get_pubkey_parameters = X509_get_pubkey_parameters;
   sec_X509_get_subject_name = X509_get_subject_name;
   sec_X509_gmtime_adj = X509_gmtime_adj;
   sec_X509_print_fp = X509_print_fp;
   sec_X509_NAME_get_text_by_OBJ = X509_NAME_get_text_by_OBJ;
   sec_X509_NAME_oneline = X509_NAME_oneline;
   sec_X509_STORE_CTX_cleanup = X509_STORE_CTX_cleanup;
   sec_X509_STORE_CTX_get_error = X509_STORE_CTX_get_error;
   sec_X509_STORE_CTX_init = X509_STORE_CTX_init;
   sec_X509_STORE_free = X509_STORE_free;
   sec_X509_STORE_load_locations = X509_STORE_load_locations;
   sec_X509_STORE_new = X509_STORE_new;
   sec_X509_verify_cert = X509_verify_cert;
#endif

   DEXIT;   
   return 0;

   error:
      DEXIT;
      return 2;
}
