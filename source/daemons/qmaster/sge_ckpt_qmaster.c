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

#include "sge.h"
#include "utility.h"
#include "def.h"
#include "sge_peL.h"
#include "sge_ckptL.h"
#include "sge_jobL.h"
#include "sge_queueL.h"
#include "sge_jataskL.h"
#include "sge_eventL.h"
#include "sge_answerL.h"
#include "sge_usersetL.h"
#include "sge_ckpt_qmaster.h"
#include "job_log.h"
#include "sge_queue_qmaster.h"
#include "sge_host_qmaster.h"
#include "read_write_queue.h"
#include "read_write_ckpt.h"
#include "sge_m_event.h"
#include "config_file.h"
#include "sge_userset_qmaster.h"
#include "sge_me.h"
#include "sge_signal.h"
#include "sge_prognames.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_job_schedd.h"
#include "gdi_utility_qmaster.h"
#include "sge_stdlib.h"
#include "msg_common.h"
#include "msg_utilib.h"
#include "msg_qmaster.h"

/* #include "pw_def.h" */
/* #include "pw_proto.h"  */
#include "sge_parse_num_par.h"

extern lList *Master_Ckpt_List;

/****** qmaster/ckpt/ckpt_mod() ***********************************************
*  NAME
*     ckpt_mod -- add/modify ckpt object in Master_Ckpt_List 
*
*  SYNOPSIS
*     int ckpt_mod (lList **alpp, lListElem *new_ckpt, lListElem *ckpt, 
*                   int add, char *ruser, char *rhost, gdi_object_t *object,
*                   int sub_command);
*
*  FUNCTION
*     This function will be called from the framework which will
*     add/modify/delete generic gdi objects.
*     The purpose of this function is it to add new ckpt
*     objects or modify existing checkpointing interfaces. 
*
*
*  INPUTS
*     alpp        - reference to an answer list.
*     new_ckpt    - if a new ckpt object will be created by this 
*                   function, than new_ckpt is new uninitialized
*                   CULL object
*                   if this function was called due to a modify request
*                   than new_ckpt will contain the old data
*                   (see add parameter)
*     ckpt        - a reduced ckpt object which contains all
*                   necessary information to create a new object
*                   or modify parts of an existing one
*     add         - 1 if a new element should be added to the master list 
*                   0 to modify an existing object
*     ruser       - username of person who invoked this gdi request
*     rhost       - hostname of the host where someone initiated an gdi call
*     object      - structure of the gdi framework which contains 
*                   additional information to perform the request
*                   (function pointers, names, CULL-types) 
*     sub_command - how should we handle sublist elements
*              SGE_GDI_CHANGE - modify sublist elements
*              SGE_GDI_APPEND - add elements to a sublist
*              SGE_GDI_REMOVE - remove sublist elements
*              SGE_GDI_SET - replace the complete sublist    
*
*  RESULT
*     [alpp] - error messages will be added to this list
*     0 - success
*     STATUS_EUNKNOWN - an error occured
******************************************************************************/ 
int ckpt_mod(lList **alpp, lListElem *new_ckpt, lListElem *ckpt, int add,
             char *ruser, char *rhost, gdi_object_t *object, int sub_command) 
{
   const char *ckpt_name;

   DENTER(TOP_LAYER, "ckpt_mod");

   /* ---- CK_name */
   if (lGetPosViaElem(ckpt, CK_name) >= 0) {
      if (add) {
         if (attr_mod_str(alpp, ckpt, new_ckpt, CK_name, SGE_ATTR_CKPT_NAME)) {
            goto ERROR;
         }
      }
      ckpt_name = lGetString(new_ckpt, CK_name);
      if (add && verify_str_key(alpp, ckpt_name, SGE_ATTR_CKPT_NAME)) {
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   } else {
      ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(CK_name), SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0); 
      goto ERROR;
   }
   
   /* ---- CK_interface */
   attr_mod_str(alpp, ckpt, new_ckpt, CK_interface, SGE_ATTR_INTERFACE);

   /* ---- CK_ckpt_command */
   attr_mod_str(alpp, ckpt, new_ckpt, CK_ckpt_command, SGE_ATTR_CKPT_COMMAND);

   /* ---- CK_migr_command */
   attr_mod_str(alpp, ckpt, new_ckpt, CK_migr_command, SGE_ATTR_MIGR_COMMAND);

   /* ---- CK_rest_command */
   attr_mod_str(alpp, ckpt, new_ckpt, CK_rest_command, 
         SGE_ATTR_RESTART_COMMAND);

   /* ---- CK_ckpt_dir */
   attr_mod_str(alpp, ckpt, new_ckpt, CK_ckpt_dir, SGE_ATTR_CKPT_DIR);
  
   /* ---- CK_when */
   if (lGetPosViaElem(ckpt, CK_when) >= 0) {
      int new_flags, flags;

      new_flags = sge_parse_checkpoint_attr(lGetString(new_ckpt, CK_when));
      flags = sge_parse_checkpoint_attr(lGetString(ckpt, CK_when));

      if (sub_command == SGE_GDI_APPEND || sub_command == SGE_GDI_CHANGE) {
         new_flags |= flags;
      } else if (sub_command == SGE_GDI_REMOVE) {
         new_flags &= (~flags);
      } else if (sub_command == SGE_GDI_SET) {
         new_flags = flags;
      }
      if (is_checkpoint_when_valid(new_flags)) { 
         lSetString(new_ckpt, CK_when, get_checkpoint_when(new_flags));
      } else {
         ERROR((SGE_EVENT, MSG_CKPT_INVALIDWHENATTRIBUTE_S, ckpt_name));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         goto ERROR;
      }
   } 

   /* ---- CK_signal */
   attr_mod_str(alpp, ckpt, new_ckpt, CK_signal, SGE_ATTR_SIGNAL);

   /* ---- CK_clean_command */
   attr_mod_str(alpp, ckpt, new_ckpt, CK_clean_command, SGE_ATTR_CLEAN_COMMAND);

   /* ---- CK_job_pid */
   attr_mod_ulong(ckpt, new_ckpt, CK_job_pid, "job_pid"); 

   /* ---- CK_queue_list */
   attr_mod_sub_list(alpp, new_ckpt, CK_queue_list, 
      QR_name, ckpt, sub_command, SGE_ATTR_QUEUE_LIST, SGE_OBJ_CKPT, 0);  

   /* validate ckpt data */
   if (validate_ckpt(new_ckpt, alpp) != STATUS_OK) {
      goto ERROR;
   }

   DEXIT;
   return 0;

ERROR:
   DEXIT;
   return STATUS_EUNKNOWN;
}

