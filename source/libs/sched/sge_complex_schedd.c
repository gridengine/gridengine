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

#ifdef WIN32NATIVE
#	include "win32nativetypes.h"
#endif

#include "sge_sched.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_parse_num_par.h"
#include "sge_static_load.h"
#include "sge_answer.h"
#include "sge_language.h"
#include "sge_string.h"
#include "sge_hostname.h"
#include "sge_schedd_conf.h"
#include "sge_queue.h"
#include "sge_host.h"
#include "sge_complex.h"
#include "gdi_utility.h"

#include "msg_common.h"
#include "msg_schedd.h"

#include "sge_complex_schedd.h"

static int resource_cmp(u_long32 relop, double req_all_slots, double src_dl);
static int string_base_cmp(u_long32 type, const char *s1, const char *s2);
static int string_cmp(u_long32 type, u_long32 relop, const char *request,
 const char *offer);

static int fillComplexFromQueue(lList **new_complexl, lList *complex_listl, lListElem *complexl, lListElem *queue, int recompute_debitation_dependent);

static int decide_dominance(lListElem *ep, double dval, const char *as_str, u_long32 mask);
static int append_complexes(lList **new_complex, lListElem *to_add, u_long32 layer, int recompute_debitation_dependent); 

static int fixed_and_consumable(lList *new_complex, lList *config, lList *actual, u_long32 layer, int recompute_debitation_dependent);

static int load_values(lList *new_complex, const char *hostname, 
                       lList *lv_list, u_long32 layer, 
                       int recompute_debitation_dependent, double lc_factor);

static int max_resources = QS_STATE_FULL;
static int global_load_correction = 0;

void set_qs_state(
int qs_state 
) {
   max_resources = qs_state;
}

int get_qs_state(void) {
   return max_resources;
}

void set_global_load_correction(
int flag 
) {
   global_load_correction = flag;
}
int get_global_load_correction(void) {
   return global_load_correction;
}


void monitor_dominance(
char *str,
u_long32 mask 
) {
   switch (mask & DOMINANT_LAYER_MASK) {
   case DOMINANT_LAYER_GLOBAL:   
      *str++ = 'g';
      break;
   case DOMINANT_LAYER_HOST:   
      *str++ = 'h';
      break;
   case DOMINANT_LAYER_QUEUE:   
      *str++ = 'q';
      break;
   default:
      *str++ = '?';
      break;
   }

   switch (mask & DOMINANT_TYPE_MASK) {
   case DOMINANT_TYPE_VALUE:   
      *str++ = 'v';
      break;
   case DOMINANT_TYPE_FIXED:   
      *str++ = 'f';
      break;
   case DOMINANT_TYPE_LOAD:   
      *str++ = 'l';
      break;
   case DOMINANT_TYPE_CLOAD:   
      *str++ = 'L';
      break;
   case DOMINANT_TYPE_CONSUMABLE:   
      *str++ = 'c';
      break;
   default:
      *str++ = '?';
      break;
   }

   *str++ = '\0';
}

/* override still available values in the 'new_complex' list 
   with fixed values from 'complex_list' or consumable values */
