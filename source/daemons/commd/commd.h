#ifndef __COMMD_H
#define __COMMD_H
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

#define MAXHOSTLEN 256
#define MAXCOMPONENTLEN 256

#define MAXID 0xffff
#define MINID 1
#define STRLEN_ID 4


/* These variables are used for profiling */
extern unsigned long unique_hosts_messages;
extern int enable_commd_profile;
extern unsigned long new_messages;
extern unsigned long del_messages;
extern int enable_commd_profile;
extern unsigned long data_message_byte_count; 
extern unsigned long data_message_count;
extern unsigned long sockets_created;
extern unsigned long connection_errors;
extern unsigned long connection_closed;
extern unsigned long connection_accept;
extern unsigned long bytes_sent;

/* These functions are used for profiling */
void enable_commd_profiling(int flag);
void reset_profiling_data(void);




/* operations for cntl */
#define O_KILL		1
#define O_TRACE		2
#define O_DUMP		3
#define O_GETID		4
#define O_UNREGISTER	5
#define O_PROFILE_ON 6         /* profiling */
#define O_PROFILE_OFF 7        /* profiling */
#define O_PROFILE_RESET 8      /* profiling */

#define MESSAGE_MAXDELIVERTIME (5*60)
#define MAXNISRETRY 10
#define HOSTREFRESHTIME 1800

#ifdef  __cplusplus
}
#endif
#endif /* __COMMD_H */
