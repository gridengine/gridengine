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

#if defined(SOLARISAMD64) || defined(SOLARIS86)
#  include <sys/processor.h>
#  include <sys/types.h>
#  include <sys/pset.h>
#endif 

#include <pthread.h>
#include "sge_mtutil.h"
#include "sge_log.h"

/* these sockets cores or threads are currently in use from SGE */
/* access them via getExecdTopologyInUse() because of initialization */
static char* logical_used_topology = NULL;
static int logical_used_topology_length = 0;

#if defined(PLPA_LINUX)
/* local functions for binding */
static bool binding_set_linear_Linux(int first_socket, int first_core, int amount_of_cores,
                        int offset);

static bool binding_set_striding_Linux(int first_socket, int first_core, int amount_of_cores,
                          int offset, int n, char** reason);

/* local functions for accessing PLPA */
static bool get_topology_linux(char** topology, int* length);
static int get_processor_id(int socket_number, int core_number);
static int get_amount_of_cores(int socket_number);
static int get_total_amount_of_cores(void);
static int get_amount_of_sockets(void); 
static bool set_processor_binding_mask(plpa_cpu_set_t* cpuset, const int processor_ids[], const int no_of_ids);
/* TODO DG do it with a pointer */
static bool bind_process_to_mask(const pid_t pid, const plpa_cpu_set_t cpuset); 
static bool has_core_binding(void);
static bool has_topology_information(void); 
static void* get_plpa_handle(void);
static void close_plpa_handle(void);

/* local handle for daemon in order to access PLPA library */
static void* plpa_lib_handle = NULL;

#endif

#if defined(SOLARISAMD64) || defined(SOLARIS86)
#  include <kstat.h>
#  include <sys/statfs.h>

/* public */
static bool get_topology_solaris(char** topology, int* length);
static bool generate_chipID_coreID_matrix(int*** matrix, int* length); 
static int get_amount_of_sockets_from_matrix(const int** matrix, const int length);
static int get_amount_of_cores_from_matrix(const int** matrix, const int length, int** cores, int* size);
static int get_amount_of_threads_from_matrix(const int** matrix, const int length, 
   int** threads, int* size); 

/* private */
static int get_chip_ids_from_matrix(const int** matrix, const int length, int** chip_ids, int* amount);
static int get_core_ids_from_matrix(const int** matrix, const int length, int** core_ids, int* amount);
static int get_ids_from_matrix(const int** matrix, const int length, const int which_ID,  
   int** ids, int* amount); 
static int is_different_id(const int id); 
static int get_amount_of_core_or_threads_from_matrix(const int** matrix, const int length, 
   int core, int** core_or_threads, int* size);

/* access functions for load report */
static int get_total_amount_of_cores_solaris(void);
static int get_total_amount_of_sockets_solaris(void);

/* for processor sets */
static bool get_processor_ids_solaris(const int** matrix, const int length, const int logical_socket_number,
      const int logical_core_number, processorid_t** pr_ids, int* pr_length);
   /* this could be used later on */
static int get_processor_id_solaris(const int** matrix, const int length, const int logical_socket_number, 
      const int logical_core_number, const int logical_thread_number, processorid_t* prid);

static int get_core_id_from_logical_core_number_solaris(const int** matrix, 
   const int length, const int chip_id, const int logical_core_number);
   
static int get_chip_id_from_logical_socket_number_solaris(const int** matrix, 
   const int length, const int logical_socket_number); 

static bool binding_set_linear_Solaris(const int first_socket, const int first_core, 
   const int amount_of_cores, const int step_size, psetid_t* psetid);

/* processor set related */
/* DG TODO do not const  */
static bool create_pset(const processorid_t* const plist, const int length, 
   psetid_t* const pset_id);

static bool delete_pset(psetid_t pset_id);

static bool bind_current_process_to_pset(psetid_t pset_id);

/* frees the memory allocated by the topology matrix */
static void free_matrix(int** matrix, const int length);

/* DG TODO: make use of this in Linux */
/* gets the positions in the topology string from a given <socket>,<core> pair */
static int get_position_in_topology(const int socket, const int core, const char* topology, 
   const int topology_length);

/* DG TODO: make use of this in Linux */
/* accounts all occupied resources given by a topology string into another one */
static bool account_job_on_topology(char** topology, const int topology_length, 
   const char* job, const int job_length);  
#endif

static int getMaxThreadsFromTopologyString(const char* topology); 

static int getMaxCoresFromTopologyString(const char* topology);

/* DG TODO length should be an output */
static bool is_starting_point(const char* topo, const int length, const int pos, 
   const int amount, const int stepsize, char** topo_account); 

/* updates the "topology in use" string by traversing the active jobs dirs */
#if 0
static bool update_binding_system_status(void);
#endif 

/* find next core in topology string */
static bool go_to_next_core(const char* topology, const int pos, int* new_pos); 

/* creates the binding file in the active jobs directory */
static bool create_binding_file(const char* topology, const char* psetid, const u_long32 job_id, 
   const u_long32 ja_task_id, const char *pe_task_id); 

/* parses the binding file from active_jobs directory and account on global string */
#if 0
static bool parse_binding_file_and_account(const char* fname);
#endif 

/* creates a string with the topology used from a single job */
static bool create_topology_used_per_job(char** accounted_topology, int* accounted_topology_length, 
            char* logical_used_topology, char* used_topo_with_job, int logical_used_topology_length);


/* here threads could be introduced */ 

/****** sge_binding/binding_set_linear() ***************************************
*  NAME
*     binding_set_linear() -- ??? 
*
*  SYNOPSIS
*     bool binding_set_linear(int first_socket, int first_core, int 
*     amount_of_cores, int offset) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     int first_socket    - ??? 
*     int first_core      - ??? 
*     int amount_of_cores - ??? 
*     int offset          - ??? 
*
*  RESULT
*     bool - 
*
*  NOTES
*     MT-NOTE: binding_set_linear() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool binding_set_linear(int first_socket, int first_core, int amount_of_cores, int offset) 
{
  
/* above is just needed in order to get it compiled without deleting code */
#if defined(PLPA_LINUX)
   return binding_set_linear_Linux(first_socket, first_core, amount_of_cores, offset);
#elif defined(SOLARISAMD64) || defined(SOLARIS86)
   /* the processor set id which we need in order to delete it when the job 
      does finish */
   psetid_t psetid;
   bool success = binding_set_linear_Solaris(first_socket, first_core, 
      amount_of_cores, 1, &psetid);

   /* TODO DG save the pset id somewhere in order to delete it */ 
   /* make it a global variable                                */ 
   /* WRITE IT TO BINDING FILE IN active_jobs DIRECTORY        */

   return success;
#else 
   /* other architectures are currently not supported */ 
   return false;
#endif
}

#if defined(SOLARISAMD64) || defined(SOLARIS86)
/****** sge_binding/binding_set_linear_Solaris() *******************************
*  NAME
*     binding_set_linear_Solaris() -- Binds current process to some cores. 
*
*  SYNOPSIS
*     bool binding_set_linear_Solaris(const int first_socket, const int 
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
*     MT-NOTE: binding_set_linear_Solaris() is not MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool binding_set_linear_Solaris(const int first_socket, const int first_core, 
   const int amount_of_cores, const int step_size, psetid_t* psetid)
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

   DENTER(TOP_LAYER, "binding_set_linear_Solaris");

   /* shepherd_trace("binding_set_linear_Solaris: first socket: %d first core: %d amount: %d stepsize: %d", 
      first_socket, first_core, amount_of_cores, step_size); */

   /* first get the topology of the host into a topology matrix */ 
   if (generate_chipID_coreID_matrix(&matrix, &mlength) != true) {
      /* couldn't generate topology matrix */
      return false;
   }

   /* check parameter */
   if (psetid == NULL) {
      /* no memory location of output parameter */
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
         free(cores);
         cores = NULL;
         return false;
      }
   }

   /* we have the first current_socket and current_core */
   /* hence we get the processor_ids (more in case of chip multithreading) */
   get_processor_ids_solaris((const int**) matrix, mlength, current_socket,
      current_core, &tmp_pid_list, &tmp_pid_list_length);
   
   if (tmp_pid_list_length < 1) {
      /* we got no Solaris processor id - abort */
      free_matrix(matrix, mlength);
      free(cores);
      cores = NULL;
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
            free(cores);
            free(pid_list);
            cores = NULL;
            return false;
         }
      } /* end while getting the correct current_socket number */
      
      /* collect the processor_ids (more in case of chip multithreading) */
      get_processor_ids_solaris((const int**) matrix, mlength, current_socket,
         current_core, &tmp_pid_list, &tmp_pid_list_length);

      /* grow allocated memory for processor ids */
      pid_list = (processorid_t *) realloc(pid_list, (pid_list_length 
         + tmp_pid_list_length) * sizeof(processorid_t));

      if (pid_list == NULL) {
         /* out of memory */ 
         free_matrix(matrix, mlength);
         free(cores);
         cores = NULL;
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
   }

   /* shepherd_trace("binding_set_linear_Solaris: length of pid list %d", pid_list_length); */

   /* finally bind the current process to the global pid_list and get the
      processor set id */
   if (create_pset(pid_list, pid_list_length, psetid) != true) {
      retval = false;
/*      shepherd_trace("binding_set_linear_Solaris: could't create pset"); */
   } else if (bind_current_process_to_pset(*psetid)) {
      /* current process is bound to psetid and psetid is output parameter */
      retval = true;
   } else {
      /* binding was not successful */
      retval = false;
/*      shepherd_trace("binding_set_linear_Solaris: couldn't bind current process to pset"); */
   }
   
   /* free memory in any case */ 
   free_matrix(matrix, mlength);
   free(cores);
   cores = NULL;
  
   return retval;
}


