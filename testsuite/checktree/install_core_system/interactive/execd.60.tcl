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

#                                                             max. column:     |
#****** install_core_system/install_execd() ******
# 
#  NAME
#     install_execd -- ??? 
#
#  SYNOPSIS
#     install_execd { } 
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
proc install_execd {} {
   global ts_config
   global CHECK_OUTPUT CORE_INSTALLED
   global check_use_installed_system CHECK_ARCH
   global CHECK_COMMD_PORT CHECK_ADMIN_USER_SYSTEM CHECK_USER
   global CHECK_DEBUG_LEVEL CHECK_EXECD_INSTALL_OPTIONS
   global CHECK_COMMD_PORT CHECK_CORE_MASTER
   global CHECK_MAIN_RESULTS_DIR CHECK_SUBMIT_ONLY_HOSTS

   set CORE_INSTALLED ""
   set INST_VERSION 0 
   set LOCAL_ALREADY_CHECKED 0 
 
   read_install_list

   set_error "0" "install_execd - no errors"

   # does cluster contain windows hosts?
   # if yes, we'll have to copy the certificates, regardless of csp mode or not
   set have_windows_host [host_conf_have_windows]

   set catch_result [ catch { eval exec "cat $ts_config(product_root)/inst_sge | grep \"SCRIPT_VERSION\" | cut -d\" -f2" } INST_VERSION ]
   puts $CHECK_OUTPUT "inst_sge version: $INST_VERSION"

   if {!$check_use_installed_system} {
      set feature_install_options ""
      foreach elem $CHECK_SUBMIT_ONLY_HOSTS {
         puts $CHECK_OUTPUT "do a qconf -as $elem ..."
         catch {  eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-as $elem" } result
         puts $CHECK_OUTPUT $result
      }
      if {$ts_config(product_feature) == "csp" || $have_windows_host} {
         set feature_install_options "-csp"
         set my_csp_host_list $ts_config(execd_nodes)
         foreach elem $CHECK_SUBMIT_ONLY_HOSTS {
           lappend my_csp_host_list $elem
         }
         foreach exec_host $my_csp_host_list {
            if { $exec_host == $CHECK_CORE_MASTER } {
               continue
            }
            copy_certificates $exec_host
         }
      }
   }
 
   foreach exec_host $ts_config(execd_nodes) {

      puts $CHECK_OUTPUT "installing execd on host $exec_host ($ts_config(product_type) system) ..."
      if {[lsearch $ts_config(execd_nodes) $exec_host] == -1 } {
         set_error "-1" "install_execd - host $exec_host is not in execd list"
         return 
      }
#      wait_for_remote_file $exec_host $CHECK_USER "$ts_config(product_root)/$ts_config(cell)/common/configuration"
      if { $check_use_installed_system != 0 } {
         set_error "0" "install_execd - no need to install execd on hosts \"$ts_config(execd_nodes)\" - noinst parameter is set"
         puts "no need to install execd on hosts \"$ts_config(execd_nodes)\", noinst parameter is set"
         if {[startup_execd $exec_host] == 0 } {
            lappend CORE_INSTALLED $exec_host
            write_install_list
            continue
         } else {
            add_proc_error "install_execd" -2 "could not startup execd on host $exec_host"
            return
         }
      }

      if {[file isfile "$ts_config(product_root)/install_execd"] != 1} {
         set_error "-1" "install_execd - install_execd file not found"
         return
      }

      set remote_arch [resolve_arch $exec_host]    
      set sensor_file [ get_loadsensor_path $exec_host ]
      if { [string compare $sensor_file ""] != 0  } {
         puts $CHECK_OUTPUT "installing load sensor:"
         puts $CHECK_OUTPUT "======================="
         puts $CHECK_OUTPUT "architecture: $remote_arch"
         puts $CHECK_OUTPUT "sensor file:  $sensor_file"
         puts $CHECK_OUTPUT "target:       $ts_config(product_root)/bin/$remote_arch/qloadsensor"
         if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
            set arguments "$sensor_file $ts_config(product_root)/bin/$remote_arch/qloadsensor"
            set result [start_remote_prog $ts_config(master_host) "root" "cp" "$arguments" prg_exit_state 60 0 "" 1 1 1 1 1] 
            puts $CHECK_OUTPUT "result: $result"
            puts $CHECK_OUTPUT "copy exit state: $prg_exit_state" 
         } else {
            puts $CHECK_OUTPUT "can not copy this file as user $CHECK_USER"
            puts $CHECK_OUTPUT "please copy this file manually!!"
            puts $CHECK_OUTPUT "if not, you will get no load values from this host (=$exec_host)"
            puts $CHECK_OUTPUT "installation will continue in 15 seconds!!"
            sleep 15
         }
      }

      set HIT_RETURN_TO_CONTINUE       [translate $exec_host 0 1 0 [sge_macro DISTINST_HIT_RETURN_TO_CONTINUE] ]
      set EXECD_INSTALL_COMPLETE       [translate $exec_host 0 1 0 [sge_macro DISTINST_EXECD_INSTALL_COMPLETE] ]
      set PREVIOUS_SCREEN              [translate $exec_host 0 1 0 [sge_macro DISTINST_PREVIOUS_SCREEN ] ]
      set CELL_NAME_FOR_EXECD          [translate $exec_host 0 1 0 [sge_macro DISTINST_CELL_NAME_FOR_EXECD ] "*"]
      set ANSWER_YES                   [translate $exec_host 0 1 0 [sge_macro DISTINST_ANSWER_YES] ]
      set ANSWER_NO                    [translate $exec_host 0 1 0 [sge_macro DISTINST_ANSWER_NO] ]
      set ADD_DEFAULT_QUEUE_INSTANCE   [translate $exec_host 0 1 0 [sge_macro DISTINST_ADD_DEFAULT_QUEUE_INSTANCE] ]
      set INSTALL_SCRIPT               [translate $exec_host 0 1 0 [sge_macro DISTINST_INSTALL_SCRIPT] "*" ]
      set IF_NOT_OK_STOP_INSTALLATION  [translate $exec_host 0 1 0 [sge_macro DISTINST_IF_NOT_OK_STOP_INSTALLATION] ]
      set LOCAL_CONFIG_FOR_HOST        [translate $exec_host 0 1 0 [sge_macro DISTINST_LOCAL_CONFIG_FOR_HOST] "$exec_host"]
      set MESSAGES_LOGGING             [translate $exec_host 0 1 0 [sge_macro DISTINST_MESSAGES_LOGGING] ]
      set USE_CONFIGURATION_PARAMS     [translate $exec_host 0 1 0 [sge_macro DISTINST_USE_CONFIGURATION_PARAMS] ]
      set CURRENT_GRID_ROOT_DIRECTORY  [translate $exec_host 0 1 0 [sge_macro DISTINST_CURRENT_GRID_ROOT_DIRECTORY] "*" "*" ]
      set CHECK_ADMINUSER_ACCOUNT      [translate $exec_host 0 1 0 [sge_macro DISTINST_CHECK_ADMINUSER_ACCOUNT] "*" "*" "*" "*" ]
      set CHECK_ADMINUSER_ACCOUNT_ANSWER      [translate $exec_host 0 1 0 [sge_macro DISTINST_CHECK_ADMINUSER_ACCOUNT_ANSWER] ]
      set INSTALL_STARTUP_SCRIPT       [translate $exec_host 0 1 0 [sge_macro DISTINST_INSTALL_STARTUP_SCRIPT] ]
      set ENTER_LOCAL_EXECD_SPOOL_DIR  [translate $exec_host 0 1 0 [sge_macro DISTINST_ENTER_LOCAL_EXECD_SPOOL_DIR] ]
      set ENTER_LOCAL_EXECD_SPOOL_DIR_ASK [translate $exec_host 0 1 0 [sge_macro DISTINST_ENTER_LOCAL_EXECD_SPOOL_DIR_ASK] ]
      set ENTER_LOCAL_EXECD_SPOOL_DIR_ENTER [translate $exec_host 0 1 0 [sge_macro DISTINST_ENTER_LOCAL_EXECD_SPOOL_DIR_ENTER] ]
      set HOSTNAME_KNOWN_AT_MASTER [translate $exec_host 0 1 0 [sge_macro DISTINST_HOSTNAME_KNOWN_AT_MASTER] ]

      # windows
      set WINDOWS_HELPER_SERVICE       [translate_macro DISTINST_EXECD_WINDOWS_HELPER_SERVICE]
      
      cd "$ts_config(product_root)"

      set prod_type_var "SGE_ROOT"
  
      if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
         set id [open_remote_spawn_process "$exec_host" "root"  "cd $$prod_type_var;./install_execd" "$CHECK_EXECD_INSTALL_OPTIONS $feature_install_options" 0 "" 1 15 1 1 1]
      } else {
         puts $CHECK_OUTPUT "--> install as user $CHECK_USER <--" 
         set id [open_remote_spawn_process "$exec_host" "$CHECK_USER"  "cd $$prod_type_var;./install_execd" "$CHECK_EXECD_INSTALL_OPTIONS $feature_install_options" 0 "" 1 15 1 1 1]
      }

      log_user 1
      puts $CHECK_OUTPUT "cd $$prod_type_var;./install_execd $CHECK_EXECD_INSTALL_OPTIONS $feature_install_options"

      set sp_id [ lindex $id 1 ] 

      set timeout 300
     
      set do_log_output 0 ;# 1 _LOG
      if { $CHECK_DEBUG_LEVEL == 2 } {
         set do_log_output 1
      }

      set do_stop 0
      while {$do_stop == 0} {
         flush stdout
         flush $CHECK_OUTPUT
         if {$do_log_output == 1} {
             puts "-->testsuite: press RETURN (main) or enter \"break\" to stop"
             set anykey [wait_for_enter 1]
             if { [string match "*break*" $anykey] } {
                break  
             }
         }
     
         set timeout 600
         log_user 1 
         expect {
            -i $sp_id full_buffer {
               set_error "-1" "install_execd - buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
               close_spawn_process $id
               return
            }

            -i $sp_id eof {
               set_error "-1" "install_execd - unexpeced eof"
               set do_stop 1
               continue
            }

            -i $sp_id "coredump" {
               set_error "-2" "install_execd - coredump on host $exec_host"
               set do_stop 1
               continue
            }

            -i $sp_id timeout { 
               set_error "-1" "install_execd - timeout while waiting for output"
               set do_stop 1
               continue
            }

            -i $sp_id "orry" { 
               set_error "-1" "install_execd - wrong root password"
               close_spawn_process $id
               return
            }

            -i $sp_id "The installation of the execution daemon will abort now" {
               set_error "-1" "install_execd - installation error"
               close_spawn_process $id
               return
            }

            -i $sp_id $USE_CONFIGURATION_PARAMS { 
               puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_YES<(10)"
               if {$do_log_output == 1} {
                    puts "press RETURN"
                    set anykey [wait_for_enter 1]
               }
               send -i $sp_id "$ANSWER_YES\n"
               continue
            }

            -i $sp_id $ENTER_LOCAL_EXECD_SPOOL_DIR_ASK {
               # If we said yes to the question whether we want to configure a local
               # spooldir, but enter an empty directory path here, inst_sge has
               # to handle this situation.
               # Beginning with INST_VERSION 4, this situation is handled correctly.
               # To thest the correct error handling, we send yes here and later on
               # (ENTER_LOCAL_EXECD_SPOOL_DIR_ENTER) we send \"\" as spooldir.
               # inst_sge has to detect the incorrect input and repeat this question.
               if { $INST_VERSION >= 4 && $LOCAL_ALREADY_CHECKED == 0 } {
                  puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_YES<(11.1)"
                  if {$do_log_output == 1} {
                     puts "press RETURN"
                     set anykey [wait_for_enter 1]
                  }
                  send -i $sp_id "$ANSWER_YES\n"
               } else {
                  set spooldir [get_local_spool_dir $exec_host execd]
                  if { $spooldir == "" } {
                     puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_NO<(11.2)"
                     if {$do_log_output == 1} {
                          puts "press RETURN"
                          set anykey [wait_for_enter 1]
                     }
                     send -i $sp_id "$ANSWER_NO\n"
                  } else {
                     puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_YES<(11.3)"
                     if {$do_log_output == 1} {
                          puts "press RETURN"
                          set anykey [wait_for_enter 1]
                     }
                     send -i $sp_id "$ANSWER_YES\n"
                  }
               }
               continue
            }   

            -i $sp_id $ENTER_LOCAL_EXECD_SPOOL_DIR_ENTER {
               puts $CHECK_OUTPUT "\n -->testsuite: send local spool directory\n"

               # Second part of inst_sge error handling test (ENTER_LOCAL_EXECD_SPOOL_DIR_ASK):
               # Sending \"\" as spooldir
               if { $INST_VERSION >= 4 && $LOCAL_ALREADY_CHECKED == 0 } {
                  set LOCAL_ALREADY_CHECKED 1
                  set spooldir ""
                  puts $CHECK_OUTPUT "checking inst_sge error handling, sending \"\" as local spooldir"
               } else {
                  set spooldir [get_local_spool_dir $exec_host execd 0]
                  puts $CHECK_OUTPUT "spooldir on host $exec_host is $spooldir"
               }

               if {$do_log_output == 1} {
                    puts "press RETURN"
                    set anykey [wait_for_enter 1]
               }

               send -i $sp_id "$spooldir\n"
               log_user 1
               continue
            }

            -i $sp_id $CELL_NAME_FOR_EXECD {
               puts $CHECK_OUTPUT "\n -->testsuite: sending $ts_config(cell)"
               set input "$ts_config(cell)\n"

               if {$do_log_output == 1} {
                  puts "-->testsuite: press RETURN"
                  set anykey [wait_for_enter 1]
               }
               send -i $sp_id $input
               continue
            } 

            -i $sp_id $MESSAGES_LOGGING {
               puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
               if {$do_log_output == 1} {
                   puts "press RETURN"
                   set anykey [wait_for_enter 1]
               }
               send -i $sp_id "\n"
               continue
            }



            -i $sp_id $IF_NOT_OK_STOP_INSTALLATION {
               if { $CHECK_ADMIN_USER_SYSTEM != 0 } {
                  puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
                  if {$do_log_output == 1} {
                       puts "press RETURN"
                       set anykey [wait_for_enter 1]
                  }
                  send -i $sp_id "\n"
                  continue
               } else {
                  set_error "-1" "install_execd - host $exec_host: tried to install not as root"
                  close_spawn_process $id
                  return
               }
            }

            -i $sp_id $INSTALL_SCRIPT { 
               puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_NO<(12)"
               if {$do_log_output == 1} {
                    puts "press RETURN"
                    set anykey [wait_for_enter 1]
               }
     
               send -i $sp_id "$ANSWER_NO\n"
               continue
            }

            -i $sp_id $INSTALL_STARTUP_SCRIPT { 
               puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_NO<(12)"
               if {$do_log_output == 1} {
                    puts "press RETURN"
                    set anykey [wait_for_enter 1]
               }
     
               send -i $sp_id "$ANSWER_NO\n"
               continue
            }

            -i $sp_id $ADD_DEFAULT_QUEUE_INSTANCE { 
               puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_YES<(13)"
               if {$do_log_output == 1} {
                    puts "(5)press RETURN"
                    set anykey [wait_for_enter 1]
               }
     
               send -i $sp_id "$ANSWER_YES\n"
               continue
            }

            -i $sp_id $CHECK_ADMINUSER_ACCOUNT_ANSWER { 
               puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_YES<(13)"
               if {$do_log_output == 1} {
                    puts "(5)press RETURN"
                    set anykey [wait_for_enter 1]
               }
     
               send -i $sp_id "$ANSWER_YES\n"
               continue
            }


            -i $sp_id "This host is unknown on the qmaster host" {
               puts $CHECK_OUTPUT "\nHostname resolving problem"
               puts $CHECK_OUTPUT "*********************************************************************"
               puts $CHECK_OUTPUT "Hostname resolving problem - use a host alias file for host $exec_host" 
               puts $CHECK_OUTPUT "**********************************************************************"
               puts $CHECK_OUTPUT "installation will continue in 15 seconds ..."
               sleep 15
               continue
            }

            -i $sp_id "There is still no service for" {
               set_error "-1" "install_execd - no TCP/IP service available"
               set do_stop 1
               continue
            }

            -i $sp_id "Check again" { 
               puts $CHECK_OUTPUT "\n -->testsuite: sending >n<(13)"
               if {$do_log_output == 1} {
                    puts "press RETURN"
                    set anykey [wait_for_enter 1]
               }
     
               send -i $sp_id "n\n"
               continue
            }

            -i $sp_id $PREVIOUS_SCREEN { 
               puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_NO<(14)"
               if {$do_log_output == 1} {
                    puts "press RETURN"
                    set anykey [wait_for_enter 1]
               }
     
               send -i $sp_id "$ANSWER_NO\n"
               continue
            }

            -i $sp_id "Error:" {
               set_error "-1" "install_execd - $expect_out(0,string)"
               close_spawn_process $id
               return
            }
            -i $sp_id "can't resolve hostname*\n" {
               set_error "-1" "install_execd - $expect_out(0,string)"
               close_spawn_process $id
               return
            }            
  
            -i $sp_id "error:\n" {
               set_error "-1" "install_execd - $expect_out(0,string)"
               close_spawn_process $id
               return
            }

            -i $sp_id $CURRENT_GRID_ROOT_DIRECTORY {
               puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
               if {$do_log_output == 1} {
                    puts "-->testsuite: press RETURN"
                    set anykey [wait_for_enter 1]
               }
               send -i $sp_id "\n"
               continue
            }

            -i $sp_id $EXECD_INSTALL_COMPLETE {
               read_install_list
               lappend CORE_INSTALLED $exec_host
               write_install_list
               set do_stop 1
               # If we compiled with code coverage, we have to 
               # wait a little bit before closing the connection.
               # Otherwise the last command executed (infotext)
               # will leave a lockfile lying around.
               if {[coverage_enabled]} {
                  sleep 2
               }
               continue
            }

            -i $sp_id $HIT_RETURN_TO_CONTINUE { 
               puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
               if {$do_log_output == 1} {
                    puts "press RETURN"
                    set anykey [wait_for_enter 1]
               }
     
               send -i $sp_id "\n"
               continue
            }
            -i $sp_id $HOSTNAME_KNOWN_AT_MASTER { 
               puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
               if {$do_log_output == 1} {
                    puts "press RETURN"
                    set anykey [wait_for_enter 1]
               }
     
               send -i $sp_id "\n"
               continue
            }

            -i $sp_id $WINDOWS_HELPER_SERVICE {
               puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<(4)"
               if {$do_log_output == 1} {
                    puts "press RETURN"
                    set anykey [wait_for_enter 1]
               }
               send -i $sp_id "\n"
               continue
            }

            -i $sp_id default {
               set_error "-1" "install_execd - undefined behaiviour: $expect_out(buffer)"
               close_spawn_process $id
               return
            }
         }
      }  ;# while 1

      # close the connection to inst_sge
      close_spawn_process $id
   }
}


