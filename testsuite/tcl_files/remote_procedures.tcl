#!/vol2/TCL_TK/glinux/bin/tclsh
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
global rlogin_spawn_session_buffer
set module_name "remote_procedures.tcl"
global rlogin_max_open_connections
set rlogin_max_open_connections 20
global last_shell_script_file last_spawn_command_arguments

global CHECK_SHELL_PROMPT
# initialize prompt handling, see expect manpage
#set CHECK_SHELL_PROMPT "(%|#|\\$|>) $"
#set CHECK_SHELL_PROMPT "\[A-Za-z>$%\]*"
set CHECK_SHELL_PROMPT "\[A-Za-z\]*\[#>$%\]*"

set descriptors [exec "/bin/sh" "-c" "ulimit -n"]
puts "    *********************************************"
puts "    * CONNECTION SETUP (remote_procedures.tcl)"
puts "    *********************************************"
puts "    * descriptors = $descriptors"
set rlogin_max_open_connections [expr ($descriptors - 15) / 3]
puts "    * rlogin_max_open_connections = $rlogin_max_open_connections"
puts "    *********************************************"

# procedures
#                                                             max. column:     |
#****** remote_procedures/setup_qping_dump() ***********************************
#  NAME
#     setup_qping_dump() -- start qping dump as remote process (as root)
#
#  SYNOPSIS
#     setup_qping_dump { log_array } 
#
#  FUNCTION
#     starts qping -dump as root user on the qmaster host and fills the
#     log_array with connection specific data
#
#  INPUTS
#     log_array - array for results and settings
#
#  SEE ALSO
#     remote_procedures/setup_qping_dump()
#     remote_procedures/get_qping_dump_output()
#     remote_procedures/cleanup_qping_dump_output()
#*******************************************************************************
proc setup_qping_dump { log_array  } {
   global CHECK_OUTPUT ts_config
   upvar $log_array used_log_array

   set master_host $ts_config(master_host)
   set master_host_arch [resolve_arch $master_host]
   set qping_binary    "$ts_config(product_root)/bin/$master_host_arch/qping"
   set qping_arguments "-dump $master_host $ts_config(commd_port) qmaster 1"
   set qping_env(SGE_QPING_OUTPUT_FORMAT) "s:1 s:2 s:3 s:4 s:5 s:6 s:7 s:8 s:9 s:10 s:11 s:12 s:13 s:14 s:15"

   if { $ts_config(gridengine_version) >= 60 } {
      set sid [open_remote_spawn_process $master_host "root" $qping_binary $qping_arguments 0 qping_env]
      set sp_id [lindex $sid 1]
   } else {
      set sid   "unsupported version < 60"
      set sp_id "unsupported version < 60"
   }

   set used_log_array(spawn_sid)    $sid
   set used_log_array(spawn_id)     $sp_id
   set used_log_array(actual_line)  0
   set used_log_array(in_block)     0

   
   if { $ts_config(gridengine_version) >= 60 } {
      set timeout 15
      while { 1 } {
         expect {
            -i $used_log_array(spawn_id) -- full_buffer {
               add_proc_error "get_qping_dump_output" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
               break
            }
            -i $used_log_array(spawn_id) eof {
               add_proc_error "setup_qping_dump" -1 "unexpected eof getting qping -dump connection to qmaster"
               break
            }
            -i $used_log_array(spawn_id) timeout {
               add_proc_error "setup_qping_dump" -1 "timeout for getting qping -dump connection to qmaster"
               break
            }
            -i $used_log_array(spawn_id) -- "*debug_client*crm*\n" {
               puts $CHECK_OUTPUT "qping is now connected to qmaster!"
               break
            }
            -i $used_log_array(spawn_id) -- "_exit_status_*\n" {
               puts $CHECK_OUTPUT "qping doesn't support -dump switch in this version"
               break
            }
            -i $used_log_array(spawn_id) -- "*\n" {
               debug_puts $expect_out(buffer)
            }
         }     
      }
   }
}


#****** remote_procedures/check_all_system_times() **********************************
#  NAME
#     check_all_system_times() -- check clock synchronity on each cluster deamon host
#
#  SYNOPSIS
#     check_all_system_times {} 
#
#  FUNCTION
#     If the sytem time difference is larger than +/-10 seconds from qmaster time
#     the function will fail
#  
#  RESULT
#     0 on success, 1 on error
#************************************************************************************
proc check_all_system_times {} {
   global CHECK_USER CHECK_OUTPUT ts_config CHECK_SCRIPT_FILE_DIR

   set return_value 0
   set host_list [host_conf_get_cluster_hosts]

   foreach host $host_list {
      puts $CHECK_OUTPUT "test connection to $host ..."
      set result [string trim [start_remote_prog $host $CHECK_USER "echo" "hallo"]]
      puts $CHECK_OUTPUT $result
   }

   set test_start [timestamp]
   foreach host $host_list {
      set tcl_bin [ get_binary_path $host "expect"]
      set time_script "$ts_config(testsuite_root_dir)/$CHECK_SCRIPT_FILE_DIR/time.tcl"
      puts $CHECK_OUTPUT "test remote system time on host $host ..."
      set result [string trim [start_remote_prog $host $CHECK_USER $tcl_bin $time_script]]
      puts $CHECK_OUTPUT $result
      set time($host) [get_string_value_between "current time is" -1 $result]
      # fix remote execution time difference
      set time($host) [expr ( $time($host) - [expr ( [timestamp] - $test_start ) ] )] 
   }

   set reference_time $time($ts_config(master_host))
   foreach host $host_list {
      set diff [expr ( $reference_time - $time($host) )]
      puts $CHECK_OUTPUT "host $host has a time difference of $diff seconds compared to host $ts_config(master_host)"

      if { $diff > 45 || $diff < -45 } {
         add_proc_error "check_all_system_times" -2 "host $host has a time difference of $diff seconds compared to host $ts_config(master_host)"
         set return_value 1
      }

   }
   return $return_value
}

#****** remote_procedures/get_qping_dump_output() ******************************
#  NAME
#     get_qping_dump_output() -- get qping dump output
#
#  SYNOPSIS
#     get_qping_dump_output { log_array } 
#
#  FUNCTION
#     This function fills up the log_array with qping -dump information. 
#     The function will return after 2 seconds when no dump output is available
#
#  INPUTS
#     log_array - array for results and settings
#
#  SEE ALSO
#     remote_procedures/setup_qping_dump()
#     remote_procedures/get_qping_dump_output()
#     remote_procedures/cleanup_qping_dump_output()
#*******************************************************************************
proc get_qping_dump_output { log_array } {
   global CHECK_OUTPUT ts_config
   upvar $log_array used_log_array
   set return_value 0


   if { $ts_config(gridengine_version) >= 60 } {
      set timeout 2
      log_user 0
      expect {
         -i $used_log_array(spawn_id) -- full_buffer {
            add_proc_error "get_qping_dump_output" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
         }
         -i $used_log_array(spawn_id) eof {
         }
         -i $used_log_array(spawn_id) timeout {
            set return_value 1
         }
         -i $used_log_array(spawn_id) -- "_exit_status_" {
         }
         -i $used_log_array(spawn_id) -- "*\n" {
            set output $expect_out(buffer)
            set output [ split $output "\n" ]
            foreach line $output {
               set line [string trim $line]
   
               if {[string length $line] == 0} {
                  continue
               }
   
   # we got a qping output line
               
               if { [string match "??:??:??*" $line] != 0 } {
                  set qping_line [split $line "|"]
                  set used_log_array(line,$used_log_array(actual_line)) $line
                  set row 1
                  foreach column $qping_line {
                     set used_log_array(line,$row,$used_log_array(actual_line)) $column
                     incr row 1
                  }
   
                  while { $row < 15 } {
                     set used_log_array(line,$row,$used_log_array(actual_line)) "not available"
                     incr row 1
                  }
                  set used_log_array(block,$used_log_array(actual_line)) "not available"
                  incr used_log_array(actual_line) 1
                  continue
               }
               
   # here we try to log addition data for the last line
               if { [string match "*block start*" $line ] != 0 } {
                  set used_log_array(in_block) 1
                  set line_block $used_log_array(actual_line)
                  incr line_block -1
                  set used_log_array(block,$line_block) ""
               }
   
               if { $used_log_array(in_block) == 1 } {
                  set line_block $used_log_array(actual_line)
                  incr line_block -1
                  append used_log_array(block,$line_block) "$line\n"
               }
               if { [string match "*block end*" $line ] != 0 } {
                  set used_log_array(in_block) 0
               }
   
            }
         }
      }
      log_user 1
   
   # qping for 60u4 and higher
   #   01 yes    time     time of debug output creation
   #   02 yes    local    endpoint service name where debug client is connected
   #   03 yes    d.       message direction
   #   04 yes    remote   name of participating communication endpoint
   #   05 yes    format   message data format
   #   06 yes    ack type message acknowledge type
   #   07 yes    msg tag  message tag information
   #   08 yes    msg id   message id
   #   09 yes    msg rid  message response id
   #   10 yes    msg len  message length
   #   11 yes    msg time time when message was sent/received
   #   12  no    xml dump commlib xml protocol output
   #   13  no    info     additional information
   
   # qping for 60u2 - 60u3
   #
   #   01 time     time of debug output creation
   #   02 local    endpoint service name where debug client is connected
   #   03 d.       message direction
   #   04 remote   name of participating communication endpoint
   #   05 format   message data format
   #   06 ack type message acknowledge type
   #   07 msg tag  message tag information
   #   08 msg id   message id
   #   09 msg rid  message response id
   #   10 msg len  message length
   #   11 msg time time when message was sent/received
   #   12 xml dump commlib xml protocol output
   #   13 info     additional information
   } else {
      puts $CHECK_OUTPUT "qping not supported !"
      set return_value 1
   }
   return $return_value
}

#****** remote_procedures/cleanup_qping_dump_output() **************************
#  NAME
#     cleanup_qping_dump_output() -- shutdwon qping dump connection
#
#  SYNOPSIS
#     cleanup_qping_dump_output { log_array } 
#
#  FUNCTION
#     close qping -dump spawn connection
#
#  INPUTS
#     log_array - array for results and settings
#
#  SEE ALSO
#     remote_procedures/setup_qping_dump()
#     remote_procedures/get_qping_dump_output()
#     remote_procedures/cleanup_qping_dump_output()
#*******************************************************************************
proc cleanup_qping_dump_output { log_array } {
   global CHECK_OUTPUT ts_config
   upvar $log_array used_log_array

   if { $ts_config(gridengine_version) >= 60 } {
      close_spawn_process $used_log_array(spawn_sid)
   }
}


#                                                             max. column:     |
#****** remote_procedures/start_remote_tcl_prog() ******
# 
#  NAME
#     start_remote_tcl_prog -- ??? 
#
#  SYNOPSIS
#     start_remote_tcl_prog { host user tcl_file tcl_procedure tcl_procargs } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     host          - ??? 
#     user          - ??? 
#     tcl_file      - ??? 
#     tcl_procedure - ??? 
#     tcl_procargs  - ??? 
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
proc start_remote_tcl_prog { host user tcl_file tcl_procedure tcl_procargs} {
  
   global CHECK_TESTSUITE_ROOT CHECK_SCRIPT_FILE_DIR CHECK_TCL_SCRIPTFILE_DIR CHECK_DEFAULTS_FILE
   global CHECK_OUTPUT CHECK_DEBUG_LEVEL
 
   log_user 1
   puts $CHECK_OUTPUT "--- start_remote_tcl_prog start ---"
   set tcl_bin [ get_binary_path $host "expect"]
   set tcl_prog "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/remote_tcl_command.sh"
   set tcl_testhome "$CHECK_TESTSUITE_ROOT"
   set tcl_defaults  "$CHECK_DEFAULTS_FILE"
   
   debug_puts "tcl_bin: \"$tcl_bin\""
   debug_puts "tcl_prog: \"$tcl_prog\""
   debug_puts "tcl_testhome: \"$tcl_testhome\""
   debug_puts "tcl_defaults: \"$tcl_defaults\""


   set debug_arg ""
   if { $CHECK_DEBUG_LEVEL != 0 } {
      set debug_arg "debug"
   }
   set remote_args "$tcl_bin $CHECK_TESTSUITE_ROOT/$CHECK_TCL_SCRIPTFILE_DIR/$tcl_file $tcl_testhome $tcl_procedure \"$tcl_procargs\" $tcl_defaults $debug_arg"

   set result ""
   debug_puts "prog: $tcl_prog"
   debug_puts "remote_args: $remote_args"
   log_user 1

   set result [ start_remote_prog "$host" "$user" "$tcl_prog" "$remote_args" prg_exit_state 600 0 "" 1 0 1]
   if { [string first "Error in procedure" $result] >= 0 } {
      add_proc_error "start_remote_tcl_prog" -2 "error in $tcl_file, proc $tcl_procedure $tcl_procargs"
   }
   puts $result
   log_user 1
   puts $CHECK_OUTPUT "--- start_remote_tcl_prog end   ---"
   return $result
}




