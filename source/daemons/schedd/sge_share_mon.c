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
#include <time.h>

#include "sge_dstring.h"

#include "sge_gdi.h"
#include "sge_all_listsL.h"
#include "sge_schedd_conf.h"
#include "sge_usageL.h"
#include "sge_time.h"
#include "msg_schedd.h"
#include "sge_language.h"
#include "scheduler.h"
#include "sgeee.h"
#include "sge_support.h"
#include "sge_answer.h"
#include "sge_userprj.h"
#include "sge_schedd_conf.h"

#include "sge_sharetree_printing.h"

static int
setup_lists(lList **sharetree, lList **users, lList **projects, lList **config)
{
   lList *alp;
   lListElem *aep, *root;
   lEnumeration *what;

   /*
    * get share tree
    */

   what = lWhat("%T(ALL)", STN_Type);
   alp=sge_gdi(SGE_SHARETREE_LIST, SGE_GDI_GET, sharetree, NULL, what);
   lFreeWhat(what);

   aep = lFirst(alp);
   answer_exit_if_not_recoverable(aep);
   if (answer_get_status(aep) != STATUS_OK) {
      fprintf(stderr, "%s", lGetString(aep, AN_text));
      exit(1);
   }
   lFreeList(alp);

   if (!*sharetree || (root = lFirst(*sharetree)) == NULL) {
      fprintf(stderr, MSG_SGESHAREMON_NOSHARETREE );
      exit(2);
   }

   /*
    * get config list
    */

   what = lWhat("%T(ALL)", SC_Type);
   alp=sge_gdi(SGE_SC_LIST, SGE_GDI_GET, config, NULL, what);
   lFreeWhat(what);

   aep = lFirst(alp);
   answer_exit_if_not_recoverable(aep);
   if (answer_get_status(aep) != STATUS_OK) {
      fprintf(stderr, "%s", lGetString(aep, AN_text));
      exit(1);
   }
   lFreeList(alp);

   /*
    * get user and project list
    */

   what = lWhat("%T(ALL)", UP_Type);
   alp = sge_gdi(SGE_USER_LIST, SGE_GDI_GET, users, NULL, what);
   lFreeWhat(what);

   what = lWhat("%T(ALL)", UP_Type);
   alp = sge_gdi(SGE_PROJECT_LIST, SGE_GDI_GET, projects, NULL, what);
   lFreeWhat(what);

   aep = lFirst(alp);
   if (answer_get_status(aep) != STATUS_OK) {
      fprintf(stderr, "%s", lGetString(aep, AN_text));
      exit(3);
   }
   lFreeList(alp);

   return 0;
}


static int
free_lists(lList *sharetree, lList *users, lList *projects, lList *config)
{
   if (sharetree)
      lFreeList(sharetree);
   if (users)
      lFreeList(users);
   if (projects)
      lFreeList(projects);
   if (config)
      lFreeList(config);
   return 0;
}




static void
usage(void)
{
      fprintf(stderr, "%s sge_share_mon [-cdfhilmnorsux] [node_names ...]\n", MSG_USAGE); 
      fprintf(stderr, "\n" );
      fprintf(stderr, "   -c count          %s", MSG_SGESHAREMON_c_OPT_USAGE );
      fprintf(stderr, "   -d delimiter      %s", MSG_SGESHAREMON_d_OPT_USAGE );
      fprintf(stderr, "   -f field[,field]  %s", MSG_SGESHAREMON_f_OPT_USAGE );
      fprintf(stderr, "   -h                %s", MSG_SGESHAREMON_h_OPT_USAGE );
      fprintf(stderr, "   -i interval       %s", MSG_SGESHAREMON_i_OPT_USAGE );
      fprintf(stderr, "   -l delimiter      %s", MSG_SGESHAREMON_l_OPT_USAGE );
      fprintf(stderr, "   -m output_mode    %s", MSG_SGESHAREMON_m_OPT_USAGE );
      fprintf(stderr, "   -n                %s", MSG_SGESHAREMON_n_OPT_USAGE );
      fprintf(stderr, "   -o output_file    %s", MSG_SGESHAREMON_o_OPT_USAGE );
      fprintf(stderr, "   -r delimiter      %s", MSG_SGESHAREMON_r_OPT_USAGE );
      fprintf(stderr, "   -s string_format  %s", MSG_SGESHAREMON_s_OPT_USAGE );
      fprintf(stderr, "   -t                %s", MSG_SGESHAREMON_t_OPT_USAGE );
      fprintf(stderr, "   -u                %s", MSG_SGESHAREMON_u_OPT_USAGE );
      fprintf(stderr, "   -x                %s", MSG_SGESHAREMON_x_OPT_USAGE );
      fprintf(stderr,"\n");
}

