#!/vol2/TCL_TK/glinux/bin/expect
# expect script 
# test SGE/SGEEE System
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

proc get_exechost_error_vdep {messages_var host} {
   upvar $messages_var messages

   #lappend messages(index) "-3"
   #set messages(-3) [translate_macro MSG_XYZ_S $host] #; another exechost specific error message
   #set messages(-3,description) "a highlevel description of the error"    ;# optional parameter
   #set messages(-3,level) -2  ;# optional parameter: we only want to raise a warning
}

proc get_hostgroup_error_vdep {messages_var host} {
   upvar $messages_var messages

   #lappend messages(index) "-3"
   #set messages(-3) [translate_macro MSG_XYZ_S $host] #; another exechost specific error message
   #set messages(-3,description) "a highlevel description of the error"    ;# optional parameter
   #set messages(-3,level) -2  ;# optional parameter: we only want to raise a warning
}

#****** sge_host.60/get_hostgroup_tree() *******************************************
#  NAME
#     get_hostgroup_tree() -- get tree like structure of host group
#
#  SYNOPSIS
#     get_hostgroup_tree { group {output_var result}  {on_host ""} {as_user ""} {raise_error 1}
#     
#
#  FUNCTION
#     Calls qconf -shgrp_tree @allhosts to retrieve tree like structure of @allhosts group
#
#  INPUTS
#     output_var      - result will be placed here
#     group        - value of host group we wish to see 
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     {raise_error 1} - raise an error condition on error (default), or just
#                       output the error message to stdout
#
#  RESULT
#     0 on success, an error code on error.
#     For a list of error codes, see sge_procedures/get_sge_error().
#
#  SEE ALSO
#     sge_procedures/get_sge_error()
#     sge_procedures/get_qconf_list()
#*******************************************************************************
proc get_hostgroup_tree {group {output_var result} {on_host ""} {as_user ""} {raise_error 1}} {

   upvar $output_var out

   # clear output variable
   if {[info exists out]} {
      unset out
   }

   set ret 0
   set result [start_sge_bin "qconf" "-shgrp_tree $group" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      parse_simple_record result out
   } else {
      set ret [get_hostgroup_tree_error $result $group $raise_error]
   }

   return $ret

}
#****** sge_host.60/get_hostgroup_tree_error() ***************************************
#  NAME
#     get_hostgroup_tree_error() -- error handling for get_hostgroup_tree
#
#  SYNOPSIS
#     get_hostgroup_tree_error { result group raise_error }
#
#  FUNCTION
#     Does the error handling for get_hostgroup_tree.
#     Translates possible error messages of qconf -shgrp_tree,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     get_hostgroup_tree. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     group       - group for which qconf -shgrp_tree has been called
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for get_calendar function:
#      -1: "wrong_hostgroup" does not exist
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc get_hostgroup_tree_error {result group raise_error} {
 
   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_HGROUP_NOTEXIST_S $group]

   # we might have version dependent, shgrp specific error messages
   get_hostgroup_error_vdep messages $group

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "get_hostgroup_tree" "qconf -shgrp_tree $group" $result messages $raise_error]

   return $ret
}
#****** sge_host.60/get_hostgroup_resolved() *******************************************
#  NAME
#     get_hostgroup_resolved() -- get list of host group
#
#  SYNOPSIS
#     get_shgrp_resolved { group {output_var result} {on_host ""} {as_user ""} {raise_error 1}
#     
#
#  FUNCTION
#     Calls qconf -shgrp_resolved $group to retrieve list of host group
#
#  INPUTS
#     output_var      - result will be placed here
#     group           - value of host group we wish to see
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     {raise_error 1} - raise an error condition on error (default), or just
#                       output the error message to stdout
#
#  RESULT
#     0 on success, an error code on error.
#     For a list of error codes, see sge_procedures/get_sge_error().
#
#  SEE ALSO
#     sge_procedures/get_sge_error()
#     sge_procedures/get_qconf_list()
#*******************************************************************************
proc get_hostgroup_resolved {group {output_var result} {on_host ""} {as_user ""} {raise_error 1}} {

   upvar $output_var out

   # clear output variable
   if {[info exists out]} {
      unset out
   }

   set ret 0
   set result [start_sge_bin "qconf" "-shgrp_resolved $group" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      parse_simple_record result out
   } else {
      set ret [get_hostgroup_resolved_error $result $group $raise_error]
   }

   return $ret

}
#****** sge_host.60/get_hostgroup_resolved_error() ***************************************
#  NAME
#     get_hostgroup_resolved_error() -- error handling for get_hostgroup_resolved
#
#  SYNOPSIS
#     get_hostgroup_resolved_error { result group raise_error }
#
#  FUNCTION
#     Does the error handling for get_hostgroup_resolved.
#     Translates possible error messages of qconf -shgrp_resolved,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     get_hostgroup_resolved. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     group       - group for which qconf -shgrp_resolved has been called
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for get_calendar function:
#      -1: "wrong_hostgroup" does not exist
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc get_hostgroup_resolved_error {result group raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_HGROUP_NOTEXIST_S $group]

   # we might have version dependent, shgrp specific error messages
   get_hostgroup_error_vdep messages $group

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "get_hostgroup_resolved" "qconf -shgrp_resolved $group" $result messages $raise_error]

   return $ret
}

