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
#ifndef SEC_MAIN
#define SEC_MAIN
#endif


#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>


#include "commlib.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_prognames.h" 
#include "sge_me.h" 
#include "basis_types.h"
#include "sge_gdi_intern.h"

#include "sec_crypto.h"          /* lib protos      */
#include "sec_lib.h"             /* lib protos      */
#include "sec_other.h"           /* other protos      */
#include "sec_local.h"           /* specific definitions */
#include "sec_pack.h"            /* pack data to pb   */
#include "sec_setup_path.h"      /* for sec_setup_path   */



/*
** NAME
**   sec_set_encrypt
**
** SYNOPSIS
**   #include "sec_lib.h"
**
**   int sec_set_encrypt(tag)
**   int tag - message tag of received message
**
** DESCRIPTION
**   This function decides within a single switch by means of the
**   message tag if a message is/should be encrypted. ATTENTION:
**   every new tag, which message is not to be encrypted, must
**    occur within this switch. Tags who are not there are encrypted
**   by default.
**
** RETURN VALUES
**   1   message with tag 'tag' is/should be encrypted
**   0   message is not encrypted
*/   
int sec_set_encrypt(
int tag 
) {
   switch(tag){
      case TAG_SEC_ANNOUNCE:
      case TAG_SEC_RESPOND:
      case TAG_SEC_ERROR: 
         return(0);

      case TAG_OLD_REQUEST: 
      case TAG_ACK_REQUEST:
      case TAG_GDI_REQUEST:
      case TAG_REPORT_REQUEST:
      case TAG_FINISH_REQUEST:
      case TAG_JOB_EXECUTION:
      case TAG_SIGJOB: 
      case TAG_SIGQUEUE:
      case TAG_KILL_EXECD:
      case TAG_NEW_FEATURES:
      case TAG_GET_NEW_CONF:
      case TAG_JOB_REPORT:         
      default: 
         return(1); 
   }
}

/*
** NAME
**   sec_files
**
** SYNOPSIS
**      #include "sec_lib.h"
**
**   int sec_files()
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
int sec_files()
{
   int     i = 0;
   FILE    *fp = NULL;

   DENTER(TOP_LAYER,"sec_files");

   /* 
   ** setup the filenames of certificates, keys, etc.
   */
   gsd.files = sec_setup_path(NULL,gsd.issgesys);
   if (gsd.files == NULL){
      ERROR((SGE_EVENT,"failed sec_setup_path\n"));
      goto error;
   }

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
/*        
        gsd.x509 = (X509 *) X509_new();
        if(gsd.x509 == NULL){
                sec_error();
                i = -1;
                goto error;
        }
   i = PEM_read_X509(fp,gsd.x509, NULL, NULL); 
*/
   gsd.x509 = PEM_read_X509(fp, NULL, NULL, NULL);
/*         if (i <= 0){ */
   if (gsd.x509 == NULL) {
      sec_error();
      i = -1;
      goto error;
   }
   fclose(fp);
   gsd.x509_len = i2D_X509(gsd.x509, NULL);

   /* 
   ** verify own certificate
   */
/*    i=X509_verify(gsd.x509, sec_verify_callback); */
   i=X509_verify(gsd.x509, NULL);
   if (i <= 0) {
      ERROR((SGE_EVENT,"error: failed verify own certificate\n"));
      sec_error();
      i = -1;
      goto error;
   }

   /* 
   ** read rsa key from file
   */
   fp = fopen(gsd.files->key_file, "r");
   if (fp == NULL){
      ERROR((SGE_EVENT,"Can't open Key _file '%s': %s!\n",
                        gsd.files->key_file,strerror(errno)));
      i = -1;
      goto error;
   }
   gsd.rsa = (RSA *) RSA_new();
   if (gsd.rsa == NULL) {
      sec_error();
      i = -1;
      goto error;
   }
   i = PEM_read_RSA(fp, gsd.rsa);
   if (i <= 0) {
      sec_error();
      i = -1;
      goto error;
   }
   fclose(fp);

   if (!gsd.issgesys)
      sec_reconnect();

   DEXIT;
   return 0;
    
   error:
      if (fp) 
         fclose(fp);
      DEXIT;
      return i;
}

