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

#include "sge_binding.h" 
#include "sgermon.h"

#include "uti/sge_binding_hlp.h"

#if defined(SOLARISAMD64) || defined(SOLARIS86)
#  include <sys/processor.h>
#  include <sys/types.h>
#  include <sys/pset.h>
#  include <kstat.h>
#  include <sys/statfs.h>
#endif 

#include <pthread.h>

#include "uti/sge_log.h"
#include "uti/sge_binding_hlp.h"

#include "sgeobj/sge_answer.h"

#include "msg_common.h"

#define BINDING_LAYER TOP_LAYER

/* 
 * these sockets cores or threads are currently in use from SGE 
 * access them via getExecdTopologyInUse() because of initialization 
 */
static char* logical_used_topology = NULL;

static int logical_used_topology_length = 0;

#if defined(PLPA_LINUX) || defined(SOLARIS86) || defined(SOLARISAMD64)  

/* creates a string with the topology used from a single job */
static bool create_topology_used_per_job(char** accounted_topology, 
               int* accounted_topology_length, char* logical_used_topology, 
               char* used_topo_with_job, int logical_used_topology_length);

static bool get_free_sockets(const char* topology, const int topology_length, 
               int** sockets, int* sockets_size);

static int account_cores_on_socket(char** topology, const int topology_length,
               const int socket_number, const int cores_needed, int** list_of_sockets,
               int* list_of_sockets_size, int** list_of_cores, int* list_of_cores_size);

static bool get_socket_with_most_free_cores(const char* topology, const int topology_length, 
               int* socket_number);

static bool account_all_threads_after_core(char** topology, const int core_pos);

#endif

#if defined(SOLARISAMD64) || defined(SOLARIS86)
static bool get_topology_solaris(char** topology, int* length);

static int get_amount_of_sockets_from_matrix(const int** matrix, const int length);

static int get_amount_of_cores_from_matrix(const int** matrix, const int length, 
               int** cores, int* size);

static int get_amount_of_threads_from_matrix(const int** matrix, const int length, 
               int** threads, int* size); 

static bool get_chip_ids_from_matrix(const int** matrix, const int length, 
               int** chip_ids, int* amount);

static bool get_core_ids_from_matrix(const int** matrix, const int length, 
               int** core_ids, int* amount);

static bool get_ids_from_matrix(const int** matrix, const int length, 
               const int which_ID, int** ids, int* amount);

/* DG TODO this function is not MT -> introduce context */
static int is_new_id(const int id);
static int is_new_id_pair(const int id, const int id2);

static int get_amount_of_core_or_threads_from_matrix(const int** matrix, 
               const int length, int core, int** core_or_threads, int* size);

static int get_chip_id_from_logical_socket_number_solaris(const int** matrix, 
   const int length, const int logical_socket_number); 

static int get_core_id_from_logical_core_number_solaris(const int** matrix, 
   const int length, const int chip_id, const int logical_core_number);

/* access functions for load report */

static int get_total_amount_of_cores_solaris(void);

static int get_total_amount_of_sockets_solaris(void);

/* for processor sets */
static bool get_processor_ids_solaris(const int** matrix, const int length, const int logical_socket_number,
      const int logical_core_number, processorid_t** pr_ids, int* pr_length);

/* this could be used later on */
static int get_processor_id_solaris(const int** matrix, const int length, const int logical_socket_number, 
      const int logical_core_number, const int logical_thread_number, processorid_t* prid);

static bool binding_set_linear_solaris(const int first_socket, const int first_core, 
   const int amount_of_cores, const int step_size, psetid_t* psetid, 
   const binding_type_t type, char** env);

/* processor set related */
static bool create_pset(const processorid_t* plist, const int length, 
   psetid_t* pset_id);

static bool delete_pset(psetid_t pset_id);

static bool bind_current_process_to_pset(psetid_t pset_id);

static void create_environment_string_solaris(const processorid_t* pid_list, 
               const int pid_list_size, char** env); 

#endif

/* arch independent functions */
/****** sge_binding/get_execd_amount_of_cores() ************************************
*  NAME
*     get_execd_amount_of_cores() -- Returns the total amount of cores the host has. 
*
*  SYNOPSIS
*     int get_execd_amount_of_cores() 
*
*  FUNCTION
*     Retrieves the total amount of cores (currently Linux only) 
*     the current host have.
*
*  RESULT
*     int - The amount of cores the current host has. 
*
*  NOTES
*     MT-NOTE: get_execd_amount_of_cores() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int get_execd_amount_of_cores() 
{
#if defined(PLPA_LINUX) 
      return get_total_amount_of_cores();
#elif defined(SOLARISAMD64) || defined(SOLARIS86) 
      return get_total_amount_of_cores_solaris();
#else   
      return 0;
#endif  
}

/****** sge_binding/get_execd_amount_of_sockets() **********************************
*  NAME
*    get_execd_amount_of_sockets() -- The total amount of sockets in the system. 
*
*  SYNOPSIS
*     int get_execd_amount_of_sockets() 
*
*  FUNCTION
*     Calculates the total amount of sockets available in the system. 
*
*  INPUTS
*
*  RESULT
*     int - The total amount of sockets available in the system.
*
*  NOTES
*     MT-NOTE: get_execd_amount_of_sockets() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int get_execd_amount_of_sockets()
{
#if defined(PLPA_LINUX) 
   return get_amount_of_sockets();
#elif defined(SOLARISAMD64) || defined(SOLARIS86) 
   return get_total_amount_of_sockets_solaris();
#else
   return 0;
#endif
}


bool get_execd_topology(char** topology, int* length)
{
   bool success = false;

   /* topology must be a NULL pointer */
   if (topology != NULL && (*topology) == NULL) {
#if defined(PLPA_LINUX)  
      if (get_topology_linux(topology, length) == true) {
         success = true;
      } else {
         success = false;
      }   
#elif defined(SOLARISAMD64) || defined(SOLARIS86) 
      if (get_topology_solaris(topology, length) == true) {
         success = true;
      } else {
         success = false;
      }   
#else 
      /* currently other architectures are not supported */
      success = false;
#endif
   }

  return success; 
}


/****** sge_binding/getExecdTopologyInUse() ************************************
*  NAME
*     getExecdTopologyInUse() -- Creates a string which represents the used topology. 
*
*  SYNOPSIS
*     bool getExecdTopologyInUse(char** topology) 
*
*  FUNCTION
*     
*     Checks all jobs (with going through active jobs directories) and their 
*     usage of the topology (binding). Afterwards global "logical_used_topology" 
*     string is up to date (which is also updated when a job ends and starts) and 
*     a copy is made available for the caller. 
*     
*     Note: The memory is allocated within this function and 
*           has to be freed from the caller afterwards.
*  INPUTS
*     char** topology - out: the current topology in use by jobs 
*
*  RESULT
*     bool - true if the "topology in use" string could be created 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: getExecdTopologyInUse() is not MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool get_execd_topology_in_use(char** topology)
{
   bool retval = false;

   /* topology must be a NULL pointer */
   if ((*topology) != NULL) {
      return false;
   }   

   if (logical_used_topology_length == 0 || logical_used_topology == NULL) {
#if defined(PLPA_LINUX) 
      /* initialize without any usage */
      get_topology_linux(&logical_used_topology, 
              &logical_used_topology_length); 
#elif defined(SOLARISAMD64) || defined(SOLARIS86) 
      get_topology_solaris(&logical_used_topology, 
               &logical_used_topology_length);
#endif
   }

   if (logical_used_topology_length > 0) {
      /* copy the string */
      (*topology) = sge_strdup(NULL, logical_used_topology);
      retval = true;
   } 
      
   return retval;   
}

#if defined(PLPA_LINUX) || defined(SOLARISAMD64) || defined(SOLARIS86) 
/* gets the positions in the topology string from a given <socket>,<core> pair */
static int get_position_in_topology(const int socket, const int core, const char* topology, 
   const int topology_length);

/* accounts all occupied resources given by a topology string into another one */
static bool account_job_on_topology(char** topology, const int topology_length, 
   const char* job, const int job_length);  

/* DG TODO length should be an output */
static bool is_starting_point(const char* topo, const int length, const int pos, 
   const int amount, const int stepsize, char** topo_account); 
#endif

/* find next core in topology string */
#if 0
static bool go_to_next_core(const char* topology, const int pos, int* new_pos); 
#endif


#if defined(SOLARISAMD64) || defined(SOLARIS86)
/****** sge_binding/binding_set_linear_solaris() *******************************
*  NAME
*     binding_set_linear_solaris() -- Binds current process to some cores. 
*
*  SYNOPSIS
*     bool binding_set_linear_solaris(const int first_socket, const int 
*     first_core, const int amount_of_cores, const int step_size, psetid_t* 
*     psetid) 
*
*  FUNCTION
*     Binds the current process to some cores using the Solaris processor sets. 
*     Creating such processor sets requires root privileges. First the socket 
*     and core numbers of the cores which have to be selected are determined. 
*     Afterwards these tuples are converted to Solaris internal processor ids. 
*     A processor set is created and these processor ids are added. Then the 
*     current process is bound to that processor set. 
*    
*     This processor set is remaining active (and consuming all processors 
*     out of this set) until the processor set is deleted. This have to be done 
*     when the job ends. 
*
*  INPUTS
*     const int first_socket    - First socket to start with 
*     const int first_core      - First core to start with 
*     const int amount_of_cores - The amount of cores to bind to 
*     const int step_size       - The step size in order to select the cores 
*     psetid_t* psetid          - out: The processor set id which was generated. 
*
*  RESULT
*     bool - true if the binding was successful - false if not
*
*  NOTES
*     MT-NOTE: binding_set_linear_solaris() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool binding_set_linear_solaris(const int first_socket, const int first_core, 
   const int amount_of_cores, const int step_size, psetid_t* psetid, 
   const binding_type_t type, char** env)
{
   /* the topology matrix */
   int** matrix = NULL;
   /* size of the topology matrix */
   int mlength = 0;
   /* are threads (to bind) available (this would be done then) */
   bool threads = false;
   /* amount of sockets in the system */
   int csockets = 0;
   /* amount of cores on each socket */ 
   int* cores = NULL;
   /* current position */ 
   int current_socket = first_socket;
   int current_core   = first_core;
   /* internal processor_ids (which could be threads or cores) */ 
   processorid_t* pid_list = NULL;
   /* current length of the internal pid_list */
   int pid_list_length = 0;
   /* temporary pid list and length */ 
   processorid_t* tmp_pid_list = NULL;
   int tmp_pid_list_length = 0;
   /* return value: successful or not */
   bool retval = true;
   /* counter */
   int i = 0;

   /* check parameter */
   if (psetid == NULL) {
      /* no memory location of output parameter */
      return false;
   }

   /* first get the topology of the host into a topology matrix */ 
   if (generate_chipID_coreID_matrix(&matrix, &mlength) != true) {
      /* couldn't generate topology matrix */
      return false;
   }

   /* count sockets in the system */
   /* count the cores on each socket */ 
   get_amount_of_cores_from_matrix((const int**)matrix, mlength, &cores, &csockets);

   /* count threads on all cores */
   /* use ALL threads from a particular core meaning that 
      we have to add more processor_ids (since they are representing 
      threads too) then selected cores */

   /* go to first position: first_socket, first_core */ 
   /* same strategy than Linux: if core number is too high, move on to next socket */
   while (cores[current_socket] <= current_core) {
      /* reduce the core number by the number of cores we stepped over */
      current_core -= cores[current_socket];   
      /* increase the socket number */
      current_socket++;
      /* check if next socket will be on system */
      if (current_socket >= csockets) {
         /* we are out of range already - do nothing - abort */
         /* free memory */
         free_matrix(matrix, mlength);
         FREE(cores);
         return false;
      }
   }

   /* we have the first current_socket and current_core */
   /* hence we get the processor_ids (more in case of chip multithreading) */
   if (get_processor_ids_solaris((const int**) matrix, mlength, current_socket,
      current_core, &tmp_pid_list, &tmp_pid_list_length) == false) {
      /* we got no Solaris processor id - abort */
      free_matrix(matrix, mlength);
      FREE(cores);
      return false;
   }

   /* allocate new memory for the global pid list */
   pid_list = (processorid_t *) malloc(tmp_pid_list_length * sizeof(processorid_t));
   /* append the processor ids to the global pid list */
   for (i = 0; i < tmp_pid_list_length; i++) {
      pid_list[i] = tmp_pid_list[i];
   }
   
   /* update length of array */
   pid_list_length = tmp_pid_list_length;

   /* try to get the processor_ids from socket and core position (could be 
      more than one because of CMT */
   for (i = 1; i < amount_of_cores; i++) {   
      
      /* LINEAR strategy: go just to the next core (step_size = 1) */
      current_core += step_size;

      /* check if 'current_core' is on current_socket */
      while (cores[current_socket] <= current_core) {
         /* reduce the core number by the number of cores we stepped over */
         current_core -= cores[current_socket];   
         /* increase the socket number */
         current_socket++;
         /* check if next socket will be on system */
         if (current_socket >= csockets) {
            /* we are out of range already - do nothing - abort */
            /* free memory */
            free_matrix(matrix, mlength);
            FREE(cores);
            FREE(pid_list);
            FREE(tmp_pid_list);
            return false;
         }
      } /* end while getting the correct current_socket number */
      
      /* collect the processor_ids (more in case of chip multithreading) */
      if (get_processor_ids_solaris((const int**) matrix, mlength, current_socket,
         current_core, &tmp_pid_list, &tmp_pid_list_length) == false) {
         /* got no internal processor ids */
         free_matrix(matrix, mlength);
         FREE(cores);
         FREE(pid_list);
         return false;
      }

      /* grow allocated memory for processor ids */
      pid_list = (processorid_t *) realloc(pid_list, (pid_list_length 
         + tmp_pid_list_length) * sizeof(processorid_t));

      if (pid_list == NULL) {
         /* out of memory */ 
         free_matrix(matrix, mlength);
         FREE(cores);
         FREE(tmp_pid_list);
         return false;
      }

      /* append the new pids to the pid list */ 
      int prid_cntr = 0;
      for (prid_cntr = 0; prid_cntr < tmp_pid_list_length; prid_cntr++) {
         /* copy processor id from the temporary list to the global list */
         pid_list[pid_list_length + prid_cntr] = tmp_pid_list[prid_cntr];
      }

      /* update global pid list length */
      pid_list_length += tmp_pid_list_length;

      FREE(tmp_pid_list);
   }

     /* check what we've todo with the processor id list: 
      - ENV -> set environment variable 
      - PE  -> DG TODO
      - SET -> create the processor set */
   if (type == BINDING_TYPE_ENV) {
      /* just set the environment variable */
      create_environment_string_solaris(pid_list, pid_list_length, env);
      *psetid = 0;
   } else if (type != BINDING_TYPE_PE) {
      /* finally bind the current process to the global pid_list and get the
         processor set id */
      if (create_pset(pid_list, pid_list_length, psetid) != true) {
         retval = false;
      } else if (bind_current_process_to_pset(*psetid)) {
         /* current process is bound to psetid and psetid is output parameter */
         retval = true;
      } else {
         /* binding was not successful */
         retval = false;
      }
   }

   /* free memory in any case */ 
   free_matrix(matrix, mlength);
   FREE(cores);
   FREE(pid_list);

   return retval;
}


