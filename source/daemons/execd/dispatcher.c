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

#ifdef SOLARISAMD64
#  include <sys/stream.h>
#endif  

#include "basis_types.h"
#include "commlib.h"
#include "dispatcher.h"
#include "sge_any_request.h"
#include "sgermon.h"
#include "sge_log.h"
#include "msg_execd.h"
#include "sge_string.h"
#include "sge_hostname.h"
#include "sge_security.h"
#include "sig_handlers.h"
#include "sge_profiling.h"
#include "sge_time.h"

/* number of messages to cache in server process
   the rest stays in commd */
#define RECEIVE_CACHESIZE 1

/* range for sleep if we cant contact commd */
#define CONNECT_PROBLEM_SLEEP_MIN 10
#define CONNECT_PROBLEM_SLEEP_MAX 120
#define CONNECT_PROBLEM_SLEEP_INC 10


/* Cache we need for buffering inbound messages */
typedef struct pbcache {
   sge_pack_buffer *pb;  /* data */
   dispatch_entry *de;   /* sender */
   struct pbcache *next;
} pbcache;


static int match_dpe(dispatch_entry *dea, dispatch_entry *deb);
static int receive_message_cach_n_ack(dispatch_entry *de, sge_pack_buffer **pb, 
                               int *tagarray, int cachesize, 
                               void (*errfunc)(const char *));
static int sendAckPb(sge_pack_buffer *apb, char *host, char *commproc, u_short id,
                   void (*errfunc)(const char *err_str));
static int isIn(int tag, int *tagarray);
static int deleteCacheTags(pbcache **lastBeforeThisCall, int *tagarray); 
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
    ->wait4commd =1 -> wait until commd can be contacted
                 =0 -> return immediately if no connect can be done

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
int dispatch( dispatch_entry*   table,
              int               tabsize, 
              int*              tagarray, 
              u_long            rcvtimeout,
              char*             err_str, 
              void              (*errfunc)(const char *), 
              int               wait4commd)
{
   /* CR - TODO: rework complete dispatch function(s) for NGC */
   dispatch_entry de,   /* filled with receive mask */
                  *te;
   int i, j, terminate, errorcode, ntab;
   u_long rcvtimeoutt=rcvtimeout;
   u_long32 dummyid = 0;
   sge_pack_buffer *pb = NULL, apb;
   int synchron;
   time_t next_prof_output = 0;

   DENTER(TOP_LAYER, "dispatch");

   if (tabsize<=0) {
      strcpy(err_str,MSG_COM_INTERNALDISPATCHCALLWITHOUTDISPATCH);
      DEXIT;
      return -1;
   }

   alloc_de(&de);       /* malloc fields in de */

   terminate = 0;
   errorcode = CL_RETVAL_OK;

   while (!terminate) {

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


      /*  trigger communication
       *  =====================
       *  -> this will block 1 second , when there are no messages to read/write 
       */

      i = receive_message_cach_n_ack(&de, &pb, tagarray, RECEIVE_CACHESIZE, errfunc); 

      DPRINTF(("receive_message_cach_n_ack() returns: %s (%s/%s/%d)\n", 
               cl_get_error_text(i), de.host, de.commproc, de.id)); 

      if (i != CL_RETVAL_OK) {
         cl_commlib_trigger(cl_com_get_handle( "execd" ,1));
      }
      sge_update_thread_alive_time(SGE_EXECD_MAIN);

      switch (i) {
      case CL_RETVAL_CONNECTION_NOT_FOUND:  /* is renewed */
        de.tag = -1;  
        /* no break; */
      case CL_RETVAL_NO_MESSAGE:
      case CL_RETVAL_SYNC_RECEIVE_TIMEOUT:
      case CL_RETVAL_OK:

         /* look for dispatch entries matching the inbound message or
            entries activated at idle times */
         for (ntab=0; ntab<tabsize; ntab++) {
            if (match_dpe(&de, &table[ntab])) {
               sigset_t old_sigset, sigset;

               if(init_packbuffer(&apb, 1024, 0) != PACK_SUCCESS) {
                  free_de(&de);
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

               j = table[ntab].func(&de, pb, &apb, &rcvtimeoutt, &synchron, 
                                    err_str, 0);

               sigprocmask(SIG_SETMASK, &old_sigset, NULL);

               rcvtimeoutt = MIN(rcvtimeout, rcvtimeoutt);
               
               /* if apb is filled send it back to the requestor */
               if (pb_filled(&apb)) {              
                  i = gdi_send_message_pb(synchron, de.commproc, de.id, de.host, 
                                   de.tag, &apb, &dummyid);
                  if (i != CL_RETVAL_OK) {
                     DPRINTF(("gdi_send_message_pb() returns: %s (%s/%s/%d)\n", 
                               cl_get_error_text(i), de.host, de.commproc, de.id));
                  }
               }
               clear_packbuffer(&apb);

               switch (j) {
               case -1:
                  terminate = 1;
                  errorcode = CL_RETVAL_UNKNOWN;
                  break;
               case 1:
                  terminate = 1;
                  errorcode = CL_RETVAL_OK;
                  break;
               }
            }
         }
         clear_packbuffer(pb);
         if (pb) 
            free(pb); /* allocated in receive_message_cach_n_ack() */

         break;
      default:
         sprintf(err_str, MSG_COM_NORCVMSG_S, cl_get_error_text(i));
         free_de(&de);
         PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM2);
         DEXIT;
         return i;
      }

      DPRINTF(("====================[ DISPATCH EPOCH ]===========================\n"));

      PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM2);

      if (prof_is_active(SGE_PROF_ALL) || terminate) {
        time_t now = sge_get_gmt();

         if (now > next_prof_output) {
            prof_output_info(SGE_PROF_ALL, false, "profiling summary:\n");
            prof_reset(SGE_PROF_ALL,NULL);
            next_prof_output = now + 60;
         }
      }
   }

   free_de(&de);
   DEXIT;
   return errorcode;
}

