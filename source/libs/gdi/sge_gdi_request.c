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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef WIN32NATIVE
#	include <unistd.h>
#endif
#include <stdlib.h>
#include "def.h"
#include "commlib.h"
#include "sge_stdlib.h"
#include "sge_gdi_intern.h"
#include "sge_c_gdi.h"
#include "sge_multiL.h"
#include "sge_answerL.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sge_uidgid.h"
#include "qm_name.h"
#include "sge_unistd.h"
#include "sge_security.h"
#ifdef KERBEROS
#  include "krb_lib.h"
#endif

#ifdef QIDL
#include "qidl_c_gdi.h"
#endif
#include "msg_utilib.h"
#include "msg_gdilib.h"

static int sge_send_receive_gdi_request(char *rhost, char *commproc, 
                                        u_short id, sge_gdi_request *out, 
                                        sge_gdi_request **in);

static int sge_get_gdi_request(char *rhost, char *commproc, u_short *id, 
                               sge_gdi_request **arp);

#ifdef QIDL
static int sge_handle_local_gdi_request(sge_gdi_request *out, 
                                        sge_gdi_request **in);
#endif

/****** gdi/request/sge_gdi() *************************************************
*  NAME
*     sge_gdi() -- request, change or delete data in the master daemon
*
*  SYNOPSIS
*     lList* sge_gdi(u_long32 target, u_long32 cmd, lList** lpp, 
*                    lCondition* cp, lEnumeration* enp) 
*
*  FUNCTION
*     Using this function an application can operate on a linked lists 
*     (CULL) stored in the master daemon. 
*
*  INPUTS
*     u_long32 target   - References a list of the master 
*              SGE_JOB_LIST    - list of jobs
*              SGE_QUEUE_LIST  - list of queues
*              SGE_CKPT_LIST   - list of checkpointing objects
*              ...
*           (a complete list ob enum values can be found in sge_gdi.h)
*
*     u_long32 cmd      - cmd is a bitmask which decribes the operation 
*           to be done. It is composed by an basic 'operation' and an 
*           'subcommand'. 
*           The 'operation' describes what to do with elements directly
*           contained within lpp.
*           The 'subcommand' bits gives the master some hints how
*           to handle elements in sublists contained in the elements 
*           of lpp.
*
*           operation:
*              SGE_GDI_GET - get a list of objects
*              SGE_GDI_ADD - add objects contained in list
*              SGE_GDI_DEL - delete objects contained in lpp
*              SGE_GDI_MOD - change some objects 
*              ...
*           subcommands:
*              SGE_GDI_SET - overwrite a sublist with given values
*              SGE_GDI_CHANGE - change sublist elements
*              SGE_GDI_APPEND - add elements into contained in sublists
*              SGE_GDI_REMOVE - remove elements contained in sublist
*              ...
*           (a complete list ob enum values and valid combinations 
*            can be found in sge_gdi.h)
*
*     lList** lpp       - This parameter is used to get a list in case 
*           of SGE_GDI_GET command. The caller is responsible for 
*           freeing by using lFreeList(). In the other cases the caller 
*           passes a list containing the (sub)elements to 
*           add/modify/delete. sge_gdi() doesn't free the passed list.
*
*     lCondition* cp    - Points to a lCondition as it is build by 
*           lWhere (refer to CULL documentation). This enumeration 
*           describes the fields in the request list of an 
*           SGE_GDI_GET-request.  
*
*     lEnumeration* enp - Points to a lEnumerations structure build 
*           by lWhat() (refer to CULL documentation) in case of 
*           SGE_GDI_GET command. This enumeration describes the fields 
*           in the requested list. 
*
*  RESULT
*     returns a CULL list reporting success/failure of the operation. 
*     This list gets allocated by sge_gdi. Again the caller is 
*     responsible for freeing. A response is a list element containing 
*     a field with a status value (AN_status). The value STATUS_OK is 
*     used in case of success. STATUS_OK and other values are defined 
*     in sge_answerL.h. the second field (AN_text) in a response list 
*     element is a string that describes the performed operation or a 
*     description of an error.
*     Each call od sge_gdi passes a list with at least one respone to 
*     the caller. The response list of a SGE_GDI_GET-operation 
*     containes only one element reporting success or failure.  
*    
*  EXAMPLE
*     In following directory you can find small applications which
*     demonstrate the use of functions contained in lib_gdi.a
*
*        <BASEDIE>/source/dist/gdi/examples 
*
*     More detailed examples can be found in the client applications 
*     (qconf, qsub ...)
*
******************************************************************************/
lList* sge_gdi(u_long32 target, u_long32 cmd, lList **lpp, lCondition *cp,
               lEnumeration *enp) 
{
   lList *alp = NULL;
   lList *mal = NULL;
   u_long32 id;
   int operation;

   DENTER(GDI_LAYER, "sge_gdi");

   operation = SGE_GDI_GET_OPERATION(cmd); 
   /* just in case */
#ifndef QHOST_TEST   
   if (operation == SGE_GDI_GET)  /* ||(operation == SGE_GDI_PERMCHECK)) */ 
      *lpp = NULL;
#endif

   if ((id = sge_gdi_multi(&alp, SGE_GDI_SEND, target, cmd, lpp ? *lpp : NULL, 
                              cp, enp, &mal)) == -1) {
      DEXIT;
      return alp;
   }

   alp = sge_gdi_extract_answer(cmd, target, id, mal, lpp);

   mal = lFreeList(mal);

   DEXIT;
   return alp;
}

