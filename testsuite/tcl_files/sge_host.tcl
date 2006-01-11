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
#       -1: host is not resolvable
#       -2: host is not an execution host
#     -999: other error
#
#  SEE ALSO
#     sge_host/get_exechost
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc get_exechost_error {result host raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1 -2"
   set messages(-1) [translate_macro MSG_EXEC_XISNOTANEXECUTIONHOST_S $host]
   set messages(-2) [translate_macro MSG_SGETEXT_CANTRESOLVEHOST_S $host]


   # we might have version dependent, exechost specific error messages
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
#     get_exechost {output_var host {on_host ""} {as_user ""} {raise_error 1}}
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
   global ts_config
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

#                                                             max. column:     |
#****** sge_host/set_exechost() ******
#
#  NAME
#     set_exechost -- set/change exec host configuration
#
#  SYNOPSIS
#     set_exechost { change_array host {fast_add 1} {on_host ""} {as_user ""} {raise_error 1} }
#
#  FUNCTION
#     Set the exec host configuration corresponding to the content of the
#     change_array.
#
#  INPUTS
#     change_array - name of an array variable that will be set by set_exechost
#     host         - name of an execution host
#     {fast_add 1} - 0: modify the attribute using qconf -me,
#                  - 1: modify the attribute using qconf -Me, faster
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     {raise_error 1} - raise an error condition on error (default), or just
#                       output the error message to stdout
#
#  RESULT
#     The array should look like follows:
#
#     set change_array(user_list)   "deadlineusers"
#     set change_array(load_scaling) "NONE"
#     ....
#     (every value that is set will be changed)
#
#
#     Here the possible change_array values with some typical settings:
#
#     hostname                   myhost.mydomain
#     load_scaling               NONE
#     complex_list               test
#     complex_values             NONE
#     user_lists                 deadlineusers
#     xuser_lists                NONE
#     projects                   NONE
#     xprojects                  NONE
#     usage_scaling              NONE
#     resource_capability_factor 0.000000
#
#     return value:
#     -100 :    unknown error
#     -1   :    on timeout
#        0 :    ok
#
#  EXAMPLE
#     get_exechost myconfig expo1
#     set myconfig(user_lists) NONE
#     set_exechost myconfig expo1
#
#
#  NOTES
#     ???
#
#  BUGS
#     ???
#
#  SEE ALSO
#     sge_host/get_exechost()
#*******************************
proc set_exechost { change_array host {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}} {
# the array should look like this:
#
# set change_array(load_scaling) NONE
# ....
# (every value that is set will be changed)
# hostname                   myhostname
# load_scaling               NONE
# complex_list               test
# complex_values             NONE
# user_lists                 deadlineusers
# xuser_lists                NONE
# projects                   NONE
# xprojects                  NONE
# usage_scaling              NONE
# resource_capability_factor 0.000000
#
# returns
# -1   on timeout
# 0    if ok

   global ts_config CHECK_OUTPUT
   global env CHECK_ARCH 
   global CHECK_CORE_MASTER

   upvar $change_array chgar
 
   set values [array names chgar]
   get_exechost old_values $host
 
   set vi_commands ""
   foreach elem $values {
      # this will quote any / to \/  (for vi - search and replace)
      set newVal $chgar($elem)

      if {[info exists old_values($elem)]} {
         # if old and new config have the same value, create no vi command,
         # if they differ, add vi command to ...
         if { [string compare $old_values($elem) $newVal] != 0 } {
            if { $newVal == "" } {
               # ... delete config entry (replace by comment)
               lappend vi_commands ":%s/^$elem .*$//\n"
               puts $CHECK_OUTPUT "vi is $vi_commands \n"
            } else {
               # ... change config entry
               set newVal1 [split $newVal {/}]
               set newVal [join $newVal1 {\/}]
               lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
               puts $CHECK_OUTPUT "New vi is $vi_commands \n"
            }
        }
      } else {
        # if the config entry didn't exist in old config: append a new line
        lappend vi_commands "A\n$elem  $newVal[format "%c" 27]"

      }
   }
   # Modify exechost from file?
   if { $fast_add } {
     set tmpfile [dump_array_to_tmpfile old_values]
     set result [start_sge_bin "qconf" "-Me ${tmpfile}"]

   } else {
   # User vi

      set CHANGED  [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_EXEC_HOSTENTRYOFXCHANGEDINEXECLIST_S] "*" ]
      set result [handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-me $host" $vi_commands "modified" $CHANGED]
      if { $result == -2 } {
         set result 0
      }
      if { $result != 0 } {
         add_proc_error "set_exechost" -1 "could not modifiy exechost $host"
         set result -1
      }
   }
  return $result
}
#****** sge_host/mod_exechost() ******
#
#  NAME
#     mod_exechost -- Wrapper around set_exechost
#
#  SYNOPSIS
#     mod_exechost { change_array host {fast_add 1} {on_host ""} {as_user ""} {raise_error 1} }
#
#  FUNCTION
#     See set_exechost
#
#  INPUTS
#     change_array - name of an array variable that will be set by set_exechost
#     host         - name of an execution host
#     {fast_add 1} - 0: modify the attribute using qconf -me,
#                  - 1: modify the attribute using qconf -Me, faster
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     {raise_error 1} - raise an error condition on error (default), or just
#                       output the error message to stdout
#
#  SEE ALSO
#     sge_host/get_exechost()
#*******************************
proc mod_exechost { change_array host {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}} {
   global CHECK_OUTPUT

   puts $CHECK_OUTPUT "Using mod_exechost as wrapper for set_exechost \n"

   return [set_exechost change_array $host $fast_add $on_host $as_user $raise_error]

}
#****** sge_host/get_exechost_list() *******************************************
#  NAME
#     get_exechost_list() -- get a list of exec hosts
#
#  SYNOPSIS
#     get_exechost_list { output_var {on_host ""} {as_user ""} {raise_error 1} 
#     } 
#
#  FUNCTION
#     Calls qconf -sel to retrieve a list of execution hosts.
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
proc get_exechost_list {output_var {on_host ""} {as_user ""} {raise_error 1}} {
   upvar $output_var out

   return [get_qconf_list "get_exechost_list" "-sel" out $on_host $as_user $raise_error]
}

#****** sge_host/get_adminhost_list() *******************************************
#  NAME
#     get_adminhost_list() -- get a list of admin hosts
#
#  SYNOPSIS
#     get_adminhost_list {{on_host ""} {as_user ""} {raise_error 1} 
#     } 
#
#  FUNCTION
#     Calls qconf -sel to retrieve a list of admin hosts.
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
proc get_adminhost_list {{on_host ""} {as_user ""} {raise_error 1}} {

   return [get_qconf_list "get_adminhost_list" "-sh" out $on_host $as_user $raise_error]
}

#****** sge_host/get_submithost_list() *******************************************
#  NAME
#     get_submithost_list() -- get a list of submit hosts
#
#  SYNOPSIS
#     get_submithost_list { {output_var result} {on_host ""} {as_user ""} {raise_error 1} 
#     } 
#
#  FUNCTION
#     Calls qconf -ss to retrieve a list of submit hosts.
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
proc get_submithost_list {{output_var result} {on_host ""} {as_user ""} {raise_error 1}} {
   upvar $output_var out

   return [get_qconf_list "get_submithost_list" "-ss" out $on_host $as_user $raise_error]
}

#****** sge_host/get_queue_config_list() *****************************************
#  NAME
#     get_queue_config_list() -- get a list of hosts  for  which  configurations
#                          are  available.
#
#  SYNOPSIS
#     get_queue_config_list { {output_var result} {on_host ""} {as_user ""} {arg ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -sconfl to list of hosts 
#
#  INPUTS
#     output_var      - result will be placed here
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     {arg ""}    - value of calendar we wish to see; default is ""
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
proc get_queue_config_list {{output_var result} {on_host ""} {as_user ""} {arg ""} {raise_error 1}} {

   upvar $output_var out

   return [get_qconf_list "get_queue_list" "-sconfl" out $on_host $as_user $raise_error]

}
#****** sge_host/get_processor_list() *****************************************
#  NAME
#     get_processor_list() -- get a list of all processors
#
#  SYNOPSIS
#     get_processor_list { {output_var result} {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -sep to retrieve all processors
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
proc get_processor_list {{output_var result} {on_host ""} {as_user ""} {raise_error 1}} {
  
   upvar $output_var out

   return [get_qconf_list "get_processor_list" "-sep" out $on_host $as_user $raise_error]

}

#****** sge_host/del_adminhost_error() ******************************************
#  NAME
#     del_adminhost_error() -- error handling for del_adminhost
#
#  SYNOPSIS
#     del_adminhost_error { result host raise_error }
#
#  FUNCTION
#     Does the error handling for del_adminhost.
#     Translates possible error messages of qconf -dh,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     del_adminhost. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     host        - host for which qconf -dh has been called
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for del_adminhost function:
#       -1: administrative host "host" does not exist
#     -999: other error
#
#  SEE ALSO
#     sge_host/get_exechost
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc del_adminhost_error {result host on_host raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1 "
   set messages(-1) [translate_macro MSG_SGETEXT_DOESNOTEXIST_SS "administrative host" $host]

   # we might have version dependent, exechost specific error messages
   get_exechost_error_vdep messages $on_host

   # now evaluate return code and raise errors
   set ret [handle_sge_errors "del_adminhost" "qconf -dh $host" $result messages $raise_error]

   return $ret
}

#****** sge_host/del_adminhost() *****************************************
#  NAME
#     del_adminhost() -- delete administrative host
#
#  SYNOPSIS
#     del_adminhost { host {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -dh host to delete administrative host
#
#  INPUTS
#     host            - administrative host which will be deleted
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
proc del_adminhost {host  {on_host ""} {as_user ""} {raise_error 1}} {

   global ts_config

   set ret 0
   set result [start_sge_bin "qconf" "-dh $host" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      set ret  $result 
   } else {
      set ret [del_adminhost_error $prg_exit_state $host $on_host $raise_error]
   }

   return $ret
}

#****** sge_host/add_adminhost() *****************************************
#  NAME
#     add_adminhost() -- add administrative host
#
#  SYNOPSIS
#     add_adminhost { host  {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -ah host to add administrative host
#
#  INPUTS
#     host            - administrative host which will be added
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
proc add_adminhost {host  {on_host ""} {as_user ""} {raise_error 1}} {

   global ts_config

   set ret 0
   set result [start_sge_bin "qconf" "-ah $host" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      set ret $result 
   } else {
      set ret [add_adminhost_error $prg_exit_state $host $on_host $raise_error]
   }

   return $ret
}

#****** sge_host/add_adminhost_error() ******************************************
#  NAME
#     add_adminhost_error() -- error handling for add_adminhost
#
#  SYNOPSIS
#     add_adminhost_error { result host raise_error }
#
#  FUNCTION
#     Does the error handling for del_adminhost.
#     Translates possible error messages of qconf -dh,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     del_adminhost. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     host        - host for which qconf -dh has been called
#     on_host     - valid host on which qconf -dh has been called
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for add_adminhost function:
#       -1: can't resolve hostname "host" 
#     -999: other error
#
#  SEE ALSO
#     sge_host/get_exechost
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc add_adminhost_error {result host on_host raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1 "
   set messages(-1) [translate_macro MSG_SGETEXT_ALREADYEXISTS_SS "adminhost" $host]

   # we might have version dependent, exechost specific error messages
   get_exechost_error_vdep messages $on_host

   # now evaluate return code and raise errors
   set ret [handle_sge_errors "add_adminhost" "qconf -ah $host" $result messages $raise_error]

   return $ret
}

