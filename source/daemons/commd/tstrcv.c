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
#include <string.h>

#include "sgermon.h"
#include "commlib.h"
#include "msg_commd.h"
#include "sge_language.h"

void usage(void);
int main(int argc, char **argv);

void usage()
{
   printf( "%s tstrcv [-s] [-host host] [-commproc name] [-id id] [-mt tag] [-pt tag] [-enrollname name] [-enrollid id] [-t timeout] [-p commdport] [-S] [-r repetitions] [-cfd]\n", MSG_USAGE );

   printf( "  -s   %s",  MSG_TSTRCV_s_OPT_USAGE );
   printf( "  -t   %s",  MSG_TSTRCV_t_OPT_USAGE );
   printf( "  -host host -commproc name -id id     %s",  MSG_TSTRCV_host_OPT_USAGE );
   printf( "  -enroll name -enrollid id     %s",  MSG_TSTRCV_enroll_OPT_USAGE );
   printf( "  -S   %s",  MSG_TSTRCV_S_OPT_USAGE );
   printf( "  -r   %s",  MSG_TSTRCV_r_OPT_USAGE );
   printf( "  -cfd %s",  MSG_TSTRCV_cfd_OPT_USAGE);
   exit(1);
}

int main(
int argc,
char **argv 
) {
   int i = 0;
   u_short id = 1, savedid, fromid = 1;
   char *buffer = NULL;
   u_long32 buflen = 0;
   char fromhost[256], savedfromhost[256], fromcommproc[256], savedfromcommproc[256], 
        enrollname[256];
   int retval = 0;
   int synchron = 0;
   int timeout = 0;
   int commdport = 0;
   int reserved_port = 0;
   int tag = 1, savedtag;
   int priority_tag_list[10];
   int priority_tags = 0;
   int repetitions = 1;
   int closefd = 1;

   DENTER_MAIN(TOP_LAYER, "tstrcv");




   *fromhost = '\0';
   *fromcommproc = '\0';
   strcpy(enrollname, "tstrcv");
   tag = 0;
#ifdef __SGE_COMPILE_WITH_GETTEXT__     
   /* init language output for gettext() , it will use the right language */
   sge_init_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */

   while (*(++argv)) {
      if (!strcmp("-h", *argv))
         usage();

      if (!strcmp("-host", *argv)) {
         argv++;
         if (!*argv)
            usage();
         strcpy(fromhost, *argv);
      }

      if (!strcmp("-t", *argv)) {
         argv++;
         if (!*argv)
            usage();
         timeout = atoi(*argv);
      }

      if (!strcmp("-p", *argv)) {
         argv++;
         if (!*argv)
            usage();
         commdport = atoi(*argv);
      }

      if (!strcmp("-commproc", *argv)) {
         argv++;
         if (!*argv)
            usage();
         strcpy(fromcommproc, *argv);
      }
      if (!strcmp("-id", *argv)) {
         argv++;
         if (!*argv)
            usage();
         fromid = atoi(*argv);
      }
      if (!strcmp("-mt", *argv)) {
         argv++;
         if (!*argv)
            usage();
         tag = atoi(*argv);
      }
      if (!strcmp("-pt", *argv)) {
         argv++;
         if (!*argv)
            usage();
         priority_tag_list[priority_tags++] = atoi(*argv);
      }

      if (!strcmp("-enrollname", *argv)) {
         argv++;
         if (!*argv)
            usage();
         strcpy(enrollname, *argv);
      }
      if (!strcmp("-enrollid", *argv)) {
         argv++;
         if (!*argv)
            usage();
         id = atoi(*argv);
      }
      if (!strcmp("-s", *argv)) {
         synchron = 1;
      }
      if (!strcmp("-S", *argv)) {
         reserved_port = 1;
      }
      if (!strcmp("-r", *argv)) {
         argv++;
         if (!*argv)
            usage();
         repetitions = atoi(*argv);
      }
      if (!strcmp("-cfd", *argv))
         closefd = 1;
   }

   /* set timeout for communication */
   if (timeout) {
      i = set_commlib_param(CL_P_TIMEOUT, timeout, NULL, NULL);
      DPRINTF(("set_commlib_param(CL_P_TIMEOUT, %d) returns %d\n",
             timeout, i));
      i = set_commlib_param(CL_P_TIMEOUT_SRCV, timeout, NULL, NULL);
      DPRINTF(("set_commlib_param(CL_P_TIMEOUT_SRCV, %d) returns %d\n",
             timeout, i));
   }

   if (commdport) {
      i = set_commlib_param(CL_P_COMMDPORT, commdport, NULL, NULL);
      DPRINTF(("set_commlib_param(CL_P_COMMDPORT, %d) returns %d\n",
             commdport, i));
   }

   if (reserved_port) {
      i = set_commlib_param(CL_P_RESERVED_PORT, 1, NULL, NULL);
      DPRINTF(("set_commlib_param(CL_P_RESERVED_PORT, %d) returns %d\n", 1, i));
   }

   i = set_commlib_param(CL_P_CLOSE_FD, closefd, NULL, NULL);
      DPRINTF(("set_commlib_param(CL_P_CLOSE_FD, %d) returns %d\n", closefd, i));
   /* enroll to commd */
   for (i = priority_tags; i < 10; i++)
      priority_tag_list[i] = 0;

   set_commlib_param(CL_P_NAME, 0, enrollname, NULL);
   set_commlib_param(CL_P_ID, (int) id, NULL, NULL);
   set_commlib_param(CL_P_PRIO_LIST, 0, NULL, priority_tag_list); 

   savedtag =  tag;
   strcpy(savedfromhost, fromhost);
   savedid = fromid;
   strcpy(savedfromcommproc, fromcommproc);
   
   while (repetitions--) {
      /* receive synchron message */
      strcpy(fromhost, savedfromhost);
      strcpy(fromcommproc, savedfromcommproc);
      tag = savedtag;
      fromid = savedid;
      
      i = receive_message(fromcommproc, &fromid, fromhost, &tag, &buffer,
                          &buflen, synchron, NULL);
      DPRINTF(("rcv_message returned: %d\n", i));
      retval = i;
      if (!i) {
         DPRINTF(("buflen = %ld\n", buflen));
         /* DPRINTF(("buffer >%s<\n", buffer)); */
         DPRINTF(("fromcommproc = %s\n", fromcommproc));
         DPRINTF(("fromid = %d\n", fromid));
         DPRINTF(("fromhost = %s\n", fromhost));
         DPRINTF(("tag = %d\n", tag));
         free(buffer);
         buffer = NULL;
      }
      else {
         printf(MSG_ERROR_S , cl_errstr(i));
      }
      fprintf(stderr, ".");
   }                            /* while repetitions */

   i = leave_commd();
   DPRINTF(("leave returned %d\n", i));
   if (i)
      printf(MSG_ERROR_S , cl_errstr(i));

   DEXIT;
   return retval ? retval : i;
}