/****** gdi/request/sge_gdi_multi() *******************************************
*  NAME
*     sge_gdi_multi() -- get, change or delete multiple lists  
*
*  SYNOPSIS
*     int sge_gdi_multi(lList** alpp, int mode, u_long32 target, 
*                       u_long32 cmd, lList* lp, lCondition* cp, 
*                       lEnumeration* enp, lList** malpp) 
*
*  FUNCTION
*     In some situations it is necessary to change multiple master lists
*     Normally someone would use multiple sge_gdi() requests which would 
*     raise frequent commlib communication.
*
*     This function makes it possible to record multiple GDI requests
*     and send them as one package through the underlaying communication
*     layers.
*
*     To to this, this function has to be called in two different modes.
*     In the first mode (=SGE_GDI_RECORD) this function records the gdi 
*     request (no communication with the master). In the second mode
*     (=SGE_GDI_SEND) the method sends all gdi requests to the master.
*
*  INPUTS
*     lList** alpp      - result of this sge_gdi_multi() call 
*
*     int mode          - What should the function do with this request 
*        SGE_GDI_RECORD - record a GDI request (no commlib comm.)
*        SGE_GDI_SEND   - send all recorded GDI requests including 
*                         the current one
*
*     u_long32 target   - References a list of the master
*              SGE_JOB_LIST    - list of jobs
*              SGE_QUEUE_LIST  - list of queues
*              SGE_CKPT_LIST   - list of checkpointing objects
*              ...
*           (a complete list ob enum values can be found in sge_gdi.h)
*
*     u_long32 cmd      - cmd is a bitmask which decribes the operation
*           to be done. It is composed by an basic 'operation' and an
*           'subcommand'.
*           The 'operation' describes what to do with elements directly
*           contained within lpp.
*           The 'subcommand' bits gives the master some hints how to 
*           handle elements in sublists contained in the elements 
*           of lpp.
*
*           operation:
*              SGE_GDI_GET - get a list of objects
*              SGE_GDI_ADD - add objects contained in list
*              SGE_GDI_DEL - delete objects contained in lpp
*              SGE_GDI_MOD - change some objects
*              ...
*           subcommands:
*              SGE_GDI_SET - overwrite a sublist with given values
*              SGE_GDI_CHANGE - change sublist elements
*              SGE_GDI_APPEND - add elements into contained in sublists
*              SGE_GDI_REMOVE - remove elements contained in sublist
*              ...
*           (a complete list ob enum values and valid combinations
*            can be found in sge_gdi.h)
*
*     lList* lp         - The caller can specify a list
*           containing the (sub)elements to add/modify/delete.
*           sge_gdi_multi() doesn't free the passed list.
*
*     lCondition* cp    - Points to a lCondition as it is build by 
*           lWhere (refer to CULL documentation). This enumeration 
*           describes the fields in the request list of an 
*           SGE_GDI_GET-request.
*
*     lEnumeration* enp - Points to a lEnumerations structure build 
*           by lWhat() (refer to CULL documentation) in case of 
*           SGE_GDI_GET command. This enumeration describes the fields 
*           in the requested list.  
*
*     lList** malpp     - in case of mode=SGE_GDI_SEND this parameter
*           returns informations for each invidual request previously
*           stored with sge_gdi_multi(mode=SGE_GDI_RECORD). 
*           sge_gdi_extract_answer() can be used to get the answer 
*           list for one of these GDI requests.
*
*  RESULT
*     -1  - if an error occured
*     (positive integer) - id which identifies the current gdi request.
*        The returned ids are only unique until this function was 
*        called with SGE_GDI_SEND as mode. Ids returned by this 
*        function can be used with sge_gdi_extract_answer() to get 
*        answer lists for single GDI requests.
*
*  EXAMPLE
*     Please have a look into the qstat client application. This client
*     demonstrates the use of this function very good. 
*
******************************************************************************/
int sge_gdi_multi(lList **alpp, int mode, u_long32 target, u_long32 cmd,
                  lList *lp, lCondition *cp, lEnumeration *enp, lList **malpp) 
{
   lListElem *map = NULL;
   sge_gdi_request *request = NULL;
   sge_gdi_request *answer = NULL;
   sge_gdi_request *an;
   static sge_gdi_request *first = NULL;
   static sge_gdi_request *last = NULL;
   static u_long32 sequence_id = 0;
   static u_long32 request_id = 0;
   int ret;
   int operation;
   static int reread_qmaster_file = 0;
   uid_t uid;
   gid_t gid;
   char username[128];
   char groupname[128];
   int status = 0;

   DENTER(GDI_LAYER, "sge_gdi_multi");

   operation = SGE_GDI_GET_OPERATION(cmd);

   if (!lp && !(operation == SGE_GDI_PERMCHECK || operation == SGE_GDI_GET 
       || operation == SGE_GDI_TRIGGER || 
       (operation == SGE_GDI_DEL && target == SGE_SHARETREE_LIST))) {
      sprintf(SGE_EVENT, MSG_GDI_POINTER_NULLPOINTERPASSEDTOSGEGDIMULIT );
      goto error;
   }

   if (!(request = new_gdi_request())) {
      sprintf(SGE_EVENT, MSG_GDI_CANTCREATEGDIREQUEST );
      goto error;
   }
   
   request->op = cmd;
   request->target = target;
   request->version = GRM_GDI_VERSION;
   request->alp = NULL;
   switch (operation) {
#ifndef QHOST_TEST
   case SGE_GDI_GET:
      request->lp = NULL;
      break;
#endif
   case SGE_GDI_MOD:
      if (enp) {
         request->lp = lSelect("lp", lp, NULL, enp);
         break;
      }
      /* no break */
   default:
      request->lp = lCopyList("lp", lp);
      break;
   }
   if ((operation == SGE_GDI_GET) || (operation == SGE_GDI_PERMCHECK)) {
      request->cp =  lCopyWhere(cp);
      request->enp = lCopyWhat(enp);
   } else {
      request->cp =  NULL;
      request->enp = NULL; 
   }

   /* 
   ** user info
   */
   uid = getuid();
   if (sge_uid2user(uid, username, sizeof(username), MAX_NIS_RETRIES)) {
      sprintf(SGE_EVENT, MSG_GDI_GETPWUIDXFAILEDERRORX_IS , 
              (int)uid, strerror(errno));
      goto error;
   }
   gid = getgid();
   if (sge_gid2group(gid, groupname, sizeof(groupname), 
         MAX_NIS_RETRIES)) {
      sprintf(SGE_EVENT, MSG_GDI_GETGRGIDXFAILEDERRORX_IS , 
              (int)gid, strerror(errno));
      goto error; 
   }

   if (sge_set_auth_info(request, uid, username, gid, groupname) == -1)
      goto error;

   /*
   ** append the new gdi request to the request list
   */
   ret = request->sequence_id = ++sequence_id;
   
   if (first) {
      last->next = request;
      last = request;
   }
   else {
      first = last = request;
   }
   
   if (mode == SGE_GDI_SEND) {

      /*
      ** the first request in the request list identifies the request uniquely
      */
      first->request_id = ++request_id;

#ifdef KERBEROS
      /* request that the Kerberos library forward the TGT */
      if (target == SGE_JOB_LIST && operation == SGE_GDI_ADD) {
         krb_set_client_flags(krb_get_client_flags() | KRB_FORWARD_TGT);
         krb_set_tgt_id(first->request_id);
      }
#endif

#ifdef QIDL      
      if (me.who != QMASTER)
#endif
         /* FIX_CONST */
         status = sge_send_receive_gdi_request(
            (char*)sge_get_master(reread_qmaster_file), 
            (char*)prognames[QMASTER], 
            0, first, &answer);
#ifdef QIDL
      else
            status = sge_handle_local_gdi_request(first, &answer);
#endif

#ifdef KERBEROS
      /* clear the forward TGT request */
      if (target == SGE_JOB_LIST && operation == SGE_GDI_ADD) {
         krb_set_client_flags(krb_get_client_flags() & ~KRB_FORWARD_TGT);
         krb_set_tgt_id(0);
      }
#endif

      if (status != 0) {
         reread_qmaster_file = 1;

         /* failed to contact qmaster ? */
         /* So we build an answer structure */
         switch (status) {
            case -2:
               sprintf(SGE_EVENT, MSG_GDI_SENDINGGDIREQUESTFAILED);
               break;
            case -3:
               sprintf(SGE_EVENT, MSG_GDI_RECEIVEGDIREQUESTFAILED );
               break;
            case -4:
               sprintf(SGE_EVENT, MSG_SGETEXT_NOQMASTER);
               break;
            case -5:
               sprintf(SGE_EVENT, MSG_GDI_SIGNALED );
               break;
            default:
               sprintf(SGE_EVENT, MSG_GDI_GENERALERRORXSENDRECEIVEGDIREQUEST_I , status);
               break;
         }
         goto error;
      }
      else
         reread_qmaster_file = 0;
    
      for (an = answer; an; an = an->next) { 
         int an_operation, an_sub_command;

         map = lAddElemUlong(malpp, MA_id, an->sequence_id, MA_Type);
         an_operation = SGE_GDI_GET_OPERATION(an->op);
         an_sub_command = SGE_GDI_GET_SUBCOMMAND(an->op);
         if (an_operation == SGE_GDI_GET || an_operation == SGE_GDI_PERMCHECK ||
               (an_operation==SGE_GDI_ADD 
                && an_sub_command==SGE_GDI_RETURN_NEW_VERSION )) {
            lSetList(map, MA_objects, an->lp);
            an->lp = NULL;
         }
         lSetList(map, MA_answers, an->alp);
         an->alp = NULL;
      }

      answer = free_gdi_request(answer);
      first = free_gdi_request(first);
      last = NULL;
      sequence_id = 0;
   }

   DEXIT;
   return ret;

   error:
      if (alpp)
         sge_add_answer(alpp, SGE_EVENT, STATUS_NOQMASTER, 0);
      answer = free_gdi_request(answer);
      first = free_gdi_request(first);
      last = NULL;
      sequence_id = 0;
      DEXIT;
      return -1;
}

