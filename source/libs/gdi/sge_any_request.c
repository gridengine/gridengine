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
#include <string.h>

#include "sge_gdiP.h"
#include "sge_any_request.h"
#include "commlib.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_profiling.h"
#include "qm_name.h"
#include "pack.h"
#include "sge_feature.h"
#include "sge_security.h"
#include "sge_unistd.h"

#include "msg_gdilib.h"

static int gdi_log_flush_func(cl_raw_list_t* list_p);
#ifdef ENABLE_NGC
#else
static void sge_log_commd_state_transition(int cl_err);
static int commd_monitor(int cl_err);
#endif

static int gdi_log_flush_func(cl_raw_list_t* list_p) {
   int ret_val;
   cl_log_list_elem_t* elem = NULL;
   DENTER(TOP_LAYER, "gdi_log_flush_func");

   if (list_p == NULL) {
      DEXIT;
      return CL_RETVAL_LOG_NO_LOGLIST;
   }

   if (  ( ret_val = cl_raw_list_lock(list_p)) != CL_RETVAL_OK) {
      DEXIT;
      return ret_val;
   }

   while ( (elem = cl_log_list_get_first_elem(list_p) ) != NULL) {
      char* param;
      char* module;
      if (elem->log_parameter == NULL) {
         param = "";
      } else {
         param = elem->log_parameter;
      }
      if (elem->log_module_name == NULL) {
         module = "";
      } else {
         module = elem->log_module_name;
      }
      /* TODO: all communication errors are only INFO's ???  CR */
      switch(elem->log_type) {
         case CL_LOG_ERROR: 
            INFO((SGE_EVENT,  "%-15s=> %s %s\n", elem->log_thread_name, elem->log_message, param ));
            break;
         case CL_LOG_WARNING:
            INFO((SGE_EVENT,"%-15s=> %s %s\n", elem->log_thread_name, elem->log_message, param ));
            break;
         case CL_LOG_INFO:
            INFO((SGE_EVENT,   "%-15s=> %s %s\n", elem->log_thread_name, elem->log_message, param ));
            break;
         case CL_LOG_DEBUG:
            DEBUG((SGE_EVENT,  "%-15s=> %s %s\n", elem->log_thread_name, elem->log_message, param ));
            break;
      }
      cl_log_list_del_log(list_p);
   }
   
   if (  ( ret_val = cl_raw_list_unlock(list_p)) != CL_RETVAL_OK) {
      DEXIT;
      return ret_val;
   } 
   DEXIT;
   return CL_RETVAL_OK;
}

/*-----------------------------------------------------------------------
 * prepare_enroll
 * just store values for later enroll() of commlib
 *
 * NOTES
 *    MT-NOTE: prepare_enroll() is MT safe
 *-----------------------------------------------------------------------*/