#                                                             max. column:     |
#
#****** remote_procedures/start_remote_prog() ******
#  NAME
#     start_remote_prog() -- start remote application
#
#  SYNOPSIS
#     start_remote_prog { hostname user exec_command exec_arguments 
#     {exit_var prg_exit_state} {mytimeout 60} {background 0} {envlist ""}} 
#
#  FUNCTION
#     This procedure will start the given command on the given remote
#     host.
#
#  INPUTS
#     hostname                  - hostname
#     user                      - user name
#     exec_command              - application to start
#     exec_arguments            - application arguments
#     {exit_var prg_exit_state} - return value of the (last) remote command
#     {mytimeout 60}            - problem timeout (for connection building)
#     {background 0}            - if not 0 -> start remote prog in background
#                                 this will always take 15 seconds 
#     {envlist}                 - array with environment settings to export
#                                 before starting program
#     { do_file_check 1 }       - internal parameter for file existence check
#                                 if 0: don't do a file existence check
#     { source_settings_file 1 }- if 1 (default):
#                                 source $SGE_ROOT/$SGE_CELL/settings.csh
#                                 if not 1: don't source settings file
#     { set_shared_lib_path 1 } - if 1 (default): set shared lib path
#                               - if not 1: don't set shared lib path 
#
#  RESULT
#     program output
#
#  EXAMPLE
#     set envlist(COLUMNS) 500
#     start_remote_prog "ahost" "auser" "ls" "-la" "prg_exit_state" "60" "0" "envlist"
#
#  NOTES
#     The exec_arguments parameter can be used to start more commands:
#
#     start_remote_prog "ahost" "auser" "cd" "/tmp ; ls -la"
#
#*******************************
#
proc start_remote_prog { hostname
                         user 
                         exec_command 
                         exec_arguments 
                         {exit_var prg_exit_state} 
                         {mytimeout 60} 
                         {background 0} 
                         {envlist ""}
                         {do_file_check 1} 
                         {source_settings_file 1} 
                         {set_shared_lib_path 1}
                         {raise_error 1}
                         {win_local_user 0}
                       } {
   global CHECK_OUTPUT CHECK_MAIN_RESULTS_DIR CHECK_DEBUG_LEVEL 
   global CHECK_HOST
   upvar $exit_var back_exit_state

   if {$envlist != ""} {
      upvar $envlist users_env
   }

   set back_exit_state -1
   set tmp_exit_status_string ""
   if {[llength $exec_command] != 1} {
      puts "= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = ="
      puts "  WARNING     WARNING   WARNING  WARNING"
      puts "  procedure start_remote_prog: \"$exec_command\""
      puts "  is not a command name; it has additional arguments" 
      puts "= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = ="
      if {$CHECK_DEBUG_LEVEL == 2} {
         wait_for_enter 
      }
   }

   # open connection
   set id [open_remote_spawn_process "$hostname" "$user" "$exec_command" "$exec_arguments" $background users_env $source_settings_file 15 $set_shared_lib_path $raise_error $win_local_user]
   if {$id == ""} {
      add_proc_error "start_remote_prog" -1 "got no spawn id" $raise_error
      set back_exit_state -255
      return ""
   }

   set myspawn_id [ lindex $id 1 ]
   set output ""
   set do_stop 0

   # in debug mode, we want to see all shell I/O
   log_user 0
   if { $CHECK_DEBUG_LEVEL != 0 } {
      log_user 1
   }

   set timeout $mytimeout
   set real_end_found 0
   set real_start_found 0
   set nr_of_lines 0
 
   debug_puts "starting command ..."
   expect {
     -i $myspawn_id -- "*\n" {
        foreach line [split $expect_out(0,string) "\n"] {
           if { $line == "" } {
              continue
           }
           if { $real_start_found == 1 } {
              append output "$line\n"
              incr nr_of_lines 1
              set tmp_exit_status_start [string first "_exit_status_:" $line]
              if { $tmp_exit_status_start >= 0 } {
                 # check if a ? is between _exit_status_:() - brackets
                 set tmp_exit_status_string [string range $line $tmp_exit_status_start end]
                 set tmp_exit_status_end [string first ")" $tmp_exit_status_string]
                 if {$tmp_exit_status_end >= 0 } {
                    set tmp_exit_status_string [ string range $tmp_exit_status_string 0 $tmp_exit_status_end ]
                 } else {
                    add_proc_error "start_remote_prog" -1 "unexpected error - did not get full exit status string" $raise_error
                 }
                 set real_end_found 1
                 set do_stop 1
                 break
              }
           } else {
              if { [ string first "_start_mark_:" $line ] >= 0 } {
                 set real_start_found 1
                 set output "_start_mark_\n"
                 if { $background == 1 } { 
                    set do_stop 1
                    break
                 }
              }
           }
        }
        if { $do_stop == 0 } {
           exp_continue
        }
     }

     -i $myspawn_id timeout {
        add_proc_error "start_remote_prog" "-1" "timeout error(1):\nmaybe the shell is expecting an interactive answer from user?\nexec commando was: \"$exec_command $exec_arguments\"\n$expect_out(buffer)\nmore information in next error message in 5 seconds!!!" $raise_error
        set timeout 5
        expect {
           -i $myspawn_id full_buffer {
              add_proc_error "start_remote_prog" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value" $raise_error
           }
           -i $myspawn_id timeout {
              add_proc_error "start_remote_prog" "-1" "no more output available" $raise_error $raise_error
           }
           -i $myspawn_id "*" {
              add_proc_error "start_remote_prog" "-1" "expect buffer:\n$expect_out(buffer)" $raise_error
           }
           -i $myspawn_id default {
              add_proc_error "start_remote_prog" "-1" "default - no more output available" $raise_error
           }
        }
     }

     -i $myspawn_id full_buffer {
        add_proc_error "start_remote_prog" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value" $raise_error
     }
   }
   debug_puts "starting command done!"

   close_spawn_process $id

   # parse output: cut leading sequence 
   set found_start [string first "_start_mark_" $output]
   if { $found_start >= 0 } {
      incr found_start 13
      set output [string range $output $found_start end]
   }

   # parse output: find end of output and rest
   if {$real_end_found == 1} {
      set found_end [string first "_exit_status_:" $output]
      if { $found_end >= 0 } {
         incr found_end -1
         set output [ string range $output 0 $found_end ] 
      }
   }

   # parse output: search exit status
   if {$real_end_found == 1} {
      set first_index [string first "(" $tmp_exit_status_string ]
      incr first_index 1
      set last_index [string first ")" $tmp_exit_status_string ]
      incr last_index -1 
      set exit_status [string range $tmp_exit_status_string $first_index $last_index]
   } else {
      set exit_status -1
   }

   debug_puts "E X I T   S T A T E   of remote prog: $exit_status"
    
   if { $CHECK_DEBUG_LEVEL == 2 } {
      wait_for_enter
   }

   set back_exit_state $exit_status
   return $output
}

#****** remote_procedures/sendmail() *******************************************
#  NAME
#     sendmail() -- sendmail in mime format (first prototype)
#
#  SYNOPSIS
#     sendmail { to subject body { send_html 0 } { cc "" } { bcc "" } 
#     { from "" } { replyto "" } { organisation "" } } 
#
#  FUNCTION
#     This procedure is under construction
#
#  INPUTS
#     to                  - ??? 
#     subject             - ??? 
#     body                - ??? 
#     { send_html 0 }     - ??? 
#     { cc "" }           - ??? 
#     { bcc "" }          - ??? 
#     { from "" }         - ??? 
#     { replyto "" }      - ??? 
#     { organisation "" } - ??? 
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
proc sendmail { to subject body { send_html 0 } { cc "" } { bcc "" } { from "" } { replyto "" } { organisation "" } { force_mail 0 } } {
   global CHECK_HOST CHECK_USER CHECK_OUTPUT ts_config CHECK_ENABLE_MAIL CHECK_MAILS_SENT CHECK_MAX_ERROR_MAILS

   if { $CHECK_ENABLE_MAIL != 1 && $force_mail == 0 } {
     puts $CHECK_OUTPUT "mail sending disabled, mails sent: $CHECK_MAILS_SENT"
     puts $CHECK_OUTPUT "mail subject: $subject"
     puts $CHECK_OUTPUT "mail body:"
     puts $CHECK_OUTPUT "$body"
     return -1
   }


   puts $CHECK_OUTPUT "--> sending mail to $to from host $ts_config(mailx_host) ...\n"
   # setup mail message    
   set mail_file [get_tmp_file_name]
   set file [open $mail_file "w"]

   puts $file "Mime-Version: 1.0"
   if { $send_html != 0 } {
      puts $file "Content-Type: text/html ; charset=ISO-8859-1"
   } else {
      puts $file "Content-Type: text/plain ; charset=ISO-8859-1"
   }

   if { $organisation != "" }  {
      puts $file "Organization: $organisation"
   }
   if { $from != "" } {
      puts $file "From: $from"
   }
   if { $replyto != "" } {
      puts $file "Reply-To: $replyto"
   }
   puts $file "To: $to"
   foreach elem $cc {
      puts $file "Cc: $elem"
   }
   foreach elem $bcc {
      puts $file "Bcc: $elem"
   }

   set new_subject "[get_version_info] ($ts_config(cell)) - $subject"

   puts $file "Subject: $new_subject"
   puts $file ""
   # after this line the mail begins
   puts $file "Grid Engine Version: [get_version_info]"
   puts $file "Subject            : $subject"
   puts $file ""
   puts $file "$body"
   puts $file ""
   puts $file "."
   close $file


   # start sendmail

   # TODO: get sendmail path
   # TODO: configure mail host in testsuite configuration

   set command "/usr/lib/sendmail"
   set arguments "-B 8BITMIME -t < $mail_file"

   set result [start_remote_prog $ts_config(mailx_host) $CHECK_USER $command $arguments prg_exit_state 60 0 "" 1 0]
   if { $prg_exit_state != 0 } {
      puts $CHECK_OUTPUT "=================================="
      puts $CHECK_OUTPUT "COULD NOT SEND MAIL:\n$result"
      puts $CHECK_OUTPUT "=================================="
      return -1
   }
   incr CHECK_MAILS_SENT 1
   if { $CHECK_MAILS_SENT == $CHECK_MAX_ERROR_MAILS } {
      set CHECK_ENABLE_MAIL 0
      sendmail $to "max mail count reached" "" 0 "" "" "" "" "" 1
   }
     
   return 0
}


