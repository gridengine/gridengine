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
#include <pwd.h>
#include <pthread.h>

#ifdef SOLARISAMD64
#  include <sys/stream.h>
#endif 

#include "cull.h"
#include "sgermon.h"
#include "sge_gdiP.h"
#include "sge_any_request.h"
#include "sge_gdi_request.h"
#include "sge_log.h"
#include "setup_path.h"
#include "sge_string.h"
#include "sge_afsutil.h"
#include "execution_states.h"
#include "sge_gdi.h"
#include "qm_name.h"
#include "sge_unistd.h"
#include "sge_feature.h"
#include "dispatcher.h"
#include "sge_security.h"
#include "sge_uidgid.h"
#include "sge_io.h"
#include "sge_stdio.h"
#include "sge_prog.h"
#include "sge_var.h"
#include "sge_job.h"
#include "sge_answer.h"
#include "sge_time.h"


#include "msg_common.h"
#include "msg_gdilib.h"
#include "sge_hostname.h"
#include "cl_commlib.h"
#include "sge_mtutil.h"

#ifdef CRYPTO
#include <openssl/evp.h>
#endif

#define SGE_SEC_BUFSIZE 1024

#define ENCODE_TO_STRING   1
#define DECODE_FROM_STRING 0

#ifdef SECURE
const char* sge_dummy_sec_string = "AIMK_SECURE_OPTION_ENABLED";

static pthread_mutex_t sec_rw_mutex = PTHREAD_MUTEX_INITIALIZER;

#define SEC_LOCK_RW()      sge_mutex_lock("sec_rw_mutex", SGE_FUNC, __LINE__, &sec_rw_mutex)
#define SEC_UNLOCK_RW()    sge_mutex_unlock("sec_rw_mutex", SGE_FUNC, __LINE__, &sec_rw_mutex)

#endif 

static bool sge_encrypt(char *intext, int inlen, char *outbuf, int outsize);
static bool sge_decrypt(char *intext, int inlen, char *outbuf, int *outsize);
static bool change_encoding(char *cbuf, int* csize, unsigned char* ubuf, int* usize, int mode);
static void dump_rcv_info(cl_com_message_t** message, cl_com_endpoint_t** sender);
static void dump_snd_info(char* un_resolved_hostname, char* component_name, unsigned long component_id, cl_xml_ack_type_t ack_type, unsigned long tag, unsigned long* mid );

static void dump_rcv_info(cl_com_message_t** message, cl_com_endpoint_t** sender) {
   DENTER(TOP_LAYER, "dump_rcv_info");
   if ( message  != NULL && sender   != NULL && *message != NULL && *sender  != NULL &&
        (*sender)->comp_host != NULL && (*sender)->comp_name != NULL ) {
         char buffer[512];
         dstring ds;
         sge_dstring_init(&ds, buffer, sizeof(buffer));

      DEBUG((SGE_EVENT,"<<<<<<<<<<<<<<<<<<<<\n"));
      DEBUG((SGE_EVENT,"gdi_rcv: reseived message from %s/%s/"U32CFormat": \n",(*sender)->comp_host, (*sender)->comp_name, u32c((*sender)->comp_id)));
      DEBUG((SGE_EVENT,"gdi_rcv: cl_xml_ack_type_t: %s\n",            cl_com_get_mih_mat_string((*message)->message_mat)));
      DEBUG((SGE_EVENT,"gdi_rcv: message tag:       %s\n",            sge_dump_message_tag( (*message)->message_tag) ));
      DEBUG((SGE_EVENT,"gdi_rcv: message id:        "U32CFormat"\n",  u32c((*message)->message_id) ));
      DEBUG((SGE_EVENT,"gdi_rcv: receive time:      %s\n",            sge_ctime((*message)->message_receive_time.tv_sec, &ds)));
      DEBUG((SGE_EVENT,"<<<<<<<<<<<<<<<<<<<<\n"));
   }
   DEXIT;
}

static void dump_snd_info(char* un_resolved_hostname, char* component_name, unsigned long component_id, 
                          cl_xml_ack_type_t ack_type, unsigned long tag, unsigned long* mid ) {
   char buffer[512];
   dstring ds;

   DENTER(TOP_LAYER, "dump_snd_info");
   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if (un_resolved_hostname != NULL && component_name != NULL) {
      DEBUG((SGE_EVENT,">>>>>>>>>>>>>>>>>>>>\n"));
      DEBUG((SGE_EVENT,"gdi_snd: sending message to %s/%s/"U32CFormat": \n", (char*)un_resolved_hostname,(char*)component_name ,u32c(component_id)));
      DEBUG((SGE_EVENT,"gdi_snd: cl_xml_ack_type_t: %s\n",            cl_com_get_mih_mat_string(ack_type)));
      DEBUG((SGE_EVENT,"gdi_snd: message tag:       %s\n",            sge_dump_message_tag( tag) ));
      if (mid) {
         DEBUG((SGE_EVENT,"gdi_snd: message id:        "U32CFormat"\n",  u32c(*mid) ));
      } else {
         DEBUG((SGE_EVENT,"gdi_snd: message id:        not handled by caller\n"));
      }
      DEBUG((SGE_EVENT,"gdi_snd: send time:         %s\n",            sge_ctime(0, &ds)));
      DEBUG((SGE_EVENT,">>>>>>>>>>>>>>>>>>>>\n"));
   } else {
      DEBUG((SGE_EVENT,">>>>>>>>>>>>>>>>>>>>\n"));
      DEBUG((SGE_EVENT,"gdi_snd: some parameters are not set\n"));
      DEBUG((SGE_EVENT,">>>>>>>>>>>>>>>>>>>>\n"));
   }
   DEXIT;
}

/****** gdi/security/sge_security_initialize() ********************************
*  NAME
*     sge_security_initialize -- initialize sge security
*
*  SYNOPSIS
*     int sge_security_initialize(char *name);
*
*  FUNCTION
*     Initialize sge security by initializing the underlying security
*     mechanism and setup the corresponding data structures
*
*  INPUTS
*     name - name of enrolling program
*
*  RETURN
*     0  in case of success, something different otherwise 
*
*  NOTES
*     MT-NOTE: sge_security_initialize() is MT safe (assumptions)
******************************************************************************/