/****** gdi/request/sge_gdi_extract_answer() **********************************
*  NAME
*     sge_gdi_extract_answer() -- exctact answers of a multi request.
*
*  SYNOPSIS
*     lList* sge_gdi_extract_answer(u_long32 cmd, u_long32 target, 
*                                   int id, lList* mal, lList** olpp) 
*
*  FUNCTION
*     This function extracts the answer for each invidual request on 
*     previous sge_gdi_multi() calls. 
*
*  INPUTS
*     u_long32 cmd    - bitmask which decribes the operation 
*        (see sge_gdi_multi)
*
*     u_long32 target - unique id to identify masters list
*        (see sge_gdi_multi) 
*
*     int id          - unique id returned by a previous
*        sge_gdi_multi() call. 
*
*     lList* mal      - List of answer/response lists returned from
*        sge_gdi_multi(mode=SGE_GDI_SEND)
*
*     lList** olpp    - This parameter is used to get a list in case 
*           of SGE_GDI_GET command. The caller is responsible for 
*           freeing by using lFreeList(). 
*
*  RESULT
*     returns a CULL list reporting success/failure of the operation. 
*     This list gets allocated by GDI. The caller is responsible 
*     for freeing. A response is a list element containing a field 
*     with a status value (AN_status). The value STATUS_OK is used 
*     in case of success. STATUS_OK and other values are defined in 
*     sge_answerL.h. the second field (AN_text) in a response list 
*     element is a string that describes the performed operation or 
*     a description of an error.
******************************************************************************/
lList *sge_gdi_extract_answer(u_long32 cmd, u_long32 target, int id,
                              lList *mal, lList **olpp) 
{
   lList *alp = NULL;
   lListElem *map = NULL;
   int operation, sub_command;

   DENTER(GDI_LAYER, "sge_gdi_extract_answer");

   operation = SGE_GDI_GET_OPERATION(cmd); 
   sub_command = SGE_GDI_GET_SUBCOMMAND(cmd);

   if (!mal || id < 0) {
      sprintf(SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC);
      sge_add_answer(&alp, SGE_EVENT, STATUS_ESYNTAX, 0);
      DEXIT;
      return alp;
   }
   
   map = lGetElemUlong(mal, MA_id, id);
   if (!map) {
      DEXIT;
      return NULL;
   }

   if ((operation == SGE_GDI_GET) || (operation == SGE_GDI_PERMCHECK) ||
       (operation == SGE_GDI_ADD && sub_command == SGE_GDI_RETURN_NEW_VERSION )) {
      if (!olpp) {
         sprintf(SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC);
         sge_add_answer(&alp, SGE_EVENT, STATUS_ESYNTAX, 0);
         DEXIT;
         return alp;
      }
      lXchgList(map, MA_objects, olpp);
   }

   lXchgList(map, MA_answers, &alp);

   DEXIT;
   return alp;
}
  
