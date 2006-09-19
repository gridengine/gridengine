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

# update an array with values of a second array
proc update_change_array { target_array change_array } {
   global CHECK_OUTPUT

   upvar $target_array target
   upvar $change_array chgar

   if [info exists chgar] {
      foreach elem [array names chgar] {
         set value [set chgar($elem)]
         puts $CHECK_OUTPUT "attribute \"$elem\" will be set to \"$value\""
         set target($elem) $value
      }
   }
}

# dump an array to a temporary file, return filename
proc dump_array_to_tmpfile { change_array } {
   global ts_config

   upvar $change_array chgar

   set tmpfile [ get_tmp_file_name ]
   set file [open $tmpfile "w"]

   if [info exists chgar] {
      foreach elem [array names chgar] {
         set value $chgar($elem)
         puts $file "$elem $value"
      }
   }

   close $file

   return $tmpfile
}

proc dump_lirs_array_to_tmpfile { change_array } {
   global ts_config CHECK_OUTPUT

   upvar $change_array chgar


   if [info exists chgar] {
      set old_name ""
      set first "true"

      set tmpfile [ get_tmp_file_name ]
      set file [open $tmpfile "w"]

      foreach elem [lsort [array names chgar]] {
         set help [split $elem ","]
         set name [lindex $help 0]
         set field [lindex $help 1]
         set value $chgar($elem)

         if { $old_name != $name} {
            # new lirs
            set old_name $name
            if { $first == "false" } {
               puts $file "\}"
            } else {
               set first "false"
            }
            puts $file "\{" 
            puts $file "name $name"
         }
         if { $field == "limit" } {
            foreach limit $value {
               puts $file "limit  $limit"
            }
         } else {
            puts $file "$field  $value"
         }
      } 

      puts $file "\}"
      close $file
   }

   return $tmpfile
}


#****** control_procedures/get_string_value_between() **************************
#  NAME
#     get_string_value_between() -- string parsing function
#
#  SYNOPSIS
#     get_string_value_between { start end line } 
#
#  FUNCTION
#     This function will return the content between the strings $start and
#     $end which must occur in $line.
#
#  INPUTS
#     start - first search parameter (first occurance)
#     end   - second search parameter 
#             if $start == $end: (last occurance)
#             if -1            : get content till end of $line 
#     line  - string to parse 
#
#  RESULT
#     string
#*******************************************************************************
proc get_string_value_between { start end line } {
   global CHECK_OUTPUT
   set pos1 [string first "$start" $line]
   incr pos1 [string length $start]

   if { $end != -1 } {
      if { $start == $end } {
         set pos2 [string last "$end"   $line]
      } else {
         set pos2 [string first "$end"   $line]
      }
      incr pos2 -1
      return [string trim [string range $line $pos1 $pos2]]
   } else {
 
      return [string trim [string range $line $pos1 end]]
   }
}