/****** qmaster/ckpt/ckpt_spool() *********************************************
*
*  NAME
*     ckpt_spool -- spool a ckpt object  
*
*  SYNOPSIS
*     int ckpt_spool(lList **alpp, lListElem *ep, gdi_object_t *object);
*
*  FUNCTION
*     This function will be called from the framework which will
*     add/modify/delete generic gdi objects.
*     After an object was modified/added successfully it
*     is necessary to spool the current state to the filesystem.
*
*
*  INPUTS
*     alpp        - reference to an answer list.
*     ep          - ckpt object which should be spooled
*     object      - structure of the gdi framework which contains 
*                   additional information to perform the request
*                   (function pointers, names, CULL-types) 
*
*  RESULT
*     [alpp] - error messages will be added to this list
*     0 - success
*     STATUS_EEXIST - an error occured
******************************************************************************/
int ckpt_spool(lList **alpp, lListElem *ep, gdi_object_t *object) 
{
   DENTER(TOP_LAYER, "ckpt_spool");

   if (!write_ckpt(1, 2, ep)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS, 
            object->object_name, lGetString(ep, CK_name)));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }
   DEXIT;
   return 0;
}

/****** qmaster/ckpt/ckpt_success() *******************************************
*
*  NAME
*     ckpt_success -- does something after an successfull modify
*
*  SYNOPSIS
*     int ckpt_success(lListElem *ep; lListElem *old_ep; gdi_object_t *object);
*
*  FUNCTION
*     This function will be called from the framework which will
*     add/modify/delete generic gdi objects.
*     After an object was modified/added and spooled successfully 
*     it is possibly necessary to perform additional tasks.
*     For example it is necessary to send some events to
+     other deamon.
*
*
*  INPUTS
*     ep          - new ckpt object 
*     old_ep      - old ckpt object before modification or
*                   NULL if a new object was added
*     object      - structure of the gdi framework which contains 
*                   additional information to perform the request
*                   (function pointers, names, CULL-types) 
*
*  RESULT
*     0 - success
******************************************************************************/ 
int ckpt_success(lListElem *ep, lListElem *old_ep, gdi_object_t *object) 
{
   const char *ckpt_name;

   DENTER(TOP_LAYER, "ckpt_success");

   ckpt_name = lGetString(ep, CK_name);

   sge_change_queue_version_qr_list(lGetList(ep, CK_queue_list), 
         old_ep ? lGetList(old_ep, CK_queue_list) : NULL,
         MSG_OBJ_CKPTI, ckpt_name);

   sge_add_event(NULL, old_ep?sgeE_CKPT_MOD:sgeE_CKPT_ADD, 0, 0, ckpt_name, ep);

   DEXIT;
   return 0;
}