/****** gdi/request/sge_send_receive_gdi_request() ****************************
*  NAME
*     sge_send_receive_gdi_request() -- snd and rcv a gdi structure 
*
*  SYNOPSIS
*     static int sge_send_receive_gdi_request(char *rhost, 
*                                char *commproc, u_short id, 
*                                sge_gdi_request *out, 
*                                sge_gdi_request **in) 
*
*  FUNCTION
*     sends and receives an gdi request structure 
*
*  INPUTS
*     char *rhost          - ??? 
*     char *commproc       - ??? 
*     u_short id           - ??? 
*     sge_gdi_request *out - ??? 
*     sge_gdi_request **in - ??? 
*
*  RESULT
*     static int - 
*        0 ok
*        -1 failed before communication
*        -2 failed sending gdi request
*        -3 failed receiving gdi request
*        -4 check_isalive() failed
*        -5 failed due to a received signal  
******************************************************************************/
static int sge_send_receive_gdi_request(char *rhost, char *commproc, 
                                        u_short id, sge_gdi_request *out,
                                        sge_gdi_request **in)
{
   int ret;
   
   DENTER(GDI_LAYER, "sge_send_receive_gdi_request");

   if (!out) {
      ERROR((SGE_EVENT,
           MSG_GDI_POINTER_NULLLISTPASSEDTOSGESENDRECEIVGDIREQUEST ));
      DEXIT;
      return -1;
   }

   if (!rhost) {
      ERROR((SGE_EVENT, MSG_GDI_POINTER_NULLRHOSTPASSEDTOSGESENDRECEIVEGDIREQUEST ));
      DEXIT;
      return -1;
   }   
   

   if ((ret = sge_send_gdi_request(1, rhost, commproc, id, out))) {
      if (ret == CL_INTR) {
         DEXIT;
         return -5;
      } else if (check_isalive(rhost)) {
         DEXIT;
         return -4;
      }
      else {
         DEXIT;
         return -2;
      }   
   }

   while (!(ret = sge_get_gdi_request(rhost, commproc, &id, in))) {
      DPRINTF(("in: request_id=%d, sequence_id=%d, target=%d, op=%d\n",
            (*in)->request_id, (*in)->sequence_id, (*in)->target, (*in)->op));
      DPRINTF(("out: request_id=%d, sequence_id=%d, target=%d, op=%d\n",
               out->request_id, out->sequence_id, out->target, out->op));

      if (*in && ((*in)->request_id == out->request_id)) {
         break;
      }
      else {
         *in = free_gdi_request(*in);
         DPRINTF(("<<<<<<<<<<<<<<< GDI MISMATCH >>>>>>>>>>>>>>>>>>>\n"));
      }
   }

   if (ret) {
      if (check_isalive(rhost)) {
         DEXIT;
         return -4;
      } 
      else {
         DEXIT;
         return -3;
      }   
   }
   
   DEXIT;
   return 0;
}

