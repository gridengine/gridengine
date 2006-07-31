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

#****** sge_host.60/get_hostgroup() *******************************************
#  NAME
#     get_hostgroup() -- get list of host group
#
#  SYNOPSIS
#     get_hostgroup { group {output_var result} {on_host ""} {as_user ""} {raise_error 1}
#
#
#  FUNCTION
#     Calls qconf -shgrp $group to retrieve list of host group
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
proc get_hostgroup {group {output_var result} {on_host ""} {as_user ""} {raise_error 1}} {

   upvar $output_var out

   # clear output variable
   if {[info exists out]} {
      unset out
   }

   set ret 0
   set result [start_sge_bin "qconf" "-shgrp $group" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      parse_simple_record result out
   } else {
      set ret [get_hostgroup_error $result $group $raise_error]
   }

   return $ret

}

#****** sge_host.60/get_hostgroup_error() ***************************************
#  NAME
#     get_hostgroup_error() -- error handling for get_hostgroup
#
#  SYNOPSIS
#     get_hostgroup_error { result group raise_error }
#
#  FUNCTION
#     Does the error handling for get_hostgroup.
#     Translates possible error messages of qconf -shgrp,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     get_hostgroup. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     group       - group for which qconf -shgrp has been called
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for get_hostgroup function:
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

#****** sge_host.60/mod_hostgroup() *******************************************
#  NAME
#     mod_hostgroup() -- Modify host group
#
#  SYNOPSIS
#     mod_hostgroup { group attribute value {fast_add 1}  {on_host ""} {as_user ""} {raise_error 1}
#
#  The array looks like this:
#
#  group_name <groupname> ;#like @allhosts
#  hostlist  host1 host2 host3 ...
#
#  FUNCTION
#     Calls qconf -Mhgrp $file to modify hostgroup, or -mhgrp 
#
#  INPUTS
#     group           - host group we wish to modify
#     attribute       - attribute we wish to modify
#     value           - value of attribute we wish to modify
#     {fast_add 1} - if not 0 the mod_hostgroup procedure will use a file for
#                    group configuration. (faster) (qconf -Mhgrp, not qconf -mhgrp)
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#
#  RESULT
#     0 on success, an error code on error.
#     For a list of error codes, see sge_procedures/get_sge_error().
#
#  SEE ALSO
#     sge_procedures/get_sge_error()
#     sge_procedures/get_qconf_list()
#*******************************************************************************
proc mod_hostgroup { group attribute value {fast_add 1} {on_host ""} {as_user ""}  {raise_error 1} } {

   global ts_config CHECK_OUTPUT
   global env CHECK_ARCH
   global CHECK_CORE_MASTER

   get_hostgroup $group old_values 
   set old_values($attribute) "$value"

   # Modify hostgroup from file?
   if { $fast_add } {
     set tmpfile [dump_array_to_tmpfile old_values]
     set ret [start_sge_bin "qconf" "-Mhgrp $tmpfile" $on_host $as_user ]
     if {$prg_exit_state == 0} {
         set result 0
      } else {
         set result [mod_hostgroup_error $ret $attribute $value $tmpfile $raise_error]
      }

   } else {
   # User vi
      set MODIFIED [translate_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS "*" "*" "@allhosts" "host group" ]
      set NOT_MODIFIED [translate_macro MSG_FILE_NOTCHANGED ]
      set ALREADY_EXISTS [ translate_macro MSG_SGETEXT_ALREADYEXISTS_SS "*" "*"]
      set UNKNOWNHOST  [translate_macro MSG_HGRP_UNKNOWNHOST "*" ]
      set UNKNOWN_ATTRIBUTE [ translate_macro MSG_UNKNOWNATTRIBUTENAME_S "$attribute" ]


      set vi_commands [build_vi_command old_values]
      set result [handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-mhgrp $group" $vi_commands $MODIFIED $ALREADY_EXISTS $UNKNOWNHOST $UNKNOWN_ATTRIBUTE $NOT_MODIFIED]

      if { $result == -1 } { 
         add_proc_error "mod_hostgroup" -1 "timeout error" $raise_error
      } elseif { $result == -2 } { 
         add_proc_error "mod_hostgroup" -1 "already exists" $raise_error 
      } elseif { $result == -3 } { 
         add_proc_error "mod_hostgroup" -1 "unknown host $value" $raise_error
      } elseif { $result == -4 } { 
         add_proc_error "mod_hostgroup" -1 "unknown attribute $attribute" $raise_error
      } elseif { $result != 0 } {
         add_proc_error "mod_hostgroup" -1 "could not modifiy hostgroup $group " $raise_error
      }
   }
  return $result

}

#****** sge_host.60/mod_hostgroup_error() ***************************************
#  NAME
#     mod_hostgroup_error() -- error handling for mod_hostgroup
#
#  SYNOPSIS
#     mod_hostgroup_error {result attribute value tmpfile raise_error }
#
#  FUNCTION
#     Does the error handling for mod_hostgroup.
#     Translates possible error messages of qconf -mattr,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     mod_hostgroup. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     attribute   - attribute we are modifying
#     value       - value    we are modifying
#     tmpfile     - temp file for -Mhgrp
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for mod_hostgroup function:
#      -1: "wrong_attr" is not an attribute
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc mod_hostgroup_error {result attribute value tmpfile raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1 -2 -3"
   set messages(-1) "error: [translate_macro MSG_UNKNOWNATTRIBUTENAME_S $attribute]"
   set messages(-2) [translate_macro MSG_HGROUP_FILEINCORRECT_S "*"]
   set messages(-3) [translate_macro MSG_HGRP_UNKNOWNHOST $value]

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "mod_hostgroup" "qconf -Mhgrp $tmpfile " $result messages $raise_error]

   return $ret
}


