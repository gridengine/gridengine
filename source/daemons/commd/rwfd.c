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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>

#include "message.h"
#include "commproc.h"
#include "commd_error.h"
#include "commd_io.h"
#include "rwfd.h"
#include "debug_malloc.h"
#include "sge_time.h"
#include "sgermon.h"
#include "sge_log.h"
#include "msg_commd.h"

/* from reset_messages_for_commproc.c */
void reset_messages_for_commproc(commproc *commp);

/* from process_received_message.c */
extern int process_received_message(message *mp, int port_security, int commdport);


/* from prolog.c */
int read_message_prolog(message *mp);
int write_message_prolog(message *mp);

/* from deliver_message.c */
extern void deliver_message(message *mp, int local);

/* from ack.c */
int read_ack(message *mp);
int write_ack(message *mp, int tofd);

/* from commd.c */
extern void lack_of_memory(void);
extern u_long too_many_fds_open;        /* time at which too many fds were open */
void log_state_transition_too_many_fds_open(u_long t);

/* from commproc.c */
extern commproc* commprocs;

extern void trace(char *);

/* system externals */
extern int rresvport(int *port);


/* local defined */
void init_send(message *mp, int reserved_port, int commdport);

static void open_connection(message *mp, int reserved_port, int commdport);
static int write_message(message *mp);
static int read_message(message *mp);
void reset_message(message *mp, const char *str);

/**********************************************************************
   - a tofd gets active for writing; transfer as much as possible 
 **********************************************************************/
void write2fd(
message *mp,
int reserved_port,
int commdport 
) {
   int i;
   char err_str[256];
   char dummy;

   DENTER(TOP_LAYER, "write2fd");
   
   if (mp)
      DEBUG((SGE_EVENT, "write2fd: message status="u32" %s", MESSAGE_STATUS(mp), message_status_string(mp)));

   if (MESSAGE_STATUS(mp) == S_SENDER_TIMEOUT) {
      /* The sender of a message timed out waiting for an acknowledge. 
         Send an nack_timeout to the receiver, then delete the message */
      if ((i = write_ack(mp, 1) == 1)) { /* try later */
         DEXIT;
         return;
      }   

      if (i != 0)
         DEBUG((SGE_EVENT, "failed to snd nack_timeout-> deleting message"));

      delete_message(mp, "failed sending nack_timeout");
      DEXIT;
      return;
   }

   if (MESSAGE_STATUS(mp) == S_WRITE_ACK) {
      if ((i = write_ack(mp, 0)) == 1)  {/* try later */
         DEXIT;
         return;
      }   

      if (i != 0) {
         DEBUG((SGE_EVENT, "failed to snd ack-> deleting message"));
      }
      delete_message(mp, i?"failed sending ack":NULL);
      DEXIT;
      return;
   }

   if (MESSAGE_STATUS(mp) == S_WRITE_ACKE) {
      if (!write_message(mp)) { /* all done remove message */
         delete_message(mp, NULL);
      }
      DEXIT;
      return;
   }

   if (MESSAGE_STATUS(mp) == S_WRITE_ACK_SND) {
      /* write acknowledge to sender, then send the message */
      if ((i = write_ack(mp, 0))) {
         if (i == -1)
            delete_message(mp, "failed sending ack on send");
         DEXIT;   
         return;
      }
      SET_MESSAGE_STATUS(mp, S_RDY_4_SND);

      /* sender is no longer online */
      fd_close(mp->fromfd, "sender no longer online");

      mp->fromfd = -1;

      init_send(mp, reserved_port, commdport);
      DEXIT;
      return;
   }

   
   if (MESSAGE_STATUS(mp) == S_CONNECTING) {
      int sso;
      /* 
       * a connect was done and returned with EINPROGRESS. 
       * a later select() on this fd returned it as active
       * EAGAIN and EWOULDBLOCK is sometime returned in connections to 
       * another host. The read is a dummy read to verify the select().
       * It returns always -1 since the other side never writes anything
       */
      errno = 0; 
      i = read(mp->tofd, &dummy, 1);
      if (i == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
         if(mp->flags & COMMD_SYNCHRON) {
            /* don't wait so long for a timeout */
            mp->ackchar = COMMD_NACK_COMMD_NOT_READY;
            SET_MESSAGE_STATUS(mp, S_WRITE_ACK);
         }
         else {
            sprintf(err_str, MSG_RWFD_CONNECTINGFAILED_SSISSIS,
                  mp->from.host->he.h_name, 
                  mp->from.name, 
                  (int) mp->from.id,
                  mp->to.host->he.h_name, 
                  mp->to.name, 
                  (int) mp->to.id,
                  strerror(errno));
            reset_message(mp, err_str);
         }
         DEXIT;
         return;
      }
      sso = 1;
#if defined(SOLARIS) && !defined(SOLARIS64)      
      setsockopt(mp->tofd, IPPROTO_TCP, TCP_NODELAY, (const char *) &sso, sizeof(int));
#else
      setsockopt(mp->tofd, IPPROTO_TCP, TCP_NODELAY, &sso, sizeof(int));
#endif           
      deliver_message(mp, 0);
      DEXIT;
      return;
   }

   if (MESSAGE_STATUS(mp) == S_CONNECTED) {
      /* connection is done we can start delivering message */
      deliver_message(mp, 0);
   }

   if (MESSAGE_STATUS(mp) == S_ACK_THEN_PROLOG) {
      /* first send the receiver an acknowledge, then follows the message 
         prolog */
      i = write_ack(mp, 1);
      if (i == -1) {            /* problems in transmission -> reset message */
         reset_message(mp, "S_ACK_THEN_PROLOG");
         DEXIT;
         return;
      }
      if (i == 1) {               /* receiver not ready -> try later */
         DEXIT;
         return;
      }   

      SET_MESSAGE_STATUS(mp, S_WRITE_PROLOG);
   }

   if (MESSAGE_STATUS(mp) == S_WRITE_PROLOG) {
      if (write_message_prolog(mp)) {
         DEXIT;
         return;
      }   
      SET_MESSAGE_STATUS(mp, S_WRITE);
   }

   if (MESSAGE_STATUS(mp) == S_WRITE) {
      if (write_message(mp)) {
         DEXIT;
         return;
      }   
      SET_MESSAGE_STATUS(mp, S_ACK);    /* wait for an acknowledge */
   }

   DEXIT;
   return;
}