/****** sge_binding/create_processor_set_explicit_solaris() ********************
*  NAME
*     create_processor_set_explicit_solaris() -- ??? 
*
*  SYNOPSIS
*     int create_processor_set_explicit_solaris(const int* list_of_sockets, 
*     const int samount, const int* list_of_cores, const int camount, const 
*     binding_type_t type, char** env) 
*
*  FUNCTION
*     Creates a processor set (when binding typ is "set") containing the 
*     given <socket,core> pairs.
*
*  INPUTS
*     const int* list_of_sockets - List of sockets to use for core binding 
*     const int samount          - Size of socket list 
*     const int* list_of_cores   - List of cores to use for core binding 
*     const int camount          - Size of core list 
*     const binding_type_t type  - Type of binding request (set,env or pe) 
*
*  OUTPUTS
*     char** env                 - String with content of SGE_BINDING or NULL 
*                                  when binding type other than "env"
*
*  RESULT
*     int - 
*
*  NOTES
*     MT-NOTE: create_processor_set_explicit_solaris() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int create_processor_set_explicit_solaris(const int* list_of_sockets,
   const int samount, const int* list_of_cores, const int camount, 
   const binding_type_t type, char** env)
{
   /* tmp variables */
   int i, j, chip_id, pr_id;
   /* internal processor IDs to bind to */
   processorid_t* pid_list = NULL;
   /* processor set id */
   processorid_t psetid;
   /* the topology matrix */
   int** matrix = NULL;
   /* size of the topology matrix */
   int length = 0;
   /* current length of the processor id list */
   int pid_list_length = 0; 
   /* length of the processor id list for one core (could be >1 in CMT case) */
   int tmp_pid_list_length = 0;
   /* processor id list for one core */
   int* tmp_pid_list = NULL;
   
   /* assert that both lists have the same length */
   if (samount != camount) {
      return -1;
   }   

   /* first get the topology of the host into a topology matrix */ 
   if (generate_chipID_coreID_matrix(&matrix, &length) != true) {
      /* couldn't generate topology matrix */
      return -1;
   }

   /* allocate new memory for the processor id list (it has the minimum size 
      of amount of cores requested but could be larger in CMT case */

   /* generate pid list for processor set creation */
   for (i = 0; i < samount; i++) {
      
      /* get the processor ids for the given socket and core */
      if (get_processor_ids_solaris((const int**) matrix, length, list_of_sockets[i],
         list_of_cores[i], &tmp_pid_list, &tmp_pid_list_length) == false) {
         /* we got no Solaris processor ID - abort */
         free_matrix(matrix, length);
         FREE(pid_list);
         return -1;
      }
     
      /* add the processor IDs to the global list */
      pid_list = (processorid_t *) realloc(pid_list, (pid_list_length 
                                    + tmp_pid_list_length) * sizeof(processorid_t));
      /* append the processor ids to the global pid list */
      for (j = pid_list_length; j < pid_list_length + tmp_pid_list_length; j++) {
         pid_list[j] = tmp_pid_list[j - pid_list_length];
      }

      /* update size of processor ID list */
      pid_list_length += tmp_pid_list_length;

      FREE(tmp_pid_list);
   }

   /* check what we've todo with the processor id list: 
      - ENV -> set environment variable 
      - PE  -> do nothing (pe_hostfile will be updated) 
      - SET -> create the processor set */
   if (type == BINDING_TYPE_ENV) {
      /* just set the environment variable */
      create_environment_string_solaris(pid_list, pid_list_length, env);
      psetid = 0;
   } else if (type == BINDING_TYPE_PE) {
      psetid = 0; 
   } else {
      /* create processor set */
      if (create_pset(pid_list, pid_list_length, &psetid) != true) {
         /* error while doing this... */
         free_matrix(matrix, length);
         FREE(pid_list);
         return -1;
      }
   }   

   /* free topology matrix */ 
   free_matrix(matrix, length);
   FREE(pid_list);

   return (int) psetid;
}

/****** sge_binding/create_environment_string_solaris() ************************
*  NAME
*     create_environment_string_solaris() -- Creates a string with processor ids. 
*
*  SYNOPSIS
*     static void create_environment_string_solaris(const processorid_t* 
*     pid_list, const int pid_list_size, char** environment) 
*
*  FUNCTION
*     Creates a string with space separated processor ids. This string is used 
*     later on as the environment varibale SGE_BINDING which can be used 
*     by the application to bind itself to these. 
*
*  INPUTS
*     const processorid_t* pid_list - List with OS internal processor ids 
*     const int pid_list_size       - Length of list
*
*  OUTPUTS
*     char** environment            - String with the list space separated. 
*
*  RESULT
*     static void - nothing
*
*  NOTES
*     MT-NOTE: create_environment_string_solaris() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static void create_environment_string_solaris(const processorid_t* pid_list, 
               const int pid_list_size, char** environment) 
{
   int plc;
   dstring proc = DSTRING_INIT;
   dstring env  = DSTRING_INIT;

   /* add all Solaris internal processor numbers */
   for (plc = 0; plc < pid_list_size; plc++) {
      sge_dstring_sprintf(&proc, "%d ", pid_list[plc]);
      sge_dstring_append_dstring(&env, &proc);
      sge_dstring_clear(&proc);
   }

   *environment = (char*) calloc((sge_dstring_strlen(&env)+1), sizeof(char));
   if (memcpy(*environment, sge_dstring_get_string(&env), 
                           sge_dstring_strlen(&env) * sizeof(char)) == NULL) {
      /* error while copying */                            
   }   

   sge_dstring_free(&env);
   sge_dstring_free(&proc);
}


/****** sge_binding/create_processor_set_striding_solaris() ********************
*  NAME
*     create_processor_set_striding_solaris() -- Creates processor set for striding strategy.  
*
*  SYNOPSIS
*     int create_processor_set_striding_solaris(const int first_socket, const 
*     int first_core, const int amount, const int step_size, const 
*     binding_type_t type, char** env) 
*
*  FUNCTION
*     Create the processor set according to the input parameters for the striding 
*     core allocation schema. Depending on the type the processor set is created 
*     (default case), an environment variable is created and returned via the 
*     env output parameter, or the pe_hostfile is changed.
*
*  INPUTS
*     const int first_socket    - Socket to begin core allocation. 
*     const int first_core      - Core number to begin allocation. 
*     const int amount          - Amount of cores to allocate. 
*     const int step_size       - Step size (distance of the allocated cores)
*     const binding_type_t type - Type of binding (set, pe or env) 
*
*  OUTPUTS
*     char** env                - String which contains the content of SGE_BINDING
*                                 when not NULL.
*
*  RESULT
*     int - 
*
*  NOTES
*     MT-NOTE: create_processor_set_striding_solaris() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int create_processor_set_striding_solaris(const int first_socket, 
   const int first_core, const int amount, const int step_size, 
   const binding_type_t type, char** env) 
{
   /* the topology matrix */
   int** matrix = NULL;
   /* size of the topology matrix */
   int mlength = 0;
   /* are threads (to bind) available (this would be done then) */
   bool threads = false;
   /* amount of sockets in the system */
   int csockets = 0;
   /* amount of cores on each socket */ 
   int* cores = NULL;
   /* current position */ 
   int current_socket = first_socket;
   int current_core   = first_core;
   /* internal processor_ids (which could be threads or cores) */ 
   processorid_t* pid_list = NULL;
   /* current length of the internal pid_list */
   int pid_list_length = 0;
   /* temporary pid list and length */ 
   processorid_t* tmp_pid_list = NULL;
   int tmp_pid_list_length = 0;
   /* return value: processor set id */
   int retval = -1;
   /* counter */
   int i = 0;
   /* processor set id */
   processorid_t psetid;

   /* first get the topology of the host into a topology matrix */ 
   if (generate_chipID_coreID_matrix(&matrix, &mlength) != true) {
      /* couldn't generate topology matrix */
      return -1;
   }

   /* count sockets in the system */
   /* count the cores on each socket */ 
   get_amount_of_cores_from_matrix((const int**)matrix, mlength, &cores, &csockets);

   /* count threads on all cores */
   /* use ALL threads from a particular core meaning that 
      we have to add more processor_ids (since they are representing 
      threads too) then selected cores */

   /* go to first position: first_socket, first_core */ 
   /* same strategy than Linux: if core number is too high, move on to next socket */
   while (cores[current_socket] <= current_core) {
      /* reduce the core number by the number of cores we stepped over */
      current_core -= cores[current_socket];   
      /* increase the socket number */
      current_socket++;
      /* check if next socket will be on system */
      if (current_socket > csockets) {
         /* we are out of range already - do nothing - abort */
         /* free memory */
         free_matrix(matrix, mlength);
         FREE(cores);
         return -2;
      }
   }

   /* we have the first current_socket and current_core */
   /* hence we get the processor_ids (more in case of chip multithreading) */
   if (get_processor_ids_solaris((const int**) matrix, mlength, current_socket,
         current_core, &tmp_pid_list, &tmp_pid_list_length) == false) {
      /* we got no Solaris processor id - abort */
      free_matrix(matrix, mlength);
      FREE(cores);
      return -3;
   }

   /* allocate new memory for the global pid list */
   pid_list = (processorid_t *) malloc(tmp_pid_list_length * sizeof(processorid_t));
   /* append the processor ids to the global pid list */
   for (i = 0; i < tmp_pid_list_length; i++) {
      pid_list[i] = tmp_pid_list[i];
   }
   
   /* update length of array */
   pid_list_length = tmp_pid_list_length;

   FREE(tmp_pid_list);

   /* try to get the processor_ids from socket and core position (could be 
      more than one because of CMT */
   for (i = 1; i < amount; i++) { 
      
      /* strategy: go just to the next core (step_size = 1) */
      current_core += step_size;

      /* check if 'current_core' is on current_socket */
      while (cores[current_socket] <= current_core) {
         /* reduce the core number by the number of cores we stepped over */
         current_core -= cores[current_socket];   
         /* increase the socket number */
         current_socket++;
         /* check if next socket will be on system */
         if (current_socket > csockets) {
            /* we are out of range already - do nothing - abort */
            /* free memory */
            free_matrix(matrix, mlength);
            FREE(cores);
            FREE(pid_list);
            return -4;
         }
      } /* end while getting the correct current_socket number */
      
      /* collect the processor_ids (more in case of chip multithreading) */
      if (get_processor_ids_solaris((const int**) matrix, mlength, current_socket,
         current_core, &tmp_pid_list, &tmp_pid_list_length) == false) {
         /* we got no Solaris processor id - abort */
         free_matrix(matrix, mlength);
         FREE(cores);
         FREE(pid_list)
         return -3; 
      }   

      /* grow allocated memory for processor ids */
      pid_list = (processorid_t *) realloc(pid_list, (pid_list_length 
         + tmp_pid_list_length) * sizeof(processorid_t));

      if (pid_list == NULL) {
         /* out of memory */ 
         free_matrix(matrix, mlength);
         FREE(cores);
         FREE(pid_list);
         FREE(tmp_pid_list);
         return -5;
      }

      /* append the new pids to the pid list */ 
      int prid_cntr = 0;
      for (prid_cntr = 0; prid_cntr < tmp_pid_list_length; prid_cntr++) {
         /* copy processor id from the temporary list to the global list */
         pid_list[pid_list_length + prid_cntr] = tmp_pid_list[prid_cntr];
      }

      FREE(tmp_pid_list);

      /* update global pid list length */
      pid_list_length += tmp_pid_list_length;
   }

   /* check what we've todo with the processor id list: 
      - ENV -> set environment variable 
      - PE  -> do nothing here  
      - SET -> create the processor set */
   if (type == BINDING_TYPE_ENV) {
      /* just set the environment variable */
      create_environment_string_solaris(pid_list, pid_list_length, env);
      retval = 0; /* not an error */
   } else if (type == BINDING_TYPE_PE) {
      retval = 0; /* do nothing - not an error */ 
   } else {
      /* finally bind the current process to the global pid_list and get the
         processor set id -> root rights required !!! */
      if (create_pset(pid_list, pid_list_length, &psetid) != true) {
         /* couldn't generate processor set */
         retval = -6;
      } else {
         /* return processor set */
         retval = (int) psetid;
      }
   }   
   
   /* free memory in any case */ 
   free_matrix(matrix, mlength);
   FREE(cores);
   FREE(pid_list);

   return retval;
}