/****** gdi/request/sge_send_gdi_request() ************************************
*  NAME
*     sge_send_gdi_request() -- send gdi request 
*
*  SYNOPSIS
*     int sge_send_gdi_request(int sync, const char *rhost, 
*                              const char *commproc, int id, 
*                              sge_gdi_request *ar) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     int sync             - ??? 
*     const char *rhost    - ??? 
*     const char *commproc - ??? 
*     int id               - ??? 
*     sge_gdi_request *ar  - ??? 
*
*  RESULT
*     int - 
*         0 success
*        -1 common failed sending
*        -2 not enough memory
*        -3 format error while unpacking
*        -4 no commd
*        -5 no peer enrolled   
*******************************************************************************/
int sge_send_gdi_request(int sync, const char *rhost, const char *commproc,
                         int id, sge_gdi_request *ar) 
{
   sge_pack_buffer pb;
   int ret, size;

   DENTER(GDI_LAYER, "sge_send_gdi_request");

   /* 
   ** retrieve packbuffer size to avoid large realloc's while packing 
   */
   init_packbuffer(&pb, 0, 1);
   ret = sge_pack_gdi_request(&pb, ar);
   size = pb_used(&pb);
   clear_packbuffer(&pb);

   if (ret == PACK_SUCCESS) {
      /*
      ** now we do the real packing
      */
      if((ret = init_packbuffer(&pb, size, 0)) == PACK_SUCCESS) {
         ret = sge_pack_gdi_request(&pb, ar);
      }
   }

   switch (ret) {
   case PACK_SUCCESS:
      break;

   case PACK_ENOMEM:
      ERROR((SGE_EVENT, MSG_GDI_MEMORY_NOTENOUGHMEMORYFORPACKINGGDIREQUEST ));
      clear_packbuffer(&pb);
      DEXIT;
      return -2;

   case PACK_FORMAT:
      ERROR((SGE_EVENT, MSG_GDI_REQUESTFORMATERROR ));
      clear_packbuffer(&pb);
      DEXIT;
      return -3;

   default:
      ERROR((SGE_EVENT, MSG_GDI_UNEXPECTEDERRORWHILEPACKINGGDIREQUEST ));
      clear_packbuffer(&pb);
      DEXIT;
      return -1;
   }

   ret = sge_send_any_request(sync, NULL, rhost, commproc, id, &pb, 
                              TAG_GDI_REQUEST);
   clear_packbuffer(&pb);

   DEXIT;
   return ret;
}

