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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#if defined(DARWIN)
#  include <termios.h>
#  include <sys/ttycom.h>
#  include <sys/ioctl.h>
#elif defined(HP11) || defined(HP1164)
#  include <termios.h>
#elif defined(INTERIX)
#  include <termios.h>
#  include <sys/ioctl.h>
#elif defined(FREEBSD) || defined(NETBSD)
#  include <termios.h>
#  include <sys/ioctl.h>
#else
#  include <termio.h>
#endif

#include "sgermon.h"
#include "sge_io.h"
#include "sge_utility.h"
#include "sge_pty.h"
#include "sge_ijs_comm.h"
#include "sge_ijs_threads.h"
#include "sge_client_ijs.h"

/* global variables */
char *g_hostname  = NULL;
COMMUNICATION_HANDLE *g_comm_handle = NULL;

/* module variables */
static int  g_exit_status = 0;
static int  g_nostdin     = 0; 
static int  g_noshell     = 0;

/*
 * static volatile sig_atomic_t received_window_change_signal = 1;
 * Flag to indicate that we have received a window change signal which has
 * not yet been processed.  This will cause a message indicating the new
 * window size to be sent to the server a little later.  This is volatile
 * because this is updated in a signal handler.
 */
static volatile sig_atomic_t received_window_change_signal = 1;
static volatile sig_atomic_t received_broken_pipe_signal = 0;
static volatile sig_atomic_t quit_pending; /* Set non-zero to quit the loop. */

volatile sig_atomic_t received_signal = 0;

/****** window_change_handler **************************************************
*  NAME
*     window_change_handler() -- handler for the window changed signal
*
*  SYNOPSIS
*     static void window_change_handler(int sig)
*
*  FUNCTION
*     Signal handler for the window change signal (SIGWINCH).  This just sets a
*     flag indicating that the window has changed.
*
*  INPUTS
*     int sig - number of the received signal
*
*  RESULT
*     void - no result
*
*  NOTES
*    MT-NOTE: window_change_handler() is not MT safe, because it uses
*             received_window_change_signal
*
*  SEE ALSO
*******************************************************************************/
static void window_change_handler(int sig)
{
   /* Do not use DPRINTF in a signal handler! */
   received_window_change_signal = 1;
   signal(SIGWINCH, window_change_handler);
}

/****** broken_pipe_handler ****************************************************
*  NAME
*     broken_pipe_handler() -- handler for the broken pipe signal
*
*  SYNOPSIS
*     static void broken_pipe_handler(int sig)
*
*  FUNCTION
*     Handler for the SIGPIPE signal.
*
*  INPUTS
*     int sig - number of the received signal
*
*  RESULT
*     void - no result
*
*  NOTES
*
*  SEE ALSO
*******************************************************************************/
static void broken_pipe_handler(int sig)
{
   received_broken_pipe_signal = 1;
   signal(SIGPIPE, broken_pipe_handler);
}

/****** signal_handler() *******************************************************
*  NAME
*     signal_handler() -- handler for quit signals
*
*  SYNOPSIS
*     static void signal_handler(int sig)
*
*  FUNCTION
*     Handler for all signals that quit the program. These signals are trapped 
*     in order to restore the terminal modes.
*
*  INPUTS
*     int sig - number of the received signal
*
*  RESULT
*     void - no result
*
*  NOTES
*
*  SEE ALSO
*******************************************************************************/
void signal_handler(int sig)
{
   received_signal = sig;
   quit_pending = 1;
}

