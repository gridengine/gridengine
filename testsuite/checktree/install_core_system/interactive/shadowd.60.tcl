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
#****** install_core_system/install_shadowd() ******
# 
#  NAME
#     install_shadowd -- ??? 
#
#  SYNOPSIS
#     install_shadowd { } 
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
proc install_shadowd {} {
   global ts_config
   global CHECK_OUTPUT CHECK_CORE_SHADOWD CORE_INSTALLED
   global check_use_installed_system CHECK_ARCH
   global CHECK_COMMD_PORT CHECK_ADMIN_USER_SYSTEM CHECK_USER
   global CHECK_DEBUG_LEVEL CHECK_EXECD_INSTALL_OPTIONS
   global CHECK_COMMD_PORT CHECK_CORE_MASTER
   global CHECK_MAIN_RESULTS_DIR CHECK_SUBMIT_ONLY_HOSTS

   set CORE_INSTALLED "" 
   read_install_list

   set_error "0" "inst_sge -sm - no errors"

   if {! $check_use_installed_system} {
      set feature_install_options ""

      if { $ts_config(product_feature) == "csp" } {
         set feature_install_options "-csp"
         set my_csp_host_list $CHECK_CORE_SHADOWD
         foreach shadow_host $my_csp_host_list {
            if { $shadow_host == $CHECK_CORE_MASTER } {
               continue
            }
            copy_certificates $shadow_host
         }
      }
   }
 
   foreach shadow_host $CHECK_CORE_SHADOWD {

      puts $CHECK_OUTPUT "installing shadowd on host $shadow_host ($ts_config(product_type) system) ..."
      if {[lsearch $CHECK_CORE_SHADOWD $shadow_host] == -1 } {
         set_error "-1" "inst_sge -sm - host $shadow_host is not in shadowd list"
         return 
      }
#      wait_for_remote_file $shadow_host $CHECK_USER "$ts_config(product_root)/$ts_config(cell)/common/configuration"
      if { $check_use_installed_system != 0 } {
         set_error "0" "install_shadowd - no need to install shadowd on hosts \"$CHECK_CORE_SHADOWD\" - noinst parameter is set"
         puts "no need to install shadowd on hosts \"$CHECK_CORE_SHADOWD\", noinst parameter is set"
         if {[startup_shadowd $shadow_host] == 0 } {
            lappend CORE_INSTALLED $shadow_host
            write_install_list
            continue
         } else {
            add_proc_error "install_shadowd" -2 "could not startup shadowd on host $shadow_host"
            return
         }
      }

      if {[file isfile "$ts_config(product_root)/inst_sge"] != 1} {
         set_error "-1" "install_shadowd - inst_sge file not found"
         return
      }

      set remote_arch [resolve_arch $shadow_host]    
 

      set HIT_RETURN_TO_CONTINUE       [translate $shadow_host 0 1 0 [sge_macro DISTINST_HIT_RETURN_TO_CONTINUE] ]
      set SHADOWD_INSTALL_COMPLETE       [translate $shadow_host 0 1 0 [sge_macro DISTINST_SHADOWD_INSTALL_COMPLETE] ]
      set ANSWER_YES                   [translate $shadow_host 0 1 0 [sge_macro DISTINST_ANSWER_YES] ]
      set ANSWER_NO                    [translate $shadow_host 0 1 0 [sge_macro DISTINST_ANSWER_NO] ]
      set INSTALL_SCRIPT               [translate $shadow_host 0 1 0 [sge_macro DISTINST_INSTALL_SCRIPT] "*" ]
      set IF_NOT_OK_STOP_INSTALLATION  [translate $shadow_host 0 1 0 [sge_macro DISTINST_IF_NOT_OK_STOP_INSTALLATION] ]
      set MESSAGES_LOGGING             [translate $shadow_host 0 1 0 [sge_macro DISTINST_MESSAGES_LOGGING] ]
      set CURRENT_GRID_ROOT_DIRECTORY  [translate $shadow_host 0 1 0 [sge_macro DISTINST_CURRENT_GRID_ROOT_DIRECTORY] "*" "*" ]
      set CHECK_ADMINUSER_ACCOUNT      [translate $shadow_host 0 1 0 [sge_macro DISTINST_CHECK_ADMINUSER_ACCOUNT] "*" "*" "*" "*" ]
      set CHECK_ADMINUSER_ACCOUNT_ANSWER      [translate $shadow_host 0 1 0 [sge_macro DISTINST_CHECK_ADMINUSER_ACCOUNT_ANSWER] ]
      set SHADOW_INFO                  [translate $shadow_host 0 1 0 [sge_macro DISTINST_SHADOW_INFO] ]
      set SHADOW_ROOT                  [translate $shadow_host 0 1 0 [sge_macro DISTINST_SHADOW_ROOT] "*" ]
      set SHADOW_CELL                  [translate $shadow_host 0 1 0 [sge_macro DISTINST_SHADOW_CELL] ]
      set HOSTNAME_KNOWN_AT_MASTER     [translate $shadow_host 0 1 0 [sge_macro DISTINST_HOSTNAME_KNOWN_AT_MASTER] ]
      set OTHER_USER_ID_THAN_ROOT      [translate $shadow_host 0 1 0 [sge_macro DISTINST_OTHER_USER_ID_THAN_ROOT] ]
      set INSTALL_AS_ADMIN_USER        [translate $shadow_host 0 1 0 [sge_macro DISTINST_INSTALL_AS_ADMIN_USER] "$CHECK_USER" ]


      cd "$ts_config(product_root)"

      set prod_type_var "SGE_ROOT"
  
      if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
         set id [open_remote_spawn_process "$shadow_host" "root"  "cd $$prod_type_var;./inst_sge" "-sm" 0 "" 1 15 1 1 1]
      } else {
         puts $CHECK_OUTPUT "--> install as user $CHECK_USER <--" 
         set id [open_remote_spawn_process "$shadow_host" "$CHECK_USER"  "cd $$prod_type_var;./inst_sge" "-sm" 0 "" 1 15 1 1 1]
      }


      log_user 1
      puts $CHECK_OUTPUT "cd $$prod_type_var;./inst_sge -sm"

      set sp_id [ lindex $id 1 ] 


      set timeout 30
     
      set do_log_output 0 ;# 1 _LOG
      if { $CHECK_DEBUG_LEVEL == 2 } {
         set do_log_output 1
      }


      set do_stop 0
      while {$do_stop == 0} {
         flush $CHECK_OUTPUT
         if {$do_log_output == 1} {
             puts "press RETURN"
             set anykey [wait_for_enter 1]
         }
     
         set timeout 300
         log_user 1 
         expect {
            -i $sp_id full_buffer {
               set_error "-1" "install_shadowd - buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
               close_spawn_process $id
               return
            }

            -i $sp_id eof {
               set_error "-1" "install_shadowd - unexpeced eof"
               set do_stop 1
               continue
            }

            -i $sp_id "coredump" {
               set_error "-2" "install_shadowd - coredump on host $shadow_host"
               set do_stop 1
               continue
            }

            -i $sp_id timeout { 
               set_error "-1" "install_shadowd - timeout while waiting for output" 
               set do_stop 1
               continue
            }


            -i $sp_id $SHADOW_CELL {
               puts $CHECK_OUTPUT "\n -->testsuite: sending $ts_config(cell)"
               set input "$ts_config(cell)\n"

               if {$do_log_output == 1} {
                  puts "-->testsuite: press RETURN"
                  set anykey [wait_for_enter 1]
               }
               send -i $sp_id $input
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

             -i $sp_id $INSTALL_AS_ADMIN_USER { 
               puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_YES<(5)"
               if {$do_log_output == 1} {
                  puts "press RETURN"
                  set anykey [wait_for_enter 1]
               }

               send -i $sp_id "$ANSWER_YES\n"
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
                  set_error "-1" "install_shadowd - host $shadow_host: tried to install not as root"
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


            -i $sp_id "Error:" {
               set_error "-1" "install_shadowd - $expect_out(0,string)"
               close_spawn_process $id 
               return
            }
            -i $sp_id "can't resolve hostname*\n" {
               set_error "-1" "install_shadowd - $expect_out(0,string)"
               close_spawn_process $id 
               return
            }            
  
            -i $sp_id "error:\n" {
               set_error "-1" "install_shadowd - $expect_out(0,string)"
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

            -i $sp_id $SHADOWD_INSTALL_COMPLETE {
               read_install_list
               lappend CORE_INSTALLED $shadow_host
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

            -i $sp_id $SHADOW_ROOT { 
               puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
               if {$do_log_output == 1} {
                    puts "press RETURN"
                    set anykey [wait_for_enter 1]
               }
     
               send -i $sp_id "\n"
               continue
            }

            -i $sp_id default {
               set_error "-1" "install_shadowd - undefined behaiviour: $expect_out(buffer)"
               close_spawn_process $id 
               return
            }
         }
      }

      # close connection to inst_sge
      close_spawn_process $id
   }
}


