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
#include <errno.h>

#include "sgermon.h"
#include "sge_userset.h"

lList *Master_Userset_List = NULL;

/*****************************************************************
 is_deadline_user

 ask whether a given user is allowed to sumbit deadline jobs.
 *****************************************************************/
int is_deadline_user(
char *username,         /* user we ask for */
lList *lp               /* userset list to scan for deadline users */
) {
   lListElem *deadline_users;

   DENTER(TOP_LAYER, "is_deadline_user");

   deadline_users = lGetElemStr(lp, US_name, DEADLINE_USERS);

   if (deadline_users && lGetSubStr(deadline_users, UE_name, username, 
         US_entries)) {
      DEXIT;
      return 1; /* found user in deadline user list */
   }

   DEXIT;
   return 0;
}

