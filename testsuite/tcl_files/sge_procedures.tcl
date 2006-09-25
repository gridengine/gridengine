#!/usr/local/bin/tclsh
# expect script 
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

global module_name
set module_name "sge_procedures.tcl"

# procedures
#                                                             max. column:     |
#****** sge_procedures/test() ******
# 
#  NAME
#     test -- ??? 
#
#  SYNOPSIS
#     test { m p } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     m - ??? 
#     p - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************
proc test {m p} {
   puts "test $m $p"
}

#****** sge_procedures/get_complex_version() ***********************************
#  NAME
#     get_complex_version() -- get information about used qconf version
#
#  SYNOPSIS
#     get_complex_version { } 
#
#  FUNCTION
#     This procedure returns 0 for qconf supporting complex_list in queue
#     objects, otherwise 1.
#
#  INPUTS
#
#  RESULT
#     0 - qconf supporting complex_list in queue
#     1 - qconf is not supporting complex_list in queue 
#
#  SEE ALSO
#*******************************************************************************
proc get_complex_version {} {
   global ts_config
   global CHECK_ARCH CHECK_OUTPUT CHECK_CORE_MASTER
   set version 0

   puts $CHECK_OUTPUT "checking complex version ..."
   catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-scl" } result
   set INVALID_OPTION [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro MSG_ANSWER_INVALIDOPTIONARGX_S] "-scl"]
   set INVALID_OPTION [string trim $INVALID_OPTION]
   
   if { [string match "*$INVALID_OPTION*" $result] == 1 } {
      set version 1
      puts $CHECK_OUTPUT "new complex version"
   } else {
      puts $CHECK_OUTPUT "old complex version"
   }
   return $version
}

#                                                             max. column:     |
#****** sge_procedures/get_qmaster_spool_dir() ******
# 
#  NAME
#     get_qmaster_spool_dir() -- return path to qmaster spool directory
#
#  SYNOPSIS
#     get_qmaster_spool_dir { } 
#
#  FUNCTION
#     This procedure returns the actual qmaster spool directory 
#     (or "" in case of an error)
#
#  RESULT
#     string with actual spool directory of qmaster
#
#
#  SEE ALSO
#     sge_procedures/get_execd_spool_dir()
#*******************************
proc get_qmaster_spool_dir {} {
   global ts_config sge_config

   set ret "unknown"

   if {![info exists sge_config(qmaster_spool_dir)]} {
      # error
      set ret "qmaster spool dir not yet initialized"
   } else {
      set ret $sge_config(qmaster_spool_dir)
   }

   return $ret
}
#                                                             max. column:     |
#****** sge_procedures/set_qmaster_spool_dir() ******
# 
#  NAME
#     set_qmaster_spool_dir() -- return path to qmaster spool directory
#
#  SYNOPSIS
#     set_qmaster_spool_dir { } 
#
#  FUNCTION
#     This procedure returns the actual qmaster spool directory 
#     (or "" in case of an error)
#
#  RESULT
#     string with actual spool directory of qmaster
#
#
#  SEE ALSO
#     sge_procedures/get_qmaster_spool_dir()
#*******************************
proc set_qmaster_spool_dir {spool_dir} {
  global sge_config

  set sge_config(qmaster_spool_dir) $spool_dir
}

#                                                             max. column:     |
#****** sge_procedures/get_execd_spool_dir() ******
# 
#  NAME
#     get_execd_spool_dir() -- return spool dir for exec host
#
#  SYNOPSIS
#     get_execd_spool_dir { host } 
#
#  FUNCTION
#     This procedure returns the actual execd spool directory on the given host.
#     If no local spool directory is specified for this host, the global 
#     configuration is used. If an error accurs the procedure returns "".
#
#  INPUTS
#     host - host name with execd installed on
#
#  RESULT
#     string
#
#  SEE ALSO
#     sge_procedures/get_qmaster_spool_dir()
#*******************************
proc get_execd_spool_dir {host} {
  global CHECK_OUTPUT 
  get_config host_config $host
  if { [info exist host_config(execd_spool_dir) ] == 0 } {
     debug_puts "--> no special execd_spool_dir for host $host"
     get_config host_config
  }
  if { [info exist host_config(execd_spool_dir) ] != 0 } {
     return $host_config(execd_spool_dir)
  } else {
     return "unknown"
  }
}

proc seek_and_desroy_sge_processes {} {
   global ts_host_config CHECK_OUTPUT CHECK_USER

   set answer_text ""
   if { [info exists kill_list] } {
      unset kill_list
   }

   foreach host $ts_host_config(hostlist) {
      puts $CHECK_OUTPUT "host $host ..."

      get_ps_info 0 $host ps_info
      for {set i 0} {$i < $ps_info(proc_count) } {incr i 1} {
         if { [string match "*$CHECK_USER*" $ps_info(string,$i)] } {
            if { [string match "*sge_*" $ps_info(string,$i)]   || 
                 [string match "*qevent*" $ps_info(string,$i)] ||
                 [string match "*qping*" $ps_info(string,$i)]   } {
               puts "ps_info(pid,$i)     = $ps_info(string,$i)"
               append answer_text "host $host: pid $ps_info(pid,$i)\n"
               if {[info exists kill_list($host)]} {
                  lappend kill_list($host) $ps_info(pid,$i)
               } else {
                  set kill_list($host) $ps_info(pid,$i)
               }
            }
         }
      }

   }

   if { $answer_text != "" } {
      puts $CHECK_OUTPUT "found matching prozesses:"
      puts $CHECK_OUTPUT "$answer_text"
      foreach host $ts_host_config(hostlist) {
         if { [info exists kill_list($host)] } {
            foreach pid $kill_list($host) {
               puts $CHECK_OUTPUT "killing pid $pid on host $host ..."
            }
         }
      }         
   }
   wait_for_enter
}


#****** sge_procedures/check_messages_files() **********************************
#  NAME
#     check_messages_files() -- check messages files for errors and warnings
#
#  SYNOPSIS
#     check_messages_files { } 
#
#  FUNCTION
#     This procedure reads in all cluster messages files from qmaster and
#     execd and returns the messages with errors or warnings.
#
#  RESULT
#     string with parsed file output
#
#*******************************************************************************
proc check_messages_files { } {
   global ts_config
   global CHECK_OUTPUT CHECK_CORE_MASTER


   set full_info ""

   foreach host $ts_config(execd_nodes) {
      set status [ check_execd_messages $host 1] 
      append full_info "\n=========================================\n"
      append full_info "execd: $host\n"
      append full_info "file : [check_execd_messages $host 2]\n"
      append full_info "=========================================\n"
      append full_info $status
   }

   set status [ check_schedd_messages 1] 
   append full_info "\n=========================================\n"
   append full_info "schedd: $CHECK_CORE_MASTER\n"
   append full_info "file  : [check_schedd_messages 2]\n"
   append full_info "=========================================\n"
   append full_info $status

   set status [ check_qmaster_messages 1] 
   append full_info "\n=========================================\n"
   append full_info "qmaster: $CHECK_CORE_MASTER\n"
   append full_info "file   : [check_qmaster_messages 2]\n"
   append full_info "=========================================\n"
   append full_info $status
   return $full_info
}


#****** sge_procedures/get_qmaster_messages_file() *****************************
#  NAME
#     get_qmaster_messages_file() -- get path to qmaster's messages file
#
#  SYNOPSIS
#     get_qmaster_messages_file { } 
#
#  FUNCTION
#     This procedure returns the path to the running qmaster's messages file
#
#  RESULT
#     path to qmaster's messages file
#
#  SEE ALSO
#     sge_procedures/get_execd_messages_file()
#     sge_procedures/get_schedd_messages_file()
#
#*******************************************************************************
proc get_qmaster_messages_file { } {
   return [ check_qmaster_messages 2 ]
}

#****** sge_procedures/check_qmaster_messages() ********************************
#  NAME
#     check_qmaster_messages() -- get qmaster messages file content
#
#  SYNOPSIS
#     check_qmaster_messages { { show_mode 0 } } 
#
#  FUNCTION
#     This procedure locates the qmaster messages file (using qconf -sconf) 
#     and returns the output of cat.
#
#  INPUTS
#     { show_mode 0 } - if not 0: return only warning and error lines
#                       if     2: return only path to qmaster messages file
#
#  RESULT
#     output string
#
#  SEE ALSO
#     sge_procedures/check_execd_messages()
#     sge_procedures/check_schedd_messages()
#*******************************************************************************
proc check_qmaster_messages { { show_mode 0 } } {
   global ts_config
   global CHECK_ARCH CHECK_USER
   global CHECK_OUTPUT CHECK_CORE_MASTER

   set spool_dir [get_qmaster_spool_dir]
   puts $CHECK_OUTPUT "\"$spool_dir\""

   set messages_file "$spool_dir/messages"

   if { $show_mode == 2 } {
      return $messages_file
   }   

   set return_value ""

   get_file_content $CHECK_CORE_MASTER $CHECK_USER $messages_file
   for { set i 1 } { $i <= $file_array(0) } { incr i 1 } {
       set line $file_array($i)
       if { ( [ string first "|E|" $line ] >= 0 )   || 
            ( [ string first "|W|" $line ] >= 0 )   ||
            ( $show_mode == 0               )  }   {
            append return_value "line $i: $line\n"
       }
   }
   return $return_value
}

#****** sge_procedures/get_schedd_messages_file() ******************************
#  NAME
#     get_schedd_messages_file() -- get path to scheduler's messages file
#
#  SYNOPSIS
#     get_schedd_messages_file { } 
#
#  FUNCTION
#     This procedure returns the path to the running scheduler's messages file
#
#  RESULT
#     path to scheduler's messages file
#
#  SEE ALSO
#     sge_procedures/get_execd_messages_file()
#     sge_procedures/get_qmaster_messages_file()
#*******************************************************************************
proc get_schedd_messages_file { } {
   return [ check_schedd_messages 2 ]
}

#****** sge_procedures/check_schedd_messages() *********************************
#  NAME
#     check_schedd_messages() -- get schedulers messages file content
#
#  SYNOPSIS
#     check_schedd_messages { { show_mode 0 } } 
#
#  FUNCTION
#     This procedure locates the schedd messages file (using qconf -sconf) 
#     and returns the output of cat.
#
#  INPUTS
#     { show_mode 0 } - if not 0: return only warning and error lines
#                       if     2: return only path to schedd messages file
#
#  RESULT
#     output string
#
#  SEE ALSO
#     sge_procedures/check_execd_messages()
#     sge_procedures/check_qmaster_messages()
#*******************************************************************************
proc check_schedd_messages { { show_mode 0 } } {
   global ts_config
   global CHECK_ARCH CHECK_USER
   global CHECK_OUTPUT CHECK_CORE_MASTER

   set spool_dir [get_qmaster_spool_dir]
   set messages_file "$spool_dir/schedd/messages"

   if { $show_mode == 2 } {
      return $messages_file
   }   

   set return_value ""

   get_file_content $CHECK_CORE_MASTER $CHECK_USER $messages_file
   for { set i 1 } { $i <= $file_array(0) } { incr i 1 } {
       set line $file_array($i)
       if { ( [ string first "|E|" $line ] >= 0 )   || 
            ( [ string first "|W|" $line ] >= 0 )   ||
            ( $show_mode == 0               )  }   {
            append return_value "line $i: $line\n"
       }
   }
   return $return_value
}


#****** sge_procedures/get_execd_messages_file() *******************************
#  NAME
#     get_execd_messages_file() -- get messages file path of execd
#
#  SYNOPSIS
#     get_execd_messages_file { hostname } 
#
#  FUNCTION
#     This procedure returns the full path to the given execd's messages file
#
#  INPUTS
#     hostname - hostname where the execd is running
#
#  RESULT
#     path to messages file of the given execd host
#
#  SEE ALSO
#     sge_procedures/get_qmaster_messages_file()
#     sge_procedures/get_schedd_messages_file()
#
#*******************************************************************************
proc get_execd_messages_file { hostname } {
   return [ check_execd_messages $hostname 2 ]
}

#****** sge_procedures/check_execd_messages() **********************************
#  NAME
#     check_execd_messages() -- get execd messages file content
#
#  SYNOPSIS
#     check_execd_messages { hostname { show_mode 0 } } 
#
#  FUNCTION
#     This procedure locates the execd messages file (using qconf -sconf) 
#     and returns the output of cat.
#
#  INPUTS
#     hostname        - hostname of execd
#     { show_mode 0 } - if not 0: return only warning and error lines
#                       if     2: return only path to execd messages file
#
#  RESULT
#     output string
#
#  SEE ALSO
#     sge_procedures/check_qmaster_messages()
#     sge_procedures/check_schedd_messages()
#*******************************************************************************
proc check_execd_messages { hostname { show_mode 0 } } {
   global ts_config
   global CHECK_ARCH CHECK_HOST CHECK_USER
   global CHECK_OUTPUT

   set program "$ts_config(product_root)/bin/$CHECK_ARCH/qconf"
   set program_arg "-sconf $hostname" 
   set output [ start_remote_prog $CHECK_HOST $CHECK_USER $program $program_arg]
   if { [string first "execd_spool_dir" $output ] < 0 } {
      set program "$ts_config(product_root)/bin/$CHECK_ARCH/qconf"
      set program_arg "-sconf global" 
      set output [ start_remote_prog $CHECK_HOST $CHECK_USER $program $program_arg]
   }

   set output [ split $output "\n" ]
   set spool_dir "unkown"
   foreach line $output {
      if { [ string first "execd_spool_dir" $line ] >= 0 } {
         set spool_dir [ lindex $line 1 ]
      }
   }
   puts $CHECK_OUTPUT "\"$spool_dir\""

   set messages_file "$spool_dir/$hostname/messages"
   if { $show_mode == 2 } {
      return $messages_file
   }

   set return_value ""

   get_file_content $hostname $CHECK_USER $messages_file
   for { set i 1 } { $i <= $file_array(0) } { incr i 1 } {
       set line $file_array($i)
       if { ( [ string first "|E|" $line ] >= 0 ) || 
            ( [ string first "|W|" $line ] >= 0 ) ||
            ( $show_mode == 0 )                }  {
            append return_value "line $i: $line\n"
       }
   }
   return $return_value
} 

#****** sge_procedures/start_sge_bin() *****************************************
#  NAME
#     start_sge_bin() -- start a sge binary
#
#  SYNOPSIS
#     start_sge_bin { bin args {host ""} {user ""} {exit_var prg_exit_state} 
#     {timeout 60} {sub_path "bin"} } 
#
#  FUNCTION
#     Starts a binary in $SGE_ROOT/bin/<arch>
#
#  INPUTS
#     bin                       - binary to start, e.g. qconf
#     args                      - arguments, e.g. "-sel"
#     {host ""}                 - host on which to execute command
#     {user ""}                 - user who shall call command
#     {exit_var prg_exit_state} - variable for returning command exit code
#     {timeout 60}              - timeout for command execution
#     {sub_path "bin"}          - component of binary path, e.g. "bin" or "utilbin"
#
#  RESULT
#     Output of called command.
#     The exit code will be placed in exit_var.
#
#  SEE ALSO
#     sge_procedures/start_sge_utilbin()
#     remote_procedures/start_remote_prog()
#*******************************************************************************
proc start_sge_bin {bin args {host ""} {user ""} {exit_var prg_exit_state} {timeout 60} {sub_path "bin"}} {
   global ts_config CHECK_OUTPUT CHECK_USER

   upvar $exit_var exit_state

   if {$host == ""} {
      set host $ts_config(master_host)
   }

   if {$user == ""} {
      set user $CHECK_USER
   }

   set arch [resolve_arch $host]
   set ret 0
   set binary "$ts_config(product_root)/$sub_path/$arch/$bin"

   debug_puts "executing $binary $args\nas user $user on host $host"
   # Add " around $args if there are more the 1 args....
   set result [start_remote_prog $host $user $binary "$args" exit_state $timeout]

   return $result
}

#****** sge_procedures/start_sge_utilbin() *************************************
#  NAME
#     start_sge_utilbin() -- start a sge utilbin binary
#
#  SYNOPSIS
#     start_sge_utilbin { bin args {host ""} {user ""} 
#     {exit_var prg_exit_state} } 
#
#  FUNCTION
#     Starts a binary in $SGE_ROOT/utilbin/<arch>
#
#  INPUTS
#     bin                       - command to start
#     args                      - arguments for command
#     {host ""}                 - host on which to start command
#     {user ""}                 - user who shall start command
#     {exit_var prg_exit_state} - variable for returning command exit code
#
#  RESULT
#     Output of called command.
#     The exit code will be placed in exit_var.
#
#  SEE ALSO
#     sge_procedures/start_sge_bin()
#*******************************************************************************
proc start_sge_utilbin {bin args {host ""} {user ""} {exit_var prg_exit_state}} {
   upvar $exit_var exit_state

   return [start_sge_bin $bin $args $host $user exit_state 60 "utilbin"]
}

#****** sge_procedures/start_source_bin() *****************************************
#  NAME
#     start_source_bin() -- start a binary in compile directory
#
#  SYNOPSIS
#     start_source_bin { bin args {host ""} {user ""} {exit_var prg_exit_state} 
#     {timeout 60} {sub_path "bin"} } 
#
#  FUNCTION
#     Starts a binary in the compile directory (gridengine/source/$buildarch).
#
#  INPUTS
#     bin                       - binary to start, e.g. test_drmaa
#     args                      - arguments, e.g. "-sel"
#     {host ""}                 - host on which to execute command
#     {user ""}                 - user who shall call command
#     {exit_var prg_exit_state} - variable for returning command exit code
#     {timeout 60}              - timeout for command execution
#
#  RESULT
#     Output of called command.
#     The exit code will be placed in exit_var.
#
#  SEE ALSO
#     sge_procedures/start_sge_bin()
#     remote_procedures/start_remote_prog()
#*******************************************************************************
proc start_source_bin {bin args {host ""} {user ""} {exit_var prg_exit_state} {timeout 60} {background 0} {envlist ""}} {
   global ts_config CHECK_OUTPUT CHECK_USER

   upvar $exit_var exit_state
  
   # pass on environment
   set env_var ""
   if {$envlist != ""} {
      upvar $envlist env
      set env_var env
   }

   if {$host == ""} {
      set host $ts_config(master_host)
   }

   if {$user == ""} {
      set user $CHECK_USER
   }

   set arch [resolve_build_arch_installed_libs $host]
   set ret 0
   set binary "$ts_config(source_dir)/$arch/$bin"

   debug_puts "executing $binary $args\nas user $user on host $host"
   # Add " around $args if there are more the 1 args....
   set result [start_remote_prog $host $user $binary "$args" exit_state $timeout $background $env_var]

   return $result
}

#****** sge_procedures/get_sge_error_generic() *********************************
#  NAME
#     get_sge_error_generic() -- provide a list of generic error messages
#
#  SYNOPSIS
#     get_sge_error_generic { messages_var } 
#
#  FUNCTION
#     The function builds a list of SGE generic error messages, i.e. messages
#     that might be returned by any SGE command, for example, if the qmaster 
#     cannot be contacted.
#
#     Messages that are common to multiple SGE versions are added directly in
#     this function.
#     For version specific messages, a function get_sge_error_generic_vdep
#     is called.
#     get_sge_error_generic_vdep is implemented in the version specific 
#     sge_procedures files (sge_procedures.53.tcl, sge_procedures.60.tcl, ...).
#
#  INPUTS
#     messages_var - TCL array containing error message description, see
#                    sge_procedures/handle_sge_errors().
#
#  NOTES
#     A list of all generic error messages is maintained in the ADOC header
#     for sge_procedures/get_sge_error().
#     If you add messages here or in the version specific functions, please
#     document them in the sge_procedures/get_sge_error() ADOC header.
#
#  SEE ALSO
#     sge_procedures/get_sge_error()
#*******************************************************************************
proc get_sge_error_generic {messages_var} {
   upvar $messages_var messages

   # messages indicating insufficient host privileges
   lappend messages(index) -200
   lappend messages(index) -201
   lappend messages(index) -202
   set messages(-200) "*[translate_macro MSG_SGETEXT_NOSUBMITORADMINHOST_S "*"]"
   set messages(-201) "*[translate_macro MSG_SGETEXT_NOADMINHOST_S "*"]"
   set messages(-202) "*[translate_macro MSG_SGETEXT_NOSUBMITHOST_S "*"]"

   # messages indicating insufficient user privileges
   lappend messages(index) -210
   lappend messages(index) -211
   set messages(-210) "*[translate_macro MSG_SGETEXT_MUSTBEMANAGER_S "*"]"
   set messages(-211) "*[translate_macro MSG_SGETEXT_MUSTBEOPERATOR_S "*"]"

   get_sge_error_generic_vdep messages
}

#****** sge_procedures/get_sge_error() *****************************************
#  NAME
#     get_sge_error() -- return error code for sge command
#
#  SYNOPSIS
#     get_sge_error { procedure command result {raise_error 1} } 
#
#  FUNCTION
#     Parses the result of a sge command and tries to find certain known
#     error messages.
#     If an error message is recognized in result, a specific error message
#     will be returned.
#     If result doesn't contain a known error message, -999 will be returned.
#
#     If requested (raise_error), an error situation will be raised 
#     (add_proc_error).
#
#     Error codes are grouped by error situation:
#        - 100-199: communication errors
#        - 200-299: permission specific error messages
#
#     List of error codes:
#        -100: sge_qmaster cannot be contacted
#
#        -200: host executing command is no admin or submit host
#        -201: host executing command is no admin host
#        -202: host executing command is no submit host
#        -210: user executing command is no manager
#        -210: user executing command is no operator
#
#  INPUTS
#     procedure       - name of the calling procedure (for error message)
#     command         - executed command (for error message)
#     result          - output of a SGE command
#     {raise_error 1} - raise an error condition on error (default), or just
#                       output the error message to stdout
#
#  RESULT
#     -999, or specific error code, see above
#
#  SEE ALSO
#     sge_procedures/handle_sge_errors()
#*******************************************************************************
proc get_sge_error {procedure command result {raise_error 1}} {
   # initialize array.
   # handle_sge_errors will add the sge generic messages
   set messages(index) {}

   # parse error messages and map to return code
   set ret [handle_sge_errors $procedure $command $result messages $raise_error]
   return $ret
}

#****** sge_procedures/handle_sge_errors() *************************************
#  NAME
#     handle_sge_errors() -- parse error messages from sge commands
#
#  SYNOPSIS
#     handle_sge_errors { procedure command result messages_var {raise_error 1} 
#     } 
#
#  FUNCTION
#     Parse error messages and raise an error condition (add_proc_error), if 
#     this is required.
#
#     Which messages can be recognized, a highlevel description and error
#     code are passed in the variable referenced by messages_var.
#     This variable is a TCL array containing the following fields:
#     - "index", containing all possible error codes
#     - <error_code>, containing the message for a certain error code
#     - <error_code,description>, a highlevel description for the error
#     - <error_code,level>, the error level for add_proc_error
#
#     handle_sge_errors will add generic sge error messages to the list of 
#     application specific error messages provided by the caller.
#
#     For application specific error messages, a range from -1 to -99 is reserved.
#     For a list of the generic error messages, see sge_procedures/get_sge_error().
#     
#  INPUTS
#     procedure       - name of the procedure calling the function 
#                       (for error output)
#     command         - sge command that had been called (for error output)
#     result          - output of the command
#     messages_var    - array with possible messages and info how to handle them
#     {raise_error 1} - whether to raise an error condition (add_proc_error) 
#                       or not
#     {prg_exit_state ""} - exit state of sge command
#
#  RESULT
#     error code, for recognized messages the corresponding error code from 
#                 messages array, or -999, if the error message is not contained 
#                 in messages array
#
#  EXAMPLE
#     set messages(index) "-1 -2"
#     set messages(-1) [translate_macro MSG_SGETEXT_CANTRESOLVEHOST_S $host]
#     set messages(-2) [translate_macro MSG_EXEC_XISNOTANEXECUTIONHOST_S $host]
#     set messages(-2,description) "$host is not configured as exec host"
#     set messages(-2,level) -3    ;# we only raise an "unsupported" warning
#     set ret [handle_sge_errors "get_exechost" "qconf -se $host" $result 
#              messages $raise_error]
#
#  SEE ALSO
#     check/add_proc_error()
#     sge_procedures/get_sge_error()
#*******************************************************************************

proc handle_sge_errors {procedure command result messages_var {raise_error 1} {prg_exit_state ""}} {
   upvar $messages_var messages

   set ret -999

   # add sge generic error messages to the array of specific error messages
   get_sge_error_generic messages

   # remove trailing garbage
   set result [string trim $result]
   # try to find error message
   foreach errno $messages(index) {
      if {[string match $messages($errno) $result]} {
         set ret $errno
         break
      }
   }

   # in case of errors, do error reporting
   if {$ret < 0} {
      set error_level -1
      set error_message "$command failed ($ret):\n"

      if {$ret == -999} {
         append error_message "$result"
      } else {
         # we might have a high level error description
         if {[info exists messages($ret,description)]} {
            append error_message "$messages($ret,description)\n"
            append error_message "command output was:\n"
         }

         append error_message "$result"

         # we might have a special error level (error, warning, unsupported)
         if {[info exists messages($ret,level)]} {
            set error_level $messages($ret,level)
         }
      }

      # generate error message or just informational/error output
      add_proc_error $procedure $error_level $error_message $raise_error

      if {$prg_exit_state != ""} {
         if {$prg_exit_state == 0 && $ret < 0} {
            add_proc_error $procedure -3 "qconf returned 0 while reporting the error message:\n$result"
         }
         if {$prg_exit_state != 0 && $ret >= 0} {
            add_proc_error $procedure -3 "qconf returned error state while its output reports success:\n$result"
         }
      }
   }

   return $ret
}

#****** sge_procedures/submit_error_job() **************************************
#  NAME
#     submit_error_job() -- submit job which will get error state
#
#  SYNOPSIS
#     submit_error_job { jobargs } 
#
#  FUNCTION
#     This procedure is submitting a job with a wrong shell option (-S). This
#     will set the job in error state. (E)
#
#  INPUTS
#     jobargs - job arguments (e.g. -o ... -e ... jobscript path)
#
#  RESULT
#     job id 
#
#  SEE ALSO
#     sge_procedures/submit_error_job()
#     sge_procedures/submit_waitjob_job()
#     sge_procedures/submit_time_job()
#*******************************************************************************
proc submit_error_job { jobargs } {
    return [submit_job "-S __no_shell $jobargs"]
}