int create_processor_set_explicit_solaris(const int* list_of_sockets,
   const int samount, const int* list_of_cores, const int camount)
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
      free_matrix(matrix, length);
      return -1;
   }   

   /* first get the topology of the host into a topology matrix */ 
   if (generate_chipID_coreID_matrix(&matrix, &length) != true) {
      /* couldn't generate topology matrix */
      free_matrix(matrix, length);
      return -1;
   }

   /* allocate new memory for the processor id list */
   pid_list = (processorid_t *) calloc(length, sizeof(processorid_t));

   /* generate pid list for processor set creation */
   for (i = 0; i < samount; i++) {
      
      /* get the processor ids for the given socket and core */
      get_processor_ids_solaris((const int**) matrix, length, list_of_sockets[i],
         list_of_cores[i], &tmp_pid_list, &tmp_pid_list_length);

      /* check if we really got at least one processor ID */
      if (tmp_pid_list_length < 1) {
         /* we got no Solaris processor ID - abort */
         free_matrix(matrix, length);
         return -1;
      }
     
      /* add the processor IDs to the global list */
      pid_list = (processorid_t *) calloc(pid_list_length + tmp_pid_list_length, 
                     sizeof(processorid_t));
      /* append the processor ids to the global pid list */
      for (j = pid_list_length; j < pid_list_length + tmp_pid_list_length; j++) {
         pid_list[j] = tmp_pid_list[j - pid_list_length];
      }

      /* update size of processor ID list */
      pid_list_length += tmp_pid_list_length;

   }

   /* create processor set */
   if (create_pset(pid_list, samount, &psetid) != true) {
      /* error while doing this... */
      free_matrix(matrix, length);
      return -1;
   }

   /* free topology matrix */ 
   free_matrix(matrix, length);

   return (int) psetid;
}

int create_processor_set_striding_solaris(const int first_socket, 
   const int first_core, const int amount, const int step_size) 
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

   DENTER(TOP_LAYER, "create_processor_set_striding_Solaris");

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
         free(cores);
         cores = NULL;
         return -2;
      }
   }

   /* we have the first current_socket and current_core */
   /* hence we get the processor_ids (more in case of chip multithreading) */
   get_processor_ids_solaris((const int**) matrix, mlength, current_socket,
      current_core, &tmp_pid_list, &tmp_pid_list_length);
   
   if (tmp_pid_list_length < 1) {
      /* we got no Solaris processor id - abort */
      free_matrix(matrix, mlength);
      free(cores);
      cores = NULL;
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

   /* try to get the processor_ids from socket and core position (could be 
      more than one because of CMT */
   for (i = 1; i < amount; i++) { 
      
      /* LINEAR strategy: go just to the next core (step_size = 1) */
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
            free(cores);
            free(pid_list);
            cores = NULL;
            return -4;
         }
      } /* end while getting the correct current_socket number */
      
      /* collect the processor_ids (more in case of chip multithreading) */
      get_processor_ids_solaris((const int**) matrix, mlength, current_socket,
         current_core, &tmp_pid_list, &tmp_pid_list_length);

      /* grow allocated memory for processor ids */
      pid_list = (processorid_t *) realloc(pid_list, (pid_list_length 
         + tmp_pid_list_length) * sizeof(processorid_t));

      if (pid_list == NULL) {
         /* out of memory */ 
         free_matrix(matrix, mlength);
         free(cores);
         cores = NULL;
         return -5;
      }

      /* append the new pids to the pid list */ 
      int prid_cntr = 0;
      for (prid_cntr = 0; prid_cntr < tmp_pid_list_length; prid_cntr++) {
         /* copy processor id from the temporary list to the global list */
         pid_list[pid_list_length + prid_cntr] = tmp_pid_list[prid_cntr];
      }

      /* update global pid list length */
      pid_list_length += tmp_pid_list_length;
   }

   /* finally bind the current process to the global pid_list and get the
      processor set id -> root rights required !!! */
   if (create_pset(pid_list, pid_list_length, &psetid) != true) {
      /* couldn't generate processor set */
      retval = -6;
   } else {
      /* return processor set */
      retval = (int) psetid;
   }
   
   /* free memory in any case */ 
   free_matrix(matrix, mlength);
   free(cores);
   cores = NULL;
  
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
static void free_matrix(int** matrix, const int length) 
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
*     const int* list_of_sockets   - ??? 
*     const int samount            - ??? 
*     const int** list_of_cores    - ??? 
*     const int score              - ??? 
*     char** topo_used_by_job      - (out) 
*     int* topo_used_by_job_length - (out) 
*
*  RESULT
*     bool - True if the job can be bound to the topology, false if not. 
*
*  NOTES
*     MT-NOTE: binding_explicit_check_and_account() is MT safe 
*
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
        logical_used_topology, logical_used_topology_length) < 0)) {
        /* the <socket>,<core> does not exist */
        possible = false;
        break;
     } 

      /* check if this core is available (DG TODO introduce threads) */
      if (logical_used_topology[i] == 'C') {
         /* do temporarily account it */
         (*topo_used_by_job)[i] = 'c';
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
*     static bool - 
*
*  NOTES
*     MT-NOTE: account_job_on_topology() is not MT safe 
*
*  BUGS
*     ??? 
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
      || (*topology) == NULL || job == NULL) {
      return false;
   }

   /* go through topology and account */
   for (i = 0; i < job_length; i++) {
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
*     Warning: Do only use this function in shepherd because of shepherd_trace!
*
*     - pset_id must not be NULL 
*     - length must be > 0 
*     - and plist must not be NULL and have to contain at least one element
*
*  INPUTS
*     const processorid_t* const plist - in: Processor id list.  
*     const int length                 - in: Length of the processor id list. 
*     psetid_t* const pset_id          - out: Pointer to the fixed location for id. 
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
static bool create_pset(const processorid_t* const plist, const int length, 
   psetid_t* const pset_id)
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
/*         shepherd_trace("assing processor %d to set %d", plist[i], pset_id); */

         if (pset_assign(*pset_id, plist[i], NULL) == -1) {
               /* shepherd_trace("assign error : we've to detroy the set!\n"); */
               /* problem while assigning a CPU to the set */
               /* DG TODO ??? we detroy the set and do no binding at all*/
               /* detroy the processor set and return with error */ 
            if (pset_destroy(*pset_id) != 0) {
               /* Ooops - we have could have a major problem with a remaining pset */
/*               shepherd_trace("Alert! Couldn't destroy pset! This have to be done manually!"); */
               successful = false;
            } else {
               successful = false;
/*               shepherd_trace("Processor set is not created because processor %d couldn't be added!", 
                  plist[i]); */
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

bool bind_shepherd_to_pset(int pset_id) 
{
   /* try to bind current process to processor set */
   if (pset_bind((psetid_t)pset_id, P_PID, P_MYID, NULL) != 0) {
      /* binding was not successful */
      return false;
   }

   /* successfully bound current process to processor set */
   return true;
}


#endif 

#if defined(PLPA_LINUX)

/****** sge_binding/binding_set_linear_Linux() ***************************************
*  NAME
*     binding_set_linear_Linux() -- Bind current process linear to chunk of cores. 
*
*  SYNOPSIS
*     bool binding_set_linear(int first_socket, int first_core, int 
*     amount_of_cores, int offset) 
*
*  FUNCTION
*     Binds current process (shepherd) to a set of cores. All processes 
*     started by the current process are inheriting the core binding (Linux).
*     
*     The core binding is done in a linear manner, that means that 
*     the process is bound to 'amount_of_cores' cores using one core 
*     after another starting at socket 'first_socket' (usually 0) and 
*     core = 'first_core' (usually 0) + 'offset'. If the core number 
*     is higher than the number of cores which are provided by socket 
*     'first_socket' then the next socket is taken (the core number 
*      defines how many cores are skiped).
*
*  INPUTS
*     int first_socket    - The first socket (starting at 0) to bind to. 
*     int first_core      - The first core to bind. 
*     int amount_of_cores - The amount of cores to bind to. 
*     int offset          - The user specified core number offset. 
*
*  RESULT
*     bool - true if binding for current process was done, false if not
*
*  NOTES
*     MT-NOTE: binding_set_linear() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool binding_set_linear_Linux(int first_socket, int first_core, int amount_of_cores,
                        int offset)
{

   /* sets bitmask in a linear manner        */ 
   /* first core is on exclusive host 0      */ 
   /* first core could be set from scheduler */ 
   /* offset is the first core to start with (make sense only with exclusive host) */

   if (has_core_binding() == true) {

      /* get access to the dynamically loaded plpa library */
      void* plpa_lib = get_plpa_handle();

      if (plpa_lib != NULL) {
         /* bitmask for processors to turn on and off */
         plpa_cpu_set_t cpuset;
         /* turn off all processors */
         PLPA_CPU_ZERO(&cpuset);

         if (has_topology_information()) {
            /* amount of cores set in processor binding mask */ 
            int cores_set;
            /* next socket to use */
            int next_socket = first_socket;
            /* the amount of cores of the next socket */
            int socket_amount_of_cores;
            /* next core to use */
            int next_core = first_core + offset;
            /* all the processor ids selected for the mask */
            int proc_id[amount_of_cores]; 
            /* maximal amount of sockets on this system */
            int max_amount_of_sockets = get_amount_of_sockets();

            /* strategy: go to the first_socket and the first_core + offset and 
               fill up socket and go to the next one. */ 
               
            /* TODO maybe better to search for using a core exclusively? */
            
            while (get_amount_of_cores(next_socket) <= next_core) {
               /* TODO which kind of warning when first socket does not offer this? */
               /* move on to next socket - could be that we have to deal only with cores 
                  instead of <socket><core> tuples */
               next_core -= get_amount_of_cores(next_socket); 
               next_socket++;
               if (next_socket >= max_amount_of_sockets) {
                  /* we are out of sockets - we do nothing */
                  return false;
               }
            }  
            
            proc_id[0] = get_processor_id(next_socket, next_core);

            /* collect the other processor ids with the strategy */
            for (cores_set = 1; cores_set < amount_of_cores; cores_set++) {
               next_core++;
               /* jump to next socket when it is needed */
               /* maybe the next socket could offer 0 cores (I can' see when, 
                  but just to be sure) */
               while ((socket_amount_of_cores = get_amount_of_cores(next_socket)) 
                  <= next_core) {
                  next_socket++;
                  next_core = next_core - socket_amount_of_cores;
                  if (next_socket >= max_amount_of_sockets) {
                     /* we are out of sockets - we do nothing */
                     return false;
                  }
               }
               /* get processor id */ 
               proc_id[cores_set] = get_processor_id(next_socket, next_core);
            }
            
            /* set the mask for all processor ids */ 
            set_processor_binding_mask(&cpuset, proc_id, amount_of_cores);
            
            /* bind process to mask */ 
            if (bind_process_to_mask((pid_t) 0, cpuset) == false) {
               /* there was an error while binding */ 
               return false;
            }

         } else {
            /* TODO DG strategy without topology information but with 
               working library? */
            return false;
         }
         
      } else {
         /* have no access to plpa library */
         return false;
      }

   }
   close_plpa_handle();
   
   return true;
}

#endif 


bool binding_set_striding(int first_socket, int first_core, int amount_of_cores,
                          int offset, int n, char** reason)
{

   #if defined(PLPA_LINUX)
      return binding_set_striding_Linux(first_socket, first_core, amount_of_cores,
                          offset, n, reason);
   #elif defined(SOLARISX86) || defined(SOLARISAMD64)
      /* TODO DG delete processor set later on */
      psetid_t psetid;
      if (binding_set_linear_Solaris(first_socket, first_core, amount_of_cores, 
                          n, &psetid) == true) {

         /* report processorset id as reason in string */
         (*reason) = (char *) calloc(10, sizeof(char)); /* processor id storage */
         sprintf((*reason), "%d", (int) psetid);
         return true;
      } else {
         /* could not do binding */ 
         return false;
      }
   #else
      /* unsupported architecture */
      return false;
   #endif 
   
}


/****** sge_binding/binding_set_striding_Linux() *************************************
*  NAME
*     binding_set_striding_Linux() -- Binds current process to cores.  
*
*  SYNOPSIS
*     bool binding_set_striding(int first_socket, int first_core, int 
*     amount_of_cores, int offset, int n) 
*
*  FUNCTION
*     Performs a core binding for the calling process according to the 
*     'striding' strategy. The first core used is specified by first_socket
*     (beginning with 0) and first_core (beginning with 0). If first_core is 
*     greater than available cores on first_socket, the next socket is examined 
*     and first_core is reduced by the skipped cores. If the first_core could 
*     not be found on system (because it was to high) no binding will be done.
*     
*     If the first core was choosen the next one is defined by the step size 'n' 
*     which is incremented to the first core found. If the socket has not the 
*     core (because it was the last core of the socket for example) the next 
*     socket is examined.
*
*     If the system is out of cores and there are still some cores to select 
*     (because of the amount_of_cores parameter) no core binding will be performed.
*    
*  INPUTS
*     int first_socket    - first socket to begin with  
*     int first_core      - first core to start with  
*     int amount_of_cores - total amount of cores to be used 
*     int offset          - core offset for first core (increments first core used) 
*     int n               - step size 
*
*  RESULT
*     bool - Returns true if the binding was performed, otherwise false.
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: binding_set_striding() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool binding_set_striding_Linux(int first_socket, int first_core, int amount_of_cores,
                          int offset, int n, char** reason)
{
   /* n := take every n-th core */ 
   bool bound = false;

#if defined(PLPA_LINUX) 
   if (has_core_binding() == true) {

      /* get access to the dynamically loaded plpa library */
      void* plpa_lib = get_plpa_handle();

      if (plpa_lib != NULL) {
         /* bitmask for processors to turn on and off */
         plpa_cpu_set_t cpuset;  
         /* turn off all processors */
         PLPA_CPU_ZERO(&cpuset);

         /* when library offers architecture: 
            - get virtual processor ids in the following manner:
              * on socket "first_socket" choose core number "first_core + offset"
              * then add n: if core is not available go to next socket
              * ...
         */
         if (has_topology_information()) {
            /* amount of cores set in processor binding mask */ 
            int cores_set = 0;
            /* next socket to use */
            int next_socket = first_socket;
            /* next core to use */
            int next_core = first_core + offset;
            /* all the processor ids selected for the mask */
            int proc_id[amount_of_cores]; 
            /* single processor id */
            int processorid;
            /* maximal amount of sockets on this system */
            int max_amount_of_sockets = get_amount_of_sockets();
            
            /* check if we are already out of range */
            if (next_socket >= max_amount_of_sockets) {
               (*reason) = "already out of sockets!";
               return false;
            }   

            while (get_amount_of_cores(next_socket) <= next_core) {
               /* TODO which kind of warning when first socket does not offer this? */
               /* move on to next socket - could be that we have to deal only with cores 
                  instead of <socket><core> tuples */
               next_core -= get_amount_of_cores(next_socket); 
               next_socket++;
               if (next_socket >= max_amount_of_sockets) {
                  /* we are out of sockets - we do nothing */
                  (*reason) = "first core: out of sockets!";
                  return false;
               }
            }  

            proc_id[0] = get_processor_id(next_socket, next_core);
            
            /* turn on processor id in mask */ 
            /* TODO */ 
            
            /* collect the rest of the processor ids */ 
            for (cores_set = 1; cores_set < amount_of_cores; cores_set++) {
               /* calculate next_core number */ 
               next_core += n;
               
               /* check if we are already out of range */
               if (next_socket >= max_amount_of_sockets) {
                  (*reason) = "out of sockets";
                  return false;
               }   

               while (get_amount_of_cores(next_socket) <= next_core) {
                  /* TODO which kind of warning when first socket does not offer this? */
                  /* move on to next socket - could be that we have to deal only with cores 
                     instead of <socket><core> tuples */
                  next_core -= get_amount_of_cores(next_socket); 
                  next_socket++;
                  if (next_socket >= max_amount_of_sockets) {
                     /* we are out of sockets - we do nothing */
                     (*reason) = "nextcore: out of sockets!";
                     return false;
                  }
               }    

               /* TODO DG add processor id to mask */ 
               processorid = get_processor_id(next_socket, next_core);
               if (processorid >= 0) {
                  proc_id[cores_set] = processorid;
               } else {
                  if (processorid == -2) {
                     (*reason) = "processor id couldn't be retrieved!";
                  }
                  if (processorid == -3) {
                     (*reason) = "socket id couldn't be retrieved!";
                  }
                  (*reason) = "couldn' set processor id!";
                  return false;
               }   
               
            } /* collecting processor ids */

            /* set the mask for all processor ids */ 
            set_processor_binding_mask(&cpuset, proc_id, amount_of_cores);
            
            /* bind process to mask */ 
            if (bind_process_to_mask((pid_t) 0, cpuset) == true) {
               /* there was an error while binding */ 
               bound = true;
            }
            
         } else {
            /* setting bitmask without topology information which could 
               not be right? */
            /* TODO DG */   
            return false;
         }

      } else {
         /* could not load plpa library */
         return false;
      }
      
   } else {
      /* has no core binding feature */
      return false;
   }
   
   #endif
   
   
   return bound;
}

/****** sge_binding/binding_one_per_socket() ***********************************
*  NAME
*     binding_one_per_socket() -- ??? 
*
*  SYNOPSIS
*     bool binding_one_per_socket(int first_socket, int amount_of_sockets, int 
*     n) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     int first_socket      - ??? 
*     int amount_of_sockets - ??? 
*     int n                 - ??? 
*
*  RESULT
*     bool - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: binding_one_per_socket() is not MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool binding_one_per_socket(int first_socket, int amount_of_sockets, int n)
{

   /* n := take the n-th core per socket */
   
   
   return true;
}