/****** sge_binding/free_matrix() **********************************************
*  NAME
*     free_matrix() -- Frees a previously allocated topology matrix. 
*
*  SYNOPSIS
*     static void free_matrix(int** matrix, const int length) 
*
*  FUNCTION
*     Frees all vectors inside the main vector. 
*
*  INPUTS
*     int** matrix     - Vectors of pointer to free. 
*     const int length - Length of vector of pointers to free. 
*
*  RESULT
*     static void - nothing
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: free_matrix() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
void free_matrix(int** matrix, const int length) 
{
   
   int i;
   if (matrix == NULL) {
      return;
   }
   for (i = 0; i < length; i++) {
      FREE(matrix[i]);
   }
   FREE(matrix);
}

/* -----------------------------------------------------------------------------
   SOLARIS PROCESSOR SETS 
*/

/****** sge_binding/create_pset() **********************************************
*  NAME
*     create_pset() -- Creates a specific processor set. 
*
*  SYNOPSIS
*     bool create_pset(const processorid_t* const plist, const int length, 
*     psetid_t* pset_id) 
*
*  FUNCTION
*     Creates a new processor set. Afterwards it attaches all processors 
*     from the given plist to the processor set. If this was successful 
*     pset_id is set to the ID of the processor set (output parameter), 
*     and the function returns true. 
*  
*     - pset_id must not be NULL 
*     - length must be > 0 
*     - and plist must not be NULL and have to contain at least one element
*
*  INPUTS
*     const processorid_t* const plist - Processor id list.  
*     const int length                 - Length of the processor id list.
*
*  OUTPUTS
*     psetid_t* const pset_id          - Pointer to the fixed location for id. 
*
*  RESULT
*     bool - true in case the pset was created and all processors from the list 
*            are in it
*
*  NOTES
*     MT-NOTE: create_pset() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool create_pset(const processorid_t* plist, const int length, 
   psetid_t* pset_id)
{
   /* counter for the processor id list */
   int i;
   /* return value which indicates if pset creation was successful */
   bool successful = true;

   /* check parameters plist must have values and pset_id must be allocated */
   if (plist == NULL || length == 0 || pset_id == NULL 
         || (pset_create(pset_id) == -1)) {
      
      /* invalid input values */
      successful = false;

   } else {  

      /* empty processor set was created */
   
      /* assign the selected processor to the set */ 
      for (i = 0; i < length && successful == true; i++) {

         /* try to assign processor id to the processor set */
         if (pset_assign(*pset_id, plist[i], NULL) == -1) {
            /* problem while assigning a CPU to the set */
            /* destroy the processor set and return with error */ 
            if (pset_destroy(*pset_id) != 0) {
               /* Ooops - we could have a major problem with a remaining pset */
               successful = false;
            } else {
               /* assigning processor failed but eventually we could delete the 
                  broken pset */
               successful = false;
            }
         }
      }

   }

   /* we could create the pset and assign all processors from the list to it */
   return successful;
}
   

/****** sge_binding/delete_pset() **********************************************
*  NAME
*     delete_pset() -- deletes the processor set  
*
*  SYNOPSIS
*     bool delete_pset(psetid_t pset_id) 
*
*  FUNCTION
*     Deletes an existing processor set with ID given as parameter. 
*
*  INPUTS
*     psetid_t pset_id - ID of the processor set
*
*  RESULT
*     bool - true in case the existing processor set could have been destroyed. 
*
*  NOTES
*     MT-NOTE: delete_pset() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool delete_pset(psetid_t pset_id)
{
   /* try to destroy the processor set */
   if (pset_destroy(pset_id) != 0) {
      /* couldn't delete pset */
      return false;
   }

   return true;
}

/****** sge_binding/bind_current_process_to_pset() *****************************
*  NAME
*     bind_current_process_to_pset() -- Bind current process to an exisiting pset 
*
*  SYNOPSIS
*     bool bind_current_process_to_pset(psetid_t pset_id) 
*
*  FUNCTION
*     Binds the current process to an existing processor set. All subprocesses 
*     (hence the job started by the shepherd) are inheriting this binding and 
*     are running *exclusively* within this set of processors. In case of a 
*     success the function returs true otherwise false.
*
*  INPUTS
*     psetid_t pset_id - Processor set id. 
*
*  RESULT
*     bool - true when the binding was successful otherwise false
*
*  NOTES
*     MT-NOTE: bind_current_process_to_pset() MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool bind_current_process_to_pset(psetid_t pset_id)
{
   /* try to bind current process to processor set */
   if (pset_bind(pset_id, P_PID, P_MYID, NULL) != 0) {
      /* binding was not successful */
      return false;
   }

   /* successfully bound current process to processor set */
   return true;
}

#endif 

#if defined(PLPA_LINUX) || defined(SOLARIS86) || defined(SOLARISAMD64)  


/****** sge_binding/account_job() **********************************************
*  NAME
*     account_job() -- Accounts core binding from a job on host global topology. 
*
*  SYNOPSIS
*     bool account_job(char* job_topology) 
*
*  FUNCTION
*      Accounts core binding from a job on host global topology.
*
*  INPUTS
*     char* job_topology - Topology used from core binding. 
*
*  RESULT
*     bool - true when successful otherwise false
*
*  NOTES
*     MT-NOTE: account_job() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool account_job(const char* job_topology)
{
   
   if (logical_used_topology_length == 0 || logical_used_topology == NULL) {

#if defined(PLPA_LINUX) 
      /* initialize without any usage */
      get_topology_linux(&logical_used_topology, 
              &logical_used_topology_length); 
#elif defined(SOLARISAMD64) || defined(SOLARIS86) 
      get_topology_solaris(&logical_used_topology, 
               &logical_used_topology_length);
#endif

   }

   return account_job_on_topology(&logical_used_topology, strlen(logical_used_topology), 
                           job_topology, strlen(job_topology)); 
}

/****** sge_binding/account_job_on_topology() **********************************
*  NAME
*     account_job_on_topology() -- Marks occupied resources. 
*
*  SYNOPSIS
*     static bool account_job_on_topology(char** topology, int* 
*     topology_length, const char* job, const int job_length) 
*
*  FUNCTION
*     Marks occupied resources from one topology string (job) which 
*     is usually a job on another topology string (topology) which 
*     is usually the execution daemon local topology string.
*
*  INPUTS
*     char** topology      - (in/out) topology on which the accounting is done 
*     int* topology_length - (in)  length of the topology stirng
*     const char* job      - (in) topology string from the job
*     const int job_length - (in) length of the topology string from the job
*
*  RESULT
*     static bool - true in case of success
*
*  NOTES
*     MT-NOTE: account_job_on_topology() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool account_job_on_topology(char** topology, const int topology_length, 
   const char* job, const int job_length)
{
   int i;
   
   /* parameter validation */
   if (topology_length != job_length ||  job_length <= 0 
      || topology == NULL || (*topology) == NULL || job == NULL) {
      return false;
   }

   /* go through topology and account */
   for (i = 0; i < job_length && job[i] != '\0'; i++) {
      if (job[i] == 'c') {
         (*topology)[i] = 'c';
      } else if (job[i] == 's') {
         (*topology)[i] = 's';
      } else if (job[i] == 't') {
         (*topology)[i] = 't';
      }
   }

   return true;
}



/****** sge_binding/binding_explicit_check_and_account() ***********************
*  NAME
*     binding_explicit_check_and_account() -- Checks if a job can be bound.  
*
*  SYNOPSIS
*     bool binding_explicit_check_and_account(const int* list_of_sockets, const 
*     int samount, const int** list_of_cores, const int score, char** 
*     topo_used_by_job, int* topo_used_by_job_length) 
*
*  FUNCTION
*     Checks if the job can bind to the given by the <socket>,<core> pairs. 
*     If so these cores are marked as used and true is returned. Also an 
*     topology string is returned where all cores consumed by the job are 
*     marked with smaller case letters. 
*
*  INPUTS
*     const int* list_of_sockets   - List of sockets to be used 
*     const int samount            - Size of list_of_sockets 
*     const int** list_of_cores    - List of cores (on sockets) to be used 
*     const int score              - Size of list_of_cores 
*
*  OUTPUTS
*     char** topo_used_by_job      -  Topology with resources job consumes marked.
*     int* topo_used_by_job_length -  Topology string length.
*
*  RESULT
*     bool - True if the job can be bound to the topology, false if not. 
*
*  NOTES
*     MT-NOTE: binding_explicit_check_and_account() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool binding_explicit_check_and_account(const int* list_of_sockets, const int samount, 
   const int* list_of_cores, const int score, char** topo_used_by_job, 
   int* topo_used_by_job_length)
{
   int i;

   /* position of <socket>,<core> in topology string */
   int pos;
   /* status if accounting was possible */
   bool possible = true;

   /* input parameter validation */
   if (samount != score || samount <= 0 || list_of_sockets == NULL 
         || list_of_cores == NULL) {
      return false;
   }

   /* check if the topology which is used already is accessable */
   if (logical_used_topology == NULL) {
      /* we have no topology string at the moment (should be initialized before) */
      if (!get_execd_topology(&logical_used_topology, &logical_used_topology_length)) {
         /* couldn't even get the topology string */
         return false;
      }
   }
   
   /* create output string */ 
   get_execd_topology(topo_used_by_job, topo_used_by_job_length);

   /* go through the <socket>,<core> pair list */
   for (i = 0; i < samount; i++) {

      /* get position in topology string */
     if ((pos = get_position_in_topology(list_of_sockets[i], list_of_cores[i], 
        logical_used_topology, logical_used_topology_length)) < 0) {
        /* the <socket>,<core> does not exist */
        possible = false;
        break;
     } 

      /* check if this core is available (DG TODO introduce threads) */
      if (logical_used_topology[pos] == 'C') {
         /* do temporarily account it */
         (*topo_used_by_job)[pos] = 'c';
         /* thread binding: account threads here */
         account_all_threads_after_core(topo_used_by_job, pos);
      } else {
         /* core not usable -> early abort */
         possible = false;
         break;
      }
   }
   
   /* do accounting if all cores can be used */
   if (possible) {
      if (account_job_on_topology(&logical_used_topology, logical_used_topology_length, 
         *topo_used_by_job, *topo_used_by_job_length) == false) {
         possible = false;
      }   
   }

   /* free memory when unsuccessful */
   if (possible == false) {
      free(*topo_used_by_job);
      *topo_used_by_job = NULL;
      *topo_used_by_job_length = 0;
   }

   return possible;
}

