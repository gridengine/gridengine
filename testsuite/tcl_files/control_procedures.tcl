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
set module_name "control_procedures.tcl"



# procedures
#                                                             max. column:     |
#****** control_procedures/handle_vi_edit() ******
# 
#  NAME
#     handle_vi_edit -- sending vi commands to application 
#
#  SYNOPSIS
#     handle_vi_edit { prog_binary prog_args vi_command_sequence 
#     expected_result {additional_expected_result "___ABCDEFG___"} 
#     {additional_expected_result2 "___ABCDEFG___"} } 
#
#  FUNCTION
#     Start an application which and send special command strings to it. Wait
#     and parse the application output.
#
#  INPUTS
#     prog_binary                                   - application binary to start 
#                                                     (e.g. qconf) 
#     prog_args                                     - application arguments (e.g. 
#                                                     -mconf) 
#     vi_command_sequence                           - list of vi command sequences 
#                                                     (e.g. 
#                                                     {:%s/^$elem .*$/$elem 10/\n}) 
#     expected_result                               - program output in no error 
#                                                     case (e.g. modified) 
#     {additional_expected_result "___ABCDEFG___"}  - additional expected_result 
#     {additional_expected_result2 "___ABCDEFG___"} - additional expected_result 
#
#  RESULT
#     0 when the output of the application contents the expected_result 
#    -1 on timeout
#    -2 on additional_expected_result
#    -3 on additional_expected_result2 
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
proc handle_vi_edit { prog_binary prog_args vi_command_sequence expected_result {additional_expected_result "___ABCDEFG___"} {additional_expected_result2 "___ABCDEFG___"}} {
   global CHECK_OUTPUT env CHECK_HOST CHECK_DEBUG_LEVEL CHECK_USER

   set env(EDITOR) [get_binary_path "$CHECK_HOST" "vim"]
   set result -100
#  set id [ eval open_spawn_process "$prog_binary" "$prog_args" ]
   set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER "$prog_binary" "$prog_args" ]
      set sp_id [ lindex $id 1 ] 
      puts $CHECK_OUTPUT "starting -> $prog_binary $prog_args"
      if {$CHECK_DEBUG_LEVEL != 0} {
         log_user 1
         set send_speed .05
         set send_line_speed 1
      } else {
         log_user 0 
         set send_speed .0001
         set send_line_speed .0001
      }

      set stop_line_wait 0
      set timeout 60

      set start_time [ timestamp ] 
      send -i $sp_id ""
      set timeout 1
      while { $stop_line_wait == 0 } {
         expect {
            -i $sp_id full_buffer {
               add_proc_error "handle_vi_edit" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
               set stop_line_wait 1
            }
            -i $sp_id "line" {
               set stop_line_wait 1
            }
            -i $sp_id timeout {  
               send -i $sp_id ""
            }
            -i $sp_id "erminal too wide" {
               add_proc_error "handle_vi_edit" -2 "got terminal to wide vi error"
               set stop_line_wait 1
            }
            -i $sp_id eof {
               add_proc_error "handle_vi_edit" -1 "unexpected end of file"
               set stop_line_wait 1
            }
         }
      }
      set timeout 0
      set send_slow "1 $send_speed" 

      foreach elem $vi_command_sequence {
         set com_length [ string length $elem ]
         set com_sent 0
         expect -i $sp_id
         if { $CHECK_DEBUG_LEVEL != 0 } {
            send -s -i $sp_id -- "$elem"
         } else {
            send -i $sp_id -- "$elem"
         }
         expect -i $sp_id
      }

      # wait 1 second for new file date!!! 
      sleep 1
      while { [ timestamp ] <= $start_time } { 
         sleep 1
      }
      send -i $sp_id ":wq\n"
      set timeout 100
      set doStop 0
      if { [string compare "" $expected_result ] == 0 } {
         set timeout 0
         set doStop 0
         while { $doStop == 0 } {
            expect {
               -i $sp_id full_buffer {
                  set doStop 1
                  add_proc_error "handle_vi_edit" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
               }
               -i $sp_id timeout {
                  set result -1
                  set doStop 0
               }
               -i $sp_id eof {
                  set doStop 1
                  set result 0
               }
               -i $sp_id "_exit_status_" {
                  set doStop 1
                  set result 0
               }
           }
        }
      } else {
         while { $doStop == 0 } {
            expect {
               -i $sp_id full_buffer {
                  set doStop 1
                  add_proc_error "handle_vi_edit" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
               }
               -i $sp_id -- "$expected_result" {
                  set result 0
               }
               -i $sp_id -- "$additional_expected_result" {
                  set result -2
               }
               -i $sp_id -- "$additional_expected_result2" {
                  set result -3
               }
               -i $sp_id timeout {
                  set result -1
                  set doStop 1
                  add_proc_error "handle_vi_edit" -1 "timeout error"
               }
               -i $sp_id eof {
                  set doStop 1
               }
               -i $sp_id "_exit_status_" {
                  set doStop 1
               }
           }
        }
     }
  puts $CHECK_OUTPUT $expect_out(buffer)
  close_spawn_process $id
  log_user 1
  foreach elem $vi_command_sequence {
      debug_puts "sequence: $elem"
      if { [string first "A" $elem ] != 0 } {
         set index1 [ string first "." $elem ]
         incr index1 -2
         set var [ string range $elem 5 $index1 ] 
        
         set index1 [ string last "$var" $elem ]
         incr index1 [ string length $var]
         incr index1 2
   
         set index2 [ string first "\n" $elem ]
         incr index2 -2
   
         set value [ string range $elem $index1 $index2 ]
         set value [ split $value "\\" ]
         set value [ join $value "" ]
         if { [ string compare $value "*$/" ] == 0 } {
            puts $CHECK_OUTPUT "--> removing \"$var\" entry"
         } else {
            if { [ string compare $var "" ] != 0 && [ string compare $value "" ] != 0  } {         
               puts $CHECK_OUTPUT "--> setting \"$var\" to \"$value\""
            } else {
               puts $CHECK_OUTPUT "--> vi command: \"$elem\""    
            }
         }
      } else {
         set add_output [ string range $elem 2 end ]
         puts $CHECK_OUTPUT "--> adding $add_output"
      }
  }
  flush stdout
  if {$CHECK_DEBUG_LEVEL != 0} {
    log_user 1
  } else {
    log_user 0 
  }

  return $result
}