proc sendmail_wrapper { address cc subject body } {
   global CHECK_HOST CHECK_OUTPUT ts_config CHECK_USER

#   set html_text ""
#   foreach line [split $body "\n"] {
#      append html_text [create_html_text $line]
#   }
#   return [sendmail $address $subject $html_text 1 $cc "" $address $address "Gridware"]

   if { $ts_config(mail_application) == "mailx" } {
      puts $CHECK_OUTPUT "using mailx to send mail ..."
      return 1
   }
   if { $ts_config(mail_application) == "sendmail" } {
      puts $CHECK_OUTPUT "using sendmail to send mail ..."
      sendmail $address $subject $body 0 $cc "" $address $address "Gridware"
      return 0
   }

   

   puts $CHECK_OUTPUT "starting $ts_config(mail_application) on host $$ts_config(mailx_host) to send mail ..."
   set tmp_file [get_tmp_file_name]
   set script [ open "$tmp_file" "w" "0755" ]
   puts $script "Grid Engine Version: [get_version_info]"
   puts $script "Subject            : $subject"
   puts $script ""
   puts $script $body
   puts $script ""
   flush $script
   close $script

   set new_subject "[get_version_info] ($ts_config(cell)) - $subject"

   wait_for_remote_file $ts_config(mailx_host) $CHECK_USER $tmp_file
   set result [start_remote_prog $ts_config(mailx_host) $CHECK_USER $ts_config(mail_application) "\"$address\" \"$cc\" \"$new_subject\" \"$tmp_file\""]
   puts $CHECK_OUTPUT "mail application returned exit code $prg_exit_state:"
   puts $CHECK_OUTPUT $result
   return 0
}

proc create_error_message { error_array} {
  global CHECK_OUTPUT
  set catch_return [catch {
  set err_string [lindex $error_array 0]
  #set err_string $error_array
  } ]
  if { $catch_return == 1 } {
     set err_string "catch error: error reading error_array"
  }
  
  set err_complete  [split $err_string "|"]
  set err_procedure [lindex $err_complete 0]
  set err_checkname [lindex $err_complete 1]
  set err_calledby  [lindex $err_complete 2]
  set err_list      [lrange $err_complete 3 end]
   set err_text ""
   foreach elem $err_list {
      if {$err_text != ""} {
         append err_text "|"
      }
      append err_text $elem
   }
  
  append output "check       : $err_checkname\n"
  append output "procedure   : $err_procedure\n"
  if { [ string compare $err_calledby $err_procedure ] != 0 } {
     append output "called from : $err_calledby\n"
  }
  append output "----------------------------------------------------------------\n"
  append output "$err_text\n"
  append output "----------------------------------------------------------------\n"

  return $output
}



proc show_proc_error { result new_error } {
   global CHECK_CUR_PROC_ERRORS CHECK_CUR_PROC_RESULTS CHECK_CUR_PROC_NAME CHECK_OUTPUT check_name CHECK_TESTSUITE_ROOT
   global CHECK_ARCH CHECK_HOST CHECK_PRODUCT_ROOT CHECK_ACT_LEVEL CHECK_CORE_MASTER CHECK_CORE_EXECD
   global CHECK_SEND_ERROR_MAILS ts_config

   if { $result != 0 } {
      set category "error"
      if { $result == -3 } {
         set category "unsupported test warning"
      }
      puts $CHECK_OUTPUT ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
      puts $CHECK_OUTPUT "$category"
      puts $CHECK_OUTPUT "runlevel    : \"[get_run_level_name $CHECK_ACT_LEVEL]\", ($CHECK_ACT_LEVEL)"
      puts $CHECK_OUTPUT ""
      set error_output [ create_error_message $new_error ]
      puts $CHECK_OUTPUT $error_output 
      puts $CHECK_OUTPUT ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"



      flush $CHECK_OUTPUT
      if { $CHECK_SEND_ERROR_MAILS == 1 } {
         append mail_body "\n"
         append mail_body "Date            : [exec date]\n"
         append mail_body "check_name      : $check_name\n"
         append mail_body "category        : $category\n"
         append mail_body "runlevel        : [get_run_level_name $CHECK_ACT_LEVEL] (level: $CHECK_ACT_LEVEL)\n"
         append mail_body "check host      : $CHECK_HOST\n"
         append mail_body "product version : [get_version_info]\n"
         append mail_body "SGE_ROOT        : $CHECK_PRODUCT_ROOT\n"
         append mail_body "master host     : $CHECK_CORE_MASTER\n"
         append mail_body "execution hosts : $CHECK_CORE_EXECD\n\n"

         append mail_body "$error_output"
         catch {
            foreach level "1 2 3 4" {
               upvar $level expect_out out
               if {[info exists out]} {
                  append mail_body "----- expect buffer in upper level $level --------\n"
                  foreach i [array names out] {
                     append mail_body "$i:\t$out($i)\n"
                  }
               }
            }
         }
       
         append mail_body "\nTestsuite configuration (ts_config):\n"
         append mail_body "====================================\n"
         show_config ts_config 0 mail_body

         mail_report "testsuite $category - $check_name" $mail_body
      }
   }
}



#****** remote_procedures/close_spawn_id() *************************************
#  NAME
#     close_spawn_id() -- close spawn_id and wait child process
#
#  SYNOPSIS
#     close_spawn_id { spawn_id } 
#
#  FUNCTION
#     Closes a certain spawn id and calls wait to have the child process
#     cleaned up.
#
#  INPUTS
#     spawn_id - the spawn_id to close
#
#  NOTES
#     The function should be called within a catch block.
#     Both close and wait called here might raise an exception.
#*******************************************************************************
proc close_spawn_id {spawn_id} {
   close -i $spawn_id
   wait -nowait -i $spawn_id
}

#****** remote_procedures/increase_timeout() ***********************************
#  NAME
#     increase_timeout() -- stepwise increase expect timeout
#
#  SYNOPSIS
#     increase_timeout { {max 5} {step 1} } 
#
#  FUNCTION
#     Stepwise increases the timeout variable in the callers context.
#     timeout is increased by $step per call of this function 
#     up to $max.
#
#  INPUTS
#     {max 5}  - maximum timeout value
#     {step 1} - step
#*******************************************************************************
proc increase_timeout {{max 5} {step 1}} {
   upvar timeout timeout

   if {$timeout < $max} {
      incr timeout $step
   }
}

#****** remote_procedures/map_special_users() **********************************
#  NAME
#     map_special_users() -- map special user names and windows user names
#
#  SYNOPSIS
#     map_special_users { hostname user win_local_user } 
#
#  FUNCTION
#     Does username mapping for testsuite special users and windows users.
#
#     Testsuite special users are user names beginning with "ts_def_con".
#     If such a user name is passed to a function opening a rlogin connection,
#     the connection will be opened as CHECK_USER.
#     If a connection as CHECK_USER or another ts_def_con user already exists,
#     a new, additional connection will be opened instead of reusing the 
#     existing one.
#
#     Further mapping is done for windows users:
#     The user id "root" is mapped to the windows user name "Administrator".
#     If we connect to a windows machine, usually the connection will be done
#     as windows domain user.
#     There are cases where we have to become a local user (user Administrator, 
#     or parameter win_local_user set to 1).
#     In this case, we'll use as hostname "$hostname+$user".
#
#     On UNIX systems, using rlogin, we'll always connect as CHECK_USER, and
#     later on switch to root or the target user.
#     Using ssh, we'll connect as CHECK_USER, or as root, and later on 
#     switch to the target user.
#
#     On Windows systems, we'll always connect as the target user (CHECK_USER,
#     Administrator, other target user).
#
#  INPUTS
#     hostname       - host to which we want to connect
#     user           - the target user id (may also be a special user id)
#     win_local_user - do we want to connect as windows local or domain user?
#
#  RESULT
#     The functions sets the following variables in the callers context:
#        - real_user:         this is the real user name on the target machine, 
#                             after mapping special users,
#                             or user name "root" to windows "Administrator".
#        - connect_user:      We'll connect to the target host as this user.
#        - connect_full_user: We'll use this user name on the rlogin / ssh 
#                             commandline.
#
#  EXAMPLE
#     map_special_users unix_host root 0
#        real_user         = root
#        connect_user      = CHECK_USER
#        connect_full_user = CHECK_USER
#
#     map_special_users unix_host sgetest 1
#        real_user         = sgetest
#        connect_user      = sgetest
#        connect_full_user = sgetest
#
#     map_special_users unix_host ts_def_con_translate 1
#        real_user         = CHECK_USER
#        connect_user      = CHECK_USER
#        connect_full_user = CHECK_USER
#
#     map_special_users win_host root 0
#        real_user         = Administrator
#        connect_user      = Administrator
#        connect_full_user = win_host+Administrator
#
#     map_special_users win_host sgetest 0
#        real_user         = sgetest
#        connect_user      = sgetest
#        connect_full_user = sgetest
#
#     map_special_users win_host sgetest 1
#        real_user         = sgetest
#        connect_user      = sgetest
#        connect_full_user = win_host+sgetest
#
#  SEE ALSO
#     remote_procedures/open_remote_spawn_process()
#*******************************************************************************
proc map_special_users {hostname user win_local_user} {
   global CHECK_USER

   upvar real_user         real_user          ;# this is the real user name on the target machine
   upvar connect_user      connect_user       ;# we'll connect as this user
   upvar connect_full_user connect_full_user  ;# using this name in rlogin/ssh -l option (windows domain!)

   # handle special user ids
   # for these special user id's, we connect as local users to windows hosts
   if {[string match "ts_def_con*" $user] == 1} {
      set real_user $CHECK_USER
   } else {
      set real_user $user
   }

   # on interix, the root user is Administrator
   # and we have to connect as the target user, as su doesn't work
   # for Administrator, we have to use "hostname+Administrator" (local user)
   if {[host_conf_get_arch $hostname] == "win32-x86"} {
      if {$user == "root"} {
         set real_user "Administrator"
         if {!$win_local_user} {
            set win_local_user 1
         }
      }

      set connect_user        $real_user

      if {$win_local_user} {
         set connect_full_user   "$hostname+$real_user"
      } else {
         set connect_full_user   $real_user
      }
   } else {
      # on unixes, we connect
      # with rlogin always as CHECK_USER (and su later, if necessary)
      # despite having the root password, we do not connect as root, as some unixes
      # disallow root login from network
      #
      # with ssh either as CHECK_USER
      #          or     as root (and su later, if necessary)
      if {[have_ssh_access]} {
         if {$real_user == $CHECK_USER} {
            set connect_user $CHECK_USER
            set connect_full_user $CHECK_USER
         } else {
            set connect_user "root"
            set connect_full_user "root"
         }
      } else {
         set connect_user $CHECK_USER
         set connect_full_user $CHECK_USER
      }
   }

   debug_puts "map_special_users: $user on host $hostname"
   debug_puts "   real_user:         $real_user"
   debug_puts "   connect_user:      $connect_user"
   debug_puts "   connect_full_user: $connect_full_user"
}

