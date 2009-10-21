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

/* this code is used by shepherd */
#include <ctype.h>
#include "sgermon.h"
#include "sge_string.h"

#include <pthread.h>
#include "sge_mtutil.h"
#include "sge_log.h"
#include "uti/sge_binding_hlp.h"

#if defined(SOLARISAMD64) || defined(SOLARIS86)
#  include <sys/processor.h>
#  include <sys/types.h>
#  include <sys/pset.h>
#endif 

#if defined(PLPA_LINUX)
/* module global variables */
/* local handle for daemon in order to access PLPA library */
#endif 


/****** sge_binding_hlp/parse_binding_parameter_string() ***********************
*  NAME
*     parse_binding_parameter_string() -- Parses binding parameter string. 
*
*  SYNOPSIS
*     bool parse_binding_parameter_string(const char* parameter, u_long32* 
*     type, dstring* strategy, int* amount, int* stepsize, int* firstsocket, 
*     int* firstcore, dstring* socketcorelist, dstring* error) 
*
*  FUNCTION
*     Parses binding parameter string and returns the values of the parameter.
*     Please check output values in dependency of the strategy string.
*
*  INPUTS
*     const char* parameter   - binding parameter string 
*
*  OUTPUT 
*     u_long32* type          - type of binding (pe = 0| env = 1|set = 2)
*     dstring* strategy       - binding strategy string
*     int* amount             - amount of cores to bind to 
*     int* stepsize           - step size between cores (or -1)
*     int* firstsocket        - first socket to use (or -1)
*     int* firstcore          - first core to use (on "first socket") (or -1)
*     dstring* socketcorelist - list of socket,core pairs with prefix explicit or NULL
*     dstring* error          - error as string in case of return false
*
*  RESULT
*     bool - true in case parsing was successful false in case of errors
*
*  NOTES
*     MT-NOTE: parse_binding_parameter_string() is MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool parse_binding_parameter_string(const char* parameter, binding_type_t* type, 
      dstring* strategy, int* amount, int* stepsize, int* firstsocket, 
      int* firstcore, dstring* socketcorelist, dstring* error)
{
   bool retval = true;

   if (parameter == NULL) {
      sge_dstring_sprintf(error, "input parameter was NULL");
      return false;
   }
   
   /* check the type [pe|env|set] (set is default) */
   if (strstr(parameter, "pe ") != NULL) {
      *type = BINDING_TYPE_PE;
   } else if (strstr(parameter, "env ") != NULL) {
      *type = BINDING_TYPE_ENV;
   } else {
      *type = BINDING_TYPE_SET;   
   }

   if (strstr(parameter, "linear") != NULL) {

      *amount = binding_linear_parse_amount(parameter);

      if (*amount  < 0) {
         /* couldn't parse amount of cores */
         sge_dstring_sprintf(error, "couldn't parse amount (linear)");
         return false;
      }

      *firstsocket = binding_linear_parse_socket_offset(parameter);
      *firstcore   = binding_linear_parse_core_offset(parameter);
      
      if (*firstsocket < 0 || *firstcore < 0) {
         /* couldn't find start <socket,core> -> must be determined 
            automatically */
         sge_dstring_sprintf(strategy, "linear_automatic");

         /* this might be an error on shepherd side only */
         *firstsocket = -1;
         *firstcore = -1;
      } else {
         sge_dstring_sprintf(strategy, "linear");
      }
      
      /* set step size to dummy */ 
      *stepsize = -1;
      
   } else if (strstr(parameter, "striding") != NULL) {
      
      *amount = binding_striding_parse_amount(parameter);
      
      if (*amount  < 0) {
         /* couldn't parse amount of cores */
         sge_dstring_sprintf(error, "couldn't parse amount (striding)");
         return false;
      }
  
      *stepsize = binding_striding_parse_step_size(parameter);

      if (*stepsize < 0) {
         sge_dstring_sprintf(error, "couldn't parse stepsize (striding)");
         return false;
      }

      *firstsocket = binding_striding_parse_first_socket(parameter);
      *firstcore = binding_striding_parse_first_core(parameter);
   
      if (*firstsocket < 0 || *firstcore < 0) {
         sge_dstring_sprintf(strategy, "striding_automatic");   

         /* this might be an error on shepherd side only */
         *firstsocket = -1;
         *firstcore = -1;
      } else {
         sge_dstring_sprintf(strategy, "striding");   
      }

   } else if (strstr(parameter, "explicit") != NULL) {

      if (binding_explicit_has_correct_syntax(parameter) == false) {
         sge_dstring_sprintf(error, "couldn't parse <socket>,<core> list (explicit)");
         retval = false;   
      } else {
         sge_dstring_sprintf(strategy, "explicit");
         /* explicit:<socket>,<core>:... */
         if (socketcorelist == NULL) {
            sge_dstring_sprintf(error, "BUG detected: DSTRING NOT INITIALIZED");
            retval = false;  
         } else {
            char* pos = strstr(parameter, "explicit"); 
            sge_dstring_copy_string(socketcorelist, pos);
            pos = NULL;
         }   
      }   
      
   } else {
      
      /* error: no valid strategy found */
      sge_dstring_sprintf(error, "couldn't parse binding parameter (no strategy found)"); 
      retval = false;
   }
   
  return retval; 
}