static FILE *
open_output(const char *file_name, const char *mode) 
{
   FILE *file = stdout;
   
   if (file_name != NULL) {
      file = fopen(file_name, mode);
      if (file == NULL) {
         fprintf(stderr, MSG_FILE_COULDNOTOPENXFORY_SS , file_name, mode);
         exit(1);
      }
   }

   return file;
}

static FILE *
close_output(FILE *file)
{
   if (file != stdout) {
      fclose(file);
      file = NULL;
   }

   return file;
}


int
main(int argc, char **argv)
{
   lList *sharetree, *users, *projects, *config;
   int interval=15;
   int err=0;
   int count=-1;
   int header=0;
   format_t format;
   char *ofile=NULL;
   const char **names=NULL;
   bool group_nodes=true;
   bool decay_usage=false;
   int c;
   extern char *optarg;
   extern int optind;
   FILE *outfile = NULL;
   char *output_mode = "a";

   dstring output_dstring = DSTRING_INIT;
   
   format.name_format  = false;
   format.delim        = "\t";
   format.line_delim   = "\n";
   format.rec_delim    = "\n";
   format.str_format   = "%s";
   format.field_names  = NULL;
   format.format_times = false;
   format.line_prefix  = NULL;


#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   sge_init_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */


   while ((c = getopt(argc, argv, "i:c:f:d:hl:m:no:r:s:tux")) != -1) {
      switch(c) {
	 case 'i':
	    if (sscanf(optarg, "%d", &interval) != 1) {
		fprintf(stderr, MSG_ERROR_XISNOTAVALIDINTERVAL_S , optarg);
		err++;
	    }
	    break;
	 case 'c':
	    if (sscanf(optarg, "%d", &count) != 1) {
		fprintf(stderr, MSG_ERROR_XISNOTAVALIDCOUNT_S , optarg);
		err++;
	    }
	    break;
         case 'f':
            format.field_names = strdup(optarg);
            break;
	 case 'o':
	    ofile = strdup(optarg);
	    break;
	 case 'd':
	    format.delim = strdup(optarg);
	    break;
         case 'h':
            header = 1;
            break;
	 case 'm':
	    output_mode = strdup(optarg);
	    break;
	 case 'n':
	    format.name_format = true;
	    break;
	 case 'l':
	    format.line_delim = strdup(optarg);
	    break;
	 case 'r':
	    format.rec_delim = strdup(optarg);
	    break;
	 case 's':
	    format.str_format = strdup(optarg);
	    break;
	 case 't':
	    format.format_times = true;
	    break;
	 case 'u':
	    decay_usage = true;
	    break;
	 case 'x':
	    group_nodes = false;
	    break;
	 case '?':
	    err++;
      }
   }

   if (err) {
      usage();
      exit(1);
   }

   if ((argc - optind) > 0) {
       names = (const char **)&argv[optind];
   }

   sge_gdi_setup("sge_share_mon", NULL);

   if (header) {
      print_hdr(&output_dstring, &format);
   }

   while(count == -1 || count-- > 0) {
      setup_lists(&sharetree, &users, &projects, &config);
      sconf_set_config(&config, NULL);

      sge_sharetree_print(&output_dstring, sharetree, users, projects, group_nodes, decay_usage, names, &format);

      if (count && strlen(format.rec_delim)) {
	      sge_dstring_sprintf_append(&output_dstring, "%s", format.rec_delim);
      }

      outfile = open_output(ofile, output_mode);
      fwrite(sge_dstring_get_string(&output_dstring), 
             sge_dstring_strlen(&output_dstring), 1, outfile);
      outfile = close_output(outfile);

      free_lists(sharetree, users, projects, config);
      sge_dstring_clear(&output_dstring);

      if (count) {
         sleep(interval);
      }
   }

   sge_gdi_shutdown();

   sge_dstring_free(&output_dstring);

   return 0;
}