#****** remote_procedures/open_remote_spawn_process() **************************
#  NAME
#     open_remote_spawn_process() -- open spawn process on remote host
#
#  SYNOPSIS
#     open_remote_spawn_process { hostname user exec_command exec_arguments 
#     { background 0 } {envlist ""} { source_settings_file 1 } 
#     { nr_of_tries 15 } } 
#
#  FUNCTION
#     This procedure creates a shell script with default settings for Grid
#     Engine and starts it as spawn process on the given host.
#
#  INPUTS
#     hostname                   -  remote host (can also be local host!)
#     user                       -  user to start script
#     exec_command               -  command after script init
#     exec_arguments             -  arguments for command
#     { background 0 }           -  if not 0: 
#                                      start command with "&" in background 
#                                   if 2:
#                                      wait 30 seconds after starting 
#                                      background process
#     {envlist ""}               -  array with environment settings to export
#                                   before starting program
#     { source_settings_file 1 } -  if 1 (default):
#                                      source $SGE_ROOT/$SGE_CELL/settings.csh
#                                   if not 1:
#                                      don't source settings file
#     { nr_of_tries 15 }         -  timout value
#
#  RESULT
#     spawn id of process (internal format, see close_spawn_process for details)
#
#  EXAMPLE
#     set id [open_remote_spawn_process "boromir" "testuser" "ls" "-la"]
#     set do_stop 0
#     set output ""
#     set sp_id [lindex $id 1]
#     while { $do_stop == 0 } {
#        expect {
#           -i $sp_id full_buffer {     ;# input buffer default size is 2000 byte 
#              set do_stop 1
#              puts "error - buffer overflow" 
#           } 
#           -i $sp_id timeout { set do_stop 1 }
#           -i $sp_id eof { set do_stop 1 }
#           -i $sp_id "*\r" {
#              set output "$output$expect_out(0,string)"
#           }
#        }
#     }  
#     close_spawn_process $id
#     puts $CHECK_OUTPUT ">>> output start <<<"
#     puts $CHECK_OUTPUT $output
#     puts $CHECK_OUTPUT ">>> output end <<<"
#
#  NOTES
#     The spawn command is from the TCL enhancement EXPECT
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     remote_procedures/increase_timeout()
#     remote_procedures/close_spawn_id()
#     remote_procedures/map_special_users()
#*******************************************************************************
proc open_remote_spawn_process { hostname 
                                 user 
                                 exec_command 
                                 exec_arguments 
                                 {background 0} 
                                 {envlist ""} 
                                 {source_settings_file 1} 
                                 {nr_of_tries 15} 
                                 {set_shared_lib_path 1}
                                 {raise_error 1}
                                 {win_local_user 0}
                               } {

   global CHECK_OUTPUT CHECK_HOST CHECK_USER CHECK_TESTSUITE_ROOT CHECK_SCRIPT_FILE_DIR
   global CHECK_EXPECT_MATCH_MAX_BUFFER CHECK_DEBUG_LEVEL
   global CHECK_SHELL_PROMPT
   global last_shell_script_file last_spawn_command_arguments

   debug_puts "open_remote_spawn_process on host \"$hostname\""
   debug_puts "user:           $user"
   debug_puts "exec_command:   $exec_command"
   debug_puts "exec_arguments: $exec_arguments"

   # check parameters
   if {$nr_of_tries < 5} {
      add_proc_error "open_remote_spawn_process" -3 "unreasonably low nr_of_tries: $nr_of_tries, setting to 5" $raise_error
   }
   # the win_local_user feature is only needed on windows
   # reset it for non windows hosts
   if {$win_local_user && [host_conf_get_arch $hostname] != "win32-x86"} {
      set win_local_user 0
   }

   # handle special user ids
   map_special_users $hostname $user $win_local_user

   # common part of all error messages
   set error_info "connection to host \"$hostname\" as user \"$user\""

   # if command shall be started as other user than CHECK_USER
   # we need root access
   if {$real_user != $CHECK_USER} {
      if {[have_root_passwd] == -1} {
         add_proc_error "open_remote_spawn_process" -2 "${error_info}\nroot access required" $raise_error
         return "" 
      }
   }

   # we might want to pass a special environment
   if {$envlist != ""} {
      upvar $envlist users_env
   }

   # for code coverage testing, we might need a special environment
   if {[coverage_enabled]} {
      coverage_per_process_setup $hostname $real_user users_env
   }

   # if the same script is executed multiple times, don't recreate it
   set re_use_script 0
   # we check for a combination of all parameters
   set spawn_command_arguments "$hostname$user$exec_command$exec_arguments$background$envlist$source_settings_file$set_shared_lib_path$win_local_user"
   if {[info exists last_spawn_command_arguments]} {
      # compare last command with this command
      if {[string compare $spawn_command_arguments $last_spawn_command_arguments] == 0} {
         # check if the script is still available
         if {[file isfile $last_shell_script_file]} {
            set re_use_script 1
         }
      }
   }

   # now remember this argument string for use in the next call to open_remote_spawn_process
   set last_spawn_command_arguments $spawn_command_arguments

   # either use the previous script, or create a new one
   if {$re_use_script} {
      set script_name $last_shell_script_file
      debug_puts "re-using script $script_name"
   } else {
      set command_name [file tail $exec_command]
      set script_name [get_tmp_file_name $hostname $command_name "sh"]
      create_shell_script "$script_name" $hostname "$exec_command" "$exec_arguments" users_env "/bin/sh" 0 $source_settings_file $set_shared_lib_path
      debug_puts "created $script_name"

      # remember name of script file for use in the next call to open_remote_spawn_process
      set last_shell_script_file $script_name
   }

   # get info about an already open rlogin connection
   get_open_spawn_rlogin_session $hostname $user $win_local_user con_data

   # we might have the required connection open
   set open_new_connection 1
   if {$con_data(pid) != 0} {
      set pid          $con_data(pid)
      set spawn_id     $con_data(spawn_id)
      set nr_of_shells $con_data(nr_shells)

      # check, if the connection is still in use - error!
      if {[is_spawn_process_in_use $spawn_id]} {
         add_proc_error "open_remote_spawn_process" -2 "$error_info\nconnection is still in use" $raise_error
         return ""
      }

      # check if the connection is OK - if not, we'll try to reopen it
      # JG: TODO: We had an optimization before:
      #           in case of not re_use_script, we do not test the connection here,
      #           but assume the connection is OK.  
      #           In the file check section, the called script file_check.sh outputs
      #           the id string (same as in check_identity.sh), and we check
      #           for correct id and file available in the same expect section.
      #           Advantage: It is faster (up to 100 ms per call)
      #           Disadvantage: We detect a dead connection too late. If the rlogin
      #           connection is dead, open_remote_spawn_process will fail.
      #           With the current implementation, we are slower, but a dead connection
      #           will be detected early enough to close and reopen it.
      if {[check_rlogin_session $spawn_id $pid $hostname $user $nr_of_shells]} {
         debug_puts "Using open rlogin connection to host \"$hostname\", user \"$user\""
         set open_new_connection 0
      }
   }

   if {$open_new_connection} {
      # on interix (windows), we often get a message
      # "in.rlogind: Forkpty: Permission denied."
      # we want to try rlogin until success
      set connect_succeeded 0
      while {$connect_succeeded == 0} {
         # no open connection - open a new one
         debug_puts "opening connection to host $hostname"

         # on unixes, we connect as CHECK_USER which will give us no passwd question,
         # and handle root passwd question when switching to the target user later on
         #
         # on windows, we have to directly connect as the target user, where we get
         # a passwd question for root and first/second foreign user
         # 
         # for unixes, we reject a passwd question (passwd = ""), for windows, we 
         # send the stored passwd
         set passwd ""

         # we either open an ssh connection (as CHECK_USER or root) 
         # or rlogin as CHECK_USER
         #
         # on windows hosts, login as root and su - <user> doesn't work.
         # here we connect as target user and answer the passwd question
         if {[host_conf_get_arch $hostname] == "win32-x86"} {
            set passwd [get_passwd $real_user]
         }

         if {[have_ssh_access]} {
            set ssh_binary [get_binary_path $CHECK_HOST ssh]
            set pid [spawn $ssh_binary "-l" $connect_full_user $hostname]
         } else {
            set pid [spawn "rlogin" $hostname "-l" $connect_full_user]
         }

         if {$pid == 0 } {
           add_proc_error "open_remote_spawn_process" -2 "${error_info}\ncould not spawn! (pid = $pid)"  $raise_error
           return "" 
         }

         # in debug mode we want to see all the shell output
         log_user 0
         if {$CHECK_DEBUG_LEVEL != 0} {
            log_user 1
         }

         # we now have one open shell
         set nr_of_shells 1

         # set buffer size for new connection
         match_max -i $spawn_id $CHECK_EXPECT_MATCH_MAX_BUFFER
         debug_puts "open_remote_spawn_process -> buffer size is: [match_max -i $spawn_id]"

         # wait for shell to start
         set connect_errors 0
         set catch_return [catch {
            set num_tries $nr_of_tries
            set timeout 2
            expect {
               -i $spawn_id eof {
                  add_proc_error "open_remote_spawn_process (startup)" -2 "${error_info}\nunexpected eof" $raise_error
                  set connect_errors 1
               }
               -i $spawn_id full_buffer {
                  add_proc_error "open_remote_spawn_process (startup)" -2 "${error_info}\nbuffer overflow" $raise_error
                  set connect_errors 1
               }
               -i $spawn_id timeout {
                  incr num_tries -1
                  if {$num_tries > 0} {
                     if {$num_tries < 77} {
                        puts -nonewline $CHECK_OUTPUT "."
                        flush $CHECK_OUTPUT
                        send -i $spawn_id -- "\n"
                     }
                     increase_timeout
                     exp_continue
                  } else {
                     add_proc_error "open_remote_spawn_process (startup)" -2 "${error_info}\nstartup timeout"  $raise_error
                     set connect_errors 1
                  }
               }
               -i $spawn_id "assword:" {
                  if {$passwd == ""} {
                     add_proc_error "open_remote_spawn_process (startup)" -2 "${error_info}\ngot unexpected password question" $raise_error
                     set connect_errors 1
                  } else {
                     send -i $spawn_id -- "$passwd\n"
                     exp_continue
                  }
               }
               -i $spawn_id "The authenticity of host*" {
                  send -i $spawn_id -- "yes\n"
                  exp_continue
               }
               -i $spawn_id "Are you sure you want to continue connecting (yes/no)?*" {
                  send -i $spawn_id -- "yes\n"
                  exp_continue
               }
               -i $spawn_id "Please type 'yes' or 'no'*" {
                  send -i $spawn_id -- "yes\n"
                  exp_continue
               }
               -i $spawn_id "in.rlogind: Forkpty: Permission denied." {
                  # interix (windows) rlogind doesn't let us login
                  # sleep a while and retry
                  puts -nonewline $CHECK_OUTPUT "x" ; flush $CHECK_OUTPUT
                  sleep 10
                  continue
               }
               -i $spawn_id -re $CHECK_SHELL_PROMPT {
                  # recognized shell prompt - now we can continue / leave this expect loop
                  debug_puts "recognized shell prompt"
               }
            }
         } catch_error_message]
         if { $catch_return == 1 } {
            add_proc_error "open_remote_spawn_process (startup)" -2 "${error_info}\n$catch_error_message"  $raise_error
            set connect_errors 1
         }

         # did we have errors?
         if {$connect_errors} {
            catch {close_spawn_id $spawn_id}
            return ""
         }

         # now we should have a running shell
         # try to start a shell script doing some output we'll wait for
         set catch_return [catch {
            set num_tries $nr_of_tries
            # try to start the shell_start_output.sh script
            send -i $spawn_id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/shell_start_output.sh\n"
            set timeout 2
            expect {
               -i $spawn_id eof {
                  add_proc_error "open_remote_spawn_process (shell_response)" -2 "${error_info}\nunexpected eof" $raise_error
                  set connect_errors 1
               }
               -i $spawn_id full_buffer {
                  add_proc_error "open_remote_spawn_process (shell_response)" -2 "${error_info}\nbuffer overflow" $raise_error
                  set connect_errors 1
               }
               -i $spawn_id timeout {
                  incr num_tries -1
                  if {$num_tries > 0} {
                     # try to restart the shell_start_output.sh script
                     send -i $spawn_id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/shell_start_output.sh\n"
                     increase_timeout
                     exp_continue
                  } else {
                     # final timeout
                     add_proc_error "open_remote_spawn_process (shell_response)" -2 "${error_info}\ntimeout" $raise_error
                     send -i $spawn_id -- "\003" ;# send CTRL+C to stop poss. running processes
                     set connect_errors 1
                  }
                  
               }
               -i $spawn_id "assword:" {
                  if {$passwd == ""} {
                     add_proc_error "open_remote_spawn_process (shell_response)" -2 "${error_info}\ngot unexpected password question" $raise_error
                     set connect_errors 1
                  } else {
                     send -i $spawn_id -- "$passwd\n"
                     exp_continue
                  }
               }
               -i $spawn_id "The authenticity of host*" {
                  send -i $spawn_id -- "yes\n"
                  exp_continue
               }
               -i $spawn_id "Are you sure you want to continue connecting (yes/no)?*" {
                  send -i $spawn_id -- "yes\n"
                  exp_continue
               }
               -i $spawn_id "Please type 'yes' or 'no'*" {
                  send -i $spawn_id -- "yes\n"
                  exp_continue
               }
               -i $spawn_id "ts_shell_response*\n" {
                  # got output from shell_start_output.sh - leaving expect
                  debug_puts "shell started"
                  set connect_succeeded 1
               }
               -i $spawn_id "in.rlogind: Forkpty: Permission denied." {
                  # interix (windows) rlogind doesn't let us login
                  # sleep a while and retry
                  puts -nonewline $CHECK_OUTPUT "x" ; flush $CHECK_OUTPUT
                  sleep 10
                  continue
               }
            }
         } catch_error_message ]
         if { $catch_return == 1 } {
            add_proc_error "open_remote_spawn_process (shell response)" -2 "${error_info}\n$catch_error_message"  $raise_error
            set connect_errors 1
         }
             
         # did we have errors?
         if {$connect_errors} {
            catch {close_spawn_id $spawn_id}
            return ""
         }
      } ;# end while loop for interix connection problems

      # now we know that we have a connection and can start a shell script
      # try to check login id
      set catch_return [ catch {
         send -i $spawn_id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/check_identity.sh\n"
         set num_tries $nr_of_tries
         set timeout 2
         expect {
            -i $spawn_id eof {
               add_proc_error "open_remote_spawn_process (identity)" -2 "${error_info}\nunexpected eof" $raise_error
               set connect_errors 1
            }
            -i $spawn_id full_buffer {
               add_proc_error "open_remote_spawn_process (identity)" -2 "${error_info}\nbuffer overflow" $raise_error
               set connect_errors 1
            }
            -i $spawn_id timeout {
               incr num_tries -1
               if {$num_tries > 0} {
                  send -i $spawn_id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/check_identity.sh\n"
                  increase_timeout
                  exp_continue
               } else {
                  # final timeout
                  add_proc_error "open_remote_spawn_process (identity)" -2 "${error_info}\nshell doesn't start or runs not as user $CHECK_USER on host $hostname"  $raise_error
                  send -i $spawn_id -- "\003" ;# send CTRL+C to stop poss. running processes
                  set connect_errors 1
               }
             }
             -i $spawn_id "__ my id is ->*${connect_user}*\n" { 
                 debug_puts "logged in as ${connect_user} - fine" 
             }
          }
      } catch_error_message]
      if {$catch_return == 1} {
         add_proc_error "open_remote_spawn_process (identity)" -2 "${error_info}\n$catch_error_message"  $raise_error
         set connect_errors 1
      }

      # did we have errors?
      if {$connect_errors} {
         catch {close_spawn_id $spawn_id}
         return ""
      }

      # here we switch to the target user.
      # if we connected to a windows host, we must already be the target user, do nothing
      # if target user is CHECK_USER, do nothing.
      # if we have ssh access, and target_user is root, do nothing
      # else switch user
      set switch_user 1
      if {$real_user == $connect_user} {
         set switch_user 0
      }

      if {$switch_user} {
         debug_puts "we have to switch user"
         set catch_return [ catch {
            if {[have_ssh_access]} {
               debug_puts "have ssh root access - switching to $real_user"
               send -i $spawn_id -- "su - $real_user\n"
            } else {
               # we had rlogin access and are CHECK_USER
               if {$real_user == "root"} {
                  debug_puts "switching to root user"
                  send -i $spawn_id -- "su - root\n"
               } else {
                  debug_puts "switching to $real_user user"
                  send -i $spawn_id -- "su - root -c 'su - $real_user'\n" 
               }
            }
            incr nr_of_shells 1

            # without ssh access, we'll get the passwd question here
            if {![have_ssh_access]} {
               set timeout 60
               expect {
                  -i $spawn_id full_buffer {
                     add_proc_error "open_remote_spawn_process (switch user)" -2 "${error_info}\nbuffer overflow" $raise_error
                     set connect_errors 1
                  }
                  -i $spawn_id eof {
                     add_proc_error "open_remote_spawn_process (switch user)" -2 "${error_info}\nunexpected eof" $raise_error
                     set connect_errors 1
                  }
                  -i $spawn_id timeout {
                     add_proc_error "open_remote_spawn_process (switch user)" -2 "${error_info}\ntimeout waiting for passwd question" $raise_error
                     set connect_errors 1
                  }
                  -i $spawn_id "assword:" {
                     after 500
                     log_user 0
                     set send_slow "1 .1"
                     send -i $spawn_id -s -- "[get_root_passwd]\n"
                     debug_puts "root password sent" 
                     if {$CHECK_DEBUG_LEVEL != 0} {
                        log_user 1
                     }
                  }
               }
            }
         } catch_error_message]
         if {$catch_return == 1} {
            add_proc_error "open_remote_spawn_process (switch user)" -2 "${error_info}\n$catch_error_message"  $raise_error
            set connect_errors 1
         }

         # did we have errors?
         if {$connect_errors} {
            catch {close_spawn_id $spawn_id}
            return ""
         }

         # now we should have the id of the target user
         # check login id
         set catch_return [catch {
            send -i $spawn_id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/check_identity.sh\n"
            set num_tries $nr_of_tries
            set timeout 2
            expect {
               -i $spawn_id eof {
                  add_proc_error "open_remote_spawn_process (new identity)" -2 "${error_info}\nunexpected eof" $raise_error
                  set connect_errors 1
               }
               -i $spawn_id full_buffer {
                  add_proc_error "open_remote_spawn_process (new identity)" -2 "${error_info}\nbuffer overflow" $raise_error
                  set connect_errors 1
               }
               -i $spawn_id timeout {
                  incr num_tries -1
                  if {$num_tries > 0} {
                     send -i $spawn_id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/check_identity.sh\n"
                     increase_timeout
                     exp_continue
                  } else {
                     # final timeout
                     add_proc_error "open_remote_spawn_process (new identity)" -2 "${error_info}\nshell doesn't start or runs not as user $real_user on host $hostname"  $raise_error
                     send -i $spawn_id -- "\003" ;# send CTRL+C to stop poss. still running processes
                     set connect_errors 1
                  }
               }
               -i $spawn_id "__ my id is ->*${real_user}*\n" { 
                  debug_puts "correctly switched to user $real_user - fine" 
               }
               -i $spawn_id "__ my id is ->*${connect_user}*\n" { 
                  add_proc_error "open_remote_spawn_process (new identity)" -2 "${error_info}\nswitch to user $real_user didn't succeed, we are still ${connect_user}" $raise_error
                  set connect_errors 1
               }
               -i $spawn_id "ermission denied" {
                  add_proc_error "open_remote_spawn_process (new identity)" -2 "${error_info}\npermission denied" $raise_error
                  set connect_errors 1
               }
               -i $spawn_id "does not exist" {
                  add_proc_error "open_remote_spawn_process (new identity)" -2 "${error_info}\nuser $real_user doesn not exist on host $hostname" $raise_error
                  set connect_errors 1
               }
               -i $spawn_id "nknown*id" {
                  add_proc_error "open_remote_spawn_process (new identity)" -2 "${error_info}\nuser $real_user doesn not exist on host $hostname" $raise_error
                  set connect_errors 1
               }
            }
         } catch_error_message]
         if {$catch_return == 1} {
            add_proc_error "open_remote_spawn_process (new identity)" -2 "${error_info}\n$catch_error_message"  $raise_error
            set connect_errors 1
         }

         # did we have errors?
         if {$connect_errors} {
            catch {close_spawn_id $spawn_id}
            return ""
         }
      } ;# switch user

      # autocorrection and autologout might make problems
      set catch_return [catch {
         send -i $spawn_id -- "unset autologout\n"
         send -i $spawn_id -- "unset correct\n"
         # JG: TODO: what if the target user has a sh/ksh/bash?
      } catch_error_message]
      if {$catch_return == 1} {
         add_proc_error "open_remote_spawn_process (unset autologout)" -2 "${error_info}\n$catch_error_message"  $raise_error
         catch {close_spawn_id $spawn_id}
         return ""
      }
      # store the connection
      add_open_spawn_rlogin_session $hostname $user $win_local_user $spawn_id $pid $nr_of_shells
   } ;# opening new connection

   # If we call the command for the first time, make sure it is available on the remote machine
   # we wait for some time, as the it might take some time until the command is visible (NFS)
   if {$re_use_script == 0} {
      set catch_return [catch {
         debug_puts "checking remote file access ..."
         send -i $spawn_id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/file_check.sh $script_name\n"
         set connect_errors 0
         set num_tries $nr_of_tries
         set timeout 2
         expect {
            -i $spawn_id full_buffer {
               add_proc_error "open_remote_spawn_process (file check)" -2 "${error_info}\nbuffer overflow" $raise_error
               set connect_errors 1
            }
            -i $spawn_id eof {
               add_proc_error "open_remote_spawn_process (file check)" -2 "${error_info}\nunexpected eof" $raise_error
               set connect_errors 1
            }
            -i $spawn_id timeout {
               incr num_tries -1
               if {$num_tries > 0} {
                  send -i $spawn_id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/file_check.sh $script_name\n"
                  increase_timeout
                  exp_continue
               } else {
                  add_proc_error "open_remote_spawn_process (file check)" -2 "${error_info}\ntimeout waiting for file_check.sh script" $raise_error
                  set connect_errors 1
               }
            }  
            -i $spawn_id "file exists" {
               # fine, we are ready to go - leave expect loop
               debug_puts "script file exists on $hostname"
            }
         }
      } catch_error_message]
      if {$catch_return == 1} {
         add_proc_error "open_remote_spawn_process (file check)" -2 "${error_info}\n$catch_error_message"  $raise_error
         set connect_errors 1
      }

      # If the connection was OK before, but the file check failed, 
      # we might have NFS problems.
      # Nothing we can do about it. We'll close the connection and
      # return error.
      if {$connect_errors} {
         del_open_spawn_rlogin_session $spawn_id
         close_spawn_process "$pid $spawn_id $nr_of_shells"
         return ""
      }
   } else {
      debug_puts "skip checking remote script, using already used script ..."
   }

   # prepare for background start
   if {$background} {
      append script_name " &"
   }

   # now start the commmand and set the connection to busy
   debug_puts "\"$hostname\"($user): starting command: $exec_command $exec_arguments"
   set catch_return [catch {
      send -i $spawn_id -- "$script_name\n"
      set_spawn_process_in_use $spawn_id
   } catch_error_message]
   if {$catch_return == 1} {
      # The connection was OK before, but send failed?
      # Should be a rare situation.
      # We'll close the connection and return error
      add_proc_error "open_remote_spawn_process (starting command)" -2 "${error_info}\n$catch_error_message"  $raise_error
      del_open_spawn_rlogin_session $spawn_id
      close_spawn_process "$pid $spawn_id $nr_of_shells"
      return ""
   }

   # for background processes, wait some time
   if {$background == 2} {
      set back_time 15 ;# let background process time to do his initialization
      while {$back_time > 0} {
         puts -nonewline $CHECK_OUTPUT "."
         flush $CHECK_OUTPUT
         sleep 1
         incr back_time -1
      }
      puts $CHECK_OUTPUT "hope background process is initalized now!"
   }

   debug_puts "number of open shells: $nr_of_shells"

   set back "$pid $spawn_id $nr_of_shells"
   return $back
}