/****** set_signal_handlers() **************************************************
*  NAME
*     set_signal_handlers() -- set all signal handlers
*
*  SYNOPSIS
*     static void set_signal_handlers(void)
*
*  FUNCTION
*     Sets all signal handlers. Doesn't overwrite SIG_IGN and therefore
*     matches the behaviour of rsh.
*
*  RESULT
*     void - no result
*
*  NOTES
*
*  SEE ALSO
*******************************************************************************/
void set_signal_handlers(void)
{
  struct sigaction old_handler, new_handler;

   /* Is SIGHUP necessary? 
    * Yes: termio(7I) says:
    * "When a modem disconnect is detected, a SIGHUP signal is sent
    *  to the terminal's controlling process.
    *  Unless other arrangements have  been  made,  these  signals
    *  cause  the  process  to  terminate. If  SIGHUP is ignored or
    *  caught, any subsequent  read  returns  with  an  end-of-file
    *  indication until the terminal is closed."
    */
   sigaction(SIGHUP, NULL, &old_handler);
   if (old_handler.sa_handler != SIG_IGN) {
      new_handler.sa_handler = signal_handler;
      sigaddset(&new_handler.sa_mask, SIGHUP);
      new_handler.sa_flags = SA_RESTART;
      sigaction(SIGHUP, &new_handler, NULL);
   }

   sigaction(SIGINT, NULL, &old_handler);
   if (old_handler.sa_handler != SIG_IGN) {
      new_handler.sa_handler = signal_handler;
      sigaddset(&new_handler.sa_mask, SIGINT);
      new_handler.sa_flags = SA_RESTART;
      sigaction(SIGINT, &new_handler, NULL);
   }

   sigaction(SIGQUIT, NULL, &old_handler);
   if (old_handler.sa_handler != SIG_IGN) {
      new_handler.sa_handler = signal_handler;
      sigaddset(&new_handler.sa_mask, SIGQUIT);
      new_handler.sa_flags = SA_RESTART;
      sigaction(SIGQUIT, &new_handler, NULL);
   }

   sigaction(SIGTERM, NULL, &old_handler);
   if (old_handler.sa_handler != SIG_IGN) {
      new_handler.sa_handler = signal_handler;
      sigaddset(&new_handler.sa_mask, SIGTERM);
      new_handler.sa_flags = SA_RESTART;
      sigaction(SIGTERM, &new_handler, NULL);
   }

   new_handler.sa_handler = window_change_handler;
   sigaddset(&new_handler.sa_mask, SIGWINCH);
   new_handler.sa_flags = SA_RESTART;
   sigaction(SIGWINCH, &new_handler, NULL);

   new_handler.sa_handler = broken_pipe_handler;
   sigaddset(&new_handler.sa_mask, SIGPIPE);
   new_handler.sa_flags = SA_RESTART;
   sigaction(SIGPIPE, &new_handler, NULL);
}

/****** client_check_window_change() *******************************************
*  NAME
*     client_check_window_change() -- check if window size was change and
*                                     submit changes to pty
*
*  SYNOPSIS
*     static void client_check_window_change(COMMUNICATION_HANDLE *handle)
*
*  FUNCTION
*     Checks if the window size of the terminal window was changed.
*     If the size was changed, submits the new window size to the
*     pty.
*     The actual change is detected by a signal (on Unix), this function
*     just checks the according flag.
*
*  INPUTS
*     COMMUNICATION_HANDLE *handle - pointer to the commlib handle
*
*  RESULT
*     void - no result
*
*  NOTES
*     MT-NOTE: client_check_window_change() is MT-safe (see comment in code)
*
*  SEE ALSO
*     window_change_handler()
*******************************************************************************/
static void client_check_window_change(COMMUNICATION_HANDLE *handle)
{
   struct winsize ws;
   char           buf[200];
   dstring        err_msg = DSTRING_INIT;

   DENTER(TOP_LAYER, "client_check_window_change");

   if (received_window_change_signal) {
      /*
       * here we can have a race condition between the two working threads,
       * but it doesn't matter - in the worst case, the new window size gets 
       * submitted two times.
       */
      received_window_change_signal = 0;
      if (ioctl(fileno(stdin), TIOCGWINSZ, &ws) >= 0) {
         DPRINTF(("sendig WINDOW_SIZE_CTRL_MSG with new window size: "
                  "%d, %d, %d, %d to shepherd\n",
                  ws.ws_row, ws.ws_col, ws.ws_xpixel, ws.ws_ypixel));

         sprintf(buf, "WS %d %d %d %d",
                 ws.ws_row, ws.ws_col, ws.ws_xpixel, ws.ws_ypixel);
         comm_write_message(handle, g_hostname, COMM_CLIENT, 1,
                            (unsigned char*)buf, strlen(buf), 
                            WINDOW_SIZE_CTRL_MSG, &err_msg);
      } else {
         DPRINTF(("client_check_windows_change: ioctl() failed! "
            "sending dummy WINDOW_SIZE_CTRL_MSG to fullfill protocol.\n"));
         sprintf(buf, "WS 60 80 480 640");
         comm_write_message(handle, g_hostname, COMM_CLIENT, 1,
            (unsigned char*)buf, strlen(buf), WINDOW_SIZE_CTRL_MSG, &err_msg);
      }
   }
   sge_dstring_free(&err_msg);
   DEXIT;
}