/****** gdi/request/sge_get_gdi_request() *************************************
*  NAME
*     sge_get_gdi_request() -- ??? 
*
*  SYNOPSIS
*     static int sge_get_gdi_request(char *host, char *commproc, 
*                                    u_short *id, sge_gdi_request** arp) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     char *host            - ??? 
*     char *commproc        - ??? 
*     u_short *id           - ??? 
*     sge_gdi_request** arp - ??? 
*
*  RESULT
*     static int - 
*         0 success 
*        -1 common failed getting
*        -2 not enough memory 
*        -3 format error while unpacking
*        -4 no commd
*        -5 no peer enrolled
*******************************************************************************/
static int sge_get_gdi_request(char *host, char *commproc, 
                               u_short *id, sge_gdi_request** arp)
{
   sge_pack_buffer pb;
   int tag = TAG_GDI_REQUEST; /* this is what we want */
   int ret;

   DENTER(GDI_LAYER, "sge_get_gdi_request");

   if (sge_get_any_request(host, commproc, id, &pb, &tag, 1)) {
      DEXIT;
      return -1;
   }

   ret = sge_unpack_gdi_request(&pb, arp);
   switch (ret) {
   case PACK_SUCCESS:
      break;

   case PACK_ENOMEM:
      ret = -2;
      ERROR((SGE_EVENT, MSG_GDI_MEMORY_NOTENOUGHMEMORYFORUNPACKINGGDIREQUEST ));
      break;

   case PACK_FORMAT:
      ret = -3;
      ERROR((SGE_EVENT, MSG_GDI_REQUESTFORMATERRORWHILEUNPACKING ));
      break;

   default:
      ret = -1;
      ERROR((SGE_EVENT, MSG_GDI_UNEXPECTEDERRORWHILEUNPACKINGGDIREQUEST ));
      break;
   }

   /* 
      we got the packing buffer filled by 
      sge_get_any_request and have to recycle it 
   */
   clear_packbuffer(&pb);

   DEXIT;
   return ret;
}

/****** gdi/request/sge_unpack_gdi_request() **********************************
*  NAME
*     sge_unpack_gdi_request() -- unpacks an gdi_request structure 
*
*  SYNOPSIS
*     int sge_unpack_gdi_request(sge_pack_buffer *pb, 
*                                sge_gdi_request **arp) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     sge_pack_buffer *pb   - ??? 
*     sge_gdi_request **arp - ??? 
*
*  RESULT
*     int - 
*         0 on success
*        -1 not enough memory
*        -2 format error 
******************************************************************************/
int sge_unpack_gdi_request(sge_pack_buffer *pb, sge_gdi_request **arp) 
{
   int ret;
   sge_gdi_request *ar = NULL;
   sge_gdi_request *prev_ar = NULL;
   u_long32 next;

   DENTER(GDI_LAYER, "sge_unpack_gdi_request");

   do {
      if (!(ar = new_gdi_request())) {
         ret = PACK_ENOMEM;
         goto error;
      }
      if ((ret=unpackint(pb, &(ar->op)) )) goto error;
      if ((ret=unpackint(pb, &(ar->target)) )) goto error;


      if ((ret=unpackint(pb, &(ar->version)) )) goto error;
DTRACE;      
      if ((ret=cull_unpack_list(pb, &(ar->lp)) )) goto error;
DTRACE;      
      if ((ret=cull_unpack_list(pb, &(ar->alp)) )) goto error;
DTRACE;      
      if ((ret=cull_unpack_cond(pb, &(ar->cp)) )) goto error;
      if ((ret=cull_unpack_enum(pb, &(ar->enp)) )) goto error;
      
      if ((ret=unpackstr(pb, &(ar->auth_info)) )) goto error;
      if ((ret=unpackint(pb, &(ar->sequence_id)) )) goto error;
      if ((ret=unpackint(pb, &(ar->request_id)) )) goto error;
      if ((ret=unpackint(pb, &next) )) goto error;

      switch (ar->op) {
      case SGE_GDI_GET:
         DPRINTF(("unpacking SGE_GDI_GET request\n"));
         break;
      case SGE_GDI_ADD:
      case SGE_GDI_ADD | SGE_GDI_RETURN_NEW_VERSION:
         DPRINTF(("unpacking SGE_GDI_ADD request\n"));
         break;
      case SGE_GDI_DEL:
      case SGE_GDI_DEL | SGE_GDI_ALL_JOBS:
      case SGE_GDI_DEL | SGE_GDI_ALL_USERS:
      case SGE_GDI_DEL | SGE_GDI_ALL_JOBS | SGE_GDI_ALL_USERS:
         DPRINTF(("unpacking SGE_GDI_DEL request\n"));
         break;
      case SGE_GDI_MOD:
      case SGE_GDI_MOD | SGE_GDI_ALL_JOBS:
      case SGE_GDI_MOD | SGE_GDI_ALL_USERS:
      case SGE_GDI_MOD | SGE_GDI_ALL_JOBS | SGE_GDI_ALL_USERS:
      case SGE_GDI_MOD | SGE_GDI_APPEND:
      case SGE_GDI_MOD | SGE_GDI_REMOVE:
      case SGE_GDI_MOD | SGE_GDI_CHANGE:
         DPRINTF(("unpacking SGE_GDI_MOD request\n"));
         break;
      case SGE_GDI_TRIGGER:
         DPRINTF(("unpacking SGE_GDI_TRIGGER request\n"));
         break;
      case SGE_GDI_PERMCHECK:
         DPRINTF(("unpacking SGE_GDI_PERMCHECK request\n"));
         break;
      case SGE_GDI_SPECIAL:
         DPRINTF(("unpacking special things\n"));
         break;
      case SGE_GDI_COPY:
         DPRINTF(("unpacking copy request\n"));
         break;
      default:
         ERROR((SGE_EVENT, MSG_GDI_ERROR_INVALIDVALUEXFORARTOOP_D , u32c(ar->op)));
         goto error;
      }

      if (!prev_ar) {
         *arp = prev_ar = ar;
      }
      else {
         prev_ar->next = ar;
         prev_ar = ar;
      }
   } while (next);
   

   DEXIT;
   return 0;

 error:
   ERROR((SGE_EVENT, MSG_GDI_CANTUNPACKGDIREQUEST ));
   *arp = free_gdi_request(*arp);
   DEXIT;
   return ret;
}