#                                                             max. column:     |
#****** remote_procedures/open_spawn_process() ******
# 
#  NAME
#     open_spawn_process -- start process with the expect "spawn" command
#
#  SYNOPSIS
#     open_spawn_process { args } 
#
#  FUNCTION
#     Starts process given in "args" and returns its spawn id and pid in a list. 
#     The first list element is the pid and the second is the spawn id. The return 
#     value is used in close_spawn_process to close the connection to this 
#     process.
#
#  INPUTS
#     args - full argument list of the process to start
#
#  RESULT
#     tcl list with id and pid of the process
#
#     - first element is the pid
#     - second element is the spawn id
#
#  EXAMPLE
#     set id [ 
#       open_spawn_process "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-dq" "$q_name"
#     ]
#     expect {
#       ...
#     }
#     puts "pid: [ lindex $id 0]"
#     puts "spawn id: [ lindex $id 1]"
#     close_spawn_process $id
#
#
#  NOTES
#     always close an opened spawn id with the procedure close_spawn_process
#
#  SEE ALSO
#     remote_procedures/open_spawn_process
#     remote_procedures/open_root_spawn_process
#     remote_procedures/close_spawn_process
#     remote_procedures/start_remote_tcl_prog
#     remote_procedures/start_remote_prog
#*******************************
proc open_spawn_process {args} {

   global CHECK_OUTPUT
   global CHECK_EXPECT_MATCH_MAX_BUFFER

   set arguments ""
   set my_arg_no 0

   foreach elem $args {
      incr my_arg_no 1
      if {$my_arg_no == 1} {
        set arguments "$elem"
      } else {
        set arguments "$arguments $elem"
      }
   }
   debug_puts $arguments
   set open_spawn_arguments $arguments

   debug_puts "starting spawn process ..."
   flush $CHECK_OUTPUT

   set pid   [ eval spawn $open_spawn_arguments ]
   set sp_id [ set spawn_id ]
   set back $pid
   lappend back $sp_id
   debug_puts "open_spawn_process:  arguments: $args"
      match_max -i $spawn_id $CHECK_EXPECT_MATCH_MAX_BUFFER
      debug_puts "open_spawn_process -> buffer size is: [match_max]"

   flush $CHECK_OUTPUT

   if {$pid == 0 } {
     add_proc_error "open_spawn_process" -1 "could not spawn! (ret_pid = $pid)" 
   }
   return $back
}