/****** tty_to_commlib() *******************************************************
*  NAME
*     tty_to_commlib() -- tty_to_commlib thread entry point and main loop
*
*  SYNOPSIS
*     void* tty_to_commlib(void *t_conf)
*
*  FUNCTION
*     Entry point and main loop of the tty_to_commlib thread.
*     Reads data from the tty and writes it to the commlib.
*
*  INPUTS
*     void *t_conf - pointer to cl_thread_settings_t struct of the thread
*
*  RESULT
*     void* - always NULL
*
*  NOTES
*     MT-NOTE: tty_to_commlib is MT-safe ?
*
*  SEE ALSO
*******************************************************************************/
void* tty_to_commlib(void *t_conf)
{
   char                 *pbuf;
   fd_set               read_fds;
   struct timeval       timeout;
   dstring              err_msg = DSTRING_INIT;
   int                  ret, nread, do_exit = 0;

   DENTER(TOP_LAYER, "tty_to_commlib");
   thread_func_startup(t_conf);
   
   /* 
    * allocate working buffer
    */
   pbuf = (char*)malloc(BUFSIZE);
   if (pbuf == NULL) {
      DPRINTF(("tty_to_commlib can't allocate working buffer: %s (%d)\n",
         strerror(errno), errno));
      do_exit = 1;
   }

   while (do_exit == 0) {
      FD_ZERO(&read_fds);
      if (g_nostdin == 0) {
         /* wait for input on tty */
         FD_SET(STDIN_FILENO, &read_fds);
      } 
      timeout.tv_sec  = 1;
      timeout.tv_usec = 0;

      DPRINTF(("tty_to_commlib: Waiting in select() for data\n"));
      ret = select(STDIN_FILENO+1, &read_fds, NULL, NULL, &timeout);

      thread_testcancel(t_conf);
      client_check_window_change(g_comm_handle);

      if (received_signal == SIGHUP ||
          received_signal == SIGINT ||
          received_signal == SIGQUIT ||
          received_signal == SIGTERM) {
         /* If we receive one of these signals, we must terminate */
         do_exit = 1;
         continue;
      }

      if (ret > 0) {
         if (g_nostdin == 1) {
            /* We should never get here if STDIN is closed */
            DPRINTF(("tty_to_commlib: STDIN ready to read while it should be closed!!!\n"));
         }
         DPRINTF(("tty_to_commlib: trying to read() from stdin\n"));
         nread = read(STDIN_FILENO, pbuf, BUFSIZE-1);
         DPRINTF(("tty_to_commlib: nread = %d\n", nread));

         if (nread < 0 && (errno == EINTR || errno == EAGAIN)) {
            DPRINTF(("tty_to_commlib: EINTR or EAGAIN\n"));
            /* do nothing */
         } else if (nread <= 0) {
            do_exit = 1;
         } else {
            DPRINTF(("tty_to_commlib: writing to commlib: %d bytes\n", nread));
            if (comm_write_message(g_comm_handle, g_hostname, 
               COMM_CLIENT, 1, (unsigned char*)pbuf, 
               (unsigned long)nread, STDIN_DATA_MSG, &err_msg) != nread) {
               DPRINTF(("tty_to_commlib: couldn't write all data\n"));
            } else {
               DPRINTF(("tty_to_commlib: data successfully written\n"));
            }
            comm_flush_write_messages(g_comm_handle, &err_msg);
         }
      } else {
         /*
          * We got either a select timeout or a select error. In both cases,
          * it's a good chance to check if our client is still alive.
          */
         DPRINTF(("tty_to_commlib: Checking if client is still alive\n"));
         if (comm_get_connection_count(g_comm_handle, &err_msg) == 0) {
            DPRINTF(("tty_to_commlib: Client is not alive! -> exiting.\n"));
            do_exit = 1;
         } else {
            DPRINTF(("tty_to_commlib: Client is still alive\n"));
         }
      }
   } /* while (do_exit == 0) */

   /* clean up */
   FREE(pbuf);
   thread_func_cleanup(t_conf);
   
   sge_dstring_free(&err_msg);
   DPRINTF(("tty_to_commlib: exiting tty_to_commlib thread!\n"));
   DEXIT;
   return NULL;
}