/*--------------------------------------------------------------
 * sge_pack_gdi_request
 *--------------------------------------------------------------*/
int sge_pack_gdi_request(sge_pack_buffer *pb, sge_gdi_request *ar) 
{
   int ret;

   DENTER(GDI_LAYER, "sge_pack_gdi_request");

   while (ar) {
      switch (ar->op) {
      case SGE_GDI_GET:
         DPRINTF(("packing SGE_GDI_GET request\n"));
         break;
      case SGE_GDI_ADD:
      case SGE_GDI_ADD | SGE_GDI_RETURN_NEW_VERSION:
         DPRINTF(("packing SGE_GDI_ADD request\n"));
         break;
      case SGE_GDI_DEL:
      case SGE_GDI_DEL | SGE_GDI_ALL_JOBS:
      case SGE_GDI_DEL | SGE_GDI_ALL_USERS:
      case SGE_GDI_DEL | SGE_GDI_ALL_JOBS | SGE_GDI_ALL_USERS:
         DPRINTF(("packing SGE_GDI_DEL request\n"));
         break;
      case SGE_GDI_MOD:

      case SGE_GDI_MOD | SGE_GDI_ALL_JOBS:
      case SGE_GDI_MOD | SGE_GDI_ALL_USERS:
      case SGE_GDI_MOD | SGE_GDI_ALL_JOBS | SGE_GDI_ALL_USERS:

      case SGE_GDI_MOD | SGE_GDI_APPEND:
      case SGE_GDI_MOD | SGE_GDI_REMOVE:
      case SGE_GDI_MOD | SGE_GDI_CHANGE:

         DPRINTF(("packing SGE_GDI_MOD request\n"));
         break;
      case SGE_GDI_TRIGGER:
         DPRINTF(("packing SGE_GDI_TRIGGER request\n"));
         break;
      case SGE_GDI_PERMCHECK:
         DPRINTF(("packing SGE_GDI_PERMCHECK request\n"));
         break;
      case SGE_GDI_SPECIAL:
         DPRINTF(("packing special things\n"));
         break;
      case SGE_GDI_COPY:
         DPRINTF(("request denied\n"));
         break;
      default:
         ERROR((SGE_EVENT, MSG_GDI_ERROR_INVALIDVALUEXFORARTOOP_D , u32c(ar->op)));
         DEXIT;
         return PACK_FORMAT;
      }

      if ((ret=packint(pb, ar->op) )) goto error;
      if ((ret=packint(pb, ar->target) )) goto error;
      if ((ret=packint(pb, ar->version) )) goto error;
      if ((ret=cull_pack_list(pb, ar->lp) )) goto error;
      if ((ret=cull_pack_list(pb, ar->alp) )) goto error;
      if ((ret=cull_pack_cond(pb, ar->cp) )) goto error;
      if ((ret=cull_pack_enum(pb, ar->enp) )) goto error;

      if ((ret=packstr(pb, ar->auth_info) )) goto error;
      if ((ret=packint(pb, ar->sequence_id) )) goto error;
      if ((ret=packint(pb, ar->request_id) )) goto error;
      if ((ret=packint(pb, ar->next ? 1 : 0) )) goto error;
   
      ar = ar->next;
   }

   DEXIT;
   return PACK_SUCCESS;

 error:
   DEXIT;
   return ret;
}


/*----------------------------------------------------------*/
sge_gdi_request *new_gdi_request()
{
   sge_gdi_request *ar;

   DENTER(GDI_LAYER, "new_gdi_request");

   ar = (sge_gdi_request *) sge_malloc(sizeof(sge_gdi_request));
   
   memset(ar, 0, sizeof(sge_gdi_request));

   DEXIT;
   return ar;
}