/****** sge_binding/free_topology() ********************************************
*  NAME
*     free_topology() -- Free cores used by a job on module global accounting string. 
*
*  SYNOPSIS
*     bool free_topology(const char* topology, const int topology_length) 
*
*  FUNCTION
*     Frees global resources (cores, sockets, or threads) which are marked as 
*     beeing used (lower case letter, like 'c' 's' 't') in the given 
*     topology string. 
*
*  INPUTS
*     const char* topology      - Topology string with the occupied resources. 
*     const int topology_length - Length of the topology string 
*
*  RESULT
*     bool - true in case of success; false in case of a topology mismatch 
*
*  NOTES
*     MT-NOTE: free_topology() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool free_topology(const char* topology, const int topology_length) 
{
   /* free cores, sockets and threads in global accounting */
   int i;
   int size = topology_length;

   if (topology_length < 0) {
      /* size not known but we stop at \0 */
      size = 1000000;
   }
   
   for (i = 0; i < size && i < logical_used_topology_length && 
      topology[i] != '\0' && logical_used_topology[i] != '\0'; i++) {
      
      if (topology[i] == 'c') {
         if (logical_used_topology[i] != 'c' && logical_used_topology[i] != 'C') {
            /* topology type mismatch: input parameter is not like local topology */
            return false;
         } else {
            logical_used_topology[i] = 'C';
         }
      } else if (topology[i] == 't') {
         if (logical_used_topology[i] != 't' && logical_used_topology[i] != 'T') {
            /* topology type mismatch: input parameter is not like local topology */
            return false;
         } else {
            logical_used_topology[i] = 'T';
         }
      } else if (topology[i] == 's') {
         if (logical_used_topology[i] != 's' && logical_used_topology[i] != 'S') {
            /* topology type mismatch: input parameter is not like local topology */
            return false;
         } else {
            logical_used_topology[i] = 'S';
         }
      }

   }

   return true;
}

#endif

/* ---------------------------------------------------------------------------*/
/* ---------------------------------------------------------------------------*/
/*                    Beginning of LINUX related functions                    */
/* ---------------------------------------------------------------------------*/
/* ---------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*/
/* ---------------------------------------------------------------------------*/
/*                    Ending of LINUX related functions                       */
/* ---------------------------------------------------------------------------*/
/* ---------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*/
/* ---------------------------------------------------------------------------*/
/*                    Beginning of SOLARIS related functions                  */
/* ---------------------------------------------------------------------------*/
/* ---------------------------------------------------------------------------*/


#if defined(SOLARISAMD64) || defined(SOLARIS86)

/****** sge_binding/get_topology_solaris() *************************************
*  NAME
*     get_topology_solaris() -- Creates the topology string. 
*
*  SYNOPSIS
*     static bool get_topology_solaris(char** topology, int* length) 
*
*  FUNCTION
*     Creates the topology string of the host. The topology pointer has 
*     to be initialized with NULL when calling this function.
*
*  OUTPUTS 
*     char** topology - Pointer to the topology string. 
*     int* length     - Length of the topology string. 
*
*  RESULT
*     static bool - 
*
*  EXAMPLE
*     char* topo = NULL;
*     int length = 0;
*     get_topology_solaris(&topo, &length);
*     printf("topology: %s", topo);
*
*  NOTES
*     MT-NOTE: get_topology_solaris() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool get_topology_solaris(char** topology, int* length)
{
   /* TODO implement the topology stuff */

   /* Algorithm: 
      - create matrix (with the topology)
      - get cores_per_socket vector (cps)
      - get threads_per_core vector (tpc) 
      - go through cps (counter 'socket')
         - append "S" 
         - for (int 'core' = 0; 'core' < cps['socket']; 'core'++)
            - append "C" 
            - for (int k = 0; k < tpc[core + listed_cores] k++) 
               - if (tpc[core] > 1) 
                  - append [tpc[core] times "T"
               - endif 
            - end for   
            - listed_core +=    
   */
   
   /* topology string */
   
   /* matrix with the kstat values */
   int** matrix = NULL;
   int matrix_length = 0;

   /* vector with the amount of cores per socket */
   int* cores_per_socket = NULL;
   int cores_per_socket_length = 0;
   
   /* vector with the amount of hw threads per core */
   int* threads_per_core = NULL;
   int threads_per_core_length = 0;

   /* counters */
   int all_cores = 0;
   int socket = 0;
   int core = 0;

   /* topology particels */
   char* S = "S"; /* socket */
   char* C = "C"; /* core   */
   char* T = "T"; /* thread */
   
   bool retval = true;
   
   dstring d_topology = DSTRING_INIT;

   (*length) = 0;

   /* generate matrix with socket_id and core_id */
   if (generate_chipID_coreID_matrix(&matrix, &matrix_length)) {

      /* clear topology string */ 
      sge_dstring_clear(&d_topology);
      
      /* get cores per socket array */
      get_amount_of_core_or_threads_from_matrix((const int**)matrix, matrix_length, 1, 
         &cores_per_socket, &cores_per_socket_length);
      
      /* get threads per core array */
      get_amount_of_core_or_threads_from_matrix((const int**)matrix, matrix_length, 0, 
         &threads_per_core, &threads_per_core_length);

      /* go through all sockets */
      for (socket = 0; socket < cores_per_socket_length; socket++) {
         /* add "S" */
         sge_dstring_append_char(&d_topology, *S);
         (*length)++;

         /* go through all cores */
         for (core = 0; core < cores_per_socket[socket]; core++) {
            
            /* add "C" */
            sge_dstring_append_char(&d_topology, *C);
            (*length)++;

            /* append the amount of threads if > 1 */
            if (threads_per_core_length > (all_cores + core) 
                  && threads_per_core[all_cores + core] > 1) {
               int t;
               for (t = 0; t < threads_per_core[all_cores + core]; t++) {
                  /* append "T" */
                  sge_dstring_append_char(&d_topology, *T);
                  (*length)++;
               }
            }

         } /* all cores */
         /* go one socket further */ 
         all_cores += cores_per_socket[socket];
      } /* all sockets */


      /* free resources allocated in subfunctions */
      free_matrix(matrix, matrix_length);
      FREE(threads_per_core);
      FREE(cores_per_socket);
   }
   
   if ((*length) == 0) {
      /* we couldn't get the kernel kstat values therefore we have no topology */
      (*topology) = sge_strdup(NULL, "NONE");
      (*length)   = 5;
      retval = false;
      sge_dstring_free(&d_topology);

   } else {
      /* we need `\0' at the end */
      (*length) += 1;
                 
      /* free matrix, cores_per_socket, and threads_per_socket vector */
      (*topology) = sge_strdup(NULL, sge_dstring_get_string(&d_topology));

      sge_dstring_free(&d_topology);
   }   

   

   return retval;
}

/****** sge_binding/generate_chipID_coreID_matrix() ********************************
*  NAME
*     generate_chipID_coreID_matrix() -- Generates matrix with OS specific proc settings. 
*
*  SYNOPSIS
*     int generate_chipID_coreID_matrix(int*** matrix, int* length) 
*
*  FUNCTION
*     Generates a two dimensional matrix with <core_id>,<socket_id>,<processor_id> 
*     tuples. The amount of tuples is returned via length. 
*     The matrix contains all entries found in the kernel kstat 
*     structure "cpu_info".
*
*     Important: matrix must be the address of a NULL pointer 
*                otherwise the function will not allocate new memory
*    
*  INPUTS
*     int*** matrix - output: pointer to the 2 dimensional matrix 
*     int* length   - output: amount of entries in the matrix 
*
*  RESULT
*     bool - true when the matrix was initialized correctly otherwise false
*
*  EXAMPLE
*     int** matrix = NULL; 
*     int length = 0;
*     if (generate_chipID_coreID_matrix(&matrix, &length)) 
*        for (int i = 0; i < length; i++)
*           printf("chip_id %d core_id %d processor_id", matrix[i][0], 
*                    matrix[i][1], matrix[i][2]);
*
*  NOTES
*     MT-NOTE: generate_chipID_coreID_matrix() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool generate_chipID_coreID_matrix(int*** matrix, int* length) 
{
   /* return value */
   bool success = true;
   
   /* kstat structures needed for accessing the kernel statistics */
	kstat_t *cpu_info = NULL;
   kstat_ctl_t *kstat = NULL;
	kstat_named_t *kdata = NULL;
   int chip_id, core_id, processor_id;

   /* initialize length of matrix (width is always 3) */ 
   (*length) = 0;
   if (*matrix != NULL) {
      /* expecting a NULL pointer */
      return false;
   }

   /* initialize kernel statistics facility */
   kstat = kstat_open();
   if (kstat == NULL) {
		/* couldn't open kstat */
		return false;
   }

   /* get pointer to the cpu_info kstat */
   cpu_info = kstat_lookup(kstat, "cpu_info", -1, NULL);

   /* loop over all cpu_info entries */
   for (; cpu_info != NULL; cpu_info = cpu_info->ks_next) {

      /* if this is not the cpu_info module skip the entry */
      if (strcmp(cpu_info->ks_module, "cpu_info")) {
         continue;
      }

      /* update kstat with current cpu_info */
		if (kstat_read(kstat, cpu_info, NULL) == -1) { 
         /* unable to get the data from kernel */
         continue;
      }   

      /* get the chip_id which reflects the socket */
      kdata = kstat_data_lookup(cpu_info, "chip_id");
      if (kdata == NULL) {
         /* couldn't get data */ 
         continue;
      }   
      chip_id = kdata->value.l;

      /* get the core_id which reflects the core and threads 
         when multiple same core ids are on one chip_id */
      kdata = kstat_data_lookup(cpu_info, "core_id");
      if (kdata == NULL) {
         /* couldn't get data */ 
         continue;
      }
      core_id = kdata->value.l;
      
      /* DG TODO -> we need a serious proof that the assertion is true */
      /* assert: the instance number is the processor_id */
      processor_id = cpu_info->ks_instance;

      /* add the values into the matrix */ 
      (*length)++;

      /* this function is not called often, so performance is not an issue here */
      *matrix = (int **) realloc(*matrix, (*length) * sizeof(int *));
      if (*matrix == NULL) {
         /* out of memory */
         success = false;
         break;
      }

      /* get the memory for the two values */
      (*matrix)[(*length)-1] = (int *) calloc(3, sizeof(int));
      if ((*matrix)[(*length)-1] != NULL) {
         /* write chip_id and core_id into the matrix */
         ((*matrix)[(*length)-1])[0] = chip_id;
         ((*matrix)[(*length)-1])[1] = core_id;
         /* and we also need the processor id */ 
         ((*matrix)[(*length)-1])[2] = processor_id;
      } else {
         /* out of memory */
         success = false;
         break;
      }
   }
   
   /* do free memory if there was an error */ 
   if (success == false) {
      int i = 0;
      /* in case we are out of memory for calloc we have a memory leak 
         of one integer - this should only occur once */
      for (;i < (*length) - 1; i++) 
         free((*matrix)[i]);
      free(*matrix);
      *matrix = NULL;
      (*length) = 0;
   }

   /* close kernel statistics facility */
   if (kstat_close(kstat) != 0) {
      /* problems while closing */ 
   }

   return success;
} 


/****** sge_binding/get_amount_of_sockets_from_matrix() ****************************
*  NAME
*     get_amount_of_sockets_from_matrix() -- Get amount of sockets. 
*
*  SYNOPSIS
*     int get_amount_of_sockets_from_matrix(const int** matrix, const int 
*     length) 
*
*  FUNCTION
*     Gets the amount of sockets out of the given topology matrix. 
*
*  INPUTS
*     const int** matrix - Pointer to the matrix. 
*     const int length   - Size of the matrix. 
*
*  RESULT
*     int - Amount of sockets the architecture have.
*
*  NOTES
*     MT-NOTE: get_amount_of_sockets_from_matrix() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int get_amount_of_sockets_from_matrix(const int** matrix, const int length)
{
   int amount     = 0;
   int* chip_ids  = NULL;

   /* we don't care about the actual chip_ids here */
   if (get_chip_ids_from_matrix(matrix, length, &chip_ids, &amount) == true) {
      FREE(chip_ids);
   } else {
      amount = 0;
   }

   return amount;
}

