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
#include <string.h>
#include <stdlib.h>

#ifndef WIN32NATIVE
#	include <unistd.h>
#endif 

#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>

#ifndef WIN32NATIVE
#	include <sys/socket.h>
#	include <sys/errno.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#include <netinet/tcp.h>
#	include <netdb.h>
#	include <sys/time.h>
#	include <errno.h>
#else 
#	include <winsock2.h>
#	include "win32nativetypes.h"
#endif 


#if defined(AIX32) || defined(AIX41)
#   include <sys/select.h>
#endif

#if defined(SOLARIS)
int gethostname(char *name, int namelen);
#endif

#ifdef WIN32
/*
 * copied from mywinsock.h
 */
#   define WSABASEERR 10000
#   define WSAEINPROGRESS (WSABASEERR+36)
#   define SIGURG 16
#   define SIGIO  23
#   define SIGCLD 20
#   define sigdelset(what,sig) (*(what) &= ~(1<<(sig)))
#endif

#include "commlib.h"
#include "commlib_util.h"
#include "message.h"
#include "commd_io.h"
#include "sge_time.h"
#include "sgermon.h"
#include "sge_crc.h"
#include "msg_commd.h"
#include "msg_common.h"
#include "sge_language.h"

#ifdef COMMLIB_ENABLE_DEBUG
#include "sge_log.h"
#endif

#ifdef QIDL
#include <pthread.h>
static pthread_key_t  commlib_state_key; 
#else
static struct commlib_state_t commlib_state_opaque =
   { 0,                       /* enrolled */
     0,                       /* ever_enrolled */ 
     {0,0,0,0,0,0,0,0,0,0},   /* stored_tag_priority_list */ 
     {'\0'},                  /* componentname */ 
     0,                       /* componentid */ 
     -1,                      /* commdport */ 
     {'\0'},                  /* commdservice */ 
     0,                       /* commdaddr_length */
     {0},                     /* commdaddr */ 
     -1,                      /* sfd */ 
     0,                       /* lastmid */ 
     0,                       /* lastgc */ 
     0,                       /* reserved_port */ 
     {'\0'},                  /* commdhost */
     60,                      /* timeout */ 
     TIMEOUT_SYNC_RCV,        /* timeout_srcv */ 
     TIMEOUT_SYNC_SND,        /* timeout_ssnd */ 
     0,                       /* offline_receive */ 
     5*60,                    /* lt_heard_from_timeout */ 
     0,                       /* closefd */ 
     0,                       /* list */
     NULL,                    /* sge_log */
     0                        /* changed_flag */
   };

static struct commlib_state_t* commlib_state = &commlib_state_opaque;

#endif

#ifdef KERBEROS
#   include "sge_gdi_intern.h"
#endif

#ifndef WIN32 
extern int rresvport(int *port);
#endif
int commlib_debug = 0;              /* no debug by default */

char COMMLIB_BUFFER[2048];

/* for some reason this would compile not on the Cray T90 until I did
   the following. */
#if defined(CRAYTSIEEE) || defined(CRAYTS)
static unsigned int send_message_ ();
#else
static unsigned int send_message_(int synchron, const char *tocomproc, int toid, const char *tohost, int tag, unsigned char *buffer, int buflen, u_long32 *mid, int ask_commproc, u_short compressed);
#endif
static int receive_message_(char *fromcommproc, u_short *fromid, char *fromhost, int *tag, char **buffer, u_long32 *buflen, int synchron, u_short *compressed);
static int enroll_(char *name, u_short *id, int *tag_priority_list);
static int leave_(void);
static int cntl_(u_short cntl_operation, u_long32 *arg, char *carg);

static u_long mid_new(void);
static int reenroll_if_necessary(void);
static int force_reenroll(void);
static int get_environments(void);

#ifndef WIN32NATIVE
static sigset_t build_n_set_mask(void);
#endif

#define RAND_ERROR 0

#if RAND_ERROR
int random_error(int val);
int rand_error = 0;
#endif

/* error strings */
/* 0-...
   local generated errors */

#if defined(SECURE) || defined(KERBEROS)
int lasterr1 = SEC_ANNOUNCE_FAILED;
#else
int lasterr1 = CL_RRESVPORT;
#endif

/* 0xff-...
   these are the errors generated by commd */

int firsterr2 = 0xf5;



/* communication */
int send2commd(unsigned char *buffer, int buflen
#ifdef COMMLIB_ENABLE_DEBUG
               , const char *context_string
#endif
               );

int recvfromcommd(unsigned char **buffer, 
                  unsigned char *header, 
                  int n, u_long32 *flags, 
                  u_short *headerlen, 
                  u_long32 *buflen
#ifdef COMMLIB_ENABLE_DEBUG
               , const char *context_string
#endif
                 );
void closeconnection(int force);


/* error messages (internationalized) for gettext() */
static const char* get_cl_errstr1(int nr);
static const char* get_cl_errstr2(int nr);

static const char* get_cl_errstr2(int nr) {
   switch(nr) 
   {
     case 0 : 
       return MSG_COMMLIB_CL_PERM;                         
     case 1  :
       return MSG_COMMLIB_CL_UNKNOWN_TARGET;
     case 2  :
       return MSG_COMMLIB_CL_UNKNOWN_RECEIVER;
     case 3  :
       return MSG_COMMLIB_NACK_COMMD_NOT_READY;
     case 4  :
       return MSG_COMMLIB_NACK_UNKNOWN_HOST;
     case 5  :
       return MSG_COMMLIB_NACK_NO_MESSAGE;
     case 6  :
       return MSG_COMMLIB_NACK_ENROLL;
     case 7  :
       return MSG_COMMLIB_NACK_OPONCE;
     case 8  :
       return MSG_COMMLIB_NACK_DELIVERY;
     case 9  :
       return MSG_COMMLIB_NACK_TIMEOUT;
     case 10  :
       return MSG_COMMLIB_NACK_CONFLICT;    
   }
   return "";
}

static const char* get_cl_errstr1(int nr) {
   switch(nr)
   {
      case CL_OK:
        return MSG_COMMLIB_CL_OK;           
      case CL_RANGE: 
        return MSG_COMMLIB_CL_RANGE;
      case CL_CREATESOCKET: 
        return MSG_COMMLIB_CL_CREATESOCKET;
      case CL_RESOLVE: 
        return MSG_COMMLIB_CL_RESOLVE;
      case CL_CONNECT: 
        return MSG_COMMLIB_CL_CONNECT;
      case CL_WRITE: 
        return MSG_COMMLIB_CL_WRITE;
      case CL_ALREADYDONE: 
        return MSG_COMMLIB_CL_ALREADYDONE;
      case CL_LOCALHOSTNAME: 
        return MSG_COMMLIB_CL_LOCALHOSTNAME;
      case CL_NOTENROLLED: 
        return MSG_COMMLIB_CL_NOTENROLLED;
      case CL_SERVICE: 
        return MSG_COMMLIB_CL_SERVICE;
      case CL_READ: 
        return MSG_COMMLIB_CL_READ;
      case CL_MALLOC: 
        return MSG_COMMLIB_CL_MALLOC;
      case CL_UNKNOWN_PARAM: 
        return MSG_COMMLIB_CL_UNKNOWN_PARAM;
      case CL_INTR: 
        return MSG_COMMLIB_CL_INTR;
      case CL_READ_TIMEOUT: 
        return MSG_COMMLIB_CL_READ_TIMEOUT;
      case CL_WRITE_TIMEOUT: 
        return MSG_COMMLIB_CL_WRITE_TIMEOUT;
      case CL_CHKSUM: 
        return MSG_COMMLIB_CL_CHKSUM; 
      case CL_RRESVPORT:
        return MSG_COMMLIB_CL_RRESVPORT;

#if defined(SECURE) || defined(KERBEROS)
      case  SEC_SEND_FAILED:
        return MSG_COMMLIB_SEC_SEND;
      case  SEC_RECEIVE_FAILED: 
        return MSG_COMMLIB_SEC_RECEIVE;
      case  SEC_ANNOUNCE_FAILED: 
        return MSG_COMMLIB_SEC_ANNOUNCE;
#endif

   }
   return "";
}

/**********************************************************************
  get an error string
 **********************************************************************/
const char *cl_errstr(int i) {

/* ??   unsigned char n = (unsigned char) i; */
   int n = i;

   DENTER(COMMD_LAYER, "cl_errstr");
   DPRINTF(("%d <= %d\n", n, lasterr1));
   if (n <= lasterr1) {
      DEXIT;
      return (get_cl_errstr1(n)); /* 1cl_errstr1[n]; */
   }
   if (n >= firsterr2 && n <= 0xff) {
      DEXIT;
      return (get_cl_errstr2( (0xff - n) )); /* cl_errstr2[0xff - n]; */
   }

   DEXIT;
   return MSG_COMMLIB_UNKNOWN_ERROR;
}

/**********************************************************************
  set commlib parameters
 **********************************************************************/
