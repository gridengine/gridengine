#ifndef __RMON_REQUEST_H
#define __RMON_REQUEST_H
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



#include "rmon_def.h"
#include "rmon_rmon.h"
#include "rmon_monitoring_level.h"

#define MONITORING_LEVEL    0
#define MESSAGE_FLUSH       1
#define CLIENT_REGISTER     2
#define SPY_REGISTER        3
#define MESSAGE_OVERFLOW    4
#define MSTAT               5
#define MDEL                6
#define MQUIT               7
#define MJOB                8
#define SPY_EXIT                        9
#define WAKE_UP             10
#define SLEEP                           11
#define MCONF                           12

#define S_ACCEPTED          1
#define S_NOT_ACCEPTED      2
#define S_STILL_WAITING     3
#define S_UNKNOWN_SPY       4
#define S_ILLEGAL_LEVEL     5
#define S_UNKNOWN_CLIENT    6
#define S_NO_TRANSITION     7
#define S_NEW_WAIT          8
#define S_ALTER_WAIT        9
#define S_DELETE_WAIT       10
#define S_NEW_TRANSITION    11
#define S_ALTER_TRANSITION  12
#define S_DELETE_TRANSITION 13
#define S_SPY_EXISTS        14
#define S_SPY_DOESNT_EXIST      15
#define S_NOT_SLEEPING          16
#define S_NOT_AWAKE                     17

#define SPY                                     0
#define CLIENT                          1

typedef struct request_type {
   u_long type;                 /* request code                                         */
   u_long status;               /* reply status (used by client)        */
   u_long uid;                  /* user id                                                      */

   u_long client_number;
   u_long inet_addr;
   u_long port;
   monitoring_level level;
   u_long childdeath;
   u_long wait;
   u_long kind;
   u_long n_client;             /* number of list elements in 1st list          */
   u_long n_spy;                /* number of list elements in 2nd list          */
   u_long n_transition;         /* number of list elements in 3rd list          */
   u_long n_wait;               /* number of list elements in 4th list          */
   u_long n_job;                /* number of list elements in 4th list          */
   u_long n_add;
   u_long n_delete;
   u_long conf_type;
   u_long conf_value;

   string programname;
} request_type;

extern request_type request;
extern u_long timeout_value;

int rmon_send_request(int sfd);
int rmon_get_request(int sfd);

int rmon_send_ack(int sfd, u_long status);
int rmon_get_ack(int sfd);

#endif /* __RMON_REQUEST_H */



