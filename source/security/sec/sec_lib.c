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


#include "commlib.h"
#include "sge.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_arch.h"
#include "sge_prognames.h" 
#include "sge_me.h" 
#include "basis_types.h"
#include "sge_gdi_intern.h"

#include "sec_crypto.h"          /* lib protos      */
#include "sec_lib.h"             /* lib protos      */
#include "sec_local.h"           /* specific definitions */

/*
** prototypes
*/
static int sec_set_encrypt(int tag);
static int sec_files(void);
static int sec_dump_connlist(void);
static int sec_undump_connlist(void);
static int sec_announce_connection(const char *tocomproc, const char *tohost);
static int sec_respond_announce(char *commproc, u_short id, char *host, char *buffer, u_long32 buflen);
static int sec_handle_announce(char *comproc, u_short id, char *host, char *buffer, u_long32 buflen);
static int sec_encrypt(char **buf, int *buflen);
static int sec_decrypt(char **buffer, u_long32 *buflen, char *host, char *commproc, int id);
/* static int sec_reconnect(void); */
/* static int sec_write_data2hd(void); */
static int sec_verify_certificate(X509 *cert);


static void sec_error(void);
static int sec_send_err(char *commproc, int id, char *host, sge_pack_buffer *pb, char *err_msg);
static int sec_set_connid(char **buffer, int *buflen);
static int sec_get_connid(char **buffer, u_long32 *buflen);
static int sec_update_connlist(const char *host, const char *commproc, int id);
static int sec_set_secdata(const char *host, const char *commproc, int id);
static int sec_insert_conn2list(char *host, char *commproc, int id);
static void sec_clearup_list(void);
static void sec_keymat2list(lListElem *element);
static void sec_list2keymat(lListElem *element);
/* static void sec_print_bytes(FILE *f, int n, char *b); */
/* static int sec_set_verify_locations(char *file_env); */
/* static int sec_verify_callback(int ok, X509 *xs, X509 *xi, int depth, int error); */
static char *sec_make_certdir(char *rootdir);
static char *sec_make_keydir(char *rootdir);
static char *sec_make_cadir(char *cell);
static char *sec_make_userdir(char *cell);
static FILES *sec_set_path(char *ca_keydir, char *ca_certdir, char *user_keydir, char *user_certdir);
static FILES *sec_setup_path(char *cell, int is_daemon);

static sec_exit_func_type install_sec_exit_func(sec_exit_func_type);

static int sec_pack_announce(int len, u_char *buf, u_char *chall, sge_pack_buffer *pb);
static int sec_unpack_announce(int *len, u_char **buf, u_char **chall, sge_pack_buffer *pb);
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
/* static int sec_pack_reconnect(u_long32 connid, u_long32 seq_send, u_long32 seq_receive, u_char *key_mat, sge_pack_buffer *pb); */
/* static int sec_unpack_reconnect(u_long32 *connid, u_long32 *seq_send, u_long32 *seq_receive, u_char **key_mat, sge_pack_buffer *pb); */


#ifdef DEBUG_SEC
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
#endif /* DEBUG_SEC */

/*
** FIXME
*/
#define PREDEFINED_PW         "troet"



/*
** global secure data
*/
GlobalSecureData gsd;
static sec_exit_func_type sec_exit_func = NULL;

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