int set_commlib_param(
int param,
int intval,
const char *strval,
int *intval_array 
) {
   DENTER(COMMD_LAYER, "set_commlib_param");

   switch (param) {
   case CL_P_NAME:
      if (!strval) {
         return CL_RANGE;
      }
      set_commlib_state_componentname(strval);
      break;
   case CL_P_ID:
      set_commlib_state_componentid(intval); 
      break;
   case CL_P_PRIO_LIST:
      if (!intval_array) {
         return CL_RANGE;
      }  
      set_commlib_state_stored_tag_priority_list(intval_array);
      break;
   case CL_P_RESERVED_PORT:
      DPRINTF(("CL_P_RESERVED_PORT = %d\n", intval));
      set_commlib_state_reserved_port(intval);   /* 0=disable else enable */
      break;

   case CL_P_COMMDHOST:
      if (!strval || secure_strlen(strval, MAXHOSTLEN + 1) > MAXHOSTLEN) {
         return CL_RANGE;
      }
      DPRINTF(("CL_P_COMMDHOST = %s\n", strval));
      set_commlib_state_commdhost(strval);
      break;

   case CL_P_TIMEOUT:
      DPRINTF(("CL_P_TIMEOUT = %d\n", intval));
      set_commlib_state_timeout(intval);
      break;

   case CL_P_TIMEOUT_SRCV:
      DPRINTF(("CL_P_TIMEOUT_SRCV = %d\n", intval));
      set_commlib_state_timeout_srcv(intval);
      break;

   case CL_P_TIMEOUT_SSND:
      DPRINTF(("CL_P_TIMEOUT_SSND = %d\n", intval));
      set_commlib_state_timeout_ssnd(intval);
      break;

   case CL_P_COMMDPORT:
      DPRINTF(("CL_P_COMMDPORT = %d\n", intval));
#ifndef WIN32NATIVE
      set_commlib_state_commdport(htons(intval));
#else 
      set_commlib_state_commdport((u_short)intval);
#endif 
      break;

   case CL_P_OFFLINE_RECEIVE:
      DPRINTF(("CL_P_OFFLINE_RECEIVE = %d\n", intval));
      set_commlib_state_offline_receive(1);
      break;

   case CL_P_LT_HEARD_FROM_TIMEOUT:
      DPRINTF(("CL_P_LT_HEARD_FROM_TIMEOUT = %d\n", intval));
      set_commlib_state_lt_heard_from_timeout(intval);
      break;

   case CL_P_CLOSE_FD:
      DPRINTF(("CL_P_CLOSE_FD = %d\n", intval));
      set_commlib_state_closefd((u_long32)intval);
      break;
   
   case CL_P_COMMDSERVICE:
      DPRINTF(("CL_P_COMMDSERVICE = %s\n", strval));
      set_commlib_state_commdservice(strval);   
      break;
      
   default:
      return CL_UNKNOWN_PARAM;
   }
   return CL_OK;
}

/**********************************************************************
  send a message

   parameters

   synchron  = 1 -> SYNCHRON
   tocomproc = name of commproc we want to send the message e.g. "commd"
   id        = if there are more than one commproc with the same name we need
               an identifier to distinguish between them
   tohost    = host the receiver lives on
   buffer    = this stuff should be sent (in some cases e.g. enroll buffer
               will be overwritten)
   buflen    = length of buffer
   mid       = in case of an asynchron message a message id is returned;
               this message id is unique for the calling commproc another
               commproc may get the same message id; mids can be used by the
               commproc to ensure that the message reached its destination
   compressed= flag if the buffer is compressed. this is sent in the msg header

   environment variables used

   COMMD_HOST    if  present host of communication daemon to contact
                 if !present the local host is used

   COMMD_SERVICE if present used to get commd port; default: "commd"

   globals used

   commdport = port commd is waiting on

   return value

   0  = OK
   !0 = CL_... errorcode

 **********************************************************************/
int send_message(
int synchron,
const char *tocomproc,
int toid,
const char *tohost,
int tag,
char *buffer,
int buflen,
u_long32 *mid,
int compressed 
) {
#ifndef WIN32NATIVE
   sigset_t omask;
#endif
   int i;

#ifndef NO_SGE_COMPILE_DEBUG
   u_long now = 0;
#endif

   DENTER(COMMD_LAYER, "send_message");

#ifndef NO_SGE_COMPILE_DEBUG
#ifndef WIN32NATIVE
   if (rmon_mlgetl(&DEBUG_ON, COMMD_LAYER) & INFOPRINT)
#endif /* WIN32NATIVE */
      now = sge_get_gmt();
#endif

#if RAND_ERROR
      i = random_error(0);
      if (i != 0) {
         DEXIT;   
         return i;
      }   
#endif

   /* we have to block signals to make communication more reliable and to
      ensure all sockets are closed properly */
#ifndef WIN32NATIVE
   omask = build_n_set_mask();
#endif

   i = send_message_(synchron, tocomproc, toid, tohost, tag,
                     (unsigned char *) buffer, buflen, mid, 0, compressed);

#ifndef WIN32NATIVE
   sigprocmask(SIG_SETMASK, &omask, NULL);
#endif 
   
   DPRINTF(("%d = send_message(synchron=%d, tocomproc=%s, toid=%d, tohost=%s, tag=%d, buffer=%p, buflen=%d, mid=%ld) needs %lds\n",
            i, synchron, tocomproc, toid, tohost, tag, buffer, buflen,
            (mid != NULL) ? *mid : 0, sge_get_gmt() - now));

   DEXIT;
   return i;
}

static unsigned int send_message_(
int synchron,
const char *tocomproc,
int toid,
const char *tohost,
int tag,
unsigned char *buffer,
int buflen,
u_long32 *mid,
int ask_commproc,
u_short compressed 
) {
   int old_param_timeout;
   unsigned char *cp;
   unsigned char ackchar = 0, *ackcharptr = &ackchar;
   unsigned int i;
   int headerlen, ret;
   unsigned char prolog_header[PROLOGLEN + HEADERLEN];
   unsigned char *header, *prolog;
   u_long flags = 0;
   u_long newmid;
   int retry;
   const int max_retrys = 3;

   DENTER(COMMD_LAYER, "send_message_");

   prolog = prolog_header;
   header = prolog_header + PROLOGLEN;

   if ((ret = reenroll_if_necessary())) {
      DEXIT;
      return ret;
   }   

   if (secure_strlen(tocomproc, MAXCOMPONENTLEN + 1) > MAXCOMPONENTLEN) {
      DEXIT;
      return CL_RANGE;
   }

   if ((toid < MINID || toid > MAXID) && toid != 0) {
      DEXIT;
      return CL_RANGE;
   }

   if (secure_strlen(tohost, MAXHOSTLEN + 1) > MAXHOSTLEN) {
      DEXIT;
      return CL_RANGE;
   }

#ifndef WIN32                   /* mysterious problem with tag check */
   if (tag <= 0 || tag > MAXTAG) {
      DEXIT;
      return CL_RANGE;
   }
#endif

   /* fill header */
   cp = header;

   if (synchron)
      flags |= COMMD_SYNCHRON;

   if (ask_commproc)
      flags |= COMMD_ASK_COMMPROC;

   newmid = mid_new();
   if (mid)
      *mid = newmid;

   cp = pack_ulong(newmid, cp);
   cp = pack_string(tohost, cp);
   cp = pack_string(tocomproc, cp);
#ifndef WIN32NATIVE
   cp = pack_ushort(toid, cp);
#else /* WIN32NATIVE */
   cp = pack_ushort((u_short)toid, cp);
#endif /* WIN32NATIVE */
   cp = pack_string(get_commlib_state_componentname(), cp);
   cp = pack_ushort(get_commlib_state_componentid(), cp);
#ifndef WIN32NATIVE
   cp = pack_ushort(tag, cp);
#else /* WIN32NATIVE */
   cp = pack_ushort((u_short)tag, cp);
#endif /* WIN32NATIVE */
   cp = pack_ushort(compressed?1:0, cp);

   headerlen = cp - header;

   /* build prolog */
   cp = pack_ulong(flags, prolog);

#ifndef WIN32NATIVE
   cp = pack_ushort(headerlen, cp);
#else /* WIN32NATIVE */
   cp = pack_ushort((u_short)headerlen, cp);
#endif /* WIN32NATIVE */

   cp = pack_ulong(buflen, cp);
   cp = pack_ulong(cksum((char*)prolog, PROLOGLEN-4), cp);

   retry = 0;
   while(1) {

      /* write prolog and header */
      DPRINTF(("send_message: sending message prolog and header\n"));
      i = send2commd(prolog_header, PROLOGLEN+headerlen
#ifdef COMMLIB_ENABLE_DEBUG
                     , "send_message_(#01)"
#endif
                     );
      if (i == COMMD_NACK_ENROLL) {
         if (retry <= max_retrys) {
            force_reenroll();
            retry++;
            DPRINTF(("send_message: sending message prolog and header "
               "%d retry\n", retry));
            continue;
         } else {
            DPRINTF(("send_message: sending message prolog and header "
               "- no retry\n"));
         }
         DEXIT;
         return i;
      } else if (i) {
         DEXIT;
         return i;
      } 

      /* write buffer */
      DPRINTF(("send_message: sending message buffer\n"));
      if (buflen) {
         i = send2commd(buffer, buflen
#ifdef COMMLIB_ENABLE_DEBUG
                        , "send_message_(#02)"
#endif
                        );
         if (i == COMMD_NACK_ENROLL) {
            if (retry <= max_retrys) {
               force_reenroll();
               retry++;
               DPRINTF(("send_message: sending message buffer "
                  "- %d retry\n", retry));
               continue;
            } else {
               DPRINTF(("send_message: sending message buffer "
                  "- no retry\n"));
            }               
            DEXIT;
            return i;
         } else if (i) {
            DEXIT;
            return i;
         }
      }

      /* wait for an acknowledge */
      DPRINTF(("send_message: waiting for acknowledge\n"));

      old_param_timeout = get_commlib_state_timeout();
      if (synchron && buflen)   /* buflen = 0 gets passed by ask_commproc() */
         set_commlib_state_timeout(get_commlib_state_timeout_ssnd());

      i = recvfromcommd((unsigned char **) &ackcharptr, NULL, 1, NULL, NULL, 
                        NULL
#ifdef COMMLIB_ENABLE_DEBUG
                        , "send_message_(#1)"
#endif
                        );
      set_commlib_state_timeout(old_param_timeout);

      DPRINTF(("send_message: acknowledge recvfromcommd returned %d\n", i));
      if ((unsigned int) ackchar == COMMD_NACK_ENROLL) {
         /* This happens, when commd goes down and is now up again. He lost
            the enroll()-information. We have to renew this. */
         closeconnection(1);
         i = force_reenroll();
         if (i) {
            DEXIT;
            return ackchar;
         }
         continue;              /* send again */
      }
      closeconnection(0);
      if (i) {
         DEXIT;
         return i;
      } else {
         DPRINTF(("send_message_ got ackchar %d\n", ackchar));
         if (ackchar == CL_OK && synchron) {
            /* put information to lt_heard_from list */
#ifndef WIN32NATIVE
         set_last_heard_from(tocomproc, toid, tohost, sge_get_gmt());
#else /* WIN32NATIVE */
          set_last_heard_from(tocomproc, (u_short)toid, tohost, sge_get_gmt());
#endif /* WIN32NATIVE */
         }
         DEXIT;
         return ackchar;
      }
   } /* while(1) */
}