#****** sge_procedures/submit_wait_type_job() **********************************
#  NAME
#     submit_wait_type_job() -- submit job and wait for accouting info
#
#  SYNOPSIS
#     submit_wait_type_job { job_type host user {variable qacct_info} } 
#
#  FUNCTION
#     This function can be used to submit different job types (standard
#     qsub job, qsh, qrsh, qrlogin, qlogin and tight integrated jobs) and
#     wait for the jobs to appear in the accouting file. The function
#     returns the job id of the job.
#
#  INPUTS
#     job_type              - "qsub", "qsh", "qrsh", "qrlogin", "qlogin" or
#                             "tight_integrated"
#     host                  - host where the job should run
#     user                  - user who should submit the job
#     {variable qacct_info} - return value for job accounting information
#
#  INFO 
#     1) user must be != $CHECK_USER
#
#     2) "tight_integrated" job need pe "tight_job_start"
# 
#
#  RESULT
#     job id (>= 1) or -1 on error
#
#*******************************************************************************
proc submit_wait_type_job { job_type host user {variable qacct_info} } {
   global ts_config CHECK_OUTPUT
   global CHECK_PRODUCT_ROOT CHECK_HOST CHECK_DEBUG_LEVEL CHECK_USER
   global CHECK_DISPLAY_OUTPUT CHECK_SCRIPT_FILE_DIR
   upvar $variable qacctinfo


   if { $user == $CHECK_USER } {
      add_proc_error "submit_wait_type_job" -1 "This procedure only works for users != \$CHECK_USER ($CHECK_USER)"
      return -1
   }
   delete_all_jobs
   wait_for_end_of_all_jobs 30

   set job_id 0
   set remote_host_arg "-l h=$host"
   set output_argument "-o /dev/null -e /dev/null"
   set job_argument "$CHECK_PRODUCT_ROOT/examples/jobs/sleeper.sh 5"

   puts $CHECK_OUTPUT "submitting job type \"$job_type\" ..."
   switch -exact $job_type {
      "qsub" {
         set job_id [submit_job "$remote_host_arg $output_argument $job_argument" 1 60 "" $user]
         wait_for_jobstart $job_id "leeper" 30 1 1
         wait_for_jobend $job_id "leeper" 30 1 1
      }

      "qrlogin" { ;# without command (qrsh without command)
         puts $CHECK_OUTPUT "starting qrsh $remote_host_arg as user $user on host $CHECK_HOST ..."
         set sid [open_remote_spawn_process $CHECK_HOST $user qrsh "$remote_host_arg"]
         set sp_id [lindex $sid 1]
         set timeout 1
         set my_tries 60

         while {1} {
            expect {
               -i $sp_id "_start_mark_*\n" {
                  puts $CHECK_OUTPUT "got start mark ..."
                  break;
               }
               -i $sp_id default {
                       if { $my_tries > 0 } {
                           incr my_tries -1
                           puts -nonewline $CHECK_OUTPUT "."
                           flush $CHECK_OUTPUT
                           continue
                       } else { 
                          add_proc_error "submit_wait_type_job" -1 "startup timeout" 
                          break;
                       }
                   }
            }
         }
         
         set my_tries 60
         while {1} {
            expect {
               -i $sp_id {[A-Za-z>$%]*} {
                       puts $CHECK_OUTPUT "startup ..."
                       break;
                   }
               -i $sp_id default {
                       if { $my_tries > 0 } {
                           incr my_tries -1
                           puts -nonewline $CHECK_OUTPUT "."
                           flush $CHECK_OUTPUT
                           continue
                       } else { 
                          add_proc_error "submit_wait_type_job" -1 "startup timeout" 
                          break;
                       }
                   }

            }
         }

         set max_timeouts 60 
         set done 0
         while {!$done} {
            expect {
               -i $sp_id full_buffer {
                  add_proc_error "submit_with_method" -1 "expect full_buffer error"
                  set done 1
               }
               -i $sp_id timeout {
                  incr max_timeouts -1

                  if { $job_id == 0 } {
                     set job_list [get_standard_job_info 0 0 1]
                     foreach job $job_list {
                        puts $CHECK_OUTPUT $job
                        if { [lindex $job 2] == "QRLOGIN" && [lindex $job 3] == $user && [lindex $job 4] == "r"  } {
                           puts $CHECK_OUTPUT "qrlogin job id is [lindex $job 0]"
                           set job_id [lindex $job 0]
                        }
                     }
                  } else {
                     send -i $sp_id "\n$ts_config(testsuite_root_dir)/$CHECK_SCRIPT_FILE_DIR/shell_start_output.sh\n"
                  }

                  if { $max_timeouts <= 0 } {
                     add_proc_error "submit_with_method" -1 "got 15 timeout errors - break"
                     set done 1 
                  }
               }
               -i $sp_id "ts_shell_response*\n" {
                  puts $CHECK_OUTPUT "found matching shell response text! Sending exit ..."
                  send -i $sp_id "exit\n"
               }

               -i $sp_id eof {
                  add_proc_error "submit_with_method" -1 "got eof"
                  set done 1
               }
               -i $sp_id "_start_mark_" {
                  puts $CHECK_OUTPUT "remote command started"
                  set done 0
               }
               -i $sp_id "_exit_status_" {
                  puts $CHECK_OUTPUT "remote command terminated"
                  set done 1
               }
               -i $sp_id "assword" {
                  add_proc_error "submit_wait_type_job" -1 "unexpected password question for user $user on host $host"
                  set done 1
               }
               -i $sp_id "*\n" {
                  set output $expect_out(buffer)
                  set output [ split $output "\n" ]
                  foreach line $output {
                     set line [string trim $line]
                     if { [string length $line] == 0 } {
                        continue
                     }
                     puts $CHECK_OUTPUT $line
                  }
               }
               -i $sp_id default {
               }
            }
         }
         close_spawn_process $sid
      }

      "qrsh" { ;# with sleeper job
         set sid [open_remote_spawn_process $CHECK_HOST $user qrsh "$remote_host_arg $job_argument"]
         set sp_id [lindex $sid 1]
         set timeout 1
         set max_timeouts 15
         set done 0
         while {!$done} {
            expect {
               -i $sp_id full_buffer {
                  add_proc_error "submit_with_method" -1 "expect full_buffer error"
                  set done 1
               }
               -i $sp_id timeout {
                  incr max_timeouts -1

                  set job_list [get_standard_job_info 0 0 1]
                  foreach job $job_list {
                     puts $CHECK_OUTPUT $job
                     if { [lindex $job 2] == "sleeper.sh" && [lindex $job 3] == $user } {
                        puts $CHECK_OUTPUT "qrsh job id is [lindex $job 0]"
                        set job_id [lindex $job 0]
                     }
                  }

                  if { $max_timeouts <= 0 } {
                     add_proc_error "submit_with_method" -1 "got 15 timeout errors - break"
                     set done 1 
                  }
               }
               -i $sp_id eof {
                  add_proc_error "submit_with_method" -1 "got eof"
                  set done 1
               }
               -i $sp_id "_start_mark_" {
                  puts $CHECK_OUTPUT "remote command started"
                  set done 0
               }
               -i $sp_id "_exit_status_" {
                  puts $CHECK_OUTPUT "remote command terminated"
                  set done 1
               }
               -i $sp_id "assword" {
                  add_proc_error "submit_wait_type_job" -1 "unexpected password question for user $user on host $host"
                  set done 1
               }
               -i $sp_id "*\n" {
                  set output $expect_out(buffer)
                  set output [ split $output "\n" ]
                  foreach line $output {
                     set line [string trim $line]
                     if { [string length $line] == 0 } {
                        continue
                     }
                     puts $CHECK_OUTPUT $line
                  }
               }
               -i $sp_id default {
               }
            }
         }
         close_spawn_process $sid
      }

      "qlogin" {
         puts $CHECK_OUTPUT "starting qlogin $remote_host_arg ..."
         set sid [open_remote_spawn_process $CHECK_HOST $user qlogin "$remote_host_arg"]
         set sp_id [lindex $sid 1]
         set timeout 1
         set max_timeouts 15
         set done 0
         while {!$done} {
            expect {
               -i $sp_id full_buffer {
                  add_proc_error "submit_with_method" -1 "expect full_buffer error"
                  set done 1
               }
               -i $sp_id timeout {
                  incr max_timeouts -1

                  if { $job_id == 0 } {
                     set job_list [get_standard_job_info 0 0 1]
                     foreach job $job_list {
                        puts $CHECK_OUTPUT $job
                        if { [lindex $job 2] == "QLOGIN" && [lindex $job 3] == $user } {
                           puts $CHECK_OUTPUT "qlogin job id is [lindex $job 0]"
                           set job_id [lindex $job 0]
                        }
                     }
                  }

                  if { $max_timeouts <= 0 } {
                     add_proc_error "submit_with_method" -1 "got 15 timeout errors - break"
                     set done 1 
                  }
               }
               -i $sp_id eof {
                  add_proc_error "submit_with_method" -1 "got eof"
                  set done 1
               }
               -i $sp_id "_start_mark_" {
                  puts $CHECK_OUTPUT "remote command started"
                  set done 0
               }
               -i $sp_id "_exit_status_" {
                  puts $CHECK_OUTPUT "remote command terminated"
                  set done 1
               }

               -i $sp_id "login:" {
                  send -i $sp_id "$user\n"
               }

               -i $sp_id "assword" { 
                  puts $CHECK_OUTPUT "got password question for user $user on host $host"
                  if { $job_id == 0 } {
                     set job_list [get_standard_job_info 0 0 1]
                     foreach job $job_list {
                        puts $CHECK_OUTPUT $job
                        if { [lindex $job 2] == "QLOGIN" && [lindex $job 3] == $user } {
                           puts $CHECK_OUTPUT "qlogin job id is [lindex $job 0]"
                           set job_id [lindex $job 0]
                        }
                     }
                  }
                  puts $CHECK_OUTPUT "deleting job with id $job_id, because we don't know user password ..."
                  delete_job $job_id
               }

               -i $sp_id "*\n" {
                  set output $expect_out(buffer)
                  set output [ split $output "\n" ]
                  foreach line $output {
                     set line [string trim $line]
                     if { [string length $line] == 0 } {
                        continue
                     }
                     puts $CHECK_OUTPUT $line
                  }
               }
               -i $sp_id default {
               }
            }
         }
         close_spawn_process $sid
      }

      "qsh" {
         puts $CHECK_OUTPUT "starting qsh $remote_host_arg on host $CHECK_HOST as user $user ..."
         puts $CHECK_OUTPUT "setting DISPLAY=$CHECK_DISPLAY_OUTPUT"
         
         set my_qsh_env(DISPLAY) $CHECK_DISPLAY_OUTPUT
         set abort_count 60
         set sid [open_remote_spawn_process $CHECK_HOST $user qsh "$remote_host_arg -now yes" 0 my_qsh_env]
         set sp_id [lindex $sid 1]
         set timeout 1
         set done 0
         while {!$done} {
            expect {
               -i $sp_id full_buffer {
                  add_proc_error "submit_with_method" -1 "expect full_buffer error"
                  set done 1
               }
               -i $sp_id timeout {

                  if { $job_id == 0 } {
                     set job_list [get_standard_job_info 0 0 1]
                     foreach job $job_list {
                        puts $CHECK_OUTPUT $job
                        if { [lindex $job 2] == "INTERACTIV" && [lindex $job 3] == $user && [lindex $job 4] == "r" } {
                           puts $CHECK_OUTPUT "qsh job id is [lindex $job 0]"
                           set job_id [lindex $job 0]
                        }
                     }
                  } else {
                        puts $CHECK_OUTPUT "ok, now deleting job $job_id ..."
                        delete_job $job_id
                        set done 1
                  }
                  incr abort_count -1
                  if { $abort_count <= 0 } {
                     add_proc_error "submit_wait_type_job" -1 "timeout waiting for start of $job_type job of user $user"
                     set done 1
                  }
               }
               -i $sp_id eof {
                  add_proc_error "submit_with_method" -1 "got eof"
                  set done 1
               }
               -i $sp_id "_start_mark_" {
                  puts $CHECK_OUTPUT "remote command started"
                  set done 0
               }
               -i $sp_id "_exit_status_" {
                  puts $CHECK_OUTPUT "remote command terminated"
                  set done 0
               }

               -i $sp_id "*\n" {
                  set output $expect_out(buffer)
                  set output [ split $output "\n" ]
                  foreach line $output {
                     set line [string trim $line]
                     if { [string length $line] == 0 } {
                        continue
                     }
                     puts $CHECK_OUTPUT $line
                  }
               }
            }
         }
         close_spawn_process $sid
      }

      "tight_integrated" {
         set master_task_id [submit_job "$remote_host_arg $output_argument -pe tight_job_start 1 $CHECK_PRODUCT_ROOT/examples/jobs/sleeper.sh 30" 1 60 "" $user]
         wait_for_jobstart $master_task_id "leeper" 30 1 1
         puts $CHECK_OUTPUT "tight integration job has been submitted, now submitting task ..."

         set my_tight_env(JOB_ID) $master_task_id
         set my_tight_env(SGE_TASK_ID) 1

         puts $CHECK_OUTPUT "starting qrsh -inherit $host $CHECK_PRODUCT_ROOT/examples/jobs/sleeper.sh 80 ..."
         set sid [open_remote_spawn_process $CHECK_HOST $user qrsh "-inherit $host $CHECK_PRODUCT_ROOT/examples/jobs/sleeper.sh 15" 0 my_tight_env]
         set sp_id [lindex $sid 1]
         set timeout 1
         set max_timeouts 30
         set done 0
         while {!$done} {
            expect {
               -i $sp_id full_buffer {
                  add_proc_error "submit_with_method" -1 "expect full_buffer error"
                  set done 1
               }
               -i $sp_id timeout {
                  incr max_timeouts -1

                  set job_list [get_standard_job_info 0 0 1]
                  foreach job $job_list {
                     puts $CHECK_OUTPUT $job
                     if { [lindex $job 2] == "Sleeper" && [lindex $job 3] == $user } {
                        puts $CHECK_OUTPUT "qrsh job id is [lindex $job 0]"
                        set job_id [lindex $job 0]
                        
                     }
                  }

                  if { $max_timeouts <= 0 } {
                     add_proc_error "submit_with_method" -1 "got 15 timeout errors - break"
                     set done 1 
                  }
               }
               -i $sp_id eof {
                  add_proc_error "submit_with_method" -1 "got eof"
                  set done 1
               }
               -i $sp_id "_start_mark_" {
                  puts $CHECK_OUTPUT "remote command started"
                  set done 0
               }
               -i $sp_id "_exit_status_" {
                  puts $CHECK_OUTPUT "remote command terminated"
                  set done 1
               }
               -i $sp_id "assword" {
                  add_proc_error "submit_wait_type_job" -1 "unexpected password question for user $user on host $host"
                  set done 1
               }
               -i $sp_id "*\n" {
                  set output $expect_out(buffer)
                  set output [ split $output "\n" ]
                  foreach line $output {
                     set line [string trim $line]
                     if { [string length $line] == 0 } {
                        continue
                     }
                     puts $CHECK_OUTPUT $line
                  }
               }
               -i $sp_id default {
               }
            }
         }
         close_spawn_process $sid

         wait_for_jobend $master_task_id "leeper" 60 1 1
      }

   }

   if { $job_id == 0 } {
      add_proc_error "submit_wait_type_job" -1 "could not submit \"$job_type\" job to host \"$host\" as user \"$user\". XWindow DISPLAY=$CHECK_DISPLAY_OUTPUT."
      return -1
   }

   
   puts $CHECK_OUTPUT "waiting for job $job_id to disapear ..."
   set my_timeout [timestamp]
   incr my_timeout 30
   while { [is_job_running $job_id "" ] != -1 } {
      after 500
      puts -nonewline $CHECK_OUTPUT "."
      flush $CHECK_OUTPUT
      if { [timestamp] > $my_timeout } {
         break
      }
   }
   puts $CHECK_OUTPUT ""



   # if job is now still running or pending, the job had problems => error
   if { [is_job_running $job_id "" ] != -1 } {
      add_proc_error "submit_wait_type_job" -1 "job still registered! Skipping test for \"$job_type\" job to host \"$host\" as user \"$user\". XWindow DISPLAY=$CHECK_DISPLAY_OUTPUT."
      return -1
   }
   

   set my_timeout [timestamp]
   incr my_timeout 60
   puts $CHECK_OUTPUT "waiting for accounting file to have information about job $job_id"
   while {[get_qacct $job_id qacctinfo "" "" 0] != 0 } {
      after 500
      puts -nonewline $CHECK_OUTPUT "."
      flush $CHECK_OUTPUT
      if { [timestamp] > $my_timeout } {
         break
      }
   }

   puts $CHECK_OUTPUT "waiting for accounting file to have information about job $job_id slave"
   if { $job_type == "tight_integrated" } {
      set my_timeout [timestamp]
      incr my_timeout 60
      while { 1 } {
         get_qacct $job_id qacctinfo
         if { [llength $qacctinfo(exit_status)] == 2 } {
            break
         } 
         after 500
         puts -nonewline $CHECK_OUTPUT "."
         flush $CHECK_OUTPUT
         if { [timestamp] > $my_timeout } {
            break
         }
      }
   }

   puts $CHECK_OUTPUT ""

   if {[get_qacct $job_id qacctinfo] != 0} {
      return -1
   }

   return $job_id
}

#****** sge_procedures/submit_time_job() ***************************************
#  NAME
#     submit_time_job() -- Submit a job with execution time
#
#  SYNOPSIS
#     submit_time_job { jobargs } 
#
#  FUNCTION
#     This procedure will submit a job with the -a option. The start time
#     is set to function call time + 2 min.
#
#  INPUTS
#     jobargs - job arguments (e.g. -o ... -e ... job start script path)
#
#  RESULT
#     job id
#
#  SEE ALSO
#     sge_procedures/submit_error_job()
#     sge_procedures/submit_waitjob_job()
#     sge_procedures/submit_time_job()
#*******************************************************************************
proc submit_time_job { jobargs } {

   set hour   [exec date "+%H"]
   set minute [exec date "+%M"]

   if { [string first "0" $hour] == 0 } {
      set hour [string index $hour 1 ]
   }  
   if { [string first "0" $minute] == 0 } {
      set minute [string index $minute 1 ]
   }  
  
   if {$minute < 58 } {
     set minute [expr ($minute + 2) ]
   } else {
     set minute [expr ($minute + 2 - 60) ]
      if {$hour < 23 } {
         set hour [expr ($hour + 1) ]
      } else {
         set hour "00"
      }
   }

   set rhour $hour
   set rminute $minute

   if {$hour < 10} {
     set rhour "0$hour"
   }
   if {$minute < 10} {
     set rminute "0$minute"
   }

   set start "[exec date +\%Y\%m\%d]$rhour$rminute"
   set result [submit_job "-a $start $jobargs"] 
   return $result
}


#****** sge_procedures/submit_waitjob_job() ************************************
#  NAME
#     submit_waitjob_job() -- submit job with hold_jid (wait for other job)
#
#  SYNOPSIS
#     submit_waitjob_job { jobargs wait_job_id } 
#
#  FUNCTION
#     This procedure will submit a job with hold_jid option set. This means that
#     the job is not started while an other job is running
#
#  INPUTS
#     jobargs     - additional job arguments ( jobscript, -e -o option ...)
#     wait_job_id - job id to wait for
#
#  RESULT
#     job id of hold_jid job
#
#  SEE ALSO
#     sge_procedures/submit_error_job()
#     sge_procedures/submit_waitjob_job()
#     sge_procedures/submit_time_job()
#*******************************************************************************
proc submit_waitjob_job { jobargs wait_job_id} {
   return [submit_job "-hold_jid $wait_job_id $jobargs"]
}


#****** sge_procedures/get_loadsensor_path() ***********************************
#  NAME
#     get_loadsensor_path() -- get loadsensor for host
#
#  SYNOPSIS
#     get_loadsensor_path { host } 
#
#  FUNCTION
#     This procedure will read the load sensor path for the given host
#
#  INPUTS
#     host - hostname to get loadsensor for
#
#  RESULT
#     full path name of loadsensor 
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc get_loadsensor_path { node } {
   global ts_config
   global CHECK_OUTPUT
   global ts_host_config
   
   set host [node_get_host $node]

   set loadsensor ""

   set arch [resolve_arch $host]

   # if we have a custom loadsensor defined in hostconfig, use this one
   if {[info exists ts_host_config($host,loadsensor)]} {
      set loadsensor $ts_host_config($host,loadsensor)
   } else {
      add_proc_error "get_loadsensor_path" -1 "no host configuration found for host \"$host\""
   }

   # if we have no custom loadsensor
   # but we are on aix, we need the ibm-loadsensor to get the default load values
   if { $loadsensor == "" } {
      if {$arch == "aix43" || $arch == "aix51"} {
         set loadsensor "$ts_config(product_root)/util/resources/loadsensors/ibm-loadsensor"
      }   
   }

   return $loadsensor
}

#
#                                                             max. column:     |
#
#****** sge_procedures/get_gid_range() ******
#  NAME
#     get_gid_range() -- get gid range for user   
#
#  SYNOPSIS
#     get_gid_range { user port } 
#
#  FUNCTION
#     This procedure ist used in the install_core_system test. It returns the
#     gid range of the requested user and port
#
#  INPUTS
#     user - user name
#     port - port number on which the cluster commd is running
#
#  RESULT
#     gid range, e.g. 13501-13700
#
#  SEE ALSO
#     ???/???
#*******************************
#
proc get_gid_range { user port } {
   global ts_config

  global CHECK_OUTPUT
  global ts_user_config

  if { [ info exists ts_user_config($port,$user) ] } {
     return $ts_user_config($port,$user)
  }
  add_proc_error "get_gid_range" -1 "no gid range defined for user $user on port $port"
  return ""
}


#                                                             max. column:     |
#****** sge_procedures/move_qmaster_spool_dir() ******
# 
#  NAME
#     move_qmaster_spool_dir -- ??? 
#
#  SYNOPSIS
#     move_qmaster_spool_dir { new_spool_dir } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     new_spool_dir - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************
proc move_qmaster_spool_dir { new_spool_dir } {
   global ts_config
  global CHECK_CORE_MASTER CHECK_OUTPUT CHECK_USER 
  global CHECK_HOST

  set old_spool_dir [ get_qmaster_spool_dir ]
 
  
  if { [ string compare $old_spool_dir $new_spool_dir ] == 0 } {
     add_proc_error "move_qmaster_spool_dir" -1 "old and new spool dir are the same"
     return
  }
 
  if { [ string compare "unknown" $old_spool_dir] == 0 } { 
     add_proc_error "move_qmaster_spool_dir" -1 "can't get qmaster spool dir"
     return
  }

  if { [string length $new_spool_dir] <= 10  } {
     # just more security (do not create undefined dirs or something like that)
     add_proc_error "move_qmaster_spool_dir" -1 "please use path with size > 10 characters"
     return
  } 

  # change qmaster spool dir and shutdown the qmaster and scheduler
#  this was the old way before 5.3.beta2. Now we can't change qmaster_spool_dir in
#  a running system. We have to do it manually.
#  set change_array(qmaster_spool_dir) $new_spool_dir
#  set_config change_array "global"
  shutdown_master_and_scheduler $CHECK_CORE_MASTER $old_spool_dir

  set vi_commands ""
 #  ":%s/^$elem .*$/$elem  $newVal/\n"
  set newVal1 [split $new_spool_dir {/}]
  set newVal [join $newVal1 {\/}]

  lappend vi_commands ":%s/^qmaster_spool_dir .*$/qmaster_spool_dir    $newVal/\n"
  set vi_binary [get_binary_path $CHECK_HOST "vim"]
  # JG: TODO: this is version dependent! Use set_qmaster_spool_dir instead!
  set result [ handle_vi_edit "$vi_binary" "$ts_config(product_root)/$ts_config(cell)/common/bootstrap" "$vi_commands" "" ] 
  puts $CHECK_OUTPUT "result: \"$result\""
  if { $result != 0 } {
     add_proc_error "shadowd_kill_master_and_scheduler" -1 "edit error when changing global configuration"
  } 

  set_qmaster_spool_dir $new_spool_dir

  puts $CHECK_OUTPUT "make copy of spool directory ..."
  # now copy the entries  
  set result [ start_remote_tcl_prog $CHECK_CORE_MASTER $CHECK_USER "file_procedures.tcl" "copy_directory" "$old_spool_dir $new_spool_dir" ]
  if { [ string first "no errors" $result ] < 0 } {
      add_proc_error "shadowd_kill_master_and_scheduler" -1 "error moving qmaster spool dir"
  }

  puts $CHECK_OUTPUT "starting up qmaster ..."
  startup_qmaster
  wait_for_load_from_all_queues 300

  set changed_spool_dir [ get_qmaster_spool_dir ]
  if { [string compare $changed_spool_dir $new_spool_dir] != 0 } {
     add_proc_error "shadowd_kill_master_and_scheduler" -1 "error changing qmaster spool dir"
  }

}

#                                                             max. column:     |
#****** sge_procedures/get_config() ******
# 
#  NAME
#     get_config -- get global or host configuration settings
#
#  SYNOPSIS
#     get_config { change_array {host "global"} } 
#
#  FUNCTION
#     Get the global or host specific configuration settings.
#
#  INPUTS
#     change_array    - name of an array variable that will get set by 
#                       get_config
#     {host "global"} - get configuration for a specific hostname (host) 
#                       or get the global configuration (global)
#
#  RESULT
#     The change_array variable is build as follows:
#
#     set change_array(xterm)   "/bin/xterm"
#     set change_array(enforce_project) "true"
#     ...
#     0  - on success
#     -1 - on error
#     
#
#  EXAMPLE
#     get_config gcluster1 lobal
#     puts $cluster1(qmaster_spool_dir)
#     
#     Here the possible change_array values with some typical settings:
#     
#     execd_spool_dir      /../$SGE_CELL/spool
#     qsi_common_dir       /../$SGE_CELL/common/qsi
#     mailer               /usr/sbin/Mail
#     xterm                /usr/bin/X11/xterm
#     load_sensor          none
#     prolog               none
#     epilog               none
#     shell_start_mode     posix_compliant
#     login_shells         sh,ksh,csh,tcsh
#     min_uid              0
#     min_gid              0
#     user_lists           none
#     xuser_lists          none
#     projects             none
#     xprojects            none
#     load_report_time     00:01:00
#     stat_log_time        12:00:00
#     max_unheard          00:02:30
#     loglevel             log_info
#     enforce_project      false
#     administrator_mail   none
#     set_token_cmd        none
#     pag_cmd              none
#     token_extend_time    none
#     shepherd_cmd         none
#     qmaster_params       none
#     execd_params         none
#     finished_jobs        0
#     gid_range            13001-13100
#     admin_user           crei
#     qlogin_command       telnet
#     qlogin_daemon        /usr/etc/telnetd
#     
#
#  SEE ALSO
#     sge_procedures/set_config()
#*******************************
proc get_config { change_array {host global}} {
  global ts_config
  global CHECK_ARCH CHECK_OUTPUT
  upvar $change_array chgar

  if { [info exists chgar] } {
     unset chgar
  }

  set catch_result [ catch {  eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-sconf" "$host"} result ]
  if { $catch_result != 0 } {
     add_proc_error "get_config" "-1" "qconf error or binary not found ($ts_config(product_root)/bin/$CHECK_ARCH/qconf)\n$result"
     return -1
  } 

  # split each line as listelement
  set help [split $result "\n"]
  foreach elem $help {
     set id [lindex $elem 0]
     set value [lrange $elem 1 end]
     if { [string compare $value ""] != 0 } {
       set chgar($id) $value
     }
  }
  return 0
}

#                                                             max. column:     |
#****** sge_procedures/set_config() ******
# 
#  NAME
#     set_config -- change global or host specific configuration
#
#  SYNOPSIS
#     set_config { change_array {host global}{do_add 0} } 
#
#  FUNCTION
#     Set the cluster global or exec host local configuration corresponding to 
#     the content of the change_array.
#
#  INPUTS
#     change_array  - name of an array variable that will be set by get_config
#     {host global} - set configuration for a specific hostname (host) or set
#                     the global configuration (global)
#     {do_add 0}    - if 1: this is a new configuration, no old one exists)
#
#  RESULT
#     -1 : timeout
#      0 : ok
#
#     The change_array variable is build as follows:
#
#     set change_array(xterm)   "/bin/xterm"
#     set change_array(enforce_project) "true"
#     ...
#     (every value that is set will be changed)
#
#
#  EXAMPLE
#     get_config gcluster1 lobal
#     set cluster1(execd_spool_dir) "/bla/bla/tmp"
#     set_config cluster1
#     
#     Here the possible change_array values with some typical settings:
#     
#     execd_spool_dir      /../$SGE_CELL/spool
#     qsi_common_dir       /../$SGE_CELL/common/qsi
#     mailer               /usr/sbin/Mail
#     xterm                /usr/bin/X11/xterm
#     load_sensor          none
#     prolog               none
#     epilog               none
#     shell_start_mode     posix_compliant
#     login_shells         sh,ksh,csh,tcsh
#     min_uid              0
#     min_gid              0
#     user_lists           none
#     xuser_lists          none
#     projects             none
#     xprojects            none
#     load_report_time     00:01:00
#     stat_log_time        12:00:00
#     max_unheard          00:02:30
#     loglevel             log_info
#     enforce_project      false
#     administrator_mail   none
#     set_token_cmd        none
#     pag_cmd              none
#     token_extend_time    none
#     shepherd_cmd         none
#     qmaster_params       none
#     execd_params         none
#     finished_jobs        0
#     gid_range            13001-13100
#     admin_user           crei
#     qlogin_command       telnet
#     qlogin_daemon        /usr/etc/telnetd
#
#  SEE ALSO
#     sge_procedures/get_config()
#*******************************
proc set_config { change_array {host global} {do_add 0} {ignore_error 0}} {
  global ts_config
  global env CHECK_ARCH CHECK_OUTPUT
  global CHECK_CORE_MASTER CHECK_USER

   upvar $change_array chgar
   set values [array names chgar]

   # get old config - we want to compare it to new one
   if { $do_add == 0 } {
      set qconf_cmd "-mconf"
      get_config old_values $host
   } else {
      set qconf_cmd "-aconf"
      set old_values(xyz) "abc"
   }

   set vi_commands [build_vi_command chgar old_values]
  
  set GIDRANGE [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_CONFIG_CONF_GIDRANGELESSTHANNOTALLOWED_I] "*"]

  set EDIT_FAILED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_PARSE_EDITFAILED]]

  if { $ts_config(gridengine_version) == 53 } {
     set MODIFIED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_CONFIG_MODIFIEDINLIST_SSS] $CHECK_USER "*" "*"]
     set ADDED    [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_CONFIG_ADDEDTOLIST_SSS] $CHECK_USER "*" "*"]
     set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "$qconf_cmd $host" $vi_commands $MODIFIED $EDIT_FAILED $ADDED $GIDRANGE ]
  } else {
     set MODIFIED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" "*" "*"]
     set ADDED    [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" "*" "*"]
     set EFFECT   [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_WARN_CHANGENOTEFFECTEDUNTILRESTARTOFEXECHOSTS] "execd_spool_dir"]
     set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "$qconf_cmd $host" $vi_commands $MODIFIED $EDIT_FAILED $ADDED $GIDRANGE $EFFECT]
  }

  if { ($ignore_error == 1) && ($result == -4) } {
     # ignore error -4 
  } else {
    if { ($result != 0) && ($result != -3) && ($result != -5) }  {
      add_proc_error "set_config" -1 "could not add or modify configuration for host $host ($result)"
    }
  }
  return $result
}

#****** set_config_and_propagate() *********************************************
#  NAME
#     set_config_and_propagate() -- set the config for the given host
#
#  SYNOPSIS
#     set_config_and_propagate { my_config {host global} } 
#
#  FUNCTION
#     Set the given config for the given host, and wait until the change has
#     propagated.  It knows that the change has progated when the last config
#     entry change appears in an execd's messages file.  If the host is global,
#     an execd is selected from the list of execution daemons.  This method
#     opens a remote process as $ts_user_config(first_foreign_user).
#
#  INPUTS
#     config    the configuration to set
#     host      the host for which the configuration should be set.  Defaults
#               to global
#*******************************************************************************
proc set_config_and_propagate { config {host global} } {
   global CHECK_OUTPUT CHECK_USER CHECK_HOST ts_config ts_user_config
   global job_environment_config 

   upvar $config my_config

   if {[array size my_config] > 0} {
      set conf_host $host

      if {$conf_host == "global"} {
         set conf_host [lindex $ts_config(execd_hosts) 0]
         set spool_dir $job_environment_config(execd_spool_dir)
      } elseif {[info exists job_environment_config(execd_spool_dir)] == 1} {
         set spool_dir $job_environment_config(execd_spool_dir)
      } else {
         get_config global_config
         set spool_dir $global_config(execd_spool_dir)
      }

      # Begin watching messages file for changes
      set messages_name "$spool_dir/$conf_host/messages"
      set tail_id [open_remote_spawn_process $conf_host $ts_user_config(first_foreign_user) "/usr/bin/tail" "-f $messages_name"]
      set sp_id [ lindex $tail_id 1 ]

      # Make configuration change
      set_config my_config $host

      # Wait for change to propagate
      puts $CHECK_OUTPUT "Waiting for configuration change to propagate to execd"

      set last_name [lindex [array names my_config] end]

      set timeout 90

      expect {
         -i $sp_id full_buffer {
          add_proc_error "job_environment_set_config" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
         }
         -i $sp_id timeout {
            add_proc_error "job_environment_set_config" -1 "setup failed (timeout waiting for config to change)"
         }
         -i $sp_id "\|execd\|$conf_host\|I\|using \"$my_config($last_name)\" for $last_name" {
            puts $CHECK_OUTPUT "Configuration changed: $last_name = \"$my_config($last_name)\""
         }
      }

      # Stop watching
      close_spawn_process $tail_id
      wait_for_load_from_all_queues 200 
   }
}

proc compare_complex {a b} {
   set len [llength $a]

   if { $len != [llength $b] } {
      return 1
   }

   # compare shortcut (case sensitive)
   if {[string compare [lindex $a 0] [lindex $b 0]] != 0} {
      return 1
   }
   # compare the complex entry element by element
   for {set i 1} {$i < $len} {incr i} {
      if {[string compare -nocase [lindex $a $i] [lindex $b $i]] != 0} {
         return 1
      }
   }

   return 0
}





