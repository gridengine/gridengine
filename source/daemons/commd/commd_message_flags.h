#ifndef __COMMD_MESSAGE_FLAGS_H
#define __COMMD_MESSAGE_FLAGS_H
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

#ifdef  __cplusplus
extern "C" {
#endif

/* message types */
#define COMMD_SYNCHRON         1
#define COMMD_POOL             2
#define COMMD_ENROLL           4
/* sender had reserved port (filled by commd) */
#define COMMD_RESVPORT         8
/* the transmitter is a commd */
#define COMMD_SCOMMD          16
#define COMMD_RECEIVE         32
#define COMMD_NOT_USED        64      /* was NOTIFICATION */
#define COMMD_NOT_USED1      128      /* was LASTACKMID */
#define COMMD_LEAVE          256
#define COMMD_CNTL           512
#define COMMD_ASK_COMMPROC  1024
#define COMMD_ASK_COMMPROCR 2048
#define COMMD_UNIQUEHOST    4096

/* ACK and NACK characters */
/* positive */
#define COMMD_CACK  0

/* negative corresponding to CL_... codes */
#define CL_PERM                     255
#define COMMD_NACK_PERM             255
#define COMMD_NACK_UNKNOWN_TARGET   254
#define CL_UNKNOWN_TARGET           COMMD_NACK_UNKNOWN_TARGET
#define COMMD_NACK_UNKNOWN_RECEIVER 253
#define CL_UNKNOWN_RECEIVER         COMMD_NACK_UNKNOWN_RECEIVER
#define COMMD_NACK_COMMD_NOT_READY  252
#define COMMD_NACK_UNKNOWN_HOST     251
#define COMMD_NACK_NO_MESSAGE       250
#define COMMD_NACK_ENROLL           249
#define COMMD_NACK_OPONCE           248
#define COMMD_NACK_DELIVERY         247
#define COMMD_NACK_TIMEOUT          246
#define COMMD_NACK_CONFLICT         245

#ifdef  __cplusplus
}
#endif
 
#endif /* __COMMD_MESSAGE_FLAGS_H */