int sge_security_initialize(const char *name)
{
   DENTER(TOP_LAYER, "sge_security_initialize");

#ifdef SECURE
   {
      static const char* dummy_string = NULL;
      dummy_string = sge_dummy_sec_string;
      if (feature_is_enabled(FEATURE_CSP_SECURITY)) {
         if (sec_init(name)) {
            DEXIT;
            return -1;
         }
      }
   }
#endif

#ifdef KERBEROS
   if (krb_init(name)) {
      DEXIT;
      return -1;
   }
#endif   

   DEXIT;
   return 0;
}

/****** gdi/security/sge_security_exit() **************************************
*  NAME
*     sge_security_exit -- exit sge security
*
*  SYNOPSIS
*     void sge_security_exit(int status);
*
*  FUNCTION
*     Execute any routines that the security mechanism needs to do when
*     the program
*
*  INPUTS
*     status - exit status value
*
*  NOTES
*     MT-NOTE: sge_security_exit() is MT safe
******************************************************************************/
void sge_security_exit(int i)
{
   DENTER(TOP_LAYER, "sge_security_exit");

#ifdef SECURE
   if (feature_is_enabled(FEATURE_CSP_SECURITY)) {
      sec_exit();
   }     
#endif

   DEXIT;
}


int gdi_receive_sec_message(cl_com_handle_t* handle,char* un_resolved_hostname, char* component_name, unsigned long component_id, int synchron, unsigned long response_mid, cl_com_message_t** message, cl_com_endpoint_t** sender) {

   int ret;
   DENTER(TOP_LAYER, "gdi_receive_sec_message");

#ifdef SECURE   
   if (feature_is_enabled(FEATURE_CSP_SECURITY)) {
      SEC_LOCK_RW();
      ret = sec_receive_message( handle, un_resolved_hostname,  component_name,  component_id,  synchron,   response_mid,  message, sender);
      if (message != NULL) {
         dump_rcv_info(message,sender);
      }
      SEC_UNLOCK_RW();
      DEXIT;
      return ret;
   }                      
#endif

   ret = cl_commlib_receive_message(handle, un_resolved_hostname,  component_name,  component_id,  
                                     synchron, response_mid,  message,  sender);
   if (message != NULL) {
      dump_rcv_info(message,sender);
   }

   DEXIT;
   return ret;
}

int gdi_send_sec_message(cl_com_handle_t* handle,
                            char* un_resolved_hostname, char* component_name, unsigned long component_id, 
                            cl_xml_ack_type_t ack_type, 
                            cl_byte_t* data, unsigned long size , 
                            unsigned long* mid, unsigned long response_mid, unsigned long tag ,
                            int copy_data,
                            int wait_for_ack) {
   int ret;
   DENTER(TOP_LAYER, "gdi_send_sec_message");

   
#ifdef SECURE
   if (feature_is_enabled(FEATURE_CSP_SECURITY)) {
      SEC_LOCK_RW();
      ret = sec_send_message(handle, un_resolved_hostname,  component_name,  component_id, 
                                  ack_type, data,  size ,
                                  mid,  response_mid,  tag , copy_data, wait_for_ack);
      dump_snd_info(un_resolved_hostname, component_name, component_id, ack_type, tag, mid);
      SEC_UNLOCK_RW();
      DEXIT;
      return ret;
   }                      
#endif

   ret = cl_commlib_send_message(handle, un_resolved_hostname,  component_name,  component_id, 
                                  ack_type, data,  size ,
                                  mid,  response_mid,  tag , copy_data, wait_for_ack);
   dump_snd_info(un_resolved_hostname, component_name, component_id, ack_type, tag, mid);
   DEXIT;
   return ret;

}

/************************************************************
   COMMLIB/SECURITY WRAPPERS
   FIXME: FUNCTIONPOINTERS SHOULD BE SET IN sge_security_initialize !!!

   Test dlopen functionality, stub libs or check if openssl calls can be added 
   without infringing a copyright

   NOTES
      MT-NOTE: gdi_send_message() is MT safe (assumptions)
*************************************************************/
int gdi_send_message(
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
   int ret;
   cl_com_handle_t* handle = NULL;
   cl_xml_ack_type_t ack_type;
   unsigned long dummy_mid;
   int use_execd_handle = 0;
   DENTER(TOP_LAYER, "gdi_send_message");




   /* CR- TODO: This is for tight integration of qrsh -inherit
    *       
    *       All GDI functions normally connect to qmaster, but
    *       qrsh -inhert want's to talk to execd. A second handle
    *       is created. All gdi functions should accept a pointer
    *       to a cl_com_handle_t* handle and use this handle to
    *       send/receive messages to the correct endpoint.
    */
   if ( tocomproc[0] == '\0') {
      DEBUG((SGE_EVENT,"tocomproc is empty string\n"));
   }
   switch (uti_state_get_mewho()) {
      case QMASTER:
      case EXECD:
         use_execd_handle = 0;
         break;
      default:
         if (strcmp(tocomproc,prognames[QMASTER]) == 0) {
            use_execd_handle = 0;
         } else {
            if (tocomproc != NULL && tocomproc[0] != '\0') {
               use_execd_handle = 1;
            }
         }
   }
   
 
   if (use_execd_handle == 0) {
      /* normal gdi send to qmaster */
      DEBUG((SGE_EVENT,"standard gdi request to qmaster\n"));
      handle = cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name() ,0);
   } else {
      /* we have to send a message to another component than qmaster */
      DEBUG((SGE_EVENT,"search handle to \"%s\"\n", tocomproc));
      handle = cl_com_get_handle("execd_handle", 0);
      if (handle == NULL) {
         int commlib_error = CL_RETVAL_OK;
         DEBUG((SGE_EVENT,"creating handle to \"%s\"\n", tocomproc));
         cl_com_create_handle(&commlib_error, CL_CT_TCP, CL_CM_CT_MESSAGE, CL_FALSE, sge_get_execd_port(), CL_TCP_DEFAULT,"execd_handle" , 0 , 1 , 0 );
         handle = cl_com_get_handle("execd_handle", 0);
         if (handle == NULL) {
            ERROR((SGE_EVENT,MSG_GDI_CANT_CREATE_HANDLE_TOEXECD_S, tocomproc));
            ERROR((SGE_EVENT,cl_get_error_text(commlib_error)));
         }
      }
   }

   ack_type = CL_MIH_MAT_NAK;
   if (synchron) {
      ack_type = CL_MIH_MAT_ACK;
   }
   if (mid != NULL) {
      dummy_mid = *mid;
   }

   ret = gdi_send_sec_message( handle, 
                                  (char*)tohost ,(char*)tocomproc ,toid , 
                                  ack_type , 
                                  (cl_byte_t*)buffer ,(unsigned long)buflen,
                                  &dummy_mid , 0 ,tag,1 , synchron);
   if (ret != CL_RETVAL_OK) {
      /* try again ( if connection timed out) */
      ret = gdi_send_sec_message( handle, 
                                     (char*)tohost ,(char*)tocomproc ,toid ,
                                     ack_type , 
                                     (cl_byte_t*)buffer ,(unsigned long)buflen,
                                     &dummy_mid , 0 ,tag,1 , synchron);
   }

   if (mid != NULL) {
      *mid = dummy_mid;
   }

   DEXIT;
   return ret;
}


