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

/* range for sleep if we cant contact commd */
#define CONNECT_PROBLEM_SLEEP_MIN 10
#define CONNECT_PROBLEM_SLEEP_MAX 120
#define CONNECT_PROBLEM_SLEEP_INC 10

static int match_dpe(dispatch_entry *dea, dispatch_entry *deb);
static int authorize_dpe(const char* admin_user, dispatch_entry *deb);
static int receive_message_cach_n_ack(sge_gdi_ctx_class_t *ctx,
                                      dispatch_entry *de, 
                                      sge_pack_buffer **pb, 
                                      void (*errfunc)(const char *));
static int sendAckPb(sge_gdi_ctx_class_t *ctx, 
                     sge_pack_buffer *apb, 
                     char *host, 
                     char *commproc, 
                     u_short id,
                     void (*errfunc)(const char *err_str));
static int alloc_de(dispatch_entry *de);
static void free_de(dispatch_entry *de);
static int copy_de(dispatch_entry *dedst, dispatch_entry *desrc);

/*********************************************************
 dispatch loop
    
    ->table is the dipatch table which controls what functions to call
            on arrival of messages
    ->tabsize is the number of entries in this table
    ->tagarray is an array of tags, which should be acknowledged
               by the dispatcher
    ->rcvtimeout is the default timeout of synchronous messagereading
                 messagefunctions can overrule this
    ->err_str is filled when a fatal error happens
    ->errfunc(char *) is called with an error string, if non fatal errors 
                      happen
    returns commlib errorcodes or CL_FIRST_FREE_EC
            in case of CL_FIRST_FREE_EC err_str is filled
    returns 0 on normal termination (triggered by a message function)


    The dispatch table controls modes of operation. The entries in this table
    are specifying a trigger for calling the attached function. They contain
    a function pointer to the attached function.
    Triggers contain message-header data (sender and messagetag) or
    a special trigger for cyclic called functions.
    Wildcards (0 for numbers and NULL for pointers can be used for triggering).

    The dispatcher caches inbound messages in order to allow acknowledges for
    a sender to be packed together. The dispatcher itself does the 
    acknowledgement for messages with tags specified in tagarray.
    The sender will get an message with tag TAG_ACK_REQUEST and filled
    with the tag of the inbound message and the first u_long32 of the inbound
    message. These pairs are packed together in one message if more than one
    messages arrives at the dispatcher.
    Acknowledgement is done synchron, so we block until message arrives, or
    will never arrive.
 *********************************************************/
