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


#****** sge_users/get_manager_list() *****************************************
#  NAME
#     get_manager_list() -- get the list of managers
#
#  SYNOPSIS
#     get_manager_list { {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -sm to retrieve a list of managers
#
#  INPUTS
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
proc get_manager_list { {on_host ""} {as_user ""} {raise_error 1}} {

   return [get_qconf_list "get_manager_list" "-sm" out $on_host $as_user $raise_error]

}
#****** sge_users/add_manager() *****************************************
#  NAME
#     add_manager() -- add manager
#
#  SYNOPSIS
#     add_manager {manager {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -am $manager to add manager
#
#  INPUTS
#     manager         - manager to be added by qconf -am
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
proc add_manager {manager  {on_host ""} {as_user ""} {raise_error 1}} {

   return [get_qconf_list "get_manager_list" "-am $manager" out $on_host $as_user $raise_error]

}

#****** sge_users/del_manager() *****************************************
#  NAME
#     del_manager() -- delete manager
#
#  SYNOPSIS
#     del_manager {manager {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -dm $manager to add manager
#
#  INPUTS
#     manager         - manager to be deleted by qconf -dm
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
proc del_manager {manager {on_host ""} {as_user ""} {raise_error 1}} {

   return [get_qconf_list "get_manager_list" "-dm $manager" out $on_host $as_user $raise_error]

}


#****** sge_users/get_operator_list() *****************************************
#  NAME
#     get_operator_list() -- get the list of operators
#
#  SYNOPSIS
#     get_operator_list { {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -so to retrieve a list of operators
#
#  INPUTS
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
proc get_operator_list { {on_host ""} {as_user ""} {raise_error 1}} {

   return [get_qconf_list "get_operator_list" "-so" out $on_host $as_user $raise_error]

}

#****** sge_users/get_userset_list() *****************************************
#  NAME
#     get_userset_list() -- get the list of usersets
#
#  SYNOPSIS
#     get_userset_list {output_var {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -sul to retrieve a list of usersets
#
#  INPUTS
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
proc get_userset_list {output_var {on_host ""} {as_user ""} {raise_error 1}} {
   upvar $output_var out
   return [get_qconf_list "get_userset_list" "-sul" out $on_host $as_user $raise_error]
}

#****** sge_users/get_ulist() ******************************************
#  NAME
#     get_ulist() -- Get template user list array
#
#  SYNOPSIS
#     get_ulist { userlist array   {raise_error 1}}
#
#  FUNCTION
#     Create user in userlist
#
#  INPUTS
#     userlist     - user name to be modifies
#     array        - array containing the changed attributes.
#     {fast_add 1} - 0: modify the attribute using qconf -mckpt,
#                  - 1: modify the attribute using qconf -Mckpt, faster
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     {raise_error 1} - raise an error condition on error (default), or just
#                       output the error message to stdout
#
#  RESULT
#
#  COMMENT 
#   Use this construct due to some issues with using get_qconf_list
#
#*******************************************************************************

proc get_ulist { userlist change_array {raise_error 1}} {
  global ts_config
  global CHECK_ARCH CHECK_OUTPUT

  upvar $change_array chgar

  set result [start_sge_bin "qconf" "-su $userlist" "" "" ]

  if { $prg_exit_state != 0 } {
     add_proc_error "get_ulist" "-1" "qconf error: $result" $raise_error
     return
  }

  # split each line as listelement
  set help [split $result "\n"]

  foreach elem $help {
     set id [lindex $elem 0]
     set value [lrange $elem 1 end]
     set value [replace_string $value "{" ""]
     set value [replace_string $value "}" ""]

     if { $id != "" } {
        set chgar($id) $value
     }
  }
}


