#ifndef __SEC_PACK_H
#define __SEC_PACK_H
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

int sec_pack_announce(int len, u_char *buf, u_char *chall, sge_pack_buffer *pb);
int sec_unpack_announce(int *len, u_char **buf, u_char **chall, sge_pack_buffer *pb);
int sec_pack_response(int len, u_char *buf, int c_enc_len, u_char *enc_chall, u_long32 connid, int k_enc_len, u_char *enc_key_mat, sge_pack_buffer *pb);
int sec_unpack_response(int *len, u_char **buf, int *c_enc_len, u_char **enc_chall, u_long32 *connid, int *k_enc_len, u_char **enc_key_mat, sge_pack_buffer *pb);
int sec_pack_message(u_long32 connid, u_long32 mac_enc_len, u_char *mac, u_long32 mssg_enc_len, sge_pack_buffer *pb);
int sec_unpack_message(u_long32 *connid, u_long32 *mac_enc_len, u_char **mac, u_long32 *mssg_enc_len, sge_pack_buffer *pb);
int sec_pack_reconnect(u_long32 connid, u_long32 seq_send, u_long32 seq_receive, u_char *key_mat, sge_pack_buffer *pb);
int sec_unpack_reconnect(u_long32 *connid, u_long32 *seq_send, u_long32 *seq_receive, u_char **key_mat, sge_pack_buffer *pb);

#endif /* __SEC_PACK_H */
