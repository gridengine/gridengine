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
#define DEBUG

#include "rmon_h.h"
#include "rmon_def.h"
#include "rmon_err.h"
#include "rmon_rmon.h"
#include "rmon_request.h"
#include "rmon_connect.h"

#include "rmon_message_list.h"
#include "rmon_message_protocol.h"

#include "rmon_c_c_flush.h"
#include "msg_rmon.h"
/********** TYPES *********************************************/

typedef struct process_list_type {
   u_long inet_addr;
   u_long port;
   u_long pid;
   u_long i;
   struct process_list_type *next;
} process_list_type;

/********** GLOBAL VARIABLES **********************************/

extern u_long message_counter;

extern u_long print_host;
extern u_long print_port;
extern u_long print_pid;
extern u_long print_numbers;

extern long wait_time;
extern long max_wait_time;
extern u_long flush_rate;
extern u_long mynumber;

extern u_long rmond[];

long was_waiting;
process_list_type *process_list = NULL;

/********** FUNCTION PROTOTYPES *******************************/

static void rmon_compute_flush_rate(u_long i, u_long max_msg);
static void rmon_print_and_delete_message_list(void);

/********** FUNCITONS *****************************************/

int rmon_c_c_flush()
{
   message_list_type **ptr, *neu;
   int i, n, m, sfd;
   u_long status, max_msg = 0;

#undef FUNC
#define FUNC "rmon_c_c_flush"

   DENTER;

   DPRINTF(("Number of this Client: %d\n", mynumber));

   request.client_number = mynumber;

   if (!(status = rmon_connect_rmond(&sfd, MESSAGE_FLUSH))) {
      switch (errval) {
      case ECONNREFUSED:
         rmon_errf(TERMINAL, MSG_RMON_NORMONDAVAILABLE);
      }
      DEXIT;
      return 0;
   }

   switch (status) {
   case S_ACCEPTED:
   case S_STILL_WAITING:
      break;

   case S_UNKNOWN_CLIENT:
      printf(MSG_RMON_THISRMONNRXWASQUITTEDBYRMOND_D , (u32c) mynumber);
      shutdown(sfd, 2);
      close(sfd);
      DEXIT;
      exit(0);

   default:
      printf(MSG_RMON_UNEXPECTEDPROTOCOLVALUEX_D , (u32c)request.status);
      shutdown(sfd, 2);
      close(sfd);
      DEXIT;
      exit(0);
   }                            /* switch */

   /* get the message-list of every spy you listen to */
   ptr = &message_list;
   n = request.n_spy;

   for (i = 0; i < n; i++) {
      m = 0;
      do {
         neu = rmon_receive_message(sfd, &status);
         switch (status) {
         case MESSAGE:
            *ptr = neu;
            ptr = &(neu->next);
            m++;
            break;

         case ERROR:
            DPRINTF(("Daten nicht gelesen !\n"));
            shutdown(sfd, 2);
            close(sfd);
            DEXIT;
            return 0;
         }
      } while (status != EOL);

      if (eol.n != m) {
         DPRINTF(("eol.n != m\n"));
         DPRINTF(("n = %d\n", n));
         DPRINTF(("m = %d\n", m));
      }

      /* compute the largest number of received messages */
      if (max_msg < eol.n)
         max_msg = eol.n;

   }                            /* for */

   if (!rmon_send_ack(sfd, S_ACCEPTED)) {
      DEXIT;
      return 0;
   }

   shutdown(sfd, 2);
   close(sfd);

   rmon_compute_flush_rate(i, max_msg);
   rmon_print_and_delete_message_list();

   DEXIT;
   return 1;
}                               /* c_c_flush  */

/* **************************************************************** */
/* SUBROUTINES                                                                                                          */
/* **************************************************************** */

static void rmon_print_and_delete_message_list()
{

   message_list_type *ml, *temp;
   process_list_type *pl = NULL, *t;
   struct in_addr bert;

#undef FUNC
#define FUNC "rmon_print_and_delete_message_list"

   DENTER;

   /* print and delete list */
   ml = message_list;
   while (ml) {

      if (print_host) {
         bert.s_addr = htonl(ml->inet_addr);
         printf("%s ", inet_ntoa(bert));
      }

      if (print_port)
         printf("%5ld ", ml->port);

      if (print_pid)
         printf("%ld ", ml->pid);

      if (print_numbers) {

         /* find process counter */
         if (!pl
             || pl->inet_addr != ml->inet_addr
             || pl->port != ml->port
             || pl->pid != ml->pid) {

            pl = process_list;
            while (pl && pl->pid != ml->pid)
               pl = pl->next;

            /* if not found make new block */
            if (!pl) {
               t = (process_list_type *) malloc(sizeof(process_list_type));
               if (!t)
                  rmon_errf(TERMINAL, MSG_RMON_MALLOCFAILED );

               t->inet_addr = ml->inet_addr;
               t->port = ml->port;
               t->pid = ml->pid;
               t->i = 0;
               t->next = process_list;

               pl = process_list = t;
            }
         }

         printf("%6ld ", pl->i++);
      }

      printf("%s", ml->data);
      temp = ml->next;
      free(ml);
      ml = temp;
   }
   fflush(stdout);
   message_list = NULL;
   message_counter = 0;

   DEXIT;

   return;
}

/* **************************************************************** */

static void rmon_compute_flush_rate(
u_long i,
u_long max_msg 
) {

#undef FUNC
#define FUNC "rmon_compute_flush_rate"

   DENTER;

   if (wait_time < max_wait_time) {
      if (i == S_STILL_WAITING) {
         was_waiting = 1;
         wait_time++;
      }

      if (i != S_STILL_WAITING && max_msg == 0) {
         was_waiting = 0;
         wait_time++;
      }
   }

   if (i != S_STILL_WAITING) {
      if (was_waiting)
         wait_time = flush_rate;

      if (max_msg != 0) {
         was_waiting = 0;
         wait_time = flush_rate;
      }
   }

   DEXIT;

   return;
}