#****** sge_users/mod_userlist() ******************************************
#  NAME
#     mod_userlist() -- Modify user list
#
#  SYNOPSIS
#     mod_userlist { userlist array  {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}}
#
#  FUNCTION
#     Modifies user in userlist
#
#  INPUTS
#     userlist     - user name to be modifies
#     array        - array containing the changed attributes.
#     {fast_add 1} - 0: modify the attribute using qconf -mckpt,
#                  - 1: modify the attribute using qconf -Mckpt, faster
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     {raise_error 1} - raise an error condition on error (default), or just
#                       output the error message to stdout
#
#  RESULT
#
#*******************************************************************************
proc mod_userlist { userlist array {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}} {
   global ts_config
   global CHECK_ARCH CHECK_OUTPUT CHECK_USER

   upvar $array current_ul

# Modify userlist from file
   if { $fast_add } {
      get_ulist $userlist old_ul
      foreach elem [array names current_ul] {
         set old_ul($elem) "$current_ul($elem)"
      }

      set tmpfile [dump_array_to_tmpfile old_ul]
      set result [start_sge_bin "qconf" "-Mu $tmpfile" $on_host $as_user ]


      if {$prg_exit_state == 0} {
         set ret 0
      } else {
         set ret [mod_userlist_error $result $userlist $tmpfile $raise_error]
      }

   } else {

      # do the work via vi
      set vi_commands [build_vi_command current_ul]
      set args "-mu $userlist"

      set MODIFIED [translate_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS "*" "*" "$userlist" "userset" ]
      set NOT_MODIFIED [translate_macro MSG_FILE_NOTCHANGED ]
      set ALREADY_EXISTS [ translate_macro MSG_SGETEXT_ALREADYEXISTS_SS "*" "*"]
      set UNKNOWN_ATTRIBUTE [ translate_macro MSG_UNKNOWNATTRIBUTENAME_S "*" ]
      set NOTULONG [ translate_macro MSG_OBJECT_VALUENOTULONG_S "*" ]

      set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" $args $vi_commands $MODIFIED $ALREADY_EXISTS $NOT_MODIFIED $NOTULONG]
      if { $result == -1 } { 
         add_proc_error "mod_userlist" -1 "timeout error" $raise_error
      } elseif { $result == -2 } { 
         add_proc_error "mod_userlist" -1 "already exists" $raise_error
      } elseif { $result == -3 } { 
         add_proc_error "mod_userlist" -1 "not modified " $raise_error
      } elseif { $result == -4 } { 
         add_proc_error "mod_userlist" -1 "not  u_long32 value" $raise_error
      } elseif { $result != 0  } { 
         add_proc_error "mod_userlist" -1 "could not modify userlist " $raise_error
      }

      set ret $result

   }

   return $ret
}

#****** sge_users/mod_userlist_error() ***************************************
#  NAME
#     mod_userlist_error() -- error handling for mod_userlist
#
#  SYNOPSIS
#     mod_userlist_error {result userlist tmpfile raise_error }
#
#  FUNCTION
#     Does the error handling for mod_user.
#     Translates possible error messages of qconf -Mu,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     mod_userlist. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     userlist    - object qconf is modifying
#     tmpfile     - temp file for qconf -Mattr
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for mod_userlist function:
#      -1: "wrong_attr" is not an attribute
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc mod_userlist_error {result userlist tmpfile raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1 -2"
   set messages(-1) [translate_macro MSG_OBJECT_VALUENOTULONG_S "*" ]
   set messages(-2) [translate_macro MSG_SGETEXT_DOESNOTEXIST_SS "userset" $userlist ]

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "mod_user" "qconf -Mu $tmpfile " $result messages $raise_error]

   return $ret
}

#****** sge_users/get_user_list() *****************************************
#  NAME
#     get_user_list() -- get the list of users
#
#  SYNOPSIS
#     get_user_list { {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -suserl to retrieve a list of users
#
#  INPUTS
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
proc get_user_list {output_var {on_host ""} {as_user ""} {raise_error 1}} {
   # JG: TODO: we need proper error handling here.
   # If no user is defined, output of qconf -suserl is 
   # "no user list defined", return code is 0, so we end up with a user list 
   # "no", "user", "list", "defined"
   upvar $output_var out
   return [get_qconf_list "get_user_list" "-suserl" out $on_host $as_user $raise_error]
}

proc get_user {output_var user {on_host ""} {as_user ""} {raise_error 1}} {
   global ts_config CHECK_OUTPUT

   upvar $output_var out

   # clear output variable
   if {[info exists out]} {
      unset out
   }

   set ret 0
   set result [start_sge_bin "qconf" "-suser $user" $on_host $as_user]

   # parse output or raise error
   set messages(index) {-1}
   set messages(-1) [translate_macro MSG_USER_XISNOKNOWNUSER_S $user]

   if {$prg_exit_state == 0} {
      # qconf is buggy. If the user is not known, it still exits 0
      if {[string first $messages(-1) $result] >= 0} {
         add_proc_error "get_user" -1 $result $raise_error
         set ret -1
      } else {
         parse_simple_record result out
      }
   } else {
      set ret [handle_sge_errors "get_user" "qconf -suser $user" $result messages $raise_error]
   }

   return $ret
}

proc del_user {user {on_host ""} {as_user ""} {raise_error 1}} {
   global ts_config CHECK_OUTPUT

   if {$ts_config(product_type) == "sge"} {
      add_proc_error "" -3 "del_user (qconf -duser) not available for sge systems" $raise_error
      return -1
   }

   set ret 0
   set result [start_sge_bin "qconf" "-duser $user" $on_host $as_user]

   # parse output or raise error
   set messages(index) {0}
   set messages(0) [translate_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS "*" "*" $user [translate_macro MSG_OBJ_USER]]

   set ret [handle_sge_errors "del_user" "qconf -duser $user" $result messages $raise_error]

   return $ret
}

