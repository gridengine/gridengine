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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <math.h>
#include <float.h>

#include "basis_types.h"
#include "sge.h"

#include "sge_bootstrap.h"

#include "sge_gdi.h"
#include "sge_all_listsL.h"
#include "commlib.h"
#include "sig_handlers.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_feature.h"
#include "sge_unistd.h"
#include "sge_stdlib.h"
#include "cull_parse_util.h"
#include "parse.h"
#include "sge_host.h"
#include "sge_complex_schedd.h"
#include "sge_parse_num_par.h"
#include "sge_select_queue.h"
#include "qstat_printing.h"
#include "sge_range.h"
#include "load_correction.h"
#include "sge_conf.h"
#include "msg_common.h"
#include "msg_clients_common.h"
#include "msg_qhost.h"
#include "sge_string.h"
#include "sge_hostname.h"
#include "sge_log.h"
#include "sge_answer.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_ulong.h"
#include "sge_centry.h"

#define QHOST_DISPLAY_QUEUES     (1<<0)
#define QHOST_DISPLAY_JOBS       (1<<1)
#define QHOST_DISPLAY_RESOURCES  (1<<2)

static lList *sge_parse_cmdline_qhost(char **argv, char **envp, lList **ppcmdline);
static lList *sge_parse_qhost(lList **ppcmdline, lList **pplres, lList **ppFres, lList **pphost, lList **ppuser, u_long32 *show);
static void qhost_usage(FILE *fp);
static void sge_print_queues(lList *ql, lListElem *hrl, lList *jl, lList *ul, lList *ehl, lList *cl, lList *pel, u_long32 show);
static void sge_print_resources(lList *ehl, lList *cl, lList *resl, lListElem *host, u_long32 show);
static void sge_print_host(lListElem *hep);
static int reformatDoubleValue(char *result, char *format, const char *oldmem);
static void get_all_lists(lList **ql, lList **jl, lList **cl, lList **ehl, lList **pel, lList *hl, lList *ul, u_long32 show);

extern char **environ;
#define INDENT    "     "

int main(int argc, char **argv);

/************************************************************************/
int main(
int argc,
char **argv 
) {
   lList *pcmdline = NULL;
   lList *ehl = NULL;
   lList *cl = NULL;
   lList *ul = NULL;
   lList *ql = NULL;
   lList *jl = NULL;
   lList *pel = NULL;
   lList *alp = NULL;
   lListElem *aep;
   lListElem *ep;
   u_long32 status = STATUS_OK;
   lList *resource_list = NULL;
   lList *resource_match_list = NULL;
   lList *host_list = NULL;
   u_long32 show = 0;
   lCondition *where = NULL;
   int print_header = 1;

   DENTER_MAIN(TOP_LAYER, "qhost");
  
   log_state_set_log_gui(1);

   sge_gdi_param(SET_MEWHO, QHOST, NULL);
   if (sge_gdi_setup(prognames[QHOST], &alp) != AE_OK) {
      answer_exit_if_not_recoverable(lFirst(alp));
      SGE_EXIT(1);
   }

   sge_setup_sig_handlers(QHOST);

   /*
   ** stage 1 of commandline parsing
   */
   alp = sge_parse_cmdline_qhost(argv, environ, &pcmdline);
   if(alp) {
      /*
      ** high level parsing error! sow answer list
      */
      for_each(aep, alp) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
      }
      lFreeList(alp);
      lFreeList(pcmdline);
      SGE_EXIT(1);
   }

   /*
   ** stage 2 of commandline parsing 
   */
   alp = sge_parse_qhost(
            &pcmdline, 
            &resource_match_list,   /* -l resource_request           */
            &resource_list,         /* -F qresource_request          */
            &host_list,             /* -h host_list                  */
            &ul,                    /* -u user_list                  */
            &show                   /* -q, -j                        */
         );

   if (alp) {
      /*
      ** low level parsing error! show answer list
      */
      for_each(aep, alp) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
      }
      alp = lFreeList(alp);
      pcmdline = lFreeList(pcmdline);
      SGE_EXIT(1);
   }


   get_all_lists(
      &ql, 
      &jl, 
      &cl, 
      &ehl, 
      &pel, 
      host_list, 
      ul,
      show);


   centry_list_init_double(cl);

   /*
   ** handle -l request for host
   */
   if (lGetNumberOfElem(resource_match_list)) {
      int selected;

      if (centry_list_fill_request(resource_match_list, cl, true, true, false)) {
         /* error message gets written by centry_list_fill_request into SGE_EVENT */
         SGE_EXIT(1);
      }
      /* prepare request */
      for_each(ep, ehl) {

         /* prepare complex attributes */
         if (!strcmp(lGetHost(ep, EH_name), SGE_TEMPLATE_NAME))
            continue;

         DPRINTF(("matching host %s with qhost -l\n", lGetHost(ep, EH_name)));

         selected = sge_select_queue(resource_match_list, NULL, ep, ehl, cl, 1, -1);

         if (selected) 
            lSetUlong(ep, EH_tagged, 1);
      }

      /*
      ** reduce the hostlist, only the tagged ones survive
      */
      where = lWhere("%T(%I == %u)", EH_Type, EH_tagged, 1);
      lSplit(&ehl, NULL, NULL, where);
      where = lFreeWhere(where);
   }

   /* scale load values and adjust consumable capacities */