/****************************************************/
/* match 2 dispatchtable entries against each other */
/****************************************************/
static int match_dpe(dea, deb)
dispatch_entry *dea, *deb;
{
   if (deb->tag == -1) /* cyclic entries allways match */
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


/*****************************************************************************
 Wrap around receive_message. Get all available messages and
 send the first two u_long32s in the received buffer preceeded by the tag of the 
 inbound message back to the sender. 
 The caller can decide what TAGS are acknowledged this way by acktags which is
 a tagarray terminated by TAG_NONE.

 If we get an error during receive or during sending acknowledges, we do not 
 return the error if we have cached pbs. Instead we return an chached element.

 So we can't indicate any lost acknowledge and therefore any lost message.

 If we cant send the acknowledges we have to delete the whole messages, in
 order to stay consistent with the sender.
 *****************************************************************************/
static int receive_message_cach_n_ack( dispatch_entry*    de,
                                       sge_pack_buffer**  pb,
                                       int*               tagarray,
                                       int                cachesize,
                                       void               (*errfunc)(const char *)) {
   static int cached_pbs = 0;      /* number of cached pbs */
   static pbcache *cache=NULL, *cacheend=NULL;  /* ptr to first and last cached element */
   pbcache *new, *lastBeforeThisCall=cacheend, *cacheptr, *cacheptrlast;

   char *buffer;
   u_long32 buflen;
   sge_pack_buffer apb;    /* for sending acknowledge answers back to sender */
   dispatch_entry deact, lastde;
   int i, receive_blocking;
   u_long32 tmpul, tmpul2;
   u_short compressed;

   DENTER(TOP_LAYER, "receive_message_cach_n_ack");

/*    DPRINTF(("----------------- message cache holds %d entries\n", cached_pbs)); */

   apb.head_ptr = NULL;        /* mark uninitialized */
   alloc_de(&deact);
   alloc_de(&lastde);

   /* We do a blocking wait if there is no message in our cache we could 
      deliver to the caller. Else we get all we can get and then return
      what we already have. ++ TODO use this pointers later too */
   cacheptr = cache;
   receive_blocking = 0;

   while (cacheptr) {
      if (match_dpe(cacheptr->de, de)) { 
         receive_blocking = 0;
         DPRINTF(("there is already a message to deliver in the cache (before communication)\n"));
         break;
      }
      cacheptr = cacheptr->next;
   }



   /* Read what we can get. If we have nothing to process we wait synchron
      for the next message to arrive. */
   i = CL_RETVAL_OK;
   while (cached_pbs < cachesize && i == CL_RETVAL_OK) {
      copy_de(&deact, de);


      i = gdi_receive_message(deact.commproc, &deact.id, deact.host, &deact.tag, 
                          &buffer, &buflen, receive_blocking, &compressed);



/*
      receive_blocking = 0;   */  /* second receive is always non blocking */
      if (i == CL_RETVAL_OK) {
         int pack_ret;


         new = (pbcache *)malloc(sizeof(pbcache));        
         new->pb = (sge_pack_buffer *)malloc(sizeof(sge_pack_buffer));
         memset (new->pb, 0, sizeof(sge_pack_buffer));
         new->de = (dispatch_entry *)malloc(sizeof(dispatch_entry));
         memset (new->de, 0, sizeof(dispatch_entry));
         alloc_de(new->de);
         copy_de(new->de, &deact);
         new->next = 0;
         pack_ret = init_packbuffer_from_buffer(new->pb, buffer, buflen, compressed);
         if(pack_ret != PACK_SUCCESS) {
            ERROR((SGE_EVENT, MSG_EXECD_INITPACKBUFFERFAILED_S, cull_pack_strerror(pack_ret)));
            continue;
         }

         if (cache)
            cacheend->next = new;
         else
            cache = new;
         cacheend = new;
         cached_pbs++;

         /* if this is the same receiver as the last one, we could add the
            acknowledge to apb. Else we have to send the acknowledge and
            reinitialize apb */

         if (apb.head_ptr) {  /* only if there is allready an acknowlege */
            if (lastde.id != deact.id || sge_hostcmp(lastde.host, deact.host) ||
                strcmp(lastde.commproc, deact.commproc)) {
               /* this is another sender -> send ack to last sender */
               DPRINTF(("(1) sending acknowledge to (%s,%s,%d)\n",
                        lastde.host, lastde.commproc, lastde.id));
               if ((i = sendAckPb(&apb, lastde.host, lastde.commproc, 
                                  lastde.id, errfunc)) != CL_RETVAL_OK) {
                  /* We cant send acknowledges, so we have to delete all 
                     newly received pbs with an acknowledgable tag */
                  DPRINTF(("can't send acknowledge, removing messages (1)\n"));
                  cached_pbs -= deleteCacheTags(!lastBeforeThisCall? &cache: 
                        &(lastBeforeThisCall->next), tagarray);
               }
               clear_packbuffer(&apb);
               copy_de(&lastde, &deact);
            }
         }
         else {
            copy_de(&lastde, &deact);
         }


         /*  assemble pb for acknowledges */
         if (isIn(deact.tag, tagarray)) {
            if (unpackint(new->pb, &tmpul) == PACK_SUCCESS &&
                unpackint(new->pb, &tmpul2) == PACK_SUCCESS ) {
               if (!apb.head_ptr)
                  if(init_packbuffer(&apb, 1024, 0) != PACK_SUCCESS) {    /* big enough */
                     i = CL_RETVAL_MALLOC;
                  }
               if(i == CL_RETVAL_OK) {   
                  packint(&apb, deact.tag);
                  packint(&apb, tmpul);                  /* ack ulong 1 */
                  packint(&apb, tmpul2);                 /* ack ulong 2 */
               }
            }
         }
      }
   }



   /* write answer to sender */
   if (apb.head_ptr) {  /* only if there is an acknowlege */
      DPRINTF(("(2) sending acknowledge to (%s,%s,%d)\n", lastde.host, lastde.commproc, lastde.id));
      if ((i = sendAckPb(&apb, lastde.host, lastde.commproc, lastde.id,
                         errfunc)) != CL_RETVAL_OK) {
         /* we have to delete all newly received pbs with an 
            acknowledgable tag */
         DPRINTF(("can't send acknowledge, removing messages (2)\n"));
/*          deleteCacheTags(lastBeforeThisCall, tagarray); */
         cached_pbs -= deleteCacheTags(!lastBeforeThisCall? &cache: &(lastBeforeThisCall->next) , tagarray);
      }
      clear_packbuffer(&apb);
   }

   /* search first entry in cache who matches 
      remove element from cache 
      set callers dispatch table entry to indicate the sender of the message
      set the error state to OK (true also if last read gives an error */
   *pb = NULL; /* may be there is none */
   if (cache) {
      cacheptr = cache;
      cacheptrlast = NULL;
      while (cacheptr) {
         if (match_dpe(cacheptr->de, de)) { 
            free_de(de);
            memcpy((char *)de, (char *)cache->de, sizeof(dispatch_entry));
            *pb = cacheptr->pb;
            free(cacheptr->de);
            cached_pbs--;
            i = CL_RETVAL_OK;  /* if we have a pb for delivery we never indicate an 
                           error */

            /* dechain cache element */
            if (cacheptrlast) {
               cacheptrlast->next = cacheptr->next;
               if (!cacheptr->next)
                  cacheend = cacheptrlast;
            }
            else {
               cache = cacheptr->next;
               if (!cacheptr->next)
                  cacheend = NULL;
            }

            free(cacheptr);
            break;      /* found -> leave loop */
         }
         cacheptrlast = cacheptr;
         cacheptr = cacheptr->next;
      }
   }
   free_de(&deact);
   free_de(&lastde);



   DEXIT;
   return i;
}


/**********************************************************
 send acknowledge packet to requestor
 **********************************************************/
static int sendAckPb(sge_pack_buffer *apb, char *host, char *commproc, u_short id,
              void (*errfunc)(const char *err_str))
{
   int i;
   u_long32 dummy;
   char err_str[256];

   DENTER(TOP_LAYER, "sendAckPb");

   i = gdi_send_message_pb(0, commproc, id, host, TAG_ACK_REQUEST, apb, &dummy);

   if (i != CL_RETVAL_OK) {
      sprintf(err_str, MSG_COM_NOACK_S,cl_get_error_text(i));
      (*errfunc)(err_str);
   }

   DEXIT;
   return i;
}

/***********************************************************
 look for tag in tagarray
 ***********************************************************/
static int isIn(
int tag, 
int *tagarray 
) {
   DENTER(CULL_LAYER, "isIn");

   while (*tagarray != TAG_NONE) {
      if (*tagarray == tag) {
         DEXIT;
         return 1;
      }
      tagarray++;
   }

   DEXIT;
   return 0;
}

/*********************************************************
 remove all pbs from the cache list which are
 - behind lastBeforeThisCall
 - are present in tagarray
 *********************************************************/
static int deleteCacheTags(
pbcache **lpp,
int *tagarray 
) {
   int n = 0;
   pbcache *to_del;

   DENTER(CULL_LAYER, "deleteCacheTags");

   while (*lpp) {
      if (isIn((*lpp)->de->tag, tagarray)) {
         to_del = *lpp;
         *lpp = (*lpp)->next; /* unchain to_del */
         
         clear_packbuffer(to_del->pb);
         free(to_del->pb);
         free_de(to_del->de);
         free(to_del->de);
         free(to_del);
         n++;
      } else 
         lpp = &((*lpp)->next); 
   }

   DEXIT;
   return n;
}

/**************************************************/
static int alloc_de(de)       /* malloc fields in de */
dispatch_entry *de;
{
   de->commproc = malloc(MAXHOSTLEN+1);
   de->host = malloc(MAXHOSTLEN+1);

   return 0;
}

/**************************************************/
static void free_de(de)       /* free fields in de */
dispatch_entry *de;
{
   free(de->commproc);
   free(de->host);
}

static int copy_de(dedst, desrc)
dispatch_entry *dedst, *desrc;
{
   dedst->tag = desrc->tag;

   if (desrc->commproc && desrc->commproc[0]) {
      desrc->commproc[MAXHOSTLEN] = '\0';
      strcpy(dedst->commproc, desrc->commproc);
   } else {
      dedst->commproc[0] = '\0';
   }

   if (desrc->host && desrc->host[0]) {
      desrc->host[MAXHOSTLEN] = '\0';
      strcpy(dedst->host, desrc->host);
   } else {
      dedst->host[0] = '\0';
   }

   dedst->id = desrc->id;

   /* no need to copy function ptr here */
   return 0;
}