/****** sge_binding/binding_n_per_socket() *************************************
*  NAME
*     binding_n_per_socket() -- ??? 
*
*  SYNOPSIS
*     bool binding_n_per_socket(int first_socket, int amount_of_sockets, int n) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     int first_socket      - ??? 
*     int amount_of_sockets - ??? 
*     int n                 - ??? 
*
*  RESULT
*     bool - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: binding_n_per_socket() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool binding_n_per_socket(int first_socket, int amount_of_sockets, int n)
{

   /* n := the first n cores of each socket */
   

   
   return true;
}


/****** sge_binding/binding_explicit() *****************************************
*  NAME
*     binding_explicit() -- Binds current process to specified CPU cores. 
*
*  SYNOPSIS
*     bool binding_explicit(int* list_of_cores, int camount, int* 
*     list_of_sockets, int samount) 
*
*  FUNCTION
*     Binds the current process to the cores specified by a <socket>,<core>
*     tuple. The tuple is given by a list of sockets and a list of cores. 
*     The elements on the same position of these lists are reflecting 
*     a tuple. Therefore the length of the lists must be the same.
*
*     Binding is currently done on Linux hosts only where the machine topology 
*     can be retrieved with PLPA library. It also does require this library.
*
*  INPUTS
*     int* list_of_sockets - List of sockets in the same order as list of cores. 
*     int samount          - Length of the list of sockets. 
*     int* list_of_cores   - List of cores in the same order as list of sockets. 
*     int camount          - Length of the list of cores. 
*
*  RESULT
*     bool - true when the current process was bound like specified with the 
*            input parameter
*
*  NOTES
*     MT-NOTE: binding_explicit() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool binding_explicit(const int* list_of_sockets, const int samount, 
   const int* list_of_cores, const int camount)
{
   /* return value: successful bound or not */ 
   bool bound = false;

   /* check if we have exactly the same amount of sockets as cores */
   if (camount != samount) {
      return false;
   }