#if defined(PLPA_LINUX)

/****** sge_binding_hlp/has_topology_information() *********************************
*  NAME
*     has_topology_information() -- Checks if current arch offers topology. 
*
*  SYNOPSIS
*     bool has_topology_information() 
*
*  FUNCTION
*     Checks if current architecture (on which this function is called) 
*     offers processor topology information or not.
*
*  RESULT
*     bool - true if the arch offers topology information false if not 
*
*  NOTES
*     MT-NOTE: has_topology_information() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool _has_topology_information() 
{
   dstring error = DSTRING_INIT;
   sge_dstring_free(&error);

   int has_topology = 0;
   
   if (plpa_have_topology_information(&has_topology) == 0 && has_topology == 1) {
      return true;
   }
    
   return false;
}

bool has_core_binding() 
{
   /* checks if plpa is working */
   /* TODO do it only once? */

   plpa_api_type_t api_type;

   if (plpa_api_probe(&api_type) == 0 && api_type == PLPA_PROBE_OK) {
      return true;
   }
   
   return false;
}



/****** sge_binding_hlp/has_core_binding() *****************************************
*  NAME
*     has_core_binding() -- Check if core binding system call is supported. 
*
*  SYNOPSIS
*     bool has_core_binding() 
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
*     bool - True if core binding could be done. False if not. 
*
*  NOTES
*     MT-NOTE: has_core_binding() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool _has_core_binding(dstring* error) 
{
   
   /* checks if plpa is working */
   /* TODO do it only once? */
   plpa_api_type_t api_type;

   if (plpa_api_probe(&api_type) == 0 && api_type == PLPA_PROBE_OK) {
      return true;
   }

   return false;
}

/****** sge_binding_hlp/get_total_amount_of_cores() ********************************
*  NAME
*     get_total_amount_of_cores() -- Fetches the total amount of cores on system. 
*
*  SYNOPSIS
*     int get_total_amount_of_cores() 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*
*  RESULT
*     int - Total amount of cores installed on the system. 
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
int get_total_amount_of_cores() 
{
   /* total amount of cores currently active on this system */
   int total_amount_of_cores = 0;
   
   if (has_core_binding() && _has_topology_information()) {
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


/****** sge_binding_hlp/get_amount_of_cores() **************************************
*  NAME
*     get_amount_of_cores() -- ??? 
*
*  SYNOPSIS
*     int get_amount_of_cores(int socket_number) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     int socket_number - Physical socket number starting at 0. 
*
*  RESULT
*     int - 
*
*  NOTES
*     MT-NOTE: get_amount_of_cores() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int get_amount_of_cores(int socket_number) 
{

   if (has_core_binding() && _has_topology_information()) {
      int socket_id;
      /* convert the reals socket number into the Linux socket_id */

      if (plpa_get_socket_id(socket_number, &socket_id) == 0) {
         int number_of_cores, max_core_id;
         /* now retrieve the amount of core for this socket number */
         if (plpa_get_core_info(socket_id, &number_of_cores, &max_core_id) == 0) {
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

/****** sge_binding_hlp/get_amount_of_sockets() ************************************
*  NAME
*     get_amount_of_sockets() -- Get the amount of available sockets.  
*
*  SYNOPSIS
*     int get_amount_of_sockets() 
*
*  FUNCTION
*     Returns the amount of sockets available on this system. 
*
*  RESULT
*     int - The amount of available sockets on system. 0 in case of 
*                  of an error.
*
*  NOTES
*     MT-NOTE: get_amount_of_sockets() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int get_amount_of_sockets() 
{

   if (has_core_binding() && _has_topology_information()) {
      int num_sockets, max_socket_id;
      
      if (plpa_get_socket_info(&num_sockets, &max_socket_id) == 0) {
         return num_sockets;
      } else {
         /* in case of an error we have 0 sockets */
         return 0;
      }
   }

   /* we have 0 cores in case something is wrong */
   return 0;
}

