#ifndef __SCHEDD_CONF_H
#define __SCHEDD_CONF_H
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

struct schedd_confel {                  
    char       *algorithm;              /* "default" or others refering to schedd' table of 
                                           scheduling algorithm */
    u_long32    sgeee_schedule_interval;      /* how often to attemt to schedule */
    u_long32    schedule_interval;      /* how often to attemt to schedule */
    u_long32    maxujobs;               /* limit on max running jobs per user */
    u_long32    queue_sort_method;      /* use seq_no to sort else use load */
    lList*      job_load_adjustments;   /* load we use per job slot to correct host load */
    u_long32    load_adjustment_decay_time; /* after this time we assume our load sensors */
                                        /* have registered the job load */
    char       *load_formula;           /* this is a formula used for combining and  */
                                        /* weighting different load values */ 
    u_long32    schedd_job_info;        /* enum schedd_job_info_key */
    lList*      schedd_job_info_list;   /* send information for this jobs to qmaster 
                                           if schedd_job_info is SCHEDD_JOB_INFO_JOB_LIST*/
  };

enum schedd_job_info_key {
   SCHEDD_JOB_INFO_FALSE=0,
   SCHEDD_JOB_INFO_TRUE,
   SCHEDD_JOB_INFO_JOB_LIST
};

typedef struct schedd_confel sge_schedd_conf_type;

extern sge_schedd_conf_type scheddconf;
extern lList *schedd_config_list;

lList *sge_set_scheduler_defaults(lList *);
void sge_show_schedd_conf(void);
int setScheddConfFromCull(sge_schedd_conf_type *, lList *);
int setScheddConfFromCmdLine(sge_schedd_conf_type *);

/* should get used to access job_load_adjustments */

int sc_set(lList **alpp, sge_schedd_conf_type *sc, lListElem *sc_ep, u_long32 *sip, lList *cmplx_list);

#endif /* __SCHEDD_CONF_H */