#if defined(PLPA_LINUX) 
   /* do only on linux when we have core binding feature in kernel */
   if (has_core_binding() == true) {
      
      /* get access to the dynamically loaded plpa library */
      void* plpa_lib = get_plpa_handle();

      if (plpa_lib != NULL && has_topology_information()) {
         /* bitmask for processors to turn on and off */
         plpa_cpu_set_t cpuset;  
         /* turn off all processors */
         PLPA_CPU_ZERO(&cpuset);
         /* the internal processor ids selected for the binding mask */
         int proc_id[camount];
         /* processor id counter */
         int pr_id_ctr;

         /* Fetch for each socket,core tuple the processor id. 
            If this is not possible for one do nothing and return false. */ 

         /* go through all socket,core tuples and get the processor id */
         for (pr_id_ctr = 0; pr_id_ctr < camount; pr_id_ctr++) { 

            /* get the processor id */
            if (&list_of_cores[pr_id_ctr] == NULL || 
                &list_of_sockets[pr_id_ctr] == NULL) {
                /* got a null pointer therefore we abort*/
               return false;
            }

            /* get the OS internal processor id */ 
            proc_id[pr_id_ctr] = get_processor_id(list_of_sockets[pr_id_ctr], 
                                                   list_of_cores[pr_id_ctr]);
            /* check if this was successful */
            if (proc_id[pr_id_ctr] < 0) {
               /* a problem occured while getting the processor id */
               /* aborting and do nothing */
               return false;
            }
         }

         /* generate the core binding mask out of the processor id array */
         set_processor_binding_mask(&cpuset, proc_id, camount);

         /* do the core binding for the current process with the mask */
         if (bind_process_to_mask((pid_t) 0, cpuset) == true) {
            /* there was an error while binding */ 
            bound = true;
         } /* couldn't be bound return false */
          
      } /* has no PLPA lib or topology information */

   } /* has no core binding ability */

   #endif
   
   return bound;
}


/****** sge_binding/binding_linear_parse_amount() ******************************
*  NAME
*    binding_linear_parse_amount() -- Parse the amount of cores to occupy. 
*
*  SYNOPSIS
*     int binding_linear_parse_amount(const char* parameter) 
*
*  FUNCTION
*    Parses a string in order to get the amount of cores requested. 
* 
*    The string has following format: "linear:<amount>:[<socket>,<core>]" 
*
*  INPUTS
*     const char* parameter - The first character of the string  
*
*  RESULT
*     int - if a value >= 0 then this reflects the number of cores
*           if a value < 0 then there was a parsing error
*
*  NOTES
*     MT-NOTE: binding_linear_parse_amount() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int binding_linear_parse_amount(const char* parameter) 
{
   int retval = -1;

   /* expect string "linear" or "linear:<amount>" or linear 
      "linear:<amount>:<firstsocket>,<firstcore>" */

   if (parameter != NULL && strstr(parameter, "linear") != NULL) {
      /* get number after linear: and before \0 or : */ 
      if (sge_strtok(parameter, ":") != NULL) {
         char* n = sge_strtok(NULL, ":");
         if (n != NULL) {
            return atoi(n);
         } 
      }   
   } 

   /* parsing error */
   return retval;
}




/****** sge_binding/bindingLinearParseSocketOffset() ***************************
*  NAME
*     bindingLinearParseSocketOffset() -- ??? 
*
*  SYNOPSIS
*     int bindingLinearParseSocketOffset(const char* parameter) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const char* parameter - ??? 
*
*  RESULT
*     int - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: bindingLinearParseSocketOffset() is not MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int binding_linear_parse_socket_offset(const char* parameter)
{
   /* offset is like "linear:<N>:<socket>,<core>) */
   if (parameter != NULL && strstr(parameter, "linear") != NULL) {
      /* fetch linear */
      if (sge_strtok(parameter, ":") != NULL) {
         /* fetch first number (if any) */
         if (sge_strtok(NULL, ":") != NULL) {
            char* offset = sge_strtok(NULL, ",");
            if (offset != NULL) { 
               /* offset points to <socket> */
               return atoi(offset);
            } 
         }
      }
   }
   
   /* wasn't able to parse */
   return -1;
}

/****** sge_binding/bindingLinearParseCoreOffset() *****************************
*  NAME
*     bindingLinearParseCoreOffset() -- ??? 
*
*  SYNOPSIS
*     int bindingLinearParseCoreOffset(const char* parameter) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const char* parameter - ??? 
*
*  RESULT
*     int - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: bindingLinearParseCoreOffset() is not MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int binding_linear_parse_core_offset(const char* parameter)
{
   /* offset is like "linear:<N>:<socket>,<core> (optional ":") */
   if (parameter != NULL && strstr(parameter, "linear") != NULL) {
      /* fetch linear */
      if (sge_strtok(parameter, ":") != NULL) {
         /* fetch first number (if any) */
         if (sge_strtok(NULL, ":") != NULL) {
            char* offset = sge_strtok(NULL, ",");
            if (offset != NULL && 
                  (offset = sge_strtok(NULL, ":")) != NULL) {
               /* offset points to <core> */
               return atoi(offset);
            } 
         }
      }
   }
  
   /* wasn't able to parse */
   return -1;
}


/****** sge_binding/binding_explicit_has_correct_syntax() *********************
*  NAME
*     binding_explicit_has_correct_syntax() -- Check if parameter has correct syntax. 
*
*  SYNOPSIS
*     bool binding_explicit_has_correct_syntax(const char* parameter) 
*
*  FUNCTION
*     This function checks if the given string is a valid argument for the 
*     -binding parameter which provides a list of socket, cores which have 
*     to be selected explicitly.
* 
*     The accepted syntax is: "explicit:[1-9][0-9]*,[1-9][0-9]*(:[1-9][0-9]*,[1-9][0-9]*)*"
*
*     This is used from parse_qsub.c.
*
*  INPUTS
*     const char* parameter - A string with the parameter. 
*
*  RESULT
*     bool - True if the parameter has the expected syntax.
*
*  NOTES
*     MT-NOTE: binding_explicit_has_correct_syntax() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool binding_explicit_has_correct_syntax(const char* parameter) 
{
   
   /* DG TODO: introduce check if particles are numbers */

   /* check if the head is correct */
   if (strstr(parameter, "explicit:") == NULL) {
      return false;
   }

   if (sge_strtok(parameter, ":") != NULL) {
      char* socket = NULL;

      /* first socket,core is mandatory */ 
      if ((socket = sge_strtok(NULL, ",")) == NULL) {
         /* we have no first socket number */
         return false;
      }
      /* check for core */
      if (sge_strtok(NULL, ":") == NULL) {
         /* we have no first core number */
         return false;
      }

      do {
         /* get socket number */ 
         if ((socket = sge_strtok(NULL, ",")) != NULL) {

            /* we have a socket therefore we need a core number */
            if (sge_strtok(NULL, ":") == NULL) {
               /* no core found */
               return false;
            }   

         } 
   
      } while (socket != NULL);  /* we try to continue with the next socket if possible */ 

   } else {
      /* this should not be reachable because of the pre-check */
      return false;
   }

   return true;
}

bool binding_explicit_exctract_sockets_cores(const char* parameter, 
   int** list_of_sockets, int* samount, int** list_of_cores, int* camount) 
{
   /* string representation of a socket number */
   char* socket = NULL;
   /* string representation of a core number */
   char* core = NULL;
   
   /* no sockets and no cores at the beginning */
   *samount = 0;
   *camount = 0;

   if (*list_of_sockets != NULL || *list_of_cores != NULL) {
      /* we expect NULL pointers because we allocate memory within the function */
      return false;
   }

   /* check if the prefix of the parameter is correct */
   if (strstr(parameter, "explicit:") == NULL) {
      return false;
   }

   if (sge_strtok(parameter, ":") != NULL) {
      
      /* first socket,core is mandatory */ 
      if ((socket = sge_strtok(NULL, ",")) == NULL) {
         /* we have no first socket number */
         return false;
      }
      if ((core = sge_strtok(NULL, ":")) == NULL) {
         /* we have no first core number */
         return false;
      }
      
      /* adding first socket,core pair */
      *samount = *camount = 1;
      *list_of_sockets = realloc(*list_of_sockets, (*samount)*sizeof(int));
      *list_of_cores = realloc(*list_of_cores, (*camount)*sizeof(int));
      (*list_of_sockets)[0] = atoi(socket);
      (*list_of_cores)[0] = atoi(core);

      do {
         /* get socket number */ 
         if ((socket = sge_strtok(NULL, ",")) != NULL) {

            /* we have a socket therefore we need a core number */
            if ((core = sge_strtok(NULL, ":")) == NULL) {
               return false;
            }   
            
            /* adding the next <socket>,<core> tuple */
            (*samount)++; (*camount)++; 
            (*list_of_sockets) = realloc(*list_of_sockets, (*samount)*sizeof(int));
            (*list_of_cores) = realloc(*list_of_cores, (*camount)*sizeof(int));
            (*list_of_sockets)[*samount] = atoi(socket);
            (*list_of_cores)[*camount] = atoi(core);
         } 
   
      } while (socket != NULL);  /* we try to continue with the next socket if possible */ 

   } else {
      /* this should not be reachable because of the pre-check */
      return false;
   }

   return true; 
}