#ifdef ENABLE_NGC
void prepare_enroll(const char *name, u_short id, int *tag_priority_list)
{
   int ret_val;
   char* qmaster_port = NULL;

   DENTER(BASIS_LAYER, "prepare_enroll");

   /* TODO: setup security for NGC */
   if (sge_security_initialize(name)) {
      CRITICAL((SGE_EVENT, MSG_GDI_INITSECURITYDATAFAILED));
      SGE_EXIT(1);
   }
   
   /* TODO: get port via service */
   qmaster_port = getenv("SGE_QMASTER_PORT");
   if (qmaster_port == NULL) {
      ERROR((SGE_EVENT, "could not get environment variable SGE_QMASTER_PORT\n"));
      SGE_EXIT(1);
   }

   /* TODO: call to cleanup commlib */
   INFO((SGE_EVENT,"starting up ngc in NO THREADS mode\n"));
   INFO((SGE_EVENT,"problem for sge_daemonize() when threads are running?"));
   ret_val = cl_com_setup_commlib(CL_NO_THREAD,CL_LOG_DEBUG,gdi_log_flush_func);
   if (ret_val != CL_RETVAL_OK) {
      ERROR((SGE_EVENT, "cl_com_setup_commlib(): %s\n",cl_get_error_text(ret_val)));
   }
   
   /*
   set_commlib_param(CL_P_COMMDSERVICE, 0, SGE_COMMD_SERVICE, NULL);
   set_commlib_param(CL_P_NAME, 0, name, NULL);
   set_commlib_param(CL_P_ID, id, NULL, NULL);
   set_commlib_param(CL_P_PRIO_LIST, 0, NULL, tag_priority_list);
   */

   /* TODO: check this timeout values */
   if (uti_state_get_mewho() == QCONF) {
      INFO((SGE_EVENT, "TODO: set timeouts for syncron rcv/snd of qconf client to 3600 !\n"));
      /*
      set_commlib_param(CL_P_TIMEOUT_SRCV, 3600, NULL, NULL);
      set_commlib_param(CL_P_TIMEOUT_SSND, 3600, NULL, NULL);
      */
   }

   /* TODO: we don't need to get the qmaster name at this point (check this)*/
   if (!(uti_state_get_mewho() == QMASTER || uti_state_get_mewho() == EXECD 
      || uti_state_get_mewho() == SCHEDD || uti_state_get_mewho() == COMMDCNTL)) {
      const char *masterhost; 

      if ((masterhost = sge_get_master(0))) {
      /*
         set_commlib_param(CL_P_COMMDHOST, 0, masterhost, NULL);
      */
      }  
   }
   
   /* TODO: implement reserved port security */
   if (feature_is_enabled(FEATURE_RESERVED_PORT_SECURITY)) {
      CRITICAL((SGE_EVENT, "reserved port security not implemented\n"));
      /*
      set_commlib_param(CL_P_RESERVED_PORT, 1, NULL, NULL);
      */
   }
   DEXIT;
}
#else
void prepare_enroll(const char *name, u_short id, int *tag_priority_list)
{
   DENTER(BASIS_LAYER, "prepare_enroll");

   if (uti_state_get_mewho() != COMMDCNTL) { 
      if (sge_security_initialize(name)) {
         CRITICAL((SGE_EVENT, MSG_GDI_INITSECURITYDATAFAILED));
         SGE_EXIT(1);
      }
   }
   
   set_commlib_param(CL_P_COMMDSERVICE, 0, SGE_COMMD_SERVICE, NULL);
   set_commlib_param(CL_P_NAME, 0, name, NULL);
   set_commlib_param(CL_P_ID, id, NULL, NULL);
   set_commlib_param(CL_P_PRIO_LIST, 0, NULL, tag_priority_list);

   if (uti_state_get_mewho() == QCONF) {
      set_commlib_param(CL_P_TIMEOUT_SRCV, 3600, NULL, NULL);
      set_commlib_param(CL_P_TIMEOUT_SSND, 3600, NULL, NULL);
   }

   if (!(uti_state_get_mewho() == QMASTER || uti_state_get_mewho() == EXECD 
      || uti_state_get_mewho() == SCHEDD || uti_state_get_mewho() == COMMDCNTL)) {
      const char *masterhost; 

      if ((masterhost = sge_get_master(0))) {
         set_commlib_param(CL_P_COMMDHOST, 0, masterhost, NULL);
      }  
   }

   if (feature_is_enabled(FEATURE_RESERVED_PORT_SECURITY)) {
      set_commlib_param(CL_P_RESERVED_PORT, 1, NULL, NULL);
   }

   DEXIT;
}
#endif

/*----------------------------------------------------------
 * sge_log_commd_state_transition
 *
 * NOTES
 *    MT-NOTE: sge_log_commd_state_transition() is MT safe
 *----------------------------------------------------------*/