/**********************************************************************
  receive a message

  fromcommproc = name of sender (NULL = everybody)
  fromid       = id of sender (0=everybody)
  fromhost     = host of sender (NULL=everyhost)
  buffer       = receivebuffer
  buflen       = length of buffer
  synchron     = if 1 wait for message else return immediately
  compressed   = contains flag if msg is compressed. taken from msg header

  wildcarded arguments (fromcommproc, fromid, fromhost) are filled with
  actual values

  environment variables used

  COMMD_HOST if  present host of communication daemon to contact
             if !present the local host is used

  globals used

  commdport = port commd is waiting on

  return value

  0  = OK
  !0 = CL_... errorcode
 **********************************************************************/
int receive_message(
char *fromcommproc,
u_short *fromid,
char *fromhost,
int *tag,
char **buffer,
u_long32 *buflen,
int synchron,
u_short *compressed 
) {
   int i;

#ifndef NO_SGE_COMPILE_DEBUG
   u_long now = 0;
#endif

   DENTER(COMMD_LAYER, "receive_message");

#ifndef NO_SGE_COMPILE_DEBUG
#ifndef WIN32NATIVE
   /* TODO: implement rmon_mlgetl and insert it here */
   if (rmon_mlgetl(&DEBUG_ON, COMMD_LAYER) & INFOPRINT)
#endif /* WIN32NATIVE */
      now = sge_get_gmt();
#endif


#if RAND_ERROR
      i = random_error(0);
      if (i != 0) {
         DEXIT;
         return i;
      }   
#endif

   i = receive_message_(fromcommproc, fromid, fromhost, tag, buffer, buflen,
                        synchron, compressed);

   DPRINTF(("%d = receive_message(fromcommproc=%s, fromid=%d, fromhost=%s, "
            "tag=%d, buffer=%p, buflen=%ld, synchron=%d) needs %lds\n",
            i, fromcommproc, *fromid, fromhost, *tag, *buffer, *buflen,
            synchron, sge_get_gmt()-now));

   DEXIT;
   return i;
}

static int receive_message_(
char *fromcommproc,
u_short *fromid,
char *fromhost,
int *tag,
char **buffer,
u_long32 *buflen,
int synchron,
u_short *compressed 
) {
   unsigned char *cp;
   unsigned char ackchar, *ackcharptr = &ackchar;
   int i, ret;
   u_short headerlen;
   unsigned char prolog_header[PROLOGLEN + HEADERLEN];
   unsigned char *header, *prolog;
   u_long32 flags = COMMD_RECEIVE;
   u_long32 mid;
   int old_param_timeout;
   ushort ustag ,uscompressed;
   char nfromhost[MAXHOSTLEN], nfromcommproc[MAXCOMPONENTLEN];
   ushort nfromid;
#ifndef WIN32NATIVE
   sigset_t omask;
#endif

   DENTER(COMMD_LAYER, "receive_message_");

   prolog = prolog_header;
   header = prolog_header + PROLOGLEN;

   if ((ret = reenroll_if_necessary())) {
      DEXIT;
      return ret;
   }   

   if (fromcommproc)
      if (secure_strlen(fromcommproc, MAXCOMPONENTLEN) >= MAXCOMPONENTLEN) {
         DEXIT;
         return CL_RANGE;
      }

   if (fromid)
      if ((*fromid < MINID || *fromid > MAXID) && *fromid != 0) {
         DEXIT;
         return CL_RANGE;
      }

   if (fromhost)
      if (secure_strlen(fromhost, MAXHOSTLEN) >= MAXHOSTLEN) {
         DEXIT;
         return CL_RANGE;
      }

   if (*tag < 0 || *tag > MAXTAG) {
      DEXIT;
      return CL_RANGE;
   }

   /* fill header */
   cp = header;

   if (synchron)
      flags |= COMMD_SYNCHRON;

   /* known from enroll */
   cp = pack_string(get_commlib_state_componentname(), cp);
   cp = pack_ushort(get_commlib_state_componentid(), cp);

   if (fromhost)
      cp = pack_string(fromhost, cp);
   else
      cp = pack_string("", cp);

   if (fromcommproc)
      cp = pack_string(fromcommproc, cp);
   else
      cp = pack_string("", cp);

   if (fromid)
      cp = pack_ushort(*fromid, cp);
   else
      cp = pack_ushort(0, cp);

   cp = pack_ushort((ushort) * tag, cp);
   cp = pack_ushort(0, cp);   /* not compressed */

   headerlen = cp - header;

   /* build prolog */
   cp = pack_ulong(flags, prolog);
   cp = pack_ushort(headerlen, cp);
   cp = pack_ulong(0, cp);
   cp = pack_ulong(cksum((char*)prolog, PROLOGLEN-4), cp);

#ifndef WIN32NATIVE
   omask = build_n_set_mask();
#endif

   while (1) {
      /* write prolog and header */
      i = send2commd(prolog_header, PROLOGLEN+headerlen
#ifdef COMMLIB_ENABLE_DEBUG
                     , "receive_message_(#01)"
#endif
                    );
      if (i) {
#ifndef WIN32NATIVE
         sigprocmask(SIG_SETMASK, &omask, NULL);
#endif
         DEXIT;
         return i;
      }

      /* wait for an acknowledge
         If this is a synchron rcv we may hang in read for a long time. So we
         need a long timeout */

      old_param_timeout = get_commlib_state_timeout();
      if (synchron)
         set_commlib_state_timeout(get_commlib_state_timeout_srcv());

      i = recvfromcommd(&ackcharptr, NULL, 1, NULL, NULL, NULL
#ifdef COMMLIB_ENABLE_DEBUG
                        , "receive_message_(#1)"
#endif
                        );

      if (i) {
#ifndef WIN32NATIVE
         sigprocmask(SIG_SETMASK, &omask, NULL);
#endif
         set_commlib_state_timeout(old_param_timeout);
         DEXIT;
         return i;
      }

      set_commlib_state_timeout(old_param_timeout);

      if ((unsigned int) ackchar == CL_UNKNOWN_RECEIVER ||
          (unsigned int) ackchar == COMMD_NACK_ENROLL) {
         /* This happens, when commd goes down and is now up again. He lost
            the enroll()-information. We have to renew this */
         closeconnection(1);
         i = force_reenroll();
         if (i) {
#ifndef WIN32NATIVE
            sigprocmask(SIG_SETMASK, &omask, NULL);
#endif
            DEXIT;
            return ackchar;
         }
         continue;              /* send again */
      }

      if (ackchar) {
         closeconnection(0);
#ifndef WIN32NATIVE
         sigprocmask(SIG_SETMASK, &omask, NULL);
#endif
         DEXIT;
         return ackchar;
      }

      break;
   } /* while (1) */
   
   /* acknowledge says everything is fine -> receive message */

   i = recvfromcommd((unsigned char **) buffer, header, 0, &flags,
                     &headerlen, buflen
#ifdef COMMLIB_ENABLE_DEBUG
                      , "receive_message_(#2)"
#endif
                    );
   if (i) {
#ifndef WIN32NATIVE
      sigprocmask(SIG_SETMASK, &omask, NULL);
#endif
      DEXIT;
      return i;
   }

   /* acknowledge receive - should we use a cookie instaed of a zero byte ? */
   i = send2commd(ackcharptr, 1
#ifdef COMMLIB_ENABLE_DEBUG
                  , "receive_message_(#01)"
#endif
                  );
   closeconnection(0);
#ifndef WIN32NATIVE
   sigprocmask(SIG_SETMASK, &omask, NULL);
#endif

   cp = header;
   cp = unpack_ulong(&mid, cp);
   cp = unpack_string(nfromhost, MAXHOSTLEN, cp);
   cp = unpack_string(nfromcommproc, MAXCOMPONENTLEN, cp);
   cp = unpack_ushort(&nfromid, cp);
   cp = unpack_ushort(&ustag, cp);
   cp = unpack_ushort(&uscompressed, cp);

#ifndef KERBEROS
   if (!*tag)
      *tag = (int) ustag;
#else
   if (!*tag || ustag == TAG_AUTH_FAILURE)      /* push TAG_AUTH_FAILURE */
      *tag = (int) ustag;
#endif

   if (!fromhost[0])
      strcpy(fromhost, nfromhost);

   if (fromcommproc && !fromcommproc[0]) {
      strcpy(fromcommproc, nfromcommproc);
   }

   if (fromid && !*fromid)
      *fromid = nfromid;

   if(compressed)
      *compressed = uscompressed;

   if (i) {
      free(*buffer);
      DEXIT;
      return i;
   }

   /* put information to lt_heard_from list */
   set_last_heard_from(nfromcommproc, nfromid, nfromhost, sge_get_gmt());

   DEXIT;
   return 0;
}