int dispatch( sge_gdi_ctx_class_t *ctx,
              dispatch_entry*   table,
              int               tabsize, 
              u_long            rcvtimeout,
              char*             err_str, 
              void              (*errfunc)(const char *))
{
   /* CR - TODO: rework complete dispatch function(s) for NGC */
   dispatch_entry de,   /* filled with received mask */
                  *te;
   int i, j, terminate, errorcode, ntab;
   bool do_re_register = false;
   u_long rcvtimeoutt=rcvtimeout;
   u_long32 dummyid = 0;
   sge_pack_buffer *pb = NULL, apb;
   int synchron = 0;
   time_t next_prof_output = 0;
   monitoring_t monitor;
   u_long32 monitor_time = 0; /* will never change in case of an execd, disables the monitoring */
   const char *admin_user = ctx->get_admin_user(ctx);

   DENTER(TOP_LAYER, "dispatch");

   if (tabsize<=0) {
      strcpy(err_str,MSG_COM_INTERNALDISPATCHCALLWITHOUTDISPATCH);
      DEXIT;
      return -1;
   }

   sge_monitor_init(&monitor, "dispatcher", NONE_EXT, EXECD_WARNING, EXECD_ERROR);

   alloc_de(&de);       /* malloc fields in de */

   terminate = 0;
   errorcode = CL_RETVAL_OK;

   while (!terminate) {
      sge_monitor_output(&monitor);
      
      PROF_START_MEASUREMENT(SGE_PROF_CUSTOM2);
      /* Scan table to see what we are waiting for 
         We have to build a receive pattern which matches all entries in the
         dispatch. */
          
      copy_de(&de, table);
      for (i=1; i<tabsize; i++) {
         te = &table[i];
         if (te->tag == -1)
            continue;
         if (de.tag && (!te->tag || ((de.tag != te->tag))))
            de.tag = 0;
         if (de.commproc && (!te->commproc || ((de.commproc != te->commproc))))
            de.commproc[0] = '\0';
         if (de.host && (!te->host || ((de.host != te->host))))
            de.host[0] = '\0';
         if (de.id && (!te->id || ((de.id != te->id))))
            de.id = 0;
      }

      MONITOR_IDLE_TIME((i = receive_message_cach_n_ack(ctx, &de, &pb, errfunc)), (&monitor), monitor_time, false);
      MONITOR_MESSAGES((&monitor));
      
      switch (i) {
      case CL_RETVAL_CONNECTION_NOT_FOUND:  /* we lost connection to qmaster */
      case CL_RETVAL_CONNECT_ERROR:         /* or we can't connect */
        do_re_register = true;
        /* no break; */
      case CL_RETVAL_NO_MESSAGE:
      case CL_RETVAL_SYNC_RECEIVE_TIMEOUT:
      case CL_RETVAL_OK:
         break;
      default:
         do_re_register = true; /* unexpected error, do reregister */
         if (cl_com_get_handle("execd", 1) == NULL) {
            terminate = 1; /* if we don't have a handle, we must leave
                            * because execd_register will create a new one.
                            * This error would be realy strange, because
                            * if this happens the local socket was destroyed.
                            */
         }
      }

      if (sge_get_com_error_flag(EXECD, SGE_COM_ACCESS_DENIED) == true ||
          sge_get_com_error_flag(EXECD, SGE_COM_ENDPOINT_NOT_UNIQUE) == true) {
         terminate = 1; /* leave dispatcher */
      }

      if (sge_get_com_error_flag(EXECD, SGE_COM_WAS_COMMUNICATION_ERROR) == true) {
         do_re_register = true;
      }

      /* 
       * trigger re-read of act_qmaster_file in case of
       * do_re_register == true
       */
      if (do_re_register == true) {
         u_long32 now = sge_get_gmt();
         static u_long32 last_qmaster_file_read = 0;
         
         if ( now - last_qmaster_file_read >= 30 ) {
            /* re-read act qmaster file (max. every 30 seconds) */
            DPRINTF(("re-read actual qmaster file\n"));
            const char *hostname = ctx->get_master(ctx, true);
            last_qmaster_file_read = now;
            if (i != CL_RETVAL_CONNECTION_NOT_FOUND &&
                i != CL_RETVAL_CONNECT_ERROR &&
                hostname != NULL) {
               /* re-register at qmaster when connection is up again */
               if (sge_execd_register_at_qmaster(ctx, true) == 0) {
                  do_re_register = false;
               }
            }
         }
      }

      /* look for dispatch entries matching the inbound message or
         entries activated at idle times */
      for (ntab=0; ntab<tabsize; ntab++) {
         if (match_dpe(&de, &table[ntab]) == 1 && authorize_dpe(admin_user, &de) == 1) {
            sigset_t old_sigset, sigset;

            if(init_packbuffer(&apb, 1024, 0) != PACK_SUCCESS) {
               free_de(&de);
               sge_monitor_free(&monitor);
               PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM2);
               DEXIT;
               return CL_RETVAL_MALLOC;
            }

            /* block these signals in application code */ 
            sigemptyset(&sigset);
            sigaddset(&sigset, SIGINT);
            sigaddset(&sigset, SIGTERM);
            sigaddset(&sigset, SIGCHLD);
#ifdef SIGCLD
            sigaddset(&sigset, SIGCLD);
#endif
            sigprocmask(SIG_BLOCK, &sigset, &old_sigset);

            j = table[ntab].func(ctx, &de, pb, &apb, &rcvtimeoutt, &synchron, 
                                 err_str, 0);

            sigprocmask(SIG_SETMASK, &old_sigset, NULL);

            cl_commlib_trigger(ctx->get_com_handle(ctx), 0);

            rcvtimeoutt = MIN(rcvtimeout, rcvtimeoutt);
            
            /* if apb is filled send it back to the requestor */
            if (pb_filled(&apb)) {              
               i = gdi2_send_message_pb(ctx, synchron, de.commproc, de.id, de.host, 
                                de.tag, &apb, &dummyid);
               MONITOR_MESSAGES_OUT((&monitor));
               if (i != CL_RETVAL_OK) {
                  DPRINTF(("gdi_send_message_pb() returns: %s (%s/%s/%d)\n", 
                            cl_get_error_text(i), de.host, de.commproc, de.id));
               }
            }
            clear_packbuffer(&apb);

            switch (j) {
            case -1:
               do_re_register = true;
               break;
            case 1:
               DPRINTF(("TERMINATE dispatcher because j == 1\n"));
               terminate = 1;
               errorcode = CL_RETVAL_OK;
               break;
            }
         }
      }
      clear_packbuffer(pb);
      FREE(pb); /* allocated in receive_message_cach_n_ack() */

      DPRINTF(("====================[ DISPATCH EPOCH ]===========================\n"));

      PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM2);

      if (prof_is_active(SGE_PROF_ALL) || terminate) {
        time_t now = (time_t)sge_get_gmt();

         if (now > next_prof_output) {
            prof_output_info(SGE_PROF_ALL, false, "profiling summary:\n");
            prof_reset(SGE_PROF_ALL,NULL);
            next_prof_output = now + 60;
         }
      }
   }

   sge_monitor_free(&monitor);
   free_de(&de);
   DEXIT;
   return errorcode;
}

