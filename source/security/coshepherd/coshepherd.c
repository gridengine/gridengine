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
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>

#include "sge_time.h"
#include "sgermon.h"
#include "sge_afsutil.h"
#include "sge_language.h"
#include "sge_unistd.h"

int main(int argc, char *argv[])
{
   SGE_STRUCT_STAT sb;
   time_t now;
   static time_t last = 0;
   int last_token_set, token_extend_time, renew_before;
   char *command, *user;
   char *tokenbuf;
   char err_str[1024+128];

   DENTER_MAIN(TOP_LAYER, "coshepherd");


#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   sge_init_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */


   if (argc != 4) {
      DEXIT;
      return 1;
   }

   if ((tokenbuf = sge_read_token(TOKEN_FILE)) == NULL) {
      DEXIT;
      return 1;
   }   
   
   command = argv[1];
   user = argv[2];
   token_extend_time = atoi(argv[3]);
   
   /* assume that toke was just set before - otherwise set to 0 */
   last_token_set = sge_get_gmt();
   renew_before = MIN(token_extend_time/10, 1800);

#define SLEEP 30

   while(1) {

      now = sge_get_gmt();      

      if (now < last)
         last = now;

      if (now - last < SLEEP) {
         DPRINTF(("sleep(%d)\n", SLEEP - (now - last)));
         sleep(SLEEP - (now - last));
         continue;
      }

      last = now;

      /* stop renewing tokens in case of job finish */
      if (SGE_STAT(TOKEN_FILE, &sb))
         break;

      if (last_token_set + token_extend_time - renew_before < now) {
         DPRINTF(("renewing AFS token : %s %s %d\n", command, user, token_extend_time));
         if (sge_afs_extend_token(command, tokenbuf, user, token_extend_time, err_str))
            DPRINTF(("AFS token renewal failed\n"));
         else
            last_token_set = sge_get_gmt();
      }
   }
   
   DPRINTF(("token file was deleted\n"));
   return 0;
}