/**********************************************************************
 a fd gets active for reading; transfer as much as possible 
 **********************************************************************/
void readfromfd(
int fd,
message *mp,
int port_security,
int commdport 
) {
   int i;
   char chr;
   commproc *commp;
   char *mls_str, *s;
   u_long mls;
   int did_read = 0;
   
   DENTER(TOP_LAYER, "readfromfd");

   DEBUG((SGE_EVENT, "readfromfd(%d, %p, %d, %d)", fd, mp, port_security,
         commdport));
   if (mp)
      DEBUG((SGE_EVENT, "message status="u32, MESSAGE_STATUS(mp)));

   /* When everything goes fine we ran through this function in one sweep.
      Reasons to leave this routine:
      - lack of memory -> reenter this function later on and try it again
      - broken connection -> delete message do not reenter function
      - sender isnt ready -> reenter function when sender is ready

      The message status indicates what is to do next (where to proceed this
      function.  */
   if (!mp) {
      /* Fd was selected cause of an waiting client. This can happen due to
         the following reasons: 
         - The commproc terminated. Here we get an EOF from the socket-pipe. 
         - The commproc decides to no longer wait for a message (Timeout).

         The commproc sends us a RCV_TIMEOUT message

         This branch is only entered for clients that close their fd
         after each message. Other commprocs handle RCV_TIMEOUT via
         a normal message
       */
      if (!(commp = search_commproc_waiting_on_fd(fd))) {
         ERROR((SGE_EVENT, MSG_RWFD_NOCOMMPROCWAITINGONFD_I , fd));
         DEXIT;
         return;
      }

      if (read(fd, &chr, 1) == 1) {
         /* commproc timed out and therefore does no longer wait for a
            messsage */
         DEBUG((SGE_EVENT, "commproc %s timed out in waiting for a message",
               commp->name));
         disconnect_commproc_using_fd(fd);
         fd_close(commp->w_fd, "commproc timed out");
         commp->w_fd = -1;
      }
      else {
         /* EOF-> broken commproc */
         DEBUG((SGE_EVENT, "delete commproc \"%s\" cause of broken connection",
               commp->name));
         fd_close(commp->w_fd, "broken commproc");
         commp->w_fd = -1;
         reset_messages_for_commproc(commp);
         delete_commproc(commp);
      }
      DEXIT;
      return;
   }

   if ((MESSAGE_STATUS(mp) == S_ACK || MESSAGE_STATUS(mp) == S_RDY_4_SND) &&
      mp->fromfd == fd) {
      i = read(mp->fromfd, &chr, 1);

      DEBUG((SGE_EVENT, "read returned: %d\n", i));

      if (i) {
         /* We got an NACK_TIMEOUT from the Sender of the message.
            Send the receiver an NACK_TIMEOUT */
         disconnect_commproc_using_fd(mp->fromfd);
         fd_close(mp->fromfd, "because of NACK_TIMEOUT");

         mp->fromfd = -1;
         mp->ackchar = COMMD_NACK_TIMEOUT;
         if (mp->tofd == -1) {  /* if there is no connection to the next hop
                                   we simply delete the message */
            delete_message(mp, "nack timeout from sender");
            DEXIT;
            return;
         }
         /* else we send the next hop (commd or receiver) an NACK_TIMEOUT */
         SET_MESSAGE_STATUS(mp, S_SENDER_TIMEOUT);
         DEXIT;
         return;
      }
      else {
         /* Sender is broken. Close connection to receiver of message.
            Then make an implicit leave() of sender */
         fd_close(mp->tofd, "tofd: sender is broken");

         mp->tofd = -1;
         fd_close(mp->fromfd, "fromfd: sender is broken");

         mp->fromfd = -1;
         commp = match_commproc(&mp->from);
         if (commp) {
            ERROR((SGE_EVENT, MSG_RWFD_DELETECOMMPROCBROKENCONNECTIONFILE_S , commp->name));
            reset_messages_for_commproc(commp);
            delete_commproc(commp);
         }
         delete_message(mp, "sender is broken");
         DEXIT;
         return;
      }
   }

   DEBUG((SGE_EVENT, "readfromfd: messagestatus="u32, MESSAGE_STATUS(mp)));

   /* read length of header and buffer */
   if (MESSAGE_STATUS(mp) == S_RECEIVE_PROLOG) {
      if ((i = read_message_prolog(mp))) {
         DEXIT;
         return;         /* try later */
      }
      
      SET_MESSAGE_STATUS(mp, S_ALLOC);
      log_message(mp);
      did_read = 1;
   }

   /* allocate header and body of message */
   if (MESSAGE_STATUS(mp) == S_ALLOC) {
      if (allocate_message(mp)) {       /* allocate space for header and body */
         lack_of_memory();
         DEXIT;
         return;                /* try later */
      }
      SET_MESSAGE_STATUS(mp, S_RECEIVE);
      log_message(mp);
   }

   /* read the header */
   if (MESSAGE_STATUS(mp) == S_RECEIVE) {
      if ((i = read_message(mp))) {
         DEXIT;
         return;       /* we cant continue */
      }   
      SET_MESSAGE_STATUS(mp, S_PROCESS);
      did_read = 1;
   }

   if (MESSAGE_STATUS(mp) == S_PROCESS) {
      /* message received; process it */
      if (process_received_message(mp, port_security, commdport)) {
         DEXIT;
         return;    /* may be mp is no longer valid */
      }
      
      /* log big messages with SGE_MSG_LOG_SIZE bytes */
      if ((mls_str = getenv("SGE_MSG_LOG_SIZE")) && (mls = atoi(mls_str))) {
         if (!mls)
            mls = 1024 * 1024;
         if (mp->buflen >= mls) {
            ERROR((SGE_EVENT, MSG_RWFD_BIGXSENDSMESSAGEYBYTESTOZ_USSIIUSSI ,
                    u32c(mls),
                    (s = mp->from.name) ? s : "*",
                    (s = get_mainname(mp->from.host)) ? s : "*",
                    (int) mp->from.id,
                    mp->tag,
                    u32c(mp->buflen),
                    (s = mp->to.name) ? s : "*",
                    (s = get_mainname(mp->to.host)) ? s : "*",
                    (int) mp->to.id));
         }
      }
   }

   /* at this point the receive part for this message is done.
    *  The write part has to take over control
    */

   if (MESSAGE_STATUS(mp) == S_ACK) {
      /* we have delivered the message now we wait for an acknowledge */
      i = read_ack(mp);
      if (i == 1) {               /* wait for select */
         DEXIT;
         return;
      }   
      if (i) {                  /* there is a problem reading ack */
         /* ++++ resend is the wrong behavior
          * we should know whether the communication partner got the message
          * (and may did what he should do before he died)
          * and consider him dead (unenroll him)
          */
         DEXIT;  
         return;
      }

      /* if this was a synchron message we send the waiting commd or commproc
       *  an ack to the fd he is waiting on. The acks build a chain from the
       *  receiver to the sender (generated by commlib:receive_message()) 
       */
      if (mp->flags & COMMD_SYNCHRON) {
         /* mp->fromfd holds the senders fd which isn't closed in case of a
            synchron message. mp->tofd can be closed cause everything is done
            for the receiver. */
         fd_close(mp->tofd, "after sending ack");

         mp->tofd = -1;
         mp->sending_to = NULL;
         SET_MESSAGE_STATUS(mp, S_WRITE_ACK);
         DEXIT;
         return;
      }
      delete_message(mp, NULL);       /* delivered and acknowledged */
      DEXIT;
      return;
   }

   if (!did_read) {
       /* the message was selected because select() returned read condition
          for the file descriptor but we didn't read ... */
      DEBUG((SGE_EVENT, "readfromfd did not read fd=%d message is in state %s",
         fd, message_status_string(mp)));
      trace(SGE_EVENT);
      if (MESSAGE_STATUS(mp) == S_CONNECTING) {
         i = read(fd, &chr, 1);
         if (!i) {
            disconnect_commproc_using_fd(fd);
            delete_message(mp, "RCV TIMEOUT while S_CONNECTING");
            DEXIT;
            return;
         }
      }
   }

   DEXIT;
   return;
}