#****** sge_host.60/add_hostgroup() *******************************************
#  NAME
#     add_hostgroup() -- Add host group
#
#  SYNOPSIS
#     add_hostgroup { group change_array {fast_add 1} {on_host ""} {as_user ""} }
#
#  The array looks like this:
#
#  group_name @template
#  hostlist NONE
#
#  FUNCTION
#     Calls qconf -Ahgrp $file to modify hostgroup, or -ahgrp
#
#  INPUTS
#     group           - value of host group we wish to see
#     change_array - name of an array variable can contain special settings
#     {fast_add 1} - if not 0 the mod_hostgroup procedure will use a file for
#                    group configuration. (faster) (qconf -Ahgrp, not qconf -ahgrp)
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#
#  RESULT
#     0 on success, an error code on error.
#     For a list of error codes, see sge_procedures/get_sge_error().
#
#  SEE ALSO
#     sge_procedures/get_sge_error()
#     sge_procedures/get_qconf_list()
#*******************************************************************************
proc add_hostgroup {group change_array {fast_add 1} {on_host ""} {as_user ""}} {
   global ts_config CHECK_OUTPUT
   global env CHECK_ARCH
   global CHECK_CORE_MASTER

   upvar $change_array chgar

   # Modify hostgroup from file?
   if { $fast_add } {
      set tmpfile [dump_array_to_tmpfile chgar]
      set result [start_sge_bin "qconf" "-Ahgrp ${tmpfile}" $on_host $as_user ]

   } else {
   # Use vi
      set vi_commands [build_vi_command chgar]
      set CHANGED  [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_EXEC_HOSTENTRYOFXCHANGEDINEXE
CLIST_S] "*" ]
      set result [handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-ahgrp $group" $vi_commands "modified" $CHANGED]
      if { $result == -2 } {
         set result 0
      }
      if { $result != 0 } {
         add_proc_error "add_hostgroup" -1 "could not add hostgroup $group"
         set result -1
      }
   }
  return $result

}

#****** sge_host.60/del_hostgroup() ********************************************
#  NAME
#     del_hostgroup() -- delete a hostgroup
#
#  SYNOPSIS
#     del_hostgroup { group } 
#
#  FUNCTION
#     Deletes the given hostgroup.
#
#  INPUTS
#     group - name of the hostgroup
#
#  RESULT
#     0 on success, 
#     <0 on error
#
#  SEE ALSO
#     sge_host.60/add_hostgroup()
#     sge_host.60/mod_hostgroup()
#     sge_procedures/handle_sge_errors()
#*******************************************************************************
proc del_hostgroup {group} {
   global ts_config
   global CHECK_USER

   set messages(index) "0"
   set messages(0) [translate_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS $CHECK_USER "*" $group "*"]

   set output [start_sge_bin "qconf" "-dhgrp $group"]

   set ret [handle_sge_errors "del_hostgroup" "qconf -dhgrp $group" $output messages]
   return $ret
}
