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
/* handle the message data structure */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#define MESSAGESTATES
#include "message.h"
#include "commd_io.h"
#include "commd_error.h"
#include "debug_malloc.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_time.h"
#include "commproc.h"
#include "rwfd.h"

#ifdef KERBEROS
#include "sge_gdiP.h"
#endif

extern commproc* commprocs;
extern void lack_of_memory(void);
extern void trace(char *);

static message *new_message(int fd);

message *message_list = NULL;
message *last_message = NULL;

/**********************************************************************/
message *create_message()
{
   message *m;

   m = (message *) malloc(sizeof(message));
   if (!m)
      return NULL;

   memset(m, 0, sizeof(message));
   m->fromfd = -1;
   m->tofd = -1;

   m->next = NULL;
   if (last_message) {
      last_message->next = m;
   }
   else {
      message_list = m;
   }
   last_message = m;
   new_messages++;  /* profiling */

   return m;
}

/* a commproc sends a new message. its fd is still open, so we only
   need to create a new message */
static message* new_message(
int fd 
) {
   message *mp;
   commproc* cp;

   DENTER(TOP_LAYER, "new_message");
   
   cp = commprocs;
   while(cp && cp->fd != fd)
      cp = cp->next;
   if(!cp)
      DPRINTF(("Cannot find commproc on fd %d\n", fd));

   DEBUG((SGE_EVENT, "making new message"));

   mp = create_message();
   if (!mp) {                   /* wait for a better time                   */
      lack_of_memory();         /* we have to avoid busy looping without a  */
      DEXIT;
      return NULL;              /* chance to do s.th.                       */
   }

   SET_MESSAGE_STATUS(mp, S_RECEIVE_PROLOG);
   mp->fromfd = fd;
   mp->bufprogress = mp->prolog;        /* first we read the prolog */

   if (cp) {
      mp->reserved_port = cp->reserved_port;
      memcpy((char*)&mp->fromaddr, (char*)&cp->fromaddr, sizeof(struct in_addr));
   }

   log_message(mp);
   DEXIT;
   return mp;
}

/**********************************************************************/
int exist_message(
message *m 
) {
   message *mp = message_list;

   while (mp) {
      if (mp == m)
         return 1;
      mp = mp->next;
   }
   return 0;
}

/**********************************************************************/
void delete_message(
message *to_delete,
char *reason 
) {
   delete_message_(to_delete, 0, reason);
}

void delete_message_(
message *to_delete,
int keep_fromfd,
char *reason 
) {
   message *m = message_list, *last = NULL;
   u_long len = 0;
   u_long already_written = 0; 
   DENTER(TOP_LAYER, "delete_message_");

   while (m && (m != to_delete)) {
      last = m;
      m = m->next;
   }

   if (!m) {
      DPRINTF(("found no message to delete\n"));
      DEXIT;
      return;
   }
   
   if (reason) {
      DEBUG((SGE_EVENT, "deleting message mid=%d: %s", (int)m->mid, reason));
   }

   if (!keep_fromfd && m->fromfd != -1)
      fd_close(m->fromfd, "deleting fromfd");
   
   if (m->tofd != -1)
      fd_close(m->tofd, "deleting tofd");

   if (last)
      last->next = m->next;
   else
      message_list = m->next;

   if (m == last_message)
      last_message = last;

   if (enable_commd_profile) {
      if (m->buflen > 0) {
         len = m->buflen + m->headerlen;
         already_written = m->bufprogress - HEADERSTART(m);
         data_message_byte_count += already_written;
         data_message_count++;
      }
   }

   if (m->bufstart)
      free(m->bufstart);
   free(m);

   m = NULL;
   del_messages++;  /* profiling */
   DEXIT;
   return;
}


/* used for profiling: count messages in list */
unsigned long count_messages(void) {
   message *messages = message_list;
   unsigned long counter = 0;
   while (messages) {
      counter++;
      messages = messages->next;
   }
   return counter;
}