/****** qmaster/ckpt/sge_del_ckpt() *******************************************
*
*  NAME
*     sge_del_ckpt -- delete ckpt object in Master_Ckpt_List 
*
*  SYNOPSIS
*     int sge_del_ckpt(lListElem *ep, lList **alpp, char *ruser, char *rhost);
* 
*  FUNCTION
*     This function will be called from the framework which will
*     add/modify/delete generic gdi objects.
*     The purpose of this function is it to delete ckpt objects. 
*
*
*  INPUTS
*     ep          - element which should be deleted
*     alpp        - reference to an answer list.
*     ruser       - username of person who invoked this gdi request
*     rhost       - hostname of the host where someone initiated an gdi call
*
*  RESULT
*     [alpp] - error messages will be added to this list
*     0 - success
*     STATUS_EUNKNOWN - an error occured
******************************************************************************/ 
int sge_del_ckpt(lListElem *ep, lList **alpp, char *ruser, char *rhost) 
{
   lListElem *found;
   int pos;
   const char *ckpt_name;
   lList **lpp = &Master_Ckpt_List;

   DENTER(TOP_LAYER, "sge_del_ckpt");

   if ( !ep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* ep is no ckpt element, if ep has no CK_name */
   if ((pos = lGetPosViaElem(ep, CK_name)) < 0) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(CK_name), SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   ckpt_name = lGetPosString(ep, pos);
   if (!ckpt_name) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }                    
   found = lGetElemStr(*lpp, CK_name, ckpt_name);

   if (!found) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, MSG_OBJ_CKPT, ckpt_name));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* remove ckpt file 1st */
   if (sge_unlink(CKPTOBJ_DIR, ckpt_name)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS, MSG_OBJ_CKPT, ckpt_name));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EDISK;
   }

   sge_add_event(NULL, sgeE_CKPT_DEL, 0, 0, ckpt_name, NULL);
   sge_change_queue_version_qr_list(lGetList(found, CK_queue_list),
      NULL, "checkpoint interface", ckpt_name);

   /* now we can remove the element */
   lRemoveElem(*lpp, found);

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS,
            ruser, rhost, ckpt_name, MSG_OBJ_CKPT));
   sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   DEXIT;
   return STATUS_OK;
}                     


/****** qmaster/ckpt/sge_locate_ckpt() ****************************************
*
*  NAME
*     sge_locate_ckpt -- find a ckpt object in the Master_Ckpt_List 
*
*  SYNOPSIS
*     int sge_locate_ckpt(
*        char *ckpt_name;
*     );
*
*  FUNCTION
*     This function will return a ckpt object by name if it exists.
*
*
*  INPUTS
*     ckpt_name   - name of the ckpt object. 
*
*  RESULT
*     NULL - ckpt object with name "ckpt_name" does not exist
*     !NULL - pointer to the cull element (CK_Type) 
******************************************************************************/ 
lListElem *sge_locate_ckpt(const char *ckpt_name) {
   return lGetElemStr(Master_Ckpt_List, CK_name, ckpt_name);
}

/****** src/sge_change_queue_version_qr_list() ********************************
*  NAME
*     sge_change_queue_version_qr_list --  
*
*  SYNOPSIS
*     int sge_change_queue_version_qr_list(
*        lList *nq; 
*        lList *oq; 
*        char *obj_name;
*        char *ckpt_name;  
*     );
*
*  FUNCTION
*
*
*  INPUTS
*     nq - QR_Type
*     oq - QR_Type
*     obj_name -
*     ckpt_name -
*
*  RESULT
*
*  EXAMPLE
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*******************************************************************************/         
void sge_change_queue_version_qr_list(lList *nq, lList *oq, 
                                      const char *obj_name,
                                      const char *ckpt_name) 
{
   const char *q_name;
   lListElem *qrep, *qep;

   DENTER(TOP_LAYER, "sge_change_queue_version_qr_list");

   /*
      change version of all queues in new queue list of new ckpt interface
   */
   for_each (qrep, nq) {
      q_name = lGetString(qrep, QR_name);
      if ((qep = sge_locate_queue(q_name))) {
         sge_change_queue_version(qep, 0, 0);
         cull_write_qconf(1, 0, QUEUE_DIR, lGetString(qep, QU_qname), NULL, 
            qep);
         DPRINTF(("increasing version of queue \"%s\" because %s"
            " \"%s\" has changed\n", q_name, obj_name, ckpt_name));
      }
   }

   /*
      change version of all queues in
      old queue list and not in the new one
   */
   for_each (qrep, oq) {
      q_name = lGetString(qrep, QR_name);
      if (!lGetElemStr(nq, QR_name, q_name) 
          && (qep = sge_locate_queue(q_name))) {
         sge_change_queue_version(qep, 0, 0);
         cull_write_qconf(1, 0, QUEUE_DIR, lGetString(qep, QU_qname), NULL, 
            qep);
         DPRINTF(("increasing version of queue \"%s\" because %s"
               " \"%s\" has changed\n", q_name, obj_name, ckpt_name));
      }
   }

   DEXIT;
   return;
}                 