/****** sge_binding/binding_striding_parse_first_core() ************************
*  NAME
*     binding_striding_parse_first_core() -- Parses core number from command line. 
*
*  SYNOPSIS
*     int binding_striding_parse_first_core(const char* parameter) 
*
*  FUNCTION
*     Parses the core number from command line in which to start binding 
*     in "striding" case. 
*
*     -binding striding:<amount>:<stepsize>:<socket>,<core>
*
*  INPUTS
*     const char* parameter - Pointer to first character of CL string. 
*
*  RESULT
*     int - -1 in case the string is corrupt or core number is not set
*           >= 0 in case the core number could parsed successfully.
*
*  NOTES
*     MT-NOTE: binding_striding_parse_first_core() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int binding_striding_parse_first_core(const char* parameter)
{
   /* DG TODO move to uti/ */
   /* "striding:<amount>:<stepsize>:<socket>,<core>" */
   if (parameter != NULL && strstr(parameter, "striding") != NULL) {
      /* fetch "striding" */
      if (sge_strtok(parameter, ":") != NULL) {
         /* fetch <amount> */
         if (sge_strtok(NULL, ":") != NULL) {
            /* fetch <stepsize> */
            if (sge_strtok(NULL, ":") != NULL) {
               /* fetch first <socket> */
               if (sge_strtok(NULL, ",") != NULL) {
                  /* fetch first <core> */ 
                  char* first_core = NULL;
                  /* end usually with line end */
                  if ((first_core = sge_strtok(NULL, ";")) != NULL) {
                     return atoi(first_core);
                  } 
               }
            }
         }
      }   
   }

   return -1;
}


/****** sge_binding/binding_striding_parse_first_socket() **********************
*  NAME
*     binding_striding_parse_first_socket() -- ??? 
*
*  SYNOPSIS
*     int binding_striding_parse_first_socket(const char* parameter) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const char* parameter - ??? 
*
*  RESULT
*     int - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: binding_striding_parse_first_socket() is not MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int binding_striding_parse_first_socket(const char* parameter)
{
   /* DG TODO move to uti/ */

   /* "striding:<amount>:<stepsize>:<socket>,<core>" */
   if (parameter != NULL && strstr(parameter, "striding") != NULL) {
      /* fetch "striding" */
      if (sge_strtok(parameter, ":") != NULL) {
         /* fetch amount*/
         if (sge_strtok(NULL, ":") != NULL) {
            /* fetch stepsize */
            if (sge_strtok(NULL, ":") != NULL) {
               /* fetch first socket */ 
               char* first_socket = NULL;
               if ((first_socket = sge_strtok(NULL, ",")) != NULL) {
                  return atoi(first_socket);
               } 
            }
         }
      }   
   }

   return -1;
}

int binding_striding_parse_amount(const char* parameter)
{
   /* striding:<amount>:<step-size>:[starting-socket,starting-core] */

   /* DG TODO move to uti/ */
   if (parameter != NULL && strstr(parameter, "striding") != NULL) {
      
      /* fetch "striding:" */
      if (sge_strtok(parameter, ":") != NULL) {
         char* amount = NULL;

         if ((amount = sge_strtok(NULL, ":")) != NULL) {
            /* get the number from amount */
            /* DG TODO check if this is really a number */
            return atoi(amount);
         }      
      }
   }

   /* couldn't parse it */
   return -1;
}

int binding_striding_parse_step_size(const char* parameter)
{
   /* DG TODO move to uti/ */
   /* striding:<amount>:<step-size>:  */ 
   if (parameter != NULL && strstr(parameter, "striding") != NULL) {
      /* fetch "striding:" */
      if (sge_strtok(parameter, ":") != NULL) {
         if (sge_strtok(NULL, ":") != NULL) {
            /* fetch step size */
            char* stepsize = NULL;
            if ((stepsize = sge_strtok(NULL, ":")) != NULL) {
               /* return step size */
               return atoi(stepsize);
            }
         }
      }
   }
   
   /* in default case take each core */
   return -1;
}


/****** sge_binding/getExecdAmountOfCores() ************************************
*  NAME
*     getExecdAmountOfCores() -- Returns the total amount of cores the host has. 
*
*  SYNOPSIS
*     int getExecdAmountOfCores() 
*
*  FUNCTION
*     Retrieves the total amount of cores (currently Linux only) 
*     the current host have.
*
*  RESULT
*     int - The amount of cores the current host has. 
*
*  NOTES
*     MT-NOTE: getExecdAmountOfCores() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int getExecdAmountOfCores() 
{
#if defined(PLPA_LINUX) 
      return get_total_amount_of_cores();
#elif defined(SOLARISAMD64) || defined(SOLARIS86) 
      return get_total_amount_of_cores_solaris();
#else   
      return 0;
#endif  
}

/****** sge_binding/getExecdAmountOfSockets() **********************************
*  NAME
*     getExecdAmountOfSockets() -- The total amount of sockets in the system. 
*
*  SYNOPSIS
*     int getExecdAmountOfSockets() 
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
*     MT-NOTE: getExecdAmountOfSockets() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int getExecdAmountOfSockets()
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
   if ((*topology) == NULL) {
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

   /* update "logical_used_topology" string */
   #if 0
   if (update_binding_system_status() == false) {
      return false;
   }
   #endif 
  
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
      (*topology) = (char *) calloc(logical_used_topology_length+1, sizeof(char));
      memcpy((*topology), logical_used_topology, sizeof(char)*(logical_used_topology_length));
      retval = true;
   } 
      
   return retval;   
}


#if 0
static bool update_binding_system_status()
{
   /* go through all active jobs dirs        */ 

   /* path to "binding" file which is in active jobs directory */
   /* there could be more (in subdirectories for parallel slave jobs */
   dstring fname  = DSTRING_INIT;

   /* get all process ids running here */ 
   char active_dir_buffer[SGE_PATH_MAX] = "";
   
   dstring active_dir;
   
   /* job and taskid */
   u_long32 jobid, jataskid;
   /*const char *pe_task_id = NULL; */

   /* list entries of job list, ja task list, and pe list */
   lListElem* jep = NULL;
/*   lListElem* petep = NULL; */
   lListElem* jatep = NULL;

   /* return value */
   bool success = true;

   sge_dstring_init(&active_dir, active_dir_buffer, sizeof(active_dir_buffer));

   /* all job ids, task ids and pe ids on execution host */

   /* for all jobs */
   for_each(jep, *(object_type_get_master_list(SGE_TYPE_JOB))) {

      /* get jobid */ 
      jobid = lGetUlong(jep, JB_job_number);
      
      /* go through all tasks */
      for_each (jatep, lGetList(jep, JB_ja_tasks)) {
        
         /* get task id */
         jataskid = lGetUlong(jatep, JAT_task_number);
         
         /* DG TODO: just go in main active_job directory (reactivate 
            the code below in order to get core usage from parallel 
            jobs) */

         /* check if parallel entries exists 
         if (lGetList(jatep, JAT_task_list) != NULL) {

             go through all parallel entries 
            for_each(petep, lGetList(jatep, JAT_task_list)) {
            
                 get parallel task id   
               pe_task_id = lGetString(petep, PET_id); 
               
                check here for existance of "binding" file and 
                  parse it ("active_jobs/X.X/pe_task_id/binding")
                  for parallel slave tasks 
               sge_get_active_job_file_path(&fname, jobid, jataskid, pe_task_id, 
                                "binding");
               
                parse binding file and account it (for execd global view 
                  of current status to report) 
               if (!parse_binding_file_and_account(sge_dstring_get_string(&fname))) {
                  success = false;
               }  

            }  all parallel entries 
         }
         */
         
         /* in any case we need the "active_jobs/X.X/binding" file */
            
         /* we have not such a parallel entries list - we need the main directory */ 
         sge_get_active_job_file_path(&fname, jobid, jataskid, NULL, 
            "binding");

         /* parse binding file and do the accounting */
         if (!parse_binding_file_and_account(sge_dstring_get_string(&fname))) {
            success = false;
         }
         

      } /* for all tasks */

   } /* for all jobs */

   sge_dstring_free(&fname);

   return success;
}
#endif 

#if 0
static bool parse_binding_file_and_account(const char* fname)
{
   
   /* binding file has following content: 
      -first line: <used_topology> as string 
      "SccCCscccc" for example for using 
       the second socket completely and 
       the first two cores of the first socket 
       (C is a free core; c a used one)           */
   FILE *fp;
   char buf[10000];
   /* size of the topology string in binding file */
   int size = 0;

   /* open binding file */
   fp = fopen(fname, "r");
   if (!fp) {
      /* can't open binding file */
      return false;
   }

   if (fgets(buf, sizeof(buf), fp)) {
      /* we don't expect to have larger content (ca. 10000 cores
         at one host */
      int i;  
      /* first check the size of the topology string 
         -> it must be equal to current "global topology 
            in use" string 
         -> current "global topology in use" string must 
            exist (if not this string would be taken for 
            that but just for savety reasons) */
      for (i = 0; i < sizeof(buf) && buf[i] != '\0'; i++) {

         if (buf[i] == 'c' || buf[i] == 'C' || 
             buf[i] == 'S' || buf[i] == 's' || 
             buf[i] == 'T' || buf[i] == 't') {

            /* all allowed characters */
            size++;
         } else {
            /* this is an unrecognized character -> therefore the content of 
               "binding" file is not correct */
            return false;
         }
      }

      buf[i] = '\0'; 
      size++;

      /* check if size is correct */
      if (logical_used_topology == NULL || 
            logical_used_topology_length == 0) {
         /* we have no topology string which represents the state of 
            the execution host yet */
         logical_used_topology = (char *) calloc(size, sizeof(char));
         logical_used_topology_length = size;
         /* copy it */
         memcpy(logical_used_topology, buf, sizeof(char) * size);
      } else {
         /* we have a global string but it must have the same size */ 
         if (size != logical_used_topology_length) {
            /* PROBLEM: the size of the topology string in "binding" file from 
               active jobs directory has NOT the same size than the current 
               global string which represents the topology in use !!! */
            return false;   
         } else {
            /* account the topology in use from active job                  */ 
            /* this is done by saving all lower case characters (logical OR)*/
            for (i = 0; i < size; i++) {
               if (buf[i] == 'c') {
                  /* account core in use   */
                  logical_used_topology[i] = 'c';
               } else if (buf[i] == 's') {
                  /* account socket in use */
                  logical_used_topology[i] = 's';
               } else if (buf[i] == 't') {
                  /* account thread in use */
                  logical_used_topology[i] = 't';
               }
            }
            
         }
      }

   } else {
      /* nothing to read -> nothing to account */
   }
   
   fclose(fp);

   return true;
}   
#endif 

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