/* this function returns a string with information
   about the used file descriptors in the message 
   list   (used for profiling) */

/* only used for debuging */

#if 0 
char* count_different_file_descriptors(void) {
   message *messages = message_list;
   int i;
   static const char* debug_buffer[8000];


   int different_file_descriptors[1000];
   int different_file_descriptor_count[1000];
   const char* help[1000];


   for (i=0;i<999;i++) {
      different_file_descriptors[i] = -901; 
      different_file_descriptor_count[i] = 0; 
   }
   while (messages) {
      int fd = messages->tofd;
      for (i=0;i<999;i++) {
         if (different_file_descriptors[i] == -901) {
            different_file_descriptors[i] = fd;
            different_file_descriptor_count[i] = different_file_descriptor_count[i] + 1; 
            break;
         }
         if (different_file_descriptors[i] == fd) {
            different_file_descriptor_count[i] = different_file_descriptor_count[i] + 1;
            break;
         }
      } 
      messages = messages->next;
   }

   strcpy(debug_buffer,"");
   for (i=0;i<999;i++) {
      if (different_file_descriptors[i] == -901) {
         break;
      }
      sprintf(help,"%d=%d ",different_file_descriptors[i],different_file_descriptor_count[i]);
      strcat(debug_buffer,help);
   }

   return debug_buffer;
} 
#endif

/**********************************************************************/
/* finds the message for a given fd. Creates a new message for
 * a commproc.
 * makenew: allows creation of a new message
 */
message *search_message(
int fd,
int makenew 
) {
   message *messages = message_list;

   DENTER(TOP_LAYER, "search_message");

   while (messages) {
      if (fd >= 0)
         if (messages->fromfd == fd || messages->tofd == fd) {
            DEXIT;
            return messages;
         }
      messages = messages->next;
   }
   /* if no existing message for this fd was found, check if there
      is a commproc using this fd and create a new message for
      this commproc. otherwise returns NULL */
   if(makenew) {
      messages = new_message(fd);
      DEXIT;
      return messages;
   } else {
      DEXIT;
      return NULL;
   }
}

/**********************************************************************/
/* Message prolog has been received. Allocate memory for header and   */
/* body of message                                                    */
/**********************************************************************/
int allocate_message(
message *mp 
) {

   /* Header has fixed length, cause maybe for retransmission we need
      another header(length). We want header and body in one peace to 
      simplify transmission. */

   mp->bufstart = malloc(HEADERLEN + mp->buflen);
   mp->bufdata = mp->bufstart + HEADERLEN;
   mp->bufprogress = HEADERSTART(mp);

   return (!mp->bufstart);
}

int message_log_fd = -1;
/**********************************************************************
  Stuff to log messages to a file. We can use a external program to
  watch the messages as they come and go.
 **********************************************************************/
int enable_message_logging(
char *fname 
) {
   return (message_log_fd = open(fname, O_WRONLY | O_APPEND | O_CREAT, 0666));
}

/**********************************************************************/
void disable_message_logging()
{
   if (message_log_fd != -1)
      close(message_log_fd);
   message_log_fd = -1;
}

/**********************************************************************/
void log_message(
message *mp 
) {
   if (message_log_fd != -1) {
      write(message_log_fd, mp, sizeof(message));
   }
}