/*    TODO                                            */
/*    is correct_capacities needed here ???           */
/*    correct_capacities(ehl, cl);                    */

   /* SGE_GLOBAL_NAME should be printed at first */
   lPSortList(ehl, "%I+", EH_name);
   ep = NULL;
   where = lWhere("%T(%I == %s)", EH_Type, EH_name, SGE_GLOBAL_NAME );
   ep = lDechainElem(ehl, lFindFirst(ehl, where));
   lFreeWhere(where); 
   if (ep) {
      lInsertElem(ehl,NULL,ep); 
   }

   /*
   ** format and print the info
   */

#define HEAD_FORMAT "%-23s %-13.13s%4.4s %5.5s %7.7s %7.7s %7.7s %7.7s\n"

   for_each(ep, ehl) {
      if (print_header) {
         print_header = 0;
         printf(HEAD_FORMAT,  MSG_HEADER_HOSTNAME, MSG_HEADER_ARCH, MSG_HEADER_NPROC, MSG_HEADER_LOAD,
             MSG_HEADER_MEMTOT, MSG_HEADER_MEMUSE, MSG_HEADER_SWAPTO, MSG_HEADER_SWAPUS);
         printf("-------------------------------------------------------------------------------\n");
      }
      sge_print_host(ep);
      sge_print_resources(ehl, cl, resource_list, ep, show);
      sge_print_queues(ql, ep, jl, NULL, ehl, cl, pel, show);
   }   

   lFreeList(ehl);
   lFreeList(alp);

   SGE_EXIT(status==STATUS_OK?0:1); /* 0 means ok - others are errors */
   DEXIT;
   return 0;
}