/*------------------------------------------------------------*/
sge_gdi_request *free_gdi_request(sge_gdi_request *ar) 
{
   sge_gdi_request *next;

   DENTER(GDI_LAYER, "free_gdi_request");

   /*
   ** free the list from first to last
   */
   while (ar) {
      /* save next pointer */
      next = ar->next;

      if (ar->host) free(ar->host);
      if (ar->commproc) free(ar->commproc);
      if (ar->auth_info) free(ar->auth_info);

      lFreeList(ar->lp);
      lFreeList(ar->alp);
      lFreeWhere(ar->cp);
      lFreeWhat(ar->enp);

      free(ar);

      ar = next;
   }

   DEXIT;
   return NULL;
}


/*--------------------------------------------------------------- 
 * sge_add_answer
 *
 *  Add an answer element to an answer list.
 *  This is used for answering requests.
 *---------------------------------------------------------------*/
int sge_add_answer(lList **alpp, const char *report, u_long32 status,
                   u_long32 quality) 
{
   lListElem *aep;

   DENTER(GDI_LAYER, "sge_add_answer");
   if ( !alpp ) {
      DEXIT;
      return -1;
   }

   /* build new answer element */
   aep = lCreateElem(AN_Type);
   lSetString(aep, AN_text, report);
   lSetUlong(aep, AN_status, status);
   lSetUlong(aep, AN_quality, quality);
   
   /* create a list for the answers */
   if (!*alpp)
      *alpp = lCreateList("answer", AN_Type);

   lAppendElem(*alpp, aep);

   DEXIT;
   return 0;
}

int answer_list_is_error_in_list(lList **answer_list)
{
   lListElem *answer = NULL;
   int ret = 0;

   if (answer_list != NULL) {
      for_each(answer, *answer_list) {
         if (lGetUlong(answer, AN_quality) ==  NUM_AN_ERROR) {
            ret = 1;
            break;
         }
      }
   }
   return ret;
}                   

/*-----------------------------------------------------------------------*/
const char *quality_text(lListElem *aep) 
{
   u_long32 q;
   static char *qt[] = {
      "ERROR",
      "WARNING",
      "INFO"
   };
   if ((q = lGetUlong(aep, AN_quality))>2)
      q = 0;
   return qt[q];  
}

/****** gdi/request/sge_get_recoverable() *************************************
*  NAME
*     sge_get_recoverable() -- analyze answer of gdi request 
*
*  SYNOPSIS
*     u_long32 sge_get_recoverable(lListElem *aep) 
*
*  FUNCTION
*     Used for analyzing the answer of an gdi request. This is client 
*     code and exits if there is no sense proceeding (May be the master 
*     cant be contacted ...). Errors are printed to stderr and returned. 
*
*  INPUTS
*     lListElem *aep - AN_Type 
*
*  RESULT
*     u_long32 - status value
*
*  NOTES
*     This function may terminate the application 
*******************************************************************************/
u_long32 sge_get_recoverable(lListElem *aep) 
{
   int pos;
   u_long32 status;

   DENTER(GDI_LAYER, "sge_get_recoverable");

   if (!aep) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      SGE_EXIT(1);
   }

   /* is the list element an answer element ? */
   if ( (pos = lGetPosViaElem(aep, AN_status))<0) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS, 
            lNm2Str(AN_status), SGE_FUNC));
      SGE_EXIT(1);
   }

   switch (status = lGetPosUlong(aep, pos)) {
      case STATUS_NOQMASTER:
      case STATUS_NOCOMMD:
      case STATUS_ENOKEY:
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         SGE_EXIT(1);
      default:
         break;
   }

   DEXIT;
   return status;
}

#ifdef QIDL

int sge_handle_local_gdi_request(
sge_gdi_request *out,
sge_gdi_request **in 
) {
   sge_gdi_request *ar = NULL;
   sge_gdi_request *an = NULL;
   
   for (ar = out; ar; ar = ar->next) {
      struct sge_auth* qidl_me = get_qidl_me();
      
      if (ar->host) free(ar->host);
      if (ar->commproc) free(ar->commproc);
      if (ar->auth_info) free(ar->auth_info);

      /* use id, commproc and host for authentication */
      ar->id = 0;
      ar->commproc = sge_strdup(NULL, "sge_qidld");

      if (sge_set_auth_info(ar, qidl_me->uid, qidl_me->user, 
                           qidl_me->gid, qidl_me->group))
         return -1;

      ar->host = sge_strdup(NULL, qidl_me->host);

      if (ar == out) {
         *in = an = new_gdi_request();
      }
      else {
         an->next = new_gdi_request();
         an = an->next;
      }
      sge_c_gdi(me.qualified_hostname, ar, an);
   }

   return 0;
}

#endif
