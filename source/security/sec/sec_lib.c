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

#include "sge_unistd.h"
#include "commlib.h"
#include "sge.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_arch.h"
#include "sge_prognames.h" 
#include "sge_me.h" 
#include "basis_types.h"
#include "sge_gdi_intern.h"
#include "sge_stat.h"
#include "sge_secL.h"
#include "sge_getpwnam.h"
#include "sge_arch.h"
#include "sge_stdlib.h"
#include "msg_sec.h"
#include "msg_gdilib.h"
#include "msg_utilib.h"

#include "sec_crypto.h"          /* lib protos      */
#include "sec_lib.h"             /* lib protos      */

#define CHALL_LEN       16
#define ValidMinutes    10          /* expiry of connection        */
#define SGESecPath      ".sge"
#define CaKey           "cakey.pem"
#define CaCert          "cacert.pem"
#define CA_DIR          "common/sgeCA"
#define CA_LOCAL_DIR    "/var/sgeCA"
#define UserKey         "key.pem"
#define UserCert        "cert.pem"
#define RandFile        "rand.seed"
#define ReconnectFile   "private/reconnect.dat"


#define INC32(a)        (((a) == 0xffffffff)? 0:(a)+1)

typedef struct gsd_str {
   EVP_CIPHER *cipher;
   EVP_MD *digest;
   int crypt_space;
   int block_len;
   u_char *key_mat;
   u_long32 key_mat_len;
   EVP_PKEY *private_key;
   X509 *x509;
   char uniqueIdentifier[BUFSIZ];
   u_long32 connid;
   int is_daemon;
   int connect;
   u_long32 seq_send;
   u_long32 seq_receive;
   ASN1_UTCTIME *refresh_time;
} GlobalSecureData;


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


/*
** prototypes
*/
static int sec_set_encrypt(int tag);
static int sec_files(void);
static int sec_is_daemon(const char *progname);

static int sec_announce_connection(GlobalSecureData *gsd, const char *tocomproc, const char *tohost);
static int sec_respond_announce(char *commproc, u_short id, char *host, char *buffer, u_long32 buflen);
static int sec_handle_announce(char *comproc, u_short id, char *host, char *buffer, u_long32 buflen);
static int sec_encrypt(sge_pack_buffer *pb, const char *buf, int buflen);
static int sec_decrypt(char **buffer, u_long32 *buflen, char *host, char *commproc, int id);
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


static void sec_error(void);
static int sec_send_err(char *commproc, int id, char *host, sge_pack_buffer *pb, const char *err_msg);
static int sec_set_connid(char **buffer, int *buflen);
static int sec_get_connid(char **buffer, u_long32 *buflen);
static int sec_update_connlist(const char *host, const char *commproc, int id);
static int sec_set_secdata(const char *host, const char *commproc, int id);
static int sec_insert_conn2list(u_long32 connid, char *host, char *commproc, int id, const char *uniqueIdentifier);
static void sec_keymat2list(lListElem *element);
static void sec_list2keymat(lListElem *element);
static int sec_verify_callback(int ok, X509_STORE_CTX *ctx);
static void sec_setup_path(int is_daemon);

static sec_exit_func_type install_sec_exit_func(sec_exit_func_type);

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

static void debug_print_ASN1_UTCTIME(char *label, ASN1_UTCTIME *time)
{
   BIO *out;
   printf("%s", label);
   out = BIO_new_fp(stdout, BIO_NOCLOSE);
   ASN1_UTCTIME_print(out, time);   
   BIO_free(out);
   printf("\n");
}   
   
/* #define SEC_DEBUG    */
#ifdef SEC_DEBUG
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

#  define DEBUG_PRINT(x, y)              printf(x,y)
#  define DEBUG_PRINT_BUFFER(x,y,z)      debug_print_buffer(x,y,z)
#else
#  define DEBUG_PRINT(x,y)
#  define DEBUG_PRINT_BUFFER(x,y,z)
#endif /* SEC_DEBUG */

/*
** FIXME
*/
#define PREDEFINED_PW         "troet"

/* #define SECRET_KEY_ERROR    */


sec_exit_func_type sec_exit_func = NULL;

/*
** global secure data
*/
static GlobalSecureData gsd;

static u_long32 connid_counter = 0;
static lList *conn_list = NULL;

static char *ca_key_file;
static char *ca_cert_file;
static char *key_file;
static char *cert_file;
static char *reconnect_file;
static char *rand_file;



/*
** NAME
**   sec_init
**
** SYNOPSIS
**   #include "sec_lib.h"
**
**   int sec_init(progname)
**   char *progname - the program name of the calling program
**
** DESCRIPTION
**   This function initialices the security related data in the global
**   gsd-struct. This must be done only once, typically before the
**   enroll to the commd.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
*/
int sec_init(const char *progname) 
{
   static int sec_initialized = 0;
   char *randfile = NULL;
   SGE_STRUCT_STAT file_info;

   DENTER(GDI_LAYER,"sec_init");

   if (sec_initialized) {
      DEXIT;
      return 0;
   }   

   /*
   ** set everything to zero
   */
   memset(&gsd, 0, sizeof(gsd));

   /*
   ** initialize error strings
   */
   ERR_clear_error();
   ERR_load_crypto_strings();

   /*
   ** load all algorithms and digests
   */
/*    OpenSSL_add_all_algorithms(); */
   EVP_add_cipher(EVP_rc4());
   EVP_add_digest(EVP_md5());
   EVP_add_digest_alias(SN_md5,"ssl2-md5");
   EVP_add_digest_alias(SN_md5,"ssl3-md5");


   /* 
   ** FIXME: am I a sge daemon? -> is_daemon() should be implemented
   */
   gsd.is_daemon = sec_is_daemon(progname);

   /* 
   ** setup the filenames of randfile, certificates, keys, etc.
   */
   sec_setup_path(gsd.is_daemon);

   /* 
   ** seed PRNG, /dev/random is used if possible
   ** if RANDFILE is set it is used
   ** otherwise rand_file is used
   */
   if (!RAND_status()) {
      randfile = getenv("RANDFILE");
      if (!SGE_STAT(randfile, &file_info)) {
         RAND_load_file(randfile, 2048);
      } else if (rand_file) {   
         RAND_load_file(rand_file, 2048); 
      } else {
         ERROR((SGE_EVENT, MSG_SEC_RANDFILENOTSET));
         DEXIT;
         return -1;
      }
   }

   /* 
   ** FIXME: init all the other stuff
   */
   gsd.digest = EVP_md5();
   gsd.cipher = EVP_rc4();
/*    gsd.cipher = EVP_des_cbc(); */
/*    gsd.cipher = EVP_enc_null(); */

   gsd.key_mat_len = GSD_KEY_MAT_32;
   gsd.key_mat = (u_char *) malloc(gsd.key_mat_len);
   if (!gsd.key_mat){
      ERROR((SGE_EVENT, MSG_MEMORY_MALLOCFAILED));
      DEXIT;
      return -1;
   }
   if(!strcmp(progname, prognames[QMASTER])) {
      /* time after qmaster clears the connection list   */
      gsd.refresh_time = X509_gmtime_adj(gsd.refresh_time, 0);
   }
   else {
      /* time after client makes a new announce to master   */
      gsd.refresh_time = X509_gmtime_adj(gsd.refresh_time, (long) 60*ValidMinutes);
   }   

   gsd.connid = gsd.seq_send = gsd.connect = gsd.seq_receive = 0;

   /* 
   ** read security related files
   */
   if(sec_files()){
      DEXIT;
      return -1;
   }

#ifdef SEC_RECONNECT   
#if 0
   /* 
   ** FIXME: install sec_exit functions for utilib
   */
   if(!strcmp(progname, prognames[QMASTER])){
      install_sec_exit_func(sec_dump_connlist);
      if (sec_undump_connlist())
                WARNING((SGE_EVENT,"warning: Can't read ConnList\n"));
   }
   else 
#endif   
   if(!gsd.is_daemon)
      install_sec_exit_func(sec_write_data2hd);

   /*
   ** read reconnect data
   ** FIXME
   */
   if (!gsd.is_daemon)
      sec_reconnect();
#else
   install_sec_exit_func(NULL);
#endif      

   
   DPRINTF(("====================[  CSP SECURITY  ]===========================\n"));

   sec_initialized = 1;

   DEXIT;
   return 0;
}