#****** remote_procedures/get_busy_spawn_rlogin_sessions() *********************
#  NAME
#     get_busy_spawn_rlogin_sessions() -- get number of busy rlogin sessions
#
#  SYNOPSIS
#     get_busy_spawn_rlogin_sessions { } 
#
#  FUNCTION
#     Returns the number of busy rlogin / ssh sessions.
#     Busy means, that a command is running in the session.
#     After a close_spawn_process, a session may stay open, but it will
#     be marked "idle".
#
#  RESULT
#     Number of busy sessions (a number 0 ... n)
#*******************************************************************************
proc get_busy_spawn_rlogin_sessions {} {
   set busy 0

   set sessions [get_open_rlogin_sessions]
   foreach session $sessions {
      if {[is_spawn_process_in_use $session]} {
         incr busy
      }
   }

   return $busy
}

#****** remote_procedures/dump_spawn_rlogin_sessions() *************************
#  NAME
#     dump_spawn_rlogin_sessions() -- dump connection information
#
#  SYNOPSIS
#     dump_spawn_rlogin_sessions { {do_output 1} } 
#
#  FUNCTION
#     Dumps information about open connections to a string buffer.
#     If do_output is 1, the string buffer will be written to 
#     CHECK_OUTPUT.
#
#  INPUTS
#     {do_output 1} - do output to CHECK_OUTPUT
#
#  RESULT
#     string containing info output
#
#  EXAMPLE
#     set output [dump_spawn_rlogin_sessions 0]
#     puts $CHECK_OUTPUT $output
#                         | myname   |     root | sgetest1 | sgetest2 | 
#     --------------------|----------|----------|----------|----------|
#     host1               |     idle |      --  |      --  |      --  | 
#     host2               |      --  |      --  |      --  |      --  | 
#     host3               |      --  |      --  |      --  |      --  | 
#     host4               |     idle |      --  |      --  |      --  | 
#     host5               |      --  |      --  |      --  |      --  | 
#     host6               |     idle |      --  |      --  |      --  | 
#     host3.domain.com    |     idle |      --  |      --  |      --  | 
#
#*******************************************************************************
proc dump_spawn_rlogin_sessions {{do_output 1}} {
   global CHECK_OUTPUT

   set host_list [host_conf_get_cluster_hosts]
   set user_list [user_conf_get_cluster_users]
   set sessions  [get_open_rlogin_sessions]

   # examine all open sessions
   foreach session $sessions {
      # get detailed session information
      get_spawn_id_rlogin_session $session connection
      set user $connection(user)
      set host $connection(hostname)

      # extend host and user list if necessary
      if {[lsearch $host_list $host] < 0} {
         lappend host_list $host
      }
      if {[lsearch $user_list $user] < 0} {
         lappend user_list $user
      }

      # output idle or busy
      if {[is_spawn_process_in_use $connection(spawn_id)]} {
         set result_array($user,$host) "busy"
      } else {
         set result_array($user,$host) "idle"
      }
   }

   set ret [print_xy_array $user_list $host_list result_array " -- "]
   
   if {$do_output} {
      puts $CHECK_OUTPUT ""
      puts $CHECK_OUTPUT $ret
      puts $CHECK_OUTPUT ""
   }

   return $ret
}

#****** remote_procedures/add_open_spawn_rlogin_session() **********************
#  NAME
#     add_open_spawn_rlogin_session() -- add spawn id to open connection list
#
#  SYNOPSIS
#     add_open_spawn_rlogin_session { hostname user spawn_id pid } 
#
#  FUNCTION
#     This procedure will add the given spawn id to the internal bookkeeping
#     of connections.
#  
#     If the number of open connections exceeds a maximum (determined from
#     file descriptor limit), the connection that has been idle for the 
#     longest time will be closed.
#
#     The following data structures are used for keeping track of sessions:
#        rlogin_spawn_session_buffer: TCL Array containing all parameters of
#           all sessions. It has one index element containing the names of all
#           open sessions (spawn ids).
#           Per spawn_id, the following data is stored:
#              - pid        pid of the expect child process from spawn command
#              - hostname   name of host to which we connected
#              - user       user for which the connection has been established.
#                           This can be a special name like ts_def_con.
#              - win_local_user For windows, we usually will connect as domain user.
#                               For some operations, e.g. accessing the local
#                               spool directory of an exec host, we have to use
#                               local users.
#              - ltime      timestamp of last use of the connection
#              - nr_shells  number of shells started in the connection
#              - in_use     is the connection in use or idle
#           The following names are used in the TCL array:
#              - index            {exp4 exp6}
#              - exp4,pid         12345
#              - exp4,hostname    myhostname
#              - ...
#              - exp6,pid         23455
#              - ...
#
#        rlogin_spawn_session_idx: TCL Array acting as an index for quick lookup
#           of connections by hostname and user.
#           It contains one entry per session. The array names are
#           <hostname>,<user>,<win_local_user>, e.g.
#           gimli,sgetest,0        exp4
#           balrog,sgetest,0       exp6
#           balrog,sgetest1,0      exp8
#           bofur,sgetest,1        exp10
#
#  INPUTS
#     hostname       - hostname of rlogin connection
#     user           - user who logged in
#     win_local_user - is the user a windows local (1) or domain user (0)
#     spawn_id       - spawn process id
#     pid            - process id of rlogin session
#
#  SEE ALSO
#     remote_procedures/get_open_spawn_rlogin_session
#     remote_procedures/get_spawn_id_rlogin_session
#     remote_procedures/del_open_spawn_rlogin_session
#     remote_procedures/remove_oldest_spawn_rlogin_session()
#*******************************************************************************
proc add_open_spawn_rlogin_session {hostname user win_local_user spawn_id pid nr_of_shells} {
   global CHECK_OUTPUT rlogin_spawn_session_buffer rlogin_spawn_session_idx
   global do_close_rlogin rlogin_max_open_connections 

   if {$do_close_rlogin != 0} {
      debug_puts "close_rlogin argument set, closing rlogin connections after use"
      return  
   }

   # if the number of connections exceed a certain maximum,
   # we have to close one connection - the one idle for the longest time
   set num_connections 0
   if {[info exists rlogin_spawn_session_buffer(index)]} {
      set num_connections [llength $rlogin_spawn_session_buffer(index)]
   }
   if {$num_connections >= $rlogin_max_open_connections} {
      debug_puts "number of open connections($num_connections) > rlogin_max_open_connections($rlogin_max_open_connections)"
      remove_oldest_spawn_rlogin_session
   }

   debug_puts "adding spawn_id $spawn_id, pid=$pid to host $hostname, user $user"

   # add session data
   set rlogin_spawn_session_buffer($spawn_id,pid)                 $pid
   set rlogin_spawn_session_buffer($spawn_id,hostname)            $hostname
   set rlogin_spawn_session_buffer($spawn_id,user)                $user
   set rlogin_spawn_session_buffer($spawn_id,win_local_user)      $win_local_user
   set rlogin_spawn_session_buffer($spawn_id,ltime)               [timestamp]
   set rlogin_spawn_session_buffer($spawn_id,nr_shells)           $nr_of_shells
   set rlogin_spawn_session_buffer($spawn_id,in_use)              0

   # add session to index
   lappend rlogin_spawn_session_buffer(index) $spawn_id

   # add session to search index
   set rlogin_spawn_session_idx($hostname,$user,$win_local_user) $spawn_id
}

#****** remote_procedures/remove_oldest_spawn_rlogin_session() *****************
#  NAME
#     remove_oldest_spawn_rlogin_session() -- remove oldest idle rlogin session
#
#  SYNOPSIS
#     remove_oldest_spawn_rlogin_session { } 
#
#  FUNCTION
#     Scans through all open rlogin session, and will close the session
#     that has been idle for the longest time.
#
#     If no session is idle, raise an error.
#
#     See remote_procedures/add_open_spawn_rlogin_session() for a description
#     of the data structures.
#
#  SEE ALSO
#     remote_procedures/add_open_spawn_rlogin_session()
#*******************************************************************************
proc remove_oldest_spawn_rlogin_session {} {
   global rlogin_spawn_session_buffer

   debug_puts "removing oldest not used rlogin session (rlogin_max_open_connections overflow)"
   set last [timestamp]
   set remove_spawn_id ""
   foreach spawn_id $rlogin_spawn_session_buffer(index) {
      set time $rlogin_spawn_session_buffer($spawn_id,ltime)
      debug_puts "$time $spawn_id $rlogin_spawn_session_buffer($spawn_id,in_use)"
      # only consider idle connections for closing
      if {$rlogin_spawn_session_buffer($spawn_id,in_use) == 0 && $last > $time} {
         set last $time
         set remove_spawn_id $spawn_id
      } 
   }

   # if we found no idle connection - error.
   if {$remove_spawn_id == ""} {
      add_proc_error "remove_oldest_spawn_rlogin_session" -2 "all [llength $rlogin_spawn_session_buffer(index)] sessions are in use - no oldest one to close.\nPlease check your file descriptor limit vs. cluster size.\nThis problem may also be caused by missing close_spawn_process calls."
   } else {
      debug_puts "longest not used element: $remove_spawn_id"

      # close the connection. We are not intested in the exit code of the 
      # previously executed command, so don't make close_spawn_process check
      # the exit code
      set pid $rlogin_spawn_session_buffer($remove_spawn_id,pid)
      set nr_shells $rlogin_spawn_session_buffer($remove_spawn_id,nr_shells)
      del_open_spawn_rlogin_session $remove_spawn_id
      close_spawn_process "$pid $remove_spawn_id $nr_shells" 1
   }
}

