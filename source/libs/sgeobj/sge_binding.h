#ifndef __SGE_BINDING_H
#define __SGE_BINDING_H

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
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "basis_types.h"
#include "sge_dstring.h"
#include "err_trace.h"
#include "sge_string.h"
#include "err_trace.h"
#include "cull_list.h"
#include "sge_object.h"
#include "sge_binding_BN_L.h"
#include "sge_spool.h"
#include "sge_binding_hlp.h"

#if defined(LINUX)
#  include <dlfcn.h>
#endif

#if defined(PLPA_LINUX) 
#  include "plpa.h"
#endif
 
/* functions related to get load values for execd (see load_avg.c) */

/* get the amount of cores available on the execution host */ 
int get_execd_amount_of_cores(void);

/* get the amount of sockets of the execution host */
int get_execd_amount_of_sockets(void);

/* get the topology string with all cores installed on the system */
bool get_execd_topology(char** topology, int* length);

/* get the topology string where all cores currently in use are marked */
bool get_execd_topology_in_use(char** topology);

#if defined(PLPA_LINUX) || defined(SOLARISAMD64) || defined(SOLARIS86)
bool account_job(const char* job_topology);
 
bool binding_set_striding(int first_socket, int first_core, int amount_of_cores,
      int offset, int stepsize, char** reason);

bool binding_one_per_socket(int first_socket, int amount_of_sockets, int n);

bool binding_n_per_socket(int first_socket, int amount_of_sockets, int n);

bool binding_explicit_exctract_sockets_cores(const char* parameter, int** list_of_sockets, 
   int* samount, int** list_of_cores, int* camount);

bool binding_explicit_check_and_account(const int* list_of_sockets, const int samount, 
   const int* list_of_cores, const int score, char** topo_used_by_job, 
   int* topo_used_by_job_length);

bool get_linear_automatic_socket_core_list_and_account(const int amount, 
      int** list_of_sockets, int* samount, int** list_of_cores, int* camount, 
      char** topo_by_job, int* topo_by_job_length);

/* functions related to get load values for execd (see load_avg.c) */
/* get the amount of cores available on the execution host */ 
int getExecdAmountOfCores(void);

/* get the amount of sockets of the execution host */
int getExecdAmountOfSockets(void);

/* get the topology string with all cores installed on the system */
bool get_execd_topology(char** topology, int* length);

/* get the topology string where all cores currently in use are marked */
bool get_execd_topology_in_use(char** topology);

/* function for determining the binding */
bool get_striding_first_socket_first_core_and_account(const int amount, const int stepsize,
   const int start_at_socket, const int start_at_core, const bool automatic,
   int* first_socket, int* first_core,
   char** accounted_topology, int* accounted_topology_length);

/* for initializing used topology on execution daemon side */
bool initialize_topology(void);

/* check if core can be used */
bool topology_is_in_use(const int socket, const int core);

/* for initializing used topology on execution daemon side */
bool initialize_topology(void);

/* free cores on execution host which were used by a job */
bool free_topology(const char* topology, const int topology_length);

#endif

#if defined(SOLARISAMD64) || defined(SOLARIS86)

int create_processor_set_striding_solaris(const int first_socket,
   const int first_core, const int amount, const int step_size, 
   const binding_type_t type, char** env);

int create_processor_set_explicit_solaris(const int* list_of_sockets,
   const int samount, const int* list_of_cores, const int camount,
   const binding_type_t type, char** env);

/* matrix represents internal kstat structure */
bool generate_chipID_coreID_matrix(int*** matrix, int* length);

/* frees the memory allocated by the topology matrix */
void free_matrix(int** matrix, const int length);
#endif

bool
binding_print_to_string(const lListElem *this_list, dstring * string);

bool
binding_parse_from_string(lListElem *this_elem, lList **answer_list, dstring *string);

bool
binding_type_to_string(binding_type_t type, dstring *string);

#endif /* __SGE_BINDING_H */