# take a name/value array and build a vi command to set new values
proc build_vi_command { change_array {current_array no_current_array_has_been_passed}} {
   global CHECK_OUTPUT
   upvar $change_array  chgar
   upvar $current_array curar

   set vi_commands "" 

   if {![info exists chgar]} {
      return ""
   }

   if {[info exists curar]} {
      # compare the new values to old ones
      foreach elem [array names chgar] {
        # this will quote any / to \/  (for vi - search and replace)
        set newVal $chgar($elem)
      
        if {[info exists curar($elem)]} {
           # if old and new config have the same value, create no vi command,
           # if they differ, add vi command to ...
           if { [string compare $curar($elem) $newVal] != 0 } {
              if {$newVal == ""} {
                 # ... delete config entry (replace by comment)
                 lappend vi_commands ":%s/^$elem .*$/#/\n"
              } else {
                 # ... change config entry
                 set newVal1 [split $newVal {/}]
                 set newVal [join $newVal1 {\/}]
                 lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
              }
           }
        } else {
           # if the config entry didn't exist in old config: append a new line
           if {$newVal != ""} {
              lappend vi_commands "A\n$elem  $newVal[format "%c" 27]"
           }
        }
     }
   } else {
      # we have no current values - just create a replace statement for each attribute
      foreach elem [array names chgar] {
         # this will quote any / to \/  (for vi - search and replace)
         set newVal $chgar($elem)
         if {$newVal != ""} {
            set newVal1 [split $newVal {/}]
            set newVal [join $newVal1 {\/}]
            lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
         }
      }
   }

   return $vi_commands
}

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
#     {additional_expected_result2 "___ABCDEFG___"} 
#     {additional_expected_result3 "___ABCDEFG___"}} 
#     {qconf_error_msg "___ABCDEFG___"}
#     {raise_error  1} }
#
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
#     {additional_expected_result3 "___ABCDEFG___"} - additional expected_result
#     {qconf_error_msg "___ABCDEFG___"}            - qconf error message 
#     {raise_error  1}                                - do add_proc_error in case of errors
#
#
#  RESULT
#     0 when the output of the application contents the expected_result 
#    -1 on timeout or other error
#    -2 on additional_expected_result
#    -3 on additional_expected_result2 
#    -4 on additional_expected_result3
#    -9 on chekcpointing qconf_error_msg
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
proc handle_vi_edit { prog_binary prog_args vi_command_sequence expected_result {additional_expected_result "___ABCDEFG___"} {additional_expected_result2 "___ABCDEFG___"} {additional_expected_result3 "___ABCDEFG___"} {additional_expected_result4 "___ABCDEFG___"} {qconf_error_msg "___ABCDEFG___"} {raise_error 1}} {
   global CHECK_OUTPUT env CHECK_HOST CHECK_DEBUG_LEVEL CHECK_USER

   set expected_result              [string trimright $expected_result "*"]
   set additional_expected_result   [string trimright $additional_expected_result "*"]
   set additional_expected_result2  [string trimright $additional_expected_result2 "*"]
   set additional_expected_result3  [string trimright $additional_expected_result3 "*"]
   set additional_expected_result4  [string trimright $additional_expected_result4 "*"]
   set qconf_error_msg  [string trimright $qconf_error_msg "*"]

   # we want to start a certain configured vi, and have no backslash continued lines
   set vi_env(EDITOR) [get_binary_path "$CHECK_HOST" "vim"]
   set result -100

   debug_puts "using EDITOR=$vi_env(EDITOR)"
   # start program (e.g. qconf)
   set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER $prog_binary "$prog_args" 0 vi_env]
   set sp_id [ lindex $id 1 ] 
   if {$CHECK_DEBUG_LEVEL != 0} {
      log_user 1
      set send_speed .001
   } else {
      log_user 0 ;# set to 1 if you wanna see vi responding
      set send_speed .0005
   }
   set send_slow "1 $send_speed" 

   debug_puts "now waiting for vi start ..."
   set error 0

   set timeout 10
   expect {
      -i $sp_id full_buffer {
         add_proc_error "handle_vi_edit" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
         set error 1
      }

      -i $sp_id eof {
         set error 1
         add_proc_error "handle_vi_edit" -1 "unexpected end of file"
      }

      -i $sp_id timeout {  
         set error 1
         add_proc_error "handle_vi_edit" -2 "timeout - can't start vi"
      }
      -i $sp_id  "_start_mark_*\n" {
         debug_puts "starting now!"
      }
   }

   set timeout 10
   expect {
      -i $sp_id full_buffer {
         set error 1
         add_proc_error "handle_vi_edit" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
      }

      -i $sp_id eof {
         set error 1
         add_proc_error "handle_vi_edit" -1 "unexpected end of file"
      }


      -i $sp_id "$qconf_error_msg" {
         set error $raise_error
         add_proc_error "handle_vi_edit"  -1 "$qconf_error_msg" $raise_error
         set result -9
         close_spawn_process $id
         return -9
      }

      -i $sp_id timeout {  
         set error 1
         add_proc_error "handle_vi_edit" -2 "timeout - can't start vi"
      }
      -i $sp_id  {[A-Za-z]*} {
         debug_puts "vi should run now ..."
      }
   }

   
   
   set timeout 1
   # wait for vi to startup and go to first line
   send -s -i $sp_id -- "G"
   set timeout_count 0

   expect {
      -i $sp_id full_buffer {
         add_proc_error "handle_vi_edit" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
         set error 1
      }

      -i $sp_id eof {
         add_proc_error "handle_vi_edit" -1 "unexpected end of file"
         set error 1
      }

      -i $sp_id timeout {  
         send -s -i $sp_id -- "G"
         incr timeout_count 1
         if { $timeout_count > 60 } {
            add_proc_error "handle_vi_edit" -2 "timeout - vi doesn't respond"
            set error 1
         } else {
            exp_continue
         }
      }

      -i $sp_id  "100%" {
      }
      
      -i $sp_id  "o lines in buffer" {
      }
      
      -i $sp_id  "erminal too wide" {
         add_proc_error "handle_vi_edit" -2 "got terminal to wide vi error"
         set error 1
      }
   }

   # we had an error during vi startup - close connection and return with error
   if {$error} {
      # maybe vi is up and we can exit
      send -s -i $sp_id -- "[format "%c" 27]" ;# ESC
      send -s -i $sp_id -- ":q!\n"            ;# exit without saving
      set timeout 10
      expect {
         -i $sp_id full_buffer {
            add_proc_error "handle_vi_edit" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
         }

         -i $sp_id eof {
            add_proc_error "handle_vi_edit" -1 "unexpected end of file"
         }

         -i $sp_id "_exit_status*\n" {
            debug_puts "vi terminated! (1)"
            exp_continue
         }

      }


      # close the connection - hopefully vi and/or the called command will exit
      close_spawn_process $id

      return -1
   }

   # second waiting: Part I:
   # =======================
   # set start time (qconf must take at least one second, because he
   # does a file stat to find out if the file was changed, so the
   # file edit process must take at least 1 second

   set start_time [clock clicks -milliseconds]
   # send the vi commands
   set timeout 1
   set timeout_count 0
   set sent_vi_commands 0
   send -s -i $sp_id -- "1G"      ;# go to first line


   foreach elem $vi_command_sequence {
      incr sent_vi_commands 1
      set com_length [ string length $elem ]
      set com_sent 0
      send -s -i $sp_id -- "$elem"
      send -s -i $sp_id -- "G"
      set timeout 1
      expect {
         -i $sp_id full_buffer {
            add_proc_error "handle_vi_edit" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
            set error 1
         }

         -i $sp_id eof {
            add_proc_error "handle_vi_edit" -1 "unexpected end of file"
            set error 1
         }
         -i $sp_id "*Hit return*" {
            send -s -i $sp_id -- "\n"
            debug_puts "found Hit return"
            exp_continue
         }
         -i $sp_id timeout {
            incr timeout_count 1
            if { $timeout_count > 15 } {
               set error 2
            } else {
               send -s -i $sp_id -- "G"
               exp_continue
            }
         }

         -i $sp_id "100%" {
         }
      }
      flush $CHECK_OUTPUT
      if { $error != 0 } {
         break
      }
   }

   if { $error == 0 } {
      
      # second waiting: Part II:
      # =======================
      # wait for file time older one second
      # we give an extra waiting time of 100 ms to be sure that
      # the vi takes at least 1 second
      set end_time [expr [clock clicks -milliseconds] + 1100 ]
      while { [clock clicks -milliseconds] < $end_time } { 
         after 100
      }
      set run_time [expr [clock clicks -milliseconds] - $start_time]

      # save and exit
      if { $CHECK_DEBUG_LEVEL != 0 } {
         after 3000
      }
      send -s -i $sp_id -- ":wq\n"
      set timeout 60

      # we just execute and don't wait for a certain result:
      # wait for exit status of command
      if { [string compare "" $expected_result ] == 0 } {
         expect {
            -i $sp_id full_buffer {
               add_proc_error "handle_vi_edit" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
               set result -1
            }
            -i $sp_id timeout {
               add_proc_error "handle_vi_edit" -1 "timeout error:$expect_out(buffer)"
               set result -1
            }
            -i $sp_id eof {
               add_proc_error "handle_vi_edit" -1 "eof error:$expect_out(buffer)"
               set result -1
            }
            -i $sp_id "_exit_status_" {
               debug_puts "vi terminated! (2) (rt=$run_time)"
               set result 0
               exp_continue
            }

        }
      } else {
         # we do expect certain result(s)
         # wait for result and/or exit status

         expect {
            -i $sp_id full_buffer {
               add_proc_error "handle_vi_edit" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
               set result -1
            }
            -i $sp_id timeout {
               set result -1
               add_proc_error "handle_vi_edit" -1 "timeout error:$expect_out(buffer)"
            }
            -i $sp_id eof {
               add_proc_error "handle_vi_edit" -1 "eof error:$expect_out(buffer)"
               set result -1
            }
            -i $sp_id  "$expected_result" {
               set result 0
               exp_continue
            }
            -i $sp_id  "$additional_expected_result" {
               set result -2
               exp_continue
            }
            -i $sp_id  "$additional_expected_result2" {
               set result -3
               exp_continue
            }
            -i $sp_id  "$additional_expected_result3" {
               set result -4
               exp_continue
            }
            -i $sp_id  "$additional_expected_result4" {
               set result -5
               exp_continue
            }
            
            -i $sp_id "_exit_status_" {
               debug_puts "vi terminated! (3)  (rt=$run_time)"
               if { $result == -100 } {
                  set pos [string last "\n" $expect_out(buffer)]
                  incr pos -2
                  set buffer_message [string range $expect_out(buffer) 0 $pos ]
                  set pos [string last "\n" $buffer_message]
                  incr pos 1
                  set buffer_message [string range $buffer_message $pos end] 

                  set message_txt ""
                  append message_txt "expect_out(buffer)=\"$expect_out(buffer)\""
                  append message_txt "expect out buffer is:\n"
                  append message_txt "   \"$buffer_message\"\n"
                  append message_txt "this doesn't match any given expression:\n"
                  append message_txt "   \"$expected_result\"\n"
                  append message_txt "   \"$additional_expected_result\"\n"
                  append message_txt "   \"$additional_expected_result2\"\n"
                  append message_txt "   \"$additional_expected_result3\"\n"
                  append message_txt "   \"$additional_expected_result4\"\n"
                  add_proc_error "handle_vi_edit" -1 $message_txt
               }
            }
         }
      }
      debug_puts "sent_vi_commands = $sent_vi_commands"
      if { $sent_vi_commands == 0 } {
         debug_puts "INFO: there was NO vi command sent!"
      }
   } else {
      if { $error == 2 } {
         send -s -i $sp_id -- "[format "%c" 27]" ;# ESC
         send -s -i $sp_id -- "[format "%c" 27]" ;# ESC
         send -s -i $sp_id -- ":q!\n"            ;# exit without saving
         set timeout 10
         expect -i $sp_id "_exit_status_"
         debug_puts "vi terminated! (4)"
         close_spawn_process $id
         set error_text ""
         append error_text "got timeout while sending vi commands\n"
         append error_text "please make sure that no single vi command sequence\n"
         append error_text "leaves the vi in \"insert mode\" !!!"
         add_proc_error "handle_vi_edit" -1 $error_text
         return -1
      }
   }

   close_spawn_process $id

   # output what we have just done
   log_user 1
   foreach elem $vi_command_sequence {
      debug_puts "sequence: $elem"
      if { [string first "A" $elem ] != 0 } {
         set index1 [ string first "." $elem ]
         incr index1 -2
         set var [ string range $elem 5 $index1 ] 
        

         # TODO: CR - the string last $var index1 position setting
         #            is buggy, because it assumes that the value 
         #            doesn't contain $var.
         #
         #       example: load_sensor /path/load_sensor_script.sh
         #         this would return "_script.sh" as value
         #
         #       Value is only used for printing the changes to the user,
         #       so this is not "really" a bug
         #       
         set index1 [ string last "$var" $elem ]
         incr index1 [ string length $var]
         incr index1 2
   
         set index2 [ string first "\n" $elem ]
         incr index2 -2
   
         set value [ string range $elem $index1 $index2 ]
         set value [ split $value "\\" ]
         set value [ join $value "" ]
         if { [ string compare $value "*$/" ] == 0 || [ string compare $value "*$/#" ] == 0 } {
            debug_puts "--> removing \"$var\" entry"
         } else {
            if { [ string compare $var "" ] != 0 && [ string compare $value "" ] != 0  } {         
               debug_puts "--> setting \"$var\" to \"${value}\""
            } else {
               if { [string compare $elem [format "%c" 27]] == 0 } {
                  debug_puts "--> vi command: \"ESC\""    
               } else {
                  set output [replace_string $elem "\n" "\\n"]
                  debug_puts "--> vi command: \"$output\"" 
               }
            }
         }
      } else {
         set add_output [ string range $elem 2 end ]
         debug_puts "--> adding [string trim $add_output "[format "%c" 27] ^"]"
      }
   }

   # debug output end
   if {$CHECK_DEBUG_LEVEL != 0} {
      log_user 1
   } else {
      log_user 0 
   }

   return $result
}