/****** sge_binding/get_chip_ids_from_matrix() *************************************
*  NAME
*     get_chip_ids_from_matrix() -- Generates a vector with chips_ids.  
*
*  SYNOPSIS
*     int get_chip_ids_from_matrix(int** matrix, int length, int* chip_ids, 
*     int* amount) 
*
*  FUNCTION
*     Generates a vector which contains all different chip_ids found within 
*     the given matrix. The output parameter "amount" does contain the amount 
*     of different chip_ids (i.e. the amount of sockets) found in the given 
*     matrix.
*
*  INPUTS
*     int** matrix  - two dimensional matrix with chip_id and core_id 
*     int length    - length of the matrix 
*     int** chip_ids - output: pointer to the new allocated vector containing all 
*                     different chip_ids in the matrix
*     int* amount   - output: size of the vector with the chip_ids (amount of 
*                     different chip_ids found withing the matrix
*
*  RESULT
*     int - The return value has the same value than the output parameter amount. 
*           It reflects the amount of different chip_ids found in the matrix.
*
*  NOTES
*     MT-NOTE: get_chip_ids_from_matrix() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool get_chip_ids_from_matrix(const int** matrix, const int length, 
                                     int** chip_ids, int* amount)
{
   return get_ids_from_matrix(matrix, length, 0, chip_ids, amount);
}



/****** sge_binding/get_core_ids_from_matrix() *************************************
*  NAME
*     get_core_ids_from_matrix() -- ??? 
*
*  SYNOPSIS
*     int get_core_ids_from_matrix(const int** matrix, const int length, int** 
*     core_ids, int* amount) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const int** matrix - Topology matrix
*     const int length   - Size of topology matrix 
*
*  OUTPUTS
*     int** core_ids     - A list with the core_ids out of the matrix.
*     int* amount        - Length of the list with the core_ids.
* 
*  RESULT
*     int - 
*
*  NOTES
*     MT-NOTE: get_core_ids_from_matrix() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool get_core_ids_from_matrix(const int** matrix, const int length, 
                                     int** core_ids, int* amount)
{
   return get_ids_from_matrix(matrix, length, 1, core_ids, amount);
}


/****** sge_binding/get_ids_from_matrix() ******************************************
*  NAME
*     get_ids_from_matrix() -- Generates ids out of the topology matrix. 
*
*  SYNOPSIS
*     int get_ids_from_matrix(const int** matrix, const int length, const int 
*     which_ID, int** ids, int* amount) 
*
*  FUNCTION
*     Scans the topology matrix for ids (either chip_ids or core_ids) and 
*     creates a new list out of them. 
*
*  INPUTS
*     const int** matrix - Topology matrix 
*     const int length   - Size of the topology matrix 
*     const int which_ID - Determines which ids (core_ids, chip_ids) to select. 
*
*  OUTPUTS
*     int** ids          - List of IDs from the matrix. 
*     int* amount        - Size of the ID list.
*
*  RESULT
*     bool - true when the list was created false otherwise
*
*  NOTES
*     MT-NOTE: get_ids_from_matrix() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool get_ids_from_matrix(const int** matrix, const int length, 
                               const int which_ID,  int** ids, int* amount) 
{

   /* go through the matrix and count the amount of different core_ids
         or chip_ids */
   int i = 0, j = 0;
   int nr_different_ids= 0;
   int found = 0;

   /* only allow 0 (chip_id) or 1 (core_id) for which_ID */
   if (which_ID != 0 && which_ID != 1) {
      return false;
   }   

   /* here the already seen IDs are stored */
   (*ids) = (int *) calloc(length, sizeof(int));

   for (i = 0; i < length; i++) {
      found = 0;

      /* check if we have this ID already once */
      for (j = 0; j < nr_different_ids; j++) {
         if ((*ids)[j] == (matrix[i])[which_ID]) {
            /* we have the chip_id already */
            found = 1;
            break;
         }   
      }
      /* add ID if necessary */
      if (found == 0) {
         /* we have never seen this chip_id: save it */
         (*ids)[nr_different_ids] = (matrix[i])[which_ID];
         /* ... and count it */
         nr_different_ids++;
      }
   }
   
   *amount = nr_different_ids;

   return true;
}

/****** sge_binding/get_amount_of_threads_from_matrix() ****************************
*  NAME
*     get_amount_of_threads_from_matrix() -- ??? 
*
*  SYNOPSIS
*     int get_amount_of_threads_from_matrix(const int** matrix, const int 
*     length, int** threads, int* size) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const int** matrix - ??? 
*     const int length   - ??? 
*     int** threads      - ??? 
*     int* size          - ??? 
*
*  RESULT
*     int - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: get_amount_of_threads_from_matrix() is not MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int get_amount_of_threads_from_matrix(const int** matrix, const int length, 
   int** threads, int* size) 
{
   return get_amount_of_core_or_threads_from_matrix(matrix, length, 0, 
               threads, size);
}


/****** sge_binding/get_amount_of_core_or_threads_from_matrix() ********************
*  NAME
*     get_amount_of_core_or_threads_from_matrix() -- gets the amounf of cores or threads  
*
*  SYNOPSIS
*     int get_amount_of_core_or_threads_from_matrix(const int** matrix, const 
*     int length, int core, int** core_or_threads, int* size) 
*
*  FUNCTION
*     Gets the amount of cores per socket out of the topology matrix when 
*     'core' input value is 1. Otherwise (when 'core' input value is 0) 
*     it stores the amount of threads per core in the 'core_or_threads' array.
*     Hence the size of the output array is either the amount of sockets or 
*     the amount of cores. 
*
*  INPUTS
*     const int** matrix    - the topology matrix 
*     const int length      - the size of the topology matrix 
*     int core              - report amount of cores per socket when 1 otherwise threads per core 
* 
*  OUTPUTS
*     int** core_or_threads - the array containing the amount of cores or threads
*     int* size             - the size of the core_or_threads array 
*
*  RESULT
*     int - 
*
*  NOTES
*     MT-NOTE: get_amount_of_core_or_threads_from_matrix() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int get_amount_of_core_or_threads_from_matrix(const int** matrix, const int length, 
   int core, int** core_or_threads, int* size)
{
   /* if core=1 then get the cores otherwise get the threads */
   int i = 0, j = 0;
   
   /* get the different (internal) chip_ids if necessary */ 
   /* get the different (internal) core_ids if necessary */
   int* ids = NULL;
   /* the amount of different chip_ids or core_ids in the matrix 
      (depending if amount of cores or amount of threads are searched) */
   int ids_length = 0;
   /* the amount of different core_ids per chip_id or amount of same core_ids*/
   int amount = 0;
   
   /* check if output parameter is NULL pointer and if core is 1 or 0 */
   if ((*core_or_threads) != NULL || core > 1 || core < 0) {
      *size = 0;
      return -1;
   }

   /* get chip_ids or core_ids depending if we want count cores or threads */
   if (core == 1) {
      /* get all sockets via the chip_ids */
      get_chip_ids_from_matrix(matrix, length, &ids, &ids_length);
   } else {
      get_core_ids_from_matrix(matrix, length, &ids, &ids_length);
   }

   /* check if we got at least one chip_id (at least one socket) */
   if (ids_length == 0) {
      *size = 0;
      FREE(ids);
      return -2;
   }

   /* allocate the vector which has to be filled with the amount of cores per socket 
      or with the amount of threads per core */
   *core_or_threads = (int *) calloc(ids_length, sizeof(int));
   /* set the amount of elements which are in the output vector */
   *size = ids_length;

   /* count cores per socket or threads per core */
   for (; i < ids_length; i++) {
      /* count the first one */
      amount = 1;

      /* reset the ID counter function */
      is_new_id(-1);
      is_new_id_pair(-1, -1);

      /* go through the matrix */
      for (j = 0; j < length; j++) {
         if (core == 1) {
            /* counting cores per socket (amount of *same* chip_ids with different core_ids) */
            if ((matrix[j])[0] == ids[i]) {

               /* store pair of chip_id and core id and check if it is already known */
               int is_different = is_new_id_pair((matrix[j])[0], (matrix[j])[1]);

               /* count only *same* chip_ids */
               if (is_new_id((matrix[j])[0]) == 0) {
                  /* this combination of chip_id and core_id is new */
                  if (is_different == 1) {
                     amount++;
                  }
               } 
            }
         } else {
            /* counting threads per core (amount of *same* core_ids) */
            if ((matrix[j])[1] == ids[i]) { 
               /* count only *same* core_ids together with chip_ids */
               if (is_new_id_pair((matrix[j])[0], (matrix[j])[1]) == 0) {
                  amount++;
               }   
            }
         }
      }
      /* save the amount of counted cores or threads in the vector */
      (*core_or_threads)[i] = amount;
   }
   
   /* free in subfunction allocated memory */
   FREE(ids);
   
   /* reset the ID counter function */
   is_new_id(-1);
   is_new_id_pair(-1, -1);

   /* return the amount of sockets or cores we have found (size of array)*/
   return ids_length;
}

/****** sge_binding/get_amount_of_cores_from_matrix() ******************************
*  NAME
*     get_amount_of_cores_from_matrix() -- Get the amount of cores per socket. 
*
*  SYNOPSIS
*     int get_amount_of_cores_from_matrix(const int** matrix, const int length, 
*     int** cores, int* size) 
*
*  FUNCTION
*     Counts the amount of cores for each socket found in the matrix. 
*     The output vector contains for each socket the number of core it has. 
*
*  INPUTS
*     const int** matrix - matrix with chip_id and core_id 
*     const int length   - amount of chip_id and core_id entries the matrix has
*     int** cores        - output: for each socket the amount of cores are 
*                                  printed 
*     int* size          - output: the length of the cores vector  
*
*  RESULT
*     int - The length of the cores vector or when negative the presence of 
*           an error.
*
*  NOTES
*     MT-NOTE: get_amount_of_cores_from_matrix() is not MT safe (because of 
*              counting) 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int get_amount_of_cores_from_matrix(const int** matrix, const int length, 
   int** cores, int* size) 
{
   return get_amount_of_core_or_threads_from_matrix(matrix, length, 1, 
             cores, size);
}


/****** sge_binding/is_new_id() **********************************************
*  NAME
*     is_new_id() -- ??? 
*
*  SYNOPSIS
*     int is_new_id(const int id) 
*
*  FUNCTION
*     Checks if an ID is unique or not. For that it stores all 
*     IDs from previous calls in an array. If the ID is found there 
*     the function returns 0 otherwise it will store the ID in the 
*     array for the next calls an returns 1. When calling the function 
*     exactly 2 times with the same ID then the first time it returns 1
*     and the second time 0. Only IDs >= 0 are allowed. 
*     
*     The last call of the function must be with an ID < 0 in order 
*     to delete all stored IDs. 
*
*  INPUTS
*     const int id - Unique positive integer as identifier.  
*
*  RESULT
*     int - 1 in case the parameter was a new ID otherwise 0.
*
*  NOTES
*     MT-NOTE: is_new_id() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int is_new_id(const int id) 
{

   /* if ID is not available, add it otherwise return 1 */
   /* different_ids, different_id_vector are static */
   /* if id < 0 : delete all ids collected so far */
   const int MAX_ID_SIZE = 11;
   static int different_ids = 0;
   static int* different_id_vector = NULL;
   /* counter */
   int i = 0;
   /* do we have the id already? */
   int found = 0;

   if (id < 0) {
      /* reset everything */
      different_ids = 0;
      FREE(different_id_vector);
      return 1;
   } 

   /* allocate memory for the ids if necessary */
   if (different_id_vector == NULL || (different_ids % (MAX_ID_SIZE-1) == 0)) {
      /* allocate a chunk of memory for the new ids */
      different_id_vector = (int *) realloc(different_id_vector, 
         (different_ids + MAX_ID_SIZE) * sizeof(int));
   }
   
   /* search the ID vector for the id */
   for (i = 0; i < different_ids; i++) {
      if (different_id_vector[i] == id) {
         found = 1;
         break;
      }   
   }

   /* when the id is new, add it */
   if (found == 0) {
      different_id_vector[different_ids] = id;
      different_ids++;
      return 1;
   } else {
      return 0;
   }
}