#ifdef ENABLE_NGC
#else
static void sge_log_commd_state_transition(int cl_err) 
{
   DENTER(BASIS_LAYER, "sge_log_commd_state_transition");

   switch (commd_monitor(cl_err)) {
   case COMMD_UP:
      WARNING((SGE_EVENT, MSG_GDI_COMMDUP ));
      break;
   case COMMD_DOWN:
      ERROR((SGE_EVENT, MSG_GDI_COMMDDOWN_S, cl_errstr(cl_err)));
      break;
   default:
      break;
   }
     
   DEXIT;
   return;
}
#endif

/*----------------------------------------------------------
 *  commd_monitor()
 *  returns 
 *     0          commd did not change his state
 *     COMMD_UP   commd gone up
 *     COMMD_DOWN commd gone down
 *
 * NOTES
 *    MT-NOTE: commd_monitor() is MT safe
 *----------------------------------------------------------*/
#ifdef ENABLE_NGC
#else
static int commd_monitor(int cl_err) 
{
   int state = gdi_state_get_commd_state();

   /* initial setup of state - only down commd is reported */
   if (state==COMMD_UNKNOWN && cl_err==CL_CONNECT) {
         gdi_state_set_commd_state(COMMD_DOWN);
         return COMMD_DOWN;
   }

   /* commd is now down but was up before */
   if (cl_err==CL_CONNECT && state == COMMD_UP)  {
      gdi_state_set_commd_state(COMMD_DOWN);
      return COMMD_DOWN;
   }

   /* commd is now up but was down before */
   if (cl_err!=CL_CONNECT && state == COMMD_DOWN)  {
      gdi_state_set_commd_state(COMMD_UP);
      return COMMD_UP;
   }

   return 0;
}
#endif

/*---------------------------------------------------------
 *  sge_send_any_request
 *  returns 0 if ok
 *          -4 if peer is not alive or rhost == NULL
 *          return value of gdi_send_message() for other errors
 *
 *  NOTES
 *     MT-NOTE: sge_send_gdi_request() is MT safe (assumptions)
 *---------------------------------------------------------*/