/****** qmaster/ckpt/validate_ckpt() ******************************************
*  NAME
*     validate_ckpt -- validate all ckpt interface parameters 
*
*  SYNOPSIS
*     int validate_ckpt(lListElem *ep, lList **alpp);
*
*  FUNCTION
*     This function will test all ckpt interface parameters.
*     If all are valid then it will return successfull.
*
*
*  INPUTS
*     ep     - element which sould be verified. 
*     answer - answer list where the function stored error messages
*
*
*  RESULT
*     [answer] - error messages will be added to this list
*     STATUS_OK - success
*     STATUS_EUNKNOWN or STATUS_EEXIST - error
******************************************************************************/ 
int validate_ckpt(lListElem *ep, lList **alpp) 
{
   static char* ckpt_interfaces[] = {
      "USERDEFINED",
      "HIBERNATOR",
      "TRANSPARENT",
      "APPLICATION-LEVEL",
      "CPR",
      "CRAY-CKPT"
   };
   static struct attr {
      int nm;
      char *text;
   } ckpt_commands[] = {
      { CK_ckpt_command, "ckpt_command" },
      { CK_migr_command, "migr_command" },
      { CK_rest_command, "restart_command"},
      { CK_clean_command, "clean_command"},
      { NoName,           NULL} };

   int i;
   int found = 0;
   const char *s, *interface;

   DENTER(TOP_LAYER, "validate_ckpt_obj");

   if (!ep) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* -------- CK_name */
   if (verify_str_key(alpp, lGetString(ep, CK_name), "checkpoint interface")) {
      DEXIT;
      return STATUS_EUNKNOWN;
   }          

   /*
   ** check if ckpt obj can be added
   ** check allowed interfaces and license
   */
   if ((interface = lGetString(ep, CK_interface))) {
      found = 0;
      for (i=0; i < (sizeof(ckpt_interfaces)/sizeof(char*)); i++) {
         if (!strcasecmp(interface, ckpt_interfaces[i])) {
            found = 1;
            break;
         }
      }

      if (!found) {
         ERROR((SGE_EVENT, MSG_SGETEXT_NO_INTERFACE_S, interface));
         sge_add_answer(alpp, SGE_EVENT, STATUS_ESEMANTIC, 0);
         DEXIT;
         return STATUS_EEXIST;
      }
      
#ifdef PW
      /* license check */
      if (!set_licensed_feature("ckpt")) {
         if (!strcasecmp(interface, "HIBERNATOR") ||
             !strcasecmp(interface, "CPR") ||
             !strcasecmp(interface, "APPLICATION-LEVEL") ||
             !strcasecmp(interface, "CRAY-CKPT")) {
            ERROR((SGE_EVENT, MSG_SGETEXT_NO_CKPT_LIC));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
            DEXIT;
            return STATUS_EEXIST;
         }
      }
#endif      
   }                     

   for (i=0; ckpt_commands[i].nm!=NoName; i++) {
      if (replace_params(lGetString(ep, ckpt_commands[i].nm),
               NULL, 0, ckpt_variables)) {
         ERROR((SGE_EVENT, MSG_OBJ_CKPTENV,
               ckpt_commands[i].text, lGetString(ep, CK_name), err_msg));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
         DEXIT;
         return STATUS_EEXIST;
      }
   }

   /* -------- CK_queue_list */
   if (verify_qr_list(alpp, lGetList(ep, CK_queue_list), MSG_OBJ_QLIST,
               MSG_OBJ_CKPTI, lGetString(ep, CK_name))!=STATUS_OK) {
      DEXIT;
      return STATUS_EEXIST;
   }

   /* -------- CK_signal */
   if ((s=lGetString(ep, CK_signal)) &&
         strcasecmp(s, "none") &&
         sys_str2signal(s)==-1) {
      ERROR((SGE_EVENT, MSG_CKPT_XISNOTASIGNALSTRING_S , s));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   DEXIT;
   return STATUS_OK;
}              