#                                                             max. column:     |
#****** sge_procedures/add_exechost() ******
# 
#  NAME
#     add_exechost -- Add a new exechost configuration object
#
#  SYNOPSIS
#     add_exechost { change_array {fast_add 1} } 
#
#  FUNCTION
#     Add a new execution host configuration object corresponding to the content of 
#     the change_array.
#
#  INPUTS
#     change_array - name of an array variable can contain special settings
#     {fast_add 1} - if not 0 the add_exechost procedure will use a file for
#                    queue configuration. (faster) (qconf -Ae, not qconf -ae)
#
#  RESULT
#     -1   timeout error
#     -2   host already exists
#      0   ok 
#
#  EXAMPLE
#     set new_host(hostname) "test"
#     add_exechost new_host
#
#  NOTES
#     the array should look like this:
#
#     set change_array(hostname) MYHOST.domain
#     ....
#     (every value that is set will be changed)
#
#     here is a list of all valid array names (template host):
#
#     change_array(hostname)                    "template"
#     change_array(load_scaling)                "NONE"
#     change_array(complex_list)                "NONE"
#     change_array(complex_values)              "NONE"
#     change_array(user_lists)                  "NONE"
#     change_array(xuser_lists)                 "NONE"
#
#     additional names for an enterprise edition system:
#     change_array(projects)                    "NONE"
#     change_array(xprojects)                   "NONE"
#     change_array(usage_scaling)               "NONE"
#     change_array(resource_capability_factor)  "0.000000"
#*******************************
proc add_exechost { change_array {fast_add 1} } {
  global ts_config
  global env CHECK_ARCH
  global CHECK_OUTPUT CHECK_TESTSUITE_ROOT 
  global CHECK_CORE_MASTER

  upvar $change_array chgar
  set values [array names chgar]

    if { $fast_add != 0 } {
     # add queue from file!
     set default_array(hostname)          "template"
     set default_array(load_scaling)      "NONE"
     set default_array(complex_list)      "NONE"
     set default_array(complex_values)    "NONE"
     set default_array(user_lists)        "NONE"
     set default_array(xuser_lists)       "NONE"
  
     if { $ts_config(product_type) == "sgeee" } {
       set default_array(projects)                    "NONE"
       set default_array(xprojects)                   "NONE"
       set default_array(usage_scaling)               "NONE"
       set default_array(resource_capability_factor)  "0.000000"
     }
  
     foreach elem $values {
        set value [set chgar($elem)]
        puts $CHECK_OUTPUT "--> setting \"$elem\" to \"$value\""
        set default_array($elem) $value
     }

     if {[file isdirectory "$CHECK_TESTSUITE_ROOT/testsuite_trash"] != 1} {
        file mkdir "$CHECK_TESTSUITE_ROOT/testsuite_trash"
     }

     set tmpfile "$CHECK_TESTSUITE_ROOT/testsuite_trash/tmpfile"
     set file [open $tmpfile "w"]
     set values [array names default_array]
     foreach elem $values {
        set value [set default_array($elem)]
        puts $file "$elem                   $value"
     }
     close $file

     set result ""
     set catch_return [ catch {  eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -Ae ${tmpfile}" } result ]
     puts $CHECK_OUTPUT $result
     set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_EXEC_ADDEDHOSTXTOEXECHOSTLIST_S] "*"]

     if { [string match "*$ADDED" $result] == 0 } {
        add_proc_error "add_exechost" "-1" "qconf error or binary not found"
        return
     }
     return
  }

  set vi_commands [build_vi_command chgar]

  set ALREADY_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" "*" ]
  set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_EXEC_ADDEDHOSTXTOEXECHOSTLIST_S] "*"]

  set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-ae" $vi_commands $ADDED $ALREADY_EXISTS ]  
  if { $result != 0 } {
     add_proc_error "add_exechost" -1 "could not add queue [set chgar(qname)] (error: $result)"
  }
  return $result
}

#****** sge_procedures/get_scheduling_info() ***********************************
#  NAME
#     get_scheduling_info() -- get scheduling information 
#
#  SYNOPSIS
#     get_scheduling_info { job_id { check_pending 1 } } 
#
#  FUNCTION
#     This procedure starts the get_qstat_j_info() procedure and returns
#     the "scheduling info" value. The procedure returns ALLWAYS a valid
#     text string.
#
#  INPUTS
#     job_id              - job id
#     { check_pending 1 } - 1(default): do a wait_forjob_pending first
#                           0         : no wait_for_jobpending() call         
#
#  RESULT
#     scheduling info text
#
#  SEE ALSO
#     sge_procedures/get_qstat_j_info()
#     sge_procedures/wait_forjob_pending()
# 
#*******************************************************************************
proc get_scheduling_info { job_id { check_pending 1 }} {
  global ts_config
   global CHECK_OUTPUT CHECK_ARCH CHECK_CORE_MASTER

   if { $check_pending == 1 } {
      set result [ wait_for_jobpending $job_id "leeper" 120 ]
      if { $result != 0 } {
         return "job not pending"
      }
   }
   trigger_scheduling

   set my_timeout [ expr ( [timestamp] + 30 ) ] 
   puts $CHECK_OUTPUT "waiting for scheduling info information ..."
   while { 1 } {
      if { [get_qstat_j_info $job_id ] } {
         set help_var_name "scheduling info" 
         if { [info exists qstat_j_info($help_var_name)] } {
            set sched_info $qstat_j_info($help_var_name)
         } else {
            set sched_info "no messages available"
         }
         set help_var_name [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SCHEDD_SCHEDULINGINFO]] 
         if { [info exists qstat_j_info($help_var_name)] } {
            set sched_info $qstat_j_info($help_var_name)
         } else {
            set sched_info "no messages available"
         }


         set INFO_NOMESSAGE [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SCHEDD_INFO_NOMESSAGE]]

         if { [string first "no messages available" $sched_info] < 0 && [string first $INFO_NOMESSAGE $sched_info] < 0 } {
            puts $CHECK_OUTPUT ""
            puts $CHECK_OUTPUT $sched_info
            return $sched_info
         }
      }
      puts -nonewline $CHECK_OUTPUT "."
      flush $CHECK_OUTPUT
      after 500
      if { [timestamp] > $my_timeout } {
         puts $CHECK_OUTPUT ""
         return "timeout"
      }
   }   
}

#                                                             max. column:     |
#****** sge_procedures/add_userset() ******
# 
#  NAME
#     add_userset -- add a userset with qconf -Au
#
#  SYNOPSIS
#     add_userset { change_array } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     change_array - Array with the values for the userset
#               Values:
#                  name    name of the userset (required)
#                  type    ACL, DEPT
#                  fshare  functional shares
#                  oticket overwrite tickets
#                  entries user list
#
#  RESULT
#     0    userset added
#     else error
#
#  EXAMPLE
#     set  userset_conf(name)    "dep0"
#     set  userset_conf(type)    "DEPT"
#     set  userset_conf(oticket) "1000"
#     set  userset_conf(entries) "codtest1"
#     
#     add_userset userset_conf
#
#  NOTES
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     tcl_files/sge_procedures/add_access_list
#     tcl_files/sge_procedures/del_access_list
#
#*******************************
proc add_userset { change_array } {
   global ts_config
   global CHECK_ARCH 
   global CHECK_CORE_MASTER CHECK_USER CHECK_HOST CHECK_USER CHECK_OUTPUT
   upvar $change_array chgar
   set values [array names chgar]
   
   if { [ string compare $ts_config(product_type) "sge" ] == 0 } {
     add_proc_error "add_userset" -1 "not possible for sge systems"
     return -3
   }
   set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" "*" "*"]
   set ALREADY_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" "*"]
   
   set f_name [get_tmp_file_name]
   set fd [open $f_name "w"]
   set org_struct(name)    "template"
   set org_struct(type)    "ACL DEPT"
   set org_struct(oticket) "0"
   set org_struct(fshare)  "0"
   set org_struct(entries) "NONE"
   foreach elem $values {
     set org_struct($elem) $chgar($elem)
   }
   set ogr_struct_names [array names org_struct]
   foreach elem  $ogr_struct_names {
     puts $CHECK_OUTPUT "$elem $org_struct($elem)"
     puts $fd "$elem $org_struct($elem)"
   }
   close $fd 
   puts $CHECK_OUTPUT "using file $f_name"
   set result [start_remote_prog $CHECK_HOST $CHECK_USER "qconf" "-Au $f_name"]
   if { $prg_exit_state != 0 } {
     add_proc_error "add_userset" -1 "error running qconf -Au"
   }
   set result [string trim $result]
   puts $CHECK_OUTPUT "\"$result\""
   #     puts $CHECK_OUTPUT "\"$ADDED\"" 
   if { [ string match "*$ADDED" $result] } {
     return 0
   }
   if { [ string match "*$ALREADY_EXISTS" $result] } {
     add_proc_error "add_userset" -1 "\"[set chgar(name)]\" already exists"
     return -2
   }
   add_proc_error "add_userset" -1 "\"error adding [set chgar(name)]\""
   return -100
}

#****** sge_procedures/add_access_list() ***************************************
#  NAME
#     add_access_list() -- add user access list
#
#  SYNOPSIS
#     add_access_list { user_array list_name } 
#
#  FUNCTION
#     This procedure starts the qconf -au command to add a new user access list.
#
#  INPUTS
#     user_array - tcl array with user names
#     list_name  - name of the new list
#
#  RESULT
#     -1 on error, 0 on success
#
#  SEE ALSO
#     sge_procedures/del_access_list()
#
#*******************************************************************************
proc add_access_list { user_array list_name } {
  global ts_config
  global CHECK_ARCH CHECK_OUTPUT CHECK_CORE_MASTER 

  set arguments ""
  foreach elem $user_array {
     append arguments "$elem,"
  }
  append arguments " $list_name"

  set result ""
  set catch_return [ catch {  
      eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -au $arguments" 
  } result ]
  puts $CHECK_OUTPUT $result
  set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_GDI_ADDTOACL_SS ] $user_array $list_name]
  if { [string first "added" $result ] < 0 && [string first $ADDED $result ] < 0 } {
     add_proc_error "add_access_list" "-1" "could not add access_list $list_name"
     return -1
  }
  return 0
}


#****** sge_procedures/del_user_from_access_list() ***************************************
#  NAME
#     del_user_from_access_list() -- delete a user from an access list
#
#  SYNOPSIS
#     del_user_from_access_list { user_name list_name } 
#
#  FUNCTION
#     This procedure starts the qconf -du command to delete a user from a 
#     access list
#
#  INPUTS
#     user_name - name of the user
#     list_name - name of access list
#
#  RESULT
#      1  User was not in the access_list
#      0  User deleted from the access_list
#     -1 on error
#
#  EXAMPLE
#
#    set result [ del_user_from_access_list "codtest1" "deadlineusers" ]
#
#    if { $result == 0 } {
#       puts $CHECK_OUTPUT "user codtest1 deleted from access list deadlineusers"
#    } elseif { $result == 1 } {
#       puts $CHECK_OUTPUT "user codtest1 did not exist on the access list deadlineusers"
#    } else {
#       set_error -1 "Can not delete user codtest1 from access list deadlineusers"
#    }
# 
#  SEE ALSO
# 
#*******************************************************************************
proc del_user_from_access_list { user_name list_name  } {
  global ts_config
  global CHECK_ARCH CHECK_OUTPUT
  global CHECK_CORE_MASTER CHECK_USER

  set result ""
  set catch_return [ catch {  
      eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -du $user_name $list_name" 
  } result ]
  puts $CHECK_OUTPUT $result
  set NOT_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_GDI_USERNOTINACL_SS] $user_name $list_name]
  set DELETED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_GDI_DELFROMACL_SS] $user_name $list_name]

  if { $result == $NOT_EXISTS } {
     return 1
  } elseif { $result == $DELETED } {
     return 0
  } else {
     add_proc_error "del_user_from_access_list" "-1" "Can not delete user $user_name from access list $list_name"
     return -1
  }
  return 0
}


#****** sge_procedures/del_access_list() ***************************************
#  NAME
#     del_access_list() -- delete user access list
#
#  SYNOPSIS
#     del_access_list { list_name } 
#
#  FUNCTION
#     This procedure starts the qconf -dul command to delete a user access
#     list.
#
#  INPUTS
#     list_name - name of access list to delete
#
#  RESULT
#     -1 on error, 0 on success
#
#  SEE ALSO
#     sge_procedures/add_access_list()
# 
#*******************************************************************************
proc del_access_list { list_name } {
  global ts_config
  global CHECK_ARCH CHECK_OUTPUT
  global CHECK_CORE_MASTER CHECK_USER

  set result ""
  set catch_return [ catch {  
      eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -dul $list_name" 
  } result ]
   puts $CHECK_OUTPUT $result
  set USER [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_OBJ_USERSET]]
  set REMOVED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" $list_name $USER]
  puts $CHECK_OUTPUT $REMOVED

  if { [ string match "*$REMOVED" $result ] == 0 } {
     add_proc_error "add_access_list" "-1" "could not delete access_list $list_name"
     return -1
  }
  return 0
}

#                                                             max. column:     |
#****** sge_procedures/add_user() ******
# 
#  NAME
#     add_user -- ??? 
#
#  SYNOPSIS
#     add_user { change_array } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     change_array - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************
proc add_user {change_array {from_file 0}} {
  global ts_config
  global CHECK_ARCH 
  global CHECK_CORE_MASTER CHECK_USER CHECK_HOST CHECK_USER CHECK_OUTPUT
  upvar $change_array chgar
  set values [array names chgar]

  if { [ string compare $ts_config(product_type) "sge" ] == 0 } {
     add_proc_error "add_user" -1 "not possible for sge systems"
     return -3
  }
  set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" "*" "*"]
  set ALREADY_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" "*"]

  if { $from_file != 0 } {
     set f_name [get_tmp_file_name]
     set fd [open $f_name "w"]
     set org_struct(name) "template"
     set org_struct(oticket) "0"
     set org_struct(fshare) "0"
     if {$ts_config(gridengine_version) != 53} {
        set org_struct(delete_time) "0"
     }
     set org_struct(default_project) "NONE"
     foreach elem $values {
        set org_struct($elem) $chgar($elem)
     }
     set ogr_struct_names [array names org_struct]
     foreach elem  $ogr_struct_names {
        puts $CHECK_OUTPUT "$elem $org_struct($elem)"
        puts $fd "$elem $org_struct($elem)"
     }
     close $fd 
     puts $CHECK_OUTPUT "using file $f_name"
     set result [start_remote_prog $CHECK_HOST $CHECK_USER "qconf" "-Auser $f_name"]
     if { $prg_exit_state != 0 } {
        add_proc_error "add_user" -1 "error running qconf -Auser"
     }
     set result [string trim $result]
     puts $CHECK_OUTPUT "\"$result\""
#     puts $CHECK_OUTPUT "\"$ADDED\"" 
     if { [ string match "*$ADDED" $result] } {
        return 0
     }
     if { [ string match "*$ALREADY_EXISTS" $result] } {
        add_proc_error "add_user" -1 "\"[set chgar(name)]\" already exists"
        return -2
     }
     add_proc_error "add_user" -1 "\"error adding [set chgar(name)]\""
     return -100
  }

  set vi_commands [build_vi_command chgar]

  set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-auser" $vi_commands $ADDED $ALREADY_EXISTS ]
  
  if {$result == -1 } { add_proc_error "add_user" -1 "timeout error" }
  if {$result == -2 } { add_proc_error "add_user" -1 "\"[set chgar(name)]\" already exists" }
  if {$result != 0  } { add_proc_error "add_user" -1 "could not add user \"[set chgar(name)]\"" }

  return $result
}

#****** sge_procedures/mod_user() **********************************************
#  NAME
#     mod_user() -- ???
#
#  SYNOPSIS
#     mod_user { change_array { from_file 0 } }
#
#  FUNCTION
#     modify user with qconf -muser or -Muser
#
#  INPUTS
#     change_array    - array name with settings to modifiy
#                       (e.g. set my_settings(default_project) NONE )
#                       -> array name "name" must be set (for username)
#
#     { from_file 0 } - if 0: use -mconf , else use -Mconf
#
#  RESULT
#     0 on success
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc mod_user { change_array { from_file 0 } } {
  global ts_config
  global CHECK_ARCH
  global CHECK_CORE_MASTER CHECK_USER CHECK_HOST CHECK_USER CHECK_OUTPUT
  upvar $change_array chgar
  set values [array names chgar]

  if { [ string compare $ts_config(product_type) "sge" ] == 0 } {
     add_proc_error "mod_user" -1 "not possible for sge systems"
     return -3
  }
  set MODIFIED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS ] $CHECK_USER "*" "*" "*"]
  set ALREADY_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" "*"]

  if { $from_file != 0 } {
     set orginal_settings [ start_remote_prog $CHECK_HOST $CHECK_USER "qconf" "-suser $chgar(name)" ]
     if { $prg_exit_state != 0 } {
        add_proc_error "mod_user" -1 "\"error modify user [set chgar(name)]\""
        return -100
     }

     if { [string first "default_project" $orginal_settings] < 0  } {
         add_proc_error "mod_user" -1 "\"error modify user [set chgar(name)]\""
        return -100
     }

     set orginal_settings [split $orginal_settings "\n"]

     set config_elements "name oticket fshare default_project"
     if {$ts_config(gridengine_version) != 53} {
        lappend config_elements "delete_time"
     }
     foreach elem $config_elements {
        foreach line $orginal_settings {
           set line [string trim $line]
           if { [ string compare $elem [lindex $line 0] ] == 0 } {
              set org_struct($elem) [lrange $line 1 end]
           }
        }
     }
     set ogr_struct_names [array names org_struct]
     foreach elem $ogr_struct_names {
        puts $CHECK_OUTPUT ">$elem=$org_struct($elem)<"
     }
     set f_name [get_tmp_file_name]
     set fd [open $f_name "w"]
     foreach elem $values {
        set org_struct($elem) $chgar($elem)
     }
     set ogr_struct_names [array names org_struct]
     foreach elem  $ogr_struct_names {
        puts $CHECK_OUTPUT "$elem $org_struct($elem)"
        puts $fd "$elem $org_struct($elem)"
     }
     close $fd
     puts $CHECK_OUTPUT "using file $f_name"
     set result [start_remote_prog $CHECK_HOST $CHECK_USER "qconf" "-Muser $f_name"]
     if { $prg_exit_state != 0 } {
        add_proc_error "mod_user" -1 "error running qconf -Auser"
     }
     set result [string trim $result]
     puts $CHECK_OUTPUT "\"$result\""
     puts $CHECK_OUTPUT "\"$MODIFIED\""
     if { [ string match "*$MODIFIED" $result] } {
        return 0
     }
     if { [ string match "*$ALREADY_EXISTS" $result] } {
        add_proc_error "mod_user" -1 "\"[set chgar(name)]\" already exists"
        return -2
     }
     add_proc_error "mod_user" -1 "\"modifiy user error  [set chgar(name)]\""
     return -100
  }

  set vi_commands [build_vi_command chgar]

  set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" " -muser $chgar(name)" $vi_commands $MODIFIED $ALREADY_EXISTS ]

  if {$result == -1 } { add_proc_error "mod_user" -1 "timeout error" }
  if {$result == -2 } { add_proc_error "mod_user" -1 "\"[set chgar(name)]\" already exists" }
  if {$result != 0  } { add_proc_error "mod_user" -1 "could not mod user \"[set chgar(name)]\"" }

  return $result
}


#                                                             max. column:     |
#****** sge_procedures/del_pe() ******
# 
#  NAME
#     del_pe -- delete parallel environment object definition
#
#  SYNOPSIS
#     del_pe { mype_name } 
#
#  FUNCTION
#     This procedure will delete a existing parallel environment, defined with
#     sge_procedures/add_pe.
#
#  INPUTS
#     mype_name - name of parallel environment to delete
#
#  RESULT
#     0  - ok
#     -1 - timeout error
#
#  SEE ALSO
#     sge_procedures/add_pe()
#*******************************
proc del_pe { mype_name } {
   global ts_config 
  global CHECK_ARCH CHECK_CORE_MASTER CHECK_USER CHECK_HOST
  global CHECK_OUTPUT

   unassign_queues_with_pe_object $mype_name

  set REMOVED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" $mype_name "*" ]

  log_user 0 
  set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-dp $mype_name"]

  set sp_id [ lindex $id 1 ]

  set result -1
  set timeout 30 	
  log_user 0 

  expect {
    -i $sp_id full_buffer {
      set result -1
      add_proc_error "del_pe" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
    }
    -i $sp_id $REMOVED {
      set result 0
    }
    -i $sp_id "removed" {
      set result 0
    }

    -i $sp_id default {
      set result -1
    }
   
  }
  close_spawn_process $id
  log_user 1
  if { $result != 0 } {
     add_proc_error "del_pe" -1 "could not delete pe \"$mype_name\""
  }
  return $result

}

#                                                             max. column:     |
#****** sge_procedures/del_calendar() ******
# 
#  NAME
#     del_calendar -- ??? 
#
#  SYNOPSIS
#     del_calendar { mycal_name } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     mycal_name - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************
proc del_calendar { mycal_name } {
  global ts_config
  global CHECK_ARCH CHECK_CORE_MASTER CHECK_USER CHECK_HOST
  
  set REMOVED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" $mycal_name "*" ]

  log_user 0
  set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-dcal $mycal_name"]

  set sp_id [ lindex $id 1 ]
  set timeout 30 
  set result -1	
  log_user 0 

  expect {
    -i $sp_id full_buffer {
      set result -1
      add_proc_error "del_calendar" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
    }
    -i $sp_id "removed" {
      set result 0
    }
    -i $sp_id $REMOVED {
      set result 0
    }

    -i $sp_id default {
      set result -1
    }
    
  }
  close_spawn_process $id
  log_user 1

  if { $result != 0 } {
     add_proc_error "del_calendar" -1 "could not delete calendar \"$mycal_name\""
  }

  return $result
}