/*
** NAME
**   sec_send_message
**
** SYNOPSIS
**      #include "sec_lib.h"
**
**   sec_send_message(synchron,tocomproc,toid,tohost,tag,buffer,buflen,mid)
**   int synchron    - transfer modus,
**   char *tocomproc - name destination program,
**   int toid        - id of communication,
**   char *tohost    - destination host,
**   int tag         - tag of message,
**   char *buffer    - buffer which contains the message to send
**   int buflen      - lenght of message,
**   u_long32  *mid    - id for asynchronous messages;
**
** DESCRIPTION
**   This function is used instead of the normal send_message call from
**   the commlib. It checks if a client has to announce to the master and 
**   if the message should be encrypted and then it sends the message.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
*/

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
   if (me.who != QMASTER) {
      if (sec_announce_connection(&gsd, tocomproc,tohost)) {
         ERROR((SGE_EVENT,"failed announce\n"));
         DEXIT;
         return SEC_ANNOUNCE_FAILED;
      }
   }   
      
   if (sec_set_encrypt(tag)) {
      if (me.who == QMASTER && conn_list) {
         /*
         ** set the corresponding key and connection information
         ** in the connection list for commd triple 
         ** (tohost, tocomproc, toid)
         */
         if (sec_set_secdata(tohost,tocomproc,toid)) {
            ERROR((SGE_EVENT, "failed set security data\n"));
            DEXIT;
            return SEC_SEND_FAILED;
         }
      }
      if (sec_encrypt(&pb, buffer, buflen)) {
         ERROR((SGE_EVENT,"failed encrypt message\n"));
         DEXIT;
         return SEC_SEND_FAILED;
      }
   }
   else if (me.who != QMASTER) {
      if (sec_set_connid(&buffer, &buflen)) {
         ERROR((SGE_EVENT,"failed set connection ID\n"));
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

/*
** NAME
**   sec_receive_message
**
** SYNOPSIS
**      #include "sec_lib.h"
**
**   int sec_receive_message(fromcommproc,fromid,fromhost,tag,
**               buffer,buflen,synchron)
**   char *fromcommproc - name of progname from the source,
**   u_short *fromid    - id of message,
**   char *fromhost     - host name of source,
**   int *tag           - tag of received message,
**   char **buffer      - buffer contains the received message,
**   u_long32 *buflen     - length of message,
**   int synchron       - transfer modus;
**
** DESCRIPTION
**   This function is used instead of the normal receive message of the
**   commlib. It handles an announce and decrypts the message if necessary.
**   The qmaster writes this connection to his connection list.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
*/
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
         ERROR((SGE_EVENT,"failed handle announce for (%s:%s:%d)\n",
                  fromhost, fromcommproc, *fromid));
         DEXIT;
         return SEC_ANNOUNCE_FAILED;
      }
   }
   else if (sec_set_encrypt(*tag)) {
      DEBUG_PRINT_BUFFER("Encrypted incoming message", 
                         (unsigned char*) (*buffer), (int)(*buflen));

      if (sec_decrypt(buffer,buflen,fromhost,fromcommproc,*fromid)) {
         ERROR((SGE_EVENT,"failed decrypt message (%s:%s:%d)\n",
                  fromhost,fromcommproc,*fromid));
         if (sec_handle_announce(fromcommproc,*fromid,fromhost,NULL,0))
            ERROR((SGE_EVENT,"failed handle decrypt error\n"));
         DEXIT;
         return SEC_RECEIVE_FAILED;
      }
   }
   else if (me.who == QMASTER) {
      if (sec_get_connid(buffer,buflen) ||
               sec_update_connlist(fromhost,fromcommproc,*fromid)) {
         ERROR((SGE_EVENT, "failed get connection ID\n"));
         DEXIT;
         return SEC_RECEIVE_FAILED;
      }
   }

   DEXIT;
   return 0;
}


/*
** NAME
**   sec_set_encrypt
**
** SYNOPSIS
**   static int sec_set_encrypt(tag)
**   int tag - message tag of received message
**
** DESCRIPTION
**   This function decides within a single switch by means of the
**   message tag if a message is/should be encrypted. ATTENTION:
**   every new tag, which message is not to be encrypted, must
**   occur within this switch. Tags who are not there are encrypted
**   by default. 
**   (definition of tags <gridengine>/source/libs/gdi/sge_gdi_intern.h)
**
**
** RETURN VALUES
**   1   message with tag 'tag' is/should be encrypted
**   0   message is not encrypted
*/   
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