/*-------------------------------------------------------------------------*/
static void sge_print_host(
lListElem *hep 
) {
   lListElem *lep;
   char *s,host_print[MAXHOSTLEN+1];
   const char *host, *arch, *num_proc;
   char load_avg[20], mem_total[20], mem_used[20], swap_total[20], swap_used[20];

   DENTER(TOP_LAYER, "sge_print_host");
   
   /*
   ** host name
   */
   host = lGetHost(hep, EH_name);

   /* cut away domain in case of ignore_fqdn */
   strncpy(host_print, host, MAXHOSTLEN);
   if (bootstrap_get_ignore_fqdn() && (s = strchr(host_print, '.')))
      *s = '\0';

   /*
   ** arch
   */
   lep=lGetSubStr(hep, HL_name, "arch", EH_load_list);
   if (lep)
      arch = lGetString(lep, HL_value); 
   else
      arch = "-";
   
   /*
   ** num_proc
   */
   lep=lGetSubStr(hep, HL_name, "num_proc", EH_load_list);
   if (lep)
      num_proc = lGetString(lep, HL_value); 
   else
      num_proc = "-";

   /*
   ** load_avg
   */
   lep=lGetSubStr(hep, HL_name, "load_avg", EH_load_list);
   if (lep)
      reformatDoubleValue(load_avg, "%.2f%c", lGetString(lep, HL_value)); 
   else
      strcpy(load_avg, "-");
   
   /*
   ** mem_total
   */
   lep=lGetSubStr(hep, HL_name, "mem_total", EH_load_list);
   if (lep)
      reformatDoubleValue(mem_total, "%.1f%c", lGetString(lep, HL_value)); 
   else
      strcpy(mem_total, "-");
   
   /*
   ** mem_used
   */
   lep=lGetSubStr(hep, HL_name, "mem_used", EH_load_list);
   if (lep)
      reformatDoubleValue(mem_used, "%.1f%c", lGetString(lep, HL_value)); 
   else
      strcpy(mem_used, "-");
   
   /*
   ** swap_total
   */
   lep=lGetSubStr(hep, HL_name, "swap_total", EH_load_list);
   if (lep)
      reformatDoubleValue(swap_total, "%.1f%c", lGetString(lep, HL_value)); 
   else
      strcpy(swap_total, "-");
   
   /*
   ** swap_used
   */
   lep=lGetSubStr(hep, HL_name, "swap_used", EH_load_list);
   if (lep)
      reformatDoubleValue(swap_used, "%.1f%c", lGetString(lep, HL_value)); 
   else
      strcpy(swap_used, "-");
   
   printf(HEAD_FORMAT, host_print ? host_print: "-", arch, num_proc, load_avg, 
                     mem_total, mem_used, swap_total, swap_used);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void sge_print_queues(
lList *qlp,
lListElem *host,
lList *jl,
lList *ul,
lList *ehl,
lList *cl,
lList *pel,
u_long32 show 
) {
   lList *load_thresholds, *suspend_thresholds;
   lListElem *qep, *cqueue;

   DENTER(TOP_LAYER, "sge_print_queues");

   for_each(cqueue, qlp) {
      lList *qinstance_list = lGetList(cqueue, CQ_qinstances);

      for_each(qep, qinstance_list) {
         if (!sge_hostcmp(lGetHost(qep, QU_qhostname), 
                          lGetHost(host, EH_name))) {
            char buf[80];

            if (show & QHOST_DISPLAY_QUEUES) { 
               /*
               ** Header/indent
               */
               printf("   ");
               /*
               ** qname
               */
               printf("%-20s ", lGetString(qep, QU_qname));
               /*
               ** qtype
               */
               {
                  dstring type_string = DSTRING_INIT;

                  qinstance_print_qtype_to_dstring(qep, &type_string, true);
                  printf("%-5.5s ", sge_dstring_get_string(&type_string));
                  sge_dstring_free(&type_string);
               }

               /* 
               ** number of used/free slots 
               */
               sprintf(buf, "%d/%d ",
                  qinstance_slots_used(qep),
                  (int)lGetUlong(qep, QU_job_slots));
               printf("%-9.9s", buf);
               /*
               ** state of queue
               */
               load_thresholds = lGetList(qep, QU_load_thresholds);
               suspend_thresholds = lGetList(qep, QU_suspend_thresholds);
               if (sge_load_alarm(NULL, qep, load_thresholds, ehl, cl, NULL)) {
                  qinstance_state_set_alarm(qep, true);
               }
               if (sge_load_alarm(NULL, qep, suspend_thresholds, ehl, cl, NULL)) {
                  qinstance_state_set_suspend_alarm(qep, true);
               }
               {
                  dstring state_string = DSTRING_INIT;

                  qinstance_state_append_to_dstring(qep, &state_string);
                  printf("%s", sge_dstring_get_string(&state_string));
                  sge_dstring_free(&state_string);
               }
               
               /*
               ** newline
               */
               printf("\n");
            }

            /*
            ** tag all jobs, we have only fetched running jobs, so every job
            ** should be visible (necessary for the qstat printing functions)
            */
            if (show & QHOST_DISPLAY_JOBS) {
               sge_print_jobs_queue(qep, jl, pel, ul, ehl, cl, 1,
                                    QSTAT_DISPLAY_ALL | 
                                    ( (show & QHOST_DISPLAY_QUEUES) ?
                                     QSTAT_DISPLAY_FULL : 0), "   ", 
                                     GROUP_NO_PETASK_GROUPS);
            }
         }
      }
   }
   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void sge_print_resources(
lList *ehl,
lList *cl,
lList *resl,
lListElem *host,
u_long32 show 
) {
   lList *rlp = NULL;
   lListElem *rep;
   char dom[5];
   dstring resource_string = DSTRING_INIT;
   const char *s;
   u_long32 dominant;
   int first = 1;

   DENTER(TOP_LAYER, "sge_print_resources");

   if (!(show & QHOST_DISPLAY_RESOURCES)) {
      DEXIT;
      return;
   }
   host_complexes2scheduler(&rlp, host, ehl, cl);
   for_each (rep , rlp) {
      if (resl) {
         lListElem *r1;
         int found = 0;
         int first_item = 0;
         for_each (r1, resl) {
            if (!strcmp(lGetString(r1, ST_name), lGetString(rep, CE_name)) ||
                !strcmp(lGetString(r1, ST_name), lGetString(rep, CE_shortcut))) {
               found = 1;
               if (first) {
                  first = 0;
                  first_item = 1;
                  printf("    Host Resource(s):   ");
               }
               break;
            }
         }
         if (!found)
            continue;
      }

      { 
         u_long32 type = lGetUlong(rep, CE_valtype);

         sge_dstring_clear(&resource_string);

         switch (type) {
         case TYPE_HOST:   
         case TYPE_STR:   
         case TYPE_CSTR:   
         case TYPE_RESTR:
            if (!(lGetUlong(rep, CE_pj_dominant)&DOMINANT_TYPE_VALUE)) {
               dominant = lGetUlong(rep, CE_pj_dominant);
               s = lGetString(rep, CE_pj_stringval);
            } else {
               dominant = lGetUlong(rep, CE_dominant);
               s = lGetString(rep, CE_stringval);
            }
            break;
         case TYPE_TIM:
            if (!(lGetUlong(rep, CE_pj_dominant)&DOMINANT_TYPE_VALUE)) {
               double val = lGetDouble(rep, CE_pj_doubleval);

               dominant = lGetUlong(rep, CE_pj_dominant);
               double_print_time_to_dstring(val, &resource_string);
               s = sge_dstring_get_string(&resource_string);
            } else {
               double val = lGetDouble(rep, CE_doubleval);

               dominant = lGetUlong(rep, CE_dominant);
               double_print_time_to_dstring(val, &resource_string);
               s = sge_dstring_get_string(&resource_string);
            }
         case TYPE_MEM:
            if (!(lGetUlong(rep, CE_pj_dominant)&DOMINANT_TYPE_VALUE)) {
               double val = lGetDouble(rep, CE_pj_doubleval);

               dominant = lGetUlong(rep, CE_pj_dominant);
               double_print_memory_to_dstring(val, &resource_string);
               s = sge_dstring_get_string(&resource_string);
            } else {
               double val = lGetDouble(rep, CE_doubleval);

               dominant = lGetUlong(rep, CE_dominant);
               double_print_memory_to_dstring(val, &resource_string);
               s = sge_dstring_get_string(&resource_string);
            }
         default:   
            if (!(lGetUlong(rep, CE_pj_dominant)&DOMINANT_TYPE_VALUE)) {
               double val = lGetDouble(rep, CE_pj_doubleval);

               dominant = lGetUlong(rep, CE_pj_dominant);
               double_print_to_dstring(val, &resource_string);
               s = sge_dstring_get_string(&resource_string);
            } else {
               double val = lGetDouble(rep, CE_doubleval);

               dominant = lGetUlong(rep, CE_dominant);
               double_print_to_dstring(val, &resource_string);
               s = sge_dstring_get_string(&resource_string);
            }
            break;
         }
      }
      monitor_dominance(dom, dominant); 
      switch(lGetUlong(rep, CE_valtype)) {
      case TYPE_INT:  
      case TYPE_TIM:  
      case TYPE_MEM:  
      case TYPE_BOO:  
      case TYPE_DOUBLE:  
      default:
         printf("   ");
         printf("%s:%s=%s\n", dom, lGetString(rep, CE_name), s);
         break;
      }
   }
   lFreeList(rlp);
   sge_dstring_free(&resource_string);
   DEXIT;
}

/*
** NAME
**   qhost_usage
** PARAMETER
**   none
** RETURN
**   none
** EXTERNAL
**   none
** DESCRIPTION
**   displays qhost_usage for qlist client
**   note that the other clients use a common function
**   for this. output was adapted to a similar look.
*/
static void qhost_usage(
FILE *fp 
) {
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "qhost_usage");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   fprintf(fp, "%s\n", feature_get_product_name(FS_SHORT_VERSION, &ds));

   fprintf(fp,"%s qhost [options]\n", MSG_SRC_USAGE);
         
   fprintf(fp, "  [-help]                    %s", MSG_QHOST_help_OPT_USAGE);
   fprintf(fp, "  [-h hostlist]              %s", MSG_QHOST_h_OPT_USAGE);
   fprintf(fp, "  [-q]                       %s", MSG_QHOST_q_OPT_USAGE);
   fprintf(fp, "  [-j]                       %s", MSG_QHOST_j_OPT_USAGE);
   fprintf(fp, "  [-l attr=val,...]          %s", MSG_QHOST_l_OPT_USAGE);
   fprintf(fp, "  [-F [resource_attribute]]  %s", MSG_QHOST_F_OPT_USAGE); 
   fprintf(fp, "  [-u user[,user,...]]       %s", MSG_QHOST_u_OPT_USAGE); 

   if (fp==stderr) {
      SGE_EXIT(1);
   } else {
      SGE_EXIT(0);   
   }
   DEXIT;
}

/****
 **** sge_parse_cmdline_qhost (static)
 ****
 **** 'stage 1' parsing of qhost-options. Parses options
 **** with their arguments and stores them in ppcmdline.
 ****/ 
static lList *sge_parse_cmdline_qhost(
char **argv,
char **envp,
lList **ppcmdline 
) {
   char **sp;
   char **rp;
   stringT str;
   lList *alp = NULL;
   DENTER(TOP_LAYER, "sge_parse_cmdline_qhost");

   rp = ++argv;
   while(*(sp=rp)) {
      /* -help */
      if ((rp = parse_noopt(sp, "-help", NULL, ppcmdline, &alp)) != sp)
         continue;
 
      /* -q option */
      if ((rp = parse_noopt(sp, "-q", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -F */
      if ((rp = parse_until_next_opt2(sp, "-F", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -h */
      if ((rp = parse_until_next_opt(sp, "-h", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -j */
      if ((rp = parse_noopt(sp, "-j", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -l */
      if ((rp = parse_until_next_opt(sp, "-l", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -u */
      if ((rp = parse_until_next_opt(sp, "-u", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* oops */
      sprintf(str, MSG_PARSE_INVALIDOPTIONARGUMENTX_S, *sp);
      qhost_usage(stderr);
      answer_list_add(&alp, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return alp;
   }
   DEXIT;
   return alp;
}

/****
 **** sge_parse_qhost (static)
 ****
 **** 'stage 2' parsing of qhost-options. Gets the options from pcmdline
 ****/
static lList *sge_parse_qhost(
lList **ppcmdline,
lList **pplres,
lList **ppFres,
lList **pphost,
lList **ppuser,
u_long32 *show 
) {
stringT str;
lList *alp = NULL;
u_long32 helpflag;
int usageshowed = 0;
u_long32 full = 0;
char * argstr = NULL;
lListElem *ep;
 
   DENTER(TOP_LAYER, "sge_parse_host");
 
   /* Loop over all options. Only valid options can be in the
      ppcmdline list. 
   */
   while(lGetNumberOfElem(*ppcmdline))
   {
      if(parse_flag(ppcmdline, "-help",  &alp, &helpflag)) {
         qhost_usage(stdout);
         SGE_EXIT(0);
         break;
      }

      if (parse_multi_stringlist(ppcmdline, "-h", &alp, pphost, ST_Type, ST_name)) {
         /* 
         ** resolve hostnames and replace them in list
         */
         for_each(ep, *pphost) {
#ifdef ENABLE_NGC
            if (sge_resolve_host(ep, ST_name) != CL_RETVAL_OK) 
#else
            if (sge_resolve_host(ep, ST_name)) 
#endif
            {
               char buf[BUFSIZ];
               sprintf(buf, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetString(ep,ST_name) );
               answer_list_add(&alp, buf, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
               DEXIT;
               return alp; 
            }

         }
         continue;
      }

      if (parse_multi_stringlist(ppcmdline, "-F", &alp, ppFres, ST_Type, ST_name)) {
         (*show) |= QHOST_DISPLAY_RESOURCES;
         continue;
      }
      if(parse_flag(ppcmdline, "-q", &alp, &full)) {
         if(full) {
            (*show) |= QHOST_DISPLAY_QUEUES;
            full = 0;
         }
         continue;
      }

      if(parse_flag(ppcmdline, "-j", &alp, &full)) {
         if(full) {
            (*show) |= QHOST_DISPLAY_JOBS;
            full = 0;
         }
         continue;
      }

      if(parse_string(ppcmdline, "-l", &alp, &argstr)) {
         *pplres = centry_list_parse_from_string(*pplres, argstr, true);
         FREE(argstr);
         continue;
      }

      if (parse_multi_stringlist(ppcmdline, "-u", &alp, ppuser, ST_Type, ST_name)) {
         (*show) |= QHOST_DISPLAY_JOBS;
         continue;
      }


   }
   if(lGetNumberOfElem(*ppcmdline)) {
     sprintf(str, MSG_PARSE_TOOMANYOPTIONS);
     if (!usageshowed)
        qhost_usage(stderr);
     answer_list_add(&alp, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
     DEXIT;
     return alp;
   }
   DEXIT;
   return alp;
}

/*-------------------------------------------------------------------------*/
static int reformatDoubleValue(
char *result,
char *format,
const char *oldmem 
) {
   char c;
   double dval;
   int ret = 1;

   DENTER(TOP_LAYER, "reformatDoubleValue");

   if (parse_ulong_val(&dval, NULL, TYPE_MEM, oldmem, NULL, 0)) {
      if (dval==DBL_MAX) {
         strcpy(result, "infinity");
      } else {
         c = '\0';

         if (fabs(dval) >= 1024*1024*1024) {
            dval /= 1024*1024*1024;
            c = 'G';
         } else if (fabs(dval) >= 1024*1024) {
            dval /= 1024*1024;
            c = 'M';
         } else if (fabs(dval) >= 1024) {
            dval /= 1024;
            c = 'K';
         }
         sprintf(result, format, dval, c);
      }
   }
   else {
      strcpy(result, ""); 
      ret = 0;
   }
   DEXIT;
   return ret;
}

/****
 **** get_all_lists (static)
 ****
 **** Gets copies of queue-, job-, complex-, exechost-list  
 **** from qmaster.
 **** The lists are stored in the .._l pointerpointer-parameters.
 **** WARNING: Lists previously stored in this pointers are not destroyed!!
 ****/
static void get_all_lists(
lList **queue_l,
lList **job_l,
lList **centry_l,
lList **exechost_l,
lList **pe_l,
lList *hostref_list,
lList *user_list,
u_long32 show 
) {
   lCondition *where= NULL, *nw = NULL, *qw = NULL, *jw = NULL, *gc_where;
   lEnumeration *q_all = NULL, *j_all = NULL, *ce_all = NULL, 
                *eh_all = NULL, *pe_all = NULL, *gc_what;
   lList *alp = NULL;
   lListElem *aep = NULL;
   lListElem *ep = NULL;
   lListElem *jatep = NULL;
   lList *mal = NULL;
   lList *conf_l = NULL;
   int q_id, j_id = 0, ce_id, eh_id, pe_id, gc_id;
   state_gdi_multi state = STATE_GDI_MULTI_INIT;

   DENTER(TOP_LAYER, "get_all_lists");
   
#if 0
   /*
   ** 2nd solution:
   ** request info from qmaster, qmaster handles which lists
   ** are needed
   ** ehl serves as a inout container for delivering request info
   ** and receiving the result
   */
   ehl = host_list;
lWriteListTo(ehl, stdout);
   alp = sge_gdi(SGE_QHOST, SGE_GDI_GET, &ehl, NULL, NULL);
#endif

   /*
   ** exechosts
   ** build where struct to filter out  either all hosts or only the 
   ** hosts listed in host_list
   */

   for_each(ep, hostref_list) {
      nw = lWhere("%T(%I == %s)", EH_Type, EH_name, lGetString(ep, ST_name));
      if (!where)
         where = nw;
      else
         where = lOrWhere(where, nw);
   }
   /* the global host has to be retrieved as well */
   if (where != NULL) {
      nw = lWhere("%T(%I == %s)", EH_Type, EH_name, SGE_GLOBAL_NAME);
      where = lOrWhere(where, nw);
   }
   
   nw = lWhere("%T(%I != %s)", EH_Type, EH_name, SGE_TEMPLATE_NAME);
   if (where)
      where = lAndWhere(where, nw);
   else
      where = nw;
   eh_all = lWhat("%T(ALL)", EH_Type);
   eh_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_EXECHOST_LIST, SGE_GDI_GET, 
                        NULL, where, eh_all, NULL, &state);
   eh_all = lFreeWhat(eh_all);
   where = lFreeWhere(where);

   if (alp) {
      printf("%s", lGetString(lFirst(alp), AN_text));
      SGE_EXIT(1);
   }

   q_all = lWhat("%T(ALL)", QU_Type);
   q_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_CQUEUE_LIST, SGE_GDI_GET, 
                        NULL, NULL, q_all, NULL, &state);
   q_all = lFreeWhat(q_all);
   qw = lFreeWhere(qw);

   if (alp) {
      printf("%s\n", lGetString(lFirst(alp), AN_text));
      SGE_EXIT(1);
   }

   /* 
   ** jobs 
   */ 
   if (job_l && (show & QHOST_DISPLAY_JOBS)) {

/* lWriteListTo(user_list, stdout); */

      for_each(ep, user_list) {
         nw = lWhere("%T(%I p= %s)", JB_Type, JB_owner, lGetString(ep, ST_name));
         if (!jw)
            jw = nw;
         else
            jw = lOrWhere(jw, nw);
      }
/* printf("-------------------------------------\n"); */
/* lWriteWhereTo(jw, stdout); */
      if (!(show & QSTAT_DISPLAY_PENDING)) {
         nw = lWhere("%T(%I->%T(!(%I m= %u)))", JB_Type, JB_ja_tasks, JAT_Type, JAT_state, JQUEUED);
         if (!jw)
            jw = nw;
         else
            jw = lAndWhere(jw, nw);
      }

      j_all = lWhat("%T(%I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I)", JB_Type, 
                     JB_job_number, 
                     JB_script_file,
                     JB_owner,
                     JB_group,
                     JB_type,
                     JB_pe,
                     JB_checkpoint_name,
                     JB_jid_predecessor_list,
                     JB_env_list,
                     JB_priority,
                     JB_jobshare,
                     JB_job_name,
                     JB_project,
                     JB_department,
                     JB_submission_time,
                     JB_deadline,
                     JB_override_tickets,
                     JB_pe_range,
                     JB_hard_resource_list,
                     JB_soft_resource_list,
                     JB_hard_queue_list,
                     JB_soft_queue_list,
                     JB_ja_structure,
                     JB_ja_tasks,
                     JB_ja_n_h_ids,
                     JB_ja_u_h_ids,
                     JB_ja_s_h_ids,
                     JB_ja_o_h_ids,
                     JB_ja_z_ids 
                    );

/* printf("======================================\n"); */
/* lWriteWhereTo(jw, stdout); */

      j_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_JOB_LIST, SGE_GDI_GET, 
                           NULL, jw, j_all, NULL, &state);
      j_all = lFreeWhat(j_all);
      jw = lFreeWhere(jw);

      if (alp) {
         printf("%s", lGetString(lFirst(alp), AN_text));
         SGE_EXIT(1);
      }
   }

   /*
   ** complexes
   */
   ce_all = lWhat("%T(ALL)", CE_Type);
   ce_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_CENTRY_LIST, SGE_GDI_GET, 
                        NULL, NULL, ce_all, NULL, &state);
   ce_all = lFreeWhat(ce_all);

   if (alp) {
      printf("%s", lGetString(lFirst(alp), AN_text));
      SGE_EXIT(1);
   }

   /*
   ** pe list
   */
   pe_all = lWhat("%T(ALL)", PE_Type);
   pe_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_PE_LIST, SGE_GDI_GET, 
                           NULL, NULL, pe_all, NULL, &state);
   pe_all = lFreeWhat(pe_all);

   if (alp) {
      printf("%s", lGetString(lFirst(alp), AN_text));
      SGE_EXIT(1);
   }

   /*
   ** global cluster configuration
   */
   gc_where = lWhere("%T(%I c= %s)", CONF_Type, CONF_hname, SGE_GLOBAL_NAME);
   gc_what = lWhat("%T(ALL)", CONF_Type);
   gc_id = sge_gdi_multi(&alp, SGE_GDI_SEND, SGE_CONFIG_LIST, SGE_GDI_GET,
                        NULL, gc_where, gc_what, &mal, &state);
   gc_what = lFreeWhat(gc_what);
   gc_where = lFreeWhere(gc_where);

   if (alp) {
      printf("%s", lGetString(lFirst(alp), AN_text));
      SGE_EXIT(1);
   }


   /*
   ** handle results
   */
   /* --- exec host */
   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_EXECHOST_LIST, eh_id, 
                                 mal, exechost_l);
   if (!alp) {
      printf(MSG_GDI_EXECHOSTSGEGDIFAILED);
      SGE_EXIT(1);
   }
   if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
      printf("%s", lGetString(aep, AN_text));
      SGE_EXIT(1);
   }
   alp = lFreeList(alp);

   /* --- queue */
   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_CQUEUE_LIST, q_id, 
                                 mal, queue_l);
   if (!alp) {
      printf(MSG_GDI_QUEUESGEGDIFAILED);
      SGE_EXIT(1);
   }
   if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
      printf("%s", lGetString(aep, AN_text));
      SGE_EXIT(1);
   }
   alp = lFreeList(alp);

   /* --- job */
   if (job_l && (show & QHOST_DISPLAY_JOBS)) {
      lListElem *ep = NULL;
      alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_JOB_LIST, j_id, mal, job_l);
      if (!alp) {
         printf(MSG_GDI_JOBSGEGDIFAILED);
         SGE_EXIT(1);
      }
      if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
         printf("%s", lGetString(aep, AN_text));
         SGE_EXIT(1);
      }
      /*
      ** tag the jobs, we need it for the printing functions
      */
      for_each(ep, *job_l) 
         for_each(jatep, lGetList(ep, JB_ja_tasks))
            lSetUlong(jatep, JAT_suitable, TAG_SHOW_IT);

      alp = lFreeList(alp);
   }

   /* --- complex attribute */
   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_CENTRY_LIST, ce_id,
                                 mal, centry_l);
   if (!alp) {
      printf(MSG_GDI_COMPLEXSGEGDIFAILED);
      SGE_EXIT(1);
   }
   if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
      printf("%s", lGetString(aep, AN_text));
      SGE_EXIT(1);
   }
   alp = lFreeList(alp);

   /* --- pe */
   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_PE_LIST, pe_id,
                                 mal, pe_l);
   if (!alp) {
      printf(MSG_GDI_COMPLEXSGEGDIFAILED);
      SGE_EXIT(1);
   }
   if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
      printf("%s", lGetString(aep, AN_text));
      SGE_EXIT(1);
   }
   alp = lFreeList(alp);

   /* --- apply global configuration for sge_hostcmp() scheme */
   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_CONFIG_LIST, gc_id, mal, &conf_l);
   if (!alp) {
      printf(MSG_GDI_SCHEDDCONFIGSGEGDIFAILED);
      SGE_EXIT(1);
   }
   if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
      printf("%s", lGetString(aep, AN_text));
      SGE_EXIT(1);
   }
   if (lFirst(conf_l)) {
      lListElem *local = NULL;
      merge_configuration(lFirst(conf_l), local, &conf, NULL);
   }
   alp = lFreeList(alp);

   mal = lFreeList(mal);

   DEXIT;
   return;
}