/****** sge_binding_hlp/get_processor_id() *****************************************
*  NAME
*     get_processor_id() -- Converts a logical socket and core number into a Linux internal. 
*
*  SYNOPSIS
*     int get_processor_id(int socket_number, int core_number) 
*
*  FUNCTION
*     Converts a logical socket and core number (this is beginning at 0 and 
*     without holes) into the Linux internal processor number.  
*
*  INPUTS
*     int socket_number - Socket (starting at 0) to search for core. 
*     int core_number   - Core number (starting at 0) to get id for. 
*
*  RESULT
*     int - Linux internal processor number or negative number on error. 
*
*  NOTES
*     MT-NOTE: get_processor_id() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int get_processor_id(int socket_number, int core_number) 
{
    
   if (has_core_binding() && _has_topology_information()) {
      int proc_id = -1;
      int socket_id = -1;

      if (plpa_get_socket_id(socket_number, &socket_id) != 0) {
         /* unable to retrieve Linux logical socket id */
         return -3;
      }

      if (plpa_map_to_processor_id(socket_id, core_number, &proc_id) == 0) {
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

bool get_topology_linux(char** topology, int* length)
{
   bool success = true;

   /* initialize length of topology string */
   (*length) = 0;

   int has_topology = 0;

   /* check if topology is supported via PLPA */
   if (plpa_have_topology_information(&has_topology) == 0 && has_topology == 1) {
      int num_sockets, max_socket_id;
         
      /* topology string */
      dstring d_topology = DSTRING_INIT;

      /* the topology string */ 
      sge_dstring_clear(&d_topology);

      /* build the topology string */

      if (plpa_get_socket_info(&num_sockets, &max_socket_id) == 0) {
            int num_cores, max_core_id, ctr_cores, ctr_sockets;
         char* s = "S"; /* socket */
         char* c = "C"; /* core   */

         for (ctr_sockets = 0; ctr_sockets <= max_socket_id; ctr_sockets++) {
            /* append new socket */
            sge_dstring_append_char(&d_topology, *s);
            (*length)++;

            /* for each socket get the number of cores */ 
            if (plpa_get_core_info(ctr_sockets, &num_cores, &max_core_id) == 0) {
                     
               for (ctr_cores = 0; ctr_cores <= max_core_id; ctr_cores++) {
                  sge_dstring_append_char(&d_topology, *c);
                  (*length)++;
               }
            }
         } /* for each socket */
            
         if ((*length) == 0) {
             (*topology) = "warning: couldn't count cores sockets";
             success = false;
         } else {
            /* convert d_topolgy into topology */
            (*length)++; /* we need `\0` at the end */
            /* copy element */ 

            (*topology) = (char *) calloc((*length), sizeof(char));
            memcpy((*topology), (char*) sge_dstring_get_string(&d_topology), 
                  (*length) * sizeof(char));
            }
            
            sge_dstring_free(&d_topology);

         } /* when socket information is available */ 
      else { 
         (*topology) = "warning: socket information not available!";
         success = false;
      }

   } else {
      (*topology) = "warning: host has no topology";
      success = false;
  }

   return success;
}

#endif 

/* ---------------------------------------------------------------------------*/
/*                Beginning of generic parsing functions                      */ 
/* ---------------------------------------------------------------------------*/

/****** sge_binding_hlp/binding_linear_parse_amount() ******************************
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

/****** sge_binding_hlp/bindingLinearParseSocketOffset() ***************************
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

/****** sge_binding_hlp/bindingLinearParseCoreOffset() *****************************
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

int binding_parse_type(const char* parameter)
{
   int type = 0;
   
   if (strstr(parameter, "env_") != NULL) {
      type = 1;
   } else if (strstr(parameter, "pe_") != NULL) {
      type = 2;
   }

   return type;
}


/****** sge_binding_hlp/binding_explicit_has_correct_syntax() *********************
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

/****** sge_binding_hlp/binding_explicit_exctract_sockets_cores() ******************
*  NAME
*     binding_explicit_exctract_sockets_cores() -- Extracts <socket>,<core> pairs. 
*
*  SYNOPSIS
*     bool binding_explicit_exctract_sockets_cores(const char* parameter, int** 
*     list_of_sockets, int* samount, int** list_of_cores, int* camount) 
*
*  FUNCTION
*     Extracts <socket>,<core> pairs specified in a string. 
*     The string has the format "explicit:<socket>,<core>[:<socket>,<core>]". 
*     
*
*  INPUTS
*     const char* parameter - pointer to the string  
*     int** list_of_sockets - out: array with the socket numbers
*     int* samount          - out: length of the socket number array 
*     int** list_of_cores   - out: array with the core numbers 
*     int* camount          - out: length of the core number array 
*
*  RESULT
*     bool - true in case of success otherwise false
*
*  NOTES
*     MT-NOTE: binding_explicit_exctract_sockets_cores() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
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
      
      /* increase the size of the arrays */
      *samount = *camount = 1;
      
      /* adding first socket,core pair */
      *list_of_sockets = realloc(*list_of_sockets, (*samount)*sizeof(int));
      *list_of_cores = realloc(*list_of_cores, (*camount)*sizeof(int));
      (*list_of_sockets)[(*samount) - 1] = atoi(socket);
      (*list_of_cores)[(*camount) - 1] = atoi(core);

      do {
         /* get socket number */ 
         if ((socket = sge_strtok(NULL, ",")) != NULL) {

            /* we have a socket therefore we need a core number */
            if ((core = sge_strtok(NULL, ":")) == NULL) {
               FREE(*list_of_sockets);
               FREE(*list_of_cores);
               return false;
            } else if (isdigit(*core) == 0)  {
               /* this is not a digit - we have no core found */
               FREE(*list_of_sockets);
               FREE(*list_of_cores);
               return false;
            }
            
            /* adding the next <socket>,<core> tuple */
            (*samount)++; (*camount)++; 
            (*list_of_sockets) = realloc(*list_of_sockets, (*samount)*sizeof(int));
            (*list_of_cores) = realloc(*list_of_cores, (*camount)*sizeof(int));
            (*list_of_sockets)[*samount - 1] = atoi(socket);
            (*list_of_cores)[*camount - 1] = atoi(core);
         } 
   
      } while (socket != NULL);  /* we try to continue with the next socket if possible */ 

   } else {
      /* this should not be reachable because of the pre-check */
      return false;
   }

   return true; 
}


