#ifndef __SGE_COMPLEX_SCHEDD_H
#define __SGE_COMPLEX_SCHEDD_H
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

/* Use this to control how resource attributes are generated:

   QS_STATE_FULL 
      All debitations caused by running jobs are in effect.
   QS_STATE_EMPTY 
      We ignore all debitations caused by running jobs.
      Ignore all but static load values.
*/
enum {
   QS_STATE_EMPTY,
   QS_STATE_FULL
};

void set_qs_state(int qs_state);

int get_qs_state(void);

void set_global_load_correction(int flag);

int get_global_load_correction(void);

void monitor_dominance(char *str, u_long32 mask);

int global_complexes2scheduler(lList **new_complex_list, 
                               lListElem *global_host, lList *complex_list, 
                               int recompute_debitation_dependent);

int host_complexes2scheduler(lList **new_complex_list, lListElem *host, 
                             lList *exechost_list, lList *complex_list, 
                             int recompute_debitation_dependent);  

int queue_complexes2scheduler(lList **new_complex_list, lListElem *queue, 
                              lList *host_list, lList *complex_list, 
                              int recompute_debitation_dependent);

int compare_complexes(int slots, lListElem *util_max_ep, lListElem *complex1, 
                      lListElem *complex2, char *availability_text, 
                      int is_threshold, int force_existence);

int fillComplexFromHost(lList **new_complex,  
                        lListElem *host, lList *complex, u_long32 layer, 
                        int recompute_debitation_dependent);

int debit_consumable(lListElem *jep, lListElem *ep, lList *complex_list, 
                     int slots, int config_nm, int actual_nm, 
                     const char *obj_name);

int attr_mod_threshold(lList **alpp, lListElem *qep, lListElem *new_ep, int nm, int primary_key, int sub_command, char *attr_name, char *object_name);

int ensure_attrib_available(lList **alpp, lListElem *ep, int nm);

#endif /* __SGE_COMPLEX_SCHEDD_H */