/**********************************************************************
  read message header and body
  return 0  if everything is read
         -1 on error
         1  if sender isn't ready

         if an error occurs the message will be dropped
 **********************************************************************/
static int read_message(
message *mp 
) {
   u_long already_read, len;
   int i, errorcode;

   DENTER(TOP_LAYER, "read_message");
   
   len = mp->buflen + mp->headerlen;
   already_read = mp->bufprogress - HEADERSTART(mp);

   DEBUG((SGE_EVENT, "read message: len=%ld  already read=%ld", 
         len, already_read));

   while (already_read < len) {
      i = read(mp->fromfd, mp->bufprogress, len - already_read);
      errorcode = errno;        /* may be altered by LOG() */

      DEBUG((SGE_EVENT, "read returned %d [%d] %s", i, errorcode,
            i == -1 ? strerror(errorcode): ""));

      if (!i) {                 
         /* System V
          * read on an empty pipe should return with 0
          * therefore there is no destinction between empty pipe and EOF
          * -> alternatives for sysV?
          */ 
         delete_message(mp, "cannot read");
         DEXIT;
         return -1;
      }
      if (i == -1) {
         if (errorcode == EWOULDBLOCK || errorcode == EAGAIN) {
            DEXIT;
            return 1;           /* sender isn't ready -> try later */
         }   
         else {                 /* a real error -> cancel message */
            delete_message(mp, "read error");
            DEXIT;
            return -1;
         }
      }
      /* we got some data -> process it */
      mp->bufprogress += i;
      already_read += i;
   }

   log_message(mp);
   DEXIT;
   return 0;
}

