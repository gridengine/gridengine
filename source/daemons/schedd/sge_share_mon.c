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

#include "sge_gdi.h"
#include "sge_all_listsL.h"
#include "sge_gdi_intern.h"
#include "sge_schedconfL.h"
#include "sge_jobL.h"
#include "sge_userprjL.h"
#include "sge_share_tree_nodeL.h"
#include "sge_usageL.h"
#include "sge_hostL.h"
#include "sge_queueL.h"
#include "sge_usersetL.h"
#include "sge_time.h"
#include "msg_schedd.h"
#include "sge_language.h"
#include "scheduler.h"
#include "sgeee.h"
#include "sge_support.h"

typedef struct {
   int name_format;
   int format_times;
   char *delim;
   char *line_delim;
   char *rec_delim;
   char *str_format;
   char *field_names;
} format_t;

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
   if (sge_get_recoverable(aep) != STATUS_OK) {
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
   if (sge_get_recoverable(aep) != STATUS_OK) {
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
   if (sge_get_recoverable(aep) != STATUS_OK) {
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



int
calculate_share_percents(lListElem *node, double parent_percent, double sibling_shares)
{
   lListElem *child;
   double sum_shares=0;

   for_each(child, lGetList(node, STN_children)) {
      sum_shares += lGetUlong(child, STN_shares);
   }

   if (sibling_shares > 0)
      lSetDouble(node, STN_proportion,
		 (double)lGetUlong(node, STN_shares) / (double)sibling_shares);
   else
      lSetDouble(node, STN_proportion, 0);

   if (sibling_shares > 0)
      lSetDouble(node, STN_adjusted_proportion,
		 parent_percent *
		 (double)lGetUlong(node, STN_shares) / (double)sibling_shares);
   else
      lSetDouble(node, STN_adjusted_proportion, 0);

   for_each(child, lGetList(node, STN_children)) {
      calculate_share_percents(child, lGetDouble(node, STN_adjusted_proportion),
			       sum_shares);
   }

   return 0;
}

double
get_usage_value(lList *usage, char *name)
{
   lListElem *ue;
   double value = 0;
   if ((ue = lGetElemStr(usage, UA_name, name)))
      value = lGetDouble(ue, UA_value);
   return value;
}


typedef enum {
   ULONG_T=0,
   DATE_T,
   STRING_T,
   DOUBLE_T
} item_type_t;

typedef struct {
   char *name;
   item_type_t type;
   void *val;
} item_t;


static double mem, cpu, io, ltmem, ltcpu, ltio, level, total,
       lt_share, st_share, actual_share, combined_usage;
static lUlong current_time, time_stamp, shares, job_count;
static const char *node_name, *user_name, *project_name;

static item_t item[] = {
    { "curr_time", DATE_T, &current_time },
    { "usage_time", DATE_T, &time_stamp },
    { "node_name", STRING_T, &node_name },
    { "user_name", STRING_T, &user_name },
    { "project_name", STRING_T, &project_name },
    { "shares", ULONG_T, &shares },
    { "job_count", ULONG_T, &job_count },
    { "level%", DOUBLE_T, &level },
    { "total%", DOUBLE_T, &total },
    { "long_target_share", DOUBLE_T, &lt_share },
    { "short_target_share", DOUBLE_T, &st_share },
    { "actual_share", DOUBLE_T, &actual_share },
    { "usage", DOUBLE_T, &combined_usage },
    { "cpu", DOUBLE_T, &cpu },
    { "mem", DOUBLE_T, &mem },
    { "io", DOUBLE_T, &io },
    { "ltcpu", DOUBLE_T, &ltcpu },
    { "ltmem", DOUBLE_T, &ltmem },
    { "ltio", DOUBLE_T, &ltio }
};

static int items = sizeof(item) / sizeof(item_t);

int
print_field(FILE *out, item_t *item, format_t *format)
{
   if (format->name_format)
      fprintf(out, "%s=", item->name);

   switch(item->type) {
      case ULONG_T:
        fprintf(out, u32, *(lUlong *)item->val);
        break;
      case DATE_T:
        {
           time_t t = *(lUlong *)item->val;
           if (t && format->format_times) {
              char *tc = strdup(ctime(&t));
              if (tc && *tc)
                 tc[strlen(tc)-1] = 0;
              fprintf(out, format->str_format, tc);
              if (tc) free(tc);
           } else {
              fprintf(out, "%d", (int) t);
           }
        }
        break;
      case DOUBLE_T:
        fprintf(out, "%f", *(double *)item->val);
        break;
      case STRING_T:
        fprintf(out, format->str_format, *(char **)item->val);
        break;
   }

   fprintf(out, "%s", format->delim);
   return 0;
}


int
print_hdr(FILE *out, format_t *format)
{
   int i;
   if (format->field_names) {
      char *field;
      char *fields = strdup(format->field_names);
      field = strtok(fields, ",");
      while (field) {
         for (i=0; i<items; i++) {
            if (strcmp(field, item[i].name)==0) {
               fprintf(out, "%s%s", item[i].name, format->delim);
               break;
            }
         }
         field = strtok(NULL, ",");
      }
      free(fields);
   } else {
      for (i=0; i<items; i++)
         fprintf(out, "%s%s", item[i].name, format->delim);
   }
   fprintf(out, "%s", format->line_delim);
   fprintf(out, "%s", format->rec_delim);

   return 0;
}


int
print_node(FILE *out, lListElem *node, lListElem *parent, lListElem *user,
	   lListElem *project, char **names, format_t *format)
{
   lList *usage=NULL, *ltusage=NULL;
   int i, fields_printed=0;

   if (!node)
       return -1;

   current_time = sge_get_gmt();
   time_stamp = user?lGetUlong(user, UP_usage_time_stamp):0;
   node_name = lGetString(node, STN_name);
   user_name = user?lGetString(user, UP_name):"";
   project_name = project?lGetString(project, UP_name):"";
   shares = lGetUlong(node, STN_shares);
   job_count = lGetUlong(node, STN_job_ref_count);
   level = lGetDouble(node, STN_proportion);
   total = lGetDouble(node, STN_adjusted_proportion);
   lt_share = lGetDouble(node, STN_m_share);
   st_share = lGetDouble(node, STN_adjusted_current_proportion);
   actual_share = lGetDouble(node, STN_actual_proportion);
   combined_usage = lGetDouble(node, STN_combined_usage);

   if (lGetList(node, STN_children) == NULL && user && project) {
      lList *projl = lGetList(user, UP_project);
      lListElem *upp;
      if (projl) {
         if ((upp=lGetElemStr(projl, UPP_name,
                              lGetString(project, UP_name)))) {
            usage = lGetList(upp, UPP_usage);
            ltusage = lGetList(upp, UPP_long_term_usage);
         }
      }
   } else if (user &&
         strcmp(lGetString(user, UP_name), lGetString(node, STN_name))==0) {
      usage = lGetList(user, UP_usage);
      ltusage = lGetList(user, UP_long_term_usage);
   } else if (project &&
         strcmp(lGetString(project, UP_name), lGetString(node, STN_name))==0) {
      usage = lGetList(project, UP_usage);
      ltusage = lGetList(project, UP_long_term_usage);
   }

   if (usage) {
      cpu = get_usage_value(usage, USAGE_ATTR_CPU);
      mem = get_usage_value(usage, USAGE_ATTR_MEM);
      io  = get_usage_value(usage, USAGE_ATTR_IO);
   } else {
      cpu = mem = io = 0;
   }

   if (ltusage) {
      ltcpu = get_usage_value(ltusage, USAGE_ATTR_CPU);
      ltmem = get_usage_value(ltusage, USAGE_ATTR_MEM);
      ltio  = get_usage_value(ltusage, USAGE_ATTR_IO);
   } else {
      ltcpu = ltmem = ltio = 0;
   }

   if (names) {
      int found=0;
      char **name = names;
      while (*name) {
         if (strcmp(*name, node_name)==0)
            found = 1;
         name++;
      }
      if (!found)
         return 1;
   }
   
   if (format->field_names) {
      char *field;
      char *fields = strdup(format->field_names);
      field = strtok(fields, ",");
      while (field) {
         for (i=0; i<items; i++) {
            if (strcmp(field, item[i].name)==0) {
               print_field(out, &item[i], format);
	       fields_printed++;
               break;
            }
         }
         field = strtok(NULL, ",");
      }
      free(fields);
   } else {
      for (i=0; i<items; i++)
         print_field(out, &item[i], format);
         fields_printed++;
   }

   if (fields_printed)
      fprintf(out, "%s", format->line_delim);

   return 0;
}


int
print_nodes(FILE *out, lListElem *node, lListElem *parent,
            lListElem *project, lList *users, lList *projects,
	    int group_nodes, char **names, format_t *format)
{
   lListElem *user, *child;
   lList *children = lGetList(node, STN_children);

   if (!project)
      project = lGetElemStr(projects, UP_name, lGetString(node, STN_name));

   if (children == NULL)
      user = lGetElemStr(users, UP_name, lGetString(node, STN_name));
   else
      user = NULL;

   if (group_nodes || (children == NULL))
      print_node(out, node, parent, user, project, names, format);

   for_each(child, children) {
       print_nodes(out, child, node, project, users, projects, 
                   group_nodes, names, format);
   }

   return 0;
}


void
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


int
main(int argc, char **argv)
{
   lList *sharetree, *users, *projects, *config;
   lListElem *root;
   int interval=15;
   int err=0;
   int count=-1;
   int header=0;
   format_t format;
   char *ofile=NULL;
   char **names=NULL;
   int group_nodes=1;
   int decay_usage=0;
   int c;
   extern char *optarg;
   extern int optind;
   FILE *outfile = stdout;
   char *output_mode = "w";
   u_long curr_time=0;
   
   format.name_format=0;
   format.delim="\t";
   format.line_delim="\n";
   format.rec_delim="\n";
   format.str_format="%s";
   format.field_names=NULL;
   format.format_times=0;


#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   install_language_func((gettext_func_type)        gettext,
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
	    format.name_format = 1;
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
	    format.format_times = 1;
	    break;
	 case 'u':
	    decay_usage = 1;
	    break;
	 case 'x':
	    group_nodes = 0;
	    break;
	 case '?':
	    err++;
      }
   }

   if (err) {
      usage();
      exit(1);
   }

   if (ofile) {
      if ((outfile = fopen(ofile, output_mode)) == NULL) {
	 fprintf(stderr, MSG_FILE_COULDNOTOPENXFORY_SS , ofile, output_mode);
	 exit(1);
      }
   }

   if ((argc - optind) > 0)
       names = &argv[optind];

   sge_gdi_setup("sge_share_mon");

   if (header)
      print_hdr(outfile, &format);

   while(count == -1 || count-- > 0) {

      setup_lists(&sharetree, &users, &projects, &config);

      root = lFirst(sharetree);

      calculate_share_percents(root, 1.0, lGetUlong(root, STN_shares));

#ifdef notdef
      lSetDouble(root, STN_m_share, 1.0);
      calculate_m_shares(root);
#endif
 
      if (decay_usage)
         curr_time = sge_get_gmt();

      _sge_calc_share_tree_proportions(sharetree,
		users, 
		projects,
		config, NULL, curr_time);

      print_nodes(outfile, root, NULL, NULL, users, projects, group_nodes,
                  names, &format);

      free_lists(sharetree, users, projects, config);

      if (count) {
	 fprintf(outfile, "%s", format.rec_delim);
         sleep(interval);
      }

      fflush(outfile);
   }

   sge_gdi_shutdown();

   return 0;
}
