#ifndef __MESSAGE_H
#define __MESSAGE_H
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
#include <sys/types.h>

#include "basis_types.h"
#include "commproc.h"
#include "commd_message_flags.h"

#define KEYLEN 16

#define PROLOGLEN 14            /* size of message prolog */
#define HEADERLEN 2048          /* maximal size of header */

/* message states */

#ifdef MESSAGESTATES
char *message_states[] = { 
   "S_CREATED", 
   "S_RECEIVE_PROLOG",
   "S_ALLOC",
   "S_RECEIVE",
   "S_PROCESS", 
   "S_RDY_4_SND",
   "S_CONNECTING",
   "S_CONNECTED",
   "S_ALLWRITTEN",
   "S_CLOSED",
   "S_ACK",
   "S_REACK",
   "S_ACK_THEN_PROLOG",
   "S_WRITE_PROLOG",
   "S_WRITE",
   "S_WRITE_ACK",
   "S_WRITE_ACKE",
   "S_WRITE_ACK_SND",
   "S_SENDER_TIMEOUT",
};
#endif

#define S_RECEIVE_PROLOG  1
#define S_ALLOC           2
#define S_RECEIVE         3
#define S_PROCESS         4
#define S_RDY_4_SND       5
#define S_CONNECTING      6
#define S_CONNECTED       7
#define S_NOTUSED1        8     /* was S_ALLWRITTEN */
#define S_NOTUSED2        9     /* was S_CLOSED */
#define S_ACK             10
#define S_NOTUSED3        11    /* was S_REACK */
#define S_ACK_THEN_PROLOG 12
#define S_WRITE_PROLOG    13
#define S_WRITE           14
#define S_WRITE_ACK       15
#define S_WRITE_ACKE      16
#define S_WRITE_ACK_SND   17
#define S_SENDER_TIMEOUT  18
#define MAX_MESSAGE_STATE 18

typedef struct message {
  commproc from;		/* commproc this message came from */
  u_long32 mid;			/* message id for control of delivery */
  commproc to;			/* commproc this message should reach */
  int tag;			/* message tag (type) */
  u_short compressed; /* compression flag */
  u_long32 flags;			/* kind and status of message */
  unsigned char *bufstart;      /* start of malloced buffer area */
  unsigned char *bufsnd;        /* pointer into buffer whats to send next */
  unsigned char *bufdata;       /* points to first databyte after header */
  unsigned char *bufprogress;	/* next to read or write */ 
  u_long32 buflen;		/* malloced size */
  u_short headerlen;		/* length of header */
  unsigned char prolog[8];      /* buffer for reading/writing the prolog */
  unsigned char senderkey[KEYLEN];	/* unused at the moment */
  char ackchar;			/* char for ACK/NACK to sender */
  int fromfd;			/* sfd of incoming message */
  int reserved_port;            /* true if message came from a reserved port */
  struct in_addr fromaddr;	/* address the message actualy came from
				   (not neccesary the original sender) */
  int tofd;			/* sfd for sending the message */
  u_long first_try;		/* first time of delivery; needed for error
				   handling */
  u_long32 creation_date;         /* Date the message arrived at the commd.
                                   Used for message timeouts. */
  int new_connection;           /* This is true if the message is the first
                                   one sent via this connection */
  commproc *sending_to;         /* At the moment we assign the tofd fd of a 
                                   message to the fd we found in the w_fd field
                                   of the receiving commproc, we set this 
                                   pointer to the commproc.   
                                   This allows in case the communication fails 
                                   to reset the w_fd field of the commproc and 
                                   let the receiver commproc to receive 
                                   again. */
  struct message *next;
} message;


#if 1
#define HEADERSTART(msg)    &(msg->bufstart[HEADERLEN - msg->headerlen])
#else
/* avoid Compiler bug in HP Compilers pointer arithmetic                   */
#define HEADERSTART(msg)   (unsigned char *) (msg->bufdata - (unsigned char *) (int) msg->headerlen)
#endif

#define MESSAGE_STATUS(message) ((message->flags & 0xff000000) >> 24)
#define SET_MESSAGE_STATUS(message, status) \
  message->flags = (message->flags & 0x00ffffff) | (status << 24)

message *create_message(void);
void delete_message(message *to_delete, char *reason);
void delete_message_(message *to_delete, int keep_fromfd, char *reason);
message *search_message(int fd, int makenew);
int allocate_message(message *mp);
message *match_message(commproc *to, commproc *from, int tag, int status);
int exist_message(message *m);

/**********************************************************************/
int enable_message_logging(char *fname);
void disable_message_logging(void);
void log_message(message *mp);
char *message_status_string(message *mp);
void print_message(message *mp, FILE *fp);
void print_messages(FILE *fp);
void look4timeouts_messages(u_long now);
unsigned long count_messages(void);                /* used for profiling */
/* char* count_different_file_descriptors(void); */ /* used for debugging */

#endif /* __MESSAGE_H */