#****** remote_procedures/get_open_spawn_rlogin_session() **********************
#  NAME
#     get_open_spawn_rlogin_session() -- get rlogin connection data
#
#  SYNOPSIS
#     get_open_spawn_rlogin_session {hostname user back_var} 
#
#  FUNCTION
#     This procedure returns the corresponding spawn_id for hostname and user
#     name.
#
#     See remote_procedures/add_open_spawn_rlogin_session() for a description
#     of the data structures.
#
#  INPUTS
#     hostname       - hostname of rlogin connection 
#     user           - user who logged in 
#     win_local_user - is the user a windows local (1) or domain user (0)
#     back_var       - name of array to store data in
#                      (the array has following names:
#                         back_var(spawn_id) 
#                         back_var(pid)     
#                         back_var(hostname) 
#                         back_var(user))
#
#  RESULT
#     1 on success, 0 if no connection is available
#
#  EXAMPLE
#     get_open_spawn_rlogin_session $hostname $user $win_local_user con_data
#     if { $con_data(pid) != 0 } {
#     }
#
#  NOTES
#     The back_var array is set to "0" if no connection is available 
#
#  SEE ALSO
#     remote_procedures/add_open_spawn_rlogin_session
#     remote_procedures/get_open_spawn_rlogin_session
#     remote_procedures/get_spawn_id_rlogin_session
#     remote_procedures/check_rlogin_session
#*******************************************************************************
proc get_open_spawn_rlogin_session {hostname user win_local_user back_var} {
   global CHECK_OUTPUT rlogin_spawn_session_buffer rlogin_spawn_session_idx
   global do_close_rlogin

   upvar $back_var back 

   # we shall not reuse connections, or
   # connection to this host/user does not exist yet
   if {$do_close_rlogin != 0 || ![info exists rlogin_spawn_session_idx($hostname,$user,$win_local_user)]} {
      clear_open_spawn_rlogin_session back
      return 0 
   }

   set spawn_id $rlogin_spawn_session_idx($hostname,$user,$win_local_user)
   set back(spawn_id)       $spawn_id
   set back(pid)            $rlogin_spawn_session_buffer($spawn_id,pid)
   set back(hostname)       $rlogin_spawn_session_buffer($spawn_id,hostname)
   set back(user)           $rlogin_spawn_session_buffer($spawn_id,user)
   set back(win_local_user) $rlogin_spawn_session_buffer($spawn_id,win_local_user)
   set back(ltime)          $rlogin_spawn_session_buffer($spawn_id,ltime)
   set back(nr_shells)      $rlogin_spawn_session_buffer($spawn_id,nr_shells)
   
   debug_puts "spawn_id  :      $back(spawn_id)"
   debug_puts "pid       :      $back(pid)"
   debug_puts "hostname  :      $back(hostname)"
   debug_puts "user      :      $back(user)"
   debug_puts "win_local_user : $back(win_local_user)"
   debug_puts "ltime     :      $back(ltime)"
   debug_puts "nr_shells :      $back(nr_shells)"

   return 1
}

#****** remote_procedures/del_open_spawn_rlogin_session() **********************
#  NAME
#     del_open_spawn_rlogin_session() -- remove rlogin session
#
#  SYNOPSIS
#     del_open_spawn_rlogin_session { spawn_id } 
#
#  FUNCTION
#     Removes a certain session from the internal bookkeeping.
#
#     See remote_procedures/add_open_spawn_rlogin_session() for a description
#     of the data structures.
#
#  INPUTS
#     spawn_id - spawn id to remove
#
#  SEE ALSO
#     remote_procedures/add_open_spawn_rlogin_session()
#*******************************************************************************
proc del_open_spawn_rlogin_session {spawn_id} {
   global rlogin_spawn_session_buffer rlogin_spawn_session_idx

   if {[info exists rlogin_spawn_session_buffer($spawn_id,pid)]} {
      # remove session from search index
      set hostname       $rlogin_spawn_session_buffer($spawn_id,hostname)
      set user           $rlogin_spawn_session_buffer($spawn_id,user)
      set win_local_user $rlogin_spawn_session_buffer($spawn_id,win_local_user)
      unset rlogin_spawn_session_idx($hostname,$user,$win_local_user)

      # remove session from array index
      set pos [lsearch -exact $rlogin_spawn_session_buffer(index) $spawn_id]
      set rlogin_spawn_session_buffer(index) [lreplace $rlogin_spawn_session_buffer(index) $pos $pos]

      # remove session data
      unset rlogin_spawn_session_buffer($spawn_id,pid)
      unset rlogin_spawn_session_buffer($spawn_id,hostname)
      unset rlogin_spawn_session_buffer($spawn_id,user)
      unset rlogin_spawn_session_buffer($spawn_id,win_local_user)
      unset rlogin_spawn_session_buffer($spawn_id,ltime)
      unset rlogin_spawn_session_buffer($spawn_id,nr_shells)
      unset rlogin_spawn_session_buffer($spawn_id,in_use)
   }
}

#****** remote_procedures/is_spawn_id_rlogin_session() *************************
#  NAME
#     is_spawn_id_rlogin_session() -- does a certain session exist?
#
#  SYNOPSIS
#     is_spawn_id_rlogin_session { spawn_id } 
#
#  FUNCTION
#     Returns if the given expect spawn id exists in the connection bookkeeping.
#
#  INPUTS
#     spawn_id - spawn id to check
#
#  RESULT
#     1, if the connection exists, else 0
#
#  SEE ALSO
#     remote_procedures/add_open_spawn_rlogin_session()
#*******************************************************************************
proc is_spawn_id_rlogin_session {spawn_id} {
   global rlogin_spawn_session_buffer

   if {[info exists rlogin_spawn_session_buffer($spawn_id,pid)]} {
      return 1
   }

   return 0
}

#****** remote_procedures/get_open_rlogin_sessions() ***************************
#  NAME
#     get_open_rlogin_sessions() -- return list of all spawn ids
#
#  SYNOPSIS
#     get_open_rlogin_sessions { } 
#
#  FUNCTION
#     Returns a list of all spawn ids in the internal bookkeeping.
#
#  RESULT
#     list of spawn ids
#
#  SEE ALSO
#     remote_procedures/add_open_spawn_rlogin_session()
#*******************************************************************************
proc get_open_rlogin_sessions {} {
   global rlogin_spawn_session_buffer

   return [lsort -dictionary $rlogin_spawn_session_buffer(index)]
}

#
# internal utility function used by get_spawn_*
# clears the return buffer in case of errors / connection not found
#
proc clear_open_spawn_rlogin_session {back_var} {
   upvar $back_var back

   set back(spawn_id)       "0"
   set back(pid)            "0"
   set back(hostname)       "0"
   set back(user)           "0"
   set back(win_local_user) "0"
   set back(ltime)           0
   set back(nr_shells)       0
}

#****** remote_procedures/get_spawn_id_rlogin_session() ************************
#  NAME
#     get_spawn_id_rlogin_session() -- get rlogin connection data
#
#  SYNOPSIS
#     get_spawn_id_rlogin_session { id back_var } 
#
#  FUNCTION
#     This procedure returns the corresponding data for a rlogin spawn id
#
#     See remote_procedures/add_open_spawn_rlogin_session() for a description
#     of the data structures.
#
#  INPUTS
#     id       - spawn id of connection
#     back_var - name of array to store data in
#                (the array has following names:
#                 back_var(spawn_id) 
#                 back_var(pid)     
#                 back_var(hostname) 
#                 back_var(user))
#
#
#  RESULT
#     1 on success, 0 if no connection is available
#
#  EXAMPLE
#     get_spawn_id_rlogin_session $sp_id con_data
#       if { $con_data(pid) != 0 } { ...}
#
#  NOTES
#     The back_var array is set to "0" if no connection is available 
#
#  SEE ALSO
#     remote_procedures/add_open_spawn_rlogin_session
#     remote_procedures/get_open_spawn_rlogin_session
#     remote_procedures/get_spawn_id_rlogin_session
#     remote_procedures/check_rlogin_session
#*******************************************************************************
proc get_spawn_id_rlogin_session {spawn_id back_var} {
   global CHECK_OUTPUT rlogin_spawn_session_buffer
   global do_close_rlogin

   upvar $back_var back 

   if {$do_close_rlogin || ![info exists rlogin_spawn_session_buffer($spawn_id,pid)]} {
      clear_open_spawn_rlogin_session back
      return 0 
   }

   set back(spawn_id)       $spawn_id
   set back(pid)            $rlogin_spawn_session_buffer($spawn_id,pid)
   set back(hostname)       $rlogin_spawn_session_buffer($spawn_id,hostname)
   set back(user)           $rlogin_spawn_session_buffer($spawn_id,user)
   set back(win_local_user) $rlogin_spawn_session_buffer($spawn_id,win_local_user)
   set back(ltime)          $rlogin_spawn_session_buffer($spawn_id,ltime)
   set back(nr_shells)      $rlogin_spawn_session_buffer($spawn_id,nr_shells)
   
   debug_puts "spawn_id       : $back(spawn_id)"
   debug_puts "pid            : $back(pid)"
   debug_puts "hostname       : $back(hostname)"
   debug_puts "user           : $back(user)"
   debug_puts "win_local_user : $back(win_local_user)"
   debug_puts "ltime          : $back(ltime)"
   debug_puts "nr_shells      : $back(nr_shells)"

   return 1 
}

#****** remote_procedures/close_open_rlogin_sessions() *************************
#  NAME
#     close_open_rlogin_sessions() -- close all open rlogin sessions
#
#  SYNOPSIS
#     close_open_rlogin_sessions { } 
#
#  FUNCTION
#     This procedure closes all open rlogin expect sessions.
#
#*******************************************************************************
proc close_open_rlogin_sessions { { if_not_working 0 } } {
   global CHECK_OUTPUT
   global do_close_rlogin

   # if we called testsuite with option close_rlogin, we have no open sessions
   if { $do_close_rlogin != 0 } {
      puts $CHECK_OUTPUT "close_open_rlogin_sessions - open rlogin session mode not activated!"
      return 0 
   }

   # gather all session names
   set sessions [get_open_rlogin_sessions]

   # close all sessions
   foreach spawn_id $sessions {
      get_spawn_id_rlogin_session $spawn_id back
      
      if {$if_not_working} {
         if {[check_rlogin_session $spawn_id $back(pid) $back(hostname) $back(user) $back(nr_shells) 1]} {
            puts $CHECK_OUTPUT "will not close spawn id $spawn_id - session is ok!"
            continue
         }
      }

      del_open_spawn_rlogin_session $spawn_id
      puts $CHECK_OUTPUT "close_open_rlogin_sessions - closing $spawn_id"
      close_spawn_process "$back(pid) $spawn_id $back(nr_shells)" 1 ;# don't check exit state
   }
}