#                                                             max. column:     |
#****** sge_procedures/was_job_running() ******
# 
#  NAME
#     was_job_running -- look for job accounting
#
#  SYNOPSIS
#     was_job_running { jobid {do_errorcheck 1} } 
#
#  FUNCTION
#     This procedure will start a qacct -j jobid. If the hob was not found in
#     the output of the qacct command, this function will return -1. This
#     means that the job is still running, or was never running.
#
#  INPUTS
#     jobid             - job identification number
#     {do_errorcheck 1} - 1: call add_proc_error if job was not found
#                         0: do not generate error messages
#
#  RESULT
#     "-1"  : if job was not found
#     or the output of qacct -j
#
#  SEE ALSO
#     check/add_proc_error()
#*******************************
proc was_job_running {jobid {do_errorcheck 1} } {
  global ts_config
  global CHECK_ARCH check_timestamp CHECK_OUTPUT
# returns
# -1 on error
# qacct in listform else

  set mytime [timestamp]

  if { $mytime == $check_timestamp } {
     puts $CHECK_OUTPUT "was_job_running - waiting for job ..."
     after 1000
  }
  set check_timestamp $mytime

  set catch_state [ catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qacct" "-j" "$jobid" } result ]
  
  if { $catch_state != 0 } {
      if {$do_errorcheck == 1 } { 
            puts $CHECK_OUTPUT "debug: $result"
            add_proc_error "was_job_running" -1 "job \"($jobid)\" not found"
      }
      return -1
  }

  set return_value [lreplace $result 0 0]
  return $return_value
}


proc match_queue {qname qlist} {
   global CHECK_OUTPUT

   puts $CHECK_OUTPUT "trying to match $qname to queues in $qlist"

   set ret {}
   foreach q $qlist {
      if {[string match "$qname*" $q] == 1} {
         lappend ret $q
      }
   }

   return $ret
}

#                                                             max. column:     |
#****** sge_procedures/slave_queue_of() ******
# 
#  NAME
#     slave_queue_of -- Get the last slave queue of a parallel job
#
#  SYNOPSIS
#     slave_queue_of { job_id {qlist {}}} 
#
#  FUNCTION
#     This procedure will return the name of the last slave queue of a
#     parallel job or "" if the SLAVE queue was not found.
#
#     If a queue list is passed via qlist parameter, the queue name returned
#     by qstat will be matched against the queue names in qlist.
#     This is sometimes necessary, if the queue name printed by qstat is 
#     truncated (SGE(EE) 5.3, long hostnames).
#
#  INPUTS
#     job_id - Identification number of the job
#     qlist  - a list of queues - one has to match the slave_queue.
#
#  RESULT
#     empty or the last queue name on which the SLAVE task is running 
#
#  SEE ALSO
#     sge_procedures/master_queue_of()
#*******************************
proc slave_queue_of { job_id {qlist {}}} {
   global ts_config
   global CHECK_OUTPUT
# return last slave queue of job
# no slave -> return ""
   puts $CHECK_OUTPUT "Looking for SLAVE QUEUE of Job $job_id."
   set slave_queue ""         ;# return -1, if there is no slave queue
   
   set result [get_standard_job_info $job_id]
   foreach elem $result {
      set whatami [lindex $elem 8]
      if { [string compare $whatami "SLAVE"] == 0 } {
         set slave_queue [lindex $elem 7]
         if {[llength $qlist] > 0} {
            # we have to match queue name from qstat against qlist
            set matching_queues [match_queue $slave_queue $qlist]
            if {[llength $matching_queues] == 0} {
               add_proc_error "slave_queue_of" -1 "no queue is matching queue list"
            } else {
               if {[llength $matching_queues] > 1} {
                  add_proc_error "slave_queue_of" -1 "multiple queues are matching queue list"
               } else {
                  set slave_queue [lindex $matching_queues 0]
               }
            }
         }
         puts $CHECK_OUTPUT "Slave is running on queue \"$slave_queue\""
      }
   }

   #set slave_queue [get_cluster_queue $slave_queue]
   
   if {$slave_queue == ""} {
     add_proc_error "slave_queue_of" -1 "no slave queue for job $job_id found"
   } 

   return $slave_queue
}

#                                                             max. column:     |
#****** sge_procedures/master_queue_of() ******
# 
#  NAME
#     master_queue_of -- get the master queue of a parallel job
#
#  SYNOPSIS
#     master_queue_of { job_id {qlist {}}} 
#
#  FUNCTION
#     This procedure will return the name of the master queue of a
#     parallel job or "" if the MASTER queue was not found.
#
#     If a queue list is passed via qlist parameter, the queue name returned
#     by qstat will be matched against the queue names in qlist.
#     This is sometimes necessary, if the queue name printed by qstat is 
#     truncated (SGE(EE) 5.3, long hostnames).
#
#  INPUTS
#     job_id - Identification number of the job
#     qlist  - a list of queues - one has to match the slave_queue.
#
#  RESULT
#     empty or the last queue name on which the MASTER task is running 
#
#  SEE ALSO
#     sge_procedures/slave_queue_of()
#*******************************
proc master_queue_of { job_id {qlist {}}} {
  global ts_config
   global CHECK_OUTPUT
# return master queue of job
# no master -> return "": ! THIS MAY NOT HAPPEN !
   puts $CHECK_OUTPUT "Looking for MASTER QUEUE of Job $job_id."
   set master_queue ""         ;# return -1, if there is no master queue
   
   set result [get_standard_job_info $job_id]
   foreach elem $result {
      set whatami [lindex $elem 8]
      if { [string compare $whatami "MASTER"] == 0 } { 
         set master_queue [lindex $elem 7]
         if {[llength $qlist] > 0} {
            # we have to match queue name from qstat against qlist
            set matching_queues [match_queue $master_queue $qlist]
            if {[llength $matching_queues] == 0} {
               add_proc_error "master_queue_of" -1 "no queue is matching queue list"
            } else {
               if {[llength $matching_queues] > 1} {
                  add_proc_error "master_queue_of" -1 "multiple queues are matching queue list"
               } else {
                  set master_queue [lindex $matching_queues 0]
               }
            }
         }
         puts $CHECK_OUTPUT "Master is running on queue \"$master_queue\""
         break                  ;# break out of loop, if Master found
       }
   }

   #set master_queue [get_cluster_queue $master_queue]

   if {$master_queue == ""} {
     add_proc_error "master_queue_of" -1 "no master queue for job $job_id found"
   }

   return $master_queue
}


#                                                             max. column:     |
#****** sge_procedures/wait_for_load_from_all_queues() ******
# 
#  NAME
#     wait_for_load_from_all_queues -- wait for load value reports from queues
#
#  SYNOPSIS
#     wait_for_load_from_all_queues { seconds } 
#
#  FUNCTION
#     This procedure waits until all queues are reporting a load value smaller
#     than 99. If this is the case all execd should be successfully connected 
#     to the qmaster.
#
#  INPUTS
#     seconds - timeout value in seconds
#
#  RESULT
#     "-1" on error
#
#  SEE ALSO
#     sge_procedures/wait_for_load_from_all_queues()
#     sge_procedures/wait_for_unknown_load()
#     file_procedures/wait_for_file()
#     sge_procedures/wait_for_jobstart()
#     sge_procedures/wait_for_end_of_transfer()
#     sge_procedures/wait_for_jobpending()
#     sge_procedures/wait_for_jobend()
#*******************************
proc wait_for_load_from_all_queues { seconds } {
  global ts_config
   global check_errno check_errstr CHECK_ARCH CHECK_OUTPUT

   set time [timestamp]

   while { 1 } {
      puts $CHECK_OUTPUT "waiting for load value report from all queues ..."
      after 1000
      set result ""
      set catch_return [ catch {exec "$ts_config(product_root)/bin/$CHECK_ARCH/qstat" "-f"} result ]
      if { $catch_return == 0 } {
         # split each line as listelement
         set help [split $result "\n"]
       
         #remove first line
         set help [lreplace $help 0 0]
         set data ""
       
         #get every line after "----..." line 
         set len [llength $help]
         for {set ind 0} {$ind < $len } {incr ind 1} {
            if {[lsearch [lindex $help $ind] "------*"] >= 0 } {
               lappend data [lindex $help [expr ( $ind + 1 )]] 
            }
         }
      
         set qcount [ llength $data]
         set qnames ""
         set slots ""
         set load ""
      
         # get line data information for queuename used/tot and load_avg
         foreach elem $data {
            set linedata $elem
            lappend qnames [lindex $linedata 0]
            set used_tot [lindex $linedata 2]
            set pos1 [ expr ( [string first "/" $used_tot] + 1 ) ]
            set pos2 [ expr ( [string length $used_tot]          - 1 ) ]
      
            lappend slots [string range $used_tot $pos1 $pos2 ]
            lappend load [lindex $linedata 3]
         }
      
         # check if load of an host is set > 99 (no exed report)
         set failed 0
         foreach elem $load {
           if {$elem == "-NA-" || $elem >= 99} {
              incr failed 1
           }
         }
    
         if { $failed == 0 } {
            return 0
         }
      } else {
        puts $CHECK_OUTPUT "qstat error or binary not found"
      }

      set runtime [expr ( [timestamp] - $time) ]
      if { $runtime >= $seconds } {
          add_proc_error "wait_for_load_from_all_queues" -1 "timeout waiting for load values < 99"
          return -1
      }
   }
}


#****** sge_procedures/wait_for_job_state() ************************************
#  NAME
#     wait_for_job_state() -- wait for job to become special job state
#
#  SYNOPSIS
#     wait_for_job_state { jobid state wait_timeout } 
#
#  FUNCTION
#     This procedure is checking the job state of the given job id by parsing
#     the qstat -f command. If the job has the state given in the parameter
#     state the procedure returns the job state. If an timeout occurs the 
#     procedure returns -1.
#
#  INPUTS
#     jobid        - job id of job to check state
#     state        - state to check for
#     wait_timeout - given timeout in seconds
#
#  RESULT
#     job state or -1 on error
#
#  SEE ALSO
#     sge_procedures/wait_for_queue_state()
#*******************************************************************************
proc wait_for_job_state { jobid state wait_timeout } {
  global ts_config

   global CHECK_OUTPUT 

   set my_timeout [ expr ( [timestamp] + $wait_timeout ) ]
   while { 1 } {
      puts $CHECK_OUTPUT "waiting for job $jobid to become job state ${state} ..."
      after 1000
      set job_state [get_job_state $jobid]
      if { [string first $state $job_state] >= 0 } {
         return $job_state
      }
      if { [timestamp] > $my_timeout } {
         add_proc_error "wait_for_job_state" -1 "timeout waiting for job $jobid to get in \"$state\" state"
         return -1
      }
   }
}

#****** sge_procedures/wait_for_queue_state() **********************************
#  NAME
#     wait_for_queue_state() -- wait for queue to become special error state
#
#  SYNOPSIS
#     wait_for_queue_state { queue state wait_timeout } 
#
#  FUNCTION
#     This procedure is checking the queue by parsing the qstat -f command. 
#
#  INPUTS
#     queue        - name of queue to check
#     state        - state to check for
#     wait_timeout - given timeout in seconds
#
#  RESULT
#     queue state or -1 on error
#
#  SEE ALSO
#     sge_procedures/wait_for_job_state()
#*******************************************************************************
proc wait_for_queue_state { queue state wait_timeout } {
  global ts_config
   global CHECK_OUTPUT

   set my_timeout [ expr ( [timestamp] + $wait_timeout ) ]
   while { 1 } {
      puts $CHECK_OUTPUT "waiting for queue $queue to get in \"${state}\" state ..."
      after 1000
      set q_state [get_queue_state $queue]
      if { [string first $state $q_state] >= 0 } {
         return $q_state
      }
      if { [timestamp] > $my_timeout } {
         add_proc_error "wait_for_queue_state" -1 "timeout waiting for queue $queue to get in \"${state}\" state"
         return -1
      }
   }
}


#****** sge_procedures/soft_execd_shutdown() ***********************************
#  NAME
#     soft_execd_shutdown() -- soft shutdown of execd
#
#  SYNOPSIS
#     soft_execd_shutdown { host } 
#
#  FUNCTION
#     This procedure starts a qconf -ke $host. If qconf reports an error,
#     the execd is killed by the shtudown_system_deamon() procedure.
#     After that qstat is called. When the load value for the given 
#     host-queue ($host.q) is 99.99 the procedure returns without error.
#     
#  INPUTS
#     host - execution daemon host to shutdown
#
#  RESULT
#     0 - success
#    -1 - error
#
#  SEE ALSO
#     sge_procedures/shutdown_system_daemon()
#*******************************************************************************
proc soft_execd_shutdown { host } {
  global ts_config
 
   global CHECK_CORE_MASTER CHECK_OUTPUT
   global CHECK_USER CHECK_ARCH

   set tries 0
   while { $tries <= 8 } {   
      incr tries 1
      set result [ start_remote_prog $CHECK_CORE_MASTER $CHECK_USER $ts_config(product_root)/bin/$CHECK_ARCH/qconf "-ke $host" ]
      if { $prg_exit_state != 0 } {
         puts $CHECK_OUTPUT "qconf -ke $host returned $prg_exit_state, hard killing execd"
         shutdown_system_daemon $host execd
      }
      set load [wait_for_unknown_load 15 "${host}.q" 0]
      if { $load == 0 } {
         puts $CHECK_OUTPUT "execd on host $host reports 99.99 load value"
         return 0
      }
   }
   add_proc_error "soft_execd_shutdown" -1 "could not shutdown execd on host $host"
   return -1
}

#****** sge_procedures/wait_for_unknown_load() *********************************
#  NAME
#     wait_for_unknown_load() -- wait for load to get >= 99 for a list of queues
#
#  SYNOPSIS
#     wait_for_unknown_load { seconds queue_array { do_error_check 1 } } 
#
#  FUNCTION
#     This procedure is starting the qstat -f command and parse the output for
#     the queue load values. If the load value of the given queue(s) have a value
#     greater than 99 the procedure will return. If not an error message is
#     generated after timeout.
#
#  INPUTS
#     seconds        - number of seconds to wait before creating timeout error
#     queue_array    - an array of queue names for which to wait
#     do_error_check - (optional) if 1: report errors
#                                 if 0: don't report errors
#
#  SEE ALSO
#     sge_procedures/wait_for_load_from_all_queues()
#  
#*******************************************************************************
proc wait_for_unknown_load { seconds queue_array { do_error_check 1 } } {
  global ts_config
   global check_errno check_errstr CHECK_ARCH CHECK_OUTPUT

   set time [timestamp]

   if { [ file isfile $ts_config(product_root)/bin/$CHECK_ARCH/qstat ] != 1} {
      return -1      
   }

   while { 1 } {
      after 1000
      puts $CHECK_OUTPUT "wait_for_unknown_load - waiting for queues\n\"$queue_array\"\nto get unknown load state ..."
      set result ""
      set catch_return [ catch {exec "$ts_config(product_root)/bin/$CHECK_ARCH/qstat" "-f"} result ]
      if { $catch_return == 0 } {
         # split each line as listelement
         set help [split $result "\n"]
       
         #remove first line
         set help [lreplace $help 0 0]
         set data ""
       
         #get every line after "----..." line 
         set len [llength $help]
         for {set ind 0} {$ind < $len } {incr ind 1} {
            if {[lsearch [lindex $help $ind] "------*"] >= 0 } {
               lappend data [lindex $help [expr ( $ind + 1 )]] 
            }
         }
      
         set qcount [ llength $data]
         set qnames ""
         set slots ""
         set load ""
      
         # get line data information for queuename used/tot and load_avg
         foreach elem $data {
            set linedata $elem

            set queue_name [lindex $linedata 0]
            set load_value [lindex $linedata 3]
            set load_values($queue_name) $load_value
         }
      
         # check if load of an host is set > 99 (no exed report)
         set failed 0
         
         foreach queue $queue_array {
            if { [info exists load_values($queue)] == 1 } {
               if { $load_values($queue) < 99 } {
                   incr failed 1
                   if { [string compare $load_values($queue) "-NA-"] == 0 } {
                      incr failed -1
                   }
               } 
            }
         }

         if { $failed == 0 } {
            return 0
         }
      } else {
        puts $CHECK_OUTPUT "qstat error or binary not found"
        if { $do_error_check == 1 } {
           add_proc_error "wait_for_unknown_load" -1 "qstat error"
        }
        return -1
      }

      set runtime [expr ( [timestamp] - $time) ]
      if { $runtime >= $seconds } {
          if { $do_error_check == 1 } {
             add_proc_error "wait_for_unknown_load" -1 "timeout waiting for load values >= 99"
          }
          return -1
      }
   }
   return 0
}


#
#                                                             max. column:     |
#
#****** sge_procedures/wait_for_end_of_all_jobs() ******
#  NAME
#     wait_for_end_of_all_jobs() -- wait for end of all jobs
#
#  SYNOPSIS
#     wait_for_end_of_all_jobs { seconds } 
#
#  FUNCTION
#     This procedure will wait until no further jobs are remaining in the cluster.
#
#  INPUTS
#     seconds - timeout value (if < 1 no timeout is set)
#
#  RESULT
#     0 - ok
#    -1 - timeout
#
#  SEE ALSO
#     sge_procedures/wait_for_jobend()
#*******************************
#
proc wait_for_end_of_all_jobs { seconds } {
  global ts_config
   global check_errno check_errstr CHECK_ARCH CHECK_OUTPUT

   set time [timestamp]

   while { 1 } {
      puts $CHECK_OUTPUT "waiting for end of all jobs ..."
      after 1000
      set result ""
      set catch_return [ catch {eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qstat -s pr" } result ]
      if { $catch_return == 0 } {
         if { [ string compare $result "" ] == 0 } {
            puts $CHECK_OUTPUT "no further jobs in system"
            return 0
         }

         # split each line as listelement
         set help [split $result "\n"]
 
         #remove first two lines
         set help [lreplace $help 0 1]
         
         foreach elem $help {
            puts $CHECK_OUTPUT $elem
         }
      } else {
        puts $CHECK_OUTPUT "qstat error or binary not found"
      }

      if { $seconds > 0 } {
         set runtime [expr ( [timestamp] - $time) ]
         if { $runtime >= $seconds } {
             add_proc_error "wait_for_end_of_all_jobs" -1 "timeout waiting for end of all jobs:\n\"$result\""
             return -1
         }
      }
   }
}

#                                                             max. column:     |
#****** sge_procedures/mqattr() ******
# 
#  NAME
#     mqattr -- Modify queue attributes
#
#  SYNOPSIS
#     mqattr { attribute entry queue_list { add_error 1  } 
#
#  FUNCTION
#     This procedure enables the caller to modify particular queue attributes.
#     Look at set_queue for queue attributes.
#
#  INPUTS
#     attribute  - name of attribute to modify
#     entry      - new value for attribute
#     queue_list - name of queues to change
#     add_error  - execute add_proc_error
#
#  RESULT
#     -1 - error
#     0  - ok
#
#  EXAMPLE
#     set return_value [mqattr "calendar" "always_disabled" "$queue_list"]
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue() 
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc mqattr { attribute entry queue_list { add_error 1 } } {
  global ts_config
# returns
# -1 on error
# 0 on success
  
  global CHECK_ARCH CHECK_CORE_MASTER CHECK_OUTPUT CHECK_USER

  puts "Trying to change attribute $attribute of queues $queue_list to $entry."

  set help "$attribute \"$entry\""   ;# create e.g. slots "5" as string
  set catch_return [ catch {  
    eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -rattr queue $help $queue_list" 
  } result ]
  if { $catch_return != 0 } {
     if { $add_error == 1} {
         add_proc_error "mqattr" "-1" "qconf error or binary not found"
     }
     return -1
  }

  # split each line as listelement
  set help [split $result "\n"]
  set counter 0
  set return_value 0
  
  foreach elem $help {
     set queue_name [lindex $queue_list $counter]
     set MODIFIED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" "*" "*"]
     if { [ string match "$MODIFIED" $elem ] != 1 } {
        puts $CHECK_OUTPUT "Could not modify queue $queue_name."
        set return_value -1
     } else {
        puts "Modified queue $queue_name successfully."
     }
     incr counter 1
   }                                       

   if { $return_value != 0 } {
     if { $add_error == 1} {
       add_proc_error "mqattr" -1 "could not modify queue \"$queue_name\""
     }
   }

   return $return_value
}

#****** sge_procedures/mhattr() ************************************************
#  NAME
#     mhattr() -- Modify host qttributes 
#
#  SYNOPSIS
#     mhattr { attribute entry host_name { add_error 1 } } 
#
#  FUNCTION
#     This procedure enables the caller to moidify particular host attributes. 
#     Look at set_exechost for host attributes.
#
#  INPUTS
#     attribute       - name of attribute to modify 
#     entry           - new value for attribute 
#     host_name       - name of the host to change 
#     { add_error 1 } - execute add_proc_error
#
#  RESULT
#     -1 - error
#     0  - ok
#
#  EXAMPLE
#     set return_value [mhattr "complex" "bla=test" "$host_name" ]
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_exechost() 
#*******************************************************************************
proc mhattr { attribute entry host_name { add_error 1 } } {
  global ts_config
# returns
# -1 on error
# 0 on success
  
  global CHECK_ARCH CHECK_CORE_MASTER CHECK_OUTPUT CHECK_USER

  puts "Trying to change attribute $attribute for host $host_name to $entry."

  set help "$attribute \"$entry\""   ;# create e.g. slots "5" as string
  set catch_return [ catch {  
    eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -rattr exechost $help $host_name" 
  } result ]
  if { $catch_return != 0 } {
     if { $add_error == 1} {
         add_proc_error "mhattr" "-1" "qconf error or binary not found"
     }
     return -1
  }

  # split each line as listelement
  set return_value 0
 
   
  set MODIFIED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" "*" "*"]
  if { [ string match "$MODIFIED" $result ] != 1 } {
     puts $CHECK_OUTPUT "Could not modify host $host_name."
     set return_value -1
  } else {
     puts "Modified host $host_name successfully."
  }

  if { $return_value != 0 } {
     if { $add_error == 1} {
       add_proc_error "mhattr" -1 "could not modify host \"$host_name\""
     }
  }

  return $return_value
}


#****** sge_procedures/mod_attr() ******************************************
#  NAME
#     mod_attr() -- modify an attribute 
#
#  SYNOPSIS
#     mod_attr { object attribute value target {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}}
#
#  FUNCTION
#     Modifies attribute of object with value for object_instance
#
#  INPUTS
#     object       - object we are modifying 
#     attribute    - attribute of object we are modifying 
#     value        - value of attribute of object we are modifying 
#     target       - target object
#     {fast_add 1} - 0: modify the attribute using qconf -mattr, 
#                  - 1: modify the attribute using qconf -Mattr, faster
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     integer value  0 on success, -2 on error
#
#*******************************************************************************
proc mod_attr { object attribute value target {fast_add 1} {on_host ""} {as_user ""} {raise_error 1} } {
   global ts_config
   global CHECK_ARCH CHECK_OUTPUT CHECK_USER CHECK_HOST

   puts $CHECK_OUTPUT "Modifying object \"$object\" attribute  \"$attribute\" value \"$value\" for target \"$target\" "

   # add queue from file?
    if { $fast_add } {
      set default_array($attribute) "$value"
      set tmpfile [dump_array_to_tmpfile default_array]
      set result [start_sge_bin "qconf" "-Mattr $object $tmpfile $target"  $on_host $as_user]
  
      if {$prg_exit_state == 0} {
         set ret 0
      } else {
         set ret [mod_attr_file_error $result $object $attribute $tmpfile $target $raise_error]   
      }

   } else {
      # add by -mattr

      set result [start_sge_bin "qconf" "-mattr  $object $attribute $value $target" $on_host $as_user ] 
      if {$prg_exit_state == 0} {
         set ret 0
      } else {
         set ret [mod_attr_error $result $object $attribute $value $target $raise_error]
      }

   }

   return $ret
}

#****** sge_procedures/mod_attr_error() ***************************************
#  NAME
#     mod_attr_error() -- error handling for mod_attr
#
#  SYNOPSIS
#     mod_attr_error {result object attribute value target raise_error }
#
#  FUNCTION
#     Does the error handling for mod_attr.
#     Translates possible error messages of qconf -mattr,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     mod_attr. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     object      - object qconf is modifying
#     tmpfile     - temp file for qconf -Mattr
#     attribute    - attribute of object we are modifying
#     value        - value of attribute of object we are modifying
#     target      - target object
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for mod_attr function:
#      -1: "wrong_attr" is not an attribute
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc mod_attr_file_error {result object attribute tmpfile target raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) "error: [translate_macro MSG_UNKNOWNATTRIBUTENAME_S $attribute ]"

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "mod_attr" "qconf -Mattr $object $tmpfile $target" $result messages $raise_error]

   return $ret
}

proc mod_attr_error {result object attribute value target raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_PARSE_BAD_ATTR_ARGS_SS $attribute $value]

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "mod_attr" "qconf -mattr $object $attribute $value $target" $result messages $raise_error]

   return $ret
}



#****** sge_procedures/get_attr() ******************************************
#  NAME
#     get_attr() -- get an attribute
#
#  SYNOPSIS
#     get_attr {object attribute  target {on_host ""} {as_user ""} {raise_error 1}}
#
#  FUNCTION
#     Get attribute of object 
#
#  INPUTS
#     object       - object we are getting 
#     attribute    - attribute of object we are modifying
#     target       - target object
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     {raise_error 1} - raise an error condition on error (default), or just
#                       output the error message to stdout
#
#  RESULT
#     array of attribute information
#
#*******************************************************************************
proc get_attr { object attribute target {on_host ""} {as_user ""} {raise_error 1} } {

   return [get_qconf_list "get_attr" "-sobjl $object $attribute $target " out $on_host $as_user $raise_error]

}
#****** sge_procedures/del_attr() ******************************************
#  NAME
#     del_attr() -- Delete an attribute
#
#  SYNOPSIS
#     del_attr { object attribute value target {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}}
#
#  FUNCTION
#     Delete attribute of object
#
#  INPUTS
#     object       - object we are deleting 
#     attribute    - attribute of queue we are deleting 
#     value        - value of attribute we are deleting 
#     target       - target object
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     {fast_add 1} - 0: modify the attribute using qconf -dattr,
#                  - 1: modify the attribute using qconf -Dattr, faster
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     integer value  0 on success, -2 on error
#
#*******************************************************************************
proc del_attr { object attribute value target {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}} {
   global ts_config
   global CHECK_ARCH CHECK_OUTPUT CHECK_USER CHECK_HOST

   puts $CHECK_OUTPUT "Deleting attribute \"$attribute\" for object \"$object\""

   # add queue from file?
    if { $fast_add } {
      set default_array($attribute) "$value"
      set tmpfile [dump_array_to_tmpfile default_array]
      set result [start_sge_bin "qconf" "-Dattr $object $tmpfile $target" $on_host $as_user]

      if {$prg_exit_state == 0} {
         set ret 0
      } else {
         set ret [del_attr_file_error $result $object $tmpfile  $attribute $target $raise_error]
      }


   } else {
   # add by -dattr

      set result [start_sge_bin "qconf" "-dattr $object $attribute $value $target" $on_host $as_user]

      if {$prg_exit_state == 0} {
         set ret 0
      } else {
         set ret [del_attr_error $result $object $attribute $value $target $raise_error]
      }

   }
 
   return $ret
}

#****** sge_procedures/del_attr_error() ***************************************
#  NAME
#     del_attr_error() -- error handling for del_attr_
#
#  SYNOPSIS
#     del_attr_error {result object attribute value target raise_error }
#
#  FUNCTION
#     Does the error handling for mod_attr.
#     Translates possible error messages of qconf -dattr,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     mod_attr. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     object      - object qconf is modifying
#     attribute    - attribute of object we are modifying
#     value        - value of attribute of object we are modifying
#     target      - target object
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for del_attr function:
#      -1: "wrong_attr" is not an attribute
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc del_attr_error {result object attribute value target raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_PARSE_BAD_ATTR_ARGS_SS $attribute $value]

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "del_attr" "qconf -dattr $object $attribute $value $target" $result messages $raise_error]

   return $ret
}

proc del_attr_file_error {result object tmpfile attribute target raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_UNKNOWNATTRIBUTENAME_S $attribute ]

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "del_attr" "qconf -Dattr $object $tmpfile $target " $result messages $raise_error]

   return $ret
}


#****** sge_procedures/add_attr() ******************************************
#  NAME
#    add_attr () -- add an attribute
#
#  SYNOPSIS
#     add_attr { object attribute value target {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}}
#
#  FUNCTION
#     Modifies attribute of object with value for object_instance
#
#  INPUTS
#     object       - object we are modifying 
#     attribute    - attribute of queue we are modifying 
#     value        - value of attribute of object we are modifying 
#     target       - target object
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     {fast_add 1} - 0: modify the attribute using qconf -aattr,
#                  - 1: modify the attribute using qconf -Aattr, faster
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     integer value  0 on success, -2 on error
#
#*******************************************************************************
proc add_attr { object attribute value target {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}} {
   global ts_config
   global CHECK_ARCH CHECK_OUTPUT CHECK_USER CHECK_HOST

   puts $CHECK_OUTPUT "Adding attribute \"$attribute\" for object \"$object\""

   # add queue from file?
    if { $fast_add } {
      set default_array($attribute) "$value"
      set tmpfile [dump_array_to_tmpfile default_array]
      set result [start_sge_bin "qconf" "-Aattr $object $tmpfile $target" $on_host $as_user]

      if {$prg_exit_state == 0} {
         set ret 0
      } else {
         set ret [add_attr_file_error $result $object $tmpfile  $attribute $target $raise_error]
      }


   } else {
      # add by -aattr

      set result [start_sge_bin "qconf" "-aattr  $object $attribute $value $target" $on_host $as_user]
      if {$prg_exit_state == 0} {
         set ret 0
      } else {
         set ret [add_attr_error $result $object $attribute $value $target $raise_error]
      }

   }

   return $ret
}

#****** sge_procedures/add_attr_error() ***************************************
#  NAME
#     add_attr_error() -- error handling for add_attr
#
#  SYNOPSIS
#     add_attr_error {result object attribute value target raise_error }
#
#  FUNCTION
#     Does the error handling for add_attr.
#     Translates possible error messages of qconf -aattr,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     add_attr. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     object      - object qconf is modifying
#     attribute    - attribute of object we are modifying
#     value        - value of attribute of object we are modifying
#     target      - target object
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for add_attr function:
#      -1: "wrong_attr" is not an attribute
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc add_attr_error {result object attribute value target raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_PARSE_BAD_ATTR_ARGS_SS $attribute $value]

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "add_attr" "qconf -aattr $object $attribute $value $target" $result messages $raise_error]

   return $ret
}

proc add_attr_file_error {result object tmpfile attribute target raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_UNKNOWNATTRIBUTENAME_S $attribute ]

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "add_attr" "qconf -Aattr $object $tmpfile $target " $result messages $raise_error]

   return $ret
}


#****** sge_procedures/replace_attr() ******************************************
#  NAME
#     replace_attr() -- Replace an attribute
#
#  SYNOPSIS
#     replace_attr {object attribute value target {fast_add 1} {on_host ""} {as_user ""} {raise_error 1} }
#
#  FUNCTION
#     Replace attribute of object
#
#  INPUTS
#     object       - object we are deleting 
#     attribute    - attribute of object we are deleting 
#     value        - value of attribute we are deleting
#     target       - target object
#     {fast_add 1} - 0: modify the attribute using qconf -rattr,
#                  - 1: modify the attribute using qconf -Rattr, faster
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     integer value  0 on success, -2 on error
#
#*******************************************************************************
proc replace_attr { object attribute value target {fast_add 1} {on_host ""} {as_user ""} {raise_error 1} } {
   global ts_config
   global CHECK_ARCH CHECK_OUTPUT CHECK_USER CHECK_HOST

   puts $CHECK_OUTPUT "Replacing attribute \"$attribute\" of object \"$object\""

   # add queue from file?
    if { $fast_add } {
      set default_array($attribute) "$value"
      set tmpfile [dump_array_to_tmpfile default_array]
      set result [start_sge_bin "qconf" "-Rattr $object $tmpfile $target" $on_host $as_user]

      if {$prg_exit_state == 0} {
         set ret 0
      } else {
         set ret [replace_attr_file_error $result $object $attribute $tmpfile $target $raise_error] }

   } else {
   # add by -rattr

   set result [start_sge_bin "qconf" "-rattr $object $attribute $value $target" $on_host $as_user ]

      if {$prg_exit_state == 0} {
         set ret 0
      } else {
         set ret [replace_attr_error $result $object $attribute $value $target $raise_error]
      }

   }

   return $ret

}

#****** sge_procedures/replace_attr_error() ***************************************
#  NAME
#     replace_attr_error() -- error handling for replace_attr
#
#  SYNOPSIS
#     replace_attr_error {result object attribute value target raise_error }
#
#  FUNCTION
#     Does the error handling for mod_attr.
#     Translates possible error messages of qconf -rattr,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     replace_attr. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     object      - object qconf is modifying
#     attribute    - attribute of object we are modifying
#     value        - value of attribute of object we are modifying
#     target      - target object
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for replace_attr function:
#      -1: "wrong_attr" is not an attribute
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc replace_attr_error {result object attribute value target raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_PARSE_BAD_ATTR_ARGS_SS $attribute $value]

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "replace_attr" "qconf -rattr $object $attribute $value $target" $result messages $raise_error]

   return $ret
}


proc replace_attr_file_error {result object attribute tmpfile target raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) "error: [translate_macro MSG_UNKNOWNATTRIBUTENAME_S $attribute ]"

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "replace_attr" "qconf -Rattr $object $tmpfile $target" $result messages $raise_error]

   return $ret
}

#                                                             max. column:     |
#****** sge_procedures/suspend_job() ******
# 
#  NAME
#     suspend_job -- set job in suspend state
#
#  SYNOPSIS
#     suspend_job { id } 
#
#  FUNCTION
#     This procedure will call qmod to suspend the given job id.
#
#  INPUTS
#     id          - job identification number
#     force       - do a qmod -f
#     error_check - raise an error in case qmod fails
#
#  RESULT
#     0  - ok
#     -1 - error
#
#  SEE ALSO
#     sge_procedures/unsuspend_job()
#*******************************
proc suspend_job { id {force 0} {error_check 1}} {
   global ts_config
   global CHECK_OUTPUT CHECK_ARCH CHECK_USER CHECK_HOST

   set SUSPEND1 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_SUSPENDTASK_SUU] $CHECK_USER $id "*" ]
   set SUSPEND2 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_SUSPENDJOB_SU] $CHECK_USER $id ]
   set ALREADY_SUSP1 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_ALREADYSUSPENDED_SUU] $CHECK_USER $id "*"]
   set ALREADY_SUSP2 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_ALREADYSUSPENDED_SU] $CHECK_USER $id]
   set FORCED_SUSP1 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_FORCESUSPENDTASK_SUU] $CHECK_USER $id "*"]
   set FORCED_SUSP2 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_FORCESUSPENDJOB_SU] $CHECK_USER $id]
   log_user 0 
	set program "$ts_config(product_root)/bin/$CHECK_ARCH/qmod"
   if {$force} {
      set sid [ open_remote_spawn_process $CHECK_HOST $CHECK_USER $program "-f -s $id"  ]     
   } else {
      set sid [ open_remote_spawn_process $CHECK_HOST $CHECK_USER $program "-s $id"  ]     
   }

   set sp_id [ lindex $sid 1 ]
	set timeout 30
   set result -1	
   log_user 0 

	expect {
      -i $sp_id full_buffer {
         set result -1 
         add_proc_error "suspend_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
      }
	   -i $sp_id -- "$SUSPEND1" {
         puts $CHECK_OUTPUT "$expect_out(0,string)"
	      set result 0
	   }
	   -i $sp_id -- "$SUSPEND2" {
         puts $CHECK_OUTPUT "$expect_out(0,string)"
	      set result 0
	   }
	   -i $sp_id -- "$ALREADY_SUSP1" {
         puts $CHECK_OUTPUT "$expect_out(0,string)"
	      set result -1
	   }
	   -i $sp_id -- "$ALREADY_SUSP2" {
         puts $CHECK_OUTPUT "$expect_out(0,string)"
	      set result -1
	   }
	   -i $sp_id -- "$FORCED_SUSP1" {
         puts $CHECK_OUTPUT "$expect_out(0,string)"
	      set result 0
	   }
	   -i $sp_id -- "$FORCED_SUSP2" {
         puts $CHECK_OUTPUT "$expect_out(0,string)"
	      set result 0
	   }
	   -i $sp_id "suspended job" {
	      set result 0
	   }
      -i $sp_id default {
         puts $CHECK_OUTPUT "unexpected output: $expect_out(0,string)"
	      set result -1
	   }
	}
	# close spawned process 
	close_spawn_process $sid
   log_user 1

   # error check
   if { $error_check && $result != 0 } {
      add_proc_error "suspend_job" -1 "could not suspend job $id"
   }

   return $result
}

#                                                             max. column:     |
#****** sge_procedures/unsuspend_job() ******
# 
#  NAME
#     unsuspend_job -- set job bakr from unsuspended state
 
#
#  SYNOPSIS
#     unsuspend_job { job } 
#
#  FUNCTION
#     This procedure will call qmod to unsuspend the given job id.
#
#  INPUTS
#     job - job identification number

#
#  RESULT
#     0   - ok
#     -1  - error
#
#  SEE ALSO
#     sge_procedures/suspend_job()
#*******************************
proc unsuspend_job { job } {
  global ts_config
  global CHECK_ARCH CHECK_HOST CHECK_USER


  set UNSUSPEND1 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_UNSUSPENDTASK_SUU] "*" "*" "*" ]
  set UNSUSPEND2 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_UNSUSPENDJOB_SU] "*" "*" ]

  log_user 0 
  # spawn process
  set program "$ts_config(product_root)/bin/$CHECK_ARCH/qmod"
  set sid [ open_remote_spawn_process $CHECK_HOST $CHECK_USER $program "-us $job" ]
  set sp_id [ lindex $sid 1 ]
  set timeout 30
  set result -1	
  log_user 0 

  expect {
       -i $sp_id full_buffer {
          set result -1 
          add_proc_error "unsuspend_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
       }
       -i $sp_id $UNSUSPEND1 {
          set result 0 
       }
       -i $sp_id $UNSUSPEND2 {
          set result 0 
       }
       -i $sp_id "unsuspended job" {
          set result 0 
       }
       -i $sp_id default {
          set result -1 
       }
       
  }

  # close spawned process 
  close_spawn_process $sid
  log_user 1   
  if { $result != 0 } {
     add_proc_error "unsuspend_job" -1 "could not unsuspend job $job"
  }
  return $result
}


#****** sge_procedures/is_job_id() *********************************************
#  NAME
#     is_job_id() -- check if job_id is a real sge job id
#
#  SYNOPSIS
#     is_job_id { job_id } 
#
#  FUNCTION
#     This procedure returns 1 if the given job id is a valid sge job id
#
#  INPUTS
#     job_id - job id
#
#  RESULT
#     1 on success, 0 on error
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc is_job_id { job_id } {
  global ts_config

   if { [string is integer $job_id] != 1} {
      if { [set pos [string first "." $job_id ]] >= 0 } {
         incr pos -1
         set array_id [  string range $job_id 0 $pos] 
         incr pos 2
         set task_rest [ string range $job_id $pos end]
         if { [string is integer $array_id] != 1} {
            add_proc_error "is_job_id" -1 "unexpected task job id: $array_id (no integer)"
            return 0
         } 
      } else {
         add_proc_error "is_job_id" -1 "unexpected job id: $job_id (no integer)"
         return 0
      }
   }
   if { $job_id <= 0 } {
      add_proc_error "is_job_id" -1 "unexpected job id: $job_id (no positive number)"
      return 0
   }
   return 1
}