/**********************************************************************
  write message header and body
  return 0  if everything is written
         -1 on error
         1  if receiver isn't ready

  if an error occurs we will try to deliver the message later on
  (set message status to S_RDY_4_SND)
 **********************************************************************/
static int write_message(
message *mp 
) {
   u_long already_written, len;
   int i, errorcode;
   char err_str[256];
   
   DENTER(TOP_LAYER, "write_message");

   len = mp->buflen + mp->headerlen;
   already_written = mp->bufprogress - HEADERSTART(mp);

   DEBUG((SGE_EVENT, "write message: len=%ld  already written=%ld", len,
         already_written));

   while(already_written < len) {
      i = write(mp->tofd, mp->bufprogress, len - already_written);
      errorcode = errno;           /* may be altered by LOG() */

      DEBUG((SGE_EVENT, "write returned %d %s", i,
            i == -1 ? strerror(errno) : ""));

      if (!i) {
         reset_message(mp, MSG_RESETMESSAGE_WRITE_MESSAGE_WRITE_0);
         DEXIT;
         return 1;
      }

      if (i == -1) {
         if (errorcode == EWOULDBLOCK || errorcode == EAGAIN) {
            DEXIT; 
            return 1;              /* sender isn't ready -> try later */
         }   
         else {                    /* a real error -> cancel message */
            sprintf(err_str, MSG_RESETMESSAGE_WRITE_MESSAGE_WRITE_NEG_1_S, strerror(errno));
            reset_message(mp, err_str);
            DEXIT;
            return -1;
         }
      }
      /* we got some data -> process it */
      mp->bufprogress += i;
      already_written += i;
   }

   if (already_written < len) {
      DEXIT;
      return 1;
   }   

   log_message(mp);
   DEXIT;
   return 0;
}