#****** control_procedures/get_uid() *******************************************
#  NAME
#     get_uid() -- get user id for user on host
#
#  SYNOPSIS
#     get_uid { user host } 
#
#  FUNCTION
#     The function returns the user id of user $user on host $host
#
#  INPUTS
#     user - username
#     host - hostname 
#
#  RESULT
#     string containing user id
#
#  SEE ALSO
#     control_procedures/get_uid()
#     control_procedures/get_gid()
#*******************************************************************************
proc get_uid { user host } {
   global CHECK_OUTPUT
   set my_uid -1

   set output [start_remote_prog $host $user id ""]
   set output [string trim [split $output " =()"]]
   set found_uid 0
   foreach line $output {
      if { $found_uid == 1 } {
         set my_uid $line
         break
      }
      if { $line == "uid" } {
         set found_uid 1
      }
   }
   return $my_uid
}


#****** control_procedures/get_gid() *******************************************
#  NAME
#     get_gid() -- get group id for user on host
#
#  SYNOPSIS
#     get_gid { user host } 
#
#  FUNCTION
#     The function returns the group id of user $user on host $host
#
#  INPUTS
#     user - username 
#     host - hostname 
#
#  RESULT
#     string containing group id
#
#  SEE ALSO
#     control_procedures/get_uid()
#     control_procedures/get_gid()
#*******************************************************************************
proc get_gid { user host } {
   global CHECK_OUTPUT
   set my_gid -1

   set output [start_remote_prog $host $user id ""]
   set output [string trim [split $output " =()"]]
   set found_gid 0
   foreach line $output {
      if { $found_gid == 1 } {
         set my_gid $line
         break
      }
      if { $line == "gid" } {
         set found_gid 1
      }
   }
   return $my_gid
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
      "sol-sparc64" - 
      "sol-amd64" -
      "solaris86" -
      "sol-x86" -
      "solaris" -
      "sol-sparc" {
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
     
      "darwin" -
      "darwin-ppc" -
      "darwin-x86" {
         set myenvironment(COLUMNS) "500"
         set result [start_remote_prog "$host" "$CHECK_USER" "ps" "-awwx -o \"pid=_____pid\" -o \"pgid=_____pgid\" -o \"ppid=_____ppid\" -o \"uid=_____uid\" -o \"state=_____s\" -o \"stime=_____stime\" -o \"vsz=_____vsz\" -o \"time=_____time\" -o \"command=_____args\"" prg_exit_state 60 0 myenvironment]
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

      "irix6" -
      "irix65" { 
         set myenvironment(COLUMNS) "500"
         set result [start_remote_prog "$host" "$CHECK_USER" "ps" "-eo \"pid,pgid,ppid,uid=LONGUID,state,stime,vsz,time,args\"" prg_exit_state 60 0 myenvironment]
         set index_names "  PID  PGID  PPID LONGUID S    STIME {VSZ   }        TIME COMMAND"
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
 
      "aix43" -
      "aix51" {
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
     
      "glinux" -
      "lx24-x86" -
      "lx26-x86" - 
      "lx24-ia64" -
      "lx26-ia64" - 
      "lx24-amd64" - 
      "lx26-amd64" -
      "lx24-sparc" - 
      "lx26-sparc" -
      "lx24-sparc64" - 
      "lx26-sparc64" {
         set myenvironment(COLUMNS) "500"
         set result [start_remote_prog "$host" "$CHECK_USER" "ps" "-weo \"pid pgid ppid uid=BIGGERUID s stime vsz time args\"" prg_exit_state 60 0 myenvironment]
         set index_names "  PID  PGID  PPID BIGGERUID S STIME   VSZ     TIME COMMAND"
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

      "alinux" -
      "lx22-alpha" -
      "lx24-alpha" {
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

      "win32-x86" {
         set myenvironment(COLUMS) "500"
         set result [start_remote_prog "$host" "$CHECK_USER" "ps" "-efo pid,group=\"GROUPNAMEGROUPNAMEGROUPNAME\",ppid,user=\"FULLUSERNAMEFULLUSERNAMEFULLUSERNAME\",state,stime,vsz,time,comm=\"COMMANDCOMMANDCOMMANDCOMMANDCOMMANDCOMMANDCOMMANDCOMMANDCOMMANDCOMMAND\"" prg_exit_state 60 0 myenvironment]
         set index_names "   PID GROUPNAMEGROUPNAMEGROUPNAME   PPID FULLUSERNAMEFULLUSERNAMEFULLUSERNAME STATE       STIME    VSZ     TIME COMMANDCOMMANDCOMMANDCOMMANDCOMMANDCOMMANDCOMMANDCOMMANDCOMMANDCOMMAND"
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

#   set fdp [open "psinfo.txt" "w"]
#   foreach elem $help_list {
#      puts $fdp $elem
#   }
#   close $fdp


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

#                                                             max. column:     |
#****** control_procedures/gethostname() ******
# 
#  NAME
#     gethostname -- ??? 
#
#  SYNOPSIS
#     gethostname { } 
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
#*******************************
proc gethostname {} {
  global ts_config CHECK_ARCH CHECK_OUTPUT env

  set catch_return [ catch { exec "$ts_config(product_root)/utilbin/$CHECK_ARCH/gethostname" "-name"} result ]
  if { $catch_return == 0 } {
     set result [split $result "."]
     set newname [lindex $result 0]
     return $newname
  } else {
     debug_puts "proc gethostname - gethostname error or binary not found"
     debug_puts "error: $result"
     debug_puts "error: $catch_return"
     debug_puts "trying local hostname call ..."
     set catch_return [ catch { exec "hostname" } result ]
     if { $catch_return == 0 } {
        set result [split $result "."]
        set newname [lindex $result 0]
        debug_puts "got hostname: \"$newname\""
        return $newname
     } else {
        debug_puts "local hostname error or binary not found"
        debug_puts "error: $result"
        debug_puts "error: $catch_return"
        debug_puts "trying local HOST environment variable ..."
        if { [ info exists env(HOST) ] } {
           set result [split $env(HOST) "."]
           set newname [lindex $result 0]
           if { [ string length $newname ] > 0 } {
               debug_puts "got hostname_ \"$newname\""
               return $newname
           } 
        }
     }
     return "unknown"
  }
} 



#                                                             max. column:     |
#****** control_procedures/resolve_arch() ******
# 
#  NAME
#     resolve_arch -- resolve architecture of host
#
#  SYNOPSIS
#     resolve_arch { {node "none"} {use_source_arch 0}} 
#
#  FUNCTION
#     Resolves the architecture of a given host.
#     Tries to call $SGE_ROOT/util/arch - if this script doesn't exist yet,
#     calls <source_dir>/dist/util/arch.
#
#     If the parameter use_source_arch is set, the function will always
#     call <source_dir>/dist/util/arch.
#     This is for example required when building new binaries:
#     The installed arch script might return a different architecture than
#     the source arch script, for example when a cluster was installed from
#     our Grid Engine packages, where we deliver lx-24-* packages also for
#     Linux kernel 2.6 machines (lx26-*), or hp11 packages for hp11-64.
#
#  INPUTS
#     {node "none"}     - return architecture of this host.
#                         If "none", resolve architecture of CHECK_HOST.
#     {use_source_arch} - use <source_dir>/dist/util/arch script.
#
#  RESULT
#     Architecture string (e.g. "sol-amd64"), "unknown" in case of errors.
#
#  SEE ALSO
#     control_procedures/resolve_arch_clear_cache()
#*******************************
proc resolve_arch {{node "none"} {use_source_arch 0}} {
   global ts_config CHECK_OUTPUT
   global CHECK_USER CHECK_SOURCE_DIR CHECK_HOST
   global arch_cache

   set host [node_get_host $node]

   if {[info exists arch_cache($host)]} {
      return $arch_cache($host)
   }

   if { [ info exists CHECK_USER ] == 0 } {
      puts $CHECK_OUTPUT "user not set, aborting"
      return "unknown"
   }
  
   if { [ info exists CHECK_SOURCE_DIR ] == 0 } {
      debug_puts "source directory not set, aborting"
      return "unknown"
   }

   if {$host == "none"} {
      set host $CHECK_HOST
   }

   # if $SGE_ROOT/util/arch is available, use this one,
   # otherwise use the one from the distribution
   if {[file exists "$ts_config(product_root)/util/arch"] && ! $use_source_arch} {
      set arch_script "$ts_config(product_root)/util/arch"
   } else {
      set arch_script "$CHECK_SOURCE_DIR/dist/util/arch"
   }

   # try to retrieve architecture
   set result [start_remote_prog $host $CHECK_USER $arch_script "" prg_exit_state 60 0 "" 1 0 0]
   if {$prg_exit_state != 0} {
      return "unknown"
   }

  set result [string trim $result]
  set result2 [split $result "\n"]
  if { [ llength $result2 ] > 1 } {
     puts $CHECK_OUTPUT "util/arch script returns more than 1 line output ..."
     foreach elem $result2  {
        puts $CHECK_OUTPUT "\"$elem\""
        if { [string first " " $elem ] < 0  } {
           set result $elem
           puts $CHECK_OUTPUT "using \"$result\" as architecture"
           break
        }
     }
  }
  if { [ llength $result2 ] < 1 } {
      puts $CHECK_OUTPUT "util/arch script returns no value ..."
      return "unknown"
  }
  if { [string first ":" $result] >= 0 } {
     puts $CHECK_OUTPUT "architecture or file \"$CHECK_SOURCE_DIR/dist/util/arch\" not found"
     return "unknown"
  }
  set result [lindex $result 0]  ;# remove CR

  if { [ string compare $result "" ] == 0 } {
     puts $CHECK_OUTPUT "architecture or file \"$CHECK_SOURCE_DIR/dist/util/arch\" not found"
     return "unknown"
  } 

  set arch_cache($host) [lindex $result 0]
  
  return $arch_cache($host)
}

#****** control_procedures/resolve_arch_clear_cache() **************************
#  NAME
#     resolve_arch_clear_cache() -- clear cache of resolve_arch()
#
#  SYNOPSIS
#     resolve_arch_clear_cache { } 
#
#  FUNCTION
#     The function resolve_arch caches its results.
#     resolve_arch_clear_cache will clear this cache to force reresolving
#     the architecture strings.
#
#     This is for example done after compiling and installing binaries.
#     In this case the newly installed arch script might return other 
#     architecture names than the previously installed one.
#
#  SEE ALSO
#     control_procedures/resolve_arch()
#*******************************************************************************
proc resolve_arch_clear_cache {} {
   global CHECK_OUTPUT arch_cache

   puts $CHECK_OUTPUT "clearing architecture cache used by resolve_arch"
   if {[info exists arch_cache]} {
      unset arch_cache
   }
}

#                                                             max. column:     |
#****** control_procedures/resolve_build_arch() ******
# 
#  NAME
#     resolve_build_arch -- ??? 
#
#  SYNOPSIS
#     resolve_build_arch { host } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     host - ??? 
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
proc resolve_build_arch { host } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT CHECK_TESTSUITE_ROOT build_arch_cache CHECK_SOURCE_DIR
  global CHECK_USER
  if { [info exists build_arch_cache($host) ] } {
     return $build_arch_cache($host)
  }

  set result [ start_remote_prog $host $CHECK_USER "cd" "$CHECK_SOURCE_DIR ; ./aimk -no-mk" prg_exit_state 60 0 "" 1 0]
 
  set result [split $result "\n"]
  set result [join $result ""]
  set result [split $result "\r"]
  set result [join $result ""]

  if { $prg_exit_state != 0 } {
     add_proc_error "resolve_build_arch" "-1" "architecture not found or aimk not found in $CHECK_SOURCE_DIR"
     return ""
  }
  set build_arch_cache($host) $result
  puts $CHECK_OUTPUT "build arch is \"$result\""

  return $build_arch_cache($host)
}

proc resolve_build_arch_installed_libs {host {raise_error 1}} {
   global ts_config CHECK_OUTPUT

   set build_arch [resolve_build_arch $host]

   # we need special handling for some architectures, e.g. HP11 64bit
   switch $build_arch {
      "HP1164" {
         set arch [resolve_arch $host]
         if {$arch == "hp11"} {
            add_proc_error "resolve_build_arch_installed_lib" -3 "We are on hp11 64bit platform (build platform HP1164) with 32bit binaries installed.\nUsing hp11 (build platform HP11) test binaries" $raise_error
            set build_arch "HP11"
         }
      }
      "LINUX86_26" {
         set arch [resolve_arch $host]
         if {$arch == "lx24-x86"} {
            add_proc_error "resolve_build_arch_installed_lib" -3 "We are on lx26-x86 platform (build platform LINUX86_26) with lx24-x86 binaries installed.\nUsing lx24-x86 (build platform LINUX86_24) test binaries" $raise_error
            set build_arch "LINUX86_24"
         }
      }
   }

   return $build_arch
}

#                                                             max. column:     |
#****** control_procedures/resolve_host() ******
# 
#  NAME
#     resolve_host -- ??? 
#
#  SYNOPSIS
#     resolve_host { name { long 0 } } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     name       - ??? 
#     { long 0 } - ??? 
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
proc resolve_host { name { long 0 } } {
   global ts_config
   global CHECK_OUTPUT
   global resolve_host_cache

   # we cannot resolve hostgroups.
   if {[string range $name 0 0] == "@" } {
      puts $CHECK_OUTPUT "hostgroups ($name) cannot be resolved"
      return $name
   }
   
   if { $long != 0 } {
      if {[info exists resolve_host_cache($name,long)]} {
         return $resolve_host_cache($name,long)
      }
   } else {
      if {[info exists resolve_host_cache($name,short)]} {
         return $resolve_host_cache($name,short)
      }
   }

   set remote_arch [ resolve_arch $ts_config(master_host) ]

   set result [ start_remote_prog $ts_config(master_host) "ts_def_con_translate" "$ts_config(product_root)/utilbin/$remote_arch/gethostbyname" "-aname $name" prg_exit_state 60 0 "" 0 ]

  if { $prg_exit_state != 0 } {
     puts $CHECK_OUTPUT "proc resolve_host - gethostbyname error or file \"$ts_config(product_root)/utilbin/$remote_arch/gethostbyname\" not found: \n$result"
     return "unknown"
  }

  set newname [string trim $result]
  if { $long == 0 } {
     set split_name [split $newname "."]
     set newname [lindex $split_name 0]
  }


  # cache result
  if { $long != 0 } {
     set resolve_host_cache($name,long) $newname
     puts $CHECK_OUTPUT "long resolve_host: \"$name\" resolved to \"$newname\""
  } else {
     set resolve_host_cache($name,short) $newname
     puts $CHECK_OUTPUT "short resolve_host: \"$name\" resolved to \"$newname\""
  }
  return $newname
}


#****** control_procedures/resolve_queue() *************************************
#  NAME
#     resolve_queue() -- resolve queue instance name
#
#  SYNOPSIS
#     resolve_queue { queue } 
#
#  FUNCTION
#     This function resolves the hostname of the queue instance and returns 
#     the corresponding name
#
#  INPUTS
#     queue - queue name e.g. "queue1@testhost"
#
#*******************************************************************************
proc resolve_queue { queue } { 
   global CHECK_OUTPUT
   set at_sign [string first "@" $queue]
   set new_queue_name $queue
   if { $at_sign >= 0 } {
      incr at_sign 1
      set host_name  [string range $queue $at_sign end]
      incr at_sign -2
      set queue_name [string range $queue 0 $at_sign]
      debug_puts "queue name:          \"$queue_name\""
      debug_puts "host name:           \"$host_name\""
      set resolved_name [resolve_host $host_name 1]
      if { $resolved_name != "unknown" } {
         set resolved_host_name $resolved_name
         debug_puts "resolved host name:  \"$resolved_host_name\""
         set new_queue_name "$queue_name@$resolved_host_name"
      } else {
         puts $CHECK_OUTPUT "can't resolve host \"$host_name\""
      }
   }
   debug_puts "queue \"$queue\" resolved to \"$new_queue_name\""

   if { [string length $new_queue_name] > 30 } {
      add_proc_error "resolve_queue" -3 "The length of the queue name \"$new_queue_name\" will exceed qstat queue name output"
   }

   return $new_queue_name 
}

# main
#if { [info exists argc ] != 0 } {
#   set TS_ROOT ""
#   set procedure ""
#   for { set i 0 } { $i < $argc } { incr i } {
#      if {$i == 0} { set TS_ROOT [lindex $argv $i] }
#      if {$i == 1} { set procedure [lindex $argv $i] }
#   }
#   if { $argc == 0 } {
#      puts "usage:\n$module_name <CHECK_TESTSUITE_ROOT> <proc> no_main <testsuite params>"
#      puts "options:"
#      puts "CHECK_TESTSUITE_ROOT -  path to TESTSUITE directory"
#      puts "proc                 -  procedure from this file with parameters"
#      puts "no_main              -  used to source testsuite file (check.exp)"
#      puts "testsuite params     -  any testsuite command option (from file check.exp)"
#      puts "                        testsuite params: file <path>/defaults.sav is needed"
#   } else {
#      source "$TS_ROOT/check.exp"
#      puts $CHECK_OUTPUT "master host is $CHECK_CORE_MASTER"
#      puts $CHECK_OUTPUT "calling \"$procedure\" ..."
#      set result [ eval $procedure ]
#      puts $result 
#      flush $CHECK_OUTPUT
#   }
#}

proc get_pid_from_file { pid_file } {
   set pid ""
   for {set x 0} {$x < 10} {incr x} {
      if [file exists $pid_file] {
         if {[file size $pid_file] > 0 } {
            set f [open $pid_file r]
            gets $f pid
            close $f

            if { $pid != "" } { 
               break
            }
         }
      }   
      after 500
   }

   return $pid
}

proc get_qmaster_pid {} {
   set qmaster_spool_dir [ get_qmaster_spool_dir ]

   set pid_file "$qmaster_spool_dir/qmaster.pid"

   return [get_pid_from_file $pid_file]
}

proc get_schedd_pid {} {
   set qmaster_spool_dir [ get_qmaster_spool_dir ]

   set pid_file "$qmaster_spool_dir/schedd/schedd.pid"

   return [get_pid_from_file $pid_file]
}

proc parse_cpu_time {s_cpu} {
   set l_cpu [split $s_cpu ":"]
   set cpu 0

   while {[llength $l_cpu] > 0} {
      scan [lindex $l_cpu 0] "%02d" part
      
      switch [llength $l_cpu] {
         1 {
            incr cpu $part
         }
         2 {
            incr cpu [expr $part * 60]
         }
         3 {
            incr cpu [expr $part * 3600]
         }
         default {
            add_proc_error "usage_parse_cpu" -1 "cannot parse cpu time $s_cpu"
         }
      }

      set l_cpu [lreplace $l_cpu 0 0]
   }

   return $cpu
}

#****** control_procedures/operational_lock() **********************************
#  NAME
#     operational_lock() -- sychronizes an operation using file-based locks
#
#  SYNOPSIS
#     operational_lock { operation_name lock_location }
#
#  FUNCTION
#     This function uses a file lock in the lock_location directory to prevent
#     multiple instances of a test suite operation from occuring simultaneously.
#     No two test suite instances maybe be within a proetcted section of code
#     on the same machine with the same operation_name and the same
#     lock_location.  If the lock_location is not specified, it defaults to
#     /tmp.
#     This algorithm is based on "ls -crt | head -1" returning the oldest lock
#     file.  In this way, clients receive the lock on a first come, first served
#     basis.
#
#  INPUTS
#     operation_name - the name of the lock
#     host           - the name of the host to lock. Defaults to $CHECK_HOST
#     lock_location  - where to store the locks.  Defaults to /tmp.
#
#  RESULTS
#     -1   - error
#      0   - success
#
#  SEE ALSO
#     control_procedures/operational_lock()
#
#*******************************************************************************
proc operational_lock {operation_name {host ""} {lock_location "/tmp"}} {
   global CHECK_USER CHECK_HOST CHECK_OUTPUT

   if {$host == ""} {
      set host $CHECK_HOST
   }

   set pid [pid]
   set lock "$lock_location/lock.$operation_name.$pid"
   set all_locks "$lock_location/lock.$operation_name.*"

   set output [start_remote_prog $CHECK_HOST $CHECK_USER "touch" $lock result]

   if {$result != 0} {
      add_proc_error "operational_lock" -1 "Could not update lock: $output"
      return -1
   }

   # ls -crt behaves approximately the same on all platforms.  On HP-UX and
   # IRIX, symbolic links are not included in the sorting, but since we're not
   # using symbolic links, it shouldn't be an issue.
   set output [start_remote_prog $CHECK_HOST $CHECK_USER "ls" "-crt $all_locks | head -1" result]

   if {$result != 0} {
      add_proc_error "operational_lock" -1 "Could not read locks: $output"
      return -1
   }

   while {[string trim $output] != $lock} {
      puts $CHECK_OUTPUT "Waiting for lock"
      after 1000

      set output [start_remote_prog $CHECK_HOST $CHECK_USER "ls" "-crt $all_locks | head -1" result]

      if {$result != 0} {
         add_proc_error "operational_lock" -1 "Could not read locks: $output"
         return -1
      }
   }

   return 0
}

#****** control_procedures/operational_unlock() ********************************
#  NAME
#     operational_unlock() -- sychronizes an operation using file-based locks
#
#  SYNOPSIS
#     operational_unlock { operation_name lock_location }
#
#  FUNCTION
#     This function removes the file lock in the lock_location directory
#     allowing other processes access to the specified operation.  If the
#     lock_location is not specified, it defaults to /tmp.
#
#  INPUTS
#     operation_name - the name of the lock
#     host           - the name of the host to lock. Defaults to $CHECK_HOST
#     lock_location  - where to store the locks.  Defaults to /tmp.
#
#  RESULTS
#     -1   - error
#      0   - success
#
#  SEE ALSO
#     control_procedures/operational_lock()
#
#*******************************************************************************
proc operational_unlock {operation_name {host ""} {lock_location "/tmp"}} {
   global CHECK_USER CHECK_HOST

   if {$host == ""} {
      set host $CHECK_HOST
   }

   set pid [pid]
   set lock "$lock_location/lock.$operation_name.$pid"

   set output [start_remote_prog $CHECK_HOST $CHECK_USER "rm" $lock result]

   if {$result != 0} {
      add_proc_error "operational_lock" -1 "Could not release lock: $output"
      return -1
   }

   return 0
}


#****** control_procedures/scale_timeout() *************************************
#  NAME
#     scale_timeout() -- scale timeout values
#
#  SYNOPSIS
#     scale_timeout { timeout {does_computation 1} {does_spooling 1} 
#     {process_invocations 1} } 
#
#  FUNCTION
#     Scales a given timeout value depending on setup.
#     The given timeout is increased, when
#        o we use classic spooling
#        o we spool on a NFS filesystem
#        o we run with code coverage
#
#  INPUTS
#     timeout                 - base timeout
#     {does_computation 1}    - is the tested
#     {does_spooling 1}       - ??? 
#     {process_invocations 1} - ??? 
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
proc scale_timeout {timeout {does_computation 1} {does_spooling 1} {process_invocations 1}} {
   global ts_config

   set ret $timeout

   # respect spooling influence
   if {$does_spooling} {
      # if we use a RPC server, assume 100% slower spooling
      if {$ts_config(bdb_server) != "none"} {
         set ret [expr $ret * 2.0]
      } else {
         # classic spooling is slower than BDB, assume 100% slower spooling
         if {$ts_config(spooling_method) == "classic"} {
            set ret [expr $ret * 2.0]
            set spool_dir [get_qmaster_spool_dir]
         } else {
            set spool_dir [get_bdb_spooldir]
         }

         # spooling on NFS mounted filesystem, assume 50% slower spooling
         set fstype [get_fstype $spool_dir]
         if {[string match "nfs*" $spool_dir]} {
            set ret [expr $ret * 1.5]
         }
      }
   }

   # respect code coverage influence
   # we assume that the process will run slightly slower
   if {[coverage_enabled]} {
      # computation will be slower - add 10% overhead
      if {$does_computation} {
         set ret [expr $ret * 1.10]
      }

      # coverage profiles are written per process invocation
      # add 1 second overhead per process invocation
      set ret [expr $ret + $process_invocations * 1]
   }

   return [format "%.0f" [expr ceil($ret)]]
}

