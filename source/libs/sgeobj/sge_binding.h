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
#include "sge_job_JB_L.h"
#include "sge_ja_task_JAT_L.h"
#include "sge_pe_task_PET_L.h"
#include "sge_spool.h"

#if ( defined(LINUXAMD64) || defined(LINUX86) ) && !defined(ULINUX86_24) && !defined(LINUXIA64_24) && !defined(ULINUXAMD64_24)
#define PLPA_LINUX
#endif 

#if defined(LINUX)
#  include <dlfcn.h>
#endif

#if defined(PLPA_LINUX) 
#  include "plpa.h"
#endif 

/* binding strategy functions */
bool binding_set_linear(int first_socket, int first_core, int amount_of_cores, int offset);
bool binding_set_striding(int first_socket, int first_core, int amount_of_cores, int offset, int n, char** reason);
bool binding_one_per_socket(int first_socket, int amount_of_sockets, int n);
bool binding_n_per_socket(int first_socket, int amount_of_sockets, int n);
bool binding_explicit(const int* list_of_sockets, const int samount, const int* list_of_cores, const int camount);

/* functions related for parsing command line (see parse_qsub.c) */
int binding_linear_parse_amount(const char* parameter);
int binding_linear_parse_core_offset(const char* parameter);
int binding_linear_parse_socket_offset(const char* parameter);

bool binding_explicit_has_correct_syntax(const char* parameter);
bool binding_explicit_exctract_sockets_cores(const char* parameter, int** list_of_sockets, 
   int* samount, int** list_of_cores, int* camount);
bool binding_explicit_check_and_account(const int* list_of_sockets, const int samount, 
   const int* list_of_cores, const int score, char** topo_used_by_job, 
   int* topo_used_by_job_length);

int binding_striding_parse_amount(const char* parameter);
int binding_striding_parse_first_core(const char* parameter);
int binding_striding_parse_first_socket(const char* parameter);
int binding_striding_parse_step_size(const char* parameter);

/* functions related to get load values for execd (see load_avg.c) */ 
int getExecdAmountOfCores(void);
int getExecdAmountOfSockets(void);
bool get_execd_topology(char** topology, int* length);
bool get_execd_topology_in_use(char** topology);

/* function for determining the binding */
bool get_striding_first_socket_first_core_and_account(const int amount, const int stepsize,
   const int start_at_socket, const int start_at_core, const bool automatic,
   int* first_socket, int* first_core,
   char** accounted_topology, int* accounted_topology_length);

/* writes the binding to the binding file (shepherd)*/
bool write_binding_file_striding(const int first_socket, const int first_core, 
   const int amount, const int stepsize, const char* psetid,
   const u_long32 job_id, const u_long32 ja_task_id, const char *pe_task_id);

/* for initializing used topology on execution daemon side */
bool initialize_topology(void);

/* free cores on execution host which were used by a job */
bool free_topology(const char* topology, const int topology_length);

/* check if core can be used */
bool topology_is_in_use(const int socket, const int core);


/* PLPA related functions are static in c file*/


#if defined(SOLARISAMD64) || defined(SOLARIS86)
bool bind_shepherd_to_pset(int pset_id);
int create_processor_set_striding_solaris(const int first_socket,
   const int first_core, const int amount, const int step_size);
int create_processor_set_explicit_solaris(const int* list_of_sockets,
   const int samount, const int* list_of_cores, const int camount);
#endif

#endif /* __SGE_BINDING_H */