/**********************************************************************
 the listen socket needs some action; a connect comes in 
 returns pointer to new message or NULL if failure
 **********************************************************************/
message* mknewconnect(
int sockfd 
) {
   struct sockaddr_in cli_addr;
   int new_sfd, sso;
   message *mp;
#ifdef AIX43
   size_t fromlen = 0;
#else
   int fromlen = 0;
#endif

   DENTER(TOP_LAYER, "mknewconnect");
   
   DEBUG((SGE_EVENT, "making new connection"));

   mp = create_message();
   if (!mp) {                   /* wait for a better time                   */
      lack_of_memory();         /* we have to avoid busy looping without a  */
      DEXIT;
      return NULL;              /* chance to do s.th.                       */
   }

   fromlen = sizeof(cli_addr);
   memset((char *) &cli_addr, 0, sizeof(cli_addr));

   new_sfd = accept(sockfd, (struct sockaddr *) &cli_addr, &fromlen);
   
   if (new_sfd == -1) {
      if (errno == EMFILE) {    /* too many open files             */
         too_many_fds_open = sge_get_gmt();         /* overloaded by too many requests */
         log_state_transition_too_many_fds_open(too_many_fds_open);
      }

      ERROR((SGE_EVENT, MSG_NET_ACCEPTFAILED_S , strerror(errno)));

      delete_message(mp, "accept failed");
      DEXIT;
      return NULL;
   }

   DEBUG((SGE_EVENT, "accepted connection on fd=%d port=%d ipaddr=%s", new_sfd,
         ntohs(cli_addr.sin_port), inet_ntoa(cli_addr.sin_addr)));

   fcntl(new_sfd, F_SETFL, O_NONBLOCK);         /* HP needs O_NONBLOCK, was O_NDELAY */

   sso = 1;
#if defined(SOLARIS) && !defined(SOLARIS64)
   if (setsockopt(new_sfd, IPPROTO_TCP, TCP_NODELAY, (const char *) &sso, sizeof(int)) == -1)
#else
   if (setsockopt(new_sfd, IPPROTO_TCP, TCP_NODELAY, &sso, sizeof(int))== -1)
#endif
      DPRINTF(("cannot setsockopt() to TCP_NODELAY.\n"));

   /* we put a message into the message list to indicate the pending
    * connection
    */
   SET_MESSAGE_STATUS(mp, S_RECEIVE_PROLOG);
   mp->fromfd = new_sfd;
   mp->reserved_port = (ntohs(cli_addr.sin_port) < IPPORT_RESERVED);
   mp->new_connection = 1;              /* have to reconnect to the commproc later on
                                           if it is not an enroll request */

   mp->bufprogress = mp->prolog;        /* first we read the prolog */

   memcpy((char *) &mp->fromaddr, (char *) &cli_addr.sin_addr,
          sizeof(struct in_addr));

   log_message(mp);
   DEXIT;
   return mp;
}

/**********************************************************************
  A message has been received. Try to make a connection to the remote
  commd (clients always initiate connections, so we never connect to a client).
  This function does everything until we have a connection. The real writing 
  can be done by the write2fd function (select driven).
  If we cant open the connection we reset the message to the RDY_4_SND state.

  mp             message for which to open the connection
  reserved_port  use reserved port (other side may test it)
  commdport      port under which remote commd resides
 **********************************************************************/
