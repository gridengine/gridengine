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
#include <unistd.h>
#include <string.h>

#include "commlib.h"
#include "sge_gdi_intern.h"
#include "msg_commd.h"
#include "sge_prognames.h"
#include "sge_language.h"
#include "sge_feature.h"

void usage(void);
int main(int argc, char **argv);

/* CONTROL Program for commd */

void usage()
{
   printf("%s\n", feature_get_product_name(FS_SHORT_VERSION));

   printf("%s\n commdcntl [-k | -t level | -d] [-p commdport] [-U] [-host dst_host]\n", MSG_USAGE);

   printf("          [-gid commprocname] [-unreg commprocname id]\n");
   printf("    -k     %s",MSG_COMMDCNTL_k_OPT_USAGE);
   printf("    -t     %s",MSG_COMMDCNTL_t_OPT_USAGE);
   printf("    -d     %s \"/tmp/commd/commd.dump\"\n",MSG_COMMDCNTL_d_OPT_USAGE);
   printf("    -p     %s",MSG_COMMDCNTL_p_OPT_USAGE);
   printf("    -U     %s",MSG_COMMDCNTL_U_OPT_USAGE);
   printf("    -gid   %s",MSG_COMMDCNTL_gid_OPT_USAGE);
   printf("    -unreg %s",MSG_COMMDCNTL_unreg_OPT_USAGE);
   exit(1);
}

int main(
int argc,
char **argv 
) {
   int i;
   u_short operation = 0;
   u_long32 arg;
   char *carg = NULL;

   int commdport = 0;
   
   /* 
   ** determine service, (will set CL_P_RESERVED_PORT from product mode file)
   */
   sge_gdi_param(SET_MEWHO,COMMDCNTL,NULL);
   sge_gdi_setup(prognames[COMMDCNTL]); 

   /*
   **  always set CL_P_RESERVED_PORT
   */
   i = set_commlib_param(CL_P_RESERVED_PORT, 1, NULL, NULL);
   if (i) {
      printf(MSG_COMMDCNTL_SETCOMMLIBPARAM2RETURNED_II , (int) 1, i);
   }


   /*
   ** eval command line
   */
   while (*(++argv)) {
      if (!strcmp("-p", *argv)) {
         argv++;
         if (!*argv)
            usage();
         commdport = atoi(*argv);
      }
      if (!strcmp("-U", *argv)) {
         /*
         **  disable CL_P_RESERVED_PORT
         */

         i = set_commlib_param(CL_P_RESERVED_PORT, 0, NULL, NULL);
         if (i) {
            printf(MSG_COMMDCNTL_SETCOMMLIBPARAM2RETURNED_II , (int) 1, i);
         }
      }
      if (!strcmp("-h", *argv)) {
         usage();
      }
      if (!strcmp("-k", *argv)) {
         operation = O_KILL;
      }
      if (!strcmp("-d", *argv)) {
         operation = O_DUMP;
      }
      if (!strcmp("-t", *argv)) {
         operation = O_TRACE;
         argv++;
         if (!*argv)
            usage();
         arg = atoi(*argv);
      }
      if (!strcmp("-gid", *argv)) {
         operation = O_GETID;
         argv++;
         if (!*argv)
            usage();
         carg = *argv;
      }
      if (!strcmp("-unreg", *argv)) {
         operation = O_UNREGISTER;
         argv++;
         if (!*argv)
            usage();
         carg = *argv;
         argv++;
         if (!*argv)
            usage();
         arg = atoi(*argv);
      }
      if (!strcmp("-host", *argv)) {
         argv++;
         if (argv) {
            set_commlib_param(CL_P_COMMDHOST, 0, *argv, NULL);
            if (getenv("COMMD_HOST"))
               putenv("COMMD_HOST=");
         }
         else
            usage();   
      }      
         
   }

   set_commlib_param(CL_P_COMMDSERVICE, 0, SGE_COMMD_SERVICE, NULL);
   
   if (!operation) {
      fprintf(stderr, MSG_COMMDCNTL_NOCTRLOPERATIONSPECIFIED );
      usage();
   }

   if (commdport) {
      i = set_commlib_param(CL_P_COMMDPORT, commdport, NULL, NULL);
      printf(MSG_COMMDCNTL_SETCOMMLIBPARAM1RETURNED_II , commdport, i);
   }


   i = cntl(operation, &arg, carg);
   if (i) {
      fprintf(stderr, MSG_CNTL_ERROR_S , cl_errstr(i));
   }
   else {
      if (operation == O_GETID) {
         printf("gid = "u32"\n", arg);
      }
   }
   return i;
}