/**********************************************************************
  enroll to commd

   name  = name of this commproc
   id    = (ref) id we want to have, if id = 0 commd returns a unused id

   environment variables used

   COMMD_HOST if  present host of communication daemon to contact
              if !present the local host is used
 **********************************************************************/

int enroll()
{
   int i, j;
   char *name;
   u_short id;
   int *tag_priority_list;

   DENTER(COMMD_LAYER, "enroll");

   name = get_commlib_state_componentname();
   id = get_commlib_state_componentid();
   tag_priority_list = get_commlib_state_addr_stored_tag_priority_list();

   i = enroll_(name, &id, tag_priority_list);

   DPRINTF(("%d = enroll(name=%s, id=%d, tag_priority_list=[\n", i, name, id));
   if (tag_priority_list) {
      for (j = 0; j < 9; j++)
         DPRINTF(("%d\n,", tag_priority_list[j]));
      DPRINTF(("%d", tag_priority_list[9]));
   }

   DPRINTF(("])\n"));

#if RAND_ERROR
   srand(sge_get_gmt());
#endif
   DEXIT;
   return i;
}

static int enroll_(
char *name,
u_short *id,
int *tag_priority_list 
) {
   unsigned char buffer[MAXCOMPONENTLEN + 1 + STRLEN_ID + 1], *bufptr = buffer;
   int i;
   unsigned char *cp;
   u_long32 chksum = 0;

#ifndef WIN32NATIVE
   sigset_t omask;
#endif

   DENTER(COMMD_LAYER, "enroll_");

   /* test parameters */

   if (get_commlib_state_enrolled()) {
      DEXIT;
      return CL_ALREADYDONE;
   }

   if (secure_strlen(name, MAXCOMPONENTLEN + 1) > MAXCOMPONENTLEN) {
      DEXIT;
      return CL_RANGE;
   }

   set_commlib_state_componentname(name);
   if (tag_priority_list)
      set_commlib_state_stored_tag_priority_list(tag_priority_list);
   else
      clear_commlib_state_stored_tag_priority_list();

   if ((i = get_environments())) {      /* look for port and host of commd */
      DEXIT;
      return i;
   }

   /* prolog */
   cp = pack_ulong(COMMD_ENROLL, buffer);
#ifndef WIN32NATIVE
   cp = pack_ushort(strlen(name) + 1 + 2 + 2 + 20, cp);
#else /* WIN32NATIVE */
   cp = pack_ushort((u_short)(strlen(name) + 1 + 2 + 2 + 20), cp);
#endif /* WIN32NATIVE */
   cp = pack_ulong(0, cp);
   chksum = cksum((char*)buffer, PROLOGLEN-4);
   cp = pack_ulong(chksum, cp);

   /* header */
   cp = pack_string(name, cp);
   cp = pack_ushort(*id, cp);
   cp = pack_ushort((u_short)get_commlib_state_closefd(), cp);
   for (i = 0; i < 10; i++)
#ifndef WIN32NATIVE
      cp = pack_ushort(get_commlib_state_stored_tag_priority_list_i(i), cp);
#else /* WIN32NATIVE */
      cp = pack_ushort((u_short)get_commlib_state_stored_tag_priority_list_i(i), cp);
#endif /* WIN32NATIVE */

#ifndef WIN32NATIVE
   omask = build_n_set_mask();
#endif

   i = send2commd(buffer, cp - buffer
#ifdef COMMLIB_ENABLE_DEBUG
                     , "enroll_(#01)"
#endif
                  );
   if (!i) {
      i = recvfromcommd(&bufptr, NULL, 1, NULL, NULL, NULL
#ifdef COMMLIB_ENABLE_DEBUG
                        , "enroll_(#1)"
#endif
                        );

      if (i) {
#ifndef WIN32NATIVE
         sigprocmask(SIG_SETMASK, &omask, NULL);
#endif
         DEXIT;
         return i;
      }
      if (buffer[0]) {
         closeconnection(1);
#ifndef WIN32NATIVE
         sigprocmask(SIG_SETMASK, &omask, NULL);
#endif
         DEXIT;
         return buffer[0];
      }
      i = recvfromcommd(&bufptr, NULL, 2, NULL, NULL, NULL
#ifdef COMMLIB_ENABLE_DEBUG
                        , "enroll_(#2)"
#endif
                        );
   }

   if (i) {
#ifndef WIN32NATIVE
      sigprocmask(SIG_SETMASK, &omask, NULL);
#endif
      closeconnection(1);
      DEXIT;
      return i;
   }

   closeconnection(0);
#ifndef WIN32NATIVE
   sigprocmask(SIG_SETMASK, &omask, NULL);
#endif
   unpack_ushort(id, buffer);
   set_commlib_state_componentid(*id);

   set_commlib_state_enrolled(1);
   set_commlib_state_ever_enrolled(1);

   DEXIT;
   return 0;
}

/**********************************************************************
 Remove messages we dont wanna hear anything about.
 This is e.g. for cleaning up at starttime
 return CL ERRORCODE
 **********************************************************************/
int remove_pending_messages (char *fromcommproc, u_short fromid,
                             char *fromhost, int tag)
{
   char fromc[MAXCOMPONENTLEN], fromh[MAXHOSTLEN];
   u_short fromi;
   int i, t;
   char *buffer = NULL;
   u_long32 buflen;

   DENTER(COMMD_LAYER, "remove_pending_messages");

   do {
      /* copy cause arguments may be changed by receive_message */
      if (fromcommproc)
         strcpy(fromc, fromcommproc);
      else
         fromc[0] = '\0';
      fromi = fromid;
      if (fromcommproc)
         strcpy(fromh, fromcommproc);
      else
         fromh[0] = '\0';
      t = tag;

      buflen = 0;

      i = receive_message(fromc, &fromi, fromh, &t, &buffer, &buflen, 0, NULL);
      if (buffer) {
         free(buffer);
         buffer = NULL;
      }

   } while (!i);

   if (i == COMMD_NACK_NO_MESSAGE) {
      DEXIT;
      return CL_OK;
   }

   DEXIT;
   return i;
}

/**********************************************************************
 commd went down and lost the information of our enroll()
 enroll again with same name/id
 It could happen that we are slow and another commproc enrolled in the
 meantime catching our id. So we have no chance reenrolling.
 **********************************************************************/
static int reenroll_if_necessary()
{
   int ret = 0;

   DENTER(COMMD_LAYER, "reenroll_if_necessary");

   if (get_commlib_state_changed_flag() || 
       !get_commlib_state_enrolled()) {
      ret = force_reenroll();
   } 
      
   DEXIT;
   return ret;
}

static int force_reenroll() {
   int ret;

   DENTER(COMMD_LAYER, "force_reenroll"); 

   if (get_commlib_state_enrolled()) {
      int i;

      i = leave_commd(); 
      if (i) {
         COMMLIB_ERROR((COMMLIB_BUFFER, "enrolled but leave_commd failed"
                  " with %s", cl_errstr(i)));
         set_commlib_state_enrolled(0);
      } 
   }    
   ret = enroll();
   if (ret) {
      COMMLIB_ERROR((COMMLIB_BUFFER, "enroll failed with status %s", 
                     cl_errstr(ret)));
   } 
   set_commlib_state_changed_flag(0);

   DEXIT;
   return ret;   
}

/**********************************************************************
  leave from commd

  environment variables used

  COMMD_HOST if  present host of communication daemon to contact
             if !present the local host is used
 **********************************************************************/

int leave_commd()
{
   int i;

   DENTER(COMMD_LAYER, "leave_commd");

   i = leave_();

   DPRINTF(("%d = leave_commd()\n", i));

   DEXIT;
   return i;
}