static void open_connection(
message *mp,
int reserved_port,
int commdport 
) {
   int port = IPPORT_RESERVED - 1, i, errorcode;
   struct sockaddr_in addr;
   struct hostent *he;
   char err_str[256];
   u_long now;
   char *tohostname;
   int sso;
   
   DENTER(TOP_LAYER, "open_connection");

   if (mp->tofd == -1) {
      /* create socket */
      if (reserved_port)
         mp->tofd = rresvport(&port);
      else {
         mp->tofd = socket(AF_INET, SOCK_STREAM, 0);
      }

      if (mp->tofd < 0) {
         ERROR((SGE_EVENT,MSG_NET_OPENSTREAMSOCKETFAILED ));
         reset_message(mp, "open connection");
         DEXIT;
         return;
      }
   }

   fcntl(mp->tofd, F_SETFL, O_NONBLOCK);

   he = &mp->to.host->he;
   tohostname = get_mainname(mp->to.host);

   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   memcpy((char *) &addr.sin_addr, (char *) he->h_addr, he->h_length);
   addr.sin_port = htons(commdport);

   DEBUG((SGE_EVENT, "connecting to %s:%d with socket fd=%d",
         inet_ntoa(*((struct in_addr *) he->h_addr)), commdport, mp->tofd));

   i = connect(mp->tofd, (struct sockaddr *) &addr, sizeof(addr));
   errorcode = errno;

   DEBUG((SGE_EVENT, "connect returns %d %s", i, i == -1 ? strerror(errno) : ""));

   if (i == -1) {

      /* if errno == EINPROGRESS the connection takes a while. We do a select
         on this socket fd for writing. If select reports the possibility to
         write, the connection was done successfully (?? can't find in the
         documentation, whether we have to do a connect again if select reports
         the possibility to write). HP dont want the 2nd connect. The result is
         a EINVAL error. Didnt find this problem on other machines. 
       */

      if (errorcode == EINPROGRESS) {
         /* we try it when select reports activity */
         SET_MESSAGE_STATUS(mp, S_CONNECTING);
         DEXIT;
         return;
      }
      else {
         strcpy(err_str, MSG_NET_ERRORCONNECTINGTOHOST );
         strcat(err_str, tohostname);
         strcat(err_str, " :");
         strcat(err_str, strerror(errorcode));
         /* close and reset mp->tofd */
         close(mp->tofd);
         mp->tofd = -1;
         reset_message(mp, err_str);
         now = sge_get_gmt();

         /* If we get a connection refused the host can be reached, but the 
            commd is not alive (or at the wrong port). Another possibility is, 
            that the commproc 
            registers to this host and is not enrolled at this moment. In both
            cases we declare the message as not deliverable. */
         if (errorcode == ECONNREFUSED) {

            INFO((SGE_EVENT, MSG_RWFD_DELIVERMESSAGEFAILEDBUTALIVE ));

            if (mp->flags & COMMD_SYNCHRON) {
               mp->ackchar = COMMD_NACK_DELIVERY;
               SET_MESSAGE_STATUS(mp, S_WRITE_ACK);
            }
            else
               delete_message(mp, "connection refused");
            DEXIT;   
            return;
         }

         if (mp->first_try) {
            if (now - mp->first_try > MESSAGE_MAXDELIVERTIME) {
               INFO((SGE_EVENT, MSG_RWFD_DELIVERMESSAGEFAILEDMAXDELTIMEEXCEEDED ));

               if (mp->flags & COMMD_SYNCHRON) {
                  mp->ackchar = COMMD_NACK_DELIVERY;
                  SET_MESSAGE_STATUS(mp, S_WRITE_ACK);
               }
               else
                  delete_message(mp, "max deliver time exceeded");
            }
         }
         else
            mp->first_try = now;
         DEXIT;   
         return;
      }
   }

   sso = 1;
#if defined(SOLARIS) && !defined(SOLARIS64)
      setsockopt(mp->tofd, IPPROTO_TCP, TCP_NODELAY, (const char *) &sso, sizeof(int));
#else
      setsockopt(mp->tofd, IPPROTO_TCP, TCP_NODELAY, &sso, sizeof(int));
#endif
      
   deliver_message(mp, 0);

   /* now we have to do a select, before writing to the newly created socket,
      cause maybe the connection is not established */

   DEXIT;
}

/**********************************************************************
  We have a message which is ready to send. May be we even got it or we
  had no receiver for it 
 **********************************************************************/