static int fixed_and_consumable(
lList *new_complex,
lList *config,
lList *actual,
u_long32 layer,
int recompute_debitation_dependent 
) {
   lListElem *conf_ep, *ep, *act_ep;
   double dval;
   char as_str[100];
   const char *name;

   DENTER(TOP_LAYER, "fixed_and_consumable");

   /* handle consumables and fixed values */
   for_each (conf_ep, config) {
      if (!(name = lGetString(conf_ep, CE_name))) {
         CRITICAL((SGE_EVENT, MSG_ATTRIB_ATTRIBUTEWITHOUTNAMEFOUND));
         continue; /* heavily bad configuration */
      }

      /* should be in our 'new_complex' list */
      if (!(ep=lGetElemStr(new_complex, CE_name, name))) {
         ERROR((SGE_EVENT, MSG_ATTRIB_XINATTRIBLISTMISSING_SU, name, u32c(layer)));
         continue; /* oops, bad configuration */
      }

      if (recompute_debitation_dependent) {
         /* touch only debitation dependent attributes */
#ifndef WIN32NATIVE
         if (!lGetBool(ep, CE_consumable) && 
			 !lGetElemStr(scheddconf.job_load_adjustments, CE_name, name)) {
#else
         if (!lGetBool(ep, CE_consumable)) {
#endif
            continue;
         }
/*          DPRINTF(("%s\tfixed_and_consumable()\n", name)); */
      }

      /* treat also consumables as fixed attributes when assuming an empty queuing system */
      if (get_qs_state()==QS_STATE_FULL && lGetBool(ep, CE_consumable)) {

         if (!(act_ep = lGetElemStr(actual, CE_name, name))) {
            ERROR((SGE_EVENT, MSG_ATTRIB_ACTUALELEMENTTOATTRIBXMISSING_S , name));
            continue;
         }

         /* consumable */ 
         switch (lGetUlong(ep, CE_relop)) {
         case CMPLXGE_OP:
         case CMPLXGT_OP:
            /* utilization */
            dval = lGetDouble(act_ep, CE_doubleval);
/*             DPRINTF(("%s amount: %f\n", name, dval)); */
            break;

         case CMPLXEQ_OP:
         case CMPLXLT_OP:
         case CMPLXLE_OP:
         case CMPLXNE_OP:
         default:
            /* availability */
            dval = lGetDouble(conf_ep, CE_doubleval) - 
               lGetDouble(act_ep, CE_doubleval);
            if (!strcmp(name, "slots")) {
/*                DPRINTF(("%s amount: %f debited: %f dval: %f\n", 
                  name, lGetDouble(conf_ep, CE_doubleval), 
                     lGetDouble(act_ep, CE_doubleval), dval)); */
            }
            break;
         }
         sprintf(as_str, "%8.3f", (float)dval);
         decide_dominance(ep, dval, as_str, layer|DOMINANT_TYPE_CONSUMABLE);
      } else {
         /* fixed value */

         u_long32 type = lGetBool(ep, CE_consumable)?
               DOMINANT_TYPE_CONSUMABLE:
               DOMINANT_TYPE_FIXED;

         switch (lGetUlong(ep, CE_valtype)) {
         case TYPE_INT:
         case TYPE_TIM:
         case TYPE_MEM:
         case TYPE_BOO:
         case TYPE_DOUBLE:
            decide_dominance(ep, lGetDouble(conf_ep, CE_doubleval), 
                  lGetString(conf_ep, CE_stringval), layer|type); 
            break;
         default:
            lSetString(ep, CE_stringval, lGetString(conf_ep, CE_stringval));
            lSetUlong(ep, CE_dominant, layer|type);
            break;
         }
      }
   }

   DEXIT;
   return 0;
}

static int decide_dominance(lListElem *ep, double dval, const char *as_str,
                            u_long32 mask) {
   u_long32 op;
   int initial;
   int nm_dominant, nm_doubleval, nm_stringval;

   DENTER(CULL_LAYER, "decide_dominance");

   if (!as_str) {
      CRITICAL((SGE_EVENT, MSG_POINTER_ASSTRISNULLFORATTRIBX_S, 
         lGetString(ep, CE_name)));
      DEXIT;
      return 1;
   }
  
   switch ( mask&DOMINANT_TYPE_MASK ) {
   /* resources available for the whole job */
   case DOMINANT_TYPE_CONSUMABLE: 
   case DOMINANT_TYPE_LOAD:
   case DOMINANT_TYPE_CLOAD:
      nm_dominant = CE_pj_dominant; 
      nm_doubleval = CE_pj_doubleval; 
      nm_stringval = CE_pj_stringval; 
      break;

   /* resources available for the each slot of a job */
   case DOMINANT_TYPE_FIXED:
   case DOMINANT_TYPE_VALUE:
   default:
      nm_dominant = CE_dominant; 
      nm_doubleval = CE_doubleval; 
      nm_stringval = CE_stringval; 
      break;
   }

   /* the value from the complex template gets overwritten in all cases 
      if there was already a value set by a previous facility 
      we take minimum/maximum - depends on relop in complex */ 

   if ((initial=((lGetUlong(ep, nm_dominant) & DOMINANT_TYPE_MASK)==
               DOMINANT_TYPE_VALUE)) ||
         ((op=lGetUlong(ep, CE_relop), op == CMPLXGE_OP || op == CMPLXGT_OP)? 
            /* max */ (dval >= lGetDouble(ep, nm_doubleval)):
            /* min */ (dval <= lGetDouble(ep, nm_doubleval)))) {

      if (!initial) {
         char new_dom[4], old_dom[4];
         const char *name = lGetString(ep, CE_name);

         monitor_dominance(old_dom, lGetUlong(ep, nm_dominant));
         monitor_dominance(new_dom, mask);
         DPRINTF(("%s:%s = %s <- %s:%s = %s\n", 
            old_dom, name, lGetString(ep, CE_stringval),
            new_dom, name, as_str));
      }

      lSetUlong(ep, nm_dominant, mask);
      lSetDouble(ep, nm_doubleval, dval);
      lSetString(ep, nm_stringval, as_str);
  
      DEXIT;
      return 0;
   }

   DEXIT;
   return 1;
}

static int append_complexes(
lList **new_complex,
lListElem *to_add,
u_long32 layer, 
int recompute_debitation_dependent  
) {
   lListElem *newep, *ep, *already_here;
   lList *lp_add;
   const lDescr *lp_add_descr;
   const char *name;
   int pos_CE_name, pos_CE_dominant, pos_CE_pj_dominant;
   int prev;

   DENTER(TOP_LAYER, "append_complexes");

   prev = lGetNumberOfElem(*new_complex);

   lp_add = lGetList(to_add, CX_entries);
   lp_add_descr = lGetListDescr(lp_add);
   pos_CE_name        = lGetPosInDescr(lp_add_descr, CE_name);
   pos_CE_dominant    = lGetPosInDescr(lp_add_descr, CE_dominant);
   pos_CE_pj_dominant = lGetPosInDescr(lp_add_descr, CE_pj_dominant);

   /* Iterate on complex templates elements */
   for_each(ep, lp_add) {

      name = lGetPosString(ep, pos_CE_name);
      already_here = lGetElemStr(*new_complex, CE_name, name);

      /* Just skip multiple occurances of attributes.
         It's expensive but the qmaster had to prevent 
         multiple occurances of complexes in queue and 
         host and global */
      if (!recompute_debitation_dependent) {
         if (already_here)
            continue;
      } else {
         /* only reinit debitation dependent attributes */
#ifndef WIN32NATIVE
         if (!lGetElemStr(scheddconf.job_load_adjustments, CE_name, name) &&
                  !lGetBool(ep, CE_consumable))
#else
         if (!lGetBool(ep, CE_consumable))
#endif
            continue; 
      
         if (!already_here) {
            ERROR((SGE_EVENT, MSG_ATTRIB_ATTRIBUTEXALLREADYINLIST_S , name));
            continue;
         }
         lRemoveElem(*new_complex, already_here);
      }

      if (!(newep = lCopyElem(ep))) {
         ERROR((SGE_EVENT,MSG_MEMORY_UNABLETOALLOCSPACEFORCOMPLEXBUILD ));
         *new_complex = lFreeList(*new_complex);
         DEXIT;
         return 1;
      }

      lSetPosUlong(newep, pos_CE_dominant, layer|DOMINANT_TYPE_VALUE);
      lSetPosUlong(newep, pos_CE_pj_dominant, layer|DOMINANT_TYPE_VALUE);
      lSetString(newep, CE_pj_stringval, lGetString(ep, CE_stringval));
      lSetDouble(newep, CE_pj_doubleval, lGetDouble(ep, CE_doubleval));

      if (!*new_complex)
         *new_complex = lCreateList("attr", CE_Type);
      lAppendElem(*new_complex, newep);
   }

#if 0
   DPRINTF(("complex \"%s\" added %d attributes\n",
         lGetString(to_add, CX_name), 
         lGetNumberOfElem(*new_complex)-prev));
#endif

   DEXIT;
   return 0;
}

/* Mapping list for generating a complex out of a queue */
struct queue2cmplx {
   char *attrname;
   char *shortcut;
   int  field;
   int type;
   int relop;
};

/* provide a list of attributes containing all global attributes */
int global_complexes2scheduler(
lList **new_complex_list,
lListElem *global_host,
lList *complex_list,
int recompute_debitation_dependent 
) {
   lListElem *complex;

   DENTER(TOP_LAYER, "global_complexes2scheduler");

   /* build global complex and add it to result */
   complex = lGetElemStr(complex_list, CX_name, "global");
   fillComplexFromHost(new_complex_list, complex_list, global_host, complex, DOMINANT_LAYER_GLOBAL, recompute_debitation_dependent);

   DEXIT;
   return 0;
}



/* provide a list of attributes containing all attributes for the given host */
int host_complexes2scheduler(
lList **new_complex_list,
lListElem *host,
lList *exechost_list,
lList *complex_list, 
int recompute_debitation_dependent 
) {
   lListElem *complex;

   DENTER(TOP_LAYER, "host_comlexes2scheduler");

   if (!host) {
      DPRINTF(("!!missing host!!\n"));
   }

   /* build global complex and add it to result */
   if (recompute_debitation_dependent || !*new_complex_list) {
      global_complexes2scheduler(new_complex_list, 
                                 host_list_locate(exechost_list, "global"), 
                                 complex_list,
                                 recompute_debitation_dependent);
   }

   /* build host complex and add it to result */
   complex = lGetElemStr(complex_list, CX_name, "host");
   fillComplexFromHost(new_complex_list, complex_list, host, complex, 
         DOMINANT_LAYER_HOST, recompute_debitation_dependent);

   DEXIT;
   return 0;
}

/**********************************************************************
 This is the function used by the scheduler to get a complete list of 
 complexes for a given queue.
 All templates are filled by actual values.
 With this list the scheduler can decide whether a request matches this
 queue.

 **********************************************************************/
int queue_complexes2scheduler(
lList **new_complex_list,
lListElem *queue,
lList *exechost_list,
lList *complex_list, 
int recompute_debitation_dependent 
) {
   lListElem *complex;

   DENTER(TOP_LAYER, "queue_complexes2scheduler");

   if (recompute_debitation_dependent || !*new_complex_list) {
      host_complexes2scheduler(
         new_complex_list, 
         queue ?
            host_list_locate(exechost_list, lGetHost(queue, QU_qhostname))
            :NULL, 
         exechost_list, 
         complex_list, 
         recompute_debitation_dependent);
      
   }

   /* build queue complex and add it to result */
   complex = lGetElemStr(complex_list, CX_name, "queue");
   fillComplexFromQueue(new_complex_list, complex_list, complex, queue, recompute_debitation_dependent);

   DEXIT;
   return 0;
}

/**********************************************************************

  Build the hosts complex for the scheduler. Input is the host complex
  template. Which allows the administrator to enable and disable the
  requestability of a complex and allows to disable the complex by 
  removing it from the complex list. 

  The host object is used to fill the template with actual load sensor
  values. 

  Returns: new_complex

new_complex;                     here we collect resulting attributes  
complex_list;                    the global complex list               
host;                            the host or the global host           
complex;                         he template                          
layer;                           which layer was dominant?             
recompute_debitation_dependent;  recompute only attribute types which  
                                 depend on the amount of debited jobs  
                                 these types are:                      
                                 - load corrected load values          
                                   (-> scheddconf.job_load_adjustments)
                                 - consumable attributes              
                                   (-> CE_consumable)                 
 **********************************************************************/
int fillComplexFromHost(lList **new_complex, lList *complex_list, 
                        lListElem *host, lListElem *complex, u_long32 layer, 
                        int recompute_debitation_dependent)
{
   lListElem *cep;
   double lc_factor = 0; /* scaling for load correction */ 
   u_long32 ulc_factor;

   DENTER(TOP_LAYER, "fillComplexFromHost");

   /* append main complex "host"/"global" ... */
   if (complex)
      append_complexes(new_complex, complex, layer, recompute_debitation_dependent);

   if (!host) { /* there may be a queue which has no host object yet */
      DEXIT;
      return 0;
   }   
   
   /* ... and all userdefined complexes */
   for_each (cep, lGetList(host, EH_complex_list)) {
      if (!(complex = lGetElemStr(complex_list, CX_name, lGetString(cep, CX_name)))) {
         ERROR((SGE_EVENT, MSG_LIST_NOCOMPLEXXATTACHEDTOHOSTY_SS , 
               lGetString(cep, CX_name), lGetHost(host, EH_name)));
         DEXIT;
         return -1;
      }
      append_complexes(new_complex, complex, layer, recompute_debitation_dependent);
   }
   
   /* is there a multiplier for load correction (may be not in qstat, qmon etc) */
   if (lGetPosViaElem(host, EH_load_correction_factor) >= 0) {
      if ((ulc_factor=lGetUlong(host, EH_load_correction_factor)))
         lc_factor = ((double)ulc_factor)/100;
   }
   
   /* handle load values */
   load_values(
      *new_complex,
      lGetHost(host, EH_name),
      lGetList(host, EH_load_list),
      layer,
      recompute_debitation_dependent,
      lc_factor);

   fixed_and_consumable(
      *new_complex, 
      lGetList(host, EH_consumable_config_list),
      lGetList(host, EH_consumable_actual_list),
      layer, 
      recompute_debitation_dependent);

   DEXIT;
   return 0;
}

static int load_values(
lList *new_complex,
const char *hostname,
lList *lv_list,
u_long32 layer,
int recompute_debitation_dependent,
double lc_factor 
) {
   lListElem *ep, *load_sensor;
   const char *name, *load_value;
   lListElem *job_load;
   u_long32 type, dom_type;
   double dval;
   char err_str[256], sval[100];

   DENTER(TOP_LAYER, "load_values");

   for_each (load_sensor, lv_list) {
      name = lGetString(load_sensor, HL_name);

      /* is this attribute contained in the complexes? */
      if (!(ep = lGetElemStr(new_complex, CE_name, name)))
         continue; /* no */

      /* get load correction for this load value ? */
#ifndef WIN32NATIVE
      job_load=lGetElemStr(scheddconf.job_load_adjustments, CE_name, name);
#else
     job_load=NULL;
#endif
      if (recompute_debitation_dependent) {
         /* touch only debitation dependent attributes */
         if (!job_load && !lGetBool(ep, CE_consumable)) {
            continue;
         }
/*          DPRINTF(("%s\tfillComplexFromHost()\n", name)); */
      }

      /* load values are accepted only in case they are static */
      if (get_qs_state()==QS_STATE_EMPTY && !sge_is_static_load_value(name))
         continue;

      load_value = lGetString(load_sensor, HL_value);
      dom_type = DOMINANT_TYPE_LOAD;

      switch (type = lGetUlong(ep, CE_valtype)) {
         case TYPE_INT:
         case TYPE_TIM:
         case TYPE_MEM:
         case TYPE_BOO:
         case TYPE_DOUBLE:
            if (parse_ulong_val(&dval, NULL, type, load_value, NULL, 0)) {

               strcpy(sval, load_value);
               /* --------------------------------
                  look for 'name' in our load_adjustments list
               */
               if (job_load) {
                  const char *s;
                  double load_correction;

                  s = lGetString(job_load, CE_stringval);
                  if (!parse_ulong_val(&load_correction, NULL, type, s,
                     err_str, 255)) {
                     ERROR((SGE_EVENT, MSG_SCHEDD_LOADADJUSTMENTSVALUEXNOTNUMERIC_S , name));
                  }
                  else {
                     if (lc_factor) {
                        double old_dval;
                        int nproc;
                        lListElem *ep_nproc;
                        const char *cp;

                        if (!strncmp(name, "np_", 3)) {
                           nproc = 1;
                           if ((ep_nproc = lGetElemStr(lv_list, HL_name, LOAD_ATTR_NUM_PROC))) {
                              cp = lGetString(ep_nproc, HL_value);
                              if (cp)
                                 nproc = MAX(1, atoi(lGetString(ep_nproc, HL_value)));
                           }

                           if (nproc != 1) {
                              DPRINTF(("fillComplexFromHost: dividing lc_factor for \"%s\" with value %f by %d to %f\n",
                                       name, lc_factor, nproc, lc_factor / nproc));
                              lc_factor /= nproc;
                           }
                        }

                        load_correction *= lc_factor;

                        /* it depends on relop in complex config
                           whether load_correction is pos/neg */
                        switch (lGetUlong(ep, CE_relop)) {
                        case CMPLXGE_OP:
                        case CMPLXGT_OP:
                           old_dval = dval;
                           dval += load_correction;
                           break;

                        case CMPLXNE_OP:
                        case CMPLXEQ_OP:
                        case CMPLXLT_OP:
                        case CMPLXLE_OP:
                        default:
                           old_dval = dval;
                           dval -= load_correction;
                           break;
                        }

                        sprintf(sval, "%8.3f", dval);
                        DPRINTF(("%s@%s: uc: %f c(%f): %f\n",
                           name, hostname, old_dval,
                           lc_factor, dval));
                     }
                     dom_type = DOMINANT_TYPE_CLOAD;
                  }
               }

               decide_dominance(ep, dval, sval, layer|dom_type);
            } /* in case of errors we let the complexes unchanged */
            break;

         case TYPE_STR:
         case TYPE_CSTR:
         case TYPE_HOST:
            lSetString(ep, CE_stringval, load_value);
            lSetUlong(ep, CE_dominant, layer|DOMINANT_TYPE_LOAD);
            break;
      }
   }

   DEXIT;
   return 0;
}

/**********************************************************************
 make a complex out of the default queue complex and a queue.
 **********************************************************************/
static int fillComplexFromQueue(new_complex, complex_list, complex, queue, recompute_debitation_dependent)
lList** new_complex;                /* here we collect resulting attributes  */
lList* complex_list;                /* the global complex list               */
lListElem *complex;                 /* the "queue" complex */
lListElem *queue;                   /* the queue itself */
int recompute_debitation_dependent; /* recompute only attribute types which  */
                                    /* depend on the amount of debited jobs  */
                                    /* these types are:                      */
                                    /* - load corrected load values          */
                                    /*   (-> scheddconf.job_load_adjustments)*/
                                    /* - consumable attributes               */
                                    /*   (-> CE_consumable)                  */
{
   lListElem *cep, *complexel;
   const char *value;
   char as_str[100];
   struct queue2cmplx *q2cptr;

   /* *INDENT-OFF* */
   static struct queue2cmplx q2c[] = {
      {"qname",            "q",   QU_qname,            TYPE_STR, CMPLXEQ_OP},
      {"hostname",         "h",   QU_qhostname,        TYPE_HOST,CMPLXEQ_OP},
      {"slots",            "s",   QU_job_slots,        TYPE_INT, CMPLXLT_OP},
      {"tmpdir",           "tmp", QU_tmpdir,           TYPE_STR, CMPLXEQ_OP},
      {"seq_no",           "seq", QU_seq_no,           TYPE_INT, CMPLXEQ_OP},
      {"rerun",            "re",  QU_rerun,            TYPE_BOO, CMPLXEQ_OP},
      {"calendar",         "cal", QU_calendar,         TYPE_STR, CMPLXEQ_OP},
      {"s_rt",             "srt", QU_s_rt,             TYPE_TIM, CMPLXLT_OP},
      {"h_rt",             "hrt", QU_h_rt,             TYPE_TIM, CMPLXLT_OP},
      {"s_cpu",            "sc",  QU_s_cpu,            TYPE_TIM, CMPLXLT_OP},
      {"h_cpu",            "hc",  QU_h_cpu,            TYPE_TIM, CMPLXLT_OP},
      {"s_fsize",          "sf",  QU_s_fsize,          TYPE_MEM, CMPLXLT_OP},
      {"h_fsize",          "hf",  QU_h_fsize,          TYPE_MEM, CMPLXLT_OP},
      {"s_data",           "sd",  QU_s_data,           TYPE_MEM, CMPLXLT_OP},
      {"h_data",           "hd",  QU_h_data,           TYPE_MEM, CMPLXLT_OP},
      {"s_stack",          "ss",  QU_s_stack,          TYPE_MEM, CMPLXLT_OP},
      {"h_stack",          "hs",  QU_h_stack,          TYPE_MEM, CMPLXLT_OP},
      {"s_core",           "sc",  QU_s_core,           TYPE_MEM, CMPLXLT_OP},
      {"h_core",           "hc",  QU_h_core,           TYPE_MEM, CMPLXLT_OP},
      {"s_rss",            "sr",  QU_s_rss,            TYPE_MEM, CMPLXLT_OP},
      {"h_rss",            "hr",  QU_h_rss,            TYPE_MEM, CMPLXLT_OP},
      {"s_vmem",           "sv",  QU_s_vmem,           TYPE_MEM, CMPLXLT_OP},
      {"h_vmem",           "hv",  QU_h_vmem,           TYPE_MEM, CMPLXLT_OP},
      {"min_cpu_interval", "mci", QU_min_cpu_interval, TYPE_TIM, CMPLXGT_OP},
      {"", "", 0, 0, 0}                               /* delimiter */
   };
   /* *INDENT-ON* */

   DENTER(TOP_LAYER, "fillComplexFromQueue");

   /* append main "queue" complex ... */
   if (complex)
      append_complexes(new_complex, complex, DOMINANT_LAYER_QUEUE, recompute_debitation_dependent);

   if (!queue) {
      DEXIT;
      return 0;
   }   

   /* ... and all userdefined complexes */
   for_each (cep, lGetList(queue, QU_complex_list)) {
      if (!(complex = lGetElemStr(complex_list, CX_name, lGetString(cep, CX_name)))) {
         ERROR((SGE_EVENT, MSG_LIST_NOCOMPLEXXATTACHEDTOQUEUEY_SS , 
               lGetString(cep, CX_name), lGetString(queue, QU_qname)));
         DEXIT;
         return -1;
      }
      append_complexes(new_complex, complex, DOMINANT_LAYER_QUEUE, recompute_debitation_dependent);
   }

   /* iterate through "queue" complex and fill in attribs from queue */
   for (q2cptr = q2c; q2cptr->attrname[0]; q2cptr++) {
      double dval;
      if (!(complexel = lGetElemStr(*new_complex, CE_name, q2cptr->attrname)))
         continue;

      if (recompute_debitation_dependent) {
         /* touch only debitation dependent attributes */ 
#ifndef WIN32NATIVE
         if (!lGetElemStr(scheddconf.job_load_adjustments, CE_name, q2cptr->attrname) &&
                  !lGetBool(complexel, CE_consumable)) {
#else
         if (!lGetBool(complexel, CE_consumable)) {
#endif
            continue;
         }
/*          DPRINTF(("%s\tfillComplexFromQueue()\n", q2cptr->attrname)); */
      }

      /* read stuff from queue and set to new elements */
      switch (q2cptr->type) {
      case TYPE_INT:
         /* read from queue and write into complex */
         dval = (double)lGetUlong(queue, q2cptr->field);
         sprintf(as_str, u32, lGetUlong(queue, q2cptr->field));
         decide_dominance(complexel, dval, as_str, DOMINANT_LAYER_QUEUE|DOMINANT_TYPE_FIXED);
         break;

      case TYPE_TIM:
      case TYPE_MEM:
      case TYPE_DOUBLE:
         /* read from queue and write into complex */
         if ((value = lGetString(queue, q2cptr->field))) {
            parse_ulong_val(&dval, NULL, q2cptr->type, value, NULL, 0); 
            decide_dominance(complexel, dval, value, DOMINANT_LAYER_QUEUE|DOMINANT_TYPE_FIXED);
         } 
         break;

      case TYPE_BOO:
         /* read from queue and write into complex */
         dval = (double)lGetBool(queue, q2cptr->field);
         sprintf(as_str, "%d", (int)lGetBool(queue, q2cptr->field));
         decide_dominance(complexel, dval, as_str, DOMINANT_LAYER_QUEUE|DOMINANT_TYPE_FIXED);
         break;

      case TYPE_STR:
      case TYPE_CSTR:
         /* read a value from queue */
         if ((value = lGetString(queue, q2cptr->field))) {
            lSetString(complexel, CE_stringval, value);
            lSetUlong(complexel, CE_dominant, DOMINANT_LAYER_QUEUE|DOMINANT_TYPE_FIXED);
         }
         break;
      case TYPE_HOST:
         /* read a value from queue */
         if ((value = lGetHost(queue, q2cptr->field))) {
            lSetString(complexel, CE_stringval, value);
            lSetUlong(complexel, CE_dominant, DOMINANT_LAYER_QUEUE|DOMINANT_TYPE_FIXED);
         }
         break;
      }
   }

   fixed_and_consumable(
      *new_complex, 
      lGetList(queue, QU_consumable_config_list),
      lGetList(queue, QU_consumable_actual_list),
      DOMINANT_LAYER_QUEUE,
      recompute_debitation_dependent);

   DEXIT;
   return 0;
}

/* wrapper for strcmp() of all string types */ 
static int string_base_cmp(u_long32 type, const char *s1, const char *s2)
{
   int match;

   if (type==TYPE_STR)
      match = strcmp(s1, s2);
   else  {
      if (type==TYPE_CSTR)
         match = strcasecmp(s1, s2);
      else
         match = sge_hostcmp(s1, s2);
   }

   return match;
}

/* compare string type attributes under consideration of relop */
static int string_cmp( u_long32 type, u_long32 relop, const char *request,
const char *offer) {
   int match;

   switch(relop) {
   case CMPLXEQ_OP:
      match = (string_base_cmp(type, request, offer) == 0);
      break;
   case CMPLXLE_OP :
      match = (string_base_cmp(type, request, offer) <= 0);
      break;
   case CMPLXLT_OP :
      match = (string_base_cmp(type, request, offer) < 0);
      break;
   case CMPLXGT_OP :
      match = (string_base_cmp(type, request, offer) > 0);
      break;
   case CMPLXGE_OP :
      match = (string_base_cmp(type, request, offer) >= 0);
      break;
   case CMPLXNE_OP :
      match = (string_base_cmp(type, request, offer) != 0);
      break;
   default:
      match = 0; /* default -> no match */
   }

   return match;
}

static int resource_cmp(
u_long32 relop,
double req,
double src_dl 
) {
   int match;

   switch(relop) { 
   case CMPLXEQ_OP :
      match = ( req==src_dl);
      break;
   case CMPLXLE_OP :
      match = ( req<=src_dl);
      break;
   case CMPLXLT_OP :
      match = ( req<src_dl);
      break;
   case CMPLXGT_OP :
      match = ( req>src_dl);
      break;
   case CMPLXGE_OP :
      match = ( req>=src_dl);
      break;
   case CMPLXNE_OP :
      match = ( req!=src_dl);
      break;
   default:
      match = 0; /* default -> no match */
   }

   return match;      
}

/*********************************************************************
 compare two complex entries (attributes)
 the type is given by the first complex
 return 1 if matched anything else if not
 *********************************************************************/
int compare_complexes(
int slots,
lListElem *util_max_ep, /* maximum of utilization - needed when computing slots for utilization attributes */
lListElem *req_cplx,
lListElem *src_cplx,
char *availability_text,
int is_threshold,
int force_existence 
) {
   u_long32 type, relop, used_relop = 0;
   double req_dl, src_dl;
   int match, m1, m2;
   const char *s;
   const char *name;
   const char *offer;
   char dom_str[5], resource_text[100];  /* , r1str[100], r2str[100] */
   char availability_text1[2048];
   char availability_text2[2048]; 

   DENTER(TOP_LAYER,"compare_complexes");

   name = lGetString(src_cplx, CE_name); 
   type = lGetUlong(src_cplx, CE_valtype);
   relop = lGetUlong(src_cplx, CE_relop);

   if (is_threshold) {
      switch (relop) {
      case CMPLXLE_OP:
         used_relop = CMPLXGT_OP;
         break;
      case CMPLXGT_OP:
         used_relop = CMPLXLE_OP;
         break;
      case CMPLXLT_OP:
         used_relop = CMPLXGE_OP;
         break;
      case CMPLXGE_OP:
         used_relop = CMPLXLT_OP;
         break;
      case CMPLXNE_OP:
      case CMPLXEQ_OP:
      default:
         used_relop = relop;
         break;
      }
   } else {
      used_relop = relop ;
   }

   switch (type) {
      const char *request;
      double req_all_slots;

   case TYPE_STR:
   case TYPE_CSTR:
   case TYPE_HOST:

      request = lGetString(req_cplx, CE_stringval);
      offer = lGetString(src_cplx, CE_stringval);
      monitor_dominance(dom_str, lGetUlong(src_cplx, CE_dominant));
#if 0
      DPRINTF(("%s(\"%s\", \"%s\")\n", type==TYPE_STR?"strcmp":"strcasecmp",
            request, offer)); 
#endif

      match = string_cmp(type, used_relop, request, offer);

      sprintf(availability_text, "%s:%s=%s", dom_str, name, offer);
#if 0
      DPRINTF(("-l %s=%s, Q: %s:%s%s%s, Comparison: %s\n",
            name, request, dom_str, name, map_op2str(relop),
            offer, match?"ok":"no match"));
#endif
      DEXIT;
      return match;

   case TYPE_INT:
   case TYPE_TIM:
   case TYPE_MEM:
   case TYPE_BOO:
   case TYPE_DOUBLE:
      s=lGetString(req_cplx, CE_stringval); 
      if (!parse_ulong_val(&req_dl, NULL, type, s, NULL, 0)) {
         DPRINTF(("%s is not of type %s\n", s, map_type2str(type)));
         req_dl = 0;
      }   

      if (is_threshold)
         m1 = m2 = 0; /* nothing exceeded per default */
      else
         m1 = m2 = 1; /* matched per default */

      /* is there a per job limit */
      if (!(lGetUlong(src_cplx, CE_pj_dominant) & (DOMINANT_TYPE_VALUE))) {
         /* Actually request matching for utilization attributes 
            fails if request is higher than what we get as per job 
            limit also for attributes marked as consumable.
        
            In future we could say that the actual value (being a load 
            value or a consumable resource value) plus the jobs request 
            may not exceed the maximal utilization from (global) host 
            or queue - but only if the attributes marked as consumable. */

         src_dl = lGetDouble(src_cplx, CE_pj_doubleval);
         
         req_all_slots = req_dl*slots;

         m1 = resource_cmp(used_relop, req_all_slots, src_dl);

         monitor_dominance(dom_str, lGetUlong(src_cplx, CE_pj_dominant));

         if (type==TYPE_BOO)
            sprintf(availability_text1, "%s:%s=%s", dom_str, name, src_dl?"true":"false");
         else  
            sprintf(availability_text1, "%s:%s=%s", dom_str, name, resource_descr(src_dl, type, resource_text));

#if 0
         if (type==TYPE_BOO) {
            DPRINTF(("-l %s=%f, Q: %s:%s%s%f, Comparison: %s\n",
                  name, req_dl?"true":"false", dom_str, name, map_op2str(used_relop),
                  src_dl?"true":"false", m1?"ok":"no match"));
         } else {
            DPRINTF(("%d times of -l %s=%s, Q: %s:%s%s%s, Comparison: %s\n",
                  slots, name, resource_descr(req_dl, type, r1str), dom_str, name, map_op2str(used_relop),
                  resource_descr(src_dl, type, r2str), m1?"ok":"no match"));
         }
#endif
      }

      /* is there a per slot limit */
      if (!(lGetUlong(src_cplx, CE_dominant) & (DOMINANT_TYPE_VALUE)) ||     /* per slot set || */
              ((lGetUlong(src_cplx, CE_dominant) & (DOMINANT_TYPE_VALUE)) && /* (per slot not set && */
           (lGetUlong(src_cplx, CE_pj_dominant) & (DOMINANT_TYPE_VALUE)) &&  /* and per job not set) */
            force_existence)) {  

         src_dl = lGetDouble(src_cplx, CE_doubleval);
         req_all_slots = req_dl;
         m2 = resource_cmp(used_relop, req_all_slots, src_dl);
         monitor_dominance(dom_str, lGetUlong(src_cplx, CE_dominant));

         if (type==TYPE_BOO)
            sprintf(availability_text2, "%s:%s=%s", dom_str, name, src_dl?"true":"false");
         else
            sprintf(availability_text2, "%s:%s=%s", dom_str, name, resource_descr(src_dl, type, resource_text));

#if 0
         if (type==TYPE_BOO) {
            DPRINTF(("-l %s=%f, Q: %s:%s%s%f, Comparison: %s\n",
                  name, req_dl?"true":"false", dom_str, name, map_op2str(used_relop),
                  src_dl?"true":"false", m2?"ok":"no match"));
         } else {
            DPRINTF(("per slot -l %s=%s, Q: %s:%s%s%s, Comparison: %s\n",
                  name, resource_descr(req_dl, type, r1str), dom_str, name, map_op2str(used_relop),
                  resource_descr(src_dl, type, r2str), m2?"ok":"no match"));
         }
#endif
      }

      if (is_threshold)
         match = m1 || m2;
      else {
         match = m1 && m2;
         if (!m1) {
            strcpy(availability_text, availability_text1);
         } else if (!m2) {
            strcpy(availability_text, availability_text2);
         } else {
            strcpy(availability_text, "");
         }
      }
      DEXIT;
      return match;

   default:  /* should never reach this -> undefined type */
      *availability_text = '\0';
      break;
   }
   DEXIT;
   return 0;
}

#ifdef TEST
/* 
for testing purposes compile (on linux) with:
gcc -Wall -DLINUX -DTEST -o complex complex.c ../LINUX/sge_parse_num_par.o ../LINUX/log.o ../LINUX/utility.o  ../LINUX/pack.o ../LINUX/free.o ../LINUX/io.o ../LINUX/libcull.a ../LINUX/librmon.a
*/

int main(int argc, char *argv[], char *envp[])
{
   lListElem *l;
  
#ifdef __SGE_COMPILE_WITH_GETTEXT__   
   /* init language output for gettext() , it will use the right language */
   sge_init_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);  
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */
 

   if (argc!=3) {
      fprintf(stdout, "usage: complex read_file write_file\n");
      exit(1);
   }

   l = read_cmplx(argv[1], "cmplx_name", NULL);

   write_cmplx(1, argv[2], l, NULL, NULL);

   return 0;
}
#endif

/* Updates all consumable actual values of queue/host
   for 'slots' slots of the given job. Since it is also 
   allowed to pass negative slot amounts for purposes of undebiting
*/
int debit_consumable(lListElem *jep, lListElem *ep, lList *complex_list,
                     int slots, int config_nm, int actual_nm, 
                     const char *obj_name) 
{
   lListElem *cr, *cr_config, *dcep;
   double dval;
   const char *name;
   int mods = 0;

   DENTER(TOP_LAYER, "debit_consumable");

   if (!ep) {
      DEXIT;
      return 0;
   }

   for_each (cr_config, lGetList(ep, config_nm)) {
      name = lGetString(cr_config, CE_name);
      dval = 0;

      /* search default request */  
      if (!(dcep = complex_list_locate_attr(complex_list, name))) {
         ERROR((SGE_EVENT, MSG_ATTRIB_MISSINGATTRIBUTEXINCOMPLEXES_S , name));
         DEXIT; 
         return -1;
      } 

      if (!lGetBool(dcep, CE_consumable)) {
         /* no error */
         continue;
      }

      /* ensure attribute is in actual list */
      if (!(cr = lGetSubStr(ep, CE_name, name, actual_nm))) {
         cr = lAddSubStr(ep, CE_name, name, actual_nm, CE_Type);
         /* CE_double is implicitly set to zero */
      }
   
      if (jep) {
         if (!get_job_contribution(&dval, name, jep, dcep)) {
            DPRINTF(("debiting %f of %s on %s %s for %d slots\n",
                  dval, name, (config_nm==QU_consumable_config_list)?"queue":"host",
                     obj_name, slots));
            lSetDouble(cr, CE_doubleval, lGetDouble(cr, CE_doubleval) + slots * dval);
            mods++;
         }
      }
   }

   DEXIT;
   return mods;
}

/* ---------------------------------------- 

   used to map between operator 
   and string form of it 

*/
const char *map_op2str(
u_long32 op 
) {
   static char *opv[] = {
      "??",
      "==", /* CMPLXEQ_OP */
      ">=", /* CMPLXGE_OP */
      ">",  /* CMPLXGT_OP */
      "<",  /* CMPLXLT_OP */
      "<=", /* CMPLXLE_OP */
      "!="  /* CMPLXNE_OP */
   };

   if (op<CMPLXEQ_OP || op>CMPLXNE_OP)
      op = 0;
   return opv[op];
}

/* ---------------------------------------- 

   used to map from type (numeric) to type (string)

*/
const char *map_type2str(
u_long32 type 
) {
   static char *typev[] = {
      "??????",
      "INT",     /* TYPE_INT */
      "STRING",  /* TYPE_STR */
      "TIME",    /* TYPE_TIM */
      "MEMORY",  /* TYPE_MEM */
      "BOOL",    /* TYPE_BOO */
      "CSTRING", /* TYPE_CSTR */
      "HOST",    /* TYPE_HOST */
      "DOUBLE",  /* TYPE_DOUBLE */

      "TYPE_ACC",/* TYPE_ACC */
      "TYPE_LOG",/* TYPE_LOG */
      "TYPE_LOF" /* TYPE_LOF */
   };

   if (type<TYPE_FIRST || type>TYPE_LAST)
      type = 0;
   return typev[type];
}

int ensure_attrib_available(
lList **alpp,
lListElem *ep,
int nm 
) {
   const char *name, *obj_name = "none", *obj_descr = "none";
   lListElem *attr;
   lList *resources = NULL; 

   DENTER(TOP_LAYER, "ensure_attrib_available");

   for_each (attr, lGetList(ep, nm)) {
      if (!resources) { /* first time build resources list */
         if ( nm==EH_consumable_config_list ) {
            if (!strcmp(lGetHost(ep, EH_name), "global"))
               global_complexes2scheduler(&resources, ep, Master_Complex_List, 0);
            else 
               host_complexes2scheduler(&resources, ep, Master_Exechost_List, Master_Complex_List, 0);
            obj_name = lGetHost(ep, EH_name);
            obj_descr = "host";
         } else {
            queue_complexes2scheduler(&resources, ep, Master_Exechost_List, Master_Complex_List, 0);
            obj_name = lGetString(ep, QU_qname);
            obj_descr = "queue";
         }
      }

      name = lGetString(attr, CE_name);
      if (!lGetElemStr(resources, CE_name, name)) {
         resources = lFreeList(resources);
         ERROR((SGE_EVENT, MSG_GDI_NO_ATTRIBUTE_SSS, name, obj_descr, obj_name));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }
   resources = lFreeList(resources);

   DEXIT;
   return 0;
}

int attr_mod_threshold(
lList **alpp,
lListElem *qep,
lListElem *new_ep,
int nm,
int primary_key,
int sub_command,
char *attr_name,
char *object_name 
) {
   int ret;

   DENTER(TOP_LAYER, "attr_mod_threshold");

   /* ---- attribute nm */
   if (lGetPosViaElem(qep, nm)>=0) {
      lListElem *tmp_elem;

      DPRINTF(("got new %s\n", attr_name));

      tmp_elem = lCopyElem(new_ep); 
      attr_mod_sub_list(alpp, tmp_elem, nm, primary_key, qep,
         sub_command, attr_name, object_name, 0); 

      ret=sge_fill_requests(lGetList(tmp_elem, nm), Master_Complex_List, 1, 0, 0);
      if (ret) {
         /* error message gets written by sge_fill_requests into SGE_EVENT */
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         lFreeElem(tmp_elem);
         DEXIT;
         return STATUS_EUNKNOWN;
      }

      lSetList(new_ep, nm, lCopyList("", lGetList(tmp_elem, nm)));
      lFreeElem(tmp_elem);

      /* check whether this attrib is available due to complex configuration */
      if (ensure_attrib_available(alpp, new_ep, nm)) {
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }

   DEXIT;
   return 0;
}