static int is_new_id_pair(const int id, const int id2)
{
   /* if ID is not available, add it otherwise return 1 */
   /* different_ids, different_id_vector are static */
   /* if id < 0 : delete all ids collected so far */
   const  int MAX_ID_SIZE            = 10;
   static int different_ids          = 0;
   static int* different_id_vector_1 = NULL;
   static int* different_id_vector_2 = NULL;

   /* counter */
   int i = 0;
   /* do we have the id already? */
   int found = 0;

   if (id < 0) {
      /* reset everything */
      different_ids = 0;
      if (different_id_vector_1 != NULL) {
         free(different_id_vector_1);
         different_id_vector_1 = NULL;
      }
      if (different_id_vector_2 != NULL) {
         free(different_id_vector_2);
         different_id_vector_2 = NULL;
      }
      return 1;
   } 

   /* allocate memory for the ids if necessary */
   if (different_id_vector_1 == NULL || different_id_vector_2 == NULL 
         || (different_ids % (MAX_ID_SIZE-1) == 0)) {
      /* allocate a chunk of memory for the new ids */
      different_id_vector_1 = (int *) realloc(different_id_vector_1,
         (different_ids + MAX_ID_SIZE) * sizeof(int));
      different_id_vector_2 = (int *) realloc(different_id_vector_2,
         (different_ids + MAX_ID_SIZE) * sizeof(int));
   }
   
   /* search the ID vector for the id */
   for (i = 0; i < different_ids; i++) {
      if (different_id_vector_1[i] == id && different_id_vector_2[i] == id2) {
         found = 1;
         break;
      }
   }

   /* when the id is new, add it */
   if (found == 0) {
      different_id_vector_1[different_ids] = id;
      different_id_vector_2[different_ids] = id2;
      different_ids++;
      return 1;
   } else {
      return 0;
  } 
}


/* access functions */ 

/****** sge_binding/get_total_amount_of_cores_solaris() ************************
*  NAME
*     get_total_amount_of_cores_solaris() -- ??? 
*
*  SYNOPSIS
*     static int get_total_amount_of_cores_solaris() 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*
*  RESULT
*     static int - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: get_total_amount_of_cores_solaris() is not MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int get_total_amount_of_cores_solaris()
{
   /* pointer to the topology matrix */
   int** matrix = NULL;
   /* length of the matrix */
   int length = 0;
   /* total amount of cores found */
   int cores_total = 0;

   /* get the topology matrix out of kstat */
   if (generate_chipID_coreID_matrix(&matrix, &length)) {
      int i = 0;
      int nr_cores = 0;
      int* cores = NULL;

      get_amount_of_cores_from_matrix((const int**)matrix, length, &cores, &nr_cores);
      
      /* sum up the amount of cores for all sockets */
      for (; i < nr_cores; i++) {
         cores_total += cores[i];
      } 
      
      /* delete vector and matrix */
      FREE(cores);
      free_matrix(matrix, length);
   }

   if (cores_total <= 0) {
      /* default case: we have one core */
      cores_total = 1;
   }

   return cores_total;
}

/****** sge_binding/get_total_amount_of_sockets_solaris() **********************
*  NAME
*     get_total_amount_of_sockets_solaris() -- ??? 
*
*  SYNOPSIS
*     static int get_total_amount_of_sockets_solaris() 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*
*  RESULT
*     static int - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: get_total_amount_of_sockets_solaris() is not MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int get_total_amount_of_sockets_solaris()
{
   /* pointer to the topology matrix */
   int** matrix = NULL;
   /* length of the matrix */
   int length = 0;
   /* total amount of sockets found */
   int sockets_total = 0;

   /* get the topology matrix out of kstat */
   if (generate_chipID_coreID_matrix(&matrix, &length)) {
      int i = 0;
      int* cores = NULL;

      for (i = 0; i < length; i++)
         if (matrix[i] == NULL)
           break;

      get_amount_of_cores_from_matrix((const int**)matrix, length, &cores, &sockets_total);
      
      /* delete vector and matrix */
      FREE(cores);
      free_matrix(matrix, length);
   }

   if (sockets_total <= 0) {
      /* default case: we have one socket */
      sockets_total = 1;
   }

   return sockets_total;
}


/****** sge_binding/get_processor_ids_solaris() ********************************
*  NAME
*     get_processor_ids_solaris() -- Returns the OS internal processor ids for a core. 
*
*  SYNOPSIS
*     static bool get_processor_ids_solaris(const int** matrix, const int 
*     length, const int logical_socket_number, const int logical_core_number, 
*     int** pr_ids, int* pr_length) 
*
*  FUNCTION
*     Returns the operating system internal processor ids for a specific core on 
*     a specific socket. The socket and core numbers are logical, that means that 
*     they start at 0 and have no holes.
*
*     In case of hyperthreading or core multi threading it will return an 
*     array with more than one element. Each OS internal processor id which 
*     does run on the specific core is reported. 
*
*  INPUTS
*     const int** matrix              - Topology matrix (from internal kstat) 
*     const int length                - Size of topology matrix. 
*     const int logical_socket_number - Logical socket number on which the core is. 
*     const int logical_core_number   - Logical core number.
*
*  OUTPUTS
*     int** pr_ids                    - Processor ids which are representing the core. 
*     int* pr_length                  - The amount of processor ids. 
*
*  RESULT
*     static bool - true when no problems occurs.
*
*  NOTES
*     MT-NOTE: get_processor_ids_solaris() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool get_processor_ids_solaris(const int** matrix, const int length, const int logical_socket_number,
      const int logical_core_number, int** pr_ids, int* pr_length)
{
   /* the collected core ids */ 
   int core_ids[length];
   /* the actual amount of found core ids */
   int amount_of_core_ids = 0;

   /* real chip_id and real core_id (mapped from the logical ones) */
   int chip_id, core_id;
   /* counter */
   int i;

   if (matrix == NULL || length == 0 || pr_ids == NULL || (*pr_ids) != NULL || pr_length == NULL) {
      return false;
   }

   /* map the logical numbers (0...n) to the system internal numbers (with holes etc.) */
   chip_id = get_chip_id_from_logical_socket_number_solaris(matrix, length, logical_socket_number);
   core_id = get_core_id_from_logical_core_number_solaris(matrix, length, chip_id, logical_core_number);

   /* get all processor ids with the same chip_ids */ 
   for (i = 0; i < length; i++) {
      /* matrix is: chip_id core_id processor_id */
      if ((matrix[i])[0] == chip_id && (matrix[i])[1] == core_id) {
         /* the third entry should be the processor id */
         core_ids[amount_of_core_ids] = (matrix[i])[2];
         amount_of_core_ids++;
      }
   }
 
   if (amount_of_core_ids <= 0) {
      return false;
   } else {  
      
      /* return the array with core ids */
      (*pr_ids) = (int*) calloc(amount_of_core_ids, sizeof(int));
      for (i = 0; i < amount_of_core_ids; i++) {
         (*pr_ids)[i] = core_ids[i];
      }
      *pr_length = amount_of_core_ids;
      
      return true;
   }   
}


/****** sge_binding/get_chip_id_from_logical_socket_number_solaris() ***********
*  NAME
*     get_chip_id_from_logical_socket_number_solaris() -- Get internal chip_id. 
*
*  SYNOPSIS
*     static int get_chip_id_from_logical_socket_number_solaris(const int** 
*     matrix, const int length, const int logical_socket_number) 
*
*  FUNCTION
*     Searches the Solaris internal chip_id for a given logical socket number. 
*     A logical socket number is a number between 0 and n-1 where n is the 
*     the total amount of sockets on the node. The chip_id may not start with 
*     0 on a system and may have wholes.
*
*  INPUTS
*     const int** matrix              - The topology matrix. 
*     const int length                - The size of the matrix. 
*     const int logical_socket_number - Logical socket number on host. 
*
*  RESULT
*     static int - Solaris internal chip_id which represents the logical 
*                  socket number. 
*
*  NOTES
*     MT-NOTE: get_chip_id_from_logical_socket_number_solaris() is not MT safe 
*              (because of is_new_id)
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int get_chip_id_from_logical_socket_number_solaris(const int** matrix, 
   const int length, const int logical_socket_number) 
{
   /* maps the logical socket number to the Solaris internal chip_id number: */
   /* take the n'th chip_id from the matrix (where n is the logical_socket_number) */ 
   int i = 0;
   /* amount of different sockets/chip_ids found yet */
   int socket_number = 0;
   
   /* be sure that no ids are left */
   is_new_id(-1);

   /* go through the resource matrix and get the 'logical_socket_number'th chip_id */
   for (i = 0; i < length; i++) {
      if (is_new_id((matrix[i])[0]) == 1) {
         /* detected a chip_id not seen before */
         socket_number++;
      }
      if ((socket_number-1) == logical_socket_number) {
         /* free memory allocated in sub-function */
         is_new_id(-1);
         /* return the chip_id */
         return (matrix[i])[0];
      }
   }
  
   /* free memory */
   is_new_id(-1);

   /* logical socket number was too high */
   return -1;
}

/****** sge_binding/get_core_id_from_logical_core_number_solaris() *************
*  NAME
*     get_core_id_from_logical_core_number_solaris() -- ??? 
*
*  SYNOPSIS
*     static int get_core_id_from_logical_core_number_solaris(const int** 
*     matrix, const int length, const int chip_id, const int 
*     logical_core_number) 
*
*  FUNCTION
*     Searches the Solaris internal core_id from a given chip_id (internal 
*     socket number) and a logical core number. The logical core number 
*     on a chip is a number between 0 and n-1 where n is the amount of 
*     cores the chip have. It is different from the internal core_id which 
*     does not neccessarly start at 0 and may not be continuous.
*
*  INPUTS
*     const int** matrix            - topology matrix 
*     const int length              - size of topology matrix 
*     const int chip_id             - internal chip_id to search on 
*     const int logical_core_number - logical core number (starting at 0) 
*                                      
*
*  RESULT
*     static int - The internal core_id representation. 
*
*  NOTES
*     MT-NOTE: get_core_id_from_logical_core_number_solaris() is not MT safe 
*              (because of is_new_id())
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int get_core_id_from_logical_core_number_solaris(const int** matrix, 
   const int length, const int chip_id, const int logical_core_number)
{
   
   /* maps the internal chip_id and the logical core number to the Solaris 
      internal core_id */
   int i = 0;
   int core_number = 0;

   if (matrix == NULL || *matrix == NULL) {
      /* this is not a matrix */ 
      return -1;
   }

   if (length == 0 || chip_id < 0 || logical_core_number < 0) {
      /* input parameters are not correct */
      return -1;
   }
   
   /* be sure that no ids are left */
   is_new_id(-1);

   for (i = 0; i < length; i++) {
      /* check if this entry is on the same chip */
      if ((matrix[i])[0] == chip_id) {

         /* check how many different core_ids we found so far */
         if (is_new_id((matrix[i])[1]) == 1) {
            core_number++;
         }
         if ((core_number-1) == logical_core_number) {
            /* free allocated memory in sub-function */
            is_new_id(-1);
            /* report the core_id */
            return (matrix[i])[1];
         }
      }
   }

   /* free memory */
   is_new_id(-1);

   /* logical core number was too high or chip_id was wrong */
   return -1;
}



#endif 

/* ---------------------------------------------------------------------------*/
/* ---------------------------------------------------------------------------*/
/*                  Ending of SOLARIS related functions                       */
/* ---------------------------------------------------------------------------*/
/* ---------------------------------------------------------------------------*/


/* ---------------------------------------------------------------------------*/
/*                   Bookkeeping of cores in use by SGE                       */ 
/* ---------------------------------------------------------------------------*/
#if defined(PLPA_LINUX) || defined(SOLARIS86) || defined(SOLARISAMD64)

