#ifndef __COMMD_IO_H
#define __COMMD_IO_H
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


unsigned char *pack_ushort(u_short us, unsigned char *cp);
int pack_ushort_len(u_short us);
unsigned char *unpack_ushort(u_short *us, unsigned char *cp);
unsigned char *pack_ulong(u_long32 ul, unsigned char *cp);
int pack_ulong_len(u_long ul);
unsigned char *unpack_ulong(u_long32 *ul, unsigned char *cp);
unsigned char *pack_string(char *str, unsigned char *cp);
int pack_string_len(char *str);
unsigned char *unpack_string(char *str, int len, unsigned char *cp);
int packed_strlen(char *cp);
int writenbytes(int sfd, char *ptr, int n);
int readnbytes(int sfd, char *ptr, int n);
int readnbytes_nb(int sfd, char *ptr, int n, int timeout);
int writenbytes_nb(int sfd, char *ptr, int n, int timeout);

#endif /* __COMMD_IO_H */