/* 
 *
 *  NOTES
 *     MT-NOTE: gdi_receive_message() is MT safe (major assumptions!)
 *
 */
int gdi_receive_message(
char *fromcommproc,
u_short *fromid,
char *fromhost,
int *tag,
char **buffer,
u_long32 *buflen,
int synchron,
u_short *compressed 
) {
   int ret;
   cl_com_handle_t* handle = NULL;
   cl_com_message_t* message = NULL;
   cl_com_endpoint_t* sender = NULL;
   int use_execd_handle = 0;

   DENTER(TOP_LAYER, "gdi_receive_message");

      /* CR- TODO: This is for tight integration of qrsh -inherit
    *       
    *       All GDI functions normally connect to qmaster, but
    *       qrsh -inhert want's to talk to execd. A second handle
    *       is created. All gdi functions should accept a pointer
    *       to a cl_com_handle_t* handle and use this handle to
    *       send/receive messages to the correct endpoint.
    */


   if ( fromcommproc[0] == '\0') {
      DEBUG((SGE_EVENT,"fromcommproc is empty string\n"));
   }
   switch (uti_state_get_mewho()) {
      case QMASTER:
      case EXECD:
         use_execd_handle = 0;
         break;
      default:
         if (strcmp(fromcommproc,prognames[QMASTER]) == 0) {
            use_execd_handle = 0;
         } else {
            if (fromcommproc != NULL && fromcommproc[0] != '\0') {
               use_execd_handle = 1;
            }
         }
   }

   if (use_execd_handle == 0) {
      /* normal gdi send to qmaster */
      DEBUG((SGE_EVENT,"standard gdi request to qmaster\n"));
      handle = cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name() ,0);
   } else {
      /* we have to send a message to another component than qmaster */
      DEBUG((SGE_EVENT,"search handle to \"%s\"\n", fromcommproc));
      handle = cl_com_get_handle("execd_handle", 0);
      if (handle == NULL) {
         int commlib_error = CL_RETVAL_OK;
         DEBUG((SGE_EVENT,"creating handle to \"%s\"\n", fromcommproc));
         cl_com_create_handle(&commlib_error, CL_CT_TCP, CL_CM_CT_MESSAGE, CL_FALSE, sge_get_execd_port(), CL_TCP_DEFAULT, "execd_handle" , 0 , 1 , 0 );
         handle = cl_com_get_handle("execd_handle", 0);
         if (handle == NULL) {
            ERROR((SGE_EVENT,MSG_GDI_CANT_CREATE_HANDLE_TOEXECD_S, fromcommproc));
            ERROR((SGE_EVENT,cl_get_error_text(commlib_error)));
         }
      }
   } 

   ret = gdi_receive_sec_message( handle,fromhost ,fromcommproc ,*fromid , synchron, 0 ,&message, &sender );

   if (ret == CL_RETVAL_CONNECTION_NOT_FOUND ) {
      if ( fromcommproc[0] != '\0' && fromhost[0] != '\0' ) {
          /* The connection was closed, reopen it */
          ret = cl_commlib_open_connection(handle,fromhost,fromcommproc, *fromid);
          INFO((SGE_EVENT,"reopen connection to %s,%s,"U32CFormat" (1)\n", fromhost , fromcommproc , u32c(*fromid)));
          if (ret == CL_RETVAL_OK) {
             INFO((SGE_EVENT,"reconnected successfully\n"));
             ret = gdi_receive_sec_message( handle,fromhost ,fromcommproc ,*fromid , synchron, 0 ,&message, &sender );
          } 
      } else {
         DEBUG((SGE_EVENT,"can't reopen a connection to unspecified host or commproc (1)\n"));
      }
   }

   if (message != NULL && ret == CL_RETVAL_OK) {
      *buffer = (char *)message->message;
      message->message = NULL;
      *buflen = message->message_length;
      if (tag) {
         *tag = message->message_tag;
      }
      if (compressed) {
         *compressed = 0;
      }

      if (sender != NULL) {
         DEBUG((SGE_EVENT,"received from: %s,"U32CFormat"\n",sender->comp_host, u32c(sender->comp_id)));
         if (fromcommproc != NULL && fromcommproc[0] == '\0') {
            strcpy(fromcommproc, sender->comp_name);
         }
         if (fromhost != NULL) {
            strcpy(fromhost, sender->comp_host);
         }
         if (fromid != NULL) {
            *fromid = sender->comp_id;
         }
      }
   }
   cl_com_free_message(&message);
   cl_com_free_endpoint(&sender);

   DEXIT;
   return ret;
}