static int leave_()
{
   int headerlen, i;
   unsigned char ackchar, *ackcharptr = &ackchar;
   unsigned char prolog_header[PROLOGLEN +HEADERLEN]; 
   unsigned char *header, *prolog, *cp;
#ifndef WIN32NATIVE
   sigset_t omask;
#endif

   DENTER(COMMD_LAYER, "leave_");

   prolog = prolog_header;
   header = prolog_header + PROLOGLEN;

   /* test parameters */

   if (!get_commlib_state_enrolled()) {
      DEXIT;
      return CL_NOTENROLLED;
   }

   /* header */
   cp = pack_string(get_commlib_state_componentname(), header);
   cp = pack_ushort(get_commlib_state_componentid(), cp);
   headerlen = cp - header;

   /* prolog */
   cp = pack_ulong(COMMD_LEAVE, prolog);
#ifndef WIN32NATIVE
   cp = pack_ushort(headerlen, cp);
#else /* WIN32NATIVE */
   cp = pack_ushort((u_short)headerlen, cp);
#endif /* WIN32NATIVE */
   cp = pack_ulong(0, cp);      /* no body */
   cp = pack_ulong(cksum((char*)prolog, PROLOGLEN-4), cp);

#ifndef WIN32NATIVE
   omask = build_n_set_mask();
#endif

   i = send2commd(prolog_header, PROLOGLEN+headerlen
#ifdef COMMLIB_ENABLE_DEBUG
                  , "leave_commd (#01)"
#endif
                  );
   if (i) {
#ifndef WIN32NATIVE
      sigprocmask(SIG_SETMASK, &omask, NULL);
#endif
      DEXIT;
      return i;
   }

   i = recvfromcommd(&ackcharptr, NULL, 1, NULL, NULL, NULL
#ifdef COMMLIB_ENABLE_DEBUG
                     , "leave_commd (#1)"
#endif
                     );

   if (i) {
#ifndef WIN32NATIVE
	  sigprocmask(SIG_SETMASK, &omask, NULL);
#endif
      DEXIT;
      return i;
   }

   closeconnection(1);
#ifndef WIN32NATIVE
   sigprocmask(SIG_SETMASK, &omask, NULL);
#endif

   if (!ackchar) {
      set_commlib_state_enrolled(0);
      set_commlib_state_ever_enrolled(0);
   }   

   DEXIT;
   return ackchar;
}

/**********************************************************************
  ask for enrolled commproc

    host
    commprocname "" = any
    commprocid (0=any)
 **********************************************************************/
unsigned int ask_commproc (const char *h, const char *commprocname, 
                           u_short commprocid)
{
   unsigned int i;
#ifndef WIN32NATIVE
   sigset_t omask;
#endif

#ifndef NO_SGE_COMPILE_DEBUG
   u_long now = 0;
#endif

   DENTER(CULL_LAYER, "ask_commproc");

#ifndef NO_SGE_COMPILE_DEBUG
#ifndef WIN32NATIVE
   if (rmon_mlgetl(&DEBUG_ON, COMMD_LAYER) & INFOPRINT)
#endif /* WIN32NATIVE */
      now = sge_get_gmt();
#endif

#if RAND_ERROR
      i = random_error(0);
      if (i != 0) {
         DEXIT;
         return i;
      }   
#endif

#ifndef WIN32NATIVE
   omask = build_n_set_mask();
#endif

   i = send_message_(1, commprocname, commprocid, h, 1, NULL, 0, NULL, 1, 0);

#ifndef WIN32NATIVE
   sigprocmask(SIG_SETMASK, &omask, NULL);
#endif

   DPRINTF(("%d = ask_commproc(host=%s, commproc=%s, id=%d) needs %lds\n",
            i, h, commprocname, commprocid, sge_get_gmt() - now));

   /* no need to call set_last_heard_from() for this commproc
      this is done inside send_message_() */

   DEXIT;
   return i;
}

/**********************************************************************
  control commd

   cntl_operation = controlling operation
   arg = additional argument to operation
   environment variables used

   COMMD_HOST if  present host of communication daemon to contact
              if !present the local host is used
 **********************************************************************/
int cntl(u_short cntl_operation, u_long32 *arg, char *carg)
{
   int i;

   DENTER(COMMD_LAYER, "cntl");

   i = cntl_(cntl_operation, arg, carg);

   DPRINTF(("%d = cntl(cntl_operation=%d, arg=%ld, carg=%s)\n",
            i, cntl_operation, arg ? *arg : 0, carg ? carg : ""));

   DEXIT;
   return i;
}

/**********************************************************************/
static int cntl_(u_short cntl_operation, u_long32 *arg, char *carg)
{
   unsigned char buffer[256], *bufptr = buffer;
   int i, headerlen;
   unsigned char *cp;
   DENTER(COMMD_LAYER, "cntl_");

   if ((i = get_environments())) {        
      /* look for port and host of commd */
      DEXIT;
      return i;
   }

   /* prolog */
   headerlen = pack_string_len(carg ? carg : "") + 6;
   cp = pack_ulong(COMMD_CNTL, buffer);
#ifndef WIN32NATIVE
   cp = pack_ushort(headerlen, cp);
#else /* WIN32NATIVE */
   cp = pack_ushort((u_short)headerlen, cp);
#endif /* WIN32NATIVE */
   cp = pack_ulong(0, cp);
   cp = pack_ulong(cksum((char*)buffer, PROLOGLEN-4), cp);

   /* header */
   cp = pack_ushort(cntl_operation, cp);
   cp = pack_ulong(*arg, cp);
   cp = pack_string(carg ? carg : "", cp);

   i = send2commd(buffer, cp - buffer
#ifdef COMMLIB_ENABLE_DEBUG
                  , "cntl_ (#01)"
#endif
                  );
   if (!i) {
      i = recvfromcommd(&bufptr, NULL, 1, NULL, NULL, NULL
#ifdef COMMLIB_ENABLE_DEBUG
                        , "cntl_ (#1)"
#endif
                        );
   }

   if (i) {
      closeconnection(0);
      DEXIT;
      return i;
   }
   if (buffer[0]) {
      closeconnection(0);
      DEXIT;
      return (unsigned char) buffer[0];
   }

   if (cntl_operation == O_TRACE) {
      while ((i = readnbytes_nb(get_commlib_state_sfd(), 
            (char *) buffer, -1, 99999)) > 0) {

         /* put output to stdout as long as pipe exists */
         buffer[i] = '\0';
         printf((char *) buffer);
         fflush(stdout);
      }
      closeconnection(0);
      if (!i) {
         fprintf(stderr, MSG_COMMLIB_LOST_CONNECTION );
#ifdef COMMLIB_ENABLE_DEBUG
         INFO((SGE_EVENT, "cntl_ returns CL_READ #1: %s\n",
                  strerror(stored_errno)));
#endif
         DEXIT;
         return CL_READ;
      }
      DEXIT;
      return 0;
   }

   if (cntl_operation == O_GETID) {

      i = recvfromcommd(&bufptr, NULL, 4, NULL, NULL, NULL
#ifdef COMMLIB_ENABLE_DEBUG
                        , "cntl_ (#2)"
#endif
                        );
      if (i) {
         closeconnection(0);
         DEXIT;
         return i;
      }

      cp = unpack_ulong(arg, buffer);
      closeconnection(0);
      DEXIT;
      return 0;
   }

   closeconnection(0);
   DEXIT;
   return buffer[0];
}

/**********************************************************************/
/* Return a new message id which should be different to all other mids.
   We do a simple incrementing with automatically wrap around at the end
   of the integer range. This should give different mids for a while.
 */
static u_long mid_new()
{
   /*static u_long lastmid = 0;*/

   inc_commlib_state_lastmid();
   if (!get_commlib_state_lastmid())
      inc_commlib_state_lastmid();

   return get_commlib_state_lastmid();
}

/**********************************************************************
   COMMUNICATION ROUTINES
 **********************************************************************/

/* Open connection to commd if not allready connected.
   Then send the buffer to the commd
   This is the version trying to avoid hanging around too long in connect().
   This was noticed on LINUX and caused us to hang for a minute.
 */