/* ---------------------------------------------------------------------------*/
/* ---------------------------------------------------------------------------*/
/*                    Beginning of LINUX related functions                    */
/* ---------------------------------------------------------------------------*/
/* ---------------------------------------------------------------------------*/

#if defined(PLPA_LINUX) 
static bool get_topology_linux(char** topology, int* length)
{
   /* topology string */
   dstring d_topology = DSTRING_INIT;
   /* return value */ 
   bool success = true;

   /* get handle to the PLPA library */
   void* plpa_handle = get_plpa_handle();

   /* initialize length of topology string */
   (*length) = 0;

   if (plpa_handle) {
      int has_topology = 0;
      /* get access to PLPA function */
      int (*topology_information)(int*) = dlsym(plpa_handle, "plpa_have_topology_information");

      /* check if topology is supported via PLPA */
      if ((*topology_information)(&has_topology) == 0 && has_topology == 1) {
         int num_sockets, max_socket_id;
         /* the topology string */ 
         sge_dstring_clear(&d_topology);

         /* build the topology string */
         int (*socket_information)(int*, int*) = dlsym(plpa_handle, "plpa_get_socket_info");

         if ((*socket_information)(&num_sockets, &max_socket_id) == 0) {
            int (*core_information)(int, int*, int*) = dlsym(plpa_handle, "plpa_get_core_info");
            int num_cores, max_core_id, ctr_cores, ctr_sockets;
            char* s = "S"; /* socket */
            char* c = "C"; /* core   */

            for (ctr_sockets = 0; ctr_sockets <= max_socket_id; ctr_sockets++) {
               /* append new socket */
               sge_dstring_append_char(&d_topology, *s);
               (*length)++;

               /* for each socket get the number of cores */ 
               if ((*core_information)(ctr_sockets, &num_cores, &max_core_id) == 0) {
                     
                  for (ctr_cores = 0; ctr_cores <= max_core_id; ctr_cores++) {
                     sge_dstring_append_char(&d_topology, *c);
                     (*length)++;
                  }
                     
               }
            } /* for each socket */

            /* convert d_topolgy into topology */
            (*length)++; /* we need `\0` at the end */
            /* (*topology) = (char*) sge_dstring_get_string(&d_topology); */
            /* copy element */ 
            (*topology) = (char *) calloc((*length), sizeof(char));
            memcpy((*topology), (char*) sge_dstring_get_string(&d_topology), 
               (*length) * sizeof(char));
         } /* when socket information is available */ 
         else { 
            (*topology) = "warning: socket information not available!";
            success = false;
         }

      } else {
         (*topology) = "warning: host has no topology";
         success = false;
      }
   } else {
      /* couldn't open library */
      /* TODO DG print warning somwhere */
      (*topology) = "warning: couldn't open plpa library";
      success = false;
   }

   /* TODO DG place it somewhere at the end of execd */ 
   close_plpa_handle();

   return success;
}


/****** sge_binding/get_processor_id() *****************************************
*  NAME
*     get_processor_id() -- ??? 
*
*  SYNOPSIS
*     static int get_processor_id(int socket_number, int core_number) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     int socket_number - Socket (starting at 0) to search for core. 
*     int core_number   - Core number (starting at 0) to get id for. 
*
*  RESULT
*     static int - 
*
*  NOTES
*     MT-NOTE: get_processor_id() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int get_processor_id(int socket_number, int core_number) 
{
    
   if (has_core_binding() && has_topology_information()) {
      int proc_id = -1;
      int socket_id = -1;
      void* plpa_handle = get_plpa_handle();
      
      int (*map_to_processor_id)(int, int, int*) = dlsym(plpa_handle, 
                                 "plpa_map_to_processor_id");

      /* we need Linux internal socket_id from socket number */
      int (*get_socket_id)(int, int*) = dlsym(plpa_handle, "plpa_get_socket_id");
      
      if ((*get_socket_id)(socket_number, &socket_id) != 0) {
         /* unable to retrieve Linux logical socket id */
         return -3;
      }

      if ((*map_to_processor_id)(socket_id, core_number, &proc_id) == 0) {
         /* everything OK: processor id was set */
         return proc_id;
      } else {
         /* processor id couldn't retrieved */
         return -2; 
      }
   } 
   
   /* no support for this topology related call */
  return -1;

}

/****** sge_binding/get_total_amount_of_cores() ********************************
*  NAME
*     get_total_amount_of_cores() -- Fetches the total amount of cores on system. 
*
*  SYNOPSIS
*     static int get_total_amount_of_cores() 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*
*  RESULT
*     static int - Total amount of cores installed on the system. 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: get_total_amount_of_cores() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int get_total_amount_of_cores() 
{
   /* total amount of cores currently active on this system */
   int total_amount_of_cores = 0;

   /* handle for PLPA library */
   void* plpa_handle = get_plpa_handle();

   if (plpa_handle != NULL && has_core_binding() && has_topology_information()) {
      /* plpa_handle just for an early pre check */ 
      int nr_socket = get_amount_of_sockets();
      int cntr;
      
      /* get for each socket the amount of cores */
      for (cntr = 0; cntr < nr_socket; cntr++) {
         total_amount_of_cores += get_amount_of_cores(cntr);
      }
   }
   /* in case we got no information about topology we return 0 */
   return total_amount_of_cores;
}

/****** sge_binding/get_amount_of_cores() **************************************
*  NAME
*     get_amount_of_cores() -- ??? 
*
*  SYNOPSIS
*     static int get_amount_of_cores(int socket_number) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     int socket_number - Physical socket number starting at 0. 
*
*  RESULT
*     static int - 
*
*  NOTES
*     MT-NOTE: get_amount_of_cores() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int get_amount_of_cores(int socket_number) 
{
   /* handle for PLPA library */
   void* plpa_handle = get_plpa_handle();

   if (plpa_handle != NULL && has_core_binding() && has_topology_information()) {
      int socket_id;
      /* convert the reals socket number into the Linux socket_id */

      int (*get_socket_id)(int, int*) = dlsym(plpa_handle, "plpa_get_socket_id");
      
      if ((*get_socket_id)(socket_number, &socket_id) == 0) {
         int number_of_cores, max_core_id;
         /* now retrieve the amount of core for this socket number */
         int (*get_core_info)(int, int*, int*) 
               = dlsym(plpa_handle, "plpa_get_core_info");
               
         if ((*get_core_info)(socket_id, &number_of_cores, &max_core_id) == 0) {
            /* return the amount of cores available */
            return number_of_cores;
         } else {
            /* error when doing library call */
            return 0;
         }   

      } else {
         /* error: we didn't get the linux socket id */
         return 0;
      }
   }

   /* we have 0 cores in case something is wrong */
   return 0;
}

/****** sge_binding/get_amount_of_sockets() ************************************
*  NAME
*     get_amount_of_sockets() -- Get the amount of available sockets.  
*
*  SYNOPSIS
*     static int get_amount_of_sockets() 
*
*  FUNCTION
*     Returns the amount of sockets available on this system. 
*
*  RESULT
*     static int - The amount of available sockets on system. 0 in case of 
*                  of an error.
*
*  NOTES
*     MT-NOTE: get_amount_of_sockets() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int get_amount_of_sockets() 
{

   void* plpa_handle = get_plpa_handle();

   if (plpa_handle != NULL && has_core_binding() && has_topology_information()) {
      int num_sockets, max_socket_id;

      int (*get_socket_info)(int *, int *) = dlsym(plpa_handle, 
         "plpa_get_socket_info");
      
      if ((*get_socket_info)(&num_sockets, &max_socket_id) == 0) {
         return num_sockets;
      } else {
         /* in case of an error we have 0 sockets */
         return 0;
      }
   }

   /* we have 0 cores in case something is wrong */
   return 0;
}


/****** sge_binding/set_processor_binding_mask() *******************************
*  NAME
*     set_processor_binding_mask() -- ??? 
*
*  SYNOPSIS
*     static bool set_processor_binding_mask(plpa_cpu_set_t* cpuset, const int* 
*     processor_ids) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     plpa_cpu_set_t* cpuset   - ??? 
*     const int* processor_ids - ??? 
*
*  RESULT
*     static bool - 
*
*  NOTES
*     MT-NOTE: set_processor_binding_mask() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool set_processor_binding_mask(plpa_cpu_set_t* cpuset, const int processor_ids[], 
                  const int no_of_ids)
{
   int proc_num;

   if (processor_ids == NULL || cpuset == NULL) {
      return false;
   }

   /* turns on all processors from processor_ids array */
   for (proc_num = 0; proc_num < no_of_ids; proc_num++) {
      PLPA_CPU_SET(processor_ids[proc_num], cpuset);
   }
  
   return true;
}

/****** sge_binding/bind_process_to_mask() *************************************
*  NAME
*     bind_process_to_mask() -- ??? 
*
*  SYNOPSIS
*     static bool bind_process_to_mask(const pid_t pid, const plpa_cpu_set_t 
*     cpuset) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const pid_t pid             - ??? 
*     const plpa_cpu_set_t cpuset - ??? 
*
*  RESULT
*     static bool - 
*
*  NOTES
*     MT-NOTE: bind_process_to_mask() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool bind_process_to_mask(const pid_t pid, const plpa_cpu_set_t cpuset)
{
   void* plpa_handle = get_plpa_handle();

   if (plpa_handle != NULL && has_core_binding() && &cpuset != NULL) {
      /* we only need core binding capabilites, no topology is required */
      /* DG TODO do it once while execd init (binding_init) */
      int (*sched_setaffinity)(pid_t, size_t, const plpa_cpu_set_t*) 
                                 = dlsym(plpa_handle, "plpa_sched_setaffinity"); 
      /* DG TODO delete addres and change header in order to make cputset to pointer */ 
      if ((*sched_setaffinity)(pid, sizeof(plpa_cpu_set_t), &cpuset) != 0) {
         return false;
      } else {
         return true;
      }   
   }
   
   return false;
}