/****************************************************/
/* match 2 dispatchtable entries against each other */
/****************************************************/
static int match_dpe(dispatch_entry* dea, dispatch_entry* deb)
{
   if (deb->tag == -1) /* cyclic entries always match */
      return 1;
   if (deb->tag && deb->tag != dea->tag)
      return 0;
   if (deb->commproc && deb->commproc[0] && 
       strcmp(deb->commproc, dea->commproc))
      return 0;
   if (deb->host && deb->host[0] && sge_hostcmp(deb->host, dea->host))
      return 0;
   if (deb->id && deb->id != dea->id)
      return 0;

   return 1;
}

/****** dispatcher/authorize_dpe() *********************************************
*  NAME
*     authorize_dpe() -- check unique identifier for dispatch entry (only in CSP mode)
*
*  SYNOPSIS
*     static int authorize_dpe(dispatch_entry *deb) 
*
*  FUNCTION
*     This function checks if the request is comming from sge_admin or root user
*     for all execd requests except TAG_JOB_EXECUTION. The TAG_JOB_EXECUTION is 
*     handled in corresponding tag function 
*
*  INPUTS
*     dispatch_entry *deb - contains host, commproc, commid and tag 
*
*  RESULT
*     static int    1 if allowed, 0 denied  
*
*  NOTES
*     MT-NOTE: authorize_dpe() is MT safe 
*
*******************************************************************************/
static int authorize_dpe(const char *admin_user, dispatch_entry *deb) 
{

  DENTER(TOP_LAYER, "authorize_dpe");

  if (deb->tag == -1 || deb->tag == 0) { /* cyclic entries always match */
    DEXIT;
    return 1;
  }

  /* Do the check for all tags except the TAG_JOB_EXECUTION. The JOB_EXECUTION tag does the check
     by it self because it needs to allow inherit jobs */
  if (deb->tag != TAG_JOB_EXECUTION) { 

      if (false == sge_security_verify_unique_identifier(true, admin_user, prognames[EXECD], 1,
                                            deb->host, deb->commproc, deb->id)) {
         DEXIT;
         return 0;
      }
   }

  DEXIT;
  return 1;
}


/*****************************************************************************
 Wrap around receive_message. Get all available messages and
 send the first two u_long32s in the received buffer preceeded by the tag of the 
 inbound message back to the sender. 
 The caller can decide what TAGS are acknowledged this way by acktags which is
 a tagarray terminated by TAG_NONE.

 If we get an error during receive or during sending acknowledges, we do not 
 return the error if we have cached pbs. Instead we return an chached element.

 So we can't indicate any lost acknowledge and therefore any lost message.

 If we can't send the acknowledges we have to delete the whole messages, in
 order to stay consistent with the sender.
 *****************************************************************************/