int sec_init(
const char *progname 
) {
   int   i=0;

   DENTER(TOP_LAYER,"sec_init");

   /* 
   ** FIXME: rand seed needs to be improved
   */
   RAND_egd("/tmp/egd");

   /*
   ** initialize error strings
   */
   ERR_clear_error();
   ERR_load_crypto_strings();

   /*
   ** load all algorithms and digests
   */
   OpenSSL_add_all_algorithms();

   /* 
   ** FIXME: am I a sge daemon? -> is_daemon() should be implemented
   */
   if(!strcmp(prognames[QMASTER],progname) ||
      !strcmp(prognames[EXECD],progname) ||
      !strcmp(prognames[SCHEDD],progname))
      gsd.is_daemon = 1;
   else
      gsd.is_daemon = 0;

   /* 
   ** FIXME: init all the other stuff
   */
   gsd.digest = EVP_md5();
/*    gsd.cipher = EVP_des_cbc(); */
   gsd.cipher = EVP_rc4();
/*    gsd.cipher = EVP_enc_null(); */

   gsd.key_mat_len = GSD_KEY_MAT_32;
   gsd.key_mat = (u_char *) malloc(gsd.key_mat_len);
   gsd.refresh_time = NULL;
   if (!gsd.key_mat){
      ERROR((SGE_EVENT,"failed malloc memory!\n"));
      i = -1;
      goto error;
   }
   if(!strcmp(progname, prognames[QMASTER]))
      /* time after qmaster clears the connection list   */
      X509_gmtime_adj(gsd.refresh_time, 0);
   else
      /* time after client makes a new announce to master   */
      X509_gmtime_adj(gsd.refresh_time, (long) 60*60*(ValidHours-1));
   gsd.connid = gsd.connid_counter = gsd.seq_send = gsd.connect = gsd.seq_receive = 0;
   gsd.conn_list = NULL;

   /* 
   ** FIXME: install sec_exit functions for utilib
   */
   if(!strcmp(progname, prognames[QMASTER])){
      install_sec_exit_func(sec_dump_connlist);
      if (sec_undump_connlist())
                WARNING((SGE_EVENT,"warning: Can't read ConnList\n"));
   }
#if 0   
FIXME
   else if(!gsd.is_daemon)
           install_sec_exit_func(sec_write_data2hd);
#endif


   /* 
   ** read security related files
   */
   if(sec_files()){
      ERROR((SGE_EVENT,"failed read security files\n"));
      i = -1;
      goto error;
   }

   DEXIT;
   return 0;

   error:
      DEXIT;
      return i;
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

   DENTER(TOP_LAYER,"sec_send_message");

   if (gsd.refresh_time)
      i = X509_cmp_current_time(gsd.refresh_time);
   if ((me.who != QMASTER) && ((gsd.connect != 1) || (i < 0))) {
      if (sec_announce_connection(tocomproc,tohost)) {
         ERROR((SGE_EVENT,"failed announce\n"));
         i = SEC_ANNOUNCE_FAILED;
         goto error;
      }
      X509_gmtime_adj(gsd.refresh_time, (long) 60*60*(ValidHours-1));
      gsd.connect = 1;
   }

   if (sec_set_encrypt(tag)) {
      if (me.who == QMASTER) {
         if(sec_set_secdata(tohost,tocomproc,toid)) {
            ERROR((SGE_EVENT,"failed set security data\n"));
            i = SEC_SEND_FAILED;
            goto error;
         }
      }
      if (sec_encrypt(&buffer,&buflen)) {
         ERROR((SGE_EVENT,"failed encrypt message\n"));
         i = SEC_SEND_FAILED;
         goto error;
      }
   }
   else if (me.who != QMASTER) {
      if (sec_set_connid(&buffer, &buflen)) {
         ERROR((SGE_EVENT,"failed set connection ID\n"));
         i = SEC_SEND_FAILED;
         goto error;
      }
   }

   i = send_message(synchron, tocomproc, toid, tohost, tag, buffer, buflen,
                     mid, compressed);

   error:
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
                        int *tag, char **buffer, u_long32 *buflen, int synchron, 
                        u_short* compressed)
{
   int i;

   DENTER(TOP_LAYER,"sec_receive_message");

   if ((i=receive_message(fromcommproc, fromid, fromhost, tag, buffer, buflen,
                       synchron, compressed))) {
      DEXIT;
      return i;
   }

   if (*tag == TAG_SEC_ANNOUNCE) {
      i = sec_handle_announce(fromcommproc,*fromid,fromhost,*buffer,
               *buflen);
      if (i) {
         ERROR((SGE_EVENT,"failed handle announce for (%s:%s:%d)\n",
                  fromhost,fromcommproc,*fromid));
         DEXIT;
         return SEC_RECEIVE_FAILED;
      }
   }
   else if (sec_set_encrypt(*tag)) {

      DEBUG_PRINT_BUFFER("Encrypted incoming message", (unsigned char*) (*buffer), (int)(*buflen));

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

   DENTER(TOP_LAYER,"sec_files");

   /* 
   ** setup the filenames of certificates, keys, etc.
   */
   gsd.files = sec_setup_path(NULL, gsd.is_daemon);
   if (gsd.files == NULL){
      ERROR((SGE_EVENT,"failed sec_setup_path\n"));
      goto error;
   }

#if 0
   /* 
   ** set location of CA
   */
   i = sec_set_verify_locations(gsd.files->ca_cert_file);
   if (i<0) {
      ERROR((SGE_EVENT,"sec_set_verify_locations failed!\n"));
      if (i == -1)
         sec_error();
      goto error;
   }
#endif
   /* 
   ** read Certificate from file
   */
   fp = fopen(gsd.files->cert_file,"r");
   if (fp == NULL) {
       ERROR((SGE_EVENT, "Can't open Cert_file '%s': %s!!\n",
                        gsd.files->cert_file, strerror(errno)));
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
      ERROR((SGE_EVENT,"failed verify own certificate\n"));
      sec_error();
      i = -1;
      goto error;
   }


   /* 
   ** read private key from file
   */
   fp = fopen(gsd.files->key_file, "r");
   if (fp == NULL){
      ERROR((SGE_EVENT,"Can't open Key _file '%s': %s!\n",
                        gsd.files->key_file,strerror(errno)));
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

#if 0
   /*
   ** FIXME
   */
   if (!gsd.is_daemon)
      sec_reconnect();
#endif


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

   DENTER(GDI_LAYER, "sec_verify_certificate");

   DPRINTF(("subject: %s\n", X509_NAME_oneline(X509_get_subject_name(cert), 0, 0)));

   ctx = X509_STORE_new();
   X509_STORE_set_default_paths(ctx);
   X509_STORE_load_locations(ctx, CA_CERT_FILE, NULL);
   X509_STORE_CTX_init(&csc, ctx, cert, NULL);
   ret = X509_verify_cert(&csc);
   X509_STORE_CTX_cleanup(&csc);

   DEXIT;
   return ret;
}   

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
      fp = fopen("sec_connlist.dat","w");
      if (fp) {
         lDumpList(fp, gsd.conn_list,0);
         fclose(fp);
      }
      else 
         return -1;

      fp = fopen("sec_connid.dat","w");
      if (fp) {
         fprintf(fp, u32, gsd.connid_counter);
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
         gsd.conn_list = lUndumpList(fp,NULL,NULL);
         fclose(fp);
      } 
      else
         return -1;

         
      fp = fopen("sec_connid.dat","r");
      if (fp){
         fscanf(fp, u32, &gsd.connid_counter);
         fclose(fp);
      }
      else
         return -1;
   } 
   return 0;
}

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
const char *tocomproc,
const char *tohost 
) {
   EVP_PKEY *master_key = NULL;
   int i;
   X509 *x509_master=NULL;
   u_long32 x509_len, x509_master_len, chall_enc_len, enc_key_len, enc_key_mat_len,
            iv_len = 0;
   unsigned char challenge[CHALL_LEN];
   unsigned char *x509_master_buf=NULL, 
                 *enc_challenge=NULL,
                 *enc_key = NULL, 
                 *enc_key_mat=NULL,
                 *iv = NULL,
                 *tmp = NULL,
                 *x509_buf = NULL;
   sge_pack_buffer pb;
   char fromhost[MAXHOSTLEN], *buffer;
   char fromcommproc[MAXCOMPONENTLEN];
   int fromtag;
   u_short fromid;
   u_short compressed = 0;
   u_long32 dummymid, buflen;
   EVP_MD_CTX ctx;
   EVP_CIPHER_CTX ectx;
   
   DENTER(TOP_LAYER, "sec_announce_connection");
   /* 
   ** send sec_announce
   */
   gsd.seq_send = gsd.seq_receive = 0;

   /* 
   ** write x509 to buffer
   */
   tmp = x509_buf = (unsigned char *) malloc(4096);
   x509_len = i2d_X509(gsd.x509, &tmp);
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
   if ((i=init_packbuffer(&pb, 8192,0))) {
      ERROR((SGE_EVENT,"failed init_packbuffer\n"));
      goto error;
   }

   /* 
   ** write data to packing buffer
   */
   i = sec_pack_announce(x509_len, x509_buf, challenge, &pb);
   if (i) {
      ERROR((SGE_EVENT,"failed pack announce buffer\n"));
      goto error;
   }

   /* 
   ** send buffer
   */
   DPRINTF(("send announce to=(%s:%s)\n",tohost,tocomproc));
   if (send_message(COMMD_SYNCHRON, tocomproc, 0, tohost, TAG_SEC_ANNOUNCE, pb.head_ptr,
                     pb.bytes_used, &dummymid, 0)) {
      i = -1;
      goto error;
   }

   clear_packbuffer(&pb);
   
   /* 
   ** receive sec_response
   */
   fromhost[0] = '\0';
   fromcommproc[0] = '\0';
   fromid = 0;
   fromtag = 0;
   if (receive_message(fromcommproc, &fromid, fromhost, &fromtag,
                        &buffer, &buflen, COMMD_SYNCHRON, &compressed)) {
      ERROR((SGE_EVENT,"failed get sec_response from (%s:%d:%s:%d)\n",
                fromcommproc,fromid,fromhost,fromtag));
      i = -1;
      goto error;
   }

   pb.head_ptr = buffer;
   pb.mem_size = buflen;
   pb.cur_ptr = pb.head_ptr;
   pb.bytes_used = 0;

   DPRINTF(("received announcement response from=(%s:%s:%d) tag=%d buflen=%d\n",
           fromhost, fromcommproc, fromid, fromtag, pb.mem_size));

   /* 
   ** manage error or wrong tags
   */
   if (fromtag == TAG_SEC_ERROR){
      ERROR((SGE_EVENT,"master reports error: "));
      ERROR((SGE_EVENT,pb.cur_ptr));
      i=-1;
      goto error;
   }
   if (fromtag != TAG_SEC_RESPOND) {
      ERROR((SGE_EVENT,"received unexpected TAG from master\n"));
      i=-1;
      goto error;
   }

   /* 
   ** unpack data from packing buffer
   */
   i = sec_unpack_response(&x509_master_len, &x509_master_buf, 
                           &chall_enc_len, &enc_challenge,
                           &gsd.connid, 
                           &enc_key_len, &enc_key, 
                           &iv_len, &iv, 
                           &enc_key_mat_len, &enc_key_mat, &pb);
   if (i) {
      ERROR((SGE_EVENT,"failed unpack response buffer\n"));
      goto error;
   }

   DEBUG_PRINT_BUFFER("enc_key", enc_key, enc_key_len);
   DEBUG_PRINT_BUFFER("enc_key_mat", enc_key_mat, enc_key_mat_len);


   DPRINTF(("gsd.connid = %ld\n", gsd.connid));
   
   /* 
   ** read x509 certificate from buffer
   */
   x509_master = X509_new();
   x509_master = d2i_X509(&x509_master, &x509_master_buf, x509_master_len);
   if (!x509_master) {
      ERROR((SGE_EVENT,"failed read master certificate\n"));
      sec_error();
      i = -1;
      goto error;
   }

   if (!sec_verify_certificate(x509_master)) {
      sec_error();
      i = -1;
      goto error;
   }
   else {
      printf("Certificate is ok\n");
   }   

   /*
   ** extract public key 
   */
   master_key = X509_get_pubkey(x509_master);
   
   if(!master_key){
      ERROR((SGE_EVENT,"cannot extract public key from master certificate\n"));
      sec_error();
      i=-1;
      goto error;
   }

   /* 
   ** decrypt challenge with master public key
   */
   EVP_VerifyInit(&ctx, gsd.digest);
   EVP_VerifyUpdate(&ctx, challenge, CHALL_LEN);
   if (!EVP_VerifyFinal(&ctx, enc_challenge, chall_enc_len, master_key)) {
      ERROR((SGE_EVENT,"challenge from master is bad\n"));
      i = -1;
      goto error;
   }   
   
   /*
   ** decrypt secret key
   */
   if (!EVP_OpenInit(&ectx, gsd.cipher, enc_key, enc_key_len, iv, gsd.private_key)) {
      ERROR((SGE_EVENT,"EVP_OpenInit failed decrypt keys\n"));
      sec_error();
      i=-1;
      goto error;
   }
   if (!EVP_OpenUpdate(&ectx, gsd.key_mat, (int*) &gsd.key_mat_len, enc_key_mat, (int) enc_key_mat_len)) {
      ERROR((SGE_EVENT,"EVP_OpenUpdate failed decrypt keys\n"));
      sec_error();
      i = -1;
      goto error;
   }
   EVP_OpenFinal(&ectx, gsd.key_mat, (int*) &gsd.key_mat_len);
   /* FIXME: problem in EVP_* lib for stream ciphers */
   gsd.key_mat_len = 32;   

   DEBUG_PRINT_BUFFER("Received key material", gsd.key_mat, gsd.key_mat_len);
   
   i=0;
   error:
   clear_packbuffer(&pb);
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
   int i, x509_len, enc_key_mat_len;
   unsigned int chall_enc_len;
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

   DENTER(TOP_LAYER, "sec_respond_announce");

   /* 
   ** prepare packbuffer pb for unpacking message
   */
   pb.head_ptr = buffer;
   pb.cur_ptr = pb.head_ptr;
   pb.bytes_used = 0;
   pb.mem_size = buflen;

   /* 
   ** prepare packbuffer for sending message
   */
   if ((i=init_packbuffer(&pb_respond, 4096,0))) {
      ERROR((SGE_EVENT,"failed init_packbuffer\n"));
      sec_send_err(commproc,id,host,&pb_respond,"failed init_packbuffer\n");
      goto error;
   }

   /* 
   ** read announce from packing buffer
   */
   if (sec_unpack_announce(&x509_len, &x509_buf, &challenge, &pb)) {
      ERROR((SGE_EVENT,"failed unpack announce\n"));
      sec_send_err(commproc, id, host, &pb_respond, "failed unpack announce\n");
      goto error;
   }

   /* 
   ** read x509 certificate
   */
   x509 = d2i_X509(&x509, &x509_buf, x509_len);
   if (x509_buf) 
      free(x509_buf);
   x509_buf = NULL;

   if (!x509){
      ERROR((SGE_EVENT,"failed read client certificate\n"));
      sec_error();
      sec_send_err(commproc, id, host, &pb_respond, "failed read client certificate\n");
      i = -1;
      goto error;
   }

   if (!sec_verify_certificate(x509)) {
      ERROR((SGE_EVENT,"failed verify client certificate\n"));
      sec_error();
      sec_send_err(commproc, id, host, &pb_respond, "Client certificate is bad\n");
      i = -1;
      goto error;
   }
   else {
      printf("Certificate is ok\n");
   }   
      
   /*
   ** extract public key 
   */
   public_key[0] = X509_get_pubkey(x509);
   
   if(!public_key[0]){
      ERROR((SGE_EVENT,"cant get public key\n"));
      sec_error();
      sec_send_err(commproc,id,host,&pb_respond,"failed extract key from cert\n");
      i=-1;
      goto error;
   }
   public_key_size = EVP_PKEY_size(public_key[0]);

   /* 
   ** malloc several buffers
   */
   x509_buf = (u_char *) malloc(4096);
   ekey[0] = (u_char *) malloc(public_key_size);
   enc_key_mat = (u_char *) malloc(public_key_size);
   enc_challenge = (u_char *) malloc(public_key_size);

   if (!x509_buf || !enc_key_mat ||  !enc_challenge) {
      ERROR((SGE_EVENT,"out of memory\n"));
      sec_send_err(commproc,id,host,&pb_respond,"failed malloc memory\n");
      i=-1;
      goto error;
   }
   
   /* 
   ** set connection ID
   */
   gsd.connid = gsd.connid_counter;

   /* 
   ** write x509 certificate to buffer
   */
   tmp = x509_buf;
   x509_len = i2d_X509(gsd.x509, &tmp);

   /*
   ** sign the challenge with master's private key
   */
   EVP_SignInit(&ctx, gsd.digest);
   EVP_SignUpdate(&ctx, challenge, CHALL_LEN);
   if (!EVP_SignFinal(&ctx, enc_challenge, &chall_enc_len, gsd.private_key)) {
      ERROR((SGE_EVENT,"failed encrypt challenge MAC\n"));
      sec_error();
      sec_send_err(commproc,id,host,&pb_respond,"failed encrypt challenge MAC\n");
      i = -1;
      goto error;
   }   

   /* 
   ** make key material
   */
   RAND_pseudo_bytes(gsd.key_mat, gsd.key_mat_len);
   DEBUG_PRINT_BUFFER("Sent key material", gsd.key_mat, gsd.key_mat_len);
   
   /*
   ** seal secret key with client's public key
   */
   memset(iv, '\0', sizeof(iv));
   if (!EVP_SealInit(&ectx, gsd.cipher, ekey, &ekeylen, iv, public_key, 1)) {
      ERROR((SGE_EVENT,"EVP_SealInit failed\n"));;
      sec_error();
      sec_send_err(commproc,id,host,&pb_respond,"failed encrypt keys\n");
      i = -1;
      goto error;
   }
   if (!EVP_EncryptUpdate(&ectx, enc_key_mat, &enc_key_mat_len, gsd.key_mat, gsd.key_mat_len)) {
      ERROR((SGE_EVENT,"failed encrypt keys\n"));
      sec_error();
      sec_send_err(commproc,id,host,&pb_respond,"failed encrypt keys\n");
      i = -1;
      goto error;
   }
   EVP_SealFinal(&ectx, enc_key_mat, &enc_key_mat_len);

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
   if (i) {
      ERROR((SGE_EVENT,"failed write response to buffer\n"));
      sec_send_err(commproc,id,host,&pb_respond,"failed write response to buffer\n");
      goto error;   
   }

   /* 
   ** insert connection to Connection List
   */
   i = sec_insert_conn2list(host,commproc,id);
   if (i) {
      ERROR((SGE_EVENT,"failed insert Connection to List\n"));
      sec_send_err(commproc, id, host, &pb_respond, "failed insert you to list\n");
      goto error;
   }

   /* 
   ** send response
   */
   DPRINTF(("send response to=(%s:%s:%d)\n", host, commproc, id));
   
   if (send_message(0, commproc, id, host, TAG_SEC_RESPOND, pb_respond.head_ptr,
                     pb_respond.bytes_used, &dummymid, 0)) {
      ERROR((SGE_EVENT,"Send message to (%s:%d:%s)\n", commproc,id,host));
      i = -1;
      goto error;
   }

   i = 0;

   error:
      if (x509_buf) 
         free(x509_buf);
      if (challenge) 
         free(challenge);
      if (enc_challenge) 
         free(enc_challenge);
      if (enc_key_mat) 
         free(enc_key_mat);
      clear_packbuffer(&pb_respond);
      if (x509) 
         X509_free(x509);
      DEXIT;   
      return i;
#if 0
   /* 
   ** make MAC from challenge
   */
   MD5_Init(&c);
   MD5_Update(&c, challenge, (u_long) CHALL_LEN);
   MD5_Final(challenge_mac, &c);

   /* 
   ** encrypt challenge with master rsa_privat_key
   */
   chall_enc_len = RSA_private_encrypt(CHALL_LEN, challenge_mac, enc_challenge,
                                       gsd.rsa, RSA_PKCS1_PADDING);
   if (chall_enc_len <= 0) {
      ERROR((SGE_EVENT,"failed encrypt challenge MAC\n"));
      sec_error();
      sec_send_err(commproc,id,host,&pb_respond,"failed encrypt challenge MAC\n");
      i = -1;
      goto error;
   }

   /* 
   ** encrypt key_mat with client rsa_public_key
   */
   enc_key_len = RSA_public_encrypt(gsd.key_mat_len, gsd.key_mat, enc_key_mat, 
                                    public_key->pkey.rsa, RSA_PKCS1_PADDING);

printf("enc_key_len: %d\n", enc_key_len);

   if (enc_key_len <= 0) {
      ERROR((SGE_EVENT,"failed encrypt keys\n"));
      sec_error();
      sec_send_err(commproc,id,host,&pb_respond,"failed encrypt keys\n");
      i = -1;
      goto error;
   }
#endif
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
char **buffer,
int *buflen 
) {
   int i;
   u_char seq_str[INTSIZE];
   u_long32 seq_no;
   lListElem *element=NULL;
   lCondition *where=NULL;
   sge_pack_buffer pb;
   EVP_CIPHER_CTX ctx;
   unsigned char iv[EVP_MAX_IV_LENGTH];
   EVP_MD_CTX mdctx;
   unsigned char md_value[EVP_MAX_MD_SIZE];
   unsigned int md_len;
   unsigned int enc_md_len;
   unsigned char *outbuf;
   unsigned int outlen = 0;
   
   DENTER(TOP_LAYER,"sec_encrypt");   

   seq_no = htonl(gsd.seq_send);
   memcpy(seq_str, &seq_no, INTSIZE);

#if 0
{
   int i;
   printf("gsd.key_mat_len = %d\n", gsd.key_mat_len);
   for (i=0; i<gsd.key_mat_len; i++)
      printf("%02x", gsd.key_mat[i]);
   printf("\n");
   printf("*buflen = %d\n", *buflen);
   for (i=0; i<(*buflen); i++)
      printf("%02x", (*buffer)[i]);
   printf("\n");
}   
#endif
   
   /*
   ** create digest
   */
   EVP_DigestInit(&mdctx, gsd.digest);
   EVP_DigestUpdate(&mdctx, gsd.key_mat, gsd.key_mat_len);
   EVP_DigestUpdate(&mdctx, seq_str, INTSIZE);
   EVP_DigestUpdate(&mdctx, *buffer, *buflen);
   EVP_DigestFinal(&mdctx, md_value, &md_len);
   
#if 0   
{
   int i;
   printf("md_len = %d\n", md_len);
   for (i=0; i<md_len; i++)
      printf("%02x", md_value[i]);
   printf("\n");
}   
#endif

   /*
   ** realloc *buffer 
   */
   outbuf = (unsigned char *) malloc(*buflen + EVP_CIPHER_block_size(gsd.cipher));
   if (!outbuf) {
      i = -1;
      ERROR((SGE_EVENT,"failed malloc memory\n"));
      goto error;
   }

   /*
   ** encrypt digest and message
   */
   memset(iv, '\0', sizeof(iv));
   EVP_EncryptInit(&ctx, gsd.cipher, gsd.key_mat, iv);
   if (!EVP_EncryptUpdate(&ctx, md_value, (int*) &enc_md_len, md_value, md_len)) {
      ERROR((SGE_EVENT,"failed encrypt MAC\n"));
      sec_error();
      i=-1;
      goto error;
   }
   if (!EVP_EncryptFinal(&ctx, md_value, (int*) &enc_md_len)) {
      ERROR((SGE_EVENT,"failed encrypt MAC\n"));
      sec_error();
      i=-1;
      goto error;
   }
   EVP_CIPHER_CTX_cleanup(&ctx);
printf("md_len=%d  enc_md_len %d\n", md_len, enc_md_len);   

   
printf("*buflen = %d\n", *buflen);

   memset(iv, '\0', sizeof(iv));
   EVP_EncryptInit(&ctx, gsd.cipher, gsd.key_mat, iv);
   if (!EVP_EncryptUpdate(&ctx, outbuf, (int*) &outlen, (unsigned char*)*buffer, *buflen)) {
      ERROR((SGE_EVENT,"failed encrypt message\n"));
      sec_error();
      i=-1;
      goto error;
   }
printf("EVP_EncryptUpdate outlen = %d\n", outlen);
   if (!EVP_EncryptFinal(&ctx, outbuf, (int*) &outlen)) {
      ERROR((SGE_EVENT,"failed encrypt message\n"));
      sec_error();
      i=-1;
      goto error;
   }
   EVP_CIPHER_CTX_cleanup(&ctx);

outlen = *buflen;

#if 0
{
   int i;
   printf("outlen = %d\n", outlen);
   for (i=0; i<outlen; i++)
      printf("%02x", outbuf[i]);
   printf("\n");
}   
#endif

   /*
   ** initialize packbuffer
   */
   if ((i=init_packbuffer(&pb, md_len + outlen, 0))) {
      ERROR((SGE_EVENT,"failed init_packbuffer\n"));
      goto error;
   }


   /*
   ** pack the information into one buffer
   */
   i = sec_pack_message(&pb, gsd.connid, md_len, md_value, outbuf, outlen);
   if (i) {
      ERROR((SGE_EVENT,"failed pack message to buffer\n"));
      goto error;
   }

   *buffer = pb.head_ptr;
   *buflen = pb.bytes_used;
   
   gsd.seq_send = INC32(gsd.seq_send);

   if (gsd.conn_list) {
      /* 
      ** search list element for this connection
      */
      where = lWhere( "%T(%I==%u)", SecurityT, SEC_ConnectionID, gsd.connid);
      element = lFindFirst(gsd.conn_list, where);
      if (!element || !where) {
         ERROR((SGE_EVENT,"no list entry for connection\n"));
         i=-1;
         goto error;
      }
      lSetUlong(element, SEC_SeqNoSend, gsd.seq_send);
   }   

   DEXIT;
   return 0;

   error:
      if(where)
         lFreeWhere(where);
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

   DENTER(TOP_LAYER, "sec_handle_announce");

   if (me.who == QMASTER) {
      if (buffer && buflen) {   
         /* 
         ** someone wants to announce to master
         */
         if (sec_respond_announce(commproc, id, host, buffer,buflen)) {
            ERROR((SGE_EVENT, "Failed send sec_response to (%s:%d:%s)\n",
                     commproc, id, host));
            i = -1;
            goto error;
         }
      }
      else{         
         /* 
         ** a sec_decrypt error has occured
         */
         if (send_message(0,commproc,id,host,TAG_SEC_ANNOUNCE, buffer,buflen,&mid, compressed)) {
            ERROR((SGE_EVENT,"Failed send summonses for announce to (%s:%d:%s)\n",commproc, id, host));
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
      if (stat(gsd.files->reconnect_file,&file_info) < 0) {
         i = 0;
         goto error;
      }
      if (remove(gsd.files->reconnect_file)) {
         ERROR((SGE_EVENT,"failed remove reconnect file '%s'\n",
                  gsd.files->reconnect_file));
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
/*    lCondition *where=NULL; */
   sge_pack_buffer pb;
   EVP_CIPHER_CTX ctx;
   unsigned char iv[EVP_MAX_IV_LENGTH];
   unsigned char md_value[EVP_MAX_MD_SIZE];
   unsigned int md_len = 0;
   unsigned char *outbuf = NULL;
   unsigned int outlen = 0;

   DENTER(TOP_LAYER, "sec_decrypt");
   /*
   ** initialize packbuffer
   */
   if ((i=init_packbuffer_from_buffer(&pb, *buffer, *buflen, 0))) {
      ERROR((SGE_EVENT,"failed init_packbuffer_from_buffer\n"));
      DEXIT;
      return i;
   }

   /* 
   ** unpack data from packing buffer
   */
   i = sec_unpack_message(&pb, &connid, &enc_mac_len, &enc_mac, 
                          &enc_msg_len, &enc_msg);
   if (i) {
      ERROR((SGE_EVENT,"failed unpack message from buffer\n"));
      packstr(&pb,"Can't unpack your message from buffer\n");
      DEXIT;
      return i;
   }

#if 0
{
   int j;
   printf("enc_mac\n");
   for (j=0;j<enc_mac_len;j++)
      printf("%02x", enc_mac[j]);
   printf("\n\n");   
   printf("enc_msg\n");
   for (j=0;j<enc_msg_len;j++)
      printf("%02x", enc_msg[j]);
   printf("\n");   
}
#endif


#if 0
FIXME
   if (gsd.conn_list) {
      sec_clearup_list();
      /* 
      ** search list element for this connection
      */
      where = lWhere( "%T(%I==%u)", SecurityT, SEC_ConnectionID, connid);
      element = lFindFirst(gsd.conn_list,where);
      if (!element || !where) {
         ERROR((SGE_EVENT,"no list entry for connection\n"));
         packstr(&pb,"Can't find you in connection list\n");
         where = lFreeWhere(where);
         DEXIT;
         return -1;
      }
      where = lFreeWhere(where);
   
      /* 
      ** get data from connection list
      */
      sec_list2keymat(element);
      gsd.seq_receive = lGetUlong(element, SEC_SeqNoReceive);
      gsd.connid = connid;
   }
   else {
      if (gsd.connid != connid) {
         ERROR((SGE_EVENT,"received wrong connection ID\n"));
         ERROR((SGE_EVENT,"it is %ld, but it should be %ld!\n",
                  connid, gsd.connid));
         packstr(&pb,"Probably you send the wrong connection ID\n");
         DEXIT;
         return -1;
      }
   }
#endif

   /*
   ** realloc *buffer 
   */
   outbuf = (unsigned char *) malloc(*buflen + EVP_CIPHER_block_size(gsd.cipher));
   if (!outbuf) {
      i = -1;
      ERROR((SGE_EVENT,"failed malloc memory\n"));
      DEXIT;
      return -1;
   }

   /*
   ** decrypt digest and message
   */
   memset(iv, '\0', sizeof(iv));
   if (!EVP_DecryptInit(&ctx, gsd.cipher, gsd.key_mat, iv)) {
      ERROR((SGE_EVENT,"failed decrypt MAC\n"));
      sec_error();
      DEXIT;
      return -1;
   }

   if (!EVP_DecryptUpdate(&ctx, md_value, (int*) &md_len, enc_mac, enc_mac_len)) {
      ERROR((SGE_EVENT,"failed decrypt MAC\n"));
      sec_error();
      DEXIT;
      return -1;
   }
   if (!EVP_DecryptFinal(&ctx, md_value, (int*) &md_len)) {
      ERROR((SGE_EVENT,"failed decrypt MAC\n"));
      sec_error();
      DEXIT;
      return -1;
   }
   EVP_CIPHER_CTX_cleanup(&ctx);
md_len = enc_mac_len;   

   memset(iv, '\0', sizeof(iv));
   if (!EVP_DecryptInit(&ctx, gsd.cipher, gsd.key_mat, iv)) {
      ERROR((SGE_EVENT,"failed decrypt MAC\n"));
      sec_error();
      DEXIT;
      return -1;
   }
   if (!EVP_DecryptUpdate(&ctx, outbuf, (int*) &outlen, (unsigned char*)enc_msg, enc_msg_len)) {
      ERROR((SGE_EVENT,"failed decrypt message\n"));
      sec_error();
      DEXIT;
      return -1;
   }
   EVP_CIPHER_CTX_cleanup(&ctx);
outlen = enc_msg_len;

   /*
   ** FIXME: check of mac ????
   */

   /* 
   ** increment sequence number and write it to connection list   
   */
   gsd.seq_receive = INC32(gsd.seq_receive);
   if(gsd.conn_list){
      lSetString(element,SEC_Host,host);
      lSetString(element,SEC_Commproc,commproc);
      lSetInt(element,SEC_Id,id);
      lSetUlong(element,SEC_SeqNoReceive,gsd.seq_receive);
   }

   /* 
   ** shorten buflen                  
   */
   *buflen = outlen;
   *buffer = (char*) outbuf;

printf("sec_decrypt = %d\n", (int) *buflen);

   DEXIT;
   return 0;
}

#if 0
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
   struct stat file_info;
   char *ptr;
   unsigned char time_str[64];

   DENTER(TOP_LAYER,"sec_reconnect");

   if (stat(gsd.files->reconnect_file, &file_info) < 0) {
      i = -1;
      goto error;
   }

   /* open reconnect file                  */
   fp = fopen(gsd.files->reconnect_file,"r");
   if (!fp) {
      i = -1;
      ERROR((SGE_EVENT,"can't open file '%s': %s\n",
            gsd.files->reconnect_file, strerror(errno)));
      goto error;
   }

   /* read time of connection validity            */
   for (i=0; i<63; i++) {
      c = getc(fp);
      if (c == EOF) {
         ERROR((SGE_EVENT,"can't read time from reconnect file\n"));
         i = -1;
         goto error;
      }
      time_str[i] = (char) c;
   }      
   d2i_ASN1_UTCTIME(&gsd.refresh_time, (unsigned char**) &time_str, 64);
      
   /* prepare packing buffer                                       */
   if ((i=init_packbuffer(&pb, 256,0))) {
      ERROR((SGE_EVENT,"failed init_packbuffer\n"));
      goto error;
   }

   ptr = pb.head_ptr;               
   for (i=0; i<file_info.st_size-63; i++) {
      c = getc(fp);
      if (c == EOF){
         ERROR((SGE_EVENT,"can't read data from reconnect file\n"));
         i = -1;
         goto error;
      }
      *(ptr++) = (char) c;
   }

   /* decrypt data with own rsa privat key            */
   nbytes = RSA_private_decrypt((int) file_info.st_size-63, 
                                (u_char*) pb.cur_ptr, (u_char*) pb.cur_ptr, 
                                 gsd.rsa, RSA_PKCS1_PADDING);
   if (nbytes <= 0) {
      ERROR((SGE_EVENT,"failed decrypt reconnect data\n"));
      sec_error();
      i = -1;
      goto error;
   }

   i = sec_unpack_reconnect(&gsd.connid, &gsd.seq_send, &gsd.seq_receive,
                              &gsd.key_mat, &pb);
   if (i) {
      ERROR((SGE_EVENT,"failed read reconnect from buffer\n"));
      goto error;
   }

   gsd.connect = 1;
   i = 0;
   
   error:
      if (fp) 
         fclose(fp);
      remove(gsd.files->reconnect_file);
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
   FILE   *fp=NULL;
   int   i,nbytes,ret;
   sge_pack_buffer pb;
   char   *ptr; 
   unsigned char   time_str[64];
   unsigned char **time_str_ptr;

   DENTER(TOP_LAYER,"sec_write_data2hd");

   /* prepare packing buffer                                       */
   if ((i=init_packbuffer(&pb, 256,0))) {
      ERROR((SGE_EVENT,"failed init_packbuffer\n"));
      goto error;
   }
   
   /* write data to packing buffer               */
   i = sec_pack_reconnect(gsd.connid, gsd.seq_send, gsd.seq_receive, 
                           gsd.key_mat, &pb);
   if (i) {
      ERROR((SGE_EVENT,"failed write reconnect to buffer\n"));
      goto error;
   }

   /* encrypt data with own public rsa key            */
   nbytes = RSA_public_encrypt(pb.bytes_used, (u_char*)pb.head_ptr, 
                               (u_char*)pb.head_ptr, gsd.rsa, 
                               RSA_PKCS1_PADDING);
   if (nbytes <= 0) {
      ERROR((SGE_EVENT,"failed encrypt reconnect data\n"));
      sec_error();
      i = -1;
      goto error;
   }

   /* open reconnect file                  */
   fp = fopen(gsd.files->reconnect_file,"w");
   if (!fp) {
      i = -1;
      ERROR((SGE_EVENT,"can't open file '%s': %s\n",
               gsd.files->reconnect_file, strerror(errno)));
      goto error;
   }

   if (!fp) {
      ERROR((SGE_EVENT,"failed open reconnect file '%s'\n",
             gsd.files->reconnect_file));
      i = -1;
      goto error;
   }

   /* write time of connection validity to file                    */
   i2d_ASN1_UTCTIME(gsd.refresh_time, time_str_ptr);
   for (i=0; i<63; i++) {
      ret = putc((int) time_str[i], fp);
      if (ret == EOF) {
         ERROR((SGE_EVENT,"can't write to file\n"));
         i = -1;
         goto error;
      }
   }

   /* write buffer to file                  */
   ptr = pb.head_ptr;
   for (i=0; i<nbytes; i++) {
      ret = putc((int) *(ptr+i),fp);
      if (ret == EOF) {
         ERROR((SGE_EVENT,"can't write to file\n"));
         i = -1;
         goto error;
      }
   }

   i=0;

   error:
      if (fp) 
         fclose(fp);
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

   DENTER(TOP_LAYER,"sec_error");
   while ((l=ERR_get_error())){
      ERROR((SGE_EVENT, ERR_error_string(l, NULL)));
      ERROR((SGE_EVENT,"\n"));
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
char *err_msg 
) {
   int   i;

   DENTER(TOP_LAYER,"sec_send_error");
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

   DENTER(TOP_LAYER,"sec_set_connid");

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

   DENTER(TOP_LAYER,"sec_set_connid");

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
   int ret = 0;
   lListElem *element=NULL;
   lCondition *where=NULL;

   DENTER(TOP_LAYER,"sec_update_connlist");

   where = lWhere( "%T(%I==%u)",SecurityT,SEC_ConnectionID,gsd.connid);
   element = lFindFirst(gsd.conn_list,where);
   if(!element || !where){
      ERROR((SGE_EVENT,"no list entry for connection\n"));
      ret = -1;
      goto error;
   }

   lSetString(element,SEC_Host,host);
   lSetString(element,SEC_Commproc,commproc);
   lSetInt(element,SEC_Id,id);

   error:
      if(where) 
         lFreeWhere(where);
      DEXIT;
      return ret;
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
   int i;
   lListElem *element=NULL;
   lCondition *where=NULL;

   DENTER(TOP_LAYER,"sec_get_secdata");

   /* 
   ** get right element from connection list         
   */
   if (id) {
      where = lWhere("%T(%I==%s && %I==%s && %I==%d)",SecurityT,
                       SEC_Host,host,SEC_Commproc,commproc,SEC_Id,id);
   }
   else {
      where = lWhere("%T(%I==%s && %I==%s)",SecurityT,
                     SEC_Host,host,SEC_Commproc,commproc);
   }

   element = lFindFirst(gsd.conn_list,where);
   if (!element || !where) {
      ERROR((SGE_EVENT,"no list entry for connection (%s:%s:%d)\n!",
             host,commproc,id));
      i=-1;
      goto error;
   } 
   
   /* 
   ** set security data                  
   */
   sec_list2keymat(element);
   gsd.connid = lGetUlong(element,SEC_ConnectionID);
   gsd.seq_send = lGetUlong(element,SEC_SeqNoSend);
   gsd.seq_receive = lGetUlong(element,SEC_SeqNoReceive);
   
   i = 0;
   error:
      if(where) 
         lFreeWhere(where);
      DEXIT;
      return(i);
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
static int sec_insert_conn2list(char *host, char *commproc, int id)
{
   int ret = 0;
   ASN1_UTCTIME *time_str = NULL;
   lListElem    *element;
   lCondition   *where;

   DENTER(TOP_LAYER,"sec_insert_conn2list");

   /* 
   ** create List if it not exists               
   */   
   if (!gsd.conn_list) 
      gsd.conn_list = lCreateList("conn_list",SecurityT);
   if (!gsd.conn_list) {
      ERROR((SGE_EVENT,"failed create Conn_List\n"));
      ret = -1;
      goto error;
   }

   /* 
   ** delete element if some with <host,commproc,id> exists   
   */
   where = lWhere( "%T(%I==%s && %I==%s && %I==%d)",SecurityT,
                    SEC_Host,host,SEC_Commproc,commproc,SEC_Id,id);
   if (!where) {
      ERROR((SGE_EVENT,"can't build condition\n"));
      ret = -1;
      goto error;
   }
   element = lFindFirst(gsd.conn_list, where);
   while (element) {
      lFreeElem(lDechainElem(gsd.conn_list,element));
                element = lFindNext(element,where);
   }

   /* 
   ** clearup list from old connections if it is time              
   */
   sec_clearup_list();
   
   /* 
   ** create element                  
   */
   element = lCreateElem(SecurityT);
   if (!element) {
      ERROR((SGE_EVENT,"failed create List_Element\n"));
      ret = -1;
      goto error;
   }

   /* 
   ** set values of element               
   */
   lSetUlong(element,SEC_ConnectionID,gsd.connid); 
   lSetString(element,SEC_Host,host); 
   lSetString(element,SEC_Commproc,commproc); 
   lSetInt(element,SEC_Id,id); 
   lSetUlong(element,SEC_SeqNoSend,0); 
   lSetUlong(element,SEC_SeqNoReceive,0); 

   sec_keymat2list(element);

   X509_gmtime_adj(time_str,60*60*ValidHours);

   lSetString(element, SEC_ExpiryDate, (char *)time_str); 

   /* 
   ** append element to list               
   */
   if ((ret=lAppendElem(gsd.conn_list,element))) 
      goto error;

   /* 
   ** increment connid                   
   */
   gsd.connid_counter = INC32(gsd.connid_counter);


   error:
      DEXIT;
      return ret;   
}

/*
** NAME
**   sec_clearup_list
**
** SYNOPSIS
**
**   static void sec_clearup_list(void)
**
** DESCRIPTION
**      This function clears the connection list from old connections.
**
*/
static void sec_clearup_list(void)
{
   lListElem *element,*del_el;
   const char *time_str=NULL;
   int i;
   ASN1_UTCTIME *utctime = NULL;

   /* 
   ** should the list be cleared from old connections ?
   */
   if (gsd.refresh_time && X509_cmp_current_time(gsd.refresh_time) < 0 ) {
      element = lFirst(gsd.conn_list);
      while (element) {
         time_str = lGetString(element, SEC_ExpiryDate);
         d2i_ASN1_UTCTIME(&utctime, (unsigned char**) &time_str, strlen(time_str));
         i = X509_cmp_current_time(utctime);
         del_el = element;
         element = lNext(element);
         if (i < 0)
            lFreeElem(lDechainElem(gsd.conn_list, del_el));
      }
      X509_gmtime_adj(gsd.refresh_time, 60*60*ClearHours);
   }
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
/*    u_char working_buf[gsd.key_mat_len + 33], *pos_ptr; */
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
/*    u_char working_buf[gsd.key_mat_len + 33],*pos_ptr; */
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

#if 0
/*
** NAME
**   sec_print_bytes
**
** SYNOPSIS
**
**   static void sec_print_bytes(f, n, b)
**   FILE *f - output device
**   int n   - number of bytes to print
**   char *b - pointer to bytes to print
**
** DESCRIPTION
**      This function prints bytes in hex code.
*/

static void sec_print_bytes(FILE *f, int n, char *b)
{
   int i;
   static char *h="0123456789abcdef";

   fflush(f);
   for (i=0; i<n; i++) {
      fputc(h[(b[i]>>4)&0x0f],f);
      fputc(h[(b[i]   )&0x0f],f);
      fputc(' ',f);
   }
}


/*
** NAME
**   sec_set_verify_locations
**
** SYNOPSIS
**
**   static int sec_set_verify_locations(file_env)
**   char *filenv - filename of the CA
**
** DESCRIPTION
**   This function sets the location of the CA.
**
** RETURN VALUES
**   0   on success
**   -1  on failure
*/
   
static int sec_set_verify_locations(
char *file_env 
) {
   int i;
   struct stat st;
   char *str;

   str=file_env;
   if ((str != NULL) && (stat(str,&st) == 0)) {
      i=X509_add_cert_file(str,X509_FILETYPE_PEM);
      if (!i) 
         return(-1);
   }
   return(0);
}

/*
** NAME
**   sec_verify_callback
**
** SYNOPSIS
**
**   static int sec_verify_callback(ok, xs, xi, depth, error)
**
** DESCRIPTION
**   This function is the callbackfunction to verify a certificate.
**
** RETURN VALUES
**   <>0     on success
**   0       on failure
*/

static int sec_verify_callback(int ok, X509 *xs, X509 *xi, int depth, int error)
{
   char *s;

   if (error == VERIFY_ERR_UNABLE_TO_GET_ISSUER) {
      s = (char *)X509_NAME_oneline(X509_get_issuer_name(xs), NULL, 0);
      if (s == NULL) {
         fprintf(stderr,"verify error\n");
         ERR_print_errors_fp(stderr);
         return 0;
      }
      fprintf(stderr,"issuer= %s\n",s);
      free(s);
   }
   if (!ok) 
      fprintf(stderr,"verify error:num=%d:%s\n",error,
                        X509_verify_cert_error_string(error));
   return ok;
}
#endif


/*==========================================================================*/
/*
** DESCRIPTION
**	These functions generate the pathnames for key and certificate files.
*/
static FILES *sec_setup_path(
char *cell,
int is_daemon 
) {
	FILES	*files;
	char	*userdir,*cadir,*user_certdir,*user_keydir,*ca_certdir,*ca_keydir;

    
	cadir = sec_make_cadir(cell);
	if (cadir == NULL){
		fprintf(stderr,"failed sec_make_cadir\n");
		return(NULL);
	}

	ca_certdir = sec_make_certdir(cadir);
   if (ca_certdir == NULL){
      fprintf(stderr,"failed sec_make_certdir\n");
      return(NULL);
   }

	ca_keydir = sec_make_keydir(cadir);
   if (ca_keydir == NULL){
      fprintf(stderr,"failed sec_make_keydir\n");
      return(NULL);
   }

	if (is_daemon){
		userdir = cadir;
		user_certdir = ca_certdir;
		user_keydir = ca_keydir;
	}
	else{
		userdir = sec_make_userdir(cell);
		if (userdir == NULL){
			fprintf(stderr,"failed sec_make_userr\n");
			return(NULL);
		}

		user_certdir = sec_make_certdir(userdir);
		if (user_certdir == NULL){
			fprintf(stderr,"failed sec_make_certdir\n");
			return(NULL);
		}

		user_keydir = sec_make_keydir(userdir);
		if (user_keydir == NULL){
			fprintf(stderr,"failed sec_make_keydir\n");
			return(NULL);
		}
	}

	files = sec_set_path(ca_keydir,ca_certdir,user_keydir,user_certdir);
   if (files == NULL){
      fprintf(stderr,"failed sec_set_path\n");
      return(NULL);
   }

	return(files);
}

FILES *sec_set_path(
char *ca_keydir,
char *ca_certdir,
char *user_keydir,
char *user_certdir 
) {
	FILES	*files;

   files = (FILES *) malloc(sizeof(FILES));
   if (files == NULL) 
      goto error;

	files->ca_key_file = (char *) malloc(strlen(ca_keydir) + strlen(CaKey) + 10);
	if (files->ca_key_file == NULL) 
      goto error;
	sprintf(files->ca_key_file,"%s/%s",ca_keydir,CaKey);

	files->ca_cert_file = (char *) malloc(strlen(ca_certdir) + strlen(CaCert) + 10);
	if (files->ca_cert_file == NULL) 
      goto error;
	sprintf(files->ca_cert_file,"%s/%s",ca_certdir,CaCert);

   files->key_file = (char *) malloc(strlen(user_keydir) + strlen(RsaKey) + 10);
   if (files->key_file == NULL) 
      goto error;
   sprintf(files->key_file,"%s/%s",user_keydir,RsaKey);

   files->cert_file = (char *) malloc(strlen(user_certdir) + strlen(Cert) + 10);
   if (files->cert_file == NULL) 
      goto error;
   sprintf(files->cert_file,"%s/%s",user_certdir,Cert);

	files->reconnect_file = (char *) malloc(strlen(user_keydir) + 
					strlen(ReconnectFile) + 10);
	if (files->reconnect_file == NULL) 
      goto error;
   sprintf(files->reconnect_file,"%s/%s",user_keydir,ReconnectFile);

	return(files);

error:
	fprintf(stderr,"failed malloc memory\n");
	return(NULL);	
}

static char *sec_make_certdir(
char *rootdir 
) {
   char *certdir,*cert_dir_root;
   struct stat     sbuf;

   certdir = (char *) malloc(strlen(CertPath) + 10);
	strcpy(certdir,CertPath);

   /* get rid of surrounding slashs                                */
   if (certdir[0] == '/')
      strcpy(certdir,certdir+1);
   if (certdir[strlen(certdir)-1] == '/')
      certdir[strlen(certdir)-1] = '\0';

   cert_dir_root = (char *) malloc(strlen(certdir) + strlen(rootdir) + 10);
   if (cert_dir_root == NULL){
      fprintf(stderr,"failed malloc memory\n");
      return(NULL);
   }
   sprintf(cert_dir_root,"%s/%s",rootdir,certdir);

   /* is cert_dir_root already there?                              */
   if (stat(cert_dir_root,&sbuf)){
      fprintf(stderr, "cert directory %s doesn't exist - making\n",
                        cert_dir_root);
      if (mkdir(cert_dir_root,(mode_t) 0755)) {
         fprintf(stderr,"could not mkdir %s\n",cert_dir_root);
         return(NULL);
      }
   }

	free(certdir);
   return(cert_dir_root);
}


static char *sec_make_keydir(
char *rootdir 
) {
	char		*keydir,*key_dir_root;
	struct stat     sbuf;

	keydir = (char *) malloc(strlen(KeyPath) + 10);
	if (keydir== NULL){
      fprintf(stderr,"failed malloc memory\n");
      return(NULL);
   }

	strcpy(keydir,KeyPath);

        /* get rid of surrounding slashs                                */
        if(keydir[0] == '/')
                strcpy(keydir,keydir+1);
        if (keydir[strlen(keydir)-1] == '/')
                keydir[strlen(keydir)-1] = '\0';

        key_dir_root = (char *) malloc(strlen(keydir) + strlen(rootdir) + 10); 
        if(key_dir_root == NULL){
                fprintf(stderr,"failed malloc memory\n");
                return(NULL);
        }
	sprintf(key_dir_root,"%s/%s",rootdir,keydir);

        /* is key_dir_root already there?				*/
        if (stat(key_dir_root,&sbuf)){
                fprintf(stderr, "key directory %s doesn't exist - making\n",
                        key_dir_root);
                if(mkdir(key_dir_root,(mode_t) 0700)){
                        fprintf(stderr,"could not mkdir %s\n",key_dir_root);
                        return(NULL);
                }
        }
	free(keydir);
	return(key_dir_root);
}
	
	
static char *sec_make_cadir(
char *cell 
) {
	const char *ca_root;
   char *ca_cell, *cell_root;
	struct stat 	sbuf;

	ca_root = sge_sge_root();

	/* is ca_root already there?					*/
	if (stat(ca_root,&sbuf)) { 
		fprintf(stderr,"could not stat %s\n",ca_root);
		return(NULL);	
	}

	if(cell) 
		ca_cell = cell;
	else{
		ca_cell = getenv("SGE_CELL");
        	if (!ca_cell || strlen(ca_cell) == 0)
                ca_cell = DEFAULT_CELL;
	}
	
	/* get rid of surrounding slashs				*/
	if(ca_cell[0] == '/')
		strcpy(ca_cell,ca_cell+1);
        if (ca_cell[strlen(ca_cell)-1] == '/')
                ca_cell[strlen(ca_cell)-1] = '\0';

	cell_root = (char *) malloc(strlen(ca_cell) + strlen(ca_root) + 10);
	if(cell_root == NULL){
		fprintf(stderr,"failed malloc memory\n");
		return(NULL);
	}
	sprintf(cell_root,"%s/%s",ca_root,ca_cell);

	/* is cell_root already there?                                	*/
        if (stat(cell_root,&sbuf)){
                fprintf(stderr, "cell_root directory %s doesn't exist - making\n",
                        cell_root);
                if(mkdir(cell_root,(mode_t) 0755)){
                        fprintf(stderr,"could not mkdir %s\n",cell_root);
                        return(NULL);
                }
        }

	return(cell_root);
}

static char *sec_make_userdir(
char *cell 
) {
        char            *user_root,*user_cell,*cell_root,*home;
        struct stat     sbuf;

        home = getenv("HOME");
        if(!home){
        	fprintf(stderr,"failed get HOME variable\n");
        	return(NULL);
        }

        /* get rid of trailing slash                            */
        if (home[strlen(home)-1] == '/')
        	home[strlen(home)-1] = '\0';
	user_root = (char *) malloc(strlen(home) + strlen(SGESecPath) + 10);
        if(user_root == NULL){
        	fprintf(stderr,"failed malloc memory\n");
        	return(NULL);
        }

        sprintf(user_root, "%s/%s",home,SGESecPath);

        /* get rid of trailing slash                                    */
        if (user_root[strlen(user_root)-1] == '/')
                user_root[strlen(user_root)-1] = '\0';

        /* is user_root already there?                                */
        if (stat(user_root,&sbuf)){
                fprintf(stderr, "root directory %s doesn't exist - making\n",
                                user_root);
                if(mkdir(user_root,(mode_t) 0755)){
                        fprintf(stderr,"could not mkdir %s\n",user_root);
                        return(NULL);
                }
        }

        if(cell) 
		user_cell = cell;
        else{
                user_cell = getenv("SGE_CELL");
                if (!user_cell || strlen(user_cell) == 0)
                user_cell = DEFAULT_CELL;
        }

        /* get rid of surrounding slashs                                */
        if(user_cell[0] == '/')
                strcpy(user_cell,user_cell+1);
        if (user_cell[strlen(user_cell)-1] == '/')
                user_cell[strlen(user_cell)-1] = '\0';

        cell_root = (char *) malloc(strlen(user_cell) + strlen(user_root) + 10);
        if(cell_root == NULL){
                fprintf(stderr,"failed malloc memory\n");
                return(NULL);
        }
        sprintf(cell_root,"%s/%s",user_root,user_cell);

        /* is cell_root already there?                                  */
        if (stat(cell_root,&sbuf)){
                fprintf(stderr, "cell_root directory %s doesn't exist - making\n",
                        cell_root);
                if(mkdir(cell_root,(mode_t) 0755)){
                        fprintf(stderr,"could not mkdir %s\n",cell_root);
                        return(NULL);
                }
        }

        return(cell_root);
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
static int sec_pack_announce(int certlen, u_char *x509_buf, u_char *challenge, sge_pack_buffer *pb)
{
   int   i;

   if((i=packint(pb,(u_long32) certlen))) 
      goto error;
   if((i=packbuf(pb,(char *) x509_buf,(u_long32) certlen))) 
      goto error;
   if((i=packbuf(pb,(char *) challenge, CHALL_LEN))) 
      goto error;

   error:
      return(i);
}

static int sec_unpack_announce(int *certlen, u_char **x509_buf, u_char **challenge, sge_pack_buffer *pb)
{
   int i;

   if((i=unpackint(pb,(u_long32 *) certlen))) 
      goto error;
   if((i=unpackbuf(pb, (char **) x509_buf, (u_long32) *certlen))) 
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

   if((i=packint(pb,(u_long32) certlen))) 
      goto error;
   if((i=packbuf(pb,(char *) x509_buf,(u_long32) certlen))) 
      goto error;
   if((i=packint(pb,(u_long32) chall_enc_len))) 
      goto error;
   if((i=packbuf(pb,(char *) enc_challenge,(u_long32) chall_enc_len)))
      goto error;
   if((i=packint(pb,(u_long32) connid))) 
      goto error;
   if((i=packint(pb,(u_long32) enc_key_len))) 
      goto error;
   if((i=packbuf(pb,(char *) enc_key,(u_long32) enc_key_len))) 
      goto error;
   if((i=packint(pb,(u_long32) iv_len))) 
      goto error;
   if((i=packbuf(pb,(char *) iv,(u_long32) iv_len))) 
      goto error;
   if((i=packint(pb,(u_long32) enc_key_mat_len))) 
      goto error;
   if((i=packbuf(pb,(char *) enc_key_mat,(u_long32) enc_key_mat_len))) 
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

   if((i=unpackint(pb,(u_long32 *) certlen))) 
      goto error;
   if((i=unpackbuf(pb,(char **) x509_buf,(u_long32) *certlen))) 
      goto error;
   if((i=unpackint(pb,(u_long32 *) chall_enc_len))) 
      goto error;
   if((i=unpackbuf(pb,(char **) enc_challenge,(u_long32) *chall_enc_len)))
      goto error;
   if((i=unpackint(pb,(u_long32 *) connid))) 
      goto error;
   if((i=unpackint(pb,(u_long32 *) enc_key_len))) 
      goto error;
   if((i=unpackbuf(pb,(char **) enc_key,(u_long32) *enc_key_len)))
      goto error;
   if((i=unpackint(pb,(u_long32 *) iv_len))) 
      goto error;
   if((i=unpackbuf(pb,(char **) iv,(u_long32) *iv_len)))
      goto error;
   if((i=unpackint(pb,(u_long32 *) enc_key_mat_len))) 
      goto error;
   if((i=unpackbuf(pb,(char **) enc_key_mat,(u_long32) *enc_key_mat_len)))
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

#if 0
static int sec_pack_reconnect(u_long32 connid, u_long32 seq_send, u_long32 seq_receive, u_char *key_mat, sge_pack_buffer *pb)
{
   int i;

   if ((i=packint(pb, connid))) 
      goto error;
   if ((i=packint(pb, seq_send))) 
      goto error;
   if ((i=packint(pb, seq_receive))) 
      goto error;
   if ((i=packbuf(pb,(char *) key_mat, gsd.key_mat_len)))
      goto error;

   error:
        return(i);
}

static int sec_unpack_reconnect(u_long32 *connid, u_long32 *seq_send, u_long32 *seq_receive, 
                         u_char **key_mat, sge_pack_buffer *pb)
{
   int i;

   if((i=unpackint(pb,(u_long32 *) connid))) 
      goto error;
   if((i=unpackint(pb,(u_long32 *) seq_send))) 
      goto error;
   if((i=unpackint(pb,(u_long32 *) seq_receive))) 
      goto error;
   if((i=unpackbuf(pb,(char **) key_mat, (u_long32) gsd.key_mat_len)))
      goto error;

   error:
      return(i);
}
#endif