/****** sge_binding/has_core_binding() *****************************************
*  NAME
*     has_core_binding() -- Check if core binding system call is supported. 
*
*  SYNOPSIS
*     static bool has_core_binding() 
*
*  FUNCTION
*     Checks if core binding is supported on the machine or not. If it is 
*     supported this does not mean that topology information (about socket 
*     and core amount) is available (which is needed for internal functions 
*     in order to perform a correct core binding).
*     Nevertheless a bitmask could be generated and core binding could be 
*     performed with this selfcreated bitmask.
*
*  RESULT
*     static bool - True if core binding could be done. False if not. 
*
*  NOTES
*     MT-NOTE: has_core_binding() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool has_core_binding() 
{
   
   /* checks if plpa is working */
   /* TODO do it only once? */

   void* plpa_handle = get_plpa_handle();

   if (plpa_handle) {
      plpa_api_type_t api_type;
      int (*api_probe)(plpa_api_type_t*) = dlsym(plpa_handle, "plpa_api_probe");

      if ((*api_probe)(&api_type) == 0 && api_type == PLPA_PROBE_OK) {
         return true;
      }
   }
   
   return false;
}

/****** sge_binding/has_topology_information() *********************************
*  NAME
*     has_topology_information() -- Checks if current arch offers topology. 
*
*  SYNOPSIS
*     static bool has_topology_information() 
*
*  FUNCTION
*     Checks if current architecture (on which this function is called) 
*     offers processor topology information or not.
*
*  RESULT
*     static bool - true if the arch offers topology information false if not 
*
*  NOTES
*     MT-NOTE: has_topology_information() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool has_topology_information() 
{
   void* plpa_handle = get_plpa_handle();

   if (plpa_handle) {
      int has_topology = 0;
      int (*topology_information)(int*) = dlsym(plpa_handle, 
                                             "plpa_have_topology_information");

      if ((*topology_information)(&has_topology) == 0 && has_topology == 1) {
         return true;
      }
   } 
    
   return false;
}

/****** sge_binding/get_plpa_handle() ******************************************
*  NAME
*     get_plpa_handle() -- Get access handle for PLPA libary. 
*
*  SYNOPSIS
*     static void* get_plpa_handle(void) 
*
*  FUNCTION
*     Opens the plpa library if installed on machine 
*     and returns the handle for using the library.
*     If the library was opened from earlier calls 
*     it returns the already created handle. 
*
*  RESULT
*     static void* - handle for accessing the PLPA library 
*
*  NOTES
*     MT-NOTE: get_plpa_handle() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static void* get_plpa_handle(void) 
{
   if (plpa_lib_handle == NULL) {
      plpa_lib_handle = dlopen("libplpa.so", RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
   }     
   
   return plpa_lib_handle;
}

/****** sge_binding/close_plpa_handle() ****************************************
*  NAME
*     close_plpa_handle() -- Close global plpa library handle. 
*
*  SYNOPSIS
*     static void close_plpa_handle(void) 
*
*  FUNCTION
*     Closes plpa library handle if not already closed. 
*
*  NOTES
*     MT-NOTE: close_plpa_handle() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static void close_plpa_handle(void) 
{
   if (plpa_lib_handle != NULL) {
      dlclose(plpa_lib_handle);
      plpa_lib_handle = NULL;
   }   
}
#endif

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
   dstring d_topology = DSTRING_INIT;
   
   /* matrix with the kstat values */
   int** matrix = NULL;

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
   
   (*length) = 0;

   /* generate matrix with socket_id and core_id */
   if (generate_chipID_coreID_matrix(&matrix, length)) {

      /* clear topology string */ 
      sge_dstring_clear(&d_topology);
      
      /* get cores per socket array */
      get_amount_of_core_or_threads_from_matrix((const int**)matrix, *length, 1, 
         &cores_per_socket, &cores_per_socket_length);
      
      /* get threads per core array */
      get_amount_of_core_or_threads_from_matrix((const int**)matrix, *length, 0, 
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
            if (threads_per_core[all_cores + core] > 1) {
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
   } else {
      /* we couldn't get the kernel kstat values therefor we have no topology */
      (*topology) = "NONE";
      (*length)   = 5;
      return false;
   }

   /* we need `\0' at the end */
   (*length) += 1;
                 
   /* free matrix, cores_per_socket, and threads_per_socket vector */
   /* (*topology) = (char *) sge_dstring_get_string(&d_topology);  */
   (*topology) = (char *) calloc((*length), sizeof(char));
   
   memcpy((*topology), (char *) sge_dstring_get_string(&d_topology), (*length));

   return true;
}

/****** lgroups/generate_chipID_coreID_matrix() ********************************
*  NAME
*     generate_chipID_coreID_matrix() -- ??? 
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
static bool generate_chipID_coreID_matrix(int*** matrix, int* length) 
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


/****** lgroups/get_amount_of_sockets_from_matrix() ****************************
*  NAME
*     get_amount_of_sockets_from_matrix() -- ??? 
*
*  SYNOPSIS
*     int get_amount_of_sockets_from_matrix(const int** matrix, const int 
*     length) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const int** matrix - ??? 
*     const int length   - ??? 
*
*  RESULT
*     int - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: get_amount_of_sockets_from_matrix() is not MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int get_amount_of_sockets_from_matrix(const int** matrix, const int length)
{
   int amount = 0;
   int* chip_ids = NULL;

   /* we don't care about the actual chip_ids here */
   get_chip_ids_from_matrix(matrix, length, &chip_ids, &amount);

   free(chip_ids);

   return amount;
}

/****** lgroups/get_chip_ids_from_matrix() *************************************
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
static int get_chip_ids_from_matrix(const int** matrix, const int length, 
   int** chip_ids, int* amount)
{
   return get_ids_from_matrix(matrix, length, 0, chip_ids, amount);
}



/****** lgroups/get_core_ids_from_matrix() *************************************
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
*     const int** matrix - ??? 
*     const int length   - ??? 
*     int** core_ids     - ??? 
*     int* amount        - ??? 
*
*  RESULT
*     int - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: get_core_ids_from_matrix() is not MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int get_core_ids_from_matrix(const int** matrix, const int length, 
   int** core_ids, int* amount)
{
   return get_ids_from_matrix(matrix, length, 1, core_ids, amount);
}


/****** lgroups/get_ids_from_matrix() ******************************************
*  NAME
*     get_ids_from_matrix() -- ??? 
*
*  SYNOPSIS
*     int get_ids_from_matrix(const int** matrix, const int length, const int 
*     which_ID, int** ids, int* amount) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const int** matrix - ??? 
*     const int length   - ??? 
*     const int which_ID - ??? 
*     int** ids          - ??? 
*     int* amount        - ??? 
*
*  RESULT
*     int - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: get_ids_from_matrix() is not MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int get_ids_from_matrix(const int** matrix, const int length, 
   const int which_ID,  int** ids, int* amount) {

   /* go through the matrix and count the amount of different core_ids
         or chip_ids */
   int i = 0, j = 0;
   int nr_different_ids= 0;
   int found = 0;

   /* only allow 0 (chip_id) or 1 (core_id) for which_ID */
   if (which_ID != 0 && which_ID != 1) {
      return -1;
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

   return nr_different_ids;
}

/****** lgroups/get_amount_of_threads_from_matrix() ****************************
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


/****** lgroups/get_amount_of_core_or_threads_from_matrix() ********************
*  NAME
*     get_amount_of_core_or_threads_from_matrix() -- ??? 
*
*  SYNOPSIS
*     int get_amount_of_core_or_threads_from_matrix(const int** matrix, const 
*     int length, int core, int** core_or_threads, int* size) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const int** matrix    - ??? 
*     const int length      - ??? 
*     int core              - ??? 
*     int** core_or_threads - ??? 
*     int* size             - ??? 
*
*  RESULT
*     int - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: get_amount_of_core_or_threads_from_matrix() is not MT safe 
*
*  BUGS
*     ??? 
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
      is_different_id(-1);

      /* go through the matrix */
      for (j = 0; j < length; j++) {
         if (core == 1) {
            /* counting cores per socket (amount of *same* socket_ids) */
            if (matrix[j][0] == ids[i]) { 
               /* count only *same* socket_ids */
               if (is_different_id(matrix[j][0]) == 0) {
                  amount++;
               }   
            }
         } else {
            /* counting threads per core (amount of *same* core_ids) */
            if (matrix[j][1] == ids[i]) { 
               /* count only *same* core_ids */
               if (is_different_id(matrix[j][1]) == 0) {
                  amount++;
               }   
            }
         }
      }
      /* save the amount of counted cores or threads in the vector */
      (*core_or_threads)[i] = amount;
   }
   
   /* free in subfunction allocated memory */
   free(ids);
   
   /* reset the ID counter function */
   is_different_id(-1);

   /* return the amount of sockets or cores we have found (size of array)*/
   return ids_length;
}

