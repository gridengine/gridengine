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
global rlogin_in_use_buffer last_shell_script_file last_spawn_command_arguments

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
   set host_list {}
   set return_value 0
   foreach host $ts_config(execd_nodes) {
      lappend host_list $host
   }
   foreach host $ts_config(shadowd_hosts) {
      if { [string compare $host "none"] != 0 } {
         if { [lsearch $host_list $host] == -1 } {
            lappend host_list $host
         }
      }
   }
   foreach host $ts_config(submit_only_hosts) {
      if { [string compare $host "none"] != 0 } {
         if { [lsearch $host_list $host] == -1 } {
            lappend host_list $host
         }
      }
   }
   if { [lsearch $host_list $ts_config(master_host)] == -1 } {
      lappend host_list $ts_config(master_host)
   }

   foreach host $host_list {
      puts $CHECK_OUTPUT "test connection to $host ..."
      set result [string trim [start_remote_prog $host $CHECK_USER "echo" "hallo"]]
      puts $CHECK_OUTPUT $result
   }

   set test_start [timestamp]
   foreach host $host_list {
      set tcl_bin [ get_binary_path $host "expect"]
      puts $CHECK_OUTPUT "test remote system time on host $host ..."
      set result [string trim [start_remote_prog $host $CHECK_USER $tcl_bin "-c 'puts \"current time is \[timestamp\]\"'"]]
      puts $CHECK_OUTPUT $result
      set time($host) [get_string_value_between "current time is" -1 $result]
      # fix remote execution time difference
      set time($host) [expr ( $time($host) - [expr ( [timestamp] - $test_start ) ] )] 
   }

   set reverence_time $time($ts_config(master_host))
   foreach host $host_list {
      set diff [expr ( $reverence_time - $time($host) )]
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
                         { do_file_check 1 } 
                         { source_settings_file 1 } 
                         { set_shared_lib_path 1 } 
                       } {
   global CHECK_OUTPUT CHECK_MAIN_RESULTS_DIR CHECK_DEBUG_LEVEL 
   global open_spawn_buffer CHECK_HOST
   upvar $exit_var back_exit_state

   if {$envlist != ""} {
      upvar $envlist users_env
   }
   
   set back_exit_state -1
   set tmp_exit_status_string ""
   if { [ llength $exec_command ] != 1 } {
      puts "= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = ="
      puts "  WARNING     WARNING   WARNING  WARNING"
      puts "  procedure start_remote_prog: \"$exec_command\""
      puts "  is not a command name; it has additional arguments" 
      puts "= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = ="
     if { $CHECK_DEBUG_LEVEL == 2 } {
         wait_for_enter 
     }
   }
   

   set id [open_remote_spawn_process "$hostname" "$user" "$exec_command" "$exec_arguments" $background users_env $source_settings_file 15 $set_shared_lib_path ]
   if { [string compare $id ""] == 0 } {
      add_proc_error "start_remote_prog" -1 "got no spawn id"
      set back_exit_state -255
      return ""
   }
   set myspawn_id [ lindex $id 1 ]



   set output ""
   set do_stop 0

   if { $CHECK_DEBUG_LEVEL != 0 } {
      log_user 1
   } else {
      log_user 0
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
                    add_proc_error "start_remote_prog" -1 "unexpected error - did not get full exit status string"
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
        add_proc_error "start_remote_prog" "-1" "timeout error(1):\nmaybe the shell is expecting an interactive answer from user?\nexec commando was: \"$exec_command $exec_arguments\"\n$expect_out(buffer)\nmore information in next error message in 5 seconds!!!"
        set timeout 5
        expect {
           -i $myspawn_id full_buffer {
              add_proc_error "start_remote_prog" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
           }
           -i $myspawn_id timeout {
              add_proc_error "start_remote_prog" "-1" "no more output available"
           }
           -i $myspawn_id "*" {
              add_proc_error "start_remote_prog" "-1" "expect buffer:\n$expect_out(buffer)"
           }
           -i $myspawn_id default {
              add_proc_error "start_remote_prog" "-1" "default - no more output available"
           }
        }
     }

     -i $myspawn_id full_buffer {
        add_proc_error "start_remote_prog" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
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
  if { $catch_return != 0 } {
     set err_string "catch error: error reading error_array"
  }
  
  set err_complete  [split $err_string "|"]
  set err_procedure [ lindex $err_complete 0 ]
  set err_text      [ lindex $err_complete 1 ]
  set err_checkname [ lindex $err_complete 2 ]
  set err_calledby  [ lindex $err_complete 3 ]
  
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
#     ???/???
#*******************************************************************************
proc open_remote_spawn_process { hostname 
                                 user 
                                 exec_command 
                                 exec_arguments 
                                 { background 0 } 
                                 { envlist "" } 
                                 { source_settings_file 1 } 
                                 { nr_of_tries 15 } 
                                 { set_shared_lib_path 1 }
                               } {

  global open_spawn_buffer CHECK_OUTPUT CHECK_HOST CHECK_USER CHECK_TESTSUITE_ROOT CHECK_SCRIPT_FILE_DIR
  global CHECK_MAIN_RESULTS_DIR CHECK_EXPECT_MATCH_MAX_BUFFER
  global rlogin_in_use_buffer last_shell_script_file last_spawn_command_arguments

  upvar 1 error_info error_info

  if {$envlist != ""} {
      upvar $envlist users_env
  }

  set this_last_spawn_command_arguments "$hostname$user$exec_command$exec_arguments$background$envlist$source_settings_file$nr_of_tries$set_shared_lib_path"
  set re_use_script 0
  if { [info exists last_spawn_command_arguments]} {
     if { [string compare $this_last_spawn_command_arguments $last_spawn_command_arguments] == 0 } {
        set re_use_script 1
     } 
#     puts $CHECK_OUTPUT "last commands: $last_spawn_command_arguments"
  }
#  puts $CHECK_OUTPUT "new  commands: $this_last_spawn_command_arguments"
  

  set last_spawn_command_arguments $this_last_spawn_command_arguments

  uplevel 1 { set remote_spawn_nr_of_shells 0 }

  uplevel 1 { global open_spawn_buffer }
  uplevel 1 { global CHECK_USER }
  uplevel 1 { global CHECK_TESTSUITE_ROOT }
  uplevel 1 { global CHECK_SCRIPT_FILE_DIR }
  uplevel 1 { global CHECK_MAIN_RESULTS_DIR }
  uplevel 1 { global CHECK_DEBUG_LEVEL }
  uplevel 1 { global CHECK_EXPECT_MATCH_MAX_BUFFER }
  uplevel 1 { global CHECK_OUTPUT }


  debug_puts "open_remote_spawn_process on host \"$hostname\""
  debug_puts "user:           $user"
  debug_puts "exec_command:   $exec_command"
  debug_puts "exec_arguments: $exec_arguments"

  set error_info "connection to host \"$hostname\" as user \"$user\""

  if { [string compare $user $CHECK_USER] != 0 } {
     if { [string match "ts_def_con*" $user] != 1 } {
        if {[have_root_passwd] == -1} {
            add_proc_error "open_remote_spawn_process" -2 "${error_info}\nroot access required"
            return "" 
         }
     }
  }

  set type [ file tail $exec_command ]

  if { $re_use_script != 0 } {
     if { [file isfile $last_shell_script_file] == 0 } {
        set re_use_script 0
     }
  }

  if { $re_use_script == 0 } {
     set script_name [get_tmp_file_name $hostname $type "sh" ]
     create_shell_script "$script_name" $hostname "$exec_command" "$exec_arguments" users_env "/bin/sh" 0 $source_settings_file $set_shared_lib_path
     debug_puts "created $script_name"
  } else {
     set script_name $last_shell_script_file
     debug_puts "re-using script $script_name"
  }

  set last_shell_script_file $script_name
  set open_spawn_buffer $script_name
  uplevel 1 { set open_remote_spawn__script_name $open_spawn_buffer }
  set open_spawn_buffer $hostname
  uplevel 1 { set open_remote_spawn__hostname $open_spawn_buffer }
  set open_spawn_buffer $user
  uplevel 1 { set open_remote_spawn__user $open_spawn_buffer }
  set open_spawn_buffer $CHECK_USER
  uplevel 1 { set open_remote_spawn__check_user $open_spawn_buffer }
  set open_spawn_buffer $exec_command
  uplevel 1 { set open_remote_spawn__prog "$open_spawn_buffer" }
  set open_spawn_buffer $exec_arguments
  uplevel 1 { set open_remote_spawn__args "$open_spawn_buffer" }
  set open_spawn_buffer $nr_of_tries
  uplevel 1 {
     set open_remote_spawn__tries "$open_spawn_buffer" 
     log_user 0
     if { $CHECK_DEBUG_LEVEL != 0 } {
        log_user 1
     }
     set timeout 120
  }

  if { $re_use_script == 0 } {
     get_open_spawn_rlogin_session $hostname $user con_data
  } else {
     get_open_spawn_rlogin_session $hostname $user con_data 1
  }
  set using_ts_def_con 0


  switch -- $user {
     "ts_def_con" {
        set user $CHECK_USER
        set using_ts_def_con 1
        set open_spawn_buffer $user
        uplevel 1 { set open_remote_spawn__user $open_spawn_buffer }
     }
     "ts_def_con2" {
        set user $CHECK_USER
        set using_ts_def_con 2
        set open_spawn_buffer $user
        uplevel 1 { set open_remote_spawn__user $open_spawn_buffer }
     }
     "ts_def_con_translate" {
        set user $CHECK_USER
        set using_ts_def_con 3
        set open_spawn_buffer $user
        uplevel 1 { set open_remote_spawn__user $open_spawn_buffer }
     }
  }

  if { $con_data(pid) != 0 } {
     debug_puts "Using open rlogin connection to host \"$hostname\",user \"$user\""

     set nr_of_shells $con_data(nr_shells)
     set back  $con_data(pid) 
     lappend back $con_data(spawn_id)
     set open_spawn_buffer $con_data(spawn_id)
     if { [is_spawn_process_in_use $back ] == 1 } {
        puts $CHECK_OUTPUT "open_remote_spawn_process($hostname $user $exec_command $exec_arguments):"
        puts $CHECK_OUTPUT "connection already in use or an old process wasn't closed."
        puts $CHECK_OUTPUT "Please use an other username or call close_spawn_process first."
     }
     uplevel 1 { 
        set open_remote_spawn__id "$open_spawn_buffer" 
     }
  } else {
     uplevel 1 { debug_puts "opening connection to host $open_remote_spawn__hostname" }
     if { [have_ssh_access] == 0 } {
        set pid [ uplevel 1 { spawn "rlogin" "$open_remote_spawn__hostname" } ] 
        uplevel 1 { incr remote_spawn_nr_of_shells 1 }
      } else {
         set ssh_binary [get_binary_path $CHECK_HOST ssh]
         uplevel 1 "set ssh_binary $ssh_binary"
         set pid [ uplevel 1 { spawn "$ssh_binary" "-l" "root" "$open_remote_spawn__hostname" } ]
         uplevel 1 { incr remote_spawn_nr_of_shells 1 }
      }
      set sp_id [uplevel 1 { set spawn_id }]
      set back $pid      ;# return value (pid and spawn_id)
      lappend back $sp_id
      set open_spawn_buffer $sp_id
      uplevel 1 { set open_remote_spawn__id "$open_spawn_buffer" }

      if {$pid == 0 } {
        add_proc_error "open_remote_spawn_process" -2 "${error_info}\ncould not spawn! (ret_pid = $pid)" 
        return "" 
      }

      match_max -i $sp_id $CHECK_EXPECT_MATCH_MAX_BUFFER
      debug_puts "open_remote_spawn_process -> buffer size is: [match_max -i $sp_id]"
      # wait for shell to start
      set catch_return [ catch {
          uplevel 1 {
             log_user 0
             if { $CHECK_DEBUG_LEVEL != 0 } {
                log_user 1
             }
             set my_tries 80
             while { 1 } {
                set timeout 1
                expect {
                   -i $spawn_id full_buffer {
                      add_proc_error "open_remote_spawn_process" -1 "${error_info}\nbuffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
                      catch { close -i $spawn_id }
                      catch { wait -nowait -i $spawn_id }
                      return ""
                   }
                   -i $spawn_id -- "The authenticity of host*" {
                      after 100
                      send -i $spawn_id "yes\n"
                   }
                   -i $spawn_id -- "Are you sure you want to continue connecting (yes/no)?*" {
                      send -i $spawn_id "yes\n"
                   }
                   -i $spawn_id -- "Please type 'yes' or 'no'*" {
                      send -i $spawn_id "yes\n"
                   }
                   -i $spawn_id -- {[A-Za-z>$%]*} {
                       debug_puts "startup ..."
                       break
                   }
                   -i $spawn_id default {
                       if { $my_tries > 0 } {
                           incr my_tries -1
                           if { $my_tries < 77 } {
                              puts -nonewline $CHECK_OUTPUT "."
                              flush $CHECK_OUTPUT
                           }
                           continue
                       } else {
                          add_proc_error "open_remote_spawn_process" -1 "${error_info}\nstartup timeout" 
                          catch { close -i $spawn_id }
                          catch { wait -nowait -i $spawn_id }
                          return ""
                       }
                   }
                }
             }
             
             set timeout 1

             send -i $spawn_id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/shell_start_output.sh\n"
             set open_remote_spawn__tries 30
             while { $open_remote_spawn__tries > 0 } {
                expect {
                  -i $spawn_id full_buffer {
                     add_proc_error "open_remote_spawn_process" -1 "${error_info}\nbuffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
                     catch { close -i $spawn_id }
                     return ""
                  }
                  -i $spawn_id timeout {
                      send -i $spawn_id "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/shell_start_output.sh\n"
                      incr open_remote_spawn__tries -1
                  }  
                   -i $spawn_id -- "The authenticity of host*" {
                      after 100
                      send -i $spawn_id "yes\n"
                   }
                   -i $spawn_id -- "Are you sure you want to continue connecting (yes/no)?*" {
                      send -i $spawn_id "yes\n"
                   }
                   -i $spawn_id -- "Please type 'yes' or 'no'*" {
                      send -i $spawn_id "yes\n"
                   }
                   -i $spawn_id -- "ts_shell_response*\n" {
                      break
                   }
                   -i $spawn_id eof {
                      add_proc_error "open_remote_spawn_process" -2 "${error_info}\nunexpected eof"
                      catch { close -i $spawn_id }
                      catch { wait -nowait -i $spawn_id }
                      return ""
                   }
                }
            }
            if { $open_remote_spawn__tries <= 0 } {
                add_proc_error "open_remote_spawn_process" -1 "${error_info}\ntimeout waiting for shell response prompt (b)"
                catch { send -i $spawn_id "\003" } ;# send CTRL+C to stop poss. running processes
                puts $CHECK_OUTPUT "closing spawn process (1) ..."
                flush $CHECK_OUTPUT
                catch { close -i $spawn_id }
                catch { wait -nowait -i $spawn_id }
                return ""
            }
             

             set mytries $open_remote_spawn__tries
             debug_puts "waiting for shell response ..."
             set timeout 1
             set next_timeout 1
             set ok 0
             send -i $spawn_id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/check_identity.sh\n"
             while { $ok != 1 } {
                expect {
                   -i $spawn_id full_buffer {
                      add_proc_error "open_remote_spawn_process" -1 "${error_info}\nbuffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
                      catch { close -i $spawn_id }
                      catch { wait -nowait -i $spawn_id }
                      return ""
                   }
                   -i $spawn_id timeout {
                      puts -nonewline $CHECK_OUTPUT "   \r$mytries\r"
                      flush $CHECK_OUTPUT

                      send -i $spawn_id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/check_identity.sh\n"
                      set timeout $next_timeout
                      if { $next_timeout < 6 } {
                         incr next_timeout 1
                      }
                      incr mytries -1 ; 
                      if { $mytries < 0 } { 
                          set ok 1
                          add_proc_error "open_remote_spawn_process" -2 "${error_info}\nshell doesn't start or runs not as user $open_remote_spawn__check_user on host $open_remote_spawn__hostname" 
                          puts $CHECK_OUTPUT "sending CTRL + C to spawn id $spawn_id ..."
                          flush $CHECK_OUTPUT
                          
                          catch { send -i $spawn_id "\003" } ;# send CTRL+C to stop evtl. running processes
                          puts $CHECK_OUTPUT "closing spawn process (2)..."
                          flush $CHECK_OUTPUT
                          catch { close -i $spawn_id }
                          catch { wait -nowait -i $spawn_id }
                          puts $CHECK_OUTPUT "closed buffer: $open_spawn_buffer"
                          return ""
                      }
                   }
                   -i $spawn_id -- "The authenticity of host*" {
                      after 100
                      send -i $spawn_id "yes\n"
                   }
                   -i $spawn_id -- "Are you sure you want to continue connecting (yes/no)?*" {
                      send -i $spawn_id "yes\n"
                   }
                   -i $spawn_id -- "Please type 'yes' or 'no'*" {
                      send -i $spawn_id "yes\n"
                   }
                   -i $spawn_id -- "Terminal type?" {
                      send -i $spawn_id -- "vt100\n"
                   }
                   -i $spawn_id -- "__ my id is ->*${open_remote_spawn__check_user}*\n" { 
                       debug_puts "shell response! - fine" 
                       send -i $spawn_id "\n"
                       set ok 1
                   }
                   -i $spawn_id -- "__ my id is ->*root*\n" { 
                       debug_puts "shell response! - fine" 
                       send -i $spawn_id "\n"
                       set ok 1
                   }
                   -i $spawn_id -- "assword" {
                      puts $CHECK_OUTPUT "--> ERROR <--"
                      puts $CHECK_OUTPUT "unexpected password question for user $open_remote_spawn__check_user on host $open_remote_spawn__hostname"
                      puts $CHECK_OUTPUT "please check .rhosts file"
                      puts $CHECK_OUTPUT "sending CTRL + C to spawn id $spawn_id ..."
                      flush $CHECK_OUTPUT
                          
                      catch { send -i $spawn_id "\003" } ;# send CTRL+C to stop evtl. running processes
                      puts $CHECK_OUTPUT "closing spawn process (3)..."
                      flush $CHECK_OUTPUT
                      catch { close -i $spawn_id }
                      catch { wait -nowait -i $spawn_id }
                      puts $CHECK_OUTPUT "closed buffer: $open_spawn_buffer"
                      return ""
                   }
                }
             }
             set timeout 60
             log_user 1
         }
      } catch_error_message ]
      if { $catch_return != 0 } {
         add_proc_error "open_remote_spawn_process" -2 "${error_info}\nerror starting shell:\n$catch_error_message" 
         uplevel 1 {
            catch { close -i $spawn_id }
            catch { wait -nowait -i $spawn_id }
         }
         return ""
      }
      catch {
         uplevel 1 {
            set open_remote_spawn__stop 0
            log_user 0
            if { $CHECK_DEBUG_LEVEL != 0 } {
              log_user 1
            }
            set timeout 60 
            while { $open_remote_spawn__stop == 0 } {
               expect {
                  -i $spawn_id full_buffer {
                     set open_remote_spawn__stop -1
                     add_proc_error "open_remote_spawn_process" -1 "${error_info}\nbuffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
                     catch { close -i $spawn_id }
                     catch { wait -nowait -i $spawn_id }
                     return ""
                  }
                  -i $spawn_id timeout {
                     set open_remote_spawn__stop -1
                     add_proc_error "open_remote_spawn_process" -2 "${error_info}\nrlogin timeout"
                     catch { close -i $spawn_id }
                     catch { wait -nowait -i $spawn_id }
                     return ""
                  } 
                  -i $spawn_id -- "ermission denied" {
                     set open_remote_spawn__stop -1
                     add_proc_error "open_remote_spawn_process" -2 "${error_info}\npermission denied"
                     catch { close -i $spawn_id }
                     catch { wait -nowait -i $spawn_id }
                     return ""
                  }
                  -i $spawn_id -- "\n" {
                        debug_puts "login sequence for user $CHECK_USER ..."
                        if { ([string compare $open_remote_spawn__user $CHECK_USER ] != 0) && 
                             ([have_ssh_access] == 0) } {
                              if { [string compare $open_remote_spawn__user "root" ] != 0 } {
                                 debug_puts "switch to root become $open_remote_spawn__user"
                                 send -i $spawn_id "su root -c 'su - $open_remote_spawn__user'\n" 
                                 incr remote_spawn_nr_of_shells 1   ;# this shell ist closed with 1 exit call!!!
                              } else {
                                 debug_puts "switch to root"
                                 send -i $spawn_id "su root\n"
                                 incr remote_spawn_nr_of_shells 1 
                              }
                        }
                        if { ([string compare $open_remote_spawn__user $CHECK_USER ] == 0) && 
                             ([have_ssh_access] == 0) } {
                             send -i $spawn_id "\n"
                             debug_puts "sending new line"
                        }
                        
                        if { [have_ssh_access] != 0 } {
                             send -i $spawn_id "su - $open_remote_spawn__user\n"
                             incr remote_spawn_nr_of_shells 1 
                             debug_puts "switching to user $open_remote_spawn__user (with env)"
                        } 
                        set open_remote_spawn__stop 1
                  }
                  -i $spawn_id eof {
                     set open_remote_spawn__stop -1
                     add_proc_error "open_remote_spawn_process" -2 "${error_info}\nunexpected eof on rlogin command"
                     catch { close -i $spawn_id }
                     catch { wait -nowait -i $spawn_id }
                     return ""
                  }
               }
            }
            log_user 1
         }
      }
   

      catch {
         if { ([string compare $user $CHECK_USER ] != 0) && ([have_ssh_access] == 0) } {
            uplevel 1 { 
               log_user 0
               if { $CHECK_DEBUG_LEVEL != 0 } {
                 log_user 1
               }
               expect {
                  -i $spawn_id full_buffer {
                     add_proc_error "open_remote_spawn_process" -1 "${error_info}\nbuffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
                     catch { close -i $spawn_id }
                     catch { wait -nowait -i $spawn_id }
                     return ""
                  }
                  -i $spawn_id timeout {
                     add_proc_error "open_remote_spawn_process" -2 "${error_info}\ntimeout waiting for password question"
                     catch { close -i $spawn_id }
                     catch { wait -nowait -i $spawn_id }
                     return ""
                  }  
                  -i $spawn_id -- "ermission denied" {
                     add_proc_error "open_remote_spawn_process" -2 "${error_info}\npermission denied error"
                     catch { close -i $spawn_id }
                     catch { wait -nowait -i $spawn_id }
                     return ""
                  }
                  -i $spawn_id -- "assword:" {
                     debug_puts "got root password question"
                     after 1000
                     log_user 0
                     set send_slow "1 .1"
                     send -i $spawn_id -s "[get_root_passwd]\n"
                     debug_puts "root password sent" 
                     if { $CHECK_DEBUG_LEVEL != 0 } {
                        log_user 1
                     }
                  }
                  -i $spawn_id eof {
                     add_proc_error "open_remote_spawn_process" -2 "${error_info}\nunexpected eof on rlogin command"
                     catch { close -i $spawn_id }
                     catch { wait -nowait -i $spawn_id }
                     return ""
                  }
               }
               log_user 1
            }
         }
      }
      set nr_of_shells [ uplevel 1 { set remote_spawn_nr_of_shells  } ]
      if { $using_ts_def_con != 0 } {
         if { $using_ts_def_con == 1 } {
            add_open_spawn_rlogin_session $hostname "ts_def_con" $sp_id $pid $nr_of_shells
         }
         if { $using_ts_def_con == 2 } {
            add_open_spawn_rlogin_session $hostname "ts_def_con2" $sp_id $pid $nr_of_shells
         }
         if { $using_ts_def_con == 3 } {
            add_open_spawn_rlogin_session $hostname "ts_def_con_translate" $sp_id $pid $nr_of_shells
         }
      } else {
         add_open_spawn_rlogin_session $hostname $user $sp_id $pid $nr_of_shells
      }
      

      debug_puts "unsetting correct ..."
      uplevel 1 {
         send -i $open_remote_spawn__id "unset correct\n"
      }
   }

   if { $re_use_script == 0 } {
      debug_puts "checking shell ..."
      set open_spawn_buffer 0
      uplevel 1 {
         log_user 0
         if { $CHECK_DEBUG_LEVEL != 0 } {
           log_user 1
         }
         set timeout 1

         debug_puts "checking remote file access ..."


         send -i $open_remote_spawn__id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/file_check.sh $open_remote_spawn__script_name\n"
         set open_remote_spawn__tries 30
         set open_remote_spawn__is_correct_user_id 0

         expect {
            -i $open_remote_spawn__id -- "__ my id is ->*${open_remote_spawn__user}*<-" { 
               set open_remote_spawn__is_correct_user_id 1
               exp_continue
            }

            -i $open_remote_spawn__id -- "file exists" {
               if { $open_remote_spawn__is_correct_user_id != 1 } {
                  add_proc_error "open_remote_spawn_process" -1 "got wrong user id or no user id"
                  set open_spawn_buffer 1
               }
            }
            -i $open_remote_spawn__id full_buffer {
               add_proc_error "open_remote_spawn_process" -1 "${error_info}\nbuffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
               set open_spawn_buffer 1
            }
            -i $open_remote_spawn__id timeout {
                debug_puts "checking remote file access ... got timeout"
                send -i $open_remote_spawn__id "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/file_check.sh $open_remote_spawn__script_name\n"
                incr open_remote_spawn__tries -1
                if { $open_remote_spawn__tries <= 0 } {
                   add_proc_error "open_remote_spawn_process" -1 "${error_info}\ntimeout waiting for file_check.sh script"
                   set open_spawn_buffer 1
                } else {
                   exp_continue
                }
            }  
         }

         log_user 1
      }
      # now check the rlogin connection because there was an error
      # with file_check.sh
      if { $open_spawn_buffer == 1 } {
         uplevel 1 {
            log_user 0
            if { $CHECK_DEBUG_LEVEL != 0 } {
              log_user 1
            }
         }
         get_open_spawn_rlogin_session $hostname $user con_data 1
         return ""
      }
   } else {
      debug_puts "skip checking shell, using already used script ..."
      uplevel 1 {
         log_user 1
      }
   }
 
   debug_puts "\"$hostname\"($user): starting command: $exec_command $exec_arguments"

   if { $background != 0 } {
      uplevel 1 { append open_remote_spawn__script_name " &" }
   }
   
   uplevel 1 { 
      log_user 0
      if { $CHECK_DEBUG_LEVEL != 0 } {
         log_user 1
      }
      send -i $open_remote_spawn__id "$open_remote_spawn__script_name\n"
   }

   if { $background == 2 } {
      set back_time 15 ;# let background process time to do his initialization
      while { $back_time > 0 } {
         puts -nonewline $CHECK_OUTPUT "."
         flush $CHECK_OUTPUT
         sleep 1
         incr back_time -1
      }
      puts $CHECK_OUTPUT "hope background process is initalized now!"
   }


   # delete_file_at_startup $script_name

   debug_puts "number of open shells: $nr_of_shells"
   lappend back $nr_of_shells

   set rlogin_in_use_buffer([lindex $back 1]) 1
  
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
#     remote_procedures/run_command_as_user
#     remote_procedures/start_remote_tcl_prog
#     remote_procedures/start_remote_prog
#     
#*******************************
proc open_spawn_process {args} {

   global CHECK_OUTPUT open_spawn_buffer env 
   uplevel 1 { global open_spawn_buffer }
   uplevel 1 { global CHECK_EXPECT_MATCH_MAX_BUFFER }
   uplevel 1 { global CHECK_OUTPUT }

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
   set open_spawn_buffer $arguments
   uplevel 1 { set open_spawn_arguments $open_spawn_buffer } 

   debug_puts "starting spawn process ..."
   flush $CHECK_OUTPUT

   set pid   [ uplevel 1 { eval spawn $open_spawn_arguments } ]
   set sp_id [ uplevel 1 { set spawn_id } ]
   set back $pid
   lappend back $sp_id
   debug_puts "open_spawn_process:  arguments: $args"
   uplevel 1 {
      match_max -i $spawn_id $CHECK_EXPECT_MATCH_MAX_BUFFER
      debug_puts "open_spawn_process -> buffer size is: [match_max]"
   }

   flush $CHECK_OUTPUT

   if {$pid == 0 } {
     add_proc_error "open_spawn_process" -1 "could not spawn! (ret_pid = $pid)" 
   }
   return $back
}


#****** remote_procedures/add_open_spawn_rlogin_session() **********************
#  NAME
#     add_open_spawn_rlogin_session() -- add spawn id to open connection list
#
#  SYNOPSIS
#     add_open_spawn_rlogin_session { hostname user spawn_id pid } 
#
#  FUNCTION
#     This procedure will add the given spawn id to the rlogin spawn open 
#     connection buffer. This buffer contains all open rlogin spawn process
#     ids. 
#
#  INPUTS
#     hostname - hostname of rlogin connection
#     user     - user who logged in
#     spawn_id - spawn process id
#     pid      - process id of rlogin session
#
#  RESULT
#     no return value
#
#  SEE ALSO
#     remote_procedures/add_open_spawn_rlogin_session
#     remote_procedures/get_open_spawn_rlogin_session
#     remote_procedures/get_spawn_id_rlogin_session
#     remote_procedures/check_rlogin_session
#*******************************************************************************
proc add_open_spawn_rlogin_session { hostname user spawn_id spawn_pid nr_of_shells} {
   global CHECK_OUTPUT rlogin_spawn_session_buffer
   global do_close_rlogin rlogin_max_open_connections 
   

   if { $do_close_rlogin != 0 } {
      debug_puts "close_rlogin argument set, closing rlogin connections after use"
      return  
   }

   set entries [array names rlogin_spawn_session_buffer]

   if { [llength $entries] >= $rlogin_max_open_connections } {
      debug_puts "number of open connections > rlogin_max_open_connections = $rlogin_max_open_connections"
      puts $CHECK_OUTPUT "removing oldest not used rlogin session (rlogin_max_open_connections overflow)"
      set now [timestamp]
      set remove_sp_id ""
      set remove_pid   "" 
      set closes       ""
      foreach sp_id $entries {
         set data [split $rlogin_spawn_session_buffer($sp_id) ";"]
         set time [ lindex $data 3 ]  
         set pid  [ lindex $data 1 ]
         if { $now > $time } {
            set now $time
            set remove_pid $pid
            set remove_sp_id $sp_id
            set closes [ lindex $data 4 ]

         } 
         debug_puts $time,$sp_id,$pid,$closes
      }
      debug_puts "longest not used element: $remove_sp_id, $remove_pid" 

      set cl_id $remove_pid
      lappend cl_id $remove_sp_id
      lappend cl_id $closes       ;# nr of open shells
      unset rlogin_spawn_session_buffer($remove_sp_id)
      # close the connection. We are not intested in the exit code of the 
      # previously executed command, so don't make close_spawn_process check
      # the exit code
      close_spawn_process $cl_id 1 2
   }

   debug_puts "Adding spawn_id=$spawn_id, rlogin pid=$spawn_pid to"
   debug_puts "host=$hostname, user=$user into open rlogin session buffer"

   set con_data "$hostname;$spawn_pid;$user;[timestamp];$nr_of_shells"
   set rlogin_spawn_session_buffer($spawn_id) $con_data 
}

#****** remote_procedures/get_open_spawn_rlogin_session() **********************
#  NAME
#     get_open_spawn_rlogin_session() -- get rlogin connection data
#
#  SYNOPSIS
#     get_open_spawn_rlogin_session { hostname user back_var } 
#
#  FUNCTION
#     This procedure returns the corresponding spawn_id for hostname and user
#     name.
#
#  INPUTS
#     hostname - hostname of rlogin connection 
#     user     - user who logged in 
#     back_var - name of array to store data in
#                (the array has following names:
#                 back_var(spawn_id) 
#                 back_var(pid)     
#                 back_var(hostname) 
#                 back_var(user))
#
#  RESULT
#     1 on success, 0 if no connection is available
#
#  EXAMPLE
#     get_open_spawn_rlogin_session $hostname $user con_data
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
proc get_open_spawn_rlogin_session { hostname user back_var {do_check 0} } {
   global CHECK_OUTPUT rlogin_spawn_session_buffer
   global do_close_rlogin

   upvar $back_var back 

   set back(spawn_id)  0
   set back(pid)       0
   set back(hostname)  0
   set back(user)      0
   set back(ltime)     0
   set back(nr_shells) 0
   

   if { $do_close_rlogin != 0 } {
      return 0 
   }


   set entries [array names rlogin_spawn_session_buffer]
   debug_puts "open rlogin connections: [llength $entries]"

   foreach elem $entries {
      set con_data $rlogin_spawn_session_buffer($elem)
      set con_data_list [split $con_data ";"] 
      if { [string compare $hostname [lindex $con_data_list 0]] == 0 } {
         if { [string compare $user [lindex $con_data_list 2]] == 0 } {
            set data_list [split $con_data ";"]
            set back(spawn_id)  $elem
            set back(pid)       [lindex $data_list 1]
            set back(user)      [lindex $data_list 2]
            set back(hostname)  $hostname
            set back(ltime)     [lindex $data_list 3]
            set back(nr_shells) [lindex $data_list 4]
            debug_puts "spawn_id (a) : $back(spawn_id)"
#            debug_puts "pid       : $back(pid)"
            debug_puts "hostname  : $back(hostname)"
            debug_puts "user:     : $back(user)"
#            debug_puts "ltime:    : $back(ltime)"
#            debug_puts "nr_shells : $back(nr_shells)"
            if { $do_check == 1 && [check_rlogin_session $back(spawn_id) $back(pid) $back(hostname) $back(user) $back(nr_shells) ] != 1 } {
               set back(spawn_id) "0"
               set back(pid)      "0"
               set back(hostname) "0"
               set back(user)     "0"
               set back(ltime)     0
               set back(nr_shells) 0
               return 0 
            } else {
               unset rlogin_spawn_session_buffer($elem)
#               debug_puts "old rlogin connection data: $con_data" 
               set con_data "$back(hostname);$back(pid);$back(user);[timestamp];$back(nr_shells)"
               debug_puts "new rlogin connection data: $con_data"
               set rlogin_spawn_session_buffer($elem) $con_data 
               return 1
            }
         }
      }
   }
   debug_puts "get_open_spawn_rlogin_session - session $user,$hostname not found"
   return 0
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
proc get_spawn_id_rlogin_session { id back_var {no_check 0}} {
   global CHECK_OUTPUT rlogin_spawn_session_buffer
   global do_close_rlogin

   upvar $back_var back 

   set back(spawn_id) "0"
   set back(pid)      "0"
   set back(hostname) "0"
   set back(user)     "0"
   set back(ltime)     0
   set back(nr_shells) 0

   if { $do_close_rlogin != 0 } {
      return 0 
   }


   if { [info exists rlogin_spawn_session_buffer($id) ] != 0 } {
      set con_data $rlogin_spawn_session_buffer($id)
      set data_list [split $con_data ";"]
      set back(spawn_id) $id
      set back(pid)      [lindex $data_list 1]
      set back(hostname) [lindex $data_list 0]
      set back(user)     [lindex $data_list 2]
      set back(ltime)     [lindex $data_list 3]
      set back(nr_shells) [lindex $data_list 4]
      
      debug_puts "spawn_id  : $back(spawn_id)"
      debug_puts "pid       : $back(pid)"
      debug_puts "hostname  : $back(hostname)"
      debug_puts "user:     : $back(user)"
      debug_puts "ltime:    : $back(ltime)"
      debug_puts "nr_shells : $back(nr_shells)"

      if { $no_check == 1 } {
         return 1 
      }
      if { [check_rlogin_session $back(spawn_id) $back(pid) $back(hostname) $back(user) $back(nr_shells)] != 1 } {
         set back(spawn_id) "0"
         set back(pid)      "0"
         set back(hostname) "0"
         set back(user)     "0"
         set back(ltime)     0
         set back(nr_shells) 0
         return 0 
      } else {
         return 1
      }
   }
   return 0
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
   global CHECK_OUTPUT rlogin_spawn_session_buffer
   global do_close_rlogin

   # if we called testsuite with option close_rlogin, we have no open sessions
   if { $do_close_rlogin != 0 } {
      puts $CHECK_OUTPUT "close_open_rlogin_sessions - open rlogin session mode not activated!"
      return 0 
   }

   # gather all session names
   set sessions [array names rlogin_spawn_session_buffer]
   set help_buf ""
   foreach session $sessions {
      lappend help_buf "$session;$rlogin_spawn_session_buffer($session)"
   }

   # close all sessions
   foreach session $help_buf {

      set data_list [split $session ";"]
      set back(spawn_id)  [lindex $data_list 0]
      set back(pid)       [lindex $data_list 2]
      set back(hostname)  [lindex $data_list 1]
      set back(user)      [lindex $data_list 3]
      set back(ltime)     [lindex $data_list 4]
      set back(nr_shells) [lindex $data_list 5]
      
      if { $if_not_working != 0 } {
         if { [check_rlogin_session $back(spawn_id) $back(pid) $back(hostname) $back(user) $back(nr_shells) 1] == 1 } {
            puts $CHECK_OUTPUT "will not close spawn id $back(spawn_id) - session is ok!"
            continue
         }
      }



      unset rlogin_spawn_session_buffer($back(spawn_id))
      set id $back(pid)
      lappend id $back(spawn_id)
      lappend id $back(nr_shells)
      puts $CHECK_OUTPUT "close_open_rlogin_sessions - closing $id"
      close_spawn_process $id 1 2  ;# don't check exit state
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
proc check_rlogin_session { spawn_id pid hostname user nr_of_shells { only_check 0 }} {
   global CHECK_OUTPUT rlogin_spawn_session_buffer CHECK_USER CHECK_TESTSUITE_ROOT CHECK_SCRIPT_FILE_DIR

   if { [info exists rlogin_spawn_session_buffer($spawn_id) ] != 0 } {
      send -i $spawn_id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/check_identity.sh\n"

      switch -- $user {
         "ts_def_con" {
            set user $CHECK_USER
         }
         "ts_def_con2" {
            set user $CHECK_USER
         }
         "ts_def_con_translate" {
            set user $CHECK_USER
         }
      }

      set timeout 1
      set mytries  5
      set open_remote_spawn__tries 15
      expect {
         -i $spawn_id -- "__ my id is ->*${user}*\n" { 
            return 1
         }
         -i $spawn_id full_buffer {
            add_proc_error "check_rlogin_session" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
         }
         -i $spawn_id timeout {
             debug_puts "got timeout for check_identity"
             incr open_remote_spawn__tries -1
             if { $open_remote_spawn__tries <= 0 } {
                add_proc_error "check_rlogin_session" -1 "timeout waiting for shell response prompt (a)"
             } else {
                if { $open_remote_spawn__tries < 12 } {
                   puts -nonewline $CHECK_OUTPUT "." 
                   flush $CHECK_OUTPUT
                }
                send -i $spawn_id -- "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/check_identity.sh\n"
                exp_continue
             }
         }
         -i $spawn_id -- "Terminal type?" {
            send -i $spawn_id -- "vt100\n"
            exp_continue
         }
      }
      if { $only_check == 0 } {
         # restart connection
         puts $CHECK_OUTPUT "check_rlogin_session: had error"
         unset rlogin_spawn_session_buffer($spawn_id)
         set id $pid
         lappend id $spawn_id
         lappend id $nr_of_shells
         puts $CHECK_OUTPUT "closing spawn id $spawn_id with pid $pid to enable new rlogin session ..."
         close_spawn_process $id 0 3
         puts $CHECK_OUTPUT "closing done."
      }
   }
   return 0 ;# error
}


#****** remote_procedures/is_spawn_process_in_use() ****************************
#  NAME
#     is_spawn_process_in_use() -- check if spawn id is in use
#
#  SYNOPSIS
#     is_spawn_process_in_use { id } 
#
#  FUNCTION
#     Test if given spawn id is already in use
#
#  INPUTS
#     id - internal spawn id number from open_remote_spawn_process()
#
#  RESULT
#     0    : not in use
#     not 0: this spawn id is not in use
#
#  SEE ALSO
#     remote_procedures/open_remote_spawn_process()
#*******************************************************************************
proc is_spawn_process_in_use { id } {
   global rlogin_in_use_buffer

   set sp_id  [lindex $id 1]
   if { [info exists rlogin_in_use_buffer($sp_id)] } {
      return $rlogin_in_use_buffer($sp_id)
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
#     remote_procedures/run_command_as_user
#     remote_procedures/start_remote_tcl_prog
#     remote_procedures/start_remote_prog
#*******************************
proc close_spawn_process { id { check_exit_state 0 } {my_uplevel 1}} {

   global CHECK_OUTPUT open_spawn_buffer CHECK_DEBUG_LEVEL rlogin_in_use_buffer
  
   set sp_id  [lindex $id 1]
   set sp_pid [lindex $id 0]
   get_spawn_id_rlogin_session $sp_id con_data 1
   set con_data(in_use) 0
   if { $con_data(pid) != 0 } {
      debug_puts "sending CTRL + C to spawn id $sp_id ..."
      send -i $sp_id "\003" ;# send CTRL+C to stop evtl. running processes in that shell
      # wait for CTRL-C to have effect
      debug_puts "Will not close spawn id \"$sp_id\", this is rlogin connection to"
      debug_puts "host \"$con_data(hostname)\", user \"$con_data(user)\""
      set rlogin_in_use_buffer($sp_id) 0
      return -1   
   }

   log_user 0  
   if { $CHECK_DEBUG_LEVEL != 0 } {
      log_user 1
   }
   if { [llength $id ] > 2  } { 
       set nr_of_shells [lindex $id 2]
       set timeout 0
       set do_stop 0
       set send_slow "1 .05"
       debug_puts "nr of open shells: $nr_of_shells"
       debug_puts "-->sending $nr_of_shells exit(s) to shell on id $sp_id"
       send -s -i $sp_id "\003" ;# send CTRL+C to stop evtl. running processes in that shell

       # wait for CTRL-C to have effect
       for {set i 0} {$i < $nr_of_shells } {incr i 1} {
          send -s -i $sp_id "exit\n"
          set timeout 6
          expect { 
              -i $sp_id full_buffer {
                 add_proc_error "close_spawn_process" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
              }
              -i $sp_id "*" {
                 debug_puts "shell exit"
              }
              -i $sp_id eof {
                 debug_puts "eof while waiting for shell exit"
              }
              -i $sp_id timeout {
                 debug_puts "timeout while waiting for shell exit"
              }
          }
       }
       set timeout 2 
       set my_tries 3
       while { $do_stop != 1 } {
          expect {
             -i $sp_id full_buffer {
                add_proc_error "close_spawn_process" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
                set do_stop 1 
             }
             -i $sp_id eof { 
                set do_stop 1 
                debug_puts "got end of file - ok"
             }
             -i $sp_id timeout {
                puts $CHECK_OUTPUT "timeout (closing shell) counter: $my_tries ..."
                if { $my_tries > 0 } {
                   send -s -i $sp_id "exit\n"
                } else {
                   add_proc_error "close_spawn_process" "-1" "error closing shell"
                   set do_stop 1
                }
                incr my_tries -1
             }
          }
          debug_puts "waiting for end of file"
       }
   } 

   set open_spawn_buffer $sp_id
   catch { 
      uplevel $my_uplevel { 
         global open_spawn_buffer
         close -i $open_spawn_buffer
         debug_puts "closed buffer: $open_spawn_buffer"
      }
   }

   log_user 1
   set wait_return "" 
   catch { set wait_return [ uplevel $my_uplevel { wait -i $open_spawn_buffer } ] }
   set wait_pid      [lindex $wait_return 0]
   set wait_spawn_id [lindex $wait_return 1]
   set wait_error    [lindex $wait_return 2]
   set wait_code     [lindex $wait_return 3]

   debug_puts "closed buffer: $open_spawn_buffer"
   debug_puts "wait pid        : $wait_pid"
   debug_puts "wait spawn id   : $wait_spawn_id"
   debug_puts "wait error      : $wait_error (-1 = operating system error, 0 = exit)"
   debug_puts "wait code       : $wait_code  (os error code or exit status)"

   # did we close the correct spawn id?
   if { ([ string compare $open_spawn_buffer $wait_spawn_id ] != 0) && ($check_exit_state == 0)} {
      add_proc_error "close_spawn_process" "-1" "wrong spawn id closed: expected $open_spawn_buffer, got $wait_spawn_id"
   }

   # on regular exit: check exit code, shall be 0
   if { $wait_error == 0 } {
      if { ($wait_code != 0) && ($check_exit_state == 0) } {
         add_proc_error "close_spawn_process" -1 "wait exit status: $wait_code"
      }
   } else {
      puts $CHECK_OUTPUT "*** operating system error: $wait_code"
      puts $CHECK_OUTPUT "spawn id: $wait_spawn_id"
      puts $CHECK_OUTPUT "wait pid: $wait_pid"
      if { ($check_exit_state == 0) } {
         add_proc_error "close_spawn_process" -1 "operating system error: $wait_code"
      }
   }
   flush $CHECK_OUTPUT
   set rlogin_in_use_buffer($sp_id) 0

   return $wait_code ;# return exit state
}


# user is a system user name
# command is a binary in $CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH or a absolute path
# args are the command options of command
# counter: how often sould the command be called
# returns each line of output in a list
#                                                             max. column:     |
#****** remote_procedures/run_command_as_user() ******
# 
#  NAME
#     run_command_as_user -- start proccess under a specific user account
#
#  SYNOPSIS
#     run_command_as_user { hostname user command args counter } 
#
#  FUNCTION
#     This procedure is using start_remote_prog to start a binary or a skript 
#     file under a specific user account.
#
#  INPUTS
#     hostname - host where the command should be started
#     user     - system user name who should start the command
#     command  - command name (if no full path is given, the product
#                root path will be used)
#     args     - command arguments
#     counter  - run the command $counter times
#
#  RESULT
#     the command output 
#
#  EXAMPLE
#     set jobargs "/home/me/testjob.sh"
#     set result [ run_command_as_user "expo1" "user1" "qsub" "$jobargs" 5]
#     puts $result
#
#  NOTES
#     This procedure starts the script file remote_submit.sh in the scripts 
#     directory of the testsuite. This script is sourcing the 
#     default/common/settings.sh file of the cluster. If the command parameter
#     has no full path entry it will add the $CHECK_PRODUCT_ROOT path in 
#     front of the command. 
#
#  SEE ALSO
#     remote_procedures/open_spawn_process
#     remote_procedures/open_root_spawn_process
#     remote_procedures/close_spawn_process
#     remote_procedures/run_command_as_user
#     remote_procedures/start_remote_tcl_prog
#     remote_procedures/start_remote_prog
#*******************************
proc run_command_as_user { hostname user command args counter } {
   global ts_config
   global CHECK_TESTSUITE_ROOT CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT open_spawn_buffer 
   global CHECK_COMMD_PORT CHECK_SCRIPT_FILE_DIR
 
   # perform su to user and submit jobs as user $user
   set program  "$CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/remote_submit.sh"           ;# script to call in su root -c option
   set par1     "$CHECK_PRODUCT_ROOT"    ;# settings script
   set par2     "$ts_config(cell)"
   if { [string first "\/" $command ] >= 0 } {
      set par3  "$command $args"
   } else {
      set par3  "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/$command $args"  ;# job to start
   }
   set par4     "$counter"  ;# number of qsub calls

   puts $CHECK_OUTPUT "running: $par3"
   set output [ start_remote_prog "$hostname" "$user" "$program" "\"$par1\" \"$par2\" \"$par3\" \"$par4\"" "prg_exit_state" "120" ]
   return $output
}




# main
# if { [info exists argc ] != 0 } {
#    set TS_ROOT ""
#    set procedure ""
#    for { set i 0 } { $i < $argc } { incr i } {
#       if {$i == 0} { set TS_ROOT [lindex $argv $i] }
#       if {$i == 1} { set procedure [lindex $argv $i] }
#    }
#    if { $argc == 0 } {
#       puts "usage:\n$module_name <CHECK_TESTSUITE_ROOT> <proc> no_main <testsuite params>"
#       puts "options:"
#       puts "CHECK_TESTSUITE_ROOT -  path to TESTSUITE directory"
#       puts "proc                 -  procedure from this file with parameters"
#       puts "no_main              -  used to source testsuite file (check.exp)"
#       puts "testsuite params     -  any testsuite command option (from file check.exp)"
#       puts "                        testsuite params: file <path>/defaults.sav is needed"
#    } else {
#       source "$TS_ROOT/check.exp"
#       puts $CHECK_OUTPUT "master host is $CHECK_CORE_MASTER"
#       puts $CHECK_OUTPUT "calling \"$procedure\" ..."
#       set result [ eval $procedure ]
#       puts $result 
#       flush $CHECK_OUTPUT
#    }
# }

