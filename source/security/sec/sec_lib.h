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

typedef int (*sec_exit_func_type)(void);
extern sec_exit_func_type sec_exit_func;

int sec_init(const char *progname);

int sec_verify_user(const char *user, const char *commproc);

int sec_clear_connectionlist(void);

int sec_send_message(
   int synchron, 
   const char *tocomproc, 
   int toid, 
   const char *tohost, 
   int tag, 
   char *buffer, 
   int buflen, 
   u_long32 *mid, 
   int compressed);

int sec_receive_message(
   char *fromcommproc, 
   u_short *fromid, 
   char *fromhost, 
   int *tag, 
   char **buffer, 
   u_long32 *buflen, 
   int synchron, 
   u_short *compressed);

#endif /* __SEC_LIB_H */