/****** lgroups/get_amount_of_cores_from_matrix() ******************************
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


/****** sge_binding/is_different_id() **********************************************
*  NAME
*     is_different_id() -- ??? 
*
*  SYNOPSIS
*     int is_different_id(const int id) 
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
*     MT-NOTE: is_different_id() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int is_different_id(const int id) 
{

   /* if ID is not available, add it otherwise return 1 */
   /* different_ids, different_id_vector are static */
   /* if id < 0 : delete all ids collected so far */
   const int MAX_ID_SIZE = 10;
   static int different_ids = 0;
   static int* different_id_vector = NULL;
   /* counter */
   int i = 0;
   /* do we have the id already? */
   int found = 0;

   if (id < 0) {
      /* reset everything */
      different_ids = 0;
      if (different_id_vector != NULL) {
         free(different_id_vector);
         different_id_vector = NULL;
      }
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
      free(cores);
      for (i = 0; i < length; i++) {
         free(matrix[i]);
         matrix[i] = NULL;
      }
      free(matrix);
      matrix = NULL;
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
      free(cores);
      for (i = 0; i < length; i++) {
         free(matrix[i]);
         matrix[i] = NULL;
      }
      free(matrix);
      matrix = NULL;
   }

   if (sockets_total <= 0) {
      /* default case: we have one socket */
      sockets_total = 1;
   }

   return sockets_total;
}


/****** sge_binding/get_processor_ids_solaris() ********************************
*  NAME
*     get_processor_ids_solaris() -- ??? 
*
*  SYNOPSIS
*     static bool get_processor_ids_solaris(const int** matrix, const int 
*     length, const int logical_socket_number, const int logical_core_number, 
*     int** pr_ids, int* pr_length) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const int** matrix              - ??? 
*     const int length                - ??? 
*     const int logical_socket_number - ??? 
*     const int logical_core_number   - ??? 
*     int** pr_ids                    - ??? 
*     int* pr_length                  - ??? 
*
*  RESULT
*     static bool - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: get_processor_ids_solaris() is not MT safe 
*
*  BUGS
*     ??? 
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

   if (matrix == NULL || length == 0) {
      return false;
   }

   /* map the logical numbers (0...n) to the system internal numbers (with holes etc.) */
   chip_id = get_chip_id_from_logical_socket_number_solaris(matrix, length, logical_socket_number);
   core_id = get_core_id_from_logical_core_number_solaris(matrix, length, chip_id, logical_core_number);

   /* get all processor ids with the same chip_ids */ 
   
   for (i = 0; i < length; i++) {
      /* matrix is: chip_id core_id processor_id */
      if (matrix[i][0] == chip_id && matrix[i][1] == core_id) {
         /* the third entry should be the processor id */
         core_ids[amount_of_core_ids] = matrix[i][2];
         amount_of_core_ids++;
      }
   }
 
   /* return the array the with core ids */
   /* TODO DG free? */
   (*pr_ids) = (int*) calloc(amount_of_core_ids, sizeof(int));
   for (i = 0; i < amount_of_core_ids; i++) {
      (*pr_ids)[i] = core_ids[i];
   }
   *pr_length = amount_of_core_ids;

   return true;
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
*              (because of is_different_id)
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
   is_different_id(-1);

   /* go through the resource matrix and get the 'logical_socket_number'th chip_id */
   for (i = 0; i < length; i++) {
      if (is_different_id(matrix[i][0]) == 1) {
         /* detected a chip_id not seen before */
         socket_number++;
      }
      if ((socket_number-1) == logical_socket_number) {
         /* free memory allocated in sub-function */
         is_different_id(-1);
         /* return the chip_id */
         return matrix[i][0];
      }
   }
   
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
*              (because of is_different_id())
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
      return -2;
   }

   if (length == 0 || chip_id < 0 || logical_core_number < 0) {
      /* input parameters are not correct */
      return -3;
   }
   
   /* be sure that no ids are left */
   is_different_id(-1);

   for (i = 0; i < length; i++) {
      /* check if this entry is on the same chip */
      if (matrix[i][0] == chip_id) {

         /* check how many different core_ids we found so far */
         if (is_different_id(matrix[i][1]) == 1) {
            core_number++;
         }
         if ((core_number-1) == logical_core_number) {
            /* free allocated memory in su:1083
            b-function */
            is_different_id(-1);
            /* report the core_id */
            return matrix[i][1];
         }
      }
   }

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


/****** sge_binding/getStridingFirstSocketFirstCore() **************************
*  NAME
*     getStridingFirstSocketFirstCore() -- Checks if and where striding would fit.
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
         /* DG TODO save with mutex */ 
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


static int getMaxThreadsFromTopologyString(const char* topology) {
   int i = 0; 
   int threads = 0;
   
   /* maximum amount of cores found per socket */
   int max_threads = 0;
   
   if (topology == NULL) {
      /* DG TODO delete dummy calls */
      getMaxCoresFromTopologyString(NULL);
      getMaxThreadsFromTopologyString(NULL);
      return 0;
   } 

   while (topology[i++] != '\0') {
      
      if (topology[i] == 'S') {
         /* new socket detected - reset thread counter */
         threads = 0;
      } else if (topology[i] == 'C') {
         /* new core detected   - reset thread counter */
         threads = 0;
      } else if (topology[i] == 'T') {
         /* count thread */
         threads++;
         if (threads > max_threads) {
            /* we found more cores on this socket than we already have */
            max_threads = threads;
         }   
      }
   }

   return max_threads;
}


static int getMaxCoresFromTopologyString(const char* topology) {
   int i = 0; 
   int cores = 0;
   /* maximum amount of cores found per socket */
   int max_cores = 0;
   
   if (topology == NULL) {
      return 0;
   } 
   
   while (topology[i++] != '\0') {
      
      if (topology[i] == 'S') {
         /* new socket detected - reset core counter */
         cores = 0;
      } else if (topology[i] == 'C') {
         /* count core */
         cores++;
         if (cores > max_cores) {
            /* we found more cores on this socket than we already have */
            max_cores = cores;
         }   
      }
   }

   return max_cores;
}


static bool go_to_next_core(const char* topology, const int pos, int* new_pos) 
{

   /* the current position is taken into account */
   int i = pos + 1;

   if (topology == NULL || i < 0) {
      return false;
   }

   while (topology[i] != '\0') {
      /* find next core */
      if (topology[i] == 'C' || topology[i] == 'c') {
         /* we've found the next core   */
         *new_pos = i;
         return true;
      }
      /* go to next character in string */
      i++;
   }
   
   return false;
}


bool write_binding_file_striding(const int first_socket, const int first_core, 
   const int amount, const int stepsize, const char* psetid, 
   const u_long32 job_id, const u_long32 ja_task_id, const char *pe_task_id)
{
   
   /* get the current topology                           */ 
   char* topology = NULL;
   /* already accounted cores                            */
   int accounted_cores = 0;
   /* for winding forward to first_socket and first_core */
   int found_sockets = 0;
   int found_cores   = 0;
   /* for stepping over cores                            */
   int tmp_cores = 0;
   /* current position in string */ 
   int i = 0;
   /* topology length */
   int topology_length = 0;


   if (stepsize < tmp_cores || first_socket < 0 || first_core < 0 || amount < 0) {
      /* wrong input parameter */
      return false;
   }

   if (get_execd_topology(&topology, &topology_length) != true) {
      /* was unable to retrieve the current topology */
      return false;
   }
   
   /* go to first socket */ 
   while (topology[i] != '\0' && i < topology_length && found_sockets < first_socket) {
      if (topology[i] == 'S') {
         found_sockets++;
      }
      i++;
   }
   /* go to first core */ 
   while (topology[i] != '\0' && i < topology_length && found_cores < first_core) {
      if (topology[i] == 'C') {
         found_cores++;
      }
      i++;
   }
   
   /* modify the string according the striding settings */
   while (topology[i] != '\0' && i < topology_length && accounted_cores < amount) {
      /* account next core                             */ 
      /* jump ahead to next one depending on step size */
      if (stepsize > tmp_cores) {
         /* go on to next core */
         if (go_to_next_core(topology, i, &i) == false) {
            /* no more cores found */
            free(topology);
            return false;
         }
         /* increase the number of cores counted already */
         tmp_cores++;
      }
      tmp_cores = 0;

      /* account */
      topology[i] = 'c';
      accounted_cores++; 
   }

   /* write string to file                              */
   if (accounted_cores == amount) {
      /* write binding file into active jobs directory  */
      return create_binding_file(topology, psetid, job_id, ja_task_id, pe_task_id);

   } else {
      /* didn't found the amount of cores needed to account 
         (this could be only possible when the topology string 
          changed between function calls) */
      return false;
   }
}


static bool create_binding_file(const char* topology, const char* psetid, 
   const u_long32 job_id, const u_long32 ja_task_id, const char *pe_task_id) 
{
   /* path to binding file */
   dstring fname  = DSTRING_INIT;
   /* pointer to binding file */
   FILE *fp;
   /* filename */
   const char* filename = NULL;

   /* get the path to the binding file */
   sge_get_active_job_file_path(&fname, job_id, ja_task_id, pe_task_id, 
                                "binding");

   filename = sge_dstring_get_string(&fname);

   /* open file or create file         */ 
   fp = fopen(filename, "w");

   if (!fp) {
      /* can't open binding file       */
      return false;
   }
   
   /* write topology string and psetid to "binding" file in active_jobs dir */
   fprintf(fp, "%s;%s", topology, psetid);
   
   fclose(fp);

   return true;
}

#if defined(SOLARISAMD64) || defined(SOLARIS86)

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
   
   for (i = 0; i < topology_length; i++) {
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
#endif


bool initialize_topology() {
   
   /* this is done when execution daemon starts        */
   /* does the scheduler also needs that ?             */
   /* assumption: logical_used_topology is not set yet */

   if (logical_used_topology != NULL) {
      /* if we have already such a string: delete it   */
      free(logical_used_topology);
      logical_used_topology_length = 0;
   }

   if (get_execd_topology(&logical_used_topology, &logical_used_topology_length)) {
      return true;
   }
   
   return false;
}


/* ---------------------------------------------------------------------------*/
/*               End of bookkeeping of cores in use by SGE                    */ 
/* ---------------------------------------------------------------------------*/

