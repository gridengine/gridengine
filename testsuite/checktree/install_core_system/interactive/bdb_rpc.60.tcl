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
#****** install_core_system/install_bdb_rpc() ******
# 
#  NAME
#     install_bdb_rpc -- ??? 
#
#  SYNOPSIS
#     install_bdb_rpc { } 
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
proc install_bdb_rpc {} {
   global ts_config
   global CHECK_OUTPUT check_use_installed_system CHECK_ARCH
   global CHECK_COMMD_PORT CHECK_ADMIN_USER_SYSTEM CHECK_USER
   global CHECK_DEBUG_LEVEL CHECK_CORE_MASTER
   global CHECK_MAIN_RESULTS_DIR CHECK_SUBMIT_ONLY_HOSTS

   set CORE_INSTALLED "" 

   read_install_list

   set_error "0" "inst_sge -db - no errors"

   if { $ts_config(bdb_server) == "none" } {
      puts $CHECK_OUTPUT "there is no rpc server configured - returning"
      return
   }

   set bdb_host $ts_config(bdb_server)
   puts $CHECK_OUTPUT "installing BDB RPC Server on host $bdb_host ($ts_config(product_type) system) ..."
   if { $check_use_installed_system != 0 } {
      set_error "0" "install_bdb_rpc - no need to install BDB RPC Server on hosts \"$ts_config(bdb_server)\" - noinst parameter is set"
      puts "no need to install BDB RPC Server on hosts \"$ts_config(bdb_server)\", noinst parameter is set"
      if {[startup_bdb_rpc $bdb_host] == 0 } {
         lappend CORE_INSTALLED $bdb_host
         write_install_list
      } else {
         add_proc_error "install_bdb_rpc" -2 "could not startup BDB RPC Server on host $bdb_host"
      }
      return
   }

   if {[file isfile "$ts_config(product_root)/inst_sge"] != 1} {
      set_error "-1" "install_bdb_rpc - inst_sge file not found"
      return
   }

   set remote_arch [resolve_arch $bdb_host]    


   set ANSWER_YES                   [translate $bdb_host 0 1 0 [sge_macro DISTINST_ANSWER_YES] ]
   set ANSWER_NO                    [translate $bdb_host 0 1 0 [sge_macro DISTINST_ANSWER_NO] ]
   set RPC_HIT_RETURN_TO_CONTINUE   [translate $bdb_host 0 1 0 [sge_macro DISTINST_RPC_HIT_RETURN_TO_CONTINUE] ]
   set RPC_WELCOME                  [translate $bdb_host 0 1 0 [sge_macro DISTINST_RPC_WELCOME] ]
   set RPC_INSTALL_AS_ADMIN         [translate $bdb_host 0 1 0 [sge_macro DISTINST_RPC_INSTALL_AS_ADMIN] "*" ]
   set RPC_SGE_ROOT                 [translate $bdb_host 0 1 0 [sge_macro DISTINST_RPC_SGE_ROOT] "*" ]
   set RPC_SGE_CELL                 [translate $bdb_host 0 1 0 [sge_macro DISTINST_RPC_SGE_CELL] "*"]
   set RPC_SERVER                   [translate $bdb_host 0 1 0 [sge_macro DISTINST_RPC_SERVER] "*" ]
   set RPC_DIRECTORY                [translate $bdb_host 0 1 0 [sge_macro DISTINST_RPC_DIRECTORY] "*" ]
   set RPC_DIRECTORY_EXISTS         [translate $bdb_host 0 1 0 [sge_macro DISTINST_RPC_DIRECTORY_EXISTS] ]
   set RPC_START_SERVER             [translate $bdb_host 0 1 0 [sge_macro DISTINST_RPC_START_SERVER] ]
   set RPC_SERVER_STARTED           [translate $bdb_host 0 1 0 [sge_macro DISTINST_RPC_SERVER_STARTED] ]
   set RPC_INSTALL_RC_SCRIPT        [translate $bdb_host 0 1 0 [sge_macro DISTINST_RPC_INSTALL_RC_SCRIPT] ]
   set RPC_SERVER_COMPLETE          [translate $bdb_host 0 1 0 [sge_macro DISTINST_RPC_SERVER_COMPLETE] ]
   set HIT_RETURN_TO_CONTINUE       [translate $bdb_host 0 1 0 [sge_macro DISTINST_HIT_RETURN_TO_CONTINUE] ]
   set INSTALL_SCRIPT               [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_INSTALL_SCRIPT] "*" ]
   set DNS_DOMAIN_QUESTION          [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_DNS_DOMAIN_QUESTION] ]

   set prod_type_var "SGE_ROOT"

   set spooldir [get_bdb_spooldir $ts_config(bdb_server) 1]
   if {$spooldir == ""} {
      add_proc_error "install_bdb_rpc" -1 "no bdb_dir and no local spooldir for host $bdb_host configured"
      return
   }

   if {[file isfile "$ts_config(product_root)/$ts_config(cell)/common/sgebdb"] == 1} {
      puts $CHECK_OUTPUT "--> shutting down BDB RPC Server <--"
      start_remote_prog "$bdb_host" "root" "$ts_config(product_root)/$ts_config(cell)/common/sgebdb" "stop" prg_exit_state 60 0 "" 1 1 1 1 1
   }

   start_remote_prog "$bdb_host" "root" "rm" "-fR $spooldir" prg_exit_state 60 0 "" 1 0 1 1 1
   if { $CHECK_ADMIN_USER_SYSTEM == 0 } {
      set inst_user "root"
   } else {
      set inst_user $CHECK_USER
      puts $CHECK_OUTPUT "--> install as user $CHECK_USER <--" 
   }
   set id [open_remote_spawn_process $bdb_host $inst_user "cd $$prod_type_var;./inst_sge" "-db" 0 "" 0 15 1 1 1]

   log_user 1
   puts $CHECK_OUTPUT "cd $$prod_type_var;./inst_sge -db"

   set sp_id [ lindex $id 1 ] 


   set timeout 30
  
   set do_log_output 0 ;# 1 _LOG
   if { $CHECK_DEBUG_LEVEL == 2 } {
      set do_log_output 1
   }


   set do_stop 0
   while {$do_stop == 0} {
      flush stdout
      flush $CHECK_OUTPUT
      if {$do_log_output == 1} {
          puts "press RETURN"
          set anykey [wait_for_enter 1]
      }
  
      set timeout 300
      log_user 1 
      expect {
         -i $sp_id full_buffer {
            set_error "-1" "inst_sge -db - buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
            close_spawn_process $id; 
            return;
         }

         -i $sp_id eof {
            set_error "-1" "inst_sge -db - unexpeced eof";
            set do_stop 1
            continue
         }

         -i $sp_id "coredump" {
            set_error "-2" "inst_sge -db - coredump on host $bdb_host";
            set do_stop 1
            continue
         }

         -i $sp_id timeout { 
            set_error "-1" "inst_sge -db - timeout while waiting for output"; 
            set do_stop 1
            continue
         }

         -i $sp_id $RPC_HIT_RETURN_TO_CONTINUE { 
            puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
            if {$do_log_output == 1} {
                 puts "press RETURN"
                 set anykey [wait_for_enter 1]
            }
  
            send -i $sp_id "\n"
            continue
         }

         -i $sp_id $RPC_WELCOME { 
            puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
            if {$do_log_output == 1} {
                 puts "press RETURN"
                 set anykey [wait_for_enter 1]
            }
  
            send -i $sp_id "\n"
            continue
         }

         -i $sp_id $RPC_INSTALL_AS_ADMIN { 
            puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
            if {$do_log_output == 1} {
                 puts "press RETURN"
                 set anykey [wait_for_enter 1]
            }
  
            send -i $sp_id "\n"
            continue
         }

         -i $sp_id $RPC_SGE_ROOT {
            puts $CHECK_OUTPUT "\n -->testsuite: sending $ts_config(product_root)"
            set input "$ts_config(product_root)\n"

            if {$do_log_output == 1} {
               puts "-->testsuite: press RETURN"
               set anykey [wait_for_enter 1]
            }
            send -i $sp_id $input
            continue
         }

         -i $sp_id $RPC_SGE_CELL {
            puts $CHECK_OUTPUT "\n -->testsuite: sending $ts_config(cell)"
            set input "$ts_config(cell)\n"

            if {$do_log_output == 1} {
               puts "-->testsuite: press RETURN"
               set anykey [wait_for_enter 1]
            }
            send -i $sp_id $input
            continue
         }

         -i $sp_id $RPC_SERVER {
            puts $CHECK_OUTPUT "\n -->testsuite: sending $ts_config(bdb_server)"
            set input "$ts_config(bdb_server)\n"

            if {$do_log_output == 1} {
               puts "-->testsuite: press RETURN"
               set anykey [wait_for_enter 1]
            }
            send -i $sp_id $input
            continue
         } 

         -i $sp_id $DNS_DOMAIN_QUESTION { 
            puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_YES<(4)"
            if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
            }
            send -i $sp_id "$ANSWER_YES\n"
            continue
         }

         -i $sp_id $RPC_DIRECTORY {
            puts $CHECK_OUTPUT "\n -->testsuite: sending $spooldir"

            if {$do_log_output == 1} {
               puts "-->testsuite: press RETURN"
               set anykey [wait_for_enter 1]
            }

            send -i $sp_id "$spooldir\n"
            continue
         }

         -i $sp_id $RPC_DIRECTORY_EXISTS { 
            puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_YES<(12)"
            if {$do_log_output == 1} {
                 puts "press RETURN"
                 set anykey [wait_for_enter 1]
            }
  
            send -i $sp_id "$ANSWER_YES\n"
            continue
         }


         -i $sp_id $RPC_START_SERVER { 
            puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
            if {$do_log_output == 1} {
                 puts "press RETURN"
                 set anykey [wait_for_enter 1]
            }
  
            send -i $sp_id "\n"
            continue
         }

         -i $sp_id $RPC_SERVER_STARTED { 
            puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
            if {$do_log_output == 1} {
                 puts "press RETURN"
                 set anykey [wait_for_enter 1]
            }
  
            send -i $sp_id "\n"
            continue
         }

         -i $sp_id $RPC_INSTALL_RC_SCRIPT { 
            puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_NO<(12)"
            if {$do_log_output == 1} {
                 puts "press RETURN"
                 set anykey [wait_for_enter 1]
            }
  
            send -i $sp_id "$ANSWER_NO\n"
            continue
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

         -i $sp_id "Error:" {
            set_error "-1" "install_bdb_rpc - $expect_out(0,string)"
            close_spawn_process $id; 
            return
         }
         -i $sp_id "can't resolve hostname*\n" {
            set_error "-1" "install_bdb_rpc - $expect_out(0,string)"
            close_spawn_process $id; 
            return
         }            

         -i $sp_id "error:\n" {
            set_error "-1" "install_bdb_rpc - $expect_out(0,string)"
            close_spawn_process $id; 
            return
         }

         -i $sp_id $RPC_SERVER_COMPLETE {
            read_install_list
            lappend CORE_INSTALLED $bdb_host
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

         -i $sp_id default {
            set_error "-1" "inst_sge -db - undefined behaviour: $expect_out(buffer)"
            close_spawn_process $id; 
            return
         }
      }
   }

   # close the connection to inst_sge
   close_spawn_process $id
}