#                                                             max. column:     |
#****** control_procedures/ps_grep() ******
# 
#  NAME
#     ps_grep -- call get_ps_info and return only expected ps information 
#
#  SYNOPSIS
#     ps_grep { forwhat { host "local" } { variable ps_info } } 
#
#  FUNCTION
#     This procedure will call the get_ps_info procedure. It will parse the 
#     get_ps_info result for the given strings and return only those process 
#     ids which match. 
#
#  INPUTS
#     forwhat              - search string (e.g. binary name) 
#     { host "local" }     - host on which the ps command should be called 
#     { variable ps_info } - variable name to store the result (default ps_info) 
#
#  RESULT
#     returns a list of indexes where the search string matches the ps output. 
#
#  EXAMPLE
# 
#   set myprocs [ ps_grep "execd" "fangorn" ]
#
#   puts "execd's on fangorn index list: $myprocs"
#
#   foreach elem $myprocs {
#     puts $ps_info(string,$elem)
#   }
#
#   output of example:
# 
#   execd's on fangorn index list: 34 39 50 59 61
#   2530   140     1   259 S Sep12  1916 00:00:14 /sge_s/glinux/sge_execd
#   7700   142     1   339 S Sep13  2024 00:03:49 /vol2/bin/glinux/sge_execd
#   19159     0     1     0 S Sep14  1772 00:31:09 /vol/bin/glinux/sgeee_execd
#   24148     0     1     0 S Sep14  2088 00:06:23 bin/glinux/sge_execd
#   15085     0     1     0 S Sep14  1904 00:27:04 /vol2/glinux/sgeee_execd
#
#  NOTES
#   look at get_ps_info procedure for more information! 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     control_procedures/get_ps_info
#*******************************
proc ps_grep { forwhat { host "local" } { variable ps_info } } {

   upvar $variable psinfo

   get_ps_info 0 $host psinfo

   set index_list ""

   for {set i 0} {$i < $psinfo(proc_count) } {incr i 1} {
      if { [string first $forwhat $psinfo(string,$i) ] >= 0 } {
         lappend index_list $i
      }
   }
   return $index_list
} 



