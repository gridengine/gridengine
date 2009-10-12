#ifndef __SGE_BINDING_HLP_H
#define __SGE_BINDING_HLP_H

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

#if ( defined(LINUXAMD64) || defined(LINUX86) ) && !defined(ULINUX86_24) && !defined(LINUXIA64_24) && !defined(ULINUXAMD64_24)
#  define PLPA_LINUX
#endif

#include "sge_dstring.h"

#if defined(PLPA_LINUX)
#  include "plpa.h"
#  include <dlfcn.h>
#endif 

/* functions related for parsing command line (see parse_qsub.c) */
/* shepherd also needs them */
int binding_linear_parse_amount(const char* parameter);
int binding_linear_parse_core_offset(const char* parameter);
int binding_linear_parse_socket_offset(const char* parameter);

int binding_striding_parse_amount(const char* parameter);
int binding_striding_parse_first_core(const char* parameter);
int binding_striding_parse_first_socket(const char* parameter);
int binding_striding_parse_step_size(const char* parameter);

bool binding_explicit_extract_sockets_cores(const char* parameter, 
   int** list_of_sockets, int* samount, int** list_of_cores, int* camount); 

#if defined(PLPA_LINUX)

bool _has_topology_information(void);

bool _has_core_binding(dstring* error);

void* _get_plpa_handle(dstring* error);

void _close_plpa_handle(void);

bool get_topology_linux(char** topology, int* length);

int get_processor_id(int socket_number, int core_number);

int get_amount_of_cores(int socket_number);

int get_total_amount_of_cores(void);

int get_amount_of_sockets(void); 

bool has_core_binding(void);

void* get_plpa_handle(void);

#endif

#endif /* __SGE_BINDING_HLP_H */