/****** gdi/security/set_sec_cred() *******************************************
*  NAME
*     set_sec_cred -- get credit for security system
*
*  SYNOPSIS
*     int set_sec_cred(lListElem *job);
*
*  FUNCTION
*     Tries to get credit for a security system (DCE or KERBEROS),
*     sets the accordant information in the job structure
*     If an error occurs the return value is unequal 0
*
*  INPUTS
*     job - the job structure
*
*  RETURN
*     0  in case of success, something different otherwise 
*
*  EXAMPLE
*
*  NOTES
*     Hope, the above description is correct - don't know the 
*     DCE/KERBEROS code.
* 
*  NOTES
*     MT-NOTE: set_sec_cred() is MT safe (major assumptions!)
******************************************************************************/
int set_sec_cred(lListElem *job)
{

   pid_t command_pid;
   FILE *fp_in, *fp_out, *fp_err;
   char *str;
   int ret = 0;
   char binary[1024];
   char cmd[2048];
   char line[1024];


   DENTER(TOP_LAYER, "set_sec_cred");
   
   if (feature_is_enabled(FEATURE_AFS_SECURITY)) {
      sprintf(binary, "%s/util/get_token_cmd", path_state_get_sge_root());

      if (sge_get_token_cmd(binary, NULL) != 0) {
         fprintf(stderr, MSG_QSH_QSUBFAILED);
         SGE_EXIT(1);
      }   
      
      command_pid = sge_peopen("/bin/sh", 0, binary, NULL, NULL, &fp_in, &fp_out, &fp_err, false);

      if (command_pid == -1) {
         fprintf(stderr, MSG_QSUB_CANTSTARTCOMMANDXTOGETTOKENQSUBFAILED_S, binary);
         SGE_EXIT(1);
      }

      str = sge_bin2string(fp_out, 0);
      
      ret = sge_peclose(command_pid, fp_in, fp_out, fp_err, NULL);
      
      lSetString(job, JB_tgt, str);
   }
      
   /*
    * DCE / KERBEROS security stuff
    *
    *  This same basic code is in qsh.c and qmon_submit.c
    *  It should really be moved to a common place. It would
    *  be nice if there was a generic job submittal function.
    */

   if (feature_is_enabled(FEATURE_DCE_SECURITY) ||
       feature_is_enabled(FEATURE_KERBEROS_SECURITY)) {
      sprintf(binary, "%s/utilbin/%s/get_cred", path_state_get_sge_root(), sge_get_arch());

      if (sge_get_token_cmd(binary, NULL) != 0) {
         fprintf(stderr, MSG_QSH_QSUBFAILED);
         SGE_EXIT(1);
      }   

      sprintf(cmd, "%s %s%s%s", binary, "sge", "@", sge_get_master(0));
      
      command_pid = sge_peopen("/bin/sh", 0, cmd, NULL, NULL, &fp_in, &fp_out, &fp_err, false);

      if (command_pid == -1) {
         fprintf(stderr, MSG_QSH_CANTSTARTCOMMANDXTOGETCREDENTIALSQSUBFAILED_S, binary);
         SGE_EXIT(1);
      }

      str = sge_bin2string(fp_out, 0);

      while (!feof(fp_err)) {
         if (fgets(line, sizeof(line), fp_err))
            fprintf(stderr, "get_cred stderr: %s", line);
      }

      ret = sge_peclose(command_pid, fp_in, fp_out, fp_err, NULL);

      if (ret) {
         fprintf(stderr, MSG_QSH_CANTGETCREDENTIALS);
      }
      
      lSetString(job, JB_cred, str);
   }
   DEXIT;
   return ret;
} 

#if 0
      
      /*
      ** AFS specific things
      */
      if (feature_is_enabled(FEATURE_AFS_SECUIRITY)) {
         pid_t command_pid;
         FILE *fp_in, *fp_out, *fp_err;
         char *cp;
         int ret;
         char binary[1024];

         sprintf(binary, "%s/util/get_token_cmd", path_state_get_sge_root());

         if (sge_get_token_cmd(binary, buf))
            goto error;

         command_pid = sge_peopen("/bin/sh", 0, binary, NULL, NULL, &fp_in, &fp_out, &fp_err, false);

         if (command_pid == -1) {
            DPRINTF(("can't start command \"%s\" to get token\n",  binary));
            sprintf(buf, "can't start command \"%s\" to get token\n",  binary);
            goto error;
         }

         cp = sge_bin2string(fp_out, 0);

         ret = sge_peclose(command_pid, fp_in, fp_out, fp_err, NULL);

         lSetString(lFirst(lp), JB_tgt, cp);
      }

      /*
      ** DCE / KERBEROS security stuff
      **
      **  This same basic code is in qsh.c and qsub.c. It really should
      **  be put in a common place.
      */

      if (feature_is_enabled(FEATURE_DCE_SECURITY) ||
          feature_is_enabled(FEATURE_KERBEROS_SECURITY)) {
         pid_t command_pid;
         FILE *fp_in, *fp_out, *fp_err;
         char *str;
         char binary[1024], cmd[2048];
         int ret;
         char line[1024];

         sprintf(binary, "%s/utilbin/%s/get_cred", path_state_get_sge_root(), sge_get_arch());

         if (sge_get_token_cmd(binary, buf) != 0)
            goto error;

         sprintf(cmd, "%s %s%s%s", binary, "sge", "@", sge_get_master(0));
      
         command_pid = sge_peopen("/bin/sh", 0, cmd, NULL, NULL, &fp_in, &fp_out, &fp_err, false);

         if (command_pid == -1) {
            DPRINTF((buf, "can't start command \"%s\" to get credentials "
                     "- qsub failed\n", binary));
            sprintf(buf, "can't start command \"%s\" to get credentials "
                    "- qsub failed\n", binary);
            goto error;
         }

         str = sge_bin2string(fp_out, 0);

         while (!feof(fp_err)) {
            if (fgets(line, sizeof(line), fp_err))
               fprintf(stderr, "get_cred stderr: %s", line);
         }

         ret = sge_peclose(command_pid, fp_in, fp_out, fp_err, NULL);

         if (ret) {
            DPRINTF(("warning: could not get credentials\n"));
            sprintf(buf, "warning: could not get credentials\n");
            goto error;
         }
         
         lSetString(lFirst(lp), JB_cred, str);
      }

#endif


/*
 * 
 *  NOTES
 *     MT-NOTE: cache_sec_cred() is MT safe (assumptions)
 */