/****** commlib_to_tty() *******************************************************
*  NAME
*     commlib_to_tty() -- commlib_to_tty thread entry point and main loop
*
*  SYNOPSIS
*     void* commlib_to_tty(void *t_conf)
*
*  FUNCTION
*     Entry point and main loop of the commlib_to_tty thread.
*     Reads data from the commlib and writes it to the tty.
*
*  INPUTS
*     void *t_conf - pointer to cl_thread_settings_t struct of the thread
*
*  RESULT
*     void* - always NULL
*
*  NOTES
*     MT-NOTE: commlib_to_tty is MT-safe ?
*
*  SEE ALSO
*******************************************************************************/
void* commlib_to_tty(void *t_conf)
{
   recv_message_t       recv_mess;
   dstring              err_msg = DSTRING_INIT;
   int                  ret = 0, do_exit = 0;

   DENTER(TOP_LAYER, "commlib_to_tty");
   thread_func_startup(t_conf);

   while (do_exit == 0) {
      /*
       * wait blocking for a message from commlib
       */
      recv_mess.cl_message = NULL;
      recv_mess.data = NULL;

      DPRINTF(("commlib_to_tty: recv_message()\n"));
      ret = comm_recv_message(g_comm_handle, CL_TRUE, &recv_mess, &err_msg);
      if (ret != COMM_RETVAL_OK) {
         /* check if we are still connected to anybody. */
         /* if not - exit. */
         DPRINTF(("commlib_to_tty: error receiving message: %s\n", 
                  sge_dstring_get_string(&err_msg)));
         if (comm_get_connection_count(g_comm_handle, &err_msg) == 0) {
            DPRINTF(("commlib_to_tty: no endpoint found\n"));
            do_exit = 1;
            continue;
         }
      }
      DPRINTF(("commlib_to_tty: received a message\n"));

      thread_testcancel(t_conf);
      client_check_window_change(g_comm_handle);

      if (received_signal == SIGHUP ||
          received_signal == SIGINT ||
          received_signal == SIGQUIT ||
          received_signal == SIGTERM) {
         /* If we receive one of these signals, we must terminate */
         DPRINTF(("commlib_to_tty: shutting down because of signal %d\n",
                 received_signal));
         do_exit = 1;
         continue;
      }

      DPRINTF(("'parsing' message\n"));
      /*
       * 'parse' message
       * A 1 byte prefix tells us what kind of message it is.
       * See sge_ijs_comm.h for message types.
       */
      if (recv_mess.cl_message != NULL) {
         char buf[100];
         switch (recv_mess.type) {
            case STDOUT_DATA_MSG:
               /* copy recv_mess.data to buf to append '\0' */
               memcpy(buf, recv_mess.data, MIN(99, recv_mess.cl_message->message_length - 1));
               buf[MIN(99, recv_mess.cl_message->message_length - 1)] = 0;
               DPRINTF(("commlib_to_tty: received stdout message, writing to tty.\n"));
               DPRINTF(("commlib_to_tty: message is: %s\n", buf));
/* TODO: If it's not possible to write all data to the tty, retry blocking
 *       until all data was written. The commlib must block then, too.
 */
               if (sge_writenbytes(STDOUT_FILENO, recv_mess.data,
                          (int)(recv_mess.cl_message->message_length-1))
                       != (int)(recv_mess.cl_message->message_length-1)) {
                  DPRINTF(("commlib_to_tty: sge_writenbytes() error\n"));
               }
               break;
            case STDERR_DATA_MSG:
               DPRINTF(("commlib_to_tty: received stderr message, writing to tty.\n"));
/* TODO: If it's not possible to write all data to the tty, retry blocking
 *       until all data was written. The commlib must block then, too.
 */
               if (sge_writenbytes(STDERR_FILENO, recv_mess.data,
                          (int)(recv_mess.cl_message->message_length-1))
                       != (int)(recv_mess.cl_message->message_length-1)) {
                  DPRINTF(("commlib_to_tty: sge_writenbytes() error\n"));
               }
               break;
            case WINDOW_SIZE_CTRL_MSG:
               /* control message */
               /* we don't expect a control message */
               DPRINTF(("commlib_to_tty: received window size message! "
                        "This was unexpected!\n"));
               break;
            case REGISTER_CTRL_MSG:
               /* control message */
               /* a client registered with us. With the next loop, the 
                * cl_commlib_trigger function will send the WINDOW_SIZE_CTRL_MSG
                * (and perhaps some data messages),  which is already in the 
                * send_messages list of the connection, to the client.
                */
               DPRINTF(("commlib_to_tty: received register message!\n"));
               /* Send the settings in response */
               sprintf(buf, "noshell = %d", g_noshell);
               ret = (int)comm_write_message(g_comm_handle, g_hostname, COMM_CLIENT, 1,
                  (unsigned char*)buf, strlen(buf)+1, SETTINGS_CTRL_MSG, &err_msg);
               DPRINTF(("commlib_to_tty: sent SETTINGS_CTRL_MSG, ret = %d\n", ret));
               break;
            case UNREGISTER_CTRL_MSG:
               /* control message */
               /* the client wants to quit, as this is the last message the client
                * sends, we can be sure to have received all messages from the 
                * client. We answer with a UNREGISTER_RESPONSE_CTRL_MSG so
                * the client knows that it can quit now. We can quit, also.
                */
               DPRINTF(("commlib_to_tty: received unregister message!\n"));
               DPRINTF(("commlib_to_tty: writing UNREGISTER_RESPONSE_CTRL_MSG\n"));

               /* copy recv_mess.data to buf to append '\0' */
               memcpy(buf, recv_mess.data, MIN(99, recv_mess.cl_message->message_length - 1));
               buf[MIN(99, recv_mess.cl_message->message_length - 1)] = 0;
               sscanf(buf, "%d", &g_exit_status);
               comm_write_message(g_comm_handle, g_hostname, COMM_CLIENT, 1, 
                  (unsigned char*)" ", 1, UNREGISTER_RESPONSE_CTRL_MSG, &err_msg);

               DPRINTF(("commlib_to_tty: received exit_status from shepherd: %d\n", 
                        g_exit_status));
               do_exit = 1;
               break;
         }
      }
      comm_free_message(&recv_mess, &err_msg);
   }

   thread_func_cleanup(t_conf);
   DPRINTF(("commlib_to_tty: exiting commlib_to_tty thread!\n"));
   sge_dstring_free(&err_msg);
   DEXIT;
   return NULL;
}

