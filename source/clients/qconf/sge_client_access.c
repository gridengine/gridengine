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

#include "sge_gdi_intern.h"
#include "sge_usersetL.h"
#include "sge_answerL.h"
#include "sge_client_access.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "msg_qconf.h"


static void report_and_free_answers(lList **alpp, lList *new_answers);

static void report_and_free_answers(
lList **alpp,
lList *new_answers 
) {
   lListElem *answer;

   DENTER(TOP_LAYER,"report_and_free_answers");

   if (alpp) { /* append all answers to existing list */
      lAddList(*alpp, new_answers); 
   }
   else { /* write errors to stderr */
      for_each (answer, new_answers) 
         if (sge_get_recoverable(answer) != STATUS_OK) 
            fprintf(stderr, "%s\n", lGetString(answer, AN_text));
      lFreeList(new_answers);
   }

   DEXIT;
   return;
}

/* - -- -- -- -- -- -- -- -- -- -- -- -- -- -

   user_args - a cull list(UE_Type) of users
   acl_args - a cull list(US_Type) of acl

   returns 
      0 on success
      -1 on error

*/
int sge_client_add_user(
lList **alpp,
lList *user_args,
lList *acl_args 
) {
   lListElem *userarg, *aclarg;
   lList *acl=NULL, *answers=NULL;
   const char *acl_name, *user_name;
   lCondition *where;
   lEnumeration *what;
   u_long32 status;
   int already;

   DENTER(TOP_LAYER, "sge_client_add_user");

   INIT_ALPP(alpp);

   what = lWhat("%T(ALL)", US_Type);

   for_each(aclarg,acl_args) {
      acl_name = lGetString(aclarg, US_name);
      where = lWhere("%T(%I==%s)", US_Type, US_name, acl_name);

      for_each(userarg, user_args) {

         already = 0;
         user_name=lGetString(userarg, UE_name);
   
         /* get old acl */
         answers = sge_gdi(SGE_USERSET_LIST, SGE_GDI_GET, &acl, where, what);
         lFreeList(answers);

         if (acl) {
            if (!lGetSubStr(lFirst(acl), UE_name, user_name, US_entries)) {
               lAddSubStr(lFirst(acl), UE_name, user_name, US_entries, UE_Type);

               /* mod the acl */
               answers = sge_gdi(SGE_USERSET_LIST, SGE_GDI_MOD, &acl, 
                        NULL, NULL);   
            }
            else
               already = 1;
         }
         else {
            /* build new list */
            lAddElemStr(&acl, US_name, acl_name, US_Type);
            lAddSubStr(lFirst(acl), UE_name, user_name, US_entries, UE_Type);
            
            /* add the acl */
            answers = sge_gdi(SGE_USERSET_LIST, SGE_GDI_ADD, &acl, 
                     NULL, NULL);   
         }

         if (already) {
            status = STATUS_EEXIST;
            SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_GDI_USERINACL_SS, user_name, acl_name));
         }
         else {
            if ((status = lGetUlong(lFirst(answers), AN_status))!=STATUS_OK) {
               const char *cp;
            
               cp = lGetString(lFirst(answers), AN_text);
               if (cp) {
                  sprintf(SGE_EVENT, "%s", cp);
               }
               else {
                  SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_GDI_CANTADDTOACL_SS, user_name, acl_name));
               }
            }
            else 
               sprintf(SGE_EVENT, MSG_GDI_ADDTOACL_SS, user_name, acl_name);
            answers = lFreeList(answers);
         }
         sge_add_answer(alpp, SGE_EVENT, status, 
            ((status == STATUS_OK) ? NUM_AN_INFO : NUM_AN_ERROR));
         acl = lFreeList(acl);

      }
      lFreeWhere(where);
   }
   lFreeWhat(what);
   DEXIT;
   return 0;
}