void cache_sec_cred(lListElem *jep, const char *rhost)
{
   DENTER(TOP_LAYER, "cache_sec_cred");

   /* 
    * Execute command to get DCE or Kerberos credentials.
    * 
    * This needs to be made asynchronous.
    *
    */

   if (feature_is_enabled(FEATURE_DCE_SECURITY) ||
       feature_is_enabled(FEATURE_KERBEROS_SECURITY)) {

      pid_t command_pid=-1;
      FILE *fp_in, *fp_out, *fp_err;
      char *str;
      char binary[1024], cmd[2048], ccname[256];
      int ret;
      char *env[2];

      /* set up credentials cache for this job */
      sprintf(ccname, "KRB5CCNAME=FILE:/tmp/krb5cc_qmaster_" u32,
              lGetUlong(jep, JB_job_number));
      env[0] = ccname;
      env[1] = NULL;

      sprintf(binary, "%s/utilbin/%s/get_cred", path_state_get_sge_root(), sge_get_arch());

      if (sge_get_token_cmd(binary, NULL) == 0) {
         char line[1024];

         sprintf(cmd, "%s %s%s%s", binary, "sge", "@", rhost);

         command_pid = sge_peopen("/bin/sh", 0, cmd, NULL, env, &fp_in, &fp_out, &fp_err, false);

         if (command_pid == -1) {
            ERROR((SGE_EVENT, MSG_SEC_NOSTARTCMD4GETCRED_SU, 
                   binary, u32c(lGetUlong(jep, JB_job_number))));
         }

         str = sge_bin2string(fp_out, 0);

         while (!feof(fp_err)) {
            if (fgets(line, sizeof(line), fp_err))
               ERROR((SGE_EVENT, MSG_QSH_GET_CREDSTDERR_S, line));
         }

         ret = sge_peclose(command_pid, fp_in, fp_out, fp_err, NULL);

         lSetString(jep, JB_cred, str);

         if (ret) {
            ERROR((SGE_EVENT, MSG_SEC_NOCRED_USSI, 
                   u32c(lGetUlong(jep, JB_job_number)), rhost, binary, ret));
         }
      } else {
         ERROR((SGE_EVENT, MSG_SEC_NOCREDNOBIN_US,  
                u32c(lGetUlong(jep, JB_job_number)), binary));
      }
   }
   DEXIT;
}   

/*
 * 
 *  NOTES
 *     MT-NOTE: delete_credentials() is MT safe (major assumptions!)
 * 
 */
void delete_credentials(lListElem *jep)
{

   DENTER(TOP_LAYER, "delete_credentials");

   /* 
    * Execute command to delete the client's DCE or Kerberos credentials.
    */
   if ((feature_is_enabled(FEATURE_DCE_SECURITY) ||
        feature_is_enabled(FEATURE_KERBEROS_SECURITY)) &&
        lGetString(jep, JB_cred)) {

      pid_t command_pid=-1;
      FILE *fp_in, *fp_out, *fp_err;
      char binary[1024], cmd[2048], ccname[256], ccfile[256], ccenv[256];
      int ret=0;
      char *env[2];
      char tmpstr[1024];

      /* set up credentials cache for this job */
      sprintf(ccfile, "/tmp/krb5cc_qmaster_" u32,
              lGetUlong(jep, JB_job_number));
      sprintf(ccenv, "FILE:%s", ccfile);
      sprintf(ccname, "KRB5CCNAME=%s", ccenv);
      env[0] = ccname;
      env[1] = NULL;

      sprintf(binary, "%s/utilbin/%s/delete_cred", path_state_get_sge_root(), sge_get_arch());

      if (sge_get_token_cmd(binary, NULL) == 0) {
         char line[1024];

         sprintf(cmd, "%s -s %s", binary, "sge");

         command_pid = sge_peopen("/bin/sh", 0, cmd, NULL, env, &fp_in, &fp_out, &fp_err, false);

         if (command_pid == -1) {
            strcpy(tmpstr, SGE_EVENT);
            ERROR((SGE_EVENT, MSG_SEC_STARTDELCREDCMD_SU,
                   binary, u32c(lGetUlong(jep, JB_job_number))));
            strcpy(SGE_EVENT, tmpstr);
         }

         while (!feof(fp_err)) {
            if (fgets(line, sizeof(line), fp_err)) {
               strcpy(tmpstr, SGE_EVENT);
               ERROR((SGE_EVENT, MSG_SEC_DELCREDSTDERR_S, line));
               strcpy(SGE_EVENT, tmpstr);
            }
         }

         ret = sge_peclose(command_pid, fp_in, fp_out, fp_err, NULL);

         if (ret != 0) {
            strcpy(tmpstr, SGE_EVENT);
            ERROR((SGE_EVENT, MSG_SEC_DELCREDRETCODE_USI,
                   u32c(lGetUlong(jep, JB_job_number)), binary, ret));
            strcpy(SGE_EVENT, tmpstr);
         }

      } else {
         strcpy(tmpstr, SGE_EVENT);
         ERROR((SGE_EVENT, MSG_SEC_DELCREDNOBIN_US,  
                u32c(lGetUlong(jep, JB_job_number)), binary));
         strcpy(SGE_EVENT, tmpstr);
      }
   }

   DEXIT;
}



/* 
 * Execute command to store the client's DCE or Kerberos credentials.
 * This also creates a forwardable credential for the user.
 *
 *  NOTES
 *     MT-NOTE: store_sec_cred() is MT safe (assumptions)
 */
