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
#define SYNCHRON         1
#define POOL             2
#define ENROLL           4
/* sender had reserved port (filled by commd) */
#define RESVPORT         8
/* the transmitter is a commd */
#define SCOMMD          16
#define RECEIVE         32
#define NOT_USED        64      /* was NOTIFICATION */
#define NOT_USED1      128      /* was LASTACKMID */
#define LEAVE          256
#define CNTL           512
#define ASK_COMMPROC  1024
#define ASK_COMMPROCR 2048
#define UNIQUEHOST    4096

/* ACK and NACK characters */
/* positive */
#define CACK  0

/* negative corresponding to CL_... codes */
#define CL_PERM               255
#define NACK_PERM             255
#define NACK_UNKNOWN_TARGET   254
#define CL_UNKNOWN_TARGET     NACK_UNKNOWN_TARGET
#define NACK_UNKNOWN_RECEIVER 253
#define CL_UNKNOWN_RECEIVER   NACK_UNKNOWN_RECEIVER
#define NACK_COMMD_NOT_READY  252
#define NACK_UNKNOWN_HOST     251
#define NACK_NO_MESSAGE       250
#define NACK_ENROLL           249
#define NACK_OPONCE           248
#define NACK_DELIVERY         247
#define NACK_TIMEOUT          246
#define NACK_CONFLICT         245

#ifdef  __cplusplus
}
#endif
 
#endif /* __COMMD_MESSAGE_FLAGS_H */
