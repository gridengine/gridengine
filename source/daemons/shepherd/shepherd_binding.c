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
 *  The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 *
 *  Copyright: 2001 by Sun Microsystems, Inc.
 *
 *  All Rights Reserved.
 *
 ************************************************************************/
/*___INFO__MARK_END__*/

#include "shepherd_binding.h"
#include "sge_binding.h"

#if defined(SOLARISAMD64) || defined(SOLARIS86)
#  include "sge_uidgid.h"
#endif

#if ( defined(LINUXAMD64) || defined(LINUX86) ) && !defined(ULINUX86_24) && !defined(LINUXIA64_24)
int do_core_binding(void) 
{
   /* Check if "binding" parameter in 'config' file 
    * is available and not set to "binding=no_job_binding".
    * If so, we do an early abortion. 
    */
   char *binding = get_conf_val("binding");

   if (binding == NULL || strcasecmp("NULL", binding) == 0) {
      shepherd_trace("do_core_binding: \"binding\" parameter not found in config file");
      return -1;
   }
   
   if (strcasecmp("no_job_binding", binding) == 0) {
      shepherd_trace("do_core_binding: skip binding - no core binding configured");
      return -2;
   }
      
   /* do a binding accorting the strategy */
   if (strstr(binding, "linear") != NULL) {
      /* do a linear binding */ 
      int amount;
      int socket;
      int core;

      shepherd_trace("do_core_binding: do linear");
   
      /* get the amount of cores to bind on */
      if ((amount = binding_linear_parse_amount(binding)) < 0) {
         shepherd_trace("do_core_binding: couldn't parse the amount of cores from config file");
         return -1;
      } 

      /* get the socket to begin binding with (choosen by execution daemon) */
      if ((socket = binding_linear_parse_socket_offset(binding)) < 0) {
         shepherd_trace("do_core_binding: couldn't get the socket number from config file");
         return -1;
      }

      /* get the core to begin binding with (choosen by execution daemon)   */
      if ((core = binding_linear_parse_core_offset(binding)) < 0) {
         shepherd_trace("do_core_binding: couldn't get the core number from config file");
         return -1;
      }

      /* perform core binding on current process */
      if (binding_set_linear(socket, core, amount, 1) == false) {

         /* core binding was not successful */
         shepherd_trace("do_core_binding: linear binding was not successful");

      } else {

         /* DG TODO create binding file in active jobs directory */ 
         shepherd_trace("do_core_binding: TODO -> create binding file");
      }

   } else if (strstr(binding, "striding") != NULL) {
      int amount = binding_striding_parse_amount(binding);
      int stepsize = binding_striding_parse_step_size(binding);
      
      /* these are the real start parameters */
      int first_socket = 0, first_core = 0;
      
      char* reason = NULL;
      
      shepherd_trace("do_core_binding: striding");

      if (amount <= 0) {
         shepherd_trace("do_core_binding: error parsing <amount>");
         return -3;
      }

      if (stepsize < 0) {
         shepherd_trace("do_core_binding: error parsing <stepsize>");
         return -3;
      }
      
      first_core   = binding_striding_parse_first_core(binding);
      first_socket = binding_striding_parse_first_socket(binding);

      /* last core has to be incremented because core 0 is first core to be used */
      if (stepsize == 0) {
         /* stepsize must be >= 1 */
         stepsize = 1;
      }

      shepherd_trace("do_core_binding: striding set binding: first_core: %d first_socket %d amount %d stepsize %d", 
         first_core, first_socket, amount, stepsize);

      /* get the first core and first socket which is available for striding    */

      /* perform core binding on current process                */

      if (binding_set_striding(first_socket, first_core, 
         amount, 0, stepsize, &reason)) {

         /* we need job_id, ja_task_id and pe_task_id */
         u_long32 job_id = 0;
         char* c_job_id = NULL;
         u_long32 task_id = 0;
         char* c_task_id = NULL;
         char* pe_task_id = NULL;
            
         /* get job_id from configuration file  */
         if (((c_job_id = get_conf_val("job_id")) && c_job_id != NULL)) {
            job_id = atoi(c_job_id);
         } else {
           shepherd_trace("do_core_binding: didn't get job id from config - abort");
            return 0;
         }

         /* get task id from configuration file */
         if (((c_task_id = get_conf_val("task_id")) && c_task_id != NULL)) {
            task_id = atoi(c_task_id);
         } else {
            shepherd_trace("do_core_binding: didn't get task id from config - abort");
            return 0;
         }
      
         /* TODO DG we need the pe_task_id -> it seems not to be in the config */
         /* hence this works only for non-slave tasks now */ 

         shepherd_trace("do_core_binding: striding: binding done");
            
         /* write binding file */
         /* if we are on Solaris the psetid is reported in 'reason' */ 
         #if defined(SOLARISX86) || defined(SOLARISAMD64) 
            /* appending the processor set id */
            if (write_binding_file_striding(first_socket, first_core, amount, 
               stepsize, reason, job_id, task_id, pe_task_id) == true) {
               shepherd_trace("do_core_binding: successfully written binding file in active jobs dir");
            } else {
               shepherd_trace("do_core_binding: problems while writing binding file in active jobs dir");
            }
         #else 
            /* we have not to append the processor set id */
            write_binding_file_striding(first_socket, first_core, amount, stepsize, NULL, 
               job_id, task_id, pe_task_id);
         #endif 

            
      } else {
         if (reason != NULL) {
            shepherd_trace("do_core_binding: issues occured - no core binding done: %s", reason);
         } else {
            shepherd_trace("do_core_binding: issues occured - no core binding done");
         }
      }   

   } else if (strstr(binding, "explicit") != NULL) {

      /* list with the sockets (first part of the <socket>,<core> tuples) */
      int* sockets = NULL;
      /* length of sockets list */
      int nr_of_sockets = 0;
      /* list with the cores to be bound on the sockets */
      int* cores = NULL;
      /* length of cores list */
      int nr_of_cores = 0;

      shepherd_trace("do_core_binding: explicit");
      
      /* get <socket>,<core> pairs out of binding string */ 
      if (binding_explicit_exctract_sockets_cores(binding, &sockets, &nr_of_sockets,
            &cores, &nr_of_cores) == true) {

         if (nr_of_sockets == 0 && nr_of_cores == 0) {
            /* no cores and no sockets are found */
            shepherd_trace("do_core_binding: explicit: no socket or no core was specified");
         } else if (nr_of_sockets != nr_of_cores) {
            shepherd_trace("do_core_binding: explicit: unequal amount of specified sockets and cores");
         } else {
            /* do core binding according the <socket>,<core> tuples */
            if (binding_explicit(sockets, nr_of_sockets, cores, nr_of_cores) == true) {
               shepherd_trace("do_core_binding: explicit: binding done");
            } else {
               shepherd_trace("do_core_binding: explicit: no core binding done");
            }
            /* we are freeing these arrays allocated from the 
                  bindingExplicitExctractSocketsCores() function 
            */      
            FREE(sockets);
            FREE(cores);
         }

      }



   } else {
      
      if (binding != NULL) {
         shepherd_trace("do_core_binding: WARNING: unknown \"binding\" parameter: %s", 
            binding);
      } else {
         shepherd_trace("do_core_binding: WARNING: binding was null!");
      }   

   }
   
   
   shepherd_trace("do_core_binding: finishing");

   return 0;
}