#                                                             max. column:     |
#****** control_procedures/get_ps_info() ******
# 
#  NAME
#     get_ps_info -- get ps output on remote or local host 
#
#  SYNOPSIS
#     get_ps_info { { pid 0 } { host "local"} { variable ps_info } 
#     {additional_run 0} } 
#
#  FUNCTION
#     This procedure will call ps on the host given and parse the output. All 
#     information is stored in a special array. If no variable parameter is 
#     given the array has the name ps_info 
#
#  INPUTS
#     { pid 0 }            - set pid for ps_info($pid,error) the 
#                            ps_info([given pid],error) array is always set when 
#                            the pid is given. You have always access to 
#                            ps_info($pid,error)
#     { host "local"}      - host on which the ps command should be started
#     { variable ps_info } - array name where the ps command output should be 
#                            stored the default for this value is "ps_info"
#     {additional_run 0}   - if it is neccessary to start more than one ps command
#                            to get the full information this number is used to be 
#                            able to differ the recursive subcalls. So this 
#                            parameter is only set when the procedure calls itself 
#                            again.
#
#
#  RESULT
#     The procedure returns an 2 dimensional array with following entries:
#
#     If the parameter pid was set to 12 then ps_info(12,error) exists after 
#     calling this procedure ps_info(12,error) is set to 0 when the pid 12 exists, 
#     otherwise it is set to -1 
#
#     when ps_info(12,error) exists the following indicies are available:
# 
#     ps_info(12,string)
#     ps_info(12,index_names)
#     ps_info(12,pgid)
#     ps_info(12,ppid)
#     ps_info(12,uid)
#     ps_info(12,state)
#     ps_info(12,stime)
#     ps_info(12,vsz)
#     ps_info(12,time)
#     ps_info(12,command)
#
#     every output of the ps command is stored into these indicies: 
#     (I is the line number (or index) of the output)
#
#     ps_info(proc_count)   : number of processes (line count of ps command)
#     ps_info(pid,I)        : pid of process
#     ps_info(pgid,I)       : process group id
#     ps_info(ppid,I)       : parent pid
#     ps_info(uid,I)        : user id
#     ps_info(state,I)      : state
#     ps_info(stime,I)      : start time 
#     ps_info(vsz,I)        : virtual size
#     ps_info(time,I)       : cpu time 
#     ps_info(command,I)    : command arguments of process
#     ps_info(string,I)     : complete line
#
#  EXAMPLE
#
#     get process group id of pid 3919:
# 
#     get_ps_info 3919 fangorn
#     if {$ps_info(3919,error) == 0} {
#        puts "process group id of pid 3919 is $ps_info(3919,pgid)"
#     } else {
#        puts "pid 3919 not found!"
#     }
#
#
#
#     print out all pids on local host:
#     
#     get_ps_info 
#     for {set i 0} {$i < $ps_info(proc_count) } {incr i 1} {
#        puts "ps_info(pid,$i)     = $ps_info(pid,$i)"
#     }
#
#  NOTES
#     o additional_run is for glinux at this time
#     o additionan_run is a number from 0 up to xxx at the end of the procedure 
#       it will start again a ps command with other information in order to mix 
#       up the information into one resulting list
#
#     o this procedure should run on following platforms:
#       solaris64, solaris, osf4, tru64, irix6, aix43, aix42, hp10, hp11, 
#       hp11-64, glinux and alinux
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     control_procedures/ps_grep
#*******************************
proc get_ps_info { { pid 0 } { host "local"} { variable ps_info } {additional_run 0} } {

   global CHECK_OUTPUT CHECK_HOST CHECK_USER
   upvar $variable psinfo

   if { [string compare $host "local" ] == 0 } {
      set host $CHECK_HOST
   } 

   set psinfo($pid,error) -1

   unset psinfo

   set psinfo($pid,error) -1
   set psinfo(proc_count) 0
   set psinfo($pid,string) "not found"


   set host_arch [ resolve_arch $host ]

   #puts "arch on host $host is $host_arch"
   
   switch -- $host_arch {

      "solaris64" - 
      "solaris86" { 
         set myenvironment(COLUMNS) "500"
         set result [start_remote_prog "$host" "$CHECK_USER" "ps" "-e -o \"pid=_____pid\" -o \"pgid=_____pgid\" -o \"ppid=_____ppid\" -o \"uid=_____uid\" -o \"s=_____s\" -o \"stime=_____stime\" -o \"vsz=_____vsz\" -o \"time=_____time\" -o \"args=_____args\"" prg_exit_state 60 0 myenvironment]
         set index_names "_____pid _____pgid _____ppid _____uid _____s _____stime _____vsz _____time _____args"
         set pid_pos     0
         set gid_pos     1
         set ppid_pos    2
         set uid_pos     3
         set state_pos   4
         set stime_pos   5
         set vsz_pos     6
         set time_pos    7
         set command_pos 8
      }
     
      "solaris" { 
         set myenvironment(COLUMNS) "500"
         set result [start_remote_prog "$host" "$CHECK_USER" "ps" "-eo \"pid pgid ppid uid s stime vsz time args\"" prg_exit_state 60 0 myenvironment]
         set index_names "  PID  PGID  PPID   UID S    STIME  VSZ        TIME COMMAND"
         set pid_pos     0
         set gid_pos     1
         set ppid_pos    2
         set uid_pos     3
         set state_pos   4
         set stime_pos   5
         set vsz_pos     6
         set time_pos    7
         set command_pos 8
      }

      "osf4" -
      "tru64" { 
         set myenvironment(COLUMNS) "500"
         
         set result [start_remote_prog "$host" "$CHECK_USER" "ps" "-eo \"pid pgid ppid uid state stime vsz time args\"" prg_exit_state 60 0 myenvironment]
         set index_names "   PID   PGID   PPID        UID {S   } {STIME   }   VSZ        TIME COMMAND"
         set pid_pos     0
         set gid_pos     1
         set ppid_pos    2
         set uid_pos     3
         set state_pos   4
         set stime_pos   5
         set vsz_pos     6
         set time_pos    7
         set command_pos 8
      }



      "irix6" { 
         set myenvironment(COLUMNS) "500"
         set result [start_remote_prog "$host" "$CHECK_USER" "ps" "-eo \"pid pgid ppid uid state stime vsz time args\"" prg_exit_state 60 0 myenvironment]
         set index_names "  PID  PGID  PPID   UID S    STIME {VSZ   }        TIME COMMAND"
         set pid_pos     0
         set gid_pos     1
         set ppid_pos    2
         set uid_pos     3
         set state_pos   4
         set stime_pos   5
         set vsz_pos     6
         set time_pos    7
         set command_pos 8
      }

 
      "aix43"   {
         set myenvironment(COLUMNS) "500"
         set result [start_remote_prog "$host" "$CHECK_USER" "ps" "-eo \"pid pgid=BIG_AIX_PGID ppid=BIG_AIX_PPID uid=BIG_AIX_UID stat=AIXSTATE started vsz=BIG_AIX_VSZ time args\"" prg_exit_state 60 0 myenvironment]
         set index_names "  PID BIG_AIX_PGID BIG_AIX_PPID BIG_AIX_UID AIXSTATE  STARTED BIG_AIX_VSZ        TIME COMMAND"
         set pid_pos     0
         set gid_pos     1
         set ppid_pos    2
         set uid_pos     3
         set state_pos   4
         set stime_pos   5
         set vsz_pos     6
         set time_pos    7
         set command_pos 8
      
      }
      
      "aix42"   {
         set myenvironment(COLUMNS) "500"

         set result [start_remote_prog "$host" "$CHECK_USER" "ps" "-eo \"pid pgid=BIG_AIX_PGID ppid=BIG_AIX_PPID uid=BIG_AIX_UID stat=AIXSTATE started vsz=BIG_AIX_VSZ time args\"" prg_exit_state 60 0 myenvironment ]
         set index_names "  PID BIG_AIX_PGID BIG_AIX_PPID BIG_AIX_UID AIXSTATE  STARTED BIG_AIX_VSZ        TIME COMMAND"
         set pid_pos     0
         set gid_pos     1
         set ppid_pos    2
         set uid_pos     3
         set state_pos   4
         set stime_pos   5
         set vsz_pos     6
         set time_pos    7
         set command_pos 8
      
      }

      

      "hp10" -
      "hp11" {
         set myenvironment(COLUMNS) "500"
         set result [start_remote_prog "$host" "$CHECK_USER" "ps" "-efl" prg_exit_state 60 0 myenvironment]
         set index_names "  F S      UID   PID  PPID  C PRI NI     ADDR   SZ    WCHAN    STIME {TTY   }    TIME COMD"
         set pid_pos     3
         set gid_pos     -1
         set ppid_pos    4
         set uid_pos     2
         set state_pos   1
         set stime_pos   11
         set vsz_pos     -1
         set time_pos    13
         set command_pos 14
      }

      "hp11-64" {
         set myenvironment(COLUMNS) "500"
         set myenvironment(UNIX95)  ""
         set result [start_remote_prog "$host" "$CHECK_USER" "ps" "-eo \"pid gid ppid uid state stime vsz time args\"" prg_exit_state 60 0 myenvironment]
         set index_names "  PID        GID  PPID        UID S    STIME     VSZ     TIME COMMAND"
         set pid_pos     0
         set gid_pos     1
         set ppid_pos    2
         set uid_pos     3
         set state_pos   4
         set stime_pos   5
         set vsz_pos     6
         set time_pos    7
         set command_pos 8
      }
      
      "glinux"    { 
         set myenvironment(COLUMNS) "500"
         set result [start_remote_prog "$host" "$CHECK_USER" "ps" "-weo \"pid pgid ppid uid s stime vsz time args\"" prg_exit_state 60 0 myenvironment]
         set index_names "  PID  PGID  PPID   UID S STIME   VSZ     TIME COMMAND"
         set pid_pos     0
         set gid_pos     1
         set ppid_pos    2
         set uid_pos     3
         set state_pos   4
         set stime_pos   5
         set vsz_pos     6
         set time_pos    7
         set command_pos 8
      }
      "slinux"    { 
         set myenvironment(COLUMNS) "500"
         set result [start_remote_prog "$host" "$CHECK_USER" "ps" "-weo \"pid pgid ppid uid s stime vsz time args\"" prg_exit_state 60 0 myenvironment]
         set index_names "  PID  PGID  PPID   UID S STIME   VSZ     TIME COMMAND"
         set pid_pos     0
         set gid_pos     1
         set ppid_pos    2
         set uid_pos     3
         set state_pos   4
         set stime_pos   5
         set vsz_pos     6
         set time_pos    7
         set command_pos 8
      }
      "alinux" {
         if { $additional_run == 0 } {
            # this is the first ps without any size position
            set myenvironment(COLUMNS) "500"
            set result [start_remote_prog "$host" "$CHECK_USER" "ps" "xajw" prg_exit_state 60 0 myenvironment]  
            #                   0     1    2      3   4    5      6   7     8     9  
            set index_names " PPID   PID  PGID   SID TTY TPGID  STAT  UID   TIME COMMAND"
            set pid_pos     1
            set gid_pos     2
            set ppid_pos    0
            set uid_pos     7
            set state_pos   6
            set stime_pos   -1
            set vsz_pos     -1
            set time_pos    8
            set command_pos 9
         } 
         if { $additional_run == 1 } {
            # this is the first ps without any size position
            set myenvironment(COLUMNS) "500"
            set result [start_remote_prog "$host" "$CHECK_USER" "ps" "waux" prg_exit_state 60 0 myenvironment]  
            #                   0       1    2    3     4      5   6   7    8       9   10
            set index_names "{USER    }   PID %CPU %MEM  SIZE   RSS TTY STAT START   TIME COMMAND"
            set pid_pos     1
            set gid_pos     -1
            set ppid_pos    -1
            set uid_pos     -1
            set state_pos   7
            set stime_pos   8
            set vsz_pos     4
            set time_pos    9
            set command_pos 10
         } 
        
      }

      default { 
         add_proc_error "get_ps_info" "-1" "unknown architecture"
         set prg_exit_state 1
         set index_names "  PID   GID  PPID   UID S    STIME  VSZ        TIME COMMAND"
         set pid_pos     0
         set gid_pos     1
         set ppid_pos    2
         set uid_pos     3
         set state_pos   4
         set stime_pos   5
         set vsz_pos     6
         set time_pos    7
         set command_pos 8
         

      }
   }

   if { $prg_exit_state != 0 } {
      add_proc_error "get_ps_info" "-1" "ps error or binary not found"
      return
   }

   set help_list [ split $result "\n" ]

#   foreach elem $help_list {
#      puts $CHECK_OUTPUT $elem
#   }


   # delete empty lines (occurs e.g. on alinux)
   set empty_index [lsearch -exact $help_list ""]
   while {$empty_index >= 0} {
      set help_list [lreplace $help_list $empty_index $empty_index]
      set empty_index [lsearch -exact $help_list ""]
   }
   

   # search ps header line
   set num_lines [llength $help_list]
   set compare_pattern [string range $index_names 1 5]
   for { set x 0 } { $x < $num_lines } { incr x 1 } {
#      if { [string compare -length 6 [lindex $help_list $x] $compare_pattern] == 0 } 
      if { [string first $compare_pattern [lindex $help_list $x]] >= 0 } {
         break
      }
         
   }

   # no header found?
   if { $x == $num_lines } {
      add_proc_error "get_ps_info" "-1" "no usable data from ps command, host=$host, host_arch=$host_arch"
      return
   }
  
   set header [ lindex $help_list $x]
   
   # cut heading garbage and header line
   set ps_list [ lrange $help_list [expr $x + 1] [expr ([llength $help_list]-1)]]
   
#   puts $CHECK_OUTPUT "index names: \n$index_names" 
#   puts $CHECK_OUTPUT "          1         2         3         4         5         6         7         8         9"
#   puts $CHECK_OUTPUT "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
#   puts $CHECK_OUTPUT "header:\n$header"
   
   set s_index 0
   set indexcount [llength $index_names]
   foreach index $index_names { 
      incr indexcount -1
      set position1 [string first $index $header]
#      puts $CHECK_OUTPUT "\nstringlength of $index is [string length $index]"
#      puts $CHECK_OUTPUT "position1 is $position1"
      set last_position [expr ($position1 + [string length $index] - 1)]
      if {$indexcount == 0 } {
         set last_position 200
      }
      set first_position $s_index 
      set s_index [ expr ($last_position + 1 )]
      #puts $CHECK_OUTPUT "position of \"$index\" is from $first_position to $last_position"
      set read_header ""
      for { set i 0} {$i< $s_index} {incr i 1} {
          set read_header "!$read_header"
      }
      set header "$read_header[string range $header $s_index [expr ([string length $header])]]"
      #puts "header is now:\n$header"

      set pos1_list($index) $first_position
      set pos2_list($index) $last_position
   }

   set process_count 0
   foreach elem $ps_list {
#   puts $CHECK_OUTPUT $elem
#         set pid_pos     0
#         set gid_pos     1
#         set ppid_pos    2
#         set uid_pos     3
#         set state_pos   4
#         set stime_pos   5
#         set vsz_pos     6
#         set time_pos    7
#         set command_pos 8

      if {$pid_pos != -1} {
         set pid_index_name [lindex $index_names $pid_pos]
         set act_pid_string [string range $elem $pos1_list($pid_index_name)  $pos2_list($pid_index_name)]
         set act_pid [string trim $act_pid_string] 
      } else {
         set act_pid -1
      }
      #puts "$act_pid : \"$elem\""
      set psinfo($act_pid,error)  0
      set psinfo($act_pid,string) $elem
      set psinfo(string,$process_count) $elem
      set psinfo($act_pid,index_names) $index_names 
      set psinfo(pid,$process_count) $act_pid
      #puts "${variable}(pid,$process_count) = $act_pid"
    
#     PGID
      if {$gid_pos != -1} { 
         set name  [lindex $index_names $gid_pos]
         set value_str [string range $elem $pos1_list($name)  $pos2_list($name)]
      } else {
         set value_str "unknown"
      }
      set value [string trim $value_str] 
      set psinfo($act_pid,pgid) $value 
      set psinfo(pgid,$process_count) $value

#     PPID 
      if {$ppid_pos != -1} { 
         set name  [lindex $index_names $ppid_pos]
         set value_str [string range $elem $pos1_list($name)  $pos2_list($name)]
      } else {
         set value_str "unknown"
      }
      set value [string trim $value_str] 
      set psinfo($act_pid,ppid) $value 
      set psinfo(ppid,$process_count) $value

#     UID 
      if { $uid_pos != -1} {
         set name  [lindex $index_names $uid_pos]
         set value_str [string range $elem $pos1_list($name)  $pos2_list($name)]
      } else {
         set value_str "unknown"
      }
      set value [string trim $value_str] 
      set psinfo($act_pid,uid) $value
      set psinfo(uid,$process_count) $value
 
#     STATE 
      if { $state_pos != -1} {
         set name  [lindex $index_names $state_pos]
         set value_str [string range $elem $pos1_list($name)  $pos2_list($name)]
      } else {
         set value_str "unknown"
      }
      set value [string trim $value_str] 
      set psinfo($act_pid,state) $value 
      set psinfo(state,$process_count) $value

#     STIME 
      if { $stime_pos  != -1} {
         set name  [lindex $index_names $stime_pos]
         set value_str [string range $elem $pos1_list($name)  $pos2_list($name)]
      } else {
         set value_str "unknown"
      }
      set value [string trim $value_str] 
      set psinfo($act_pid,stime) $value 
      set psinfo(stime,$process_count) $value

#     VSZ
      if { $vsz_pos != -1} {  
         set name  [lindex $index_names $vsz_pos]
         set value_str [string range $elem $pos1_list($name)  $pos2_list($name)]
      } else {
         set value_str "unknown"
      }
      set value [string trim $value_str] 
      set psinfo($act_pid,vsz) $value 
      set psinfo(vsz,$process_count) $value

#     TIME
      if { $time_pos != -1} { 
         set name  [lindex $index_names $time_pos]
         set value_str [string range $elem $pos1_list($name)  $pos2_list($name)]
      } else {
         set value_str "unknown"
      }
      set value [string trim $value_str] 
      set psinfo($act_pid,time) $value 
      set psinfo(time,$process_count) $value

#     COMMAND
      if { $command_pos != -1} {
         set name  [lindex $index_names $command_pos]
         set value_str [string range $elem $pos1_list($name)  $pos2_list($name)]
      } else {
         set value_str "unknown"
      }
      set value [string trim $value_str] 
      set psinfo($act_pid,command) $value 
      set psinfo(command,$process_count) $value

      incr process_count 1
      set psinfo(proc_count) $process_count
   }
      
# PID  PGID  PPID   UID S    STIME  VSZ        TIME COMMAND

   # here is the merge of more ps commands happening
   switch -- $host_arch {
      "alinux" {
         if { $additional_run == 0 } { 
            # calling second ps
            get_ps_info $pid $host ps_add_run 1
            #puts $CHECK_OUTPUT "ps_add_run $pid is $ps_add_run($pid,string)"
            # now merge the relevant data
            for {set i 0} {$i < $psinfo(proc_count) } {incr i 1} {
               set act_pid $psinfo(pid,$i)
               #set act_pid $ps_add_run(pid,$i)
               if {[info exists ps_add_run($act_pid,vsz)]} {
                  #puts $CHECK_OUTPUT "     copy got value vsz for pid $act_pid"
                  #puts $CHECK_OUTPUT "       old value psinfo(vsz,$i) = $psinfo(vsz,$i)"
                  #puts $CHECK_OUTPUT "       old value psinfo(stime,$i) = $psinfo(stime,$i)"
                  set psinfo(vsz,$i) $ps_add_run($act_pid,vsz)
                  set psinfo(stime,$i) $ps_add_run($act_pid,stime)
                  #puts $CHECK_OUTPUT "       new value psinfo(vsz,$i) = $psinfo(vsz,$i)"
                  #puts $CHECK_OUTPUT "       new value psinfo(stime,$i) = $psinfo(stime,$i)"
                  #puts $CHECK_OUTPUT "        old value psinfo($act_pid,vsz) = $psinfo($act_pid,vsz)"
                  #puts $CHECK_OUTPUT "        old value psinfo($act_pid,stime) = $psinfo($act_pid,stime)"
                  set psinfo($act_pid,vsz) $ps_add_run($act_pid,vsz)
                  set psinfo($act_pid,stime) $ps_add_run($act_pid,stime)
                  #puts $CHECK_OUTPUT "        new value psinfo($act_pid,vsz) = $psinfo($act_pid,vsz)"
                  #puts $CHECK_OUTPUT "        new value psinfo($act_pid,stime) = $psinfo($act_pid,stime)"
                  
               } else {
                  puts $CHECK_OUTPUT "--> value vsz for pid $act_pid not found"
               }
            }
            return
         } 
         if { $additional_run == 1 } { 
            # second ps run
            return
         } 
      }
   }
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
      source "$TS_ROOT/check.exp"
      puts $CHECK_OUTPUT "master host is $CHECK_CORE_MASTER"
      puts $CHECK_OUTPUT "calling \"$procedure\" ..."
      set result [ eval $procedure ]
      puts $result 
      flush $CHECK_OUTPUT
   }
}

