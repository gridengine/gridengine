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

#include "sge_gdi_intern.h"
#include "commlib.h"
#include "sge_prognames.h"
#include "sge_me.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_exit.h"
#include "utility.h"
#include "qm_name.h"
#include "pack.h"
#include "sge_feature.h"
#include "msg_utilib.h"
#include "msg_gdilib.h"
#include "sge_security.h"

static void sge_log_commd_state_transition(int cl_err);
enum { COMMD_UNKNOWN = 0, COMMD_UP, COMMD_DOWN};
static int commd_monitor(int cl_err);

/*-----------------------------------------------------------------------
 * prepare_enroll
 * just store values for later enroll() of commlib
 *-----------------------------------------------------------------------*/
void prepare_enroll(const char *name, u_short id, int *tag_priority_list)
{
   DENTER(BASIS_LAYER, "prepare_enroll");

   /*
   ** initialize security context
   */
   if (sge_security_initialize(name)) {
      CRITICAL((SGE_EVENT, MSG_GDI_INITSECURITYDATAFAILED));
      SGE_EXIT(1);
   }
   
   set_commlib_param(CL_P_COMMDSERVICE, 0, SGE_COMMD_SERVICE, NULL);
   set_commlib_param(CL_P_NAME, 0, name, NULL);
   set_commlib_param(CL_P_ID, id, NULL, NULL);
   set_commlib_param(CL_P_PRIO_LIST, 0, NULL, tag_priority_list);

   if (!(me.who == QMASTER || me.who == EXECD || me.who == SCHEDD || me.who == COMMDCNTL)) {
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

/*----------------------------------------------------------
 * sge_log_commd_state_transition
 *----------------------------------------------------------*/
static void sge_log_commd_state_transition(
int cl_err 
) {
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

/*----------------------------------------------------------
 *  commd_monitor()
 *  returns 
 *     0          commd did not change his state
 *     COMMD_UP   commd gone up
 *     COMMD_DOWN commd gone down
 *----------------------------------------------------------*/
static int commd_monitor(
int cl_err 
) {
   static int state = COMMD_UNKNOWN;

   /* initial setup of state - only down commd is reported */
   if (state==COMMD_UNKNOWN && cl_err==CL_CONNECT)
         return state = COMMD_DOWN;

   /* commd is now down but was up before */
   if (cl_err==CL_CONNECT && state == COMMD_UP) 
      return state = COMMD_DOWN;

   /* commd is now up but was down before */
   if (cl_err!=CL_CONNECT && state == COMMD_DOWN) 
      return state = COMMD_UP;

   return 0;
}

/*---------------------------------------------------------
 *  sge_send_any_request
 *  returns 0 if ok
 *          -4 if peer is not alive or rhost == NULL
 *          return value of gdi_send_message() for other errors
 *---------------------------------------------------------*/
int sge_send_any_request(
int synchron,
u_long32 *mid,
const char *rhost,
const char *commproc,
int id,
sge_pack_buffer *pb,
int tag 
) {
   static int schedd_first = 1, execd_first = 1, first_time = 1;
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
   if (first_time && synchron) {
      if (check_isalive(rhost)) {
         i = -4;
      }
      first_time = 0;
   }

   if (i == 0) {
      if (me.who==SCHEDD && schedd_first) {
         remove_pending_messages(NULL, 0, 0, 0);
         schedd_first = 0;
         /* commlib call to mark all commprocs as unknown */
         reset_last_heard();
      }
      else if (me.who == EXECD && execd_first) {
         remove_pending_messages(NULL, 0, 0, 0);
         /* commlib call to mark all commprocs as unknown */
         reset_last_heard();

         execd_first = 0;
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

/*----------------------------------------------------------
 * sge_get_any_request
 *
 * returns 0               on success
 *         -1              rhost is NULL 
 *         commlib return values (always positive)
 *----------------------------------------------------------*/
int sge_get_any_request(
char *rhost,
char *commproc,
u_short *id,
sge_pack_buffer *pb,
int *tag,
int synchron 
) {
   int dummytag=0;
   char *buffer;
   u_long32 buflen;
   int i;
   ushort usid=0;
   char host[MAXHOSTLEN+1];
   u_short compressed;

   DENTER(GDI_LAYER, "sge_get_any_request");

   if (id)
      usid = (ushort)*id;

   if (!rhost) {
      ERROR((SGE_EVENT, MSG_GDI_RHOSTISNULLFORGETANYREQUEST ));
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
      DEXIT;
      return i;
   }

   if (id)
      *id = usid;

   /* fill it in the packing buffer */
   init_packbuffer_from_buffer(pb, buffer, buflen, compressed);

   if (rhost[0] == '\0') {    /* If we receive from anybody return the sender */
      strcpy(rhost, host);
   }

   DEXIT;
   return 0;
}


/**********************************************************************
  send a message giving a packbuffer

  same as gdi_send_message, but this is delivered a sge_pack_buffer.
  this function flushes the z_stream_buffer if compression is turned on
  and passes the result on to send_message
  Always use this function instead of gdi_send_message directly, even
  if compression is turned off.
**********************************************************************/
int gdi_send_message_pb(
int synchron,
const char *tocomproc,
int toid,
const char *tohost,
int tag,
sge_pack_buffer *pb,
u_long32 *mid 
) {
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