bool get_linear_automatic_socket_core_list_and_account(const int amount, 
      int** list_of_sockets, int* samount, int** list_of_cores, int* camount, 
      char** topo_by_job, int* topo_by_job_length)
{
   /* return value: if it is possible to fit the request on the host  */
   bool possible       = true;   
   
   /* temp topology string where accounting is done on     */
   char* tmp_topo_busy = NULL;

   /* amount of cores we could account already             */
   int used_cores      = 0;

   /* the numbers of the sockets which are completely free */
   int* sockets        = NULL;
   int sockets_size    = 0;

   /* tmp counter */
   int i;

   /* get the topology which could be used by the job */
   tmp_topo_busy = (char *) calloc(logical_used_topology_length, sizeof(char));
   memcpy(tmp_topo_busy, logical_used_topology, logical_used_topology_length*sizeof(char));

   /* 1. Find all free sockets and try to fit the request on them     */
   if (get_free_sockets(tmp_topo_busy, logical_used_topology_length, &sockets, 
         &sockets_size) == true) {
      
      /* there are free sockets: use them */
      for (i = 0; i < sockets_size && used_cores < amount; i++) {
         int needed_cores = amount - used_cores;
         used_cores += account_cores_on_socket(&tmp_topo_busy, logical_used_topology_length, 
                           sockets[i], needed_cores, list_of_sockets, samount, 
                           list_of_cores, camount);
      }

      FREE(sockets);
   }

   /* 2. If not all cores fit there - fill up the rest of the sockets */
   if (used_cores < amount) {
      
      /* the socket which offers some cores */
      int socket_free = 0;
      /* the amount of cores we still need */
      int needed_cores = amount - used_cores;

      while (needed_cores > 0) {
         /* get the socket with the most free cores */
         if (get_socket_with_most_free_cores(tmp_topo_busy, logical_used_topology_length,
               &socket_free) == true) {
            
            int accounted_cores = account_cores_on_socket(&tmp_topo_busy, 
                                    logical_used_topology_length, socket_free, 
                                    needed_cores, list_of_sockets, samount, 
                                    list_of_cores, camount);

            if (accounted_cores < 1) {
               /* there must be a bug in one of the last two functions! */
               possible = false;
               break;
            }

            needed_cores -= accounted_cores;
            
          } else {
            /* we don't have free cores anymore */
            possible = false;
            break;
          }
       }   

   }

   if (possible == true) {
      /* calculate the topology used by the job out of */ 
      create_topology_used_per_job(topo_by_job, topo_by_job_length, 
         logical_used_topology, tmp_topo_busy, logical_used_topology_length);

      /* make the temporary accounting permanent */
      memcpy(logical_used_topology, tmp_topo_busy, logical_used_topology_length*sizeof(char));
   } 
     
   FREE(tmp_topo_busy);

   return possible;
}

static bool get_socket_with_most_free_cores(const char* topology, const int topology_length, 
               int* socket_number) 
{
   /* get the socket which offers most free cores */
   int highest_amount_of_cores = 0;
   *socket_number              = 0;
   int current_socket          = -1;
   int i;
   /* number of unbound cores on the current socket */
   int current_free_cores      = 0;

   /* go through the topology, remember the socket with the highest amount 
      of free cores so far and update it when it is neccessary */
   for (i = 0; i < topology_length && topology[i] != '\0'; i++) {
      
      if (topology[i] == 'S' || topology[i] == 's') {
         /* we are on a new socket */
         current_socket++;
         /* reset core counter */
         current_free_cores = 0;
      } else if (topology[i] == 'C') {
         current_free_cores++;
         
         /* remember if the socket offers more free cores */
         if (current_free_cores > highest_amount_of_cores) {
            highest_amount_of_cores = current_free_cores;
            *socket_number          = current_socket;
         }

      }

   }

   if (highest_amount_of_cores <= 0) {
      /* there is no core free */
      return false;
   } else {
      /* we've found the socket which offers most free cores (socket_number) */
      return true;
   }
}

static bool account_all_threads_after_core(char** topology, const int core_pos)
{
   /* we need the position after the C in the topology string (example: "SCTTSCTT"
      or "SCCSCC") */
   int next_pos = core_pos + 1;

   /* check correctness of input values */
   if (topology == NULL || (*topology) == NULL || core_pos < 0 || strlen(*topology) <= core_pos) {
      return false;
   }
   
   /* check if we are at the last core of the string without T's at the end */
   if (next_pos >= strlen(*topology)) {
      /* there is no thread on the last core to account: thats a success anyway */
      return true;
   } else {
      /* set all T's at the current position */
      while ((*topology)[next_pos] == 'T' || (*topology)[next_pos] == 't') {
         /* account the thread */
         (*topology)[next_pos] = 't';
         next_pos++;
      } 
   }

   return true;
}


static int account_cores_on_socket(char** topology, const int topology_length,
               const int socket_number, const int cores_needed, int** list_of_sockets,
               int* list_of_sockets_size, int** list_of_cores, int* list_of_cores_size)
{
   int i;
   /* socket number we are at the moment */
   int current_socket_number = -1;
   /* return value */
   int retval;

   /* try to use as many cores as possible on a specific socket 
      but not more */
   
   /* jump to the specific socket given by the "socket_number" */
   for (i = 0; i < topology_length && (*topology)[i] != '\0'; i++) {
      if ((*topology)[i] == 'S' || (*topology)[i] == 's') {
         current_socket_number++;
         if (current_socket_number >= socket_number) {
            /* we are at the beginning of socket #"socket_number" */
            break;
         }   
      }
   }

   /* check if we reached that socket or if it was out of range */
   if (socket_number != current_socket_number) {

      /* early abort because we couldn't find the socket we were 
         searching for */ 
      retval = 0;

   } else {
      
      /* we are at a 'S' or 's' and going to the next 'S' or 's' 
         and collecting all cores in between */
      
      int core_counter = 0;   /* current core number on the socket */
      i++;                    /* just forward to the first core on the socket */  
      retval  = 0;            /* need to initialize the amount of cores we found */

      for (; i < topology_length && (*topology)[i] != '\0'; i++) {
         if ((*topology)[i] == 'C') {
            /* take this core */
            (*list_of_sockets_size)++;    /* the socket list is growing */
            (*list_of_cores_size)++;      /* the core list is growing */
            *list_of_sockets = (int *) realloc(*list_of_sockets, (*list_of_sockets_size) 
                                          * sizeof(int));
            *list_of_cores   = (int *) realloc(*list_of_cores, (*list_of_cores_size)  
                                          * sizeof(int));
            /* store the logical <socket,core> tuple inside the lists */
            (*list_of_sockets)[(*list_of_sockets_size) - 1]   = socket_number;
            (*list_of_cores)[(*list_of_cores_size) - 1]       = core_counter;
            /* increase the amount of cores we've collected so far */
            retval++;
            /* move forward to the next core */
            core_counter++;
            /* do accounting */
            (*topology)[i] = 'c';
            /* thread binding: accounting is done here */
            account_all_threads_after_core(topology, i);

         } else if ((*topology)[i] == 'c') {
            /* this core is already in use */
            /* move forward to the next core */
            core_counter++;
         } else if ((*topology)[i] == 'S' || (*topology)[i] == 's') {
            /* we are already on another socket which we can not use */
            break;
         }

         if (retval >= cores_needed) {
            /* we have already collected as many cores we need to collect */
            break;
         }
      }
      
   }

   return retval;
}


static bool get_free_sockets(const char* topology, const int topology_length, 
               int** sockets, int* sockets_size)
{
   /* temporary counter */
   int i, j;
   /* this amount of sockets we discovered already */ 
   int socket_number  = 0;

   (*sockets) = NULL;
   (*sockets_size) = 0;

   /* go through the whole topology and check if there are some sockets
      completely unbound */
   for (i = 0; i < topology_length && topology[i] != '\0'; i++) {

      if (topology[i] == 'S' || topology[i] == 's') {

         /* we're on a new socket: check all cores (and skip threads) after it */
         bool free = true;

         /* check the topology till the next socket (or end) */
         for (j = i + 1; j < topology_length && topology[j] != '\0'; j++) {
            if (topology[j] == 'c') {
               /* this socket has at least one core in use */
               free = false;
            } else if (topology[j] == 'S' || topology[j] == 's') {
               break;
            }
         }

         /* fast forward */
         i = j;

         /* check if this socket had a core in use */ 
         if (free == true) {
            /* this socket can be used completely */ 
            (*sockets) = (int *) realloc(*sockets, ((*sockets_size)+1)*sizeof(int));
            (*sockets)[(*sockets_size)] = socket_number;
            (*sockets_size)++;
         }
         
         /* increment the amount of sockets we discovered so far */
         socket_number++;

      } /* end if this is a socket */
      
   }

   /* it was successful when we found at least one socket not used by any job */
   if ((*sockets_size) > 0) {
      /* we also have to free the list outside afterwards */
      return true;
   } else {
      return false;
   }
}



/****** sge_binding/get_striding_first_socket_first_core_and_account() ********
*  NAME
*     get_striding_first_socket_first_core_and_account() -- Checks if and where 
*                                                           striding would fit.
*
*  SYNOPSIS
*     bool getStridingFirstSocketFirstCore(const int amount, const int 
*     stepsize, int* first_socket, int* first_core) 
*
*  FUNCTION
*     This operating system independent function checks (depending on 
*     the underlaying topology string and the topology string which 
*     reflects already execution units in use) if it is possible to 
*     bind the job in a striding manner to cores on the host. 
*     
*     This function requires the topology string and the string with the 
*     topology currently in use. 
*
*  INPUTS
*     const int amount    - Amount of cores to allocate. 
*     const int stepsize  - Distance of the cores to allocate.
*     const int start_at_socket - First socket to begin the search with (usually at 0).
*     const int start_at_core   - First core to begin the search with (usually at 0). 
*     int* first_socket   - out: First socket when striding is possible (return value).
*     int* first_core     - out: First core when striding is possible (return value).
*
*  RESULT
*     bool - if true striding is possible at <first_socket, first_core> 
*
*  NOTES
*     MT-NOTE: getStridingFirstSocketFirstCore() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool get_striding_first_socket_first_core_and_account(const int amount, const int stepsize,
   const int start_at_socket, const int start_at_core, const bool automatic,  
   int* first_socket, int* first_core, char** accounted_topology, 
   int* accounted_topology_length) 
{
   /* return value: if it is possible to fit the request on the host */
   bool possible   = false;   
   
   /* position in topology string */ 
   int i = 0;

   /* socket and core counter in order to find the first core and socket */
   int sc = -1; 
   int cc = -1;
   
   /* these core and socket counters are added later on .. */
   int found_cores   = 0;
   int found_sockets = 0; /* first socket is given implicitely */
   
   /* temp topology string where accounting is done on */
   char* tmp_topo_busy;

   /* initialize socket and core where the striding will fit */
   *first_socket   = 0;
   *first_core     = 0;

   if (start_at_socket < 0 || start_at_core < 0) {
      /* wrong input parameter */
      return false;
   }

   if (logical_used_topology == NULL) {
      /* we have no topology string at the moment (should be initialized before) */
      if (!get_execd_topology(&logical_used_topology, &logical_used_topology_length)) {
         /* couldn't even get the topology string */
         return false;
      }
   }
   /* temporary accounting string -> account on this and 
      when eventually successful then copy this string back 
      to global topo_busy string */
   tmp_topo_busy = (char *) calloc(logical_used_topology_length + 1, sizeof(char));
   memcpy(tmp_topo_busy, logical_used_topology, logical_used_topology_length*sizeof(char));

   /* we have to go to the first position given by the arguments 
      (start_at_socket and start_at_core) */
   for (i = 0; i < logical_used_topology_length; i++) {

      if (logical_used_topology[i] == 'C' || logical_used_topology[i] == 'c') {
         /* found core   -> update core counter   */
         cc++;
      } else if (logical_used_topology[i] == 'S' || logical_used_topology[i] == 's') {
         /* found socket -> update socket counter */
         sc++;
         /* we're changing socket -> no core found on this one yet */
         cc = -1;
      } else if (logical_used_topology[i] == '\0') {
         /* we couldn't find start socket start string */
         possible = false;
         free(tmp_topo_busy);
         return possible;
      }
      
      if (sc == start_at_socket && cc == start_at_core) {
         /* we found our starting point (we remember 'i' for next loop!) */
         break;
      }
   }
   
   /* check if we found the socket and core we want to start searching */
   if (sc != start_at_socket || cc != start_at_core) {
      /* could't find the start socket and start core */
      free(tmp_topo_busy);
      return false;
   }

   /* check each position of the topology string */
   /* we reuse 'i' from last loop -> this is the position where we begin */
   for (; i < logical_used_topology_length && logical_used_topology[i] != '\0'; i++) {
      
      /* this could be optimized (with increasing i in case if it is not
         possible) */  
      if (is_starting_point(logical_used_topology, logical_used_topology_length, i, amount, stepsize, 
            &tmp_topo_busy)) {
         /* we can do striding with this as starting point */
         possible = true;
         /* update place where we can begin */
         *first_socket = start_at_socket + found_sockets;
         *first_core   = start_at_core + found_cores;
         /* return the accounted topology */ 
         create_topology_used_per_job(accounted_topology, accounted_topology_length, 
            logical_used_topology, tmp_topo_busy, logical_used_topology_length);
         /* finally do execution host wide accounting */
         /* DG TODO mutex */ 
         memcpy(logical_used_topology, tmp_topo_busy, logical_used_topology_length*sizeof(char));

         break;
      } else { 

         /* else retry and update socket and core number to start with */

         if (logical_used_topology[i] == 'C' || logical_used_topology[i] == 'c') {
            /* jumping over a core */
            found_cores++;
            /* a core is a valid starting point for binding in non-automatic case */ 
            /* if we have a fixed start socket and a start core we do not retry 
               it with the next core available (when introducing T's this have to 
               be added there too) */
            if (automatic == false) {
               possible = false;
               break;
            }

         } else if (logical_used_topology[i] == 'S' || logical_used_topology[i] == 's') {
            /* jumping over a socket */
            found_sockets++;
            /* we are at core 0 on the new socket */
            found_cores = 0;
         }
         /* at the moment we are not interested in threads or anything else */
         
      }
   
   } /* end go through the whole topology string */
   
   free(tmp_topo_busy);
   return possible;
}