/* - -- -- -- -- -- -- -- -- -- -- -- -- -- -

   user_args - a cull list(UE_Type) of users
   acl_args - a cull list(US_Type) of acl

   returns 
      0 on success
      -1 on error

*/
int sge_client_del_user(
lList **alpp,
lList *user_args,
lList *acl_args 
) {
   lListElem *userarg, *aclarg;
   lList *acl=NULL, *answers=NULL;
   const char *acl_name, *user_name;
   lCondition *where;
   lEnumeration *what;
   u_long32 status;

   DENTER(TOP_LAYER, "sge_client_del_user");

   INIT_ALPP(alpp);

   what = lWhat("%T(ALL)", US_Type);

   for_each(aclarg,acl_args) {
      acl_name = lGetString(aclarg, US_name);
      where = lWhere("%T(%I==%s)", US_Type, US_name, acl_name);

      for_each(userarg, user_args) {
         int breakit = 0;
         char *cp = NULL;
         user_name=lGetString(userarg, UE_name);
         /* get old acl */
         answers = sge_gdi(SGE_USERSET_LIST, SGE_GDI_GET, &acl, where, what);
         cp = sge_strdup(cp, lGetString(lFirst(answers), AN_text));
         answers = lFreeList(answers);
         if (acl) {
            free(cp);
            cp = NULL;
            if (lGetSubStr(lFirst(acl), UE_name, user_name, US_entries)) {
               lDelSubStr(lFirst(acl), UE_name, user_name, US_entries);
               answers = sge_gdi(SGE_USERSET_LIST, SGE_GDI_MOD, &acl, NULL, NULL);
               cp = sge_strdup(cp, lGetString(lFirst(answers), AN_text));
               status = lGetUlong(lFirst(answers), AN_status);
               answers = lFreeList(answers);
            }
            else
               status = STATUS_EEXIST + 1;
         }
         else 
            status = STATUS_EEXIST;

         if (status != STATUS_OK) {
            if (status == STATUS_EEXIST) {
               SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_GDI_ACLDOESNOTEXIST_S, acl_name));
               breakit = 1;        
            }
	    else if (status == STATUS_EEXIST + 1) {
               SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_GDI_USERNOTINACL_SS, user_name, acl_name));
            }
            else if (cp) {
               sprintf(SGE_EVENT, "%s", cp);
            }
	    else {
               SGE_ADD_MSG_ID( sprintf(SGE_EVENT, MSG_GDI_CANTDELFROMACL_SS,  user_name, acl_name));
            }

         }
         else {
            sprintf(SGE_EVENT, MSG_GDI_DELFROMACL_SS, user_name, acl_name);
         }
         sge_add_answer(alpp, SGE_EVENT, status, 
                        ((status == STATUS_OK) ? NUM_AN_INFO : NUM_AN_ERROR));
         acl = lFreeList(acl);
         
         if (cp) {
            free(cp);
            cp = NULL;
         }
         if (breakit)
            break;
            
      }
      lFreeWhere(where);
   }
   lFreeWhat(what);

   DEXIT;
   return 0;
}

/* - -- -- -- -- -- -- -- -- -- -- -- -- -- -- -   

   sge_client_get_acls()

   acl_args 
      a list containing US_name fields 
   
   try to get all acls named in acl_args from qmaster

   returns 
      -1 on error
      0 on success

*/
int sge_client_get_acls(
lList **alpp,
lList *acl_args,
lList **dst 
) {
   lList *answers;
   lListElem *aclarg;
   lCondition *where, *newcp;
   lEnumeration *what;
   const char *acl_name;
   
   DENTER(TOP_LAYER, "sge_client_get_acls");

   INIT_ALPP(alpp);

   where = NULL;
   for_each(aclarg,acl_args) {

      acl_name = lGetString(aclarg, US_name);
      newcp = lWhere("%T(%I==%s)", US_Type, US_name, acl_name);
      if (!where) 
         where = newcp;
      else
         where = lOrWhere(where, newcp);
   }
   what = lWhat("%T(ALL)", US_Type);
   answers = sge_gdi(SGE_USERSET_LIST, SGE_GDI_GET, dst, where, what);
   lFreeWhat(what);
   lFreeWhere(where);

   report_and_free_answers(alpp, answers);

   DEXIT;
   return 0;
}