int store_sec_cred(sge_gdi_request *request, lListElem *jep, int do_authentication, lList** alpp)
{

   DENTER(TOP_LAYER, "store_sec_cred");

   if ((feature_is_enabled(FEATURE_DCE_SECURITY) ||
        feature_is_enabled(FEATURE_KERBEROS_SECURITY)) &&
       (do_authentication || lGetString(jep, JB_cred))) {

      pid_t command_pid;
      FILE *fp_in, *fp_out, *fp_err;
      char line[1024], binary[1024], cmd[2048], ccname[256];
      int ret;
      char *env[2];

      if (do_authentication && lGetString(jep, JB_cred) == NULL) {
         ERROR((SGE_EVENT, MSG_SEC_NOAUTH_U, u32c(lGetUlong(jep, JB_job_number))));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -1;
      }

      /* set up credentials cache for this job */
      sprintf(ccname, "KRB5CCNAME=FILE:/tmp/krb5cc_qmaster_" u32,
              lGetUlong(jep, JB_job_number));
      env[0] = ccname;
      env[1] = NULL;

      sprintf(binary, "%s/utilbin/%s/put_cred", path_state_get_sge_root(), sge_get_arch());

      if (sge_get_token_cmd(binary, NULL) == 0) {
         sprintf(cmd, "%s -s %s -u %s", binary, "sge", lGetString(jep, JB_owner));

         command_pid = sge_peopen("/bin/sh", 0, cmd, NULL, env, &fp_in, &fp_out, &fp_err, false);

         if (command_pid == -1) {
            ERROR((SGE_EVENT, MSG_SEC_NOSTARTCMD4GETCRED_SU,
                   binary, u32c(lGetUlong(jep, JB_job_number))));
         }

         sge_string2bin(fp_in, lGetString(jep, JB_cred));

         while (!feof(fp_err)) {
            if (fgets(line, sizeof(line), fp_err))
               ERROR((SGE_EVENT, MSG_SEC_PUTCREDSTDERR_S, line));
         }

         ret = sge_peclose(command_pid, fp_in, fp_out, fp_err, NULL);

         if (ret) {
            ERROR((SGE_EVENT, MSG_SEC_NOSTORECRED_USI,
                   u32c(lGetUlong(jep, JB_job_number)), binary, ret));
         }

         /*
          * handle authentication failure
          */

         if (do_authentication && (ret != 0)) {
            ERROR((SGE_EVENT, MSG_SEC_NOAUTH_U, u32c(lGetUlong(jep, JB_job_number))));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            DEXIT;
            return -1;
         }

      } else {
         ERROR((SGE_EVENT, MSG_SEC_NOSTORECREDNOBIN_US, 
                u32c(lGetUlong(jep, JB_job_number)), binary));
      }
   }
#ifdef KERBEROS

   /* get client TGT and store in job entry */

   {
      krb5_error_code rc;
      krb5_creds ** tgt_creds = NULL;
      krb5_data outbuf;

      outbuf.length = 0;

      if (krb_get_tgt(request->host, request->commproc, request->id,
		      request->request_id, &tgt_creds) == 0) {
      
	 if ((rc = krb_encrypt_tgt_creds(tgt_creds, &outbuf))) {
	    ERROR((SGE_EVENT, MSG_SEC_KRBENCRYPTTGT_SSIS, 
            request->host, request->commproc, request->id, error_message(rc)));
	 }

	 if (rc == 0)
	    lSetString(jep, JB_tgt,
                       krb_bin2str(outbuf.data, outbuf.length, NULL));

	 if (outbuf.length)
	    krb5_xfree(outbuf.data);

         /* get rid of the TGT credentials */
         krb_put_tgt(request->host, request->commproc, request->id,
		     request->request_id, NULL);

      }
   }

#endif

   return 0;
}   




/*
 *
 *  NOTES
 *     MT-NOTE: store_sec_cred2() is MT safe (assumptions)
 */
int store_sec_cred2(lListElem *jelem, int do_authentication, int *general, char* err_str)
{
   int ret = 0;
   const char *cred;
   
   DENTER(TOP_LAYER, "store_sec_cred2");

   if ((feature_is_enabled(FEATURE_DCE_SECURITY) ||
        feature_is_enabled(FEATURE_KERBEROS_SECURITY)) &&
       (cred = lGetString(jelem, JB_cred)) && cred[0]) {

      pid_t command_pid;
      FILE *fp_in, *fp_out, *fp_err;
      char binary[1024], cmd[2048], ccname[256], ccfile[256], ccenv[256],
           jobstr[64];
      int ret;
      char *env[3];
      lListElem *vep;

      /* set up credentials cache for this job */
      sprintf(ccfile, "/tmp/krb5cc_%s_" u32, "sge", lGetUlong(jelem, JB_job_number));
      sprintf(ccenv, "FILE:%s", ccfile);
      sprintf(ccname, "KRB5CCNAME=%s", ccenv);
      sprintf(jobstr, "JOB_ID="u32, lGetUlong(jelem, JB_job_number));
      env[0] = ccname;
      env[1] = jobstr;
      env[2] = NULL;
      vep = lAddSubStr(jelem, VA_variable, "KRB5CCNAME", JB_env_list, VA_Type);
      lSetString(vep, VA_value, ccenv);

      sprintf(binary, "%s/utilbin/%s/put_cred", path_state_get_sge_root(),
              sge_get_arch());

      if (sge_get_token_cmd(binary, NULL) == 0) {
         char line[1024];

         sprintf(cmd, "%s -s %s -u %s -b %s", binary, "sge",
                 lGetString(jelem, JB_owner), lGetString(jelem, JB_owner));

         command_pid = sge_peopen("/bin/sh", 0, cmd, NULL, env, &fp_in, &fp_out, &fp_err, false);

         if (command_pid == -1) {
            ERROR((SGE_EVENT, MSG_SEC_NOSTARTCMD4GETCRED_SU, binary, u32c(lGetUlong(jelem, JB_job_number))));
         }

         sge_string2bin(fp_in, lGetString(jelem, JB_cred));

         while (!feof(fp_err)) {
            if (fgets(line, sizeof(line), fp_err))
               ERROR((SGE_EVENT, MSG_SEC_PUTCREDSTDERR_S, line));
         }

         ret = sge_peclose(command_pid, fp_in, fp_out, fp_err, NULL);

         if (ret) {
            ERROR((SGE_EVENT, MSG_SEC_NOSTORECRED_USI, u32c(lGetUlong(jelem, JB_job_number)), binary, ret));
         }

         /*
          * handle authentication failure
          */                                                  
                                                              
         if (do_authentication && (ret != 0)) {               
            ERROR((SGE_EVENT, MSG_SEC_KRBAUTHFAILURE,
                   u32c(lGetUlong(jelem, JB_job_number))));         
            sprintf(err_str, MSG_SEC_KRBAUTHFAILUREONHOST,
                    u32c(lGetUlong(jelem, JB_job_number)),
                    uti_state_get_unqualified_hostname());                 
            *general = GFSTATE_JOB;                            
         }                                                    
      } 
      else {
         ERROR((SGE_EVENT, MSG_SEC_NOSTORECREDNOBIN_US, u32c(lGetUlong(jelem, JB_job_number)), binary));
      }
   }
   DEXIT;
   return ret;
}

#ifdef KERBEROS
/*
 *
 *  NOTES
 *     MT-NOTE: kerb_job() is not MT safe
 */
