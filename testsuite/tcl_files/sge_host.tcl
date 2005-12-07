#___INFO__MARK_BEGIN__
##########################################################################
#
#  The Contents of this file are made available subject to the terms of
#  the Sun Industry Standards Source License Version 1.2
#
#  Sun Microsystems Inc., March, 2001
#
#
#  Sun Industry Standards Source License Version 1.2
#  =================================================
#  The contents of this file are subject to the Sun Industry Standards
#  Source License Version 1.2 (the "License"); You may not use this file
#  except in compliance with the License. You may obtain a copy of the
#  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
#
#  Software provided under this License is provided on an "AS IS" basis,
#  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
#  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
#  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
#  See the License for the specific provisions governing your rights and
#  obligations concerning the Software.
#
#  The Initial Developer of the Original Code is: Sun Microsystems, Inc.
#
#  Copyright: 2001 by Sun Microsystems, Inc.
#
#  All Rights Reserved.
#
##########################################################################
#___INFO__MARK_END__

#****** sge_host/get_exechost_error() ******************************************
#  NAME
#     get_exechost_error() -- error handling for get_exechost
#
#  SYNOPSIS
#     get_exechost_error { result host raise_error } 
#
#  FUNCTION
#     Does the error handling for get_exechost.
#     Translates possible error messages of qconf -se,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     get_exechost. While the qconf call and parsing the result is 
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     host        - host for which qconf -se has been called
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for get_exechost function:
#      -1: host is not resolvable
#      -2: host is not an execution host
#     -99: other error
#
#  SEE ALSO
#     sge_host/get_exechost
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc get_exechost_error {result host raise_error} {
   global ts_config CHECK_OUTPUT

   # recognize certain error messages and return special return code
   set messages(index) "-1 -2"
   set messages(-1) [translate_macro MSG_SGETEXT_CANTRESOLVEHOST_S $host]
   set messages(-2) [translate_macro MSG_EXEC_XISNOTANEXECUTIONHOST_S $host]

   # we might have version dependent error messages
   get_exechost_error_vdep messages $host
  
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "get_exechost" "qconf -se $host" $result messages $raise_error]

   return $ret
}


#****** sge_host/get_exechost() ************************************************
#  NAME
#     get_exechost() -- get exechost properties (qconf -se)
#
#  SYNOPSIS
#     get_exechost { output_var host {on_host ""} {as_user ""} {raise_error 1} 
#     } 
#
#  FUNCTION
#     Calls qconf -se $host to retrieve exec host properties.
#
#  INPUTS
#     output_var      - TCL array for storing the result
#     host            - exechost for query
#     {on_host ""}    - execute qconf on this host
#     {as_user ""}    - execute qconf as this user
#     {raise_error 1} - do add_proc error, or only output error messages
#
#  RESULT
#     0 on success.
#     < 0 in case of errors, see sge_host/get_exechost_error()
#
#  SEE ALSO
#     sge_host/get_exechost_error()
#*******************************************************************************
proc get_exechost {output_var {host global} {on_host ""} {as_user ""} {raise_error 1}} {
   global ts_config CHECK_OUTPUT
   upvar $output_var out

   # clear output variable
   if {[info exists out]} {
      unset out
   }

   set ret 0
   set result [start_sge_bin "qconf" "-se $host" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      parse_simple_record result out
   } else {
      set ret [get_exechost_error $result $host $raise_error]
   }

   return $ret
}

proc test_get_exechost {} {
   global CHECK_OUTPUT

   set ret [get_exechost output bilbur "" "" 0]
   puts $CHECK_OUTPUT "get_exechost bilbur: $ret"

   set ret [get_exechost output "dontknowthishost" "" "" 0]
   puts $CHECK_OUTPUT "get_exechost bilbur: $ret"

   set ret [get_exechost output sowa "" "" 0]
   puts $CHECK_OUTPUT "get_exechost sowa: $ret"

   set ret [get_exechost output bilbur sowa "" 0]
   puts $CHECK_OUTPUT "get_exechost bilbur on host sowa: $ret"
}


