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

#****** sge_sharetree/add_sharetree_node() *******************************************
#  NAME
#     add_sharetree_node() -- add sharetree node project user1 shares1 user2 shares2
#
#  SYNOPSIS
#     add_sharetree_node { project user1 shares1 user2 shares2 {output_var result} {on_host ""} {as_user ""}  {raise_error 1}
#     }
#
#  FUNCTION
#     Calls qconf -astnode project to add user1, user2 to project with shares1,  shares2
#
#  INPUTS
#     output_var      - result will be placed here
#     project         - project for which  we wish to see sharetree; 
#     user1           - user1   for which  we wish to chage shares; 
#     shares1         - shares   for user1;
#     user2           - user2   for which  we wish to chage shares; 
#     shares2        - shares   for user12
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
proc add_sharetree_node {project user1 shares1 user2 shares2 {output_var result} {on_host ""} {as_user ""} {raise_error 1}} {

   upvar $output_var out

   # clear output variable
   if {[info exists out]} {
      unset out
   }

   set ret 0
   set result [start_sge_bin "qconf" "-astnode /$project=100,/$project/$user1=$shares1,/$project/$user2=$shares2" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      parse_simple_record result out
   } else {
      set ret [add_sharetree_node_error $result $project $user1 $shares1 $user2 $shares2 $raise_error]
   }

   return $ret

}

#****** sge_sharetree/add_sharetree_node_error() ***************************************
#  NAME
#     add_sharetree_node_error() -- error handling for add_sharetree_node
#
#  SYNOPSIS
#     add_sharetree_node_error { result project user1 shares1 user2 shares2 raise_error }
#
#  FUNCTION
#     Does the error handling for add_sharetree_node.
#     Translates possible error messages of qconf -astnode,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     add_sharetree_node. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     project     - project for which qconf -astnode has been called
#     user1           - user1   for which  we wish to chage shares;
#     shares1         - shares   for user1;
#     user2           - user2   for which  we wish to chage shares;
#     shares2        - shares   for user2
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for add_sharetree_node function:
#      -1: "wrong_calendar" is not a calendar
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc add_sharetree_node_error {result project user1 shares1 user2 shares2 raise_error} {
 
   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_CALENDAR_XISNOTACALENDAR_S $calendar]

   # we might have version dependent, calendar specific error messages
   get_sharetree_error_vdep messages $project

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "add_sharetree_node" "qconf -astnode /$project=100,/$project/$user1=$shares1,/$project/$user2=$shares2" $result messages $raise_error]

   return $ret
}

 
#****** sge_sharetree/get_sharetree_list() *****************************************
#  NAME
#     get_sharetree_list() -- get a list of all sharetrees
#
#  SYNOPSIS
#     get_sharetree_list { project {output_var result} {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -sstnode /$project to retrieve all sharetrees for project
#
#  INPUTS
#     project         - project for which we do  qconf -sstnode
#     output_var      - result will be placed here
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
proc get_sharetree_list { project {output_var result} {on_host ""} {as_user ""} {raise_error 1}} {
   upvar $output_var out

   return [get_qconf_list "get_sharetree_list" "-sstnode /$project" out $on_host $as_user $raise_error]

}

#****** sge_sharetree/mod_sharetree_node() *******************************************
#  NAME
#     mod_sharetree_node() -- modify sharetree node project user1 shares1 user2 shares2
#
#  SYNOPSIS
#     mod_sharetree_node { project user1 shares1 user2 shares2 {output_var result} {on_host ""} {as_user ""}  {raise_error 1}
#     }
#
#  FUNCTION
#     Calls qconf -mstnode project to modify shares of user1, user2 in  project
#
#  INPUTS
#     output_var      - result will be placed here
#     project         - project for which  we wish to see sharetree;
#     user1           - user1   for which  we wish to chage shares;
#     shares1         - shares   for user1;
#     user2           - user2   for which  we wish to chage shares;
#     shares2        - shares   for user12
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
proc mod_sharetree_node {project user1 shares1 user2 shares2 {output_var result} {on_host ""} {as_user ""} {raise_error 1}} {

   upvar $output_var out

   # clear output variable
   if {[info exists out]} {
      unset out
   }

   set ret 0
   set result [start_sge_bin "qconf" "-mstnode /$project=100,/$project/$user1=$shares1,/$project/$user2=$shares2" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      parse_simple_record result out
   } else {
      set ret [mod_sharetree_node_error $result $project $user1 $shares1 $user2 $shares2 $raise_error]
   }

   return $ret

}

#****** sge_sharetree/mod_sharetree_node_error() ***************************************
#  NAME
#     mod_sharetree_node_error() -- error handling for mod_sharetree_node
#
#  SYNOPSIS
#     mod_sharetree_node_error { result project user1 shares1 user2 shares2 raise_error }
#
#  FUNCTION
#     Does the error handling for mod_sharetree_node.
#     Translates possible error messages of qconf -astnode,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     mod_sharetree_node. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     project     - project for which qconf -astnode has been called
#     user1           - user1   for which  we wish to chage shares;
#     shares1         - shares   for user1;
#     user2           - user2   for which  we wish to chage shares;
#     shares2        - shares   for user2
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for mod_sharetree_node function:
#      -1: "wrong_calendar" is not a calendar
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc mod_sharetree_node_error {result project user1 shares1 user2 shares2 raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_CALENDAR_XISNOTACALENDAR_S $calendar]

   # we might have version dependent, calendar specific error messages
   get_sharetree_error_vdep messages $project

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "mod_sharetree_node" "qconf -mstnode /$project=100,/$project/$user1=$shares1,/$project/$user2=$shares2" $result messages $raise_error]

   return $ret
}
#****** sge_sharetree/del_sharetree_node() *******************************************
#  NAME
#     del_sharetree_node() -- delete sharetree node project user
#
#  SYNOPSIS
#     del_sharetree_node { project user {output_var result} {on_host ""} {as_user ""}  {raise_error 1}
#     }
#
#  FUNCTION
#     Calls qconf -dstnode project to delete user in  project
#
#  INPUTS
#     output_var      - result will be placed here
#     project         - project for which  we wish to see sharetree;
#     user            - user   which  we wish to delete
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
proc del_sharetree_node {project user {output_var result} {on_host ""} {as_user ""} {raise_error 1}} {

   upvar $output_var out

   # clear output variable
   if {[info exists out]} {
      unset out
   }

   set ret 0
   set result [start_sge_bin "qconf" "-dstnode /$project/$user" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      parse_simple_record result out
   } else {
      set ret [del_sharetree_node_error $result $project $user $raise_error]
   }

   return $ret

}

#****** sge_sharetree/del_sharetree_node_error() ***************************************
#  NAME
#     del_sharetree_node_error() -- error handling for del_sharetree_node
#
#  SYNOPSIS
#     del_sharetree_node_error { result project user raise_error }
#
#  FUNCTION
#     Does the error handling for del_sharetree_node.
#     Translates possible error messages of qconf -dstnode,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     del_sharetree_node. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     project     - project for which qconf -astnode has been called
#     user        - user   which  we wish to delete
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for del_sharetree_node function:
#      -1: "wrong_user" is not a user
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc del_sharetree_node_error {result project user raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_CALENDAR_XISNOTACALENDAR_S $calendar]

   # we might have version dependent, calendar specific error messages
   get_sharetree_error_vdep messages $project

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "del_sharetree_node" "qconf -dstnode /$project/$user" $result messages $raise_error]

   return $ret
}