int kerb_job(
lListElem *jelem,
struct dispatch_entry *de 
) {
   /* get TGT and store in job entry and in user's credentials cache */
   krb5_error_code rc;
   krb5_creds ** tgt_creds = NULL;
   krb5_data outbuf;

   DENTER(TOP_LAYER, "kerb_job");

   outbuf.length = 0;

   if (krb_get_tgt(de->host, de->commproc, de->id, lGetUlong(jelem, JB_job_number), &tgt_creds) == 0) {
      struct passwd *pw;

      if ((rc = krb_encrypt_tgt_creds(tgt_creds, &outbuf))) {
         ERROR((SGE_EVENT, MSG_SEC_KRBENCRYPTTGTUSER_SUS, lGetString(jelem, JB_owner),
                u32c(lGetUlong(jelem, JB_job_number)), error_message(rc)));
      }

      if (rc == 0)
         lSetString(jelem, JB_tgt, krb_bin2str(outbuf.data, outbuf.length, NULL));

      if (outbuf.length)
         krb5_xfree(outbuf.data);

      pw = sge_getpwnam(lGetString(jelem, JB_owner));

      if (pw) {
         if (krb_store_forwarded_tgt(pw->pw_uid,
               lGetUlong(jelem, JB_job_number),
               tgt_creds) == 0) {
            char ccname[40];
            lListElem *vep;

            krb_get_ccname(lGetUlong(jelem, JB_job_number), ccname);
            vep = lAddSubStr(jelem, VA_variable, "KRB5CCNAME", JB_env_list, VA_Type);
            lSetString(vep, VA_value, ccname);
         }

      } else {
         ERROR((SGE_EVENT, MSG_SEC_NOUID_SU, lGetString(jelem, JB_owner), u32c(lGetUlong(jelem, JB_job_number))));
      }

      /* clear TGT out of client entry (this frees the TGT credentials) */
      krb_put_tgt(de->host, de->commproc, de->id, lGetUlong(jelem, JB_job_number), NULL);
   }
   
   DEXIT;
   return 0;
}
#endif


/* 
 *  FUNCTION
 *     get TGT from job entry and store in client connection 
 *
 *  NOTES
 *     MT-NOTE: tgt2cc() is not MT safe (assumptions)
 */
void tgt2cc(lListElem *jep, const char *rhost, const char* target)
{

#ifdef KERBEROS
   krb5_error_code rc;
   krb5_creds ** tgt_creds = NULL;
   krb5_data inbuf;
   char *tgtstr = NULL;
   u_long32 jid = 0;
   
   DENTER(TOP_LAYER, "tgt2cc");
   inbuf.length = 0;
   jid = lGetUlong(jep, JB_job_number);
   
   if ((tgtstr = lGetString(jep, JB_tgt))) { 
      inbuf.data = krb_str2bin(tgtstr, NULL, &inbuf.length);
      if (inbuf.length) {
         if ((rc = krb_decrypt_tgt_creds(&inbuf, &tgt_creds))) {
            ERROR((SGE_EVENT, MSG_SEC_KRBDECRYPTTGT_US, u32c(jid),
                   error_message(rc)));
         }
      }
      if (rc == 0)
         if (krb_put_tgt(rhost, target, 0, jid, tgt_creds) == 0) {
            krb_set_tgt_id(jid);
 
            tgt_creds = NULL;
         }

      if (inbuf.length)
         krb5_xfree(inbuf.data);

      if (tgt_creds)
         krb5_free_creds(krb_context(), *tgt_creds);
   }

   DEXIT;
#endif

}


/*
 *
 *  NOTES
 *     MT-NOTE: tgtcclr() is MT safe (assumptions)
 */
void tgtcclr(lListElem *jep, const char *rhost, const char* target)
{
#ifdef KERBEROS

   /* clear client TGT */
   krb_put_tgt(rhost, target, 0, lGetUlong(jep, JB_job_number), NULL);
   krb_set_tgt_id(0);

#endif
}


/*
** authentication information
**
** NOTES
**    MT-NOTE: sge_set_auth_info() is MT safe (assumptions)
**    MT-NOTE: sge_set_auth_info() is not MT safe when -DCRYPTO is set
**
*/
int sge_set_auth_info(sge_gdi_request *request, uid_t uid, char *user, 
                        gid_t gid, char *group)
{
   char buffer[SGE_SEC_BUFSIZE];
   char obuffer[3*SGE_SEC_BUFSIZE];

   DENTER(TOP_LAYER, "sge_set_auth_info");

   sprintf(buffer, pid_t_fmt" "pid_t_fmt" %s %s", uid, gid, user, group);
   if (!sge_encrypt(buffer, sizeof(buffer), obuffer, sizeof(obuffer))) {
      DEXIT;
      return -1;
   }   

   request->auth_info = sge_strdup(NULL, obuffer);

   DEXIT;
   return 0;
}

/*
** NOTES
**    MT-NOTE: sge_decrypt() is MT safe (assumptions)
*/
int sge_get_auth_info(sge_gdi_request *request, uid_t *uid, char *user, 
                        gid_t *gid, char *group)
{
   char dbuffer[2*SGE_SEC_BUFSIZE];
   int dlen = 0;

   DENTER(TOP_LAYER, "sge_get_auth_info");

   if (!sge_decrypt(request->auth_info, strlen(request->auth_info), dbuffer, &dlen)) {
      DEXIT;
      return -1;
   }   

   if (sscanf(dbuffer, pid_t_fmt" "pid_t_fmt" %s %s", uid, gid, user, group) != 4) {
      DEXIT;
      return -1;
   }   

   DEXIT;
   return 0;
}


#ifndef CRYPTO
/*
** standard encrypt/decrypt functions
**
** MT-NOTE: sge_encrypt() is MT safe
*/
static bool sge_encrypt(char *intext, int inlen, char *outbuf, int outsize)
{
   int len;

   DENTER(TOP_LAYER, "sge_encrypt");

/*    DPRINTF(("======== intext:\n"SFN"\n=========\n", intext)); */

   len = strlen(intext);
   if (!change_encoding(outbuf, &outsize, (unsigned char*) intext, &len, ENCODE_TO_STRING)) {
      DEXIT;
      return false;
   }   

/*    DPRINTF(("======== outbuf:\n"SFN"\n=========\n", outbuf)); */

   DEXIT;
   return true;
}

