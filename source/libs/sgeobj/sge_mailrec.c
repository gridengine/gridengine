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

#include "sgermon.h"
#include "sge_string.h"
#include "sge_stdlib.h"
#include "sge_job.h"
#include "sge_mailrec.h"
#include "cull_parse_util.h"

#include "msg_common.h"

/****** sgeobj/mailrec/mailrec_parse() ****************************************
*  NAME
*     mailrec_parse() -- Parse a list of mail recipients 
*
*  SYNOPSIS
*     int mailrec_parse(lList **lpp, const char *mail_str) 
*
*  FUNCTION
*     Parse a list of mail recipients.
*     user[@host][,user[@host],...] 
*
*  INPUTS
*     lList **lpp          - MR_Type list 
*     const char *mail_str - stringlist of mail recipients 
*
*  RESULT
*     int - error state
*        0 - success
*       >0 - error
*
*  SEE ALSO
*     sgeobj/mailrec/mailrec_unparse() 
*******************************************************************************/
int mailrec_parse(lList **lpp, const char *mail_str) 
{
   const char *user;
   const char *host;
   char **str_str;
   char **pstr;
   lListElem *ep, *tmp;
   char *mail;

   DENTER(TOP_LAYER, "mailrec_parse");

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
         if (!sge_strnullcmp(host, lGetHost(tmp, MR_host))) {
            /* got this mail adress twice */
            continue;
         }
      }

      /* got a new adress - add it */
      ep = lCreateElem(MR_Type);
      lSetString(ep, MR_user, user);
      if (host) 
         lSetHost(ep, MR_host, host);
      lAppendElem(*lpp, ep);
   }

   FREE(mail);
   FREE(str_str);
   DEXIT;
   return 0;
}

/****** sgeobj/mailrec/mailrec_unparse() **************************************
*  NAME
*     mailrec_unparse() -- Build a string of mail reipients 
*
*  SYNOPSIS
*     int mailrec_unparse(lList *head, char *mail_str, 
*                         unsigned int mail_str_len) 
*
*  FUNCTION
*     Build a string of mail reipients ("user@host,user,...") 
*
*  INPUTS
*     lList *head               - MR_Type list
*     char *mail_str            - buffer to be filled 
*     unsigned int mail_str_len - size of buffer 
*
*  RESULT
*     int - error state
*        0 - success
*       >0 - error
*
*  SEE ALSO
*     sgeobj/mailrec/mailrec_parse() 
*******************************************************************************/
int mailrec_unparse(lList *head, char *mail_str, unsigned int mail_str_len)
{
   int len=0;
   int comma_needed = 0; /* whether we need to insert a comma */
   char tmpstr[1000];    /* need 1000 for brain damaged mail addresse(e)s */
   lListElem *elem;
   const char *h;
   const char *u;

   if (!head) {
      strcpy(mail_str, MSG_NONE);
      return 0;
   }

   *mail_str = '\0';

   for_each(elem,head) {
      if (!(u = lGetString(elem, MR_user)))
         u = MSG_SMALLNULL;

      if (!(h = lGetHost(elem, MR_host)))
         sprintf(tmpstr, "%s", u);
      else
         sprintf(tmpstr, "%s@%s", u, h);

      if (strlen(tmpstr)+len+1+comma_needed > mail_str_len)
         return 1;              /* forgot the rest */

      if (comma_needed)
         strcat(mail_str, ",");
      else
         comma_needed = 1;      /* need comma after first mailaddress */

      strcat(mail_str, tmpstr);
   }
   return 0;
}

