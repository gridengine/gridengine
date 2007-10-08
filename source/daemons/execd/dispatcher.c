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
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "basis_types.h"
#include "commlib.h"
#include "dispatcher.h"
#include "sgermon.h"
#include "sge_log.h"
#include "msg_execd.h"
#include "sge_string.h"
#include "sge_hostname.h"
#include "sge_security.h"
#include "sig_handlers.h"
#include "sge_profiling.h"
#include "sge_time.h"
#include "qm_name.h"
#include "execd.h"
#include "uti/sge_monitor.h"
#include "sge_bootstrap.h"
#include "sge_prog.h"
#include "sgeobj/sge_ack.h"
#include "gdi/sge_gdi2.h"
#include "execd_job_exec.h"
#include "execd_ticket.h"
#include "job_report_execd.h"
#include "execd_signal_queue.h"
#include "execd_kill_execd.h"
#include "execd_get_new_conf.h"
#include "execd_ck_to_do.h"

int sge_execd_process_messages(sge_gdi_ctx_class_t *ctx, char* err_str, void (*errfunc)(const char *))
{
   monitoring_t monitor;
   bool terminate = false;
   bool do_re_register = false;
   int ret = CL_RETVAL_OK;

   DENTER(TOP_LAYER, "sge_execd_process_messages");

   sge_monitor_init(&monitor, "dispatcher", NONE_EXT, EXECD_WARNING, EXECD_ERROR);

   while (!terminate) {
      struct_msg_t msg;
      char* buffer     = NULL;
      u_long32 buflen  = 0;
      sge_monitor_output(&monitor);

      memset((void*)&msg, 0, sizeof(struct_msg_t));


      cl_commlib_trigger(ctx->get_com_handle(ctx), 1);
      ret = gdi2_receive_message(ctx, msg.snd_name, &msg.snd_id, msg.snd_host, 
                              &msg.tag, &buffer, &buflen, 0);
      init_packbuffer_from_buffer(&msg.buf, buffer, buflen);

      if (ret == CL_RETVAL_OK) {
         bool is_apb_used = false;
         sge_pack_buffer apb;
         int atag = 0;

         switch (msg.tag)
         {
            case TAG_JOB_EXECUTION:
               if (init_packbuffer(&apb, 1024, 0) == PACK_SUCCESS) {
                  do_job_exec(ctx, &msg, &apb);
                  is_apb_used = true;
                  atag = msg.tag;
               }
               break;
            case TAG_SLAVE_ALLOW:
               do_job_slave(ctx, &msg);
               break;
            case TAG_CHANGE_TICKET:
               do_ticket(ctx, &msg);
               break;
            case TAG_ACK_REQUEST:
               do_ack(ctx, &msg);
               break;
            case TAG_SIGQUEUE:
            case TAG_SIGJOB:
               if (init_packbuffer(&apb, 1024, 0) == PACK_SUCCESS) {
                  do_signal_queue(ctx, &msg, &apb);
                  is_apb_used = true;
                  atag = TAG_ACK_REQUEST;
               }
               break;
            case TAG_KILL_EXECD:
               do_kill_execd(ctx, &msg);
               break;
            case TAG_GET_NEW_CONF:
               do_get_new_conf(ctx, &msg);
               break;
            default:
               DPRINTF(("***** UNKNOWN TAG TYPE %d\n", msg.tag));
               break;
         }

         clear_packbuffer(&(msg.buf));
         if (is_apb_used) {
            if (pb_filled(&apb)) {
               u_long32 dummyid = 0;
               gdi2_send_message_pb(ctx, 0, msg.snd_name, msg.snd_id, msg.snd_host, 
                                atag, &apb, &dummyid);
            }
            clear_packbuffer(&apb);
         }
      } else if ((ret != CL_RETVAL_NO_MESSAGE) && (ret != CL_RETVAL_SYNC_RECEIVE_TIMEOUT)) {
         switch (ret) {
            case CL_RETVAL_CONNECTION_NOT_FOUND:  /* we lost connection to qmaster */
            case CL_RETVAL_CONNECT_ERROR:         /* or we can't connect */
              do_re_register = true;
              break;
            default:
               do_re_register = true; /* unexpected error, do reregister */
               if (cl_com_get_handle("execd", 1) == NULL) {
                  terminate = true; /* if we don't have a handle, we must leave
                                     * because execd_register will create a new one.
                                     * This error would be realy strange, because
                                     * if this happens the local socket was destroyed.
                                     */
               }
         }
         if (sge_get_com_error_flag(EXECD, SGE_COM_ACCESS_DENIED) ||
             sge_get_com_error_flag(EXECD, SGE_COM_ENDPOINT_NOT_UNIQUE)) {
            terminate = true; /* leave dispatcher */
         } else if (sge_get_com_error_flag(EXECD, SGE_COM_WAS_COMMUNICATION_ERROR) == true) {
            do_re_register = true;
         }

         /* 
          * trigger re-read of act_qmaster_file in case of
          * do_re_register == true
          */
         if (!terminate && do_re_register) {
            u_long32 now = sge_get_gmt();
            static u_long32 last_qmaster_file_read = 0;
            
            if (now - last_qmaster_file_read >= 30) {
               /* re-read act qmaster file (max. every 30 seconds) */
               DPRINTF(("re-read actual qmaster file\n"));
               ctx->get_master(ctx, true);
               last_qmaster_file_read = now;

               /* re-register at qmaster when connection is up again */
               if (sge_execd_register_at_qmaster(ctx, true) == 0) {
                  do_re_register = false;
               }
            }
         }
      }

      /* do cyclic stuff */
      if (do_ck_to_do(ctx) == 1) {
         terminate = true;
         ret = CL_RETVAL_OK;
      }
   }
   sge_monitor_free(&monitor);

   DRETURN(ret);
}