/****** sge_binding_hlp/binding_striding_parse_first_core() ************************
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


/****** sge_binding_hlp/binding_striding_parse_first_socket() **********************
*  NAME
*     binding_striding_parse_first_socket() -- Parses the socket to begin binding on. 
*
*  SYNOPSIS
*     int binding_striding_parse_first_socket(const char* parameter) 
*
*  FUNCTION
*     Parses the "striding:" parameter string for the socket number.
*
*     The string is expected to have following syntax: 
*    
*           "striding:<amount>:<stepsize>[:<socket>,<core>]"
*
*  INPUTS
*     const char* parameter - Points to the string with the query. 
*
*  RESULT
*     int - Returns the socket number in case it could be parsed otherwise -1
*
*  NOTES
*     MT-NOTE: binding_striding_parse_first_socket() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int binding_striding_parse_first_socket(const char* parameter)
{
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


/****** sge_binding_hlp/binding_striding_parse_amount() ****************************
*  NAME
*     binding_striding_parse_amount() -- Parses the amount of cores to bind to. 
*
*  SYNOPSIS
*     int binding_striding_parse_amount(const char* parameter) 
*
*  FUNCTION
*     Parses the amount of cores to bind to out of "striding:" parameter string.
*
*     The string is expected to have following syntax: 
*    
*           "striding:<amount>:<stepsize>[:<socket>,<core>]"
*
*  INPUTS
*     const char* parameter - Points to the string with the query. 
*
*  RESULT
*     int - Returns the amount of cores to bind to otherwise -1.
*
*  NOTES
*     MT-NOTE: binding_striding_parse_amount() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int binding_striding_parse_amount(const char* parameter)
{
   /* striding:<amount>:<step-size>:[starting-socket,starting-core] */

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