/*
** MT-NOTE: standard sge_decrypt() is MT safe
*/
static bool sge_decrypt(char *intext, int inlen, char *outbuf, int* outsize)
{
   unsigned char decbuf[2*SGE_SEC_BUFSIZE];
   int declen = sizeof(decbuf);

   DENTER(TOP_LAYER, "sge_decrypt");

   if (!change_encoding(intext, &inlen, decbuf, &declen, DECODE_FROM_STRING)) {
      return false;
   }   
   decbuf[declen] = '\0';

   strcpy(outbuf, (char*)decbuf);

/*    DPRINTF(("======== outbuf:\n"SFN"\n=========\n", outbuf)); */

   DEXIT;
   return true;
}

#else

/*
** MT-NOTE: EVP based sge_encrypt() is not MT safe
*/
static bool sge_encrypt(char *intext, int inlen, char *outbuf, int outsize)
{

   int enclen, tmplen;
   unsigned char encbuf[2*SGE_SEC_BUFSIZE];

   unsigned char key[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
   unsigned char iv[] = {1,2,3,4,5,6,7,8};
   EVP_CIPHER_CTX ctx;

   DENTER(TOP_LAYER, "sge_encrypt");

/*    DPRINTF(("======== intext:\n"SFN"\n=========\n", intext)); */

   if (!EVP_EncryptInit(&ctx, /*EVP_enc_null() EVP_bf_cbc()*/EVP_cast5_ofb(), key, iv)) {
      printf("EVP_EncryptInit failure !!!!!!!\n");
      DEXIT;
      return false;
   }   

   if (!EVP_EncryptUpdate(&ctx, encbuf, &enclen, (unsigned char*) intext, inlen)) {
      DEXIT;
      return false;
   }

   if (!EVP_EncryptFinal(&ctx, encbuf + enclen, &tmplen)) {
      DEXIT;
      return false;
   }
   enclen += tmplen;
   EVP_CIPHER_CTX_cleanup(&ctx);

   if (!change_encoding(outbuf, &outsize, encbuf, &enclen, ENCODE_TO_STRING)) {
      DEXIT;
      return false;
   }   

/*    DPRINTF(("======== outbuf:\n"SFN"\n=========\n", outbuf)); */

   DEXIT;
   return true;
}

static bool sge_decrypt(char *intext, int inlen, char *outbuf, int* outsize)
{

   int outlen, tmplen;
   unsigned char decbuf[2*SGE_SEC_BUFSIZE];
   int declen = sizeof(decbuf);

   unsigned char key[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
   unsigned char iv[] = {1,2,3,4,5,6,7,8};
   EVP_CIPHER_CTX ctx;

   DENTER(TOP_LAYER, "sge_decrypt");

   if (!change_encoding(intext, &inlen, decbuf, &declen, DECODE_FROM_STRING)) {
      DEXIT;
      return false;
   }   

   if (!EVP_DecryptInit(&ctx, /* EVP_enc_null() EVP_bf_cbc()*/EVP_cast5_ofb(), key, iv)) {
      DEXIT;
      return false;
   }
   
   if (!EVP_DecryptUpdate(&ctx, (unsigned char*)outbuf, &outlen, decbuf, declen)) {
      DEXIT;
      return false;
   }

   if (!EVP_DecryptFinal(&ctx, (unsigned char*)outbuf + outlen, &tmplen)) {
      DEXIT;
      return false;
   }
   EVP_CIPHER_CTX_cleanup(&ctx);

   *outsize = outlen+tmplen;

/*    DPRINTF(("======== outbuf:\n"SFN"\n=========\n", outbuf)); */

   DEXIT;
   return true;
}

#endif


#define LOQUAD(i) (((i)&0x0F))
#define HIQUAD(i) (((i)&0xF0)>>4)
#define SETBYTE(hi, lo)  ((((hi)<<4)&0xF0) | (0x0F & (lo)))

/*
 *
 * NOTES
 *    MT-NOTE: change_encoding() is MT safe
 *
 */
static bool change_encoding(char *cbuf, int* csize, unsigned char* ubuf, int* usize, int mode)
{
   static const char alphabet[16] = {"*b~de,gh&j§lrn=p"};

   DENTER(TOP_LAYER, "change_encoding");

   if (mode == ENCODE_TO_STRING) {
      /*
      ** encode to string
      */
      int i, j;
      int enclen = *usize;
      if ((*csize) < (2*enclen+1)) {
         DEXIT;
         return false;
      }

      for (i=0,j=0; i<enclen; i++) {
         cbuf[j++] = alphabet[HIQUAD(ubuf[i])];
         cbuf[j++] = alphabet[LOQUAD(ubuf[i])];
      }
      cbuf[j] = '\0';
   }

   if (mode == DECODE_FROM_STRING) {
      /*
      ** decode from string
      */
      char *p;
      int declen;
      if ((*usize) < (*csize)) {
         DEXIT;
         return false;
      }
      for (p=cbuf, declen=0; *p; p++, declen++) {
         int hi, lo, j;
         for (j=0; j<16; j++) {
            if (*p == alphabet[j]) 
               break;
         }
         hi = j;
         p++;
         for (j=0; j<16; j++) {
            if (*p == alphabet[j]) 
               break;
         }
         lo = j;
         ubuf[declen] = (unsigned char) SETBYTE(hi, lo);
      }   
      *usize = declen;
   }
      
   DEXIT;   
   return true;
}

/* MT-NOTE: sge_security_verify_user() is MT safe (assumptions) */
int sge_security_verify_user(const char *host, const char *commproc, u_short id, const char *user) 
{
   DENTER(TOP_LAYER, "sge_security_verify_user");

#ifdef SECURE
   if (feature_is_enabled(FEATURE_CSP_SECURITY)) {
      if (!sec_verify_user(user, commproc)) {
         DEXIT;
         return False;
     }
   }  
#endif

#ifdef KERBEROS

   if (krb_verify_user(host, commproc, id, user) < 0) {
      DEXIT;
      return False;
   }

#endif /* KERBEROS */

   DEXIT;
   return True;
}   

/* MT-NOTE: sge_security_ck_to_do() is MT safe (assumptions) */
void sge_security_event_handler(te_event_t anEvent)
{
#ifdef SECURE
   if (feature_is_enabled(FEATURE_CSP_SECURITY)) {
      sec_clear_connectionlist();
   }   
#endif
   
#ifdef KERBEROS
   krb_check_for_idle_clients();
#endif
}

