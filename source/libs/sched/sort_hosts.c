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
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if !defined(WIN32) && !defined(DARWIN) && !defined(FREEBSD)
#   include <malloc.h>
#endif

#include "sgermon.h"
#include "sge.h"
#include "cull.h"
#include "sge_all_listsL.h"
#include "sge_parse_num_par.h"
#include "sort_hosts.h"
#include "sge_complex_schedd.h"
#include "sge_sched.h"
#include "sge_schedd_conf.h"
#include "sge_feature.h"
#include "sge_string.h"
#include "sge_log.h"
#include "sge_host.h"
#include "sge_qinstance.h"
#include "msg_schedd.h"

static char load_ops[]={
        '+',
        '-',
        '*',
        '/',
        '&',
        '|',
        '^',
        '\0'
};


enum {
        LOAD_OP_NONE=-1,
        LOAD_OP_PLUS,
        LOAD_OP_MINUS,
        LOAD_OP_TIMES,
        LOAD_OP_DIV,
        LOAD_OP_AND,
        LOAD_OP_OR,
        LOAD_OP_XOR
};

/* prototypes */
static double scaled_mixed_load( lListElem *global, lListElem *host, const lList *centry_list);
static int get_load_value(double *dvalp, lListElem *global, lListElem *host, const lList *centry_list, const char *attrname);

/*************************************************************************

   sort_host_list

   purpose:
      sort host list according to a load evaluation formula.

   return values:
      0 on success; -1 otherwise

   input parameters:
      hl             :  the host list to be sorted
      centry_list    :  the complex entry list
      formula        :  the load evaluation formula (containing no blanks)

   output parameters:
      hl             :  the sorted host list

*************************************************************************/
/* TODO SG: change from complex list to accessing the necisary parts directly */
int sort_host_list(
lList *hl,           /* EH_Type */ 
lList *centry_list   /* CE_Type */
) {
   lListElem *hlp;
   lListElem *global;
   const char *host;
   double load;
/*   lList *tcl = NULL;*/

   
   DENTER(TOP_LAYER, "sort_host_list");

   global = host_list_locate(hl, "global");

   for_each (hlp, hl) {
      host = lGetHost(hlp,EH_name);
      if (strcmp(host,"global")) { /* don't treat global */
         /* build complexes for that host */
         lSetDouble(hlp, EH_sort_value, load = scaled_mixed_load(global, hlp, centry_list));
         DPRINTF(("%s: %f\n", lGetHost(hlp, EH_name), load));
      }
   }

   if (lPSortList(hl,"%I+",EH_sort_value)) {
      DEXIT;
      return -1;
   } else {
      DEXIT;
      return 0;
   }
}


/*************************************************************************
   scaled_mixed_load:

   purpose:
      compute scaled and weighted load for a particular host according
      to a load formula and that host's load and load scaling lists.

   return value:
      load value to be used for sorting the host list. in case of
      errors, a value of ERROR_LOAD_VAL is returned thereby ensuring
      that hosts with incorrect load reporting are considered heavily
      loaded.

   input paramters:
      load_list      : the host's load list  -> to be passed further
      scaling_list   : the host's load scaling list   -> to be passed
                       further
      host_cplx      : the entries list of the host complex -> to be
                       passed further
      lc_factor      : factor that is used to implement load correction: 
                       0 means no load correction, 
                       n load correction for n new jobs
*************************************************************************/
static double scaled_mixed_load( lListElem *global, lListElem *host, const lList *centry_list)
{
   char *cp, *tf, *ptr, *ptr2, *par_name, *op_ptr=NULL;
   double val=0, val2=0;
   double load=0;
   int op_pos, next_op=LOAD_OP_NONE;
   const char *load_formula = sconf_get_load_formula();
   DENTER(TOP_LAYER, "scaled_mixed_load");

   /* we'll use strtok ==> we need a safety copy */
   if (!(tf = strdup(load_formula))) {
      DEXIT;
      return ERROR_LOAD_VAL;
   }

   /* + and - have the lowest precedence. all else are equal,
    * thus formula is delimited by + or - signs
    */
   for (cp=strtok(tf, "+-"); cp; cp = strtok(NULL, "+-")) {

      /* ---------------------------------------- */
      /* get scaled load value                    */
      /* determine 1st components value           */
      if (!(val = strtol(cp, &ptr, 0)) && ptr == cp) {
         /* it is not an integer ==> it's got to be a load value */
         if (!(par_name = sge_delim_str(cp,&ptr,load_ops)) ||
/*               get_load_value(&val, tcl, par_name, source_name)) {*/
               get_load_value(&val, global, host, centry_list, par_name)) {
            if (par_name)
               free(par_name);
            free(tf);
            DEXIT;
            return ERROR_LOAD_VAL;
         }
         free(par_name);
         par_name=NULL;
      }

      /* ---------------------------------------- */
      /* for the load value                       */
      /* *ptr now contains the delimiting character for val */
      if (*ptr) {
         /* if the delimiter is not \0 it's got to be a operator -> find it */
         if (!(op_ptr=strchr(load_ops,(int) *ptr))) {
            free(tf);
            DEXIT;
            return ERROR_LOAD_VAL;
         }
         op_pos = (int) (op_ptr - load_ops);

         /* ------------------------------- */
         /* look for a weightening factors  */
         /* determine 2nd component's value */
         ptr++;
         if (!(val2 = (double)strtol(ptr,&ptr2,0)) && ptr2 == ptr) {
            /* it is not an integer ==> it's got to be a load value */
            if (!(par_name = sge_delim_str(ptr,NULL,load_ops)) ||
               get_load_value(&val2, global, host, centry_list, par_name)) {
               if (par_name)
                  free(par_name);
               free(tf);
               DEXIT;
               return ERROR_LOAD_VAL;
            }
            free(par_name);
            par_name=NULL;
         }

         /* ------------------------------- */
         /*  apply according load operator  */
         switch (op_pos) {
            case LOAD_OP_TIMES:
               val *= val2;
               break;
            case LOAD_OP_DIV:
               val /= val2;
               break;
            case LOAD_OP_AND: {
               u_long32 tmp;
               tmp = (u_long32)val & (u_long32)val2;
               val = (double)tmp;
               break;
            }
            case LOAD_OP_OR: {
               u_long32 tmp;
               tmp = (u_long32)val | (u_long32)val2; 
               val = (double)tmp;
               break;
            }
            case LOAD_OP_XOR: {
               u_long32 tmp;
               tmp = (u_long32)val ^ (u_long32)val2;
               val = (double)tmp;
               break;
            }
         }     /* switch (op_pos) */
      }     /* if (*ptr) */
   
      /* now we have the intermediate result from the subexpression in
       * between a + or - operator in val. next we've to add or
       * subtract from the current result value.
       */

      /* next_op is the next operation from the last run of the while loop.
       * thus next_op now is the operation to applied
       */
      switch (next_op) {
         case LOAD_OP_NONE:
            /* this is the first run -> just set load */
            load = val;
            break;
         case LOAD_OP_PLUS:
            load += val;
            break;
         case LOAD_OP_MINUS:
            load -= val;
            break;
      }
      
      /* determine next_op from the safety copy of the stripped formula */
      if (load_formula[cp-tf+strlen(cp)] == '+')
         next_op = LOAD_OP_PLUS;
      else
         next_op = LOAD_OP_MINUS;
   }

   free(tf);
   DEXIT;
   return load;
}



