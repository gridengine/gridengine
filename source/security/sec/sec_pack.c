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
#include "basis_types.h"
#include "cull_pack.h"
#include "sec_pack.h"
#include "sec_local.h"

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
int sec_pack_announce(int certlen, u_char *x509_buf, u_char *challenge, sge_pack_buffer *pb)
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

int sec_unpack_announce(int *certlen, u_char **x509_buf, u_char **challenge, sge_pack_buffer *pb)
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

int sec_pack_response(int certlen, u_char *x509_buf, int chall_enc_len, u_char *enc_challenge, 
                      u_long32 connid, int key_enc_len, u_char *enc_key_mat, sge_pack_buffer *pb)
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
   if((i=packint(pb,(u_long32) key_enc_len))) 
      goto error;
   if((i=packbuf(pb,(char *) enc_key_mat,(u_long32) key_enc_len))) 
      goto error;

   error:
      return(i);
}

int sec_unpack_response(int *certlen, u_char **x509_buf, int *chall_enc_len, u_char **enc_challenge, u_long32 *connid, int *key_enc_len, u_char **enc_key_mat, sge_pack_buffer *pb)
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
   if((i=unpackint(pb,(u_long32 *) key_enc_len))) 
      goto error;
   if((i=unpackbuf(pb,(char **) enc_key_mat,(u_long32) *key_enc_len)))
      goto error;

   error:
      return(i);
}

int sec_pack_message(u_long32 connid, u_long32 mac_enc_len, u_char *mac, u_long32 mssg_enc_len, sge_pack_buffer *pb)
{
   int i;

   if((i=packint(pb,(u_long32) connid))) 
      goto error;
   if((i=packint(pb,(u_long32) mac_enc_len))) 
      goto error;
   if((i=packbuf(pb,(char *) mac, (u_long32) mac_enc_len))) 
      goto error;
   if((i=packint(pb,(u_long32) mssg_enc_len))) 
      goto error;

   error:
      return(i);
}

int sec_unpack_message(u_long32 *connid, u_long32 *mac_enc_len, u_char **mac, u_long32 *mssg_enc_len, sge_pack_buffer *pb)
{
   int i;

   if((i=unpackint(pb,(u_long32 *) connid))) 
      goto error;
   if((i=unpackint(pb,(u_long32 *) mac_enc_len))) 
      goto error;
   if((i=unpackbuf(pb,(char**) mac, (u_long32) *mac_enc_len))) 
      goto error;
   if((i=unpackint(pb,(u_long32 *) mssg_enc_len))) 
      goto error;

   error:
      return(i);
}

int sec_pack_reconnect(u_long32 connid, u_long32 seq_send, u_long32 seq_receive, u_char *key_mat, sge_pack_buffer *pb)
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

int sec_unpack_reconnect(u_long32 *connid, u_long32 *seq_send, u_long32 *seq_receive, 
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