int send2commd(unsigned char *buffer, int buflen 
#ifdef COMMLIB_ENABLE_DEBUG
               , const char *context_string
#endif
) {
#ifndef WIN32                   /* var not needed */
   int port = IPPORT_RESERVED - 1;
#endif

   struct sockaddr_in addr;
   int i, si, connect_time, sso;
   struct timeval timeout;
   fd_set writefds;
   char dummy;
   int rpflag = 0;
   int connected_flag = 0;
#ifdef WIN32NATIVE
   u_long nonzero = 1;
   u_long zero = 0;
   int wsalasterror;
#endif 

   DENTER(COMMD_LAYER, "send2commd");

   if (get_commlib_state_sfd() == -1) {
      /* no connection done -> open one */

      connect_time   = 60;
      connected_flag = 0;
      while (connect_time > 0) {
         uid_t euid;
         if (get_commlib_state_reserved_port()) {
#ifndef WIN32 
            DPRINTF(("before seteuid: uid/gid (%ld/%ld), euid/egid (%ld/%ld)\n", 
                     getuid(), getgid(), geteuid(), getegid()));
            euid = geteuid();
            if (euid)
               seteuid(0);
            DPRINTF(("before rresvport: uid/gid (%ld/%ld), euid/egid (%ld/%ld)\n", 
                     getuid(), getgid(), geteuid(), getegid()));
            set_commlib_state_sfd(rresvport(&port));
            if (get_commlib_state_sfd() == -1)
               rpflag = 1;
            if (euid)
               seteuid(euid);
            DPRINTF(("after reset of euid: uid/gid (%ld/%ld), euid/egid (%ld/%ld)\n", 
                     getuid(), getgid(), geteuid(), getegid()));

            DPRINTF(("sfd=%d\n", get_commlib_state_sfd()));
#endif
         }
         else
            set_commlib_state_sfd(socket(AF_INET, SOCK_STREAM, 0));

#ifndef WIN32NATIVE 
         if (get_commlib_state_sfd() == -1)
#else
         if (get_commlib_state_sfd() == INVALID_SOCKET)
#endif
         {
            DEXIT;
            return rpflag ? CL_RRESVPORT : CL_CREATESOCKET; 
         }
       

#ifndef WIN32NATIVE
         fcntl(get_commlib_state_sfd(), F_SETFL, O_NONBLOCK);
#else 
         ioctlsocket(get_commlib_state_sfd(), FIONBIO, &zero);
#endif 

         addr.sin_family = AF_INET;
         memcpy((char *) &addr.sin_addr, (char *) get_commlib_state_addr_commdaddr(), get_commlib_state_commdaddr_length());
         addr.sin_port = get_commlib_state_commdport();

         i = connect(get_commlib_state_sfd(), (struct sockaddr *) &addr, sizeof(addr));
         DPRINTF(("connect returns %d\n", i)); 

         if (i == -1 ) {
#ifndef WIN32NATIVE
            if (errno == EINPROGRESS )
#else 
            wsalasterror = WSAGetLastError();
            if (wsalasterror == WSAEINPROGRESS )
#endif 
           { 
               DPRINTF(("errno == EINPROGRESS\n"));

               timeout.tv_sec = 15;
               timeout.tv_usec = 0;
               FD_ZERO(&writefds);

#ifndef WIN32NATIVE
               FD_SET(get_commlib_state_sfd(), &writefds);
#else
               FD_SET((u_int)get_commlib_state_sfd(), &writefds);
#endif
#if defined(HPUX) || defined(HP10_01) || defined(HPCONVEX)
               si = select(get_commlib_state_sfd() + 1 ,NULL ,(int *) &writefds, NULL, &timeout);
#else
               si = select(get_commlib_state_sfd() + 1 ,NULL ,(fd_set *) &writefds, NULL, &timeout);
#endif
      
               DPRINTF(("select returns %d\n", si));
            
               switch (si) {
               case -1:
                  /* select error */
                  DPRINTF(("select returns %d: %s\n", si, strerror(errno)));
                  DPRINTF(("now closing connection with closeconnection(1)\n"));
                  closeconnection(1);
                  DEXIT;
                  return CL_CONNECT;
               case 0:
                  /* 15 seconds expired */
                  closeconnection(1);
                  connect_time -= 15;
                  continue;
               default:
       

                   /* Test for success of connect. EAGAIN was noticed in
                     connections to another host  (not on the same). 
    
                      This dummy read is only for producing an error. The
                      connected server would not send any data after 
                      a client connect.
                   */

#ifndef WIN32NATIVE
	               i = read(get_commlib_state_sfd(), &dummy, 1);
                  if (i == -1 && errno != EAGAIN && errno != EWOULDBLOCK )
#else 
                  i = recv(get_commlib_state_sfd(), &dummy, 1, 0);
                  if (i == -1 &&  WSAGetLastError() != WSAEWOULDBLOCK)
#endif 
                  {
                     DPRINTF(("Read returns %d (errno = %d): %s\n", i, errno, strerror(errno)));
/*                     if ( errno == ECONNREFUSED ) { */
                         DPRINTF(("RECONNECT\n"));
/*                         fprintf(stderr,"\nRECONNECT\n");  */
                         closeconnection(1);
                         connect_time -= 15;
                         sleep(1); 
                         continue;
/*                     }  */
/*                     closeconnection(1);
                     DEXIT;
                     return CL_CONNECT; */
                  }

                  connected_flag = 1;
                  break;        /* connect succeeded */
               }
               break;
            }
            else {
               DPRINTF(("connect returns %d: %s\n", i, strerror(errno)));
               closeconnection(1);
               DEXIT;
               return CL_CONNECT;
            }
         }
         else {
            connected_flag = 1;
            break;              /* connect succeeded */
         }
      }  /* while ... */
      if (!connected_flag) {
          closeconnection(1);
          DEXIT;
          return CL_CONNECT;
      }
      
      sso = 1;
#if defined(SOLARIS) && !defined(SOLARIS64)
      if (setsockopt(get_commlib_state_sfd(), IPPROTO_TCP, TCP_NODELAY, 
          (const char *) &sso, sizeof(int)) == -1)
#else
      if (setsockopt(get_commlib_state_sfd(), IPPROTO_TCP, TCP_NODELAY, 
          &sso, sizeof(int))== -1)
#endif
         DPRINTF(("cannot setsockopt() to TCP_NODELAY.\n"));
   }
   DPRINTF(("writenbytes_nb(%d, %p, %d)\n", get_commlib_state_sfd(), 
      buffer, buflen));

   i = writenbytes_nb(get_commlib_state_sfd(), (char *) buffer, buflen, 60);

   if (i) {
      if(i == -1 || i == -4) {
         closeconnection(1);
      } else {
         closeconnection(0);
      }
   } 

   if (i) {
      if (i == -4) {
         DEXIT;
         return COMMD_NACK_ENROLL;
      }
      if (i == -2) {
         DEXIT;
         return CL_WRITE_TIMEOUT;
      }
#ifdef COMMLIB_ENABLE_DEBUG
         INFO((SGE_EVENT, "send2commd returns CL_WRITE #1 (%s): %s\n",
                  context_string, strerror(stored_errno)));
#endif
      DEXIT;
      return CL_WRITE;
   }

   DEXIT;
   return 0;
}

/**********************************************************************
  receive buffer from commd
   if n>0 receive exact n bytes (buffer is given by caller)
   if n==0 receive message format PROLOG+HEADER+BUFFER (buffer has to be freed
   by caller, header is given by caller) and return pointers to buffer
 **********************************************************************/
int recvfromcommd(unsigned char **buffer, unsigned char *header, int n,
                  u_long32 *flags, u_short *headerlen, u_long32 *buflen 
#ifdef COMMLIB_ENABLE_DEBUG
                  , const char *context_string
#endif
                  ) {
   unsigned char prolog[PROLOGLEN], *cp;
   char *bptr = NULL;
   int i;
   u_long32 crc32 = 0;

   DENTER(COMMD_LAYER, "recvfromcommd");

   if (n) {
      i = readnbytes_nb(get_commlib_state_sfd(), (char *) *buffer, n, 
            get_commlib_state_timeout());
      if (i) {
         if (i > 0) {
            DPRINTF(("readnbytes leaves %d bytes unread\n", i));
         }

         if (i == -2) {
            DPRINTF(("recvfromcommd: timeout\n"));
            closeconnection(1);  /* instead of the former RCV_TIMEOUT:
                                    force close connection to commd */
            DEXIT;
            return COMMD_NACK_TIMEOUT;
         }

         if (i == -3) {
            closeconnection(0);
            DEXIT;
            return CL_INTR;
         }
         closeconnection(1);
#ifdef COMMLIB_ENABLE_DEBUG
         INFO((SGE_EVENT, "recvfromcommd returns CL_READ #1 (%s): %s\n",
               context_string, strerror(stored_errno)));
#endif
         DEXIT;
         return CL_READ;
      } 
      DEXIT;
      return 0;
   }

   if (readnbytes_nb(get_commlib_state_sfd(), (char *) prolog, PROLOGLEN, 60)) {
      closeconnection(1);
#ifdef COMMLIB_ENABLE_DEBUG
         INFO((SGE_EVENT, "recvfromcommd returns CL_READ #2 (%s): %s\n",
                  context_string, strerror(stored_errno)));
#endif
      DEXIT;
      return CL_READ;
   }
   cp = unpack_ulong(flags, prolog);
   cp = unpack_ushort(headerlen, cp);
   cp = unpack_ulong(buflen, cp);
   cp = unpack_ulong(&crc32, cp);

   if(crc32 != cksum((char*)prolog, PROLOGLEN-4)) {
      closeconnection(1);
      DEXIT;
      return CL_CHKSUM;
   }

   if (*buflen) {
      bptr = malloc(*buflen);
      if (!bptr) {
         closeconnection(0);
         DEXIT;
         return CL_MALLOC;
      }
   }
   if ((i = readnbytes_nb(get_commlib_state_sfd(), (char *) header, *headerlen, 60)) ||
       (i = readnbytes_nb(get_commlib_state_sfd(), bptr, *buflen, 60))) {
      free(bptr);
      if (i == -2) {
         closeconnection(0);
         DEXIT;
         return CL_READ_TIMEOUT;
      }
      closeconnection(1);
#ifdef COMMLIB_ENABLE_DEBUG
      INFO((SGE_EVENT, "recvfromcommd returns CL_READ #3 (%s): %s\n",
               context_string, strerror(stored_errno)));
#endif
      DEXIT;
      return CL_READ;
   }
   *buffer = (unsigned char *) bptr;

   DEXIT;
   return 0;
}

/***********************************/
void closeconnection(
int force 
) {
   DENTER(COMMD_LAYER, "closeconnection");

   if(!force && !get_commlib_state_closefd()) {
      DEXIT;
      return;
   }

#ifndef WIN32NATIVE
	if (get_commlib_state_sfd() != -1) {
     	shutdown(get_commlib_state_sfd(), 1);
		close(get_commlib_state_sfd());
		DPRINTF(("closed sfd %d\n", get_commlib_state_sfd()));
	   set_commlib_state_sfd(-1);
	}
#else 
	if (get_commlib_state_sfd() != INVALID_SOCKET) {
		shutdown(get_commlib_state_sfd(), 1);
		closesocket(get_commlib_state_sfd());
		DPRINTF(("closed sfd %d\n", get_commlib_state_sfd()));
	   set_commlib_state_sfd(INVALID_SOCKET);
	}
#endif 

   DEXIT;
}