static bool create_topology_used_per_job(char** accounted_topology, int* accounted_topology_length, 
            char* logical_used_topology, char* used_topo_with_job, int logical_used_topology_length)
{        
   /* tmp counter */
   int i;

   /* length of output string remains the same */
   (*accounted_topology_length) = logical_used_topology_length;
   
   /* copy string of current topology in use */
   (*accounted_topology) = calloc(logical_used_topology_length+1, sizeof(char));
   if ((*accounted_topology) == NULL) {
      /* out of memory */
      return false;
   }

   memcpy((*accounted_topology), logical_used_topology, sizeof(char)*logical_used_topology_length);
   
   /* revert all accounting from other jobs */ 
   for (i = 0; i < logical_used_topology_length; i++) {
      if ((*accounted_topology)[i] == 'c') {
         (*accounted_topology)[i] = 'C';
      } else if ((*accounted_topology)[i] == 's') {
         (*accounted_topology)[i] = 'S';
      } else if ((*accounted_topology)[i] == 't') {
         (*accounted_topology)[i] = 'T';
      }
   }

   /* account all the resources the job consumes: these are all occupied 
      resources in used_topo_with_job String that are not occupied in 
      logical_used_topology String */
   for (i = 0; i < logical_used_topology_length; i++) {

      if (used_topo_with_job[i] == 'c' && logical_used_topology[i] == 'C') {
         /* this resource is from job exclusively used */
         (*accounted_topology)[i] = 'c';
      }

      if (used_topo_with_job[i] == 't' && logical_used_topology[i] == 'T') {
         /* this resource is from job exclusively used */
         (*accounted_topology)[i] = 't';
      }

      if (used_topo_with_job[i] == 's' && logical_used_topology[i] == 'S') {
         /* this resource is from job exclusively used */
         (*accounted_topology)[i] = 's';
      }
      
   }

   return true;
}

/****** sge_binding/is_starting_point() ****************************************
*  NAME
*     is_starting_point() -- Checks if 'pos' is a valid first core for striding.
*
*  SYNOPSIS
*     bool is_starting_point(const char* topo, const int length, const int pos,
*     const int amount, const int stepsize) 
*
*  FUNCTION
*     Checks if 'pos' is a starting point for binding the 'amount' of cores
*     in a striding manner on the host. The topo string contains 'C's for unused
*     cores and 'c's for cores in use.
*
*  INPUTS
*     const char* topo   - String representing the topology currently in use.
*     const int length   - Length of topology string.
*     const int pos      - Position within the topology string.
*     const int amount   - Amount of cores to bind to.
*     const int stepsize - Step size when binding in a striding manner.
*
*  OUTPUTS
*     char* topo_account - Here the accounting is done on.
*
*  RESULT
*     bool - true if striding with the given parameters is possible.
*
*  NOTES
*     MT-NOTE: is_starting_point() is not MT safe
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool is_starting_point(const char* topo, const int length, const int pos, 
   const int amount, const int stepsize, char** topo_account) {
   
   /* go through the topology (in use) string with the beginning at pos 
      and try to fit all cores in there */ 
   int i;   
   /* core counter in order to fulfill the stepsize property */
   int found_cores = 1;
   /* so many cores we have collected so far */
   int accounted_cores = 0;
   /* return value */ 
   bool is_possible = false;

   /* stepsize must be 1 or greater */
   if (stepsize < 1) {
      return false;
   }
   /* position in string must be smaller than string length */
   if (pos >= length) {
      return false;
   }
   /* topology string must not be NULL */
   if (topo == NULL) {
      return false;
   }
   /* amount must be 1 or greater */
   if (amount < 1) {
      return false;
   }

   /* fist check if this is a valid core */ 
   if (topo[pos] != 'C' || topo[pos] == '\0') {
      /* not possible this is not a valid free core (could be a socket,
         thread, or core in use) */
      return false;
   }

   /* we count this core */ 
   accounted_cores++;
   /* this core is used */
   (*topo_account)[pos] = 'c';
   /* thread binding: account following threads */
   account_all_threads_after_core(topo_account, pos); 

   if (accounted_cores == amount) {
      /* we have all cores and we are still within the string */
      is_possible = true;
      return is_possible;
   }

   /* go to the remaining topology which is in use */ 
   for (i = pos + 1; i < length && topo[i] != '\0'; i++) {
   
      if (topo[i] == 'C') {
         /* we found an unused core */
         if (found_cores >= stepsize) {
            /* this core we need and it is free - good */
            found_cores = 1;
            /* increase the core counter */
            accounted_cores++;
            /* this core is used */
            (*topo_account)[i] = 'c';
            /* thread binding: bind following threads */
            account_all_threads_after_core(topo_account, i); 

         } else if (found_cores < stepsize) {
            /* this core we don't need */
            found_cores++;
         }
      } else if (topo[i] == 'c') {
         /* this is a core in use */
         if (found_cores >= stepsize) {
            /* this core we DO NEED but it is busy */
            return false;
         } else if (found_cores < stepsize) {
            /* this core we don't need */
            found_cores++;
         }
      } 
      
      /* accounted cores */ 
      if (accounted_cores == amount) {
         /* we have all cores and we are still within the string */
         is_possible = true;
         break;
      }
   }
   
   /* using this core as first core is possible */
   return is_possible;
}   

static int get_position_in_topology(const int socket, const int core, 
   const char* topology, const int topology_length)
{
   
   int i;
   /* position of <socket>,<core> in the topology string */
   int retval = -1;

   /* current position */
   int s = -1;
   int c = -1;
   int t = -1;

   if (topology_length <= 0 || socket < 0 || core < 0 || topology == NULL) {
      return false;
   }
   
   for (i = 0; i < topology_length && topology[i] != '\0'; i++) {
      if (topology[i] == 'S') {
         /* we've got a new socket */
         s++;
         /* invalidate core counter */
         c = -1;
      } else if (topology[i] == 'C') {
         /* we've got a new core */
         c++;
         /* invalidate thread counter */
         t = -1;
      } else if (topology[i] == 'T') {
         /* we've got a new thread */
         t++;
      }
      /* check if we are at the position seeking for */
      if (socket == s && core == c) {
         retval = i;
         break;
      }   
   }

   return retval;
}

bool initialize_topology() {
   
   /* this is done when execution daemon starts        */
   
   if (logical_used_topology == NULL) {
      if (get_execd_topology(&logical_used_topology, &logical_used_topology_length)) {
         return true;
      }
   }

   return false;
}

#endif

/* ---------------------------------------------------------------------------*/
/*               End of bookkeeping of cores in use by SGE                    */ 
/* ---------------------------------------------------------------------------*/

bool
binding_print_to_string(const lListElem *this_elem, dstring *string) {
   bool ret = true;

   DENTER(BINDING_LAYER, "binding_print_to_string");
   if (this_elem != NULL && string != NULL) {
      const char *const strategy = lGetString(this_elem, BN_strategy);   
      binding_type_t type = (binding_type_t)lGetUlong(this_elem, BN_type);

      switch (type) {
         case BINDING_TYPE_SET:
            sge_dstring_append(string, "set "); 
            break; 
         case BINDING_TYPE_PE:
            sge_dstring_append(string, "pe "); 
            break; 
         case BINDING_TYPE_ENV:
            sge_dstring_append(string, "env "); 
            break; 
         default:
            sge_dstring_append(string, "unknown "); 
      }

      if (strcmp(strategy, "linear_automatic") == 0) {
         sge_dstring_sprintf_append(string, "%s:"sge_U32CFormat, 
            "linear", sge_u32c(lGetUlong(this_elem, BN_parameter_n)));
      } else if (strcmp(strategy, "linear") == 0) {
         sge_dstring_sprintf_append(string, "%s:"sge_U32CFormat":"sge_U32CFormat","sge_U32CFormat, 
            "linear", sge_u32c(lGetUlong(this_elem, BN_parameter_n)), 
            sge_u32c(lGetUlong(this_elem, BN_parameter_socket_offset)),
            sge_u32c(lGetUlong(this_elem, BN_parameter_core_offset)));
      } else if (strcmp(strategy, "striding_automatic") == 0) {
         sge_dstring_sprintf_append(string, "%s:"sge_U32CFormat":"sge_U32CFormat, 
            "striding", sge_u32c(lGetUlong(this_elem, BN_parameter_n)),
            sge_u32c(lGetUlong(this_elem, BN_parameter_striding_step_size)));
      } else if (strcmp(strategy, "striding") == 0) {
         sge_dstring_sprintf_append(string, "%s:"sge_U32CFormat":"sge_U32CFormat":"sge_U32CFormat","sge_U32CFormat, 
            "striding", sge_u32c(lGetUlong(this_elem, BN_parameter_n)),
            sge_u32c(lGetUlong(this_elem, BN_parameter_striding_step_size)),
            sge_u32c(lGetUlong(this_elem, BN_parameter_socket_offset)),
            sge_u32c(lGetUlong(this_elem, BN_parameter_core_offset)));
      } else if (strcmp(strategy, "explicit") == 0) {
         sge_dstring_sprintf_append(string, "%s", lGetString(this_elem, BN_parameter_explicit));
      } else {
         sge_dstring_append(string, "unknown");
      }
   }
   DRETURN(ret);
}

bool
binding_parse_from_string(lListElem *this_elem, lList **answer_list, dstring *string) 
{
   bool ret = true;

   DENTER(BINDING_LAYER, "binding_parse_from_string");

   if (this_elem != NULL && string != NULL) {
      int amount = 0;
      int stepsize = 0;
      int firstsocket = 0;
      int firstcore = 0;
      binding_type_t type = BINDING_TYPE_NONE; 
      dstring strategy = DSTRING_INIT;
      dstring socketcorelist = DSTRING_INIT;
      dstring error = DSTRING_INIT;

      if (parse_binding_parameter_string(sge_dstring_get_string(string), 
               &type, &strategy, &amount, &stepsize, &firstsocket, &firstcore, 
               &socketcorelist, &error) != true) {
         dstring parse_binding_error = DSTRING_INIT;

         sge_dstring_sprintf(&parse_binding_error, "-binding: ");
         sge_dstring_append_dstring(&parse_binding_error, &error);

         answer_list_add_sprintf(answer_list, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR,
                                 MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S, 
                                 sge_dstring_get_string(&parse_binding_error));

         sge_dstring_free(&parse_binding_error);
         ret = false;
      } else {
         lSetString(this_elem, BN_strategy, sge_dstring_get_string(&strategy));
         
         lSetUlong(this_elem, BN_type, type);
         lSetUlong(this_elem, BN_parameter_socket_offset, (firstsocket >= 0) ? firstsocket : 0);
         lSetUlong(this_elem, BN_parameter_core_offset, (firstcore >= 0) ? firstcore : 0);
         lSetUlong(this_elem, BN_parameter_n, (amount >= 0) ? amount : 0);
         lSetUlong(this_elem, BN_parameter_striding_step_size, (stepsize >= 0) ? stepsize : 0);
         
         if (strstr(sge_dstring_get_string(&strategy), "explicit") != NULL) {
            lSetString(this_elem, BN_parameter_explicit, sge_dstring_get_string(&socketcorelist));
         }
      }

      sge_dstring_free(&strategy);
      sge_dstring_free(&socketcorelist);
      sge_dstring_free(&error);
   }

   DRETURN(ret);
}

