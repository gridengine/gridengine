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
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "message.h"
#include "commproc.h"
#include "commd_error.h"
#include "commd_io.h"
#include "sge_time.h"
#include "sgermon.h"
#include "sge_log.h"
#include "rwfd.h"
#include "msg_commd.h"



/* from reset_messages_for_commproc.c */
void reset_messages_for_commproc(commproc *commp);

/* exported by commd.c */
extern void lack_of_memory(void);

extern message *termination_message;
extern int message_tracefd;

extern char *aliasfile;

extern void deliver_message(message *mp, int local);
extern void init_send(message *mp, int reserved_port, int commdport);

void trace(char *);
static char *mid2str(u_long32 mid);

void dump(void);

int process_received_message(message *mp, int port_security, int commdport);

/******************************************************************/
/* A message came in. Look what it is and start whatever is to do */
/* return 0 if mp is still valid */
/******************************************************************/
int process_received_message(
message *mp,
int port_security,
int commdport 
) {
   commproc *new, *commp, to, from, tmpnew;
   host *h, *thost, *fhost = NULL;
   unsigned char *cp;
   char tohost[MAXHOSTLEN], fromhost[MAXHOSTLEN], hostname[MAXHOSTLEN];
   message *m;
   int synchron, i;
   u_short operation, id, refresh_aliases, us;
   char newname[MAXCOMPONENTLEN];
   int tag;
   ushort ustag;
   u_short compressed;
   u_long32 op_arg;
   char op_carg[MAXCOMPONENTLEN];       /* only char parameter at the moment is 
                                           commproc */
   int found;
   char *uhostname;
   u_long32 now;
   u_short closefd;
   
   DENTER(TOP_LAYER, "process_received_message");

   DEBUG((SGE_EVENT, "process received message portsec=%d commdport=%d fromfd=%d tofd=%d",
         port_security, commdport,mp->fromfd, mp->tofd));

   /* set arrival time */
   now = sge_get_gmt();
   mp->creation_date = now;

   /* look for host of sender */
   h = search_host(NULL, (char *) &mp->fromaddr);

   if (h)
      DEBUG((SGE_EVENT, "found sender host %s", get_mainname(h)));
   else
      DEBUG((SGE_EVENT, "sender host not found"));

   /********** security section ******************/

   if (port_security && !mp->reserved_port) {
      mp->ackchar = COMMD_NACK_PERM;
      SET_MESSAGE_STATUS(mp, S_WRITE_ACK);

      DEBUG((SGE_EVENT, "message denied cause of unsecure port"));
      trace(SGE_EVENT);

      DEXIT;
      return 0;
   }

   /*********** a commproc wants to enroll **********/
   if (mp->flags & COMMD_ENROLL) {

      DEBUG((SGE_EVENT, "* enrolling commproc"));

      if (!h) {
         read_aliasfile(aliasfile);
         h = newhost_addr((char *) &mp->fromaddr);

         if (!h) {
            DEBUG((SGE_EVENT, "message from unknown host %s",
                  inet_ntoa(mp->fromaddr)));
            trace(SGE_EVENT);

            mp->ackchar = COMMD_NACK_UNKNOWN_HOST;
            SET_MESSAGE_STATUS(mp, S_WRITE_ACK);

            DEXIT;
            return 0;
         }
      }

      cp = unpack_string(newname, MAXCOMPONENTLEN, HEADERSTART(mp));
      cp = unpack_ushort(&id, cp);
      cp = unpack_ushort(&closefd, cp);

      if (id) {                 /* if id is not specified we always
                                   find a unique commproc description */
         tmpnew.host = h;
         strcpy(tmpnew.name, newname);
         tmpnew.id = id;

         commp = match_commproc(&tmpnew);
         if (commp) {           /* allready enrolled -> NACK_CONFLICT */
            DEBUG((SGE_EVENT, "Commproc tries to enroll with same host, name and id: %s:%s:%d",
                  get_mainname(h), newname, id));
            trace(SGE_EVENT);

            mp->ackchar = COMMD_NACK_CONFLICT;
            SET_MESSAGE_STATUS(mp, S_WRITE_ACK);

            DEXIT;
            return 0;
         }
      }

      if(!closefd) /* commproc wants to keep fd open, so store fd info */
         new = add_commproc(mp->fromfd, mp->reserved_port, &mp->fromaddr);
      else
         new = add_commproc(-1, 0, NULL);
      if (!new) {               /* try later */
         lack_of_memory();
         
         DEXIT;
         return 0;
      }
      new->host = h;
      strcpy(new->name, newname);

      if (setcommprocid(new, id)) {
         mp->ackchar = COMMD_NACK_COMMD_NOT_READY;
         
         ERROR((SGE_EVENT, MSG_PROC_RECEIVED_MESS_OUTOFCOMMPROCIDS ));
         trace(SGE_EVENT);

         delete_commproc(new);
         SET_MESSAGE_STATUS(mp, S_WRITE_ACK);
         DEXIT;
         return 0;
      }
      
      for (i = 0; i < 10; i++) {
         cp = unpack_ushort(&us, cp);
         new->tag_priority_list[i] = (int) us;
      }

      NOTICE((SGE_EVENT, "ENROLL: %s:%d from %s %d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
              new->name, new->id, get_mainname(new->host),
              new->tag_priority_list[0], new->tag_priority_list[1],
              new->tag_priority_list[2], new->tag_priority_list[3],
              new->tag_priority_list[4], new->tag_priority_list[5],
              new->tag_priority_list[6], new->tag_priority_list[7],
              new->tag_priority_list[8], new->tag_priority_list[9]));
      trace(SGE_EVENT);

      /* reassemble message as acknowledge message */

      mp->ackchar = COMMD_CACK;
#if defined(CRAY) || defined(NECSX4) || defined(NECSX5)
      mp->bufprogress = &(mp->bufstart[HEADERLEN - 3]);
#else
      mp->bufprogress = (unsigned char *) (mp->bufdata - (unsigned char *) 3);
#endif
      cp = mp->bufprogress;
      *cp++ = mp->ackchar;
      pack_ushort(new->id, cp);
      mp->buflen = 0;
      mp->headerlen = 3;
      mp->tofd = mp->fromfd;
      mp->fromfd = -1;
      SET_MESSAGE_STATUS(mp, S_WRITE_ACKE);

      log_message(mp);
      DEXIT;
      return 0;
   }                            /* ENROLL */

/*********** CONTROL OPERATION **********/
   if (mp->flags & COMMD_CNTL) {

      DEBUG((SGE_EVENT, "* controlling operation"));

      if (!h) {
         read_aliasfile(aliasfile);
         h = newhost_addr((char *) &mp->fromaddr);

         if (!h) {
            DEBUG((SGE_EVENT, "control message from unknown host %s",
                    inet_ntoa(mp->fromaddr)));
            trace(SGE_EVENT);

            mp->ackchar = COMMD_NACK_UNKNOWN_HOST;
            SET_MESSAGE_STATUS(mp, S_WRITE_ACK);

            DEXIT;
            return 0;
         }
      }

      cp = unpack_ushort(&operation, HEADERSTART(mp));
      cp = unpack_ulong(&op_arg, cp);
      cp = unpack_string(op_carg, sizeof(op_carg), cp);

      DEBUG((SGE_EVENT, "CNTL OPERATION %d:"u32":%s from host %s", 
            operation, op_arg, op_carg, get_mainname(h)));
      trace(SGE_EVENT);

      if (operation == O_KILL)
         termination_message = mp;      /* life until message is acknowledged */

      if (operation == O_DUMP)
         dump();

      /* trace operation: client waits for ascii text on his fd */
      if (operation == O_TRACE) {
         char ackchar;

         if (message_tracefd == -1) {
            DEBUG((SGE_EVENT, "open tracefd=%d", mp->fromfd));

            message_tracefd = mp->fromfd;
            ackchar = COMMD_CACK;
            write(message_tracefd, &ackchar, 1);
            delete_message_(mp, 1, NULL);
         }
         else {
            trace("rejected trace cntl message (only one trace allowed)");
            ackchar = COMMD_NACK_OPONCE;
            i = write(mp->fromfd, &ackchar, 1);
            DEBUG((SGE_EVENT, "fromfd=%d i=%d", mp->fromfd, i));
            delete_message(mp, "only one trace commproc allowed");
         }
         DEXIT;
         return 1;
      }

      if (operation == O_GETID) {
         commp = search_commproc(NULL, op_carg, 0);
         if (commp) {
            mp->ackchar = COMMD_CACK;
            mp->headerlen = (u_short) (1 + pack_ulong_len(commp->id));
            mp->bufprogress = HEADERSTART(mp);
            cp = mp->bufprogress;
            *cp++ = mp->ackchar;
            pack_ulong(commp->id, cp);
            mp->buflen = 0;
            mp->tofd = mp->fromfd;
            mp->fromfd = -1;
            SET_MESSAGE_STATUS(mp, S_WRITE_ACKE);
            
            DEXIT;
            return 0;
         }
         else {
            mp->ackchar = COMMD_NACK_UNKNOWN_TARGET;
            SET_MESSAGE_STATUS(mp, S_WRITE_ACK);
            
            DEXIT;
            return 0;
         }
      }

      if (operation == O_UNREGISTER) {
         found = 0;
         while ((commp = search_commproc(NULL, op_carg, op_arg))) {
            found = 1;

            if (commp->w_fd != -1) {
               fd_close(commp->w_fd, "to manually unregister");
               commp->w_fd = -1;
            }
            reset_messages_for_commproc(commp);
            delete_commproc(commp);
         }

         if (found) {
            mp->ackchar = COMMD_CACK;
            SET_MESSAGE_STATUS(mp, S_WRITE_ACK);
            
            DEXIT;
            return 0;
         }
         else {
            mp->ackchar = COMMD_NACK_UNKNOWN_TARGET;
            SET_MESSAGE_STATUS(mp, S_WRITE_ACK);
            
            DEXIT;
            return 0;
         }
      }
      /* reassemble message as acknowledge message */

      mp->ackchar = COMMD_CACK;
      SET_MESSAGE_STATUS(mp, S_WRITE_ACK);

      log_message(mp);
      DEXIT;
      return 0;
   }                            /* CONTROL */

   /* for all further message types the sender has to be enrolled if he is not
      a commd */

   /* unknown host ? */
   if (!h && (mp->flags & COMMD_SCOMMD)) {
      read_aliasfile(aliasfile);
      h = newhost_addr((char *) &mp->fromaddr);
   }
   if (!h) {
      WARNING((SGE_EVENT, "can't resolve host address %s", inet_ntoa(mp->fromaddr)));
      trace(SGE_EVENT);

      mp->ackchar = COMMD_NACK_ENROLL;
      SET_MESSAGE_STATUS(mp, S_WRITE_ACK);
      DEXIT;
      return 0;
   }

/*********** a commproc wants to leave **********/
   if (mp->flags & COMMD_LEAVE) {

      DEBUG((SGE_EVENT, "* leaving commproc"));

      cp = unpack_string(mp->from.name, MAXCOMPONENTLEN, HEADERSTART(mp));
      cp = unpack_ushort(&(mp->from.id), cp);
      mp->from.host = h;

      commp = match_commproc(&mp->from);
      NOTICE((SGE_EVENT, "LEAVE: %s:%d from %s%s", mp->from.name, mp->from.id,
              get_mainname(h), commp ? "" : " (not enrolled)"));
      trace(SGE_EVENT);

      if (commp) {
         if (mp->new_connection && reconnect_commproc_with_fd(commp, mp->fromfd, "LEAVE")) {
            mp->ackchar = COMMD_NACK_CONFLICT;
            SET_MESSAGE_STATUS(mp, S_WRITE_ACK);

            DEXIT;
            return 0;
         }

         mp->ackchar = COMMD_CACK;
         reset_messages_for_commproc(commp);
         commp->fd = -1;    /* this leaves the fd open for a short time */
                            /* otherwise we could not send ACK */
                            /* fd gets closed when messages is deleted */
         delete_commproc(commp);
      }
      else
         mp->ackchar = COMMD_NACK_ENROLL;

      SET_MESSAGE_STATUS(mp, S_WRITE_ACK);

      log_message(mp);
      
      DEXIT;
      return 0;
   }                            /* LEAVE */

   /* a commproc asks about a unique hostname and may force a reread
      of the alias file */

   if (mp->flags & COMMD_UNIQUEHOST) {

      DEBUG((SGE_EVENT, "* commproc asks about UNIQUE HOST"));

      cp = unpack_string(hostname, MAXHOSTLEN, HEADERSTART(mp));
      cp = unpack_ushort(&refresh_aliases, cp);

      cp = unpack_string(from.name, MAXCOMPONENTLEN, cp);
      cp = unpack_ushort(&from.id, cp);
      from.host = h;

      if (refresh_aliases)
         read_aliasfile(aliasfile);

      h = search_host(hostname, NULL);
      if (!h)
         h = newhost_name(hostname, NULL);

      if (!h && !refresh_aliases)
         read_aliasfile(aliasfile);

      if (h)
         uhostname = get_mainname(h);
      else
         uhostname = "<unknown host>";

      DEBUG((SGE_EVENT, "UNIQUE HOST: asked host=%s returning %s",
            hostname, uhostname));
      trace(SGE_EVENT);

      /* look for asking commproc */
      if (!(commp = match_commproc(&from))) {
         DEBUG((SGE_EVENT, "unique host query: commproc not enrolled: %s:%s",
               from.name, get_mainname(from.host)));
         trace(SGE_EVENT);

         mp->ackchar = COMMD_NACK_UNKNOWN_RECEIVER;
         SET_MESSAGE_STATUS(mp, S_WRITE_ACK);

         DEXIT;
         return 0;
      }

      commproc_touch(commp, now);       /* sign of life */
      if (mp->new_connection && reconnect_commproc_with_fd(commp, mp->fromfd, "UNIQUE HOST")) {
         mp->ackchar = COMMD_NACK_CONFLICT;
         SET_MESSAGE_STATUS(mp, S_WRITE_ACK);

         DEXIT;
         return 0;
      }

      if (!h) {
         mp->ackchar = COMMD_NACK_UNKNOWN_HOST;
         SET_MESSAGE_STATUS(mp, S_WRITE_ACK);
         
         DEXIT;
         return 0;
      }

      /* reassemble message as acknowledge message */

      mp->ackchar = COMMD_CACK;
      mp->headerlen = strlen(uhostname) + 4;
      mp->bufprogress = HEADERSTART(mp);
      cp = mp->bufprogress;
      *cp++ = mp->ackchar;
      cp = pack_ushort(mp->headerlen - 3, cp);  /* strlen incl. \0 */
      cp = pack_string(uhostname, cp);
      mp->buflen = 0;
      mp->tofd = mp->fromfd;
      mp->fromfd = -1;
      SET_MESSAGE_STATUS(mp, S_WRITE_ACKE);

      log_message(mp);

      DEXIT;
      return 0;
   }

   /**********************************************************************
     process receive message 
    **********************************************************************/
   if (mp->flags & COMMD_RECEIVE) {

      DEBUG((SGE_EVENT, "* receive message"));

      memset(&from, 0, sizeof(from));
      memset(&to, 0, sizeof(to));

      synchron = mp->flags & COMMD_SYNCHRON;

      /* look into header of message */
      cp = HEADERSTART(mp);
      thost = h;
      cp = unpack_string(to.name, MAXCOMPONENTLEN, cp);
      cp = unpack_ushort(&to.id, cp);

      cp = unpack_string(fromhost, MAXHOSTLEN, cp);     /* sender host */
      cp = unpack_string(from.name, MAXCOMPONENTLEN, cp);
      cp = unpack_ushort(&from.id, cp);
      cp = unpack_ushort(&ustag, cp);
      tag = ustag;
      cp = unpack_ushort(&compressed, cp);

      if (fromhost[0]) {
         fhost = search_host(fromhost, NULL);
         if (!fhost) {
            read_aliasfile(aliasfile);
            fhost = newhost_name(fromhost, NULL);
         }

         if (!fhost) {
            ERROR((SGE_EVENT, MSG_PROC_RECEIVED_MESS_RECEIVEQUERYMESSFROMUNOWNHOST_S ,
                  fromhost));
            trace(SGE_EVENT);

            mp->ackchar = COMMD_NACK_UNKNOWN_HOST;
            SET_MESSAGE_STATUS(mp, S_WRITE_ACK);

            DEXIT;
            return 0;
         }
         from.host = fhost;
      }
      else
         from.host = NULL;      /* match any sender */

      /* look for receiving commproc */
      to.host = thost;
      commp = match_commproc(&to);
      if (!commp) {
         DEBUG((SGE_EVENT, "receive query: receiver not enrolled: %s:%s",
               to.name, get_mainname(to.host)));
         trace(SGE_EVENT);

         mp->ackchar = COMMD_NACK_UNKNOWN_RECEIVER;
         SET_MESSAGE_STATUS(mp, S_WRITE_ACK);

         DEXIT;
         return 0;
      }

      commproc_touch(commp, now);       /* sign of life */

      if (mp->new_connection && reconnect_commproc_with_fd(commp, mp->fromfd, "RECEIVE")) {
         mp->ackchar = COMMD_NACK_CONFLICT;
         SET_MESSAGE_STATUS(mp, S_WRITE_ACK);

         DEXIT;
         return 0;
      }

      /* look for message which matches receivers wishes and is ready to 
         send */
      m = match_message(commp, &from, tag, S_RDY_4_SND);

      DEBUG((SGE_EVENT, "receive query: message to receive: from=(%s %s %d) to=(%s %s %d) tag=%d %s%s",
            fromhost[0] ? fromhost : "anyhost",
            from.name[0] ? from.name : "anycommproc", from.id, 
            commp->name, commp->host->mainname, commp->id,
            tag,
            m ? "found " : "not found",
            m ? mid2str(m->mid):""));
      trace(SGE_EVENT);

      if (m) {
         /* we can deliver immediately -> activate message which can be 
            delivered */
         m->tofd = mp->fromfd;
         SET_MESSAGE_STATUS(m, S_RDY_4_SND);
         delete_message_(mp, 1, NULL);        /* message no longer needed, dont close fd */

         deliver_message(m, 1);
         
         DEXIT;
         return 1;
      } else {
         if (synchron) {
            /* store wait condition in receiver commproc structure */
            if (commp->w_fd != -1) {
               ERROR((SGE_EVENT, MSG_PROC_RECEIVED_MESS_ALLREADYINRECEIVECLOSEFD_ISISI,
                      (int)commp->w_fd, commp->w_name, (int) commp->w_id,
                      commp->w_host ? commp->w_host->mainname : MSG_COMMPROC_ANY,
                      (int)commp->w_tag));
               fd_close(commp->w_fd, "to overwrite receive state");
               commp->w_fd = -1;

#if 1 
               delete_commproc(commp);
               DEXIT;
               return 0;
#endif
            }                   
            commp->w_fd = mp->fromfd;
            strcpy(commp->w_name, from.name);
            commp->w_host = fhost;
            commp->w_id = from.id;
            commp->w_tag = tag;
            delete_message_(mp, 1, NULL);  /* message no longer needed, don't close fd */
            DEXIT;
            return 1;
         } else {
            /* in case of an asynchron request send NACK immediately because
               receiver doesn't want to wait for a message */
            DEBUG((SGE_EVENT, "receive query: no message available for receiver"));
            trace(SGE_EVENT);

            mp->ackchar = COMMD_NACK_NO_MESSAGE;
            SET_MESSAGE_STATUS(mp, S_WRITE_ACK);
            DEXIT;
            return 0;
         }
      }
   }

   /************************************************************
     This is a normal message to be sent to a receiver.
    ************************************************************/

   DEBUG((SGE_EVENT, "* send message"));

   /* look into header of message and fill mp->to and mp->from */
   cp = HEADERSTART(mp);
   cp = unpack_ulong(&mp->mid, cp);
   cp = unpack_string(tohost, MAXHOSTLEN, cp);
   cp = unpack_string(mp->to.name, MAXCOMPONENTLEN, cp);
   cp = unpack_ushort(&mp->to.id, cp);

   if (mp->flags & COMMD_SCOMMD)
      cp = unpack_string(fromhost, MAXHOSTLEN, cp);

   cp = unpack_string(mp->from.name, MAXCOMPONENTLEN, cp);
   cp = unpack_ushort(&mp->from.id, cp);

   cp = unpack_ushort(&ustag, cp);
   mp->tag = ustag;
   cp = unpack_ushort(&mp->compressed, cp);
   thost = search_host(tohost, NULL);

   if (!thost) {
      read_aliasfile(aliasfile);
      thost = newhost_name(tohost, NULL);
   }

   if (!thost) {
      ERROR((SGE_EVENT, MSG_PROC_RECEIVED_MESS_UNKNOWNRECEIVERHOST_S , tohost));
      trace(SGE_EVENT);

      mp->ackchar = COMMD_NACK_UNKNOWN_HOST;
      SET_MESSAGE_STATUS(mp, S_WRITE_ACK);
      
      DEXIT;
      return 0;
   }

   DEBUG((SGE_EVENT,
         "message to send%s%s: to=(%s %s %d) from=(%s %s %d) tag=%d len="u32" mid=%d",
         ((mp->flags & COMMD_ASK_COMMPROC) ? " (ask)" : ""),
         ((mp->flags & COMMD_SCOMMD) ? " (COMMD<->COMMD)" : ""),
         tohost, mp->to.name, mp->to.id,
         ((mp->flags & COMMD_SCOMMD) ? fromhost : get_mainname(h)), mp->from.name,
         mp->from.id, mp->tag, mp->buflen, (int)mp->mid));
   trace(SGE_EVENT);

   /* If the message came direct from the sender 'fromhost' is the host we got
      from accept(). If not we have to rely on the commd who is sending */
   if (mp->flags & COMMD_SCOMMD) {
      fhost = search_host(fromhost, NULL);
      if (!fhost) {
         read_aliasfile(aliasfile);
         fhost = newhost_name(fromhost, NULL);
         if (!fhost) {
            ERROR((SGE_EVENT, MSG_PROC_RECEIVED_MESS_UNKNOWNSENDERHOST_S , fromhost));
            trace(SGE_EVENT);

            mp->ackchar = COMMD_NACK_UNKNOWN_HOST;
            SET_MESSAGE_STATUS(mp, S_WRITE_ACK);
         
            /* there was a delete_message() */   
            DEXIT;
            return 0;
         }
      }
   }
   else
      fhost = h;

   mp->from.host = fhost;
   mp->to.host = thost;

   /* we need a commproc for the sender of the message, because we
      don't know whether to close the fd or not So lets find the commproc.
      does not apply to COMMD<->COMMD communication */
   commp = match_commproc(&mp->from);
   if (!commp && !(mp->flags & COMMD_SCOMMD)) {
      DEBUG((SGE_EVENT, "send message: unknown sender of message"));
      mp->ackchar = COMMD_NACK_ENROLL;
      SET_MESSAGE_STATUS(mp, S_WRITE_ACK);

      DEXIT;
      return 0;
   }
   
   /* For timing out commprocs we have to update the time marker of the
      sending commproc if registered locally.  */
   if(commp) {
      commproc_touch(commp, now);
      if (mp->new_connection && reconnect_commproc_with_fd(commp, mp->fromfd, "SEND")) {
         mp->ackchar = COMMD_NACK_CONFLICT;
         SET_MESSAGE_STATUS(mp, S_WRITE_ACK);

         DEXIT;
         return 0;
      }
   }

   if (mp->flags & COMMD_SYNCHRON) {
      SET_MESSAGE_STATUS(mp, S_RDY_4_SND);
      log_message(mp);
      init_send(mp, port_security, commdport);

      DEXIT;
      return 1;                 /* may be mp is deleted on error */
   }
   else {
      /* asynchron messages are acknowledged immediately */
      mp->ackchar = COMMD_CACK;
      SET_MESSAGE_STATUS(mp, S_WRITE_ACK_SND);
      log_message(mp);
      
      DEXIT;
      return 0;
   }
}

/**********************************************************************
  commdcntl -t gets this stuff printed

  trace messages as they come and go without sending to much information

  If commdcntl is broken we get a SIGPIPE, which we ignore and we get
  a negative return value from write. If this happens we shutdown the
  socket and allows another commdcntl to connect.
 **********************************************************************/
void trace(
char *str 
) {
   char nl = '\n';
   int i;

   DENTER(BASIS_LAYER, "trace");
   
   if (message_tracefd != -1) {
      i = write(message_tracefd, str, strlen(str));
      if (i >= 1) {
         i = write(message_tracefd, &nl, 1);
      }
      
      if (i == -1 || !i) {
         shutdown(message_tracefd, 2);
         close(message_tracefd);
         message_tracefd = -1;
         DEBUG((SGE_EVENT, "broken pipe to trace module->closing my side of connection"));
      }
   }
   DEXIT;
}

static char *mid2str(
u_long32 mid 
) {
   static char buffer[20];
   sprintf(buffer, "mid=%d", (int)mid);
   return buffer; 
}