#****** remote_procedures/check_rlogin_session() *******************************
#  NAME
#     check_rlogin_session() -- check if rlogin session is alive
#
#  SYNOPSIS
#     check_rlogin_session { spawn_id pid hostname user } 
#
#  FUNCTION
#     This procedure checks if the given rlogin spawn session is alive. If not
#     it will close the session.
#
#  INPUTS
#     spawn_id - spawn process id
#     pid      - process id of rlogin session
#     hostname - hostname of rlogin connection 
#     user     - user who logged in 
#     { only_check 0 } - if not 0: don't close spawn session on error
#
#  RESULT
#     1 on success, 0 if no connection is available
#
#  SEE ALSO
#     remote_procedures/add_open_spawn_rlogin_session
#     remote_procedures/get_open_spawn_rlogin_session
#     remote_procedures/get_spawn_id_rlogin_session
#     remote_procedures/check_rlogin_session
#*******************************************************************************
proc check_rlogin_session { spawn_id pid hostname user nr_of_shells {only_check 0} {raise_error 1}} {
   global CHECK_OUTPUT CHECK_USER CHECK_TESTSUITE_ROOT CHECK_SCRIPT_FILE_DIR

   debug_puts "check_rlogin_session: $spawn_id $pid $hostname $user $nr_of_shells $only_check"
   if {![is_spawn_id_rlogin_session $spawn_id]} {
      # connection is not open
      debug_puts "check_rlogin_session: connection is not open"
      return 0
   }

   # handle special user ids
   get_spawn_id_rlogin_session $spawn_id con_data
   map_special_users $hostname $user $con_data(win_local_user)

   # perform the following test:
   # - start the check_identity.sh script
   # - wait for correct output
   set connection_ok 0
   set catch_return [catch {
      send -i $spawn_id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/check_identity.sh\n"
      set num_tries 15
      set timeout 2
      expect {
         -i $spawn_id full_buffer {
            add_proc_error "check_rlogin_session" -2 "buffer overflow" $raise_error
         }
         -i $spawn_id eof {
            add_proc_error "check_rlogin_session" -2 "unexpected eof" $raise_error
         }
         -i $spawn_id timeout {
            incr num_tries -1
            if {$num_tries > 0} {
               if {$num_tries < 12} {
                  puts -nonewline $CHECK_OUTPUT "." 
                  flush $CHECK_OUTPUT
               }
               send -i $spawn_id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/check_identity.sh\n"
               increase_timeout
               exp_continue
            } else {
               add_proc_error "check_rlogin_session" -2 "timeout waiting for shell response" $raise_error
            }
         }
         -i $spawn_id "__ my id is ->*${real_user}*\n" {
            debug_puts "connection is ok"
            set connection_ok 1
         }
      }
   } catch_error_message]
   if { $catch_return == 1 } {
      add_proc_error "check_rlogin_session" -2 "$catch_error_message" $raise_error
   }

   # are we done?
   if {$connection_ok} {
      return 1
   }

   # if we got here, there was an error
   # in case we shall not only check, but also react on errors,
   # we'll close the connection now
   if {$only_check == 0} {
      puts $CHECK_OUTPUT "check_rlogin_session: closing $spawn_id $pid to enable new rlogin session ..."

      # unregister connection
      del_open_spawn_rlogin_session $spawn_id
      close_spawn_process "$pid $spawn_id $nr_of_shells"
   }

   return 0 ;# error
}

#****** remote_procedures/set_spawn_process_in_use() ***************************
#  NAME
#     set_spawn_process_in_use() -- set info if a session is in use
#
#  SYNOPSIS
#     set_spawn_process_in_use { spawn_id {in_use 1} } 
#
#  FUNCTION
#     Stores the information, if a certain session is in use or not, in the 
#     internal bookkeeping.
#
#  INPUTS
#     spawn_id   - ??? 
#     {in_use 1} - ??? 
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
proc set_spawn_process_in_use {spawn_id {in_use 1}} {
   global rlogin_spawn_session_buffer

   set rlogin_spawn_session_buffer($spawn_id,in_use) $in_use
   set rlogin_spawn_session_buffer($spawn_id,ltime) [timestamp]
}

#****** remote_procedures/is_spawn_process_in_use() ****************************
#  NAME
#     is_spawn_process_in_use() -- check if spawn id is in use
#
#  SYNOPSIS
#     is_spawn_process_in_use { spawn_id } 
#
#  FUNCTION
#     Test if given spawn id is already in use
#
#     See remote_procedures/add_open_spawn_rlogin_session() for a description
#     of the data structures.
#
#  INPUTS
#     spawn_id - internal spawn id number from open_remote_spawn_process()
#
#  RESULT
#     0    : not in use
#     not 0: this spawn id is in use
#
#  SEE ALSO
#     remote_procedures/open_remote_spawn_process()
#     remote_procedures/add_open_spawn_rlogin_session()
#*******************************************************************************
proc is_spawn_process_in_use {spawn_id} {
   global rlogin_spawn_session_buffer

   if {[info exists rlogin_spawn_session_buffer($spawn_id,pid)]} {
      return $rlogin_spawn_session_buffer($spawn_id,in_use)
   }

   return 0
}

#                                                             max. column:     |
#****** remote_procedures/close_spawn_process() ******
# 
#  NAME
#     close_spawn_process -- close open spawn process id 
#
#  SYNOPSIS
#     close_spawn_process { id { check_exit_state 0 } } 
#
#  FUNCTION
#     This procedure will close the process associated with the spawn id
#     returned from the procedures open_spawn_process or open_root_spawn_process.
#
#     Sends a CTRL-C to the session to terminate possibly still running
#     processes.
#
#  INPUTS
#     id - spawn process id (returned from open_spawn_process or 
#          open_root_spawn_process)
#     { check_exit_state 0 } - if 0: check exit state
#
#  RESULT
#     exit state of the "spawned" process
#
#  EXAMPLE
#     see open_root_spawn_process or open_spawn_process 
#
#  NOTES
#     After a process is "spawned" with the  open_spawn_process procedure it 
#     must be closed with the close_spawn_process procedure. id is the return 
#     value of open_spawn_process or open_root_spawn_process.
#     If a open spawn process id is not closed, it will not free the file
#     descriptor for that id. If all file descriptors are used, no new spawn
#     process can be forked!
#
#  SEE ALSO
#     remote_procedures/open_spawn_process
#     remote_procedures/open_root_spawn_process
#     remote_procedures/close_spawn_process
#     remote_procedures/start_remote_tcl_prog
#     remote_procedures/start_remote_prog
#*******************************
proc close_spawn_process {id {check_exit_state 0}} {
   global CHECK_OUTPUT CHECK_DEBUG_LEVEL
   global CHECK_SHELL_PROMPT
 
   set pid      [lindex $id 0]
   set spawn_id [lindex $id 1]
   if {[llength $id] > 2} {
      set nr_of_shells [lindex $id 2]
   } else {
      set nr_of_shells 0
   }

   debug_puts "close_spawn_process: closing $spawn_id $pid"

   # in debug mode we want to see all the shell output
   log_user 0
   if {$CHECK_DEBUG_LEVEL != 0} {
      log_user 1
   }

   # get connection info
   get_spawn_id_rlogin_session $spawn_id con_data

   # regular call from a check.
   # in case we shall close the connection after an error, con_data(pid) will be 0
   # there might still be a program running
   # stop it by sending CTRL C and mark the connection as idle
   set do_return ""
   if {$con_data(pid) != 0} {
      # mark the connection idle
      set_spawn_process_in_use $spawn_id 0

      # if we have code coverage analysis enabled, give the process
      # some time to finish writing coverage data
      # hopefully one second is enough
      if {[coverage_enabled]} {
         sleep 2
      }

      # stop still remaining running processes and wait for shell prompt
      set catch_return [catch {
         debug_puts "close_spawn_process: sending CTRL-C"
         send -i $spawn_id -- "\003" ;# CTRL-C
         set timeout 2
         set num_tries 10
         expect {
            -i $spawn_id eof {
               add_proc_error "close_spawn_process (regular close)" -2 "unexpected eof"
               close_spawn_id $spawn_id
            }
            -i $spawn_id full_buffer {
               add_proc_error "close_spawn_process (regular close)" -2 "buffer overflow"
               close_spawn_id $spawn_id
            }
            -i $spawn_id timeout {
               incr num_tries -1
               if {$num_tries > 0} {
                  debug_puts "close_spawn_process: sending CTRL-C"
                  send -i $spawn_id -- "\003" ;# CTRL-C
                  increase_timeout
                  exp_continue
               } else {
                  add_proc_error "close_spawn_process (regular close)" -2 "timeout waiting for shell prompt"
               }
            }
            -i $spawn_id -re $CHECK_SHELL_PROMPT {
               debug_puts "got shell prompt"
               set do_return -1
            }
         }
      } catch_error_message]
      if {$catch_return == 1} {
         add_proc_error "close_spawn_process (regular close)" -2 "$catch_error_message" 
      }
      
      # are we done?
      if {$do_return != ""} {
         return $do_return
      }
      
      # if we get here, we ran into an error
      # we will not return, but continue, really closing the connection
   }

   # we have shells to close (by sending exit)
   if {$nr_of_shells > 0} {
      debug_puts "nr of open shells: $nr_of_shells"
      debug_puts "-->sending $nr_of_shells exit(s) to shell on id $spawn_id"

      set catch_return [catch {
         # send CTRL-C to stop poss. still running processes
         send -i $spawn_id -- "\003"
         
         # wait for CTRL-C to take effect
         set timeout 5
         expect {
            -i $spawn_id full_buffer {
               add_proc_error "close_spawn_process (exit)" "-2" "buffer overflow"
            }
            -i $spawn_id eof {
               # do not raise an error here - we use this code to close broken connections
               debug_puts "eof while waiting for shell prompt after CTRL-C"
            }
            -i $spawn_id timeout {
               # do not raise an error here - we use this code to close broken connections
               debug_puts "timeout while waiting for shell prompt after CTRL-C"
            }
            -i $spawn_id -re $CHECK_SHELL_PROMPT {
               debug_puts "got shell prompt after CTRL-C"
            }
         }

         # now we try to close the shells with "exit"
         send -i $spawn_id -- "exit\n"
         expect {
            -i $spawn_id full_buffer {
               add_proc_error "close_spawn_process (exit)" "-2" "buffer overflow"
            }
            -i $spawn_id eof {
               # do not raise an error here - we use this code to close broken connections
               debug_puts "eof after exit - ok"
            }
            -i $spawn_id timeout {
               # do not raise an error here - we use this code to close broken connections
               debug_puts "timeout while waiting for shell prompt after exit"
            }
            -i $spawn_id -re $CHECK_SHELL_PROMPT {
               debug_puts "got shell prompt after exit"
               # if we get a shell prompt, the exit succeeded, one shell exited
               incr nr_of_shells -1

               # if we still have open shells, send "exit"
               if {$nr_of_shells > 0} {
                  send -i $spawn_id -- "exit\n"
                  exp_continue
               } else {
                  debug_puts "all shells exited - ok"
               }
            }
         }
      } catch_error_message]
      if {$catch_return == 1} {
         add_proc_error "close_spawn_process (exit)" -2 "$catch_error_message" 
      }
   }

   # unregister connection
   del_open_spawn_rlogin_session $spawn_id

   # now shutdown the spawned process
   set catch_return [catch {
      debug_puts "closing $spawn_id"
      close -i $spawn_id
   } catch_error_message]
   if {$catch_return == 1} {
      add_proc_error "close_spawn_process (close)" -2 "$catch_error_message" 
   }

   # wait for spawned process to exit
   set wait_code 0
   set catch_return [catch {
      set wait_return   [wait -i $spawn_id]
      set wait_pid      [lindex $wait_return 0]
      set wait_spawn_id [lindex $wait_return 1]
      set wait_error    [lindex $wait_return 2]
      set wait_code     [lindex $wait_return 3]

      debug_puts "closed buffer   : $spawn_id"
      debug_puts "wait pid        : $wait_pid"
      debug_puts "wait spawn id   : $wait_spawn_id"
      debug_puts "wait error      : $wait_error (-1 = operating system error, 0 = exit)"
      debug_puts "wait code       : $wait_code  (os error code or exit status)"

      # if requested by caller, do certain error checks:
      if {$check_exit_state == 0} {
         # did we close the correct spawn id?
         if {$spawn_id != $wait_spawn_id} {
            add_proc_error "close_spawn_process (wait)" "-2" "closed wrong spawn id: expected $spawn_id, but got $wait_spawn_id"
         }

         # did we close the correct pid?
         if {$pid != $wait_pid} {
            add_proc_error "close_spawn_process (wait)" "-2" "closed wrong pid: expected $pid, but got $wait_pid"
         }

         # on regular exit: check exit code, shall be 0
         if {$wait_error == 0} {
            if {$wait_code != 0} {
               add_proc_error "close_spawn_process (wait)" -2 "wait exit status: $wait_code"
            }
         } else {
            debug_puts "*** operating system error: $wait_code"
            debug_puts "spawn id: $wait_spawn_id"
            debug_puts "wait pid: $wait_pid"
            add_proc_error "close_spawn_process (wait)" -1 "operating system error: $wait_code"
         }
      }
   } catch_error_message]
   if {$catch_return == 1} {
      add_proc_error "close_spawn_process (wait)" -2 "$catch_error_message" 
   }

   return $wait_code ;# return exit state
}