/***********************************************************************/
/* look for a message which came from commproc 'from' and is addressed */
/* to commproc 'to'                                                    */
/***********************************************************************/
message *match_message(
commproc *to,
commproc *from,
int tag,
int status 
) {
   message *m = message_list, *best = NULL;
   int bestpriority = 10, *ip = to->tag_priority_list, i;

   while (m) {
      if (match2commprocs(&m->to, to) && match2commprocs(&m->from, from) &&
          (status == 0 || MESSAGE_STATUS(m) == status) &&
#ifndef KERBEROS
          (!tag || (tag == m->tag))) {
#else
      /* yes, this is ugly, but we need to pass authorization
         failure through regardless of the client's request */
          (!tag || (tag == m->tag) || m->tag == TAG_AUTH_FAILURE)) {
#endif /* KERBEROS */

#ifndef KERBEROS
         if (tag || !ip[0])     /* tag given or no tag priority */
            return m;
#else
         if (tag || !ip[0] || m->tag == TAG_AUTH_FAILURE)
            return m;
#endif /* KERBEROS */
         if (best) {
            i = 0;
            while (i < bestpriority && ip[i] && ip[i] != m->tag)
               i++;
            if (i < bestpriority && ip[i] && ip[i] == m->tag) {
               bestpriority = i;
               best = m;
            }
         }
         else
            best = m;
      }
      m = m->next;
   }

   return best;
}

/**************************************************/

char *message_status_string(
message *mp 
) {
   u_long status = MESSAGE_STATUS(mp);
   return (status <= MAX_MESSAGE_STATE) ? message_states[status] : "ill";
}

/**************************************************/
void print_message(
message *mp,
FILE *fp 
) {
   u_long status;
   dstring ds;
   char buffer[128];

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   fprintf(fp, "from:\n");
   print_commproc(&mp->from, fp);
   fprintf(fp, "to:\n");
   print_commproc(&mp->to, fp);
   fprintf(fp, "mid="u32" headerlen=%d buflen="u32"\n", mp->mid, mp->headerlen,
           mp->buflen);
   status = MESSAGE_STATUS(mp);
   fprintf(fp, "tofd=%d fromfd=%d tag=%d status=%ld (%s)\n", mp->tofd,
           mp->fromfd, mp->tag, status, message_status_string(mp));
   fprintf(fp, "flags: %s, creation date: %s\n",
           (mp->flags & COMMD_SYNCHRON) ? "SYNCHRON" : "ASYNCHRON",
           sge_ctime32(&mp->creation_date, &ds));

   if (mp->flags & COMMD_ENROLL)
      fprintf(fp, "ENROLL ");
   if (mp->flags & COMMD_RESVPORT)
      fprintf(fp, "RESVPORT ");
   if (mp->flags & COMMD_SCOMMD)
      fprintf(fp, "COMMD ");
   if (mp->flags & COMMD_RECEIVE)
      fprintf(fp, "RECEIVE ");
   if (mp->flags & COMMD_LEAVE)
      fprintf(fp, "LEAVE ");
   if (mp->flags & COMMD_CNTL)
      fprintf(fp, "CNTL ");
   if (mp->flags & COMMD_ASK_COMMPROC)
      fprintf(fp, "ASK_COMMPROC ");
   if (mp->flags & COMMD_ASK_COMMPROC)
      fprintf(fp, "ASK_COMMPROCR ");

   fprintf(fp, "\n");
}

/**************************************************/
void print_messages(
FILE *fp 
) {
   message *mp = message_list;

   while (mp) {
      print_message(mp, fp);
      if (mp->next)
         fprintf(fp, "---------------\n");
      mp = mp->next;
   }
}

/*************************************************/
void look4timeouts_messages(
u_long now 
) {
   message *mp = message_list, *next;

   DENTER(TOP_LAYER, "look4timeouts_messages");

   while (mp) {
      next = mp->next;
      if ((MESSAGE_STATUS(mp) == S_RDY_4_SND) &&
          (now - mp->creation_date > MESSAGE_MAXDELIVERTIME)) {

         DEBUG((SGE_EVENT, 
                "message deleted (timeout): to=(%s %s %d) from=(%s %s %d) tag=%d len="u32,
                mp->to.host->he.h_name, mp->to.name, mp->to.id,
                mp->from.host->he.h_name, mp->from.name, mp->from.id,
                mp->tag, mp->buflen));

         delete_message(mp, "exceeded max deliver time");
      }
      mp = next;
   }
   DEXIT;
}