void init_send(
message *mp,
int reserved_port,
int commdport 
) {
   commproc *commp;
   int answer = 0;

   DENTER(TOP_LAYER, "init_send");
   
   /* Is the target commproc enrolled on this machine ? */
   commp = match_commproc(&mp->to);

   DEBUG((SGE_EVENT, "target %s on this host", commp ? "enrolled" : "not enrolled"));

   /* If we are on the host reponsible for the commproc who is asked for
      (target commproc is enrolled, or target host == localhost) and this
      is an ask_commproc request we generate an answer */
   if (mp->flags & COMMD_ASK_COMMPROC) {
      DEBUG((SGE_EVENT, "rwfd ask commproc"));

      if (commp) {
         /* The commproc is enrolled on this host */
         answer = 1;
         mp->ackchar = COMMD_CACK;
      }
      else {
         if (mp->to.host == localhost) {
            /* The commproc isnot enrolled on this host, but this is the
               target host -> reply negative */
            answer = 1;
            mp->ackchar = COMMD_NACK_UNKNOWN_RECEIVER;
         }
      }
      if (answer) {
         mp->flags &= ~COMMD_ASK_COMMPROC;    /* clear */
         mp->flags |= COMMD_ASK_COMMPROCR;    /* reply flag */
         SET_MESSAGE_STATUS(mp, S_WRITE_ACK);
         log_message(mp);
         DEXIT;
         return;                /* dont deliver message to receiver! */
      }
   }

   /* look whether receiver is a local registered commproc or receiver is
      expected to be on the local host */
   if (commp || mp->to.host == localhost) {
      /* deliver direct */
      if (commp && commp->w_fd != -1) {
         DEBUG((SGE_EVENT, "receiver commproc is waiting for message (w_fd=%d, w_name=%s, w_id=%d, w_host=%s, w_tag=%d)",
               commp->w_fd, commp->w_name, commp->w_id,
               commp->w_host ? commp->w_host->mainname : "any",
               commp->w_tag));
      }
      if (commp && (commp->w_fd != -1) && 
          ((commp->w_name[0] == '\0') ||
          !strncmp(mp->from.name, commp->w_name, MAXHOSTLEN)) &&
          (!commp->w_id || mp->from.id == commp->w_id) &&
          (!commp->w_host || commp->w_host == mp->from.host) &&
          (!commp->w_tag || commp->w_tag == mp->tag)) {
         /* commproc is waiting for this message */

         mp->tofd = commp->w_fd;   /* assign message to waiting fd */
         commp->w_fd = -1;         /* mark receiver no longer waiting */
         mp->sending_to = commp;   /* store receiver. We may need this in case of an error */
         deliver_message(mp, 1);
         log_message(mp);
         DEXIT;
         return;
      }
      else {                       /* message has to wait until receiver receives */
         DEBUG((SGE_EVENT, "message waiting for receiver"));
         DEXIT;
         return;
      }
   }
   else {
      /* deliver to remote commd */
      open_connection(mp, reserved_port, commdport);
      DEXIT;
      return;                   /* we have to wait another select; 
                                   look into open_connection() */
   }
   /* no DEXIT; statement not reached */
}

/**************************************************************
 failed to deliver -> spool for later retries
 We have no chance to proceed where the write fails. Shutdown socket
 assume commproc no longer waiting for message.
 **************************************************************/
void reset_message(
message *mp,
const char *str 
) {
   DENTER(TOP_LAYER, "reset_message");
   
   DEBUG((SGE_EVENT, "reset message: mid=%d %s", (int)mp->mid, str));
   trace(SGE_EVENT);

   SET_MESSAGE_STATUS(mp, S_RDY_4_SND);
   if (mp->tofd != -1) {
      fd_close(mp->tofd, "to reset message");
      mp->tofd = -1;
   }
   
   DEXIT;
}

/* shuts down and closes a fd if it is not used by a commproc */
void fd_close(
int fd,
char *comment 
) {
   commproc* cp = commprocs;
   
   DENTER(TOP_LAYER, "fd_close");

   /* browse through all commprocs */
   while(cp && cp->fd != fd)
      cp = cp->next;
   /* none found, so it is ok to close fd */
   if(!cp) {
      shutdown(fd, 2);
      close(fd);
      DEBUG((SGE_EVENT, "closing fd=%d %s", fd, comment));
   }

   DEXIT;
}
