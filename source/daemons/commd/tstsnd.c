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

#include "sgermon.h"
#include "commlib.h"
#include "msg_commd.h"
#include "sge_language.h"


void usage(void);
int main(int argc, char **argv);


void usage()
{  
   printf("%s\n"
          "tstsnd [-s] [-host host] [-commproc name] [-id id] [-mt tag] [-r repetitions]\n"
          "       [-enrollname name] [-enrollid id] [-t timeout] [-buflen buflen]\n"
          "       [-p commdport] [-S] [-cfd]\n"
          "tstsnd [-p commdport] -ae hostname name id   name=any, id=0 means any id\n"
          "tstsnd [-p commdport] -uh hostname 0|1  search unique hostname\n"
          "                                        force commd to reread aliasfile\n\n" , MSG_USAGE);
  
   printf(" -s             %s", MSG_TSTSND_s_OPT_USAGE);
   printf(" -host        | %s", MSG_TSTSND_host_OPT_USAGE);
   printf(" -commproc    | \n");
   printf(" -id          | \n");
   printf(" -mt            %s", MSG_TSTSND_mt_OPT_USAGE);
   printf(" -enrollname  | %s", MSG_TSTSND_enrollname_OPT_USAGE);
   printf(" -enrollid    | \n");
   printf(" -t             %s", MSG_TSTSND_t_OPT_USAGE);
   printf(" -p             %s", MSG_TSTSND_p_OPT_USAGE);
   printf(" -S             %s", MSG_TSTSND_S_OPT_USAGE);
   printf(" -r             %s", MSG_TSTSND_r_OPT_USAGE);
   printf(" -cfd           %s", MSG_TSTSND_cfd_OPT_USAGE);
   exit(1);
}

int main(
int argc,
char **argv 
) {
   unsigned int i, refresh_aliases = 0;
   u_short enrollid = 1, toid = 1;
   u_long32 mid;
   char *buffer;
   int buflen;
   int synchron_send = 0;
   int ask_enrolled_id = -1;
   int timeout = 0;
   char host[256], commproc[256], enrollname[256], ask_enrolled_hostname[256];
   char ask_enrolled_name[256], unique_hostname[256];
   int commdport = 0;
   int reserved_port = 0;
   int tag = 1;
   int repetitions = 1;
   int closefd = 0;

   DENTER_MAIN(TOP_LAYER, "tstsnd"); 
#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   sge_init_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */
   unique_hostname[0] = '\0';

   host[0] = '\0';
   strcpy(commproc, "tstrcv");
   strcpy(enrollname, "tstsnd");
   buffer = (char *) malloc(strlen("Hello") + 1);
   buflen = strlen("Hello") + 1;
   

   while (*(++argv)) {
      if (!strcmp("-s", *argv)) {
         synchron_send = 1;
      }

      if (!strcmp("-S", *argv)) {
         reserved_port = 1;
      }

      if (!strcmp("-host", *argv)) {
         argv++;
         if (!*argv)
            usage();
         strcpy(host, *argv);
      }

      if (!strcmp("-buflen", *argv)) {
         free(buffer);
         argv++;
         if (!*argv)
            usage();
         buflen = atoi(*argv);
         if (!(buffer = (char *) malloc(buflen))) {
            printf(MSG_MEMORY_MALLOCSIZEFAILED_D , u32c(buflen));
            exit(1);
         }   
      }   
         
      if (!strcmp("-t", *argv)) {
         argv++;
         if (!*argv)
            usage();
         timeout = atoi(*argv);
      }

      if (!strcmp("-h", *argv)) {
         usage();
      }
      if (!strcmp("-commproc", *argv)) {
         argv++;
         if (!*argv)
            usage();
         strcpy(commproc, *argv);
      }
      if (!strcmp("-id", *argv)) {
         argv++;
         if (!*argv)
            usage();
         toid = atoi(*argv);
      }
      if (!strcmp("-mt", *argv)) {
         argv++;
         if (!*argv)
            usage();
         tag = atoi(*argv);
      }

      if (!strcmp("-p", *argv)) {
         argv++;
         if (!*argv)
            usage();
         commdport = atoi(*argv);
      }
      if (!strcmp("-r", *argv)) {
         argv++;
         if (!*argv)
            usage();
         repetitions = atoi(*argv);
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
         enrollid = atoi(*argv);
      }
      if (!strcmp("-ae", *argv)) {
         argv++;
         if (!*argv)
            usage();
         strcpy(ask_enrolled_hostname, *argv);
         argv++;
         if (!*argv)
            usage();
         strcpy(ask_enrolled_name, *argv);
         argv++;
         if (!*argv)
            usage();
         ask_enrolled_id = atoi(*argv);
         if (!strcmp("any", ask_enrolled_name))
            ask_enrolled_name[0] = '\0';
      }
      if (!strcmp("-uh", *argv)) {
         argv++;
         if (!*argv)
            usage();
         strcpy(unique_hostname, *argv);
         argv++;
         if (!*argv)
            usage();
         refresh_aliases = atoi(*argv);
      }
      if (!strcmp("-cfd", *argv))
         closefd = 1;
   }

   if (timeout) {
      i = set_commlib_param(CL_P_TIMEOUT, timeout, NULL, NULL);
      DPRINTF(("set_commlib_param(CL_P_TIMEOUT, %d) returns %d\n",
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

   set_commlib_param(CL_P_NAME, 0, enrollname, NULL);
   set_commlib_param(CL_P_ID, (int) enrollid, NULL, NULL);

   while (repetitions--) {
      if (ask_enrolled_id != -1) {
         /* just ask for an enrolled commproc */
         i = ask_commproc(ask_enrolled_hostname, ask_enrolled_name,
                          ask_enrolled_id);
         switch (i) {
         case 0:
            printf(MSG_TSTSND_ENROLLED );
            break;
         case CL_UNKNOWN_RECEIVER:
         case COMMD_NACK_DELIVERY:
            printf(MSG_TSTSND_NOTENROLLED );
            break;
         default:
            fprintf(stderr, MSG_ERROR_S, cl_errstr(i));
         }
      }
      else {
         if (unique_hostname[0]) {
            i = getuniquehostname(unique_hostname, host, refresh_aliases);
            if (i) {
               printf(MSG_ERROR_S , cl_errstr(i));
               leave_commd();
               return i;
            }
            printf(MSG_NET_UNIQUEHOSTNAMEIS_S , host);
         }
         else {
            /* send message */
            strcpy(buffer, "hallo");
            printf(".");
            fflush(stdout);
            i = send_message(synchron_send, commproc, toid, host, tag, buffer,
                             buflen, &mid, 0);

            DPRINTF(("send_message returned: %d mid=%ld\n", i, mid));
            if (i) {
               printf(MSG_ERROR_S , cl_errstr(i));
               leave_commd();
               return i;
            }
         }
      }
   }                            /* while repetitions */

   i = leave_commd();
   DPRINTF(("leave returned %d\n", i));
   if (i)
      printf(MSG_ERROR_S , cl_errstr(i));

   return 0;
}