/****** do_server_loop() *******************************************************
*  NAME
*     do_server_loop() -- The servers main loop
*
*  SYNOPSIS
*     int do_server_loop(u_long32 job_id, int nostdin, int noshell,
*                        int is_rsh, int is_qlogin, int force_pty,
*                        int *p_exit_status)
*
*  FUNCTION
*     The main loop of the commlib server, handling the data transfer from
*     and to the client.
*
*  INPUTS
*     u_long32 job_id    - SGE job id of this job
*     int nostdin        - The "-nostdin" switch
*     int noshell        - The "-noshell" switch
*     int is_rsh         - Is it a qrsh with commandline?
*     int is_qlogin      - Is it a qlogin or qrsh without commandline?
*     int force_pty      - The user forced use of pty by the "-pty yes" switch
*     int *p_exit_status -
*
*  RESULT
*     int - 0: Ok.
*           1: An error occured.
*
*  NOTES
*     MT-NOTE: do_server_loop is not MT-safe
*
*  SEE ALSO
*     do_client_loop()
*******************************************************************************/
int do_server_loop(u_long32 job_id, int nostdin, int noshell,
                   int is_rsh, int is_qlogin, int force_pty,
                   int *p_exit_status)
{
   bool              terminal_is_in_raw_mode = false;
   int               ret = 0;
   dstring           err_msg = DSTRING_INIT;
   THREAD_HANDLE     *pthread_tty_to_commlib = NULL;
   THREAD_HANDLE     *pthread_commlib_to_tty = NULL;
   THREAD_LIB_HANDLE *thread_lib_handle = NULL;
   cl_raw_list_t     *cl_com_log_list = NULL;

   DENTER(TOP_LAYER, "do_server_loop");

   if (p_exit_status == NULL) {
      return 1;
   }

   cl_com_log_list = cl_com_get_log_list();

   g_nostdin = nostdin;
   g_noshell = noshell;

   /*
    * qrsh without command and qlogin both have is_rsh == 0 and is_qlogin == 1
    * qrsh with command and qsh don't need to set terminal mode.
    * If the user requested a pty we also have to set terminal mode.
    */
   if ((force_pty == 2 && is_rsh == 0 && is_qlogin == 1) || force_pty == 1) {
      /*
       * Set this terminal to raw mode, just output everything, don't interpreti
       * it. Let the pty on the client side interpret the characters.
       */
      ret = terminal_enter_raw_mode();
      if (ret != 0) {
         fprintf(stderr, "Can't set terminal mode. Error reason: %s (%d)\n",
            strerror(ret), ret);
         return 1;
      } else {
         terminal_is_in_raw_mode = true;
      }
   }

   /*
    * Setup thread list and create two worker threads
    */
   DPRINTF(("creating worker thread\n"));
   thread_init_lib(&thread_lib_handle);
   ret = create_thread(thread_lib_handle, &pthread_tty_to_commlib, cl_com_log_list,
      "tty_to_commlib thread", 1, tty_to_commlib);
   if (ret != CL_RETVAL_OK) {
      DPRINTF(("can't create tty_to_commlib thread, return value = %d\n",
         ret));
      goto cleanup;
   }

   ret = create_thread(thread_lib_handle, &pthread_commlib_to_tty, cl_com_log_list,
      "commlib_to_tty thread", 1, commlib_to_tty);
   if (ret != CL_RETVAL_OK) {
      DPRINTF(("can't create commlib_to_tty thread, return value = %d\n",
         ret));
      goto cleanup;
   }

   /*
    * From here on, the two worker threads are doing all the work.
    * This main thread is just waiting until the client closes the 
    * connection to us, which causes the commlib_to_tty thread to 
    * exit. Then it closes the tty_to_commlib thread, too, and 
    * cleans up everything.
    */
   DPRINTF(("waiting for end of commlib_to_tty thread\n"));
   thread_join(pthread_commlib_to_tty);

   DPRINTF(("shutting down tty_to_commlib thread\n"));
   thread_shutdown(pthread_tty_to_commlib);
/*
 * TODO: Why doesn't this work on some hosts (e.g. oin)
 *       Is it necessary at all?

   DPRINTF(("wait until there is no client connected any more\n"));
   comm_wait_for_no_connection(g_comm_handle, COMM_CLIENT, 10, &err_msg);
*/
   cl_com_set_error_func(NULL);
   cl_com_ignore_timeouts(CL_TRUE);
   DPRINTF(("shut down the connection from our side\n"));
   ret = cl_commlib_shutdown_handle(g_comm_handle, CL_FALSE);
   g_comm_handle = NULL;
   /*
    * Close stdin to awake the tty_to_commlib-thread from the select() call.
    * thread_shutdown() doesn't work on all architectures.
    */
   close(STDIN_FILENO);

   DPRINTF(("waiting for end of tty_to_commlib thread\n"));
   thread_join(pthread_tty_to_commlib);
cleanup:
   /*
    * Set our terminal back to 'unraw' mode. Should be done automatically
    * by OS on process end, but we want to be sure.
    */
   sge_dstring_free(&err_msg);
   if (terminal_is_in_raw_mode == true) {
      ret = terminal_leave_raw_mode();
      DPRINTF(("terminal_leave_raw_mode() returned %d (%s)\n", ret, strerror(ret)));
      terminal_is_in_raw_mode = false;
   }

   *p_exit_status = g_exit_status;

   thread_cleanup_lib(&thread_lib_handle);
   DRETURN(0);
}

