#ifndef __SEC_LIB_H 
#define __SEC_LIB_H
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

int sec_set_encrypt(int tag);
int sec_init(char *progname);
int sec_files(void);
int sec_dump_connlist(void);
int sec_undump_connlist(void);
int sec_announce_connection(char *commproc, char *cell);
int sec_respond_announce(char *commproc, u_short id, char *host, char *buffer, u_long32 buflen);
int sec_handle_announce(char *comproc, u_short id, char *host, char *buffer, u_long32 buflen);
int sec_send_message(int synchron, char *tocomproc, int toid, char *tohost, int tag, char *buffer, u_long32 buflen, u_long32 *mid, int compressed);
int sec_encrypt(char **buffer, int *buflen);
int sec_receive_message(char *fromcommproc, u_short *fromid, char *fromhost, int *tag, char **buffer, u_long32 *buflen, int synchron, u_short *compressed);
int sec_decrypt(char **buffer, u_long32 *buflen, char *host, char *commproc, int id);
int sec_reconnect(void);
int sec_write_data2hd(void);

#endif /* __SEC_LIB_H */