/****** commd/getuniquehostname() **********************************
*
*  NAME
*     getuniquehostname() -- resolve hostname to get his primary name 
*
*  SYNOPSIS
*
*     #include "commlib.h"
*     #include <commd/commlib.h>
* 
*     int getuniquehostname(char *hostin, char *hostout, int refresh_aliases); 
*       
*
*  FUNCTION
*
*
*  INPUTS
*     char* hostin          - host for which to get the primary hostname
*     char* hostout         - filled with primary host
*     int   refresh_aliases - reload aliasfile
*
*  RESULT
*     0  = OK
*     !0 = CL_... errorcode
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     src/sge_resolveMappingList()
*     
****************************************************************************
*/
/**********************************************************************
  get unique hostname / force reread of aliasfile

   parameters

   hostin          = host for which to get the primary hostname
   hostout         = filled with primary host
   refresh_aliases = reload aliasfile

   0  = OK
   !0 = CL_... errorcode
 **********************************************************************/
int getuniquehostname(const char *hostin, char *hostout, int refresh_aliases) 
{
   unsigned char *cp;
   unsigned char ackchar, *ackcharptr = &ackchar;
   unsigned int i;
   int headerlen;
   unsigned char header[HEADERLEN], prolog[PROLOGLEN], *headerptr = header;
   u_long flags = 0;
   u_short hostnamelen;
   int secStrLen = 0;
#ifndef WIN32NATIVE
   sigset_t omask;
#endif 

   DENTER(COMMD_LAYER, "getuniquehostname");

   if ((i = reenroll_if_necessary())) {
      DEXIT;
      return i;
   } 

   if (hostin == NULL) {
      DEXIT;
      return CL_NOTENROLLED;
   }

   secStrLen = secure_strlen(hostin, MAXHOSTLEN + 1);

   if (secStrLen == 0) {
      DEXIT;
      return CL_NOTENROLLED;
   }

   if (secStrLen > MAXHOSTLEN) {
      DEXIT;
      return CL_RANGE;
   }

   /* fill header */
   cp = header;

   flags |= COMMD_UNIQUEHOST;

   cp = pack_string(hostin, cp);
#ifndef WIN32NATIVE
   cp = pack_ushort(refresh_aliases, cp);
#else /* WIN32NATIVE */
   cp = pack_ushort((u_short)refresh_aliases, cp);
#endif /* WIN32NATIVE */

   /* known from enroll - needed for reconnet in commd */
   cp = pack_string(get_commlib_state_componentname(), cp);
   cp = pack_ushort(get_commlib_state_componentid(), cp);

   headerlen = cp - header;

   /* build prolog */
   cp = pack_ulong(flags, prolog);
#ifndef WIN32NATIVE
   cp = pack_ushort(headerlen, cp);
#else /* WIN32NATIVE */
   cp = pack_ushort((u_short)headerlen, cp);
#endif /* WIN32NATIVE */
   cp = pack_ulong(0, cp);
   cp = pack_ulong(cksum((char*)prolog, PROLOGLEN-4), cp);

   /* write prolog */
   i = send2commd(prolog, PROLOGLEN
#ifdef COMMLIB_ENABLE_DEBUG
                  , "getuniquehostname (#01)"
#endif
                  );
   if (i) {
      DEXIT;
      return i;
   }

   /* write header */
   i = send2commd(header, headerlen
#ifdef COMMLIB_ENABLE_DEBUG
                  , "getuniquehostname (#02)"
#endif 
                );
   if (i) {
      DEXIT;
      return i;
   }

   /* we should get a acknowledge, the length of the hostname and the
      hostname itself */

   while(1) {
      /* wait for an acknowledge */
      i = recvfromcommd((unsigned char **) &ackcharptr, NULL, 1,
            NULL, NULL, NULL
#ifdef COMMLIB_ENABLE_DEBUG
                        , "getuniquehostname (#1)"
#endif
                        );
      if (i) {
         closeconnection(0);
         DEXIT;
         return i;
      }
      if ((unsigned int) ackchar == CL_UNKNOWN_RECEIVER ||
          (unsigned int) ackchar == COMMD_NACK_ENROLL) {
         /* This happens, when commd goes down and is now up again. He lost
            the enroll()-information. We have to renew this */
         closeconnection(1);
         i = force_reenroll();
         if (i) {
#ifndef WIN32NATIVE
            sigprocmask(SIG_SETMASK, &omask, NULL);
#endif
            DEXIT;
            return ackchar;
         }
         continue;              /* send again */
      }      
      if (ackchar) {
         closeconnection(0);
         DEXIT;
         return ackchar;
      }
      break;
   }

   i = recvfromcommd((unsigned char **) &headerptr, NULL, 2, NULL, NULL, NULL
#ifdef COMMLIB_ENABLE_DEBUG
                     , "getuniquehostname (#2)"
#endif
                     );
   if (i) {
      closeconnection(0);
      DEXIT;
      return i;
   }

   unpack_ushort(&hostnamelen, header);
   if (hostnamelen > MAXHOSTLEN) {
      closeconnection(0);
      DEXIT;
      return CL_RANGE;
   }

   i = recvfromcommd((unsigned char **) &headerptr, NULL, hostnamelen, NULL, 
                     NULL, NULL
#ifdef COMMLIB_ENABLE_DEBUG
                     , "getuniquehostname (#3)"
#endif
                     );
   if (i) {
      closeconnection(0);
      DEXIT;
      return i;
   }

   closeconnection(0);

   cp = unpack_string(hostout, hostnamelen, header);

   DEXIT;
   return CL_OK;
}


/****** commlib/generate_commd_port_and_service_status_message() ***************
*  NAME
*     generate_commd_port_and_service_status_message() -- master failure error
*
*  SYNOPSIS
*     void generate_commd_port_and_service_status_message(int commlib_error, 
*     char* buffer) 
*
*  FUNCTION
*     This function is used to generate an error message when the qmaster 
*     is unreachable. The error message is copied into a static buffer.
*     The buffer have a length of MAX_STRING_SIZE byte.
*
*     if commlib_error is negative, the default output is 
*     "qmaster not reachable"
*     if commlib_error is 0 the function will try to create the correct answer
*     if commlib_error is > 0 the function will report the commlib error
*
*  INPUTS
*     int commlib_error - if negative: output is "qmaster not reachable"
*                         if zero    : try to create correct answer
*                         if positive: report commlib error
*
*     char* buffer      - buffer for error message 
*
*******************************************************************************/
void generate_commd_port_and_service_status_message(int commlib_error, char* buffer) {
   int port             = 0;
   char *service        = NULL;
   char *commdhost      = NULL;
   char *commd_port_env = NULL;

   DENTER(TOP_LAYER, "generate_commd_port_and_service_status_message");

   DPRINTF(("commlib_error =  %d\n",commlib_error));

   port           = get_commlib_state_commdport();
   service        = get_commlib_state_commdservice();
   commdhost      = get_commlib_state_commdhost();
   commd_port_env = getenv("COMMD_PORT");   

   if (service == NULL) {
      service = "unknown";
   }
   if (commdhost == NULL) {
      commdhost = "unknown";
   }

   if (buffer != NULL) {
      /* commlib error is negative, we don't know why we can't reach qmaster*/
      if ( commlib_error < 0 ) {
         sprintf(buffer, MSG_SGETEXT_NOQMASTER_REACHABLE );
      }

      /* commlib error is 0 (CL_OK), there was no commlib error */
      if ( commlib_error == CL_OK || 
           commlib_error == CL_CONNECT || 
           commlib_error == CL_SERVICE ||
           ( commlib_error >= CL_FIRST_FREE_EC && commlib_error != COMMD_NACK_UNKNOWN_RECEIVER) ) {
         if ( port < 0 ) {
            sprintf(buffer, MSG_SGETEXT_NOQMASTER_NOPORT_NOSERVICE_SS,commdhost, service);
         } else if ( commd_port_env != NULL ) { 
            sprintf(buffer, MSG_SGETEXT_NOQMASTER_PORT_ENV_SI,commdhost,port);
         } else {
            sprintf(buffer, MSG_SGETEXT_NOQMASTER_PORT_SERVICE_ENV_SIS,commdhost,port,service);
         } 
      } else {
         if (commlib_error == COMMD_NACK_UNKNOWN_RECEIVER) {
            sprintf(buffer, MSG_SGETEXT_NOQMASTER_SUBSCR_AT_COMMD_S, commdhost );
         } else {
            sprintf(buffer, MSG_SGETEXT_NOQMASTER_REACHABLE_COMMLIB_S , cl_errstr(commlib_error) );
         }
      } 
   }
   DEXIT;
}