#                                                             max. column:     |
#****** sge_procedures/delete_job() ******
# 
#  NAME
#     delete_job -- delete job with jobid
#
#  SYNOPSIS
#     delete_job { jobid { wait_for_end 0 }} 
#
#  FUNCTION
#     This procedure will delete the job with the given jobid
#
#  INPUTS
#     jobid              - job identification number
#     { wait_for_end 0 } - optional, if not 0: wait for end of job 
#                          (till qstat -f $jobid returns "job not found")
#
#  RESULT
#     0   - ok
#    -1   - qdel error
#    -4   - unknown message from qdel
#    -5   - timeout or buffer overflow error
#
#  SEE ALSO
#     sge_procedures/submit_job()
#*******************************
proc delete_job { jobid {wait_for_end 0} {all_users 0}} {
   global ts_config
   global CHECK_ARCH CHECK_OUTPUT CHECK_HOST
   global CHECK_USER


   set REGISTERED1 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_REGDELTASK_SUU] "*" "*" "*"]
   set REGISTERED2 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_REGDELJOB_SU] "*" "*" ]
   set DELETED1  [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_DELETETASK_SUU] "*" "*" "*"]
   set DELETED2  [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_DELETEJOB_SU] "*" "*" ]

   if {$ts_config(gridengine_version) == 53} {
      set UNABLETOSYNC "asldfja?sldkfj?ajf?ajf"
   } else {
      set UNABLETOSYNC [translate $CHECK_HOST 1 0 0 [sge_macro MSG_COM_NOSYNCEXECD_SU] $CHECK_USER $jobid]
   }

   set result -100
   if { [ is_job_id $jobid] } {
      # spawn process
      set program "$ts_config(product_root)/bin/$CHECK_ARCH/qdel"

      # beginning with SGE 6.0 we need to specify if we want to delete jobs from
      # other users (as admin user)
      set args ""
      if { $ts_config(gridengine_version) != 53 } {
         if { $all_users } {
            set args "-u '*'"
         }
      }
      set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER "$program" "$args $jobid" ]
      set sp_id [ lindex $id 1 ]
      set timeout 60 	
      log_user 1
   
      while { $result == -100 } {
      expect {
          -i $sp_id timeout {
             add_proc_error "delete_job" "-1" "timeout waiting for qdel"
             set result -5
          }

          -i $sp_id full_buffer {
             set result -5
             add_proc_error "delete_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
          }
          -i $sp_id $REGISTERED1 {
             puts $CHECK_OUTPUT $expect_out(0,string)
             set result 0
          }
          -i $sp_id $REGISTERED2 {
             puts $CHECK_OUTPUT $expect_out(0,string)
             set result 0
          }
          -i $sp_id "registered the job" {
             puts $CHECK_OUTPUT $expect_out(0,string)
             set result 0
          }
          -i $sp_id  $DELETED1 {
             puts $CHECK_OUTPUT $expect_out(0,string)
             set result 0
          }
          -i $sp_id  $DELETED2 {
             puts $CHECK_OUTPUT $expect_out(0,string)
             set result 0
          }
          -i $sp_id "has deleted job" {
             puts $CHECK_OUTPUT $expect_out(0,string)
             set result 0
          }
          -i $sp_id $UNABLETOSYNC {
            add_proc_error "delete_job" -1 "$UNABLETOSYNC"
            set result -1
          }
          -i default {
             if { [info exists expect_out(buffer)] } {
#                puts $CHECK_OUTPUT $expect_out(buffer)
                add_proc_error "delete_job" -1 "expect default switch\noutput was:\n>$expect_out(buffer)<"
             }
             set result -4 
          }
      }
      }
      # close spawned process 
      close_spawn_process $id 1
      log_user 1
   } else {
      add_proc_error "delete_job" -1 "job id is no integer"
   }
   if { $result != 0 } {
      add_proc_error "delete_job" -1 "could not delete job $jobid\nresult=$result"
   }
   if { $wait_for_end != 0 && $result == 0 } {
      set my_timeout [timestamp]
      set my_second_qdel_timeout $my_timeout
      incr my_second_qdel_timeout 80
      incr my_timeout 160
      while { [get_qstat_j_info $jobid ] != 0 } {
          puts $CHECK_OUTPUT "waiting for jobend ..."
          if { [timestamp] > $my_timeout } {
             add_proc_error "delete_job" -1 "timeout while waiting for jobend"
             break;
          }
          if { [timestamp] > $my_second_qdel_timeout } {
             set my_second_qdel_timeout $my_timeout
             delete_job $jobid
          }
          after 1000
      }
   }
   return $result
}