/***********************************************************************

   get_load_value

 ***********************************************************************/
static int get_load_value(double *dvalp, lListElem *global, lListElem *host, const lList *centry_list, const char *attrname) 
{
   lListElem *cep;
   u_long32 dominant;

   DENTER(TOP_LAYER, "get_load_value");

   /* search complex */

   if(!(cep = get_attribute_by_name(global, host, NULL, attrname, centry_list, DISPATCH_TIME_NOW, 0))){
      /* 
       * admin has forgotten to configure complex for 
       * load value in load formula 
       */

      ERROR((SGE_EVENT, MSG_ATTRIB_NOATTRIBXINCOMPLEXLIST_SS , attrname, lGetHost(host, EH_name) ));
      DEXIT;
      return 1;
   }

   if (lGetUlong(cep, CE_pj_dominant) & DOMINANT_TYPE_VALUE) {
      *dvalp = lGetDouble(cep, CE_doubleval);
      dominant = lGetUlong(cep, CE_dominant);
   } else {
      *dvalp = lGetDouble(cep, CE_pj_doubleval);
      dominant = lGetUlong(cep, CE_pj_dominant);
   }

   cep = lFreeElem(cep);

   /*
    * No value available.
    */

   DEXIT;
   return 0;
}


int debit_job_from_hosts(
lListElem *job,     /* JB_Type */
lList *granted,     /* JG_Type */
lList *host_list,   /* EH_Type */
lList *centry_list, /* CE_Type */
int *sort_hostlist
) {
   lSortOrder *so = NULL;
   lListElem *gel, *hep;
   lListElem *global;
   const char *hnm;

   double old_sort_value, new_sort_value;

   DENTER(TOP_LAYER, "debit_job_from_hosts");

   so = lParseSortOrderVarArg(lGetListDescr(host_list), "%I+", EH_sort_value);

   global = host_list_locate(host_list, "global");

   /* debit from hosts */
   for_each(gel, granted) {  
      u_long32 ulc_factor;
      int slots = lGetUlong(gel, JG_slots);

      hnm = lGetHost(gel, JG_qhostname);
      hep = host_list_locate(host_list, hnm); 

      if (sconf_get_load_adjustment_decay_time() && lGetNumberOfElem(sconf_get_job_load_adjustments())) {
         /* increase host load for each scheduled job slot */
         ulc_factor = lGetUlong(hep, EH_load_correction_factor);
         ulc_factor += 100*slots;
         lSetUlong(hep, EH_load_correction_factor, ulc_factor);
      }   

      debit_host_consumable(job, host_list_locate(host_list, "global"), centry_list, slots);
      debit_host_consumable(job, hep, centry_list, slots);

      /* compute new combined load for this host and put it into the host */
      old_sort_value = lGetDouble(hep, EH_sort_value); 

      new_sort_value = scaled_mixed_load(global, hep, centry_list);

      if(new_sort_value != old_sort_value) {
         lSetDouble(hep, EH_sort_value, new_sort_value);
         if (sort_hostlist)
            *sort_hostlist = 1;
         DPRINTF(("Increasing sort value of Host %s from %f to %f\n", 
            hnm, old_sort_value, new_sort_value));
      }

      lResortElem(so, hep, host_list);
   }

   if(so) {
      lFreeSortOrder(so);
   }   

   DEXIT; 
   return 0;
}

/*
 * jep: JB_Type
 * hep: EH_Type
 * centry_list: CE_Type
 */
int 
debit_host_consumable(lListElem *jep, lListElem *hep, lList *centry_list, int slots) 
{
   return rc_debit_consumable(jep, hep, centry_list, slots, 
                           EH_consumable_config_list, 
                           EH_resource_utilization, 
                           lGetHost(hep, EH_name));
}