/*
** NAME
**   sec_init
**
** SYNOPSIS
**      #include "sec_lib.h"
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
char *progname 
) {
   int   i=0;
   u_long   l=time(NULL);

   DENTER(TOP_LAYER,"sec_init");

   ERR_clear_error();
   ERR_load_crypto_strings();

   /* 
   ** rand seed needs to be improved
   */
   RAND_seed((unsigned char *)&l, sizeof(l));

   /* 
   ** am I a sge daemon?
   */
   if(!strcmp(prognames[QMASTER],progname) ||
      !strcmp(prognames[EXECD],progname) ||
      !strcmp(prognames[SCHEDD],progname))
      gsd.issgesys = 1;
   else
      gsd.issgesys = 0;

   /* 
   ** Cipher mode specific settings
   */
   switch (CIPHER_MODE) {
      case DES_192_EDE3_CBC_WITH_MD5 :
         gsd.crypt_init = sec_des_ede3_cbc_init;
         gsd.crypt = sec_des_ede3_cbc;
         gsd.block_len = GSD_BLOCK_LEN;
         gsd.keys_len = sizeof(DES_EDE3_CBC_STATE);
         gsd.key_mat_len = GSD_KEY_MAT_32;   
         /* 
         ** must be shorter or equal to 32 bytes
         ** as long as the key_mat is konverted to Ulong for list
         */
         break;

      case DES_64_CBC_WITH_MD5 :
         gsd.crypt_init = sec_des_cbc_init;
         gsd.crypt = sec_des_cbc;
         gsd.block_len = GSD_BLOCK_LEN;
         gsd.keys_len = sizeof(DES_CBC_STATE);
         gsd.key_mat_len = GSD_KEY_MAT_16;
         break;

      default :
         ERROR((SGE_EVENT,"error: Unknown Cipher mode!\n"));
         i = -1;
         goto error;
   }

   /* 
   ** init all the other stuff
   */
   gsd.key_mat = (u_char *) malloc(gsd.key_mat_len);
   gsd.keys = (char *) malloc(gsd.keys_len);
   gsd.refresh_time = (char *) malloc(64);
   if (!gsd.keys || !gsd.key_mat || !gsd.refresh_time){
      ERROR((SGE_EVENT,"error: failed malloc memory!\n"));
      i = -1;
      goto error;
   }
   if(!strcmp(progname, prognames[QMASTER]))
      /* time after qmaster clears the connection list   */
      X509_gmtime_adj(gsd.refresh_time,0);
   else
      /* time after client makes a new announce to master   */
      X509_gmtime_adj(gsd.refresh_time,60*60*(ValidHours-1));
   gsd.crypt_space = 3*INTSIZE + 2*gsd.block_len + MD5_DIGEST_LENGTH;
   gsd.connid = gsd.connid_counter = gsd.seq_send = gsd.connect = 
   gsd.seq_receive = 0;
   gsd.conn_list = NULL;

   /* 
   ** install sec_exit functions for utilib
   */
   if(!strcmp(progname, prognames[QMASTER])){
      install_sec_exit_func(sec_dump_connlist);
      if(sec_undump_connlist())
                WARNING((SGE_EVENT,"warning: Can't read ConnList\n"));
   }
   else if(!gsd.issgesys)
           install_sec_exit_func(sec_write_data2hd);

   /* 
   ** read security related files
   */
   if(sec_files()){
      ERROR((SGE_EVENT,"error: failed read security files\n"));
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
**   sec_dump_connlist
**
** SYNOPSIS
**      #include "sec_lib.h"
**
**   int sec_dump_connlist()
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
int sec_dump_connlist()
{
   FILE    *fp;

   if (me.who == QMASTER) {
      fp = fopen("sec_connlist.dat","w");
      if (fp) {
         lDumpList(fp,gsd.conn_list,0);
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
**      #include "sec_lib.h"
**
**   int sec_undump_connlist()
**
** DESCRIPTION
**   This function reads the connection list from disk when master
**   is starting up.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
*/
int sec_undump_connlist()
{
        FILE    *fp;

        if(me.who == QMASTER){
                fp = fopen("sec_connlist.dat","r");
                if(fp){
                        gsd.conn_list = lUndumpList(fp,NULL,NULL);
                        fclose(fp);
                }
                else
                        return(-1);
                fp = fopen("sec_connid.dat","r");
                if(fp){
                        fscanf(fp, u32, &gsd.connid_counter);
                        fclose(fp);
                }
                else
                        return(-1);
        } 
        return(0);
}

/*
** NAME
**
** SYNOPSIS
**      #include "sec_lib.h"
**
**   int sec_announce_connection(commproc,cell)
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
int sec_announce_connection(
char *commproc,
char *cell 
) {
   X509_STORE *ctx = NULL;
   X509_STORE_CTX csc;
   EVP_PKEY *public_key = NULL;
   
   int i, x509_len, key_enc_len, chall_enc_len;
   X509 *x509=NULL;
   RSA *rsa=NULL;
   MD5_CTX c;
   u_char challenge[CHALL_LEN], *x509_buf=NULL, *enc_challenge=NULL,
          *enc_key_mat=NULL, *tmp, challenge_mac[MD5_DIGEST_LENGTH];
   sge_pack_buffer pb;
   char fromhost[MAXHOSTLEN], *buffer;
   char fromcommproc[MAXCOMPONENTLEN];
   int fromtag;
   u_short fromid;
   u_short compressed = 0;
   u_long32 dummymid, buflen;
   
   DENTER(TOP_LAYER, "sec_announce_connection");

   /* 
   ** send sec_announce
   */
   gsd.seq_send = gsd.seq_receive = 0;

   /* 
   ** prepare packing buffer
   */
   if ((i=init_packbuffer(&pb, 4096,0))) {
      ERROR((SGE_EVENT,"error: failed init_packbuffer\n"));
      goto error;
   }

   /* 
   ** write x509 to buffer
   */
   x509_buf = (u_char *) malloc(4096);
   if (!x509_buf) {
      ERROR((SGE_EVENT,"error: out of memory\n"));
      i=-1;
      goto error;
   }
   tmp = x509_buf;      /* because i2D moves buffer      */
   i2D_X509(gsd.x509,&tmp);

   /* 
   ** make challenge
   */
   MD5_rand(CHALL_LEN,challenge);

   /* 
   ** write data to packing buffer
   */
   i = sec_pack_announce(gsd.x509_len,x509_buf,challenge,&pb);
   if (i) {
      ERROR((SGE_EVENT,"error: failed pack announce buffer\n"));
      goto error;
   }

   /* 
   ** send buffer
   */
   printf("send announce to=(%s:%s)\n",cell,commproc);
   if (send_message_pb(SYNCHRON, commproc, 0, cell, TAG_SEC_ANNOUNCE, &pb,
                        &dummymid)) {
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
   if (receive_message(fromcommproc,&fromid,fromhost,&fromtag,
                        &buffer,&buflen,SYNCHRON, &compressed)) {
      ERROR((SGE_EVENT,"error: failed get sec_response from (%s:%d:%s:%d)\n",
                fromcommproc,fromid,fromhost,fromtag));
      i = -1;
      goto error;
   }

   pb.head_ptr = buffer;
   pb.mem_size = buflen;
   pb.cur_ptr = pb.head_ptr;
   pb.bytes_used = 0;

   /*
   printf("received response from=(%s:%s:%d) tag=%d buflen=%d\n",
           fromhost, fromcommproc, fromid, fromtag, pb.mem_size);
   */

   /* 
   ** manage error or wrong tags
   */
   if(fromtag == TAG_SEC_ERROR){
      ERROR((SGE_EVENT,"master reports error: "));
      ERROR((SGE_EVENT,pb.cur_ptr));
      i=-1;
      goto error;
   }
   if (fromtag != TAG_SEC_RESPOND) {
      ERROR((SGE_EVENT,"error: received unexpected TAG from master\n"));
      i=-1;
      goto error;
   }

   /* 
   ** unpack data from packing buffer
   */
   if (x509_buf) 
      free(x509_buf);
   i = sec_unpack_response(&x509_len, &x509_buf, &chall_enc_len, &enc_challenge,
                           &gsd.connid, &key_enc_len, &enc_key_mat,&pb);
   if (i) {
      ERROR((SGE_EVENT,"error: failed unpack response buffer\n"));
      goto error;
   }
   
   /* 
   ** read x509 certificate from buffer
   */
   x509 = (X509 *) X509_new_D2i_X509(x509_len, x509_buf);
   if (!x509) {
      ERROR((SGE_EVENT,"error: failed read foreign certificate\n"));
      sec_error();
      i = -1;
      goto error;
   }

   ctx = X509_STORE_new();
   X509_STORE_set_default_paths(ctx);
   X509_STORE_CTX_init(&csc, ctx, x509, NULL);

   /*
   ** verify certificate
   */
   i = X509_verify_cert(&csc);
   X509_STORE_CTX_cleanup(&csc);
   if (!i) {
      ERROR((SGE_EVENT,"error: failed verify foreign certificate\n"));
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
   public_key = X509_get_pubkey(x509);
   
   if(!public_key){
      ERROR((SGE_EVENT,"error: cant get public key\n"));
      sec_error();
      i=-1;
      goto error;
   }

   /* 
   ** decrypt challenge with master public key
   */
   i = RSA_public_decrypt(chall_enc_len, enc_challenge, enc_challenge,
                           public_key->pkey.rsa, RSA_PKCS1_PADDING);
   if (i<=0) {
      ERROR((SGE_EVENT,"error: failed decrypt challenge\n"));
      sec_error();
      i=-1;
      goto error;
   }

   /* 
   ** make MAC from own challenge
   */
   MD5_Init(&c);
   MD5_Update(&c,challenge,(u_long) CHALL_LEN);
   MD5_Final(challenge_mac,&c);
   
   /* 
   ** compare your challenge with challenge from master
   */
   if (memcmp(challenge_mac,enc_challenge,CHALL_LEN)){
      ERROR((SGE_EVENT,"error: challenge from master is bad\n"));
      i=-1;
      goto error;
   }
 
   /* 
   ** decrypt key_mat with client private key
   */
   i = RSA_private_decrypt(key_enc_len, enc_key_mat, enc_key_mat, 
                           gsd.rsa, RSA_PKCS1_PADDING);
   if (i<=0) {
      ERROR((SGE_EVENT,"error: failed decrypt keys\n"));
      sec_error();
      i=-1;
      goto error;
   }

   /* 
   ** make keys from key_mat
   */
   memcpy(gsd.key_mat,enc_key_mat,gsd.key_mat_len);
   gsd.crypt_init(gsd.key_mat);

   i=0;
   error:
   clear_packbuffer(&pb);
   if (x509_buf) 
      free(x509_buf);
   if (enc_challenge) 
      free(enc_challenge);
   if (enc_key_mat) 
      free(enc_key_mat);
   if (x509) 
      X509_free(x509);
   if (rsa) 
      RSA_free(rsa);
   DEXIT;
   return i;
}

/*
** NAME
**    sec_respond_announce
**
** SYNOPSIS
**      #include "sec_lib.h"
**
**   int sec_respond_announce(commproc,id,host,buffer,buflen)
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
int sec_respond_announce(char *commproc, u_short id, char *host, 
                         char *buffer, u_long32 buflen)
{
   X509_STORE *ctx = NULL;
   X509_STORE_CTX csc;
   EVP_PKEY *public_key = NULL;

   int i, x509_len, chall_enc_len, key_enc_len;
   u_char *x509_buf=NULL, *enc_key_mat=NULL, *tmp=NULL, *challenge=NULL,
          *enc_challenge=NULL, challenge_mac[MD5_DIGEST_LENGTH];
   X509 *x509=NULL;
   RSA *rsa=NULL;
   MD5_CTX c;
   sge_pack_buffer pb, pb2;
   u_long32 dummymid;

   DENTER(TOP_LAYER, "sec_respond_announce");

   /* 
   ** read announce from packing buffer
   */

   /* 
   ** prepare packbuffer for unpacking message
   */
   pb.head_ptr = buffer;
   pb.cur_ptr = pb.head_ptr;
   pb.bytes_used = 0;
   pb.mem_size = buflen;

   /* 
   ** prepare packbuffer for sending message
   */
   if ((i=init_packbuffer(&pb2, 4096,0))) {
      ERROR((SGE_EVENT,"error: failed init_packbuffer\n"));
      sec_send_err(commproc,id,host,&pb2,"failed init_packbuffer\n");
      goto error;
   }

   i = sec_unpack_announce(&x509_len, &x509_buf, &challenge, &pb);
   if (i) {
      ERROR((SGE_EVENT,"error: failed unpack announce\n"));
      sec_send_err(commproc, id, host, &pb2, "failed unpack announce\n");
      goto error;
   }

   /* 
   ** read x509 certificate
   */
   x509 = (X509 *) X509_new_D2i_X509(x509_len, x509_buf);
   if (!x509){
      ERROR((SGE_EVENT,"error: failed read foreign certificate\n"));
      sec_error();
      sec_send_err(commproc, id, host, &pb2, "failed read your certificate\n");
      i = -1;
      goto error;
   }

   ctx = X509_STORE_new();
   X509_STORE_set_default_paths(ctx);
   X509_STORE_CTX_init(&csc, ctx, x509, NULL);

   /*
   ** verify certificate
   */
   i = X509_verify_cert(&csc);
   X509_STORE_CTX_cleanup(&csc);
   if (!i) {
      ERROR((SGE_EVENT,"error: failed verify foreign certificate\n"));
      sec_error();
      sec_send_err(commproc, id, host, &pb2, "Your certificate is bad\n");
      i = -1;
      goto error;
   }
   else {
      printf("Certificate is ok\n");
   }   
      
   /*
   ** extract public key 
   */
   public_key = X509_get_pubkey(x509);
   
   if(!public_key){
      ERROR((SGE_EVENT,"error: cant get public key\n"));
      sec_error();
      sec_send_err(commproc,id,host,&pb2,"failed extract key from cert\n");
      i=-1;
      goto error;
   }

   /* 
   ** malloc several buffers
   */
   if (x509_buf) 
      free(x509_buf);
   x509_buf = (u_char *) malloc(4096);
   enc_key_mat = (u_char *) malloc(1024);
   enc_challenge = (u_char *) malloc(1024);

   if (!x509_buf || !enc_key_mat || !enc_challenge) {
      ERROR((SGE_EVENT,"error: out of memory\n"));
      sec_send_err(commproc,id,host,&pb2,"failed malloc memory\n");
      i=-1;
      goto error;
   }
   
   /* 
   ** make key material
   */
   MD5_rand(gsd.key_mat_len,gsd.key_mat);

   /* 
   ** write response
   */

   /* 
   ** set connection ID
   */
   gsd.connid = gsd.connid_counter;

   /* 
   ** write x509 certificate to buffer
   */
   tmp = x509_buf;           /* because i2D moves buffer           */
   i2D_X509(gsd.x509,&tmp);

   /* 
   ** make MAC from challenge
   */
   MD5_Init(&c);
   MD5_Update(&c,challenge,(u_long) CHALL_LEN);
   MD5_Final(challenge_mac,&c);

   /* 
   ** encrypt challenge with master rsa_privat_key
   */
   chall_enc_len = RSA_private_encrypt(CHALL_LEN, challenge_mac, enc_challenge,
                                       gsd.rsa, RSA_PKCS1_PADDING);
   if (chall_enc_len <= 0) {
      ERROR((SGE_EVENT,"error: failed encrypt challenge MAC\n"));
      sec_error();
      sec_send_err(commproc,id,host,&pb2,"failed encrypt challenge MAC\n");
      i = -1;
      goto error;
   }

   /* 
   ** encrypt key_mat with client rsa_public_key
   */
   key_enc_len = RSA_public_encrypt(gsd.key_mat_len, gsd.key_mat, enc_key_mat, 
                                    public_key->pkey.rsa, RSA_PKCS1_PADDING);
   if (key_enc_len <= 0) {
      ERROR((SGE_EVENT,"error: failed encrypt keys\n"));
      sec_error();
      sec_send_err(commproc,id,host,&pb2,"failed encrypt keys\n");
      i = -1;
      goto error;
   }

   /* 
   ** write response data to packing buffer
   */
   i=sec_pack_response(gsd.x509_len, x509_buf, chall_enc_len, enc_challenge,
                       gsd.connid, key_enc_len, enc_key_mat, &pb2);
   if (i) {
      ERROR((SGE_EVENT,"error: failed write response to buffer\n"));
      sec_send_err(commproc,id,host,&pb2,"failed write response to buffer\n");
      goto error;   
   }

   /* 
   ** insert connection to Connection List
   */
   i = sec_insert_conn2list(host,commproc,id);
   if (i) {
      ERROR((SGE_EVENT,"error: failed insert Connection to List\n"));
      sec_send_err(commproc, id, host, &pb2, "failed insert you to list\n");
      goto error;
   }

   /* 
   ** send response
   */
   printf("send response to=(%s:%s:%d)\n", host, commproc, id);
   if (send_message_pb(0, commproc, id, host, TAG_SEC_RESPOND, &pb2, &dummymid)) {
      ERROR((SGE_EVENT,"error: Send message to (%s:%d:%s)\n", commproc,id,host));
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
      clear_packbuffer(&pb2);
      if (x509) 
         X509_free(x509);
      if (rsa) 
         RSA_free(rsa);

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
char *tocomproc,
int toid,
char *tohost,
int tag,
char *buffer,
int buflen,
u_long32 *mid,
int compressed 
) {
   int i;

   DENTER(TOP_LAYER,"sec_send_message");

   i = sec_time_cmp(gsd.refresh_time);
   if ((me.who != QMASTER) && ((gsd.connect != 1) || (i < 0))) {
      if(sec_announce_connection(tocomproc,tohost)) {
         ERROR((SGE_EVENT,"error: failed announce\n"));
         i = SEC_ANNOUNCE_FAILED;
         goto error;
      }
      X509_gmtime_adj(gsd.refresh_time,60*60*(ValidHours-1));
      gsd.connect = 1;
   }

   if (sec_set_encrypt(tag)) {
      if (me.who == QMASTER) {
         if(sec_set_secdata(tohost,tocomproc,toid)) {
            ERROR((SGE_EVENT,"error: failed set security data\n"));
            i = SEC_SEND_FAILED;
            goto error;
         }
      }
      if (sec_encrypt(&buffer,&buflen)) {
         ERROR((SGE_EVENT,"error: failed encrypt message\n"));
         i = SEC_SEND_FAILED;
         goto error;
      }
   }
   else if (me.who != QMASTER) {
      if (sec_set_connid(&buffer, &buflen)) {
         ERROR((SGE_EVENT,"error: failed set connection ID\n"));
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
**   sec_encrypt
**
** SYNOPSIS
**      #include "sec_lib.h"
**
**   int sec_encrypt(buffer,buflen)
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
int sec_encrypt(
char **buffer,
int *buflen 
) {
   int i;
   u_char seq_str[INTSIZE], mac[MD5_DIGEST_LENGTH+2*GSD_BLOCK_LEN];
   u_long32 seq_no, mssg_enc_len, mac_enc_len, enc_buflen;
   MD5_CTX c;
   lListElem *element=NULL;
   lCondition *where=NULL;
   sge_pack_buffer pb;

   
   DENTER(TOP_LAYER,"sec_encrypt");   
   /* 
   ** set pointers to for message
   */

   enc_buflen = *buflen + gsd.crypt_space;
   *buffer = (char *) realloc(*buffer,enc_buflen);   
   if (!*buffer) {
      i = -1;
      ERROR((SGE_EVENT,"error: failed malloc memory\n"));
      goto error;
   }
   
   /* 
   ** make MAC
   */
   seq_no = htonl(gsd.seq_send);
   memcpy(seq_str,&seq_no,INTSIZE);

   MD5_Init(&c);
   MD5_Update(&c,gsd.key_mat,gsd.key_mat_len);
   MD5_Update(&c,seq_str,INTSIZE);
   MD5_Update(&c,*buffer,*buflen);
   MD5_Final(mac,&c);

   /* 
   ** encrypt mac
   */
   mac_enc_len = gsd.crypt(MD5_DIGEST_LENGTH, mac, mac, DES_ENCRYPT);
   if (mac_enc_len <= 0) {
      ERROR((SGE_EVENT,"error: failed encrypt MAC\n"));
      sec_error();
      i=-1;
      goto error;
   }

   /* 
   ** encrypt message
   */
   mssg_enc_len = gsd.crypt(*buflen, (u_char*)*buffer, (u_char*)*buffer, DES_ENCRYPT);   
   if (mssg_enc_len <=0) {
      ERROR((SGE_EVENT,"error: failed encrypt message\n"));
      sec_error();
      i=-1;
      goto error;
   }

   /* 
   ** get packing buffer ready for crypt data
   */
   pb.head_ptr = *buffer;
   pb.cur_ptr = pb.head_ptr + *buflen + gsd.block_len;
   pb.bytes_used = *buflen + gsd.block_len;
   pb.mem_size = enc_buflen;

   /* 
   ** pack data to packing buffer
   */
   i = sec_pack_message(gsd.connid,mac_enc_len,mac,mssg_enc_len,&pb);
   if (i) {
      ERROR((SGE_EVENT,"error: failed pack message to buffer\n"));
      goto error;
   }
   
   gsd.seq_send = INC32(gsd.seq_send);

   if (gsd.conn_list) {
      /* 
      ** search list element for this connection
      */
      where = lWhere( "%T(%I==%u)", SecurityT, SEC_ConnectionID, gsd.connid);
      element = lFindFirst(gsd.conn_list,where);
      if (!element || !where) {
         ERROR((SGE_EVENT,"error: no list entry for connection\n"));
         i=-1;
         goto error;
      }
      lSetUlong(element,SEC_SeqNoSend,gsd.seq_send);
   }   

   *buflen = enc_buflen;
   i=0;

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
**      #include "sec_lib.h"
**
**   int sec_handle_announce(commproc,id,host,buffer,buflen)
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
int sec_handle_announce(char *commproc, u_short id, char *host, char *buffer, u_long32 buflen)
{
   int i;
   u_long32 mid;
   struct stat file_info;
   int compressed = 0;

   DENTER(TOP_LAYER,"sec_handle_announce");

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
   else if (gsd.issgesys) {
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
         ERROR((SGE_EVENT,"error: failed remove reconnect file '%s'\n",
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

   i = receive_message(fromcommproc,fromid,fromhost,tag,buffer,buflen,
            synchron, compressed);
   if (i) 
      goto error;

   if (*tag == TAG_SEC_ANNOUNCE) {
      i = sec_handle_announce(fromcommproc,*fromid,fromhost,*buffer,
               *buflen);
      if (i) {
         ERROR((SGE_EVENT,"error: failed handle announce for (%s:%s:%d)\n",
                  fromhost,fromcommproc,*fromid));
         i = SEC_RECEIVE_FAILED;
         goto error;   
      }
   }
   else if (sec_set_encrypt(*tag)) {
      i = sec_decrypt(buffer,buflen,fromhost,fromcommproc,*fromid);
      if (i) {
         ERROR((SGE_EVENT,"error: failed decrypt message (%s:%s:%d)\n",
                  fromhost,fromcommproc,*fromid));
         i = SEC_RECEIVE_FAILED;
         if (sec_handle_announce(fromcommproc,*fromid,fromhost,NULL,0))
            ERROR((SGE_EVENT,"error: failed handle decrypt error\n"));
         goto error;   
      }
   }
   else if (me.who == QMASTER) {
      if (sec_get_connid(buffer,buflen) ||
               sec_update_connlist(fromhost,fromcommproc,*fromid)) {
         ERROR((SGE_EVENT, "error: failed get connection ID\n"));
         i = SEC_RECEIVE_FAILED;
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
**      #include "sec_lib.h"
**
**   int sec_decrypt(buffer,buflen,host,commproc,id)
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

int sec_decrypt(char **buffer, u_long32 *buflen, char *host, char *commproc, int id)
{
   int i;
   u_char *enc_mac_ptr=NULL, seq_str[INTSIZE], mac[MD5_DIGEST_LENGTH],
          enc_mac[MD5_DIGEST_LENGTH+2*GSD_BLOCK_LEN];
   u_long32 connid, seq_no, mssg_len, mssg_enc_len, mac_enc_len;
   MD5_CTX c;
   lListElem *element=NULL;
   lCondition *where=NULL;
   sge_pack_buffer pb;

   DENTER(TOP_LAYER, "sec_decrypt");

   mssg_len = *buflen - gsd.crypt_space;

   /* 
   ** prepare packing buffer
   */
   pb.head_ptr = *buffer;
   pb.cur_ptr = pb.head_ptr + mssg_len + gsd.block_len;
   pb.bytes_used = mssg_len + gsd.block_len;;
   pb.mem_size = *buflen;

   /* 
   ** unpack data from packing buffer
   */
   i = sec_unpack_message(&connid,&mac_enc_len,&enc_mac_ptr,&mssg_enc_len,&pb);
   memcpy(enc_mac,enc_mac_ptr,mac_enc_len);

   /*
   ** reset the packing buffer for errors
   */
   pb.cur_ptr = pb.head_ptr;
   pb.bytes_used = 0;

   if (i) {
      ERROR((SGE_EVENT,"error: failed unpack message from buffer\n"));
      packstr(&pb,"Can't unpack your message from buffer\n");
      goto error;
   }

   if (gsd.conn_list) {
      sec_clearup_list();
      /* 
      ** search list element for this connection
      */
      where = lWhere( "%T(%I==%u)", SecurityT, SEC_ConnectionID, connid);
      element = lFindFirst(gsd.conn_list,where);
      if (!element || !where) {
         ERROR((SGE_EVENT,"error: no list entry for connection\n"));
         packstr(&pb,"Can't find you in connection list\n");
         i=-1;
         goto error;
      }
   
      /* 
      ** get data from connection list
      */
      sec_list2keymat(element);
      gsd.crypt_init(gsd.key_mat);
      gsd.seq_receive = lGetUlong(element,SEC_SeqNoReceive);
      gsd.connid = connid;
   }
   else {
      if (gsd.connid != connid) {
         ERROR((SGE_EVENT,"error: received wrong connection ID\n"));
         ERROR((SGE_EVENT,"it is %ld, but it should be %ld!\n",
                  connid, gsd.connid));
         packstr(&pb,"Probably you send the wrong connection ID\n");
         i = -1;
         goto error;
      }
   }


   /* 
   ** decrypt Message Authentication Code MAC
   */
   i = gsd.crypt(mac_enc_len, enc_mac, enc_mac, DES_DECRYPT);
   if (i <= 0) {
      ERROR((SGE_EVENT,"error: failed decrypt MAC\n"));
      packstr(&pb,"Can't decrypt MAC\n");
      sec_error();
      i=-1;
      goto error;
   }

   /* 
   ** decrypt message
   */
   i = gsd.crypt(mssg_enc_len, (u_char*)*buffer, (u_char*)*buffer, DES_DECRYPT);
   if (i <= 0) {
      ERROR((SGE_EVENT,"error: failed decrypt message\n"));
      packstr(&pb,"Can't decrypt message\n");
      sec_error();
      i=-1;
      goto error;
   }

   /* 
   ** make MAC and compare it with received Mac
   */
   seq_no = htonl(gsd.seq_receive);
   memcpy(seq_str,&seq_no,INTSIZE);

   MD5_Init(&c);
   MD5_Update(&c,gsd.key_mat,(u_long) gsd.key_mat_len);
   MD5_Update(&c,seq_str,INTSIZE);
   MD5_Update(&c,*buffer,mssg_len);
   MD5_Final(mac,&c);

   if (memcmp(mac,enc_mac,MD5_DIGEST_LENGTH)) {
      ERROR((SGE_EVENT,"error: the received MAC is bad\n"));
      packstr(&pb,"The received MAC is bad\n");
      i=-1;
      goto error;
   }

   /* increment sequence number and write it to connection list   */
   gsd.seq_receive = INC32(gsd.seq_receive);
   if(gsd.conn_list){
      lSetString(element,SEC_Host,host);
      lSetString(element,SEC_Commproc,commproc);
      lSetInt(element,SEC_Id,id);
      lSetUlong(element,SEC_SeqNoReceive,gsd.seq_receive);
   }

   /* shorten buflen                  */
   *buflen = mssg_len;

   i=0;
   error:
   if(where)
       lFreeWhere(where);
   if(enc_mac_ptr) free(enc_mac_ptr);
   DEXIT;
   return(i);
}

/*
** NAME
**   sec_reconnect
**
** SYNOPSIS
**      #include "sec_lib.h"
**
**   int sec_reconnect()
**
** DESCRIPTION
**   This function reads the security related data from disk and decrypts
**   it for clients.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
*/

int sec_reconnect()
{
   FILE *fp=NULL;
   int i, c, nbytes;
   sge_pack_buffer pb;
   struct stat file_info;
   char *ptr;

   DENTER(TOP_LAYER,"sec_reconnect");

   if (stat(gsd.files->reconnect_file, &file_info) < 0) {
      i = -1;
      goto error;
   }

   /* open reconnect file                  */
   fp = fopen(gsd.files->reconnect_file,"r");
   if (!fp) {
      i = -1;
      ERROR((SGE_EVENT,"error: can't open file '%s': %s\n",
            gsd.files->reconnect_file, strerror(errno)));
      goto error;
   }

   /* read time of connection validity            */
   ptr = gsd.refresh_time;
   for (i=0; i<63; i++) {
      c = getc(fp);
      if (c == EOF) {
         ERROR((SGE_EVENT,"error: can't read time from reconnect file\n"));
         i = -1;
         goto error;
      }
      *(ptr++) = (char) c;
   }      
      
   /* prepare packing buffer                                       */
   if ((i=init_packbuffer(&pb, 256,0))) {
      ERROR((SGE_EVENT,"error: failed init_packbuffer\n"));
      goto error;
   }

   ptr = pb.head_ptr;               
   for (i=0; i<file_info.st_size-63; i++) {
      c = getc(fp);
      if (c == EOF){
         ERROR((SGE_EVENT,"error: can't read data from reconnect file\n"));
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
      ERROR((SGE_EVENT,"error: failed decrypt reconnect data\n"));
      sec_error();
      i = -1;
      goto error;
   }

   i = sec_unpack_reconnect(&gsd.connid, &gsd.seq_send, &gsd.seq_receive,
                              &gsd.key_mat, &pb);
   if (i) {
      ERROR((SGE_EVENT,"error: failed read reconnect from buffer\n"));
      goto error;
   }

   gsd.crypt_init(gsd.key_mat);
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
**      #include "sec_lib.h"
**
**   int sec_write_data2hd()
**
** DESCRIPTION
**   This function writes the security related data to disk, encrypted
**   with RSA private key.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
*/

int sec_write_data2hd()
{
   FILE   *fp=NULL;
   int   i,nbytes,ret;
   sge_pack_buffer pb;
   char   *ptr; 

   DENTER(TOP_LAYER,"sec_write_data2hd");

   /* prepare packing buffer                                       */
   if ((i=init_packbuffer(&pb, 256,0))) {
      ERROR((SGE_EVENT,"error: failed init_packbuffer\n"));
      goto error;
   }
   
   /* write data to packing buffer               */
   i = sec_pack_reconnect(gsd.connid,gsd.seq_send,gsd.seq_receive, 
                           gsd.key_mat,&pb);
   if (i) {
      ERROR((SGE_EVENT,"error: failed write reconnect to buffer\n"));
      goto error;
   }

   /* encrypt data with own public rsa key            */
   nbytes = RSA_public_encrypt(pb.bytes_used, (u_char*)pb.head_ptr, 
                               (u_char*)pb.head_ptr, gsd.rsa, 
                               RSA_PKCS1_PADDING);
   if (nbytes <= 0) {
      ERROR((SGE_EVENT,"error: failed encrypt reconnect data\n"));
      sec_error();
      i = -1;
      goto error;
   }

   /* open reconnect file                  */
   fp = fopen(gsd.files->reconnect_file,"w");
   if (!fp) {
      i = -1;
      ERROR((SGE_EVENT,"error: can't open file '%s': %s\n",
               gsd.files->reconnect_file, strerror(errno)));
      goto error;
   }

   if (!fp) {
      ERROR((SGE_EVENT,"error: failed open reconnect file '%s'\n",
             gsd.files->reconnect_file));
      i = -1;
      goto error;
   }

   /* write time of connection validity to file                    */
   for (i=0; i<63; i++) {
      ret = putc((int) gsd.refresh_time[i], fp);
      if (ret == EOF) {
         ERROR((SGE_EVENT,"error: can't write to file\n"));
         i = -1;
         goto error;
      }
   }

   /* write buffer to file                  */
   ptr = pb.head_ptr;
   for (i=0; i<nbytes; i++) {
      ret = putc((int) *(ptr+i),fp);
      if (ret == EOF) {
         ERROR((SGE_EVENT,"error: can't write to file\n"));
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