#ifdef ENABLE_NGC
int sge_send_any_request(int synchron, u_long32 *mid, const char *rhost, 
                         const char *commproc, int id, sge_pack_buffer *pb, 
                         int tag,u_long32  response_id)
{
   int i;
   u_long32 me_who;
   cl_xml_ack_type_t ack_type;
   cl_com_handle_t* handle = NULL;
   unsigned long dummy_mid = 0;

   DENTER(GDI_LAYER, "sge_send_any_request");

   ack_type = CL_MIH_MAT_NAK;

   if (!rhost) {
      ERROR((SGE_EVENT, MSG_GDI_RHOSTISNULLFORSENDREQUEST ));
      DEXIT;
      return CL_RETVAL_PARAMS;
   }
   


   me_who = uti_state_get_mewho();
   handle = cl_com_get_handle((char*)prognames[me_who] ,0);
   if (handle == NULL) {
      int my_component_id = 0; /* 1 for daemons, 0=automatical for clients */
      if ( uti_state_get_mewho() == QMASTER ||
           uti_state_get_mewho() == EXECD   ||
           uti_state_get_mewho() == SCHEDD  ) {
         my_component_id = 1;   
      }

      handle = cl_com_create_handle(CL_CT_TCP, CL_CM_CT_MESSAGE, 0,0,atoi(getenv("SGE_QMASTER_PORT")), (char*)prognames[uti_state_get_mewho()], my_component_id , 1 , 0 );
      if (handle == NULL) {
         CRITICAL((SGE_EVENT,"can't create handle\n"));
      }
   }
   if (synchron) {
      ack_type = CL_MIH_MAT_ACK;
   }

   INFO((SGE_EVENT,"sending to id: %s,%d, size of message: %ld\n",commproc,id, (unsigned long) pb->bytes_used));

   i = cl_commlib_send_message( handle,(char*)rhost,(char*)commproc , id, ack_type , (cl_byte_t*)pb->head_ptr ,(unsigned long) pb->bytes_used , &dummy_mid ,response_id,tag,1 , synchron);
   
   if (mid) {
      *mid = dummy_mid;
   }
   if (i != CL_RETVAL_OK) {
      ERROR((SGE_EVENT, MSG_GDI_SENDMESSAGETOCOMMPROCFAILED_SSISS ,
                   (synchron ? "" : "a"),
                   commproc, 
                   id, 
                   rhost, 
                   cl_get_error_text(i)));
   }
   DEXIT;
   return i;
}
#else
int sge_send_any_request(int synchron, u_long32 *mid, const char *rhost, 
                         const char *commproc, int id, sge_pack_buffer *pb, 
                         int tag) 
{
   u_long32 dummymid;
   int i;

   DENTER(GDI_LAYER, "sge_send_any_request");

   if (!rhost) {
      ERROR((SGE_EVENT, MSG_GDI_RHOSTISNULLFORSENDREQUEST ));
      DEXIT;
      return -4;
   }
   
   /*
    *  the first time a client calls synchronous the 
    *  qmaster he gets an ask_commproc() for free
    */
   i = 0;
   if (gdi_state_get_first_time() && synchron) {
#ifdef ENABLE_NGC
      if (check_isalive(rhost) != CL_RETVAL_OK) {
         i = -4;
      }
#else
      if (check_isalive(rhost)) {
         i = -4;
      }
#endif
      gdi_state_set_first_time(0);
   }

   if (i == 0) {
      u_long32 me_who = uti_state_get_mewho();
      if ((me_who == SCHEDD || me_who == EXECD) && gdi_state_get_daemon_first()) {
         remove_pending_messages(NULL, 0, 0, 0);
         /* commlib call to mark all commprocs as unknown */
         reset_last_heard();
         gdi_state_set_daemon_first(0);
      }

      i = gdi_send_message_pb(synchron, commproc, id, rhost, tag,
                           pb, mid?mid:&dummymid);
      
      sge_log_commd_state_transition(i);

      if (i) {
         switch (i) {
         case CL_CONNECT: 
         case CL_INTR:
         case CL_WRITE_TIMEOUT:
            /* break; */

         default:
            ERROR((SGE_EVENT, MSG_GDI_SENDMESSAGETOCOMMPROCFAILED_SSISS ,
                   (synchron ? "" : "a"),
                   commproc, 
                   id, 
                   rhost, 
                   cl_errstr(i)));
            break;
         }
      }
   }

   DEXIT;
   return i;
}
#endif


/*----------------------------------------------------------
 * sge_get_any_request
 *
 * returns 0               on success
 *         -1              rhost is NULL 
 *         commlib return values (always positive)
 *
 * NOTES
 *    MT-NOTE: sge_get_any_request() is MT safe (assumptions)
 *----------------------------------------------------------*/
