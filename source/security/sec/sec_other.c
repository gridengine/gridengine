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
#include <stdlib.h>            /* free         */
#include <sys/stat.h>             /* stat         */
#include <string.h>

#include "sgermon.h"
#include "sge_log.h"
#include "basis_types.h"
#include "sge_gdi_intern.h"

#include "sec_other.h"
#include "sec_local.h"

/*
** NAME
**   sec_error
**
** SYNOPSIS
**      #include "sec_other.h"
**   
**   void sec_error()
**
** DESCRIPTION
**      This function prints error messages from crypto library
**
*/
void sec_error(void)
{
   long   l;

   DENTER(TOP_LAYER,"sec_crypt_error");
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
**      #include "sec_other.h"
**
**   int sec_send_err(commproc,id,host,pb,err_msg)
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

int sec_send_err(
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
   ERROR((SGE_EVENT,"error: failed send error message\n"));
   DEXIT;
   return(-1);
}

/*
** NAME
**   sec_set_connid
**
** SYNOPSIS
**      #include "sec_other.h"
**
**   int sec_set_connid(buffer,buflen)
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

int sec_set_connid(char **buffer, u_long32 *buflen)
{
   u_long32 i, new_buflen;
   sge_pack_buffer pb;

   DENTER(TOP_LAYER,"sec_set_connid");

   new_buflen = *buflen + INTSIZE;

   pb.head_ptr = *buffer;
   pb.bytes_used = *buflen;
   pb.mem_size = *buflen;

   if((i = packint(&pb,gsd.connid))){
      ERROR((SGE_EVENT,"error: failed pack ConnID to buffer"));
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
**      #include "sec_other.h"
**
**   int sec_get_connid(buffer,buflen)
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

int sec_get_connid(char **buffer, u_long32 *buflen)
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
      ERROR((SGE_EVENT,"error: failed unpack ConnID from buffer"));
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
**      #include "sec_other.h"
**
**   int sec_update_connlist(host,commproc,id)
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

int sec_update_connlist(char *host, char *commproc, int id)
{
   int ret = 0;
   lListElem *element=NULL;
   lCondition *where=NULL;

   DENTER(TOP_LAYER,"sec_update_connlist");

   where = lWhere( "%T(%I==%u)",SecurityT,SEC_ConnectionID,gsd.connid);
   element = lFindFirst(gsd.conn_list,where);
   if(!element || !where){
      ERROR((SGE_EVENT,"error: no list entry for connection\n"));
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
**      #include "sec_other.h"
**
**   int sec_set_secdata(host,commproc,id)
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
int sec_set_secdata(char *host, char *commproc, int id)
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
      ERROR((SGE_EVENT,"error: no list entry for connection (%s:%s:%d)\n!",
             host,commproc,id));
      i=-1;
      goto error;
   } 
   
   /* 
   ** set security data                  
   */
   sec_list2keymat(element);
   gsd.crypt_init(gsd.key_mat);
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
**      #include "sec_other.h"
**
**   int sec_insert_conn2list(host,commproc,id)
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
int sec_insert_conn2list(char *host, char *commproc, int id)
{
   int ret = 0;
   char time_str[64];
   lListElem    *element;
   lCondition   *where;

   DENTER(TOP_LAYER,"sec_insert_conn2list");

   /* 
   ** create List if it not exists               
   */   
   if (!gsd.conn_list) 
      gsd.conn_list = lCreateList("conn_list",SecurityT);
   if (!gsd.conn_list) {
      ERROR((SGE_EVENT,"error: failed create Conn_List\n"));
      ret = -1;
      goto error;
   }

   /* 
   ** delete element if some with <host,commproc,id> exists   
   */
   where = lWhere( "%T(%I==%s && %I==%s && %I==%d)",SecurityT,
                    SEC_Host,host,SEC_Commproc,commproc,SEC_Id,id);
   if (!where) {
      ERROR((SGE_EVENT,"error: can't build condition\n"));
      ret = -1;
      goto error;
   }
   element = lFindFirst(gsd.conn_list,where);
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
      ERROR((SGE_EVENT,"error: failed create List_Element\n"));
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

   lSetString(element,SEC_ExpiryDate,time_str); 

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
**      #include "sec_other.h"
**
**   void sec_clearup_list(void)
**
** DESCRIPTION
**      This function clears the connection list from old connections.
**
*/
void sec_clearup_list(void)
{
   lListElem *element,*del_el;
   char *time_str=NULL;
   int i;

   /* 
   ** should the list be cleared from old connections ?
   */
   if (sec_time_cmp(gsd.refresh_time) < 0 ) {
      element = lFirst(gsd.conn_list);
      while (element) {
         time_str = lGetString(element, SEC_ExpiryDate);
         i = sec_time_cmp(time_str);
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
**      sec_time_cmp
**
** SYNOPSIS
**      #include "sec_other.h"
**
**      int sec_time_cmp(str)
**   char *str - this string contains a time
**
** DESCRIPTION
**      This function compares the actual time with the time in 'str'.
**
** RETURN VALUES
**      0       on failure
**      -1      actual time > str
**   1   actual time < str
*/

int sec_time_cmp(char *str)
{
   time_t offset;
   char buff1[100],buff2[100],*p;
   int i,j;

   p=buff1;
   i=strlen(str);
   if ((i < 11) || (i > 15)) return(0);
   memcpy(p,str,10);
   p+=10;
   str+=10;

   if ((*str == 'Z') || (*str == '-') || (*str == '+')){
      *(p++)='0'; *(p++)='0'; 
   }
   else{ 
      *(p++)= *(str++); *(p++)= *(str++); 
   }
   *(p++)='Z';
   *(p++)='\0';

   if (*str == 'Z') {
      offset=0;
   }
   else {
      if ((*str != '+') && (str[5] != '-'))
         return(0);
      offset=((str[1]-'0')*10+(str[2]-'0'))*60;
      offset+=(str[3]-'0')*10+(str[4]-'0');
      if (*str == '-')
         offset-=offset;
   }
   X509_gmtime_adj(buff2,offset);

   i=(buff1[0]-'0')*10+(buff1[1]-'0');
   if (i < 70) 
      i+=100;
   j=(buff2[0]-'0')*10+(buff2[1]-'0');
   if (j < 70) 
      j+=100;

   if (i < j) 
      return (-1);
   if (i > j) 
      return (1);
   i=strcmp(buff1,buff2);
   if (i == 0) /* wait a second then return younger :-) */
      return(-1);
   else
      return(i);
}

/*
** NAME
**      sec_keymat2list
**
** SYNOPSIS
**      #include "sec_other.h"
**   
**   void sec_keymat2list(element)
**   lListElem *element - the elment where to put the key material
**
** DESCRIPTION
**   This function converts the key material to unsigned long to store
**   it within the connection list, a cull list.
*/

void sec_keymat2list(lListElem *element)
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
**      sec_list2keymat
**
** SYNOPSIS
**      #include "sec_other.h"
**
**      void sec_list2keymat(element)
**      lListElem *element - the elment where to get the key material
**
** DESCRIPTION
**      This function converts the igned longs from the connection list
**   to key material.
*/

void sec_list2keymat(lListElem *element)
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

/*
** NAME
**      sec_print_bytes
**
** SYNOPSIS
**      #include "sec_other.h"
**
**   void sec_print_bytes(f, n, b)
**   FILE *f - output device
**   int n   - number of bytes to print
**   char *b - pointer to bytes to print
**
** DESCRIPTION
**      This function prints bytes in hex code.
*/

void sec_print_bytes(FILE *f, int n, char *b)
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
**      sec_set_verify_locations
**
** SYNOPSIS
**      #include "sec_other.h"
**
**      int sec_set_verify_locations(file_env)
**      char *filenv - filename of the CA
**
** DESCRIPTION
**      This function sets the location of the CA.
**
** RETURN VALUES
**   0   on success
**      -1      on failure
*/
   
int sec_set_verify_locations(
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
**      sec_verify_callback
**
** SYNOPSIS
**      #include "sec_other.h"
**
**      int sec_verify_callback(ok, xs, xi, depth, error)
**
** DESCRIPTION
**      This function is the callbackfunction to verify a certificate.
**
** RETURN VALUES
**      <>0     on success
**      0       on failure
*/

#if 0
int sec_verify_callback(int ok, X509 *xs, X509 *xi, int depth, int error)
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

/*
** NAME
**      sec_des_cbc_init
**
** SYNOPSIS
**      #include "sec_other.h"
**
**      void sec_des_cbc_init(key_material)
**   u_char *key_material - the key material to initiate
**
** DESCRIPTION
**      This function initiates the keymaterial and the IV for the 
**   DES-CBC-Mode.
*/
void sec_des_cbc_init(u_char *key_material)
{
   DES_CBC_STATE *ds;

   ds=(DES_CBC_STATE *) gsd.keys;
   des_set_odd_parity((C_Block *)&(key_material[0]));
   des_set_key((C_Block *)&(key_material[0]),ds->k1);
   memcpy(ds->iv,&(key_material[8]),DES_KEY_SZ);
}

/*
** NAME
**      sec_des_cbc
**
** SYNOPSIS
**      #include "sec_other.h"
**
**      u_long32 sec_des_cbc(len,from,to,encrypt)
**   u_long32  len     - number of bytes to de/encrypt
**   u_char  *from   - source of bytes
**   u_char  *to     - destination of bytes
**   int     encrypt - value is 0 for decryption 
**
** DESCRIPTION
**      This function de/encrypts 'len' bytes from 'from' to 'to'
**   with the DES-CBC-Mode.
**
** RETURN VALUES
**      u_long32   number of de/encrypted bytes
*/
u_long32 sec_des_cbc(u_long32 len, u_char *from, u_char *to, int encrypt)
{
   DES_CBC_STATE      *ds;
   u_long32             l;

   ds = (DES_CBC_STATE *) gsd.keys;
   l = len % 8;
   if (l==0)
      l=8;
   len+=(8-l);
   des_ncbc_encrypt(from, to, (long)len, ds->k1, (des_cblock *)ds->iv, encrypt);
   memcpy(ds->iv,&(gsd.key_mat[8]),DES_KEY_SZ);

   return len;
}

/*
** NAME
**      sec_des_ede3_cbc_init
**
** SYNOPSIS
**      #include "sec_other.h"
**
**      void sec_des_ede3_cbc_init(key_material)
**      u_char *key_material - the key material to initiate
**
** DESCRIPTION
**      This function initiates the keymaterial and the IV for the 
**      DES-EDE-CBC-Mode.
*/

void sec_des_ede3_cbc_init(u_char *key_material)
{
   DES_EDE3_CBC_STATE *ds;

   ds = (DES_EDE3_CBC_STATE *)gsd.keys;
   des_set_odd_parity((C_Block *)&(key_material[0]));
   des_set_odd_parity((C_Block *)&(key_material[8]));
   des_set_odd_parity((C_Block *)&(key_material[16]));
   des_set_key((C_Block *)&(key_material[0]),ds->k1);
   des_set_key((C_Block *)&(key_material[8]),ds->k2);
   des_set_key((C_Block *)&(key_material[16]),ds->k3);
   memcpy(ds->iv, &(key_material[24]), DES_KEY_SZ);
}

/*
** NAME
**      sec_des_ede3_cbc
**
** SYNOPSIS
**      #include "sec_other.h"
**
**      u_long32 sec_des_ede3_cbc(len,from,to,encrypt)
**      u_long32  len     - number of bytes to de/encrypt
**      u_char  *from   - source of bytes
**      u_char  *to     - destination of bytes
**      int     encrypt - value is 0 for decryption
**
** DESCRIPTION
**      This function de/encrypts 'len' bytes from 'from' to 'to'
**      with the DES-EDE-CBC-Mode.
**
** RETURN VALUES
**      u_long32  number of de/encrypted bytes
*/

u_long32 sec_des_ede3_cbc(u_long32 len, u_char *from, u_char *to, int encrypt)
{
   DES_EDE3_CBC_STATE *ds;
   u_long32 l;

   ds = (DES_EDE3_CBC_STATE *) gsd.keys;
   l = len % 8;
   if (l==0)
      l=8;
   len += (8-l);
   des_ede3_cbc_encrypt(from, to, (long)len, ds->k1, ds->k2, ds->k3,
                        (des_cblock *)ds->iv, encrypt);
   memcpy(ds->iv, &(gsd.key_mat[24]), DES_KEY_SZ);

   return len;
}

/*
** NAME
**      sec_md5_mac
**
** SYNOPSIS
**      #include "sec_other.h"
**
**      void sec_md5_mac(len,data,md)
**   u_long len - number of bytes 
**   u_char *data - bytes for MAC
**   u_char *md - destination for MAC
**
** DESCRIPTION
**      This function computes the MAC for a data.
*/
void sec_md5_mac(u_long32 len, unsigned char *data, unsigned char *md)
{
   MD5_CTX ctx;

   MD5_Init(&ctx);
   MD5_Update(&ctx,data,len);
   MD5_Final(md,&ctx);
}