#                                                             max. column:     |
#****** sge_procedures/submit_job() ******
# 
#  NAME
#     submit_job -- submit a job with qsub
#
#  SYNOPSIS
#     submit_job { args {do_error_check 1} {submit_timeout 60} {host ""} 
#                  {user ""} { cd_dir ""} { show_args 1 }  }
#
#  FUNCTION
#     This procedure will submit a job.
#
#  INPUTS
#     args                - a string of qsub arguments/parameters
#     {do_error_check 1}  - if 1 (default): add global erros (add_proc_error)
#                           if not 1: do not add errors
#     {submit_timeout 30} - timeout (default is 30 sec.)
#     {host ""}           - host on which to execute qsub (default $CHECK_HOST)
#     {user ""}           - user who shall submit job (default $CHECK_USER)
#     {cd_dir ""}         - optional: do cd to given directory first
#     { show_args 1 }     - optional: show job arguments
#
#  RESULT
#     This procedure returns:
#     
#     jobid   of array or job if submit was successfull (value > 1)
#        -1   on timeout error
#        -2   if usage was printed on -help or commandfile argument
#        -3   if usage was printed NOT on -help or commandfile argument
#        -4   if verify output was printed on -verify argument
#        -5   if verify output was NOT printed on -verfiy argument
#        -6   job could not be scheduled, try later
#        -7   has to much tasks - error
#        -8   unknown resource - error
#        -9   can't resolve hostname - error
#       -10   resource not requestable - error
#       -11   not allowed to submit jobs - error
#       -12   no access to project - error
#       -13   unkown option - error
#       -22   user tries to submit a job with a deadline, but the user is not in
#             the deadline user access list
#
#      -100   on error 
#     
#
#  EXAMPLE
#     set jobs ""
#     set my_outputs "-o /dev/null -e /dev/null"
#     set arguments "$my_outputs -q $rerun_queue -r y $ts_config(product_root)/examples/jobs/sleeper.sh 1000"
#     lappend jobs [submit_job $arguments]
#
#  SEE ALSO
#     sge_procedures/delete_job()
#     check/add_proc_error()
#*******************************
proc submit_job { args {do_error_check 1} {submit_timeout 60} {host ""} {user ""} { cd_dir ""} { show_args 1 } } {
  global ts_config
  global CHECK_HOST CHECK_ARCH CHECK_OUTPUT CHECK_USER
  global CHECK_DEBUG_LEVEL

  set return_value " "

  if {$host == ""} {
    set host $ts_config(master_host)
  }

  if {$user == ""} {
    set user $CHECK_USER
  }

  set arch [resolve_arch $host]

  if { $ts_config(gridengine_version) == 53 } {
     set JOB_SUBMITTED       [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_SUBMITJOB_USS] "*" "*" "*"]
     set JOB_SUBMITTED_DUMMY [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_SUBMITJOB_USS] "__JOB_ID__" "__JOB_NAME__" "__JOB_ARG__"]
     set SUCCESSFULLY        [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QSUB_YOURIMMEDIATEJOBXHASBEENSUCCESSFULLYSCHEDULED_U] "*"]
     set UNKNOWN_RESOURCE2   [translate $CHECK_HOST 1 0 0 [sge_macro MSG_SCHEDD_JOBREQUESTSUNKOWNRESOURCE] ]
     set JOBALREADYEXISTS [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_JOBALREADYEXISTS_U] "*"]
     set NAMETOOLONG         "blah blah blah no NAMETOOLONG in 5.3"
     set INVALID_JOB_REQUEST "blah blah blah no INVALID_JOB_REQUEST in 5.3"
     set UNKNOWN_QUEUE [translate_macro MSG_JOB_QUNKNOWN_S "*"]
     set POSITIVE_PRIO "blah blah blah no POSITIVE_PRIO in 5.3"
  } else {
     # 6.0 and higher
     set JOB_SUBMITTED       [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QSUB_YOURJOBHASBEENSUBMITTED_SS] "*" "*"]
     set JOB_SUBMITTED_DUMMY [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QSUB_YOURJOBHASBEENSUBMITTED_SS] "__JOB_ID__" "__JOB_NAME__"]
     set SUCCESSFULLY        [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QSUB_YOURIMMEDIATEJOBXHASBEENSUCCESSFULLYSCHEDULED_S] "*"]
     set UNKNOWN_RESOURCE2 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_SCHEDD_JOBREQUESTSUNKOWNRESOURCE_S] ]
     set NAMETOOLONG         [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_NAMETOOLONG_I] "*"]
     set JOBALREADYEXISTS [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_JOBALREADYEXISTS_S] "*"]
     set INVALID_JOB_REQUEST [translate $CHECK_HOST 1 0 0 [sge_macro MSG_INVALIDJOB_REQUEST_S] "*"]
     set UNKNOWN_QUEUE [translate_macro MSG_QREF_QUNKNOWN_S "*"]
     if {$ts_config(gridengine_version) == 60} {
        set POSITIVE_PRIO "blah blah blah no POSITIVE_PRIO in 6.0"
     } else {
        # 6.5 and higher
        set POSITIVE_PRIO [translate_macro MSG_JOB_NONADMINPRIO]
     }
  }

  set INVALID_PRIORITY [translate_macro MSG_PARSE_INVALIDPRIORITYMUSTBEINNEG1023TO1024]
  set WRONG_TYPE          [translate $CHECK_HOST 1 0 0 [sge_macro MSG_CPLX_WRONGTYPE_SSS] "*" "*" "*"]
  set ERROR_OPENING       [translate $CHECK_HOST 1 0 0 [sge_macro MSG_FILE_ERROROPENINGXY_SS] "*" "*"]
  set NOT_ALLOWED_WARNING [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_NOTINANYQ_S] "*" ]
  set NO_DEADLINE_USER    [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_NODEADLINEUSER_S] $user ]


  set JOB_ARRAY_SUBMITTED       [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_SUBMITJOBARRAY_UUUUSS] "*" "*" "*" "*" "*" "*" ]
  set JOB_ARRAY_SUBMITTED_DUMMY [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_SUBMITJOBARRAY_UUUUSS] "__JOB_ID__" "" "" "" "__JOB_NAME__" "__JOB_ARG__"]


  set TRY_LATER           [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QSUB_YOURQSUBREQUESTCOULDNOTBESCHEDULEDDTRYLATER]]
  set USAGE               [translate $CHECK_HOST 1 0 0 [sge_macro MSG_GDI_USAGE_USAGESTRING] ]

   if { $ts_config(gridengine_version) == 53 } {
      set UNAMBIGUOUSNESS [translate $CHECK_HOST 1 0 0   [sge_macro MSG_JOB_MOD_JOBNAMEVIOLATESJOBNET_SSUU] "*" "*" "*" "*" ]
      set NON_AMBIGUOUS   [translate $CHECK_HOST 1 0 0   [sge_macro MSG_JOB_MOD_JOBNETPREDECESSAMBIGUOUS_SUU] "*" "*" "*" ]
   } else {
      # JG: TODO: jobnet handling completely changed - what are the messages?
      set UNAMBIGUOUSNESS "a?sjfas?kjfa?jf"
      set NON_AMBIGUOUS   "aajapsoidfupoijpoaisdfup9"
   }
  set UNKNOWN_OPTION  [translate $CHECK_HOST 1 0 0   [sge_macro MSG_ANSWER_UNKOWNOPTIONX_S] "*" ]
  set NO_ACC_TO_PRJ1  [translate $CHECK_HOST 1 0 0   [sge_macro MSG_SGETEXT_NO_ACCESS2PRJ4USER_SS] "*" "*"]
  set NO_ACC_TO_PRJ2  [translate $CHECK_HOST 1 0 0   [sge_macro MSG_STREE_USERTNOACCESS2PRJ_SS] "*" "*"]
  set NOT_ALLOWED1    [translate $CHECK_HOST 1 0 0   [sge_macro MSG_JOB_NOPERMS_SS] "*" "*"]
  set NOT_ALLOWED2    [translate $CHECK_HOST 1 0 0   [sge_macro MSG_JOB_PRJNOSUBMITPERMS_S] "*" ]
  set NOT_REQUESTABLE [translate $CHECK_HOST 1 0 0   [sge_macro MSG_SGETEXT_RESOURCE_NOT_REQUESTABLE_S] "*" ]
  set CAN_T_RESOLVE   [translate $CHECK_HOST 1 0 0   [sge_macro MSG_SGETEXT_CANTRESOLVEHOST_S] "*" ]
  set UNKNOWN_RESOURCE1 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_SGETEXT_UNKNOWN_RESOURCE_S] "*" ]
  set TO_MUCH_TASKS [translate $CHECK_HOST 1 0 0     [sge_macro MSG_JOB_MORETASKSTHAN_U] "*" ]
  set WARNING_OPTION_ALREADY_SET [translate $CHECK_HOST 1 0 0 [sge_macro MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S] "*"]
  set ONLY_ONE_RANGE [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QCONF_ONLYONERANGE]]
  set PARSE_DUPLICATEHOSTINFILESPEC [translate $CHECK_HOST 1 0 0 [sge_macro MSG_PARSE_DUPLICATEHOSTINFILESPEC]] 
  set GDI_NEGATIVSTEP [translate $CHECK_HOST 1 0 0 [sge_macro MSG_GDI_NEGATIVSTEP]] 
  set GDI_INITIALPORTIONSTRINGNODECIMAL_S [translate $CHECK_HOST 0 0 0 [sge_macro MSG_GDI_INITIALPORTIONSTRINGNODECIMAL_S] "*" ] 


  set help_translation  [translate $CHECK_HOST 1 0 0 [sge_macro MSG_GDI_KEYSTR_COLON]]
  set COLON_NOT_ALLOWED [translate $CHECK_HOST 1 0 0 [sge_macro MSG_GDI_KEYSTR_MIDCHAR_SC] "$help_translation" ":"]

  append USAGE " qsub"

  if { $show_args == 1 } {
     puts $CHECK_OUTPUT "job submit args:\n$args"
  }
  # spawn process
  set program "$ts_config(product_root)/bin/$arch/qsub"
  if { $cd_dir != "" } {
     set id [ open_remote_spawn_process "$host" "$user" "cd" "$cd_dir;$program $args" ]
     puts $CHECK_OUTPUT "cd to $cd_dir"
  } else {
     set id [ open_remote_spawn_process "$host" "$user" "$program" "$args" ]
  }
  set sp_id [ lindex $id 1 ]

  set timeout $submit_timeout
  set do_again 1


  log_user 0  ;# debug log_user 0
  if { $CHECK_DEBUG_LEVEL != 0 } {
     log_user 1
  }

  while { $do_again == 1 } {
     set timeout $submit_timeout

     set do_again 0
     expect {
          -i $sp_id full_buffer {
             set return_value -1    
             add_proc_error "submit_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
          }
          -i $sp_id timeout {
             puts $CHECK_OUTPUT "submit_job - timeout(1)"
             add_proc_error "submit_job" "-1" "got timeout(1) error"
             set return_value -1 
          }
          -i $sp_id eof {
             puts $CHECK_OUTPUT "submit_job - end of file unexpected"
             set return_value -1
          }
          -i $sp_id -- $NOT_ALLOWED_WARNING {
             puts $CHECK_OUTPUT "got warning: job can't run in any queue" 
             set outtext $expect_out(0,string) 
             puts $CHECK_OUTPUT "string is: \"$outtext\""
             set do_again 1
          }
          -i $sp_id -- $WARNING_OPTION_ALREADY_SET {
             puts $CHECK_OUTPUT "got warning: option has already been set" 
             set outtext $expect_out(0,string) 
             puts $CHECK_OUTPUT "string is: \"$outtext\""
             set do_again 1
          }
          -i $sp_id -- $JOB_SUBMITTED {
             set job_id_pos [ string first "__JOB_ID__" $JOB_SUBMITTED_DUMMY ]
             set job_name_pos [ string first "__JOB_NAME__" $JOB_SUBMITTED_DUMMY ]
             if { $job_id_pos > $job_name_pos } {
                add_proc_error "submit_job" "-1" "locale switches parameter for qsub string! This is not supported yet"
             }
             incr job_id_pos -1
             set job_id_prefix [ string range $JOB_SUBMITTED_DUMMY 0 $job_id_pos ]
             set job_id_prefix_length [ string length $job_id_prefix]
   
             set outtext $expect_out(0,string) 
#             puts $CHECK_OUTPUT "string is: \"$outtext\""
   #          puts $CHECK_OUTPUT "dummy  is: \"$JOB_SUBMITTED_DUMMY\""
             set id_pos [ string first $job_id_prefix $outtext]
             incr id_pos $job_id_prefix_length
             set submitjob_jobid [string range $outtext $id_pos end]
             set space_pos [ string first " " $submitjob_jobid ]
             set submitjob_jobid [string range $submitjob_jobid 0 $space_pos ]
             set submitjob_jobid [string trim $submitjob_jobid]
             if {[string first "." $submitjob_jobid] >= 0} {
                puts $CHECK_OUTPUT "This is a job array"
                set new_jobid [lindex [split $submitjob_jobid "."] 0]
                puts $CHECK_OUTPUT "Array has ID $new_jobid"
                set submitjob_jobid $new_jobid 
             }
   
   # try to figure out more
             set timeout 30
             expect {
                -i $sp_id full_buffer {
                   set return_value -1
                   add_proc_error "submit_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
                }
                -i $sp_id timeout {
                   puts $CHECK_OUTPUT "submit_job - timeout(2)"
                   set return_value -1 
                }
                -i $sp_id "_exit_status_" {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id eof {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id $SUCCESSFULLY  {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id $TRY_LATER {
                   set return_value -6
                }
                
             }       
   
          }
          -i $sp_id -- $JOB_ARRAY_SUBMITTED {
             set job_id_pos [ string first "__JOB_ID__" $JOB_ARRAY_SUBMITTED_DUMMY ]
             set job_name_pos [ string first "__JOB_NAME__" $JOB_ARRAY_SUBMITTED_DUMMY ]
             set job_arg_pos [ string first "__JOB_ARG__" $JOB_ARRAY_SUBMITTED_DUMMY ]
             if { $job_id_pos > $job_name_pos || $job_id_pos > $job_arg_pos } {
                add_proc_error "submit_job" "-1" "locale switches parameter for qsub string! This is not supported yet"
             }
             incr job_id_pos -1
             set job_id_prefix [ string range $JOB_ARRAY_SUBMITTED_DUMMY 0 $job_id_pos ]
             set job_id_prefix_length [ string length $job_id_prefix]
   
             set outtext $expect_out(0,string) 
   #          puts $CHECK_OUTPUT "string is: \"$outtext\""
   #          puts $CHECK_OUTPUT "dummy  is: \"$JOB_ARRAY_SUBMITTED_DUMMY\""
             set id_pos [ string first $job_id_prefix $outtext]
             incr id_pos $job_id_prefix_length
             set submitjob_jobid [string range $outtext $id_pos end]
             set space_pos [ string first " " $submitjob_jobid ]
             set submitjob_jobid [string range $submitjob_jobid 0 $space_pos ]
             set submitjob_jobid [string trim $submitjob_jobid]
             if {[string first "." $submitjob_jobid] >= 0} {
                puts $CHECK_OUTPUT "This is a job array"
                set new_jobid [lindex [split $submitjob_jobid "."] 0]
                puts $CHECK_OUTPUT "Array has ID $new_jobid"
                set submitjob_jobid $new_jobid 
             }
   
   # try to figure out more
             set timeout 30
             expect {
                -i $sp_id full_buffer {
                   set return_value -1
                   add_proc_error "submit_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
                }
                -i $sp_id timeout {
                   puts $CHECK_OUTPUT "submit_job - timeout(3)"
                   set return_value -1 
                }
                -i $sp_id "_exit_status_" {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id eof {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id $SUCCESSFULLY  {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id $TRY_LATER {
                   set return_value -6
                }
                
             }       
   
          }
          
          
          -i $sp_id -- "job*has been submitted" {
             
             set outtext $expect_out(0,string) 
             set submitjob_jobid [lindex $outtext 1];
             if {[string first "." $submitjob_jobid] >= 0} {
                puts $CHECK_OUTPUT "This is a job array"
                set new_jobid [lindex [split $submitjob_jobid "."] 0]
                puts $CHECK_OUTPUT "Array has ID $new_jobid"
                set submitjob_jobid $new_jobid 
             }
   
   # try to figure out more
             set timeout 30
             expect {
                -i $sp_id full_buffer {
                   set return_value -1
                   add_proc_error "submit_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
                }
                -i $sp_id timeout {
                   puts $CHECK_OUTPUT "submit_job - timeout(4)"
                   set return_value -1 
                }
                -i $sp_id "_exit_status_" {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id eof {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id "successfully scheduled" {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id "try again later" {
                   set return_value -6
                }
                
             }       
   
          }
   
          -i $sp_id "usage: qsub" {
             puts $CHECK_OUTPUT "got usage ..."
             if {([string first "help" $args] >= 0 ) || ([string first "commandfile" $args] >= 0)} {
                set return_value -2 
             } else { 
                set return_value -3
             }
          } 
          -i $sp_id $USAGE {
             puts $CHECK_OUTPUT "got usage ..."
             if {([string first "help" $args] >= 0 ) || ([string first "commandfile" $args] >= 0)} {
                set return_value -2 
             } else { 
                set return_value -3
             }
          } 
   
          -i $sp_id "job_number" {
            if {[string first "verify" $args] >= 0 } {
               set return_value -4
            } else {
               set return_value -5
            } 
          }
          -i $sp_id $TO_MUCH_TASKS {
             set return_value -7
          }
          -i $sp_id "submit a job with more than" {
             set return_value -7
          }
          -i $sp_id $UNKNOWN_RESOURCE1 {
             set return_value -8
          }
          -i $sp_id $UNKNOWN_RESOURCE2 {
             set return_value -8
          }
          -i $sp_id "unknown resource" {
             set return_value -8
          }
          -i $sp_id $CAN_T_RESOLVE {
             set return_value -9
          }
          -i $sp_id "can't resolve hostname" {
             set return_value -9
          }
          -i $sp_id $NOT_REQUESTABLE {
             set return_value -10
          }
          -i $sp_id "configured as non requestable" {
             set return_value -10
          }
          
          -i $sp_id $NOT_ALLOWED1 {
             set return_value -11
          }       
          -i $sp_id $NOT_ALLOWED2 {
             set return_value -11
          }       
          -i $sp_id "not allowed to submit jobs" {
             set return_value -11
          }
          -i $sp_id $NO_ACC_TO_PRJ1 {
             puts $CHECK_OUTPUT "got string(2): \"$expect_out(0,string)\""
             set return_value -12
          }
          -i $sp_id $NO_ACC_TO_PRJ2 {
         
             set return_value -12
          }
          -i $sp_id "no access to project" {
             set return_value -12
          }
          -i $sp_id $UNKNOWN_OPTION {
             set return_value -13
          }
          -i $sp_id "Unknown option" {
             set return_value -13
          }
          -i $sp_id $NON_AMBIGUOUS {
             set return_value -14
          }
          -i $sp_id "non-ambiguous jobnet predecessor" {
             set return_value -14
          }
          -i $sp_id $UNAMBIGUOUSNESS {
             set return_value -15
          }
          -i $sp_id "using job name \"*\" for*violates reference unambiguousness" {
             set return_value -15
          }
          -i $sp_id $ERROR_OPENING {
             set return_value -16
          }
          -i $sp_id $COLON_NOT_ALLOWED {
             set return_value -17
          }
          -i $sp_id $ONLY_ONE_RANGE {
             set return_value -18
          }
          -i $sp_id "$PARSE_DUPLICATEHOSTINFILESPEC" { 
             set return_value -19
          }
          -i $sp_id "two files are specified for the same host" { 
             set return_value -19
          }
         
          -i $sp_id $GDI_NEGATIVSTEP { 
             set return_value -20
          }

          -i $sp_id $GDI_INITIALPORTIONSTRINGNODECIMAL_S { 
             set return_value -21
          }
          -i $sp_id $NO_DEADLINE_USER {
             set return_value -22
          }
          -i $sp_id $WRONG_TYPE {
             set return_value -23
          }
          -i $sp_id $NAMETOOLONG {
             set return_value -24
          }
          -i $sp_id $JOBALREADYEXISTS {
             set return_value -25
          }
          -i $sp_id $INVALID_JOB_REQUEST {
             set return_value -26
          }
          -i $sp_id $INVALID_PRIORITY {
             set return_value -27
          }
          -i $sp_id $UNKNOWN_QUEUE {
             set return_value -28
          }
          -i $sp_id -- $POSITIVE_PRIO {
             set return_value -29
          }
        }
     }
 
     # close spawned process 

     if { $do_error_check == 1 } {
        close_spawn_process $id
     } else {
        close_spawn_process $id 1
     }
   
     if {$do_error_check == 1} { 
       switch -- $return_value {
          "-1"  { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-2"  { add_proc_error "submit_job" 0  [get_submit_error $return_value]  }
          "-3"  { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-4"  { add_proc_error "submit_job" 0  [get_submit_error $return_value]  }
          "-5"  { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-6"  { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-7"  { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-8"  { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-9"  { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-10" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-11" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-12" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-13" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-14" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-15" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-16" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-17" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-18" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-19" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-20" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-21" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-22" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-23" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-24" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-25" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-26" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-27" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-28" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-29" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }

          default { add_proc_error "submit_job" 0 "job $return_value submitted - ok" }
       }
     }
     if { $return_value <= 0 && $do_error_check != 1 } {
        puts $CHECK_OUTPUT "submit_job returned error: [get_submit_error $return_value]"
     }
     puts $CHECK_OUTPUT "submit job: returning job id: $return_value"
     return $return_value
}



#****** sge_procedures/get_submit_error() **************************************
#  NAME
#     get_submit_error() -- resolve negative error value from submit_job()
#
#  SYNOPSIS
#     get_submit_error { error_id } 
#
#  FUNCTION
#     This procedure is used to get an error text from an negative return
#     value of the submit_job() procedure.
#
#  INPUTS
#     error_id - negative return value from submit_job() call
#
#  RESULT
#     Error text
#
#  SEE ALSO
#     sge_procedures/submit_job()
#*******************************************************************************
proc get_submit_error { error_id } {
  global ts_config
   switch -- $error_id {
      "-1"  { return "timeout error" }
      "-2"  { return "usage was printed on -help or commandfile argument - ok" }
      "-3"  { return "usage was printed NOT on -help or commandfile argument - error" }
      "-4"  { return "verify output was printed on -verify argument - ok" }
      "-5"  { return "verify output was NOT printed on -verfiy argument - error" }
      "-6"  { return "job could not be scheduled, try later - error" }
      "-7"  { return "has to much tasks - error" }
      "-8"  { return "unknown resource - error" }
      "-9"  { return "can't resolve hostname - error" }
      "-10" { return "resource not requestable - error" }
      "-11" { return "not allowed to submit jobs - error" }
      "-12" { return "no acces to project - error" }
      "-13" { return "Unkown option - error" }
      "-14" { return "non-ambiguous jobnet predecessor - error" }
      "-15" { return "job violates reference unambiguousness - error" }
      "-16" { return "error opening file - error" }
      "-17" { return "colon not allowed in account string - error" }
      "-18" { return "-t option only allows one range specification" } 
      "-19" { return "two files are specified for the same host" }
      "-20" { return "negative step in range is not allowed" }
      "-21" { return "-t step of range must be a decimal number" }
      "-22" { return "user is not in access list deadlineusers" }
      "-23" { return "wrong type for submit -l option" }
      "-24" { return "the job name (-N option) is too long" }
      "-25" { return "duplicate job id found - issue 2028?" }
      "-26" { return "general job verification error?" }
      "-27" { return "invalid priority given with -p switch" }
      "-28" { return "unknown queue requested" }
      "-29" { return "positive priority requested as non operator" }

      default { return "unknown error" }
   }
}

#                                                             max. column:     |
#****** sge_procedures/get_grppid_of_job() ******
# 
#  NAME
#     get_grppid_of_job -- get grppid of job
#
#  SYNOPSIS
#     get_grppid_of_job { jobid } 
#
#  FUNCTION
#     This procedure opens the job_pid file in the execution host spool directory
#     and returns the content of this file (grppid).
#     
#
#  INPUTS
#     jobid - identification number of job
#
#  RESULT
#     grppid of job 
#
#  SEE ALSO
#     sge_procedures/get_suspend_state_of_job()
#*******************************
proc get_grppid_of_job { jobid {host ""}} {
   global ts_config
   global CHECK_OUTPUT CHECK_HOST CHECK_USER

   # default for host parameter, use localhost
   if {$host == ""} {
      set host $CHECK_HOST
   }

   # read execd spooldir from local or global config
   get_config value $host
   if {[info exists value(execd_spool_dir)]} {
      set spool_dir $value(execd_spool_dir)
      puts $CHECK_OUTPUT "using local exec spool dir"  
   } else {
      puts $CHECK_OUTPUT "using global exec spool dir"  
      get_config value
      set spool_dir $value(execd_spool_dir)
   }

   puts $CHECK_OUTPUT "Exec Spool Dir is: $spool_dir"

   # build name of pid file
   set pidfile "$spool_dir/$host/active_jobs/$jobid.1/job_pid"

   wait_for_remote_file $host $CHECK_USER $pidfile

   # read pid from pidfile on execution host
   set real_pid [start_remote_prog $host $CHECK_USER "cat" "$pidfile"]
   if {$prg_exit_state != 0} {
      add_proc_error "get_grppid_of_job" -1 "can't read $pidfile on host $host: $real_pid"
      set real_pid ""
   }

   # trim trailing newlines etc.
   set real_pid [string trim $real_pid]
   return $real_pid
}


#
#****** sge_procedures/get_suspend_state_of_job() ******************************
#
#  NAME
#     get_suspend_state_of_job() -- get suspend state of job from ps command
#
#  SYNOPSIS
#     get_suspend_state_of_job { jobid { pidlist pid_list } {do_error_check 1} 
#     { pgi process_group_info } } 
#
#  FUNCTION
#     This procedure returns the suspend state of jobid (letter from ps command). 
#     Beyond that a array (pidlist) is set, in which all process id of the process 
#     group are listed. The caller of the function can access the array pid_list!
#
#  INPUTS
#     jobid                      - job identification number
#     { host "" }                - host the job is running on, default $CHECK_HOST
#     { pidlist pid_list }       - name of variable to store the pidlist
#     {do_error_check 1}         - enable error messages (add_proc_error), default
#                                  if not 1 the procedure will not report errors
#     { pgi process_group_info } - string with ps output of process group of job
#
#  RESULT
#     suspend state (letter from ps command)
#
#  SEE ALSO
#     sge_procedures/get_grppid_of_job()
#     sge_procedures/add_proc_error()
#*******************************************************************************
proc get_suspend_state_of_job { jobid {host ""} { pidlist pid_list } {do_error_check 1} { pgi process_group_info } } {
  global ts_config
   global CHECK_OUTPUT CHECK_HOST CHECK_ARCH

   upvar $pidlist pidl 
   upvar $pgi pginfo

   if {$host == ""} {
      set host $CHECK_HOST
   }

   # give the system time to change the processes before ps call!!
   # JG: TODO: this should not be done here, but in the caller's context!
   after 5000

   # get process group id
   set real_pid [get_grppid_of_job $jobid $host]
   puts $CHECK_OUTPUT "grpid is \"$real_pid\" on host \"$host\""


   set time_now [timestamp]
   set time_out [ expr ($time_now + 60 )  ]   ;# timeout is 60 seconds

   set have_errors 0
   while { [timestamp] < $time_out } {
      # get current process list (ps)
      get_ps_info $real_pid $host ps_list
      
   
      # copy pid_list from ps_list
      set pscount $ps_list(proc_count)
      set pidl ""
      for {set i 0} { $i < $pscount } {incr i 1} {
         if { $ps_list(pgid,$i) == $real_pid } { 
            lappend pidl $ps_list(pid,$i)
         }
      } 
      puts $CHECK_OUTPUT "Process group has [llength $pidl] processes ($pidl)"
    
      #  
      set state_count 0
      set state_letter ""
      set pginfo ""
      foreach elem $pidl {
         puts $CHECK_OUTPUT $ps_list($elem,string)
         append pginfo "$ps_list($elem,string)\n"
         if { $state_count == 0 } {
            set state_letter $ps_list($elem,state)
         }
         incr state_count 1
         if { ( [string compare $state_letter $ps_list($elem,state)] != 0 ) && ($do_error_check == 1 ) } {
            if { [string compare $state_letter "T"] == 0} { 
               set have_errors 1
               # we report an error if not all processes have the state "T
            }
         }
      }
      if { $have_errors == 0 } {
         break;
      }
   }

   if { $have_errors != 0 } {
      add_proc_error "get_suspend_state_of_job" -1 "not all processes in pgrp has the same state \"$state_letter\""
   }

   return $state_letter
}




#                                                             max. column:     |
#****** sge_procedures/get_job_info() ******
# 
#  NAME
#     get_job_info -- get qstat -ext jobinformation 
#
#  SYNOPSIS
#     get_job_info { jobid } 
#
#  FUNCTION
#     This procedure runs the qstat -ext command and returns the output
#
#  INPUTS
#     jobid - job id (if job id = -1 the complete joblist is returned)
#
#  RESULT
#     "" if job was not found or the call fails
#     output of qstat -ext
#
#  SEE ALSO
#     sge_procedures/get_job_info()
#     sge_procedures/get_standard_job_info()
#     sge_procedures/get_extended_job_info()
#*******************************
proc get_job_info { jobid } {
  global ts_config
# return:
# info of qstat -ext for jobid
# nothing if job was not found
# complete joblist if jobid is -1
   global CHECK_ARCH 

   if { [string compare $ts_config(product_type) "sge"] == 0 } {
      add_proc_error "get_job_info" -1 "this call is not accepted for sge system"
      return "" 
   }

   set catch_return [ catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qstat" "-ext" } result ]
   if { $catch_return != 0 } {
      add_proc_error "get_job_info" -1 "qstat error or binary not found"
      return ""
   }

  # split each line as listelement
   set back ""
   set help [split $result "\n"]
   foreach line $help { 
      if { [lindex $line 0] == $jobid } {
         set back $line
         return $back
      }
   }

   if { ($jobid == -1) && ( [ llength $help ] > 2 ) } {
      
      set help [lreplace $help 0 1]
      return $help
   }
   return $back
}

#                                                             max. column:     |
#****** sge_procedures/get_standard_job_info() ******
# 
#  NAME
#     get_standard_job_info -- get jobinfo with qstat
#
#  SYNOPSIS
#     get_standard_job_info { jobid { add_empty 0} { get_all 0 } } 
#
#  FUNCTION
#     This procedure will call the qstat command without arguments.
#
#  INPUTS
#     jobid           - job id 
#     { add_empty 0 } - if 1: add lines with does not contain a job id
#                       information (SLAVE jobs)
#     { get_all   0 } - if 1: get every output line (ignore job id)
#
#  RESULT
#     - info of qstat for jobid
#     - nothing if job was not found
#     
#     each list element has following sublists:
#     job-ID        (index 0)
#     prior         (index 1)
#     name          (index 2)
#     user          (index 3) 
#     state         (index 4) 
#     submit/start  (index 5)  
#     at            (index 6)
#     queue         (index 7)
#     master        (index 8)    
#     ja-task-ID    (index 9)     
#
#  EXAMPLE
#     set result [get_standard_job_info 5]
#     if { llength $results > 0 } {
#        puts "user [lindex $result 3] submitted job 5"
#     }
#
#
#  SEE ALSO
#     sge_procedures/get_job_info()
#     sge_procedures/get_standard_job_info()
#     sge_procedures/get_extended_job_info()
#*******************************
proc get_standard_job_info { jobid { add_empty 0} { get_all 0 } } {
  global ts_config
   global CHECK_ARCH CHECK_OUTPUT

   if {$ts_config(gridengine_version) == 53} {
      set catch_return [ catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qstat" } result ]
   } else {
      set catch_return [ catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qstat" "-g" "t" } result ]
   }
   if { $catch_return != 0 } {
      add_proc_error "get_standard_job_info" -1 "qstat error or binary not found"
      return ""
   }


  # split each line as listelement
   set back ""
   set help [split $result "\n"]
   foreach line $help { 
#      puts $CHECK_OUTPUT $line
      if { [lindex $line 0] == $jobid } {
         lappend back $line
         continue
      }
      if { $add_empty != 0 } {
         if { [llength $line] == 8 } {
            lappend back "-1 $line"
            puts $CHECK_OUTPUT "adding empty job lines" 
            continue
         }

         if { [llength $line] == 2 } {
            lappend back "-1 0 x x x x x $line"
            puts $CHECK_OUTPUT "adding empty job lines" 
            continue
         }
      }
      if { $get_all != 0 } {
         lappend back $line
      }
   }
   return $back
}

#                                                             max. column:     |
#****** sge_procedures/get_extended_job_info() ******
# 
#  NAME
#     get_extended_job_info -- get extended job information (qstat ..)
#
#  SYNOPSIS
#     get_extended_job_info { jobid {variable job_info} } 
#
#  FUNCTION
#     This procedure is calling the qstat (qstat -ext if sgeee) and returns
#     the output of the qstat in array form.
#
#  INPUTS
#     jobid               - job identifaction number
#     {variable job_info} - name of variable array to store the output
#     {do_replace_NA}     - 1 : if not set, don't replace NA settings
#
#  RESULT
#     0, if job was not found
#     1, if job was found
#     
#     fills array $variable with info found in qstat output with the following symbolic names:
#     id
#     prior
#     name
#     user
#     state
#     time (submit or starttime) [UNIX-timestamp]
#     queue
#     master
#     jatask
#    
#     additional entries in case of SGEEE system:
#     project
#     department
#     deadline [UNIX-timestamp]
#     cpu [s]
#     mem [GBs]
#     io [?]
#     tckts
#     ovrts
#     otckt
#     dtckt
#     ftckt
#     stckt
#     share
#
#  EXAMPLE
#  proc testproc ... { 
#     ...
#     if {[get_extended_job_info $job_id] } {
#        if { $job_info(cpu) < 10 } {
#           add_proc_error "testproc" -1 "online usage probably does not work on $host"
#        }
#     } else {
#        add_proc_error "testproc" -1 "get_extended_jobinfo failed for job $job_id on host $host"
#     }
#     ...
#     set_error 0 "ok"
#  }
#
#  SEE ALSO
#     sge_procedures/get_job_info()
#     sge_procedures/get_standard_job_info()
#     sge_procedures/get_extended_job_info()
#*******************************
proc get_extended_job_info {jobid {variable job_info} { do_replace_NA 1 } } {
  global ts_config
   global CHECK_ARCH
   upvar $variable jobinfo

   if {[info exists jobinfo]} {
      unset jobinfo
   }

   if {$ts_config(product_type) == "sgeee" } {
      set exit_code [catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qstat" -ext} result]
      set ext 1
   } else {
      set exit_code [catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qstat" } result]
      set ext 0
   }

   if { $exit_code == 0 } {
      parse_qstat result jobinfo $jobid $ext $do_replace_NA
      return 1
   }
  
   return 0
}

#****** sge_procedures/get_qstat_j_info() **************************************
#  NAME
#     get_qstat_j_info() -- get qstat -j information
#
#  SYNOPSIS
#     get_qstat_j_info { jobid {variable qstat_j_info} } 
#
#  FUNCTION
#     This procedure starts qstat -j for the given job id and returns
#     information in an tcl array. 
#
#  INPUTS
#     jobid                   - job id of job
#     {variable qstat_j_info} - array to store information
#
#  SEE ALSO
#     parser/parse_qstat_j()
#*******************************************************************************
proc get_qstat_j_info {jobid {variable qstat_j_info}} {
  global ts_config
   global CHECK_ARCH
   global CHECK_OUTPUT
   upvar $variable jobinfo

   set exit_code [catch { eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qstat -j $jobid" } result]
   if { $exit_code == 0 } {
      set result "$result\n"
      set my_result ""
      set help [split $result "\n"]
      foreach elem_org $help {
         set elem $elem_org
         # removing message id for localized strings
         if { [string first "\[" $elem ] == 0 } {
            set close_pos [ string first "\]" $elem ]
            incr close_pos 1
            set elem [ string range $elem $close_pos end]
            set elem [ string trim $elem]
            debug_puts "removing message id: \"$elem\""
         }
         if { [string first ":" $elem] >= 0 && [string first ":" $elem] < 30 } {
            append my_result "\n$elem" 
         } else {
            append my_result ",$elem"
         }
      }
      set my_result "$my_result\n"
      parse_qstat_j my_result jobinfo $jobid 
      set a_names [array names jobinfo]
#      foreach elem $a_names {
#         puts "$elem: $jobinfo($elem)"
#      }
#      wait_for_enter
      return 1
   }
   return 0
}



#****** sge_procedures/get_qconf_se_info() *************************************
#  NAME
#     get_qconf_se_info() -- get qconf -se information
#
#  SYNOPSIS
#     get_qconf_se_info { hostname {variable qconf_se_info} } 
#
#  FUNCTION
#     This procedure starts qconf -se for the given hostname and returns
#     an tcl array with the output of the command.
#
#  INPUTS
#     hostname                 - execution host name
#     {variable qconf_se_info} - array to store information
#
#
#  SEE ALSO
#      parser/parse_qconf_se()
#*******************************************************************************
proc get_qconf_se_info {hostname {variable qconf_se_info}} {
   global ts_config CHECK_OUTPUT CHECK_USER
   upvar $variable jobinfo

   set arch [resolve_arch $ts_config(master_host)]
   set qconf "$ts_config(product_root)/bin/$arch/qconf"
   set result [start_remote_prog $ts_config(master_host) $CHECK_USER $qconf "-se $hostname"]
   puts $CHECK_OUTPUT $result
   if { $prg_exit_state == 0 } {
      set result "$result\n"
      parse_qconf_se result jobinfo $hostname
      return 1
   }

   return 0
}


#****** sge_procedures/get_qacct_error() ***************************************
#  NAME
#     get_qacct_error() -- error handling for get_qacct
#
#  SYNOPSIS
#     get_qacct_error { result job_id raise_error } 
#
#  FUNCTION
#     Does the error handling for get_qacct.
#     Translates possible error messages of qacct -j <job_id>
#
#  INPUTS
#     result      - qacct output
#     job_id      - job_id for which qacct -j has been called
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for get_qacct function:
#       -1: if accounting file cannot be found (no job ran since cluster startup)
#       -2: if job id is not found in accounting file
#     -999: other error
#
#  NOTES
#     There are most certainly much more error codes that could be handled.
#*******************************************************************************
proc get_qacct_error {result job_id raise_error} {
   global ts_config CHECK_OUTPUT

   # recognize certain error messages and return special return code
   set messages(index) "-1 -2"
   set messages(-1) "*[translate_macro MSG_HISTORY_NOJOBSRUNNINGSINCESTARTUP]"
   set messages(-2) "*[translate_macro MSG_HISTORY_JOBIDXNOTFOUND_D $job_id]"

   # should we have version dependent error messages, create following 
   # procedure in sge_procedures.<version>.tcl
   # get_qacct_error_vdep messages

   # now evaluate return code and raise errors
   set ret [handle_sge_errors "get_qacct" "qacct -j $job_id" $result messages $raise_error]

   return $ret
}

#                                                             max. column:     |
#****** sge_procedures/get_qacct() ******
# 
#  NAME
#     get_qacct -- get job accounting information
#
#  SYNOPSIS
#     get_qacct {job_id {variable qacct_info} {on_host ""} {as_user ""} {raise_error 1}} 
#
#  FUNCTION
#     This procedure will parse the qacct output for the given job id and fill 
#     up the given variable name with information.
#
#  INPUTS
#     job_id                  - job identification number
#     {variable qacct_info}   - name of variable to save the results
#     {on_host ""}            - execute qacct on this host
#     {as_user ""}            - execute qacct as this user
#     {raise_error 1}         - do add_proc error, or only output error messages
#
#  RESULT
#     0, if job was found
#     < 0, on error - see get_qacct_error for a description of possible error codes
#
#  EXAMPLE
#     
#     if { [get_qacct $job_id] == 0 } {
#        set cpu [expr $qacct_info(ru_utime) + $qacct_info(ru_stime)]
#        if { $cpu < 30 } {
#           add_proc_error "example" -1 "cpu entry in accounting ($qacct_info(cpu)) seems 
#                                     to be wrong for job $job_id on host $host"
#        }
#
#        if { $ts_config(product_type) == "sgeee" } {
#           # compute absolute diffence between cpu and ru_utime + ru_stime
#           set difference [expr $cpu - $qacct_info(cpu)]
#           set difference [expr $difference * $difference]
#           if { $difference > 1 } {
#              add_proc_error "example" -1 "accounting: cpu($qacct_info(cpu)) is not the 
#                            sum of ru_utime and ru_stime ($cpu) for 
#                            job $job_id on host $host"
#           }
#        }
#     }
#
#  NOTES
#     look at parser/parse_qacct() for more information
#
#  SEE ALSO
#     parser/parse_qacct()
#     sge_procedures/get_qacct_error()
#*******************************
proc get_qacct {job_id {variable qacct_info} {on_host ""} {as_user ""} {raise_error 1}} {
   global ts_config CHECK_OUTPUT

   upvar $variable qacctinfo
  
   # clear output variable
   if { [info exists qacctinfo] } {
      unset qacctinfo
   }

   set ret 0
   set result [start_sge_bin "qacct" "-j $job_id" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      parse_qacct result qacctinfo $job_id
   } else {
      set ret [get_qacct_error $result $job_id $raise_error]
   }

   return $ret
}


#                                                             max. column:     |
#****** sge_procedures/is_job_running() ******
# 
#  NAME
#     is_job_running -- get run information of job
#
#  SYNOPSIS
#     is_job_running { jobid jobname } 
#
#  FUNCTION
#     This procedure will call qstat -f for job information
#
#  INPUTS
#     jobid   - job identifaction number 
#     jobname - name of the job (string)
#
#  RESULT
#      0 - job is not running (but pending)
#      1 - job is running
#     -1 - not in stat list
#
#  NOTES
#     This procedure returns 1 (job is running) when the job
#     is spooled to a queue. This doesn not automatically mean
#     that the job is "real running".
#
#  SEE ALSO
#     sge_procedures/is_job_running()
#     sge_procedures/is_pid_with_name_existing()
#*******************************
proc is_job_running { jobid jobname } {
   global ts_config
   global CHECK_USER CHECK_OUTPUT check_timestamp

   if { $jobname == ""  } {
      set check_job_name 0
   } else {
      set check_job_name 1
   }
   set arch [resolve_arch $ts_config(master_host)]
   set qstat "$ts_config(product_root)/bin/$arch/qstat"
   set result [start_remote_prog $ts_config(master_host) $CHECK_USER $qstat "-f" catch_state]

   set mytime [timestamp]
   if { $mytime == $check_timestamp } {
      after 1000
   }
   set check_timestamp $mytime

   if { $catch_state != 0 } {
      puts $CHECK_OUTPUT "debug: catch_state: $catch_state"
      puts $CHECK_OUTPUT "debug: result: \n$result"
      return -1
   }

   # split each line as listelement
   set help [split $result "\n"]
   set running_flag 1

   set found 0
   foreach line $help {
#     puts $CHECK_OUTPUT "debug: $line"
     if {[lsearch $line "####*"] >= 0} {
       set running_flag 0
     }

     if { $check_job_name != 0 } {
        if { ([string first $jobname $line ] >= 0) && ([lindex $line 0] == $jobid)  } {
          set found 1;
          break;
        }
     } else {
        if { [lindex $line 0] == $jobid } {
          set found 1;
          break;
        }
     }
   } 

   if { $found == 1 } {
      return $running_flag
   }
   return -1
}



#****** sge_procedures/get_job_state() *****************************************
#  NAME
#     get_job_state() -- get job state information
#
#  SYNOPSIS
#     get_job_state { jobid { not_all_equal 0 } { taskid task_id } } 
#
#  FUNCTION
#     This procedure parses the output of the qstat -f command and returns
#     the job state or an tcl array with detailed information
#
#  INPUTS
#     jobid               - Job id of job to get information for
#     { not_all_equal 0 } - if 0 (default): The procedure will wait until
#                           all tasks of a job array have the same state
#                           if 1: The procedure will return an tcl list
#                           with the job states and fill the array (given
#                           optional in parameter 3) "task_id" with information.
#     { taskid task_id }  - tcl array name to fill information if not_all_equal 
#                           is set to 1
#
#  RESULT
#     tcl array:
#          task_id($lfnr,state)     -> task state
#          task_id($lfnr,task)      -> task no
#
#          lfnr is a number between 0 and the length of the returned tcl list
#         
#
#*******************************************************************************
proc get_job_state { jobid { not_all_equal 0 } { taskid task_id } } {
  global ts_config
   global CHECK_ARCH CHECK_OUTPUT check_timestamp
   upvar $taskid r_task_id
   set mytime [timestamp]

   if { $mytime == $check_timestamp } {
      after 1000
   }
   set check_timestamp $mytime

   set my_timeout [ expr ( $mytime + 100 ) ]
   set states_all_equal 0
   while { $my_timeout > [timestamp] && $states_all_equal == 0 } {   
      set states_all_equal 1

      if {$ts_config(gridengine_version) == 53} {
         set catch_state [ catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qstat" "-f" } result ]
      } else {
         set catch_state [ catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qstat" "-f" "-g" "t"} result ]
      }
      if { $catch_state != 0 } {
         puts $CHECK_OUTPUT "debug: $result"
         return -1
      }
   #   puts $CHECK_OUTPUT "debug: catch_state: $catch_state"
   
      # split each line as listelement
      set help [split $result "\n"]
      set running_flag 1
   
      set states ""
      set lfnr 0
      foreach line $help {
        if { [lindex $line 0] == $jobid } {
           lappend states [lindex $line 4]
           debug_puts "debug: $line"
           if { [lindex $line 7] == "MASTER" } {
              set r_task_id($lfnr,task)  [lindex $line 8]
           } else {
              set r_task_id($lfnr,task)  [lindex $line 7]
           }
           set r_task_id($lfnr,state) [lindex $line 4]
           incr lfnr 1
        }
      }
      if { $states == "" } {
         set states -1
      }
      
      set main_state [lindex $states 0]
      if { $not_all_equal == 0 } {
         for { set elem 0 } { $elem < [llength $states] } { incr elem 1 } {
            if { [ string compare [lindex $states $elem] $main_state ] != 0 } {
               puts $CHECK_OUTPUT "jobstate of task $elem is: [lindex $states $elem], waiting ..."
               set states_all_equal 0
            } 
         }
         after 1000
      }
   }
   if { $not_all_equal != 0 } {
      return $states
   }

   if { $states_all_equal == 1 } {
      return $main_state
   }
 
   add_proc_error "get_job_state" -1 "more than one job id found with different states"
   return -1
}

# wait for start of job ($jobid,$jobname) ; timeout after $seconds
# results : -1 on timeout ; 0 on jobstart
#                                                             max. column:     |
#****** sge_procedures/wait_for_jobstart() ******
# 
#  NAME
#     wait_for_jobstart -- wait for job to get out of pending list
#
#  SYNOPSIS
#     wait_for_jobstart { jobid jobname seconds {do_errorcheck 1} {do_tsm 0} } 
#
#  FUNCTION
#     This procedure will call the is_job_running procedure in a while
#     loop. When the job is scheduled to a queue the job is "running" 
#     and the procedure returns.
#
#  INPUTS
#     jobid             - job identification number
#     jobname           - name of the job
#     seconds           - timeout in seconds
#     {do_errorcheck 1} - enable error check (default)
#                         if 0: do not report errors
#     {do_tsm 0}        - do qconf -tsm before waiting
#                         if 1: do qconf -tsm (trigger scheduler) 
#
#  RESULT
#     -1 - job is not running (timeout error)
#      0 - job is running ( not in pending state)
#
#  EXAMPLE
#     foreach elem $jobs {
#        wait_for_jobstart $elem "Sleeper" 300
#        wait_for_end_of_transfer $elem 300
#        append jobs_string "$elem "
#     }
#
#  SEE ALSO
#     sge_procedures/wait_for_load_from_all_queues()
#     file_procedures/wait_for_file()
#     sge_procedures/wait_for_jobstart()
#     sge_procedures/wait_for_end_of_transfer()
#     sge_procedures/wait_for_jobpending()
#     sge_procedures/wait_for_jobend()
#*******************************
proc wait_for_jobstart { jobid jobname seconds {do_errorcheck 1} {do_tsm 0} } {
  global ts_config
  
  global CHECK_OUTPUT CHECK_ARCH

  if { [is_job_id $jobid] != 1  } {
     if { $do_errorcheck == 1 } {
          add_proc_error "wait_for_jobstart" -1 "got unexpected job id: $jobid"
     }
     return -1
  }

   if {$do_tsm == 1} {
      trigger_scheduling
   }

  puts $CHECK_OUTPUT "Waiting for start of job $jobid ($jobname)"
  if { $do_errorcheck != 1 } {
     puts $CHECK_OUTPUT "error check is switched off"
  }
   
  set time [timestamp]
  while {1} {
    
    set run_result [is_job_running $jobid $jobname]
    if {$run_result == 1} {
       break;
    } 
    set runtime [expr ( [timestamp] - $time) ]
    if { $runtime >= $seconds } {
       if { $do_errorcheck == 1 } {
          add_proc_error "wait_for_jobstart" -1 "timeout waiting for job \"$jobid\" \"$jobname\""
       }
       return -1
    }
    after 500
  }
  return 0
}


#                                                             max. column:     |
#****** sge_procedures/wait_for_end_of_transfer() ******
# 
#  NAME
#     wait_for_end_of_transfer -- wait transfer end of job
#
#  SYNOPSIS
#     wait_for_end_of_transfer { jobid seconds } 
#
#  FUNCTION
#     This procedure will parse the qstat output of the job for the t state. If
#     no t state is found for the given job id, the procedure will return.
#
#  INPUTS
#     jobid   - job identification number
#     seconds - timeout in seconds
#
#  RESULT
#      0 - job is not in transferstate
#     -1 - timeout
#
#  EXAMPLE
#     see "sge_procedures/wait_for_jobstart"
#
#  SEE ALSO
#     sge_procedures/wait_for_load_from_all_queues()
#     file_procedures/wait_for_file()
#     sge_procedures/wait_for_jobstart()
#     sge_procedures/wait_for_end_of_transfer()
#     sge_procedures/wait_for_jobpending()
#     sge_procedures/wait_for_jobend()
#*******************************
proc wait_for_end_of_transfer { jobid seconds } {
  global ts_config
  global CHECK_OUTPUT

  puts $CHECK_OUTPUT "Waiting for job $jobid to finish transfer state"
  
  set time [timestamp] 
  while {1} {
    set run_result [get_standard_job_info $jobid ]
    set job_state ""
    set had_error 0
    foreach line $run_result {
       set tmp_job_id [lindex $line 0]
       set tmp_job_state [ lindex $line 4 ]
       if { $tmp_job_id == $jobid } {
          if { $job_state == "" } {
             set job_state $tmp_job_state
          } else {
             if { $job_state != $tmp_job_state } {
                puts $CHECK_OUTPUT "job has different states ..."
                set had_error 1
                break
             }
          }
       }
    }

    if { $had_error != 0 } {
       after 1000
       continue
    }

    if { [string first "t" $job_state ] < 0} {
       puts $CHECK_OUTPUT "job $jobid is running ($job_state)"
       break;
    }
    
    set runtime [expr ( [timestamp] - $time) ]
    if { $runtime >= $seconds } {
       add_proc_error "wait_for_end_of_transfer" -1 "timeout waiting for job \"$jobid\""
       return -1
    }
    after 1000
  }
  return 0
}

# wait for job to be in pending state ($jobid,$jobname) ; timeout after $seconds
# results : -1 on timeout ; 0 on pending
#                                                             max. column:     |
#****** sge_procedures/wait_for_jobpending() ******
# 
#  NAME
#     wait_for_jobpending -- wait for job to get into pending state
#
#  SYNOPSIS
#     wait_for_jobpending { jobid jobname seconds { or_running 0 } } 
#
#  FUNCTION
#     This procedure will return when the job is in pending state.
#
#  INPUTS
#     jobid   - job identification number
#     jobname - name of the job
#     seconds - timeout value in seconds
#     { or_running 0 } - if job is already running, report no error
#
#  RESULT
#     -1  on timeout
#     0   when job is in pending state
#
#  EXAMPLE
#     foreach elem $sched_jobs {
#         wait_for_jobpending $elem "Sleeper" 300
#     }
#
#  SEE ALSO
#     sge_procedures/wait_for_load_from_all_queues()
#     file_procedures/wait_for_file()
#     sge_procedures/wait_for_jobstart()
#     sge_procedures/wait_for_end_of_transfer()
#     sge_procedures/wait_for_jobpending()
#     sge_procedures/wait_for_jobend()
#*******************************
proc wait_for_jobpending {jobid jobname seconds {or_running 0}} {
  global ts_config CHECK_OUTPUT

  puts $CHECK_OUTPUT "Waiting for job $jobid ($jobname) to get in pending state"

  if {[is_job_id $jobid] != 1} {
     puts $CHECK_OUTPUT "job is not integer"
     add_proc_error "wait_for_jobpending" -1 "unexpected job id: $jobid"
     return -1
  }

  set time [timestamp] 
  while {1} {
    set run_result [is_job_running $jobid $jobname]
    if {$run_result == 0} {
       break;
    }
    if {$run_result == 1 && $or_running == 1  } {
       break;
    }
    set runtime [expr ( [timestamp] - $time) ]
    if { $runtime >= $seconds } {
       add_proc_error "wait_for_jobpending" -1 "timeout waiting for job \"$jobid\" \"$jobname\" (timeout was $seconds sec)"
       return -1
    }
    after 1000
  }
  return 0
}


# set job in hold state
# results: -1 on timeout, 0 ok
#                                                             max. column:     |
#****** sge_procedures/hold_job() ******
# 
#  NAME
#     hold_job -- set job in hold state
#
#  SYNOPSIS
#     hold_job { jobid } 
#
#  FUNCTION
#     This procedure will use the qhold binary to set a job into hold state.
#
#  INPUTS
#     jobid - job identification number
#
#  RESULT
#        0 - ok
#       -1 - timeout error    
#
#  SEE ALSO
#     sge_procedures/release_job()
#     sge_procedures/hold_job()
#*******************************
proc hold_job { jobid } {
  global ts_config

   global CHECK_ARCH  CHECK_HOST CHECK_USER

   set MODIFIED_HOLD [translate $CHECK_HOST 1 0 0 [sge_macro MSG_SGETEXT_MOD_JOBS_SU] "*" "*"]

   # spawn process
   log_user 0
   set program "$ts_config(product_root)/bin/$CHECK_ARCH/qhold"
   set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER $program "$jobid" ]

   set sp_id [ lindex $id 1 ]
   set timeout 30
   set result -1
   	
   log_user 0 

   expect {
       -i $sp_id full_buffer {
          set result -1 
          add_proc_error "hold_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value" 
       }
       -i $sp_id "modified hold of job" {
          set result 0
       }
       -i $sp_id $MODIFIED_HOLD {
          set result 0
       }
       -i $sp_id default {
          set result -1 
       }

   }
   # close spawned process 
   close_spawn_process $id
   log_user 1
   if { $result != 0 } {
      add_proc_error "hold_job" -1 "could not hold job $jobid"
   }
   return $result

}


#                                                             max. column:     |
#****** sge_procedures/release_job() ******
# 
#  NAME
#     release_job -- release job from hold state
#
#  SYNOPSIS
#     release_job { jobid } 
#
#  FUNCTION
#     This procedure will release the job from hold.
#
#  INPUTS
#     jobid - job identification number
#
#  RESULT
#      0   - ok
#     -1   - timeout error
#
#  SEE ALSO
#     sge_procedures/release_job()
#     sge_procedures/hold_job()
#*******************************
proc release_job { jobid } {
  global ts_config

   global CHECK_ARCH  CHECK_HOST CHECK_USER
 
   # spawn process
   log_user 0

   set MODIFIED_HOLD [translate $CHECK_HOST 1 0 0 [sge_macro MSG_SGETEXT_MOD_JOBS_SU] "*" "*"]
   set MODIFIED_HOLD_ARRAY [ translate $CHECK_HOST 1 0 0 [sge_macro MSG_SGETEXT_MOD_JATASK_SUU] "*" "*" "*"]

   set program "$ts_config(product_root)/bin/$CHECK_ARCH/qrls"
   set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER $program "$jobid" ]

   set sp_id [ lindex $id 1 ]
   set timeout 30
   set result -1	
   log_user 0

   expect {
       -i $sp_id full_buffer {
          set result -1
          add_proc_error "release_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
       }
       -i $sp_id $MODIFIED_HOLD {
          set result 0
       }
       -i $sp_id $MODIFIED_HOLD_ARRAY {
          set result 0
       }
 
       -i $sp_id "modified hold of job" {
          set result 0
       }
       -i $sp_id default {
          set result -1 
       }
   }

   # close spawned process 
   close_spawn_process $id
   log_user 1
   if { $result != 0 } {
      add_proc_error "release_job" -1 "could not release job $jobid"
   }
   return $result

}


#                                                             max. column:     |
#****** sge_procedures/wait_for_jobend() ******
# 
#  NAME
#     wait_for_jobend -- wait for end of job
#
#  SYNOPSIS
#     wait_for_jobend { jobid jobname seconds 
#                       { runcheck 1} 
#                       { wait_for_end 0 } } 
#
#  FUNCTION
#     This procedure is testing first if the given job is really running. After
#     that it waits for the job to disappear in the qstat output.
#
#  INPUTS
#     jobid   - job identification number
#     jobname - name of job
#     seconds - timeout in seconds
#
#     optional parameters:
#     { runcheck }     - if 1 (default): check if job is running
#     { wait_for_end } - if 0 (default): no for real job end waiting (job
#                                        removed from qmaster internal list)
#                        if NOT 0:       wait for qmaster to remove job from
#                                        internal list
#
#  RESULT
#      0 - job stops running
#     -1 - timeout error
#     -2 - job is not running
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     sge_procedures/wait_for_end_of_all_jobs()
#     sge_procedures/wait_for_load_from_all_queues()
#     file_procedures/wait_for_file()
#     sge_procedures/wait_for_jobstart()
#     sge_procedures/wait_for_end_of_transfer()
#     sge_procedures/wait_for_jobpending()
#     sge_procedures/wait_for_jobend()
#*******************************
proc wait_for_jobend { jobid jobname seconds {runcheck 1} { wait_for_end 0 } } {
  global ts_config
  
  global CHECK_OUTPUT

  puts $CHECK_OUTPUT "Waiting for end of job $jobid ($jobname)"
  
  if { $runcheck == 1 } {
     if { [is_job_running $jobid $jobname] != 1 } {
        add_proc_error "wait_for_jobend" -1 "job \"$jobid\" \"$jobname\" is not running"
        return -2
     }
  }
 
  set time [timestamp]
  while {1} {
    set run_result [is_job_running $jobid $jobname]
    if {$run_result == -1} {
       break;
    } 
    set runtime [expr ( [timestamp] - $time) ]
    if { $runtime >= $seconds } {
       add_proc_error "wait_for_jobend" -1 "timeout waiting for job \"$jobid\" \"$jobname\":\nis_job_running returned $run_result"
       return -1
    }
    after 1000
  }

  if { $wait_for_end != 0 } {
      set my_timeout [timestamp]
      incr my_timeout 90
      while { [get_qstat_j_info $jobid ] != 0 } {
          puts $CHECK_OUTPUT "waiting for jobend ..."
          after 1000
          if { [timestamp] > $my_timeout } {
             add_proc_error "wait_for_jobend" -1 "timeout while waiting for jobend"
             break;
          }
      }
   }

  return 0
}


#****** sge_procedures/startup_qmaster() ***************************************
#  NAME
#     startup_qmaster() -- startup qmaster (and scheduler) daemon
#
#  SYNOPSIS
#     startup_qmaster { {and_scheduler 1}  {env_list ""}  } 
#
#  FUNCTION
#     Startup the qmaster daemon on the configured testsuite host. An environ
#     ment can be set which is used as parameter for the start_remote_prog()
#     call.
#
#  INPUTS
#     {and_scheduler 1} - optional: also start the schedd daemon 
#     env_list          - optional: use given environment
#
#  SEE ALSO
#     sge_procedures/shutdown_core_system()
#     sge_procedures/shutdown_master_and_scheduler()
#     sge_procedures/shutdown_all_shadowd()
#     sge_procedures/shutdown_system_daemon()
#     sge_procedures/startup_qmaster()
#     sge_procedures/startup_execd()
#     sge_procedures/startup_shadowd()
#*******************************************************************************
proc startup_qmaster { {and_scheduler 1} {env_list ""} {on_host ""} } {
   global ts_config
   global CHECK_OUTPUT
   global CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER
   global CHECK_SCRIPT_FILE_DIR CHECK_TESTSUITE_ROOT CHECK_DEBUG_LEVEL
   global schedd_debug master_debug CHECK_DISPLAY_OUTPUT CHECK_SGE_DEBUG_LEVEL

   if {$env_list != ""} {
      upvar $env_list envlist
   }

   set start_host $CHECK_CORE_MASTER

   if { $on_host != "" } {
      set start_host $on_host
   }

   if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
      if { [have_root_passwd] != 0  } {
         add_proc_error "startup_qmaster" "-2" "no root password set or ssh not available"
         return -1
      }
      set startup_user "root"
   } else {
      set startup_user $CHECK_USER
   } 

   if {$and_scheduler} {
      set schedd_message "and scheduler"
   } else {
      set schedd_message ""
   }
   puts $CHECK_OUTPUT "starting up qmaster $schedd_message on host \"$start_host\" as user \"$startup_user\""
   set arch [resolve_arch $start_host]

   if { $master_debug != 0 } {
      puts $CHECK_OUTPUT "using DISPLAY=${CHECK_DISPLAY_OUTPUT}"
      start_remote_prog "$start_host" "$startup_user" "/usr/bin/X11/xterm" "-bg darkolivegreen -fg navajowhite -sl 5000 -sb -j -display $CHECK_DISPLAY_OUTPUT -e $CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/debug_starter.sh /tmp/out.$CHECK_USER.qmaster.$start_host \"$CHECK_SGE_DEBUG_LEVEL\" $ts_config(product_root)/bin/${arch}/sge_qmaster &" prg_exit_state 60 2 ""
   } else {
      start_remote_prog "$start_host" "$startup_user" "$ts_config(product_root)/bin/${arch}/sge_qmaster" ";sleep 2" prg_exit_state 60 0 envlist

   }

   if {$and_scheduler} {
      puts $CHECK_OUTPUT "starting up scheduler ..."
      if { $schedd_debug != 0 } {
         puts $CHECK_OUTPUT "using DISPLAY=${CHECK_DISPLAY_OUTPUT}"
         puts $CHECK_OUTPUT "starting schedd as $startup_user" 
         start_remote_prog "$start_host" "$startup_user" "/usr/bin/X11/xterm" "-bg darkolivegreen -fg navajowhite -sl 5000 -sb -j -display $CHECK_DISPLAY_OUTPUT -e $CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/debug_starter.sh /tmp/out.$CHECK_USER.schedd.$start_host \"$CHECK_SGE_DEBUG_LEVEL\" $ts_config(product_root)/bin/${arch}/sge_schedd &" prg_exit_state 60 2 ""
      } else {
         puts $CHECK_OUTPUT "starting schedd as $startup_user" 
         set result [start_remote_prog "$start_host" "$startup_user" "$ts_config(product_root)/bin/${arch}/sge_schedd" "" prg_exit_state 60 0 envlist]
         puts $CHECK_OUTPUT $result
      }
   }
 
   return 0
}

#****** sge_procedures/startup_scheduler() *************************************
#  NAME
#     startup_scheduler() -- ??? 
#
#  SYNOPSIS
#     startup_scheduler { } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc startup_scheduler {} {
  global ts_config
   global CHECK_OUTPUT
   global CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER
   global CHECK_SCRIPT_FILE_DIR CHECK_TESTSUITE_ROOT CHECK_DEBUG_LEVEL
   global schedd_debug CHECK_DISPLAY_OUTPUT CHECK_SGE_DEBUG_LEVEL

   if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
      if { [have_root_passwd] != 0  } {
         add_proc_error "startup_scheduler" "-2" "no root password set or ssh not available"
         return -1
      }
      set startup_user "root"
   } else {
      set startup_user $CHECK_USER
   } 

   puts $CHECK_OUTPUT "starting up scheduler on host \"$CHECK_CORE_MASTER\" as user \"$startup_user\""
   set arch [resolve_arch $CHECK_CORE_MASTER]

   if { $schedd_debug != 0 } {
      puts $CHECK_OUTPUT "using DISPLAY=${CHECK_DISPLAY_OUTPUT}"
      start_remote_prog "$CHECK_CORE_MASTER" "$startup_user" "/usr/bin/X11/xterm" "-bg darkolivegreen -fg navajowhite -sl 5000 -sb -j -display $CHECK_DISPLAY_OUTPUT -e $CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/debug_starter.sh /tmp/out.$CHECK_USER.schedd.$CHECK_CORE_MASTER \"$CHECK_SGE_DEBUG_LEVEL\" $ts_config(product_root)/bin/${arch}/sge_schedd &" prg_exit_state 60 2 ""
   } else {
      start_remote_prog "$CHECK_CORE_MASTER" "$startup_user" "$ts_config(product_root)/bin/${arch}/sge_schedd" ""
   }
   
   return 0
}

#****** sge_procedures/startup_execd_raw() *************************************
#  NAME
#     startup_execd_raw() -- startup execd without using startup script
#
#  SYNOPSIS
#     startup_execd_raw { hostname } 
#
#  FUNCTION
#     Startup execd on remote host
#
#  INPUTS
#     hostname - host to start up execd
#
#  RESULT
#     0 -> ok   1 -> error
#
#  SEE ALSO
#     sge_procedures/startup_execd()
#*******************************************************************************
proc startup_execd_raw { hostname {envlist ""}} {
  global ts_config
   global CHECK_OUTPUT
   global CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER
   global CHECK_CORE_MASTER

   if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
      if { [have_root_passwd] != 0  } {
         add_proc_error "startup_execd" "-2" "no root password set or ssh not available"
         return -1
      }
      set startup_user "root"
   } else {
      set startup_user $CHECK_USER
   }

   puts $CHECK_OUTPUT "starting up execd on host \"$hostname\" as user \"$startup_user\""
   set remote_arch [ resolve_arch $hostname ]

   # Setup environment
   if {$envlist != ""} {
      upvar $envlist my_envlist

      set names [array names my_envlist]

      foreach var $names {
         set my_environment($var) $my_envlist($var)
      }
   }

   set my_environment(COMMD_HOST) $CHECK_CORE_MASTER

   set output [start_remote_prog "$hostname" "$startup_user" "$ts_config(product_root)/bin/$remote_arch/sge_execd" "-nostart-commd" prg_exit_state 60 0 my_environment 1 1 1]

   set ALREADY_RUNNING [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_COMMPROC_ALREADY_STARTED_S] "*"]

   if { [string match "*$ALREADY_RUNNING" $output ] } {
      add_proc_error "startup_execd" -1 "execd on host $hostname is already running"
      return -1
   }
   return 0
}

# return values: 
# 3 - master and scheduler are running
# 2 - master is running, scheduler is not running
# 1 - master is not running, scheduler is running
# 0 - master and scheduler are not running

#                                                             max. column:     |
#****** sge_procedures/are_master_and_scheduler_running() ******
# 
#  NAME
#     are_master_and_scheduler_running -- ??? 
#
#  SYNOPSIS
#     are_master_and_scheduler_running { hostname qmaster_spool_dir } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname          - ??? 
#     qmaster_spool_dir - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************
proc are_master_and_scheduler_running { hostname qmaster_spool_dir } {
  global ts_config
 
   global CHECK_OUTPUT CHECK_USER 

   set qmaster_pid -1
   set scheduler_pid -1

   set running 0


   set qmaster_pid [ start_remote_prog "$hostname" "$CHECK_USER" "cat" "$qmaster_spool_dir/qmaster.pid" ]
   set qmaster_pid [ string trim $qmaster_pid ]
   if { $prg_exit_state != 0 } {
      set qmaster_pid -1
   }

   set scheduler_pid [ start_remote_prog "$hostname" "$CHECK_USER" "cat" "$qmaster_spool_dir/schedd/schedd.pid" ]
   set scheduler_pid [ string trim $scheduler_pid ]
   if { $prg_exit_state != 0 } {
      set scheduler_pid -1
   }


   get_ps_info $qmaster_pid $hostname

   if { ($ps_info($qmaster_pid,error) == 0) && ( [ string first "qmaster" $ps_info($qmaster_pid,string)] >= 0 )  } {
      incr running 2
   }

   get_ps_info $scheduler_pid $hostname

   if { ($ps_info($scheduler_pid,error) == 0) && ( [ string first "schedd" $ps_info($scheduler_pid,string)] >= 0  ) } {
      incr running 1
   }

   return $running
}


# kills master and scheduler on the given hostname
#                                                             max. column:     |
#****** sge_procedures/shutdown_master_and_scheduler() ******
# 
#  NAME
#     shutdown_master_and_scheduler -- ??? 
#
#  SYNOPSIS
#     shutdown_master_and_scheduler { hostname qmaster_spool_dir } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname          - ??? 
#     qmaster_spool_dir - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     sge_procedures/shutdown_core_system()
#     sge_procedures/shutdown_master_and_scheduler()
#     sge_procedures/shutdown_all_shadowd()
#     sge_procedures/shutdown_system_daemon()
#     sge_procedures/startup_qmaster()
#     sge_procedures/startup_execd()
#     sge_procedures/startup_shadowd()
#*******************************
proc shutdown_master_and_scheduler {hostname qmaster_spool_dir} {
   shutdown_scheduler $hostname $qmaster_spool_dir
   shutdown_qmaster $hostname $qmaster_spool_dir
}

#****** sge_procedures/shutdown_scheduler() ************************************
#  NAME
#     shutdown_scheduler() -- ??? 
#
#  SYNOPSIS
#     shutdown_scheduler { hostname qmaster_spool_dir } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname          - ??? 
#     qmaster_spool_dir - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc shutdown_scheduler {hostname qmaster_spool_dir} {
   global ts_config
   global CHECK_OUTPUT CHECK_USER CHECK_ADMIN_USER_SYSTEM
   global CHECK_ARCH

   puts $CHECK_OUTPUT "shutdown_scheduler ..."

   puts $CHECK_OUTPUT ""
   puts $CHECK_OUTPUT "killing scheduler on host $hostname ..."
   puts $CHECK_OUTPUT "retrieving data from spool dir $qmaster_spool_dir"



   set scheduler_pid [ get_scheduler_pid $hostname $qmaster_spool_dir ]

   get_ps_info $scheduler_pid $hostname
   if { ($ps_info($scheduler_pid,error) == 0) } {
      if { [ is_pid_with_name_existing $hostname $scheduler_pid "sge_schedd" ] == 0 } { 
         puts $CHECK_OUTPUT "shutdown schedd with pid $scheduler_pid on host $hostname"
         puts $CHECK_OUTPUT "do a qconf -ks ..."
         catch {  eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-ks" } result
         puts $CHECK_OUTPUT $result
         after 1500
         shutdown_system_daemon $hostname sched

      } else {
         add_proc_error "shutdown_scheduler" "-1" "scheduler pid $scheduler_pid not found"
         set scheduler_pid -1
      }
   } else {
      add_proc_error "shutdown_scheduler" "-1" "ps_info failed (1), pid=$scheduler_pid"
      set scheduler_pid -1
   }

   puts $CHECK_OUTPUT "done."
}  
#****** sge_procedures/is_scheduler_alive() ************************************
#  NAME
#     is_scheduler_alive() -- ??? 
#
#  SYNOPSIS
#     is_scheduler_alive { hostname qmaster_spool_dir } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname          - ??? 
#     qmaster_spool_dir - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc is_scheduler_alive { hostname qmaster_spool_dir } {
   global ts_config

   set scheduler_pid [get_scheduler_pid $hostname $qmaster_spool_dir]
   get_ps_info $scheduler_pid $hostname
   
   set alive 0
   if { ($ps_info($scheduler_pid,error) == 0) } {
      if { [ is_pid_with_name_existing $hostname $scheduler_pid "sge_schedd" ] == 0 } { 
         set alive 1
      }
   }

   return $alive
}

#****** sge_procedures/is_qmaster_alive() **************************************
#  NAME
#     is_qmaster_alive() -- check if qmaster process is running
#
#  SYNOPSIS
#     is_qmaster_alive { hostname qmaster_spool_dir } 
#
#  FUNCTION
#     This function searches the process table for a running qmaster process
#
#  INPUTS
#     hostname          - qmaster hostname
#     qmaster_spool_dir - qmaster spool dir
#
#  RESULT
#     1 on success
#     0 on error
#
#  SEE ALSO
#     sge_procedures/is_scheduler_alive()
#*******************************************************************************
proc is_qmaster_alive { hostname qmaster_spool_dir } {
   global ts_config

   set qmaster_pid [get_qmaster_pid]
   get_ps_info $qmaster_pid $hostname
   
   set alive 0
   if { ($ps_info($qmaster_pid,error) == 0) } {
      if { [ is_pid_with_name_existing $hostname $qmaster_pid "sge_qmaster" ] == 0 } { 
         set alive 1
      }
   }

   return $alive
}

#****** sge_procedures/get_scheduler_pid() *************************************
#  NAME
#     get_scheduler_pid() -- ??? 
#
#  SYNOPSIS
#     get_scheduler_pid { hostname qmaster_spool_dir } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname          - ??? 
#     qmaster_spool_dir - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc get_scheduler_pid { hostname qmaster_spool_dir } {
   global ts_config
   global CHECK_USER 

   set scheduler_pid -1
   set scheduler_pid [ start_remote_prog "$hostname" "$CHECK_USER" "cat" "$qmaster_spool_dir/schedd/schedd.pid" ]
   set scheduler_pid [ string trim $scheduler_pid ]
   if { $prg_exit_state != 0 } {
      set scheduler_pid -1
   }
   return $scheduler_pid
}

#****** sge_procedures/shutdown_qmaster() **************************************
#  NAME
#     shutdown_qmaster() -- ??? 
#
#  SYNOPSIS
#     shutdown_qmaster { hostname qmaster_spool_dir } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname          - ??? 
#     qmaster_spool_dir - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc shutdown_qmaster {hostname qmaster_spool_dir} {
   global ts_config
   global CHECK_OUTPUT CHECK_USER CHECK_ADMIN_USER_SYSTEM
   global CHECK_ARCH

   puts $CHECK_OUTPUT "shutdown_qmaster ..."

   puts $CHECK_OUTPUT ""
   puts $CHECK_OUTPUT "killing qmaster on host $hostname ..."
   puts $CHECK_OUTPUT "retrieving data from spool dir $qmaster_spool_dir"

   set qmaster_pid -1

   set qmaster_pid [ start_remote_prog "$hostname" "$CHECK_USER" "cat" "$qmaster_spool_dir/qmaster.pid" ]
   set qmaster_pid [ string trim $qmaster_pid ]
   if { $prg_exit_state != 0 } {
      set qmaster_pid -1
   }
 
   get_ps_info $qmaster_pid $hostname
   if { ($ps_info($qmaster_pid,error) == 0) } {
      if { [ is_pid_with_name_existing $hostname $qmaster_pid "sge_qmaster" ] == 0 } { 
         puts $CHECK_OUTPUT "killing qmaster with pid $qmaster_pid on host $hostname"
         puts $CHECK_OUTPUT "do a qconf -km ..."

         catch {  eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-km" } result
         puts $CHECK_OUTPUT $result

         wait_till_qmaster_is_down $hostname
       
         shutdown_system_daemon $hostname qmaster
      } else {
         add_proc_error "shutdown_qmaster" "-1" "qmaster pid $qmaster_pid not found"
         set qmaster_pid -1
      }
   } else {
      add_proc_error "shutdown_qmaster" "-1" "ps_info failed (2), pid=$qmaster_pid"
      set qmaster_pid -1
   }

   puts $CHECK_OUTPUT "done."
}  

#                                                             max. column:     |
#****** sge_procedures/shutdown_all_shadowd() ******
# 
#  NAME
#     shutdown_all_shadowd -- ??? 
#
#  SYNOPSIS
#     shutdown_all_shadowd { hostname } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     sge_procedures/shutdown_core_system()
#     sge_procedures/shutdown_master_and_scheduler()
#     sge_procedures/shutdown_all_shadowd()
#     sge_procedures/shutdown_system_daemon()
#     sge_procedures/startup_qmaster()
#     sge_procedures/startup_execd()
#     sge_procedures/startup_shadowd()
#*******************************
proc shutdown_all_shadowd { hostname } {
   global ts_config
   global CHECK_OUTPUT CHECK_ADMIN_USER_SYSTEM
   global CHECK_USER 

   set num_proc 0

   puts $CHECK_OUTPUT ""
   puts $CHECK_OUTPUT "shutdown all shadowd daemon for system installed at $ts_config(product_root) ..."

   set index_list [ ps_grep "$ts_config(product_root)" "$hostname" ]
   set new_index ""
   foreach elem $index_list {
      if { [ string first "shadowd" $ps_info(string,$elem) ] >= 0 } {
         lappend new_index $elem
      }
   } 
   set num_proc [llength $new_index]
   puts $CHECK_OUTPUT "Number of matching processes: $num_proc"
   foreach elem $new_index {
      debug_puts $ps_info(string,$elem)
      if { [ is_pid_with_name_existing $hostname $ps_info(pid,$elem) "sge_shadowd" ] == 0 } {
         puts $CHECK_OUTPUT "killing process [ set ps_info(pid,$elem) ] ..."
         if { [ have_root_passwd ] == -1 } {
            set_root_passwd 
         }
         if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
             start_remote_prog "$hostname" "root" "kill" "$ps_info(pid,$elem)"
         } else {
             start_remote_prog "$hostname" "$CHECK_USER" "kill" "$ps_info(pid,$elem)"
         }
      } 
   }

   foreach elem $new_index {
      debug_puts $ps_info(string,$elem)
      if { [ is_pid_with_name_existing $hostname $ps_info(pid,$elem) "sge_shadowd" ] == 0 } {
         add_proc_error "shutdown_all_shadowd" "-3" "could not shutdown shadowd at host $elem with term signal"
         puts $CHECK_OUTPUT "Killing process with kill signal [ set ps_info(pid,$elem) ] ..."
         if { [ have_root_passwd ] == -1 } {
            set_root_passwd 
         }
         if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
             start_remote_prog "$hostname" "root" "kill" "-9 $ps_info(pid,$elem)"
         } else {
             start_remote_prog "$hostname" "$CHECK_USER" "kill" "-9 $ps_info(pid,$elem)"
         }
      } 
   }

   foreach elem $new_index {
      debug_puts $ps_info(string,$elem)
      if { [ is_pid_with_name_existing $hostname $ps_info(pid,$elem) "sge_shadowd" ] == 0 } {
         add_proc_error "shutdown_all_shadowd" "-1" "could not shutdown shadowd at host $elem with kill signal"
      }
   }

   return $num_proc
}


#                                                             max. column:     |
#****** sge_procedures/shutdown_bdb_rpc() ******
# 
#  NAME
#     shutdown_bdb_rpc -- ??? 
#
#  SYNOPSIS
#     shutdown_bdb_rpc { hostname } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     sge_procedures/shutdown_core_system()
#     sge_procedures/shutdown_master_and_scheduler()
#     sge_procedures/shutdown_all_shadowd()
#     sge_procedures/shutdown_bdb_rpc()
#     sge_procedures/shutdown_system_daemon()
#     sge_procedures/startup_qmaster()
#     sge_procedures/startup_execd()
#     sge_procedures/startup_shadowd()
#*******************************
proc shutdown_bdb_rpc { hostname } {
   global ts_config
   global CHECK_OUTPUT CHECK_ADMIN_USER_SYSTEM
   global CHECK_USER 

   set num_proc 0

   puts $CHECK_OUTPUT ""
   puts $CHECK_OUTPUT "shutdown bdb_rpc daemon for system installed at $ts_config(product_root) ..."

   set index_list [ ps_grep "$ts_config(product_root)" "$hostname" ]
   set new_index ""
   foreach elem $index_list {
      if { [ string first "berkeley_db_svc" $ps_info(string,$elem) ] >= 0 } {
         lappend new_index $elem
      }
   } 
   set num_proc [llength $new_index]
   puts $CHECK_OUTPUT "Number of matching processes: $num_proc"
   foreach elem $new_index {
      debug_puts $ps_info(string,$elem)
      if { [ is_pid_with_name_existing $hostname $ps_info(pid,$elem) "berkeley_db_svc" ] == 0 } {
         puts $CHECK_OUTPUT "killing process [ set ps_info(pid,$elem) ] ..."
         if { [ have_root_passwd ] == -1 } {
            set_root_passwd 
         }
         if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
             start_remote_prog "$hostname" "root" "kill" "$ps_info(pid,$elem)"
         } else {
             start_remote_prog "$hostname" "$CHECK_USER" "kill" "$ps_info(pid,$elem)"
         }
      } 
   }

   foreach elem $new_index {
      debug_puts $ps_info(string,$elem)
      if { [ is_pid_with_name_existing $hostname $ps_info(pid,$elem) "berkeley_db_svc" ] == 0 } {
         add_proc_error "shutdown_bdb_rpc" "-3" "could not shutdown berkeley_db_svc at host $elem with term signal"
         puts $CHECK_OUTPUT "Killing process with kill signal [ set ps_info(pid,$elem) ] ..."
         if { [ have_root_passwd ] == -1 } {
            set_root_passwd 
         }
         if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
             start_remote_prog "$hostname" "root" "kill" "-9 $ps_info(pid,$elem)"
         } else {
             start_remote_prog "$hostname" "$CHECK_USER" "kill" "-9 $ps_info(pid,$elem)"
         }
      } 
   }

   foreach elem $new_index {
      debug_puts $ps_info(string,$elem)
      if { [ is_pid_with_name_existing $hostname $ps_info(pid,$elem) "berkeley_db_svc" ] == 0 } {
         add_proc_error "shutdown_bdb_rpc" "-1" "could not shutdown berkeley_db_svc at host $elem with kill signal"
      }
   }

   return $num_proc
}


#
#                                                             max. column:     |
#
#****** sge_procedures/is_pid_with_name_existing() ******
#  NAME
#     is_pid_with_name_existing -- search for process on remote host 
#
#  SYNOPSIS
#     is_pid_with_name_existing { host pid proc_name } 
#
#  FUNCTION
#     This procedure will start the checkprog binary with the given parameters. 
#     
#
#  INPUTS
#     host      - remote host 
#     pid       - pid of process 
#     proc_name - process program name 
#
#  RESULT
#     0 - ok; != 0 on error 
#
#  SEE ALSO
#     sge_procedures/is_job_running()
#     sge_procedures/is_pid_with_name_existing()
#*******************************
#
proc is_pid_with_name_existing { host pid proc_name } {
   global ts_config
  global CHECK_OUTPUT CHECK_ARCH CHECK_USER

  puts -nonewline $CHECK_OUTPUT "$host: checkprog $pid $proc_name ... "
  set my_arch [ resolve_arch $host ]
  set output [start_remote_prog $host $CHECK_USER $ts_config(product_root)/utilbin/$my_arch/checkprog "$pid $proc_name"]
  if { $prg_exit_state == 0} {
     puts $CHECK_OUTPUT "running"
  } else {
     puts $CHECK_OUTPUT "not running"
  }
  return $prg_exit_state
}


#
#                                                             max. column:     |
#
#****** sge_procedures/shutdown_system_daemon() ******
#  NAME
#     shutdown_system_daemon -- kill running sge daemon 
#
#  SYNOPSIS
#     shutdown_system_daemon { host type } 
#
#  FUNCTION
#     This procedure will kill all commd, execd, qmaster, shadowd or sched 
#     processes on the given host. 
#     It does not matter weather the system is sgeee or sge (sge or sgeee). 
#
#  INPUTS
#     host     - remote host 
#     typelist - list of processes to kill (commd, execd, qmaster, shadowd or sched)
#     { do_term_signal_kill_first 1 } - if set to 1 the first kill signal is
#                                       SIG_TERM and SIG_KILL only if SIG_TERM
#                                       wasn't successful
#                                     - if 0 the procedure sends immediately a
#                                       SIG_KILL signal.
#
#  RESULT
#     none 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     sge_procedures/shutdown_core_system()
#     sge_procedures/shutdown_master_and_scheduler()
#     sge_procedures/shutdown_all_shadowd()
#     sge_procedures/shutdown_system_daemon()
#     sge_procedures/startup_qmaster()
#     sge_procedures/startup_execd()
#     sge_procedures/startup_shadowd()
#*******************************
#
proc shutdown_system_daemon { host typelist { do_term_signal_kill_first 1 } } {
   global ts_config
   global CHECK_ARCH 
   global CHECK_CORE_MASTER CHECK_CORE_INSTALLED CHECK_OUTPUT CHECK_USER 
   global CHECK_ADMIN_USER_SYSTEM


   puts $CHECK_OUTPUT "shutdown_system_daemon ... ($host/$typelist)" 
   set process_names ""
   foreach type $typelist {
      if { [ string compare $type "execd" ] == 0 } {
         lappend process_names "sge_execd" 
      }
      if { [ string compare $type "sched" ] == 0 } {
         lappend process_names "sge_schedd" 
      }
      if { [ string compare $type "qmaster" ] == 0 } {
         lappend process_names "sge_qmaster" 
      }
      if { [ string compare $type "commd" ] == 0 } {
         lappend process_names "sge_commd" 
      }
      if { [ string compare $type "shadowd" ] == 0 } {
         lappend process_names "sge_shadowd" 
      }
   }

   if { [llength $process_names] != [llength $typelist] } {
      add_proc_error "shutdown_system_daemon" -1 "type should be commd, execd, qmaster, shadowd or sched"
      puts $CHECK_OUTPUT "shutdown_system_daemon ... done"
      return -1
   }

   set found_p [ ps_grep "$ts_config(product_root)/" $host ]
   set nr_of_found_qmaster_processes_or_threads 0

   foreach process_name $process_names {

      puts $CHECK_OUTPUT "looking for \"$process_name\" processes on host $host ..."
      foreach elem $found_p {
         if { [ string first $process_name $ps_info(string,$elem) ] >= 0 } {
            debug_puts "current ps info: $ps_info(string,$elem)"
            if { [ is_pid_with_name_existing $host $ps_info(pid,$elem) $process_name ] == 0 } {
               incr nr_of_found_qmaster_processes_or_threads 1
               puts $CHECK_OUTPUT "found running $process_name with pid $ps_info(pid,$elem) on host $host"
               debug_puts $ps_info(string,$elem)
               if { [ have_root_passwd ] == -1 } {
                   set_root_passwd 
               }
               if { $CHECK_ADMIN_USER_SYSTEM == 0 } {
                   set kill_user "root"  
               } else {
                   set kill_user $CHECK_USER
               }
               if { $do_term_signal_kill_first == 1 } {
                  set kill_pid_ids ""
                  foreach tmp_elem $found_p {
                     if { [ string first $process_name $ps_info(string,$tmp_elem) ] >= 0 } {
                        append kill_pid_ids " $ps_info(pid,$tmp_elem)"
                     }
                  }
                  puts $CHECK_OUTPUT "killing (SIG_TERM) process(es) $kill_pid_ids on host $host, kill user is $kill_user"
                  puts $CHECK_OUTPUT [ start_remote_prog $host $kill_user kill $kill_pid_ids ]

                  set sig_term_wait_timeout 30
                  while { [is_pid_with_name_existing $host $ps_info(pid,$elem) $process_name] == 0 } {
                     puts $CHECK_OUTPUT "waiting for process termination ..." 
                     after 1000
                     incr sig_term_wait_timeout -1
                     if { $sig_term_wait_timeout <= 0 } {
                        break
                     }
                  }
               }
               if { [ is_pid_with_name_existing $host $ps_info(pid,$elem) $process_name ] == 0 } {
                   puts $CHECK_OUTPUT "killing (SIG_KILL) process $ps_info(pid,$elem) on host $host, kill user is $kill_user"
                   puts $CHECK_OUTPUT [ start_remote_prog $host $kill_user kill "-9 $ps_info(pid,$elem)" ]
                   after 1000
                   if { [ is_pid_with_name_existing $host $ps_info(pid,$elem) $process_name ] == 0 } {
                       puts $CHECK_OUTPUT "pid:$ps_info(pid,$elem) kill failed (host: $host)"
                       add_proc_error "" -1 "could not shutdown \"$process_name\" on host $host"
                   } else {
                       puts $CHECK_OUTPUT "pid:$ps_info(pid,$elem) process killed (host: $host)"
                   }
               }
            } else {
               if { $nr_of_found_qmaster_processes_or_threads == 0 } {
                  puts $CHECK_OUTPUT "checkprog error"
                  add_proc_error "" -3 "could not shutdown \"$process_name\" on host $host"
               }
            }
         }
      }
   }
   puts $CHECK_OUTPUT "shutdown_system_daemon ... done"
   return 0
}

#                                                             max. column:     |
#****** sge_procedures/shutdown_core_system() ******
# 
#  NAME
#     shutdown_core_system -- shutdown complete cluster
#
#  SYNOPSIS
#     shutdown_core_system { } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     sge_procedures/shutdown_core_system()
#     sge_procedures/startup_core_system()
#     sge_procedures/shutdown_master_and_scheduler()
#     sge_procedures/shutdown_all_shadowd()
#     sge_procedures/shutdown_system_daemon()
#     sge_procedures/startup_qmaster()
#     sge_procedures/startup_execd()
#     sge_procedures/startup_shadowd()
#*******************************
proc shutdown_core_system { { only_hooks 0 } } {
   global ts_config
   global CHECK_ARCH 
   global CHECK_CORE_MASTER 
   global CHECK_OUTPUT
   global CHECK_USER
   global CHECK_ADMIN_USER_SYSTEM do_compile

   exec_shutdown_hooks

   if { $only_hooks != 0 } {
      puts $CHECK_OUTPUT "skip shutdown core system, I am in only hooks mode"
      return
   }
   
   foreach sh_host $ts_config(shadowd_hosts) {
      shutdown_all_shadowd $sh_host
   }
   puts $CHECK_OUTPUT "killing scheduler and all execds in the cluster ..."

   set result ""
   set do_ps_kill 0
   set result [ start_remote_prog "$CHECK_CORE_MASTER" "$CHECK_USER" "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-ks -ke all" ]

   puts $CHECK_OUTPUT "qconf -ke all -ks returned $prg_exit_state"
   if { $prg_exit_state == 0 } {
      puts $CHECK_OUTPUT $result
   } else {
      set do_ps_kill 1
      puts $CHECK_OUTPUT "shutdown_core_system - qconf error or binary not found\n$result"
   }

#   sleep 15 ;# give the schedd and execd's some time to shutdown
   wait_for_unknown_load 120 all.q 0


   puts $CHECK_OUTPUT "killing qmaster ..."

   set result ""
   set do_ps_kill 0
   set result [ start_remote_prog "$CHECK_CORE_MASTER" "$CHECK_USER" "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-km" ]

   puts $CHECK_OUTPUT "qconf -km returned $prg_exit_state"
   if { $prg_exit_state == 0 } {
      puts $CHECK_OUTPUT $result
   } else {
      set do_ps_kill 1
      puts $CHECK_OUTPUT "shutdown_core_system - qconf error or binary not found\n$result"
   }

   if { [wait_till_qmaster_is_down $CHECK_CORE_MASTER] != 0 } {
      shutdown_system_daemon $CHECK_CORE_MASTER "qmaster"
   }

   if { $ts_config(gridengine_version) == 53 } {
      puts $CHECK_OUTPUT "killing all commds in the cluster ..." 
     
      set do_it_as_root 0
      foreach elem $ts_config(execd_nodes) { 
          puts $CHECK_OUTPUT "killing commd on host $elem"
          if { $do_it_as_root == 0 } { 
             set result [ start_remote_prog "$CHECK_CORE_MASTER" "$CHECK_USER" "$ts_config(product_root)/bin/$CHECK_ARCH/sgecommdcntl" "-U -k -host $elem"  ]
          } else {
             set result [ start_remote_prog "$CHECK_CORE_MASTER" "root" "$ts_config(product_root)/bin/$CHECK_ARCH/sgecommdcntl" "-k -host $elem"  ]
          } 
          if { $prg_exit_state == 0 } {
             puts $CHECK_OUTPUT $result
          } else {
             puts $CHECK_OUTPUT $result
             if { $prg_exit_state == 255 } {
                puts $CHECK_OUTPUT "\"sgecommdcntl -k\" must be started by root user (to get reserved port)!"
                puts $CHECK_OUTPUT "try again as root user ..." 
                if { [ have_root_passwd ] == -1 } {
                   set_root_passwd 
                }
                if { $CHECK_ADMIN_USER_SYSTEM != 1 } {
                   set result [ start_remote_prog "$CHECK_CORE_MASTER" "root" "$ts_config(product_root)/bin/$CHECK_ARCH/sgecommdcntl" "-k -host $elem"  ]
                }
             }
             if { $prg_exit_state == 0 } {
                set do_it_as_root 1 
                puts $CHECK_OUTPUT $result
                puts $CHECK_OUTPUT "sgecommdcntl -k -host $elem - success"
             } else {
                set do_ps_kill 1
                puts $CHECK_OUTPUT "shutdown_core_system - commdcntl error or binary not found"
             }
          }
      }
   } else {
      shutdown_system_daemon $CHECK_CORE_MASTER "sched execd qmaster"
   }

   if { $do_compile == 0} {
      if { $do_ps_kill == 1 } {
         puts $CHECK_OUTPUT "perhaps master is not running, trying to shutdown cluster with ps information"
      }
      set hosts_to_check $CHECK_CORE_MASTER
      foreach elem $ts_config(execd_nodes) {
         if { [ string first $elem $hosts_to_check ] < 0 } {
            lappend hosts_to_check $elem
         }
      }
      set proccess_names "sched"
      lappend proccess_names "execd"
      lappend proccess_names "qmaster"
      if {$ts_config(gridengine_version) == 53} {
         lappend proccess_names "commd"
      }

      foreach host $hosts_to_check { 
            puts $CHECK_OUTPUT ""
            shutdown_system_daemon $host $proccess_names
      }
   }

   if {$ts_config(bdb_server) != "none"} {
      foreach rpc_host $ts_config(bdb_server) {
         shutdown_bdb_rpc $rpc_host
      }
   }

   # check for core files
   # core file in qmaster spool directory
   set spooldir [get_spool_dir $ts_config(master_host) qmaster]
   check_for_core_files $ts_config(master_host) $spooldir

   # core files in execd spool directories
   foreach host $ts_config(execd_nodes) { 
      set spooldir [get_spool_dir $host execd] 
      check_for_core_files $host "$spooldir"
   }
}

#****** sge_procedures/startup_core_system() ***********************************
#  NAME
#     startup_core_system() -- startup complete cluster
#
#  SYNOPSIS
#     startup_core_system { } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#
#  RESULT
#     ??? 
#
#  SEE ALSO
#      sge_procedures/shutdown_core_system()
#*******************************************************************************
proc startup_core_system {} {
   global ts_config
   global CHECK_ARCH 
   global CHECK_CORE_MASTER 
   global CHECK_OUTPUT
   global CHECK_USER
   global CHECK_ADMIN_USER_SYSTEM do_compile


   if { [ have_root_passwd ] == -1 } {
      set_root_passwd 
   }

   # startup of BDB RPC service
   startup_bdb_rpc $ts_config(bdb_server)

   # startup of schedd and qmaster 
   startup_qmaster

   
   # startup all shadowds
   # 
   foreach sh_host $ts_config(shadowd_hosts) {
      startup_shadowd $sh_host
   }

   # startup of all execd
   foreach ex_host $ts_config(execd_nodes) {
      startup_execd $ex_host
   }
   
   # now execute all startup hooks
   exec_startup_hooks

}



#                                                             max. column:     |
#****** sge_procedures/add_operator() ******
# 
#  NAME
#     add_operator
#
#  SYNOPSIS
#     add_operator { anOperator } 
#
#  FUNCTION
#     Add user ''anOperator'' to operator list.
#
#  INPUTS
#     anOperator - Operator to add
#
#  RESULT
#     0 - Operator has been successfully added
#    -1 - Otherwise 
#
#  SEE ALSO
#     sge_procedures/delete_operator
#
#*******************************
#
proc add_operator { anOperator } {
   global ts_config
   global CHECK_OUTPUT CHECK_ARCH CHECK_CORE_MASTER

   catch {eval exec $ts_config(product_root)/bin/$CHECK_ARCH/qconf "-ao $anOperator" } result
   set result [string trim $result]

   set ADDEDTOLIST   [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] "*" "*" $anOperator "*" ]
   set ALREADYEXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" $anOperator ]

   if {[string match $ADDEDTOLIST $result]} {
      puts $CHECK_OUTPUT "added $anOperator to operator list"
      return 0
   } elseif {[string match $ALREADYEXISTS $result]} {
      puts $CHECK_OUTPUT "operator $anOperator already exists"
      return 0
   } else {
      return -1
   }
}


proc wait_till_qmaster_is_down { host } {
   global ts_config CHECK_OUTPUT

   set process_names "sge_qmaster" 
   
   set my_timeout [ expr ( [timestamp] + 60 ) ] 

   while { 1 } {
      set found_p [ ps_grep "$ts_config(product_root)/" $host ]
      set nr_of_found_qmaster_processes_or_threads 0

      foreach process_name $process_names {
         puts $CHECK_OUTPUT "looking for \"$process_name\" processes on host $host ..."
         foreach elem $found_p {
            if { [ string first $process_name $ps_info(string,$elem) ] >= 0 } {
               debug_puts "current ps info: $ps_info(string,$elem)"
               if { [ is_pid_with_name_existing $host $ps_info(pid,$elem) $process_name ] == 0 } {
                  incr nr_of_found_qmaster_processes_or_threads 1
                  puts $CHECK_OUTPUT "found running $process_name with pid $ps_info(pid,$elem) on host $host"
                  debug_puts $ps_info(string,$elem)
               }
            }
         }
      }
      if { [timestamp] > $my_timeout } {
         add_proc_error "wait_till_qmaster_is_down" -3 "timeout while waiting for qmaster going down"
         return -1
      }
      if { $nr_of_found_qmaster_processes_or_threads == 0 } {
         puts $CHECK_OUTPUT "no qmaster processes running"
         return 0      
      } else {
         puts $CHECK_OUTPUT "still qmaster processes running ..."
      }
      after 1000
   }
}

#                                                             max. column:     |
#****** sge_procedures/delete_operator() ******
# 
#  NAME
#     delete_operator
#
#  SYNOPSIS
#     delete_operator { anOperator } 
#
#  FUNCTION
#     Delete user ''anOperator'' from operator list.
#
#  INPUTS
#     anOperator - Operator to delete
#
#  RESULT
#     0 - Operator has been successfully deleted
#    -1 - Otherwise 
#
#  SEE ALSO
#     sge_procedures/add_operator
#
#*******************************
#
proc delete_operator {anOperator} {
   global ts_config
   global CHECK_OUTPUT CHECK_ARCH CHECK_CORE_MASTER

   catch {eval exec $ts_config(product_root)/bin/$CHECK_ARCH/qconf "-do $anOperator" } result
   set result [string trim $result]

   set REMOVEDFROMLIST [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] "*" "*" $anOperator "*" ]
   set DOESNOTEXIST [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_DOESNOTEXIST_SS] "*" $anOperator ]

   if {[string match $REMOVEDFROMLIST $result]} {
      puts $CHECK_OUTPUT "removed $anOperator from operator list"
      return 0
   } elseif {[string match $DOESNOTEXIST $result]} {
      puts $CHECK_OUTPUT "operator $anOperator does not exists"
      return 0
   } else {
      return -1
   }
}

#                                                             max. column:     |
#****** sge_procedures/submit_with_method() ******
# 
#  NAME
#     submit_with_method
#
#  SYNOPSIS
#     submit_with_method {submit_method options script args}
#
#  FUNCTION
#     Submit a job using different submit methods, e.g. qsub, qrsh, qsh, 
#     qrlogin (qrsh without command), qlogin
#
#     The job output is available via the sid returned.
#
#  INPUTS
#     submit_method - method to use: "qsub" or "qrsh"
#     options       - options to the submit command
#     script        - script to start
#     args          - arguments to the script
#     tail_host     - host on which a tail to the output file will be done.
#                     This may be important, it should be the host where the job, or
#                     the master task of the job is run to avoid NFS latencies.
#
#  RESULT
#     a session id from open_spawn_process, or an empty string ("") on error
#
#  NOTES
#     Only qsub and qrsh are implemented.
#
#*******************************
#
proc submit_with_method {submit_method options script args tail_host} {
   global ts_config
   global CHECK_OUTPUT CHECK_HOST CHECK_USER 
   global CHECK_PROTOCOL_DIR
  
   # preprocessing args - it is treated as list for some reason - options not.
   set job_args [lindex $args 0]
   foreach arg [lrange $args 1 end] {
      append job_args " $arg"
   }
  
   switch -exact $submit_method {
      qsub {
         puts $CHECK_OUTPUT "submitting job using qsub, reading from job output file"
         # create job output file
         set job_output_file "$CHECK_PROTOCOL_DIR/check.out"
         catch {exec touch $job_output_file} output
         # initialize tail to logfile
         set sid [init_logfile_wait $tail_host $job_output_file]
         # submit job
         submit_job "-o $job_output_file -j y $options $script $job_args" 1 60
      }

      qrsh {
         puts $CHECK_OUTPUT "submitting job using qrsh, reading from stdout/stderr"
#         set command "-c \\\"$ts_config(product_root)/bin/[resolve_arch $CHECK_HOST]/qrsh -noshell $options $script $job_args\\\""
         set command "$ts_config(product_root)/bin/[resolve_arch $CHECK_HOST]/qrsh"
         set cmd_args "-noshell $options $script $job_args"
#set command ls
#set cmd_args "-la"
         set sid [open_remote_spawn_process $CHECK_HOST $CHECK_USER "$command" "$cmd_args"]
         set sp_id [lindex $sid 1]
         set done 0
         while {!$done} {
            set timeout 60
            expect {
               -i $sp_id full_buffer {
                  add_proc_error "submit_with_method" -1 "expect full_buffer error"
                  set done 1
               }
               -i $sp_id timeout {
                  add_proc_error "submit_with_method" -1 "timeout"
                  set done 1
               }
               -i $sp_id eof {
                  add_proc_error "submit_with_method" -1 "got eof"
                  set done 1
               }
               -i $sp_id "_start_mark_*\n" {
                  puts $CHECK_OUTPUT "remote command started"
                  set done 1
               }
               -i $sp_id default {
                  
               }
            }
         }
      }

      default {
         set sid ""
         add_proc_error "submit_with_method" -1 "unknown submit method $submit_method"
      }
   }

   puts $CHECK_OUTPUT "submitted job, sid = $sid"
   return $sid
}

# main
if { [info exists argc ] != 0 } {
   set TS_ROOT ""
   set procedure ""
   for { set i 0 } { $i < $argc } { incr i } {
      if {$i == 0} { set TS_ROOT [lindex $argv $i] }
      if {$i == 1} { set procedure [lindex $argv $i] }
   }
   if { $argc == 0 } {
      puts "usage:\n$module_name <CHECK_TESTSUITE_ROOT> <proc> no_main <testsuite params>"
      puts "options:"
      puts "CHECK_TESTSUITE_ROOT -  path to TESTSUITE directory"
      puts "proc                 -  procedure from this file with parameters"
      puts "no_main              -  used to source testsuite file (check.exp)"
      puts "testsuite params     -  any testsuite command option (from file check.exp)"
      puts "                        testsuite params: file <path>/defaults.sav is needed"
   } else {
      #source "$TS_ROOT/check.exp"
      puts $CHECK_OUTPUT "master host is $CHECK_CORE_MASTER"
      puts $CHECK_OUTPUT "calling \"$procedure\" ..."
      set result [ eval $procedure ]
      puts $result 
      flush $CHECK_OUTPUT
   }
}

#****** sge_procedures/copy_certificates() **********************************
#  NAME
#     copy_certificates() -- copy csp (ssl) certificates to the specified host
#
#  SYNOPSIS
#     copy_certificates { host } 
#
#  FUNCTION
#     copy csp (ssl) certificates to the specified host
#
#  INPUTS
#     host - host where the certificates has to be copied. (Master installation
#            must be called before)
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc copy_certificates { host } {
   global ts_config ts_user_config
   global CHECK_OUTPUT CHECK_ADMIN_USER_SYSTEM CHECK_USER

   set remote_arch [resolve_arch $host]
   
   puts $CHECK_OUTPUT "installing CA keys"
   puts $CHECK_OUTPUT "=================="
   puts $CHECK_OUTPUT "host:         $host"
   puts $CHECK_OUTPUT "architecture: $remote_arch"
   puts $CHECK_OUTPUT "port:         $ts_config(commd_port)"
   puts $CHECK_OUTPUT "source:       \"/var/sgeCA/port${ts_config(commd_port)}/\" on host $ts_config(master_host)"
   puts $CHECK_OUTPUT "target:       \"/var/sgeCA/port${ts_config(commd_port)}/\" on host $host"
  
   if { $CHECK_ADMIN_USER_SYSTEM == 0 } {
      puts $CHECK_OUTPUT "we have root access, fine !"
      set CA_ROOT_DIR "/var/sgeCA"
      set TAR_FILE "${CA_ROOT_DIR}/port${ts_config(commd_port)}.tar"
      if {$remote_arch == "win32-x86"} {
         set UNTAR_OPTS "-xvf"
      } else {
         set UNTAR_OPTS "-xpvf"
      }

      puts $CHECK_OUTPUT "removing existing tar file \"$TAR_FILE\" ..."
      set result [ start_remote_prog "$ts_config(master_host)" "root" "rm" "$TAR_FILE" ]
      puts $CHECK_OUTPUT $result

      puts $CHECK_OUTPUT "taring Certificate Authority (CA) directory into \"$TAR_FILE\""
      set tar_bin [get_binary_path $ts_config(master_host) "tar"]
      set remote_command_param "$CA_ROOT_DIR; ${tar_bin} -cpvf $TAR_FILE ./port${ts_config(commd_port)}/*"
      set result [ start_remote_prog "$ts_config(master_host)" "root" "cd" "$remote_command_param" ]
      puts $CHECK_OUTPUT $result

      if { $prg_exit_state != 0 } {
         add_proc_error "copy_certificates" -2 "could not tar Certificate Authority (CA) directory into \"$TAR_FILE\""
      } else {
         puts $CHECK_OUTPUT "copy tar file \"$TAR_FILE\"\nto \"$ts_config(results_dir)/port${ts_config(commd_port)}.tar\" ..."
         set result [ start_remote_prog "$ts_config(master_host)" "$CHECK_USER" "cp" "$TAR_FILE $ts_config(results_dir)/port${ts_config(commd_port)}.tar" prg_exit_state 300 ]
         puts $CHECK_OUTPUT $result
                    
         # tar file will be on nfs - wait for it to be visible
         wait_for_remote_file $host "root" "$ts_config(results_dir)/port${ts_config(commd_port)}.tar"
                    
         puts $CHECK_OUTPUT "copy tar file \"$ts_config(results_dir)/port${ts_config(commd_port)}.tar\"\nto \"$TAR_FILE\" on host $host as root user ..."
         set result [ start_remote_prog "$host" "root" "cp" "$ts_config(results_dir)/port${ts_config(commd_port)}.tar $TAR_FILE" prg_exit_state 300 ]
         puts $CHECK_OUTPUT $result

         set tar_bin [get_binary_path $host "tar"]

         puts $CHECK_OUTPUT "untaring Certificate Authority (CA) directory in \"$CA_ROOT_DIR\""
         start_remote_prog "$host" "root" "cd" "$CA_ROOT_DIR" 
         if { $prg_exit_state != 0 } { 
            set result [ start_remote_prog "$host" "root" "mkdir" "-p $CA_ROOT_DIR" ]
         }   

         set result [ start_remote_prog "$host" "root" "cd" "$CA_ROOT_DIR; ${tar_bin} $UNTAR_OPTS $TAR_FILE" prg_exit_state 300 ]
         puts $CHECK_OUTPUT $result
         if { $prg_exit_state != 0 } {
            add_proc_error "copy_certificates" -2 "could not untar \"$TAR_FILE\" on host $host;\ntar-bin:$tar_bin"
         } 

         puts $CHECK_OUTPUT "removing tar file \"$TAR_FILE\" on host $host ..."
         set result [ start_remote_prog "$host" "root" "rm" "$TAR_FILE" ]
         puts $CHECK_OUTPUT $result

         puts $CHECK_OUTPUT "removing tar file \"$ts_config(results_dir)/port${ts_config(commd_port)}.tar\" ..."
         set result [ start_remote_prog "$ts_config(master_host)" "$CHECK_USER" "rm" "$ts_config(results_dir)/port${ts_config(commd_port)}.tar" ]
         puts $CHECK_OUTPUT $result
      }
                
      puts $CHECK_OUTPUT "removing tar file \"$TAR_FILE\" ..."
      set result [ start_remote_prog "$ts_config(master_host)" "root" "rm" "$TAR_FILE" ]
      puts $CHECK_OUTPUT $result

      # on windows, we have to correct the file permissions
      if {$remote_arch == "win32-x86"} {
         puts $CHECK_OUTPUT "correcting certificate file permissions on windows host"
         set users "$CHECK_USER $ts_user_config(first_foreign_user) $ts_user_config(second_foreign_user)"
         foreach user $users {
            start_remote_prog $host "root" "chown" "-R $user /var/sgeCA/port${ts_config(commd_port)}/default/userkeys/$user"
         }
      }

      # check for syncron clock times
      set my_timeout [timestamp]
      incr my_timeout 600
      while { 1 } {
         set result [start_remote_prog $host $CHECK_USER "$ts_config(product_root)/bin/$remote_arch/qstat" "-f"]
         puts $CHECK_OUTPUT $result
         if { $prg_exit_state == 0 } {
            puts $CHECK_OUTPUT "qstat -f works, fine!"
            break
         }
         puts $CHECK_OUTPUT "waiting for qstat -f to work ..."
         puts $CHECK_OUTPUT "please check hosts for synchron clock times"
         after 10000
         if { [timestamp] > $my_timeout } {
            add_proc_error "copy_certificates" -2 "$host: timeout while waiting for qstat to work (please check hosts for synchron clock times)"
         }
      }
   } else {
      puts $CHECK_OUTPUT "can not copy this files as user $CHECK_USER"
      puts $CHECK_OUTPUT "installation error"
      add_proc_error "copy_certificates" -2 "$host: can't copy certificate files as user $CHECK_USER"
   }
}


#                                                             max. column:     |
#****** sge_procedures/is_daemon_running() ******
# 
#  NAME
#     is_daemon_running 
#
#  SYNOPSIS
#     is_daemon_running { hostname daemon } 
#
#  FUNCTION
#     Checks, if a daemon is running of the given host.
#     This function does a ps_grep, which seeks for the given
#     daemon name running within the actual SGE_ROOT directory.
#     The daemon can be clearly identified. 
#
#  INPUTS
#     hostname  - name of host which should be checked
#     daemon    - name of daemon (sge_execd, sge_qmaster, ...) 
#
#  RESULT
#     0 - the given daemon is not running on given host 
#     Otherwise the number of running daemons is returned 
#
#  SEE ALSO
#     sge_procedures/is_daemon_running
#
#*******************************
#

proc is_daemon_running { hostname daemon } {
   global ts_config CHECK_OUTPUT

   set found_p [ ps_grep $ts_config(product_root) $hostname ]
   set execd_count 0

   foreach elem $found_p {
      if { [string match "*$daemon*" $ps_info(string,$elem)] } {
         debug_puts $ps_info(string,$elem)
         puts $CHECK_OUTPUT "$daemon is running on host $hostname"
         incr execd_count 1
      }
   }
   if { $execd_count > 1 } {
      add_proc_error "is_daemon_running" -1 "Host: $hostname -> Found 2 running $daemon in one environment!" 

   }
   return $execd_count
}

#****** sge_procedures/restore_qtask_file() ************************************
#  NAME
#     restore_qtask_file() -- restore qtask file from template
#
#  SYNOPSIS
#     restore_qtask_file { } 
#
#  FUNCTION
#     Copies $SGE_ROOT/util/qtask to $SGE_ROOT/$SGE_CELL/common/qtask
#
#  RESULT
#     1 on success, else 0
#
#  SEE ALSO
#     sge_procedures/append_to_qtask_file()
#*******************************************************************************
proc restore_qtask_file {} {
   global ts_config CHECK_OUTPUT CHECK_USER

   set ret 1

   # restore the qtask file from util/qtask
   puts $CHECK_OUTPUT "restoring qtask file from template util/qtask"
   set qtask_file "$ts_config(product_root)/$ts_config(cell)/common/qtask"
   set qtask_template "$ts_config(product_root)/util/qtask"
   set output [start_remote_prog $ts_config(master_host) $CHECK_USER "cp" "$qtask_template $qtask_file"]
   if {$prg_exit_state != 0} {
      add_proc_error "restore_qtask_file" -1 "error restoring qtask file:\n$output"
      set ret 0
   }
   foreach node $ts_config(execd_nodes) {
      wait_for_remote_file $node $CHECK_USER $qtask_file
   }

   return $ret
}

#****** sge_procedures/append_to_qtask_file() **********************************
#  NAME
#     append_to_qtask_file() -- append line(s) to qtask file
#
#  SYNOPSIS
#     append_to_qtask_file { content } 
#
#  FUNCTION
#     Appends lines given as argument to the global qtask file.
#
#  INPUTS
#     content - lines to be appended
#
#  RESULT
#     1 on success, else 0
#
#  EXAMPLE
#     append_to_qtask_file "mozilla -l h=myhost"
#
#  SEE ALSO
#     sge_procedures/restore_qtask_file()
#*******************************************************************************
proc append_to_qtask_file {content} {
   global ts_config CHECK_OUTPUT

   set ret 1

   set qtask_file "$ts_config(product_root)/$ts_config(cell)/common/qtask"

   # make sure we have a qtask file
   if {[file isfile $qtask_file] == 0} {
      set ret [restore_qtask_file]
   }

   if {$ret} {
      set error [catch {
         set f [open $qtask_file "a"]
         puts $f $content
         close $f
      } output]   

      if {$error != 0} {
         add_proc_error "append_to_qtask_file" -1 "error appending to qtask file:\n$output"
         set ret 0
      }
   }

   return $ret
}

#****** sge_procedures/get_shared_lib_var() ************************************
#  NAME
#     get_shared_lib_var() -- get the env var used for the shared lib path
#
#  SYNOPSIS
#     get_shared_lib_var {hostname}
#
#  FUNCTION
#     Returns the name of the variable that holds the shared library path on
#     the given host.
#
#  INPUTS
#     hostname  The name of the host whose shared lib var will be fetched.
#               Defaults to $CHECK_HOST.  Returns "" on failure.
#
#  RESULT
#     The name of the shared library path variable
#
#  EXAMPLE
#     set shlib [get_shared_lib_path foo]
#*******************************************************************************
proc get_shared_lib_var {{hostname ""}} {
   global ts_config CHECK_HOST CHECK_USER

   set shlib_var ""
   set host $hostname

   if {$host == ""} {
      set host $ts_config(master_host)
   }

   set shlib_var [string trim [start_remote_prog $host $CHECK_USER "$ts_config(product_root)/util/arch" "-lib"]]

   return $shlib_var
}


#****** sge_procedures/get_qconf_list() ****************************************
#  NAME
#     get_qconf_list() -- return a list from qconf -s* command
#
#  SYNOPSIS
#     get_qconf_list { procedure option output_var {on_host ""} {as_user ""} 
#     {raise_error 1} } 
#
#  FUNCTION
#     Calls qconf with the give option and returns the results as a list by
#     splitting multiple lines into list elements.
#
#     The function can be used to call the qconf show list options, e.g.
#     -sh, -ss -sel, -sm, -so, -suserl, ...
#
#     It will usually be called by a wrapper function, e.g. get_adminhost_list().
#
#  INPUTS
#     procedure       - calling procedure
#     option          - qconf option to call
#     output_var      - output list will be placed here (call by reference)
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
#     sge_host/get_adminhost_list()
#*******************************************************************************
proc get_qconf_list {procedure option output_var {on_host ""} {as_user ""} {raise_error 1}} {
   global ts_config CHECK_OUTPUT
   upvar $output_var out

   # clear output variable
   if {[info exists out]} {
      unset out
   }

   set ret 0
   set result [start_sge_bin "qconf" $option $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      parse_multiline_list result out
   } else {
      set ret [get_sge_error $procedure "qconf $option" $result $raise_error]
   }

   return $ret
}

#****** sge_procedures/get_scheduler_status() *****************************************
#  NAME
#    get_scheduler_status () -- get the scheduler status 
#
#  SYNOPSIS
#     get_scheduler_status { {output_var result} {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -sss to retrieve the scheduler status 
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
proc get_scheduler_status {{output_var result} {on_host ""} {as_user ""} {raise_error 1}} {
   upvar $output_var out

   return [get_qconf_list "get_scheduler_status" "-sss" out $on_host $as_user $raise_error]

}

#****** sge_procedures/get_detached_settings() *****************************************
#  NAME
#    get_detached_settings () -- get the detached settings in the cluster  config 
#
#  SYNOPSIS
#     get_detached_settings { {output_var result} {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -sds to retrieve the detached settings in the cluster  config
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
proc get_detached_settings {{output_var result} {on_host ""} {as_user ""} {raise_error 1}} {
   upvar $output_var out

   return [get_qconf_list "get_detached_settings" "-sds" out $on_host $as_user $raise_error]

}

#****** sge_procedures/get_event_client_list() *****************************************
#  NAME
#     get_event_client_list() -- get the event client list
#
#  SYNOPSIS
#     get_event_client_list { {output_var result} {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -secl to retrieve the event client list
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
proc get_event_client_list {{output_var result} {on_host ""} {as_user ""} {raise_error 1}} {
   upvar $output_var out

   return [get_qconf_list "get_event_client_list" "-secl" out $on_host $as_user $raise_error]

}

#****** sge_procedures/trigger_scheduling() ************************************
#  NAME
#     trigger_scheduling() -- trigger a scheduler run
#
#  SYNOPSIS
#     trigger_scheduling { } 
#
#  FUNCTION
#     Triggers a scheduler run by calling qconf -tsm.
#*******************************************************************************
proc trigger_scheduling {} {
   global CHECK_OUTPUT

   puts $CHECK_OUTPUT "triggering scheduler run"

   set output [start_sge_bin "qconf" "-tsm"]
   if {$prg_exit_state != 0} {
      add_proc_error "trigger_scheduling" -1 "qconf -tsm failed:\n$output"
   }
}

#****** sge_procedures/wait_for_job_end() **************************************
#  NAME
#     wait_for_job_end() -- waits for a job to leave qmaster
#
#  SYNOPSIS
#     wait_for_job_end { job_id {timeout 60} } 
#
#  FUNCTION
#     Waits until a job is no longer referenced in qmaster (after sge_schedd
#     has sent a job delete order to sge_qmaster).
#
#  INPUTS
#     job_id       - job id to wait for
#     {timeout 60} - how long to wait
#
#  SEE ALSO
#     sge_procedures/get_qstat_j_info()
#*******************************************************************************
proc wait_for_job_end {job_id {timeout 60}} {
   global ts_config CHECK_OUTPUT

   # we wait until now + timeout
   set my_timeout [expr [timestamp] + $timeout]

   # if the job is still in qmaster, wait until it leaves qmaster
   if {[get_qstat_j_info $job_id] != 0} {
      puts -nonewline $CHECK_OUTPUT "waiting for job $job_id to leave qmaster ..."
      flush $CHECK_OUTPUT

      sleep 1
      while {[get_qstat_j_info $job_id] != 0} {
         puts -nonewline $CHECK_OUTPUT "."
         flush $CHECK_OUTPUT
         if {[timestamp] > $my_timeout} {
            add_proc_error "delete_job" -1 "timeout while waiting for job $job_id leave qmaster"
            break
         }

         sleep 1
      }
      puts $CHECK_OUTPUT ""
   }
}