#endif 

#if defined(SOLARIS86) || defined(SOLARISAMD64)
int do_core_binding(void)
{
   int retval = 0; 

   /* just read out what is in "config" file and attach to the given psrset if 
      it is specified */
   char *binding = get_conf_val("binding");
   
   if (binding == NULL) {
      shepherd_trace("do_core_binding: \"binding\" parameter not found in config file");
      retval = -1;
   } else if (strcasecmp("no_job_binding", binding) == 0 || strcasecmp("NULL", binding) == 0) {
      shepherd_trace("do_core_binding: skip binding - no core binding configured");
      retval = -2;
   }

   if (retval == 0 && strstr(binding, "psrset:") != NULL) {
      int processor_set_id = 0;
      shepherd_trace("do_core_binding: psrset found - attaching to it!");

      /* parse the psrset number right after "psrset:" */
      if (sge_strtok(binding, ":") != NULL) {
         /* parse the rest of the line */
         char* pset_id;
         if ((pset_id = sge_strtok(NULL, ":")) != NULL) {
            /* finally get the processor set id */
            processor_set_id = atoi(pset_id);
         } else {
            shepherd_trace("do_core_binding: couldn't find the psrset id after \"psrset:\" in config file (binding)");
            retval = -5;
         }
      } else {
         shepherd_trace("do_core_binding: found string \"psrset:\" but no \":\" - almost impossible");
         retval = -5;
      }

      if (retval == 0) {
         if (processor_set_id == -1) {            
            /* prcoessor_set_id == -1: Check here for a special processor_set_id (negative; 0)
               which does show that no binding is needed since this processor set
               would require (exactly) all of the remaining cores. Creating 
               such a processor set is not possible because one processor must 
               left for the OS. But the job is implicitly bound to the processors 
               since it can not use any onther processor from the other processor 
               sets. */
            shepherd_trace("do_core_binding: psrset not created since all remaining processors would be used");
            shepherd_trace("do_core_binding: binding is done implicitly");
         } else {
            /* start user rights (root) are required for creating processor sets */
            sge_switch2start_user();
            
            if (bind_shepherd_to_pset(processor_set_id) == false) {
               shepherd_trace("do_core_binding: couldn't bind to existing processor set!");
            } else {
               shepherd_trace("do_core_binding: successfully bound to existing processor set!");
            }
   
            /* switch back to admin user */
            sge_switch2admin_user();
         }
      }

   } else {  /* "psrset" is not in config file defined */
      shepherd_trace("do_core_binding: no processor set found in config file! do nothing");
      retval = -4;
   }

   shepherd_trace("do_core_binding: finishing");

   return retval;
}

#endif 