#ifdef ENABLE_NGC
int sge_get_any_request(char *rhost, char *commproc, u_short *id, sge_pack_buffer *pb, int *tag, int synchron, u_long32 for_request_mid, u_long32* mid) 
{
   int i;
   ushort usid=0;
   char host[MAXHOSTLEN+1];
   cl_com_message_t* message = NULL;
   cl_com_endpoint_t* sender = NULL;
   cl_com_handle_t* handle = NULL;


   DENTER(GDI_LAYER, "sge_get_any_request");

   PROF_START_MEASUREMENT(SGE_PROF_GDI);

   if (id)
      usid = (ushort)*id;

   if (!rhost) {
      ERROR((SGE_EVENT, MSG_GDI_RHOSTISNULLFORGETANYREQUEST ));
      PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
      DEXIT;
      return -1;
   }
   
   strcpy(host, rhost);

   handle = cl_com_get_handle((char*)prognames[uti_state_get_mewho()] ,0);
   cl_commlib_trigger(handle);
   i = cl_commlib_receive_message( handle, rhost, commproc, usid, synchron, for_request_mid, &message, &sender);
   if (i != CL_RETVAL_OK) {
      if (i != CL_RETVAL_NO_MESSAGE) {
         /* This if for errors */
         INFO((SGE_EVENT, MSG_GDI_RECEIVEMESSAGEFROMCOMMPROCFAILED_SISS , 
               (commproc[0] ? commproc : "any"), 
               (unsigned int) usid, 
               (host[0] ? host : "any"),
                cl_get_error_text(i)));
      }
      cl_com_free_message(&message);
      cl_com_free_endpoint(&sender);
      /* This is if no message is there */
      PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
      DEXIT;
      return i;
   }    

   /* ok, we received a message */
   if (message != NULL ) {
      if (sender != NULL && id) {
         *id = sender->comp_id;
      }
      if (tag) 
        *tag = message->message_tag;
      if (mid)
        *mid = message->message_id;


      /* fill it in the packing buffer */
      i = init_packbuffer_from_buffer(pb, (char*)message->message, message->message_length , 0);


      /* TODO: the packbuffer must be hold, not deleted !!! */
      message->message = NULL;

      if(i != PACK_SUCCESS) {
         ERROR((SGE_EVENT, MSG_GDI_ERRORUNPACKINGGDIREQUEST_S, cull_pack_strerror(i)));
         PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
         DEXIT;
         return CL_RETVAL_READ_ERROR;
      } 

      if (sender != NULL ) {
         if (rhost[0] == '\0') {
            strcpy(rhost, sender->comp_host); /* If we receive from anybody return the sender */
         }
         if (commproc[0] == '\0') {
            strcpy(commproc , sender->comp_name); /* If we receive from anybody return the sender */
         }
      }

      cl_com_free_endpoint(&sender);
      cl_com_free_message(&message);
   }
   PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
   DEXIT;
   return CL_RETVAL_OK;
}
#else
int sge_get_any_request(char *rhost, char *commproc, u_short *id, 
                        sge_pack_buffer *pb, int *tag, int synchron) 
{
   int dummytag=0;
   char *buffer = NULL;
   u_long32 buflen;
   int i;
   ushort usid=0;
   char host[MAXHOSTLEN+1];
   u_short compressed;

   DENTER(GDI_LAYER, "sge_get_any_request");

   PROF_START_MEASUREMENT(SGE_PROF_GDI);

   if (id)
      usid = (ushort)*id;

   if (!rhost) {
      ERROR((SGE_EVENT, MSG_GDI_RHOSTISNULLFORGETANYREQUEST ));
      PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
      DEXIT;
      return -1;
   }
   
   strcpy(host, rhost);
       
   if (tag) 
      dummytag = *tag;

   i = gdi_receive_message(commproc, &usid, host, &dummytag, &buffer, 
                       &buflen, synchron, &compressed);
   
   if (tag) 
      *tag = dummytag;                    
           
   sge_log_commd_state_transition(i);       

   if (i) {
      switch (i) {
      case COMMD_NACK_NO_MESSAGE:
         DPRINTF(("got no message\n"));
         break;
         
      case COMMD_NACK_TIMEOUT:
         DPRINTF(("receive message timed out\n"));
         break;

      case CL_CONNECT:
         DPRINTF(("commd is down\n"));
         break;

      case CL_INTR:
      case CL_READ:
         DPRINTF(("receive message interrupted by signal\n"));
         /* break; */

      default:
         INFO((SGE_EVENT, 
                MSG_GDI_RECEIVEMESSAGEFROMCOMMPROCFAILED_SISS ,
                (commproc[0] ? commproc : "any"), 
                (unsigned int) usid,
                (host[0] ? host : "any"),
                cl_errstr(i)));
         break;
      }
      PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
      DEXIT;
      return i;
   }

   if (id)
      *id = usid;

   /* fill it in the packing buffer */
   i = init_packbuffer_from_buffer(pb, buffer, buflen, compressed);
   if(i != PACK_SUCCESS) {
      ERROR((SGE_EVENT, MSG_GDI_ERRORUNPACKINGGDIREQUEST_S, cull_pack_strerror(i)));
      PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
      DEXIT;
      return CL_READ;
   }

   if (rhost[0] == '\0') {    /* If we receive from anybody return the sender */
      strcpy(rhost, host);
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
   DEXIT;
   return CL_OK;
}
#endif


/**********************************************************************
  send a message giving a packbuffer

  same as gdi_send_message, but this is delivered a sge_pack_buffer.
  this function flushes the z_stream_buffer if compression is turned on
  and passes the result on to send_message
  Always use this function instead of gdi_send_message directly, even
  if compression is turned off.
  
    NOTES
       MT-NOTE: gdi_send_message_pb() is MT safe (assumptions)
**********************************************************************/
int gdi_send_message_pb(int synchron, const char *tocomproc, int toid, 
                        const char *tohost, int tag, sge_pack_buffer *pb, 
                        u_long32 *mid) 
{
   long ret = 0;

   DENTER(GDI_LAYER, "gdi_send_message_pb");

   if ( !pb ) {
       DPRINTF(("no pointer for sge_pack_buffer\n"));
       ret = gdi_send_message(synchron, tocomproc, toid, tohost, tag, NULL, 0, mid, 0);
       DEXIT;
       return ret;
   }

#ifdef COMMCOMPRESS
   if(pb->mode == 0) {
      if(flush_packbuffer(pb) == PACK_SUCCESS)
         ret = gdi_send_message(synchron, tocomproc, toid, tohost, tag, (char*)pb->head_ptr, pb->cpr.total_out, mid, 1);
      else
         ret = CL_MALLOC;
   }
   else
#endif
      ret = gdi_send_message(synchron, tocomproc, toid, tohost, tag, pb->head_ptr, pb->bytes_used, mid, 0);

   DEXIT;
   return ret;
}

/*-------------------------------------------------------------------------*
 * check_isalive
 *    check if master is registered and alive
 *    calls is_commd_alive() and ask_commproc()
 * returns
 *    0                    if qmaster is enrolled at sge_commd
 *    > 0                  commlib error number (always positve)
 *    CL_FIRST_FREE_EC+1   can't get masterhost
 *    CL_FIRST_FREE_EC+2   can't connect to commd
 *
 *  NOTES
 *     MT-NOTE: check_isalive() is MT safe
 *-------------------------------------------------------------------------*/
#ifdef ENABLE_NGC
int check_isalive(const char *masterhost) 
{
   int alive = CL_RETVAL_OK;
 
   DENTER(TOP_LAYER, "check_isalive");

   /* TODO: check this alive check!! */
   INFO((SGE_EVENT,"TODO: MAKE alive test is this alive check ok?\n"));

   if (!masterhost) {
      DPRINTF(("can't get masterhost\n"));
      DEXIT;
      return CL_RETVAL_UNKNOWN_ENDPOINT;
   }
   
   DEXIT;
   return alive;
}
#else
int check_isalive(const char *masterhost) 
{
   int alive = 0;
 
   DENTER(TOP_LAYER, "check_isalive");
 
   if (!masterhost) {
      DPRINTF(("can't get masterhost\n"));
      DEXIT;
      return CL_FIRST_FREE_EC+1;
   }
 
   /* check if prog is alive */
   if (uti_state_get_mewho() == QMON) {
      if (!is_commd_alive()) {
         DPRINTF(("can't connect to commd\n"));
         DEXIT;
         return CL_FIRST_FREE_EC+2;
      }
   }
 
   alive = ask_commproc(masterhost, prognames[QMASTER], 1);
 
   if (alive) {
      DPRINTF(("no qmaster: ask_commproc(\"%s\", \"%s\", %d): %s\n",
               masterhost, prognames[QMASTER], 1, cl_errstr(alive)));
   }
 
   DEXIT;
   return alive;
}
#endif 
