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
 *  License at http://www.gridengine.sunsource.net/license.html
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
#include <stdlib.h>

#include "sgermon.h"
#include "sge_string.h"
#include "def.h"
#include "sge_jobL.h"
#include "parse_mail.h"
#include "cull_parse_util.h"

/* 
   here's what we're gonna do with the list elements:
   user[@host][,user[@host],...]
 */
int cull_parse_mail_list(
lList **lpp,
char *mail_str 
) {
   char *user;
   char *host;
   char **str_str;
   char **pstr;
   lListElem *ep, *tmp;
   char *mail;

   DENTER(TOP_LAYER, "cull_parse_mail_list");

   if (!lpp) {
      DEXIT;
      return 1;
   }

   mail = sge_strdup(NULL, mail_str);
   if (!mail) {
      *lpp = NULL;
      DEXIT;
      return 2;
   }
   str_str = string_list(mail, ",", NULL);
   if (!str_str || !*str_str) {
      *lpp = NULL;
      FREE(mail);
      DEXIT;
      return 3;
   }

   if (!*lpp) {
      *lpp = lCreateList("mail list", MR_Type);
      if (!*lpp) {
         FREE(mail);
         FREE(str_str);
         DEXIT;
         return 4;
      }
   }

   for (pstr = str_str; *pstr; pstr++) {
      user = sge_strtok(*pstr, "@");
      host = sge_strtok(NULL, "@");
      if ((tmp=lGetElemStr(*lpp, MR_user, user))) {
         if (!sge_strnullcmp(host, lGetString(tmp, MR_host))) {
            /* got this mail adress twice */
            continue;
         }
      }

      /* got a new adress - add it */
      ep = lCreateElem(MR_Type);
      lSetString(ep, MR_user, user);
      if (host) 
         lSetString(ep, MR_host, host);
      lAppendElem(*lpp, ep);
   }

   FREE(mail);
   FREE(str_str);
   DEXIT;
   return 0;
}