/****** sge_binding_hlp/binding_striding_parse_step_size() *************************
*  NAME
*     binding_striding_parse_step_size() -- Parses the step size out of the "striding" query. 
*
*  SYNOPSIS
*     int binding_striding_parse_step_size(const char* parameter) 
*
*  FUNCTION
*     Parses the step size for the core binding strategy "striding" out of the 
*     query.
* 
*     The query string is expected to have following syntax: 
*    
*           "striding:<amount>:<stepsize>[:<socket>,<core>]"
*
*  INPUTS
*     const char* parameter - Points to the string with the query. 
*
*  RESULT
*     int - Returns the step size or -1 when it could not been parsed. 
*
*  NOTES
*     MT-NOTE: binding_striding_parse_step_size() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int binding_striding_parse_step_size(const char* parameter)
{
   /* striding:<amount>:<step-size>:  */ 
   if (parameter != NULL && strstr(parameter, "striding") != NULL) {
      /* fetch "striding:" */
      if (sge_strtok(parameter, ":") != NULL) {
         if (sge_strtok(NULL, ":") != NULL) {
            /* fetch step size */
            char* stepsize = NULL;
            if ((stepsize = sge_strtok(NULL, ":")) != NULL) {
               /* the step size must be followed by " " or ":" or "\0" 
                  in order to avoid garbage like "striding:2:0,0" */
               if ((stepsize+1) == NULL || *(stepsize+1) == ' ' || 
                     *(stepsize+1) == ':' || *(stepsize+1) == '\0') {
                  /* return step size */
                  return atoi(stepsize);
               }     
            }
         }
      }
   }
   
   /* in default case take each core */
   return -1;
}

/****** sge_binding_hlp/binding_striding_parse_step_size() *************************
*  NAME
*     binding_striding_parse_step_size() -- Parses the step size out of the "striding" query. 
*
*  SYNOPSIS
*     int binding_striding_parse_step_size(const char* parameter) 
*
*  FUNCTION
*     Parses the step size for the core binding strategy "striding" out of the 
*     query.
* 
*     The query string is expected to have following syntax: 
*    
*           "striding:<amount>:<stepsize>[:<socket>,<core>]"
*
*  INPUTS
*     const char* parameter - Points to the string with the query. 
*
*  RESULT
*     int - Returns the step size or -1 when it could not been parsed. 
*
*  NOTES
*     MT-NOTE: binding_striding_parse_step_size() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool binding_explicit_extract_sockets_cores(const char* parameter, 
   int** list_of_sockets, int* samount, int** list_of_cores, int* camount) 
{
   /* string representation of a socket number */
   char* socket = NULL;
   /* string representation of a core number */
   char* core = NULL;
   bool do_endlessly = true;

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

      while (do_endlessly) { 
         /* get socket number */ 
         if ((socket = sge_strtok(NULL, ",")) == NULL || (isdigit(*socket) == 0)) {
            break;
         }

         /* we have a socket therefore we need a core number */
         if ((core = sge_strtok(NULL, ":")) == NULL || (isdigit(*core) == 0)) {
            /* missing core number */
            return false;
         }   
         
         /* adding the next <socket>,<core> tuple */
         (*samount)++; (*camount)++; 
         (*list_of_sockets) = realloc(*list_of_sockets, (*samount)*sizeof(int));
         (*list_of_cores) = realloc(*list_of_cores, (*camount)*sizeof(int));
         (*list_of_sockets)[*samount-1] = atoi(socket);
         (*list_of_cores)[*camount-1] = atoi(core);
      }        /* we try to continue with the next socket if possible */ 
               /* if "S" or "s" is found this is because the binding string 
                  in config file is parsed and the topology used by the job "SccScc" 
                  is followed */

   } else {
      /* this should not be reachable because of the pre-check */
      return false;
   }

   return true; 
}

/****** uti/binding_hlp/binding_get_topology_for_job() ********************
*  NAME
*     binding_get_topology_for_job() -- Returns topology string. 
*
*  SYNOPSIS
*     const char *
*     binding_get_topology_for_job(const char *binding_result)
*
*  FUNCTION
*     Returns the topology string of a host where the cores are 
*     marked with lowercase letters for those cores that were
*     "bound" to a certain job.
*     
*     It is assumed the 'binding_result' parameter that is
*     passed to this function was previously returned by
*     create_binding_strategy_string_linux()
*     create_binding_strategy_string_solaris()
*
*  INPUTS
*     const char *binding_result - string returned by
*                         create_binding_strategy_string_linux() 
*                         create_binding_strategy_string_solaris() 
*
*  RESULT
*     const char * - topology string like "SCc"
*******************************************************************************/
const char *
binding_get_topology_for_job(const char *binding_result) {
   const char *topology_result = NULL;

   if (binding_result != NULL) {
      /* find test after last colon character including this character */
      topology_result = strrchr(binding_result, ':');

      /* skip colon character */
      if (topology_result != NULL) {
         topology_result++;
      }
   }
   return topology_result;
}