/*
** NAME
**   sec_files
**
** SYNOPSIS
**
**   static int sec_files()
**
** DESCRIPTION
**   This function reads security related files from hard disk
**   (key- and certificate-file) and verifies the own certificate.
**   It's normally called from the following sec_init function.
**
** RETURN VALUES
**   0    on success
**   -1   on failure
*/
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
   gsd.x509 = PEM_read_X509(fp, NULL, NULL, PREDEFINED_PW);
   if (gsd.x509 == NULL) {
      sec_error();
      i = -1;
      goto error;
   }
   fclose(fp);

   if (!sec_verify_certificate(gsd.x509)) {
      ERROR((SGE_EVENT, MSG_SEC_FAILEDVERIFYOWNCERT));
      sec_error();
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
   gsd.private_key = PEM_read_PrivateKey(fp, NULL, NULL, PREDEFINED_PW);
   fclose(fp);

   if (!gsd.private_key) {
      sec_error();
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

/*
** verify certificate
*/
static int sec_verify_certificate(X509 *cert)
{
   X509_STORE *ctx = NULL;
   X509_STORE_CTX csc;
   int ret;
   int err;
   char *x509_name = NULL;

   DENTER(GDI_LAYER, "sec_verify_certificate");

   x509_name = X509_NAME_oneline(X509_get_subject_name(cert), NULL, 0);
   DPRINTF(("subject: %s\n", x509_name));
   free(x509_name);
   DPRINTF(("ca_cert_file: %s\n", ca_cert_file));
   ctx = X509_STORE_new();
   X509_STORE_load_locations(ctx, ca_cert_file, NULL);
   X509_STORE_CTX_init(&csc, ctx, cert, NULL);
   if (getenv("SGE_CERT_DEBUG")) {
      X509_STORE_set_verify_cb_func(&csc, sec_verify_callback);
   }   
   ret = X509_verify_cert(&csc);
   err = X509_STORE_CTX_get_error(&csc);
   X509_STORE_free(ctx);
   X509_STORE_CTX_cleanup(&csc);
   
   switch (err) {
      case X509_V_ERR_CERT_NOT_YET_VALID:
         ERROR((SGE_EVENT, "certificate not yet valid\n"));
         break;

      case X509_V_ERR_CERT_HAS_EXPIRED:
         ERROR((SGE_EVENT, "certificate has expired\n"));
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
*/
static int sec_dump_connlist()
{
   FILE    *fp;

   if (me.who == QMASTER) {
      fp = fopen("sec_connlist.dat", "w");
      if (fp) {
         lDumpList(fp, conn_list, 0);
         fclose(fp);
      }
      else 
         return -1;

      fp = fopen("sec_connid.dat","w");
      if (fp) {
         fprintf(fp, u32, connid_counter);
         fclose(fp);
      }
      else
         return -1;
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
*/
static int sec_undump_connlist()
{
   FILE    *fp;

   if (me.who == QMASTER) {
      fp = fopen("sec_connlist.dat","r");
      if (fp){
         conn_list = lUndumpList(fp,NULL,NULL);
         fclose(fp);
      } 
      else
         return -1;

         
      fp = fopen("sec_connid.dat","r");
      if (fp){
         fscanf(fp, u32, &connid_counter);
         fclose(fp);
      }
      else
         return -1;
   } 
   return 0;
}

#endif

/*
** NAME
**
** SYNOPSIS
**
**   static int sec_announce_connection(commproc,cell)
**   char *commproc - the program name of the destination, 
**   char *cell     - the host name of the destination;
**
** DESCRIPTION
**   This function is used from clients to announce to the master and to
**   receive his responce. It should be called before the first 
**   communication and its aim is to negotiate a secret key which is
**   used to encrypt the further communication.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
*/
static int sec_announce_connection(
GlobalSecureData *gsd,
const char *tocomproc,
const char *tohost 
) {
   EVP_PKEY *master_key = NULL;
   int i;
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
   char fromhost[MAXHOSTLEN];
   char fromcommproc[MAXCOMPONENTLEN];
   int fromtag;
   u_short fromid;
   u_short compressed = 0;
   u_long32 dummymid;
   EVP_MD_CTX ctx;
   EVP_CIPHER_CTX ectx;
   
   DENTER(GDI_LAYER, "sec_announce_connection");

   /*
   ** try to reconnect
   */
   if (gsd->connect) {
/* debug_print_ASN1_UTCTIME("gsd.refresh_time: ", gsd->refresh_time); */
      if (gsd->refresh_time && (X509_cmp_current_time(gsd->refresh_time) > 0)) {
         DPRINTF(("++++ Connection %d still valid\n", (int) gsd->connid));      
         DEXIT;      
         return 0;
      }   
      DPRINTF(("---- Connection needs sec_announce_connection\n"));
   }

   /* 
   ** send sec_announce
   */
   gsd->seq_send = gsd->seq_receive = 0;

   /* 
   ** write x509 to buffer
   */
   x509_len = i2d_X509(gsd->x509, NULL);
   tmp = x509_buf = (unsigned char *) malloc(x509_len);
   x509_len = i2d_X509(gsd->x509, &tmp);
   
   if (!x509_len) {
      ERROR((SGE_EVENT,"i2d_x509 failed\n"));
      i=-1;
      goto error;
   }

   /* 
   ** build challenge and challenge mac
   */
   RAND_pseudo_bytes(challenge, CHALL_LEN);

   /* 
   ** prepare packing buffer
   */
   if ((i=init_packbuffer(&pb, 0, 0))) {
      ERROR((SGE_EVENT, MSG_SEC_INITPACKBUFFERFAILED));
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
   DPRINTF(("send announce to=(%s:%s)\n",tohost,tocomproc));
   if (send_message(COMMD_SYNCHRON, tocomproc, 0, tohost, 
                    TAG_SEC_ANNOUNCE, pb.head_ptr, pb.bytes_used, 
                    &dummymid, 0)) {
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
   fromhost[0] = '\0';
   fromcommproc[0] = '\0';
   fromid = 0;
   fromtag = 0;
   if ((i = receive_message(fromcommproc, &fromid, fromhost, &fromtag,
                        &pb.head_ptr, (u_long32 *) &pb.mem_size, 
                        COMMD_SYNCHRON, &compressed))) {
      ERROR((SGE_EVENT, MSG_SEC_RESPONSEFAILED_SISIS,
             fromcommproc, fromid, fromhost, fromtag, cl_errstr(i)));
      i = -1;
      goto error;
   }

   pb.cur_ptr = pb.head_ptr;
   pb.bytes_used = 0;

   DPRINTF(("received announcement response from=(%s:%s:%d) tag=%d buflen=%d\n",
           fromhost, fromcommproc, fromid, fromtag, pb.mem_size));

   /* 
   ** manage error or wrong tags
   */
   if (fromtag == TAG_SEC_ERROR){
      ERROR((SGE_EVENT, MSG_SEC_MASTERERROR));
      ERROR((SGE_EVENT, pb.cur_ptr));
      i=-1;
      goto error;
   }
   if (fromtag != TAG_SEC_RESPOND) {
      ERROR((SGE_EVENT, MSG_SEC_UNEXPECTEDTAG));
      i=-1;
      goto error;
   }

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
   x509_master = d2i_X509(NULL, &tmp, x509_master_len);
   if (!x509_master) {
      ERROR((SGE_EVENT, MSG_SEC_MASTERCERTREADFAILED));
      sec_error();
      i = -1;
      goto error;
   }
   /*
   ** free x509_master_buf
   */
   free(x509_master_buf);
   x509_master_buf = NULL;


   if (!sec_verify_certificate(x509_master)) {
      sec_error();
      i = -1;
      goto error;
   }
   DPRINTF(("Master certificate is ok\n"));

   /*
   ** extract public key 
   */
   master_key = X509_get_pubkey(x509_master);
   
   if(!master_key){
      ERROR((SGE_EVENT, MSG_SEC_MASTERGETPUBKEYFAILED));
      sec_error();
      i=-1;
      goto error;
   }

   /* 
   ** decrypt challenge with master public key
   */
   EVP_VerifyInit(&ctx, gsd->digest);
   EVP_VerifyUpdate(&ctx, challenge, CHALL_LEN);
   if (!EVP_VerifyFinal(&ctx, enc_challenge, chall_enc_len, master_key)) {
      ERROR((SGE_EVENT, MSG_SEC_MASTERBADCHALLENGE));
      i = -1;
      goto error;
   }   
   
   /*
   ** decrypt secret key
   */
   if (!EVP_OpenInit(&ectx, gsd->cipher, enc_key, enc_key_len, iv, gsd->private_key)) {
      ERROR((SGE_EVENT,"EVP_OpenInit failed decrypt keys\n"));
      EVP_CIPHER_CTX_cleanup(&ectx);
      sec_error();
      i=-1;
      goto error;
   }
   if (!EVP_OpenUpdate(&ectx, gsd->key_mat, (int*) &(gsd->key_mat_len), enc_key_mat, (int) enc_key_mat_len)) {
      ERROR((SGE_EVENT,"EVP_OpenUpdate failed decrypt keys\n"));
      EVP_CIPHER_CTX_cleanup(&ectx);
      sec_error();
      i = -1;
      goto error;
   }
   EVP_OpenFinal(&ectx, gsd->key_mat, (int*) &(gsd->key_mat_len));
   EVP_CIPHER_CTX_cleanup(&ectx);
   /* FIXME: problem in EVP_* lib for stream ciphers */
   gsd->key_mat_len = enc_key_mat_len;   

   DEBUG_PRINT_BUFFER("Received key material", gsd->key_mat, gsd->key_mat_len);
   
   /*
   ** set own connid to assigned connid
   ** set refresh_time to current time + 60 * ValidMinutes
   ** set connect flag
   */
   gsd->connect = 1;
   gsd->connid = connid;
   gsd->refresh_time = X509_gmtime_adj(gsd->refresh_time, 
                                       (long) 60*ValidMinutes);


   i=0;

   error:
   clear_packbuffer(&pb);
   if (x509_buf)
      free(x509_buf);
   if (x509_master_buf)
      free(x509_master_buf);
   if (x509_master)
      X509_free(x509_master);
   if (enc_challenge) 
      free(enc_challenge);
   if (enc_key) 
      free(enc_key);
   if (iv) 
      free(iv);
   if (enc_key_mat) 
      free(enc_key_mat);
   if (master_key)
      EVP_PKEY_free(master_key);

   DEXIT;
   return i;
}

/*
** NAME
**    sec_respond_announce
**
** SYNOPSIS
**
**   static int sec_respond_announce(commproc,id,host,buffer,buflen)
**   char *commproc - the program name of the source of the message, 
**   u_short id     - the id of the communication, 
**   char *host     - the host name of the sender, 
**   char *buffer   - this buffer contains the incoming message, 
**   u_long32 buflen  - the length of the message;
**   
** DESCRIPTION
**   This function handles the announce of a client and sends a responce
**   to him which includes the secret key enrypted.
**   
** RETURN VALUES
**      0       on success
**      -1      on failure
*/
static int sec_respond_announce(char *commproc, u_short id, char *host, 
                         char *buffer, u_long32 buflen)
{
   EVP_PKEY *public_key[1];
   u_long32 i = 0, x509_len = 0, enc_key_mat_len = 0;
   u_long32 chall_enc_len = 0;
   unsigned char *x509_buf = NULL, *tmp = NULL;
   u_char *enc_key_mat=NULL, *challenge=NULL, *enc_challenge=NULL;
   X509 *x509 = NULL;
   sge_pack_buffer pb, pb_respond;
   u_long32 dummymid;
   int public_key_size;
   unsigned char iv[EVP_MAX_IV_LENGTH];
   unsigned char *ekey[1];
   int ekeylen;
   EVP_CIPHER_CTX ectx;
   EVP_MD_CTX ctx;
   char uniqueIdentifier[BUFSIZ];

   DENTER(GDI_LAYER, "sec_respond_announce");

   /* 
   ** prepare packbuffer pb for unpacking message
   */
   if ((i=init_packbuffer_from_buffer(&pb, buffer, buflen, 0))) {
      ERROR((SGE_EVENT, MSG_SEC_INITPACKBUFFERFAILED));
      goto error;
   }

   /* 
   ** prepare packbuffer for sending message
   */
   if ((i=init_packbuffer(&pb_respond, 0, 0))) {
      ERROR((SGE_EVENT, MSG_SEC_INITPACKBUFFERFAILED));
      sec_send_err(commproc, id, host, &pb_respond, 
                   MSG_SEC_INITPACKBUFFERFAILED);
      goto error;
   }

   /* 
   ** read announce from packing buffer
   */
   if (sec_unpack_announce(&x509_len, &x509_buf, &challenge, &pb)) {
      ERROR((SGE_EVENT, MSG_SEC_UNPACKANNOUNCEFAILED));
      sec_send_err(commproc, id, host, &pb_respond, 
                   MSG_SEC_UNPACKANNOUNCEFAILED);
      goto error;
   }

   /* 
   ** read x509 certificate
   */
   tmp = x509_buf;
   x509 = d2i_X509(NULL, &tmp, x509_len);
   free(x509_buf);
   x509_buf = NULL;

   if (!x509){
      ERROR((SGE_EVENT, MSG_SEC_CLIENTCERTREADFAILED));
      sec_error();
      sec_send_err(commproc, id, host, &pb_respond, 
                   MSG_SEC_CLIENTCERTREADFAILED);
      i = -1;
      goto error;
   }

   if (!sec_verify_certificate(x509)) {
      ERROR((SGE_EVENT, MSG_SEC_CLIENTCERTVERIFYFAILED));
      sec_error();
      sec_send_err(commproc, id, host, &pb_respond, 
                   MSG_SEC_CLIENTCERTVERIFYFAILED);
      i = -1;
      goto error;
   }
   DPRINTF(("Client certificate is ok\n"));
      
   /*
   ** reset uniqueIdentifier and get uniqueIdentifier from client cert
   */
   memset(uniqueIdentifier, '\0', BUFSIZ);
   if (X509_NAME_get_text_by_OBJ(X509_get_subject_name(x509), 
      OBJ_nid2obj(NID_uniqueIdentifier), uniqueIdentifier, 
                  sizeof(uniqueIdentifier))) {
      DPRINTF(("UID: %s\n", uniqueIdentifier));
   }   
   
   /*
   ** extract public key 
   */
   public_key[0] = X509_get_pubkey(x509);

   X509_free(x509);
   x509 = NULL;
   
   if(!public_key[0]){
      ERROR((SGE_EVENT, MSG_SEC_CLIENTGETPUBKEYFAILED));
      sec_error();
      sec_send_err(commproc,id,host,&pb_respond, 
                   MSG_SEC_CLIENTGETPUBKEYFAILED);
      i=-1;
      goto error;
   }
   public_key_size = EVP_PKEY_size(public_key[0]);

   /* 
   ** malloc several buffers
   */
   ekey[0] = (u_char *) malloc(public_key_size);
   enc_key_mat = (u_char *) malloc(public_key_size);
   enc_challenge = (u_char *) malloc(public_key_size);

   if (!enc_key_mat ||  !enc_challenge) {
      ERROR((SGE_EVENT, MSG_MEMORY_MALLOCFAILED));
      sec_send_err(commproc,id,host,&pb_respond, MSG_MEMORY_MALLOCFAILED);
      i=-1;
      goto error;
   }
   
   /* 
   ** set connection ID
   */
   gsd.connid = connid_counter;

   /* 
   ** write x509 certificate to buffer
   */
   x509_len = i2d_X509(gsd.x509, NULL);
   tmp = x509_buf = (u_char *) malloc(x509_len);
   x509_len = i2d_X509(gsd.x509, &tmp);

   /*
   ** sign the challenge with master's private key
   */
   EVP_SignInit(&ctx, gsd.digest);
   EVP_SignUpdate(&ctx, challenge, CHALL_LEN);
   if (!EVP_SignFinal(&ctx, enc_challenge, (unsigned int*) &chall_enc_len, gsd.private_key)) {
      ERROR((SGE_EVENT, MSG_SEC_ENCRYPTCHALLENGEFAILED));
      sec_error();
      sec_send_err(commproc, id, host, &pb_respond,
                   MSG_SEC_ENCRYPTCHALLENGEFAILED);
      i = -1;
      goto error;
   }   

   /* 
   ** make key material
   */
   RAND_pseudo_bytes(gsd.key_mat, gsd.key_mat_len);
   DEBUG_PRINT_BUFFER("Sent key material", gsd.key_mat, gsd.key_mat_len);
   DEBUG_PRINT_BUFFER("Sent enc_challenge", enc_challenge, chall_enc_len);
   
   /*
   ** seal secret key with client's public key
   */
   memset(iv, '\0', sizeof(iv));
   if (!EVP_SealInit(&ectx, gsd.cipher, ekey, &ekeylen, iv, public_key, 1)) {
      ERROR((SGE_EVENT, MSG_SEC_SEALINITFAILED));;
      EVP_CIPHER_CTX_cleanup(&ectx);
      sec_error();
      sec_send_err(commproc,id,host,&pb_respond, MSG_SEC_SEALINITFAILED);
      i = -1;
      goto error;
   }
   if (!EVP_EncryptUpdate(&ectx, enc_key_mat, (int *)&enc_key_mat_len, gsd.key_mat, gsd.key_mat_len)) {
      ERROR((SGE_EVENT, MSG_SEC_ENCRYPTKEYFAILED));
      EVP_CIPHER_CTX_cleanup(&ectx);
      sec_error();
      sec_send_err(commproc,id,host,&pb_respond, MSG_SEC_ENCRYPTKEYFAILED);
      i = -1;
      goto error;
   }
   EVP_SealFinal(&ectx, enc_key_mat, (int*) &enc_key_mat_len);

   enc_key_mat_len = gsd.key_mat_len;

   DEBUG_PRINT_BUFFER("Sent ekey[0]", ekey[0], ekeylen);
   DEBUG_PRINT_BUFFER("Sent enc_key_mat", enc_key_mat, enc_key_mat_len);

   /* 
   ** write response data to packing buffer
   */
   i=sec_pack_response(x509_len, x509_buf, chall_enc_len, enc_challenge,
                       gsd.connid, 
                       ekeylen, ekey[0], 
                       EVP_CIPHER_CTX_iv_length(&ectx), iv,
                       enc_key_mat_len, enc_key_mat,
                       &pb_respond);

   EVP_CIPHER_CTX_cleanup(&ectx);

   if (i) {
      ERROR((SGE_EVENT, MSG_SEC_PACKRESPONSEFAILED));
      sec_send_err(commproc, id, host, &pb_respond, MSG_SEC_PACKRESPONSEFAILED);
      goto error;   
   }

   /* 
   ** insert connection to Connection List
   */
   if (sec_insert_conn2list(gsd.connid, host, commproc, id, uniqueIdentifier)) {
      ERROR((SGE_EVENT, MSG_SEC_INSERTCONNECTIONFAILED));
      sec_send_err(commproc, id, host, &pb_respond, 
                   MSG_SEC_INSERTCONNECTIONFAILED);
      goto error;
   }

   /* 
   ** send response
   */
   DPRINTF(("send response to=(%s:%s:%d)\n", host, commproc, id));
   
   if (send_message(0, commproc, id, host, TAG_SEC_RESPOND, pb_respond.head_ptr,
                     pb_respond.bytes_used, &dummymid, 0)) {
      ERROR((SGE_EVENT, MSG_SEC_SENDRESPONSEFAILED_SIS, commproc, id, host));
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
         X509_free(x509);
      if (public_key[0]) 
         EVP_PKEY_free(public_key[0]);

      DEXIT;   
      return i;
}

/*
** NAME
**   sec_encrypt
**
** SYNOPSIS
**
**   static int sec_encrypt(buffer,buflen)
**   char **buffer - buffer contains the message to encrypt and finaly
**            - the encrypted message,
**   int *buflen   - the old and the new message length;
**
** DESCRIPTION
**   This function encrypts a message, calculates the MAC.
** RETURN VALUES
**      0       on success
**      -1      on failure
*/
static int sec_encrypt(
sge_pack_buffer *pb,
const char *inbuf,
int inbuflen 
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

   seq_no = htonl(gsd.seq_send);
   memcpy(seq_str, &seq_no, INTSIZE);

   /*
   ** create digest
   */
   EVP_DigestInit(&mdctx, gsd.digest);
   EVP_DigestUpdate(&mdctx, gsd.key_mat, gsd.key_mat_len);
   EVP_DigestUpdate(&mdctx, seq_str, INTSIZE);
   EVP_DigestUpdate(&mdctx, inbuf, inbuflen);
   EVP_DigestFinal(&mdctx, md_value, &md_len);
   
   /*
   ** malloc outbuf 
   */
   outbuf = malloc(inbuflen + EVP_CIPHER_block_size(gsd.cipher));
   if (!outbuf) {
      ERROR((SGE_EVENT, MSG_MEMORY_MALLOCFAILED));
      i=-1;
      goto error;
   }

   /*
   ** encrypt digest and message
   */
#ifdef SECRET_KEY_ERROR   
   {
      static int count = 0;
      if (++count == 10) {
         RAND_pseudo_bytes(gsd.key_mat, gsd.key_mat_len);
         count = 0;
      }   
   }   
#endif   
   
   memset(iv, '\0', sizeof(iv));
   EVP_EncryptInit(&ctx, gsd.cipher, gsd.key_mat, iv);
   if (!EVP_EncryptUpdate(&ctx, md_value, (int*) &enc_md_len, md_value, md_len)) {
      ERROR((SGE_EVENT, MSG_SEC_ENCRYPTMACFAILED));
      sec_error();
      i=-1;
      goto error;
   }
   if (!EVP_EncryptFinal(&ctx, md_value, (int*) &enc_md_len)) {
      ERROR((SGE_EVENT, MSG_SEC_ENCRYPTMACFAILED));
      sec_error();
      i=-1;
      goto error;
   }
   EVP_CIPHER_CTX_cleanup(&ctx);


   memset(iv, '\0', sizeof(iv));
   EVP_EncryptInit(&ctx, gsd.cipher, gsd.key_mat, iv);
   if (!EVP_EncryptUpdate(&ctx, (unsigned char *) outbuf, (int*)&outlen,
                           (unsigned char*)inbuf, inbuflen)) {
      ERROR((SGE_EVENT, MSG_SEC_ENCRYPTMSGFAILED));
      sec_error();
      i=-1;
      goto error;
   }
   if (!EVP_EncryptFinal(&ctx, (unsigned char*)outbuf, (int*) &outlen)) {
      ERROR((SGE_EVENT, MSG_SEC_ENCRYPTMSGFAILED));
      sec_error();
      i=-1;
      goto error;
   }
   EVP_CIPHER_CTX_cleanup(&ctx);
   if (!outlen)
      outlen = inbuflen;

   /*
   ** initialize packbuffer
   */
   if ((i=init_packbuffer(pb, md_len + outlen, 0))) {
      ERROR((SGE_EVENT, MSG_SEC_INITPACKBUFFERFAILED));
      goto error;
   }


   /*
   ** pack the information into one buffer
   */
   i = sec_pack_message(pb, gsd.connid, md_len, md_value, 
                           (unsigned char *)outbuf, outlen);
   
   if (i) {
      ERROR((SGE_EVENT, MSG_SEC_PACKMSGFAILED));
      goto error;
   }

   gsd.seq_send = INC32(gsd.seq_send);

   if (conn_list) {
      lListElem *element=NULL;
      
      element = lGetElemUlong(conn_list, SEC_ConnectionID, gsd.connid);
      if (!element) {
         ERROR((SGE_EVENT, MSG_SEC_CONNECTIONNOENTRY));
         i = -1;
         goto error;
      }
      lSetUlong(element, SEC_SeqNoSend, gsd.seq_send);
   }   

   i=0;

   error:
      EVP_CIPHER_CTX_cleanup(&ctx);
      if (outbuf)
         free(outbuf);
      DEXIT;
      return i;
}

/*
** NAME
**   sec_handle_announce
**
** SYNOPSIS
**
**   static int sec_handle_announce(commproc,id,host,buffer,buflen)
**   char *commproc - program name of the destination of responce,
**   u_short id     - id of communication,
**   char *host     - destination host,
**   char *buffer   - buffer contains announce,
**   u_long32 buflen  - lenght of announce;
**
** DESCRIPTION
**   This function handles an incoming message with tag TAG_SEC_ANNOUNCE.
**   If I am the master I send a responce to the client, if not I make
**   an announce to the master.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
*/
static int sec_handle_announce(char *commproc, u_short id, char *host, char *buffer, u_long32 buflen)
{
   int i;
   u_long32 mid;
   struct stat file_info;
   int compressed = 0;

   DENTER(GDI_LAYER, "sec_handle_announce");

   if (me.who == QMASTER) {
      if (buffer && buflen) {   
         /* 
         ** someone wants to announce to master
         */
         if (sec_respond_announce(commproc, id, host, buffer,buflen)) {
            ERROR((SGE_EVENT, MSG_SEC_RESPONSEFAILED_SIS, commproc, id, host));
            i = -1;
            goto error;
         }
      }
      else{         
         /* 
         ** a sec_decrypt error has occured
         */
         if (send_message(0,commproc,id,host,TAG_SEC_ANNOUNCE, buffer,buflen,&mid, compressed)) {
            ERROR((SGE_EVENT, MSG_SEC_SUMMONSESFAILED_SIS, commproc, id, host));
            i = -1;
            goto error;
         }
      }
   }
   else if (gsd.is_daemon) {
      gsd.connect = 0;
   }
   else {
      printf("You should reconnect - please try command again!\n");
      gsd.connect = 0;
      if (stat(reconnect_file,&file_info) < 0) {
         i = 0;
         goto error;
      }
      if (remove(reconnect_file)) {
         ERROR((SGE_EVENT,"failed remove reconnect file '%s'\n",
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

/*
** NAME
**   sec_decrypt
**
** SYNOPSIS
**
**   static int sec_decrypt(buffer,buflen,host,commproc,id)
**   char **buffer  - message to decrypt and message after decryption, 
**   u_long32 *buflen - message length before and after decryption,
**   char *host     - name of host,
**   char *commproc - name program name,
**   int id         - id of connection;
**
** DESCRIPTION
**   This function decrypts a message from and to buffer and checks the
**   MAC. 
**
** RETURN VALUES
**      0       on success
**      -1      on failure
*/

static int sec_decrypt(char **buffer, u_long32 *buflen, char *host, char *commproc, int id)
{
   int i;
   u_char *enc_msg = NULL;
   u_char *enc_mac = NULL;
   u_long32 connid, enc_msg_len, enc_mac_len;
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
      ERROR((SGE_EVENT,"failed init_packbuffer_from_buffer\n"));
      goto error;
   }

   /* 
   ** unpack data from packing buffer
   */
   i = sec_unpack_message(&pb, &connid, &enc_mac_len, &enc_mac, 
                          &enc_msg_len, &enc_msg);
   if (i) {
      ERROR((SGE_EVENT,"failed unpack message from buffer\n"));
      packstr(&pb,"Can't unpack your message from buffer\n");
      goto error;
   }

   if (conn_list) {
      /*
      ** for debugging
      */
      if (getenv("SGE_DEBUG_CONNLIST")) {
         lListElem *el;
         ASN1_UTCTIME *utctime = ASN1_UTCTIME_new();
         for_each(el, conn_list) {
            const char *dtime = lGetString(el, SEC_ExpiryDate);
            utctime = d2i_ASN1_UTCTIME(&utctime, (unsigned char**) &dtime, 
                                             strlen(dtime));
            debug_print_ASN1_UTCTIME("Valid Time: ", utctime);
            printf("Connection: %d (%s, %s, %d)\n", 
                      (int) lGetUlong(el, SEC_ConnectionID), 
                      lGetHost(el, SEC_Host),
                      lGetString(el, SEC_Commproc),
                      lGetInt(el, SEC_Id));
         }
         ASN1_UTCTIME_free(utctime);
      }
   
      /* 
      ** search list element for this connection
      */
      element = lGetElemUlong(conn_list, SEC_ConnectionID, connid);
      if (!element) {
         ERROR((SGE_EVENT,"no list entry for connection %d\n", (int) connid));
         packstr(&pb,"Can't find you in connection list\n");
         i = -1;
         goto error;
      }
   
      /* 
      ** get data from connection list
      */
      sec_list2keymat(element);
      strcpy(gsd.uniqueIdentifier, lGetString(element, SEC_UniqueIdentifier)); 
      gsd.seq_receive = lGetUlong(element, SEC_SeqNoReceive);
      gsd.connid = connid;
   }
   else {
      if (gsd.connid != connid) {
         ERROR((SGE_EVENT,"received wrong connection ID\n"));
         ERROR((SGE_EVENT,"it is %d, but it should be %d!\n",
                  (int) connid, (int) gsd.connid));
         packstr(&pb,"Probably you send the wrong connection ID\n");
         i = -1;
         goto error;
      }
   }

   /*
   ** malloc *buffer 
   */
   *buffer = malloc(*buflen + EVP_CIPHER_block_size(gsd.cipher));
   if (!*buffer) {
      ERROR((SGE_EVENT,"failed malloc memory\n"));
      i = -1;
      goto error;
   }

   /*
   ** decrypt digest and message
   */
   memset(iv, '\0', sizeof(iv));
   if (!EVP_DecryptInit(&ctx, gsd.cipher, gsd.key_mat, iv)) {
      ERROR((SGE_EVENT,"failed decrypt MAC\n"));
      EVP_CIPHER_CTX_cleanup(&ctx);
      sec_error();
      i = -1;
      goto error;
   }

   if (!EVP_DecryptUpdate(&ctx, md_value, (int*) &md_len, 
                           enc_mac, enc_mac_len)) {
      ERROR((SGE_EVENT,"failed decrypt MAC\n"));
      EVP_CIPHER_CTX_cleanup(&ctx);
      sec_error();
      i = -1;
      goto error;
   }
   if (!EVP_DecryptFinal(&ctx, md_value, (int*) &md_len)) {
      ERROR((SGE_EVENT,"failed decrypt MAC\n"));
      EVP_CIPHER_CTX_cleanup(&ctx);
      sec_error();
      i = -1;
      goto error;
   }
   EVP_CIPHER_CTX_cleanup(&ctx);
   if (!md_len)
      md_len = enc_mac_len;   


   memset(iv, '\0', sizeof(iv));
   if (!EVP_DecryptInit(&ctx, gsd.cipher, gsd.key_mat, iv)) {
      ERROR((SGE_EVENT,"failed decrypt MAC\n"));
      EVP_CIPHER_CTX_cleanup(&ctx);
      sec_error();
      i = -1;
      goto error;
   }
   if (!EVP_DecryptUpdate(&ctx, (unsigned char*) *buffer, (int*) &outlen,
                           (unsigned char*)enc_msg, enc_msg_len)) {
      ERROR((SGE_EVENT,"failed decrypt message\n"));
      EVP_CIPHER_CTX_cleanup(&ctx);
      sec_error();
      i = -1;
      goto error;
   }
   if (!EVP_DecryptFinal(&ctx, (unsigned char*) *buffer, (int*)&outlen)) {
      ERROR((SGE_EVENT,"failed decrypt message\n"));
      EVP_CIPHER_CTX_cleanup(&ctx);
      sec_error();
      i = -1;
      goto error;
   }
   EVP_CIPHER_CTX_cleanup(&ctx);
   if (!outlen)
      outlen = enc_msg_len;

   /*
   ** FIXME: check of mac ????
   */

   /* 
   ** increment sequence number and write it to connection list   
   */
   gsd.seq_receive = INC32(gsd.seq_receive);
   if(conn_list){
      lSetHost(element, SEC_Host, host);
      lSetString(element, SEC_Commproc, commproc);
      lSetInt(element, SEC_Id, id);
      lSetUlong(element, SEC_SeqNoReceive, gsd.seq_receive);
   }

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
/*
** NAME
**   sec_reconnect
**
** SYNOPSIS
**
**   static int sec_reconnect()
**
** DESCRIPTION
**   This function reads the security related data from disk and decrypts
**   it for clients.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
*/
static int sec_reconnect()
{
   FILE *fp=NULL;
   int i, c, nbytes;
   sge_pack_buffer pb;
   SGE_STRUCT_STAT file_info;
   char *ptr;
   u_char *time_str = NULL;
   u_long32 len;
   BIO *bio = NULL;

   DENTER(GDI_LAYER,"sec_reconnect");

   if (SGE_STAT(reconnect_file, &file_info) < 0) {
      i = -1;
      goto error;
   }

   /* prepare packing buffer                                       */
   if ((i=init_packbuffer(&pb, file_info.st_size, 0))) {
      ERROR((SGE_EVENT,"failed init_packbuffer\n"));
      goto error;
   }
   /* 
   ** read encrypted buffer from file
   */
   bio = BIO_new_file(reconnect_file, "rb");
   i = BIO_read(bio, pb.head_ptr, file_info.st_size);
   if (!i) {
      ERROR((SGE_EVENT,"can't read from file\n"));
      i = -1;
      goto error;
   }
   BIO_free(bio);

   /* decrypt data with own rsa privat key            */
   nbytes = RSA_private_decrypt((SGE_OFF_T)file_info.st_size, 
                                (u_char*) pb.cur_ptr, (u_char*) pb.cur_ptr, 
                                 gsd.private_key->pkey.rsa, RSA_PKCS1_PADDING);
   if (nbytes <= 0) {
      ERROR((SGE_EVENT,"failed decrypt reconnect data\n"));
      sec_error();
      i = -1;
      goto error;
   }

   i = sec_unpack_reconnect(&pb, &gsd.connid, &gsd.seq_send, &gsd.seq_receive,
                              &gsd.key_mat, &gsd.key_mat_len, &time_str, &len);
   if (i) {
      ERROR((SGE_EVENT,"failed read reconnect from buffer\n"));
      goto error;
   }

   ptr = time_str;
   d2i_ASN1_UTCTIME(&gsd.refresh_time, (unsigned char **)&time_str, len);
   free((char*) ptr);

debug_print_ASN1_UTCTIME("gsd.refresh_time: ", gsd.refresh_time);

   gsd.connect = 1;
   i = 0;
   
   error:
      if (fp) 
         fclose(fp);
      remove(reconnect_file);
      clear_packbuffer(&pb);
      DEXIT;
      return i;
}

/*
** NAME
**   sec_write_data2hd
**
** SYNOPSIS
**
**   static int sec_write_data2hd()
**
** DESCRIPTION
**   This function writes the security related data to disk, encrypted
**   with RSA private key.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
*/
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

   /* prepare packing buffer                                       */
   if ((i=init_packbuffer(&pb, 0, 0))) {
      ERROR((SGE_EVENT,"failed init_packbuffer\n"));
      i = -1;
      goto error;
   }
   
   tmp = time_str;
   len = i2d_ASN1_UTCTIME(gsd.refresh_time, &tmp);
   time_str[len] = '\0';
   
   /* 
   ** write data to packing buffer
   */
   if (sec_pack_reconnect(&pb, gsd.connid, gsd.seq_send, gsd.seq_receive,
                           gsd.key_mat, gsd.key_mat_len, 
                           time_str, len)) {
      ERROR((SGE_EVENT,"failed write reconnect to buffer\n"));
      i = -1;
      goto error;
   }

   /* 
   ** encrypt data with own public rsa key
   */
   evp = X509_get_pubkey(gsd.x509);
   nbytes = RSA_public_encrypt(pb.bytes_used, (u_char*)pb.head_ptr, 
                               (u_char*)pb.head_ptr, evp->pkey.rsa, 
                               RSA_PKCS1_PADDING);
   if (nbytes <= 0) {
      ERROR((SGE_EVENT,"failed encrypt reconnect data\n"));
      sec_error();
      i = -1;
      goto error;
   }

   /* 
   ** write encrypted buffer to file
   */
   bio = BIO_file_new(reconnect_file, "w");
   if (!bio) {
      ERROR((SGE_EVENT,"can't open file '%s': %s\n",
               reconnect_file, strerror(errno)));
      i = -1;
      goto error;
   }
   i = BIO_write(bio, pb.head_ptr, nbytes);
   if (!i) {
      ERROR((SGE_EVENT,"can't write to file\n"));
      i = -1;
      goto error;
   }

   i=0;

   error:
      BIO_free(bio);
      clear_packbuffer(&pb);   
      DEXIT;
      return i;
}

#endif


/*===========================================================================*/

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
*/
static void sec_error(void)
{
   long   l;

   DENTER(GDI_LAYER,"sec_error");
   while ((l=ERR_get_error())){
      ERROR((SGE_EVENT, "%s\n", ERR_error_string(l, NULL)));
   }
   DEXIT;
}

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
*/

static int sec_send_err(
char *commproc,
int id,
char *host,
sge_pack_buffer *pb,
const char *err_msg 
) {
   int   i;

   DENTER(GDI_LAYER,"sec_send_error");
   if((i=packstr(pb,err_msg))) 
      goto error;
   if((i=sge_send_any_request(0,NULL,host,commproc,id,pb,TAG_SEC_ERROR)))
      goto error;
   else{
      DEXIT;
      return(0);
   }
   error:
   ERROR((SGE_EVENT,"failed send error message\n"));
   DEXIT;
   return(-1);
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

   if((i = packint(&pb,gsd.connid))){
      ERROR((SGE_EVENT,"failed pack ConnID to buffer"));
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
*/

static int sec_get_connid(char **buffer, u_long32 *buflen)
{
   int i;
   u_long32 new_buflen;
   sge_pack_buffer pb;

   DENTER(GDI_LAYER,"sec_set_connid");

   new_buflen = *buflen - INTSIZE;

   pb.head_ptr = *buffer;
   pb.cur_ptr = pb.head_ptr + new_buflen;
   pb.bytes_used = new_buflen;
   pb.mem_size = *buflen;

   if((i = unpackint(&pb,&gsd.connid))){
      ERROR((SGE_EVENT,"failed unpack ConnID from buffer"));
      goto error;
   }

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
*/

static int sec_update_connlist(const char *host, const char *commproc, int id)
{
   lListElem *element=NULL;

   DENTER(GDI_LAYER,"sec_update_connlist");

   element = lGetElemUlong(conn_list, SEC_ConnectionID, gsd.connid);
   if (!element){
      ERROR((SGE_EVENT,"no list entry for connection\n"));
      DEXIT;
      return -1;
   }
   lSetHost(element, SEC_Host,host);
   lSetString(element, SEC_Commproc,commproc);
   lSetInt(element, SEC_Id,id);

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
*/
static int sec_set_secdata(const char *host, const char *commproc, int id)
{
   lListElem *element=NULL;
   lCondition *where=NULL;

   DENTER(GDI_LAYER,"sec_set_secdata");

   /* 
   ** get right element from connection list         
   */
   if (id) {
      where = lWhere("%T(%I==%s && %I==%s && %I==%d)", SecurityT,
                       SEC_Host, host, SEC_Commproc, commproc, SEC_Id, id);
   }
   else {
      where = lWhere("%T(%I==%s && %I==%s)", SecurityT,
                     SEC_Host, host, SEC_Commproc, commproc);
   }

   element = lFindFirst(conn_list, where);
   where = lFreeWhere(where);
   if (!element) {
      ERROR((SGE_EVENT,"no list entry for connection (%s:%s:%d)\n!",
             host,commproc,id));
      DEXIT;
      return -1;
   } 
   
   /* 
   ** set security data                  
   */
   sec_list2keymat(element);
   strcpy(gsd.uniqueIdentifier, lGetString(element, SEC_UniqueIdentifier));
   gsd.connid = lGetUlong(element, SEC_ConnectionID);
   gsd.seq_send = lGetUlong(element, SEC_SeqNoSend);
   gsd.seq_receive = lGetUlong(element, SEC_SeqNoReceive);
   
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
*/
static int sec_insert_conn2list(u_long32 connid, char *host, char *commproc, int id, const char *uniqueIdentifier)
{
   lListElem *element;
   lCondition *where;

   DENTER(GDI_LAYER, "sec_insert_conn2list");
   
   /* 
   ** delete element if some with <host,commproc,id> exists   
   */
   where = lWhere("%T(%I==%s && %I==%s && %I==%d)", SecurityT,
                    SEC_Host, host,
                    SEC_Commproc, commproc,
                    SEC_Id, id);
   if (!where) {
      ERROR((SGE_EVENT, "can't build condition\n"));
      DEXIT;
      return -1;
   }
   element = lFindFirst(conn_list, where);
   where = lFreeWhere(where);

   if (!element && sec_is_daemon(commproc)) {
      where = lWhere("%T(%I==%s && %I==%s)", SecurityT,
                       SEC_Host, host,
                       SEC_Commproc, commproc);
      if (!where) {
         ERROR((SGE_EVENT, "can't build condition\n"));
         DEXIT;
         return -1;
      }
      element = lFindFirst(conn_list, where);
      where = lFreeWhere(where);
   }   

   /* 
   ** add new element if it does not exist                  
   */
   if (!element || !conn_list) {
      element = lAddElemUlong(&conn_list, SEC_ConnectionID, 
                                 connid, SecurityT);
   }
   if (!element) {
      ERROR((SGE_EVENT,"failed adding element to conn_list\n"));
      DEXIT;
      return -1;
   }

   /* 
   ** set values of element               
   */
   lSetUlong(element, SEC_ConnectionID, connid);
   lSetHost(element, SEC_Host, host);
   lSetString(element, SEC_Commproc, commproc); 
   lSetInt(element, SEC_Id, id); 
   lSetString(element, SEC_UniqueIdentifier, uniqueIdentifier); 
   lSetUlong(element, SEC_SeqNoSend, 0); 
   lSetUlong(element, SEC_SeqNoReceive, 0); 
   sec_keymat2list(element);
{
   int len;
   unsigned char *tmp;
   ASN1_UTCTIME *asn1_time_str = NULL;
   unsigned char time_str[50]; 

   asn1_time_str = X509_gmtime_adj(asn1_time_str, 60*ValidMinutes);
   tmp = time_str;
   len = i2d_ASN1_UTCTIME(asn1_time_str, &tmp);
   time_str[len] = '\0';
   ASN1_UTCTIME_free(asn1_time_str);

   lSetString(element, SEC_ExpiryDate, (char*) time_str);
}   
   DPRINTF(("++++ added %d (%s, %s, %d) of %s to conn_list\n", 
               (int)connid, host, commproc, id, uniqueIdentifier));

   /* 
   ** increment connid                   
   */
   connid_counter = INC32(connid_counter);

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
*/

static void sec_keymat2list(lListElem *element)
{
   int i;
   u_long32 ul[8];
   u_char working_buf[32 + 33], *pos_ptr;

   memcpy(working_buf,gsd.key_mat,gsd.key_mat_len);
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
   memcpy(gsd.key_mat,working_buf,gsd.key_mat_len);

}

/*==========================================================================*/
/*
** DESCRIPTION
**	These functions generate the pathnames for key and certificate files.
** and stores them in module static vars and forces an SGE_EXIT if something
** is wrong
*/
static void sec_setup_path(
int is_daemon 
) {
   SGE_STRUCT_STAT sbuf;
	char *userdir = NULL;
	char *user_local_dir = NULL;
	char *ca_root = NULL;
	char *ca_local_root = NULL;
   char *sge_cakeyfile = NULL;
   char *sge_keyfile = NULL;
   int len;
   char *cp = NULL;

   DENTER(GDI_LAYER, "sec_setup_path");

   cp = getenv("COMMD_PORT");
   
   /*
   ** malloc ca_root string and check if directory has been created during
   ** install otherwise exit
   */
   len = strlen(sge_get_root_dir(1)) + strlen(sge_get_default_cell()) +
         strlen(CA_DIR) + 3;
   ca_root = sge_malloc(len);
   sprintf(ca_root, "%s/%s/%s", sge_get_root_dir(1), 
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
      ca_key_file = sge_malloc(strlen(sge_cakeyfile));
      strcpy(ca_key_file, sge_cakeyfile);
   } else {
      if (getenv("SGE_NO_CA_LOCAL_ROOT")) {
         ca_local_root = ca_root;
      } else {
         len = strlen(CA_LOCAL_DIR) + 
               (cp ? strlen(cp)+4:strlen(SGE_COMMD_SERVICE)) +
               strlen(sge_get_default_cell()) + 3;
         ca_local_root = sge_malloc(len);
         if (cp)
            sprintf(ca_local_root, "%s/port%s/%s", CA_LOCAL_DIR, cp, 
                     sge_get_default_cell());
         else
            sprintf(ca_local_root, "%s/%s/%s", CA_LOCAL_DIR, SGE_COMMD_SERVICE, 
                     sge_get_default_cell());
      }   
      if (is_daemon && SGE_STAT(ca_local_root, &sbuf)) { 
         CRITICAL((SGE_EVENT, MSG_SEC_CALOCALROOTNOTFOUND_S, ca_local_root));
         SGE_EXIT(1);
      }
      ca_key_file = sge_malloc(strlen(ca_local_root) + strlen("private") + strlen(CaKey) + 3);
      sprintf(ca_key_file, "%s/%s/%s", ca_local_root, "private", CaKey);
   }

   if (is_daemon && SGE_STAT(ca_key_file, &sbuf)) { 
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
   if ((sge_keyfile = getenv("SGE_KEYFILE"))) {
      key_file = sge_malloc(strlen(sge_keyfile));
      strcpy(key_file, sge_keyfile);
   } else {   
      if (is_daemon){
         userdir = strdup(ca_root);
         user_local_dir = ca_local_root;
      } else {
         struct passwd *pw;
         pw = sge_getpwnam(me.user_name);
         if (!pw) {   
            CRITICAL((SGE_EVENT, MSG_SEC_USERNOTFOUND_S, me.user_name));
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

      key_file = sge_malloc(strlen(user_local_dir) + strlen("private") + strlen(UserKey) + 3);
      sprintf(key_file, "%s/private/%s", user_local_dir, UserKey);

      if (SGE_STAT(key_file, &sbuf)) { 
         free(key_file);
         key_file = sge_malloc(strlen(ca_local_root) + strlen("userkeys") + 
                                 strlen(me.user_name) + strlen(UserKey) + 4);
         sprintf(key_file, "%s/%s/%s/%s", ca_local_root, "userkeys", me.user_name, UserKey);
      }   

      if (!RAND_status()) {
         rand_file = sge_malloc(strlen(user_local_dir) + strlen("private") + strlen(RandFile) + 3);
         sprintf(rand_file, "%s/private/%s", user_local_dir, RandFile);

         if (SGE_STAT(rand_file, &sbuf)) { 
            free(rand_file);
            rand_file = sge_malloc(strlen(ca_local_root) + strlen("userkeys") + 
                                    strlen(me.user_name) + strlen(RandFile) + 4);
            sprintf(rand_file, "%s/%s/%s/%s", ca_local_root, "userkeys", me.user_name, RandFile);
         }   
      }   
   }
   if (SGE_STAT(key_file, &sbuf)) { 
      CRITICAL((SGE_EVENT, MSG_SEC_KEYFILENOTFOUND_S, key_file));
      SGE_EXIT(1);
   }
   DPRINTF(("key_file: %s\n", key_file));

   if (!RAND_status()) {
      if (SGE_STAT(rand_file, &sbuf)) { 
         WARNING((SGE_EVENT, MSG_SEC_RANDFILENOTFOUND_S, rand_file));
      }
      else {
         DPRINTF(("rand_file: %s\n", rand_file));
      }   
   }    

   cert_file = sge_malloc(strlen(userdir) + strlen("certs") + strlen(UserCert) + 3);
   sprintf(cert_file, "%s/certs/%s", userdir, UserCert);

   if (SGE_STAT(cert_file, &sbuf)) {
      free(cert_file);
      cert_file = sge_malloc(strlen(ca_local_root) + strlen("userkeys") + 
                              strlen(me.user_name) + strlen(UserCert) + 4);
      sprintf(cert_file, "%s/%s/%s/%s", ca_local_root, "userkeys", me.user_name, UserCert);
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
}

static sec_exit_func_type install_sec_exit_func(
sec_exit_func_type new 
) {
   sec_exit_func_type old;

   old = sec_exit_func;
   sec_exit_func = new;
   return(old);
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
**   This function checks if the unix user name that has been send corresponds **   to the certificate
**
** RETURN VALUES
**      1       on success
**      0       on failure
*/
int sec_verify_user(const char *user, const char *commproc) 
{
   DENTER(GDI_LAYER,"sec_verify_user");

/*    if (!sec_is_daemon(commproc)) { */
      DPRINTF(("commproc = '%s' user = '%s', gsd.uniqueIdentifier = '%s'\n", 
               commproc, user, gsd.uniqueIdentifier));
      if (strcmp(user, gsd.uniqueIdentifier)) {
         DEXIT;
         return 0;
      }   
/*    } */
      

   DEXIT;
   return 1;
}

static int sec_is_daemon(const char *progname)
{
   if (!strcmp(prognames[QMASTER],progname) ||
      !strcmp(prognames[EXECD],progname) ||
      !strcmp(prognames[SCHEDD],progname))
      return True;
   else
      return False;
}      

static int sec_verify_callback(int ok, X509_STORE_CTX *ctx)
{
   X509 *cert;
   
   printf("ok = %d\n", ok);
   cert = X509_STORE_CTX_get_current_cert(ctx);
   if (cert) {
      X509_print_fp(stdout, cert);
debug_print_ASN1_UTCTIME("not_before: ", X509_get_notBefore(cert));
debug_print_ASN1_UTCTIME("not_after: ", X509_get_notAfter(cert));
printf(" notBefore X509_cmp_current_time = %d\n",  X509_cmp_current_time(X509_get_notBefore(cert)));
printf(" notAfter X509_cmp_current_time = %d\n",  X509_cmp_current_time(X509_get_notAfter(cert)));
   }
   else
      printf("No cert\n");

   return ok;
}   



int sec_clear_connectionlist(void)
{
   lListElem *element = NULL;

   DENTER(GDI_LAYER, "sec_clear_connectionlist");
   
   /*
   ** remove any outdated entries first
   */
   if (conn_list) {
      ASN1_UTCTIME *utctime = ASN1_UTCTIME_new();

      element = lFirst(conn_list);
      while (element) {
         lListElem *del_el;
         const char *dtime = lGetString(element, SEC_ExpiryDate);
         utctime = d2i_ASN1_UTCTIME(&utctime, (unsigned char**) &dtime, 
                                          strlen(dtime));
         /* debug_print_ASN1_UTCTIME("utctime: ", utctime); */
         del_el = element;
         element = lNext(element);
         if ((X509_cmp_current_time(utctime) < 0)) { 
            DPRINTF(("---- removed %d (%s, %s, %d) from conn_list\n", 
                      (int) lGetUlong(del_el, SEC_ConnectionID), 
                      lGetHost(del_el, SEC_Host),
                      lGetString(del_el, SEC_Commproc),
                      lGetInt(del_el, SEC_Id)));
            if (!sec_is_daemon(lGetString(del_el, SEC_Commproc))) {
               lRemoveElem(conn_list, del_el);
            }
         }   
      }
      ASN1_UTCTIME_free(utctime);
   }
   DEXIT;
   return True;
}