/***************************************************************/
/* get commdport and commdhost out of environment variables    */
/* if commdport is overruled by the commlib user or we got it  */
/* allready dont use environment                               */
/* ports are prior to services                                 */
/***************************************************************/
static int get_environments()
{
   char localhost[MAXHOSTLEN];
   struct servent *se = NULL;
   char *commdhost;
   struct hostent *he;
   int nisretry;
   char *cp;

   /* get host of commd */
   if (get_commlib_state_commdhost()[0]) {
      commdhost = get_commlib_state_commdhost();
   }
   else {
      commdhost = getenv("COMMD_HOST");
      if (!commdhost) {
         gethostname(localhost, sizeof(localhost));
         commdhost = localhost;
      }
   }

   he = gethostbyname(commdhost);
   if (!he)
      return CL_RESOLVE;

   /* store ip address of commd */

   set_commlib_state_commdaddr_length(he->h_length);
   memcpy((char *) get_commlib_state_addr_commdaddr(), (char *) he->h_addr, get_commlib_state_commdaddr_length());

   /* if not allready done get port of commd */
   if (get_commlib_state_commdport() == -1) {
      if ((cp = getenv("COMMD_PORT"))) {

#ifndef WIN32NATIVE
         set_commlib_state_commdport(htons(atoi(cp)));
#else /* WIN32NATIVE */
         set_commlib_state_commdport(htons((u_short)atoi(cp)));
#endif /* WIN32NATIVE */

         return 0;
      }

      if (!((cp = getenv("COMMD_SERVICE")))) {
         cp = get_commlib_state_commdservice();
         if (cp[0] == '\0')
            cp = "unknown_service";
      }

      nisretry = MAXNISRETRY;   /* NIS sometimes neede several attempts */
      while (nisretry-- && !((se = getservbyname(cp, "tcp"))));
      if (!se)
         return CL_SERVICE;

      set_commlib_state_commdport(se->s_port);
   }

   return 0;
}

/****************************************************************
 build and set signal mask for communication
 return old mask for restoring after communication
 ****************************************************************/
#ifndef WIN32NATIVE
static sigset_t build_n_set_mask()
{
   sigset_t mask, omask;

   sigprocmask(SIG_SETMASK, NULL, &omask);      /* get mask */
   mask = omask;

   sigdelset(&mask, SIGABRT);
   sigdelset(&mask, SIGBUS);
   sigdelset(&mask, SIGILL);
   sigdelset(&mask, SIGQUIT);
   sigdelset(&mask, SIGURG);
   sigdelset(&mask, SIGIO);
   sigdelset(&mask, SIGSEGV);
   sigdelset(&mask, SIGFPE);
   sigaddset(&mask, SIGCLD);

   sigprocmask(SIG_SETMASK, &mask, NULL);
   return omask;
}
#endif

#if RAND_ERROR
int random_error(
int val 
) {
   double r;
   int rand_val;

   if (!rand_error)
      return val;
      
   rand_val = rand();
   r = ((double)rand_val/(double)RAND_MAX); /* 0.0 - 1.0 */
   if (r<0.9) {
      return val;
   }
   val = ((double)rand()/(double)RAND_MAX ) * (CL_WRITE_TIMEOUT - CL_RANGE) + 1;
   fprintf(stderr, MSG_COMMLIB_BITFLIPPER_FS , r, cl_errstr(val));
   return val;
}
#endif


/* state access functions */
#ifdef QIDL
static void commlib_state_destroy(void* state) {
   free(state);
}

void commlib_init() {
   pthread_key_create(&commlib_state_key, &commlib_state_destroy);
}

static void commlib_state_init(struct commlib_state_t* state) {
   int i;

   for(i=0; i<sizeof(state->stored_tag_priority_list); i++)
      state->stored_tag_priority_list[i] = 0;
   state->enrolled = 0;
   state->ever_enrolled = 0;
   state->componentname[0] = '\0';
   state->componentid = 0;
   state->commdport = -1;          /* commdport in network order */
   state->commdservice[0] = '\0';
   state->commdaddr_length = 0;
   state->sfd = -1;
   state->lastmid = 0;
   state->lastgc = 0;
   state->reserved_port = 0;
   state->commdhost[0] = '\0';
   state->timeout = 60;
   state->timeout_srcv = TIMEOUT_SYNC_RCV;
   state->timeout_ssnd = TIMEOUT_SYNC_SND;
   state->offline_receive = 0;
   state->lt_heard_from_timeout = 0;
   state->closefd = 0;
   state->list = NULL;
   state->sge_log = NULL;
   state->changed_flag = 0;
}
#endif

int get_commlib_state_enrolled() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->enrolled;
}

int get_commlib_state_ever_enrolled() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->ever_enrolled;
}

int* get_commlib_state_addr_stored_tag_priority_list() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->stored_tag_priority_list;
}

int get_commlib_state_stored_tag_priority_list_i(
int i 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->stored_tag_priority_list[i];
}

char* get_commlib_state_componentname() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->componentname;
}

u_short get_commlib_state_componentid() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->componentid;
}

u_short* get_commlib_state_addr_componentid() { 
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return &(commlib_state->componentid);
}

int get_commlib_state_commdport() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->commdport;
}

char* get_commlib_state_commdservice() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->commdservice ;
}
      
int get_commlib_state_commdaddr_length() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->commdaddr_length;
}

struct in_addr* get_commlib_state_addr_commdaddr() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return &(commlib_state->commdaddr);
}

int get_commlib_state_sfd() { 
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->sfd;
}

u_long get_commlib_state_lastmid() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->lastmid;
}

u_long get_commlib_state_lastgc() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->lastgc;
}

int get_commlib_state_reserved_port() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->reserved_port;
}

char* get_commlib_state_commdhost() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->commdhost;
}

int get_commlib_state_timeout() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->timeout;
}

int get_commlib_state_timeout_srcv() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->timeout_srcv;
}

int get_commlib_state_timeout_ssnd() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->timeout_ssnd;
}

int get_commlib_state_offline_receive() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->offline_receive;
}

int get_commlib_state_lt_heard_from_timeout() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->lt_heard_from_timeout;
}

int get_commlib_state_closefd() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->closefd;
}

entry* get_commlib_state_list() {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->list;
}

sge_log_ftype get_commlib_state_logging_function () {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   return commlib_state->sge_log;      
}

int get_commlib_state_changed_flag(void) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif     
   return commlib_state->changed_flag;
} 

void set_commlib_state_enrolled(
int state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->enrolled = state;
}

void set_commlib_state_ever_enrolled(
int state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->ever_enrolled = state;
}

void set_commlib_state_stored_tag_priority_list(
int *state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   memcpy(commlib_state->stored_tag_priority_list, state, sizeof(commlib_state->stored_tag_priority_list));
   commlib_state->changed_flag = 1;
}

void set_commlib_state_componentname(
const char *state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   strcpy(commlib_state->componentname, state);
   commlib_state->changed_flag = 1; 
}

void set_commlib_state_componentid(u_short state)
{
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->componentid = (u_short)state;
   commlib_state->changed_flag = 1; 
}

void set_commlib_state_commdport(
int state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->commdport = state;
   commlib_state->changed_flag = 1; 
}

void set_commlib_state_commdservice(
const char *state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   strcpy(commlib_state->commdservice, state);
}

void set_commlib_state_commdaddr_length(
int state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->commdaddr_length = state;
}

void set_commlib_state_sfd(
int state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->sfd= state;
}

void set_commlib_state_lastmid(
u_long state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->lastmid= state;
}

void set_commlib_state_lastgc(
u_long state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->lastgc= state;
}

void set_commlib_state_reserved_port(
int state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->reserved_port= state;
}

void set_commlib_state_commdhost(
const char *state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   strcpy(commlib_state->commdhost, state);
   commlib_state->changed_flag = 1; 
}

void set_commlib_state_timeout(
int state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->timeout= state;
}

void set_commlib_state_timeout_srcv(
int state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->timeout_srcv= state;
}

void set_commlib_state_timeout_ssnd(
int state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->timeout_ssnd= state;
}

void set_commlib_state_offline_receive(
int state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->offline_receive= state;
}

void set_commlib_state_lt_heard_from_timeout(
int state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->lt_heard_from_timeout= state;
}

void set_commlib_state_closefd(
int state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   if(!get_commlib_state_enrolled())
      commlib_state->closefd= state;
}

void set_commlib_state_list(
entry *state 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->list= state;
}

void set_commlib_state_logging_function(
sge_log_ftype sge_log_function 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   } else {
      commlib_state = pthread_getspecific(commlib_state_key);  
   }
#endif
   commlib_state->sge_log = sge_log_function;
}

void set_commlib_state_changed_flag(
int flag 
) {
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   } else {
      commlib_state = pthread_getspecific(commlib_state_key);
   }
#endif   
   commlib_state->changed_flag = flag;
}

void inc_commlib_state_lastmid()
{
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   commlib_state->lastmid++;
}

void clear_commlib_state_stored_tag_priority_list()
{
#ifdef QIDL
   struct commlib_state_t* commlib_state;
   if(!pthread_getspecific(commlib_state_key)) {
      commlib_state = (struct commlib_state_t*)malloc (sizeof(struct commlib_state_t));
      commlib_state_init(commlib_state);
      pthread_setspecific(commlib_state_key, (void*)commlib_state);
   }
   else
      commlib_state = pthread_getspecific(commlib_state_key);
#endif
   memset(commlib_state->stored_tag_priority_list, 0, sizeof(commlib_state->stored_tag_priority_list));
}

int is_commd_alive () {
   reenroll_if_necessary();
   return get_commlib_state_enrolled();
}  