static int receive_message_cach_n_ack( sge_gdi_ctx_class_t *ctx,
                                       dispatch_entry*    de,
                                       sge_pack_buffer**  pb,


                                       void               (*errfunc)(const char *)) {
   char* buffer     = NULL;
   u_long32 buflen  = 0;
   int i            = 0;


   DENTER(TOP_LAYER, "receive_message_cach_n_ack");

   /* here we receive a message (wait select timeout if there is no message) */
   i = gdi2_receive_message(ctx, de->commproc, &(de->id), de->host, 
                              &(de->tag), &buffer, &buflen, 0);


   DPRINTF(("receiving message returned "SFQ"\n", cl_get_error_text(i)));

   if (i == CL_RETVAL_OK) {
      sge_pack_buffer* mypb = NULL;
      int pack_ret;
/*
      DPRINTF(("host: %s\n", de->host ));
      DPRINTF(("comp: %s\n", de->commproc ));
      DPRINTF(("  id: %d\n", de->id ));
*/

      /* unpack the message */
      mypb = (sge_pack_buffer*) malloc(sizeof(sge_pack_buffer));
      if (mypb == NULL) {
         return CL_RETVAL_MALLOC;
      }

      pack_ret = init_packbuffer_from_buffer(mypb, buffer, buflen);
      if(pack_ret != PACK_SUCCESS) {
         ERROR((SGE_EVENT, MSG_EXECD_INITPACKBUFFERFAILED_S, cull_pack_strerror(pack_ret)));
         return CL_RETVAL_UNKNOWN;
      }
      

      /*  assemble pb for acknowledges */
      switch (de->tag) {
         case TAG_SIGJOB:
         case TAG_SIGQUEUE: {

            sge_pack_buffer apb;
            u_long32 tmpul   = 0;
            u_long32 tmpul2  = 0;

            if (unpackint(mypb, &tmpul) == PACK_SUCCESS &&
                unpackint(mypb, &tmpul2) == PACK_SUCCESS ) {
               if(init_packbuffer(&apb, 1024, 0) == PACK_SUCCESS) {
                  packint(&apb, de->tag);
                  packint(&apb, tmpul);                  /* ack ulong 1 */
                  packint(&apb, tmpul2);                 /* ack ulong 2 */

                  DPRINTF(("(2) sending acknowledge to (%s,%s,%d)\n", de->host, de->commproc, de->id));
                  i = sendAckPb(ctx, &apb, de->host, de->commproc, de->id, errfunc);
                  clear_packbuffer(&apb);
               } else {
                  return CL_RETVAL_MALLOC;
               }
            } else {
               ERROR((SGE_EVENT, "unpacking failed"));
               return CL_RETVAL_UNKNOWN;
            }
            break;
         }
      }


      *pb = mypb;

/*
      DPRINTF(("return de host: %s\n", de->host ));
      DPRINTF(("return de comp: %s\n", de->commproc ));
      DPRINTF(("return de id  : %d\n", de->id ));
*/
   } else {
/*
      DPRINTF(("nothing in commlib, trigger commlib ...\n"));
*/
      cl_commlib_trigger(ctx->get_com_handle(ctx), 1);
   }
   DRETURN(i);
}


/**********************************************************
 send acknowledge packet to requestor
 **********************************************************/
static int sendAckPb(sge_gdi_ctx_class_t *ctx, 
                     sge_pack_buffer *apb, char *host, char *commproc, u_short id,
                     void (*errfunc)(const char *err_str))
{
   int i = CL_RETVAL_OK;
   u_long32 dummy = 0;
   char err_str[256];

   DENTER(TOP_LAYER, "sendAckPb");

   i = gdi2_send_message_pb(ctx, 0, commproc, id, host, TAG_ACK_REQUEST, apb, &dummy);

   if (i != CL_RETVAL_OK) {
      sprintf(err_str, MSG_COM_NOACK_S,cl_get_error_text(i));
      (*errfunc)(err_str);
   }

   DRETURN(i);
}



/**************************************************/
static int alloc_de(de)       /* malloc fields in de */
dispatch_entry *de;
{
   de->id = 0;
   de->commproc = malloc(CL_MAXHOSTLEN+1);
   de->host = malloc(CL_MAXHOSTLEN+1);

   return 0;
}

/**************************************************/
static void free_de(dispatch_entry *de)       /* free fields in de */
{
   free(de->commproc);
   free(de->host);
}

static int copy_de(dispatch_entry *dedst, dispatch_entry* desrc)
{
   dedst->tag = desrc->tag;

   if (desrc->commproc && desrc->commproc[0]) {
      desrc->commproc[CL_MAXHOSTLEN] = '\0';
      strcpy(dedst->commproc, desrc->commproc);
   } else {
      dedst->commproc[0] = '\0';
   }

   if (desrc->host && desrc->host[0]) {
      desrc->host[CL_MAXHOSTLEN] = '\0';
      strcpy(dedst->host, desrc->host);
   } else {
      dedst->host[0] = '\0';
   }

   dedst->id = desrc->id;

   /* no need to copy function ptr here */
   return 0;
}
