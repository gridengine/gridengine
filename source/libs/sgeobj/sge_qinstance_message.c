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

#include "sgermon.h"
#include "sge_string.h"
#include "sge_log.h"
#include "cull_list.h"
#include "sge.h"

#include "sge_answer.h"
#include "sge_qinstance_message.h"
#include "msg_sgeobjlib.h"

/* EB: ADOC: add commets */

static bool
qim_list_add(lList **this_list, u_long32 type, const char *message);

static bool
qim_list_trash_all_of_type_X(lList **this_list, u_long32 type);

static bool
qim_list_add(lList **this_list, u_long32 type, const char *message) 
{
   bool ret = true;

   DENTER(TOP_LAYER, "qim_list_add");
   if (this_list != NULL && message != NULL) {
      lListElem *new_elem = lAddElemUlong(this_list, QIM_type, type, QIM_Type);

      lSetString(new_elem, QIM_message, message);
   }
   DEXIT;
   return ret;
}

static bool
qim_list_trash_all_of_type_X(lList **this_list, u_long32 type) 
{
   bool ret = true;
   
   DENTER(TOP_LAYER, "qim_list_trash_all_of_type_X");
   if (this_list != NULL) {
      lListElem *elem = NULL;
      lListElem *next_elem = NULL;

      next_elem = lFirst(*this_list);
      while ((elem = next_elem) != NULL) {
         u_long32 elem_type = lGetUlong(elem, QIM_type);

         next_elem = lNext(elem);
         if ((elem_type & type) != 0) {
            lRemoveElem(*this_list, elem);
         }
      }
      if (lGetNumberOfElem(*this_list) == 0) {
         *this_list = lFreeList(*this_list);
      }
   }
   DEXIT;
   return ret;
}

bool
qinstance_message_add(lListElem *this_elem, u_long32 type, const char *message)
{
   bool ret = true;

   DENTER(TOP_LAYER, "qinstance_message_add");
   if (this_elem != NULL) {
      lList *qim_list = NULL;

      lXchgList(this_elem, QU_message_list, &qim_list);
      ret &= qim_list_add(&qim_list, type, message);
      lXchgList(this_elem, QU_message_list, &qim_list);
   }
   DEXIT;
   return ret;
}

bool
qinstance_message_trash_all_of_type_X(lListElem *this_elem, u_long32 type)
{
   bool ret = true;

   DENTER(TOP_LAYER, "qinstance_message_trash_all_of_type_X");
   if (this_elem != NULL) {
      lList *qim_list = NULL;

      lXchgList(this_elem, QU_message_list, &qim_list);
      ret &= qim_list_trash_all_of_type_X(&qim_list, type);
      lXchgList(this_elem, QU_message_list, &qim_list);
   }
   DEXIT;
   return ret;
}

/* SG: removed, because message might go over multiple lines. The code is kept
   as a starting point, if we want to add the error message spooling for the
   classic spooling. One has to find a solution to remove the "\n" and replace
   them by a string.
*/
#if 0
bool qinstance_message2string(const lListElem *queue, u_long32 type, dstring *target) {
   const char *del = "<&elem&>";
   lList *messageList = lGetList(queue, QU_message_list);
   lListElem *message = NULL;

   sge_dstring_sprintf_append(target, "%s", del);

   
   
   for_each(message, messageList) {
      if ((lGetUlong(message, QIM_type) & type) != 0){
         /*char *tempMessage = NULL;
         tempMessage = strdup(lGetString(message, QIM_message);*/
               
         sge_dstring_sprintf_append(target, u32"%s%s%s", lGetUlong(message, QIM_type), del, lGetString(message, QIM_message), del);
      }
   }
   return true;
}

bool qinsteance_message_from_string(lListElem *queue, const char *target, lList **answer) {
   const char *del = "<&elem&>";
   int del_length = strlen(del);
   static const int max_size = 1000;
   const char *token = NULL;
   const char *token2 = NULL;
   char tmp[max_size];
   int size;
   int error_code;
   lList *messageList = lCreateList("messages", QIM_Type);

   if (target == NULL){
      return false;
   }
   
   while((token = strstr(target, del)) != NULL) {
      lListElem *message = lCreateElem(QIM_Type);

      /* error nr */
      token += del_length;
      token2 = strstr(token, del);
      if (token2 == NULL)
         return false;
      if ((size = token2 - token) > max_size)
         size = max_size;
      strncpy(tmp, token, size); 
      error_code = atoi(tmp);
      lSetUlong(message, QIM_type, error_code);
      
      /* error message */
      token = token2 + del_length;
      token2 = strstr(token, del);
      if (token2 == NULL){
         return false;
      }
      if ((size = token2 - token) > max_size)
         size = max_size;
      strncpy(tmp, token, size);   
      lSetString(message, QIM_message, tmp);
     
      /* next */
      token = token2 + del_length;
   }
   
   lSetList(queue, QU_message_list, messageList);
   
   return true;
}
#endif
